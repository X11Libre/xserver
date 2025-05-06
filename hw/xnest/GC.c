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

#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include <X11/fonts/fontstruct.h>
#include "mistruct.h"
#include "region.h"

#include "xnest-xcb.h"

#include "Display.h"
#include "XNGC.h"
#include "GCOps.h"
#include "Drawable.h"
#include "XNFont.h"
#include "Color.h"

DevPrivateKeyRec xnestGCPrivateKeyRec;

static GCFuncs xnestFuncs = {
    xnestValidateGC,
    xnestChangeGC,
    xnestCopyGC,
    xnestDestroyGC,
    xnestChangeClip,
    xnestDestroyClip,
    xnestCopyClip,
};

static GCOps xnestOps = {
    xnestFillSpans,
    xnestSetSpans,
    xnestPutImage,
    xnestCopyArea,
    xnestCopyPlane,
    xnestPolyPoint,
    xnestPolylines,
    xnestPolySegment,
    xnestPolyRectangle,
    xnestPolyArc,
    xnestFillPolygon,
    xnestPolyFillRect,
    xnestPolyFillArc,
    xnestPolyText8,
    xnestPolyText16,
    xnestImageText8,
    xnestImageText16,
    xnestImageGlyphBlt,
    xnestPolyGlyphBlt,
    xnestPushPixels
};

Bool
xnestCreateGC(GCPtr pGC)
{
    pGC->funcs = &xnestFuncs;
    pGC->ops = &xnestOps;

    pGC->miTranslate = 1;

    xnestGCPriv(pGC)->gc = xcb_generate_id(xnestUpstreamInfo.conn);
    xcb_create_gc(xnestUpstreamInfo.conn,
                  xnestGCPriv(pGC)->gc,
                  xnestDefaultDrawables[pGC->depth],
                  0,
                  NULL);

    return TRUE;
}

void
xnestValidateGC(GCPtr pGC, unsigned long changes, DrawablePtr pDrawable)
{
}

void
xnestChangeGC(GCPtr pGC, unsigned long mask)
{
    xcb_params_gc_t values;

    if (mask & GCFunction)
        values.function = pGC->alu;

    if (mask & GCPlaneMask)
        values.plane_mask = pGC->planemask;

    if (mask & GCForeground)
        values.foreground = xnestPixel(pGC->fgPixel);

    if (mask & GCBackground)
        values.background = xnestPixel(pGC->bgPixel);

    if (mask & GCLineWidth)
        values.line_width = pGC->lineWidth;

    if (mask & GCLineStyle)
        values.line_style = pGC->lineStyle;

    if (mask & GCCapStyle)
        values.cap_style = pGC->capStyle;

    if (mask & GCJoinStyle)
        values.join_style = pGC->joinStyle;

    if (mask & GCFillStyle)
        values.fill_style = pGC->fillStyle;

    if (mask & GCFillRule)
        values.fill_rule = pGC->fillRule;

    if (mask & GCTile) {
        if (pGC->tileIsPixel)
            mask &= ~GCTile;
        else
            values.tile = xnestPixmap(pGC->tile.pixmap);
    }

    if (mask & GCStipple)
        values.stipple = xnestPixmap(pGC->stipple);

    if (mask & GCTileStipXOrigin)
        values.tile_stipple_origin_x = pGC->patOrg.x;

    if (mask & GCTileStipYOrigin)
        values.tile_stipple_origin_y = pGC->patOrg.y;

    if (mask & GCFont) {
        assert(xnestFontPriv(pGC->font));
        values.font = xnestFontPriv(pGC->font)->font_id;
    }
    if (mask & GCSubwindowMode)
        values.subwindow_mode = pGC->subWindowMode;

    if (mask & GCGraphicsExposures)
        values.graphics_exposures = pGC->graphicsExposures;

    if (mask & GCClipXOrigin)
        values.clip_originX = pGC->clipOrg.x;

    if (mask & GCClipYOrigin)
        values.clip_originY = pGC->clipOrg.y;

    if (mask & GCClipMask)      /* this is handled in change clip */
        mask &= ~GCClipMask;

    if (mask & GCDashOffset)
        values.dash_offset = pGC->dashOffset;

    if (mask & GCDashList) {
        mask &= ~GCDashList;
        xcb_set_dashes(xnestUpstreamInfo.conn,
                       xnest_upstream_gc(pGC),
                       pGC->dashOffset,
                       pGC->numInDashList,
                       (uint8_t*) pGC->dash);
    }

    if (mask & GCArcMode)
        values.arc_mode = pGC->arcMode;

    if (mask)
        xcb_aux_change_gc(xnestUpstreamInfo.conn,
                          xnest_upstream_gc(pGC),
                          mask,
                          &values);
}

void
xnestCopyGC(GCPtr pGCSrc, unsigned long mask, GCPtr pGCDst)
{
    xcb_copy_gc(xnestUpstreamInfo.conn,
                xnestGC(pGCSrc),
                xnestGC(pGCDst),
                mask);
}

void
xnestDestroyGC(GCPtr pGC)
{
    xcb_free_gc(xnestUpstreamInfo.conn, xnestGC(pGC));
}

void
xnestChangeClip(GCPtr pGC, int type, void *pValue, int nRects)
{
    xnestDestroyClip(pGC);

    switch (type) {
    case CT_NONE:
        {
            uint32_t pixmap = XCB_PIXMAP_NONE;
            xcb_change_gc(xnestUpstreamInfo.conn,
                          xnest_upstream_gc(pGC),
                          XCB_GC_CLIP_MASK,
                          &pixmap);
        }
        pValue = NULL;
        break;

    case CT_REGION:
        {
            nRects = RegionNumRects((RegionPtr) pValue);
            xcb_rectangle_t *rects= calloc(nRects, sizeof(xcb_rectangle_t));
            if (rects == NULL) {
                ErrorF("xnestChangeClip: memory alloc failure");
                return;
            }
            BoxPtr pBox = RegionRects((RegionPtr) pValue);
            for (int i = nRects; i-- > 0;)
                rects[i] = (xcb_rectangle_t) {
                    .x = pBox[i].x1,
                    .y = pBox[i].y1,
                    .width = pBox[i].x2 - pBox[i].x1,
                    .height = pBox[i].y2 - pBox[i].y1,
                };
            xcb_set_clip_rectangles(
                xnestUpstreamInfo.conn,
                XCB_CLIP_ORDERING_UNSORTED,
                xnest_upstream_gc(pGC),
                0,
                0,
                nRects,
                rects);

            free(rects);
        }
        break;

    case CT_PIXMAP:
        {
            uint32_t val = xnestPixmap((PixmapPtr) pValue);
            xcb_change_gc(xnestUpstreamInfo.conn,
                          xnest_upstream_gc(pGC),
                          XCB_GC_CLIP_MASK,
                          &val);
        }
        /*
         * Need to change into region, so subsequent uses are with
         * current pixmap contents.
         */
        pGC->clientClip = (*pGC->pScreen->BitmapToRegion) ((PixmapPtr) pValue);
        dixDestroyPixmap((PixmapPtr) pValue, 0);
        pValue = pGC->clientClip;
        break;

    case CT_UNSORTED:
        xcb_set_clip_rectangles(
            xnestUpstreamInfo.conn,
            XCB_CLIP_ORDERING_UNSORTED,
            xnest_upstream_gc(pGC),
            pGC->clipOrg.x, pGC->clipOrg.y,
            nRects,
            (xcb_rectangle_t*)pValue);
        break;

    case CT_YSORTED:
        xcb_set_clip_rectangles(
            xnestUpstreamInfo.conn,
            XCB_CLIP_ORDERING_Y_SORTED,
            xnest_upstream_gc(pGC),
            pGC->clipOrg.x,
            pGC->clipOrg.y,
            nRects,
            (xcb_rectangle_t*)pValue);
        break;

    case CT_YXSORTED:
        xcb_set_clip_rectangles(
            xnestUpstreamInfo.conn,
            XCB_CLIP_ORDERING_YX_SORTED,
            xnest_upstream_gc(pGC),
            pGC->clipOrg.x,
            pGC->clipOrg.y,
            nRects,
            (xcb_rectangle_t*)pValue);

        break;

    case CT_YXBANDED:
        xcb_set_clip_rectangles(
            xnestUpstreamInfo.conn,
            XCB_CLIP_ORDERING_YX_BANDED,
            xnest_upstream_gc(pGC),
            pGC->clipOrg.x,
            pGC->clipOrg.y,
            nRects,
            (xcb_rectangle_t*)pValue);
        break;
    }

    switch (type) {
    default:
        break;

    case CT_UNSORTED:
    case CT_YSORTED:
    case CT_YXSORTED:
    case CT_YXBANDED:
        /* server clip representation is a region */
        pGC->clientClip = RegionFromRects(nRects, (xRectangle *) pValue, type);
        free(pValue);
        pValue = pGC->clientClip;
        break;
    }

    pGC->clientClip = pValue;
}

void
xnestDestroyClip(GCPtr pGC)
{
    if (pGC->clientClip) {
        RegionDestroy(pGC->clientClip);
        uint32_t val = XCB_PIXMAP_NONE;
        xcb_change_gc(xnestUpstreamInfo.conn,
                      xnest_upstream_gc(pGC),
                      XCB_GC_CLIP_MASK,
                      &val);
        pGC->clientClip = NULL;
    }
}

void
xnestCopyClip(GCPtr pGCDst, GCPtr pGCSrc)
{
    if (pGCSrc->clientClip) {
        RegionPtr pRgn = RegionCreate(NULL, 1);
        RegionCopy(pRgn, pGCSrc->clientClip);
        xnestChangeClip(pGCDst, CT_REGION, pRgn, 0);
    } else {
        xnestDestroyClip(pGCDst);
    }
}
