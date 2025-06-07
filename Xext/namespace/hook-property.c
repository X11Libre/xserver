#define HOOK_NAME "property"

#include <dix-config.h>

#include <stdio.h>

#include "dix/dix_priv.h"
#include "dix/registry_priv.h"
#include "include/propertyst.h"
#include "Xext/xacestr.h"

#include "_mRNA.h"
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
    struct X_mRNAClientPriv *obj = XnsClientPriv(dixClientForWindow(param->pWin));

    ATOM name = (*param->ppProp)->propertyName;

    if (XnsClientSameNS(subj, obj))
        return;

    if (param->pWin == subj->ns->rootWindow)
        return;

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
}
