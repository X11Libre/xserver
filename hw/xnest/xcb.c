/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>

#include <X11/X.h>
#include <X11/Xdefs.h>
#include <X11/Xproto.h>

#include "include/gc.h"

#include "Xnest.h"
#include "xnest-xcb.h"
#include "XNGC.h"

#include "Display.h"

xnestUpstreamInfoRec xnestUpstreamInfo = { 0 };

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

uint32_t xnestUpstreamXID(void) {
    return xcb_generate_id(xnestUpstreamInfo.conn);
}

/* retrieve upstream GC XID for our xserver GC */
uint32_t xnestUpstreamGC(GCPtr pGC) {
    if (pGC == NULL) return 0;

    xnestPrivGC *priv = dixLookupPrivate(&(pGC)->devPrivates, xnestGCPrivateKey);
    if (priv == NULL) return 0;

    return priv->gc;
}

void xnestEncodeWindowAttr(XnSetWindowAttr attr, uint32_t mask, uint32_t *values)
{
    int idx = 0;
#define EXTRA_VALUE(flag,val) if (mask & flag) { values[idx++] = attr.val; }
    EXTRA_VALUE(XCB_CW_BACK_PIXMAP,       background_pixmap);
    EXTRA_VALUE(XCB_CW_BACK_PIXEL,        background_pixel)
    EXTRA_VALUE(XCB_CW_BORDER_PIXMAP,     border_pixmap)
    EXTRA_VALUE(XCB_CW_BORDER_PIXEL,      border_pixel)
    EXTRA_VALUE(XCB_CW_BIT_GRAVITY,       bit_gravity)
    EXTRA_VALUE(XCB_CW_WIN_GRAVITY,       win_gravity);
    EXTRA_VALUE(XCB_CW_BACKING_STORE,     backing_store);
    EXTRA_VALUE(XCB_CW_BACKING_PLANES,    backing_planes);
    EXTRA_VALUE(XCB_CW_BACKING_PIXEL,     backing_pixel);
    EXTRA_VALUE(XCB_CW_OVERRIDE_REDIRECT, override_redirect);
    EXTRA_VALUE(XCB_CW_SAVE_UNDER,        save_under);
    EXTRA_VALUE(XCB_CW_EVENT_MASK,        event_mask);
    EXTRA_VALUE(XCB_CW_DONT_PROPAGATE,    do_not_propagate_mask);
    EXTRA_VALUE(XCB_CW_COLORMAP,          colormap);
    EXTRA_VALUE(XCB_CW_CURSOR,            cursor);
#undef EXTRA_VALUE
}

void xnConfigureWindow(xcb_connection_t *conn, uint32_t window, uint32_t mask, XnWindowChanges values)
{
    if (mask) {
        uint32_t value_list[16] = { 0 };

        int idx = 0;

#define EXTRA_VALUE(flag,val) if (mask & flag) { value_list[idx++] = values.val; }
        EXTRA_VALUE(XCB_CONFIG_WINDOW_X,            x);
        EXTRA_VALUE(XCB_CONFIG_WINDOW_Y,            y);
        EXTRA_VALUE(XCB_CONFIG_WINDOW_WIDTH,        width);
        EXTRA_VALUE(XCB_CONFIG_WINDOW_HEIGHT,       height);
        EXTRA_VALUE(XCB_CONFIG_WINDOW_BORDER_WIDTH, border_width);
        EXTRA_VALUE(XCB_CONFIG_WINDOW_SIBLING,      sibling);
        EXTRA_VALUE(XCB_CONFIG_WINDOW_STACK_MODE,   stack_mode);
#undef EXTRA_VALUE

        xcb_configure_window(conn, window, mask, value_list);
    }
}

void xnestEncodeKeyboardControl(XnKeyboardControl ctrl, long mask, uint32_t *value)
{
    if (mask & KBKeyClickPercent)
        *value++ = ctrl.key_click_percent;

    if (mask & KBBellPercent)
        *value++ = ctrl.bell_percent;

    if (mask & KBBellPitch)
        *value++ = ctrl.bell_pitch;

    if (mask & KBBellDuration)
        *value++ = ctrl.bell_duration;

    if (mask & KBLed)
        *value++ = ctrl.led;

    if (mask & KBLedMode)
        *value++ = ctrl.led_mode;

    if (mask & KBKey)
        *value++ = ctrl.key;

    if (mask & KBAutoRepeatMode)
        *value++ = ctrl.auto_repeat_mode;
}

void xnChangeGC(xcb_connection_t *conn, uint32_t gc, XnGCValues gcval, uint32_t mask)
{
    char value_list[128] = { 0 };
    char *walk = value_list;

#define EXTRA_VALUE(flag,val) if (mask & flag) { *((uint32_t*)walk) = gcval.val; walk+=4; }
    EXTRA_VALUE(GCFunction,          function);
    EXTRA_VALUE(GCPlaneMask,         plane_mask);
    EXTRA_VALUE(GCForeground,        foreground);
    EXTRA_VALUE(GCBackground,        background);
    EXTRA_VALUE(GCLineWidth,         line_width);
    EXTRA_VALUE(GCLineStyle,         line_style);
    EXTRA_VALUE(GCCapStyle,          cap_style);
    EXTRA_VALUE(GCJoinStyle,         join_style);
    EXTRA_VALUE(GCFillStyle,         fill_style);
    EXTRA_VALUE(GCFillRule,          fill_rule);
    EXTRA_VALUE(GCTile,              tile);
    EXTRA_VALUE(GCStipple,           stipple);
    EXTRA_VALUE(GCTileStipXOrigin,   ts_x_origin);
    EXTRA_VALUE(GCTileStipYOrigin,   ts_y_origin);
    EXTRA_VALUE(GCFont,              font);
    EXTRA_VALUE(GCSubwindowMode,     subwindow_mode);
    EXTRA_VALUE(GCGraphicsExposures, graphics_exposures);
    EXTRA_VALUE(GCClipXOrigin,       clip_x_origin);
    EXTRA_VALUE(GCClipYOrigin,       clip_y_origin);
    EXTRA_VALUE(GCClipMask,          clip_mask);
    EXTRA_VALUE(GCDashOffset,        dash_offset);
    EXTRA_VALUE(GCDashList,          dashes);
    EXTRA_VALUE(GCArcMode,           arc_mode);
#undef EXTRA_VALUE

    xcb_change_gc(conn, gc, mask, value_list);
}

const char WM_COLORMAP_WINDOWS[] = "WM_COLORMAP_WINDOWS";

void xnestWMColormapWindows(xcb_connection_t *conn, xcb_window_t w, xcb_window_t *windows, int count)
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
