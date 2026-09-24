/*
 * RemoteDesktop Extension Private Definitions
 *
 * SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 */

#ifndef _REMOTEDESKTOP_PRIV_H_
#define _REMOTEDESKTOP_PRIV_H_

#include <stdbool.h>
#include <X11/Xdefs.h>

#include "dix/dix_priv.h"
#include "dix/resource_priv.h"
#include "dix/callback_priv.h"
#include "include/callback.h"
#include "include/regionstr.h"
#include "include/windowstr.h"
#include "include/scrnintstr.h"
#include "Xext/remotedesktop/remotedesktopproto.h"
#include "Xext/damage/damageext_priv.h"

#define REMOTEDESKTOP_NAME "X-REMOTE-DESKTOP"

/* Forward declarations */
typedef struct _RemoteDesktopProtocol RemoteDesktopProtocolRec, *RemoteDesktopProtocolPtr;
typedef struct _RemoteDesktopSession RemoteDesktopSessionRec, *RemoteDesktopSessionPtr;
typedef struct _RemoteDesktopScreenPrivate RemoteDesktopScreenPrivateRec, *RemoteDesktopScreenPrivatePtr;
typedef struct _RemoteDesktopConfig RemoteDesktopConfigRec, *RemoteDesktopConfigPtr;

/* Protocol capability flags (match remotedesktopproto.h) */
#define RD_CAP_AUTH_NONE         (1 << 0)
#define RD_CAP_AUTH_PASSWORD     (1 << 1)
#define RD_CAP_AUTH_TLS_CERT     (1 << 2)
#define RD_CAP_ENCRYPTION_TLS    (1 << 3)
#define RD_CAP_ENCODING_RAW      (1 << 4)
#define RD_CAP_ENCODING_HEXTILE  (1 << 5)
#define RD_CAP_ENCODING_ZRLE     (1 << 6)
#define RD_CAP_ENCODING_TIGHT    (1 << 7)
#define RD_CAP_VIEW_ONLY         (1 << 8)
#define RD_CAP_CLIPBOARD         (1 << 9)
#define RD_CAP_FILE_TRANSFER     (1 << 10)
#define RD_CAP_AUDIO             (1 << 11)
#define RD_CAP_USB_REDIRECTION   (1 << 12)

/* Session states */
#define RD_STATE_STOPPED         0
#define RD_STATE_RUNNING         1
#define RD_STATE_ERROR           2

/* Notify event subtypes */
#define RD_NOTIFY_CLIENT_CONNECTED      0
#define RD_NOTIFY_CLIENT_DISCONNECTED   1
#define RD_NOTIFY_CONFIG_CHANGED        2
#define RD_NOTIFY_ERROR                 3

/* Config value types */
#define RD_CONFIG_TYPE_STRING    0
#define RD_CONFIG_TYPE_INTEGER   1
#define RD_CONFIG_TYPE_BOOLEAN   2

/* Config entry for protocol-specific configuration */
struct _RemoteDesktopConfig {
    char *key;
    int type;                    /* RD_CONFIG_TYPE_* */
    union {
        char *str_val;
        int64_t int_val;
        bool bool_val;
    } value;
    struct _RemoteDesktopConfig *next;
};

/* Protocol backend interface */
struct _RemoteDesktopProtocol {
    const char *name;                    /* "VNC", "RDP" */
    const char *vendor;
    uint32_t version;
    uint32_t capabilities;

    /* Lifecycle */
    Bool (*Init)(ScreenPtr pScreen, RemoteDesktopProtocolPtr self,
                 RemoteDesktopConfigPtr config);
    void (*Fini)(ScreenPtr pScreen, RemoteDesktopProtocolPtr self);
    Bool (*Start)(ScreenPtr pScreen, RemoteDesktopProtocolPtr self);
    void (*Stop)(ScreenPtr pScreen, RemoteDesktopProtocolPtr self);
    Bool (*Reconfigure)(ScreenPtr pScreen, RemoteDesktopProtocolPtr self,
                        RemoteDesktopConfigPtr config);

    /* Damage notification - called when screen damage occurs */
    void (*DamageNotify)(RemoteDesktopProtocolPtr self, RegionPtr pRegion);

    /* Framebuffer access - get current framebuffer pointer/stride/bpp */
    Bool (*GetFramebuffer)(ScreenPtr pScreen, RemoteDesktopProtocolPtr self,
                           void **fb_addr, int *stride, int *bpp);

    /* Client management */
    void (*ClientConnected)(RemoteDesktopProtocolPtr self, int client_fd);
    void (*ClientDisconnected)(RemoteDesktopProtocolPtr self, int client_fd);

    /* Private data for backend */
    void *private;

    /* List linkage */
    struct xorg_list entry;
};

/* Session object */
struct _RemoteDesktopSession {
    XID session_id;
    ScreenPtr pScreen;
    RemoteDesktopProtocolPtr protocol;
    RemoteDesktopConfigPtr config;
    int state;                         /* RD_STATE_* */
    DamagePtr pDamage;                 /* Damage object for dirty tracking */
    uint32_t connected_clients;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    char *error_message;               /* If state == RD_STATE_ERROR */

    /* List linkage for screen's session list */
    struct xorg_list entry;
};

/* Screen private data */
struct _RemoteDesktopScreenPrivate {
    struct xorg_list protocol_list;    /* Registered protocols for this screen */
    struct xorg_list session_list;     /* Active sessions on this screen */
    DamagePtr pDamage;                 /* Shared damage object for root window */
    RegionRec last_damage;             /* Last reported damage region */
    Bool damage_active;                /* Whether damage tracking is active */

    /* Wrapped CloseScreen for cleanup */
    CloseScreenProcPtr CloseScreen;

    /* Framebuffer cache */
    void *framebuffer;
    int fb_stride;
    int fb_bpp;
    Bool fb_valid;
};

/* Resource types */
extern RESTYPE RemoteDesktopSessionType;
extern RESTYPE RemoteDesktopProtocolType;

/* Private key for screen private data */
extern DevPrivateKeyRec RemoteDesktopScreenPrivateKeyRec;
#define RemoteDesktopScreenPrivateKey (&RemoteDesktopScreenPrivateKeyRec)

/* Protocol registration */
Bool RemoteDesktopRegisterProtocol(RemoteDesktopProtocolPtr protocol);
void RemoteDesktopUnregisterProtocol(RemoteDesktopProtocolPtr protocol);
RemoteDesktopProtocolPtr RemoteDesktopFindProtocol(const char *name);

/* Session management */
RemoteDesktopSessionPtr RemoteDesktopCreateSession(ScreenPtr pScreen,
                                                    XID session_id,
                                                    RemoteDesktopProtocolPtr protocol,
                                                    RemoteDesktopConfigPtr config);
void RemoteDesktopDestroySession(RemoteDesktopSessionPtr session);
RemoteDesktopSessionPtr RemoteDesktopFindSession(XID session_id);
Bool RemoteDesktopConfigureSession(RemoteDesktopSessionPtr session,
                                    RemoteDesktopConfigPtr config);
Bool RemoteDesktopSetSessionState(RemoteDesktopSessionPtr session, int state);

/* Damage handling */
void RemoteDesktopDamageReport(DamagePtr pDamage, RegionPtr pRegion, void *closure);
void RemoteDesktopDamageDestroy(DamagePtr pDamage, void *closure);
Bool RemoteDesktopSetupDamage(ScreenPtr pScreen, RemoteDesktopSessionPtr session);
void RemoteDesktopTeardownDamage(RemoteDesktopSessionPtr session);

/* Configuration parsing */
RemoteDesktopConfigPtr RemoteDesktopParseConfig(const char *data, int len);
void RemoteDesktopFreeConfig(RemoteDesktopConfigPtr config);
char *RemoteDesktopConfigGetString(RemoteDesktopConfigPtr config, const char *key);
int64_t RemoteDesktopConfigGetInt(RemoteDesktopConfigPtr config, const char *key);
bool RemoteDesktopConfigGetBool(RemoteDesktopConfigPtr config, const char *key);

/* Event sending */
void RemoteDesktopSendNotifyEvent(RemoteDesktopSessionPtr session,
                                   int subtype, int detail);

/* Screen private access */
static inline RemoteDesktopScreenPrivatePtr
RemoteDesktopGetScreenPrivate(ScreenPtr pScreen)
{
    return dixGetPrivate(&pScreen->devPrivates, RemoteDesktopScreenPrivateKey);
}

static inline void
RemoteDesktopSetScreenPrivate(ScreenPtr pScreen, RemoteDesktopScreenPrivatePtr priv)
{
    dixSetPrivate(&pScreen->devPrivates, RemoteDesktopScreenPrivateKey, priv);
}

/* Extension entry point */
extern ExtensionEntry *RemoteDesktopExtensionEntry;
extern int RemoteDesktopReqCode;
extern int RemoteDesktopEventBase;
extern int RemoteDesktopErrorBase;

#endif /* _REMOTEDESKTOP_PRIV_H_ */