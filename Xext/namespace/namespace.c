#include <dix-config.h>

#include <stdio.h>
#include <X11/Xmd.h>

#include "dix/client_priv.h"
#include "dix/devices_priv.h"
#include "dix/dix_priv.h"
#include "dix/extension_priv.h"
#include "dix/property_priv.h"
#include "dix/selection_priv.h"
#include "dix/server_priv.h"
#include "dix/window_priv.h"
#include "include/os.h"
#include "miext/extinit_priv.h"
#include "Xext/xacestr.h"
#include "os/mitauth.h"
#include "os/client_priv.h"

#include <X11/Xatom.h>

#include "namespace.h"
#include "hooks.h"

Bool noNamespaceExtension = TRUE;

DevPrivateKeyRec namespaceClientPrivKeyRec = { 0 };

void
NamespaceExtensionInit(void)
{
    XNS_LOG("initializing namespace extension ...\n");

    /* load configuration */
    if (!XnsLoadConfig()) {
        XNS_LOG("No config file. disabling Xns extension\n");
        return;
    }

    if (!(dixRegisterPrivateKey(&namespaceClientPrivKeyRec, PRIVATE_CLIENT,
            sizeof(struct XnamespaceClientPriv)) &&
          AddCallback(&ClientStateCallback, hookClientState, NULL) &&
          AddCallback(&PostInitRootWindowCallback, hookInitRootWindow, NULL) &&
          AddCallback(&PropertyFilterCallback, hookWindowProperty, NULL) &&
          AddCallback(&SelectionFilterCallback, hookSelectionFilter, NULL) &&
          AddCallback(&ExtensionAccessCallback, hookExtAccess, NULL) &&
          AddCallback(&ExtensionDispatchCallback, hookExtDispatch, NULL) &&
          AddCallback(&ServerAccessCallback, hookServerAccess, NULL) &&
          AddCallback(&ClientDestroyCallback, hookClientDestroy, NULL) &&
          AddCallback(&ClientAccessCallback, hookClient, NULL) &&
          AddCallback(&DeviceAccessCallback, hookDevice, NULL) &&
          XaceRegisterCallback(XACE_PROPERTY_ACCESS, hookPropertyAccess, NULL) &&
          XaceRegisterCallback(XACE_RECEIVE_ACCESS, hookReceive, NULL) &&
          XaceRegisterCallback(XACE_RESOURCE_ACCESS, hookResourceAccess, NULL) &&
          XaceRegisterCallback(XACE_SEND_ACCESS, hookSend, NULL)))
        FatalError("NamespaceExtensionInit: allocation failure\n");

    /* Do the serverClient */
    struct XnamespaceClientPriv *srv = XnsClientPriv(serverClient);
    *srv = (struct XnamespaceClientPriv) { .isServer = TRUE };
    XnamespaceAssignClient(srv, &ns_root);
}

void XnamespaceAssignClient(struct XnamespaceClientPriv *priv, struct Xnamespace *newns)
{
    if (priv->ns != NULL)
        priv->ns->refcnt--;

    priv->ns = newns;

    if (newns != NULL)
        newns->refcnt++;
}

void XnamespaceAssignClientByName(struct XnamespaceClientPriv *priv, const char *name)
{
    struct Xnamespace *newns = XnsFindByName(name);

    if (newns == NULL)
        newns = &ns_anon;

    XnamespaceAssignClient(priv, newns);
}

struct Xnamespace* XnsFindByAuth(size_t szAuthProto, const char* authProto, size_t szAuthToken, const char* authToken)
{
    struct Xnamespace *walk;
    xorg_list_for_each_entry(walk, &ns_list, entry) {
        struct auth_token *at;
        xorg_list_for_each_entry(at, &walk->auth_tokens, entry) {
            int protoLen = at->authProto ? strlen(at->authProto) : 0;
            if ((protoLen == szAuthProto) &&
                (at->authTokenLen == szAuthToken) &&
                (memcmp(at->authTokenData, authToken, szAuthToken)==0) &&
                (memcmp(at->authProto, authProto, szAuthProto)==0))
                return walk;
        }
    }

    // default to anonymous if credentials aren't assigned to specific NS
    return &ns_anon;
}

XID GenerateAuthForXnamespace(struct Xnamespace *curr) {
    struct auth_token *gen_token = calloc(1, sizeof(struct auth_token));
    if (gen_token == NULL)
        FatalError("Xnamespace: failed allocating token\n");
    gen_token->authProto = strdup(XAUTH_PROTO_MIT);
    gen_token->authTokenLen = 16;
    gen_token->authTokenData = calloc(1, gen_token->authTokenLen);

    // GenerateAuthorization() logic not needed, this is the only important line
    arc4random_buf(gen_token->authTokenData, gen_token->authTokenLen);

    gen_token->authId = AddAuthorization(strlen(gen_token->authProto),
                                         gen_token->authProto,
                                         gen_token->authTokenLen,
                                         gen_token->authTokenData);

    xorg_list_append_ndup(&gen_token->entry, &curr->auth_tokens);

    XNS_LOG("Generated Auth Token ");
    for (int i=0; i<gen_token->authTokenLen; i++)
        printf("%02X", (unsigned char)gen_token->authTokenData[i]);
    printf(" For %s\n",curr->name);
    return gen_token->authId;
}

int RevokeAuthForXnamespace(struct Xnamespace *curr) {
    struct auth_token *auth_token_walk;
    struct auth_token *auth_token_tmp;
    int k = 0;
    xorg_list_for_each_entry_safe(auth_token_walk, auth_token_tmp, &curr->auth_tokens, entry) {
        XNS_LOG("Revoking key");
        printf(" auth: \"%s\" \"", auth_token_walk->authProto);
        for (int i=0; i<auth_token_walk->authTokenLen; i++)
            printf("%02X", (unsigned char)auth_token_walk->authTokenData[i]);
        printf("\" for namespace %s\n",curr->name);
        if (RemoveAuthorization(
                strlen(auth_token_walk->authProto),
                       auth_token_walk->authProto,
                       auth_token_walk->authTokenLen,
                       auth_token_walk->authTokenData)!=0) {
            xorg_list_del(&auth_token_walk->entry);
            free(auth_token_walk->authProto);
            free(auth_token_walk->authTokenData);
            free(auth_token_walk);
            k++;
        }
    }
    if (k!=0)
        return k;
    return 0;
}

int XnamespaceAssignByClientName(struct XnamespaceClientPriv *subj, const char *clientName) {
    struct client_token *c_walk;
    struct auth_token *auth_token_walk;

    xorg_list_for_each_entry(c_walk, &client_list, entry) {
        // test for the name if it doesn't exist yet
        if (strcmp(clientName, c_walk->clientName) == 0) {
            XNS_LOG("%s matching ns found: %s\n",clientName,c_walk->Designation->name);
            XnamespaceAssignClient(subj, c_walk->Designation);
            // assign with first found token
            xorg_list_for_each_entry(auth_token_walk, &c_walk->Designation->auth_tokens, entry) {
                subj->authId = auth_token_walk->authId;
                // (at least one) auth found for namespace, pass it on to the priv
                return Success;
            }
            XNS_LOG("No auth tokens for namespace, client still assigned\n");
            return Success;
        }
    }
    // failed to find a match
    return 1;
}

struct Xnamespace *GenerateNewXnamespaceForClient(struct Xnamespace *copyfrom, const char* newname) {
    struct Xnamespace *new_run_ns = calloc(1, sizeof(struct Xnamespace));
    if (new_run_ns==NULL) {
        XNS_LOG("Failed to alloc new namespace");
        return NULL;
    }
    if (copyfrom!=NULL) {
        new_run_ns->perms   = copyfrom->perms;}
    new_run_ns->builtin = FALSE; // just in case
    // "fancy" formatting for namespace names

    XNS_LOG("New Namespace Creation : %s\n",newname);

    new_run_ns->name = strdup(newname);

    NewVirtualRootWindowForXnamespace(ns_root.rootWindow, new_run_ns);

    xorg_list_append_ndup(&new_run_ns->entry, &ns_list);
    return new_run_ns;
}

void PrintXnamespaces(void) {
    struct Xnamespace *walk;
    XNS_LOG("Namespaces: ");
    xorg_list_for_each_entry(walk, &ns_list, entry) {
        printf("%s, ",walk->name);
    }
    printf("\n");
}

int DeleteXnamespace(struct Xnamespace *curr) {
    if (curr->refcnt==0 && curr->builtin == 0) {
        // non-critical. auth is optional?
        if (RevokeAuthForXnamespace(curr)==0) {
            XNS_LOG("DELETE: No auth or failed to revoke auth\n"); }
        XNS_LOG("Deleting namespace: %s\n", curr->name);
        // critical? namespaces should always have windows so this should always pass
        DeleteWindow(curr->rootWindow, ns_root.rootWindow->drawable.id);
        xorg_list_del(&curr->entry);
        // no references to this namespace
        free((char*)curr->name);
        free(curr);
        return Success;
    }
    return 1;
}

int PruneXnamespaces(void) {
    struct Xnamespace *walk;
    struct Xnamespace *hold;
    xorg_list_for_each_entry_safe(walk, hold, &ns_list, entry) {
        if (walk->refcnt == 0 && walk->builtin == 0) {
            XNS_LOG("pruning empty non-retained namespace %s\n",walk->name);
            if (DeleteXnamespace(walk)!=0) {
                // this shouldn't fail
                XNS_LOG("failed to delete namespace %s\n",walk->name);
            }
        }
    }
    return 0;
}

struct Xnamespace *XnsFindByName(const char* name) {
    struct Xnamespace *walk;
    xorg_list_for_each_entry(walk, &ns_list, entry) {
        if (strcmp(walk->name, name)==0)
            return walk;
    }
    return NULL;
}

char** GetXnamespacesAsCharr (void) {
    // needs to walk twice since the size of the list isn't stored anywhere
    int count = 0;
    struct Xnamespace *walk;
    xorg_list_for_each_entry(walk, &ns_list, entry) {count++;}
    char **ns = calloc(1+ count, sizeof(char*));
    count = 0; // reset
    xorg_list_for_each_entry(walk, &ns_list, entry) {ns[count++] = strdup(walk->name);}
    ns[count] = NULL; // null terminated
    return ns;
}

void XnsRegisterPid(struct XnamespaceClientPriv *subj) {
    struct xns_pid_entry *pid_entry = calloc(1, sizeof(struct xns_pid_entry));
    pid_entry->pid = subj->pid;
    xorg_list_append(&pid_entry->entry, &subj->ns->pids);
    return;
}
void XnsRemovePid(struct XnamespaceClientPriv *subj) {
    struct xns_pid_entry *pWalk, *pTemp;
    xorg_list_for_each_entry_safe(pWalk, pTemp, &subj->ns->pids, entry) {
        if(subj->pid==pWalk->pid) {
            xorg_list_del(&pWalk->entry);
            free(pWalk);
        }
    }
}
int XnsAssignByPid(struct XnamespaceClientPriv *subj) {
    struct xns_pid_entry *entry_pWalk;
    struct Xnamespace *pWalk;
    xorg_list_for_each_entry(pWalk, &ns_list, entry) {
        xorg_list_for_each_entry(entry_pWalk, &pWalk->pids, entry) {
            if(subj->pid==entry_pWalk->pid) {
                XnamespaceAssignClient(subj, pWalk);
                struct auth_token *auth_token_walk;
                xorg_list_for_each_entry(auth_token_walk, &pWalk->auth_tokens, entry) {
                    subj->authId = auth_token_walk->authId;
                    return Success;
                }
                // fallback to simple return
                return Success;
            }
        }
    }
    return 1;
}
