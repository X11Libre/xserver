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

#include <X11/X.h>
#include <X11/Xdefs.h>
#include <X11/Xproto.h>

#include "dix/cursor_priv.h"
#include "dix/inpututils_priv.h"
#include "mi/mi_priv.h"

#include "screenint.h"
#include "input.h"
#include "misc.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "servermd.h"
#include "inputstr.h"

#include "xnest-xcb.h"

#include "Args.h"
#include "Color.h"
#include "Display.h"
#include "Screen.h"
#include "XNWindow.h"
#include "Events.h"
#include "Keyboard.h"
#include "Pointer.h"
#include "mipointer.h"

CARD32 lastEventTime = 0;

void
ProcessInputEvents(void)
{
    mieqProcessInputEvents();
}

int
TimeSinceLastInputEvent(void)
{
    if (lastEventTime == 0)
        lastEventTime = GetTimeInMillis();
    return GetTimeInMillis() - lastEventTime;
}

void
SetTimeSinceLastInputEvent(void)
{
    lastEventTime = GetTimeInMillis();
}

void
xnestQueueKeyEvent(int type, unsigned int keycode)
{
    lastEventTime = GetTimeInMillis();
    QueueKeyboardEvents(xnestKeyboardDevice, type, keycode);
}

#define EVTYPE(tname) tname *ev = (tname*)event

static void
xnest_handle_event(xcb_generic_event_t *event)
{
    if (!event)
        return;

    switch (event->response_type & ~0x80) {
        case KeyPress:
        {
            EVTYPE(xcb_key_press_event_t);
            xnestUpdateModifierState(ev->state);
            xnestQueueKeyEvent(KeyPress, ev->detail);
            break;
        }

        case KeyRelease:
        {
            EVTYPE(xcb_key_release_event_t);
            xnestUpdateModifierState(ev->state);
            xnestQueueKeyEvent(KeyRelease, ev->detail);
            break;
        }

        case ButtonPress:
        {
            ValuatorMask mask;
            EVTYPE(xcb_button_press_event_t);
            valuator_mask_set_range(&mask, 0, 0, NULL);
            xnestUpdateModifierState(ev->state);
            lastEventTime = GetTimeInMillis();
            QueuePointerEvents(xnestPointerDevice, ButtonPress,
                               ev->detail, POINTER_RELATIVE, &mask);
            break;
        }

        case ButtonRelease:
        {
            ValuatorMask mask;
            EVTYPE(xcb_button_release_event_t);
            valuator_mask_set_range(&mask, 0, 0, NULL);
            xnestUpdateModifierState(ev->state);
            lastEventTime = GetTimeInMillis();
            QueuePointerEvents(xnestPointerDevice, ButtonRelease,
                               ev->detail, POINTER_RELATIVE, &mask);
            break;
        }

        case MotionNotify:
        {
            EVTYPE(xcb_motion_notify_event_t);
            ValuatorMask mask;
            int valuators[2];
            valuators[0] = ev->event_x;
            valuators[1] = ev->event_y;
            valuator_mask_set_range(&mask, 0, 2, valuators);
            lastEventTime = GetTimeInMillis();
            QueuePointerEvents(xnestPointerDevice, MotionNotify,
                               0, POINTER_ABSOLUTE, &mask);
            break;
        }

        case FocusIn:
        {
            EVTYPE(xcb_focus_in_event_t);
            if (ev->detail != NotifyInferior) {
                ScreenPtr pScreen = xnestScreen(ev->event);
                if (pScreen)
                    xnestDirectInstallColormaps(pScreen);
            }
            break;
        }

        case FocusOut:
        {
            EVTYPE(xcb_focus_out_event_t);
            if (ev->detail != NotifyInferior) {
                ScreenPtr pScreen = xnestScreen(ev->event);
                if (pScreen)
                    xnestDirectUninstallColormaps(pScreen);
            }
            break;
        }

        case KeymapNotify:
            break;

        case EnterNotify:
        {
            EVTYPE(xcb_enter_notify_event_t);
            if (ev->detail != NotifyInferior) {
                ScreenPtr pScreen = xnestScreen(ev->event);
                if (pScreen) {
                    ValuatorMask mask;
                    int valuators[2];
                    NewCurrentScreen(inputInfo.pointer, pScreen,
                                     ev->event_x, ev->event_y);
                    valuators[0] = ev->event_x;
                    valuators[1] = ev->event_y;
                    valuator_mask_set_range(&mask, 0, 2, valuators);
                    lastEventTime = GetTimeInMillis();
                    QueuePointerEvents(xnestPointerDevice, MotionNotify,
                                       0, POINTER_ABSOLUTE, &mask);
                    xnestDirectInstallColormaps(pScreen);
                }
            }
            break;
        }

        case LeaveNotify:
        {
            EVTYPE(xcb_leave_notify_event_t);
            if (ev->detail != NotifyInferior) {
                ScreenPtr pScreen = xnestScreen(ev->event);
                if (pScreen) {
                    xnestDirectUninstallColormaps(pScreen);
                }
            }
            break;
        }

        case DestroyNotify:
        {
            xcb_destroy_notify_event_t *ev = (xcb_destroy_notify_event_t*)event;
            if (xnestParentWindow &&
                ev->window == xnestParentWindow)
                exit(0);
            break;
        }

        case CirculateNotify:
        case ConfigureNotify:
        case GravityNotify:
        case MapNotify:
        case ReparentNotify:
        case UnmapNotify:
            break;

        case Expose:
        {
            EVTYPE(xcb_expose_event_t);
            WindowPtr pWin = xnestWindowPtr(ev->window);
            if (pWin && ev->width && ev->height) {
                RegionRec Rgn;
                BoxRec Box;
                Box.x1 = pWin->drawable.x + wBorderWidth(pWin) + ev->x;
                Box.y1 = pWin->drawable.y + wBorderWidth(pWin) + ev->y;
                Box.x2 = Box.x1 + ev->width;
                Box.y2 = Box.y1 + ev->height;
                RegionInit(&Rgn, &Box, 1);
                miSendExposures(pWin, &Rgn, Box.x1, Box.y1);
            }
        }
        break;

        case NoExpose:
            ErrorF("xnest: received stray NoExpose\n");
        break;
        case GraphicsExpose:
            ErrorF("xnest: received stray GraphicsExpose\n");
        break;

        default:
            ErrorF("xnest warning: unhandled event: %d\n", event->response_type);
            break;
    }
}

void
xnestCollectEvents(void)
{
    /* process queued events */
    struct xnest_event_queue *tmp = NULL, *walk = NULL;
    xorg_list_for_each_entry_safe(walk, tmp, &xnestUpstreamInfo.eventQueue.entry, entry) {
        xnest_handle_event(walk->event);
        xorg_list_del(&walk->entry);
        free(walk->event);
        free(walk);
    }

    xcb_flush(xnestUpstreamInfo.conn);

    int err = xcb_connection_has_error(xnestUpstreamInfo.conn);
    if (err) {
        ErrorF("Xnest: upstream connection error: %d\n", err);
        exit(0);
    }

    /* fetch new events from xcb */
    xcb_generic_event_t *event = NULL;
    while ((event = xcb_poll_for_event(xnestUpstreamInfo.conn))) {
        xnest_handle_event(event);
        free(event);
    }

    xcb_flush(xnestUpstreamInfo.conn);
}
