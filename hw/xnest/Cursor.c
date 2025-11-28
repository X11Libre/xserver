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

#include <stdint.h>

#include <X11/X.h>
#include <X11/Xdefs.h>
#include <X11/Xproto.h>

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

#include "screenint.h"
#include "input.h"
#include "misc.h"
#include "cursorstr.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "mipointrst.h"

#include "xnest-xcb.h"

#include "Display.h"
#include "Screen.h"
#include "XNCursor.h"
#include "Keyboard.h"
#include "Args.h"

xnestCursorFuncRec xnestCursorFuncs = { NULL };

Bool
xnestRealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor)
{
    uint32_t valuemask = XCB_GC_FUNCTION | XCB_GC_PLANE_MASK | XCB_GC_FOREGROUND
                         | XCB_GC_BACKGROUND | XCB_GC_CLIP_MASK;

    xcb_params_gc_t values = {
        .function   = XCB_GX_COPY,
        .plane_mask = ((uint32_t)~0L),
        .foreground = 1L,
    };

    xcb_aux_change_gc(xnestUpstreamInfo.conn, xnestBitmapGC, valuemask, &values);

    uint32_t const winId = xnestDefaultWindows[pScreen->myNum];

    Pixmap const source = xcb_generate_id(xnestUpstreamInfo.conn);
    xcb_create_pixmap(xnestUpstreamInfo.conn, 1, source, winId, pCursor->bits->width, pCursor->bits->height);

    Pixmap const mask = xcb_generate_id(xnestUpstreamInfo.conn);
    xcb_create_pixmap(xnestUpstreamInfo.conn, 1, mask, winId, pCursor->bits->width, pCursor->bits->height);

    size_t const pixmap_len = BitmapBytePad(pCursor->bits->width) * pCursor->bits->height;

    xcb_put_image(xnestUpstreamInfo.conn,
                  XCB_IMAGE_FORMAT_XY_BITMAP,
                  source,
                  xnestBitmapGC,
                  pCursor->bits->width,
                  pCursor->bits->height,
                  0, // x
                  0, // y
                  0, // left_pad
                  1, // depth
                  pixmap_len,
                  (uint8_t*) pCursor->bits->source);

    xcb_put_image(xnestUpstreamInfo.conn,
                  XCB_IMAGE_FORMAT_XY_BITMAP,
                  mask,
                  xnestBitmapGC,
                  pCursor->bits->width,
                  pCursor->bits->height,
                  0, // x
                  0, // y
                  0, // left_pad
                  1, // depth
                  pixmap_len,
                  (uint8_t*) pCursor->bits->mask);

    xnestSetCursorPriv(pCursor, pScreen, calloc(1, sizeof(xnestPrivCursor)));
    uint32_t cursor = xcb_generate_id(xnestUpstreamInfo.conn);
    xcb_create_cursor(xnestUpstreamInfo.conn, cursor, source, mask,
                      pCursor->foreRed, pCursor->foreGreen, pCursor->foreBlue,
                      pCursor->backRed, pCursor->backGreen, pCursor->backBlue,
                      pCursor->bits->xhot, pCursor->bits->yhot);

    xnestCursor(pCursor, pScreen) = cursor;

    xcb_free_pixmap(xnestUpstreamInfo.conn, source);
    xcb_free_pixmap(xnestUpstreamInfo.conn, mask);

    return TRUE;
}

Bool
xnestUnrealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor)
{
    xcb_free_cursor(xnestUpstreamInfo.conn, xnestCursor(pCursor, pScreen));
    free(xnestGetCursorPriv(pCursor, pScreen));
    return TRUE;
}

void
xnestRecolorCursor(ScreenPtr pScreen, CursorPtr pCursor, Bool displayed)
{
    xcb_recolor_cursor(xnestUpstreamInfo.conn,
                       xnestCursor(pCursor, pScreen),
                       pCursor->foreRed,
                       pCursor->foreGreen,
                       pCursor->foreBlue,
                       pCursor->backRed,
                       pCursor->backGreen,
                       pCursor->backBlue);
}

void
xnestSetCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor, int x,
               int y)
{
    if (pCursor) {
        uint32_t cursor = xnestCursor(pCursor, pScreen);

        xcb_change_window_attributes(xnestUpstreamInfo.conn,
                                     xnestDefaultWindows[pScreen->myNum],
                                     XCB_CW_CURSOR,
                                     &cursor);
    }
}

void
xnestMoveCursor(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y)
{
}

Bool
xnestDeviceCursorInitialize(DeviceIntPtr pDev, ScreenPtr pScreen)
{
    xnestCursorFuncPtr pScreenPriv;

    pScreenPriv = (xnestCursorFuncPtr)
        dixLookupPrivate(&pScreen->devPrivates, &xnestScreenCursorFuncKeyRec);

    return pScreenPriv->spriteFuncs->DeviceCursorInitialize(pDev, pScreen);
}

void
xnestDeviceCursorCleanup(DeviceIntPtr pDev, ScreenPtr pScreen)
{
    xnestCursorFuncPtr pScreenPriv;

    pScreenPriv = (xnestCursorFuncPtr)
        dixLookupPrivate(&pScreen->devPrivates, &xnestScreenCursorFuncKeyRec);

    pScreenPriv->spriteFuncs->DeviceCursorCleanup(pDev, pScreen);
}
