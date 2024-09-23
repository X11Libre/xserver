/*
 * Copyright © 2011-2014 Intel Corporation
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

#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#ifdef XWL_HAS_GLAMOR
#include <glamor.h>
#endif

#include <X11/Xatom.h>
#include <X11/Xfuncproto.h>

#include "dix/input_priv.h"
#include "dix/property_priv.h"
#include "os/osdep.h"
#include "os/xserver_poll.h"

#include <micmap.h>
#include <misyncshm.h>
#include <os.h>
#include <fb.h>
#include <dixstruct.h>
#include <propertyst.h>
#include <inputstr.h>
#include <xacestr.h>

#include "xwayland-cursor.h"
#include "xwayland-screen.h"
#include "xwayland-window.h"
#include "xwayland-input.h"
#include "xwayland-output.h"
#include "xwayland-pixmap.h"
#include "xwayland-present.h"
#include "xwayland-shm.h"
#ifdef XWL_HAS_EI
#include "xwayland-xtest.h"
#endif
#ifdef XWL_HAS_GLAMOR
#include "xwayland-glamor.h"
#endif

#ifdef MITSHM
#include "shmint.h"
#endif

#include "xdg-output-unstable-v1-client-protocol.h"
#include "viewporter-client-protocol.h"
#include "xdg-shell-client-protocol.h"
#include "xwayland-shell-v1-client-protocol.h"
#include "tearing-control-v1-client-protocol.h"
#include "fractional-scale-v1-client-protocol.h"

static DevPrivateKeyRec xwl_screen_private_key;
static DevPrivateKeyRec xwl_client_private_key;

#define DEFAULT_DPI 96

_X_NORETURN
static void _X_ATTRIBUTE_PRINTF(1, 2)
xwl_give_up(const char *f, ...)
{
    va_list args;

    va_start(args, f);
    VErrorFSigSafe(f, args);
    va_end(args);

    CloseWellKnownConnections();
    OsCleanup(TRUE);
    fflush(stderr);
    exit(1);
}

struct xwl_client *
xwl_client_get(ClientPtr client)
{
    return dixLookupPrivate(&client->devPrivates, &xwl_client_private_key);
}

struct xwl_screen *
xwl_screen_get(ScreenPtr screen)
{
    return dixLookupPrivate(&screen->devPrivates, &xwl_screen_private_key);
}

Bool
xwl_screen_has_viewport_support(struct xwl_screen *xwl_screen)
{
    return wl_compositor_get_version(xwl_screen->compositor) >=
                            WL_SURFACE_DAMAGE_BUFFER_SINCE_VERSION &&
           xwl_screen->viewporter != NULL;
}

Bool
xwl_screen_has_resolution_change_emulation(struct xwl_screen *xwl_screen)
{
    /* Resolution change emulation is only supported in rootless mode and
     * it requires viewport support.
     */
    return xwl_screen->rootless && xwl_screen_has_viewport_support(xwl_screen);
}

/* Return the output @ 0x0, falling back to the first output in the list */
struct xwl_output *
xwl_screen_get_first_output(struct xwl_screen *xwl_screen)
{
    struct xwl_output *xwl_output;

    xorg_list_for_each_entry(xwl_output, &xwl_screen->output_list, link) {
        if (xwl_output->x == 0 && xwl_output->y == 0)
            return xwl_output;
    }

    if (xorg_list_is_empty(&xwl_screen->output_list))
        return NULL;

    return xorg_list_first_entry(&xwl_screen->output_list, struct xwl_output, link);
}

struct xwl_output *
xwl_screen_get_fixed_or_first_output(struct xwl_screen *xwl_screen)
{
    if (xwl_screen->fixed_output)
        return xwl_screen->fixed_output;

    return xwl_screen_get_first_output(xwl_screen);
}

int
xwl_screen_get_width(struct xwl_screen *xwl_screen)
{
    return round(xwl_screen->width);
}

int
xwl_screen_get_height(struct xwl_screen *xwl_screen)
{
    return round(xwl_screen->height);
}

static void
xwl_property_callback(CallbackListPtr *pcbl, void *closure,
                      void *calldata)
{
    ScreenPtr screen = closure;
    PropertyStateRec *rec = calldata;
    struct xwl_screen *xwl_screen;
    struct xwl_window *xwl_window;

    if (rec->win->drawable.pScreen != screen)
        return;

    xwl_window = xwl_window_get(rec->win);
    if (!xwl_window)
        return;

    xwl_screen = xwl_screen_get(screen);

    if (rec->prop->propertyName == xwl_screen->allow_commits_prop)
        xwl_window_update_property(xwl_window, rec);
}

#define readOnlyPropertyAccessMask (DixReadAccess |\
                                    DixGetAttrAccess |\
                                    DixListPropAccess |\
                                    DixGetPropAccess)

static void
xwl_access_property_callback(CallbackListPtr *pcbl, void *closure,
                             void *calldata)
{
    XacePropertyAccessRec *rec = calldata;
    PropertyPtr prop = *rec->ppProp;
    ClientPtr client = rec->client;
    Mask access_mode = rec->access_mode;
    ScreenPtr pScreen = closure;
    struct xwl_screen *xwl_screen = xwl_screen_get(pScreen);

    if (prop->propertyName == xwl_screen->allow_commits_prop) {
        /* Only the WM and the Xserver itself */
        if (client != serverClient &&
            client->index != xwl_screen->wm_client_id &&
            (access_mode & ~readOnlyPropertyAccessMask) != 0)
            rec->status = BadAccess;
    }
}

#undef readOnlyPropertyAccessMask

static void
xwl_root_window_finalized_callback(CallbackListPtr *pcbl,
                                   void *closure,
                                   void *calldata)
{
    ScreenPtr screen = closure;
    struct xwl_screen *xwl_screen = xwl_screen_get(screen);
    struct xwl_queued_drm_lease_device *queued_device, *next;

    xorg_list_for_each_entry_safe(queued_device, next,
                                  &xwl_screen->queued_drm_lease_devices, link) {
        xwl_screen_add_drm_lease_device(xwl_screen, queued_device->id);
        xorg_list_del(&queued_device->link);
        free(queued_device);
    }
    DeleteCallback(&RootWindowFinalizeCallback, xwl_root_window_finalized_callback, screen);
}

Bool
xwl_close_screen(ScreenPtr screen)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(screen);
    struct xwl_output *xwl_output, *next_xwl_output;
    struct xwl_seat *xwl_seat, *next_xwl_seat;
    struct xwl_wl_surface *xwl_wl_surface, *xwl_wl_surface_next;
#ifdef XWL_HAS_GLAMOR
    xwl_dmabuf_feedback_destroy(&xwl_screen->default_feedback);
#endif
    DeleteCallback(&PropertyStateCallback, xwl_property_callback, screen);
    XaceDeleteCallback(XACE_PROPERTY_ACCESS, xwl_access_property_callback, screen);

    xorg_list_for_each_entry_safe(xwl_output, next_xwl_output,
                                  &xwl_screen->output_list, link)
        xwl_output_destroy(xwl_output);

    if (xwl_screen->fixed_output)
        xwl_output_destroy(xwl_screen->fixed_output);

    xorg_list_for_each_entry_safe(xwl_seat, next_xwl_seat,
                                  &xwl_screen->seat_list, link)
        xwl_seat_destroy(xwl_seat);

    xwl_screen_release_tablet_manager(xwl_screen);

    struct xwl_drm_lease_device *device_data, *next;
    xorg_list_for_each_entry_safe(device_data, next,
                                  &xwl_screen->drm_lease_devices, link)
        xwl_screen_destroy_drm_lease_device(xwl_screen,
                                            device_data->drm_lease_device);

    xorg_list_for_each_entry_safe(xwl_wl_surface, xwl_wl_surface_next,
                                  &xwl_screen->pending_wl_surface_destroy, link)
        xwl_window_surface_do_destroy(xwl_wl_surface);

    RemoveNotifyFd(xwl_screen->wayland_fd);

    wl_display_disconnect(xwl_screen->display);

    screen->CloseScreen = xwl_screen->CloseScreen;

    free(xwl_screen);

    return screen->CloseScreen(screen);
}

static struct xwl_seat *
xwl_screen_get_default_seat(struct xwl_screen *xwl_screen)
{
    if (xorg_list_is_empty(&xwl_screen->seat_list))
        return NULL;

    return container_of(xwl_screen->seat_list.prev,
                        struct xwl_seat,
                        link);
}

static void
xwl_cursor_warped_to(DeviceIntPtr device,
                     ScreenPtr screen,
                     ClientPtr client,
                     WindowPtr window,
                     SpritePtr sprite,
                     int x, int y)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(screen);
    struct xwl_seat *xwl_seat = device->public.devicePrivate;
    struct xwl_window *xwl_window;
    WindowPtr focus;

    if (!xwl_seat)
        xwl_seat = xwl_screen_get_default_seat(xwl_screen);

    if (!window)
        window = XYToWindow(sprite, x, y);

    xwl_window = xwl_window_from_window(window);
    if (!xwl_window && xwl_seat->focus_window) {
        focus = xwl_seat->focus_window->toplevel;

        /* Warps on non wl_surface backed Windows are only allowed
         * as long as the pointer stays within the focus window.
         */
        if (x >= focus->drawable.x &&
            y >= focus->drawable.y &&
            x < focus->drawable.x + focus->drawable.width &&
            y < focus->drawable.y + focus->drawable.height) {
            if (!window) {
                DebugF("Warp relative to pointer, assuming pointer focus\n");
                xwl_window = xwl_seat->focus_window;
            } else if (window == screen->root) {
                DebugF("Warp on root window, assuming pointer focus\n");
                xwl_window = xwl_seat->focus_window;
            }
        }
    }
    if (!xwl_window)
        return;

    xwl_seat_emulate_pointer_warp(xwl_seat, xwl_window, sprite, x, y);
}

static void
xwl_set_shape(WindowPtr window,
              int kind)
{
    ScreenPtr screen = window->drawable.pScreen;
    struct xwl_screen *xwl_screen;
    struct xwl_window *xwl_window;

    xwl_screen = xwl_screen_get(screen);
    xwl_window = xwl_window_get(window);

    screen->SetShape = xwl_screen->SetShape;
    (*screen->SetShape) (window, kind);
    xwl_screen->SetShape = screen->SetShape;
    screen->SetShape = xwl_set_shape;

    if (!xwl_window)
        return;

    if (kind == ShapeInput) {
        xwl_window_set_input_region(xwl_window, wInputShape(window));
        if (xwl_window->allow_commits)
            wl_surface_commit(xwl_window->surface);
    }
}

static struct xwl_window *
find_matching_input_output_window(struct xwl_screen *xwl_screen,
                                  WindowPtr window)
{
    struct xwl_window *xwl_window;

    xorg_list_for_each_entry(xwl_window, &xwl_screen->window_list, link_window) {
        /* When confining happens on InputOnly windows, work out the InputOutput
         * window that would be covered by its geometry.
         */
        if (window->drawable.x < xwl_window->toplevel->drawable.x ||
            window->drawable.x + window->drawable.width >
            xwl_window->toplevel->drawable.x + xwl_window->toplevel->drawable.width ||
            window->drawable.y < xwl_window->toplevel->drawable.y ||
            window->drawable.y + window->drawable.height >
            xwl_window->toplevel->drawable.y + xwl_window->toplevel->drawable.height)
            continue;

        if (xwl_window->toplevel->drawable.class == InputOnly)
            continue;

        return xwl_window;
    }

    return NULL;
}

static void
xwl_cursor_confined_to(DeviceIntPtr device,
                       ScreenPtr screen,
                       WindowPtr window)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(screen);
    struct xwl_seat *xwl_seat = device->public.devicePrivate;
    struct xwl_window *xwl_window;

    /* If running rootful with host grab requested, do not tamper with
     * pointer confinement.
     */
    if (!xwl_screen->rootless && xwl_screen->host_grab && xwl_screen->has_grab)
        return;

    if (!xwl_seat)
        xwl_seat = xwl_screen_get_default_seat(xwl_screen);

    /* xwl_seat hasn't been setup yet, don't do anything just yet */
    if (!xwl_seat)
        return;

    if (window == screen->root) {
        xwl_seat_unconfine_pointer(xwl_seat);
        return;
    }

    xwl_window = xwl_window_from_window(window);
    if (!xwl_window && window->drawable.class == InputOnly) {
        DebugF("Confine on InputOnly window, finding matching toplevel\n");
        xwl_window = find_matching_input_output_window(xwl_screen, window);
    }
    if (!xwl_window)
        return;

    xwl_seat_confine_pointer(xwl_seat, xwl_window);
}

void
xwl_screen_check_resolution_change_emulation(struct xwl_screen *xwl_screen)
{
    struct xwl_window *xwl_window;

    xorg_list_for_each_entry(xwl_window, &xwl_screen->window_list, link_window)
        xwl_window_check_resolution_change_emulation(xwl_window);
}

static void
xwl_screen_post_damage(struct xwl_screen *xwl_screen)
{
    struct xwl_window *xwl_window, *next_xwl_window;
    struct xorg_list commit_window_list;

    xorg_list_init(&commit_window_list);

    xorg_list_for_each_entry_safe(xwl_window, next_xwl_window,
                                  &xwl_screen->damage_window_list, link_damage) {
        /* If we're waiting on a frame callback from the server,
         * don't attach a new buffer. */
        if (xwl_window->frame_callback)
            continue;

        if (!xwl_window->allow_commits)
            continue;

        xwl_window_post_damage(xwl_window);
        xorg_list_del(&xwl_window->link_damage);
        xorg_list_append(&xwl_window->link_damage, &commit_window_list);
    }

    if (xorg_list_is_empty(&commit_window_list))
        return;

#ifdef XWL_HAS_GLAMOR
    if (xwl_screen->glamor)
        glamor_block_handler(xwl_screen->screen);
#endif

    xorg_list_for_each_entry_safe(xwl_window, next_xwl_window,
                                  &commit_window_list, link_damage) {
        wl_surface_commit(xwl_window->surface);
        xorg_list_del(&xwl_window->link_damage);
    }
}

static void
xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base,
                 uint32_t serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    xdg_wm_base_ping,
};

static void
registry_global(void *data, struct wl_registry *registry, uint32_t id,
                const char *interface, uint32_t version)
{
    struct xwl_screen *xwl_screen = data;

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        uint32_t request_version = 1;

        if (version >= WL_SURFACE_DAMAGE_BUFFER_SINCE_VERSION)
            request_version = WL_SURFACE_DAMAGE_BUFFER_SINCE_VERSION;

        xwl_screen->compositor =
            wl_registry_bind(registry, id, &wl_compositor_interface, request_version);
    }
    else if (strcmp(interface, wl_shm_interface.name) == 0) {
        xwl_screen->shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        xwl_screen->xdg_wm_base =
            wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(xwl_screen->xdg_wm_base,
                                 &xdg_wm_base_listener,
                                 NULL);
    }
    else if (strcmp(interface, wl_output_interface.name) == 0 && version >= 2) {
        if (xwl_output_create(xwl_screen, id, (xwl_screen->fixed_output == NULL), version))
            xwl_screen->expecting_event++;
    }
    else if (strcmp(interface, zxdg_output_manager_v1_interface.name) == 0) {
        /* We support xdg-output from version 1 to version 3 */
        version = min(version, 3);
        xwl_screen->xdg_output_manager =
            wl_registry_bind(registry, id, &zxdg_output_manager_v1_interface, version);
        xwl_screen_init_xdg_output(xwl_screen);
    }
    else if (strcmp(interface, wp_drm_lease_device_v1_interface.name) == 0) {
        if (xwl_screen->screen->root == NULL) {
            struct xwl_queued_drm_lease_device *queued = malloc(sizeof(struct xwl_queued_drm_lease_device));
            queued->id = id;
            xorg_list_append(&queued->link, &xwl_screen->queued_drm_lease_devices);
        } else {
            xwl_screen_add_drm_lease_device(xwl_screen, id);
        }
    }
    else if (strcmp(interface, wp_viewporter_interface.name) == 0) {
        xwl_screen->viewporter = wl_registry_bind(registry, id, &wp_viewporter_interface, 1);
    }
    else if (strcmp(interface, xwayland_shell_v1_interface.name) == 0 && xwl_screen->rootless) {
        xwl_screen->xwayland_shell =
            wl_registry_bind(registry, id, &xwayland_shell_v1_interface, 1);
    }
    else if (strcmp(interface, wp_tearing_control_manager_v1_interface.name) == 0) {
        xwl_screen->tearing_control_manager =
            wl_registry_bind(registry, id, &wp_tearing_control_manager_v1_interface, 1);
    }
    else if (strcmp(interface, wp_fractional_scale_manager_v1_interface.name) == 0) {
        xwl_screen->fractional_scale_manager =
            wl_registry_bind(registry, id, &wp_fractional_scale_manager_v1_interface, 1);
    }
#ifdef XWL_HAS_GLAMOR
    else if (xwl_screen->glamor) {
        xwl_glamor_init_wl_registry(xwl_screen, registry, id, interface,
                                    version);
    }
#endif
}

static void
global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
    struct xwl_screen *xwl_screen = data;
    struct xwl_output *xwl_output, *tmp_xwl_output;
    struct xwl_drm_lease_device *lease_device, *tmp_lease_device;

    xorg_list_for_each_entry_safe(xwl_output, tmp_xwl_output,
                                  &xwl_screen->output_list, link) {
        if (xwl_output->server_output_id == name) {
            xwl_output_remove(xwl_output);
            break;
        }
    }

    xorg_list_for_each_entry_safe(lease_device, tmp_lease_device,
                                  &xwl_screen->drm_lease_devices, link) {
        if (lease_device->id == name) {
            wp_drm_lease_device_v1_release(lease_device->drm_lease_device);
            break;
        }
    }
}

static const struct wl_registry_listener registry_listener = {
    registry_global,
    global_remove
};

static void
xwl_read_events (struct xwl_screen *xwl_screen)
{
    int ret;

    if (xwl_screen->wait_flush)
        return;

    ret = wl_display_read_events(xwl_screen->display);
    if (ret == -1)
        xwl_give_up("failed to read Wayland events: %s\n", strerror(errno));

    xwl_screen->prepare_read = 0;

    ret = wl_display_dispatch_pending(xwl_screen->display);
    if (ret == -1)
        xwl_give_up("failed to dispatch Wayland events: %s\n", strerror(errno));
}

static int
xwl_display_pollout (struct xwl_screen *xwl_screen, int timeout)
{
    struct pollfd poll_fd;

    poll_fd.fd = wl_display_get_fd(xwl_screen->display);
    poll_fd.events = POLLOUT;

    return xserver_poll(&poll_fd, 1, timeout);
}

#ifdef XWL_HAS_LIBDECOR
static void
xwl_dispatch_events_with_libdecor(struct xwl_screen *xwl_screen)
{
    int ret = 0;

    assert(!xwl_screen->rootless);

    ret = libdecor_dispatch(xwl_screen->libdecor_context, 0);
    if (ret == -1)
        xwl_give_up("failed to dispatch Wayland events with libdecor: %s\n",
                    strerror(errno));
}

static void
handle_libdecor_error(struct libdecor *context,
                      enum libdecor_error error,
                      const char *message)
{
    xwl_give_up("libdecor error (%d): %s\n", error, message);
}

static struct libdecor_interface libdecor_iface = {
    .error = handle_libdecor_error,
};
#endif

static void
xwl_dispatch_events (struct xwl_screen *xwl_screen)
{
    int ret = 0;
    int ready;

    if (xwl_screen->wait_flush)
        goto pollout;

    while (xwl_screen->prepare_read == 0 &&
           wl_display_prepare_read(xwl_screen->display) == -1) {
        ret = wl_display_dispatch_pending(xwl_screen->display);
        if (ret == -1)
            xwl_give_up("failed to dispatch Wayland events: %s\n",
                       strerror(errno));
    }

    xwl_screen->prepare_read = 1;

pollout:
    ready = xwl_display_pollout(xwl_screen, 5);
    if (ready == -1 && errno != EINTR)
        xwl_give_up("error polling on Xwayland fd: %s\n", strerror(errno));

    if (ready > 0)
        ret = wl_display_flush(xwl_screen->display);

    if (ret == -1 && errno != EAGAIN)
        xwl_give_up("failed to write to Xwayland fd: %s\n", strerror(errno));

    xwl_screen->wait_flush = (ready == 0 || ready == -1 || ret == -1);
}

static void
socket_handler(int fd, int ready, void *data)
{
    struct xwl_screen *xwl_screen = data;

#ifdef XWL_HAS_LIBDECOR
    if (xwl_screen->libdecor_context) {
        xwl_dispatch_events_with_libdecor(xwl_screen);
        return;
    }
#endif
    xwl_read_events (xwl_screen);
}

static void
wakeup_handler(void *data, int err)
{
}

static void
block_handler(void *data, void *timeout)
{
    struct xwl_screen *xwl_screen = data;

    xwl_screen_post_damage(xwl_screen);
#ifdef XWL_HAS_LIBDECOR
    if (xwl_screen->libdecor_context) {
        xwl_dispatch_events_with_libdecor(xwl_screen);
        return;
    }
#endif
    xwl_dispatch_events (xwl_screen);
}

void
xwl_sync_events (struct xwl_screen *xwl_screen)
{
#ifdef XWL_HAS_LIBDECOR
    if (xwl_screen->libdecor_context) {
        xwl_dispatch_events_with_libdecor(xwl_screen);
        return;
    }
#endif
    xwl_dispatch_events (xwl_screen);
    xwl_read_events (xwl_screen);
}

void xwl_surface_damage(struct xwl_screen *xwl_screen,
                        struct wl_surface *surface,
                        int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (wl_surface_get_version(surface) >= WL_SURFACE_DAMAGE_BUFFER_SINCE_VERSION)
        wl_surface_damage_buffer(surface, x, y, width, height);
    else
        wl_surface_damage(surface, x, y, width, height);
}

void
xwl_screen_roundtrip(struct xwl_screen *xwl_screen)
{
    int ret;

    do {
        ret = wl_display_roundtrip(xwl_screen->display);
    } while (ret >= 0 && xwl_screen->expecting_event);

    if (ret < 0)
        xwl_give_up("could not connect to wayland server\n");
}

static int
xwl_server_grab(ClientPtr client)
{
    struct xwl_screen *xwl_screen;

    /* Allow GrabServer for the X11 window manager.
     * Xwayland only has 1 screen (no Zaphod for Xwayland) so we check
     * for the first and only screen here.
     */
    xwl_screen = xwl_screen_get(screenInfo.screens[0]);
    if (xwl_screen->wm_client_id == client->index)
        return xwl_screen->GrabServer(client);

    /* For all other clients, just pretend it works for compatibility,
       but do nothing */
    return Success;
}

static int
xwl_server_ungrab(ClientPtr client)
{
    struct xwl_screen *xwl_screen;

    /* Same as above, allow UngrabServer for the X11 window manager only */
    xwl_screen = xwl_screen_get(screenInfo.screens[0]);
    if (xwl_screen->wm_client_id == client->index)
        return xwl_screen->UngrabServer(client);

    /* For all other clients, just pretend it works for compatibility,
       but do nothing */
    return Success;
}

static void
xwl_screen_setup_custom_vector(struct xwl_screen *xwl_screen)
{
    /* Rootfull Xwayland does not need a custom ProcVector (yet?) */
    if (!xwl_screen->rootless)
        return;

    xwl_screen->GrabServer = ProcVector[X_GrabServer];
    xwl_screen->UngrabServer = ProcVector[X_UngrabServer];

    ProcVector[X_GrabServer] = xwl_server_grab;
    ProcVector[X_UngrabServer] = xwl_server_ungrab;
}

int
xwl_screen_get_next_output_serial(struct xwl_screen *xwl_screen)
{
    return xwl_screen->output_name_serial++;
}

void
xwl_screen_lost_focus(struct xwl_screen *xwl_screen)
{
    struct xwl_seat *xwl_seat;

    xorg_list_for_each_entry(xwl_seat, &xwl_screen->seat_list, link) {
        xwl_seat_leave_ptr(xwl_seat, TRUE);
        xwl_seat_leave_kbd(xwl_seat);
    }
}

Bool
xwl_screen_should_use_fractional_scale(struct xwl_screen *xwl_screen)
{
    /* Fullscreen uses a viewport already */
    if (xwl_screen->fullscreen)
        return FALSE;

    if (xwl_screen->rootless)
        return FALSE;

    /* We need both fractional scale and viewporter protocols */
    if (!xwl_screen->fractional_scale_manager)
        return FALSE;

    if (!xwl_screen->viewporter)
        return FALSE;

    return xwl_screen->hidpi;
}

Bool
xwl_screen_update_global_surface_scale(struct xwl_screen *xwl_screen)
{
    ScreenPtr screen = xwl_screen->screen;
    struct xwl_window *xwl_window;
    int32_t old_scale;

    if (xwl_screen_should_use_fractional_scale(xwl_screen))
        return FALSE;

    if (xwl_screen->rootless)
        return FALSE;

    if (xwl_screen->fullscreen)
        return FALSE;

    if (!xwl_screen->hidpi)
        return FALSE;

    if (screen->root == NullWindow)
        return FALSE;

    xwl_window = xwl_window_get(screen->root);
    if (!xwl_window)
        return FALSE;

    old_scale = xwl_screen->global_surface_scale;
    xwl_screen->global_surface_scale = xwl_window_get_max_output_scale(xwl_window);

    return (xwl_screen->global_surface_scale != old_scale);
}

Bool
xwl_screen_init(ScreenPtr pScreen, int argc, char **argv)
{
    static const char allow_commits[] = "_XWAYLAND_ALLOW_COMMITS";
    struct xwl_screen *xwl_screen;
    Pixel red_mask, blue_mask, green_mask;
    int ret, bpc, green_bpc, i;
    unsigned int xwl_width = 640;
    unsigned int xwl_height = 480;
    Bool use_fixed_size = FALSE;

    if (!dixRegisterPrivateKey(&xwl_screen_private_key, PRIVATE_SCREEN, 0))
        return FALSE;
    if (!xwl_pixmap_init())
        return FALSE;
    if (!xwl_window_init())
        return FALSE;
    /* There are no easy to use new / delete client hooks, we could use a
     * ClientStateCallback, but it is easier to let the dix code manage the
     * memory for us. This will zero fill the initial xwl_client data.
     */
    if (!dixRegisterPrivateKey(&xwl_client_private_key, PRIVATE_CLIENT,
                               sizeof(struct xwl_client)))
        return FALSE;

    xwl_screen = calloc(1, sizeof *xwl_screen);
    if (xwl_screen == NULL)
        return FALSE;

    dixSetPrivate(&pScreen->devPrivates, &xwl_screen_private_key, xwl_screen);
    xwl_screen->screen = pScreen;

#ifdef XWL_HAS_EI
    if (!xwayland_ei_init())
        return FALSE;
#endif

#ifdef XWL_HAS_GLAMOR
    xwl_screen->glamor = XWL_GLAMOR_DEFAULT;
#endif

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-rootless") == 0) {
            xwl_screen->rootless = 1;

            /* Disable the XSS extension on Xwayland rootless.
             *
             * Xwayland is just a Wayland client, no X11 screensaver
             * should be expected to work reliably on Xwayland rootless.
             */
#ifdef SCREENSAVER
            noScreenSaverExtension = TRUE;
#endif
            ScreenSaverTime = 0;
            ScreenSaverInterval = 0;
            defaultScreenSaverTime = 0;
            defaultScreenSaverInterval = 0;
        }
        else if (strcmp(argv[i], "-shm") == 0) {
            xwl_screen->glamor = XWL_GLAMOR_NONE;
        }
#ifdef XWL_HAS_GLAMOR
        else if (strcmp(argv[i], "-glamor") == 0) {
            if (strncmp(argv[i + 1], "es", 2) == 0)
                xwl_screen->glamor = XWL_GLAMOR_GLES;
            else if (strncmp(argv[i + 1], "gl", 2) == 0)
                xwl_screen->glamor = XWL_GLAMOR_GL;
            else if (strncmp(argv[i + 1], "off", 3) == 0)
                xwl_screen->glamor = XWL_GLAMOR_NONE;
            else
                ErrorF("Xwayland glamor: unknown rendering API selected\n");
        }
#endif
        else if (strcmp(argv[i], "-force-xrandr-emulation") == 0) {
            xwl_screen->force_xrandr_emulation = 1;
        }
        else if (strcmp(argv[i], "-geometry") == 0) {
            sscanf(argv[i + 1], "%ix%i", &xwl_width, &xwl_height);
            if (xwl_width == 0 || xwl_height == 0) {
                ErrorF("invalid argument for -geometry %s\n", argv[i + 1]);
                return FALSE;
            }
            use_fixed_size = 1;
        }
        else if (strcmp(argv[i], "-fullscreen") == 0) {
            use_fixed_size = 1;
            xwl_screen->fullscreen = 1;
        }
        else if (strcmp(argv[i], "-output") == 0) {
            xwl_screen->output_name = argv[i + 1];
        }
        else if (strcmp(argv[i], "-host-grab") == 0) {
            xwl_screen->host_grab = 1;
            xwl_screen->has_grab = 1;
        }
        else if (strcmp(argv[i], "-decorate") == 0) {
#ifdef XWL_HAS_LIBDECOR
            xwl_screen->decorate = 1;
            use_fixed_size = 1;
#else
            ErrorF("This build does not have libdecor support\n");
#endif
        }
        else if (strcmp(argv[i], "-enable-ei-portal") == 0) {
#ifdef XWL_HAS_EI_PORTAL
            xwl_screen->enable_ei_portal = 1;
#else
            ErrorF("This build does not have XDG portal support\n");
#endif
        }
        else if (strcmp(argv[i], "-nokeymap") == 0) {
            xwl_screen->nokeymap = 1;
        }
        else if (strcmp(argv[i], "-hidpi") == 0) {
            xwl_screen->hidpi = 1;
        }
    }

    if (!xwl_screen->rootless) {
        use_fixed_size = 1;
        xwl_screen->width = xwl_width;
        xwl_screen->height = xwl_height;
    } else if (use_fixed_size) {
        ErrorF("error, cannot set a geometry when running rootless\n");
        return FALSE;
    }

    xwl_screen->display = wl_display_connect(NULL);
    if (xwl_screen->display == NULL) {
        ErrorF("could not connect to wayland server\n");
        return FALSE;
    }

#ifdef XWL_HAS_GLAMOR
    if (xwl_screen->glamor && !xwl_glamor_init_gbm(xwl_screen)) {
        ErrorF("xwayland glamor: failed to setup GBM backend, falling back to sw accel\n");
        xwl_screen->glamor = 0;
    }
#endif

    /* In rootless mode, we don't have any screen storage, and the only
     * rendering should be to redirected mode. */
    if (xwl_screen->rootless)
        xwl_screen->root_clip_mode = ROOT_CLIP_INPUT_ONLY;
    else
        xwl_screen->root_clip_mode = ROOT_CLIP_FULL;

    xorg_list_init(&xwl_screen->output_list);
    xorg_list_init(&xwl_screen->seat_list);
    xorg_list_init(&xwl_screen->damage_window_list);
    xorg_list_init(&xwl_screen->window_list);
    xorg_list_init(&xwl_screen->drm_lease_devices);
    xorg_list_init(&xwl_screen->queued_drm_lease_devices);
    xorg_list_init(&xwl_screen->drm_leases);
    xorg_list_init(&xwl_screen->pending_wl_surface_destroy);
    xwl_screen->depth = 24;
    xwl_screen->global_surface_scale = 1;

    if (!monitorResolution)
        monitorResolution = DEFAULT_DPI;

    if (use_fixed_size) {
        if (!xwl_screen_init_randr_fixed(xwl_screen))
            return FALSE;
    } else {
        if (!xwl_screen_init_output(xwl_screen))
            return FALSE;
    }

    xwl_screen->expecting_event = 0;
    xwl_screen->registry = wl_display_get_registry(xwl_screen->display);
    wl_registry_add_listener(xwl_screen->registry,
                             &registry_listener, xwl_screen);
    xwl_screen_roundtrip(xwl_screen);


    if (xwl_screen->fullscreen && xwl_screen->rootless) {
        ErrorF("error, cannot set fullscreen when running rootless\n");
        return FALSE;
    }

    if (xwl_screen->fullscreen && xwl_screen->decorate) {
        ErrorF("error, cannot use the decorate option when running fullscreen\n");
        return FALSE;
    }

    if (xwl_screen->fullscreen && !xwl_screen_has_viewport_support(xwl_screen)) {
        ErrorF("missing viewport support in the compositor, ignoring fullscreen\n");
        xwl_screen->fullscreen = FALSE;
    }

    if (xwl_screen->host_grab && xwl_screen->rootless) {
        ErrorF("error, cannot use host grab when running rootless\n");
        return FALSE;
    }

    if (!xwl_screen->rootless && !xwl_screen->xdg_wm_base) {
        ErrorF("missing XDG-WM-Base protocol\n");
        return FALSE;
    }

    bpc = xwl_screen->depth / 3;
    green_bpc = xwl_screen->depth - 2 * bpc;
    blue_mask = (1 << bpc) - 1;
    green_mask = ((1 << green_bpc) - 1) << bpc;
    red_mask = blue_mask << (green_bpc + bpc);

    miSetVisualTypesAndMasks(xwl_screen->depth,
                             ((1 << TrueColor) | (1 << DirectColor)),
                             green_bpc, TrueColor,
                             red_mask, green_mask, blue_mask);

    miSetPixmapDepths();

    ret = fbScreenInit(pScreen, NULL,
                       xwl_screen_get_width(xwl_screen),
                       xwl_screen_get_height(xwl_screen),
                       monitorResolution, monitorResolution, 0,
                       BitsPerPixel(xwl_screen->depth));
    if (!ret)
        return FALSE;

    fbPictureInit(pScreen, 0, 0);

#ifdef MITSHM
    ShmRegisterFbFuncs(pScreen);
#endif

#ifdef HAVE_XSHMFENCE
    if (!miSyncShmScreenInit(pScreen))
        return FALSE;
#endif

#ifdef XWL_HAS_LIBDECOR
    if (xwl_screen->decorate && !xwl_screen->rootless) {
        xwl_screen->libdecor_context = libdecor_new(xwl_screen->display, &libdecor_iface);
        xwl_screen->wayland_fd = libdecor_get_fd(xwl_screen->libdecor_context);
    }
    else
#endif
    {
        xwl_screen->wayland_fd = wl_display_get_fd(xwl_screen->display);
    }
    SetNotifyFd(xwl_screen->wayland_fd, socket_handler, X_NOTIFY_READ, xwl_screen);
    RegisterBlockAndWakeupHandlers(block_handler, wakeup_handler, xwl_screen);

    pScreen->blackPixel = 0;
    pScreen->whitePixel = 1;

    ret = fbCreateDefColormap(pScreen);

    if (!xwl_screen_init_cursor(xwl_screen))
        return FALSE;

#ifdef XWL_HAS_GLAMOR
    if (xwl_screen->glamor && !xwl_glamor_init(xwl_screen)) {
       ErrorF("Failed to initialize glamor, falling back to sw\n");
       xwl_screen->glamor = XWL_GLAMOR_NONE;
    }
#endif

    xwl_screen->present = xwl_present_init(pScreen);

    if (!xwl_screen->glamor) {
        xwl_screen->CreateScreenResources = pScreen->CreateScreenResources;
        pScreen->CreateScreenResources = xwl_shm_create_screen_resources;
        pScreen->CreatePixmap = xwl_shm_create_pixmap;
        pScreen->DestroyPixmap = xwl_shm_destroy_pixmap;
    }

    xwl_screen->RealizeWindow = pScreen->RealizeWindow;
    pScreen->RealizeWindow = xwl_realize_window;

    xwl_screen->UnrealizeWindow = pScreen->UnrealizeWindow;
    pScreen->UnrealizeWindow = xwl_unrealize_window;

    dixScreenHookWindowDestroy(pScreen, xwl_window_destroy, NULL);

    xwl_screen->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = xwl_close_screen;

    xwl_screen->ChangeWindowAttributes = pScreen->ChangeWindowAttributes;
    pScreen->ChangeWindowAttributes = xwl_change_window_attributes;

    xwl_screen->ClipNotify = pScreen->ClipNotify;
    pScreen->ClipNotify = xwl_clip_notify;

    xwl_screen->ConfigNotify = pScreen->ConfigNotify;
    pScreen->ConfigNotify = xwl_config_notify;

    xwl_screen->ResizeWindow = pScreen->ResizeWindow;
    pScreen->ResizeWindow = xwl_resize_window;

    xwl_screen->MoveWindow = pScreen->MoveWindow;
    pScreen->MoveWindow = xwl_move_window;

    xwl_screen->SetWindowPixmap = pScreen->SetWindowPixmap;
    pScreen->SetWindowPixmap = xwl_window_set_window_pixmap;

    xwl_screen->SetShape = pScreen->SetShape;
    pScreen->SetShape = xwl_set_shape;

    pScreen->CursorWarpedTo = xwl_cursor_warped_to;
    pScreen->CursorConfinedTo = xwl_cursor_confined_to;

    xwl_screen->allow_commits_prop = MakeAtom(allow_commits,
                                              strlen(allow_commits),
                                              TRUE);
    if (xwl_screen->allow_commits_prop == BAD_RESOURCE)
        return FALSE;

    AddCallback(&PropertyStateCallback, xwl_property_callback, pScreen);
    AddCallback(&RootWindowFinalizeCallback, xwl_root_window_finalized_callback, pScreen);
    XaceRegisterCallback(XACE_PROPERTY_ACCESS, xwl_access_property_callback, pScreen);

    xwl_screen_setup_custom_vector(xwl_screen);

    xwl_screen_roundtrip(xwl_screen);

    return ret;
}
