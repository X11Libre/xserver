/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later */
/* Copyright (C) 2026 Enrico Weigelt, metux IT consult <info@metux.net> */
#define HOOK_NAME "selection"

#include <dix-config.h>

#include <stdio.h>

#include "dix/dix_priv.h"
#include "dix/selection_priv.h"

#include "namespace.h"
#include "hooks.h"

static inline const char *stripNS(const char* name) {
    if ((!name) || (name[0] != '<'))
        return name; // can this ever happen ?
    const char *got = strchr(name, '>');
    if (!got)
        return name;
    return ++got;
}

/*
 * This hook is rewriting the client visible selection names to internally used,
 * per namespace ones. Whenever a client is asking for a selection, it's name
 * is replaced by a namespaced one, e.g. asking for "PRIMARY" while being in
 * namespace "foo" will become "<foo>PRIMARY"
 *
 * A malicious client could still send specially crafted messages to others,
 * asking them to send their selection data to him. This needs to be solved
 * separately, by a send hook.
 */
void hookSelectionFilter(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XNS_HOOK_HEAD(SelectionFilterParamRec);

    /* When the server dispatches SelectionRequest to a confined selection owner,
       authorize that owner to write only the requested (requestor, property) pair.
       When a confined sandbox requests a file-URI target from a host owner, route
       the SelectionRequest through the namespace's URI helper if registered. */
    if (param->op == SELECTION_FILTER_EV_REQUEST) {
        struct XnamespaceClientPriv *ownerPriv = XnsClientPriv(param->recvClient);
        if (ownerPriv && ownerPriv->ns && !ownerPriv->ns->superPower &&
            !XnsClientSameNS(subj, ownerPriv) &&
            XnsClipboardAllowed(subj->ns) && XnsClipboardAllowed(ownerPriv->ns)) {
            Atom prop = (param->property != None) ? param->property : param->target;
            XnsGrantSelectionTransfer(ownerPriv->ns, param->selection, param->requestor, prop);
        } else if (!subj->ns->superPower && ownerPriv && ownerPriv->ns &&
                   ownerPriv->ns->superPower && XnsIsUriTarget(param->target)) {
            ClientPtr helperClient = XnsFindUriHelperClient(subj->ns);
            if (helperClient && helperClient != param->recvClient)
                param->recvClient = helperClient;
        }
        return;
    }

    /* Never allow non-superPower clients to claim or inspect internal _XNS_* selections */
    if (!subj->ns->superPower && XnsIsInternalSelection(param->selection)) {
        const char *orig = NameForAtom(param->selection);
        if (orig && (param->op == SELECTION_FILTER_GETOWNER ||
                     param->op == SELECTION_FILTER_SETOWNER ||
                     param->op == SELECTION_FILTER_CONVERT ||
                     param->op == SELECTION_FILTER_LISTEN)) {
            char selname[PATH_MAX] = { 0 };
            snprintf(selname, sizeof(selname) - 1, "<%s>%s", subj->ns->name, orig);
            param->selection = dixAddAtom(selname);
        }
        return;
    }

    /* no rewrite if client is in root namespace or has unrestricted shared clipboard */
    if (subj->ns->superPower || subj->ns->allowClipboard) {
        if (param->op == SELECTION_FILTER_SETOWNER && param->owner != None) {
            XnsRevokeAllPasteTickets(subj->ns);
            if (XnsIsXdndSelection(param->selection))
                XnsClearXdndSource(None);
        }
        if (param->op == SELECTION_FILTER_NOTIFY) {
            const char *orig = NameForAtom(param->selection);
            const char *stripped = stripNS(orig);
            if (stripped && stripped != orig)
                param->selection = dixAddAtom(stripped);
        }
        return;
    }

    /* Allow confined clients to discover and watch host WM, compositor, and
       system tray manager selections without allowing them to take ownership */
    if ((param->op == SELECTION_FILTER_GETOWNER ||
         param->op == SELECTION_FILTER_LISTEN) &&
        XnsIsDesktopSelection(param->selection))
        return;

    /* Smart auto-clipboard mode (focus-gated copy + physical press while focused) */
    if (subj->ns->allowClipboardAuto) {
        switch (param->op) {
            case SELECTION_FILTER_SETOWNER:
                if (XnsNamespaceHasFocus(subj->ns)) {
                    if (param->owner != None) {
                        XnsRevokeAllPasteTickets(subj->ns);
                        if (XnsIsXdndSelection(param->selection))
                            XnsClearXdndSource(None);
                    }
                    return;
                }
                if (param->owner == None &&
                    XnsOwnsGlobalSelection(subj->ns, param->selection))
                    return;
                break;
            case SELECTION_FILTER_GETOWNER:
            case SELECTION_FILTER_CONVERT:
                if (XnsHasPasteTicket(subj->ns) ||
                    XnsOwnsGlobalSelection(subj->ns, param->selection) ||
                    (XnsHasActiveXdndSource(subj->ns, None) &&
                     XnsIsXdndSelection(param->selection)))
                    return;
                break;
            case SELECTION_FILTER_LISTEN:
                return;
            default:
                break;
        }
    }

    const char *origSelectionName = NameForAtom(param->selection);
    if (!origSelectionName)
        return;

    switch (param->op) {
        case SELECTION_FILTER_GETOWNER:
        case SELECTION_FILTER_SETOWNER:
        case SELECTION_FILTER_CONVERT:
        case SELECTION_FILTER_LISTEN:
        {
            // TODO: check whether window really belongs to the client
            char selname[PATH_MAX] = { 0 };
            snprintf(selname, sizeof(selname)-1, "<%s>%s", subj->ns->name, origSelectionName);
            param->selection = dixAddAtom(selname);
            break;
        }

        case SELECTION_FILTER_NOTIFY:
        {
            // need to translate back, since we're having the ns-prefixed name here
            const char *stripped = stripNS(origSelectionName);
            param->selection = dixAddAtom(stripped);
            break;
        }

        // nothing to do here: already having the client visible name
        case SELECTION_FILTER_EV_REQUEST:
        case SELECTION_FILTER_EV_CLEAR:
        break;
    }
}
