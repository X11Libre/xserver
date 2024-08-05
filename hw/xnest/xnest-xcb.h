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

typedef struct {
        int function;           /* logical operation */
        unsigned long plane_mask;/* plane mask */
        unsigned long foreground;/* foreground pixel */
        unsigned long background;/* background pixel */
        int line_width;         /* line width */
        int line_style;         /* LineSolid, LineOnOffDash, LineDoubleDash */
        int cap_style;          /* CapNotLast, CapButt,
                                   CapRound, CapProjecting */
        int join_style;         /* JoinMiter, JoinRound, JoinBevel */
        int fill_style;         /* FillSolid, FillTiled,
                                   FillStippled, FillOpaqueStippled */
        int fill_rule;          /* EvenOddRule, WindingRule */
        int arc_mode;           /* ArcChord, ArcPieSlice */
        xcb_pixmap_t tile;            /* tile pixmap for tiling operations */
        xcb_pixmap_t stipple;         /* stipple 1 plane pixmap for stippling */
        int ts_x_origin;        /* offset for tile or stipple operations */
        int ts_y_origin;
        xcb_font_t font;              /* default text font for text operations */
        int subwindow_mode;     /* ClipByChildren, IncludeInferiors */
        Bool graphics_exposures;/* boolean, should exposures be generated */
        int clip_x_origin;      /* origin for clipping */
        int clip_y_origin;
        xcb_pixmap_t clip_mask;       /* bitmap clipping; other calls for rects */
        int dash_offset;        /* patterned/dashed line information */
        char dashes;
} XnGCValues;

void xnChangeGC(xcb_connection_t *conn, uint32_t gc, XnGCValues gcval, uint32_t mask);

#endif /* __XNEST__XCB_H */
