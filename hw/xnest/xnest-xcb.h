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

#endif /* __XNEST__XCB_H */
