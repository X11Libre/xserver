/*
 * RemoteDesktop Extension - Main Entry Point
 *
 * SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 */

#include <dix-config.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <X11/X.h>
#include <X11/Xproto.h>

#include "dix/dix_priv.h"
#include "dix/extension_priv.h"
#include "dix/request_priv.h"
#include "dix/resource_priv.h"
#include "dix/screenint_priv.h"
#include "dix/callback_priv.h"
#include "include/callback.h"
#include "include/regionstr.h"
#include "include/windowstr.h"
#include "include/scrnintstr.h"
#include "include/privates.h"
#include "os/client_priv.h"
#include "Xext/remotedesktop/remotedesktopproto.h"
#include "Xext/remotedesktop/remotedesktop_priv.h"
#include "Xext/damage/damageext_priv.h"

#define MAX_PROTOCOL_NAME_LEN 64
#define MAX_CONFIG_DATA_LEN (64 * 1024)

/* Global state */
ExtensionEntry *RemoteDesktopExtensionEntry = NULL;
int RemoteDesktopReqCode = 0;
int RemoteDesktopEventBase = 0;
int RemoteDesktopErrorBase = 0;

RESTYPE RemoteDesktopSessionType = 0;
RESTYPE RemoteDesktopProtocolType = 0;

DevPrivateKeyRec RemoteDesktopScreenPrivateKeyRec;

_X_EXPORT Bool noRemoteDesktopExtension = FALSE;

/* Protocol list */
static struct xorg_list protocol_list = { &protocol_list, &protocol_list };

/* Forward declarations */
static int ProcRemoteDesktopDispatch(ClientPtr client);
static int SProcRemoteDesktopDispatch(ClientPtr client);

static int ProcRemoteDesktopQueryVersion(ClientPtr client);
static int ProcRemoteDesktopListProtocols(ClientPtr client);
static int ProcRemoteDesktopCreateSession(ClientPtr client);
static int ProcRemoteDesktopDestroySession(ClientPtr client);
static int ProcRemoteDesktopConfigureSession(ClientPtr client);
static int ProcRemoteDesktopGetSessionInfo(ClientPtr client);
static int ProcRemoteDesktopSetSessionState(ClientPtr client);

static int SProcRemoteDesktopQueryVersion(ClientPtr client);
static int SProcRemoteDesktopListProtocols(ClientPtr client);
static int SProcRemoteDesktopCreateSession(ClientPtr client);
static int SProcRemoteDesktopDestroySession(ClientPtr client);
static int SProcRemoteDesktopConfigureSession(ClientPtr client);
static int SProcRemoteDesktopGetSessionInfo(ClientPtr client);
static int SProcRemoteDesktopSetSessionState(ClientPtr client);

static void RemoteDesktopCloseDown(ExtensionEntry *extEntry);
static int FreeRemoteDesktopSession(void *value, XID id);
static int FreeRemoteDesktopProtocol(void *value, XID id);
static Bool RemoteDesktopScreenInit(ScreenPtr pScreen);
static Bool RemoteDesktopCloseScreen(ScreenPtr pScreen);

/* Request vectors */
static int (*ProcRemoteDesktopVector[])(ClientPtr) = {
    ProcRemoteDesktopQueryVersion,
    ProcRemoteDesktopListProtocols,
    ProcRemoteDesktopCreateSession,
    ProcRemoteDesktopDestroySession,
    ProcRemoteDesktopConfigureSession,
    ProcRemoteDesktopGetSessionInfo,
    ProcRemoteDesktopSetSessionState,
};

static int (*SProcRemoteDesktopVector[])(ClientPtr) = {
    SProcRemoteDesktopQueryVersion,
    SProcRemoteDesktopListProtocols,
    SProcRemoteDesktopCreateSession,
    SProcRemoteDesktopDestroySession,
    SProcRemoteDesktopConfigureSession,
    SProcRemoteDesktopGetSessionInfo,
    SProcRemoteDesktopSetSessionState,
};

/* ================================================================
 * Protocol Registration
 * ================================================================ */

Bool
RemoteDesktopRegisterProtocol(RemoteDesktopProtocolPtr protocol)
{
    if (!protocol || !protocol->name)
        return FALSE;

    /* Check for duplicate */
    RemoteDesktopProtocolPtr existing;
    xorg_list_for_each_entry(existing, &protocol_list, entry) {
        if (strcmp(existing->name, protocol->name) == 0)
            return FALSE;
    }

    xorg_list_add(&protocol->entry, &protocol_list);
    return TRUE;
}

void
RemoteDesktopUnregisterProtocol(RemoteDesktopProtocolPtr protocol)
{
    if (protocol) {
        xorg_list_del(&protocol->entry);
    }
}

RemoteDesktopProtocolPtr
RemoteDesktopFindProtocol(const char *name)
{
    if (!name)
        return NULL;

    RemoteDesktopProtocolPtr proto;
    xorg_list_for_each_entry(proto, &protocol_list, entry) {
        if (strcmp(proto->name, name) == 0)
            return proto;
    }
    return NULL;
}

/* ================================================================
 * Session Management
 * ================================================================ */

static int
FreeRemoteDesktopSession(void *value, XID id)
{
    RemoteDesktopSessionPtr session = value;
    if (session) {
        /* Cleanup damage */
        if (session->pDamage) {
            DamageDestroy(session->pDamage);
            session->pDamage = NULL;
        }
        /* Remove from screen's session list */
        xorg_list_del(&session->entry);
        /* Free config */
        if (session->config) {
            RemoteDesktopFreeConfig(session->config);
        }
        free(session->error_message);
        free(session);
    }
    return Success;
}

static int
FreeRemoteDesktopProtocol(void *value, XID id)
{
    /* Protocol objects are global, not per-session. This is just for resource tracking. */
    return Success;
}

RemoteDesktopSessionPtr
RemoteDesktopCreateSession(ScreenPtr pScreen, XID session_id,
                           RemoteDesktopProtocolPtr protocol,
                           RemoteDesktopConfigPtr config)
{
    if (!pScreen || !protocol)
        return NULL;

    RemoteDesktopScreenPrivatePtr screen_priv = RemoteDesktopGetScreenPrivate(pScreen);
    if (!screen_priv)
        return NULL;

    RemoteDesktopSessionPtr session = calloc(1, sizeof(RemoteDesktopSessionRec));
    if (!session)
        return NULL;

    session->session_id = session_id;
    session->pScreen = pScreen;
    session->protocol = protocol;
    session->config = config;
    session->state = RD_STATE_STOPPED;
    session->connected_clients = 0;
    session->bytes_sent = 0;
    session->bytes_received = 0;
    session->error_message = NULL;

    /* Add to screen's session list */
    xorg_list_add(&session->entry, &screen_priv->session_list);

    /* Register as resource */
    if (!AddResource(session_id, RemoteDesktopSessionType, session)) {
        xorg_list_del(&session->entry);
        free(session);
        return NULL;
    }

    return session;
}

void
RemoteDesktopDestroySession(RemoteDesktopSessionPtr session)
{
    if (session) {
        FreeResource(session->session_id, X11_RESTYPE_NONE);
    }
}

RemoteDesktopSessionPtr
RemoteDesktopFindSession(XID session_id)
{
    RemoteDesktopSessionPtr session;
    int rc = dixLookupResourceByType((void **)&session, session_id,
                                      RemoteDesktopSessionType,
                                      serverClient, DixReadAccess);
    if (rc != Success)
        return NULL;
    return session;
}

Bool
RemoteDesktopConfigureSession(RemoteDesktopSessionPtr session,
                              RemoteDesktopConfigPtr config)
{
    if (!session || !session->protocol)
        return FALSE;

    if (session->protocol->Reconfigure) {
        return session->protocol->Reconfigure(session->pScreen, session->protocol, config);
    }
    return FALSE;
}

Bool
RemoteDesktopSetSessionState(RemoteDesktopSessionPtr session, int state)
{
    if (!session || !session->protocol)
        return FALSE;

    if (state == RD_STATE_RUNNING && session->state != RD_STATE_RUNNING) {
        if (session->protocol->Start) {
            if (!session->protocol->Start(session->pScreen, session->protocol)) {
                session->state = RD_STATE_ERROR;
                return FALSE;
            }
        }
        /* Setup damage tracking */
        if (!RemoteDesktopSetupDamage(session->pScreen, session)) {
            if (session->protocol->Stop)
                session->protocol->Stop(session->pScreen, session->protocol);
            session->state = RD_STATE_ERROR;
            return FALSE;
        }
        session->state = RD_STATE_RUNNING;
        RemoteDesktopSendNotifyEvent(session, RD_NOTIFY_CONFIG_CHANGED, 0);
    }
    else if (state == RD_STATE_STOPPED && session->state == RD_STATE_RUNNING) {
        RemoteDesktopTeardownDamage(session);
        if (session->protocol->Stop)
            session->protocol->Stop(session->pScreen, session->protocol);
        session->state = RD_STATE_STOPPED;
        RemoteDesktopSendNotifyEvent(session, RD_NOTIFY_CONFIG_CHANGED, 0);
    }

    return TRUE;
}

/* ================================================================
 * Damage Handling
 * ================================================================ */

static void
RemoteDesktopDamageReportPerScreen(DamagePtr pDamage, RegionPtr pRegion, void *closure)
{
    RemoteDesktopSessionPtr session = (RemoteDesktopSessionPtr)closure;
    if (!session || session->state != RD_STATE_RUNNING || !session->protocol)
        return;

    if (session->protocol->DamageNotify) {
        session->protocol->DamageNotify(session->protocol, pRegion);
    }
}

static void
RemoteDesktopDamageDestroyPerScreen(DamagePtr pDamage, void *closure)
{
    /* The session's pDamage pointer is set to NULL in TeardownDamage */
}

Bool
RemoteDesktopSetupDamage(ScreenPtr pScreen, RemoteDesktopSessionPtr session)
{
    WindowPtr pRoot = pScreen->root;
    if (!pRoot)
        return FALSE;

    RemoteDesktopScreenPrivatePtr screen_priv = RemoteDesktopGetScreenPrivate(pScreen);
    if (!screen_priv)
        return FALSE;

#ifdef XINERAMA
    if (PanoramiXIsEnabled()) {
        /* For Xinerama, create damage object per screen */
        int num_screens = screenInfo.numScreens;
        screen_priv->damage_array = calloc(num_screens, sizeof(DamagePtr));
        if (!screen_priv->damage_array)
            return FALSE;
        screen_priv->num_damage_screens = num_screens;

        for (int i = 0; i < num_screens; i++) {
            ScreenPtr scr = screenInfo.screens[i];
            if (!scr || !scr->root)
                continue;

            DamagePtr dmg = DamageCreate(RemoteDesktopDamageReportPerScreen,
                                          RemoteDesktopDamageDestroyPerScreen,
                                          DamageReportRawRegion,
                                          FALSE,
                                          scr,
                                          session);
            if (!dmg) {
                /* Cleanup on failure */
                for (int j = 0; j < i; j++) {
                    if (screen_priv->damage_array[j]) {
                        DamageDestroy(screen_priv->damage_array[j]);
                    }
                }
                free(screen_priv->damage_array);
                screen_priv->damage_array = NULL;
                screen_priv->num_damage_screens = 0;
                return FALSE;
            }

            DamageRegister(&scr->root->drawable, dmg);
            screen_priv->damage_array[i] = dmg;

            /* Report initial full-screen damage for this screen */
            RegionPtr pRegion = &scr->root->borderClip;
            DamageReportDamage(dmg, pRegion);
        }

        /* Keep first damage as primary for backward compatibility */
        session->pDamage = screen_priv->damage_array[0];
        screen_priv->damage_active = TRUE;

        return TRUE;
    }
#endif

    /* Single screen (non-Xinerama) */
    session->pDamage = DamageCreate(RemoteDesktopDamageReportPerScreen,
                                     RemoteDesktopDamageDestroyPerScreen,
                                     DamageReportRawRegion,
                                     FALSE,
                                     pScreen,
                                     session);
    if (!session->pDamage)
        return FALSE;

    DamageRegister(&pRoot->drawable, session->pDamage);

    /* Report initial full-screen damage */
    RegionPtr pRegion = &pRoot->borderClip;
    DamageReportDamage(session->pDamage, pRegion);

    screen_priv->damage_active = TRUE;

    return TRUE;
}

void
RemoteDesktopTeardownDamage(RemoteDesktopSessionPtr session)
{
    if (!session || !session->pScreen)
        return;

    RemoteDesktopScreenPrivatePtr screen_priv = RemoteDesktopGetScreenPrivate(session->pScreen);
    if (!screen_priv)
        return;

    /* Cleanup per-screen damage objects */
    if (screen_priv->damage_array) {
        for (int i = 0; i < screen_priv->num_damage_screens; i++) {
            if (screen_priv->damage_array[i]) {
                DamageUnregister(screen_priv->damage_array[i]);
                DamageDestroy(screen_priv->damage_array[i]);
            }
        }
        free(screen_priv->damage_array);
        screen_priv->damage_array = NULL;
        screen_priv->num_damage_screens = 0;
    }

    /* Cleanup primary damage (for non-Xinerama) */
    if (session->pDamage) {
        DamageUnregister(session->pDamage);
        DamageDestroy(session->pDamage);
        session->pDamage = NULL;
    }

    screen_priv->damage_active = FALSE;
}

/* ================================================================
 * Configuration Parsing
 * ================================================================ */

RemoteDesktopConfigPtr
RemoteDesktopParseConfig(const char *data, int len)
{
    if (!data || len <= 0)
        return NULL;

    /* Simple key=value format, one per line, NULL-terminated */
    RemoteDesktopConfigPtr head = NULL;
    RemoteDesktopConfigPtr *tail = &head;

    const char *end = data + len;
    const char *p = data;

    while (p < end) {
        const char *line_end = memchr(p, '\n', end - p);
        if (!line_end)
            line_end = end;

        int line_len = line_end - p;
        if (line_len > 0) {
            /* Find '=' */
            const char *eq = memchr(p, '=', line_len);
            if (eq && eq > p && eq < line_end) {
                int key_len = eq - p;
                int val_len = line_end - (eq + 1);

                RemoteDesktopConfigPtr entry = calloc(1, sizeof(RemoteDesktopConfigRec));
                if (!entry)
                    break;

                entry->key = strndup(p, key_len);
                if (!entry->key) {
                    free(entry);
                    break;
                }

                /* Determine type and parse value */
                if (val_len >= 4 && strncasecmp(eq + 1, "true", 4) == 0) {
                    entry->type = RD_CONFIG_TYPE_BOOLEAN;
                    entry->value.bool_val = true;
                }
                else if (val_len >= 5 && strncasecmp(eq + 1, "false", 5) == 0) {
                    entry->type = RD_CONFIG_TYPE_BOOLEAN;
                    entry->value.bool_val = false;
                }
                else if (val_len > 0 && eq[1] >= '0' && eq[1] <= '9') {
                    entry->type = RD_CONFIG_TYPE_INTEGER;
                    entry->value.int_val = strtoll(eq + 1, NULL, 10);
                }
                else {
                    entry->type = RD_CONFIG_TYPE_STRING;
                    entry->value.str_val = strndup(eq + 1, val_len);
                }

                *tail = entry;
                tail = &entry->next;
            }
        }

        p = line_end + 1;
    }

    return head;
}

void
RemoteDesktopFreeConfig(RemoteDesktopConfigPtr config)
{
    while (config) {
        RemoteDesktopConfigPtr next = config->next;
        free(config->key);
        if (config->type == RD_CONFIG_TYPE_STRING)
            free(config->value.str_val);
        free(config);
        config = next;
    }
}

char *
RemoteDesktopConfigGetString(RemoteDesktopConfigPtr config, const char *key)
{
    while (config) {
        if (strcmp(config->key, key) == 0 && config->type == RD_CONFIG_TYPE_STRING)
            return config->value.str_val;
        config = config->next;
    }
    return NULL;
}

int64_t
RemoteDesktopConfigGetInt(RemoteDesktopConfigPtr config, const char *key)
{
    while (config) {
        if (strcmp(config->key, key) == 0 && config->type == RD_CONFIG_TYPE_INTEGER)
            return config->value.int_val;
        config = config->next;
    }
    return 0;
}

bool
RemoteDesktopConfigGetBool(RemoteDesktopConfigPtr config, const char *key)
{
    while (config) {
        if (strcmp(config->key, key) == 0 && config->type == RD_CONFIG_TYPE_BOOLEAN)
            return config->value.bool_val;
        config = config->next;
    }
    return false;
}

/* ================================================================
 * Framebuffer Access
 * ================================================================ */

static void *
RemoteDesktopDefaultFramebufferAccessor(ScreenPtr pScreen, int *stride, int *bpp)
{
    WindowPtr pRoot = pScreen->root;
    if (!pRoot)
        return NULL;

    /* Check for cached framebuffer first */
    RemoteDesktopScreenPrivatePtr priv = RemoteDesktopGetScreenPrivate(pScreen);
    if (priv && priv->fb_valid && priv->framebuffer) {
        if (stride) *stride = priv->fb_stride;
        if (bpp) *bpp = priv->fb_bpp;
        return priv->framebuffer;
    }

    /* Use GetImage/GetSpans fallback for DDX-agnostic access */
    int width = pRoot->drawable.width;
    int height = pRoot->drawable.height;
    int depth = pRoot->drawable.depth;
    
    if (width <= 0 || height <= 0 || depth <= 0)
        return NULL;

    /* Try to use GetImage if available */
    if (pScreen->GetImage) {
        unsigned int format = (depth == 24) ? ZPixmap : ZPixmap;
        unsigned long planeMask = 0xFFFFFFFF;
        char *image_data = malloc(width * height * 4); /* Max 32bpp */
        if (!image_data)
            return NULL;

        pScreen->GetImage((DrawablePtr)pRoot, 0, 0, width, height, format, planeMask, image_data);

        if (priv) {
            priv->framebuffer = image_data;
            priv->fb_stride = width * 4; /* Assuming 32bpp for cache */
            priv->fb_bpp = 32;
            priv->fb_valid = TRUE;
        }

        if (stride) *stride = width * 4;
        if (bpp) *bpp = 32;
        return image_data;
    }

    /* Fallback to GetSpans if GetImage not available */
    if (pScreen->GetSpans) {
        DDXPointRec pt = {0, 0};
        int widths[1] = {width};
        char *span_data = malloc(width * height * 4);
        if (!span_data)
            return NULL;

        char *dst = span_data;
        for (int y = 0; y < height; y++) {
            pt.x = 0;
            pt.y = y;
            pScreen->GetSpans((DrawablePtr)pRoot, width, &pt, &widths[0], 1, dst);
            dst += width * 4;
        }

        if (priv) {
            priv->framebuffer = span_data;
            priv->fb_stride = width * 4;
            priv->fb_bpp = 32;
            priv->fb_valid = TRUE;
        }

        if (stride) *stride = width * 4;
        if (bpp) *bpp = 32;
        return span_data;
    }

    return NULL;
}

void
RemoteDesktopSetFramebufferAccessor(ScreenPtr pScreen,
                                    void *(*accessor)(ScreenPtr, int*, int*))
{
    RemoteDesktopScreenPrivatePtr priv = RemoteDesktopGetScreenPrivate(pScreen);
    if (priv) {
        priv->GetFramebuffer = accessor;
    }
}

Bool
RemoteDesktopGetScreenFramebuffer(ScreenPtr pScreen, void **fb_addr, int *stride, int *bpp)
{
    RemoteDesktopScreenPrivatePtr priv = RemoteDesktopGetScreenPrivate(pScreen);
    if (!priv)
        return FALSE;

    if (priv->GetFramebuffer) {
        void *fb = priv->GetFramebuffer(pScreen, stride, bpp);
        if (fb) {
            if (fb_addr) *fb_addr = fb;
            return TRUE;
        }
    }

    /* Fallback to default */
    void *fb = RemoteDesktopDefaultFramebufferAccessor(pScreen, stride, bpp);
    if (fb && fb_addr) *fb_addr = fb;

    return fb != NULL;
}

void
RemoteDesktopSendNotifyEvent(RemoteDesktopSessionPtr session, int subtype, int detail)
{
    if (!session || !session->protocol)
        return;

    xRemoteDesktopNotifyEvent ev = {
        .type = RemoteDesktopEventBase + RemoteDesktopNotify,
        .subType = subtype,
        .sequenceNumber = 0, /* filled by WriteEventsToClient */
        .sessionId = session->session_id,
        .detail = detail,
        .timestamp = currentTime.milliseconds,
    };

    /* Send to all clients interested in this extension */
    for (int i = 1; i < currentMaxClients; i++) {
        ClientPtr client = clients[i];
        if (client && client->majorOp == RemoteDesktopReqCode) {
            WriteEventsToClient(client, 1, (xEvent *)&ev);
        }
    }
}

/* ================================================================
 * Screen Initialization / Close
 * ================================================================ */

static Bool
RemoteDesktopScreenInit(ScreenPtr pScreen)
{
    RemoteDesktopScreenPrivatePtr priv = calloc(1, sizeof(RemoteDesktopScreenPrivateRec));
    if (!priv)
        return FALSE;

    xorg_list_init(&priv->protocol_list);
    xorg_list_init(&priv->session_list);
    RegionNull(&priv->last_damage);
    priv->damage_active = FALSE;
    priv->damage_array = NULL;
    priv->num_damage_screens = 0;
    priv->framebuffer = NULL;
    priv->fb_stride = 0;
    priv->fb_bpp = 0;
    priv->fb_valid = FALSE;
    priv->GetFramebuffer = RemoteDesktopDefaultFramebufferAccessor;

    /* Wrap CloseScreen - standard save-and-chain pattern */
    priv->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = RemoteDesktopCloseScreen;

    RemoteDesktopSetScreenPrivate(pScreen, priv);
    return TRUE;
}

static Bool
RemoteDesktopCloseScreen(ScreenPtr pScreen)
{
    RemoteDesktopScreenPrivatePtr priv = RemoteDesktopGetScreenPrivate(pScreen);
    if (!priv)
        return TRUE;

    /* Destroy all sessions on this screen */
    RemoteDesktopSessionPtr session, next;
    xorg_list_for_each_entry_safe(session, next, &priv->session_list, entry) {
        RemoteDesktopDestroySession(session);
    }

    /* Destroy per-screen damage objects */
    if (priv->damage_array) {
        for (int i = 0; i < priv->num_damage_screens; i++) {
            if (priv->damage_array[i]) {
                DamageUnregister(priv->damage_array[i]);
                DamageDestroy(priv->damage_array[i]);
            }
        }
        free(priv->damage_array);
        priv->damage_array = NULL;
        priv->num_damage_screens = 0;
    }

    RegionUninit(&priv->last_damage);

    /* Restore CloseScreen */
    pScreen->CloseScreen = priv->CloseScreen;

    Bool ret = pScreen->CloseScreen(pScreen);

    free(priv);
    RemoteDesktopSetScreenPrivate(pScreen, NULL);

    return ret;
}

/* ================================================================
 * Extension Initialization
 * ================================================================ */

static void
RemoteDesktopCloseDown(ExtensionEntry *extEntry)
{
    /* Cleanup global protocol list */
    RemoteDesktopProtocolPtr proto, next;
    xorg_list_for_each_entry_safe(proto, next, &protocol_list, entry) {
        xorg_list_del(&proto->entry);
        /* Protocol objects are statically allocated, don't free */
    }
}

/* Request handlers */

static int
ProcRemoteDesktopQueryVersion(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xRemoteDesktopQueryVersionReq);
    X_REQUEST_FIELD_CARD32(majorVersion);
    X_REQUEST_FIELD_CARD32(minorVersion);

    xRemoteDesktopQueryVersionReply reply = {
        .majorVersion = REMOTEDESKTOP_MAJOR_VERSION,
        .minorVersion = REMOTEDESKTOP_MINOR_VERSION,
    };

    if (stuff->majorVersion < REMOTEDESKTOP_MAJOR_VERSION) {
        reply.majorVersion = stuff->majorVersion;
        reply.minorVersion = stuff->minorVersion;
    }
    else if (stuff->majorVersion == REMOTEDESKTOP_MAJOR_VERSION &&
             stuff->minorVersion < REMOTEDESKTOP_MINOR_VERSION) {
        reply.minorVersion = stuff->minorVersion;
    }

    X_REPLY_FIELD_CARD32(majorVersion);
    X_REPLY_FIELD_CARD32(minorVersion);

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
ProcRemoteDesktopListProtocols(ClientPtr client)
{
    REQUEST_SIZE_MATCH(xRemoteDesktopListProtocolsReq);

    xRemoteDesktopListProtocolsReply reply = { 0 };
    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    int count = 0;
    RemoteDesktopProtocolPtr proto;
    xorg_list_for_each_entry(proto, &protocol_list, entry) {
        count++;
    }

    reply.nProtocols = count;

    /* First send reply header */
    int rc = X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
    if (rc != Success)
        return rc;

    /* Then send protocol info for each protocol */
    xorg_list_for_each_entry(proto, &protocol_list, entry) {
        int name_len = strlen(proto->name);
        int vendor_len = proto->vendor ? strlen(proto->vendor) : 0;

        x_rpcbuf_write_CARD16(&rpcbuf, name_len);
        x_rpcbuf_write_CARD8s(&rpcbuf, (CARD8 *)proto->name, name_len);

        x_rpcbuf_write_CARD16(&rpcbuf, vendor_len);
        if (vendor_len > 0)
            x_rpcbuf_write_CARD8s(&rpcbuf, (CARD8 *)proto->vendor, vendor_len);

        x_rpcbuf_write_CARD32(&rpcbuf, proto->version);
        x_rpcbuf_write_CARD32(&rpcbuf, proto->capabilities);
    }

    return Success;
}

static int
ProcRemoteDesktopCreateSession(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xRemoteDesktopCreateSessionReq);
    X_REQUEST_FIELD_CARD32(sessionId);
    X_REQUEST_FIELD_CARD32(screen);
    X_REQUEST_FIELD_CARD16(protocolNameLen);
    X_REQUEST_FIELD_CARD16(configDataLen);

    REQUEST_AT_LEAST_SIZE(xRemoteDesktopCreateSessionReq);
    REQUEST_FIXED_SIZE(xRemoteDesktopCreateSessionReq,
                       stuff->protocolNameLen + stuff->configDataLen);

    /* Validate screen */
    if (stuff->screen >= screenInfo.numScreens)
        return BadValue;

    ScreenPtr pScreen = screenInfo.screens[stuff->screen];
    if (!pScreen)
        return BadValue;

    /* Check if session ID already in use */
    LEGAL_NEW_RESOURCE(stuff->sessionId, client);

    /* Read protocol name */
    char protocol_name[MAX_PROTOCOL_NAME_LEN + 1] = { 0 };
    if (stuff->protocolNameLen > MAX_PROTOCOL_NAME_LEN)
        return BadLength;

    memcpy(protocol_name, (char *)&stuff[1], stuff->protocolNameLen);
    protocol_name[stuff->protocolNameLen] = '\0';

    /* Read config data */
    const char *config_data = (const char *)&stuff[1] + stuff->protocolNameLen;

    /* Find protocol */
    RemoteDesktopProtocolPtr protocol = RemoteDesktopFindProtocol(protocol_name);
    if (!protocol)
        return RemoteDesktopErrorBase + BadRemoteDesktopProtocol;

    /* Parse config */
    RemoteDesktopConfigPtr config = RemoteDesktopParseConfig(config_data, stuff->configDataLen);

    /* Create session */
    RemoteDesktopSessionPtr session = RemoteDesktopCreateSession(pScreen, stuff->sessionId,
                                                                  protocol, config);
    if (!session) {
        RemoteDesktopFreeConfig(config);
        return BadAlloc;
    }

    return Success;
}

static int
ProcRemoteDesktopDestroySession(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xRemoteDesktopDestroySessionReq);
    X_REQUEST_FIELD_CARD32(sessionId);

    RemoteDesktopSessionPtr session = RemoteDesktopFindSession(stuff->sessionId);
    if (!session)
        return RemoteDesktopErrorBase + BadRemoteDesktopSession;

    RemoteDesktopDestroySession(session);
    return Success;
}

static int
ProcRemoteDesktopConfigureSession(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xRemoteDesktopConfigureSessionReq);
    X_REQUEST_FIELD_CARD32(sessionId);
    X_REQUEST_FIELD_CARD16(configDataLen);

    REQUEST_FIXED_SIZE(xRemoteDesktopConfigureSessionReq, stuff->configDataLen);

    RemoteDesktopSessionPtr session = RemoteDesktopFindSession(stuff->sessionId);
    if (!session)
        return RemoteDesktopErrorBase + BadRemoteDesktopSession;

    const char *config_data = (const char *)&stuff[1];
    RemoteDesktopConfigPtr config = RemoteDesktopParseConfig(config_data, stuff->configDataLen);
    if (!config)
        return BadAlloc;

    Bool success = RemoteDesktopConfigureSession(session, config);
    RemoteDesktopFreeConfig(config);

    if (!success)
        return RemoteDesktopErrorBase + BadRemoteDesktopConfig;

    RemoteDesktopSendNotifyEvent(session, RD_NOTIFY_CONFIG_CHANGED, 0);
    return Success;
}

static int
ProcRemoteDesktopGetSessionInfo(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xRemoteDesktopGetSessionInfoReq);
    X_REQUEST_FIELD_CARD32(sessionId);

    RemoteDesktopSessionPtr session = RemoteDesktopFindSession(stuff->sessionId);
    if (!session)
        return RemoteDesktopErrorBase + BadRemoteDesktopSession;

    const char *proto_name = session->protocol ? session->protocol->name : "";
    int proto_name_len = strlen(proto_name);

    /* Serialize config for reply */
    char config_buf[1024] = { 0 };
    int config_len = 0;
    RemoteDesktopConfigPtr cfg = session->config;
    while (cfg && config_len < sizeof(config_buf) - 64) {
        int key_len = strlen(cfg->key);
        int val_len = 0;
        char *val_str = NULL;

        if (cfg->type == RD_CONFIG_TYPE_STRING && cfg->value.str_val)
            val_str = cfg->value.str_val;
        else if (cfg->type == RD_CONFIG_TYPE_INTEGER)
            val_len = snprintf(config_buf + config_len + key_len + 1,
                               sizeof(config_buf) - config_len - key_len - 1,
                               "%ld", cfg->value.int_val);
        else if (cfg->type == RD_CONFIG_TYPE_BOOLEAN)
            val_len = snprintf(config_buf + config_len + key_len + 1,
                               sizeof(config_buf) - config_len - key_len - 1,
                               "%s", cfg->value.bool_val ? "true" : "false");

        if (config_len + key_len + 1 + val_len + 1 >= sizeof(config_buf))
            break;

        memcpy(config_buf + config_len, cfg->key, key_len);
        config_len += key_len;
        config_buf[config_len++] = '=';
        if (val_str) {
            val_len = strlen(val_str);
            memcpy(config_buf + config_len, val_str, val_len);
            config_len += val_len;
        } else if (val_len > 0) {
            config_len += val_len;
        }
        config_buf[config_len++] = '\n';
        cfg = cfg->next;
    }

    xRemoteDesktopGetSessionInfoReply reply = {
        .sessionId = session->session_id,
        .screen = session->pScreen ? session->pScreen->myNum : 0,
        .protocolNameLen = proto_name_len,
        .configDataLen = config_len,
        .state = session->state,
        .connectedClients = session->connected_clients,
        .bytesSent = session->bytes_sent,
        .bytesReceived = session->bytes_received,
    };

    X_REPLY_FIELD_CARD32(sessionId);
    X_REPLY_FIELD_CARD32(screen);
    X_REPLY_FIELD_CARD16(protocolNameLen);
    X_REPLY_FIELD_CARD16(configDataLen);
    X_REPLY_FIELD_CARD32(state);
    X_REPLY_FIELD_CARD32(connectedClients);
    X_REPLY_FIELD_CARD64(bytesSent);
    X_REPLY_FIELD_CARD64(bytesReceived);

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    int rc = X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
    if (rc != Success)
        return rc;

    x_rpcbuf_write_CARD8s(&rpcbuf, (CARD8 *)proto_name, proto_name_len);
    if (config_len > 0)
        x_rpcbuf_write_CARD8s(&rpcbuf, (CARD8 *)config_buf, config_len);

    return Success;
}

static int
ProcRemoteDesktopSetSessionState(ClientPtr client)
{
    REQUEST(xRemoteDesktopSetSessionStateReq);

    X_REQUEST_FIELD_CARD32(sessionId);
    /* state is a CARD8 in the request, access directly */
    CARD8 state = ((xRemoteDesktopSetSessionStateReq *)stuff)->state;

    RemoteDesktopSessionPtr session = RemoteDesktopFindSession(stuff->sessionId);
    if (!session)
        return RemoteDesktopErrorBase + BadRemoteDesktopSession;

    if (state < RD_STATE_STOPPED || state > RD_STATE_ERROR)
        return BadValue;

    if (!RemoteDesktopSetSessionState(session, state))
        return RemoteDesktopErrorBase + BadRemoteDesktopConfig;

    return Success;
}

/* Swapped request handlers */

static int
SProcRemoteDesktopQueryVersion(ClientPtr client)
{
    REQUEST(xRemoteDesktopQueryVersionReq);
    swaps(&stuff->length);
    swapl(&stuff->majorVersion);
    swapl(&stuff->minorVersion);
    return ProcRemoteDesktopQueryVersion(client);
}

static int
SProcRemoteDesktopListProtocols(ClientPtr client)
{
    REQUEST(xRemoteDesktopListProtocolsReq);
    swaps(&stuff->length);
    return ProcRemoteDesktopListProtocols(client);
}

static int
SProcRemoteDesktopCreateSession(ClientPtr client)
{
    REQUEST(xRemoteDesktopCreateSessionReq);
    swaps(&stuff->length);
    swapl(&stuff->sessionId);
    swapl(&stuff->screen);
    swaps(&stuff->protocolNameLen);
    swaps(&stuff->configDataLen);
    return ProcRemoteDesktopCreateSession(client);
}

static int
SProcRemoteDesktopDestroySession(ClientPtr client)
{
    REQUEST(xRemoteDesktopDestroySessionReq);
    swaps(&stuff->length);
    swapl(&stuff->sessionId);
    return ProcRemoteDesktopDestroySession(client);
}

static int
SProcRemoteDesktopConfigureSession(ClientPtr client)
{
    REQUEST(xRemoteDesktopConfigureSessionReq);
    swaps(&stuff->length);
    swapl(&stuff->sessionId);
    swaps(&stuff->configDataLen);
    return ProcRemoteDesktopConfigureSession(client);
}

static int
SProcRemoteDesktopGetSessionInfo(ClientPtr client)
{
    REQUEST(xRemoteDesktopGetSessionInfoReq);
    swaps(&stuff->length);
    swapl(&stuff->sessionId);
    return ProcRemoteDesktopGetSessionInfo(client);
}

static int
SProcRemoteDesktopSetSessionState(ClientPtr client)
{
    REQUEST(xRemoteDesktopSetSessionStateReq);
    swaps(&stuff->length);
    swapl(&stuff->sessionId);
    return ProcRemoteDesktopSetSessionState(client);
}

static int
ProcRemoteDesktopDispatch(ClientPtr client)
{
    REQUEST(xReq);
    if (stuff->data < ARRAY_SIZE(ProcRemoteDesktopVector))
        return (*ProcRemoteDesktopVector[stuff->data])(client);
    return BadRequest;
}

static int
SProcRemoteDesktopDispatch(ClientPtr client)
{
    REQUEST(xReq);
    if (stuff->data < ARRAY_SIZE(SProcRemoteDesktopVector))
        return (*SProcRemoteDesktopVector[stuff->data])(client);
    return BadRequest;
}

/* ================================================================
 * Extension Init
 * ================================================================ */

void
RemoteDesktopExtensionInit(void)
{
    ExtensionEntry *extEntry;

    /* Register resource types */
    RemoteDesktopSessionType = CreateNewResourceType(FreeRemoteDesktopSession,
                                                      "RemoteDesktopSession");
    if (!RemoteDesktopSessionType)
        return;

    RemoteDesktopProtocolType = CreateNewResourceType(FreeRemoteDesktopProtocol,
                                                       "RemoteDesktopProtocol");
    if (!RemoteDesktopProtocolType)
        return;

    /* Register screen private key */
    if (!dixRegisterPrivateKey(&RemoteDesktopScreenPrivateKeyRec,
                                PRIVATE_SCREEN,
                                0))
        return;

    /* Initialize per-screen data */
    DIX_FOR_EACH_SCREEN({
        RemoteDesktopScreenInit(walkScreen);
    });

    /* Add extension */
    extEntry = AddExtension(REMOTEDESKTOP_NAME,
                            REMOTEDESKTOP_NUMBER_EVENTS,
                            REMOTEDESKTOP_NUMBER_ERRORS,
                            ProcRemoteDesktopDispatch,
                            SProcRemoteDesktopDispatch,
                            RemoteDesktopCloseDown,
                            StandardMinorOpcode);

    if (!extEntry)
        return;

    RemoteDesktopExtensionEntry = extEntry;
    RemoteDesktopReqCode = extEntry->base;
    RemoteDesktopEventBase = extEntry->eventBase;
    RemoteDesktopErrorBase = extEntry->errorBase;

    /* Set resource error values */
    SetResourceTypeErrorValue(RemoteDesktopSessionType,
                              RemoteDesktopErrorBase + BadRemoteDesktopSession);
    SetResourceTypeErrorValue(RemoteDesktopProtocolType,
                              RemoteDesktopErrorBase + BadRemoteDesktopProtocol);

    /* Register event swap */
    EventSwapVector[RemoteDesktopEventBase + RemoteDesktopNotify] =
        (EventSwapPtr)NULL; /* No swap needed for fixed-size event */

    LogMessageVerb(X_INFO, 1, "RemoteDesktop extension initialized\n");
}