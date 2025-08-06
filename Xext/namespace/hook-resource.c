#define HOOK_NAME "resource"

#include <dix-config.h>

#include <inttypes.h>
#include <X11/extensions/XI2proto.h>

#include "dix/dix_priv.h"
#include "dix/extension_priv.h"
#include "dix/registry_priv.h"
#include "dix/window_priv.h"
#include "Xext/xacestr.h"

#include "namespace.h"
#include "hooks.h"

static int checkAllowed(Mask requested, Mask allowed) {
    return ((requested & allowed) == requested);
}

void hookResourceAccess(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XNS_HOOK_HEAD(XaceResourceAccessRec);
    ClientPtr owner = dixLookupXIDOwner(param->id);
    struct XnamespaceClientPriv *obj = XnsClientPriv(owner);

    // server can do anything
    if (param->client == serverClient)
        goto pass;

    // special filtering for windows: block transparency for untrusted clients
    if (param->rtype == X11_RESTYPE_WINDOW) {
        WindowPtr pWindow = (WindowPtr) param->res;
        if (param->access_mode & DixCreateAccess) {
            if (!subj->ns->allowTransparency) {
                pWindow->forcedBG = TRUE;
            }
        }
    }

    // resource access inside same container is always permitted
    if (XnsClientSameNS(subj, obj))
        goto pass;

    // check for root windows (screen or ns-virtual)
    if (param->rtype == X11_RESTYPE_WINDOW) {
        WindowPtr pWindow = (WindowPtr) param->res;

        /* white-listed operations on namespace's virtual root window */
        if (pWindow == subj->ns->rootWindow) {
            switch (client->majorOp) {
                case X_DeleteProperty:
                case X_ChangeProperty:
                case X_GetProperty:
                case X_RotateProperties:
                case X_QueryTree:
                    goto pass;
            }
            XNS_HOOK_LOG("unhandled access to NS' virtual root window 0x%0" PRIx32 "\n", pWindow->drawable.id);
        }

        /* white-listed operations on actual root window */
        if (pWindow && (pWindow == pWindow->drawable.pScreen->root)) {
            switch (client->majorOp) {
                case X_CreateWindow:
                    if (checkAllowed(param->access_mode, DixAddAccess))
                        goto pass;
                break;

                case X_CreateGC:
                case X_CreatePixmap:
                    if (checkAllowed(param->access_mode, DixGetAttrAccess))
                        goto pass;
                break;

                // we reach here when destroying a top-level window:
                // ProcDestroyWindow() checks whether one may remove a child
                // from it's parent.
                case X_DestroyWindow:
                    if (param->access_mode == DixRemoveAccess)
                        goto pass;
                break;

                case X_TranslateCoords:
                case X_QueryTree:
                    goto pass;

                case X_ChangeWindowAttributes:
                case X_QueryPointer:
                    goto reject;

                case X_SendEvent:
                    /* send hook needs to take care of this */
                    goto pass;

                case EXTENSION_MAJOR_XINPUT:
                    switch(client->minorOp) {
                        // needed by xeyes. we should filter the mask
                        case X_XISelectEvents:
                            goto pass;
                    }
                    XNS_HOOK_LOG("unhandled XI operation on (real) root window\n");
                    goto reject;
            }
        }
    }

    /* server resources */
    if (obj->isServer) {
        if (param->rtype == X11_RESTYPE_COLORMAP) {
            if (checkAllowed(param->access_mode, DixReadAccess | DixGetPropAccess | DixUseAccess | DixGetAttrAccess | DixAddAccess))
                goto pass;
        }

        if (param->rtype == X11_RESTYPE_WINDOW) {
            /* allowed ones should already been catched above */
            XNS_HOOK_LOG("REJECT server owned window 0x%0" PRIx32 "!\n", ((WindowPtr)param->res)->drawable.id);
            goto reject;
        }

        if (checkAllowed(param->access_mode, DixReadAccess))
            goto pass;
    }

reject: ;
    char accModeStr[128];
    LookupDixAccessName(param->access_mode, (char*)&accModeStr, sizeof(accModeStr));

    XNS_HOOK_LOG("BLOCKED access 0x%07lx %s to %s 0x%06lx of client %d @ %s\n",
        (unsigned long)param->access_mode,
        accModeStr,
        LookupResourceName(param->rtype),
        (unsigned long)param->id,
        owner->index, // resource owner
        obj->ns->name);

    param->status = BadAccess;
    return;

pass:
    // request is passed as it is (or already had been rewritten)
    param->status = Success;
}
