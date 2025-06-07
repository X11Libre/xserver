#define HOOK_NAME "clienstate"

#include <dix-config.h>

#include "dix/registry_priv.h"
#include "os/client_priv.h"
#include "os/auth.h"

#include "_mRNA.h"
#include "hooks.h"

void hookClientState(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XNS_HOOK_HEAD(NewClientInfoRec);

    switch (client->clientState) {
    case ClientStateInitial:
        // better assign *someting* than null -- clients can't do anything yet anyways
        X_mRNAAssignClient(subj, &ns_anon);
        break;

    case ClientStateRunning:
        subj->authId = AuthorizationIDOfClient(client);

        short unsigned int name_len = 0, data_len = 0;
        const char * name = NULL;
        char * data = NULL;
        if (AuthorizationFromID(subj->authId, &name_len, &name, &data_len, &data)) {
            X_mRNAAssignClient(subj, XnsFindByAuth(name_len, name, data_len, data));
        } else {
            XNS_HOOK_LOG("no auth data - assuming anon\n");
        }
        break;

    case ClientStateRetained:
        X_mRNAAssignClient(subj, NULL);
        break;
    case ClientStateGone:
        X_mRNAAssignClient(subj, NULL);
        break;
    default:
        XNS_HOOK_LOG("unknown state =%d\n", client->clientState);
        break;
    }
}
