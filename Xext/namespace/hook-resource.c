#define HOOK_NAME "resource"

#include <dix-config.h>

#include <inttypes.h>
#include <X11/extensions/XI2proto.h>
#include <X11/extensions/shmproto.h>

#include "dix/dix_priv.h"
#include "dix/extension_priv.h"
#include "dix/registry_priv.h"
#include "dix/window_priv.h"
#include "Xext/xacestr.h"

#include "xfixes/xfixesint.h"
#include "randr/randrstr_priv.h"

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

    /* server can do anything */
    if (param->client == serverClient)
        goto pass;

    /* special filtering for windows: block transparency for untrusted clients */
    if (param->rtype == X11_RESTYPE_WINDOW) {
        WindowPtr pWindow = (WindowPtr) param->res;
        if (param->access_mode & DixCreateAccess) {
            if (!subj->ns->perms.allowTransparency) {
                pWindow->forcedBG = TRUE;
            }
        }
    }


    /* resource access inside same namespace is always permitted */
    if (subj->ns->superPower || XnsClientSameNS(subj, obj))
        goto pass;

    /* whitelist actions to root namespace */
    if (obj->ns->isRoot) {
        /* randr events to root */
        if (param->rtype == RREventType) {
            if (subj->ns->perms.allowRandr)
                goto pass;
        }
        switch (client->majorOp) {
            /* should be safe to expose globally from root */
            case X_GetProperty:
            case X_TranslateCoords:
            case X_GetGeometry:
            case X_QueryTree:
            case X_GetWindowAttributes:
            case X_DestroyWindow:
                goto pass;
            case X_QueryPointer:
                if (subj->ns->perms.allowMouseMotion)
                    goto pass;
            case EXTENSION_MAJOR_XINPUT:
                switch(client->minorOp) {
                    /* needed by xeyes. we should filter the mask */
                    case X_XIQueryPointer:
                        if (subj->ns->perms.allowMouseMotion)
                            goto pass;
                }
            /* needed for gimp? should be safe. */
            case EXTENSION_MAJOR_SHM:
                if (subj->ns->perms.allowScreen)
                    goto pass;
                if (client->minorOp == X_ShmCreatePixmap)
                    goto pass;
            case EXTENSION_MAJOR_COMPOSITE:
                if (subj->ns->perms.allowComposite)
                    goto pass;
            case X_GetImage:
            case X_CopyArea:
                if (subj->ns->perms.allowScreen)
                    goto pass;
        }
    }

    /* check for root windows (screen or ns-virtual) */
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
            XNS_HOOK_LOG("unhandled access to NS' virtual root window 0x%0lx\n", (unsigned long)pWindow->drawable.id);
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
                case X_CreateColormap:
                    if (checkAllowed(param->access_mode, DixGetAttrAccess))
                        goto pass;
                break;

                /* we reach here when destroying a top-level window:
                 * ProcDestroyWindow() checks whether one may remove a child
                 * from it's parent.
                 */
                case X_DestroyWindow:
                    if (param->access_mode == DixRemoveAccess)
                        goto pass;
                break;

                case X_TranslateCoords:
                case X_QueryTree:
                    goto pass;

                case X_GetWindowAttributes:
                case X_ChangeWindowAttributes:
                    goto pass;
                    /* needed by many programs. should be safe? */
                case X_QueryPointer:
                    if (subj->ns->perms.allowMouseMotion)
                        goto pass;
                    goto reject;

                case X_GrabPointer:
                    if (subj->ns->perms.allowXInput)
                        goto pass;
                case X_SendEvent:
                    /* send hook needs to take care of this */
                    goto pass;

                case EXTENSION_MAJOR_XINPUT:
                    switch(client->minorOp) {
                        /* needed by xeyes. we should filter the mask */
                        case X_XIQueryPointer:
                            if (subj->ns->perms.allowXInput)
                                goto pass;
                            goto reject;
                        case X_XISelectEvents:
                            goto pass;
                    }
                    XNS_HOOK_LOG("unhandled XI operation on (real) root window\n");
                    goto reject;
                case EXTENSION_MAJOR_RANDR:
                    if (subj->ns->perms.allowRandr)
                        goto pass;
                goto reject;
                case EXTENSION_MAJOR_GLX:
                case EXTENSION_MAJOR_DRI2:
                case EXTENSION_MAJOR_DRI3:
                case EXTENSION_MAJOR_RENDER:
                    if (subj->ns->perms.allowRender)
                        goto pass;
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
            XNS_HOOK_LOG("REJECT server owned window 0x%0lx!\n", (unsigned long)((WindowPtr)param->res)->drawable.id);
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
        owner->index, /* resource owner */
        obj->ns->name);

    param->status = BadAccess;
    return;

pass:
    /* request is passed as it is (or already had been rewritten) */
    param->status = Success;
}
