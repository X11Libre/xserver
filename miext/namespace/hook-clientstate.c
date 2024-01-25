
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

/*
 * Looks up a request name
 */
static inline const char *
ContainerLookupRequestName(ClientPtr client)
{
    return LookupRequestName(client->majorOp, client->minorOp);
}

void hookClientState(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    NewClientInfoRec *pci = calldata;
    ClientPtr client = pci->client;
    struct client_priv *subj = clientPriv(client);
    int clientId = client->index;

    switch (client->clientState) {
    case ClientStateInitial:
        // better assign *someting* than null -- clients can't do anything yet anyways
        clientAssignNS(subj, &namespaces[NS_ANONYMOUS]);
        break;

    case ClientStateRunning:
        subj->authId = AuthorizationIDOfClient(client);

        short unsigned int name_len = 0, data_len = 0;
        const char * name = NULL;
        char * data = NULL;
        if (AuthorizationFromID(subj->authId, &name_len, &name, &data_len, &data))
            clientAssignNS(subj, findNSByAuth2(name_len, name, data_len, data));

        if (subj->ns)
            printf("[client RUNNING] id=%d namespace %s\n", clientId, subj->ns->id);
        else
            printf("[client RUNNING] id=%d without namespace\n", clientId);

        break;

    case ClientStateRetained:
        printf("[client RETAINED] id=%d\n", clientId);
        clientAssignNS(subj, NULL);
        break;
    case ClientStateGone:
        printf("[client GONE] id=%d\n", clientId);
        clientAssignNS(subj, NULL);
        break;
    default:
        printf("[client UNKNOWN] id=%d state=%d\n", clientId, client->clientState);
        break;
    }
}
