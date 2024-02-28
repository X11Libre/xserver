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

#ifndef XWAYLAND_WINDOW_H
#define XWAYLAND_WINDOW_H

#include <xwayland-config.h>

#include <sys/types.h>

#include <stdio.h>
#include <unistd.h>

#include <X11/X.h>

#include "dix/property_priv.h"

#include <dix.h>
#include <propertyst.h>
#include <validate.h>
#include <wayland-util.h>
#include <xf86drm.h>

#include "xwayland-types.h"
#include "xwayland-dmabuf.h"

struct xwl_wl_surface {
    OsTimerPtr wl_surface_destroy_timer;
    struct wl_surface *wl_surface;
    struct xorg_list link;
};

struct xwl_window_output {
    struct xorg_list link;
    struct xwl_output *xwl_output;
};

struct xwl_window {
    struct xwl_screen *xwl_screen;
    struct wl_surface *surface;
    struct wp_viewport *viewport;
    float viewport_scale_x, viewport_scale_y;
    int surface_scale;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;

    /* Top-level window for the Wayland surface:
     * - With rootful, the root window itself
     * - With rootless, a direct child of the root window
     * Mainly useful when the top-level window is needed, can also be used for
     * the X dimensions of the Wayland surface though.
     */
    WindowPtr toplevel;

    /* The window associated with the Wayland surface:
     * - If the top-level window has descendants which:
     *   - Cover it completely
     *   - Have no alpha channel
     *   - Use a different window pixmap than their parent for storage
     *   then the surface window is the lowest-level such descendant.
     * - Otherwise it's the top-level window itself.
     * Mainly useful for code dealing with (buffers for) the Wayland surface,
     * can also be used for the X dimensions of the Wayland surface though.
     */
    WindowPtr surface_window;
    RegionPtr surface_window_damage;

    struct xorg_list link_damage;
    struct xorg_list link_window;
    struct wl_callback *frame_callback;
    Bool allow_commits;
    struct xorg_list window_buffers_available;
    struct xorg_list window_buffers_unavailable;
    OsTimerPtr window_buffers_timer;
    struct wl_output *wl_output;
    struct wl_output *wl_output_fullscreen;
    struct xorg_list xwl_output_list;
    struct xorg_list frame_callback_list;
#ifdef XWL_HAS_LIBDECOR
    struct libdecor_frame *libdecor_frame;
#endif
    struct xwayland_surface_v1 *xwayland_surface;
    struct xwl_dmabuf_feedback feedback;
    /* If TRUE, the window buffer format supports scanout with implicit modifier */
    Bool has_implicit_scanout_support;
    struct wp_tearing_control_v1 *tearing_control;
    struct wp_fractional_scale_v1 *fractional_scale;
    int fractional_scale_numerator;
    struct wp_linux_drm_syncobj_surface_v1 *surface_sync;
};

struct xwl_window *xwl_window_get(WindowPtr window);
RegionPtr xwl_window_get_damage_region(struct xwl_window *xwl_window);
struct xwl_window *xwl_window_from_window(WindowPtr window);

Bool is_surface_from_xwl_window(struct wl_surface *surface);

void xwl_window_update_property(struct xwl_window *xwl_window,
                                PropertyStateRec *propstate);
Bool xwl_window_is_toplevel(WindowPtr window);
void xwl_window_check_resolution_change_emulation(struct xwl_window *xwl_window);
void xwl_window_rootful_update_title(struct xwl_window *xwl_window);
void xwl_window_rootful_update_fullscreen(struct xwl_window *xwl_window,
                                          struct xwl_output *xwl_output);
void xwl_window_set_window_pixmap(WindowPtr window, PixmapPtr pixmap);
void xwl_window_update_surface_window(struct xwl_window *xwl_window);

void xwl_window_leave_output(struct xwl_window *xwl_window,
                             struct xwl_output *xwl_output);
int xwl_window_get_max_output_scale(struct xwl_window *xwl_window);
Bool xwl_realize_window(WindowPtr window);
Bool xwl_unrealize_window(WindowPtr window);
Bool xwl_change_window_attributes(WindowPtr window, unsigned long mask);
void xwl_clip_notify(WindowPtr window, int dx, int dy);
int xwl_config_notify(WindowPtr window,
                      int x, int y,
                      int width, int height, int bw,
                      WindowPtr sib);
void xwl_resize_window(WindowPtr window,
                       int x, int y,
                       unsigned int width, unsigned int height,
                       WindowPtr sib);
void xwl_move_window(WindowPtr window,
                     int x, int y,
                     WindowPtr next_sib,
                     VTKind kind);
Bool xwl_destroy_window(WindowPtr window);
void xwl_window_post_damage(struct xwl_window *xwl_window);
void xwl_window_create_frame_callback(struct xwl_window *xwl_window);
void xwl_window_surface_do_destroy(struct xwl_wl_surface *xwl_wl_surface);
void xwl_window_set_input_region(struct xwl_window *xwl_window, RegionPtr input_shape);

Bool xwl_window_init(void);

#endif /* XWAYLAND_WINDOW_H */
