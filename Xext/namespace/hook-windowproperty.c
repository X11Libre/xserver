#define HOOK_NAME "windowproperty"

#include <dix-config.h>

#include <inttypes.h>
#include <X11/Xmd.h>

#include "dix/dix_priv.h"
#include "dix/property_priv.h"
#include "dix/window_priv.h"

#include "namespace.h"
#include "hooks.h"

void hookWindowProperty(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XNS_HOOK_HEAD(PropertyFilterParam);
    const ClientPtr owner = dixLookupXIDOwner(param->window);
    struct XnamespaceClientPriv *obj = XnsClientPriv(owner);
    /* no redirect on super power
     * whitelist anything that goes to caller's own namespace
     */
    if (subj->ns->superPower || XnsClientSameNS(subj, obj))
        return;
    /* skip redirect. these expose a lot of information but WMs hold the info. */
    if (obj->ns->isRoot) {
        switch(client->majorOp) {
            case X_GetProperty:
                return;
        }
    }

    if (!owner) {
        param->status = BadWindow;
        param->skip = TRUE;
        XNS_HOOK_LOG("owner of window 0x%0llx doesn't exist\n", (unsigned long long)param->window);
        return;
    }

    /* allow access to namespace virtual root */
    if (param->window == subj->ns->rootWindow->drawable.id)
        return;

    /* redirect root window access to namespace's virtual root */
    if (dixWindowIsRoot(param->window)) {
        param->window = subj->ns->rootWindow->drawable.id;
        return;
    }
}
