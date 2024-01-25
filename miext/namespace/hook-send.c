
// 2do: set namespace properties on windows

#include <dix-config.h>

#include <stdio.h> // printf // FIXME: use Xorg debug functions
#include <stdbool.h>

#include "windowstr.h"
#include "propertyst.h"
#include "xacestr.h"
#include "registry.h"
#include "extinit.h"
#include "extnsionst.h"
#include "protocol-versions.h"

#include "namespace.h"
#include "clientpriv.h"
#include "hooks.h"

void hookSend(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XaceSendAccessRec *rec = calldata;
    ClientPtr client = rec->client;
    struct client_priv *subj = clientPriv(client);
    struct client_priv *obj = clientPriv(wClient(rec->pWin));

    if (client == NULL) {
        printf("hookSend() client is NULL\n");
        return;
    }

//        if (SecurityDoCheck(subj, obj, DixSendAccess, 0) == Success)
//            return;
//
//        for (i = 0; i < rec->count; i++)
//            if (rec->events[i].u.u.type != UnmapNotify &&
//                rec->events[i].u.u.type != ConfigureRequest &&
//                rec->events[i].u.u.type != ClientMessage) {
//
//                SecurityAudit("Security: denied client %d from sending event "
//                              "of type %s to window 0x%lx of client %d\n",
//                              rec->client->index,
//                              LookupEventName(rec->events[i].u.u.type),
//                              (unsigned long)rec->pWin->drawable.id,
//                              wClient(rec->pWin)->index);
//                rec->status = BadAccess;
//                return;
//            }

    if (clientSameNS(subj, obj)) {
        printf("[send] same namespace\n");
    }

    for (int i = 0; i < rec->count; i++) {
        printf("[send] client %d sending event of type %s to window 0x%lx of client %d op %s\n",
            client->index,
            LookupEventName(rec->events[i].u.u.type),
            (unsigned long)rec->pWin->drawable.id,
            wClient(rec->pWin)->index,
            LookupRequestName(client->majorOp, client->minorOp));
    }
}
