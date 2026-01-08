/************************************************************

Copyright 1989, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

********************************************************/

#include <dix-config.h>

#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/extensions/shapeproto.h>

#include "dix/client_priv.h"
#include "dix/dix_priv.h"
#include "dix/gc_priv.h"
#include "dix/request_priv.h"
#include "dix/rpcbuf_priv.h"
#include "dix/screenint_priv.h"
#include "dix/screen_hooks_priv.h"
#include "dix/window_priv.h"
#include "miext/extinit_priv.h"
#include "Xext/panoramiX.h"
#include "Xext/panoramiXsrv.h"

#include "misc.h"
#include "os.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "extnsionst.h"
#include "dixstruct.h"
#include "opaque.h"
#include "regionstr.h"
#include "gcstruct.h"
#include "protocol-versions.h"

Bool noShapeExtension = FALSE;

typedef RegionPtr (*CreateDftPtr) (WindowPtr    /* pWin */
    );

static DevPrivateKeyRec ShapeWindowPrivateKeyRec;

static void SShapeNotifyEvent(xShapeNotifyEvent * /* from */ ,
                              xShapeNotifyEvent *       /* to */
    );

/* SendShapeNotify, CreateBoundingShape and CreateClipShape are used
 * externally by the Xfixes extension and are now defined in window.h
 */

static int ShapeEventBase = 0;

/*
 * each window has a list of clients requesting
 * ShapeNotify events.  Each client has a resource
 * for each window it selects ShapeNotify input for,
 * this resource is used to delete the ShapeNotifyRec
 * entry from the per-window queue.
 */

typedef struct _ShapeEvent *ShapeEventPtr;

typedef struct _ShapeEvent {
    ShapeEventPtr next;
    ClientPtr client;
    WindowPtr window;
} ShapeEventRec;

#define  SHAPE_WINDOW_PRIVADDR(pWin) ((ShapeEventPtr *) \
dixLookupPrivateAddr(&(pWin)->devPrivates, &ShapeWindowPrivateKeyRec))

static int
ShapeDelClientFromWin(WindowPtr pWin, void *value) {
    ClientPtr client = value;
    ShapeEventPtr *pHead = SHAPE_WINDOW_PRIVADDR(pWin);
    ShapeEventPtr *prev = pHead;
    ShapeEventPtr curr = *pHead;

    while (curr) {
        if (curr->client == client) {
            *prev = curr->next;
            free(curr);
            break;
        }
        prev = &curr->next;
        curr = curr->next;
    }
    return WT_WALKCHILDREN;
}

/****************
 * ShapeExtensionInit
 *
 * Called from InitExtensions in main() or from QueryExtension() if the
 * extension is dynamically loaded.
 *
 ****************/

static int
RegionOperate(ClientPtr client,
              WindowPtr pWin,
              int kind,
              RegionPtr *destRgnp,
              RegionPtr srcRgn, int op, int xoff, int yoff, CreateDftPtr create)
{
    if (srcRgn && (xoff || yoff))
        RegionTranslate(srcRgn, xoff, yoff);
    if (!pWin->parent) {
        if (srcRgn)
            RegionDestroy(srcRgn);
        return Success;
    }

    /* May/30/2001:
     * The shape.PS specs say if src is None, existing shape is to be
     * removed (and so the op-code has no meaning in such removal);
     * see shape.PS, page 3, ShapeMask.
     */
    if (srcRgn == NULL) {
        if (*destRgnp != NULL) {
            RegionDestroy(*destRgnp);
            *destRgnp = 0;
            /* go on to remove shape and generate ShapeNotify */
        }
        else {
            /* May/30/2001:
             * The target currently has no shape in effect, so nothing to
             * do here.  The specs say that ShapeNotify is generated whenever
             * the client region is "modified"; since no modification is done
             * here, we do not generate that event.  The specs does not say
             * "it is an error to request removal when there is no shape in
             * effect", so we return good status.
             */
            return Success;
        }
    }
    else
        switch (op) {
        case ShapeSet:
            if (*destRgnp)
                RegionDestroy(*destRgnp);
            *destRgnp = srcRgn;
            srcRgn = 0;
            break;
        case ShapeUnion:
            if (*destRgnp)
                RegionUnion(*destRgnp, *destRgnp, srcRgn);
            break;
        case ShapeIntersect:
            if (*destRgnp)
                RegionIntersect(*destRgnp, *destRgnp, srcRgn);
            else {
                *destRgnp = srcRgn;
                srcRgn = 0;
            }
            break;
        case ShapeSubtract:
            if (!*destRgnp)
                *destRgnp = (*create) (pWin);
            RegionSubtract(*destRgnp, *destRgnp, srcRgn);
            break;
        case ShapeInvert:
            if (!*destRgnp)
                *destRgnp = RegionCreate((BoxPtr) 0, 0);
            else
                RegionSubtract(*destRgnp, srcRgn, *destRgnp);
            break;
        default:
            client->errorValue = op;
            return BadValue;
        }
    if (srcRgn)
        RegionDestroy(srcRgn);
    (*pWin->drawable.pScreen->SetShape) (pWin, kind);
    SendShapeNotify(pWin, kind);
    return Success;
}

RegionPtr
CreateBoundingShape(WindowPtr pWin)
{
    BoxRec extents;

    extents.x1 = -wBorderWidth(pWin);
    extents.y1 = -wBorderWidth(pWin);
    extents.x2 = pWin->drawable.width + wBorderWidth(pWin);
    extents.y2 = pWin->drawable.height + wBorderWidth(pWin);
    return RegionCreate(&extents, 1);
}

RegionPtr
CreateClipShape(WindowPtr pWin)
{
    BoxRec extents;

    extents.x1 = 0;
    extents.y1 = 0;
    extents.x2 = pWin->drawable.width;
    extents.y2 = pWin->drawable.height;
    return RegionCreate(&extents, 1);
}

static int
ProcShapeQueryVersion(ClientPtr client)
{
    REQUEST_SIZE_MATCH(xShapeQueryVersionReq);

    xShapeQueryVersionReply reply = {
        .majorVersion = SERVER_SHAPE_MAJOR_VERSION,
        .minorVersion = SERVER_SHAPE_MINOR_VERSION
    };

    if (client->swapped) {
        swaps(&reply.majorVersion);
        swaps(&reply.minorVersion);
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
ShapeRectangles(ClientPtr client, xShapeRectanglesReq *stuff)
{
    WindowPtr pWin;
    xRectangle *prects;
    int nrects, ctype, rc;
    RegionPtr srcRgn;
    RegionPtr *destRgn;
    CreateDftPtr createDefault;

    UpdateCurrentTime();
    rc = dixLookupWindow(&pWin, stuff->dest, client, DixSetAttrAccess);
    if (rc != Success)
        return rc;
    switch (stuff->destKind) {
    case ShapeBounding:
        createDefault = CreateBoundingShape;
        break;
    case ShapeClip:
        createDefault = CreateClipShape;
        break;
    case ShapeInput:
        createDefault = CreateBoundingShape;
        break;
    default:
        client->errorValue = stuff->destKind;
        return BadValue;
    }
    if ((stuff->ordering != Unsorted) && (stuff->ordering != YSorted) &&
        (stuff->ordering != YXSorted) && (stuff->ordering != YXBanded)) {
        client->errorValue = stuff->ordering;
        return BadValue;
    }
    nrects = ((client->req_len << 2) - sizeof(xShapeRectanglesReq));
    if (nrects & 4)
        return BadLength;
    nrects >>= 3;
    prects = (xRectangle *) &stuff[1];
    ctype = VerifyRectOrder(nrects, prects, (int) stuff->ordering);
    if (ctype < 0)
        return BadMatch;
    srcRgn = RegionFromRects(nrects, prects, ctype);

    if (!MakeWindowOptional(pWin))
        return BadAlloc;

    switch (stuff->destKind) {
    case ShapeBounding:
        destRgn = &pWin->optional->boundingShape;
        break;
    case ShapeClip:
        destRgn = &pWin->optional->clipShape;
        break;
    case ShapeInput:
        destRgn = &pWin->optional->inputShape;
        break;
    default:
        return BadValue;
    }

    return RegionOperate(client, pWin, (int) stuff->destKind,
                         destRgn, srcRgn, (int) stuff->op,
                         stuff->xOff, stuff->yOff, createDefault);
}

static int
ProcShapeRectangles(ClientPtr client)
{
    REQUEST(xShapeRectanglesReq);
    REQUEST_AT_LEAST_SIZE(xShapeRectanglesReq);

    if (client->swapped) {
        swapl(&stuff->dest);
        swaps(&stuff->xOff);
        swaps(&stuff->yOff);
        SwapRestS(stuff);
    }

#ifdef XINERAMA
    if (noPanoramiXExtension)
        return ShapeRectangles(client, stuff);

    PanoramiXRes *win;
    int result;

    result = dixLookupResourceByType((void **) &win, stuff->dest, XRT_WINDOW,
                                     client, DixWriteAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->dest = win->info[walkScreenIdx].id;
        result = ShapeRectangles(client, stuff);
        if (result != Success)
            break;
    });

    return result;
#else
    return ShapeRectangles(client, stuff);
#endif
}

static int
ShapeMask(ClientPtr client, xShapeMaskReq *stuff)
{
    WindowPtr pWin;
    ScreenPtr pScreen;
    RegionPtr srcRgn;
    RegionPtr *destRgn;
    PixmapPtr pPixmap;
    CreateDftPtr createDefault;
    int rc;

    UpdateCurrentTime();
    rc = dixLookupWindow(&pWin, stuff->dest, client, DixSetAttrAccess);
    if (rc != Success)
        return rc;
    switch (stuff->destKind) {
    case ShapeBounding:
        createDefault = CreateBoundingShape;
        break;
    case ShapeClip:
        createDefault = CreateClipShape;
        break;
    case ShapeInput:
        createDefault = CreateBoundingShape;
        break;
    default:
        client->errorValue = stuff->destKind;
        return BadValue;
    }
    pScreen = pWin->drawable.pScreen;
    if (stuff->src == None)
        srcRgn = 0;
    else {
        rc = dixLookupResourceByType((void **) &pPixmap, stuff->src,
                                     X11_RESTYPE_PIXMAP, client, DixReadAccess);
        if (rc != Success)
            return rc;
        if (pPixmap->drawable.pScreen != pScreen ||
            pPixmap->drawable.depth != 1)
            return BadMatch;
        srcRgn = BitmapToRegion(pScreen, pPixmap);
        if (!srcRgn)
            return BadAlloc;
    }

    if (!MakeWindowOptional(pWin))
        return BadAlloc;

    switch (stuff->destKind) {
    case ShapeBounding:
        destRgn = &pWin->optional->boundingShape;
        break;
    case ShapeClip:
        destRgn = &pWin->optional->clipShape;
        break;
    case ShapeInput:
        destRgn = &pWin->optional->inputShape;
        break;
    default:
        return BadValue;
    }

    return RegionOperate(client, pWin, (int) stuff->destKind,
                         destRgn, srcRgn, (int) stuff->op,
                         stuff->xOff, stuff->yOff, createDefault);
}

static int
ProcShapeMask(ClientPtr client)
{
    REQUEST(xShapeMaskReq);
    REQUEST_SIZE_MATCH(xShapeMaskReq);

    if (client->swapped) {
        swapl(&stuff->dest);
        swaps(&stuff->xOff);
        swaps(&stuff->yOff);
        swapl(&stuff->src);
    }

#ifdef XINERAMA
    if (noPanoramiXExtension)
        return ShapeMask(client, stuff);

    PanoramiXRes *win, *pmap;
    int result;

    result = dixLookupResourceByType((void **) &win, stuff->dest, XRT_WINDOW,
                                     client, DixWriteAccess);
    if (result != Success)
        return result;

    if (stuff->src != None) {
        result = dixLookupResourceByType((void **) &pmap, stuff->src,
                                         XRT_PIXMAP, client, DixReadAccess);
        if (result != Success)
            return result;
    }
    else
        pmap = NULL;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->dest = win->info[walkScreenIdx].id;
        if (pmap)
            stuff->src = pmap->info[walkScreenIdx].id;
        result = ShapeMask(client, stuff);
        if (result != Success)
            break;
    });

    return result;
#else
    return ShapeMask(client, stuff);
#endif
}

static int
ShapeCombine(ClientPtr client, xShapeCombineReq *stuff)
{
    WindowPtr pSrcWin, pDestWin;
    RegionPtr srcRgn;
    RegionPtr *destRgn;
    CreateDftPtr createDefault;
    CreateDftPtr createSrc;
    RegionPtr tmp;
    int rc;

    UpdateCurrentTime();
    rc = dixLookupWindow(&pDestWin, stuff->dest, client, DixSetAttrAccess);
    if (rc != Success)
        return rc;
    if (!MakeWindowOptional(pDestWin))
        return BadAlloc;

    switch (stuff->destKind) {
    case ShapeBounding:
        createDefault = CreateBoundingShape;
        break;
    case ShapeClip:
        createDefault = CreateClipShape;
        break;
    case ShapeInput:
        createDefault = CreateBoundingShape;
        break;
    default:
        client->errorValue = stuff->destKind;
        return BadValue;
    }

    rc = dixLookupWindow(&pSrcWin, stuff->src, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;
    switch (stuff->srcKind) {
    case ShapeBounding:
        srcRgn = wBoundingShape(pSrcWin);
        createSrc = CreateBoundingShape;
        break;
    case ShapeClip:
        srcRgn = wClipShape(pSrcWin);
        createSrc = CreateClipShape;
        break;
    case ShapeInput:
        srcRgn = wInputShape(pSrcWin);
        createSrc = CreateBoundingShape;
        break;
    default:
        client->errorValue = stuff->srcKind;
        return BadValue;
    }
    if (pSrcWin->drawable.pScreen != pDestWin->drawable.pScreen) {
        return BadMatch;
    }

    if (srcRgn) {
        tmp = RegionCreate((BoxPtr) 0, 0);
        RegionCopy(tmp, srcRgn);
        srcRgn = tmp;
    }
    else
        srcRgn = (*createSrc) (pSrcWin);

    if (!MakeWindowOptional(pDestWin))
        return BadAlloc;

    switch (stuff->destKind) {
    case ShapeBounding:
        destRgn = &pDestWin->optional->boundingShape;
        break;
    case ShapeClip:
        destRgn = &pDestWin->optional->clipShape;
        break;
    case ShapeInput:
        destRgn = &pDestWin->optional->inputShape;
        break;
    default:
        return BadValue;
    }

    return RegionOperate(client, pDestWin, (int) stuff->destKind,
                         destRgn, srcRgn, (int) stuff->op,
                         stuff->xOff, stuff->yOff, createDefault);
}

static int
ProcShapeCombine(ClientPtr client)
{
    REQUEST(xShapeCombineReq);
    REQUEST_AT_LEAST_SIZE(xShapeCombineReq);

    if (client->swapped) {
        swapl(&stuff->dest);
        swaps(&stuff->xOff);
        swaps(&stuff->yOff);
        swapl(&stuff->src);
    }

#ifdef XINERAMA
    if (noPanoramiXExtension)
        return ShapeCombine(client, stuff);

    PanoramiXRes *win, *win2;
    int result;

    result = dixLookupResourceByType((void **) &win, stuff->dest, XRT_WINDOW,
                                     client, DixWriteAccess);
    if (result != Success)
        return result;

    result = dixLookupResourceByType((void **) &win2, stuff->src, XRT_WINDOW,
                                     client, DixReadAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->dest = win->info[walkScreenIdx].id;
        stuff->src = win2->info[walkScreenIdx].id;
        result = ShapeCombine(client, stuff);
        if (result != Success)
            break;
    });

    return result;
#else
    return ShapeCombine(client, stuff);
#endif
}

static int
ShapeOffset(ClientPtr client, xShapeOffsetReq *stuff)
{
    WindowPtr pWin;
    RegionPtr srcRgn;
    int rc;

    UpdateCurrentTime();
    rc = dixLookupWindow(&pWin, stuff->dest, client, DixSetAttrAccess);
    if (rc != Success)
        return rc;
    switch (stuff->destKind) {
    case ShapeBounding:
        srcRgn = wBoundingShape(pWin);
        break;
    case ShapeClip:
        srcRgn = wClipShape(pWin);
        break;
    case ShapeInput:
        srcRgn = wInputShape(pWin);
        break;
    default:
        client->errorValue = stuff->destKind;
        return BadValue;
    }
    if (srcRgn) {
        RegionTranslate(srcRgn, stuff->xOff, stuff->yOff);
        (*pWin->drawable.pScreen->SetShape) (pWin, stuff->destKind);
    }
    SendShapeNotify(pWin, (int) stuff->destKind);
    return Success;
}

static int
ProcShapeOffset(ClientPtr client)
{
    REQUEST(xShapeOffsetReq);
    REQUEST_AT_LEAST_SIZE(xShapeOffsetReq);

    if (client->swapped) {
        swapl(&stuff->dest);
        swaps(&stuff->xOff);
        swaps(&stuff->yOff);
    }

#ifdef XINERAMA
    PanoramiXRes *win;
    int result;

    if (noPanoramiXExtension)
        return ShapeOffset(client, stuff);

    result = dixLookupResourceByType((void **) &win, stuff->dest, XRT_WINDOW,
                                     client, DixWriteAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->dest = win->info[walkScreenIdx].id;
        result = ShapeOffset(client, stuff);
        if (result != Success)
            break;
    });

    return result;
#else
    return ShapeOffset(client, stuff);
#endif
}

static int
ProcShapeQueryExtents(ClientPtr client)
{
    REQUEST(xShapeQueryExtentsReq);
    REQUEST_SIZE_MATCH(xShapeQueryExtentsReq);

    if (client->swapped)
        swapl(&stuff->window);

    WindowPtr pWin;
    int rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    RegionPtr boundRegion;
    BoxRec boundBox;
    if ((boundRegion = wBoundingShape(pWin))) {
        /* this is done in two steps because of a compiler bug on SunOS 4.1.3 */
        BoxRec *pExtents = RegionExtents(boundRegion);
        boundBox = *pExtents;
    }
    else {
        boundBox.x1 = -wBorderWidth(pWin);
        boundBox.y1 = -wBorderWidth(pWin);
        boundBox.x2 = pWin->drawable.width + wBorderWidth(pWin);
        boundBox.y2 = pWin->drawable.height + wBorderWidth(pWin);
    }

    RegionPtr shapeRegion;
    BoxRec shapeBox;
    if ((shapeRegion = wClipShape(pWin))) {
        /* this is done in two steps because of a compiler bug on SunOS 4.1.3 */
        BoxRec *pExtents = RegionExtents(shapeRegion);
        shapeBox = *pExtents;
    }
    else {
        shapeBox.x1 = 0;
        shapeBox.y1 = 0;
        shapeBox.x2 = pWin->drawable.width;
        shapeBox.y2 = pWin->drawable.height;
    }

    xShapeQueryExtentsReply reply = {
        .boundingShaped = (wBoundingShape(pWin) != 0),
        .clipShaped = (wClipShape(pWin) != 0),
        .xBoundingShape = boundBox.x1,
        .yBoundingShape = boundBox.y1,
        .widthBoundingShape = boundBox.x2 - boundBox.x1,
        .heightBoundingShape = boundBox.y2 - boundBox.y1,
        .xClipShape = shapeBox.x1,
        .yClipShape = shapeBox.y1,
        .widthClipShape = shapeBox.x2 - shapeBox.x1,
        .heightClipShape = shapeBox.y2 - shapeBox.y1,
    };

    if (client->swapped) {
        swaps(&reply.xBoundingShape);
        swaps(&reply.yBoundingShape);
        swaps(&reply.widthBoundingShape);
        swaps(&reply.heightBoundingShape);
        swaps(&reply.xClipShape);
        swaps(&reply.yClipShape);
        swaps(&reply.widthClipShape);
        swaps(&reply.heightClipShape);
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
ProcShapeSelectInput(ClientPtr client)
{
    REQUEST(xShapeSelectInputReq);
    WindowPtr pWin;
    ShapeEventPtr pNewShapeEvent;
    int rc;

    REQUEST_SIZE_MATCH(xShapeSelectInputReq);

    if (client->swapped)
        swapl(&stuff->window);
    rc = dixLookupWindow(&pWin, stuff->window, client, DixReceiveAccess);
    if (rc != Success)
        return rc;
    ShapeEventPtr pShapeEvent, *pHead = SHAPE_WINDOW_PRIVADDR(pWin);
    switch (stuff->enable) {
    case xTrue:

        /* check for existing entry. */
        for (pShapeEvent = *pHead;
             pShapeEvent; pShapeEvent = pShapeEvent->next) {
            if (pShapeEvent->client == client) {
                return Success;
            }
        }

        /* Form the event */
        pNewShapeEvent = calloc(1, sizeof(ShapeEventRec));
        if (!pNewShapeEvent)
            return BadAlloc;
        pNewShapeEvent->next = *pHead;
        pNewShapeEvent->client = client;
        pNewShapeEvent->window = pWin;
        dixSetPrivate(&pWin->devPrivates, &ShapeWindowPrivateKeyRec, pNewShapeEvent);
        break;
    case xFalse:
        /* remove the events with (client) */
        ShapeDelClientFromWin(pWin,client);
        break;
    default:
        client->errorValue = stuff->enable;
        return BadValue;
    }
    return Success;
}

/*
 * deliver the event
 */

void
SendShapeNotify(WindowPtr pWin, int which)
{
    BoxRec extents;
    RegionPtr region;
    BYTE shaped;

    ShapeEventPtr pShapeEvent, *pHead = SHAPE_WINDOW_PRIVADDR(pWin);

    switch (which) {
    case ShapeBounding:
        region = wBoundingShape(pWin);
        if (region) {
            extents = *RegionExtents(region);
            shaped = xTrue;
        }
        else {
            extents.x1 = -wBorderWidth(pWin);
            extents.y1 = -wBorderWidth(pWin);
            extents.x2 = pWin->drawable.width + wBorderWidth(pWin);
            extents.y2 = pWin->drawable.height + wBorderWidth(pWin);
            shaped = xFalse;
        }
        break;
    case ShapeClip:
        region = wClipShape(pWin);
        if (region) {
            extents = *RegionExtents(region);
            shaped = xTrue;
        }
        else {
            extents.x1 = 0;
            extents.y1 = 0;
            extents.x2 = pWin->drawable.width;
            extents.y2 = pWin->drawable.height;
            shaped = xFalse;
        }
        break;
    case ShapeInput:
        region = wInputShape(pWin);
        if (region) {
            extents = *RegionExtents(region);
            shaped = xTrue;
        }
        else {
            extents.x1 = -wBorderWidth(pWin);
            extents.y1 = -wBorderWidth(pWin);
            extents.x2 = pWin->drawable.width + wBorderWidth(pWin);
            extents.y2 = pWin->drawable.height + wBorderWidth(pWin);
            shaped = xFalse;
        }
        break;
    default:
        return;
    }
    UpdateCurrentTimeIf();
    for (pShapeEvent = *pHead; pShapeEvent; pShapeEvent = pShapeEvent->next) {
        xShapeNotifyEvent se = {
            .type = ShapeNotify + ShapeEventBase,
            .kind = which,
            .window = pWin->drawable.id,
            .x = extents.x1,
            .y = extents.y1,
            .width = extents.x2 - extents.x1,
            .height = extents.y2 - extents.y1,
            .time = currentTime.milliseconds,
            .shaped = shaped
        };
        WriteEventsToClient(pShapeEvent->client, 1, (xEvent *) &se);
    }
}

static int
ProcShapeInputSelected(ClientPtr client)
{
    REQUEST(xShapeInputSelectedReq);
    WindowPtr pWin;
    int enabled, rc;

    REQUEST_SIZE_MATCH(xShapeInputSelectedReq);

    if (client->swapped)
        swapl(&stuff->window);

    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    ShapeEventPtr pShapeEvent, *pHead = SHAPE_WINDOW_PRIVADDR(pWin);
    enabled = xFalse;
    if (pHead) {
        for (pShapeEvent = *pHead; pShapeEvent; pShapeEvent = pShapeEvent->next) {
            if (pShapeEvent->client == client) {
                enabled = xTrue;
                break;
            }
        }
    }

    xShapeInputSelectedReply reply = {
        .enabled = enabled,
    };

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
ProcShapeGetRectangles(ClientPtr client)
{
    REQUEST(xShapeGetRectanglesReq);
    WindowPtr pWin;
    int nrects, rc;
    RegionPtr region;

    REQUEST_SIZE_MATCH(xShapeGetRectanglesReq);

    if (client->swapped)
        swapl(&stuff->window);

    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;
    switch (stuff->kind) {
    case ShapeBounding:
        region = wBoundingShape(pWin);
        break;
    case ShapeClip:
        region = wClipShape(pWin);
        break;
    case ShapeInput:
        region = wInputShape(pWin);
        break;
    default:
        client->errorValue = stuff->kind;
        return BadValue;
    }

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    if (!region) {
        xRectangle rect;
        switch (stuff->kind) {
        case ShapeBounding:
            rect.x = -(int) wBorderWidth(pWin);
            rect.y = -(int) wBorderWidth(pWin);
            rect.width = pWin->drawable.width + wBorderWidth(pWin);
            rect.height = pWin->drawable.height + wBorderWidth(pWin);
            break;
        case ShapeClip:
            rect.x = 0;
            rect.y = 0;
            rect.width = pWin->drawable.width;
            rect.height = pWin->drawable.height;
            break;
        case ShapeInput:
            rect.x = -(int) wBorderWidth(pWin);
            rect.y = -(int) wBorderWidth(pWin);
            rect.width = pWin->drawable.width + wBorderWidth(pWin);
            rect.height = pWin->drawable.height + wBorderWidth(pWin);
            break;
        }
        nrects = 1;
        x_rpcbuf_write_CARD16s(&rpcbuf, (CARD16*)&rect, 4);
    }
    else {
        nrects = RegionNumRects(region);
        BoxPtr boxes = RegionRects(region);
        for (int i=0; i < nrects; i++) {
            xRectangle rect = {
                .x = boxes[i].x1,
                .y = boxes[i].y1,
                .width = boxes[i].x2 - boxes[i].x1,
                .height = boxes[i].y2 - boxes[i].y1,
            };
            x_rpcbuf_write_CARD16s(&rpcbuf, (CARD16*)&rect, 4);
        };
    }

    xShapeGetRectanglesReply reply = {
        .ordering = YXBanded,
        .nrects = nrects
    };

    if (client->swapped) {
        swapl(&reply.nrects);
    }

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

static int
ProcShapeDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
    case X_ShapeQueryVersion:
        return ProcShapeQueryVersion(client);
    case X_ShapeRectangles:
        return ProcShapeRectangles(client);
    case X_ShapeMask:
        return ProcShapeMask(client);
    case X_ShapeCombine:
        return ProcShapeCombine(client);
    case X_ShapeOffset:
        return ProcShapeOffset(client);
    case X_ShapeQueryExtents:
        return ProcShapeQueryExtents(client);
    case X_ShapeSelectInput:
        return ProcShapeSelectInput(client);
    case X_ShapeInputSelected:
        return ProcShapeInputSelected(client);
    case X_ShapeGetRectangles:
        return ProcShapeGetRectangles(client);
    default:
        return BadRequest;
    }
}

static void _X_COLD
SShapeNotifyEvent(xShapeNotifyEvent * from, xShapeNotifyEvent * to)
{
    to->type = from->type;
    to->kind = from->kind;
    cpswapl(from->window, to->window);
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswaps(from->x, to->x);
    cpswaps(from->y, to->y);
    cpswaps(from->width, to->width);
    cpswaps(from->height, to->height);
    cpswapl(from->time, to->time);
    to->shaped = from->shaped;
}

static void
ShapeWindowDestroy(CallbackListPtr *pcbl, ScreenPtr pScreen, WindowPtr pWin)
{
    /* free the events before the window's devPrivates are free'd by destruction */
    ShapeEventPtr pShapeEvent, next;
    ShapeEventPtr *pHead = SHAPE_WINDOW_PRIVADDR(pWin);

    pShapeEvent = *pHead;
    while (pShapeEvent) {
        next = pShapeEvent->next;
        free(pShapeEvent);
        pShapeEvent = next;
    }
    dixSetPrivate(&pWin->devPrivates, &ShapeWindowPrivateKeyRec, NULL);
}

static void
ShapeClientDestroyCallback(CallbackListPtr *pcbl, void *unused, void *calldata)
{
    ClientPtr client = calldata;
    DIX_FOR_EACH_SCREEN({
        WalkTree(walkScreen, ShapeDelClientFromWin, client);
    })
}

void
ShapeExtensionInit(void)
{
    ExtensionEntry *extEntry;

    if (!dixRegisterPrivateKey(&ShapeWindowPrivateKeyRec, PRIVATE_WINDOW, 0))
        return;

    DIX_FOR_EACH_SCREEN({
        dixScreenHookWindowDestroy(walkScreen,ShapeWindowDestroy);
    })

    AddCallback(&ClientDestroyCallback, ShapeClientDestroyCallback, NULL);

    if ((extEntry = AddExtension(SHAPENAME, ShapeNumberEvents, 0,
                                 ProcShapeDispatch, ProcShapeDispatch,
                                 NULL, StandardMinorOpcode))) {
        ShapeEventBase = extEntry->eventBase;
        EventSwapVector[ShapeEventBase] = (EventSwapPtr) SShapeNotifyEvent;
    }
}
