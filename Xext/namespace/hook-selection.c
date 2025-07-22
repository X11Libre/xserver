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

    /* no rewrite if client is in root namespace */
    if (subj->ns->superPower)
        return;

    const char *origSelectionName = NameForAtom(param->selection);

    char selname[PATH_MAX] = { 0 };
    snprintf(selname, sizeof(selname)-1, "<%s>%s", subj->ns->name, origSelectionName);
    Atom realSelection = dixAddAtom(selname);

    switch (param->op) {
        case SELECTION_FILTER_GETOWNER:
        case SELECTION_FILTER_SETOWNER:
        case SELECTION_FILTER_CONVERT:
        case SELECTION_FILTER_LISTEN:
            // TODO: check whether window really belongs to the client
            param->selection = realSelection;
        break;

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
