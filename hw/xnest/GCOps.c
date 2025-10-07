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
#include <X11/fonts/fontstruct.h>

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

#include "regionstr.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "region.h"
#include "servermd.h"

#include "xnest-xcb.h"

#include "Display.h"
#include "Screen.h"
#include "XNGC.h"
#include "XNFont.h"
#include "GCOps.h"
#include "Drawable.h"

void
xnestFillSpans(DrawablePtr pDrawable, GCPtr pGC, int nSpans, xPoint * pPoints,
               int *pWidths, int fSorted)
{
    ErrorF("xnest warning: function xnestFillSpans not implemented\n");
}

void
xnestSetSpans(DrawablePtr pDrawable, GCPtr pGC, char *pSrc,
              xPoint * pPoints, int *pWidths, int nSpans, int fSorted)
{
    ErrorF("xnest warning: function xnestSetSpans not implemented\n");
}

void
xnestGetSpans(DrawablePtr pDrawable, int maxWidth, DDXPointPtr pPoints,
              int *pWidths, int nSpans, char *pBuffer)
{
    ErrorF("xnest warning: function xnestGetSpans not implemented\n");
}

void
xnestQueryBestSize(int class, unsigned short *pWidth, unsigned short *pHeight,
                   ScreenPtr pScreen)
{
    xcb_generic_error_t *err = NULL;
    xcb_query_best_size_reply_t *reply = xcb_query_best_size_reply(
        xnestUpstreamInfo.conn,
        xcb_query_best_size(
            xnestUpstreamInfo.conn,
            class,
            xnestDefaultWindows[pScreen->myNum],
            *pWidth,
            *pHeight),
        &err);

    if (err) {
        ErrorF("QueryBestSize request failed: %d\n", err->error_code);
        free(err);
        return;
    }

    if (!reply) {
        ErrorF("QueryBestSize request failed: no reply\n");
        return;
    }

    *pWidth = reply->width;
    *pHeight = reply->height;
    free(reply);
}

void
xnestPutImage(DrawablePtr pDrawable, GCPtr pGC, int depth, int x, int y,
              int w, int h, int leftPad, int format, char *pImage)
{
    xcb_put_image(xnestUpstreamInfo.conn,
                  format,
                  xnestDrawable(pDrawable),
                  xnest_upstream_gc(pGC),
                  w,
                  h,
                  x,
                  y,
                  leftPad,
                  depth,
                  (format == XCB_IMAGE_FORMAT_Z_PIXMAP ? (unsigned)PixmapBytePad(w, depth)
                                                       : BitmapBytePad((unsigned)(w + leftPad))) * (unsigned)h,
                  (uint8_t*)pImage);
}

void
xnestGetImage(DrawablePtr pDrawable, int x, int y, int w, int h,
              unsigned int format, unsigned long planeMask, char *pImage)
{
    xcb_generic_error_t * err = NULL;
    xcb_get_image_reply_t *reply= xcb_get_image_reply(
        xnestUpstreamInfo.conn,
        xcb_get_image(
            xnestUpstreamInfo.conn,
            format,
            xnestDrawable(pDrawable),
            x, y, w, h, planeMask),
        &err);

    if (err) {
        //  badMatch may happeen if the upstream window is currently minimized
        if (err->error_code != BadMatch)
            LogMessage(X_WARNING, "xnestGetImage: received error %d\n", err->error_code);
        free(err);
        return;
    }

    if (!reply) {
        LogMessage(X_WARNING, "xnestGetImage: received no reply\n");
        return;
    }

    memmove(pImage, xcb_get_image_data(reply), xcb_get_image_data_length(reply));
    free(reply);
}

static RegionPtr
xnestBitBlitHelper(GCPtr pGC)
{
    if (!pGC->graphicsExposures)
        return NullRegion;
    else {
        RegionPtr pReg, pTmpReg;
        Bool pending, overlap;

        pReg = RegionCreate(NULL, 1);
        pTmpReg = RegionCreate(NULL, 1);
        if (!pReg || !pTmpReg)
            return NullRegion;

        xcb_flush(xnestUpstreamInfo.conn);

        pending = TRUE;
        while (pending) {
            xcb_generic_event_t *event = xcb_wait_for_event(xnestUpstreamInfo.conn);
            if (!event) {
                pending = FALSE;
                break;
            }

            switch (event->response_type & ~0x80) {
                case NoExpose:
                    pending = FALSE;
                    free(event);
                    break;

                case GraphicsExpose:
                {
                    xcb_graphics_exposure_event_t* ev = (xcb_graphics_exposure_event_t*)event;
                    BoxRec Box = {
                        .x1 = ev->x,
                        .y1 = ev->y,
                        .x2 = ev->x + ev->width,
                        .y2 = ev->y + ev->height,
                    };
                    RegionReset(pTmpReg, &Box);
                    RegionAppend(pReg, pTmpReg);
                    pending = ev->count;
                    free(event);
                    break;
                }
                default:
                {
                    struct xnest_event_queue *q = malloc(sizeof(struct xnest_event_queue));
                    q->event = event;
                    xorg_list_add(&q->entry, &xnestUpstreamInfo.eventQueue.entry);
                }
            }
        }

        RegionDestroy(pTmpReg);
        RegionValidate(pReg, &overlap);
        return pReg;
    }
}

RegionPtr
xnestCopyArea(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable,
              GCPtr pGC, int srcx, int srcy, int width, int height,
              int dstx, int dsty)
{
    xcb_copy_area(xnestUpstreamInfo.conn,
                  xnestDrawable(pSrcDrawable),
                  xnestDrawable(pDstDrawable),
                  xnest_upstream_gc(pGC),
                  srcx, srcy, dstx, dsty, width, height);

    return xnestBitBlitHelper(pGC);
}

RegionPtr
xnestCopyPlane(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable,
               GCPtr pGC, int srcx, int srcy, int width, int height,
               int dstx, int dsty, unsigned long plane)
{
    xcb_copy_plane(xnestUpstreamInfo.conn,
                   xnestDrawable(pSrcDrawable),
                   xnestDrawable(pDstDrawable),
                   xnest_upstream_gc(pGC),
                   srcx, srcy, dstx, dsty, width, height, plane);

    return xnestBitBlitHelper(pGC);
}

void
xnestPolyPoint(DrawablePtr pDrawable, GCPtr pGC, int mode, int nPoints,
               DDXPointPtr pPoints)
{
    /* xPoint and xcb_segment_t are defined in the same way, both matching
       the protocol layout, so we can directly typecast them */
    xcb_poly_point(xnestUpstreamInfo.conn,
                   mode,
                   xnestDrawable(pDrawable),
                   xnest_upstream_gc(pGC),
                   nPoints,
                   (xcb_point_t*)pPoints);
}

void
xnestPolylines(DrawablePtr pDrawable, GCPtr pGC, int mode, int nPoints,
               DDXPointPtr pPoints)
{
    /* xPoint and xcb_segment_t are defined in the same way, both matching
       the protocol layout, so we can directly typecast them */
    xcb_poly_line(xnestUpstreamInfo.conn,
                  mode,
                  xnestDrawable(pDrawable),
                  xnest_upstream_gc(pGC),
                  nPoints,
                  (xcb_point_t*)pPoints);
}

void
xnestPolySegment(DrawablePtr pDrawable, GCPtr pGC, int nSegments,
                 xSegment * pSegments)
{
    /* xSegment and xcb_segment_t are defined in the same way, both matching
       the protocol layout, so we can directly typecast them */
    xcb_poly_segment(xnestUpstreamInfo.conn,
                     xnestDrawable(pDrawable),
                     xnest_upstream_gc(pGC),
                     nSegments,
                     (xcb_segment_t*)pSegments);
}

void
xnestPolyRectangle(DrawablePtr pDrawable, GCPtr pGC, int nRectangles,
                   xRectangle *pRectangles)
{
    /* xRectangle and xcb_rectangle_t are defined in the same way, both matching
       the protocol layout, so we can directly typecast them */
    xcb_poly_rectangle(xnestUpstreamInfo.conn,
                       xnestDrawable(pDrawable),
                       xnest_upstream_gc(pGC),
                       nRectangles,
                       (xcb_rectangle_t*)pRectangles);
}

void
xnestPolyArc(DrawablePtr pDrawable, GCPtr pGC, int nArcs, xArc * pArcs)
{
    /* xArc and xcb_arc_t are defined in the same way, both matching
       the protocol layout, so we can directly typecast them */
    xcb_poly_arc(xnestUpstreamInfo.conn,
                 xnestDrawable(pDrawable),
                 xnest_upstream_gc(pGC),
                 nArcs,
                 (xcb_arc_t*)pArcs);
}

void
xnestFillPolygon(DrawablePtr pDrawable, GCPtr pGC, int shape, int mode,
                 int nPoints, DDXPointPtr pPoints)
{
    /* xPoint and xcb_segment_t are defined in the same way, both matching
       the protocol layout, so we can directly typecast them */
    xcb_fill_poly(xnestUpstreamInfo.conn,
                  xnestDrawable(pDrawable),
                  xnest_upstream_gc(pGC),
                  shape,
                  mode,
                  nPoints,
                  (xcb_point_t*)pPoints);
}

void
xnestPolyFillRect(DrawablePtr pDrawable, GCPtr pGC, int nRectangles,
                  xRectangle *pRectangles)
{
    /* xRectangle and xcb_rectangle_t are defined in the same way, both matching
       the protocol layout, so we can directly typecast them */
    xcb_poly_fill_rectangle(xnestUpstreamInfo.conn,
                            xnestDrawable(pDrawable),
                            xnest_upstream_gc(pGC),
                            nRectangles,
                            (xcb_rectangle_t*)pRectangles);
}

void
xnestPolyFillArc(DrawablePtr pDrawable, GCPtr pGC, int nArcs, xArc * pArcs)
{
    /* xArc and xcb_arc_t are defined in the same way, both matching
       the protocol layout, so we can directly typecast them */
    xcb_poly_fill_arc(xnestUpstreamInfo.conn,
                      xnestDrawable(pDrawable),
                      xnest_upstream_gc(pGC),
                      nArcs,
                      (xcb_arc_t*)pArcs);
}

int
xnestPolyText8(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int count,
               char *string)
{
    // we need to prepend a xTextElt struct before our actual characters
    // won't get more than 254 elements, since it's already processed by doPolyText()
    int const bufsize = sizeof(xTextElt) + count;
    uint8_t *buffer = malloc(bufsize);
    xTextElt *elt = (xTextElt*)buffer;
    elt->len = count;
    elt->delta = 0;
    memcpy(buffer+2, string, count);

    xcb_poly_text_8(xnestUpstreamInfo.conn,
                    xnestDrawable(pDrawable),
                    xnest_upstream_gc(pGC),
                    x,
                    y,
                    bufsize,
                    (uint8_t*)buffer);

    free(buffer);

    return x + xnest_text_width(xnestFontPriv(pGC->font), string, count);
}

int
xnestPolyText16(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int count,
                unsigned short *string)
{
    // we need to prepend a xTextElt struct before our actual characters
    // won't get more than 254 elements, since it's already processed by doPolyText()
    int const bufsize = sizeof(xTextElt) + count*2;
    uint8_t *buffer = malloc(bufsize);
    xTextElt *elt = (xTextElt*)buffer;
    elt->len = count;
    elt->delta = 0;
    memcpy(buffer+2, string, count*2);

    xcb_poly_text_16(xnestUpstreamInfo.conn,
                     xnestDrawable(pDrawable),
                     xnest_upstream_gc(pGC),
                     x,
                     y,
                     bufsize,
                     buffer);

    free(buffer);

    return x + xnest_text_width_16(xnestFontPriv(pGC->font), string, count);
}

void
xnestImageText8(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int count,
                char *string)
{
    xcb_image_text_8(xnestUpstreamInfo.conn,
                     count,
                     xnestDrawable(pDrawable),
                     xnest_upstream_gc(pGC),
                     x,
                     y,
                     string);
}

void
xnestImageText16(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int count,
                 unsigned short *string)
{
    xcb_image_text_16(xnestUpstreamInfo.conn,
                      count,
                      xnestDrawable(pDrawable),
                      xnest_upstream_gc(pGC),
                      x,
                      y,
                      (xcb_char2b_t*)string);
}

void
xnestImageGlyphBlt(DrawablePtr pDrawable, GCPtr pGC, int x, int y,
                   unsigned int nGlyphs, CharInfoPtr * pCharInfo,
                   void *pGlyphBase)
{
    ErrorF("xnest warning: function xnestImageGlyphBlt not implemented\n");
}

void
xnestPolyGlyphBlt(DrawablePtr pDrawable, GCPtr pGC, int x, int y,
                  unsigned int nGlyphs, CharInfoPtr * pCharInfo,
                  void *pGlyphBase)
{
    ErrorF("xnest warning: function xnestPolyGlyphBlt not implemented\n");
}

void
xnestPushPixels(GCPtr pGC, PixmapPtr pBitmap, DrawablePtr pDst,
                int width, int height, int x, int y)
{
    /* only works for solid bitmaps */
    if (pGC->fillStyle == FillSolid) {
        xcb_params_gc_t params = {
            .fill_style = XCB_FILL_STYLE_STIPPLED,
            .tile_stipple_origin_x = x,
            .tile_stipple_origin_y = y,
            .stipple = xnestPixmap(pBitmap),
        };
        xcb_aux_change_gc(xnestUpstreamInfo.conn,
                          xnest_upstream_gc(pGC),
                          XCB_GC_FILL_STYLE | XCB_GC_TILE_STIPPLE_ORIGIN_X |
                              XCB_GC_TILE_STIPPLE_ORIGIN_Y | XCB_GC_STIPPLE,
                          &params);

        xcb_rectangle_t rect = {
            .x = x, .y = y, .width = width, .height = height,
        };
        xcb_poly_fill_rectangle(xnestUpstreamInfo.conn,
                                xnestDrawable(pDst),
                                xnest_upstream_gc(pGC),
                                1,
                                &rect);

        xcb_aux_change_gc(xnestUpstreamInfo.conn,
                          xnest_upstream_gc(pGC),
                          XCB_GC_FILL_STYLE,
                          &params);
    }
    else
        ErrorF("xnest warning: function xnestPushPixels not implemented\n");
}
