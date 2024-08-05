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

typedef struct {
    uint32_t background_pixmap;
    uint32_t background_pixel;
    uint32_t border_pixmap;
    uint32_t border_pixel;
    uint16_t bit_gravity;
    uint16_t win_gravity;
    uint16_t backing_store;
    uint32_t backing_planes;
    uint32_t backing_pixel;
    Bool save_under;
    uint32_t event_mask;
    uint32_t do_not_propagate_mask;
    Bool override_redirect;
    uint32_t colormap;
    uint32_t cursor;
} XnSetWindowAttr;

void xnestEncodeWindowAttr(XnSetWindowAttr attr, uint32_t mask, uint32_t *values);

typedef struct {
    int x, y;
    int width, height;
    int border_width;
    uint32_t sibling;
    int stack_mode;
} XnWindowChanges;

void xnConfigureWindow(xcb_connection_t *conn, uint32_t window, uint32_t mask, XnWindowChanges values);

typedef struct {
    int key_click_percent;
    int bell_percent;
    int bell_pitch;
    int bell_duration;
    int led;
    int led_mode;
    int key;
    int auto_repeat_mode;
} XnKeyboardControl;

void xnestEncodeKeyboardControl(XnKeyboardControl ctrl, long mask, uint32_t *value);

#endif /* __XNEST__XCB_H */
