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

struct client_token {
    struct xorg_list entry;
    char *clientName;
    struct Xnamespace *Designation;
};

struct Xns_perm_list {
    Bool allowComposite;
    Bool allowGlobalKeyboard;
    Bool allowMouseMotion;
    Bool allowRandr;
    Bool allowRender;
    Bool allowScreen;
    Bool allowShape;
    Bool allowTransparency;
    Bool allowXInput;
    Bool allowXKeyboard;
};

struct xns_pid_entry {
    struct xorg_list entry;
    int pid;
};

struct Xnamespace {
    struct xorg_list entry;
    Bool builtin;
    Bool deny;                      // connection deny flag. should stay unused.
    Bool isRoot;                    // only ever used by root namespace
    Bool superPower;
    WindowPtr rootWindow;
    const char *name;
    size_t refcnt;
    struct Xns_perm_list perms;
    struct xorg_list auth_tokens;
    struct xorg_list pids;
};

extern struct xorg_list client_list;
extern struct xorg_list ns_list;
extern struct Xnamespace ns_root;
extern struct Xnamespace ns_anon;
extern struct Xnamespace *ns_default;

struct XnamespaceClientPriv {
    Bool isServer;
    XID authId;
    int pid;
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
XID GenerateAuthForXnamespace(struct Xnamespace *curr);
int RevokeAuthForXnamespace(struct Xnamespace *curr);
int XnamespaceAssignByClientName(struct XnamespaceClientPriv *subj, const char *clientName);
struct Xnamespace *GenerateNewXnamespaceForClient(struct Xnamespace *copyfrom, const char* newname);
void NewVirtualRootWindowForXnamespace(WindowPtr rootWindow, struct Xnamespace *curr);
int DeleteXnamespace(struct Xnamespace *curr);
void PrintXnamespaces(void);
int PruneXnamespaces(void);
char** GetXnamespacesAsCharr (void);
void XnsRegisterPid(struct XnamespaceClientPriv *subj);
void XnsRemovePid(struct XnamespaceClientPriv *subj);
int XnsAssignByPid(struct XnamespaceClientPriv *subj);

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
