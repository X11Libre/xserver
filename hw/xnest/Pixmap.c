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

#include "regionstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "gc.h"
#include "servermd.h"
#include "privates.h"
#include "mi.h"

#include "xnest-xcb.h"

#include "Display.h"
#include "Screen.h"
#include "XNPixmap.h"

DevPrivateKeyRec xnestPixmapPrivateKeyRec;

PixmapPtr
xnestCreatePixmap(ScreenPtr pScreen, int width, int height, int depth,
                  unsigned usage_hint)
{
    PixmapPtr pPixmap;

    pPixmap = AllocatePixmap(pScreen, 0);
    if (!pPixmap)
        return NullPixmap;
    pPixmap->drawable.type = DRAWABLE_PIXMAP;
    pPixmap->drawable.depth = depth;
    pPixmap->drawable.bitsPerPixel = depth;
    pPixmap->drawable.width = width;
    pPixmap->drawable.height = height;
    pPixmap->drawable.pScreen = pScreen;
    pPixmap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    pPixmap->refcnt = 1;
    pPixmap->devKind = PixmapBytePad(width, depth);
    pPixmap->usage_hint = usage_hint;
    if (width && height) {
        uint32_t pixmap = xcb_generate_id(xnestUpstreamInfo.conn);
        xcb_create_pixmap(xnestUpstreamInfo.conn, depth, pixmap,
                          xnestDefaultWindows[pScreen->myNum], width, height);
        xnestPixmapPriv(pPixmap)->pixmap = pixmap;
    }
    else
        xnestPixmapPriv(pPixmap)->pixmap = 0;

    return pPixmap;
}

Bool
xnestDestroyPixmap(PixmapPtr pPixmap)
{
    if (--pPixmap->refcnt)
        return TRUE;
    xcb_free_pixmap(xnestUpstreamInfo.conn, xnestPixmap(pPixmap));
    FreePixmap(pPixmap);
    return TRUE;
}

Bool
xnestModifyPixmapHeader(PixmapPtr pPixmap, int width, int height, int depth,
                        int bitsPerPixel, int devKind, void *pPixData)
{
  if(!xnestPixmapPriv(pPixmap)->pixmap && width > 0 && height > 0) {
        uint32_t pixmap = xcb_generate_id(xnestUpstreamInfo.conn);
        xcb_create_pixmap(xnestUpstreamInfo.conn, depth, pixmap,
                          xnestDefaultWindows[pPixmap->drawable.pScreen->myNum],
                          width, height);
        xnestPixmapPriv(pPixmap)->pixmap = pixmap;
  }

  return miModifyPixmapHeader(pPixmap, width, height, depth,
                              bitsPerPixel, devKind, pPixData);
}

RegionPtr
xnestPixmapToRegion(PixmapPtr pPixmap)
{
    register RegionPtr pReg, pTmpReg;
    unsigned long previousPixel, currentPixel;
    BoxRec Box = { 0, 0, 0, 0 };
    Bool overlap;

    if (pPixmap->drawable.depth != 1) {
        LogMessage(X_WARNING, "xnestPixmapToRegion() depth != 1: %d\n", pPixmap->drawable.depth);
        return NULL;
    }

    xcb_generic_error_t *err = NULL;
    xcb_get_image_reply_t *reply = xcb_get_image_reply(
        xnestUpstreamInfo.conn,
        xcb_get_image(
            xnestUpstreamInfo.conn,
            XCB_IMAGE_FORMAT_XY_PIXMAP,
            xnestPixmap(pPixmap),
            0,
            0,
            pPixmap->drawable.width,
            pPixmap->drawable.height,
            ~0),
        &err);

    if (err) {
        //  badMatch may happeen if the upstream window is currently minimized
        if (err->error_code != BadMatch)
            ErrorF("xnestGetImage: received error %d\n", err->error_code);
        free(err);
        return NULL;
    }

    if (!reply) {
        ErrorF("xnestGetImage: received no reply\n");
        return NULL;
    }

    pReg = RegionCreate(NULL, 1);
    pTmpReg = RegionCreate(NULL, 1);
    if (!pReg || !pTmpReg) {
        free(reply);
        return NullRegion;
    }

    uint8_t *image_data = xcb_get_image_data(reply);
    for (size_t y = 0; y < pPixmap->drawable.height; y++) {
        Box.y1 = y;
        Box.y2 = y + 1;
        previousPixel = 0L;
        const size_t line_start = BitmapBytePad(pPixmap->drawable.width) * y;

        for (size_t x = 0; x < pPixmap->drawable.width; x++) {
            currentPixel = ((image_data[line_start + (x/8)]) >> (x % 8)) & 1;
            if (previousPixel != currentPixel) {
                if (previousPixel == 0L) {
                    /* left edge */
                    Box.x1 = x;
                }
                else if (currentPixel == 0L) {
                    /* right edge */
                    Box.x2 = x;
                    RegionReset(pTmpReg, &Box);
                    RegionAppend(pReg, pTmpReg);
                }
                previousPixel = currentPixel;
            }
        }
        if (previousPixel != 0L) {
            /* right edge because of the end of pixmap */
            Box.x2 = pPixmap->drawable.width;
            RegionReset(pTmpReg, &Box);
            RegionAppend(pReg, pTmpReg);
        }
    }

    RegionDestroy(pTmpReg);
    free(reply);

    RegionValidate(pReg, &overlap);

    return pReg;
}
