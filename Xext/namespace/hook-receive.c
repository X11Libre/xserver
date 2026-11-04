/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later */
/* Copyright (C) 2026 Enrico Weigelt, metux IT consult <info@metux.net> */
#define HOOK_NAME "recieve"

#include <dix-config.h>

#include <X11/Xmd.h>
#include <X11/extensions/randr.h>

#include "dix/extension_priv.h"
#include "dix/registry_priv.h"
#include "dix/resource_priv.h"
#include "Xext/xacestr.h"
#include "Xext/randr/randrstr_priv.h"

#include "namespace.h"
#include "hooks.h"

static inline Bool isRootWin(WindowPtr pWin) {
    return (pWin->parent == NullWindow && dixClientForWindow(pWin) == serverClient);
}

void
hookReceive(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XNS_HOOK_HEAD(XaceReceiveAccessRec);
    struct XnamespaceClientPriv *obj = XnsClientPriv(dixClientForWindow(param->pWin));

    /* Events originating on a per-namespace virtual root window (owned by
       serverClient and parented to the screen root) are private to that
       namespace, even for superPower clients whose rootWindow is realRoot. */
    if (param->pWin && param->pWin != subj->ns->rootWindow &&
        param->pWin->parent && isRootWin(param->pWin->parent) &&
        dixClientForWindow(param->pWin) == serverClient)
        goto reject;

    // superPower clients (e.g. host WM) can receive any event
    if (subj->ns->superPower)
        goto pass;

    /* Grant a short input-focus ticket on genuine physical button/key events
       delivered to one of this namespace's own windows (e.g. clicking a tray
       icon or unfocused window to open a popup menu or claim focus). */
    if (XnsClientSameNS(subj, obj) && param->pWin && !isRootWin(param->pWin)) {
        for (int i = 0; i < param->count; i++) {
            if ((param->events[i].u.u.type & 0x80) == 0 &&
                XnsIsInputEvent(&param->events[i])) {
                XnsGrantInputFocusTicket(subj->ns);
                break;
            }
        }
    }

    /* Grant paste access on genuine (non-SendEvent) physical key/button press
       delivered to a focused namespace in auto-clipboard mode */
    if (subj->ns->allowClipboardAuto && XnsNamespaceHasFocus(subj->ns)) {
        for (int i = 0; i < param->count; i++) {
            if ((param->events[i].u.u.type & 0x80) == 0 &&
                XnsIsPressEvent(&param->events[i])) {
                XnsGrantPasteTicket(subj->ns);
                break;
            }
        }
    }

    // send and receive within same namespace permitted without restrictions
    if (XnsClientSameNS(subj, obj))
        goto pass;

    for (int i=0; i<param->count; i++) {
        const int type = param->events[i].u.u.type & 0x7f;
        switch (type) {
            case SelectionNotify:
            case SelectionRequest:
            case SelectionClear:
                if (obj && XnsClipboardAllowed(subj->ns) && XnsClipboardAllowed(obj->ns))
                    continue;
                goto reject;

            case ConfigureNotify:
                if (param->pWin && (isRootWin(param->pWin) || param->pWin == subj->ns->rootWindow))
                    continue;
                goto reject;

            case DestroyNotify:
                if (param->pWin && XnsIsSystemTrayWindow(param->pWin))
                    continue;
                goto reject;

            case PropertyNotify:
                if (param->pWin && param->pWin == subj->ns->rootWindow)
                    continue;
                if (param->pWin && isRootWin(param->pWin) &&
                    !XnsIsSensitiveRootProperty(subj->ns,
                                                param->events[i].u.property.atom))
                    continue;
                /* Needed by INCR selection owners watching target property deletion */
                if (obj && param->pWin && XnsClipboardTargetAllowed(obj->ns) &&
                    XnsSelectionTransferAllowed(subj->ns, param->pWin->drawable.id,
                                                param->events[i].u.property.atom))
                    continue;
                goto reject;

            case GenericEvent: {
                xGenericEvent *gev = (xGenericEvent*)&param->events[i].u;
                if (gev->extension == EXTENSION_MAJOR_XINPUT) {
                    switch (gev->evtype) {
                        case XI_RawMotion:
                        case XI_RawButtonPress:
                        case XI_RawButtonRelease:
                            if ((!subj->ns->allowMouseMotion) || !isRootWin(param->pWin) ||
                                !XnsNamespaceHasFocus(subj->ns))
                                goto reject;
                            continue;
                        case XI_DeviceChanged:
                        case XI_HierarchyChanged:
                        case XI_PropertyEvent:
                            if ((!subj->ns->allowXInput) || !isRootWin(param->pWin))
                                goto reject;
                            continue;
                        case XI_RawKeyPress:
                        case XI_RawKeyRelease:
                            goto reject;
                        default:
                            XNS_HOOK_LOG("XI unknown %d\n", gev->evtype);
                            goto reject;
                    }
                }
                if (gev->extension == EXTENSION_MAJOR_PRESENT)
                    continue;
                XNS_HOOK_LOG("BLOCKED #%d generic event extension=%d\n", i, gev->extension);
                goto reject;
            }
            break;

            default:
                if (RREventBase &&
                    (type == RREventBase + RRScreenChangeNotify ||
                     type == RREventBase + RRNotify) &&
                    param->pWin &&
                    (isRootWin(param->pWin) || param->pWin == subj->ns->rootWindow))
                    continue;
                XNS_HOOK_LOG("BLOCKED event type #%d 0%0x 0%0x %s %s%s\n", i, type, param->events[i].u.u.detail,
                    LookupEventName(type), (type & 128) ? "fake" : "",
                    isRootWin(param->pWin) ? " (root window)" : "");
                goto reject;
            break;
        }
    }

pass:
    return;

reject:
    param->status = BadAccess;
    XNS_HOOK_LOG("BLOCKED client %d [NS %s] receiving event sent to window 0x%lx of client %d [NS %s]\n",
        client->index,
        subj->ns->name,
        (unsigned long)param->pWin->drawable.id,
        dixClientForWindow(param->pWin)->index,
        (obj && obj->ns) ? obj->ns->name : "<none>");
    return;
}
