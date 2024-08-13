/*

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/
#include <dix-config.h>

#ifdef WIN32
#include <X11/Xwindows.h>
#endif

#include <X11/X.h>
#include <X11/Xdefs.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include <xcb/xkb.h>

#include "screenint.h"
#include "inputstr.h"
#include "misc.h"
#include "scrnintstr.h"
#include "servermd.h"

#include "Xnest.h"
#include "xnest-xcb.h"

#include "Display.h"
#include "Screen.h"
#include "Keyboard.h"
#include "Args.h"
#include "Events.h"

// must come after Xnest.h, because of trickery to avoid name clash
#include <X11/XKBlib.h>
#include <X11/extensions/XKB.h>

#include "xkbsrv.h"

DeviceIntPtr xnestKeyboardDevice = NULL;

void
xnestBell(int volume, DeviceIntPtr pDev, void *ctrl, int cls)
{
    xcb_bell(xnestUpstreamInfo.conn, volume);
}

void
DDXRingBell(int volume, int pitch, int duration)
{
    xcb_bell(xnestUpstreamInfo.conn, volume);
}

void
xnestChangeKeyboardControl(DeviceIntPtr pDev, KeybdCtrl * ctrl)
{
#if 0
    unsigned long value_mask;
    int i;

    value_mask = KBKeyClickPercent |
        KBBellPercent | KBBellPitch | KBBellDuration | KBAutoRepeatMode;

    xcb_params_keyboard_t values = {
        .key_click_percent = ctrl->click,
        .bell_percent = ctrl->bell,
        .bell_pitch = ctrl->bell_pitch,
        .bell_duration = ctrl->bell_duration,
        .auto_repeat_mode = ctrl->autoRepeat ? AutoRepeatModeOn : AutoRepeatModeOff,
    };

    xcb_aux_change_keyboard_control(xnestUpstreamInfo.conn, value_mask, &values);
    /*
       value_mask = KBKey | KBAutoRepeatMode;
       At this point, we need to walk through the vector and compare it
       to the current server vector.  If there are differences, report them.
     */

    value_mask = KBLed | KBLedMode;
    for (i = 1; i <= 32; i++) {
        values.led = i;
        values.led_mode =
            (ctrl->leds & (1 << (i - 1))) ? LedModeOn : LedModeOff;

        xcb_aux_change_keyboard_control(xnestUpstreamInfo.conn, value_mask, &values);
    }
#endif
}

int
xnestKeyboardProc(DeviceIntPtr pDev, int onoff)
{
    XModifierKeymap *modifier_keymap;
    CARD8 modmap[MAP_LENGTH];
    int i, j;
    XKeyboardState values;

    switch (onoff) {
    case DEVICE_INIT:
    {
        const int min_keycode = xnestUpstreamInfo.setup->min_keycode;
        const int max_keycode = xnestUpstreamInfo.setup->max_keycode;
        const int num_keycode = max_keycode - min_keycode + 1;

        xcb_get_keyboard_mapping_reply_t * keymap_reply = xnest_get_keyboard_mapping(
            xnestUpstreamInfo.conn,
            min_keycode,
            num_keycode);

        if (!keymap_reply) {
            ErrorF("Couldn't get keyboard mappings: no reply");
            goto XkbError;
        }

        KeySymsRec keySyms = {
            .minKeyCode = min_keycode,
            .maxKeyCode = max_keycode,
            .mapWidth = keymap_reply->keysyms_per_keycode,
            .map = xcb_get_keyboard_mapping_keysyms(keymap_reply),
        };

        memset(modmap, 0, sizeof(modmap));
        modifier_keymap = XGetModifierMapping(xnestDisplay);
        for (j = 0; j < 8; j++)
            for (i = 0; i < modifier_keymap->max_keypermod; i++) {
                CARD8 keycode;

                if ((keycode =
                     modifier_keymap->modifiermap[j *
                                                  modifier_keymap->
                                                  max_keypermod + i]))
                    modmap[keycode] |= 1 << j;
            }
        XFreeModifiermap(modifier_keymap);

        InitKeyboardDeviceStruct(pDev, NULL,
                                 xnestBell, xnestChangeKeyboardControl);

        XkbApplyMappingChange(pDev, &keySyms, keySyms.minKeyCode,
                              keySyms.maxKeyCode - keySyms.minKeyCode + 1,
                              modmap, serverClient);

        free(keymap_reply);

        xnest_xkb_init(xnestUpstreamInfo.conn);

        int device_id = xnest_xkb_device_id(xnestUpstreamInfo.conn);

        xcb_generic_error_t *err = NULL;
        xcb_xkb_get_controls_reply_t *reply = xcb_xkb_get_controls_reply(
            xnestUpstreamInfo.conn,
            xcb_xkb_get_controls(xnestUpstreamInfo.conn, device_id),
            &err);

        if (err) {
            ErrorF("Couldn't get keyboard controls for %d: error %d\n", device_id, err->error_code);
            free(err);
            goto XkbError;
        }

        if (!reply) {
            ErrorF("Couldn't get keyboard controls for %d: no reply", device_id);
            goto XkbError;
        }

        XkbControlsRec ctrls = {
            .mk_dflt_btn = reply->mouseKeysDfltBtn,
            .num_groups = reply->numGroups,
            .groups_wrap = reply->groupsWrap,
            .internal = (XkbModsRec) {
                .mask = reply->internalModsMask,
                .real_mods = reply->internalModsRealMods,
                .vmods = reply->internalModsVmods,
            },
            .ignore_lock = (XkbModsRec) {
                .mask = reply->ignoreLockModsMask,
                .real_mods = reply->ignoreLockModsRealMods,
                .vmods = reply->ignoreLockModsVmods,
            },
            .enabled_ctrls = reply->enabledControls,
            .repeat_delay = reply->repeatDelay,
            .repeat_interval = reply->repeatInterval,
            .slow_keys_delay = reply->slowKeysDelay,
            .debounce_delay = reply->debounceDelay,
            .mk_delay = reply->mouseKeysDelay,
            .mk_interval = reply->mouseKeysInterval,
            .mk_time_to_max = reply->mouseKeysTimeToMax,
            .mk_max_speed = reply->mouseKeysMaxSpeed,
            .mk_curve = reply->mouseKeysCurve,
            .ax_options = reply->accessXOption,
            .ax_timeout = reply->accessXTimeout,
            .axt_opts_mask = reply->accessXTimeoutOptionsMask,
            .axt_opts_values = reply->accessXTimeoutOptionsValues,
            .axt_ctrls_mask = reply->accessXTimeoutMask,
            .axt_ctrls_values = reply->accessXTimeoutValues,
        };
        memcpy(&ctrls.per_key_repeat, reply->perKeyRepeat, sizeof(ctrls.per_key_repeat));

        XkbDDXChangeControls(pDev, &ctrls, &ctrls);
        break;
    }
    case DEVICE_ON:
        xnestEventMask |= XNEST_KEYBOARD_EVENT_MASK;
        for (i = 0; i < xnestNumScreens; i++)
            xcb_change_window_attributes(xnestUpstreamInfo.conn,
                                         xnestDefaultWindows[i],
                                         XCB_CW_EVENT_MASK,
                                         &xnestEventMask);
        break;
    case DEVICE_OFF:
        xnestEventMask &= ~XNEST_KEYBOARD_EVENT_MASK;
        for (i = 0; i < xnestNumScreens; i++)
            xcb_change_window_attributes(xnestUpstreamInfo.conn,
                                         xnestDefaultWindows[i],
                                         XCB_CW_EVENT_MASK,
                                         &xnestEventMask);
        break;
    case DEVICE_CLOSE:
        break;
    }
    return Success;

 XkbError:
    XGetKeyboardControl(xnestDisplay, &values);
    memmove((char *) defaultKeyboardControl.autoRepeats,
            (char *) values.auto_repeats, sizeof(values.auto_repeats));

    InitKeyboardDeviceStruct(pDev, NULL, xnestBell, xnestChangeKeyboardControl);
    return Success;
}

void
xnestUpdateModifierState(unsigned int state)
{
    DeviceIntPtr pDev = xnestKeyboardDevice;
    KeyClassPtr keyc = pDev->key;
    int i;
    CARD8 mask;
    int xkb_state;

    if (!pDev)
        return;

    xkb_state = XkbStateFieldFromRec(&pDev->key->xkbInfo->state);
    state = state & 0xff;

    if (xkb_state == state)
        return;

    for (i = 0, mask = 1; i < 8; i++, mask <<= 1) {
        int key;

        /* Modifier is down, but shouldn't be */
        if ((xkb_state & mask) && !(state & mask)) {
            int count = keyc->modifierKeyCount[i];

            for (key = 0; key < MAP_LENGTH; key++)
                if (keyc->xkbInfo->desc->map->modmap[key] & mask) {
                    if (mask == LockMask) {
                        xnestQueueKeyEvent(KeyPress, key);
                        xnestQueueKeyEvent(KeyRelease, key);
                    }
                    else if (key_is_down(pDev, key, KEY_PROCESSED))
                        xnestQueueKeyEvent(KeyRelease, key);

                    if (--count == 0)
                        break;
                }
        }

        /* Modifier should be down, but isn't */
        if (!(xkb_state & mask) && (state & mask))
            for (key = 0; key < MAP_LENGTH; key++)
                if (keyc->xkbInfo->desc->map->modmap[key] & mask) {
                    xnestQueueKeyEvent(KeyPress, key);
                    if (mask == LockMask)
                        xnestQueueKeyEvent(KeyRelease, key);
                    break;
                }
    }
}
