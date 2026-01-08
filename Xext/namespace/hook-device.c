#define HOOK_NAME "device"

#include <dix-config.h>

#include <X11/extensions/XIproto.h>
#include <X11/extensions/XI2proto.h>
#include <X11/extensions/XKB.h>

#include "dix/devices_priv.h"
#include "dix/dix_priv.h"
#include "dix/extension_priv.h"
#include "dix/registry_priv.h"

#include "namespace.h"
#include "hooks.h"

void hookDevice(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XNS_HOOK_HEAD(DeviceAccessCallbackParam);

    if (subj->ns->superPower)
        goto pass;

    // should be safe to pass for anybody
    switch (client->majorOp) {
        case X_QueryPointer:
            if (subj->ns->perms.allowMouseMotion)
                goto pass;
            goto block;
        case X_QueryKeymap:
            if (subj->ns->perms.allowGlobalKeyboard)
                goto pass;
            goto block;
        case X_GetInputFocus:
        case X_GetKeyboardMapping:
        case X_GetModifierMapping:
        case X_GrabButton: // needed by xterm -- should be safe
            goto pass;
        case EXTENSION_MAJOR_XKEYBOARD:
            if (subj->ns->perms.allowXKeyboard)
                goto pass;
            switch(client->minorOp) {
                case X_kbSelectEvents:      // needed by xterm
                case X_kbGetMap:            // needed by xterm
                case X_kbBell:              // needed by GIMP
                case X_kbPerClientFlags:    // needed by firefox
                case X_kbGetState:          // needed by firefox
                case X_kbGetNames:          // needed by firefox
                case X_kbGetControls:       // needed by firefox
                    goto pass;
                default:
                    XNS_HOOK_LOG("BLOCKED unhandled XKEYBOARD %s\n", LookupRequestName(client->majorOp, client->minorOp));
                    goto block;
            }

        case X_GrabPointer:
        case X_GetPointerMapping:
        case X_SetInputFocus:
        case X_WarpPointer:
            if (subj->ns->perms.allowXInput)
                goto pass;
            goto block;
        case X_GrabKeyboard:
        case X_UngrabKeyboard:
            if (subj->ns->perms.allowXKeyboard)
                goto pass;
            goto block;

        case EXTENSION_MAJOR_XINPUT:
            switch (client->minorOp) {
                case X_ListInputDevices:
                case X_XIGetProperty:
                    goto pass;
                case X_XIQueryPointer:
                    if (subj->ns->perms.allowMouseMotion)
                        goto pass;
                    goto block;

                case X_XIQueryDevice:
                case X_XIChangeCursor:
                case X_XIGrabDevice:
                case X_XIUngrabDevice:
                    if (subj->ns->perms.allowXInput)
                        goto pass;
                goto block;
                default:
                    XNS_HOOK_LOG("BLOCKED unhandled Xinput request\n");
                    goto block;
            }
    }

block:
    param->status = BadAccess;
    return;

pass:
    param->status = Success;
    return;
}
