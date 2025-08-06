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

    // no redirect on super power
    if (subj->ns->superPower)
        return;

    const ClientPtr owner = dixLookupXIDOwner(param->window);
    if (!owner) {
        param->status = BadWindow;
        param->skip = TRUE;
        XNS_HOOK_LOG("owner of window 0x%0" PRIx32 "doesn't exist\n", param->window);
        return;
    }

    // whitelist anything that goes to caller's own namespace
    struct XnamespaceClientPriv *obj = XnsClientPriv(owner);
    if (XnsClientSameNS(subj, obj))
        return;

    // allow access to namespace virtual root
    if (param->window == subj->ns->rootWindow->drawable.id)
        return;

    // redirect root window access to namespace's virtual root
    if (dixWindowIsRoot(param->window)) {
        param->window = subj->ns->rootWindow->drawable.id;
        return;
    }
}
