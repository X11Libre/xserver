/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later */
/* Copyright (C) 2026 Enrico Weigelt, metux IT consult <info@metux.net> */
#define HOOK_NAME "device"

#include <dix-config.h>

#include <X11/extensions/XIproto.h>
#include <X11/extensions/XI2proto.h>
#include <X11/extensions/XKB.h>

#include "dix/devices_priv.h"
#include "dix/dix_priv.h"
#include "dix/extension_priv.h"
#include "dix/registry_priv.h"
#include "include/inputstr.h"

#include "namespace.h"
#include "hooks.h"

void hookDevice(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    XNS_HOOK_HEAD(DeviceAccessCallbackParam);

    if (param->access_mode & DixSetFocusAccess)
        XnsRevokeAllPasteTickets(subj->ns);

    if (subj->ns->superPower)
        goto pass;

    /* Reading live physical device state (DixReadAccess: QueryKeymap,
       KeymapNotify, QueryDeviceState, XIQueryDevice button/axis state,
       XkbGetState, XkbSendStateNotify, QueryPointer/XIQueryPointer):
       - Global keymap polling (QueryKeymap, QueryDeviceState) is always
         denied so the caller receives a zeroed bitmap without a fatal XError.
       - Pointer coordinates in QueryPointer/XIQueryPointer require allowMouseMotion.
       - Live keyboard/modifier/button state requires namespace focus. */
    if (param->access_mode & DixReadAccess) {
        if (client->majorOp == X_QueryKeymap ||
            (client->majorOp == EXTENSION_MAJOR_XINPUT &&
             client->minorOp == X_QueryDeviceState)) {
            goto block;
        }
        if ((client->majorOp == X_QueryPointer ||
             (client->majorOp == EXTENSION_MAJOR_XINPUT &&
              client->minorOp == X_XIQueryPointer)) &&
            param->dev && IsPointerDevice(param->dev)) {
            if (!subj->ns->allowMouseMotion)
                goto block;
        } else if (!XnsNamespaceHasFocus(subj->ns)) {
            goto block;
        }
    }

    /* Prevent unfocused background sandboxes from stealing keyboard focus,
       warping the pointer, or actively grabbing input devices.
       Passive grabs (GrabButton, GrabKey, XIPassiveGrabDevice) on the client's
       own windows are already gated by dixLookupWindow(..., DixSetAttrAccess)
       and must succeed during window creation before focus is assigned. */
    Bool isPassiveGrab =
        (client->majorOp == X_GrabButton ||
         client->majorOp == X_GrabKey ||
         (client->majorOp == EXTENSION_MAJOR_XINPUT &&
          (client->minorOp == X_GrabDeviceButton ||
           client->minorOp == X_GrabDeviceKey ||
           client->minorOp == X_XIPassiveGrabDevice)));

    if (!isPassiveGrab &&
        ((param->access_mode & (DixSetFocusAccess | DixGrabAccess | DixWriteAccess)) ||
         client->majorOp == X_SetInputFocus ||
         client->majorOp == X_GrabPointer ||
         client->majorOp == X_GrabKeyboard ||
         client->majorOp == X_WarpPointer)) {
        if (!XnsNamespaceHasFocus(subj->ns)) {
            XNS_HOOK_LOG("BLOCKED background focus/grab/warp by %s\n",
                         subj->ns->name);
            goto block;
        }
    }

    if (client->majorOp == X_WarpPointer) {
        if (subj->ns->allowMouseMotion && XnsNamespaceHasFocus(subj->ns))
            goto pass;
        goto block;
    }

    if (client->majorOp == EXTENSION_MAJOR_XKEYBOARD) {
        if (subj->ns->allowXKeyboard || client->minorOp == X_kbPerClientFlags)
            goto pass;
        if ((param->access_mode & ~(DixReadAccess | DixGetAttrAccess |
                                    DixUseAccess | DixBellAccess)) == 0)
            goto pass;
        XNS_HOOK_LOG("BLOCKED XKEYBOARD write/manage request %s\n",
                     LookupRequestName(client->majorOp, client->minorOp));
        goto block;
    }

    if (client->majorOp == EXTENSION_MAJOR_XINPUT) {
        if (client->minorOp == X_XIWarpPointer) {
            if (subj->ns->allowMouseMotion && XnsNamespaceHasFocus(subj->ns))
                goto pass;
            goto block;
        }
        if (subj->ns->allowXInput) {
            if ((param->access_mode & (DixManageAccess | DixCreateAccess | DixDestroyAccess)) == 0)
                goto pass;
        }
        if ((param->access_mode & ~(DixReadAccess | DixGetAttrAccess)) == 0)
            goto pass;
        XNS_HOOK_LOG("BLOCKED XInput request %s\n",
                     LookupRequestName(client->majorOp, client->minorOp));
        goto block;
    }

    /* Core device queries, focus, bell, grabs, and event thawing */
    if ((param->access_mode & ~(DixReadAccess | DixGetAttrAccess | DixGetFocusAccess |
                                DixSetFocusAccess | DixUseAccess | DixBellAccess |
                                DixGrabAccess | DixFreezeAccess | DixForceAccess)) == 0)
        goto pass;

block:
    param->status = BadAccess;
    return;

pass:
    param->status = Success;
    return;
}
