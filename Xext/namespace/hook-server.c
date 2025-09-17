#define HOOK_NAME "server"

#include <dix-config.h>

#include "dix/dix_priv.h"
#include "dix/registry_priv.h"
#include "dix/server_priv.h"
#include "Xext/xacestr.h"

#include "namespace.h"
#include "hooks.h"

void hookServerAccess(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XNS_HOOK_HEAD(ServerAccessCallbackParam);

    if (subj->ns->superPower)
        goto pass;

    switch (client->majorOp) {
        case X_ListFonts:
        case X_ListFontsWithInfo:
            goto pass;

        case X_GrabServer:
            goto reject;
    }

    XNS_HOOK_LOG("BLOCKED access to server configuration request %s\n",
        LookupRequestName(client->majorOp, client->minorOp));

reject:
    param->status = BadAccess;
    return;

pass:
    param->status = Success;
}
