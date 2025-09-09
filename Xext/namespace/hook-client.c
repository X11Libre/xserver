#define HOOK_NAME "client"

#include <dix-config.h>

#include "dix/dix_priv.h"
#include "dix/extension_priv.h"
#include "dix/registry_priv.h"

#include "Xext/xacestr.h"

#include "namespace.h"
#include "hooks.h"

void hookClient(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XNS_HOOK_HEAD(XaceClientAccessRec);
    struct XnamespaceClientPriv *obj = XnsClientPriv(param->target);

    if (subj->ns->superPower || XnsClientSameNS(subj, obj))
        return;

    XNS_HOOK_LOG("BLOCKED access on client %d\n", param->target->index);

    /* returning BadValue instead of BadAccess, because we're pretending
       the requested client doens't even exist at all. */
    param->status = BadValue;
}
