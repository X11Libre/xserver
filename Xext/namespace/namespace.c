#include <dix-config.h>

#include <stdio.h>
#include <X11/Xmd.h>

#include "dix/dix_priv.h"
#include "dix/property_priv.h"
#include "dix/selection_priv.h"
#include "include/os.h"
#include "miext/extinit_priv.h"
#include "Xext/xacestr.h"

#include "_mRNA.h"
#include "hooks.h"

Bool noNamespaceExtension = TRUE;

DevPrivateKeyRec _mRNAClientPrivKeyRec = { 0 };

void
NamespaceExtensionInit(void)
{
    XNS_LOG("initializing _mRNA extension ...\n");

    /* load configuration */
    if (!XnsLoadConfig()) {
        XNS_LOG("No config file. disabling Xns extension\n");
        return;
    }

    if (!(dixRegisterPrivateKey(&_mRNAClientPrivKeyRec, PRIVATE_CLIENT,
            sizeof(struct X_mRNAClientPriv)) &&
          AddCallback(&ClientStateCallback, hookClientState, NULL) &&
          AddCallback(&PostInitRootWindowCallback, hookInitRootWindow, NULL) &&
          AddCallback(&PropertyFilterCallback, hookWindowProperty, NULL) &&
          AddCallback(&SelectionFilterCallback, hookSelectionFilter, NULL) &&
          XaceRegisterCallback(XACE_CLIENT_ACCESS, hookClient, NULL) &&
          XaceRegisterCallback(XACE_DEVICE_ACCESS, hookDevice, NULL) &&
          XaceRegisterCallback(XACE_EXT_DISPATCH, hookExtDispatch, NULL) &&
          XaceRegisterCallback(XACE_EXT_ACCESS, hookExtAccess, NULL) &&
          XaceRegisterCallback(XACE_PROPERTY_ACCESS, hookPropertyAccess, NULL) &&
          XaceRegisterCallback(XACE_RECEIVE_ACCESS, hookReceive, NULL) &&
          XaceRegisterCallback(XACE_RESOURCE_ACCESS, hookResourceAccess, NULL) &&
          XaceRegisterCallback(XACE_SEND_ACCESS, hookSend, NULL) &&
          XaceRegisterCallback(XACE_SERVER_ACCESS, hookServerAccess, NULL)))
        FatalError("NamespaceExtensionInit: allocation failure\n");

    /* Do the serverClient */
    struct X_mRNAClientPriv *srv = XnsClientPriv(serverClient);
    *srv = (struct X_mRNAClientPriv) { .isServer = TRUE };
    X_mRNAAssignClient(srv, &ns_root);
}

void X_mRNAAssignClient(struct X_mRNAClientPriv *priv, struct X_mRNA *newns)
{
    if (priv->ns != NULL)
        priv->ns->refcnt--;

    priv->ns = newns;

    if (newns != NULL)
        newns->refcnt++;
}

void X_mRNAAssignClientByName(struct X_mRNAClientPriv *priv, const char *name)
{
    struct X_mRNA *newns = XnsFindByName(name);

    if (newns == NULL)
        newns = &ns_anon;

    X_mRNAAssignClient(priv, newns);
}

struct X_mRNA* XnsFindByAuth(size_t szAuthProto, const char* authProto, size_t szAuthToken, const char* authToken)
{
    struct X_mRNA *walk;
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
