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
    if ((priv == NULL) || (priv->gc == NULL)) return 0;

    // make sure Xlib's GC cache is written out before using (server side) GC.
    XFlushGC(xnestDisplay, priv->gc);
    return priv->gc->gid;
}
