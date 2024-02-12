/*
 * Copyright © 2014 Intel Corporation
 * Copyright © 2008 Kristian Høgsberg
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of the
 * copyright holders not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include <xwayland-config.h>

#include <linux/input.h>
#include <sys/mman.h>

#include "dix/dix_priv.h"
#include "dix/exevents_priv.h"
#include "dix/input_priv.h"

#include <inputstr.h>
#include <xkbsrv.h>
#include <xserver-properties.h>
#include <inpututils.h>
#include <mi.h>
#include <mipointer.h>
#include <mipointrst.h>
#include <misc.h>

#include "xwayland-cursor.h"
#include "xwayland-input.h"
#include "xwayland-window.h"
#include "xwayland-screen.h"

#ifdef XWL_HAS_EI
#include "xwayland-xtest.h"
#endif

#include "pointer-constraints-unstable-v1-client-protocol.h"
#include "relative-pointer-unstable-v1-client-protocol.h"
#include "tablet-unstable-v2-client-protocol.h"
#include "pointer-gestures-unstable-v1-client-protocol.h"
#include "xwayland-keyboard-grab-unstable-v1-client-protocol.h"
#include "keyboard-shortcuts-inhibit-unstable-v1-client-protocol.h"

#define SCROLL_AXIS_HORIZ 2
#define SCROLL_AXIS_VERT 3

struct sync_pending {
    struct xorg_list l;
    DeviceIntPtr pending_dev;
};

static DevPrivateKeyRec xwl_tablet_private_key;

static void
xwl_pointer_warp_emulator_handle_motion(struct xwl_pointer_warp_emulator *warp_emulator,
                                        double dx,
                                        double dy,
                                        double dx_unaccel,
                                        double dy_unaccel);
static void
xwl_pointer_warp_emulator_maybe_lock(struct xwl_pointer_warp_emulator *warp_emulator,
                                     struct xwl_window *xwl_window,
                                     SpritePtr sprite,
                                     int x, int y);

static Bool
xwl_seat_maybe_lock_on_hidden_cursor(struct xwl_seat *xwl_seat);

static void
xwl_seat_destroy_confined_pointer(struct xwl_seat *xwl_seat);

static void
init_tablet_manager_seat(struct xwl_screen *xwl_screen,
                         struct xwl_seat *xwl_seat);
static void
release_tablet_manager_seat(struct xwl_seat *xwl_seat);

static void
xwl_pointer_control(DeviceIntPtr device, PtrCtrl *ctrl)
{
    /* Nothing to do, dix handles all settings */
}

static DeviceIntPtr
get_pointer_device(struct xwl_seat *xwl_seat)
{
    if (xwl_seat->relative_pointer)
        return xwl_seat->relative_pointer;
    else
        return xwl_seat->pointer;
}

static Bool
init_pointer_buttons(DeviceIntPtr device)
{
#define NBUTTONS 10
    BYTE map[NBUTTONS + 1];
    int i = 0;
    Atom btn_labels[NBUTTONS] = { 0 };

    for (i = 1; i <= NBUTTONS; i++)
        map[i] = i;

    btn_labels[0] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_LEFT);
    btn_labels[1] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_MIDDLE);
    btn_labels[2] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_RIGHT);
    btn_labels[3] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_UP);
    btn_labels[4] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_DOWN);
    btn_labels[5] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_HWHEEL_LEFT);
    btn_labels[6] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_HWHEEL_RIGHT);
    /* don't know about the rest */

    if (!InitButtonClassDeviceStruct(device, NBUTTONS, btn_labels, map))
        return FALSE;

    return TRUE;
}

static void
maybe_fake_grab_devices(struct xwl_seat *xwl_seat)
{
    struct xwl_screen *xwl_screen = xwl_seat->xwl_screen;
    struct xwl_window *xwl_window;

    if (xwl_screen->rootless)
        return;

    if (!xwl_screen->host_grab)
        return;

    if (!xwl_screen->has_grab)
        return;

    if (!xwl_screen->screen->root)
        return;

    xwl_window = xwl_window_get(xwl_screen->screen->root);
    if (!xwl_window)
        return;

    xwl_seat_confine_pointer(xwl_seat, xwl_window);

    if (!xwl_screen->shortcuts_inhibit_manager)
        return;

    if (xwl_screen->shortcuts_inhibit)
        return;

    xwl_screen->shortcuts_inhibit =
        zwp_keyboard_shortcuts_inhibit_manager_v1_inhibit_shortcuts (
            xwl_screen->shortcuts_inhibit_manager,
            xwl_window->surface,
            xwl_seat->seat);
}

static void
maybe_fake_ungrab_devices(struct xwl_seat *xwl_seat)
{
    struct xwl_screen *xwl_screen = xwl_seat->xwl_screen;

    xwl_seat_unconfine_pointer(xwl_seat);

    if (!xwl_screen->shortcuts_inhibit)
        return;

    zwp_keyboard_shortcuts_inhibitor_v1_destroy (xwl_screen->shortcuts_inhibit);
    xwl_screen->shortcuts_inhibit = NULL;
}

static int
xwl_pointer_proc(DeviceIntPtr device, int what)
{
#define NAXES 4
    Atom axes_labels[NAXES] = { 0 };

    switch (what) {
    case DEVICE_INIT:
        device->public.on = FALSE;

        if (!init_pointer_buttons(device))
            return BadValue;

        axes_labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_X);
        axes_labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_Y);
        axes_labels[SCROLL_AXIS_HORIZ] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_HWHEEL);
        axes_labels[SCROLL_AXIS_VERT] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_WHEEL);

        if (!InitValuatorClassDeviceStruct(device, NAXES, axes_labels,
                                           GetMotionHistorySize(), Absolute))
            return BadValue;

        /* Valuators */
        InitValuatorAxisStruct(device, 0, axes_labels[0],
                               0, 0xFFFF, 10000, 0, 10000, Absolute);
        InitValuatorAxisStruct(device, 1, axes_labels[1],
                               0, 0xFFFF, 10000, 0, 10000, Absolute);
        InitValuatorAxisStruct(device, SCROLL_AXIS_HORIZ, axes_labels[2],
                               NO_AXIS_LIMITS, NO_AXIS_LIMITS, 0, 0, 0, Relative);
        InitValuatorAxisStruct(device, SCROLL_AXIS_VERT, axes_labels[3],
                               NO_AXIS_LIMITS, NO_AXIS_LIMITS, 0, 0, 0, Relative);

        SetScrollValuator(device, SCROLL_AXIS_HORIZ, SCROLL_TYPE_HORIZONTAL, 1.0, SCROLL_FLAG_NONE);
        SetScrollValuator(device, SCROLL_AXIS_VERT, SCROLL_TYPE_VERTICAL, 1.0, SCROLL_FLAG_PREFERRED);

        if (!InitPtrFeedbackClassDeviceStruct(device, xwl_pointer_control))
            return BadValue;

        return Success;

    case DEVICE_ON:
        device->public.on = TRUE;
        return Success;

    case DEVICE_OFF:
    case DEVICE_CLOSE:
        device->public.on = FALSE;
        return Success;
    }

    return BadMatch;

#undef NBUTTONS
#undef NAXES
}

static int
xwl_pointer_proc_relative(DeviceIntPtr device, int what)
{
#define NAXES 4
    Atom axes_labels[NAXES] = { 0 };

    switch (what) {
    case DEVICE_INIT:
        device->public.on = FALSE;

        axes_labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_X);
        axes_labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_Y);
        axes_labels[2] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_HWHEEL);
        axes_labels[3] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_WHEEL);

        /*
         * We'll never send buttons, but XGetPointerMapping might in certain
         * situations make the client think we have no buttons.
         */
        if (!init_pointer_buttons(device))
            return BadValue;

        if (!InitValuatorClassDeviceStruct(device, NAXES, axes_labels,
                                           GetMotionHistorySize(), Relative))
            return BadValue;

        /* Valuators */
        InitValuatorAxisStruct(device, 0, axes_labels[0],
                               NO_AXIS_LIMITS, NO_AXIS_LIMITS, 0, 0, 0, Relative);
        InitValuatorAxisStruct(device, 1, axes_labels[1],
                               NO_AXIS_LIMITS, NO_AXIS_LIMITS, 0, 0, 0, Relative);
        InitValuatorAxisStruct(device, 2, axes_labels[2],
                               NO_AXIS_LIMITS, NO_AXIS_LIMITS, 0, 0, 0, Relative);
        InitValuatorAxisStruct(device, 3, axes_labels[3],
                               NO_AXIS_LIMITS, NO_AXIS_LIMITS, 0, 0, 0, Relative);

        SetScrollValuator(device, 2, SCROLL_TYPE_HORIZONTAL, 1.0, SCROLL_FLAG_NONE);
        SetScrollValuator(device, 3, SCROLL_TYPE_VERTICAL, 1.0, SCROLL_FLAG_PREFERRED);

        if (!InitPtrFeedbackClassDeviceStruct(device, xwl_pointer_control))
            return BadValue;

        return Success;

    case DEVICE_ON:
        device->public.on = TRUE;
        return Success;

    case DEVICE_OFF:
    case DEVICE_CLOSE:
        device->public.on = FALSE;
        return Success;
    }

    return BadMatch;

#undef NAXES
}

static int
xwl_pointer_proc_pointer_gestures(DeviceIntPtr device, int what)
{
#define NTOUCHPOINTS 20
#define NAXES 2
    Atom axes_labels[NAXES] = { 0 };

    switch (what) {
    case DEVICE_INIT:
        device->public.on = FALSE;

        /* We need to setup a pointer device so that the device is attached to
           master pointer device.
        */
        axes_labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_X);
        axes_labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_Y);

        /*
         * We'll never send buttons, but XGetPointerMapping might in certain
         * situations make the client think we have no buttons.
         */
        if (!init_pointer_buttons(device))
            return BadValue;

        if (!InitValuatorClassDeviceStruct(device, NAXES, axes_labels,
                                           GetMotionHistorySize(), Relative))
            return BadValue;

        InitValuatorAxisStruct(device, 0, axes_labels[0],
                               NO_AXIS_LIMITS, NO_AXIS_LIMITS, 0, 0, 0, Relative);
        InitValuatorAxisStruct(device, 1, axes_labels[1],
                               NO_AXIS_LIMITS, NO_AXIS_LIMITS, 0, 0, 0, Relative);

        InitGestureClassDeviceStruct(device, NTOUCHPOINTS);
        return Success;

    case DEVICE_ON:
        device->public.on = TRUE;
        return Success;

    case DEVICE_OFF:
    case DEVICE_CLOSE:
        device->public.on = FALSE;
        return Success;
    }

    return BadMatch;

#undef NTOUCHPOINTS
#undef NAXES
}

static void
xwl_keyboard_control(DeviceIntPtr device, KeybdCtrl *ctrl)
{
}

static int
xwl_keyboard_proc(DeviceIntPtr device, int what)
{
    struct xwl_seat *xwl_seat = device->public.devicePrivate;
    int len;

    switch (what) {
    case DEVICE_INIT:
        device->public.on = FALSE;
        if (xwl_seat->keymap)
            len = strnlen(xwl_seat->keymap, xwl_seat->keymap_size);
        else
            len = 0;
        if (!InitKeyboardDeviceStructFromString(device, xwl_seat->keymap,
                                                len,
                                                NULL, xwl_keyboard_control))
            return BadValue;

        return Success;
    case DEVICE_ON:
        device->public.on = TRUE;
        return Success;

    case DEVICE_OFF:
    case DEVICE_CLOSE:
        device->public.on = FALSE;
        return Success;
    }

    return BadMatch;
}

static int
xwl_touch_proc(DeviceIntPtr device, int what)
{
#define NTOUCHPOINTS 20
#define NBUTTONS 1
#define NAXES 2
    Atom btn_labels[NBUTTONS] = { 0 };
    Atom axes_labels[NAXES] = { 0 };
    BYTE map[NBUTTONS + 1] = { 0 };

    switch (what) {
    case DEVICE_INIT:
        device->public.on = FALSE;

        axes_labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_MT_POSITION_X);
        axes_labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_MT_POSITION_Y);

        if (!InitValuatorClassDeviceStruct(device, NAXES, axes_labels,
                                           GetMotionHistorySize(), Absolute))
            return BadValue;

        if (!InitButtonClassDeviceStruct(device, NBUTTONS, btn_labels, map))
            return BadValue;

        if (!InitTouchClassDeviceStruct(device, NTOUCHPOINTS,
                                        XIDirectTouch, NAXES))
            return BadValue;

        /* Valuators */
        InitValuatorAxisStruct(device, 0, axes_labels[0],
                               0, 0xFFFF, 10000, 0, 10000, Absolute);
        InitValuatorAxisStruct(device, 1, axes_labels[1],
                               0, 0xFFFF, 10000, 0, 10000, Absolute);

        if (!InitPtrFeedbackClassDeviceStruct(device, xwl_pointer_control))
            return BadValue;

        return Success;

    case DEVICE_ON:
        device->public.on = TRUE;
        return Success;

    case DEVICE_OFF:
    case DEVICE_CLOSE:
        device->public.on = FALSE;
        return Success;
    }

    return BadMatch;
#undef NAXES
#undef NBUTTONS
#undef NTOUCHPOINTS
}

static int
xwl_tablet_proc(DeviceIntPtr device, int what)
{
#define NBUTTONS 9
#define NAXES 6
    Atom btn_labels[NBUTTONS] = { 0 };
    Atom axes_labels[NAXES] = { 0 };
    BYTE map[NBUTTONS + 1] = { 0 };
    int i;

    switch (what) {
    case DEVICE_INIT:
        device->public.on = FALSE;

        for (i = 1; i <= NBUTTONS; i++)
            map[i] = i;

        axes_labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_X);
        axes_labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_Y);
        axes_labels[2] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_PRESSURE);
        axes_labels[3] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_TILT_X);
        axes_labels[4] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_TILT_Y);
        axes_labels[5] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_WHEEL);

        if (!InitValuatorClassDeviceStruct(device, NAXES, axes_labels,
                                           GetMotionHistorySize(), Absolute))
            return BadValue;

        /* Valuators - match the xf86-input-wacom ranges */
        InitValuatorAxisStruct(device, 0, axes_labels[0],
                               0, 262143, 10000, 0, 10000, Absolute);
        InitValuatorAxisStruct(device, 1, axes_labels[1],
                               0, 262143, 10000, 0, 10000, Absolute);
        /* pressure */
        InitValuatorAxisStruct(device, 2, axes_labels[2],
                               0, 65535, 1, 0, 1, Absolute);
        /* tilt x */
        InitValuatorAxisStruct(device, 3, axes_labels[3],
                               -64, 63, 57, 0, 57, Absolute);
        /* tilt y */
        InitValuatorAxisStruct(device, 4, axes_labels[4],
                               -64, 63, 57, 0, 57, Absolute);
        /* abs wheel (airbrush) or rotation (artpen) */
        InitValuatorAxisStruct(device, 5, axes_labels[5],
                               -900, 899, 1, 0, 1, Absolute);

        if (!InitPtrFeedbackClassDeviceStruct(device, xwl_pointer_control))
            return BadValue;

        if (!InitButtonClassDeviceStruct(device, NBUTTONS, btn_labels, map))
            return BadValue;

        return Success;

    case DEVICE_ON:
        device->public.on = TRUE;
        return Success;

    case DEVICE_OFF:
    case DEVICE_CLOSE:
        device->public.on = FALSE;
        return Success;
    }

    return BadMatch;
#undef NAXES
#undef NBUTTONS
}

static void
pointer_handle_enter(void *data, struct wl_pointer *pointer,
                     uint32_t serial, struct wl_surface *surface,
                     wl_fixed_t sx_w, wl_fixed_t sy_w)
{
    struct xwl_seat *xwl_seat = data;
    struct xwl_screen *xwl_screen = xwl_seat->xwl_screen;
    DeviceIntPtr dev = get_pointer_device(xwl_seat);
    DeviceIntPtr master;
    int i;
    int sx, sy;
    int dx, dy;
    ScreenPtr pScreen = xwl_screen->screen;
    ValuatorMask mask;

    /* There's a race here where if we create and then immediately
     * destroy a surface, we might end up in a state where the Wayland
     * compositor sends us an event for a surface that doesn't exist.
     *
     * Don't process enter events in this case.
     */
    if (surface == NULL)
        return;

    if (!is_surface_from_xwl_window(surface))
        return;

    sx = wl_fixed_to_int(sx_w) * xwl_screen->global_surface_scale;
    sy = wl_fixed_to_int(sy_w) * xwl_screen->global_surface_scale;

    xwl_seat->xwl_screen->serial = serial;
    xwl_seat->pointer_enter_serial = serial;

    xwl_seat->focus_window = wl_surface_get_user_data(surface);
    dx = xwl_seat->focus_window->toplevel->drawable.x;
    dy = xwl_seat->focus_window->toplevel->drawable.y;

    /* We just entered a new xwindow, forget about the old last xwindow */
    xwl_seat->last_focus_window = NULL;

    master = GetMaster(dev, POINTER_OR_FLOAT);
    (*pScreen->SetCursorPosition) (dev, pScreen, dx + sx, dy + sy, TRUE);

    miPointerInvalidateSprite(master);

    CheckMotion(NULL, master);

    /* Ideally, X clients shouldn't see these button releases.  When
     * the pointer leaves a window with buttons down, it means that
     * the wayland compositor has grabbed the pointer.  The button
     * release event is consumed by whatever grab in the compositor
     * and won't be sent to clients (the X server is a client).
     * However, we need to reset X's idea of which buttons are up and
     * down, and they're all up (by definition) when the pointer
     * enters a window.  We should figure out a way to swallow these
     * events, perhaps using an X grab whenever the pointer is not in
     * any X window, but for now just send the events. */
    valuator_mask_zero(&mask);
    for (i = 0; i < dev->button->numButtons; i++)
        if (BitIsOn(dev->button->down, i))
            QueuePointerEvents(dev, ButtonRelease, i, 0, &mask);

    /* The last cursor frame we committed before the pointer left one
     * of our surfaces might not have been shown. In that case we'll
     * have a cursor surface frame callback pending which we need to
     * clear so that we can continue submitting new cursor frames. */
    if (xwl_cursor_clear_frame_cb(&xwl_seat->cursor))
        xwl_seat_set_cursor(xwl_seat);

    if (xwl_seat->pointer_warp_emulator) {
        xwl_pointer_warp_emulator_maybe_lock(xwl_seat->pointer_warp_emulator,
                                             xwl_seat->focus_window,
                                             NULL, 0, 0);
    }
    else {
        xwl_seat_maybe_lock_on_hidden_cursor(xwl_seat);
    }

    maybe_fake_grab_devices(xwl_seat);
}

void
xwl_seat_leave_ptr(struct xwl_seat *xwl_seat, Bool focus_lost)
{
    DeviceIntPtr dev = get_pointer_device(xwl_seat);

    if (!dev)
        return;

    if (focus_lost)
        CheckMotion(NULL, GetMaster(dev, POINTER_OR_FLOAT));

    maybe_fake_ungrab_devices(xwl_seat);
}

static void
pointer_handle_leave(void *data, struct wl_pointer *pointer,
                     uint32_t serial, struct wl_surface *surface)
{
    struct xwl_seat *xwl_seat = data;
    struct xwl_screen *xwl_screen = xwl_seat->xwl_screen;
    Bool focus_lost = FALSE;

    xwl_screen->serial = serial;

    /* The pointer has left a known xwindow, save it for a possible match
     * in sprite_check_lost_focus()
     */
    if (xwl_seat->focus_window) {
        xwl_seat->last_focus_window = xwl_seat->focus_window;
        xwl_seat->focus_window = NULL;
        focus_lost = TRUE;
    }

    if (xwl_screen->rootless)
        xwl_seat_leave_ptr(xwl_seat, focus_lost);
}

static void
dispatch_relative_motion_with_warp(struct xwl_seat *xwl_seat)
{
    struct xwl_screen *xwl_screen = xwl_seat->xwl_screen;
    double dx, dx_unaccel;
    double dy, dy_unaccel;

    dx = xwl_seat->pending_pointer_event.dx;
    dy = xwl_seat->pending_pointer_event.dy;
    dx_unaccel = xwl_seat->pending_pointer_event.dx_unaccel;
    dy_unaccel = xwl_seat->pending_pointer_event.dy_unaccel;

    dx *= xwl_screen->global_surface_scale;
    dy *= xwl_screen->global_surface_scale;
    dx_unaccel *= xwl_screen->global_surface_scale;
    dy_unaccel *= xwl_screen->global_surface_scale;

    dx *= xwl_seat->focus_window->viewport_scale_x;
    dy *= xwl_seat->focus_window->viewport_scale_y;
    dx_unaccel *= xwl_seat->focus_window->viewport_scale_x;
    dy_unaccel *= xwl_seat->focus_window->viewport_scale_y;

    xwl_pointer_warp_emulator_handle_motion(xwl_seat->pointer_warp_emulator,
                                            dx, dy,
                                            dx_unaccel, dy_unaccel);
}

static void
dispatch_absolute_motion(struct xwl_seat *xwl_seat)
{
    struct xwl_screen *xwl_screen = xwl_seat->xwl_screen;
    ValuatorMask mask;
    DeviceIntPtr device;
    int flags;
    int event_x = wl_fixed_to_int(xwl_seat->pending_pointer_event.x);
    int event_y = wl_fixed_to_int(xwl_seat->pending_pointer_event.y);
    int drawable_x = xwl_seat->focus_window->toplevel->drawable.x;
    int drawable_y = xwl_seat->focus_window->toplevel->drawable.y;
    int x;
    int y;

    event_x *= xwl_screen->global_surface_scale;
    event_y *= xwl_screen->global_surface_scale;

    event_x *= xwl_seat->focus_window->viewport_scale_x;
    event_y *= xwl_seat->focus_window->viewport_scale_y;

    x = drawable_x + event_x;
    y = drawable_y + event_y;

    valuator_mask_zero(&mask);
    valuator_mask_set(&mask, 0, x);
    valuator_mask_set(&mask, 1, y);

    if (xwl_seat->pending_pointer_event.has_relative) {
         flags = POINTER_ABSOLUTE | POINTER_SCREEN | POINTER_NORAW;
         device = xwl_seat->relative_pointer;
    } else {
         flags = POINTER_ABSOLUTE | POINTER_SCREEN;
         device = xwl_seat->pointer;
    }

    QueuePointerEvents(device, MotionNotify, 0, flags, &mask);
}

static void
dispatch_relative_motion(struct xwl_seat *xwl_seat)
{
    struct xwl_screen *xwl_screen = xwl_seat->xwl_screen;
    ValuatorMask mask;
    double event_dx = xwl_seat->pending_pointer_event.dx;
    double event_dy = xwl_seat->pending_pointer_event.dy;
    double event_dx_unaccel = xwl_seat->pending_pointer_event.dx_unaccel;
    double event_dy_unaccel = xwl_seat->pending_pointer_event.dy_unaccel;

    event_dx *= xwl_screen->global_surface_scale;
    event_dy *= xwl_screen->global_surface_scale;
    event_dx_unaccel *= xwl_screen->global_surface_scale;
    event_dy_unaccel *= xwl_screen->global_surface_scale;

    event_dx *= xwl_seat->focus_window->viewport_scale_x;
    event_dy *= xwl_seat->focus_window->viewport_scale_y;
    event_dx_unaccel *= xwl_seat->focus_window->viewport_scale_x;
    event_dy_unaccel *= xwl_seat->focus_window->viewport_scale_y;

    valuator_mask_zero(&mask);
    valuator_mask_set_unaccelerated(&mask, 0, event_dx, event_dx_unaccel);
    valuator_mask_set_unaccelerated(&mask, 1, event_dy, event_dy_unaccel);

    QueuePointerEvents(xwl_seat->relative_pointer, MotionNotify, 0,
                       POINTER_RAWONLY, &mask);
}

static void
dispatch_scroll_motion(struct xwl_seat *xwl_seat)
{
    ValuatorMask mask;
    const int divisor = 10;
    wl_fixed_t dy = xwl_seat->pending_pointer_event.scroll_dy;
    wl_fixed_t dx = xwl_seat->pending_pointer_event.scroll_dx;
    int32_t dy_v120 = xwl_seat->pending_pointer_event.scroll_dy_v120;
    int32_t dx_v120 = xwl_seat->pending_pointer_event.scroll_dx_v120;

    valuator_mask_zero(&mask);
    if (xwl_seat->pending_pointer_event.has_vertical_scroll_v120)
        valuator_mask_set_double(&mask, SCROLL_AXIS_VERT, dy_v120 / 120.0);
    else if (xwl_seat->pending_pointer_event.has_vertical_scroll)
        valuator_mask_set_double(&mask,
                                 SCROLL_AXIS_VERT,
                                 wl_fixed_to_double(dy) / divisor);

    if (xwl_seat->pending_pointer_event.has_horizontal_scroll_v120)
        valuator_mask_set_double(&mask, SCROLL_AXIS_HORIZ, dx_v120 / 120.0);
    else if (xwl_seat->pending_pointer_event.has_horizontal_scroll)
        valuator_mask_set_double(&mask,
                                 SCROLL_AXIS_HORIZ,
                                 wl_fixed_to_double(dx) / divisor);

    QueuePointerEvents(get_pointer_device(xwl_seat),
                       MotionNotify, 0, POINTER_RELATIVE, &mask);
}


static void
dispatch_pointer_motion_event(struct xwl_seat *xwl_seat)
{
    Bool has_relative = xwl_seat->pending_pointer_event.has_relative;
    Bool has_absolute = xwl_seat->pending_pointer_event.has_absolute;

    if (xwl_seat->pointer_warp_emulator && has_relative) {
        dispatch_relative_motion_with_warp(xwl_seat);
    } else {
        if (has_relative)
            dispatch_relative_motion(xwl_seat);

        if (has_absolute)
            dispatch_absolute_motion(xwl_seat);
    }

    if (xwl_seat->pending_pointer_event.has_vertical_scroll ||
        xwl_seat->pending_pointer_event.has_horizontal_scroll ||
        xwl_seat->pending_pointer_event.has_vertical_scroll_v120 ||
        xwl_seat->pending_pointer_event.has_horizontal_scroll_v120)
        dispatch_scroll_motion(xwl_seat);

    xwl_seat->pending_pointer_event.has_absolute = FALSE;
    xwl_seat->pending_pointer_event.has_relative = FALSE;
    xwl_seat->pending_pointer_event.has_vertical_scroll = FALSE;
    xwl_seat->pending_pointer_event.has_horizontal_scroll = FALSE;
    xwl_seat->pending_pointer_event.has_vertical_scroll_v120 = FALSE;
    xwl_seat->pending_pointer_event.has_horizontal_scroll_v120 = FALSE;
}

static void
pointer_handle_motion(void *data, struct wl_pointer *pointer,
                      uint32_t time, wl_fixed_t sx_w, wl_fixed_t sy_w)
{
    struct xwl_seat *xwl_seat = data;

    if (!xwl_seat->focus_window)
        return;

    xwl_seat->pending_pointer_event.has_absolute = TRUE;
    xwl_seat->pending_pointer_event.x = sx_w;
    xwl_seat->pending_pointer_event.y = sy_w;

    if (wl_proxy_get_version((struct wl_proxy *) xwl_seat->wl_pointer) < 5)
        dispatch_pointer_motion_event(xwl_seat);
}

static void
pointer_handle_button(void *data, struct wl_pointer *pointer, uint32_t serial,
                      uint32_t time, uint32_t button, uint32_t state)
{
    struct xwl_seat *xwl_seat = data;
    int index;
    ValuatorMask mask;

    xwl_seat->xwl_screen->serial = serial;

    switch (button) {
    case BTN_LEFT:
        index = 1;
        break;
    case BTN_MIDDLE:
        index = 2;
        break;
    case BTN_RIGHT:
        index = 3;
        break;
    default:
        /* Skip indexes 4-7: they are used for vertical and horizontal scroll.
           The rest of the buttons go in order: BTN_SIDE becomes 8, etc. */
        index = 8 + button - BTN_SIDE;
        break;
    }

    valuator_mask_zero(&mask);
    QueuePointerEvents(get_pointer_device(xwl_seat),
                       state ? ButtonPress : ButtonRelease, index, 0, &mask);
}

static void
pointer_handle_axis(void *data, struct wl_pointer *pointer,
                    uint32_t time, uint32_t axis, wl_fixed_t value)
{
    struct xwl_seat *xwl_seat = data;

    switch (axis) {
    case WL_POINTER_AXIS_VERTICAL_SCROLL:
        xwl_seat->pending_pointer_event.has_vertical_scroll = TRUE;
        xwl_seat->pending_pointer_event.scroll_dy = value;
        break;
    case WL_POINTER_AXIS_HORIZONTAL_SCROLL:
        xwl_seat->pending_pointer_event.has_horizontal_scroll = TRUE;
        xwl_seat->pending_pointer_event.scroll_dx = value;
        break;
    }
}

static void
pointer_handle_frame(void *data, struct wl_pointer *wl_pointer)
{
    struct xwl_seat *xwl_seat = data;

    if (!xwl_seat->focus_window)
        return;

    dispatch_pointer_motion_event(xwl_seat);
}

static void
pointer_handle_axis_source(void *data, struct wl_pointer *wl_pointer, uint32_t axis_source)
{
}

static void
pointer_handle_axis_stop(void *data, struct wl_pointer *wl_pointer,
                         uint32_t time, uint32_t axis)
{
    struct xwl_seat *xwl_seat = data;

    switch (axis) {
    case WL_POINTER_AXIS_VERTICAL_SCROLL:
        xwl_seat->pending_pointer_event.has_vertical_scroll = TRUE;
        xwl_seat->pending_pointer_event.scroll_dy = 0;
        break;
    case WL_POINTER_AXIS_HORIZONTAL_SCROLL:
        xwl_seat->pending_pointer_event.has_horizontal_scroll = TRUE;
        xwl_seat->pending_pointer_event.scroll_dx = 0;
        break;
    }
}

static void
pointer_handle_axis_discrete(void *data, struct wl_pointer *wl_pointer,
                             uint32_t axis, int32_t discrete)
{
    struct xwl_seat *xwl_seat = data;

    switch (axis) {
    case WL_POINTER_AXIS_VERTICAL_SCROLL:
        xwl_seat->pending_pointer_event.has_vertical_scroll_v120 = TRUE;
        xwl_seat->pending_pointer_event.scroll_dy_v120 = 120 * discrete;
        break;
    case WL_POINTER_AXIS_HORIZONTAL_SCROLL:
        xwl_seat->pending_pointer_event.has_horizontal_scroll_v120 = TRUE;
        xwl_seat->pending_pointer_event.scroll_dx_v120 = 120 * discrete;
        break;
    }
}

static void
pointer_handle_axis_v120(void *data, struct wl_pointer *pointer,
                         uint32_t axis, int32_t v120)
{
    struct xwl_seat *xwl_seat = data;

    switch (axis) {
    case WL_POINTER_AXIS_VERTICAL_SCROLL:
        xwl_seat->pending_pointer_event.has_vertical_scroll_v120 = TRUE;
        xwl_seat->pending_pointer_event.scroll_dy_v120 = v120;
        break;
    case WL_POINTER_AXIS_HORIZONTAL_SCROLL:
        xwl_seat->pending_pointer_event.has_horizontal_scroll_v120 = TRUE;
        xwl_seat->pending_pointer_event.scroll_dx_v120 = v120;
        break;
    }
}

static const struct wl_pointer_listener pointer_listener = {
    pointer_handle_enter,
    pointer_handle_leave,
    pointer_handle_motion,
    pointer_handle_button,
    pointer_handle_axis,
    pointer_handle_frame,
    pointer_handle_axis_source,
    pointer_handle_axis_stop,
    pointer_handle_axis_discrete,
    pointer_handle_axis_v120,
};

static void
relative_pointer_handle_relative_motion(void *data,
                                        struct zwp_relative_pointer_v1 *zwp_relative_pointer_v1,
                                        uint32_t utime_hi,
                                        uint32_t utime_lo,
                                        wl_fixed_t dxf,
                                        wl_fixed_t dyf,
                                        wl_fixed_t dx_unaccelf,
                                        wl_fixed_t dy_unaccelf)
{
    struct xwl_seat *xwl_seat = data;

    xwl_seat->pending_pointer_event.has_relative = TRUE;
    xwl_seat->pending_pointer_event.dx = wl_fixed_to_double(dxf);
    xwl_seat->pending_pointer_event.dy = wl_fixed_to_double(dyf);
    xwl_seat->pending_pointer_event.dx_unaccel = wl_fixed_to_double(dx_unaccelf);
    xwl_seat->pending_pointer_event.dy_unaccel = wl_fixed_to_double(dy_unaccelf);

    if (!xwl_seat->focus_window)
        return;

    if (wl_proxy_get_version((struct wl_proxy *) xwl_seat->wl_pointer) < 5)
        dispatch_pointer_motion_event(xwl_seat);
}

static const struct zwp_relative_pointer_v1_listener relative_pointer_listener = {
    relative_pointer_handle_relative_motion,
};

static void
pointer_gesture_swipe_handle_begin(void *data,
                                   struct zwp_pointer_gesture_swipe_v1 *swipe,
                                   uint32_t serial,
                                   uint32_t time,
                                   struct wl_surface *surface,
                                   uint32_t fingers)
{
    struct xwl_seat *xwl_seat = data;

    if (surface != NULL && !is_surface_from_xwl_window(surface))
        return;

    xwl_seat->pointer_gesture_swipe_fingers = fingers;
    QueueGestureSwipeEvents(xwl_seat->pointer_gestures,
                            XI_GestureSwipeBegin, fingers, 0, 0.0, 0.0, 0.0, 0.0);
}

static void
pointer_gesture_swipe_handle_update(void *data,
                                    struct zwp_pointer_gesture_swipe_v1 *swipe,
                                    uint32_t time,
                                    wl_fixed_t dxf,
                                    wl_fixed_t dyf)
{
    struct xwl_seat *xwl_seat = data;
    double dx = wl_fixed_to_double(dxf);
    double dy = wl_fixed_to_double(dyf);

    QueueGestureSwipeEvents(xwl_seat->pointer_gestures,
                            XI_GestureSwipeUpdate,
                            xwl_seat->pointer_gesture_swipe_fingers,
                            0,
                            dx, dy,
                            dx, dy);
}

static void
pointer_gesture_swipe_handle_end(void *data,
                                 struct zwp_pointer_gesture_swipe_v1 *swipe,
                                 uint32_t serial,
                                 uint32_t time,
                                 int32_t cancelled)
{
    struct xwl_seat *xwl_seat = data;
    uint32_t flags = 0;

    if (cancelled)
        flags |= XIGestureSwipeEventCancelled;

    QueueGestureSwipeEvents(xwl_seat->pointer_gestures,
                            XI_GestureSwipeEnd,
                            xwl_seat->pointer_gesture_swipe_fingers,
                            flags, 0.0, 0.0, 0.0, 0.0);
}

static const struct zwp_pointer_gesture_swipe_v1_listener pointer_gesture_swipe_listener = {
    pointer_gesture_swipe_handle_begin,
    pointer_gesture_swipe_handle_update,
    pointer_gesture_swipe_handle_end
};

static void
pointer_gesture_pinch_handle_begin(void *data,
                                   struct zwp_pointer_gesture_pinch_v1 *pinch,
                                   uint32_t serial,
                                   uint32_t time,
                                   struct wl_surface *surface,
                                   uint32_t fingers)
{
    struct xwl_seat *xwl_seat = data;

    if (surface != NULL && !is_surface_from_xwl_window(surface))
        return;

    xwl_seat->pointer_gesture_pinch_fingers = fingers;
    xwl_seat->pointer_gesture_pinch_last_scale = 1.0;
    QueueGesturePinchEvents(xwl_seat->pointer_gestures,
                            XI_GesturePinchBegin, fingers, 0, 0.0, 0.0, 0.0, 0.0,
                            1.0, 0.0);
}

static void
pointer_gesture_pinch_handle_update(void *data,
                                    struct zwp_pointer_gesture_pinch_v1 *pinch,
                                    uint32_t time,
                                    wl_fixed_t dxf,
                                    wl_fixed_t dyf,
                                    wl_fixed_t scalef,
                                    wl_fixed_t rotation)
{
    struct xwl_seat *xwl_seat = data;
    double dx = wl_fixed_to_double(dxf);
    double dy = wl_fixed_to_double(dyf);
    double scale = wl_fixed_to_double(scalef);

    xwl_seat->pointer_gesture_pinch_last_scale = scale;
    QueueGesturePinchEvents(xwl_seat->pointer_gestures,
                            XI_GesturePinchUpdate,
                            xwl_seat->pointer_gesture_pinch_fingers,
                            0,
                            dx, dy,
                            dx, dy,
                            scale, wl_fixed_to_double(rotation));
}

static void
pointer_gesture_pinch_handle_end(void *data,
                                 struct zwp_pointer_gesture_pinch_v1 *pinch,
                                 uint32_t serial,
                                 uint32_t time,
                                 int32_t cancelled)
{
    struct xwl_seat *xwl_seat = data;
    uint32_t flags = 0;

    if (cancelled)
        flags |= XIGesturePinchEventCancelled;

    QueueGesturePinchEvents(xwl_seat->pointer_gestures,
                            XI_GesturePinchEnd,
                            xwl_seat->pointer_gesture_pinch_fingers,
                            flags, 0.0, 0.0, 0.0, 0.0,
                            xwl_seat->pointer_gesture_pinch_last_scale, 0.0);
}

static const struct zwp_pointer_gesture_pinch_v1_listener pointer_gesture_pinch_listener = {
    pointer_gesture_pinch_handle_begin,
    pointer_gesture_pinch_handle_update,
    pointer_gesture_pinch_handle_end
};

static void
maybe_toggle_fake_grab(struct xwl_seat *xwl_seat, uint32_t key)
{
    struct xwl_screen *xwl_screen = xwl_seat->xwl_screen;
    struct xwl_window *xwl_window;
    XkbStateRec state_rec;
    uint32_t xkb_state;

    if (xwl_screen->rootless)
        return;

    if (!xwl_screen->host_grab)
        return;

    state_rec = xwl_seat->keyboard->key->xkbInfo->state;
    xkb_state = (XkbStateFieldFromRec(&state_rec) & 0xff);

    if (((key == KEY_LEFTSHIFT || key == KEY_RIGHTSHIFT) && (xkb_state & ControlMask)) ||
        ((key == KEY_LEFTCTRL || key == KEY_RIGHTCTRL) && (xkb_state & ShiftMask))) {

        xwl_screen->has_grab = !xwl_screen->has_grab;

        if (xwl_screen->has_grab)
            maybe_fake_grab_devices(xwl_seat);
        else
            maybe_fake_ungrab_devices(xwl_seat);

        xwl_window = xwl_window_get(xwl_screen->screen->root);
        if (xwl_window)
            xwl_window_rootful_update_title(xwl_window);
    }
}

static void
keyboard_handle_key(void *data, struct wl_keyboard *keyboard, uint32_t serial,
                    uint32_t time, uint32_t key, uint32_t state)
{
    struct xwl_seat *xwl_seat = data;
    uint32_t *k, *end;

    xwl_seat->xwl_screen->serial = serial;

    end = (uint32_t *) ((char *) xwl_seat->keys.data + xwl_seat->keys.size);
    for (k = xwl_seat->keys.data; k < end; k++) {
        if (*k == key)
            *k = *--end;
    }
    xwl_seat->keys.size = (char *) end - (char *) xwl_seat->keys.data;
    if (state) {
        k = wl_array_add(&xwl_seat->keys, sizeof *k);
        *k = key;
    }

    QueueKeyboardEvents(xwl_seat->keyboard,
                        state ? KeyPress : KeyRelease, key + 8);

    if (!state)
        maybe_toggle_fake_grab(xwl_seat, key);
}

static void
keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard,
                       uint32_t format, int fd, uint32_t size)
{
    struct xwl_seat *xwl_seat = data;
    struct xwl_screen *xwl_screen = xwl_seat->xwl_screen;
    DeviceIntPtr master;
    XkbDescPtr xkb;
    XkbChangesRec changes = { 0 };

    if (xwl_screen->nokeymap)
        return;

    if (xwl_seat->keymap)
        munmap(xwl_seat->keymap, xwl_seat->keymap_size);

    xwl_seat->keymap_size = size;
    xwl_seat->keymap = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (xwl_seat->keymap == MAP_FAILED) {
        xwl_seat->keymap_size = 0;
        xwl_seat->keymap = NULL;
        goto out;
    }

    xkb = XkbCompileKeymapFromString(xwl_seat->keyboard, xwl_seat->keymap,
                                     strnlen(xwl_seat->keymap,
                                             xwl_seat->keymap_size));
    if (!xkb)
        goto out;

    XkbUpdateDescActions(xkb, xkb->min_key_code, XkbNumKeys(xkb), &changes);

    if (xwl_seat->keyboard->key)
        /* Keep the current controls */
        XkbCopyControls(xkb, xwl_seat->keyboard->key->xkbInfo->desc);

    XkbDeviceApplyKeymap(xwl_seat->keyboard, xkb);

    master = GetMaster(xwl_seat->keyboard, MASTER_KEYBOARD);
    if (master)
        XkbDeviceApplyKeymap(master, xkb);

    XkbFreeKeyboard(xkb, XkbAllComponentsMask, TRUE);

 out:
    close(fd);
}

static void
keyboard_handle_enter(void *data, struct wl_keyboard *keyboard,
                      uint32_t serial,
                      struct wl_surface *surface, struct wl_array *keys)
{
    struct xwl_seat *xwl_seat = data;
    uint32_t *k;

    if (surface != NULL && !is_surface_from_xwl_window(surface))
        return;

    xwl_seat->xwl_screen->serial = serial;
    xwl_seat->keyboard_focus = surface;

    /* If `leave` wasn't sent (for a destroyed surface), release keys here. */
    wl_array_for_each(k, &xwl_seat->keys)
        QueueKeyboardEvents(xwl_seat->keyboard, LeaveNotify, *k + 8);

    wl_array_copy(&xwl_seat->keys, keys);
    wl_array_for_each(k, &xwl_seat->keys)
        QueueKeyboardEvents(xwl_seat->keyboard, EnterNotify, *k + 8);

    maybe_fake_grab_devices(xwl_seat);
}

void
xwl_seat_leave_kbd(struct xwl_seat *xwl_seat)
{
    uint32_t *k;

    wl_array_for_each(k, &xwl_seat->keys)
        QueueKeyboardEvents(xwl_seat->keyboard, LeaveNotify, *k + 8);
    xwl_seat->keys.size = 0;

    xwl_seat->keyboard_focus = NULL;

    maybe_fake_ungrab_devices(xwl_seat);
}

static void
keyboard_handle_leave(void *data, struct wl_keyboard *keyboard,
                      uint32_t serial, struct wl_surface *surface)
{
    struct xwl_seat *xwl_seat = data;
    struct xwl_screen *xwl_screen = xwl_seat->xwl_screen;

    xwl_screen->serial = serial;

    if (xwl_screen->rootless)
        xwl_seat_leave_kbd(xwl_seat);
}

static void
keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard,
                          uint32_t serial, uint32_t mods_depressed,
                          uint32_t mods_latched, uint32_t mods_locked,
                          uint32_t group)
{
    struct xwl_seat *xwl_seat = data;
    DeviceIntPtr dev;
    XkbStateRec old_state, *new_state;
    xkbStateNotify sn;
    CARD16 changed;

    mieqProcessInputEvents();

    for (dev = inputInfo.devices; dev; dev = dev->next) {
        if (dev != xwl_seat->keyboard &&
            dev != GetMaster(xwl_seat->keyboard, MASTER_KEYBOARD))
            continue;

        old_state = dev->key->xkbInfo->state;
        new_state = &dev->key->xkbInfo->state;

        new_state->locked_group = group & XkbAllGroupsMask;
        new_state->base_mods = mods_depressed & XkbAllModifiersMask;
        new_state->locked_mods = mods_locked & XkbAllModifiersMask;
        XkbLatchModifiers(dev, XkbAllModifiersMask,
                          mods_latched & XkbAllModifiersMask);

        XkbComputeDerivedState(dev->key->xkbInfo);

        changed = XkbStateChangedFlags(&old_state, new_state);
        if (!changed)
            continue;

        sn.keycode = 0;
        sn.eventType = 0;
        sn.requestMajor = XkbReqCode;
        sn.requestMinor = X_kbLatchLockState;   /* close enough */
        sn.changed = changed;
        XkbSendStateNotify(dev, &sn);
    }
}

static void
remove_sync_pending(DeviceIntPtr dev)
{
    struct xwl_seat *xwl_seat = dev->public.devicePrivate;
    struct sync_pending *p, *npd;

    if (!xwl_seat)
        return;

    xorg_list_for_each_entry_safe(p, npd, &xwl_seat->sync_pending, l) {
        if (p->pending_dev == dev) {
            xorg_list_del(&xwl_seat->sync_pending);
            free (p);
            return;
        }
    }
}

static void
sync_callback(void *data, struct wl_callback *callback, uint32_t serial)
{
    DeviceIntPtr dev = (DeviceIntPtr) data;

    remove_sync_pending(dev);
    wl_callback_destroy(callback);
}

static const struct wl_callback_listener sync_listener = {
   sync_callback
};

static Bool
keyboard_check_repeat (DeviceIntPtr dev, XkbSrvInfoPtr xkbi, unsigned key)
{
    struct xwl_seat *xwl_seat = dev->public.devicePrivate;
    struct xwl_screen *xwl_screen;
    struct wl_callback *callback;
    struct sync_pending *p;

    if (!xwl_seat)
        return FALSE;

    /* Make sure we didn't miss a possible reply from the compositor */
    xwl_screen = xwl_seat->xwl_screen;
    xwl_sync_events (xwl_screen);

    xorg_list_for_each_entry(p, &xwl_seat->sync_pending, l) {
        if (p->pending_dev == dev) {
            ErrorF("Key repeat discarded, Wayland compositor doesn't "
                   "seem to be processing events fast enough!\n");

            return FALSE;
        }
    }

    p = XNFalloc(sizeof(struct sync_pending));
    p->pending_dev = dev;
    callback = wl_display_sync (xwl_screen->display);
    xorg_list_add(&p->l, &xwl_seat->sync_pending);

    wl_callback_add_listener(callback, &sync_listener, dev);

    return TRUE;
}

static void
keyboard_handle_repeat_info (void *data, struct wl_keyboard *keyboard,
                             int32_t rate, int32_t delay)
{
    struct xwl_seat *xwl_seat = data;
    DeviceIntPtr dev;
    XkbControlsPtr ctrl;

    if (rate < 0 || delay < 0) {
        ErrorF("Wrong rate/delay: %d, %d\n", rate, delay);
        return;
    }

    for (dev = inputInfo.devices; dev; dev = dev->next) {
        if (dev != xwl_seat->keyboard &&
            dev != GetMaster(xwl_seat->keyboard, MASTER_KEYBOARD))
            continue;

        if (rate != 0) {
            ctrl = dev->key->xkbInfo->desc->ctrls;
            ctrl->repeat_delay = delay;
            /* rate is number of keys per second */
            ctrl->repeat_interval = 1000 / rate;

            XkbSetRepeatKeys(dev, -1, AutoRepeatModeOn);
        } else
            XkbSetRepeatKeys(dev, -1, AutoRepeatModeOff);
    }
}

static const struct wl_keyboard_listener keyboard_listener = {
    keyboard_handle_keymap,
    keyboard_handle_enter,
    keyboard_handle_leave,
    keyboard_handle_key,
    keyboard_handle_modifiers,
    keyboard_handle_repeat_info,
};

static struct xwl_touch *
xwl_seat_lookup_touch(struct xwl_seat *xwl_seat, int32_t id)
{
    struct xwl_touch *xwl_touch, *next_xwl_touch;

    xorg_list_for_each_entry_safe(xwl_touch, next_xwl_touch,
                                  &xwl_seat->touches, link_touch) {
        if (xwl_touch->id == id)
            return xwl_touch;
    }

    return NULL;
}

static void
xwl_touch_send_event(struct xwl_touch *xwl_touch,
                     struct xwl_seat *xwl_seat, int type)
{
    double dx, dy, x, y;
    ValuatorMask mask;

    dx = xwl_touch->window->toplevel->drawable.x;
    dy = xwl_touch->window->toplevel->drawable.y;

    x = (dx + xwl_touch->x) * 0xFFFF / xwl_screen_get_width(xwl_seat->xwl_screen);
    y = (dy + xwl_touch->y) * 0xFFFF / xwl_screen_get_height(xwl_seat->xwl_screen);

    valuator_mask_zero(&mask);
    valuator_mask_set_double(&mask, 0, x);
    valuator_mask_set_double(&mask, 1, y);
    QueueTouchEvents(xwl_seat->touch, type, xwl_touch->id, 0, &mask);
}

static void
touch_handle_down(void *data, struct wl_touch *wl_touch,
                  uint32_t serial, uint32_t time,
                  struct wl_surface *surface,
                  int32_t id, wl_fixed_t sx_w, wl_fixed_t sy_w)
{
    struct xwl_seat *xwl_seat = data;
    struct xwl_screen *xwl_screen = xwl_seat->xwl_screen;
    struct xwl_touch *xwl_touch;

    if (surface == NULL)
        return;

    if (!is_surface_from_xwl_window(surface))
        return;

    xwl_touch = calloc(1, sizeof *xwl_touch);
    if (xwl_touch == NULL) {
        ErrorF("%s: ENOMEM\n", __func__);
        return;
    }

    xwl_touch->window = wl_surface_get_user_data(surface);
    xwl_touch->id = id;
    xwl_touch->x = wl_fixed_to_int(sx_w);
    xwl_touch->y = wl_fixed_to_int(sy_w);
    xorg_list_add(&xwl_touch->link_touch, &xwl_seat->touches);

    xwl_touch->x *= xwl_screen->global_surface_scale;
    xwl_touch->y *= xwl_screen->global_surface_scale;

    xwl_touch->x *= xwl_touch->window->viewport_scale_x;
    xwl_touch->y *= xwl_touch->window->viewport_scale_y;

    xwl_touch_send_event(xwl_touch, xwl_seat, XI_TouchBegin);
}

static void
touch_handle_up(void *data, struct wl_touch *wl_touch,
                uint32_t serial, uint32_t time, int32_t id)
{
    struct xwl_touch *xwl_touch;
    struct xwl_seat *xwl_seat = data;

    xwl_touch = xwl_seat_lookup_touch(xwl_seat, id);

    if (!xwl_touch)
        return;

    xwl_touch_send_event(xwl_touch, xwl_seat, XI_TouchEnd);
    xorg_list_del(&xwl_touch->link_touch);
    free(xwl_touch);
}

static void
touch_handle_motion(void *data, struct wl_touch *wl_touch,
                    uint32_t time, int32_t id,
                    wl_fixed_t sx_w, wl_fixed_t sy_w)
{
    struct xwl_seat *xwl_seat = data;
    struct xwl_screen *xwl_screen = xwl_seat->xwl_screen;
    struct xwl_touch *xwl_touch;

    xwl_touch = xwl_seat_lookup_touch(xwl_seat, id);

    if (!xwl_touch)
        return;

    xwl_touch->x = wl_fixed_to_int(sx_w);
    xwl_touch->y = wl_fixed_to_int(sy_w);

    xwl_touch->x *= xwl_screen->global_surface_scale;
    xwl_touch->y *= xwl_screen->global_surface_scale;

    xwl_touch->x *= xwl_touch->window->viewport_scale_x;
    xwl_touch->y *= xwl_touch->window->viewport_scale_y;

    xwl_touch_send_event(xwl_touch, xwl_seat, XI_TouchUpdate);
}

static void
touch_handle_frame(void *data, struct wl_touch *wl_touch)
{
}

static void
touch_handle_cancel(void *data, struct wl_touch *wl_touch)
{
    struct xwl_seat *xwl_seat = data;
    struct xwl_touch *xwl_touch, *next_xwl_touch;

    xorg_list_for_each_entry_safe(xwl_touch, next_xwl_touch,
                                  &xwl_seat->touches, link_touch) {
        /* We can't properly notify of cancellation to the X client
         * once it thinks it has the ownership, send at least a
         * TouchEnd event.
         */
        xwl_touch_send_event(xwl_touch, xwl_seat, XI_TouchEnd);
        xorg_list_del(&xwl_touch->link_touch);
        free(xwl_touch);
    }
}

static const struct wl_touch_listener touch_listener = {
    touch_handle_down,
    touch_handle_up,
    touch_handle_motion,
    touch_handle_frame,
    touch_handle_cancel
};

static struct xwl_seat *
find_matching_seat(DeviceIntPtr device)
{
    DeviceIntPtr dev;

    for (dev = inputInfo.devices; dev; dev = dev->next)
        if (dev->deviceProc == xwl_keyboard_proc &&
            device == GetMaster(dev, MASTER_KEYBOARD))
                return (struct xwl_seat *) dev->public.devicePrivate;

    return NULL;
}

static void
release_grab(struct xwl_seat *xwl_seat)
{
    if (xwl_seat->keyboard_grab)
        zwp_xwayland_keyboard_grab_v1_destroy(xwl_seat->keyboard_grab);
    xwl_seat->keyboard_grab = NULL;
}

static void
set_grab(struct xwl_seat *xwl_seat, struct xwl_window *xwl_window)
{
    struct xwl_screen *xwl_screen;

    if (!xwl_window)
        return;

    /* We already have a grab */
    if (xwl_seat->keyboard_grab)
        release_grab (xwl_seat);

    xwl_screen = xwl_seat->xwl_screen;
    xwl_seat->keyboard_grab =
        zwp_xwayland_keyboard_grab_manager_v1_grab_keyboard(xwl_screen->wp_grab,
                                                            xwl_window->surface,
                                                            xwl_seat->seat);
}

static void
find_toplevel_callback(void *resource, XID id, void *user_data)
{
    WindowPtr window = resource;
    WindowPtr *toplevel = user_data;

    /* Pick the first realized toplevel we find */
    if (*toplevel == NullWindow && window->realized && xwl_window_is_toplevel(window))
        *toplevel = window;
}

static WindowPtr
xwl_keyboard_search_window(ClientPtr client)
{
    WindowPtr window = NullWindow;

    FindClientResourcesByType(client, X11_RESTYPE_WINDOW, find_toplevel_callback, &window);

    return window;
}

static void
xwl_keyboard_activate_grab(DeviceIntPtr device, GrabPtr grab, TimeStamp time, Bool passive)
{
    struct xwl_seat *xwl_seat = device->public.devicePrivate;
    WindowPtr grab_window = grab->window;

    /* We are not interested in passive grabs */
    if (!passive) {
        /* If the device is the MASTER_KEYBOARD, we don't have an xwl_seat */
        if (xwl_seat == NULL)
            xwl_seat = find_matching_seat(device);
        if (xwl_seat) {
            if (grab_window == xwl_seat->xwl_screen->screen->root)
                grab_window = xwl_keyboard_search_window(GetCurrentClient());
            if (grab_window)
                set_grab(xwl_seat, xwl_window_from_window(grab_window));
        }
    }

    ActivateKeyboardGrab(device, grab, time, passive);
}

static void
xwl_keyboard_deactivate_grab(DeviceIntPtr device)
{
    struct xwl_seat *xwl_seat = device->public.devicePrivate;

    /* If the device is the MASTER_KEYBOARD, we don't have an xwl_seat */
    if (xwl_seat == NULL)
        xwl_seat = find_matching_seat(device);
    if (xwl_seat)
        release_grab (xwl_seat);

    DeactivateKeyboardGrab(device);
}

static void
setup_keyboard_grab_handler (DeviceIntPtr device)
{
    device->deviceGrab.ActivateGrab = xwl_keyboard_activate_grab;
    device->deviceGrab.DeactivateGrab = xwl_keyboard_deactivate_grab;
}

static DeviceIntPtr
add_device(struct xwl_seat *xwl_seat,
           const char *driver, DeviceProc device_proc)
{
    DeviceIntPtr dev = NULL;
    static Atom type_atom;
    char name[32];

    dev = AddInputDevice(serverClient, device_proc, TRUE);
    if (dev == NULL)
        return NULL;

    if (type_atom == None)
        type_atom = MakeAtom(driver, strlen(driver), TRUE);
    snprintf(name, sizeof name, "%s:%d", driver, xwl_seat->id);
    AssignTypeAndName(dev, type_atom, name);
    dev->public.devicePrivate = xwl_seat;
    dev->type = SLAVE;
    dev->spriteInfo->spriteOwner = FALSE;

    return dev;
}

static void
disable_device(DeviceIntPtr dev)
{
    DisableDevice(dev, TRUE);
    dev->public.devicePrivate = NULL;
}

static void
enable_device(struct xwl_seat *xwl_seat, DeviceIntPtr dev)
{
    dev->public.devicePrivate = xwl_seat;
    EnableDevice(dev, TRUE);
}


static void
init_pointer(struct xwl_seat *xwl_seat)
{
    xwl_seat->wl_pointer = wl_seat_get_pointer(xwl_seat->seat);
    wl_pointer_add_listener(xwl_seat->wl_pointer,
                            &pointer_listener, xwl_seat);

    if (xwl_seat->pointer == NULL) {
        xwl_seat_set_cursor(xwl_seat);
        xwl_seat->pointer =
            add_device(xwl_seat, "xwayland-pointer", xwl_pointer_proc);
        ActivateDevice(xwl_seat->pointer, TRUE);
    }
    enable_device(xwl_seat, xwl_seat->pointer);
}

static void
release_pointer(struct xwl_seat *xwl_seat)
{
    wl_pointer_release(xwl_seat->wl_pointer);
    xwl_seat->wl_pointer = NULL;

    if (xwl_seat->pointer)
        disable_device(xwl_seat->pointer);
}

static void
init_relative_pointer(struct xwl_seat *xwl_seat)
{
    if (xwl_seat->relative_pointer == NULL) {
        xwl_seat->relative_pointer =
            add_device(xwl_seat, "xwayland-relative-pointer",
                       xwl_pointer_proc_relative);
        ActivateDevice(xwl_seat->relative_pointer, TRUE);
    }
    enable_device(xwl_seat, xwl_seat->relative_pointer);
}

static void
init_relative_pointer_listener(struct xwl_seat *xwl_seat)
{
    struct zwp_relative_pointer_manager_v1 *relative_pointer_manager =
        xwl_seat->xwl_screen->relative_pointer_manager;

    if (relative_pointer_manager) {
        xwl_seat->wp_relative_pointer =
            zwp_relative_pointer_manager_v1_get_relative_pointer(
                relative_pointer_manager, xwl_seat->wl_pointer);
        zwp_relative_pointer_v1_add_listener(xwl_seat->wp_relative_pointer,
                                             &relative_pointer_listener,
                                             xwl_seat);
    }
}

static void
release_relative_pointer(struct xwl_seat *xwl_seat)
{
    if (xwl_seat->wp_relative_pointer) {
        zwp_relative_pointer_v1_destroy(xwl_seat->wp_relative_pointer);
        xwl_seat->wp_relative_pointer = NULL;
    }

    if (xwl_seat->relative_pointer)
        disable_device(xwl_seat->relative_pointer);
}

static void
init_pointer_gestures_device(struct xwl_seat *xwl_seat)
{
    if (xwl_seat->pointer_gestures == NULL) {
        xwl_seat->pointer_gestures =
            add_device(xwl_seat, "xwayland-pointer-gestures",
                       xwl_pointer_proc_pointer_gestures);
        ActivateDevice(xwl_seat->pointer_gestures, TRUE);
    }
    enable_device(xwl_seat, xwl_seat->pointer_gestures);
}

static void
init_pointer_gestures_listener(struct xwl_seat *xwl_seat)
{
    struct zwp_pointer_gestures_v1 *pointer_gestures =
            xwl_seat->xwl_screen->pointer_gestures;

    if (pointer_gestures && !xwl_seat->wp_pointer_gesture_swipe) {
        xwl_seat->wp_pointer_gesture_swipe =
                zwp_pointer_gestures_v1_get_swipe_gesture(pointer_gestures,
                                                          xwl_seat->wl_pointer);
        zwp_pointer_gesture_swipe_v1_set_user_data(xwl_seat->wp_pointer_gesture_swipe,
                                                   xwl_seat);
        zwp_pointer_gesture_swipe_v1_add_listener(xwl_seat->wp_pointer_gesture_swipe,
                                                  &pointer_gesture_swipe_listener,
                                                  xwl_seat);
    }

    if (pointer_gestures && !xwl_seat->wp_pointer_gesture_pinch) {
        xwl_seat->wp_pointer_gesture_pinch =
                zwp_pointer_gestures_v1_get_pinch_gesture(pointer_gestures,
                                                          xwl_seat->wl_pointer);
        zwp_pointer_gesture_pinch_v1_set_user_data(xwl_seat->wp_pointer_gesture_pinch,
                                                   xwl_seat);
        zwp_pointer_gesture_pinch_v1_add_listener(xwl_seat->wp_pointer_gesture_pinch,
                                                  &pointer_gesture_pinch_listener,
                                                  xwl_seat);
    }
}

static void
release_pointer_gestures_device(struct xwl_seat *xwl_seat)
{
    if (xwl_seat->wp_pointer_gesture_swipe) {
        zwp_pointer_gesture_swipe_v1_destroy(xwl_seat->wp_pointer_gesture_swipe);
        xwl_seat->wp_pointer_gesture_swipe = NULL;
    }

    if (xwl_seat->wp_pointer_gesture_pinch) {
        zwp_pointer_gesture_pinch_v1_destroy(xwl_seat->wp_pointer_gesture_pinch);
        xwl_seat->wp_pointer_gesture_pinch = NULL;
    }

    if (xwl_seat->pointer_gestures)
        disable_device(xwl_seat->pointer_gestures);
}

static void
init_keyboard(struct xwl_seat *xwl_seat)
{
    DeviceIntPtr master;

    xwl_seat->wl_keyboard = wl_seat_get_keyboard(xwl_seat->seat);
    wl_keyboard_add_listener(xwl_seat->wl_keyboard,
                             &keyboard_listener, xwl_seat);

    if (xwl_seat->keyboard == NULL) {
        xwl_seat->keyboard =
            add_device(xwl_seat, "xwayland-keyboard", xwl_keyboard_proc);
        ActivateDevice(xwl_seat->keyboard, TRUE);
    }
    enable_device(xwl_seat, xwl_seat->keyboard);
    xwl_seat->keyboard->key->xkbInfo->checkRepeat = keyboard_check_repeat;

    if (xwl_seat->xwl_screen->wp_grab) {
        /* We have Xwayland grab protocol supported by the compositor */
        master = GetMaster(xwl_seat->keyboard, MASTER_KEYBOARD);
        if (master)
            setup_keyboard_grab_handler(master);
    }
}

static void
release_keyboard(struct xwl_seat *xwl_seat)
{
    release_grab(xwl_seat);
    wl_keyboard_release(xwl_seat->wl_keyboard);
    xwl_seat->wl_keyboard = NULL;

    if (xwl_seat->keyboard) {
        remove_sync_pending(xwl_seat->keyboard);
        disable_device(xwl_seat->keyboard);
    }
}

static void
init_touch(struct xwl_seat *xwl_seat)
{
    xwl_seat->wl_touch = wl_seat_get_touch(xwl_seat->seat);
    wl_touch_add_listener(xwl_seat->wl_touch,
                          &touch_listener, xwl_seat);

    if (xwl_seat->touch == NULL) {
        xwl_seat->touch =
            add_device(xwl_seat, "xwayland-touch", xwl_touch_proc);
        ActivateDevice(xwl_seat->touch, TRUE);
    }
    enable_device(xwl_seat, xwl_seat->touch);
}

static void
release_touch(struct xwl_seat *xwl_seat)
{
    wl_touch_release(xwl_seat->wl_touch);
    xwl_seat->wl_touch = NULL;

    if (xwl_seat->touch)
        disable_device(xwl_seat->touch);
}

static void
seat_handle_capabilities(void *data, struct wl_seat *seat,
                         enum wl_seat_capability caps)
{
    struct xwl_seat *xwl_seat = data;

    if (caps & WL_SEAT_CAPABILITY_POINTER && xwl_seat->wl_pointer == NULL) {
        init_pointer(xwl_seat);
        init_relative_pointer(xwl_seat);
        init_relative_pointer_listener(xwl_seat);
        init_pointer_gestures_device(xwl_seat);
        init_pointer_gestures_listener(xwl_seat);
    } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && xwl_seat->wl_pointer) {
        release_pointer(xwl_seat);
        release_relative_pointer(xwl_seat);
        release_pointer_gestures_device(xwl_seat);
    }

    if (caps & WL_SEAT_CAPABILITY_KEYBOARD && xwl_seat->wl_keyboard == NULL) {
        init_keyboard(xwl_seat);
    } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && xwl_seat->wl_keyboard) {
        release_keyboard(xwl_seat);
    }

    if (caps & WL_SEAT_CAPABILITY_TOUCH && xwl_seat->wl_touch == NULL) {
        init_touch(xwl_seat);
    } else if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && xwl_seat->wl_touch) {
        release_touch(xwl_seat);
    }

    xwl_seat->xwl_screen->expecting_event--;
}

static void
seat_handle_name(void *data, struct wl_seat *seat,
                 const char *name)
{

}

static const struct wl_seat_listener seat_listener = {
    seat_handle_capabilities,
    seat_handle_name
};

static void
xwl_cursor_init(struct xwl_cursor *xwl_cursor, struct xwl_screen *xwl_screen,
                void (* update_proc)(struct xwl_cursor *))
{
    xwl_cursor->surface = wl_compositor_create_surface(xwl_screen->compositor);
    xwl_cursor->update_proc = update_proc;
    xwl_cursor->frame_cb = NULL;
    xwl_cursor->needs_update = FALSE;
}

static void
xwl_seat_update_cursor(struct xwl_cursor *xwl_cursor)
{
    struct xwl_seat *xwl_seat = wl_container_of(xwl_cursor, xwl_seat, cursor);
    xwl_seat_set_cursor(xwl_seat);
}

static void
create_input_device(struct xwl_screen *xwl_screen, uint32_t id, uint32_t version)
{
    struct xwl_seat *xwl_seat;
    int seat_version = 8;

    xwl_seat = calloc(1, sizeof *xwl_seat);
    if (xwl_seat == NULL) {
        ErrorF("%s: ENOMEM\n", __func__);
        return;
    }

    xwl_seat->xwl_screen = xwl_screen;
    xorg_list_add(&xwl_seat->link, &xwl_screen->seat_list);

    xwl_seat->seat =
        wl_registry_bind(xwl_screen->registry, id,
                         &wl_seat_interface, min(version, seat_version));
    xwl_seat->id = id;

    xwl_cursor_init(&xwl_seat->cursor, xwl_seat->xwl_screen,
                    xwl_seat_update_cursor);
    wl_seat_add_listener(xwl_seat->seat, &seat_listener, xwl_seat);

    init_tablet_manager_seat(xwl_screen, xwl_seat);

    wl_array_init(&xwl_seat->keys);

    xorg_list_init(&xwl_seat->touches);
    xorg_list_init(&xwl_seat->sync_pending);
}

void
xwl_seat_destroy(struct xwl_seat *xwl_seat)
{
    struct xwl_touch *xwl_touch, *next_xwl_touch;
    struct sync_pending *p, *npd;

    xorg_list_for_each_entry_safe(xwl_touch, next_xwl_touch,
                                  &xwl_seat->touches, link_touch) {
        xorg_list_del(&xwl_touch->link_touch);
        free(xwl_touch);
    }

    xorg_list_for_each_entry_safe(p, npd, &xwl_seat->sync_pending, l) {
        xorg_list_del(&xwl_seat->sync_pending);
        free (p);
    }

    release_tablet_manager_seat(xwl_seat);

    release_grab(xwl_seat);
    wl_seat_destroy(xwl_seat->seat);
    xwl_cursor_release(&xwl_seat->cursor);
    wl_array_release(&xwl_seat->keys);
    free(xwl_seat);
}

static void
tablet_handle_name(void *data, struct zwp_tablet_v2 *tablet, const char *name)
{
}

static void
tablet_handle_id(void *data, struct zwp_tablet_v2 *tablet, uint32_t vid,
                  uint32_t pid)
{
}

static void
tablet_handle_path(void *data, struct zwp_tablet_v2 *tablet, const char *path)
{
}

static void
tablet_handle_done(void *data, struct zwp_tablet_v2 *tablet)
{
    struct xwl_tablet *xwl_tablet = data;
    struct xwl_seat *xwl_seat = xwl_tablet->seat;

    if (xwl_seat->stylus == NULL) {
        xwl_seat->stylus = add_device(xwl_seat, "xwayland-tablet stylus", xwl_tablet_proc);
        ActivateDevice(xwl_seat->stylus, TRUE);
    }
    enable_device(xwl_seat, xwl_seat->stylus);

    if (xwl_seat->eraser == NULL) {
        xwl_seat->eraser = add_device(xwl_seat, "xwayland-tablet eraser", xwl_tablet_proc);
        ActivateDevice(xwl_seat->eraser, TRUE);
    }
    enable_device(xwl_seat, xwl_seat->eraser);

    if (xwl_seat->puck == NULL) {
        xwl_seat->puck = add_device(xwl_seat, "xwayland-tablet cursor", xwl_tablet_proc);
        ActivateDevice(xwl_seat->puck, TRUE);
    }
    enable_device(xwl_seat, xwl_seat->puck);
}

static void
tablet_handle_removed(void *data, struct zwp_tablet_v2 *tablet)
{
    struct xwl_tablet *xwl_tablet = data;
    struct xwl_seat *xwl_seat = xwl_tablet->seat;

    xorg_list_del(&xwl_tablet->link);

    /* The tablet is merely disabled, not removed. The next tablet
       will re-use the same X devices */
    if (xorg_list_is_empty(&xwl_seat->tablets)) {
        if (xwl_seat->stylus)
            disable_device(xwl_seat->stylus);
        if (xwl_seat->eraser)
            disable_device(xwl_seat->eraser);
        if (xwl_seat->puck)
            disable_device(xwl_seat->puck);
        /* pads are removed separately */
    }

    zwp_tablet_v2_destroy(tablet);
    free(xwl_tablet);
}

static const struct zwp_tablet_v2_listener tablet_listener = {
    tablet_handle_name,
    tablet_handle_id,
    tablet_handle_path,
    tablet_handle_done,
    tablet_handle_removed
};

static void
tablet_tool_receive_type(void *data, struct zwp_tablet_tool_v2 *tool,
                         uint32_t type)
{
    struct xwl_tablet_tool *xwl_tablet_tool = data;
    struct xwl_seat *xwl_seat = xwl_tablet_tool->seat;

    switch (type) {
        case ZWP_TABLET_TOOL_V2_TYPE_ERASER:
            xwl_tablet_tool->xdevice = xwl_seat->eraser;
            break;
        case ZWP_TABLET_TOOL_V2_TYPE_MOUSE:
        case ZWP_TABLET_TOOL_V2_TYPE_LENS:
            xwl_tablet_tool->xdevice = xwl_seat->puck;
            break;
        default:
            xwl_tablet_tool->xdevice = xwl_seat->stylus;
            break;
    }
}

static void
tablet_tool_receive_hardware_serial(void *data, struct zwp_tablet_tool_v2 *tool,
                                    uint32_t hi, uint32_t low)
{
}

static void
tablet_tool_receive_hardware_id_wacom(void *data, struct zwp_tablet_tool_v2 *tool,
                                      uint32_t hi, uint32_t low)
{
}

static void
tablet_tool_receive_capability(void *data, struct zwp_tablet_tool_v2 *tool,
                               uint32_t capability)
{
}

static void
tablet_tool_receive_done(void *data, struct zwp_tablet_tool_v2 *tool)
{
}

static void
tablet_tool_receive_removed(void *data, struct zwp_tablet_tool_v2 *tool)
{
    struct xwl_tablet_tool *xwl_tablet_tool = data;

    xorg_list_del(&xwl_tablet_tool->link);
    xwl_cursor_release(&xwl_tablet_tool->cursor);
    zwp_tablet_tool_v2_destroy(tool);
    free(xwl_tablet_tool);
}

static void
tablet_tool_proximity_in(void *data, struct zwp_tablet_tool_v2 *tool,
                         uint32_t serial, struct zwp_tablet_v2 *tablet,
                         struct wl_surface *wl_surface)
{
    struct xwl_tablet_tool *xwl_tablet_tool = data;
    struct xwl_seat *xwl_seat = xwl_tablet_tool->seat;

    /* There's a race here where if we create and then immediately
     * destroy a surface, we might end up in a state where the Wayland
     * compositor sends us an event for a surface that doesn't exist.
     *
     * Don't process enter events in this case.
     *
     * see pointer_handle_enter()
     */
    if (wl_surface == NULL)
        return;

    if (!is_surface_from_xwl_window(wl_surface))
        return;

    xwl_tablet_tool->proximity_in_serial = serial;
    xwl_seat->tablet_focus_window = wl_surface_get_user_data(wl_surface);

    /* If there is a cursor surface frame callback pending, we need to clear it
     * so that we can continue submitting new cursor frames.
     */
    xwl_cursor_clear_frame_cb(&xwl_tablet_tool->cursor);
    xwl_tablet_tool_set_cursor(xwl_tablet_tool);
}

static void
tablet_tool_proximity_out(void *data, struct zwp_tablet_tool_v2 *tool)
{
    struct xwl_tablet_tool *xwl_tablet_tool = data;
    struct xwl_seat *xwl_seat = xwl_tablet_tool->seat;

    xwl_tablet_tool->proximity_in_serial = 0;
    xwl_seat->tablet_focus_window = NULL;

    xwl_tablet_tool->pressure = 0;
    xwl_tablet_tool->tilt_x = 0;
    xwl_tablet_tool->tilt_y = 0;
    xwl_tablet_tool->rotation = 0;
    xwl_tablet_tool->slider = 0;
}

static void
tablet_tool_down(void *data, struct zwp_tablet_tool_v2 *tool, uint32_t serial)
{
    struct xwl_tablet_tool *xwl_tablet_tool = data;
    struct xwl_seat *xwl_seat = xwl_tablet_tool->seat;
    ValuatorMask mask;

    xwl_seat->xwl_screen->serial = serial;

    valuator_mask_zero(&mask);
    QueuePointerEvents(xwl_tablet_tool->xdevice, ButtonPress, 1, 0, &mask);
}

static void
tablet_tool_up(void *data, struct zwp_tablet_tool_v2 *tool)
{
    struct xwl_tablet_tool *xwl_tablet_tool = data;
    ValuatorMask mask;

    valuator_mask_zero(&mask);
    QueuePointerEvents(xwl_tablet_tool->xdevice, ButtonRelease, 1, 0, &mask);
}

static void
tablet_tool_motion(void *data, struct zwp_tablet_tool_v2 *tool,
                   wl_fixed_t x, wl_fixed_t y)
{
    struct xwl_tablet_tool *xwl_tablet_tool = data;
    struct xwl_seat *xwl_seat = xwl_tablet_tool->seat;
    struct xwl_screen *xwl_screen = xwl_seat->xwl_screen;
    int32_t dx, dy;
    double sx = wl_fixed_to_double(x);
    double sy = wl_fixed_to_double(y);

    if (!xwl_seat->tablet_focus_window)
        return;

    sx *= xwl_screen->global_surface_scale;
    sy *= xwl_screen->global_surface_scale;

    sx *= xwl_seat->tablet_focus_window->viewport_scale_x;
    sy *= xwl_seat->tablet_focus_window->viewport_scale_y;

    dx = xwl_seat->tablet_focus_window->toplevel->drawable.x;
    dy = xwl_seat->tablet_focus_window->toplevel->drawable.y;

    xwl_tablet_tool->x = (double) dx + sx;
    xwl_tablet_tool->y = (double) dy + sy;
}

static void
tablet_tool_pressure(void *data, struct zwp_tablet_tool_v2 *tool,
                     uint32_t pressure)
{
    struct xwl_tablet_tool *xwl_tablet_tool = data;
    struct xwl_seat *xwl_seat = xwl_tablet_tool->seat;

    if (!xwl_seat->tablet_focus_window)
        return;

    /* normalized to 65535 already */
    xwl_tablet_tool->pressure = pressure;
}

static void
tablet_tool_distance(void *data, struct zwp_tablet_tool_v2 *tool,
                     uint32_t distance_raw)
{
}

static void
tablet_tool_tilt(void *data, struct zwp_tablet_tool_v2 *tool,
                 wl_fixed_t tilt_x, wl_fixed_t tilt_y)
{
    struct xwl_tablet_tool *xwl_tablet_tool = data;
    struct xwl_seat *xwl_seat = xwl_tablet_tool->seat;
    struct xwl_screen *xwl_screen = xwl_seat->xwl_screen;

    if (!xwl_seat->tablet_focus_window)
        return;

    xwl_tablet_tool->tilt_x = wl_fixed_to_double(tilt_x);
    xwl_tablet_tool->tilt_y = wl_fixed_to_double(tilt_y);

    xwl_tablet_tool->tilt_x *= xwl_screen->global_surface_scale;
    xwl_tablet_tool->tilt_y *= xwl_screen->global_surface_scale;

    xwl_tablet_tool->tilt_x *= xwl_seat->tablet_focus_window->viewport_scale_x;
    xwl_tablet_tool->tilt_y *= xwl_seat->tablet_focus_window->viewport_scale_y;
}

static void
tablet_tool_rotation(void *data, struct zwp_tablet_tool_v2 *tool,
                     wl_fixed_t angle)
{
    struct xwl_tablet_tool *xwl_tablet_tool = data;
    struct xwl_seat *xwl_seat = xwl_tablet_tool->seat;
    double rotation = wl_fixed_to_double(angle);

    if (!xwl_seat->tablet_focus_window)
        return;

    /* change origin (buttons facing right [libinput +90 degrees]) and
     * scaling (5 points per degree) to match wacom driver behavior
     */
    rotation = remainderf(rotation + 90.0f, 360.0f);
    rotation *= 5.0f;
    xwl_tablet_tool->rotation = rotation;
}

static void
tablet_tool_slider(void *data, struct zwp_tablet_tool_v2 *tool,
                   int32_t position_raw)
{
    struct xwl_tablet_tool *xwl_tablet_tool = data;
    struct xwl_seat *xwl_seat = xwl_tablet_tool->seat;
    float position = position_raw / 65535.0;

    if (!xwl_seat->tablet_focus_window)
        return;

    xwl_tablet_tool->slider = (position * 1799.0f) - 900.0f;
}

static void
tablet_tool_wheel(void *data, struct zwp_tablet_tool_v2 *tool,
                  wl_fixed_t degrees, int32_t clicks)
{
    struct xwl_tablet_tool *xwl_tablet_tool = data;
    struct xwl_seat *xwl_seat = xwl_tablet_tool->seat;

    if (!xwl_seat->tablet_focus_window)
        return;

    xwl_tablet_tool->wheel_clicks = clicks;
}

static void
tablet_tool_button_state(void *data, struct zwp_tablet_tool_v2 *tool,
                         uint32_t serial, uint32_t button, uint32_t state)
{
    struct xwl_tablet_tool *xwl_tablet_tool = data;
    struct xwl_seat *xwl_seat = xwl_tablet_tool->seat;
    uint32_t *mask = &xwl_tablet_tool->buttons_now;
    int xbtn = 0;

    /* BTN_0 .. BTN_9 */
    if (button >= 0x100 && button <= 0x109) {
        xbtn = button - 0x100 + 1;
    }
    /* BTN_A .. BTN_Z */
    else if (button >= 0x130 && button <= 0x135) {
        xbtn = button - 0x130 + 10;
    }
    /* BTN_BASE .. BTN_BASE6 */
    else if (button >= 0x126 && button <= 0x12b) {
        xbtn = button - 0x126 + 16;
    }
    else {
        switch (button) {
        case 0x110: /* BTN_LEFT    */
        case 0x14a: /* BTN_TOUCH   */
            xbtn = 1;
            break;

        case 0x112: /* BTN_MIDDLE  */
        case 0x14b: /* BTN_STYLUS  */
            xbtn = 2;
            break;

        case 0x111: /* BTN_RIGHT   */
        case 0x14c: /* BTN_STYLUS2 */
            xbtn = 3;
            break;

        case 0x113: /* BTN_SIDE    */
        case 0x116: /* BTN_BACK    */
        case 0x149: /* BTN_STYLUS3 */
            xbtn = 8;
            break;

        case 0x114: /* BTN_EXTRA   */
        case 0x115: /* BTN_FORWARD */
            xbtn = 9;
            break;
        }
    }

    if (!xbtn) {
        ErrorF("unknown tablet button number %d\n", button);
        return;
    }

    BUG_RETURN(xbtn >= 8 * sizeof(*mask));

    if (state)
        SetBit(mask, xbtn - 1);
    else
        ClearBit(mask, xbtn - 1);

    xwl_seat->xwl_screen->serial = serial;
}

static void
tablet_tool_frame(void *data, struct zwp_tablet_tool_v2 *tool, uint32_t time)
{
    struct xwl_tablet_tool *xwl_tablet_tool = data;
    ValuatorMask mask;
    uint32_t released, pressed, diff;
    int button;

    valuator_mask_zero(&mask);
    valuator_mask_set_double(&mask, 0, xwl_tablet_tool->x);
    valuator_mask_set_double(&mask, 1, xwl_tablet_tool->y);
    valuator_mask_set(&mask, 2, xwl_tablet_tool->pressure);
    valuator_mask_set_double(&mask, 3, xwl_tablet_tool->tilt_x);
    valuator_mask_set_double(&mask, 4, xwl_tablet_tool->tilt_y);
    valuator_mask_set_double(&mask, 5, xwl_tablet_tool->rotation + xwl_tablet_tool->slider);

    QueuePointerEvents(xwl_tablet_tool->xdevice, MotionNotify, 0,
               POINTER_ABSOLUTE | POINTER_DESKTOP, &mask);

    valuator_mask_zero(&mask);

    diff = xwl_tablet_tool->buttons_prev ^ xwl_tablet_tool->buttons_now;
    released = diff & ~xwl_tablet_tool->buttons_now;
    pressed = diff & xwl_tablet_tool->buttons_now;

    button = 1;
    while (released) {
        if (released & 0x1)
            QueuePointerEvents(xwl_tablet_tool->xdevice,
                               ButtonRelease, button, 0, &mask);
        button++;
        released >>= 1;
    }

    button = 1;
    while (pressed) {
        if (pressed & 0x1)
            QueuePointerEvents(xwl_tablet_tool->xdevice,
                               ButtonPress, button, 0, &mask);
        button++;
        pressed >>= 1;
    }

    xwl_tablet_tool->buttons_prev = xwl_tablet_tool->buttons_now;

    while (xwl_tablet_tool->wheel_clicks) {
            if (xwl_tablet_tool->wheel_clicks < 0) {
                button = 4;
                xwl_tablet_tool->wheel_clicks++;
            }
            else {
                button = 5;
                xwl_tablet_tool->wheel_clicks--;
            }

            QueuePointerEvents(xwl_tablet_tool->xdevice,
                               ButtonPress, button, 0, &mask);
            QueuePointerEvents(xwl_tablet_tool->xdevice,
                               ButtonRelease, button, 0, &mask);

    }
}

static const struct zwp_tablet_tool_v2_listener tablet_tool_listener = {
    tablet_tool_receive_type,
    tablet_tool_receive_hardware_serial,
    tablet_tool_receive_hardware_id_wacom,
    tablet_tool_receive_capability,
    tablet_tool_receive_done,
    tablet_tool_receive_removed,
    tablet_tool_proximity_in,
    tablet_tool_proximity_out,
    tablet_tool_down,
    tablet_tool_up,
    tablet_tool_motion,
    tablet_tool_pressure,
    tablet_tool_distance,
    tablet_tool_tilt,
    tablet_tool_rotation,
    tablet_tool_slider,
    tablet_tool_wheel,
    tablet_tool_button_state,
    tablet_tool_frame
};

static void
tablet_pad_ring_destroy(struct xwl_tablet_pad_ring *ring)
{
    zwp_tablet_pad_ring_v2_destroy(ring->ring);
    xorg_list_del(&ring->link);
    free(ring);
}

static void
tablet_pad_ring_source(void *data,
                       struct zwp_tablet_pad_ring_v2 *zwp_tablet_pad_ring_v2,
                       uint32_t source)
{
}

static void
tablet_pad_ring_angle(void *data,
                      struct zwp_tablet_pad_ring_v2 *zwp_tablet_pad_ring_v2,
                      wl_fixed_t degrees)
{
    struct xwl_tablet_pad_ring *ring = data;
    struct xwl_tablet_pad *pad = ring->group->pad;
    double deg = wl_fixed_to_double(degrees);
    ValuatorMask mask;

    valuator_mask_zero(&mask);
    valuator_mask_set(&mask, 5 + ring->index, deg/360.0  * 71);
    QueuePointerEvents(pad->xdevice, MotionNotify, 0, 0, &mask);
}

static void
tablet_pad_ring_stop(void *data,
                     struct zwp_tablet_pad_ring_v2 *zwp_tablet_pad_ring_v2)
{
}

static void
tablet_pad_ring_frame(void *data,
                      struct zwp_tablet_pad_ring_v2 *zwp_tablet_pad_ring_v2,
                      uint32_t time)
{
}

static const struct zwp_tablet_pad_ring_v2_listener tablet_pad_ring_listener = {
    tablet_pad_ring_source,
    tablet_pad_ring_angle,
    tablet_pad_ring_stop,
    tablet_pad_ring_frame,
};


static void
tablet_pad_strip_destroy(struct xwl_tablet_pad_strip *strip)
{
    zwp_tablet_pad_strip_v2_destroy(strip->strip);
    xorg_list_del(&strip->link);
    free(strip);
}

static void
tablet_pad_strip_source(void *data,
                        struct zwp_tablet_pad_strip_v2 *zwp_tablet_pad_strip_v2,
                        uint32_t source)
{
}

static void
tablet_pad_strip_position(void *data,
                          struct zwp_tablet_pad_strip_v2 *zwp_tablet_pad_strip_v2,
                          uint32_t position)
{
    struct xwl_tablet_pad_strip *strip = data;
    struct xwl_tablet_pad *pad = strip->group->pad;
    ValuatorMask mask;

    valuator_mask_zero(&mask);
    valuator_mask_set(&mask, 3 + strip->index, position/65535.0 * 2048);
    QueuePointerEvents(pad->xdevice, MotionNotify, 0, 0, &mask);
}

static void
tablet_pad_strip_stop(void *data,
                      struct zwp_tablet_pad_strip_v2 *zwp_tablet_pad_strip_v2)
{
}

static void
tablet_pad_strip_frame(void *data,
                       struct zwp_tablet_pad_strip_v2 *zwp_tablet_pad_strip_v2,
                       uint32_t time)
{
}

static const struct zwp_tablet_pad_strip_v2_listener tablet_pad_strip_listener = {
    tablet_pad_strip_source,
    tablet_pad_strip_position,
    tablet_pad_strip_stop,
    tablet_pad_strip_frame,
};

static void
tablet_pad_group_destroy(struct xwl_tablet_pad_group *group)
{
    struct xwl_tablet_pad_ring *r, *tr;
    struct xwl_tablet_pad_strip *s, *ts;

    xorg_list_for_each_entry_safe(r, tr,
                                  &group->pad_group_ring_list,
                                  link)
        tablet_pad_ring_destroy(r);

    xorg_list_for_each_entry_safe(s, ts,
                                  &group->pad_group_strip_list,
                                  link)
        tablet_pad_strip_destroy(s);

    zwp_tablet_pad_group_v2_destroy(group->group);
    xorg_list_del(&group->link);
    free(group);
}

static void
tablet_pad_group_buttons(void *data,
                         struct zwp_tablet_pad_group_v2 *zwp_tablet_pad_group_v2,
                         struct wl_array *buttons)
{

}

static void
tablet_pad_group_ring(void *data,
                      struct zwp_tablet_pad_group_v2 *zwp_tablet_pad_group_v2,
                      struct zwp_tablet_pad_ring_v2 *wp_ring)
{
    static unsigned int ring_index = 0;
    struct xwl_tablet_pad_group *group = data;
    struct xwl_tablet_pad_ring *ring;

    ring = calloc(1, sizeof *ring);
    if (ring == NULL) {
        ErrorF("%s ENOMEM\n", __func__);
        return;
    }

    ring->index = ring_index++;
    ring->group = group;
    ring->ring = wp_ring;

    xorg_list_add(&ring->link, &group->pad_group_ring_list);

    zwp_tablet_pad_ring_v2_add_listener(wp_ring, &tablet_pad_ring_listener,
                                        ring);
}

static void
tablet_pad_group_strip(void *data,
                       struct zwp_tablet_pad_group_v2 *zwp_tablet_pad_group_v2,
                       struct zwp_tablet_pad_strip_v2 *wp_strip)
{
    static unsigned int strip_index = 0;
    struct xwl_tablet_pad_group *group = data;
    struct xwl_tablet_pad_strip *strip;

    strip = calloc(1, sizeof *strip);
    if (strip == NULL) {
        ErrorF("%s ENOMEM\n", __func__);
        return;
    }

    strip->index = strip_index++;
    strip->group = group;
    strip->strip = wp_strip;

    xorg_list_add(&strip->link, &group->pad_group_strip_list);

    zwp_tablet_pad_strip_v2_add_listener(wp_strip, &tablet_pad_strip_listener,
                                         strip);
}

static void
tablet_pad_group_modes(void *data,
                       struct zwp_tablet_pad_group_v2 *zwp_tablet_pad_group_v2,
                       uint32_t modes)
{

}

static void
tablet_pad_group_done(void *data,
                      struct zwp_tablet_pad_group_v2 *zwp_tablet_pad_group_v2)
{

}

static void
tablet_pad_group_mode_switch(void *data,
                             struct zwp_tablet_pad_group_v2 *zwp_tablet_pad_group_v2,
                             uint32_t time,
                             uint32_t serial,
                             uint32_t mode)
{

}

static struct zwp_tablet_pad_group_v2_listener tablet_pad_group_listener = {
    tablet_pad_group_buttons,
    tablet_pad_group_ring,
    tablet_pad_group_strip,
    tablet_pad_group_modes,
    tablet_pad_group_done,
    tablet_pad_group_mode_switch,
};

static int
xwl_tablet_pad_proc(DeviceIntPtr device, int what)
{
    struct xwl_tablet_pad *pad = dixGetPrivate(&device->devPrivates,
                                               &xwl_tablet_private_key);
    /* Axis layout mirrors that of xf86-input-wacom to have better
       compatibility with existing clients */
#define NAXES 7
    Atom axes_labels[NAXES] = { 0 };
    BYTE map[MAX_BUTTONS + 1];
    int i = 0;
    Atom btn_labels[MAX_BUTTONS] = { 0 }; /* btn labels are meaningless */
    int nbuttons;

    switch (what) {
    case DEVICE_INIT:
        device->public.on = FALSE;

        axes_labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_X);
        axes_labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_Y);
        /* The others have no good mapping */

        if (!InitValuatorClassDeviceStruct(device, NAXES, axes_labels,
                                           GetMotionHistorySize(), Absolute))
            return BadValue;

        for (i = 1; i <= MAX_BUTTONS; i++)
            map[i] = i;

        /* We need at least 7 buttons to allow scrolling */
        nbuttons = min(max(pad->nbuttons + 4, 7), MAX_BUTTONS);

        if (!InitButtonClassDeviceStruct(device, nbuttons,
                                         btn_labels, map))
            return BadValue;

        /* Valuators */
        InitValuatorAxisStruct(device, 0, axes_labels[0],
                               0, 100, 1, 0, 1, Absolute);
        InitValuatorAxisStruct(device, 1, axes_labels[1],
                               0, 100, 1, 0, 1, Absolute);
        /* Pressure - unused, for backwards compat only */
        InitValuatorAxisStruct(device, 2, axes_labels[2],
                               0, 2048, 1, 0, 1, Absolute);
        /* strip x */
        InitValuatorAxisStruct(device, 3, axes_labels[3],
                               0, 2048, 1, 0, 1, Absolute);
        /* strip y */
        InitValuatorAxisStruct(device, 4, axes_labels[4],
                               0, 2048, 1, 0, 1, Absolute);
        /* ring */
        InitValuatorAxisStruct(device, 5, axes_labels[5],
                               0, 71, 1, 0, 1, Absolute);
        /* ring2 */
        InitValuatorAxisStruct(device, 6, axes_labels[6],
                               0, 71, 1, 0, 1, Absolute);

        if (!InitPtrFeedbackClassDeviceStruct(device, xwl_pointer_control))
            return BadValue;

        return Success;

    case DEVICE_ON:
        device->public.on = TRUE;
        return Success;

    case DEVICE_OFF:
    case DEVICE_CLOSE:
        device->public.on = FALSE;
        return Success;
    }

    return BadMatch;
#undef NAXES
}

static void
tablet_pad_group(void *data,
                 struct zwp_tablet_pad_v2 *zwp_tablet_pad_v2,
                 struct zwp_tablet_pad_group_v2 *pad_group)
{
    struct xwl_tablet_pad *pad = data;
    struct xwl_tablet_pad_group *group;

    group = calloc(1, sizeof *group);
    if (group == NULL) {
        ErrorF("%s ENOMEM\n", __func__);
        return;
    }

    group->pad = pad;
    group->group = pad_group;
    xorg_list_init(&group->pad_group_ring_list);
    xorg_list_init(&group->pad_group_strip_list);

    xorg_list_add(&group->link, &pad->pad_group_list);

    zwp_tablet_pad_group_v2_add_listener(pad_group,
                                         &tablet_pad_group_listener,
                                         group);
}

static void
tablet_pad_path(void *data,
                struct zwp_tablet_pad_v2 *zwp_tablet_pad_v2,
                const char *path)
{

}

static void
tablet_pad_buttons(void *data,
                   struct zwp_tablet_pad_v2 *zwp_tablet_pad_v2,
                   uint32_t buttons)
{
    struct xwl_tablet_pad *pad = data;

    pad->nbuttons = buttons;
}

static void
tablet_pad_done(void *data,
                struct zwp_tablet_pad_v2 *zwp_tablet_pad_v2)
{
    struct xwl_tablet_pad *pad = data;

    pad->xdevice = add_device(pad->seat, "xwayland-tablet-pad",
                              xwl_tablet_pad_proc);
    dixSetPrivate(&pad->xdevice->devPrivates, &xwl_tablet_private_key, pad);
    ActivateDevice(pad->xdevice, TRUE);
    EnableDevice(pad->xdevice, TRUE);
}

static void
tablet_pad_button(void *data,
                  struct zwp_tablet_pad_v2 *zwp_tablet_pad_v2,
                  uint32_t time,
                  uint32_t button,
                  uint32_t state)
{
    struct xwl_tablet_pad *pad = data;
    ValuatorMask mask;

    button++; /* wayland index vs X's 1-offset */
    /* skip scroll wheel buttons 4-7 */
    button = button > 3 ? button + 4 : button;

    valuator_mask_zero(&mask);
    QueuePointerEvents(pad->xdevice,
                       state ? ButtonPress : ButtonRelease, button, 0, &mask);
}

static void
tablet_pad_enter(void *data,
                 struct zwp_tablet_pad_v2 *zwp_tablet_pad_v2,
                 uint32_t serial,
                 struct zwp_tablet_v2 *tablet,
                 struct wl_surface *surface)
{
    /* pairs the pad with the tablet but also to set the focus. We
     * don't care about the pairing and always use X's focus */
}

static void
tablet_pad_leave(void *data,
                 struct zwp_tablet_pad_v2 *zwp_tablet_pad_v2,
                 uint32_t serial,
                 struct wl_surface *surface)
{
    /* pairs the pad with the tablet but also to set the focus. We
     * don't care about the pairing and always use X's focus */
}

static void
tablet_pad_removed(void *data,
                   struct zwp_tablet_pad_v2 *zwp_tablet_pad_v2)
{
    struct xwl_tablet_pad *pad = data;
    struct xwl_tablet_pad_group *g, *tg;

    xorg_list_for_each_entry_safe(g, tg, &pad->pad_group_list, link)
        tablet_pad_group_destroy(g);

    RemoveDevice(pad->xdevice, TRUE);
    xorg_list_del(&pad->link);
    zwp_tablet_pad_v2_destroy(pad->pad);
    free(pad);
}

static const struct zwp_tablet_pad_v2_listener tablet_pad_listener = {
    tablet_pad_group,
    tablet_pad_path,
    tablet_pad_buttons,
    tablet_pad_done,
    tablet_pad_button,
    tablet_pad_enter,
    tablet_pad_leave,
    tablet_pad_removed,
};

static void
tablet_seat_handle_add_tablet(void *data, struct zwp_tablet_seat_v2 *tablet_seat,
                              struct zwp_tablet_v2 *tablet)
{
    struct xwl_seat *xwl_seat = data;
    struct xwl_tablet *xwl_tablet;

    xwl_tablet = calloc(1, sizeof *xwl_tablet);
    if (xwl_tablet == NULL) {
        ErrorF("%s ENOMEM\n", __func__);
        return;
    }

    xwl_tablet->tablet = tablet;
    xwl_tablet->seat = xwl_seat;

    xorg_list_add(&xwl_tablet->link, &xwl_seat->tablets);

    zwp_tablet_v2_add_listener(tablet, &tablet_listener, xwl_tablet);
}

static void
xwl_tablet_tool_update_cursor(struct xwl_cursor *xwl_cursor)
{
    struct xwl_tablet_tool *xwl_tablet_tool = wl_container_of(xwl_cursor,
                                                              xwl_tablet_tool,
                                                              cursor);
    xwl_tablet_tool_set_cursor(xwl_tablet_tool);
}

static void
tablet_seat_handle_add_tool(void *data, struct zwp_tablet_seat_v2 *tablet_seat,
                            struct zwp_tablet_tool_v2 *tool)
{
    struct xwl_seat *xwl_seat = data;
    struct xwl_screen *xwl_screen = xwl_seat->xwl_screen;
    struct xwl_tablet_tool *xwl_tablet_tool;

    xwl_tablet_tool = calloc(1, sizeof *xwl_tablet_tool);
    if (xwl_tablet_tool == NULL) {
        ErrorF("%s ENOMEM\n", __func__);
        return;
    }

    xwl_tablet_tool->tool = tool;
    xwl_tablet_tool->seat = xwl_seat;
    xwl_cursor_init(&xwl_tablet_tool->cursor, xwl_screen,
                    xwl_tablet_tool_update_cursor);

    xorg_list_add(&xwl_tablet_tool->link, &xwl_seat->tablet_tools);

    zwp_tablet_tool_v2_add_listener(tool, &tablet_tool_listener, xwl_tablet_tool);
}

static void
tablet_seat_handle_add_pad(void *data, struct zwp_tablet_seat_v2 *tablet_seat,
                           struct zwp_tablet_pad_v2 *pad)
{
    struct xwl_seat *xwl_seat = data;
    struct xwl_tablet_pad *xwl_tablet_pad;

    xwl_tablet_pad = calloc(1, sizeof *xwl_tablet_pad);
    if (xwl_tablet_pad == NULL) {
        ErrorF("%s ENOMEM\n", __func__);
        return;
    }

    xwl_tablet_pad->pad = pad;
    xwl_tablet_pad->seat = xwl_seat;
    xorg_list_init(&xwl_tablet_pad->pad_group_list);

    xorg_list_add(&xwl_tablet_pad->link, &xwl_seat->tablet_pads);

    zwp_tablet_pad_v2_add_listener(pad, &tablet_pad_listener,
                                   xwl_tablet_pad);
}

static const struct zwp_tablet_seat_v2_listener tablet_seat_listener = {
    tablet_seat_handle_add_tablet,
    tablet_seat_handle_add_tool,
    tablet_seat_handle_add_pad
};

static void
init_tablet_manager_seat(struct xwl_screen *xwl_screen,
                         struct xwl_seat *xwl_seat)
{
    xorg_list_init(&xwl_seat->tablets);
    xorg_list_init(&xwl_seat->tablet_tools);
    xorg_list_init(&xwl_seat->tablet_pads);

    if (!xwl_screen->tablet_manager)
        return;

    xwl_seat->tablet_seat =
        zwp_tablet_manager_v2_get_tablet_seat(xwl_screen->tablet_manager,
                                              xwl_seat->seat);

    zwp_tablet_seat_v2_add_listener(xwl_seat->tablet_seat, &tablet_seat_listener, xwl_seat);
}

static void
release_tablet_manager_seat(struct xwl_seat *xwl_seat)
{
    struct xwl_tablet *xwl_tablet, *next_xwl_tablet;
    struct xwl_tablet_tool *xwl_tablet_tool, *next_xwl_tablet_tool;
    struct xwl_tablet_pad *xwl_tablet_pad, *next_xwl_tablet_pad;

    xorg_list_for_each_entry_safe(xwl_tablet_pad, next_xwl_tablet_pad,
                                  &xwl_seat->tablet_pads, link) {
        xorg_list_del(&xwl_tablet_pad->link);
        zwp_tablet_pad_v2_destroy(xwl_tablet_pad->pad);
        free(xwl_tablet_pad);
    }

    xorg_list_for_each_entry_safe(xwl_tablet_tool, next_xwl_tablet_tool,
                                  &xwl_seat->tablet_tools, link) {
        xorg_list_del(&xwl_tablet_tool->link);
        zwp_tablet_tool_v2_destroy(xwl_tablet_tool->tool);
        free(xwl_tablet_tool);
    }

    xorg_list_for_each_entry_safe(xwl_tablet, next_xwl_tablet,
                                  &xwl_seat->tablets, link) {
        xorg_list_del(&xwl_tablet->link);
        zwp_tablet_v2_destroy(xwl_tablet->tablet);
        free(xwl_tablet);
    }

    if (xwl_seat->tablet_seat) {
        zwp_tablet_seat_v2_destroy(xwl_seat->tablet_seat);
        xwl_seat->tablet_seat = NULL;
    }
}

static void
init_tablet_manager(struct xwl_screen *xwl_screen, uint32_t id, uint32_t version)
{
    struct xwl_seat *xwl_seat;

    xwl_screen->tablet_manager = wl_registry_bind(xwl_screen->registry,
                                                  id,
                                                  &zwp_tablet_manager_v2_interface,
                                                  min(version,1));

    xorg_list_for_each_entry(xwl_seat, &xwl_screen->seat_list, link) {
        init_tablet_manager_seat(xwl_screen, xwl_seat);
    }
}

void
xwl_screen_release_tablet_manager(struct xwl_screen *xwl_screen)
{
    if (xwl_screen->tablet_manager) {
        zwp_tablet_manager_v2_destroy(xwl_screen->tablet_manager);
        xwl_screen->tablet_manager = NULL;
    }
}

static void
init_relative_pointer_manager(struct xwl_screen *xwl_screen,
                              uint32_t id, uint32_t version)
{
    xwl_screen->relative_pointer_manager =
        wl_registry_bind(xwl_screen->registry, id,
                         &zwp_relative_pointer_manager_v1_interface,
                         1);
}

static void
init_pointer_constraints(struct xwl_screen *xwl_screen,
                         uint32_t id, uint32_t version)
{
    xwl_screen->pointer_constraints =
        wl_registry_bind(xwl_screen->registry, id,
                         &zwp_pointer_constraints_v1_interface,
                         1);
}

static void
init_pointer_gestures(struct xwl_screen *xwl_screen,
                      uint32_t id, uint32_t version)
{
    xwl_screen->pointer_gestures =
        wl_registry_bind(xwl_screen->registry, id,
                         &zwp_pointer_gestures_v1_interface,
                         1);
}

static void
init_keyboard_grab(struct xwl_screen *xwl_screen,
                   uint32_t id, uint32_t version)
{
    struct xwl_seat *xwl_seat;
    DeviceIntPtr master;

    xwl_screen->wp_grab =
         wl_registry_bind(xwl_screen->registry, id,
                          &zwp_xwayland_keyboard_grab_manager_v1_interface,
                          1);

    xorg_list_for_each_entry(xwl_seat, &xwl_screen->seat_list, link) {
        if (xwl_seat->keyboard) {
            master = GetMaster(xwl_seat->keyboard, MASTER_KEYBOARD);
            if (master)
                setup_keyboard_grab_handler(master);
        }
    }
}

static void
init_keyboard_shortcuts_inhibit(struct xwl_screen *xwl_screen,
                                uint32_t id, uint32_t version)
{
    xwl_screen->shortcuts_inhibit_manager =
         wl_registry_bind(xwl_screen->registry, id,
                          &zwp_keyboard_shortcuts_inhibit_manager_v1_interface,
                          1);
}

/* The compositor may send us wl_seat and its capabilities before sending e.g.
   relative_pointer_manager or pointer_gesture interfaces. This would result in
   devices being created in capabilities handler, but listeners not, because
   the interfaces weren't available at the time. So we manually attempt to setup
   listeners again.
*/
static void
maybe_init_relative_pointer_listeners_after_capabilities(struct xwl_screen* xwl_screen)
{
    struct xwl_seat *xwl_seat;
    xorg_list_for_each_entry(xwl_seat, &xwl_screen->seat_list, link) {
        if (xwl_seat->wl_pointer) {
            init_relative_pointer_listener(xwl_seat);
        }
    }
}

static void
maybe_init_pointer_gesture_listeners_after_capabilities(struct xwl_screen* xwl_screen)
{
    struct xwl_seat *xwl_seat;
    xorg_list_for_each_entry(xwl_seat, &xwl_screen->seat_list, link) {
        if (xwl_seat->wl_pointer) {
            init_pointer_gestures_listener(xwl_seat);
        }
    }
}

static void
input_handler(void *data, struct wl_registry *registry, uint32_t id,
              const char *interface, uint32_t version)
{
    struct xwl_screen *xwl_screen = data;

    if (strcmp(interface, wl_seat_interface.name) == 0 && version >= 3) {
        create_input_device(xwl_screen, id, version);
        xwl_screen->expecting_event++;
    } else if (strcmp(interface, zwp_relative_pointer_manager_v1_interface.name) == 0) {
        init_relative_pointer_manager(xwl_screen, id, version);
        maybe_init_relative_pointer_listeners_after_capabilities(xwl_screen);
    } else if (strcmp(interface, zwp_pointer_constraints_v1_interface.name) == 0) {
        init_pointer_constraints(xwl_screen, id, version);
    } else if (strcmp(interface, zwp_pointer_gestures_v1_interface.name) == 0) {
        init_pointer_gestures(xwl_screen, id, version);
        maybe_init_pointer_gesture_listeners_after_capabilities(xwl_screen);
    } else if (strcmp(interface, zwp_tablet_manager_v2_interface.name) == 0) {
        init_tablet_manager(xwl_screen, id, version);
    } else if (strcmp(interface, zwp_xwayland_keyboard_grab_manager_v1_interface.name) == 0) {
        init_keyboard_grab(xwl_screen, id, version);
    } else if (strcmp(interface, zwp_keyboard_shortcuts_inhibit_manager_v1_interface.name) == 0) {
        init_keyboard_shortcuts_inhibit(xwl_screen, id, version);
    }
}

static void
global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
}

static const struct wl_registry_listener input_listener = {
    input_handler,
    global_remove,
};

void
ProcessInputEvents(void)
{
    mieqProcessInputEvents();
}

void
DDXRingBell(int volume, int pitch, int duration)
{
}

static Bool
sprite_check_lost_focus(SpritePtr sprite, WindowPtr window)
{
    DeviceIntPtr device, master;
    struct xwl_seat *xwl_seat;

    for (device = inputInfo.devices; device; device = device->next) {
        /* Ignore non-wayland devices */
        if (device->deviceProc == xwl_pointer_proc &&
            device->spriteInfo->sprite == sprite)
            break;
    }

    if (!device)
        return FALSE;

    xwl_seat = device->public.devicePrivate;
    if (!xwl_seat)
        return FALSE;

    master = GetMaster(device, POINTER_OR_FLOAT);
    if (!master || !master->lastSlave)
        return FALSE;

    /* We do want the last active slave, we only check on slave xwayland
     * devices so we can find out the xwl_seat, but those don't actually own
     * their sprite, so the match doesn't mean a lot.
     */
    if (master->lastSlave != get_pointer_device(xwl_seat))
        return FALSE;

    if (xwl_seat->focus_window != NULL &&
        xwl_seat->cursor_confinement_window != NULL &&
        xwl_seat->focus_window != xwl_seat->cursor_confinement_window)
        return TRUE;

    if (xwl_seat->focus_window == NULL &&
        xwl_seat->last_focus_window != NULL &&
        (xwl_seat->last_focus_window->toplevel == window ||
         IsParent(xwl_seat->last_focus_window->toplevel, window)))
        return TRUE;

    return FALSE;
}

static WindowPtr
xwl_xy_to_window(ScreenPtr screen, SpritePtr sprite, int x, int y)
{
    struct xwl_screen *xwl_screen;
    WindowPtr ret;

    xwl_screen = xwl_screen_get(screen);

    screen->XYToWindow = xwl_screen->XYToWindow;
    ret = screen->XYToWindow(screen, sprite, x, y);
    xwl_screen->XYToWindow = screen->XYToWindow;
    screen->XYToWindow = xwl_xy_to_window;

    /* If the device controlling the sprite has left the Wayland surface but
     * the DIX still finds the pointer within the X11 window, it means that
     * the pointer has crossed to another native Wayland window, in this
     * case, pretend we entered the root window so that a LeaveNotify
     * event is emitted.
     */
    if (sprite_check_lost_focus(sprite, ret)) {
        sprite->spriteTraceGood = 1;
        return sprite->spriteTrace[0];
    }

    return ret;
}

void
xwl_seat_clear_touch(struct xwl_seat *xwl_seat, struct xwl_window *xwl_window)
{
    struct xwl_touch *xwl_touch, *next_xwl_touch;

    xorg_list_for_each_entry_safe(xwl_touch, next_xwl_touch,
                                  &xwl_seat->touches, link_touch) {
        if (xwl_touch->window == xwl_window) {
            xorg_list_del(&xwl_touch->link_touch);
            free(xwl_touch);
        }
    }
}

static void
xwl_pointer_warp_emulator_set_fake_pos(struct xwl_pointer_warp_emulator *warp_emulator,
                                       int x,
                                       int y)
{
    struct zwp_locked_pointer_v1 *locked_pointer =
        warp_emulator->locked_pointer;
    WindowPtr window;
    int sx, sy;

    if (!warp_emulator->locked_pointer)
        return;

    if (!warp_emulator->xwl_seat->focus_window)
        return;

    window = warp_emulator->xwl_seat->focus_window->toplevel;
    if (x >= window->drawable.x ||
        y >= window->drawable.y ||
        x < (window->drawable.x + window->drawable.width) ||
        y < (window->drawable.y + window->drawable.height)) {
        sx = x - window->drawable.x;
        sy = y - window->drawable.y;
        zwp_locked_pointer_v1_set_cursor_position_hint(locked_pointer,
                                                       wl_fixed_from_int(sx),
                                                       wl_fixed_from_int(sy));
        wl_surface_commit(warp_emulator->xwl_seat->focus_window->surface);
    }
}

static Bool
xwl_pointer_warp_emulator_is_locked(struct xwl_pointer_warp_emulator *warp_emulator)
{
    if (warp_emulator->locked_pointer)
        return TRUE;
    else
        return FALSE;
}

static void
xwl_pointer_warp_emulator_lock(struct xwl_pointer_warp_emulator *warp_emulator)
{
    struct xwl_seat *xwl_seat = warp_emulator->xwl_seat;
    struct xwl_screen *xwl_screen = xwl_seat->xwl_screen;
    struct zwp_pointer_constraints_v1 *pointer_constraints =
        xwl_screen->pointer_constraints;
    struct xwl_window *lock_window = xwl_seat->focus_window;

    warp_emulator->locked_window = lock_window;

    warp_emulator->locked_pointer =
        zwp_pointer_constraints_v1_lock_pointer(pointer_constraints,
                                                lock_window->surface,
                                                xwl_seat->wl_pointer,
                                                NULL,
                                                ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT);
}

static void
xwl_pointer_warp_emulator_maybe_lock(struct xwl_pointer_warp_emulator *warp_emulator,
                                     struct xwl_window *xwl_window,
                                     SpritePtr sprite,
                                     int x, int y)
{
    struct xwl_seat *xwl_seat = warp_emulator->xwl_seat;
    GrabPtr pointer_grab = xwl_seat->pointer->deviceGrab.grab;

    if (warp_emulator->locked_pointer)
        return;

    /*
     * If there is no grab, and the window doesn't have pointer focus, ignore
     * the warp, as under Wayland it won't receive input anyway.
     */
    if (!pointer_grab && xwl_seat->focus_window != xwl_window)
        return;

    /*
     * If there is a grab, but it's not an ownerEvents grab and the destination
     * is not the pointer focus, ignore it, as events wouldn't be delivered
     * there anyway.
     */
    if (pointer_grab &&
        !pointer_grab->ownerEvents &&
        sprite &&
        XYToWindow(sprite, x, y) != xwl_seat->focus_window->toplevel)
        return;

    xwl_pointer_warp_emulator_lock(warp_emulator);
}

static void
xwl_pointer_warp_emulator_warp(struct xwl_pointer_warp_emulator *warp_emulator,
                               struct xwl_window *xwl_window,
                               SpritePtr sprite,
                               int x, int y)
{
    xwl_pointer_warp_emulator_maybe_lock(warp_emulator,
                                         xwl_window,
                                         sprite,
                                         x, y);
    xwl_pointer_warp_emulator_set_fake_pos(warp_emulator, x, y);
}

static void
xwl_pointer_warp_emulator_handle_motion(struct xwl_pointer_warp_emulator *warp_emulator,
                                        double dx,
                                        double dy,
                                        double dx_unaccel,
                                        double dy_unaccel)
{
    struct xwl_seat *xwl_seat = warp_emulator->xwl_seat;
    ValuatorMask mask;
    WindowPtr window;
    int x, y;

    valuator_mask_zero(&mask);
    valuator_mask_set_unaccelerated(&mask, 0, dx, dx_unaccel);
    valuator_mask_set_unaccelerated(&mask, 1, dy, dy_unaccel);

    QueuePointerEvents(xwl_seat->relative_pointer, MotionNotify, 0,
                       POINTER_RELATIVE, &mask);

    window = xwl_seat->focus_window->toplevel;
    miPointerGetPosition(xwl_seat->pointer, &x, &y);

    if (xwl_pointer_warp_emulator_is_locked(warp_emulator) &&
        xwl_seat->cursor_confinement_window != warp_emulator->locked_window &&
        (x < window->drawable.x ||
         y < window->drawable.y ||
         x >= (window->drawable.x + window->drawable.width) ||
         y >= (window->drawable.y + window->drawable.height)))
        xwl_seat_destroy_pointer_warp_emulator(xwl_seat);
    else
        xwl_pointer_warp_emulator_set_fake_pos(warp_emulator, x, y);
}

static struct xwl_pointer_warp_emulator *
xwl_pointer_warp_emulator_create(struct xwl_seat *xwl_seat)
{
    struct xwl_pointer_warp_emulator *warp_emulator;

    warp_emulator = calloc(1, sizeof *warp_emulator);
    if (!warp_emulator) {
        ErrorF("%s: ENOMEM\n", __func__);
        return NULL;
    }

    warp_emulator->xwl_seat = xwl_seat;

    return warp_emulator;
}

static void
xwl_pointer_warp_emulator_destroy(struct xwl_pointer_warp_emulator *warp_emulator)
{
    if (warp_emulator->locked_pointer)
        zwp_locked_pointer_v1_destroy(warp_emulator->locked_pointer);
    free(warp_emulator);
}

static void
xwl_seat_create_pointer_warp_emulator(struct xwl_seat *xwl_seat)
{
    if (xwl_seat->confined_pointer)
        xwl_seat_destroy_confined_pointer(xwl_seat);

    xwl_seat->pointer_warp_emulator =
        xwl_pointer_warp_emulator_create(xwl_seat);
}

static Bool
xwl_seat_can_emulate_pointer_warp(struct xwl_seat *xwl_seat)
{
    struct xwl_screen *xwl_screen;

    if (!xwl_seat)
        return FALSE;

    if (!xwl_seat->pointer)
        return FALSE;

    xwl_screen = xwl_seat->xwl_screen;

    if (!xwl_screen->relative_pointer_manager)
        return FALSE;

    if (!xwl_screen->pointer_constraints)
        return FALSE;

    return TRUE;
}

void
xwl_seat_emulate_pointer_warp(struct xwl_seat *xwl_seat,
                              struct xwl_window *xwl_window,
                              SpritePtr sprite,
                              int x, int y)
{
    if (!xwl_seat_can_emulate_pointer_warp(xwl_seat))
        return;

    if (xwl_seat->x_cursor != NULL)
        return;

    if (!xwl_seat->pointer_warp_emulator)
        xwl_seat_create_pointer_warp_emulator(xwl_seat);

    if (!xwl_seat->pointer_warp_emulator)
        return;

    xwl_pointer_warp_emulator_warp(xwl_seat->pointer_warp_emulator,
                                   xwl_window,
                                   sprite,
                                   x, y);
}

static Bool
xwl_seat_maybe_lock_on_hidden_cursor(struct xwl_seat *xwl_seat)
{
    struct xwl_screen *xwl_screen = xwl_seat->xwl_screen;

    /* Some clients use hidden cursor+confineTo+relative motion
     * to implement infinite panning (eg. 3D views), lock the
     * pointer for so the relative pointer is used.
     */
    if (xwl_seat->x_cursor)
        return FALSE;

    if (!xwl_seat->focus_window)
        return FALSE;

    if (!xwl_screen->rootless)
        return FALSE;

    if (xwl_seat->cursor_confinement_window != xwl_seat->focus_window)
        return FALSE;

    if (xwl_seat->confined_pointer)
        xwl_seat_destroy_confined_pointer(xwl_seat);

    xwl_seat_create_pointer_warp_emulator(xwl_seat);
    xwl_pointer_warp_emulator_lock(xwl_seat->pointer_warp_emulator);
    return TRUE;
}

void
xwl_seat_cursor_visibility_changed(struct xwl_seat *xwl_seat)
{
    if (xwl_seat->pointer_warp_emulator && xwl_seat->x_cursor != NULL) {
        xwl_seat_destroy_pointer_warp_emulator(xwl_seat);
    } else if (!xwl_seat->x_cursor && xwl_seat->cursor_confinement_window) {
        /* If the cursor goes hidden as is confined, lock it for
         * relative motion to work. */
        xwl_seat_maybe_lock_on_hidden_cursor(xwl_seat);
    }
}

void
xwl_seat_destroy_pointer_warp_emulator(struct xwl_seat *xwl_seat)
{
    if (!xwl_seat->pointer_warp_emulator)
        return;

    xwl_pointer_warp_emulator_destroy(xwl_seat->pointer_warp_emulator);
    xwl_seat->pointer_warp_emulator = NULL;

    if (xwl_seat->cursor_confinement_window) {
        xwl_seat_confine_pointer(xwl_seat,
                                 xwl_seat->cursor_confinement_window);
    }
}

void
xwl_seat_confine_pointer(struct xwl_seat *xwl_seat,
                         struct xwl_window *xwl_window)
{
    struct zwp_pointer_constraints_v1 *pointer_constraints =
        xwl_seat->xwl_screen->pointer_constraints;

    if (!pointer_constraints)
        return;

    if (!xwl_seat->wl_pointer)
        return;

    if (xwl_seat->cursor_confinement_window == xwl_window &&
        xwl_seat->confined_pointer)
        return;

    xwl_seat_unconfine_pointer(xwl_seat);

    xwl_seat->cursor_confinement_window = xwl_window;

    if (xwl_seat->pointer_warp_emulator)
        return;

    if (xwl_seat_maybe_lock_on_hidden_cursor(xwl_seat))
        return;

    xwl_seat->confined_pointer =
        zwp_pointer_constraints_v1_confine_pointer(pointer_constraints,
                                                   xwl_window->surface,
                                                   xwl_seat->wl_pointer,
                                                   NULL,
                                                   ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT);
}

static void
xwl_seat_destroy_confined_pointer(struct xwl_seat *xwl_seat)
{
    zwp_confined_pointer_v1_destroy(xwl_seat->confined_pointer);
    xwl_seat->confined_pointer = NULL;
}

void
xwl_seat_unconfine_pointer(struct xwl_seat *xwl_seat)
{
    xwl_seat->cursor_confinement_window = NULL;

    if (xwl_seat->confined_pointer)
        xwl_seat_destroy_confined_pointer(xwl_seat);
}

void
InitInput(int argc, char *argv[])
{
    ScreenPtr pScreen = screenInfo.screens[0];
    struct xwl_screen *xwl_screen = xwl_screen_get(pScreen);

    if (!dixRegisterPrivateKey(&xwl_tablet_private_key, PRIVATE_DEVICE, 0)) {
        ErrorF("Failed to register private key\n");
        return;
    }

    mieqInit();

    xwl_screen->input_registry = wl_display_get_registry(xwl_screen->display);
    wl_registry_add_listener(xwl_screen->input_registry, &input_listener,
                             xwl_screen);

    xwl_screen->XYToWindow = pScreen->XYToWindow;
    pScreen->XYToWindow = xwl_xy_to_window;

    xwl_screen_roundtrip(xwl_screen);
#ifdef XWL_HAS_EI
    if (xwl_screen->rootless)
        xwayland_override_xtest();
#endif
}

void
CloseInput(void)
{
#ifdef XWL_HAS_EI
    xwayland_restore_xtest();
#endif
    mieqFini();
}
