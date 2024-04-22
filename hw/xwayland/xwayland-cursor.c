/*
 * Copyright © 2014 Intel Corporation
 * Copyright © 2011 Kristian Høgsberg
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

#include "mi/mipointer_priv.h"

#include "scrnintstr.h"
#include "servermd.h"
#include "cursorstr.h"
#include "inputstr.h"
#include "mipointer.h"

#include "xwayland-cursor.h"
#include "xwayland-input.h"
#include "xwayland-pixmap.h"
#include "xwayland-screen.h"
#include "xwayland-shm.h"
#include "xwayland-types.h"

#include "tablet-unstable-v2-client-protocol.h"

#define DELAYED_X_CURSOR_TIMEOUT 5 /* ms */

static DevPrivateKeyRec xwl_cursor_private_key;

static void
expand_source_and_mask(CursorPtr cursor, CARD32 *data)
{
    CARD32 *p, d, fg, bg;
    CursorBitsPtr bits = cursor->bits;
    int x, y, stride, i, bit;

    p = data;
    fg = ((cursor->foreRed & 0xff00) << 8) |
          (cursor->foreGreen & 0xff00) |
          (cursor->foreBlue >> 8);
    bg = ((cursor->backRed & 0xff00) << 8) |
          (cursor->backGreen & 0xff00) |
          (cursor->backBlue >> 8);
    stride = BitmapBytePad(bits->width);
    for (y = 0; y < bits->height; y++)
        for (x = 0; x < bits->width; x++) {
            i = y * stride + x / 8;
            bit = 1 << (x & 7);
            if (bits->source[i] & bit)
                d = fg;
            else
                d = bg;
            if (bits->mask[i] & bit)
                d |= 0xff000000;
            else
                d = 0x00000000;

            *p++ = d;
        }
}

static Bool
xwl_realize_cursor(DeviceIntPtr device, ScreenPtr screen, CursorPtr cursor)
{
    PixmapPtr pixmap;

    pixmap = xwl_shm_create_pixmap(screen, cursor->bits->width,
                                   cursor->bits->height, 32,
                                   CREATE_PIXMAP_USAGE_BACKING_PIXMAP);
    dixSetPrivate(&cursor->devPrivates, &xwl_cursor_private_key, pixmap);

    return TRUE;
}

static Bool
xwl_unrealize_cursor(DeviceIntPtr device, ScreenPtr screen, CursorPtr cursor)
{
    PixmapPtr pixmap;
    struct xwl_screen *xwl_screen;
    struct xwl_seat *xwl_seat;

    pixmap = dixGetPrivate(&cursor->devPrivates, &xwl_cursor_private_key);
    if (!pixmap)
        return TRUE;

    dixSetPrivate(&cursor->devPrivates, &xwl_cursor_private_key, NULL);

    /* When called from FreeCursor(), device is always NULL */
    xwl_screen = xwl_screen_get(screen);
    xorg_list_for_each_entry(xwl_seat, &xwl_screen->seat_list, link) {
        if (cursor == xwl_seat->x_cursor)
            xwl_seat->x_cursor = NULL;
    }

    return xwl_shm_destroy_pixmap(pixmap);
}

static void
frame_callback(void *data,
               struct wl_callback *callback,
               uint32_t time)
{
    struct xwl_cursor *xwl_cursor = data;

    xwl_cursor_clear_frame_cb(xwl_cursor);
    if (xwl_cursor->needs_update) {
        xwl_cursor->needs_update = FALSE;
        xwl_cursor->update_proc(xwl_cursor);
    }
}

static const struct wl_callback_listener frame_listener = {
    frame_callback
};

static void
xwl_cursor_buffer_release_callback(void *data)
{
    /* drop the reference we took in set_cursor */
    xwl_shm_destroy_pixmap(data);
}

static void
xwl_cursor_copy_bits_to_pixmap(CursorPtr cursor, PixmapPtr pixmap)
{
    int stride;

    stride = cursor->bits->width * 4;
    if (cursor->bits->argb)
        memcpy(pixmap->devPrivate.ptr,
               cursor->bits->argb, cursor->bits->height * stride);
    else
        expand_source_and_mask(cursor, pixmap->devPrivate.ptr);
}

static void
xwl_cursor_attach_pixmap(struct xwl_seat *xwl_seat,
                         struct xwl_cursor *xwl_cursor, PixmapPtr pixmap)
{
    struct wl_buffer *buffer;
    struct xwl_screen *xwl_screen = xwl_seat->xwl_screen;

    buffer = xwl_shm_pixmap_get_wl_buffer(pixmap);
    if (!buffer) {
        ErrorF("cursor: Error getting buffer\n");
        return;
    }

    wl_surface_attach(xwl_cursor->surface, buffer, 0, 0);
    wl_surface_set_buffer_scale(xwl_cursor->surface, xwl_screen->global_surface_scale);
    xwl_surface_damage(xwl_screen, xwl_cursor->surface, 0, 0,
                       xwl_seat->x_cursor->bits->width,
                       xwl_seat->x_cursor->bits->height);

    xwl_cursor->frame_cb = wl_surface_frame(xwl_cursor->surface);
    wl_callback_add_listener(xwl_cursor->frame_cb, &frame_listener, xwl_cursor);

    /* Hold a reference on the pixmap until it's released by the compositor */
    pixmap->refcnt++;
    xwl_pixmap_set_buffer_release_cb(pixmap,
                                     xwl_cursor_buffer_release_callback,
                                     pixmap);

    wl_surface_commit(xwl_cursor->surface);
}

Bool
xwl_cursor_clear_frame_cb(struct xwl_cursor *xwl_cursor)
{
    if (xwl_cursor->frame_cb) {
        wl_callback_destroy(xwl_cursor->frame_cb);
        xwl_cursor->frame_cb = NULL;
        return TRUE;
    }

    return FALSE;
}

void
xwl_seat_set_cursor(struct xwl_seat *xwl_seat)
{
    struct xwl_cursor *xwl_cursor = &xwl_seat->cursor;
    struct xwl_screen *xwl_screen = xwl_seat->xwl_screen;
    PixmapPtr pixmap;
    CursorPtr cursor;
    int xhot, yhot;

    if (!xwl_seat->wl_pointer)
        return;

    if (!xwl_seat->x_cursor) {
        wl_pointer_set_cursor(xwl_seat->wl_pointer,
                              xwl_seat->pointer_enter_serial, NULL, 0, 0);
        xwl_cursor_clear_frame_cb(xwl_cursor);
        xwl_cursor->needs_update = FALSE;
        return;
    }

    if (xwl_cursor->frame_cb) {
        xwl_cursor->needs_update = TRUE;
        return;
    }

    cursor = xwl_seat->x_cursor;
    pixmap = dixGetPrivate(&cursor->devPrivates, &xwl_cursor_private_key);
    if (!pixmap)
        return;

    xwl_cursor_copy_bits_to_pixmap(cursor, pixmap);

    xhot = xwl_seat->x_cursor->bits->xhot / xwl_screen->global_surface_scale;
    yhot = xwl_seat->x_cursor->bits->yhot / xwl_screen->global_surface_scale;

    wl_pointer_set_cursor(xwl_seat->wl_pointer,
                          xwl_seat->pointer_enter_serial,
                          xwl_cursor->surface,
                          xhot,
                          yhot);

    xwl_cursor_attach_pixmap(xwl_seat, xwl_cursor, pixmap);
}

void
xwl_tablet_tool_set_cursor(struct xwl_tablet_tool *xwl_tablet_tool)
{
    struct xwl_seat *xwl_seat = xwl_tablet_tool->seat;
    struct xwl_screen *xwl_screen = xwl_seat->xwl_screen;
    struct xwl_cursor *xwl_cursor = &xwl_tablet_tool->cursor;
    PixmapPtr pixmap;
    CursorPtr cursor;
    int xhot, yhot;

    if (!xwl_seat->x_cursor) {
        zwp_tablet_tool_v2_set_cursor(xwl_tablet_tool->tool,
                                      xwl_tablet_tool->proximity_in_serial,
                                      NULL, 0, 0);
        xwl_cursor_clear_frame_cb(xwl_cursor);
        xwl_cursor->needs_update = FALSE;
        return;
    }

    if (xwl_cursor->frame_cb) {
        xwl_cursor->needs_update = TRUE;
        return;
    }

    cursor = xwl_seat->x_cursor;
    pixmap = dixGetPrivate(&cursor->devPrivates, &xwl_cursor_private_key);
    if (!pixmap)
        return;

    xwl_cursor_copy_bits_to_pixmap(cursor, pixmap);

    xhot = xwl_seat->x_cursor->bits->xhot / xwl_screen->global_surface_scale;
    yhot = xwl_seat->x_cursor->bits->yhot / xwl_screen->global_surface_scale;

    zwp_tablet_tool_v2_set_cursor(xwl_tablet_tool->tool,
                                  xwl_tablet_tool->proximity_in_serial,
                                  xwl_cursor->surface,
                                  xhot,
                                  yhot);

    xwl_cursor_attach_pixmap(xwl_seat, xwl_cursor, pixmap);
}

void
xwl_cursor_release(struct xwl_cursor *xwl_cursor)
{
    wl_surface_destroy(xwl_cursor->surface);
    xwl_cursor_clear_frame_cb(xwl_cursor);
}

static void
xwl_seat_update_all_cursors(struct xwl_seat *xwl_seat)
{
    struct xwl_tablet_tool *xwl_tablet_tool;

    xwl_seat_set_cursor(xwl_seat);

    xorg_list_for_each_entry(xwl_tablet_tool, &xwl_seat->tablet_tools, link) {
        if (xwl_tablet_tool->proximity_in_serial != 0)
            xwl_tablet_tool_set_cursor(xwl_tablet_tool);
    }

    /* Clear delayed cursor if any */
    xwl_seat->pending_x_cursor = NULL;
}

static void
xwl_seat_update_cursor_visibility(struct xwl_seat *xwl_seat)
{
    xwl_seat->x_cursor = xwl_seat->pending_x_cursor;
    xwl_seat_cursor_visibility_changed(xwl_seat);
    xwl_seat_update_all_cursors(xwl_seat);
}

static void
xwl_set_cursor_free_timer(struct xwl_seat *xwl_seat)
{
    if (xwl_seat->x_cursor_timer) {
        TimerFree(xwl_seat->x_cursor_timer);
        xwl_seat->x_cursor_timer = NULL;
    }
}

static CARD32
xwl_set_cursor_timer_callback(OsTimerPtr timer, CARD32 time, void *arg)
{
    struct xwl_seat *xwl_seat = arg;

    xwl_set_cursor_free_timer(xwl_seat);
    xwl_seat_update_cursor_visibility(xwl_seat);

    /* Don't re-arm the timer */
    return 0;
}

static void
xwl_set_cursor_delayed(struct xwl_seat *xwl_seat, CursorPtr cursor)
{
    xwl_seat->pending_x_cursor = cursor;

    if (xwl_seat->x_cursor_timer == NULL) {
        xwl_seat->x_cursor_timer = TimerSet(xwl_seat->x_cursor_timer,
                                            0, DELAYED_X_CURSOR_TIMEOUT,
                                            &xwl_set_cursor_timer_callback,
                                            xwl_seat);
    }
}

static void
xwl_set_cursor(DeviceIntPtr device,
               ScreenPtr screen, CursorPtr cursor, int x, int y)
{
    struct xwl_seat *xwl_seat;
    Bool cursor_visibility_changed;

    xwl_seat = device->public.devicePrivate;
    if (xwl_seat == NULL)
        return;

    cursor_visibility_changed = !!xwl_seat->x_cursor ^ !!cursor;

    if (!cursor_visibility_changed) {
        /* Cursor remains shown or hidden, apply the change immediately */
        xwl_set_cursor_free_timer(xwl_seat);
        xwl_seat->x_cursor = cursor;
        xwl_seat_update_all_cursors(xwl_seat);
        return;
    }

    xwl_seat->pending_x_cursor = cursor;
    if (cursor) {
        /* Cursor is being shown, delay the change until moved or timed out */
        xwl_set_cursor_delayed(xwl_seat, cursor);
    } else {
        /* Cursor is being hidden, apply the change immediately */
        xwl_seat_update_cursor_visibility(xwl_seat);
    }
}

static void
xwl_move_cursor(DeviceIntPtr device, ScreenPtr screen, int x, int y)
{
    struct xwl_seat *xwl_seat;

    xwl_seat = device->public.devicePrivate;
    if (xwl_seat == NULL)
        return;

    xwl_set_cursor_free_timer(xwl_seat);

    if (xwl_seat->pending_x_cursor)
        xwl_seat_update_cursor_visibility(xwl_seat);
}

static Bool
xwl_device_cursor_initialize(DeviceIntPtr device, ScreenPtr screen)
{
    return TRUE;
}

static void
xwl_device_cursor_cleanup(DeviceIntPtr device, ScreenPtr screen)
{
    struct xwl_seat *xwl_seat;

    xwl_seat = device->public.devicePrivate;
    if (xwl_seat)
        xwl_set_cursor_free_timer(xwl_seat);
}

static miPointerSpriteFuncRec xwl_pointer_sprite_funcs = {
    xwl_realize_cursor,
    xwl_unrealize_cursor,
    xwl_set_cursor,
    xwl_move_cursor,
    xwl_device_cursor_initialize,
    xwl_device_cursor_cleanup
};

static Bool
xwl_cursor_off_screen(ScreenPtr *ppScreen, int *x, int *y)
{
    return FALSE;
}

static void
xwl_cross_screen(ScreenPtr pScreen, Bool entering)
{
}

static void
xwl_pointer_warp_cursor(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y)
{
    miPointerWarpCursor(pDev, pScreen, x, y);
}

static miPointerScreenFuncRec xwl_pointer_screen_funcs = {
    xwl_cursor_off_screen,
    xwl_cross_screen,
    xwl_pointer_warp_cursor
};

Bool
xwl_screen_init_cursor(struct xwl_screen *xwl_screen)
{
    if (!dixRegisterPrivateKey(&xwl_cursor_private_key, PRIVATE_CURSOR, 0))
        return FALSE;

    return miPointerInitialize(xwl_screen->screen,
                               &xwl_pointer_sprite_funcs,
                               &xwl_pointer_screen_funcs, TRUE);
}
