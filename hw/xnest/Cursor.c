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

#include <X11/X.h>
#include <X11/Xdefs.h>
#include <X11/Xproto.h>

#include "screenint.h"
#include "input.h"
#include "misc.h"
#include "cursorstr.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "mipointrst.h"

#include "Xnest.h"
#include "xnest-xcb.h"

#include "Display.h"
#include "Screen.h"
#include "XNCursor.h"
#include "Visual.h"
#include "Keyboard.h"
#include "Args.h"

xnestCursorFuncRec xnestCursorFuncs = { NULL };

Bool
xnestRealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor)
{
    XImage *ximage;
    XColor fg_color, bg_color;
    unsigned long valuemask;
    XGCValues values;

    valuemask = GCFunction |
        GCPlaneMask | GCForeground | GCBackground | GCClipMask;

    values.function = GXcopy;
    values.plane_mask = AllPlanes;
    values.foreground = 1L;
    values.background = 0L;
    values.clip_mask = XCB_PIXMAP_NONE;

    XChangeGC(xnestDisplay, xnestBitmapGC, valuemask, &values);

    uint32_t winId = xnestDefaultWindows[pScreen->myNum];

    Pixmap source = xcb_generate_id(xnestUpstreamInfo.conn);
    xcb_create_pixmap(xnestUpstreamInfo.conn, 1, source, winId, pCursor->bits->width, pCursor->bits->height);

    Pixmap mask = xcb_generate_id(xnestUpstreamInfo.conn);
    xcb_create_pixmap(xnestUpstreamInfo.conn, 1, mask, winId, pCursor->bits->width, pCursor->bits->height);

    ximage = XCreateImage(xnestDisplay,
                          xnestDefaultVisual(pScreen),
                          1, XYBitmap, 0,
                          (char *) pCursor->bits->source,
                          pCursor->bits->width,
                          pCursor->bits->height, BitmapPad(xnestDisplay), 0);

    XPutImage(xnestDisplay, source, xnestBitmapGC, ximage,
              0, 0, 0, 0, pCursor->bits->width, pCursor->bits->height);

    XFree(ximage);

    ximage = XCreateImage(xnestDisplay,
                          xnestDefaultVisual(pScreen),
                          1, XYBitmap, 0,
                          (char *) pCursor->bits->mask,
                          pCursor->bits->width,
                          pCursor->bits->height, BitmapPad(xnestDisplay), 0);

    XPutImage(xnestDisplay, mask, xnestBitmapGC, ximage,
              0, 0, 0, 0, pCursor->bits->width, pCursor->bits->height);

    XFree(ximage);

    fg_color.red = pCursor->foreRed;
    fg_color.green = pCursor->foreGreen;
    fg_color.blue = pCursor->foreBlue;

    bg_color.red = pCursor->backRed;
    bg_color.green = pCursor->backGreen;
    bg_color.blue = pCursor->backBlue;

    xnestSetCursorPriv(pCursor, pScreen, calloc(1, sizeof(xnestPrivCursor)));
    xnestCursor(pCursor, pScreen) =
        XCreatePixmapCursor(xnestDisplay, source, mask, &fg_color, &bg_color,
                            pCursor->bits->xhot, pCursor->bits->yhot);

    xcb_free_pixmap(xnestUpstreamInfo.conn, source);
    xcb_free_pixmap(xnestUpstreamInfo.conn, mask);

    return TRUE;
}

Bool
xnestUnrealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor)
{
    XFreeCursor(xnestDisplay, xnestCursor(pCursor, pScreen));
    free(xnestGetCursorPriv(pCursor, pScreen));
    return TRUE;
}

void
xnestRecolorCursor(ScreenPtr pScreen, CursorPtr pCursor, Bool displayed)
{
    XColor fg_color, bg_color;

    fg_color.red = pCursor->foreRed;
    fg_color.green = pCursor->foreGreen;
    fg_color.blue = pCursor->foreBlue;

    bg_color.red = pCursor->backRed;
    bg_color.green = pCursor->backGreen;
    bg_color.blue = pCursor->backBlue;

    XRecolorCursor(xnestDisplay,
                   xnestCursor(pCursor, pScreen), &fg_color, &bg_color);
}

void
xnestSetCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor, int x,
               int y)
{
    if (pCursor) {
        XDefineCursor(xnestDisplay,
                      xnestDefaultWindows[pScreen->myNum],
                      xnestCursor(pCursor, pScreen));
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
