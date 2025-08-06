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
    struct xorg_list auth_tokens;
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
struct Xnamespace *XnsFindByName(const char* name);
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
