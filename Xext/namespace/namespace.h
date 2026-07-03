#ifndef __XSERVER_NAMESPACE_H
#define __XSERVER_NAMESPACE_H

#include <stdio.h>
#include <X11/Xmd.h>

#include "include/dixstruct.h"
#include "include/list.h"
#include "include/privates.h"
#include "include/window.h"
#include "include/windowstr.h"

struct auth_token {
    struct xorg_list entry;
    char *authProto;
    char *authTokenData;
    size_t authTokenLen;
    XID authId;
    CARD32 handle;              /* per-namespace token handle */
};

struct Xnamespace {
    struct xorg_list entry;
    const char *name;
    Bool builtin;
    Bool allowMouseMotion;
    Bool allowShape;
    Bool allowTransparency;
    Bool allowXInput;
    Bool allowXKeyboard;
    Bool superPower;
    Bool autoRemove;           /* destroy when the last client exits */
    struct xorg_list auth_tokens;
    CARD32 tokenHandleSeq;      /* monotonic per-namespace handle counter */
    size_t refcnt;
    WindowPtr rootWindow;
};

extern struct xorg_list ns_list;
extern struct Xnamespace ns_root;
extern struct Xnamespace ns_anon;

struct XnamespaceClientPriv {
    Bool isServer;
    XID authId;
    struct Xnamespace* ns;
};

#define NS_NAME_ROOT      "root"
#define NS_NAME_ANONYMOUS "anon"

extern DevPrivateKeyRec namespaceClientPrivKeyRec;

Bool XnsLoadConfig(void);
void XnsProtoExtensionInit(void);
WindowPtr XnsCreateVirtualRoot(WindowPtr realRoot, const char *name);
struct Xnamespace *XnsFindByName(const char* name);
struct Xnamespace *XnsLookup(const char *name, size_t namelen);
int XnsAddToken(struct Xnamespace *ns, const char *proto, size_t protolen,
                const char *data, size_t datalen, CARD32 *handleOut);

/* namespace-model setter layer (config.c), shared with the protocol handlers */
struct Xnamespace *XnsCreate(const char *name, size_t namelen,
                             CARD32 caps, CARD32 attrs, int *err);
void XnsDestroyNamespace(struct Xnamespace *ns);
int  XnsDelete(struct Xnamespace *ns, CARD8 onClients);
int  XnsSetCaps(struct Xnamespace *ns, CARD32 mask, CARD32 values);
int  XnsRemoveToken(struct Xnamespace *ns, CARD32 handle);
CARD32 XnsCountTokens(struct Xnamespace *ns);
CARD32 XnsCaps(const struct Xnamespace *ns);
CARD32 XnsAttrs(const struct Xnamespace *ns);
struct Xnamespace* XnsFindByAuth(size_t szAuthProto, const char* authProto, size_t szAuthToken, const char* authToken);
void XnamespaceAssignClient(struct XnamespaceClientPriv *priv, struct Xnamespace *ns);
void XnamespaceAssignClientByName(struct XnamespaceClientPriv *priv, const char *name);

static inline struct XnamespaceClientPriv *XnsClientPriv(ClientPtr client) {
    if (client == NULL) return NULL;
    return dixLookupPrivate(&client->devPrivates, &namespaceClientPrivKeyRec);
}

static inline Bool XnsClientSameNS(struct XnamespaceClientPriv *p1, struct XnamespaceClientPriv *p2)
{
    if (!p1 && !p2)
        return TRUE;
    if (!p1 || !p2)
        return FALSE;
    return (p1->ns == p2->ns);
}

#define XNS_LOG(...) do { printf("XNS "); printf(__VA_ARGS__); } while (0)

static inline Bool streq(const char *a, const char *b)
{
    if (!a && !b)
        return TRUE;
    if (!a || !b)
        return FALSE;
    return (strcmp(a,b) == 0);
}

#endif /* __XSERVER_NAMESPACE_H */
