/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_icccm.h>

#include <X11/X.h>
#include <X11/Xdefs.h>
#include <X11/Xproto.h>
#include <xcb/xkb.h>

#include "include/gc.h"
#include "include/servermd.h"

#include "Xnest.h"
#include "xnest-xcb.h"
#include "xnest-xkb.h"
#include "XNGC.h"

#include "Display.h"

struct xnest_upstream_info xnestUpstreamInfo = { 0 };

void xnest_upstream_setup(void) {
    xnestUpstreamInfo.screenId = DefaultScreen(xnestDisplay);

    /* retrieve setup data for our screen */
    xnestUpstreamInfo.conn = XGetXCBConnection(xnestDisplay);
    xnestUpstreamInfo.setup = xcb_get_setup(xnestUpstreamInfo.conn);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator (xnestUpstreamInfo.setup);

    for (int i = 0; i < xnestUpstreamInfo.screenId; ++i)
        xcb_screen_next (&iter);
    xnestUpstreamInfo.screenInfo = iter.data;
}

/* retrieve upstream GC XID for our xserver GC */
uint32_t xnest_upstream_gc(GCPtr pGC) {
    if (pGC == NULL) return 0;

    xnestPrivGC *priv = dixLookupPrivate(&(pGC)->devPrivates, xnestGCPrivateKey);
    if (priv == NULL) return 0;

    return priv->gc;
}

const char WM_COLORMAP_WINDOWS[] = "WM_COLORMAP_WINDOWS";

void xnest_wm_colormap_windows(
    xcb_connection_t *conn,
    xcb_window_t w,
    xcb_window_t *windows,
    int count)
{
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(
        conn,
        xcb_intern_atom(
            conn, 0,
            sizeof(WM_COLORMAP_WINDOWS)-1,
            WM_COLORMAP_WINDOWS),
        NULL);

    if (!reply)
        return;

    xcb_icccm_set_wm_colormap_windows_checked(
        conn,
        w,
        reply->atom,
        count,
        (xcb_window_t*)windows);

    free(reply);
}

uint32_t xnest_create_bitmap_from_data(
     xcb_connection_t *conn,
     uint32_t drawable,
     const char *data,
     uint32_t width,
     uint32_t height)
{
    uint32_t pix = xcb_generate_id(xnestUpstreamInfo.conn);
    xcb_create_pixmap(conn, 1, pix, drawable, width, height);

    uint32_t gc = xcb_generate_id(xnestUpstreamInfo.conn);
    xcb_create_gc(conn, gc, pix, 0, NULL);

    const int leftPad = 0;

    xcb_put_image(conn,
                  XYPixmap,
                  pix,
                  gc,
                  width,
                  height,
                  0 /* dst_x */,
                  0 /* dst_y */,
                  leftPad,
                  1 /* depth */,
                  BitmapBytePad(width + leftPad) * height,
                  (uint8_t*)data);

    xcb_free_gc(conn, gc);
    return pix;
}

uint32_t xnest_create_pixmap_from_bitmap_data(
    xcb_connection_t *conn,
    uint32_t drawable,
    const char *data,
    uint32_t width,
    uint32_t height,
    uint32_t fg,
    uint32_t bg,
    uint16_t depth)
{
    uint32_t pix = xcb_generate_id(xnestUpstreamInfo.conn);
    xcb_create_pixmap(conn, depth, pix, drawable, width, height);

    uint32_t gc = xcb_generate_id(xnestUpstreamInfo.conn);
    xcb_create_gc(conn, gc, pix, 0, NULL);

    xcb_params_gc_t gcv = {
        .foreground = fg,
        .background = bg
    };

    xcb_aux_change_gc(conn, gc, XCB_GC_FOREGROUND | XCB_GC_BACKGROUND, &gcv);

    const int leftPad = 0;
    xcb_put_image(conn,
                  XYBitmap,
                  pix,
                  gc,
                  width,
                  height,
                  0 /* dst_x */,
                  0 /* dst_y */,
                  leftPad,
                  1 /* depth */,
                  BitmapBytePad(width + leftPad) * height,
                  (uint8_t*)data);

    xcb_free_gc(conn, gc);
    return pix;
}

void xnest_set_command(
    xcb_connection_t *conn,
    xcb_window_t window,
    char **argv,
    int argc)
{
    int i = 0, nbytes = 0;

    for (i = 0, nbytes = 0; i < argc; i++)
        nbytes += strlen(argv[i]) + 1;

    if (nbytes >= (2^16) - 1)
        return;

    char buf[nbytes+1];
    char *bp = buf;

    /* copy arguments into single buffer */
    for (i = 0; i < argc; i++) {
        strcpy(bp, argv[i]);
        bp += strlen(argv[i]) + 1;
    }

    xcb_change_property(conn,
                        XCB_PROP_MODE_REPLACE,
                        window,
                        XCB_ATOM_WM_COMMAND,
                        XCB_ATOM_STRING,
                        8,
                        nbytes,
                        buf);
}

void xnest_xkb_init(xcb_connection_t *conn)
{
    xcb_generic_error_t *err = NULL;
    xcb_xkb_use_extension_reply_t *reply = xcb_xkb_use_extension_reply(
        xnestUpstreamInfo.conn,
        xcb_xkb_use_extension(
            xnestUpstreamInfo.conn,
            XCB_XKB_MAJOR_VERSION,
            XCB_XKB_MINOR_VERSION),
        &err);

    if (err) {
        ErrorF("failed query xkb extension: %d\n", err->error_code);
        free(err);
    } else {
        free(reply);
    }
}

#define XkbGBN_AllComponentsMask_2 ( \
    XCB_XKB_GBN_DETAIL_TYPES | \
    XCB_XKB_GBN_DETAIL_COMPAT_MAP | \
    XCB_XKB_GBN_DETAIL_CLIENT_SYMBOLS | \
    XCB_XKB_GBN_DETAIL_SERVER_SYMBOLS | \
    XCB_XKB_GBN_DETAIL_INDICATOR_MAPS | \
    XCB_XKB_GBN_DETAIL_KEY_NAMES | \
    XCB_XKB_GBN_DETAIL_GEOMETRY | \
    XCB_XKB_GBN_DETAIL_OTHER_NAMES)

int xnest_xkb_device_id(xcb_connection_t *conn)
{
    int device_id = -1;
    uint8_t xlen[6] = { 0 };
    xcb_generic_error_t *err = NULL;

    xcb_xkb_get_kbd_by_name_reply_t *reply = xcb_xkb_get_kbd_by_name_reply(
        xnestUpstreamInfo.conn,
        xcb_xkb_get_kbd_by_name_2(
            xnestUpstreamInfo.conn,
            XCB_XKB_ID_USE_CORE_KBD,
            XkbGBN_AllComponentsMask_2,
            XkbGBN_AllComponentsMask_2,
            0,
            sizeof(xlen),
            xlen),
        &err);

    if (err) {
        ErrorF("failed retrieving core keyboard: %d\n", err->error_code);
        free(err);
        return -1;
    }

    if (!reply) {
        ErrorF("failed retrieving core keyboard: no reply");
        return -1;
    }

    device_id = reply->deviceID;
    free(reply);
    return device_id;
}

xcb_get_keyboard_mapping_reply_t *xnest_get_keyboard_mapping(
    xcb_connection_t *conn,
    int min_keycode,
    int count
) {
    xcb_generic_error_t *err= NULL;
    xcb_get_keyboard_mapping_reply_t * reply = xcb_get_keyboard_mapping_reply(
        xnestUpstreamInfo.conn,
        xcb_get_keyboard_mapping(conn, min_keycode, count),
        &err);

    if (err) {
        ErrorF("Couldn't get keyboard mapping: %d\n", err->error_code);
        free(err);
    }

    return reply;
}

void xnest_get_pointer_control(
    xcb_connection_t *conn,
    int *acc_num,
    int *acc_den,
    int *threshold)
{
    xcb_generic_error_t *err = NULL;
    xcb_get_pointer_control_reply_t *reply = xcb_get_pointer_control_reply(
        xnestUpstreamInfo.conn,
        xcb_get_pointer_control(xnestUpstreamInfo.conn),
        &err);

    if (err) {
        ErrorF("error retrieving pointer control data: %d\n", err->error_code);
        free(err);
    }

    if (!reply) {
        ErrorF("error retrieving pointer control data: no reply\n");
        return;
    }

    *acc_num = reply->acceleration_numerator;
    *acc_den = reply->acceleration_denominator;
    *threshold = reply->threshold;
    free(reply);
}

xRectangle xnest_get_geometry(xcb_connection_t *conn, uint32_t window)
{
    xcb_generic_error_t *err = NULL;
    xcb_get_geometry_reply_t *reply = xcb_get_geometry_reply(
        xnestUpstreamInfo.conn,
        xcb_get_geometry(xnestUpstreamInfo.conn, window),
        &err);

    if (err) {
        ErrorF("failed getting window attributes for %d: %d\n", window, err->error_code);
        free(err);
        return (xRectangle) { 0 };
    }

    if (!reply) {
        ErrorF("failed getting window attributes for %d: no reply\n", window);
        return (xRectangle) { 0 };
    }

    return (xRectangle) {
        .x = reply->x,
        .y = reply->y,
        .width = reply->width,
        .height = reply->height };
}

static int __readint(const char *str, const char **next)
{
    int res = 0, sign = 1;

    if (*str=='+')
        str++;
    else if (*str=='-') {
        str++;
        sign = -1;
    }

    for (; (*str>='0') && (*str<='9'); str++)
        res = (res * 10) + (*str-'0');

    *next = str;
    return sign * res;
}

int xnest_parse_geometry(const char *string, xRectangle *geometry)
{
    int mask = 0;
    const char *next;
    xRectangle temp = { 0 };

    if ((string == NULL) || (*string == '\0')) return 0;

    if (*string == '=')
        string++;  /* ignore possible '=' at beg of geometry spec */

    if (*string != '+' && *string != '-' && *string != 'x') {
        temp.width = __readint(string, &next);
        if (string == next)
            return 0;
        string = next;
        mask |= XCB_CONFIG_WINDOW_WIDTH;
    }

    if (*string == 'x' || *string == 'X') {
        string++;
        temp.height = __readint(string, &next);
        if (string == next)
            return 0;
        string = next;
        mask |= XCB_CONFIG_WINDOW_HEIGHT;
    }

    if ((*string == '+') || (*string== '-')) {
        if (*string== '-') {
            string++;
            temp.x = -__readint(string, &next);
            if (string == next)
                return 0;
            string = next;
        }
        else
        {
            string++;
            temp.x = __readint(string, &next);
            if (string == next)
                return 0;
            string = next;
        }
        mask |= XCB_CONFIG_WINDOW_X;
        if ((*string == '+') || (*string== '-')) {
            if (*string== '-') {
                string++;
                temp.y = -__readint(string, &next);
                if (string == next)
                    return 0;
                string = next;
            }
            else
            {
                string++;
                temp.y = __readint(string, &next);
                if (string == next)
                    return 0;
                string = next;
            }
            mask |= XCB_CONFIG_WINDOW_Y;
        }
    }

    if (*string != '\0') return 0;

    if (mask & XCB_CONFIG_WINDOW_X)
        geometry->x = temp.x;
    if (mask & XCB_CONFIG_WINDOW_Y)
        geometry->y = temp.y;
    if (mask & XCB_CONFIG_WINDOW_WIDTH)
        geometry->width = temp.width;
    if (mask & XCB_CONFIG_WINDOW_HEIGHT)
        geometry->height = temp.height;

    return mask;
}
