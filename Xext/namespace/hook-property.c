#define HOOK_NAME "property"

#include <dix-config.h>

#include <stdio.h>

#include "dix/dix_priv.h"
#include "dix/registry_priv.h"
#include "include/propertyst.h"
#include "Xext/xacestr.h"

#include "namespace.h"
#include "hooks.h"

static inline Bool winIsRoot(WindowPtr pWin) {
    if (!pWin)
        return FALSE;
    if (pWin->drawable.pScreen->root == pWin)
        return TRUE;
    return FALSE;
}

void hookPropertyAccess(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XNS_HOOK_HEAD(XacePropertyAccessRec);
    struct XnamespaceClientPriv *obj = XnsClientPriv(dixClientForWindow(param->pWin));

    ATOM name = (*param->ppProp)->propertyName;

    if (subj->ns->superPower || XnsClientSameNS(subj, obj))
        goto pass;

    if (param->pWin == subj->ns->rootWindow)
        goto pass;

    // Whitelisted atoms - potentially a global allow tag?
    if (obj->ns->isRoot) {
        switch (client->majorOp) {
            case X_GetProperty:
                // TODO: turn this mess into a switch? they're mostly xcb atoms - tricky
                const char* atomNameTest = NameForAtom(name);
                // can this expose anything?
                if (strcmp("_NET_WORKAREA", atomNameTest)==0)
                    goto pass;
                // questionable
                if (strcmp("_NET_ACTIVE_WINDOW", atomNameTest)==0)
                    goto pass;
                // harmless?
                if (strcmp("WM_NAME", atomNameTest)==0)
                    goto pass;
                if (strcmp("_NET_WM_NAME", atomNameTest)==0)
                    goto pass;
                if (strcmp("WM_CLASS", atomNameTest)==0)
                    goto pass;
                if (strcmp("WM_STATE", atomNameTest)==0)
                    goto pass;
                if (strcmp("_NET_SUPPORTING_WM_CHECK", atomNameTest)==0)
                    goto pass;
                if (strcmp("_NET_SUPPORTED", atomNameTest)==0)
                    goto pass;
                // we already whitelist X_QueryTree, these do the same thing
                if (strcmp("_NET_CLIENT_LIST", atomNameTest)==0)
                    goto pass;
                if (strcmp("_NET_CLIENT_LIST_STACKING", atomNameTest)==0)
                    goto pass;
        }
    }

    if (winIsRoot(param->pWin)) {
        XNS_HOOK_LOG("window is the screen's root window\n");
    } else {
        XNS_HOOK_LOG("not a root window\n");
    }

    XNS_HOOK_LOG("access to property %s (atom 0x%x) window 0x%lx of client %d\n",
        NameForAtom(name),
        name,
        (unsigned long)param->pWin->drawable.id,
        dixClientForWindow(param->pWin)->index);
    param->status = BadAccess;
    return;
pass:
    param->status = Success;
    return;
}
