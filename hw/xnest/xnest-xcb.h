/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef __XNEST__XCB_H
#define __XNEST__XCB_H

#include <xcb/xcb.h>

struct xnest_upstream_info {
    xcb_connection_t *conn;
    uint32_t screenId;
    const xcb_screen_t *screenInfo;
    const xcb_setup_t *setup;
};

extern struct xnest_upstream_info xnestUpstreamInfo;

/* fetch upstream connection's xcb setup data */
void xnest_upstream_setup(void);

/* retrieve upstream GC XID for our xserver GC */
uint32_t xnest_upstream_gc(GCPtr pGC);

void xnest_wm_colormap_windows(xcb_connection_t *conn, xcb_window_t w,
                               xcb_window_t *windows, int count);

uint32_t xnest_create_bitmap_from_data(xcb_connection_t *conn, uint32_t drawable,
                                       const char *data, uint32_t width, uint32_t height);

uint32_t xnest_create_pixmap_from_bitmap_data(xcb_connection_t *conn, uint32_t drawable,
                                         const char *data, uint32_t width, uint32_t height,
                                         uint32_t fg, uint32_t bg, uint16_t depth);

void xnest_set_command(xcb_connection_t *conn, xcb_window_t window, char ** argv, int argc);

void xnest_xkb_init(xcb_connection_t *conn);
int xnest_xkb_device_id(xcb_connection_t *conn);

xcb_get_keyboard_mapping_reply_t *xnest_get_keyboard_mapping(xcb_connection_t *conn,
                                                             int min_keycode,
                                                             int count);

void xnest_get_pointer_control(xcb_connection_t *conn, int *acc_num, int *acc_den, int *threshold);

#endif /* __XNEST__XCB_H */
