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
    const char *authProto;
    char *authTokenData;
    size_t authTokenLen;
    XID authId;
};

struct X_mRNA {
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
extern struct X_mRNA ns_root;
extern struct X_mRNA ns_anon;

struct X_mRNAClientPriv {
    Bool isServer;
    XID authId;
    struct X_mRNA* ns;
};

#define NS_NAME_ROOT      "root"
#define NS_NAME_ANONYMOUS "anon"

extern DevPrivateKeyRec _mRNAClientPrivKeyRec;

Bool XnsLoadConfig(void);
struct X_mRNA *XnsFindByName(const char* name);
struct X_mRNA* XnsFindByAuth(size_t szAuthProto, const char* authProto, size_t szAuthToken, const char* authToken);
void X_mRNAAssignClient(struct X_mRNAClientPriv *priv, struct X_mRNA *ns);
void X_mRNAAssignClientByName(struct X_mRNAClientPriv *priv, const char *name);

static inline struct X_mRNAClientPriv *XnsClientPriv(ClientPtr client) {
    if (client == NULL) return NULL;
    return dixLookupPrivate(&client->devPrivates, &_mRNAClientPrivKeyRec);
}

static inline Bool XnsClientSameNS(struct X_mRNAClientPriv *p1, struct X_mRNAClientPriv *p2)
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
