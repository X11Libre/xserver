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
        case X_GetInputFocus:
        case X_GetKeyboardMapping:
        case X_GetModifierMapping:
        case X_GrabButton: // needed by xterm -- should be safe
            goto pass;
        case EXTENSION_MAJOR_XKEYBOARD:
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
        case EXTENSION_MAJOR_XINPUT:
            switch (client->minorOp) {
                case X_ListInputDevices:
                case X_XIQueryDevice:
                    goto pass;
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
