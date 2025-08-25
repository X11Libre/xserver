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
    int things;
    RegionPtr pRegion;

    REQUEST(xXFixesCreateRegionReq);

    REQUEST_AT_LEAST_SIZE(xXFixesCreateRegionReq);
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

int _X_COLD
SProcXFixesCreateRegion(ClientPtr client)
{
    REQUEST(xXFixesCreateRegionReq);
    REQUEST_AT_LEAST_SIZE(xXFixesCreateRegionReq);
    swapl(&stuff->region);
    SwapRestS(stuff);
    return ProcXFixesCreateRegion(client);
}

int
ProcXFixesCreateRegionFromBitmap(ClientPtr client)
{
    RegionPtr pRegion;
    PixmapPtr pPixmap;
    int rc;

    REQUEST(xXFixesCreateRegionFromBitmapReq);

    REQUEST_SIZE_MATCH(xXFixesCreateRegionFromBitmapReq);
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

int _X_COLD
SProcXFixesCreateRegionFromBitmap(ClientPtr client)
{
    REQUEST(xXFixesCreateRegionFromBitmapReq);
    REQUEST_SIZE_MATCH(xXFixesCreateRegionFromBitmapReq);
    swapl(&stuff->region);
    swapl(&stuff->bitmap);
    return ProcXFixesCreateRegionFromBitmap(client);
}

int
ProcXFixesCreateRegionFromWindow(ClientPtr client)
{
    RegionPtr pRegion;
    Bool copy = TRUE;
    WindowPtr pWin;
    int rc;

    REQUEST(xXFixesCreateRegionFromWindowReq);

    REQUEST_SIZE_MATCH(xXFixesCreateRegionFromWindowReq);
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

int _X_COLD
SProcXFixesCreateRegionFromWindow(ClientPtr client)
{
    REQUEST(xXFixesCreateRegionFromWindowReq);
    REQUEST_SIZE_MATCH(xXFixesCreateRegionFromWindowReq);
    swapl(&stuff->region);
    swapl(&stuff->window);
    return ProcXFixesCreateRegionFromWindow(client);
}

int
ProcXFixesCreateRegionFromGC(ClientPtr client)
{
    RegionPtr pRegion, pClip;
    GCPtr pGC;
    int rc;

    REQUEST(xXFixesCreateRegionFromGCReq);

    REQUEST_SIZE_MATCH(xXFixesCreateRegionFromGCReq);
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

int _X_COLD
SProcXFixesCreateRegionFromGC(ClientPtr client)
{
    REQUEST(xXFixesCreateRegionFromGCReq);
    REQUEST_SIZE_MATCH(xXFixesCreateRegionFromGCReq);
    swapl(&stuff->region);
    swapl(&stuff->gc);
    return ProcXFixesCreateRegionFromGC(client);
}

int
ProcXFixesCreateRegionFromPicture(ClientPtr client)
{
    RegionPtr pRegion;
    PicturePtr pPicture;

    REQUEST(xXFixesCreateRegionFromPictureReq);

    REQUEST_SIZE_MATCH(xXFixesCreateRegionFromPictureReq);
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

int _X_COLD
SProcXFixesCreateRegionFromPicture(ClientPtr client)
{
    REQUEST(xXFixesCreateRegionFromPictureReq);
    REQUEST_SIZE_MATCH(xXFixesCreateRegionFromPictureReq);
    swapl(&stuff->region);
    swapl(&stuff->picture);
    return ProcXFixesCreateRegionFromPicture(client);
}

int
ProcXFixesDestroyRegion(ClientPtr client)
{
    REQUEST(xXFixesDestroyRegionReq);
    RegionPtr pRegion;

    REQUEST_SIZE_MATCH(xXFixesDestroyRegionReq);
    VERIFY_REGION(pRegion, stuff->region, client, DixWriteAccess);
    FreeResource(stuff->region, X11_RESTYPE_NONE);
    return Success;
}

int _X_COLD
SProcXFixesDestroyRegion(ClientPtr client)
{
    REQUEST(xXFixesDestroyRegionReq);
    REQUEST_SIZE_MATCH(xXFixesDestroyRegionReq);
    swapl(&stuff->region);
    return ProcXFixesDestroyRegion(client);
}

int
ProcXFixesSetRegion(ClientPtr client)
{
    int things;
    RegionPtr pRegion, pNew;

    REQUEST(xXFixesSetRegionReq);

    REQUEST_AT_LEAST_SIZE(xXFixesSetRegionReq);
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

int _X_COLD
SProcXFixesSetRegion(ClientPtr client)
{
    REQUEST(xXFixesSetRegionReq);
    REQUEST_AT_LEAST_SIZE(xXFixesSetRegionReq);
    swapl(&stuff->region);
    SwapRestS(stuff);
    return ProcXFixesSetRegion(client);
}

int
ProcXFixesCopyRegion(ClientPtr client)
{
    RegionPtr pSource, pDestination;

    REQUEST(xXFixesCopyRegionReq);
    REQUEST_SIZE_MATCH(xXFixesCopyRegionReq);

    VERIFY_REGION(pSource, stuff->source, client, DixReadAccess);
    VERIFY_REGION(pDestination, stuff->destination, client, DixWriteAccess);

    if (!RegionCopy(pDestination, pSource))
        return BadAlloc;

    return Success;
}

int _X_COLD
SProcXFixesCopyRegion(ClientPtr client)
{
    REQUEST(xXFixesCopyRegionReq);
    REQUEST_SIZE_MATCH(xXFixesCopyRegionReq);
    swapl(&stuff->source);
    swapl(&stuff->destination);
    return ProcXFixesCopyRegion(client);
}

int
ProcXFixesCombineRegion(ClientPtr client)
{
    RegionPtr pSource1, pSource2, pDestination;

    REQUEST(xXFixesCombineRegionReq);

    REQUEST_SIZE_MATCH(xXFixesCombineRegionReq);
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

int _X_COLD
SProcXFixesCombineRegion(ClientPtr client)
{
    REQUEST(xXFixesCombineRegionReq);
    REQUEST_SIZE_MATCH(xXFixesCombineRegionReq);
    swapl(&stuff->source1);
    swapl(&stuff->source2);
    swapl(&stuff->destination);
    return ProcXFixesCombineRegion(client);
}

int
ProcXFixesInvertRegion(ClientPtr client)
{
    RegionPtr pSource, pDestination;
    BoxRec bounds;

    REQUEST(xXFixesInvertRegionReq);

    REQUEST_SIZE_MATCH(xXFixesInvertRegionReq);
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

int _X_COLD
SProcXFixesInvertRegion(ClientPtr client)
{
    REQUEST(xXFixesInvertRegionReq);
    REQUEST_SIZE_MATCH(xXFixesInvertRegionReq);
    swapl(&stuff->source);
    swaps(&stuff->x);
    swaps(&stuff->y);
    swaps(&stuff->width);
    swaps(&stuff->height);
    swapl(&stuff->destination);
    return ProcXFixesInvertRegion(client);
}

int
ProcXFixesTranslateRegion(ClientPtr client)
{
    RegionPtr pRegion;

    REQUEST(xXFixesTranslateRegionReq);

    REQUEST_SIZE_MATCH(xXFixesTranslateRegionReq);
    VERIFY_REGION(pRegion, stuff->region, client, DixWriteAccess);

    RegionTranslate(pRegion, stuff->dx, stuff->dy);
    return Success;
}

int _X_COLD
SProcXFixesTranslateRegion(ClientPtr client)
{
    REQUEST(xXFixesTranslateRegionReq);
    REQUEST_SIZE_MATCH(xXFixesTranslateRegionReq);
    swapl(&stuff->region);
    swaps(&stuff->dx);
    swaps(&stuff->dy);
    return ProcXFixesTranslateRegion(client);
}

int
ProcXFixesRegionExtents(ClientPtr client)
{
    RegionPtr pSource, pDestination;

    REQUEST(xXFixesRegionExtentsReq);

    REQUEST_SIZE_MATCH(xXFixesRegionExtentsReq);
    VERIFY_REGION(pSource, stuff->source, client, DixReadAccess);
    VERIFY_REGION(pDestination, stuff->destination, client, DixWriteAccess);

    RegionReset(pDestination, RegionExtents(pSource));

    return Success;
}

int _X_COLD
SProcXFixesRegionExtents(ClientPtr client)
{
    REQUEST(xXFixesRegionExtentsReq);
    REQUEST_SIZE_MATCH(xXFixesRegionExtentsReq);
    swapl(&stuff->source);
    swapl(&stuff->destination);
    return ProcXFixesRegionExtents(client);
}

int
ProcXFixesFetchRegion(ClientPtr client)
{
    RegionPtr pRegion;
    BoxPtr pExtent;
    BoxPtr pBox;
    int i, nBox;

    REQUEST(xXFixesFetchRegionReq);

    REQUEST_SIZE_MATCH(xXFixesFetchRegionReq);
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

    xXFixesFetchRegionReply rep = {
        .x = pExtent->x1,
        .y = pExtent->y1,
        .width = pExtent->x2 - pExtent->x1,
        .height = pExtent->y2 - pExtent->y1,
    };

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swaps(&rep.x);
        swaps(&rep.y);
        swaps(&rep.width);
        swaps(&rep.height);
    }

    X_SEND_REPLY_WITH_RPCBUF(client, rep, rpcbuf);
    return Success;
}

int _X_COLD
SProcXFixesFetchRegion(ClientPtr client)
{
    REQUEST(xXFixesFetchRegionReq);
    REQUEST_SIZE_MATCH(xXFixesFetchRegionReq);
    swapl(&stuff->region);
    return ProcXFixesFetchRegion(client);
}

static int
PanoramiXFixesSetGCClipRegion(ClientPtr client, xXFixesSetGCClipRegionReq *stuff);

static int
SingleXFixesSetGCClipRegion(ClientPtr client, xXFixesSetGCClipRegionReq *stuff);

int
ProcXFixesSetGCClipRegion(ClientPtr client)
{
    REQUEST(xXFixesSetGCClipRegionReq);
    REQUEST_SIZE_MATCH(xXFixesSetGCClipRegionReq);

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

int _X_COLD
SProcXFixesSetGCClipRegion(ClientPtr client)
{
    REQUEST(xXFixesSetGCClipRegionReq);
    REQUEST_SIZE_MATCH(xXFixesSetGCClipRegionReq);
    swapl(&stuff->gc);
    swapl(&stuff->region);
    swaps(&stuff->xOrigin);
    swaps(&stuff->yOrigin);
    return ProcXFixesSetGCClipRegion(client);
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

static int
PanoramiXFixesSetWindowShapeRegion(ClientPtr client, xXFixesSetWindowShapeRegionReq *stuff);

int
ProcXFixesSetWindowShapeRegion(ClientPtr client)
{
    REQUEST(xXFixesSetWindowShapeRegionReq);
    REQUEST_SIZE_MATCH(xXFixesSetWindowShapeRegionReq);

#ifdef XINERAMA
    if (XFixesUseXinerama)
        return PanoramiXFixesSetWindowShapeRegion(client, stuff);
#endif
    return SingleXFixesSetWindowShapeRegion(client, stuff);
}

int _X_COLD
SProcXFixesSetWindowShapeRegion(ClientPtr client)
{
    REQUEST(xXFixesSetWindowShapeRegionReq);
    REQUEST_SIZE_MATCH(xXFixesSetWindowShapeRegionReq);
    swapl(&stuff->dest);
    swaps(&stuff->xOff);
    swaps(&stuff->yOff);
    swapl(&stuff->region);
    return ProcXFixesSetWindowShapeRegion(client);
}

static int
SingleXFixesSetPictureClipRegion(ClientPtr client, xXFixesSetPictureClipRegionReq *stuff);

static int
PanoramiXFixesSetPictureClipRegion(ClientPtr client, xXFixesSetPictureClipRegionReq *stuff);

int
ProcXFixesSetPictureClipRegion(ClientPtr client)
{
    REQUEST(xXFixesSetPictureClipRegionReq);
    REQUEST_SIZE_MATCH(xXFixesSetPictureClipRegionReq);

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

int _X_COLD
SProcXFixesSetPictureClipRegion(ClientPtr client)
{
    REQUEST(xXFixesSetPictureClipRegionReq);
    REQUEST_SIZE_MATCH(xXFixesSetPictureClipRegionReq);
    swapl(&stuff->picture);
    swapl(&stuff->region);
    swaps(&stuff->xOrigin);
    swaps(&stuff->yOrigin);
    return ProcXFixesSetPictureClipRegion(client);
}

int
ProcXFixesExpandRegion(ClientPtr client)
{
    RegionPtr pSource, pDestination;

    REQUEST(xXFixesExpandRegionReq);
    BoxPtr pTmp;
    BoxPtr pSrc;
    int nBoxes;
    int i;

    REQUEST_SIZE_MATCH(xXFixesExpandRegionReq);
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

int _X_COLD
SProcXFixesExpandRegion(ClientPtr client)
{
    REQUEST(xXFixesExpandRegionReq);
    REQUEST_SIZE_MATCH(xXFixesExpandRegionReq);
    swapl(&stuff->source);
    swapl(&stuff->destination);
    swaps(&stuff->left);
    swaps(&stuff->right);
    swaps(&stuff->top);
    swaps(&stuff->bottom);
    return ProcXFixesExpandRegion(client);
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

    unsigned int walkScreenIdx;
    FOR_NSCREENS_BACKWARD(walkScreenIdx) {
        stuff->gc = gc->info[walkScreenIdx].id;
        result = SingleXFixesSetGCClipRegion(client, stuff);
        if (result != Success)
            break;
    }

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

    unsigned int walkScreenIdx;
    FOR_NSCREENS_FORWARD(walkScreenIdx) {
        ScreenPtr walkScreen = screenInfo.screens[walkScreenIdx];
        stuff->dest = win->info[walkScreenIdx].id;

        if (reg)
            RegionTranslate(reg, -walkScreen->x, -walkScreen->y);

        result = SingleXFixesSetWindowShapeRegion(client, stuff);

        if (reg)
            RegionTranslate(reg, walkScreen->x, walkScreen->y);

        if (result != Success)
            break;
    }

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

    unsigned int walkScreenIdx;
    FOR_NSCREENS_BACKWARD(walkScreenIdx) {
        ScreenPtr walkScreen = screenInfo.screens[walkScreenIdx];
        stuff->picture = pict->info[walkScreenIdx].id;

        if (reg)
            RegionTranslate(reg, -walkScreen->x, -walkScreen->y);

        result = SingleXFixesSetPictureClipRegion(client, stuff);

        if (reg)
            RegionTranslate(reg, walkScreen->x, walkScreen->y);

        if (result != Success)
            break;
    }

    return result;
}

#endif /* XINERAMA */
