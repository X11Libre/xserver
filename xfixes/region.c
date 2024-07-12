/*
 * Copyright © 2003 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <dix-config.h>

#include "dix/dix_priv.h"
#include "dix/request_priv.h"
#include "dix/rpcbuf_priv.h"
#include "dix/window_priv.h"
#include "render/picturestr_priv.h"
#include "Xext/panoramiX.h"
#include "Xext/panoramiXsrv.h"

#include "xfixesint.h"
#include "scrnintstr.h"

#include <regionstr.h>
#include <gcstruct.h>
#include <window.h>

RESTYPE RegionResType;

static int
RegionResFree(void *data, XID id)
{
    RegionPtr pRegion = (RegionPtr) data;

    RegionDestroy(pRegion);
    return Success;
}

RegionPtr
XFixesRegionCopy(RegionPtr pRegion)
{
    RegionPtr pNew = RegionCreate(RegionExtents(pRegion),
                                  RegionNumRects(pRegion));

    if (!pNew)
        return 0;
    if (!RegionCopy(pNew, pRegion)) {
        RegionDestroy(pNew);
        return 0;
    }
    return pNew;
}

Bool
XFixesRegionInit(void)
{
    RegionResType = CreateNewResourceType(RegionResFree, "XFixesRegion");

    return RegionResType != 0;
}

int
ProcXFixesCreateRegion(ClientPtr client)
{
    X_REQUEST_HEAD_AT_LEAST(xXFixesCreateRegionReq);
    X_REQUEST_FIELD_CARD32(region);
    X_REQUEST_REST_CARD16();

    int things;
    RegionPtr pRegion;

    LEGAL_NEW_RESOURCE(stuff->region, client);

    things = (client->req_len << 2) - sizeof(xXFixesCreateRegionReq);
    if (things & 4)
        return BadLength;
    things >>= 3;

    pRegion = RegionFromRects(things, (xRectangle *) (stuff + 1), CT_UNSORTED);
    if (!pRegion)
        return BadAlloc;
    if (!AddResource(stuff->region, RegionResType, (void *) pRegion))
        return BadAlloc;

    return Success;
}

int
ProcXFixesCreateRegionFromBitmap(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXFixesCreateRegionFromBitmapReq);
    X_REQUEST_FIELD_CARD32(region);
    X_REQUEST_FIELD_CARD32(bitmap);

    RegionPtr pRegion;
    PixmapPtr pPixmap;
    int rc;

    LEGAL_NEW_RESOURCE(stuff->region, client);

    rc = dixLookupResourceByType((void **) &pPixmap, stuff->bitmap, X11_RESTYPE_PIXMAP,
                                 client, DixReadAccess);
    if (rc != Success) {
        client->errorValue = stuff->bitmap;
        return rc;
    }
    if (pPixmap->drawable.depth != 1)
        return BadMatch;

    pRegion = BitmapToRegion(pPixmap->drawable.pScreen, pPixmap);

    if (!pRegion)
        return BadAlloc;

    if (!AddResource(stuff->region, RegionResType, (void *) pRegion))
        return BadAlloc;

    return Success;
}

int
ProcXFixesCreateRegionFromWindow(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXFixesCreateRegionFromWindowReq);
    X_REQUEST_FIELD_CARD32(region);
    X_REQUEST_FIELD_CARD32(window);

    RegionPtr pRegion;
    Bool copy = TRUE;
    WindowPtr pWin;
    int rc;

    LEGAL_NEW_RESOURCE(stuff->region, client);
    rc = dixLookupResourceByType((void **) &pWin, stuff->window, X11_RESTYPE_WINDOW,
                                 client, DixGetAttrAccess);
    if (rc != Success) {
        client->errorValue = stuff->window;
        return rc;
    }
    switch (stuff->kind) {
    case WindowRegionBounding:
        pRegion = wBoundingShape(pWin);
        if (!pRegion) {
            pRegion = CreateBoundingShape(pWin);
            copy = FALSE;
        }
        break;
    case WindowRegionClip:
        pRegion = wClipShape(pWin);
        if (!pRegion) {
            pRegion = CreateClipShape(pWin);
            copy = FALSE;
        }
        break;
    default:
        client->errorValue = stuff->kind;
        return BadValue;
    }
    if (copy && pRegion)
        pRegion = XFixesRegionCopy(pRegion);
    if (!pRegion)
        return BadAlloc;
    if (!AddResource(stuff->region, RegionResType, (void *) pRegion))
        return BadAlloc;

    return Success;
}

int
ProcXFixesCreateRegionFromGC(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXFixesCreateRegionFromGCReq);
    X_REQUEST_FIELD_CARD32(region);
    X_REQUEST_FIELD_CARD32(gc);

    RegionPtr pRegion, pClip;
    GCPtr pGC;
    int rc;

    LEGAL_NEW_RESOURCE(stuff->region, client);

    rc = dixLookupGC(&pGC, stuff->gc, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    if (pGC->clientClip) {
        pClip = (RegionPtr) pGC->clientClip;
        pRegion = XFixesRegionCopy(pClip);
        if (!pRegion)
            return BadAlloc;
    } else {
        return BadMatch;
    }

    if (!AddResource(stuff->region, RegionResType, (void *) pRegion))
        return BadAlloc;

    return Success;
}

int
ProcXFixesCreateRegionFromPicture(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXFixesCreateRegionFromPictureReq);
    X_REQUEST_FIELD_CARD32(region);
    X_REQUEST_FIELD_CARD32(picture);

    RegionPtr pRegion;
    PicturePtr pPicture;

    LEGAL_NEW_RESOURCE(stuff->region, client);

    VERIFY_PICTURE(pPicture, stuff->picture, client, DixGetAttrAccess);

    if (!pPicture->pDrawable)
        return RenderErrBase + BadPicture;

    if (pPicture->clientClip) {
        pRegion = XFixesRegionCopy((RegionPtr) pPicture->clientClip);
        if (!pRegion)
            return BadAlloc;
    } else {
        return BadMatch;
    }

    if (!AddResource(stuff->region, RegionResType, (void *) pRegion))
        return BadAlloc;

    return Success;
}

int
ProcXFixesDestroyRegion(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXFixesDestroyRegionReq);
    X_REQUEST_FIELD_CARD32(region);

    RegionPtr pRegion;

    VERIFY_REGION(pRegion, stuff->region, client, DixWriteAccess);
    FreeResource(stuff->region, X11_RESTYPE_NONE);
    return Success;
}

int
ProcXFixesSetRegion(ClientPtr client)
{
    X_REQUEST_HEAD_AT_LEAST(xXFixesSetRegionReq);
    X_REQUEST_FIELD_CARD32(region);
    X_REQUEST_REST_CARD16();

    int things;
    RegionPtr pRegion, pNew;

    VERIFY_REGION(pRegion, stuff->region, client, DixWriteAccess);

    things = (client->req_len << 2) - sizeof(xXFixesCreateRegionReq);
    if (things & 4)
        return BadLength;
    things >>= 3;

    pNew = RegionFromRects(things, (xRectangle *) (stuff + 1), CT_UNSORTED);
    if (!pNew)
        return BadAlloc;
    if (!RegionCopy(pRegion, pNew)) {
        RegionDestroy(pNew);
        return BadAlloc;
    }
    RegionDestroy(pNew);
    return Success;
}

int
ProcXFixesCopyRegion(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXFixesCopyRegionReq);
    X_REQUEST_FIELD_CARD32(source);
    X_REQUEST_FIELD_CARD32(destination);

    RegionPtr pSource, pDestination;

    VERIFY_REGION(pSource, stuff->source, client, DixReadAccess);
    VERIFY_REGION(pDestination, stuff->destination, client, DixWriteAccess);

    if (!RegionCopy(pDestination, pSource))
        return BadAlloc;

    return Success;
}

int
ProcXFixesCombineRegion(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXFixesCombineRegionReq);
    X_REQUEST_FIELD_CARD32(source1);
    X_REQUEST_FIELD_CARD32(source2);
    X_REQUEST_FIELD_CARD32(destination);

    RegionPtr pSource1, pSource2, pDestination;

    VERIFY_REGION(pSource1, stuff->source1, client, DixReadAccess);
    VERIFY_REGION(pSource2, stuff->source2, client, DixReadAccess);
    VERIFY_REGION(pDestination, stuff->destination, client, DixWriteAccess);

    switch (stuff->xfixesReqType) {
    case X_XFixesUnionRegion:
        if (!RegionUnion(pDestination, pSource1, pSource2))
            return BadAlloc;
        break;
    case X_XFixesIntersectRegion:
        if (!RegionIntersect(pDestination, pSource1, pSource2))
            return BadAlloc;
        break;
    case X_XFixesSubtractRegion:
        if (!RegionSubtract(pDestination, pSource1, pSource2))
            return BadAlloc;
        break;
    }

    return Success;
}

int
ProcXFixesInvertRegion(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXFixesInvertRegionReq);
    X_REQUEST_FIELD_CARD32(source);
    X_REQUEST_FIELD_CARD16(x);
    X_REQUEST_FIELD_CARD16(y);
    X_REQUEST_FIELD_CARD16(width);
    X_REQUEST_FIELD_CARD16(height);
    X_REQUEST_FIELD_CARD32(destination);

    RegionPtr pSource, pDestination;
    BoxRec bounds;

    VERIFY_REGION(pSource, stuff->source, client, DixReadAccess);
    VERIFY_REGION(pDestination, stuff->destination, client, DixWriteAccess);

    /* Compute bounds, limit to 16 bits */
    bounds.x1 = stuff->x;
    bounds.y1 = stuff->y;
    if ((int) stuff->x + (int) stuff->width > MAXSHORT)
        bounds.x2 = MAXSHORT;
    else
        bounds.x2 = stuff->x + stuff->width;

    if ((int) stuff->y + (int) stuff->height > MAXSHORT)
        bounds.y2 = MAXSHORT;
    else
        bounds.y2 = stuff->y + stuff->height;

    if (!RegionInverse(pDestination, pSource, &bounds))
        return BadAlloc;

    return Success;
}

int
ProcXFixesTranslateRegion(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXFixesTranslateRegionReq);
    X_REQUEST_FIELD_CARD32(region);
    X_REQUEST_FIELD_CARD16(dx);
    X_REQUEST_FIELD_CARD16(dy);

    RegionPtr pRegion;

    VERIFY_REGION(pRegion, stuff->region, client, DixWriteAccess);

    RegionTranslate(pRegion, stuff->dx, stuff->dy);
    return Success;
}

int
ProcXFixesRegionExtents(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXFixesRegionExtentsReq);
    X_REQUEST_FIELD_CARD32(source);
    X_REQUEST_FIELD_CARD32(destination);

    RegionPtr pSource, pDestination;

    VERIFY_REGION(pSource, stuff->source, client, DixReadAccess);
    VERIFY_REGION(pDestination, stuff->destination, client, DixWriteAccess);

    RegionReset(pDestination, RegionExtents(pSource));

    return Success;
}

int
ProcXFixesFetchRegion(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXFixesFetchRegionReq);
    X_REQUEST_FIELD_CARD32(region);

    RegionPtr pRegion;
    BoxPtr pExtent;
    BoxPtr pBox;
    int i, nBox;

    VERIFY_REGION(pRegion, stuff->region, client, DixReadAccess);

    pExtent = RegionExtents(pRegion);
    pBox = RegionRects(pRegion);
    nBox = RegionNumRects(pRegion);

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    for (i = 0; i < nBox; i++) {
        x_rpcbuf_write_rect(&rpcbuf,
                            pBox[i].x1,
                            pBox[i].y1,
                            pBox[i].x2 - pBox[i].x1,
                            pBox[i].y2 - pBox[i].y1);
    }

    xXFixesFetchRegionReply reply = {
        .x = pExtent->x1,
        .y = pExtent->y1,
        .width = pExtent->x2 - pExtent->x1,
        .height = pExtent->y2 - pExtent->y1,
    };

    REPLY_FIELD_CARD16(x);
    REPLY_FIELD_CARD16(y);
    REPLY_FIELD_CARD16(width);
    REPLY_FIELD_CARD16(height);

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

#ifdef XINERAMA
static int
PanoramiXFixesSetGCClipRegion(ClientPtr client, xXFixesSetGCClipRegionReq *stuff);
#endif

static int
SingleXFixesSetGCClipRegion(ClientPtr client, xXFixesSetGCClipRegionReq *stuff);

int
ProcXFixesSetGCClipRegion(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXFixesSetGCClipRegionReq);
    X_REQUEST_FIELD_CARD32(gc);
    X_REQUEST_FIELD_CARD32(region);
    X_REQUEST_FIELD_CARD16(xOrigin);
    X_REQUEST_FIELD_CARD16(yOrigin);

#ifdef XINERAMA
    if (XFixesUseXinerama)
        return PanoramiXFixesSetGCClipRegion(client, stuff);
#endif
    return SingleXFixesSetGCClipRegion(client, stuff);
}

static int
SingleXFixesSetGCClipRegion(ClientPtr client, xXFixesSetGCClipRegionReq *stuff)
{
    GCPtr pGC;
    RegionPtr pRegion;
    ChangeGCVal vals[2];
    int rc;

    rc = dixLookupGC(&pGC, stuff->gc, client, DixSetAttrAccess);
    if (rc != Success)
        return rc;

    VERIFY_REGION_OR_NONE(pRegion, stuff->region, client, DixReadAccess);

    if (pRegion) {
        pRegion = XFixesRegionCopy(pRegion);
        if (!pRegion)
            return BadAlloc;
    }

    vals[0].val = stuff->xOrigin;
    vals[1].val = stuff->yOrigin;
    ChangeGC(NULL, pGC, GCClipXOrigin | GCClipYOrigin, vals);
    (*pGC->funcs->ChangeClip) (pGC, pRegion ? CT_REGION : CT_NONE,
                               (void *) pRegion, 0);

    return Success;
}

typedef RegionPtr (*CreateDftPtr) (WindowPtr pWin);

static int
SingleXFixesSetWindowShapeRegion(ClientPtr client, xXFixesSetWindowShapeRegionReq *stuff)
{
    WindowPtr pWin;
    RegionPtr pRegion;
    RegionPtr *pDestRegion;
    int rc;

    rc = dixLookupResourceByType((void **) &pWin, stuff->dest, X11_RESTYPE_WINDOW,
                                 client, DixSetAttrAccess);
    if (rc != Success) {
        client->errorValue = stuff->dest;
        return rc;
    }
    VERIFY_REGION_OR_NONE(pRegion, stuff->region, client, DixWriteAccess);
    switch (stuff->destKind) {
    case ShapeBounding:
    case ShapeClip:
    case ShapeInput:
        break;
    default:
        client->errorValue = stuff->destKind;
        return BadValue;
    }
    if (pRegion) {
        pRegion = XFixesRegionCopy(pRegion);
        if (!pRegion)
            return BadAlloc;
        if (!MakeWindowOptional(pWin))
            return BadAlloc;
        switch (stuff->destKind) {
        default:
        case ShapeBounding:
            pDestRegion = &pWin->optional->boundingShape;
            break;
        case ShapeClip:
            pDestRegion = &pWin->optional->clipShape;
            break;
        case ShapeInput:
            pDestRegion = &pWin->optional->inputShape;
            break;
        }
        if (stuff->xOff || stuff->yOff)
            RegionTranslate(pRegion, stuff->xOff, stuff->yOff);
    }
    else {
        if (pWin->optional) {
            switch (stuff->destKind) {
            default:
            case ShapeBounding:
                pDestRegion = &pWin->optional->boundingShape;
                break;
            case ShapeClip:
                pDestRegion = &pWin->optional->clipShape;
                break;
            case ShapeInput:
                pDestRegion = &pWin->optional->inputShape;
                break;
            }
        }
        else
            pDestRegion = &pRegion;     /* a NULL region pointer */
    }
    if (*pDestRegion)
        RegionDestroy(*pDestRegion);
    *pDestRegion = pRegion;
    (*pWin->drawable.pScreen->SetShape) (pWin, stuff->destKind);
    SendShapeNotify(pWin, stuff->destKind);
    return Success;
}

#ifdef XINERAMA
static int
PanoramiXFixesSetWindowShapeRegion(ClientPtr client, xXFixesSetWindowShapeRegionReq *stuff);
#endif

int
ProcXFixesSetWindowShapeRegion(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXFixesSetWindowShapeRegionReq);
    X_REQUEST_FIELD_CARD32(dest);
    X_REQUEST_FIELD_CARD16(xOff);
    X_REQUEST_FIELD_CARD16(yOff);
    X_REQUEST_FIELD_CARD32(region);

#ifdef XINERAMA
    if (XFixesUseXinerama)
        return PanoramiXFixesSetWindowShapeRegion(client, stuff);
#endif
    return SingleXFixesSetWindowShapeRegion(client, stuff);
}

static int
SingleXFixesSetPictureClipRegion(ClientPtr client, xXFixesSetPictureClipRegionReq *stuff);

#ifdef XINERAMA
static int
PanoramiXFixesSetPictureClipRegion(ClientPtr client, xXFixesSetPictureClipRegionReq *stuff);
#endif

int
ProcXFixesSetPictureClipRegion(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXFixesSetPictureClipRegionReq);
    X_REQUEST_FIELD_CARD32(picture);
    X_REQUEST_FIELD_CARD32(region);
    X_REQUEST_FIELD_CARD16(xOrigin);
    X_REQUEST_FIELD_CARD16(yOrigin);

#ifdef XINERAMA
    if (XFixesUseXinerama)
        return PanoramiXFixesSetPictureClipRegion(client, stuff);
#endif
    return SingleXFixesSetPictureClipRegion(client, stuff);
}

static int
SingleXFixesSetPictureClipRegion(ClientPtr client, xXFixesSetPictureClipRegionReq *stuff)
{
    PicturePtr pPicture;
    RegionPtr pRegion;

    VERIFY_PICTURE(pPicture, stuff->picture, client, DixSetAttrAccess);
    VERIFY_REGION_OR_NONE(pRegion, stuff->region, client, DixReadAccess);

    if (!pPicture->pDrawable)
        return RenderErrBase + BadPicture;

    return SetPictureClipRegion(pPicture, stuff->xOrigin, stuff->yOrigin,
                                pRegion);
}

int
ProcXFixesExpandRegion(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXFixesExpandRegionReq);
    X_REQUEST_FIELD_CARD32(source);
    X_REQUEST_FIELD_CARD32(destination);
    X_REQUEST_FIELD_CARD16(left);
    X_REQUEST_FIELD_CARD16(right);
    X_REQUEST_FIELD_CARD16(top);
    X_REQUEST_FIELD_CARD16(bottom);

    RegionPtr pSource, pDestination;

    BoxPtr pTmp;
    BoxPtr pSrc;
    int nBoxes;
    int i;

    VERIFY_REGION(pSource, stuff->source, client, DixReadAccess);
    VERIFY_REGION(pDestination, stuff->destination, client, DixWriteAccess);

    nBoxes = RegionNumRects(pSource);
    pSrc = RegionRects(pSource);
    if (nBoxes) {
        pTmp = calloc(nBoxes, sizeof(BoxRec));
        if (!pTmp)
            return BadAlloc;
        for (i = 0; i < nBoxes; i++) {
            pTmp[i].x1 = pSrc[i].x1 - stuff->left;
            pTmp[i].x2 = pSrc[i].x2 + stuff->right;
            pTmp[i].y1 = pSrc[i].y1 - stuff->top;
            pTmp[i].y2 = pSrc[i].y2 + stuff->bottom;
        }
        RegionEmpty(pDestination);
        for (i = 0; i < nBoxes; i++) {
            RegionRec r;

            RegionInit(&r, &pTmp[i], 0);
            RegionUnion(pDestination, pDestination, &r);
        }
        free(pTmp);
    }
    return Success;
}

#ifdef XINERAMA

static int
PanoramiXFixesSetGCClipRegion(ClientPtr client, xXFixesSetGCClipRegionReq *stuff)
{
    int result = Success;
    PanoramiXRes *gc;

    if ((result = dixLookupResourceByType((void **) &gc, stuff->gc, XRT_GC,
                                          client, DixWriteAccess))) {
        client->errorValue = stuff->gc;
        return result;
    }

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->gc = gc->info[walkScreenIdx].id;
        result = SingleXFixesSetGCClipRegion(client, stuff);
        if (result != Success)
            break;
    });

    return result;
}

static int
PanoramiXFixesSetWindowShapeRegion(ClientPtr client, xXFixesSetWindowShapeRegionReq *stuff)
{
    int result = Success;
    PanoramiXRes *win;
    RegionPtr reg = NULL;

    if ((result = dixLookupResourceByType((void **) &win, stuff->dest,
                                          XRT_WINDOW, client,
                                          DixWriteAccess))) {
        client->errorValue = stuff->dest;
        return result;
    }

    if (win->u.win.root)
        VERIFY_REGION_OR_NONE(reg, stuff->region, client, DixReadAccess);

    XINERAMA_FOR_EACH_SCREEN_FORWARD({
        stuff->dest = win->info[walkScreenIdx].id;

        if (reg)
            RegionTranslate(reg, -walkScreen->x, -walkScreen->y);

        result = SingleXFixesSetWindowShapeRegion(client, stuff);

        if (reg)
            RegionTranslate(reg, walkScreen->x, walkScreen->y);

        if (result != Success)
            break;
    });

    return result;
}

static int
PanoramiXFixesSetPictureClipRegion(ClientPtr client, xXFixesSetPictureClipRegionReq *stuff)
{
    int result = Success;
    PanoramiXRes *pict;
    RegionPtr reg = NULL;

    if ((result = dixLookupResourceByType((void **) &pict, stuff->picture,
                                          XRT_PICTURE, client,
                                          DixWriteAccess))) {
        client->errorValue = stuff->picture;
        return result;
    }

    if (pict->u.pict.root)
        VERIFY_REGION_OR_NONE(reg, stuff->region, client, DixReadAccess);

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->picture = pict->info[walkScreenIdx].id;

        if (reg)
            RegionTranslate(reg, -walkScreen->x, -walkScreen->y);

        result = SingleXFixesSetPictureClipRegion(client, stuff);

        if (reg)
            RegionTranslate(reg, walkScreen->x, walkScreen->y);

        if (result != Success)
            break;
    });

    return result;
}

#endif /* XINERAMA */
