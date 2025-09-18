#ifndef __XSERVER_NAMESPACE_HOOKS_H
#define __XSERVER_NAMESPACE_HOOKS_H

#include "dix/registry_priv.h"
#include "include/misc.h"

#include "namespace.h"

#define XNS_HOOK_LOG(...) do { \
        LogMessage(X_INFO, "XNS [" HOOK_NAME "] (#%d@%d) {%s} <%s>: ", \
            (client ? client->index : -1), \
            (client ? client->sequence : -1), \
            (subj ? (subj->ns ? subj->ns->name : "(no ns)") : "<no client>"), \
            LookupRequestName(client ? client->majorOp : 0, \
                              client ? client->minorOp : 0)); \
        LogMessage(X_INFO, __VA_ARGS__); \
    } while (0)

#define XNS_HOOK_HEAD(t) \
    t *param = calldata; \
    ClientPtr client = param->client; \
    if (!client) { \
        /* XNS_LOG("hook %s NULL client\n", HOOK_NAME); */ \
    } \
    struct XnamespaceClientPriv *subj = XnsClientPriv(client);

void hookClient(CallbackListPtr *pcbl, void *unused, void *calldata);
void hookClientState(CallbackListPtr *pcbl, void *unused, void *calldata);
void hookDevice(CallbackListPtr *pcbl, void *unused, void *calldata);
void hookExtAccess(CallbackListPtr *pcbl, void *unused, void *calldata);
void hookExtDispatch(CallbackListPtr *pcbl, void *unused, void *calldata);
void hookInitRootWindow(CallbackListPtr *pcbl, void *unused, void *calldata);
void hookPropertyAccess(CallbackListPtr *pcbl, void *unused, void *calldata);
void hookReceive(CallbackListPtr *pcbl, void *unused, void *calldata);
void hookResourceAccess(CallbackListPtr *pcbl, void *unused, void *calldata);
void hookSelectionFilter(CallbackListPtr *pcbl, void *unused, void *calldata);
void hookSend(CallbackListPtr *pcbl, void *unused, void *calldata);
void hookServerAccess(CallbackListPtr *pcbl, void *unused, void *calldata);
void hookWindowProperty(CallbackListPtr *pcbl, void *unused, void *calldata);

#endif /* __XSERVER_NAMESPACE_HOOKS_H */
