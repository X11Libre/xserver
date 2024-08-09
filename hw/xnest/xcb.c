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

#include "include/gc.h"
#include "include/servermd.h"

#include "Xnest.h"
#include "xnest-xcb.h"
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
