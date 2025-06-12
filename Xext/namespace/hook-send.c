#define HOOK_NAME "send"

#include <dix-config.h>

#include "dix/registry_priv.h"
#include "dix/resource_priv.h"
#include "Xext/xacestr.h"

#include "_mRNA.h"
#include "hooks.h"

/* TRUE if subj client is allowed to do things on obj)
 * usually if they're in the same _mRNA or subj is in a parent
 * _mRNA of obj
 */
static Bool clientAllowedOnClient(ClientPtr subj, ClientPtr obj) {
    struct X_mRNAClientPriv *subjPriv = XnsClientPriv(subj);
    struct X_mRNAClientPriv *objPriv = XnsClientPriv(obj);

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
    struct X_mRNAClientPriv *obj = XnsClientPriv(targetClient);
    if (clientAllowedOnClient(client, targetClient))
        goto pass;

    XNS_HOOK_LOG("BLOCK target @ %s\n", obj->ns->name);
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
