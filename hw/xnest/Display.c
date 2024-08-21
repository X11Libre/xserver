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

#include "xnest-xcb.h"

#include "Display.h"
#include "Init.h"
#include "Args.h"

#include "icon"
#include "screensaver"

Colormap *xnestDefaultColormaps;
int xnestNumPixmapFormats;
Drawable xnestDefaultDrawables[MAXDEPTH + 1];
Pixmap xnestIconBitmap;
Pixmap xnestScreenSaverPixmap;
uint32_t xnestBitmapGC;
uint32_t xnestEventMask;

void
xnestOpenDisplay(int argc, char *argv[])
{
    int i;

    if (!xnestDoFullGeneration)
        return;

    xnestCloseDisplay();

    if (!xnest_upstream_setup(xnestDisplayName))
        FatalError("Unable to open display \"%s\".\n", xnestDisplayName);

    if (xnestParentWindow != (Window) 0)
        xnestEventMask = XCB_EVENT_MASK_STRUCTURE_NOTIFY;
    else
        xnestEventMask = 0L;

    for (i = 0; i <= MAXDEPTH; i++)
        xnestDefaultDrawables[i] = XCB_WINDOW_NONE;

    xcb_format_t *fmt = xcb_setup_pixmap_formats(xnestUpstreamInfo.setup);
    const xcb_format_t *fmtend = fmt + xcb_setup_pixmap_formats_length(xnestUpstreamInfo.setup);
    for(; fmt != fmtend; ++fmt) {
        xcb_depth_iterator_t depth_iter;
        for (depth_iter = xcb_screen_allowed_depths_iterator(xnestUpstreamInfo.screenInfo);
             depth_iter.rem;
             xcb_depth_next(&depth_iter))
        {
            if (fmt->depth == 1 || fmt->depth == depth_iter.data->depth) {
                uint32_t pixmap = xcb_generate_id(xnestUpstreamInfo.conn);
                xcb_create_pixmap(xnestUpstreamInfo.conn,
                                  fmt->depth,
                                  pixmap,
                                  xnestUpstreamInfo.screenInfo->root,
                                  1, 1);
                xnestDefaultDrawables[fmt->depth] = pixmap;
            }
        }
    }

    xnestBitmapGC = xcb_generate_id(xnestUpstreamInfo.conn);
    xcb_create_gc(xnestUpstreamInfo.conn,
                  xnestBitmapGC,
                  xnestDefaultDrawables[1],
                  0,
                  NULL);

    if (!(xnestUserGeometry & XCB_CONFIG_WINDOW_X))
        xnestGeometry.x = 0;

    if (!(xnestUserGeometry & XCB_CONFIG_WINDOW_Y))
        xnestGeometry.y = 0;

    if (xnestParentWindow == 0) {
        if (!(xnestUserGeometry & XCB_CONFIG_WINDOW_WIDTH))
            xnestGeometry.width = 3 * xnestUpstreamInfo.screenInfo->width_in_pixels / 4;

        if (!(xnestUserGeometry & XCB_CONFIG_WINDOW_HEIGHT))
            xnestGeometry.height = 3 * xnestUpstreamInfo.screenInfo->height_in_pixels / 4;
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
    if (!xnestDoFullGeneration || !xnestUpstreamInfo.conn)
        return;

    /*
       If xnestDoFullGeneration all x resources will be destroyed upon closing
       the display connection.  There is no need to generate extra protocol.
     */
    free(xnestVisualMap);
    xnestVisualMap = NULL;
    xnestNumVisualMap = 0;

    xcb_disconnect(xnestUpstreamInfo.conn);
    xnestUpstreamInfo.conn = NULL;
    xnestUpstreamInfo.screenInfo = NULL;
    xnestUpstreamInfo.setup = NULL;
}
