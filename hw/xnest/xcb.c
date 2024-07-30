/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include <xcb/xcb.h>

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
    if ((priv == NULL) || (priv->gc == NULL)) return 0;

    // make sure Xlib's GC cache is written out before using (server side) GC.
    XFlushGC(xnestDisplay, priv->gc);
    return priv->gc->gid;
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
