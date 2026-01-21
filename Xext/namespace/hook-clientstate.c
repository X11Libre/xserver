#define HOOK_NAME "clienstate"

#include <dix-config.h>
#include <libgen.h>

#include "dix/registry_priv.h"
#include "os/client_priv.h"
#include "os/auth.h"

#include "namespace.h"
#include "hooks.h"

void hookClientState(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XNS_HOOK_HEAD(NewClientInfoRec);

    switch (client->clientState) {
    case ClientStateInitial: {
        /* nothing can happen in this state. auth cookie can't be loaded from env yet */
        subj->pid = GetClientPid(client);
        break;
    }
    case ClientStateRunning: {
        /* just get actual name instead of path or command flags */
        const char *clientName = strtok(basename((char*)GetClientCmdName(client)), " ");

        XNS_LOG("pid: %d\n",GetClientPid(client));

        subj->authId = AuthorizationIDOfClient(client);
        /* only change if uninitialized from client name walk (0) */
        if (subj->authId==0 && XnamespaceAssignByClientName(subj,clientName)==0) {
            XnsRegisterPid(subj);
            return;
        }

        /* check env (XAUTHORITY) */
        short unsigned int name_len = 0, data_len = 0;
        const char * name = NULL;
        char * data = NULL;
        if (AuthorizationFromID(subj->authId, &name_len, &name, &data_len, &data)) {
            XnamespaceAssignClient(subj, XnsFindByAuth(name_len, name, data_len, data));
        } /* midpoint - we're not in the client lists nor do we have auth from env */
        else if(XnsAssignByPid(subj)==0) {
            /* processes can connect multiple times, keep them together */
            return;
        }
        else if (ns_default->deny) {
            XNS_LOG("Deny Connection Request From %s\n",clientName);
            client->noClientException = -1;
            return;
        }
        else if (!ns_default->builtin) {
            /* builtin flag should only be unset if the config is set as new_ns
             * "fancy" formatted name
             */
            int len = snprintf(NULL, 0, "%s%d", clientName, client->index);
            char *str = malloc(len + 1);
            snprintf(str, len+1, "%s%d", clientName, client->index);

            struct Xnamespace *new_run_ns = GenerateNewXnamespaceForClient(&ns_anon, str);
            free(str);
            if (new_run_ns!=NULL) {
                subj->authId = GenerateAuthForXnamespace(new_run_ns);
                XnamespaceAssignClient(subj, new_run_ns);}
            else {
                XNS_LOG("Failed to assign new namespace, assigning to anon\n");
                XnamespaceAssignClient(subj,&ns_anon);
            }
        }
        /* authId should ONLY be 0 at this point if it's truly unauthorized */
        if (subj->authId!=0) {
            XnsRegisterPid(subj);
            return;
        }
        XNS_HOOK_LOG("No Auth, Assigning to default %s\n",ns_default->name);
        /* if we end up here, there is no auth - dump to the default */
        XnamespaceAssignClient(subj,ns_default);
        XnsRegisterPid(subj);
        break;
    }
    case ClientStateRetained:
        break;
    case ClientStateGone:
        break;
    default:
        XNS_HOOK_LOG("unknown state =%d\n", client->clientState);
        break;
    }
}

void hookClientDestroy(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    ClientPtr client = calldata;
    struct XnamespaceClientPriv *subj = XnsClientPriv(client);

    if (!subj)
        return; /* no XNS devprivate assigned ? */
    XnsRemovePid(subj);
    if(subj->ns->builtin==0) {
        if (subj->ns->refcnt==1) {
            /* this was the last client in the (new by default) namespace */
            subj->ns->refcnt--;
            /* Delete function checks for 0 client references */
            if (DeleteXnamespace(subj->ns)!=0)
                XNS_LOG("Failed to delete namespace\n");
            subj->ns = NULL;
            return;
        } else {
            /* don't delete, clients are still connected */
        }
    }
    XnamespaceAssignClient(subj, NULL);
    /* the devprivate is embedded, so no free() necessary */
}
