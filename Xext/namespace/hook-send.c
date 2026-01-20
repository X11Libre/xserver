#define HOOK_NAME "send"

#include <dix-config.h>

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
    if (clientAllowedOnClient(client, targetClient))
        goto pass;

    for (int i = 0; i < param->count; i++) {
        const int type = param->events[i].u.u.type;
        // catch messages for root namespace
        if (obj->ns->isRoot) {
            const char* evname = LookupEventName(type);
            if (strcmp(evname,LookupEventName(ClientMessage))==0)
                goto pass;
            if (strcmp(evname,LookupEventName(UnmapNotify))==0)
                goto pass;
        }

        XNS_HOOK_LOG("BLOCK target @ %s\n", obj->ns->name);
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
