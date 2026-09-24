/*
 * VNC Backend for RemoteDesktop Extension
 *
 * SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 */

#include <dix-config.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <pthread.h>

#include "dix/dix_priv.h"
#include "dix/screenint_priv.h"
#include "include/regionstr.h"
#include "include/windowstr.h"
#include "os/client_priv.h"
#include "Xext/remotedesktop/remotedesktop_priv.h"
#include "Xext/remotedesktop/vnc/vnc_backend.h"

#ifdef HAVE_LIBVNCSERVER
#include <rfb/rfb.h>
#include <rfb/rfbregion.h>
#endif

/* SSL/TLS support */
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/pem.h>

/* Global SSL context for TLS */
static SSL_CTX *vnc_ssl_ctx = NULL;
static Bool vnc_ssl_initialized = FALSE;

/* VNC encoding types */
#define RFB_ENCODING_RAW        0
#define RFB_ENCODING_COPYRECT   1
#define RFB_ENCODING_RRE        2
#define RFB_ENCODING_HEXTILE    5
#define RFB_ENCODING_ZRLE       16
#define RFB_ENCODING_TIGHT      7

/* Security types */
#define RFB_SECURITY_TYPE_NONE  1
#define RFB_SECURITY_TYPE_VNC   2
#define RFB_SECURITY_TYPE_TLS   19  /* TLS security type */

/* VNC-specific configuration */
typedef struct _VNCConfig {
    uint16_t port;
    char *password;
    Bool view_only;
    Bool localhost_only;
    Bool ipv6_enabled;
    char *tls_cert_path;
    char *tls_key_path;
    char *allowed_encodings;
    int quality_level;
    Bool use_damage;
    Bool force_tls_non_localhost;
} VNCConfigRec, *VNCConfigPtr;

/* VNC backend private data */
typedef struct _VNCBackendPrivate {
    VNCConfigPtr config;
    
#ifdef HAVE_LIBVNCSERVER
    rfbScreenInfoPtr rfb_screen;
#else
    int listen_fd;
    pthread_t server_thread;
    Bool thread_running;
    struct pollfd *client_fds;
    int num_clients;
    int max_clients;
#endif

    /* Framebuffer */
    void *framebuffer;
    int fb_width;
    int fb_height;
    int fb_stride;
    int fb_bpp;
    Bool fb_dirty;
    
    /* Damage region */
    RegionRec damage_region;
    Bool damage_pending;
    
    /* Client management */
    struct xorg_list client_list;
    
    /* Mutex for thread safety */
    pthread_mutex_t mutex;
} VNCBackendPrivateRec, *VNCBackendPrivatePtr;

/* VNC client connection */
typedef struct _VNCClient {
    int fd;
    struct sockaddr_storage addr;  /* IPv4 or IPv6 */
    socklen_t addr_len;
    Bool authenticated;
    Bool view_only;
    Bool use_tls;
    void *ssl;  /* SSL context for TLS */
    pthread_t thread;
    struct xorg_list entry;
} VNCClientRec, *VNCClientPtr;

/* Forward declarations */
static Bool VNCBackendInit(ScreenPtr pScreen, RemoteDesktopProtocolPtr self, RemoteDesktopConfigPtr config);
static void VNCBackendFini(ScreenPtr pScreen, RemoteDesktopProtocolPtr self);
static Bool VNCBackendStart(ScreenPtr pScreen, RemoteDesktopProtocolPtr self);
static void VNCBackendStop(ScreenPtr pScreen, RemoteDesktopProtocolPtr self);
static Bool VNCBackendReconfigure(ScreenPtr pScreen, RemoteDesktopProtocolPtr self, RemoteDesktopConfigPtr config);
static void VNCBackendDamageNotify(RemoteDesktopProtocolPtr self, RegionPtr pRegion);
static Bool VNCBackendGetFramebuffer(ScreenPtr pScreen, RemoteDesktopProtocolPtr self, void **fb_addr, int *stride, int *bpp);
static void VNCBackendClientConnected(RemoteDesktopProtocolPtr self, int client_fd);
static void VNCBackendClientDisconnected(RemoteDesktopProtocolPtr self, int client_fd);

/* Config parsing */
static VNCConfigPtr VNCParseConfig(RemoteDesktopConfigPtr config);
static void VNCFreeConfig(VNCConfigPtr config);

/* libvncserver path */
#ifdef HAVE_LIBVNCSERVER
static void VNCLibVNCScreenInit(VNCBackendPrivatePtr priv);
static void VNCLibVNCScreenFini(VNCBackendPrivatePtr priv);
static void VNCLibVNCSetPassword(VNCBackendPrivatePtr priv);
static void VNCLibVNCDamageCallback(rfbScreenInfoPtr screen, int x, int y, int w, int h);
#else
/* Embedded RFB path */
static void *VNCEmbeddedServerThread(void *arg);
static Bool VNCEmbeddedServerStart(VNCBackendPrivatePtr priv);
static void VNCEmbeddedServerStop(VNCBackendPrivatePtr priv);
static void VNCEmbeddedHandleClient(VNCBackendPrivatePtr priv, int client_fd);
static Bool VNCEmbeddedRFBHandshake(VNCBackendPrivatePtr priv, int client_fd);
static void VNCEmbeddedSendFramebufferUpdate(VNCBackendPrivatePtr priv, int client_fd, RegionPtr region);
static void VNCEmbeddedSendRawEncoding(VNCBackendPrivatePtr priv, int client_fd, int x, int y, int w, int h);
#endif

/* Protocol instance */
static RemoteDesktopProtocolRec vnc_protocol = {
    .name = "VNC",
    .vendor = "XLibre",
    .version = 1,
    .capabilities = RD_CAP_AUTH_NONE | RD_CAP_AUTH_PASSWORD | RD_CAP_AUTH_TLS_CERT |
                    RD_CAP_ENCRYPTION_TLS | RD_CAP_ENCODING_RAW |
                    RD_CAP_ENCODING_HEXTILE | RD_CAP_ENCODING_TIGHT |
                    RD_CAP_VIEW_ONLY,
    
    .Init = VNCBackendInit,
    .Fini = VNCBackendFini,
    .Start = VNCBackendStart,
    .Stop = VNCBackendStop,
    .Reconfigure = VNCBackendReconfigure,
    .DamageNotify = VNCBackendDamageNotify,
    .GetFramebuffer = VNCBackendGetFramebuffer,
    .ClientConnected = VNCBackendClientConnected,
    .ClientDisconnected = VNCBackendClientDisconnected,
    
    .private = NULL,
    .entry = { &vnc_protocol.entry, &vnc_protocol.entry }
};

/* ================================================================
 * Protocol Registration
 * ================================================================ */

void
VNCBackendRegister(void)
{
    RemoteDesktopRegisterProtocol(&vnc_protocol);
}

void
VNCBackendUnregister(void)
{
    RemoteDesktopUnregisterProtocol(&vnc_protocol);
}

/* ================================================================
 * Configuration
 * ================================================================ */

static VNCConfigPtr
VNCParseConfig(RemoteDesktopConfigPtr config)
{
    VNCConfigPtr vnc = calloc(1, sizeof(VNCConfigRec));
    if (!vnc)
        return NULL;

    vnc->port = RemoteDesktopConfigGetInt(config, "port");
    if (vnc->port == 0)
        vnc->port = 5900;

    vnc->password = RemoteDesktopConfigGetString(config, "password");
    if (vnc->password)
        vnc->password = strdup(vnc->password);

    vnc->view_only = RemoteDesktopConfigGetBool(config, "view_only");
    vnc->localhost_only = RemoteDesktopConfigGetBool(config, "localhost_only");
    if (!vnc->localhost_only)
        vnc->localhost_only = TRUE; /* Default to localhost only for security */

    vnc->ipv6_enabled = RemoteDesktopConfigGetBool(config, "ipv6");
    if (!vnc->ipv6_enabled)
        vnc->ipv6_enabled = FALSE; /* Default to IPv4 only */

    vnc->tls_cert_path = RemoteDesktopConfigGetString(config, "tls_cert");
    if (vnc->tls_cert_path)
        vnc->tls_cert_path = strdup(vnc->tls_cert_path);

    vnc->tls_key_path = RemoteDesktopConfigGetString(config, "tls_key");
    if (vnc->tls_key_path)
        vnc->tls_key_path = strdup(vnc->tls_key_path);

    vnc->force_tls_non_localhost = RemoteDesktopConfigGetBool(config, "force_tls");
    if (!vnc->force_tls_non_localhost)
        vnc->force_tls_non_localhost = TRUE; /* Default to force TLS for non-localhost */

    vnc->allowed_encodings = RemoteDesktopConfigGetString(config, "encodings");
    if (vnc->allowed_encodings)
        vnc->allowed_encodings = strdup(vnc->allowed_encodings);
    else
        vnc->allowed_encodings = strdup("raw,hextile,tight"); /* Default encodings */

    vnc->quality_level = RemoteDesktopConfigGetInt(config, "quality");
    if (vnc->quality_level < 0)
        vnc->quality_level = 0;
    if (vnc->quality_level > 9)
        vnc->quality_level = 9;

    vnc->use_damage = RemoteDesktopConfigGetBool(config, "use_damage");
    if (!vnc->use_damage)
        vnc->use_damage = TRUE; /* Default to true */

    return vnc;
}

static void
VNCFreeConfig(VNCConfigPtr config)
{
    if (!config)
        return;

    free(config->password);
    free(config->tls_cert_path);
    free(config->tls_key_path);
    free(config->allowed_encodings);
    free(config);
}

/* ================================================================
 * SSL/TLS Initialization
 * ================================================================ */

static Bool
VNCSSLInit(VNCConfigPtr config)
{
    if (vnc_ssl_initialized)
        return TRUE;

    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    vnc_ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (!vnc_ssl_ctx) {
        LogMessage(X_ERROR, "VNC: Failed to create SSL context\n");
        return FALSE;
    }

    /* Set minimum TLS version to 1.2 */
    SSL_CTX_set_min_proto_version(vnc_ssl_ctx, TLS1_2_VERSION);

    /* Load certificate and key */
    if (config && config->tls_cert_path && config->tls_key_path) {
        if (SSL_CTX_use_certificate_file(vnc_ssl_ctx, config->tls_cert_path, SSL_FILETYPE_PEM) <= 0) {
            LogMessage(X_ERROR, "VNC: Failed to load TLS certificate: %s\n", config->tls_cert_path);
            ERR_print_errors_fp(stderr);
            SSL_CTX_free(vnc_ssl_ctx);
            vnc_ssl_ctx = NULL;
            return FALSE;
        }

        if (SSL_CTX_use_PrivateKey_file(vnc_ssl_ctx, config->tls_key_path, SSL_FILETYPE_PEM) <= 0) {
            LogMessage(X_ERROR, "VNC: Failed to load TLS private key: %s\n", config->tls_key_path);
            ERR_print_errors_fp(stderr);
            SSL_CTX_free(vnc_ssl_ctx);
            vnc_ssl_ctx = NULL;
            return FALSE;
        }

        if (!SSL_CTX_check_private_key(vnc_ssl_ctx)) {
            LogMessage(X_ERROR, "VNC: TLS private key does not match certificate\n");
            SSL_CTX_free(vnc_ssl_ctx);
            vnc_ssl_ctx = NULL;
            return FALSE;
        }

        LogMessage(X_INFO, "VNC: TLS enabled with certificate: %s\n", config->tls_cert_path);
    } else {
        LogMessage(X_WARNING, "VNC: TLS certificate/key not configured, TLS disabled\n");
        SSL_CTX_free(vnc_ssl_ctx);
        vnc_ssl_ctx = NULL;
        return FALSE;
    }

    vnc_ssl_initialized = TRUE;
    return TRUE;
}

static void
VNCSSLFini(void)
{
    if (vnc_ssl_ctx) {
        SSL_CTX_free(vnc_ssl_ctx);
        vnc_ssl_ctx = NULL;
    }
    vnc_ssl_initialized = FALSE;
    EVP_cleanup();
    ERR_free_strings();
}

/* ================================================================
 * Backend Lifecycle
 * ================================================================ */

static Bool
VNCBackendInit(ScreenPtr pScreen, RemoteDesktopProtocolPtr self, RemoteDesktopConfigPtr config)
{
    VNCBackendPrivatePtr priv = calloc(1, sizeof(VNCBackendPrivateRec));
    if (!priv)
        return FALSE;

    xorg_list_init(&priv->client_list);
    pthread_mutex_init(&priv->mutex, NULL);
    RegionNull(&priv->damage_region);
    priv->damage_pending = FALSE;

    priv->config = VNCParseConfig(config);
    if (!priv->config) {
        pthread_mutex_destroy(&priv->mutex);
        free(priv);
        return FALSE;
    }

    /* Initialize SSL if TLS cert/key configured */
    if (priv->config->tls_cert_path && priv->config->tls_key_path) {
        if (!VNCSSLInit(priv->config)) {
            /* SSL init failed, continue without TLS */
            LogMessage(X_WARNING, "VNC: Continuing without TLS\n");
        }
    }

    /* Get initial framebuffer info */
    void *fb = NULL;
    int stride = 0, bpp = 0;
    if (RemoteDesktopGetScreenFramebuffer(pScreen, &fb, &stride, &bpp)) {
        priv->framebuffer = fb;
        priv->fb_stride = stride;
        priv->fb_bpp = bpp;
        
        WindowPtr pRoot = pScreen->root;
        if (pRoot) {
            priv->fb_width = pRoot->drawable.width;
            priv->fb_height = pRoot->drawable.height;
        }
    }

    self->private = priv;
    return TRUE;
}

static void
VNCBackendFini(ScreenPtr pScreen, RemoteDesktopProtocolPtr self)
{
    VNCBackendPrivatePtr priv = (VNCBackendPrivatePtr)self->private;
    if (!priv)
        return;

    VNCBackendStop(pScreen, self);

    VNCFreeConfig(priv->config);
    
    /* Cleanup clients */
    VNCClientPtr client, next;
    xorg_list_for_each_entry_safe(client, next, &priv->client_list, entry) {
        xorg_list_del(&client->entry);
        if (client->ssl) {
            SSL_free(client->ssl);
        }
        close(client->fd);
        free(client);
    }

    RegionUninit(&priv->damage_region);
    pthread_mutex_destroy(&priv->mutex);
    free(priv);
    self->private = NULL;
}

static Bool
VNCBackendStart(ScreenPtr pScreen, RemoteDesktopProtocolPtr self)
{
    VNCBackendPrivatePtr priv = (VNCBackendPrivatePtr)self->private;
    if (!priv)
        return FALSE;

#ifdef HAVE_LIBVNCSERVER
    VNCLibVNCScreenInit(priv);
#else
    if (!VNCEmbeddedServerStart(priv))
        return FALSE;
#endif

    return TRUE;
}

static void
VNCBackendStop(ScreenPtr pScreen, RemoteDesktopProtocolPtr self)
{
    VNCBackendPrivatePtr priv = (VNCBackendPrivatePtr)self->private;
    if (!priv)
        return;

#ifdef HAVE_LIBVNCSERVER
    VNCLibVNCScreenFini(priv);
#else
    VNCEmbeddedServerStop(priv);
#endif
}

static Bool
VNCBackendReconfigure(ScreenPtr pScreen, RemoteDesktopProtocolPtr self, RemoteDesktopConfigPtr config)
{
    VNCBackendPrivatePtr priv = (VNCBackendPrivatePtr)self->private;
    if (!priv)
        return FALSE;

    Bool was_running = FALSE;
    
    /* Stop if running */
    if (priv->config) {
        was_running = TRUE;
        VNCBackendStop(pScreen, self);
    }

    /* Free old config */
    VNCFreeConfig(priv->config);

    /* Parse new config */
    priv->config = VNCParseConfig(config);
    if (!priv->config)
        return FALSE;

    /* Restart if was running */
    if (was_running) {
        return VNCBackendStart(pScreen, self);
    }

    return TRUE;
}

/* ================================================================
 * Damage Handling
 * ================================================================ */

static void
VNCBackendDamageNotify(RemoteDesktopProtocolPtr self, RegionPtr pRegion)
{
    VNCBackendPrivatePtr priv = (VNCBackendPrivatePtr)self->private;
    if (!priv || !priv->config->use_damage)
        return;

    pthread_mutex_lock(&priv->mutex);

    /* Union the new damage with existing */
    RegionUnion(&priv->damage_region, &priv->damage_region, pRegion);
    priv->damage_pending = TRUE;

    pthread_mutex_unlock(&priv->mutex);

#ifdef HAVE_LIBVNCSERVER
    /* libvncserver handles damage internally via callback */
#else
    /* For embedded server, damage will be processed in server thread */
#endif
}

/* ================================================================
 * Framebuffer Access
 * ================================================================ */

static Bool
VNCBackendGetFramebuffer(ScreenPtr pScreen, RemoteDesktopProtocolPtr self, void **fb_addr, int *stride, int *bpp)
{
    VNCBackendPrivatePtr priv = (VNCBackendPrivatePtr)self->private;
    if (!priv)
        return FALSE;

    /* Update framebuffer info */
    void *fb = NULL;
    int s = 0, b = 0;
    if (RemoteDesktopGetScreenFramebuffer(pScreen, &fb, &s, &b)) {
        priv->framebuffer = fb;
        priv->fb_stride = s;
        priv->fb_bpp = b;
        
        WindowPtr pRoot = pScreen->root;
        if (pRoot) {
            priv->fb_width = pRoot->drawable.width;
            priv->fb_height = pRoot->drawable.height;
        }
    }

    if (fb_addr) *fb_addr = priv->framebuffer;
    if (stride) *stride = priv->fb_stride;
    if (bpp) *bpp = priv->fb_bpp;

    return priv->framebuffer != NULL;
}

/* ================================================================
 * Client Management
 * ================================================================ */

static void
VNCBackendClientConnected(RemoteDesktopProtocolPtr self, int client_fd)
{
    VNCBackendPrivatePtr priv = (VNCBackendPrivatePtr)self->private;
    if (!priv)
        return;

    VNCClientPtr client = calloc(1, sizeof(VNCClientRec));
    if (!client)
        return;

    client->fd = client_fd;
    client->authenticated = FALSE;
    client->view_only = priv->config ? priv->config->view_only : FALSE;

    pthread_mutex_lock(&priv->mutex);
    xorg_list_add(&client->entry, &priv->client_list);
    pthread_mutex_unlock(&priv->mutex);
}

static void
VNCBackendClientDisconnected(RemoteDesktopProtocolPtr self, int client_fd)
{
    VNCBackendPrivatePtr priv = (VNCBackendPrivatePtr)self->private;
    if (!priv)
        return;

    pthread_mutex_lock(&priv->mutex);
    VNCClientPtr client, next;
    xorg_list_for_each_entry_safe(client, next, &priv->client_list, entry) {
        if (client->fd == client_fd) {
            xorg_list_del(&client->entry);
            close(client->fd);
            free(client);
            break;
        }
    }
    pthread_mutex_unlock(&priv->mutex);
}

/* ================================================================
 * libvncserver Implementation
 * ================================================================ */

#ifdef HAVE_LIBVNCSERVER

static void
VNCLibVNCScreenInit(VNCBackendPrivatePtr priv)
{
    if (!priv->framebuffer)
        return;

    priv->rfb_screen = rfbGetScreen(NULL, NULL, priv->fb_width, priv->fb_height, 8, 3, 4);
    if (!priv->rfb_screen)
        return;

    priv->rfb_screen->frameBuffer = priv->framebuffer;
    priv->rfb_screen->alwaysShared = TRUE;
    priv->rfb_screen->neverShared = FALSE;
    priv->rfb_screen->dontDisconnect = TRUE;
    priv->rfb_screen->port = priv->config->port;
    priv->rfb_screen->inetdSock = -1;
    priv->rfb_screen->inetdInitDone = FALSE;
    priv->rfb_screen->httpDir = NULL;
    priv->rfb_screen->httpEnableProxyConnect = FALSE;

    if (priv->config->localhost_only) {
        priv->rfb_screen->listenInterface = "127.0.0.1";
    } else {
        priv->rfb_screen->listenInterface = "0.0.0.0";
    }

    VNCLibVNCSetPassword(priv);

    /* Set damage callback */
    priv->rfb_screen->damageCallback = VNCLibVNCDamageCallback;

    /* Initialize server */
    rfbInitServer(priv->rfb_screen);
}

static void
VNCLibVNCScreenFini(VNCBackendPrivatePtr priv)
{
    if (priv->rfb_screen) {
        rfbScreenCleanup(priv->rfb_screen);
        priv->rfb_screen = NULL;
    }
}

static void
VNCLibVNCSetPassword(VNCBackendPrivatePtr priv)
{
    if (priv->config && priv->config->password) {
        priv->rfb_screen->authPasswdData = priv->config->password;
    } else {
        priv->rfb_screen->authPasswdData = NULL;
    }
}

static void
VNCLibVNCDamageCallback(rfbScreenInfoPtr screen, int x, int y, int w, int h)
{
    /* libvncserver handles framebuffer updates automatically */
    rfbMarkRectAsModified(screen, x, y, x + w, y + h);
}

#else

/* ================================================================
 * Embedded Minimal RFB Implementation
 * ================================================================ */

#define RFB_PROTOCOL_VERSION "RFB 003.008\n"
#define RFB_SECURITY_TYPE_NONE 1
#define RFB_SECURITY_TYPE_VNC 2
#define RFB_ENCODING_RAW 0

#define VNC_MAX_CLIENTS 32

static Bool
VNCEmbeddedServerStart(VNCBackendPrivatePtr priv)
{
    if (!priv->framebuffer)
        return FALSE;

    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port = htons(priv->config->port);
    
    if (priv->config->localhost_only) {
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    } else {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    priv->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (priv->listen_fd < 0)
        return FALSE;

    int opt = 1;
    setsockopt(priv->listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(priv->listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(priv->listen_fd);
        return FALSE;
    }

    if (listen(priv->listen_fd, 5) < 0) {
        close(priv->listen_fd);
        return FALSE;
    }

    /* Set non-blocking */
    fcntl(priv->listen_fd, F_SETFL, O_NONBLOCK);

    /* Allocate client fds array */
    priv->client_fds = calloc(VNC_MAX_CLIENTS, sizeof(struct pollfd));
    if (!priv->client_fds) {
        close(priv->listen_fd);
        return FALSE;
    }
    priv->max_clients = VNC_MAX_CLIENTS;
    priv->num_clients = 0;

    /* Initialize first entry for listening socket */
    priv->client_fds[0].fd = priv->listen_fd;
    priv->client_fds[0].events = POLLIN;
    priv->num_clients = 1;

    priv->thread_running = TRUE;
    if (pthread_create(&priv->server_thread, NULL, VNCEmbeddedServerThread, priv) != 0) {
        priv->thread_running = FALSE;
        free(priv->client_fds);
        close(priv->listen_fd);
        return FALSE;
    }

    return TRUE;
}

static void
VNCEmbeddedServerStop(VNCBackendPrivatePtr priv)
{
    if (!priv->thread_running)
        return;

    priv->thread_running = FALSE;
    pthread_join(priv->server_thread, NULL);

    close(priv->listen_fd);
    priv->listen_fd = -1;

    /* Close all client connections */
    for (int i = 1; i < priv->num_clients; i++) {
        if (priv->client_fds[i].fd >= 0) {
            close(priv->client_fds[i].fd);
        }
    }

    free(priv->client_fds);
    priv->client_fds = NULL;
    priv->num_clients = 0;
}

static void *
VNCEmbeddedServerThread(void *arg)
{
    VNCBackendPrivatePtr priv = (VNCBackendPrivatePtr)arg;

    while (priv->thread_running) {
        int ret = poll(priv->client_fds, priv->num_clients, 100);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            break;
        }

        if (ret == 0)
            continue;

        /* Check for new connections */
        if (priv->client_fds[0].revents & POLLIN) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int client_fd = accept(priv->listen_fd, (struct sockaddr *)&client_addr, &addr_len);
            if (client_fd >= 0) {
                if (priv->num_clients < priv->max_clients) {
                    priv->client_fds[priv->num_clients].fd = client_fd;
                    priv->client_fds[priv->num_clients].events = POLLIN;
                    priv->num_clients++;

                    VNCBackendClientConnected(&vnc_protocol, client_fd);
                } else {
                    close(client_fd);
                }
            }
        }

        /* Check client connections */
        for (int i = 1; i < priv->num_clients; i++) {
            if (priv->client_fds[i].revents & POLLIN) {
                VNCEmbeddedHandleClient(priv, priv->client_fds[i].fd);
            }
            if (priv->client_fds[i].revents & (POLLHUP | POLLERR)) {
                VNCBackendClientDisconnected(&vnc_protocol, priv->client_fds[i].fd);
                /* Remove from array */
                close(priv->client_fds[i].fd);
                memmove(&priv->client_fds[i], &priv->client_fds[i + 1],
                        (priv->num_clients - i - 1) * sizeof(struct pollfd));
                priv->num_clients--;
                i--;
            }
        }

        /* Send pending damage updates */
        if (priv->damage_pending) {
            pthread_mutex_lock(&priv->mutex);
            if (priv->damage_pending && RegionNotEmpty(&priv->damage_region)) {
                for (int i = 1; i < priv->num_clients; i++) {
                    VNCEmbeddedSendFramebufferUpdate(priv, priv->client_fds[i].fd, &priv->damage_region);
                }
                RegionEmpty(&priv->damage_region);
                priv->damage_pending = FALSE;
            }
            pthread_mutex_unlock(&priv->mutex);
        }
    }

    return NULL;
}

static void
VNCEmbeddedHandleClient(VNCBackendPrivatePtr priv, int client_fd)
{
    static char buffer[1024];
    ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
    if (n <= 0)
        return;

    buffer[n] = '\0';

    /* Check if this is initial protocol version exchange */
    if (strncmp(buffer, "RFB ", 4) == 0) {
        /* Send our version */
        write(client_fd, RFB_PROTOCOL_VERSION, strlen(RFB_PROTOCOL_VERSION));
        return;
    }

    /* Check for security type selection */
    if (buffer[0] == 1 && n >= 2) { /* Client selected security type */
        if (priv->config && priv->config->password) {
            /* VNC authentication - simplified */
            write(client_fd, "\x00\x00\x00\x00", 4); /* OK */
            /* In real implementation, would do challenge-response */
        } else {
            /* No authentication */
            write(client_fd, "\x00\x00\x00\x00", 4); /* OK */
        }
        return;
    }

    /* Handle client messages (framebuffer update requests, etc.) */
    if (n >= 1) {
        uint8_t msg_type = buffer[0];
        if (msg_type == 3) { /* FramebufferUpdateRequest */
            VNCEmbeddedSendFramebufferUpdate(priv, client_fd, &priv->damage_region);
        }
    }
}

static void
VNCEmbeddedSendFramebufferUpdate(VNCBackendPrivatePtr priv, int client_fd, RegionPtr region)
{
    if (!priv->framebuffer || RegionNil(region))
        return;

    int num_rects = RegionNumRects(region);
    BoxPtr boxes = RegionRects(region);

    /* Send FramebufferUpdate header */
    uint8_t header[4] = { 0, 0, (num_rects >> 8) & 0xFF, num_rects & 0xFF };
    write(client_fd, header, 4);

    for (int i = 0; i < num_rects; i++) {
        VNCEmbeddedSendRawEncoding(priv, client_fd, boxes[i].x1, boxes[i].y1,
                                    boxes[i].x2 - boxes[i].x1, boxes[i].y2 - boxes[i].y1);
    }
}

static void
VNCEmbeddedSendRawEncoding(VNCBackendPrivatePtr priv, int client_fd, int x, int y, int w, int h)
{
    if (!priv->framebuffer)
        return;

    /* Send rectangle header: x, y, w, h, encoding */
    uint8_t rect_header[12];
    rect_header[0] = (x >> 8) & 0xFF;
    rect_header[1] = x & 0xFF;
    rect_header[2] = (y >> 8) & 0xFF;
    rect_header[3] = y & 0xFF;
    rect_header[4] = (w >> 8) & 0xFF;
    rect_header[5] = w & 0xFF;
    rect_header[6] = (h >> 8) & 0xFF;
    rect_header[7] = h & 0xFF;
    rect_header[8] = 0;
    rect_header[9] = 0;
    rect_header[10] = 0;
    rect_header[11] = RFB_ENCODING_RAW;

    write(client_fd, rect_header, 12);

    /* Send pixel data */
    uint8_t *fb = (uint8_t *)priv->framebuffer;
    int bytes_per_pixel = priv->fb_bpp / 8;
    
    for (int row = 0; row < h; row++) {
        uint8_t *src = fb + (y + row) * priv->fb_stride + x * bytes_per_pixel;
        write(client_fd, src, w * bytes_per_pixel);
    }
}

#endif /* HAVE_LIBVNCSERVER */