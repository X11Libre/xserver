/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later */
/* Copyright (C) 2026 Enrico Weigelt, metux IT consult <info@metux.net> */
#define HOOK_NAME "resource"

#include <dix-config.h>

#include <inttypes.h>
#include <X11/extensions/XI2proto.h>
#include <X11/extensions/randrproto.h>

#include "dix/dix_priv.h"
#include "dix/extension_priv.h"
#include "dix/registry_priv.h"
#include "dix/window_priv.h"
#include "Xext/xacestr.h"

#include "namespace.h"
#include "hooks.h"

static int checkAllowed(Mask requested, Mask allowed) {
    return ((requested & allowed) == requested);
}

void hookResourceAccess(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XNS_HOOK_HEAD(XaceResourceAccessRec);
    ClientPtr owner = dixLookupXIDOwner(param->id);
    struct XnamespaceClientPriv *obj = XnsClientPriv(owner);

    // server can do anything
    if (param->client == serverClient)
        goto pass;

    // no restriction on super power
    if (subj->ns->superPower)
        goto pass;

    // special filtering for windows: block transparency for untrusted clients
    if (param->rtype == X11_RESTYPE_WINDOW) {
        WindowPtr pWindow = (WindowPtr) param->res;
        if (param->access_mode & DixCreateAccess) {
            if (!subj->ns->allowTransparency) {
                pWindow->forcedBG = TRUE;
            }
        }
    }

    // resource access inside same namespace is always permitted
    if (XnsClientSameNS(subj, obj))
        goto pass;

    // check for root windows (screen or ns-virtual)
    if (param->rtype == X11_RESTYPE_WINDOW) {
        WindowPtr pWindow = (WindowPtr) param->res;

        /* Namespace's own virtual root window: allow everything except destroy */
        if (pWindow == subj->ns->rootWindow) {
            if (!(param->access_mode & DixDestroyAccess))
                goto pass;
            goto reject;
        }

        /* Delegate SendEvent checks to hookSend so disallowed events are
           dropped cleanly by ProcSendEvent without raising a fatal BadAccess */
        if (client->majorOp == X_SendEvent &&
            checkAllowed(param->access_mode, DixSendAccess))
            goto pass;

        /* Operations on the actual screen root window */
        if (pWindow && (pWindow == pWindow->drawable.pScreen->root)) {
            switch (client->majorOp) {
                case X_GetImage:
                case X_CopyArea:
                case X_CopyPlane:
                case EXTENSION_MAJOR_COMPOSITE:
                case EXTENSION_MAJOR_DAMAGE:
                    goto reject;

                case EXTENSION_MAJOR_SHM:
                    if (param->access_mode & DixReadAccess)
                        goto reject;
                    break;

                case X_QueryTree:
                    goto pass;

                case X_QueryPointer:
                case X_WarpPointer:
                    if (subj->ns->allowMouseMotion)
                        goto pass;
                    goto reject;

                case EXTENSION_MAJOR_XINPUT:
                    if (client->minorOp == X_XIQueryPointer ||
                        client->minorOp == X_XIWarpPointer) {
                        if (subj->ns->allowMouseMotion)
                            goto pass;
                        goto reject;
                    }
                    break;
            }

            /* Allow non-destructive queries, event subscriptions, and top-level
               window creation/reparent/removal on the root window; block
               attribute modification (DixSetAttrAccess/DixWriteAccess) and WM
               redirection (DixManageAccess). */
            if (checkAllowed(param->access_mode,
                             DixReadAccess | DixGetAttrAccess | DixGetPropAccess |
                             DixListPropAccess | DixReceiveAccess | DixAddAccess |
                             DixRemoveAccess | DixUseAccess))
                goto pass;

            goto reject;
        }

        /* Selection owner inspecting the requestor window (toolkits such as
           GTK wrap it via GetWindowAttributes/GetGeometry/QueryTree before
           answering), writing converted data, or sending SelectionNotify to it */
        if (obj && XnsClipboardTargetAllowed(obj->ns) &&
            XnsSelectionTransferAllowed(subj->ns, pWindow->drawable.id, None)) {
            if (client->majorOp == X_QueryTree &&
                checkAllowed(param->access_mode, DixListAccess))
                goto pass;
            if (checkAllowed(param->access_mode,
                             DixGetAttrAccess |
                             DixSetPropAccess | DixGetPropAccess |
                             DixReceiveAccess | DixSendAccess))
                goto pass;
        }

        /* Host system tray manager window (_NET_SYSTEM_TRAY_S0 owner): allow
           visual/orientation property queries, StructureNotify subscription,
           and SYSTEM_TRAY_REQUEST_DOCK ClientMessage delivery (checked in hookSend). */
        if (XnsIsSystemTrayWindow(pWindow)) {
            if (client->majorOp == X_QueryTree &&
                checkAllowed(param->access_mode, DixListAccess))
                goto pass;
            if (checkAllowed(param->access_mode,
                             DixGetAttrAccess | DixGetPropAccess |
                             DixListPropAccess | DixReceiveAccess | DixSendAccess))
                goto pass;
        }

        /* Active host XDND source window: allow target sandbox to inspect
           XdndTypeList/XdndActionList and send XdndStatus/XdndFinished replies. */
        if (pWindow && XnsHasActiveXdndSource(subj->ns, pWindow->drawable.id) &&
            obj && obj->ns && obj->ns->superPower &&
            XnsClipboardTargetAllowed(subj->ns)) {
            if (checkAllowed(param->access_mode,
                             DixGetAttrAccess | DixGetPropAccess |
                             DixReceiveAccess | DixSendAccess))
                goto pass;
        }
    }

    /* Server-owned shared resources (colormaps, fonts, cursors, Render formats, Sync counters) */
    if (obj->isServer) {
        if (param->rtype == X11_RESTYPE_WINDOW) {
            XNS_HOOK_LOG("REJECT server owned window 0x%0lx!\n", (unsigned long)((WindowPtr)param->res)->drawable.id);
            goto reject;
        }

        /* ProcRRSelectInput attaches its per-window subscriber list (RREventType)
           to rootWindow->drawable.id (owned by serverClient) and looks it up with
           DixWriteAccess right after checking DixReceiveAccess on the window. */
        if (client->majorOp == EXTENSION_MAJOR_RANDR &&
            client->minorOp == X_RRSelectInput &&
            checkAllowed(param->access_mode, DixWriteAccess))
            goto pass;

        if (checkAllowed(param->access_mode,
                         DixReadAccess | DixGetPropAccess | DixListPropAccess |
                         DixUseAccess | DixGetAttrAccess | DixListAccess |
                         DixAddAccess | DixRemoveAccess | DixReceiveAccess))
            goto pass;
    }

reject:
    if (xnsDebugLog) {
        char accModeStr[128];
        LookupDixAccessName(param->access_mode, (char*)&accModeStr, sizeof(accModeStr));

        XNS_HOOK_LOG("BLOCKED access 0x%07lx %s to %s 0x%06lx of client %d @ %s\n",
            (unsigned long)param->access_mode,
            accModeStr,
            LookupResourceName(param->rtype),
            (unsigned long)param->id,
            owner ? owner->index : -1,
            (obj && obj->ns) ? obj->ns->name : "<none>");
    }

    param->status = BadAccess;
    return;

pass:
    // request is passed as it is (or already had been rewritten)
    param->status = Success;
}
