/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later */
/* Copyright (C) 2026 Enrico Weigelt, metux IT consult <info@metux.net> */
#define HOOK_NAME "send"

#include <dix-config.h>

#include "dix/dix_priv.h"
#include "dix/registry_priv.h"
#include "dix/resource_priv.h"
#include "Xext/xacestr.h"

#include "namespace.h"
#include "hooks.h"

/* TRUE if subj client is allowed to do things on obj)
 * usually if they're in the same namespace or subj is in a parent
 * namespace of obj
 */
static Bool clientAllowedOnClient(ClientPtr subj, ClientPtr obj) {
    struct XnamespaceClientPriv *subjPriv = XnsClientPriv(subj);
    struct XnamespaceClientPriv *objPriv = XnsClientPriv(obj);

    if (subjPriv && subjPriv->ns->superPower)
        return TRUE;

    return XnsClientSameNS(subjPriv, objPriv);
}

void hookSend(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XNS_HOOK_HEAD(XaceSendAccessRec);

    /* if no sending client, then it's coming internally from the server itself */
    if (!client)
        goto pass;

    ClientPtr targetClient = dixClientForWindow(param->pWin);
    struct XnamespaceClientPriv *obj = XnsClientPriv(targetClient);

    /* If a superPower host client (e.g. WM or hotkey bridge) sends a press
       key/button event to a focused auto-clipboard sandbox, grant paste access */
    if (subj->ns->superPower && obj && obj->ns && obj->ns->allowClipboardAuto &&
        XnsNamespaceHasFocus(obj->ns)) {
        for (int i = 0; i < param->count; i++) {
            if (XnsIsPressEvent(&param->events[i])) {
                XnsGrantPasteTicket(obj->ns);
                break;
            }
        }
    }

    /* Track host-initiated XDND sessions into a confined sandbox. */
    if (subj->ns->superPower && obj && obj->ns && !XnsClientSameNS(subj, obj) &&
        !obj->ns->superPower && XnsClipboardTargetAllowed(obj->ns)) {
        for (int i = 0; i < param->count; i++) {
            if ((param->events[i].u.u.type & 0x7f) != ClientMessage)
                continue;
            const char *msgName = NameForAtom(param->events[i].u.clientMessage.u.l.type);
            if (!msgName)
                continue;
            Window srcWin = (Window)param->events[i].u.clientMessage.u.l.longs0;
            if (streq(msgName, "XdndEnter") || streq(msgName, "XdndPosition") ||
                streq(msgName, "XdndDrop")) {
                ClientPtr srcOwner = dixLookupXIDOwner(srcWin);
                struct XnamespaceClientPriv *srcPriv = XnsClientPriv(srcOwner);
                if (srcWin != None && srcPriv && srcPriv->ns && srcPriv->ns->superPower)
                    obj->ns->xdndSourceWin = srcWin;
            } else if (streq(msgName, "XdndLeave")) {
                if (srcWin == obj->ns->xdndSourceWin)
                    obj->ns->xdndSourceWin = None;
            }
        }
    }

    if (clientAllowedOnClient(client, targetClient))
        goto pass;

    if (param->pWin && param->pWin == subj->ns->rootWindow)
        goto pass;

    /* Allow SelectionNotify from selection owner to active requestor window */
    if (obj && param->pWin && XnsClipboardTargetAllowed(obj->ns) &&
        XnsSelectionTransferAllowed(subj->ns, param->pWin->drawable.id, None)) {
        Bool allSelectionNotify = TRUE;
        for (int i = 0; i < param->count; i++) {
            if ((param->events[i].u.u.type & 0x7f) != SelectionNotify) {
                allSelectionNotify = FALSE;
                break;
            }
        }
        if (allSelectionNotify)
            goto pass;
    }

    /* Allow XdndStatus and XdndFinished replies from the target sandbox to the
       active host XDND source window. */
    if (param->pWin && XnsHasActiveXdndSource(subj->ns, param->pWin->drawable.id) &&
        obj && obj->ns && obj->ns->superPower &&
        XnsClipboardTargetAllowed(subj->ns)) {
        Bool allAllowedXdnd = TRUE;
        Bool clearSource = FALSE;
        for (int i = 0; i < param->count; i++) {
            if ((param->events[i].u.u.type & 0x7f) != ClientMessage) {
                allAllowedXdnd = FALSE;
                break;
            }
            const char *msgName = NameForAtom(param->events[i].u.clientMessage.u.l.type);
            if (!msgName || (!streq(msgName, "XdndStatus") && !streq(msgName, "XdndFinished"))) {
                allAllowedXdnd = FALSE;
                break;
            }
            XID targetWin = (XID)param->events[i].u.clientMessage.u.l.longs0;
            ClientPtr targetOwner = dixLookupXIDOwner(targetWin);
            if (!targetOwner || !XnsClientSameNS(subj, XnsClientPriv(targetOwner))) {
                allAllowedXdnd = FALSE;
                break;
            }
            if (streq(msgName, "XdndFinished"))
                clearSource = TRUE;
        }
        if (allAllowedXdnd) {
            if (clearSource)
                subj->ns->xdndSourceWin = None;
            goto pass;
        }
    }

    /* Allow _NET_SYSTEM_TRAY_OPCODE (SYSTEM_TRAY_REQUEST_DOCK) and balloon
       message data ClientMessages to the host system tray manager window when
       the docked icon window belongs to the sender's own namespace. */
    if (param->pWin && XnsIsSystemTrayWindow(param->pWin)) {
        Atom trayOpcode = MakeAtom("_NET_SYSTEM_TRAY_OPCODE", 23, FALSE);
        Atom trayMsgData = MakeAtom("_NET_SYSTEM_TRAY_MESSAGE_DATA", 29, FALSE);
        Bool allAllowedTrayMsg = TRUE;
        for (int i = 0; i < param->count; i++) {
            int type = param->events[i].u.u.type & 0x7f;
            if (type != ClientMessage) {
                allAllowedTrayMsg = FALSE;
                break;
            }
            Atom msgType = param->events[i].u.clientMessage.u.l.type;
            if (trayOpcode != BAD_RESOURCE && msgType == trayOpcode) {
                XID iconWin = (XID)param->events[i].u.clientMessage.u.l.longs2;
                ClientPtr iconOwner = dixLookupXIDOwner(iconWin);
                if (!iconOwner || !XnsClientSameNS(subj, XnsClientPriv(iconOwner))) {
                    allAllowedTrayMsg = FALSE;
                    break;
                }
                continue;
            }
            if (trayMsgData != BAD_RESOURCE && msgType == trayMsgData)
                continue;
            allAllowedTrayMsg = FALSE;
            break;
        }
        if (allAllowedTrayMsg)
            goto pass;
    }

    /* Allow EWMH / startup-notification ClientMessage sent to the real root
       window when its window field references a window in the sender's own
       namespace or the namespace's virtual root window */
    if (param->pWin && param->pWin == param->pWin->drawable.pScreen->root) {
        Bool allOwnClientMsg = TRUE;
        for (int i = 0; i < param->count; i++) {
            if ((param->events[i].u.u.type & 0x7f) != ClientMessage) {
                allOwnClientMsg = FALSE;
                break;
            }
            Window targetWin = param->events[i].u.clientMessage.window;
            if (subj->ns->rootWindow && targetWin == subj->ns->rootWindow->drawable.id)
                continue;
            ClientPtr winOwner = dixLookupXIDOwner(targetWin);
            if (!winOwner || !XnsClientSameNS(subj, XnsClientPriv(winOwner))) {
                allOwnClientMsg = FALSE;
                break;
            }
        }
        if (allOwnClientMsg)
            goto pass;
    }

    XNS_HOOK_LOG("BLOCK target @ %s\n", (obj && obj->ns) ? obj->ns->name : "<none>");
    for (int i = 0; i < param->count; i++) {
        XNS_HOOK_LOG("sending event of type %s to window 0x%lx of client %d\n",
            LookupEventName(param->events[i].u.u.type),
            (unsigned long)param->pWin->drawable.id,
            targetClient->index);
    }

    param->status = BadAccess;
    return;

pass:
    param->status = Success;
    return;
}
