/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later */
/* Copyright (C) 2026 Enrico Weigelt, metux IT consult <info@metux.net> */
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

    if (subj->ns->superPower)
        return;

    if (XnsClientSameNS(subj, obj))
        return;

    if (param->pWin == subj->ns->rootWindow)
        return;

    /* Read-only access to non-sensitive properties on the real root window
       (filtered by hookWindowProperty) or system tray manager window. */
    if (((winIsRoot(param->pWin) && !XnsIsSensitiveRootProperty(subj->ns, name)) ||
         XnsIsSystemTrayWindow(param->pWin)) &&
        ((param->access_mode & ~(DixReadAccess | DixGetAttrAccess)) == 0))
        return;

    /* Active host XDND source window: read-only access to XdndTypeList / XdndActionList */
    if (param->pWin && XnsHasActiveXdndSource(subj->ns, param->pWin->drawable.id) &&
        obj && obj->ns && obj->ns->superPower &&
        XnsClipboardTargetAllowed(subj->ns) &&
        XnsIsXdndProperty(name) &&
        ((param->access_mode & ~(DixReadAccess | DixGetAttrAccess)) == 0))
        return;

    /* Selection owner writing the requested target property onto requestor window */
    if (obj && client->majorOp == X_ChangeProperty &&
        ((param->access_mode & ~(DixWriteAccess | DixCreateAccess |
                                 DixBlendAccess | DixPostAccess)) == 0) &&
        XnsClipboardTargetAllowed(obj->ns) &&
        XnsSelectionTransferAllowed(subj->ns, param->pWin->drawable.id, name))
        return;

    XNS_HOOK_LOG("BLOCKED access to property %s (atom 0x%x) window 0x%lx of client %d\n",
        NameForAtom(name),
        name,
        (unsigned long)param->pWin->drawable.id,
        dixClientForWindow(param->pWin)->index);
    param->status = BadAccess;
}
