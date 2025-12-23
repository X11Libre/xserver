#define HOOK_NAME "ext-dispatch"

#include <dix-config.h>

#include <stdio.h>
#include <X11/Xdefs.h> // syncproto.h is broken
#include <X11/Xmd.h>
#include <X11/extensions/composite.h>
#include <X11/extensions/syncproto.h>
#include <X11/extensions/XIproto.h>
#include <X11/extensions/XKB.h>
#include <X11/extensions/xfixeswire.h>

#include "dix/dix_priv.h"
#include "dix/extension_priv.h"
#include "dix/registry_priv.h"
#include "Xext/xacestr.h"

#include "namespace.h"
#include "hooks.h"

void hookExtDispatch(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XNS_HOOK_HEAD(ExtensionAccessCallbackParam);

    /* root NS has super powers */
    if (subj->ns->superPower)
        goto pass;

    switch (client->majorOp) {
        /* unrestricted access to these */
        case EXTENSION_MAJOR_BIG_REQUESTS:
        case EXTENSION_MAJOR_DAMAGE:
        case EXTENSION_MAJOR_DOUBLE_BUFFER:
        case EXTENSION_MAJOR_DPMS:
        case EXTENSION_MAJOR_GENERIC_EVENT:
        case EXTENSION_MAJOR_PRESENT:
        case EXTENSION_MAJOR_XC_MISC:
        case EXTENSION_MAJOR_XRESOURCE:
        case EXTENSION_MAJOR_XINERAMA:
            goto pass;

        /* allow several operations */
        case EXTENSION_MAJOR_XKEYBOARD:
            if (subj->ns->perms.allowXKeyboard)
                goto pass;
            switch (client->minorOp) {
                case X_kbUseExtension:
                case X_kbGetMap:
                case X_kbSelectEvents: // fixme: might need special filtering
                case X_kbGetState:
                case X_kbGetNames:
                case X_kbGetControls:
                case X_kbPerClientFlags:
                    goto pass;
            }
            XNS_HOOK_LOG("BLOCKED unhandled XKEYBOARD call: %s\n", param->ext->name);
            goto reject;

        case EXTENSION_MAJOR_GLX:
        case EXTENSION_MAJOR_DRI2:
        case EXTENSION_MAJOR_DRI3:
        case EXTENSION_MAJOR_RENDER:
            if (subj->ns->perms.allowRender)
                goto pass;
            goto reject;
        case EXTENSION_MAJOR_RANDR:
            if (subj->ns->perms.allowRandr)
                goto pass;
            switch(client->minorOp) {
                case X_RenderQueryVersion:
                    goto pass;
            }
            goto reject;
        case EXTENSION_MAJOR_COMPOSITE:
            if (subj->ns->perms.allowComposite)
                goto pass;
            switch(client->minorOp) {
                case X_CompositeQueryVersion:
                    goto pass;
            }
            goto reject;
        case EXTENSION_MAJOR_SHM:
            if (subj->ns->perms.allowScreen)
                goto pass;
            goto reject;
        /* allow if namespace has flag set */
        case EXTENSION_MAJOR_SHAPE:
            if (subj->ns->perms.allowShape)
                goto pass;
            goto reject;
        case EXTENSION_MAJOR_XINPUT:
            if (subj->ns->perms.allowXInput)
                goto pass;
            switch (client->minorOp) {
                case X_ListInputDevices:
                    goto pass;
            }
            goto reject;

        case EXTENSION_MAJOR_XFIXES:
            switch (client->minorOp) {
                case X_XFixesQueryVersion:
                case X_XFixesCreateRegion:
                case X_XFixesSetCursorName:
                case X_XFixesGetCursorImage:
                case X_XFixesGetCursorImageAndName:
                case X_XFixesSelectSelectionInput:
                case X_XFixesDestroyRegion:
                case X_XFixesSetRegion:
                    goto pass;
            }
            XNS_HOOK_LOG("BLOCKED unhandled XFIXES call: %s\n", param->ext->name);
            goto reject;
        break;

        case EXTENSION_MAJOR_SYNC:
            // should be safe to enable
                    goto pass;
        break;

        /* really blacklisted */
        case EXTENSION_MAJOR_MIT_SCREEN_SAVER:
        case EXTENSION_MAJOR_RECORD:
        case EXTENSION_MAJOR_SECURITY:
        case EXTENSION_MAJOR_XTEST:
            goto reject;
        break;
    }

    XNS_HOOK_LOG("unhandled extension call: %s\n", param->ext->name);
    return;

reject:
    XNS_HOOK_LOG("rejecting extension call: %s\n", param->ext->name);
    param->status = BadAccess;
    return;

pass:
    param->status = Success;
    return;
}
