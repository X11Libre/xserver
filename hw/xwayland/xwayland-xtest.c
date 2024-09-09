/*
 * Copyright © 2020 Red Hat
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

#include <errno.h>
#include <unistd.h>
#include <libgen.h>
#include <libei.h>

#include "dix/dix_priv.h"
#include "dix/input_priv.h"
#include "os/client_priv.h"
#include "os/log_priv.h"

#include <inputstr.h>
#include <inpututils.h>

#ifdef XWL_HAS_EI_PORTAL
#include "liboeffis.h"
#endif

#include "xwayland-screen.h"
#include "xwayland-xtest.h"

#define debug_ei(...) DebugF("[xwayland ei] " __VA_ARGS__)
#define error_ei(...) ErrorF("[xwayland ei] " __VA_ARGS__)

#define SCROLL_STEP 120 /* libei's definition of a logical scroll step */

static struct xorg_list clients_for_reuse;

static DevPrivateKeyRec xwl_ei_private_key;
static DevPrivateKeyRec xwl_device_data_private_key;

struct xwl_device_data {
    DeviceSendEventsProc sendEventsProc;
};

struct xwl_emulated_event {
    DeviceIntPtr dev;
    int type;
    int detail;
    int flags;
    ValuatorMask mask;
    struct xorg_list link;
};

struct xwl_abs_device {
    struct xorg_list link;
    struct ei_device *device;
};

struct xwl_ei_client {
    struct xorg_list link;      /* in clients_for_reuse */
    ClientPtr client;           /* can be NULL if the X11 client is gone */
    char *cmdline;
    bool accept_pointer, accept_keyboard, accept_abs;
    struct ei *ei;
    int ei_fd;
#ifdef XWL_HAS_EI_PORTAL
    struct oeffis *oeffis;
    int oeffis_fd;
#endif
    struct ei_seat *ei_seat;
    struct ei_device *ei_pointer;
    struct ei_device *ei_keyboard;
    struct xorg_list abs_devices;
    struct xorg_list pending_emulated_events;

    OsTimerPtr disconnect_timer;
};

static void xwl_handle_ei_event(int fd, int ready, void *data);
static bool xwl_dequeue_emulated_events(struct xwl_ei_client *xwl_ei_client);

static struct xwl_device_data *
xwl_device_data_get(DeviceIntPtr dev)
{
    return dixLookupPrivate(&dev->devPrivates, &xwl_device_data_private_key);
}

static struct xwl_ei_client *
get_xwl_ei_client(ClientPtr client)
{
    return dixLookupPrivate(&client->devPrivates, &xwl_ei_private_key);
}

static void
xwl_queue_emulated_event(struct xwl_ei_client *xwl_ei_client, DeviceIntPtr dev,
                         int type, int detail, int flags, const ValuatorMask *mask)
{
    struct xwl_emulated_event *xwl_emulated_event;

    xwl_emulated_event = calloc(1, sizeof *xwl_emulated_event);
    if (!xwl_emulated_event) {
        error_ei("OOM, cannot queue event\n");
        return;
    }

    xwl_emulated_event->dev = dev;
    xwl_emulated_event->type = type;
    xwl_emulated_event->detail = detail;
    xwl_emulated_event->flags = flags;
    valuator_mask_copy(&xwl_emulated_event->mask, mask);

    xorg_list_append(&xwl_emulated_event->link,
        &xwl_ei_client->pending_emulated_events);
}

static void
xwl_clear_emulated_events(struct xwl_ei_client *xwl_ei_client)
{
    struct xwl_emulated_event *xwl_emulated_event, *next_xwl_emulated_event;

    xorg_list_for_each_entry_safe(xwl_emulated_event, next_xwl_emulated_event,
        &xwl_ei_client->pending_emulated_events, link) {
        xorg_list_del(&xwl_emulated_event->link);
        free(xwl_emulated_event);
    }
}

static void
add_ei_device(struct xwl_ei_client *xwl_ei_client, struct ei_device *device)
{
    bool used = true;

    /* Note: pointers in libei are split across four capabilities:
       pointer/pointer-absolute/button/scroll. We expect any decent
       compositor to give pointers the button + scroll interfaces too,
       if that's not the case we can look into *why* and fix this as needed.
       Meanwhile, we ignore any device that doesn't have button + scroll
       in addition to pointer caps.
      */

    if (ei_device_has_capability(device, EI_DEVICE_CAP_POINTER) &&
        ei_device_has_capability(device, EI_DEVICE_CAP_BUTTON) &&
        ei_device_has_capability(device, EI_DEVICE_CAP_SCROLL) &&
        xwl_ei_client->ei_pointer == NULL) {

        xwl_ei_client->ei_pointer = ei_device_ref(device);
        used = true;
    }

    if (ei_device_has_capability(device, EI_DEVICE_CAP_KEYBOARD) &&
        xwl_ei_client->ei_keyboard == NULL) {
        xwl_ei_client->ei_keyboard = ei_device_ref(device);
        used = true;
    }

    if (ei_device_has_capability(device, EI_DEVICE_CAP_POINTER_ABSOLUTE) &&
        ei_device_has_capability(device, EI_DEVICE_CAP_BUTTON) &&
        ei_device_has_capability(device, EI_DEVICE_CAP_SCROLL)) {
        struct xwl_abs_device *abs = calloc(1, sizeof(*abs));

        if (abs) {
            xorg_list_add(&abs->link, &xwl_ei_client->abs_devices);
            abs->device = ei_device_ref(device);
            used = true;
        }
    }

    if (!used)
        ei_device_close(device);
}

static void
free_oeffis(struct xwl_ei_client *xwl_ei_client)
{
#ifdef XWL_HAS_EI_PORTAL
    if (xwl_ei_client->oeffis) {
        debug_ei("Removing OEFFIS fd=%d\n", xwl_ei_client->oeffis_fd);
        if (xwl_ei_client->oeffis_fd >= 0)
            RemoveNotifyFd(xwl_ei_client->oeffis_fd);
        xwl_ei_client->oeffis = oeffis_unref(xwl_ei_client->oeffis);
    }
#endif
}

static void
free_ei(struct xwl_ei_client *xwl_ei_client)
{
    struct ei *ei = xwl_ei_client->ei;
    struct xwl_abs_device *abs, *tmp;
    ClientPtr client = xwl_ei_client->client;

    TimerCancel(xwl_ei_client->disconnect_timer);
    xorg_list_del(&xwl_ei_client->link);

    debug_ei("Removing EI fd=%d\n", xwl_ei_client->ei_fd);
    if (xwl_ei_client->ei_fd >= 0)
        RemoveNotifyFd(xwl_ei_client->ei_fd);
    ei_device_unref(xwl_ei_client->ei_pointer);
    ei_device_unref(xwl_ei_client->ei_keyboard);
    xorg_list_for_each_entry_safe(abs, tmp, &xwl_ei_client->abs_devices, link) {
        xorg_list_del(&abs->link);
        ei_device_unref(abs->device);
        free(abs);
    }

    xwl_clear_emulated_events(xwl_ei_client);
    if (client)
        dixSetPrivate(&client->devPrivates, &xwl_ei_private_key, NULL);

    free_oeffis(xwl_ei_client);

    ei_seat_unref(xwl_ei_client->ei_seat);
    ei_unref(ei);

    free(xwl_ei_client->cmdline);
    free(xwl_ei_client);
}

#ifdef XWL_HAS_EI_PORTAL
static void
setup_ei_from_oeffis(struct xwl_ei_client *xwl_ei_client)
{
    struct oeffis *oeffis = xwl_ei_client->oeffis;

    xwl_ei_client->ei_fd = oeffis_get_eis_fd(oeffis);
    if (xwl_ei_client->ei_fd < 0) {
        error_ei("Failed to setup EI file descriptor from oeffis\n");
        return;
    }
    ei_setup_backend_fd(xwl_ei_client->ei, xwl_ei_client->ei_fd);
    SetNotifyFd(xwl_ei_client->ei_fd, xwl_handle_ei_event,
        X_NOTIFY_READ, xwl_ei_client);
}

static void
xwl_handle_oeffis_event(int fd, int ready, void *data)
{
    struct xwl_ei_client *xwl_ei_client = data;
    struct oeffis *oeffis = xwl_ei_client->oeffis;
    enum oeffis_event_type event_type;
    bool done = false;

    oeffis_dispatch(oeffis);

    do {
        event_type = oeffis_get_event(oeffis);
        switch (event_type) {
            case OEFFIS_EVENT_NONE:
                debug_ei("OEFFIS event none\n");
                done = true;
                break;
            case OEFFIS_EVENT_CONNECTED_TO_EIS:
                debug_ei("OEFFIS connected to EIS\n");
                setup_ei_from_oeffis(xwl_ei_client);
                break;
            case OEFFIS_EVENT_DISCONNECTED:
                debug_ei("OEFFIS disconnected: %s\n",
                    oeffis_get_error_message(oeffis));
                xwl_dequeue_emulated_events(xwl_ei_client);
                free_ei(xwl_ei_client);
                done = true;
                break;
            case OEFFIS_EVENT_CLOSED:
                debug_ei("OEFFIS closed\n");
                free_ei(xwl_ei_client);
                done = true;
                break;
        }
    }
    while (!done);
}
#endif

static bool
setup_oeffis(struct xwl_ei_client *xwl_ei_client)
{
#ifdef XWL_HAS_EI_PORTAL
    xwl_ei_client->oeffis_fd = -1;
    xwl_ei_client->oeffis = oeffis_new(NULL);
    if (!xwl_ei_client->oeffis)
        return false;

    xwl_ei_client->oeffis_fd = oeffis_get_fd(xwl_ei_client->oeffis);
    if (xwl_ei_client->oeffis_fd < 0) {
        error_ei("Failed to setup OEFFIS file descriptor\n");
        return false;
    }

    SetNotifyFd(xwl_ei_client->oeffis_fd, xwl_handle_oeffis_event,
        X_NOTIFY_READ, xwl_ei_client);

    oeffis_create_session(xwl_ei_client->oeffis,
                          OEFFIS_DEVICE_KEYBOARD | OEFFIS_DEVICE_POINTER);

    return true;
#else
    return false;
#endif
}

static bool
setup_ei_from_socket(struct xwl_ei_client *xwl_ei_client)
{
    int rc;

    rc = ei_setup_backend_socket(xwl_ei_client->ei, NULL);

    if (rc != 0) {
        error_ei("Setup failed: %s\n", strerror(-rc));
        return false;
    }

    xwl_ei_client->ei_fd = ei_get_fd(xwl_ei_client->ei);
    if (xwl_ei_client->ei_fd < 0) {
        error_ei("Failed to setup EI file descriptor from socket\n");
        return false;
    }

    SetNotifyFd(xwl_ei_client->ei_fd, xwl_handle_ei_event,
        X_NOTIFY_READ, xwl_ei_client);

    return true;
}

static struct xwl_ei_client *
setup_ei(ClientPtr client)
{
    ScreenPtr pScreen = screenInfo.screens[0];
    struct xwl_ei_client *xwl_ei_client = NULL;
    struct xwl_screen *xwl_screen = xwl_screen_get(pScreen);
    struct ei *ei = NULL;
    char buffer[PATH_MAX];
    const char *cmdname;
    char *client_name = NULL;
    bool status = false;

    cmdname = GetClientCmdName(client);
    if (cmdname) {
        snprintf(buffer, sizeof(buffer) - 1, "%s", cmdname);
        client_name = basename(buffer);
    }

    if (!client_name) {
        error_ei("Failed to retrieve the client command line name\n");
        goto out;
    }

    xwl_ei_client = calloc(1, sizeof *xwl_ei_client);
    if (!xwl_ei_client) {
        error_ei("OOM, cannot setup EI\n");
        goto out;
    }

    xwl_ei_client->cmdline = Xstrdup(cmdname);
    xorg_list_init(&xwl_ei_client->link);

    ei = ei_new(NULL);
    ei_configure_name(ei, basename(client_name));

    /* We can't send events to EIS until we have a device and the device
     * is resumed.
     */
    xwl_ei_client->accept_pointer = false;
    xwl_ei_client->accept_keyboard = false;
    xwl_ei_client->accept_abs = false;
    xwl_ei_client->ei = ei;
    xwl_ei_client->ei_fd = -1;
    xwl_ei_client->client = client;
    xorg_list_init(&xwl_ei_client->pending_emulated_events);
    xorg_list_init(&xwl_ei_client->abs_devices);

    if (xwl_screen->enable_ei_portal)
        status = setup_oeffis(xwl_ei_client);
    if (!status)
        status = setup_ei_from_socket(xwl_ei_client);

    if (!status) {
        free(xwl_ei_client);
        xwl_ei_client = NULL;
        ei_unref(ei);
        error_ei("EI setup failed\n");
        /* We failed to setup EI using either backends, give up on EI. */
        xwayland_restore_xtest();
    }

 out:
    return xwl_ei_client;
}

static CARD32
disconnect_timer_cb(OsTimerPtr timer, CARD32 time, void *arg)
{
    struct xwl_ei_client *xwl_ei_client = arg;

    free_ei(xwl_ei_client);

    return 0;
}

static void
xwl_ei_start_emulating(struct xwl_ei_client *xwl_ei_client)
{
    static uint32_t sequence = 0;
    struct xwl_abs_device *abs;

    sequence++;
    if (xwl_ei_client->ei_pointer)
        ei_device_start_emulating(xwl_ei_client->ei_pointer, sequence);
    if (xwl_ei_client->ei_keyboard)
        ei_device_start_emulating(xwl_ei_client->ei_keyboard, sequence);
    xorg_list_for_each_entry(abs, &xwl_ei_client->abs_devices, link) {
        ei_device_start_emulating(abs->device, sequence);
    }
}

static void
xwl_ei_stop_emulating(struct xwl_ei_client *xwl_ei_client)
{
    struct xwl_abs_device *abs;

    if (xwl_ei_client->ei_pointer)
        ei_device_stop_emulating(xwl_ei_client->ei_pointer);
    if (xwl_ei_client->ei_keyboard)
        ei_device_stop_emulating(xwl_ei_client->ei_keyboard);
    xorg_list_for_each_entry(abs, &xwl_ei_client->abs_devices, link) {
        ei_device_stop_emulating(abs->device);
    }
}

static void
xwl_ei_handle_client_gone(struct xwl_ei_client *xwl_ei_client)
{
    ClientPtr client = xwl_ei_client->client;

    /* Make this EI client struct re-usable. xdotool only exists for a
     * fraction of a second, so let's make it re-use the same client every
     * time - this makes it easier to e.g. pause it */
    xorg_list_add(&xwl_ei_client->link, &clients_for_reuse);

    if (xorg_list_is_empty(&xwl_ei_client->pending_emulated_events))
        xwl_ei_stop_emulating(xwl_ei_client);

    debug_ei("Client %s is now reusable\n", xwl_ei_client->cmdline);

    /* Otherwise, we keep the EI part but break up with the X11 client */
    assert(client);
    dixSetPrivate(&client->devPrivates, &xwl_ei_private_key, NULL);
    xwl_ei_client->client = NULL;

    /* Set a timer for 10 minutes. If the same client doesn't reconnect,
     * free it properly */
    xwl_ei_client->disconnect_timer =
        TimerSet(xwl_ei_client->disconnect_timer, 0,
        10 * 60 * 1000, disconnect_timer_cb, xwl_ei_client);
}

static void
xwl_ei_state_client_callback(CallbackListPtr *pcbl, void *unused, void *data)
{
    NewClientInfoRec *clientinfo = (NewClientInfoRec *) data;
    ClientPtr client = clientinfo->client;
    struct xwl_ei_client *xwl_ei_client = get_xwl_ei_client(client);

    switch (client->clientState) {
        case ClientStateGone:
        case ClientStateRetained:
            if (xwl_ei_client)
                xwl_ei_handle_client_gone(xwl_ei_client);
            break;
        default:
            break;
    }
}

static inline unsigned int
buttonmap(unsigned int b)
{
    unsigned int button;

    switch (b) {
        case 0:
            button = 0;
            break;
        case 1:
            button = 0x110; /* BTN_LEFT   */
            break;
        case 2:
            button = 0x112; /* BTN_MIDDLE */
            break;
        case 3:
            button = 0x111; /* BTN_RIGHT  */
            break;
        default:
            button = b - 8 + 0x113; /* BTN_SIDE  */
            break;
    }

    return button;
}

static void
xwl_send_abs_event_to_ei(struct xwl_ei_client *xwl_ei_client, int sx, int sy)
{
    struct xwl_abs_device *abs;
    struct ei *ei = xwl_ei_client->ei;

    xorg_list_for_each_entry(abs, &xwl_ei_client->abs_devices, link) {
        struct ei_region *r;
        size_t idx = 0;

        while ((r = ei_device_get_region(abs->device, idx++))) {
            double x = sx, y = sy;

            if (ei_region_contains(r, x, y)) {
                ei_device_pointer_motion_absolute(abs->device, sx, sy);
                ei_device_frame(abs->device, ei_now(ei));
                return;
            }
        }
    }
}

static bool
xwl_send_event_to_ei(struct xwl_ei_client *xwl_ei_client,
                     int type, int detail, int flags, const ValuatorMask *mask)
{
    struct ei *ei = xwl_ei_client->ei;
    struct ei_device *ei_device = NULL;
    int x = 0, y = 0;

    debug_ei("Sending event type %d to EIS\n", type);

    switch (type) {
        case MotionNotify:
            valuator_mask_fetch(mask, 0, &x);
            valuator_mask_fetch(mask, 1, &y);

            if (flags & POINTER_ABSOLUTE) {
                if (!xwl_ei_client->accept_abs)
                    return false;

                xwl_send_abs_event_to_ei(xwl_ei_client, x, y);
            }
            else if (x || y) {
                if (!xwl_ei_client->accept_pointer)
                    return false;

                ei_device = xwl_ei_client->ei_pointer;
                ei_device_pointer_motion(ei_device, x, y);
                ei_device_frame(ei_device, ei_now(ei));
            }
            break;
        case ButtonPress:
        case ButtonRelease:
            if (!xwl_ei_client->accept_pointer)
                return false;

            ei_device = xwl_ei_client->ei_pointer;
            if (detail < 4 || detail > 7) {
                ei_device_button_button(ei_device,
                    buttonmap(detail), type == ButtonPress);
                ei_device_frame(ei_device, ei_now(ei));
            /* Scroll only on release */
            } else if (type == ButtonRelease) {
                if (detail == 4) {
                    ei_device_scroll_discrete(ei_device, 0, -SCROLL_STEP);
                } else if (detail == 5) {
                    ei_device_scroll_discrete(ei_device, 0, SCROLL_STEP);
                } else if (detail == 6) {
                    ei_device_scroll_discrete(ei_device, -SCROLL_STEP, 0);
                } else if (detail == 7) {
                    ei_device_scroll_discrete(ei_device, SCROLL_STEP, 0);
                }
                ei_device_frame(ei_device, ei_now(ei));
            }
            break;
        case KeyPress:
        case KeyRelease:
            if (!xwl_ei_client->accept_keyboard)
                return false;

            ei_device = xwl_ei_client->ei_keyboard;
            ei_device_keyboard_key(ei_device, detail - 8, type == KeyPress);
            ei_device_frame(ei_device, ei_now(ei));
            break;
        default:
            error_ei("XTEST event type %d is not implemented\n", type);
            break;
    }

    return true;
}

static struct xwl_ei_client *
reuse_client(ClientPtr client)
{
    struct xwl_ei_client *xwl_ei_client = NULL;
    const char *cmdname = GetClientCmdName(client);

    if (!cmdname)
        return NULL;

    debug_ei("Client maybe up for re-use: %s\n", cmdname);
    xorg_list_for_each_entry(xwl_ei_client, &clients_for_reuse, link) {
        debug_ei("Checking if we can re-use %s\n", xwl_ei_client->cmdline);
        if (xwl_ei_client->cmdline &&
            strcmp(xwl_ei_client->cmdline, cmdname) == 0) {
            debug_ei("Re-using client for %s\n", cmdname);
            xorg_list_del(&xwl_ei_client->link);
            xorg_list_init(&xwl_ei_client->link);
            TimerCancel(xwl_ei_client->disconnect_timer);
            xwl_ei_start_emulating(xwl_ei_client);
            return xwl_ei_client;
        }
    }

    return NULL;
}

static void
xwayland_xtest_fallback(DeviceIntPtr dev,
                        int type, int detail, int flags, const ValuatorMask *mask)
{
    struct xwl_device_data *xwl_device_data = xwl_device_data_get(dev);

    if (xwl_device_data->sendEventsProc != NULL) {
        debug_ei("EI failed, using XTEST as fallback for sending events\n");
        (xwl_device_data->sendEventsProc)(dev, type, detail, flags, mask);
    }
}


static void
xwayland_xtest_send_events(DeviceIntPtr dev,
                           int type, int detail, int flags, const ValuatorMask *mask)
{
    ClientPtr client;
    struct xwl_ei_client *xwl_ei_client;
    bool accept = false;

    if (!IsXTestDevice(dev, NULL))
        return;

    client = GetCurrentClient();
    xwl_ei_client = get_xwl_ei_client(client);
    if (!xwl_ei_client) {
        xwl_ei_client = reuse_client(client);
        if (xwl_ei_client)
            xwl_ei_client->client = client;
    }

    if (!xwl_ei_client) {
        if (!(xwl_ei_client = setup_ei(client))) {
            xwayland_xtest_fallback(dev, type, detail, flags, mask);
            return;
        }
    }
    dixSetPrivate(&client->devPrivates, &xwl_ei_private_key, xwl_ei_client);

    switch (type) {
        case MotionNotify:
            if (flags & POINTER_ABSOLUTE)
                accept = xwl_ei_client->accept_abs;
            else
                accept = xwl_ei_client->accept_pointer;
            break;
        case ButtonPress:
        case ButtonRelease:
            accept = xwl_ei_client->accept_pointer;
            break;
        case KeyPress:
        case KeyRelease:
            accept = xwl_ei_client->accept_keyboard;
            break;
        default:
            return;
    }

    if (accept) {
        xwl_send_event_to_ei(xwl_ei_client, type, detail, flags, mask);
    }
    else {
        debug_ei("Not yet connected to EIS, queueing events\n");
        xwl_queue_emulated_event(xwl_ei_client, dev, type, detail, flags, mask);
    }

}

static bool
xwl_dequeue_emulated_events(struct xwl_ei_client *xwl_ei_client)
{
    struct xwl_emulated_event *xwl_emulated_event, *next_xwl_emulated_event;
    bool sent;

    xorg_list_for_each_entry_safe(xwl_emulated_event, next_xwl_emulated_event,
        &xwl_ei_client->pending_emulated_events, link) {
        sent = xwl_send_event_to_ei(xwl_ei_client,
                                    xwl_emulated_event->type,
                                    xwl_emulated_event->detail,
                                    xwl_emulated_event->flags,
                                    &xwl_emulated_event->mask);
        if (!sent)
            xwayland_xtest_fallback(xwl_emulated_event->dev,
                                    xwl_emulated_event->type,
                                    xwl_emulated_event->detail,
                                    xwl_emulated_event->flags,
                                    &xwl_emulated_event->mask);

        xorg_list_del(&xwl_emulated_event->link);
        free(xwl_emulated_event);
    }
    return true;
}

static void
xwl_ei_update_caps(struct xwl_ei_client *xwl_ei_client,
                   struct ei_device *ei_device)
{
    struct xwl_abs_device *abs;

    if (ei_device == xwl_ei_client->ei_pointer)
        xwl_ei_client->accept_pointer = true;

    if (ei_device == xwl_ei_client->ei_keyboard)
        xwl_ei_client->accept_keyboard = true;

    xorg_list_for_each_entry(abs, &xwl_ei_client->abs_devices, link) {
        if (ei_device == abs->device)
            xwl_ei_client->accept_abs = true;
    }
}

static bool
xwl_ei_devices_are_ready(struct xwl_ei_client *xwl_ei_client)
{
    if ((xwl_ei_client->accept_keyboard ||
         !ei_seat_has_capability(xwl_ei_client->ei_seat, EI_DEVICE_CAP_KEYBOARD)) &&
        (xwl_ei_client->accept_pointer ||
         !ei_seat_has_capability(xwl_ei_client->ei_seat, EI_DEVICE_CAP_POINTER)) &&
        (xwl_ei_client->accept_abs ||
         !ei_seat_has_capability(xwl_ei_client->ei_seat, EI_DEVICE_CAP_POINTER_ABSOLUTE)))
        return true;

    return false;
}

static void
xwl_handle_ei_event(int fd, int ready, void *data)
{
    struct xwl_ei_client *xwl_ei_client = data;
    struct ei *ei;
    bool done = false;

    ei = xwl_ei_client->ei;

    ei_dispatch(ei);
    do {
        enum ei_event_type type;
        struct ei_event *e = ei_get_event(ei);
        struct ei_device *ei_device;

        if (!e)
            break;

        ei_device = ei_event_get_device(e);
        type = ei_event_get_type(e);
        switch (type) {
            case EI_EVENT_CONNECT:
                debug_ei("Connected\n");
                break;
            case EI_EVENT_SEAT_ADDED:
                /* We take the first seat that comes along and
                 * add our device there */
                if (!xwl_ei_client->ei_seat) {
                    struct ei_seat *seat = ei_event_get_seat(e);

                    xwl_ei_client->ei_seat = ei_seat_ref(seat);
                    debug_ei("Using seat: %s (caps: %s%s%s%s%s)\n",
                        ei_seat_get_name(seat), ei_seat_has_capability(seat,
                            EI_DEVICE_CAP_KEYBOARD) ? "k" : "",
                        ei_seat_has_capability(seat,
                            EI_DEVICE_CAP_POINTER) ? "p" : "",
                        ei_seat_has_capability(seat,
                            EI_DEVICE_CAP_POINTER_ABSOLUTE) ? "a" : "",
                        ei_seat_has_capability(seat,
                            EI_DEVICE_CAP_BUTTON) ? "b" : "",
                        ei_seat_has_capability(seat,
                            EI_DEVICE_CAP_SCROLL) ? "s" : "");
                    ei_seat_bind_capabilities(seat,
                                              EI_DEVICE_CAP_POINTER,
                                              EI_DEVICE_CAP_POINTER_ABSOLUTE,
                                              EI_DEVICE_CAP_BUTTON,
                                              EI_DEVICE_CAP_SCROLL,
                                              EI_DEVICE_CAP_KEYBOARD, NULL);
                }
                break;
            case EI_EVENT_SEAT_REMOVED:
                if (ei_event_get_seat(e) == xwl_ei_client->ei_seat) {
                    debug_ei("Seat was removed\n");
                    xwl_ei_client->ei_seat =
                        ei_seat_unref(xwl_ei_client->ei_seat);
                }
                break;
            case EI_EVENT_DEVICE_ADDED:
                debug_ei("New device: %s\n", ei_device_get_name(ei_device));
                add_ei_device(xwl_ei_client, ei_device);
                break;
            case EI_EVENT_DEVICE_REMOVED:
                debug_ei("Device removed: %s\n", ei_device_get_name(ei_device));
                {
                    struct xwl_abs_device *abs, *tmp;

                    xorg_list_for_each_entry_safe(abs, tmp,
                        &xwl_ei_client->abs_devices, link) {
                        if (abs->device != ei_device)
                            continue;
                        ei_device_unref(abs->device);
                        xorg_list_del(&abs->link);
                        free(abs);
                    }
                }
                if (xwl_ei_client->ei_pointer == ei_device)
                    xwl_ei_client->ei_pointer =
                        ei_device_unref(xwl_ei_client->ei_pointer);
                if (xwl_ei_client->ei_keyboard == ei_device)
                    xwl_ei_client->ei_keyboard =
                        ei_device_unref(xwl_ei_client->ei_keyboard);
                break;
            case EI_EVENT_DISCONNECT:
                debug_ei("Disconnected\n");
                free_ei(xwl_ei_client);
                done = true;
                break;
            case EI_EVENT_DEVICE_PAUSED:
                debug_ei("Device paused\n");
                if (ei_device == xwl_ei_client->ei_pointer)
                    xwl_ei_client->accept_pointer = false;
                if (ei_device == xwl_ei_client->ei_keyboard)
                    xwl_ei_client->accept_keyboard = false;
                {
                    struct xwl_abs_device *abs;

                    xorg_list_for_each_entry(abs, &xwl_ei_client->abs_devices,
                        link) {
                        if (ei_device == abs->device)
                            xwl_ei_client->accept_abs = false;
                    }
                }
                break;
            case EI_EVENT_DEVICE_RESUMED:
                debug_ei("Device resumed\n");
                xwl_ei_update_caps(xwl_ei_client, ei_device);
                /* Server has accepted our device (or resumed them),
                 * we can now start sending events */
                /* FIXME: Maybe add a timestamp and discard old events? */
                if (xwl_ei_devices_are_ready(xwl_ei_client)) {
                    xwl_ei_start_emulating(xwl_ei_client);
                    xwl_dequeue_emulated_events(xwl_ei_client);
                }
                if (!xwl_ei_client->client &&
                    xorg_list_is_empty(&xwl_ei_client->pending_emulated_events))
                    /* All events dequeued and client has disconnected in the meantime */
                    xwl_ei_stop_emulating(xwl_ei_client);
                break;
            case EI_EVENT_KEYBOARD_MODIFIERS:
                debug_ei("Ignored event %s (%d)\n", ei_event_type_to_string(type), type);
                /* Don't care */
                break;
            default:
                error_ei("Unhandled event %s (%d)\n", ei_event_type_to_string(type), type);
                break;
        }
        ei_event_unref(e);
    } while (!done);
}

Bool
xwayland_ei_init(void)
{
    xorg_list_init(&clients_for_reuse);

    if (!dixRegisterPrivateKey(&xwl_ei_private_key, PRIVATE_CLIENT, 0)) {
        ErrorF("Failed to register EI private key\n");
        return FALSE;
    }

    if (!AddCallback(&ClientStateCallback, xwl_ei_state_client_callback, NULL)) {
        ErrorF("Failed to add client state callback\n");
        return FALSE;
    }

    if (!dixRegisterPrivateKey(&xwl_device_data_private_key, PRIVATE_DEVICE,
                               sizeof(struct xwl_device_data))) {
        ErrorF("Failed to register private key for XTEST override\n");
        return FALSE;
    }

    return TRUE;
}

static void
xwayland_override_events_proc(DeviceIntPtr dev)
{
    struct xwl_device_data *xwl_device_data = xwl_device_data_get(dev);

    if (xwl_device_data->sendEventsProc != NULL)
        return;

    /* Save original sendEventsProc handler in case */
    xwl_device_data->sendEventsProc = dev->sendEventsProc;

    /* Set up our own sendEventsProc to forward events to EI */
    debug_ei("Overriding XTEST for %s\n", dev->name);
    dev->sendEventsProc = xwayland_xtest_send_events;
}

static void
xwayland_restore_events_proc(DeviceIntPtr dev)
{
    struct xwl_device_data *xwl_device_data = xwl_device_data_get(dev);

    if (xwl_device_data->sendEventsProc == NULL)
        return;

    /* Restore original sendEventsProc handler */
    debug_ei("Restoring XTEST for %s\n", dev->name);
    dev->sendEventsProc = xwl_device_data->sendEventsProc;
    xwl_device_data->sendEventsProc = NULL;
}

void
xwayland_override_xtest(void)
{
    DeviceIntPtr d;

    nt_list_for_each_entry(d, inputInfo.devices, next) {
        xwayland_override_events_proc(d);
    }
}

void
xwayland_restore_xtest(void)
{
    DeviceIntPtr d;

    nt_list_for_each_entry(d, inputInfo.devices, next) {
        xwayland_restore_events_proc(d);
    }
}
