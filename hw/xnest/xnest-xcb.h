/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef __XNEST__XCB_H
#define __XNEST__XCB_H

#include <xcb/xcb.h>

typedef struct {
    xcb_connection_t *conn;
    uint32_t screenId;
    const xcb_screen_t *screenInfo;
    const xcb_setup_t *setup;
} xnestUpstreamInfoRec;

extern xnestUpstreamInfoRec xnestUpstreamInfo;

/* fetch upstream connection's xcb setup data */
void xnest_upstream_setup(void);

/* generate a new XID for upstream X11 connection */
uint32_t xnestUpstreamXID(void);

/* retrieve upstream GC XID for our xserver GC */
uint32_t xnestUpstreamGC(GCPtr pGC);

#endif /* __XNEST__XCB_H */
