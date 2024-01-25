
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

static bool isRootWin(WindowPtr win) {
    return (win->parent == NullWindow && wClient(win)->index == 0);
}

#include "exglobals.h"

void
handleReceive(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XaceReceiveAccessRec *rec = calldata;
    ClientPtr client = rec->client;
    struct client_priv *subj = clientPriv(client);
    struct client_priv *obj = clientPriv(wClient(rec->pWin));

    // send and receive within same namespace permitted without restrictions
    if (clientSameNS(subj, obj))
        return;

    for (int i=0; i<rec->count; i++) {
        int type = rec->events[i].u.u.type;
        switch (type) {
            case GenericEvent:
                xGenericEvent *gev = &rec->events[i].u;
                if (gev->extension == IReqCode) {
                    switch (gev->evtype) {
                        case XI_RawMotion:
                            if ((!subj->ns->allow_mouse_motion) || (!isRootWin(rec->pWin)))
                                rec->status = BadAccess;
                            continue;
                        default:
                            rec->status = BadAccess;
                            printf("XI unknown %d\n", gev->evtype);
                    }
                }
            break;

            default:
                printf("event type 0%0x 0%0x %s %s\n", type, rec->events[i].u.u.detail,
                    LookupEventName(type), (type & 128) ? "fake" : "");
                printf("====> default case\n");
                rec->status = BadAccess;

                if (isRootWin(rec->pWin))
                    printf("receiving from root window\n");
                else
                    printf("not from root window\n");
            break;
        }
    }

    if (rec->status == BadAccess) {
        printf("BLOCKED [receive] client %d receiving event sent to window 0x%lx of client %d\n",
            client->index,
            (unsigned long)rec->pWin->drawable.id,
            wClient(rec->pWin)->index);
    }
}
