/*

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/
#include <dix-config.h>

#include <string.h>
#include <errno.h>

#include <X11/X.h>
#include <X11/Xproto.h>

#include "os/client_priv.h"
#include "os/osdep.h"

#include "screenint.h"
#include "input.h"
#include "misc.h"
#include "scrnintstr.h"
#include "servermd.h"

#include "Xnest.h"
#include "xnest-xcb.h"

#include "Display.h"
#include "Init.h"
#include "Args.h"

#include "icon"
#include "screensaver"

Display *xnestDisplay = NULL;
XVisualInfo *xnestVisuals;
int xnestNumVisuals;
int xnestDefaultVisualIndex;
Colormap *xnestDefaultColormaps;
static uint16_t xnestNumDefaultColormaps;
int *xnestDepths;
int xnestNumDepths;
XPixmapFormatValues *xnestPixmapFormats;
int xnestNumPixmapFormats;
Drawable xnestDefaultDrawables[MAXDEPTH + 1];
Pixmap xnestIconBitmap;
Pixmap xnestScreenSaverPixmap;
uint32_t xnestBitmapGC;
unsigned long xnestEventMask;

static int _X_NORETURN
x_io_error_handler(Display * dpy)
{
    ErrorF("Lost connection to X server: %s\n", strerror(errno));
    CloseWellKnownConnections();
    OsCleanup(1);
    exit(1);
}

void
xnestOpenDisplay(int argc, char *argv[])
{
    XVisualInfo vi;
    long mask;
    int i, j;

    if (!xnestDoFullGeneration)
        return;

    XSetIOErrorHandler(x_io_error_handler);

    xnestCloseDisplay();

    xnestDisplay = XOpenDisplay(xnestDisplayName);
    if (xnestDisplay == NULL)
        FatalError("Unable to open display \"%s\".\n",
                   XDisplayName(xnestDisplayName));

    if (xnestSynchronize)
        XSynchronize(xnestDisplay, TRUE);

    xnest_upstream_setup();

    mask = VisualScreenMask;
    vi.screen = xnestUpstreamInfo.screenId;
    xnestVisuals = XGetVisualInfo(xnestDisplay, mask, &vi, &xnestNumVisuals);
    if (xnestNumVisuals == 0 || xnestVisuals == NULL)
        FatalError("Unable to find any visuals.\n");

    if (xnestUserDefaultClass || xnestUserDefaultDepth) {
        xnestDefaultVisualIndex = UNDEFINED;
        for (i = 0; i < xnestNumVisuals; i++)
            if ((!xnestUserDefaultClass ||
                 xnestVisuals[i].class == xnestDefaultClass)
                &&
                (!xnestUserDefaultDepth ||
                 xnestVisuals[i].depth == xnestDefaultDepth)) {
                xnestDefaultVisualIndex = i;
                break;
            }
        if (xnestDefaultVisualIndex == UNDEFINED)
            FatalError("Unable to find desired default visual.\n");
    }
    else {
        vi.visualid = XVisualIDFromVisual(DefaultVisual(xnestDisplay,
                                                        xnestUpstreamInfo.screenId));
        xnestDefaultVisualIndex = 0;
        for (i = 0; i < xnestNumVisuals; i++)
            if (vi.visualid == xnestVisuals[i].visualid)
                xnestDefaultVisualIndex = i;
    }

    xnestNumDefaultColormaps = xnestNumVisuals;
    xnestDefaultColormaps = calloc(xnestNumDefaultColormaps,
                                        sizeof(Colormap));
    for (i = 0; i < xnestNumDefaultColormaps; i++) {
        xnestDefaultColormaps[i] = xcb_generate_id(xnestUpstreamInfo.conn);
        xcb_create_colormap(xnestUpstreamInfo.conn,
                            XCB_COLORMAP_ALLOC_NONE,
                            xnestDefaultColormaps[i],
                            xnestUpstreamInfo.screenInfo->root,
                            xnestVisuals[i].visual->visualid);
    }

    xnestDepths = XListDepths(xnestDisplay, xnestUpstreamInfo.screenId,
                              &xnestNumDepths);

    xnestPixmapFormats = XListPixmapFormats(xnestDisplay,
                                            &xnestNumPixmapFormats);

    if (xnestParentWindow != (Window) 0)
        xnestEventMask = XCB_EVENT_MASK_STRUCTURE_NOTIFY;
    else
        xnestEventMask = 0L;

    for (i = 0; i <= MAXDEPTH; i++)
        xnestDefaultDrawables[i] = XCB_WINDOW_NONE;

    for (i = 0; i < xnestNumPixmapFormats; i++)
        for (j = 0; j < xnestNumDepths; j++)
            if (xnestPixmapFormats[i].depth == 1 ||
                xnestPixmapFormats[i].depth == xnestDepths[j]) {
                uint32_t pixmap = xcb_generate_id(xnestUpstreamInfo.conn);
                xcb_create_pixmap(xnestUpstreamInfo.conn,
                                  xnestPixmapFormats[i].depth,
                                  pixmap,
                                  xnestUpstreamInfo.screenInfo->root,
                                  1, 1);
                xnestDefaultDrawables[xnestPixmapFormats[i].depth] = pixmap;
            }

    xnestBitmapGC = xcb_generate_id(xnestUpstreamInfo.conn);
    xcb_create_gc(xnestUpstreamInfo.conn,
                  xnestBitmapGC,
                  xnestDefaultDrawables[1],
                  0,
                  NULL);

    if (!(xnestUserGeometry & XValue))
        xnestX = 0;

    if (!(xnestUserGeometry & YValue))
        xnestY = 0;

    if (xnestParentWindow == 0) {
        if (!(xnestUserGeometry & WidthValue))
            xnestWidth = 3 * xnestUpstreamInfo.screenInfo->width_in_pixels / 4;

        if (!(xnestUserGeometry & HeightValue))
            xnestHeight = 3 * xnestUpstreamInfo.screenInfo->height_in_pixels / 4;
    }

    if (!xnestUserBorderWidth)
        xnestBorderWidth = 1;

    xnestIconBitmap =
        xnest_create_bitmap_from_data(xnestUpstreamInfo.conn,
                                      xnestUpstreamInfo.screenInfo->root,
                                      (char *) icon_bits, icon_width, icon_height);

    xnestScreenSaverPixmap =
        xnest_create_pixmap_from_bitmap_data(
                                    xnestUpstreamInfo.conn,
                                    xnestUpstreamInfo.screenInfo->root,
                                    (char *) screensaver_bits,
                                    screensaver_width,
                                    screensaver_height,
                                    xnestUpstreamInfo.screenInfo->white_pixel,
                                    xnestUpstreamInfo.screenInfo->black_pixel,
                                    xnestUpstreamInfo.screenInfo->root_depth);
}

void
xnestCloseDisplay(void)
{
    if (!xnestDoFullGeneration || !xnestDisplay)
        return;

    /*
       If xnestDoFullGeneration all x resources will be destroyed upon closing
       the display connection.  There is no need to generate extra protocol.
     */

    free(xnestDefaultColormaps);
    XFree(xnestVisuals);
    XFree(xnestDepths);
    XFree(xnestPixmapFormats);
    XCloseDisplay(xnestDisplay);
}
