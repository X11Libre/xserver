/*****************************************************************
Copyright (c) 1991, 1997 Digital Equipment Corporation, Maynard, Massachusetts.
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR ANY CLAIM, DAMAGES, INCLUDING,
BUT NOT LIMITED TO CONSEQUENTIAL OR INCIDENTAL DAMAGES, OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Digital Equipment Corporation
shall not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from Digital
Equipment Corporation.
******************************************************************/

/* Massively rewritten by Mark Vojkovich <markv@valinux.com> */

#include <dix-config.h>

#include <stdio.h>
#include <X11/X.h>
#include <X11/Xproto.h>

#include "dix/dix_priv.h"
#include "dix/request_priv.h"
#include "dix/rpcbuf_priv.h"
#include "dix/screenint_priv.h"
#include "dix/server_priv.h"
#include "dix/window_priv.h"
#include "os/osdep.h"
#include "os/mathx_priv.h"
#include "Xext/panoramiX.h"
#include "Xext/panoramiXsrv.h"

#include "windowstr.h"
#include "dixfontstr.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#include "opaque.h"
#include "inputstr.h"
#include "migc.h"
#include "dixstruct.h"
#include "resource.h"
#include "panoramiXh.h"

#define XINERAMA_IMAGE_BUFSIZE (256*1024)
#define INPUTONLY_LEGAL_MASK (CWWinGravity | CWEventMask | \
                              CWDontPropagate | CWOverrideRedirect | CWCursor )

int
PanoramiXCreateWindow(ClientPtr client)
{
    PanoramiXRes *parent, *newWin;
    PanoramiXRes *backPix = NULL;
    PanoramiXRes *bordPix = NULL;
    PanoramiXRes *cmap = NULL;

    REQUEST(xCreateWindowReq);
    int pback_offset = 0, pbord_offset = 0, cmap_offset = 0;
    int result, len;
    int orig_x, orig_y;
    XID orig_visual, tmp;

    REQUEST_AT_LEAST_SIZE(xCreateWindowReq);

    len = client->req_len - bytes_to_int32(sizeof(xCreateWindowReq));
    if (Ones(stuff->mask) != len)
        return BadLength;

    result = dixLookupResourceByType((void **) &parent, stuff->parent,
                                     XRT_WINDOW, client, DixWriteAccess);
    if (result != Success)
        return result;

    if (stuff->class == CopyFromParent)
        stuff->class = parent->u.win.class;

    if ((stuff->class == InputOnly) && (stuff->mask & (~INPUTONLY_LEGAL_MASK)))
        return BadMatch;

    if ((Mask) stuff->mask & CWBackPixmap) {
        pback_offset = Ones((Mask) stuff->mask & (CWBackPixmap - 1));
        tmp = *((CARD32 *) &stuff[1] + pback_offset);
        if ((tmp != None) && (tmp != ParentRelative)) {
            result = dixLookupResourceByType((void **) &backPix, tmp,
                                             XRT_PIXMAP, client, DixReadAccess);
            if (result != Success)
                return result;
        }
    }
    if ((Mask) stuff->mask & CWBorderPixmap) {
        pbord_offset = Ones((Mask) stuff->mask & (CWBorderPixmap - 1));
        tmp = *((CARD32 *) &stuff[1] + pbord_offset);
        if (tmp != CopyFromParent) {
            result = dixLookupResourceByType((void **) &bordPix, tmp,
                                             XRT_PIXMAP, client, DixReadAccess);
            if (result != Success)
                return result;
        }
    }
    if ((Mask) stuff->mask & CWColormap) {
        cmap_offset = Ones((Mask) stuff->mask & (CWColormap - 1));
        tmp = *((CARD32 *) &stuff[1] + cmap_offset);
        if (tmp != CopyFromParent) {
            result = dixLookupResourceByType((void **) &cmap, tmp,
                                             XRT_COLORMAP, client,
                                             DixReadAccess);
            if (result != Success)
                return result;
        }
    }

    if (!(newWin = calloc(1, sizeof(PanoramiXRes))))
        return BadAlloc;

    newWin->type = XRT_WINDOW;
    newWin->u.win.visibility = VisibilityNotViewable;
    newWin->u.win.class = stuff->class;
    newWin->u.win.root = FALSE;
    panoramix_setup_ids(newWin, client, stuff->wid);

    if (stuff->class == InputOnly)
        stuff->visual = CopyFromParent;
    orig_visual = stuff->visual;
    orig_x = stuff->x;
    orig_y = stuff->y;

    ScreenPtr masterScreen = dixGetMasterScreen();

    Bool parentIsRoot = (stuff->parent == masterScreen->root->drawable.id)
                     || (stuff->parent == masterScreen->screensaver.wid);

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->wid = newWin->info[walkScreenIdx].id;
        stuff->parent = parent->info[walkScreenIdx].id;
        if (parentIsRoot) {
            stuff->x = orig_x - walkScreen->x;
            stuff->y = orig_y - walkScreen->y;
        }
        if (backPix)
            *((CARD32 *) &stuff[1] + pback_offset) = backPix->info[walkScreenIdx].id;
        if (bordPix)
            *((CARD32 *) &stuff[1] + pbord_offset) = bordPix->info[walkScreenIdx].id;
        if (cmap)
            *((CARD32 *) &stuff[1] + cmap_offset) = cmap->info[walkScreenIdx].id;
        if (orig_visual != CopyFromParent)
            stuff->visual = PanoramiXTranslateVisualID(walkScreenIdx, orig_visual);
        result = DoCreateWindowReq(client, stuff, (XID*)&stuff[1]);
        if (result != Success)
            break;
    });

    if (result == Success)
        AddResource(newWin->info[0].id, XRT_WINDOW, newWin);
    else
        free(newWin);

    return result;
}

int
PanoramiXChangeWindowAttributes(ClientPtr client)
{
    PanoramiXRes *win;
    PanoramiXRes *backPix = NULL;
    PanoramiXRes *bordPix = NULL;
    PanoramiXRes *cmap = NULL;

    REQUEST(xChangeWindowAttributesReq);
    int pback_offset = 0, pbord_offset = 0, cmap_offset = 0;
    int result, len;
    XID tmp;

    REQUEST_AT_LEAST_SIZE(xChangeWindowAttributesReq);

    len = client->req_len - bytes_to_int32(sizeof(xChangeWindowAttributesReq));
    if (Ones(stuff->valueMask) != len)
        return BadLength;

    result = dixLookupResourceByType((void **) &win, stuff->window,
                                     XRT_WINDOW, client, DixWriteAccess);
    if (result != Success)
        return result;

    if ((win->u.win.class == InputOnly) &&
        (stuff->valueMask & (~INPUTONLY_LEGAL_MASK)))
        return BadMatch;

    if ((Mask) stuff->valueMask & CWBackPixmap) {
        pback_offset = Ones((Mask) stuff->valueMask & (CWBackPixmap - 1));
        tmp = *((CARD32 *) &stuff[1] + pback_offset);
        if ((tmp != None) && (tmp != ParentRelative)) {
            result = dixLookupResourceByType((void **) &backPix, tmp,
                                             XRT_PIXMAP, client, DixReadAccess);
            if (result != Success)
                return result;
        }
    }
    if ((Mask) stuff->valueMask & CWBorderPixmap) {
        pbord_offset = Ones((Mask) stuff->valueMask & (CWBorderPixmap - 1));
        tmp = *((CARD32 *) &stuff[1] + pbord_offset);
        if (tmp != CopyFromParent) {
            result = dixLookupResourceByType((void **) &bordPix, tmp,
                                             XRT_PIXMAP, client, DixReadAccess);
            if (result != Success)
                return result;
        }
    }
    if ((Mask) stuff->valueMask & CWColormap) {
        cmap_offset = Ones((Mask) stuff->valueMask & (CWColormap - 1));
        tmp = *((CARD32 *) &stuff[1] + cmap_offset);
        if (tmp != CopyFromParent) {
            result = dixLookupResourceByType((void **) &cmap, tmp,
                                             XRT_COLORMAP, client,
                                             DixReadAccess);
            if (result != Success)
                return result;
        }
    }

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->window = win->info[walkScreenIdx].id;
        if (backPix)
            *((CARD32 *) &stuff[1] + pback_offset) = backPix->info[walkScreenIdx].id;
        if (bordPix)
            *((CARD32 *) &stuff[1] + pbord_offset) = bordPix->info[walkScreenIdx].id;
        if (cmap)
            *((CARD32 *) &stuff[1] + cmap_offset) = cmap->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_ChangeWindowAttributes]) (client);
    });

    return result;
}

int
PanoramiXDestroyWindow(ClientPtr client)
{
    PanoramiXRes *win;
    int result;

    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);

    result = dixLookupResourceByType((void **) &win, stuff->id, XRT_WINDOW,
                                     client, DixDestroyAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->id = win->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_DestroyWindow]) (client);
        if (result != Success)
            break;
    });

    /* Since ProcDestroyWindow is using FreeResource, it will free
       our resource for us on the last pass through the loop above */

    return result;
}

int
PanoramiXDestroySubwindows(ClientPtr client)
{
    PanoramiXRes *win;
    int result;

    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);

    result = dixLookupResourceByType((void **) &win, stuff->id, XRT_WINDOW,
                                     client, DixDestroyAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->id = win->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_DestroySubwindows]) (client);
        if (result != Success)
            break;
    });

    /* DestroySubwindows is using FreeResource which will free
       our resources for us on the last pass through the loop above */

    return result;
}

int
PanoramiXChangeSaveSet(ClientPtr client)
{
    PanoramiXRes *win;
    int result;

    REQUEST(xChangeSaveSetReq);

    REQUEST_SIZE_MATCH(xChangeSaveSetReq);

    result = dixLookupResourceByType((void **) &win, stuff->window,
                                     XRT_WINDOW, client, DixReadAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->window = win->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_ChangeSaveSet]) (client);
        if (result != Success)
            break;
    });

    return result;
}

int
PanoramiXReparentWindow(ClientPtr client)
{
    PanoramiXRes *win, *parent;
    int result;
    int x, y;

    REQUEST(xReparentWindowReq);

    REQUEST_SIZE_MATCH(xReparentWindowReq);

    result = dixLookupResourceByType((void **) &win, stuff->window,
                                     XRT_WINDOW, client, DixWriteAccess);
    if (result != Success)
        return result;

    result = dixLookupResourceByType((void **) &parent, stuff->parent,
                                     XRT_WINDOW, client, DixWriteAccess);
    if (result != Success)
        return result;

    x = stuff->x;
    y = stuff->y;

    ScreenPtr masterScreen = dixGetMasterScreen();

    Bool parentIsRoot = (stuff->parent == masterScreen->root->drawable.id)
                     || (stuff->parent == masterScreen->screensaver.wid);

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->window = win->info[walkScreenIdx].id;
        stuff->parent = parent->info[walkScreenIdx].id;
        if (parentIsRoot) {
            stuff->x = x - walkScreen->x;
            stuff->y = y - walkScreen->y;
        }
        result = (*SavedProcVector[X_ReparentWindow]) (client);
        if (result != Success)
            break;
    });

    return result;
}

int
PanoramiXMapWindow(ClientPtr client)
{
    PanoramiXRes *win;
    int result;

    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);

    result = dixLookupResourceByType((void **) &win, stuff->id,
                                     XRT_WINDOW, client, DixReadAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_FORWARD({
        stuff->id = win->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_MapWindow]) (client);
        if (result != Success)
            break;
    });

    return result;
}

int
PanoramiXMapSubwindows(ClientPtr client)
{
    PanoramiXRes *win;
    int result;

    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);

    result = dixLookupResourceByType((void **) &win, stuff->id,
                                     XRT_WINDOW, client, DixReadAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_FORWARD({
        stuff->id = win->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_MapSubwindows]) (client);
        if (result != Success)
            break;
    });

    return result;
}

int
PanoramiXUnmapWindow(ClientPtr client)
{
    PanoramiXRes *win;
    int result;

    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);

    result = dixLookupResourceByType((void **) &win, stuff->id,
                                     XRT_WINDOW, client, DixReadAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_FORWARD({
        stuff->id = win->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_UnmapWindow]) (client);
        if (result != Success)
            break;
    });

    return result;
}

int
PanoramiXUnmapSubwindows(ClientPtr client)
{
    PanoramiXRes *win;
    int result;

    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);

    result = dixLookupResourceByType((void **) &win, stuff->id,
                                     XRT_WINDOW, client, DixReadAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_FORWARD({
        stuff->id = win->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_UnmapSubwindows]) (client);
        if (result != Success)
            break;
    });

    return result;
}

int
PanoramiXConfigureWindow(ClientPtr client)
{
    PanoramiXRes *win;
    PanoramiXRes *sib = NULL;
    WindowPtr pWin;
    int result, len, sib_offset = 0, x = 0, y = 0;
    int x_offset = -1;
    int y_offset = -1;

    REQUEST(xConfigureWindowReq);

    REQUEST_AT_LEAST_SIZE(xConfigureWindowReq);

    len = client->req_len - bytes_to_int32(sizeof(xConfigureWindowReq));
    if (Ones(stuff->mask) != len)
        return BadLength;

    /* because we need the parent */
    result = dixLookupResourceByType((void **) &pWin, stuff->window,
                                     X11_RESTYPE_WINDOW, client, DixWriteAccess);
    if (result != Success)
        return result;

    result = dixLookupResourceByType((void **) &win, stuff->window,
                                     XRT_WINDOW, client, DixWriteAccess);
    if (result != Success)
        return result;

    if ((Mask) stuff->mask & CWSibling) {
        XID tmp;

        sib_offset = Ones((Mask) stuff->mask & (CWSibling - 1));
        if ((tmp = *((CARD32 *) &stuff[1] + sib_offset))) {
            result = dixLookupResourceByType((void **) &sib, tmp, XRT_WINDOW,
                                             client, DixReadAccess);
            if (result != Success)
                return result;
        }
    }

    ScreenPtr masterScreen = dixGetMasterScreen();

    if (pWin->parent && ((pWin->parent == masterScreen->root) ||
                         (pWin->parent->drawable.id == masterScreen->screensaver.wid))) {
        if ((Mask) stuff->mask & CWX) {
            x_offset = 0;
            x = *((CARD32 *) &stuff[1]);
        }
        if ((Mask) stuff->mask & CWY) {
            y_offset = (x_offset == -1) ? 0 : 1;
            y = *((CARD32 *) &stuff[1] + y_offset);
        }
    }

    /* have to go forward or you get expose events before
       ConfigureNotify events */
    XINERAMA_FOR_EACH_SCREEN_FORWARD({
        stuff->window = win->info[walkScreenIdx].id;
        if (sib)
            *((CARD32 *) &stuff[1] + sib_offset) = sib->info[walkScreenIdx].id;
        if (x_offset >= 0)
            *((CARD32 *) &stuff[1] + x_offset) = x - walkScreen->x;
        if (y_offset >= 0)
            *((CARD32 *) &stuff[1] + y_offset) = y - walkScreen->y;
        result = (*SavedProcVector[X_ConfigureWindow]) (client);
        if (result != Success)
            break;
    });

    return result;
}

int
PanoramiXCirculateWindow(ClientPtr client)
{
    PanoramiXRes *win;
    int result;

    REQUEST(xCirculateWindowReq);

    REQUEST_SIZE_MATCH(xCirculateWindowReq);

    result = dixLookupResourceByType((void **) &win, stuff->window,
                                     XRT_WINDOW, client, DixWriteAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_FORWARD({
        stuff->window = win->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_CirculateWindow]) (client);
        if (result != Success)
            break;
    });

    return result;
}

int
PanoramiXGetGeometry(ClientPtr client)
{
    DrawablePtr pDraw;
    int rc;

    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    rc = dixLookupDrawable(&pDraw, stuff->id, client, M_ANY, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    ScreenPtr masterScreen = dixGetMasterScreen();

    xGetGeometryReply reply = {
        .root = masterScreen->root->drawable.id,
        .depth = pDraw->depth,
        .width = pDraw->width,
        .height = pDraw->height,
        .x = 0,
        .y = 0,
        .borderWidth = 0
    };

    if (stuff->id == reply.root) {
        xWindowRoot *root = (xWindowRoot *)
            (ConnectionInfo + connBlockScreenStart);

        reply.width = root->pixWidth;
        reply.height = root->pixHeight;
    }
    else if (WindowDrawable(pDraw->type)) {
        WindowPtr pWin = (WindowPtr) pDraw;

        reply.x = pWin->origin.x - wBorderWidth(pWin);
        reply.y = pWin->origin.y - wBorderWidth(pWin);
        if ((pWin->parent == masterScreen->root) ||
            (pWin->parent->drawable.id == masterScreen->screensaver.wid)) {
            reply.x += masterScreen->x;
            reply.y += masterScreen->y;
        }
        reply.borderWidth = pWin->borderWidth;
    }

    if (client->swapped) {
        swapl(&reply.root);
        swaps(&reply.x);
        swaps(&reply.y);
        swaps(&reply.width);
        swaps(&reply.height);
        swaps(&reply.borderWidth);
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

int
PanoramiXTranslateCoords(ClientPtr client)
{
    INT16 x, y;

    REQUEST(xTranslateCoordsReq);
    int rc;
    WindowPtr pWin, pDst;

    REQUEST_SIZE_MATCH(xTranslateCoordsReq);
    rc = dixLookupWindow(&pWin, stuff->srcWid, client, DixReadAccess);
    if (rc != Success)
        return rc;
    rc = dixLookupWindow(&pDst, stuff->dstWid, client, DixReadAccess);
    if (rc != Success)
        return rc;

    ScreenPtr masterScreen = dixGetMasterScreen();

    if ((pWin == masterScreen->root) ||
        (pWin->drawable.id == masterScreen->screensaver.wid)) {
        x = stuff->srcX - masterScreen->x;
        y = stuff->srcY - masterScreen->y;
    }
    else {
        x = pWin->drawable.x + stuff->srcX;
        y = pWin->drawable.y + stuff->srcY;
    }
    pWin = pDst->firstChild;

    XID child = None;
    while (pWin) {
        BoxRec box;

        if ((pWin->mapped) &&
            (x >= pWin->drawable.x - wBorderWidth(pWin)) &&
            (x < pWin->drawable.x + (int) pWin->drawable.width +
             wBorderWidth(pWin)) &&
            (y >= pWin->drawable.y - wBorderWidth(pWin)) &&
            (y < pWin->drawable.y + (int) pWin->drawable.height +
             wBorderWidth(pWin))
            /* When a window is shaped, a further check
             * is made to see if the point is inside
             * borderSize
             */
            && (!wBoundingShape(pWin) ||
                RegionContainsPoint(wBoundingShape(pWin),
                                    x - pWin->drawable.x,
                                    y - pWin->drawable.y, &box))
            ) {
            child = pWin->drawable.id;
            pWin = (WindowPtr) NULL;
        }
        else
            pWin = pWin->nextSib;
    }

    INT16 dstX = x - pDst->drawable.x;
    INT16 dstY = y - pDst->drawable.y;
    if ((pDst == masterScreen->root) ||
        (pDst->drawable.id == masterScreen->screensaver.wid)) {
        dstX += masterScreen->x;
        dstY += masterScreen->y;
    }

    xTranslateCoordsReply reply = {
        .sameScreen = xTrue,
        .dstX = dstX,
        .dstY = dstY,
        .child = child
    };

    if (client->swapped) {
        swapl(&reply.child);
        swaps(&reply.dstX);
        swaps(&reply.dstY);
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

int
PanoramiXCreatePixmap(ClientPtr client)
{
    PanoramiXRes *refDraw, *newPix;
    int result;

    REQUEST(xCreatePixmapReq);

    REQUEST_SIZE_MATCH(xCreatePixmapReq);
    client->errorValue = stuff->pid;

    result = dixLookupResourceByClass((void **) &refDraw, stuff->drawable,
                                      XRC_DRAWABLE, client, DixReadAccess);
    if (result != Success)
        return (result == BadValue) ? BadDrawable : result;

    if (!(newPix = calloc(1, sizeof(PanoramiXRes))))
        return BadAlloc;

    newPix->type = XRT_PIXMAP;
    newPix->u.pix.shared = FALSE;
    panoramix_setup_ids(newPix, client, stuff->pid);

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->pid = newPix->info[walkScreenIdx].id;
        stuff->drawable = refDraw->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_CreatePixmap]) (client);
        if (result != Success)
            break;
    });

    if (result == Success)
        AddResource(newPix->info[0].id, XRT_PIXMAP, newPix);
    else
        free(newPix);

    return result;
}

int
PanoramiXFreePixmap(ClientPtr client)
{
    PanoramiXRes *pix;
    int result;

    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);

    client->errorValue = stuff->id;

    result = dixLookupResourceByType((void **) &pix, stuff->id, XRT_PIXMAP,
                                     client, DixDestroyAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->id = pix->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_FreePixmap]) (client);
        if (result != Success)
            break;
    });

    /* Since ProcFreePixmap is using FreeResource, it will free
       our resource for us on the last pass through the loop above */

    return result;
}

int
PanoramiXCreateGC(ClientPtr client)
{
    PanoramiXRes *refDraw;
    PanoramiXRes *newGC;
    PanoramiXRes *stip = NULL;
    PanoramiXRes *tile = NULL;
    PanoramiXRes *clip = NULL;

    REQUEST(xCreateGCReq);
    int tile_offset = 0, stip_offset = 0, clip_offset = 0;
    int result, len;
    XID tmp;

    REQUEST_AT_LEAST_SIZE(xCreateGCReq);

    client->errorValue = stuff->gc;
    len = client->req_len - bytes_to_int32(sizeof(xCreateGCReq));
    if (Ones(stuff->mask) != len)
        return BadLength;

    result = dixLookupResourceByClass((void **) &refDraw, stuff->drawable,
                                      XRC_DRAWABLE, client, DixReadAccess);
    if (result != Success)
        return (result == BadValue) ? BadDrawable : result;

    if ((Mask) stuff->mask & GCTile) {
        tile_offset = Ones((Mask) stuff->mask & (GCTile - 1));
        if ((tmp = *((CARD32 *) &stuff[1] + tile_offset))) {
            result = dixLookupResourceByType((void **) &tile, tmp, XRT_PIXMAP,
                                             client, DixReadAccess);
            if (result != Success)
                return result;
        }
    }
    if ((Mask) stuff->mask & GCStipple) {
        stip_offset = Ones((Mask) stuff->mask & (GCStipple - 1));
        if ((tmp = *((CARD32 *) &stuff[1] + stip_offset))) {
            result = dixLookupResourceByType((void **) &stip, tmp, XRT_PIXMAP,
                                             client, DixReadAccess);
            if (result != Success)
                return result;
        }
    }
    if ((Mask) stuff->mask & GCClipMask) {
        clip_offset = Ones((Mask) stuff->mask & (GCClipMask - 1));
        if ((tmp = *((CARD32 *) &stuff[1] + clip_offset))) {
            result = dixLookupResourceByType((void **) &clip, tmp, XRT_PIXMAP,
                                             client, DixReadAccess);
            if (result != Success)
                return result;
        }
    }

    if (!(newGC = calloc(1, sizeof(PanoramiXRes))))
        return BadAlloc;

    newGC->type = XRT_GC;
    panoramix_setup_ids(newGC, client, stuff->gc);

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->gc = newGC->info[walkScreenIdx].id;
        stuff->drawable = refDraw->info[walkScreenIdx].id;
        if (tile)
            *((CARD32 *) &stuff[1] + tile_offset) = tile->info[walkScreenIdx].id;
        if (stip)
            *((CARD32 *) &stuff[1] + stip_offset) = stip->info[walkScreenIdx].id;
        if (clip)
            *((CARD32 *) &stuff[1] + clip_offset) = clip->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_CreateGC]) (client);
        if (result != Success)
            break;
    });

    if (result == Success)
        AddResource(newGC->info[0].id, XRT_GC, newGC);
    else
        free(newGC);

    return result;
}

int
PanoramiXChangeGC(ClientPtr client)
{
    PanoramiXRes *gc;
    PanoramiXRes *stip = NULL;
    PanoramiXRes *tile = NULL;
    PanoramiXRes *clip = NULL;

    REQUEST(xChangeGCReq);
    int tile_offset = 0, stip_offset = 0, clip_offset = 0;
    int result, len;
    XID tmp;

    REQUEST_AT_LEAST_SIZE(xChangeGCReq);

    len = client->req_len - bytes_to_int32(sizeof(xChangeGCReq));
    if (Ones(stuff->mask) != len)
        return BadLength;

    result = dixLookupResourceByType((void **) &gc, stuff->gc, XRT_GC,
                                     client, DixReadAccess);
    if (result != Success)
        return result;

    if ((Mask) stuff->mask & GCTile) {
        tile_offset = Ones((Mask) stuff->mask & (GCTile - 1));
        if ((tmp = *((CARD32 *) &stuff[1] + tile_offset))) {
            result = dixLookupResourceByType((void **) &tile, tmp, XRT_PIXMAP,
                                             client, DixReadAccess);
            if (result != Success)
                return result;
        }
    }
    if ((Mask) stuff->mask & GCStipple) {
        stip_offset = Ones((Mask) stuff->mask & (GCStipple - 1));
        if ((tmp = *((CARD32 *) &stuff[1] + stip_offset))) {
            result = dixLookupResourceByType((void **) &stip, tmp, XRT_PIXMAP,
                                             client, DixReadAccess);
            if (result != Success)
                return result;
        }
    }
    if ((Mask) stuff->mask & GCClipMask) {
        clip_offset = Ones((Mask) stuff->mask & (GCClipMask - 1));
        if ((tmp = *((CARD32 *) &stuff[1] + clip_offset))) {
            result = dixLookupResourceByType((void **) &clip, tmp, XRT_PIXMAP,
                                             client, DixReadAccess);
            if (result != Success)
                return result;
        }
    }

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->gc = gc->info[walkScreenIdx].id;
        if (tile)
            *((CARD32 *) &stuff[1] + tile_offset) = tile->info[walkScreenIdx].id;
        if (stip)
            *((CARD32 *) &stuff[1] + stip_offset) = stip->info[walkScreenIdx].id;
        if (clip)
            *((CARD32 *) &stuff[1] + clip_offset) = clip->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_ChangeGC]) (client);
        if (result != Success)
            break;
    });

    return result;
}

int
PanoramiXCopyGC(ClientPtr client)
{
    PanoramiXRes *srcGC, *dstGC;
    int result;

    REQUEST(xCopyGCReq);

    REQUEST_SIZE_MATCH(xCopyGCReq);

    result = dixLookupResourceByType((void **) &srcGC, stuff->srcGC, XRT_GC,
                                     client, DixReadAccess);
    if (result != Success)
        return result;

    result = dixLookupResourceByType((void **) &dstGC, stuff->dstGC, XRT_GC,
                                     client, DixWriteAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->srcGC = srcGC->info[walkScreenIdx].id;
        stuff->dstGC = dstGC->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_CopyGC]) (client);
        if (result != Success)
            break;
    });

    return result;
}

int
PanoramiXSetDashes(ClientPtr client)
{
    PanoramiXRes *gc;
    int result;

    REQUEST(xSetDashesReq);

    REQUEST_FIXED_SIZE(xSetDashesReq, stuff->nDashes);

    result = dixLookupResourceByType((void **) &gc, stuff->gc, XRT_GC,
                                     client, DixWriteAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->gc = gc->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_SetDashes]) (client);
        if (result != Success)
            break;
    });

    return result;
}

int
PanoramiXSetClipRectangles(ClientPtr client)
{
    PanoramiXRes *gc;
    int result;

    REQUEST(xSetClipRectanglesReq);

    REQUEST_AT_LEAST_SIZE(xSetClipRectanglesReq);

    result = dixLookupResourceByType((void **) &gc, stuff->gc, XRT_GC,
                                     client, DixWriteAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->gc = gc->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_SetClipRectangles]) (client);
        if (result != Success)
            break;
    });

    return result;
}

int
PanoramiXFreeGC(ClientPtr client)
{
    PanoramiXRes *gc;
    int result;

    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);

    result = dixLookupResourceByType((void **) &gc, stuff->id, XRT_GC,
                                     client, DixDestroyAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->id = gc->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_FreeGC]) (client);
        if (result != Success)
            break;
    });

    /* Since ProcFreeGC is using FreeResource, it will free
       our resource for us on the last pass through the loop above */

    return result;
}

int
PanoramiXClearToBackground(ClientPtr client)
{
    PanoramiXRes *win;
    int result, x, y;
    Bool isRoot;

    REQUEST(xClearAreaReq);

    REQUEST_SIZE_MATCH(xClearAreaReq);

    result = dixLookupResourceByType((void **) &win, stuff->window,
                                     XRT_WINDOW, client, DixWriteAccess);
    if (result != Success)
        return result;

    x = stuff->x;
    y = stuff->y;
    isRoot = win->u.win.root;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->window = win->info[walkScreenIdx].id;
        if (isRoot) {
            stuff->x = x - walkScreen->x;
            stuff->y = y - walkScreen->y;
        }
        result = (*SavedProcVector[X_ClearArea]) (client);
        if (result != Success)
            break;
    });

    return result;
}

/*
    For Window to Pixmap copies you're screwed since each screen's
    pixmap will look like what it sees on its screen.  Unless the
    screens overlap and the window lies on each, the two copies
    will be out of sync.  To remedy this we do a GetImage and PutImage
    in place of the copy.  Doing this as a single Image isn't quite
    correct since it will include the obscured areas but we will
    have to fix this later. (MArk).
*/

int
PanoramiXCopyArea(ClientPtr client)
{
    int result, srcx, srcy, dstx, dsty, width, height;
    PanoramiXRes *gc, *src, *dst;
    Bool srcIsRoot = FALSE;
    Bool dstIsRoot = FALSE;
    Bool srcShared, dstShared;

    REQUEST(xCopyAreaReq);

    REQUEST_SIZE_MATCH(xCopyAreaReq);

    result = dixLookupResourceByClass((void **) &src, stuff->srcDrawable,
                                      XRC_DRAWABLE, client, DixReadAccess);
    if (result != Success)
        return (result == BadValue) ? BadDrawable : result;

    srcShared = IS_SHARED_PIXMAP(src);

    result = dixLookupResourceByClass((void **) &dst, stuff->dstDrawable,
                                      XRC_DRAWABLE, client, DixWriteAccess);
    if (result != Success)
        return (result == BadValue) ? BadDrawable : result;

    dstShared = IS_SHARED_PIXMAP(dst);

    if (dstShared && srcShared)
        return (*SavedProcVector[X_CopyArea]) (client);

    result = dixLookupResourceByType((void **) &gc, stuff->gc, XRT_GC,
                                     client, DixReadAccess);
    if (result != Success)
        return result;

    if ((dst->type == XRT_WINDOW) && dst->u.win.root)
        dstIsRoot = TRUE;
    if ((src->type == XRT_WINDOW) && src->u.win.root)
        srcIsRoot = TRUE;

    srcx = stuff->srcX;
    srcy = stuff->srcY;
    dstx = stuff->dstX;
    dsty = stuff->dstY;
    width = stuff->width;
    height = stuff->height;
    if ((dst->type == XRT_PIXMAP) && (src->type == XRT_WINDOW)) {
        DrawablePtr drawables[MAXSCREENS];
        DrawablePtr pDst;
        GCPtr pGC = NULL;
        char *data;
        int pitch, rc;

        XINERAMA_FOR_EACH_SCREEN_BACKWARD({
            rc = dixLookupDrawable(drawables + walkScreenIdx, src->info[walkScreenIdx].id, client, 0,
                                   DixGetAttrAccess);
            if (rc != Success)
                return rc;
            drawables[walkScreenIdx]->pScreen->SourceValidate(drawables[walkScreenIdx], 0, 0,
                                                  drawables[walkScreenIdx]->width,
                                                  drawables[walkScreenIdx]->height,
                                                  IncludeInferiors);
        });

        pitch = PixmapBytePad(width, drawables[0]->depth);
        if (!(data = calloc(height, pitch)))
            return BadAlloc;

        XineramaGetImageData(drawables, srcx, srcy, width, height, ZPixmap, ~0,
                             data, pitch, srcIsRoot);

        XINERAMA_FOR_EACH_SCREEN_BACKWARD({
            stuff->gc = gc->info[walkScreenIdx].id;
            VALIDATE_DRAWABLE_AND_GC(dst->info[walkScreenIdx].id, pDst, DixWriteAccess);
            if (drawables[0]->depth != pDst->depth) {
                client->errorValue = stuff->dstDrawable;
                free(data);
                return BadMatch;
            }

            (*pGC->ops->PutImage) (pDst, pGC, pDst->depth, dstx, dsty,
                                   width, height, 0, ZPixmap, data);
            if (dstShared)
                break;
        });
        free(data);

        if (pGC && pGC->graphicsExposures) {
            RegionRec rgn;
            int dx, dy;
            BoxRec sourceBox;

            dx = drawables[0]->x;
            dy = drawables[0]->y;
            if (srcIsRoot) {
                ScreenPtr masterScreen = dixGetMasterScreen();
                dx += masterScreen->x;
                dy += masterScreen->y;
            }

            sourceBox.x1 = MIN(srcx + dx, 0);
            sourceBox.y1 = MIN(srcy + dy, 0);
            sourceBox.x2 = MAX(sourceBox.x1 + width, 32767);
            sourceBox.y2 = MAX(sourceBox.y1 + height, 32767);

            RegionInit(&rgn, &sourceBox, 1);

            /* subtract the (screen-space) clips of the source drawables */
            XINERAMA_FOR_EACH_SCREEN_BACKWARD({
                RegionPtr sd;

                if (pGC->subWindowMode == IncludeInferiors)
                    sd = NotClippedByChildren((WindowPtr)drawables[walkScreenIdx]);
                else
                    sd = &((WindowPtr)drawables[walkScreenIdx])->clipList;

                if (srcIsRoot)
                    RegionTranslate(&rgn, -walkScreen->x, -walkScreen->y);

                RegionSubtract(&rgn, &rgn, sd);

                if (srcIsRoot)
                    RegionTranslate(&rgn, walkScreen->x, walkScreen->y);

                if (pGC->subWindowMode == IncludeInferiors)
                    RegionDestroy(sd);
            });

            /* -dx/-dy to get back to dest-relative, plus request offsets */
            RegionTranslate(&rgn, -dx + dstx, -dy + dsty);

            /* intersect with gc clip; just one screen is fine because pixmap */
            RegionIntersect(&rgn, &rgn, pGC->pCompositeClip);

            /* and expose */
            SendGraphicsExpose(client, &rgn, dst->info[0].id, X_CopyArea, 0);
            RegionUninit(&rgn);
        }
    }
    else {
        DrawablePtr pDst = NULL, pSrc = NULL;
        GCPtr pGC = NULL;
        RegionRec totalReg;
        int rc;

        RegionNull(&totalReg);

        XINERAMA_FOR_EACH_SCREEN_BACKWARD({
            RegionPtr pRgn;

            stuff->dstDrawable = dst->info[walkScreenIdx].id;
            stuff->srcDrawable = src->info[walkScreenIdx].id;
            stuff->gc = gc->info[walkScreenIdx].id;
            if (srcIsRoot) {
                stuff->srcX = srcx - walkScreen->x;
                stuff->srcY = srcy - walkScreen->y;
            }
            if (dstIsRoot) {
                stuff->dstX = dstx - walkScreen->x;
                stuff->dstY = dsty - walkScreen->y;
            }

            VALIDATE_DRAWABLE_AND_GC(stuff->dstDrawable, pDst, DixWriteAccess);

            if (stuff->dstDrawable != stuff->srcDrawable) {
                rc = dixLookupDrawable(&pSrc, stuff->srcDrawable, client, 0,
                                       DixReadAccess);
                if (rc != Success)
                    return rc;

                if ((pDst->pScreen != pSrc->pScreen) ||
                    (pDst->depth != pSrc->depth)) {
                    client->errorValue = stuff->dstDrawable;
                    return BadMatch;
                }
            }
            else
                pSrc = pDst;

            pRgn = (*pGC->ops->CopyArea) (pSrc, pDst, pGC,
                                          stuff->srcX, stuff->srcY,
                                          stuff->width, stuff->height,
                                          stuff->dstX, stuff->dstY);
            if (pGC->graphicsExposures && pRgn) {
                if (srcIsRoot) {
                    RegionTranslate(pRgn, walkScreen->x, walkScreen->y);
                }
                RegionAppend(&totalReg, pRgn);
                RegionDestroy(pRgn);
            }

            if (dstShared)
                break;
        });

        if (pGC->graphicsExposures) {
            Bool overlap;

            RegionValidate(&totalReg, &overlap);
            SendGraphicsExpose(client, &totalReg, stuff->dstDrawable,
                               X_CopyArea, 0);
            RegionUninit(&totalReg);
        }
    }

    return Success;
}

int
PanoramiXCopyPlane(ClientPtr client)
{
    int srcx, srcy, dstx, dsty, rc;
    PanoramiXRes *gc, *src, *dst;
    Bool srcIsRoot = FALSE;
    Bool dstIsRoot = FALSE;
    Bool srcShared, dstShared;
    DrawablePtr psrcDraw, pdstDraw = NULL;
    GCPtr pGC = NULL;
    RegionRec totalReg;

    REQUEST(xCopyPlaneReq);

    REQUEST_SIZE_MATCH(xCopyPlaneReq);

    rc = dixLookupResourceByClass((void **) &src, stuff->srcDrawable,
                                  XRC_DRAWABLE, client, DixReadAccess);
    if (rc != Success)
        return (rc == BadValue) ? BadDrawable : rc;

    srcShared = IS_SHARED_PIXMAP(src);

    rc = dixLookupResourceByClass((void **) &dst, stuff->dstDrawable,
                                  XRC_DRAWABLE, client, DixWriteAccess);
    if (rc != Success)
        return (rc == BadValue) ? BadDrawable : rc;

    dstShared = IS_SHARED_PIXMAP(dst);

    if (dstShared && srcShared)
        return (*SavedProcVector[X_CopyPlane]) (client);

    rc = dixLookupResourceByType((void **) &gc, stuff->gc, XRT_GC,
                                 client, DixReadAccess);
    if (rc != Success)
        return rc;

    if ((dst->type == XRT_WINDOW) && dst->u.win.root)
        dstIsRoot = TRUE;
    if ((src->type == XRT_WINDOW) && src->u.win.root)
        srcIsRoot = TRUE;

    srcx = stuff->srcX;
    srcy = stuff->srcY;
    dstx = stuff->dstX;
    dsty = stuff->dstY;

    RegionNull(&totalReg);

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        RegionPtr pRgn;

        stuff->dstDrawable = dst->info[walkScreenIdx].id;
        stuff->srcDrawable = src->info[walkScreenIdx].id;
        stuff->gc = gc->info[walkScreenIdx].id;
        if (srcIsRoot) {
            stuff->srcX = srcx - walkScreen->x;
            stuff->srcY = srcy - walkScreen->y;
        }
        if (dstIsRoot) {
            stuff->dstX = dstx - walkScreen->x;
            stuff->dstY = dsty - walkScreen->y;
        }

        VALIDATE_DRAWABLE_AND_GC(stuff->dstDrawable, pdstDraw, DixWriteAccess);
        if (stuff->dstDrawable != stuff->srcDrawable) {
            rc = dixLookupDrawable(&psrcDraw, stuff->srcDrawable, client, 0,
                                   DixReadAccess);
            if (rc != Success)
                return rc;

            if (pdstDraw->pScreen != psrcDraw->pScreen) {
                client->errorValue = stuff->dstDrawable;
                return BadMatch;
            }
        }
        else
            psrcDraw = pdstDraw;

        if (stuff->bitPlane == 0 || (stuff->bitPlane & (stuff->bitPlane - 1)) ||
            (stuff->bitPlane > (1L << (psrcDraw->depth - 1)))) {
            client->errorValue = stuff->bitPlane;
            return BadValue;
        }

        pRgn = (*pGC->ops->CopyPlane) (psrcDraw, pdstDraw, pGC,
                                       stuff->srcX, stuff->srcY,
                                       stuff->width, stuff->height,
                                       stuff->dstX, stuff->dstY,
                                       stuff->bitPlane);
        if (pGC->graphicsExposures && pRgn) {
            RegionAppend(&totalReg, pRgn);
            RegionDestroy(pRgn);
        }

        if (dstShared)
            break;
    });

    if (pGC->graphicsExposures) {
        Bool overlap;

        RegionValidate(&totalReg, &overlap);
        SendGraphicsExpose(client, &totalReg, stuff->dstDrawable,
                           X_CopyPlane, 0);
        RegionUninit(&totalReg);
    }

    return Success;
}

int
PanoramiXPolyPoint(ClientPtr client)
{
    PanoramiXRes *gc, *draw;
    int result, npoint;
    Bool isRoot;

    REQUEST(xPolyPointReq);

    REQUEST_AT_LEAST_SIZE(xPolyPointReq);

    result = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                      XRC_DRAWABLE, client, DixWriteAccess);
    if (result != Success)
        return (result == BadValue) ? BadDrawable : result;

    if (IS_SHARED_PIXMAP(draw))
        return (*SavedProcVector[X_PolyPoint]) (client);

    result = dixLookupResourceByType((void **) &gc, stuff->gc, XRT_GC,
                                     client, DixReadAccess);
    if (result != Success)
        return result;

    isRoot = (draw->type == XRT_WINDOW) && draw->u.win.root;
    npoint = bytes_to_int32((client->req_len << 2) - sizeof(xPolyPointReq));
    if (npoint > 0) {
        xPoint *origPts = calloc(npoint, sizeof(xPoint));
        if (!origPts)
            return BadAlloc;

        memcpy((char *) origPts, (char *) &stuff[1], npoint * sizeof(xPoint));

        XINERAMA_FOR_EACH_SCREEN_FORWARD({
            if (walkScreenIdx)
                memcpy(&stuff[1], origPts, npoint * sizeof(xPoint));

            if (isRoot) {
                int x_off = walkScreen->x;
                int y_off = walkScreen->y;

                if (x_off || y_off) {
                    xPoint *pnts = (xPoint *) &stuff[1];
                    int i =
                        (stuff->coordMode == CoordModePrevious) ? 1 : npoint;

                    while (i--) {
                        pnts->x -= x_off;
                        pnts->y -= y_off;
                        pnts++;
                    }
                }
            }

            stuff->drawable = draw->info[walkScreenIdx].id;
            stuff->gc = gc->info[walkScreenIdx].id;
            result = (*SavedProcVector[X_PolyPoint]) (client);
            if (result != Success)
                break;
        });

        free(origPts);
        return result;
    }
    else
        return Success;
}

int
PanoramiXPolyLine(ClientPtr client)
{
    PanoramiXRes *gc, *draw;
    int result, npoint;
    Bool isRoot;

    REQUEST(xPolyLineReq);

    REQUEST_AT_LEAST_SIZE(xPolyLineReq);

    result = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                      XRC_DRAWABLE, client, DixWriteAccess);
    if (result != Success)
        return (result == BadValue) ? BadDrawable : result;

    if (IS_SHARED_PIXMAP(draw))
        return (*SavedProcVector[X_PolyLine]) (client);

    result = dixLookupResourceByType((void **) &gc, stuff->gc, XRT_GC,
                                     client, DixReadAccess);
    if (result != Success)
        return result;

    isRoot = IS_ROOT_DRAWABLE(draw);
    npoint = bytes_to_int32((client->req_len << 2) - sizeof(xPolyLineReq));
    if (npoint > 0) {
        xPoint *origPts = calloc(npoint, sizeof(xPoint));
        if (!origPts)
            return BadAlloc;
        memcpy((char *) origPts, (char *) &stuff[1], npoint * sizeof(xPoint));

        XINERAMA_FOR_EACH_SCREEN_FORWARD({
            if (walkScreenIdx)
                memcpy(&stuff[1], origPts, npoint * sizeof(xPoint));

            if (isRoot) {
                int x_off = walkScreen->x;
                int y_off = walkScreen->y;

                if (x_off || y_off) {
                    xPoint *pnts = (xPoint *) &stuff[1];
                    int i =
                        (stuff->coordMode == CoordModePrevious) ? 1 : npoint;

                    while (i--) {
                        pnts->x -= x_off;
                        pnts->y -= y_off;
                        pnts++;
                    }
                }
            }

            stuff->drawable = draw->info[walkScreenIdx].id;
            stuff->gc = gc->info[walkScreenIdx].id;
            result = (*SavedProcVector[X_PolyLine]) (client);
            if (result != Success)
                break;
        });

        free(origPts);
        return result;
    }
    else
        return Success;
}

int
PanoramiXPolySegment(ClientPtr client)
{
    int result, nsegs, i;
    PanoramiXRes *gc, *draw;
    Bool isRoot;

    REQUEST(xPolySegmentReq);

    REQUEST_AT_LEAST_SIZE(xPolySegmentReq);

    result = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                      XRC_DRAWABLE, client, DixWriteAccess);
    if (result != Success)
        return (result == BadValue) ? BadDrawable : result;

    if (IS_SHARED_PIXMAP(draw))
        return (*SavedProcVector[X_PolySegment]) (client);

    result = dixLookupResourceByType((void **) &gc, stuff->gc, XRT_GC,
                                     client, DixReadAccess);
    if (result != Success)
        return result;

    isRoot = IS_ROOT_DRAWABLE(draw);

    nsegs = (client->req_len << 2) - sizeof(xPolySegmentReq);
    if (nsegs & 4)
        return BadLength;
    nsegs >>= 3;
    if (nsegs > 0) {
        xSegment *origSegs = calloc(nsegs, sizeof(xSegment));
        if (!origSegs)
            return BadAlloc;
        memcpy((char *) origSegs, (char *) &stuff[1], nsegs * sizeof(xSegment));

        XINERAMA_FOR_EACH_SCREEN_FORWARD({
            if (walkScreenIdx) /* skip on screen #0 */
                memcpy(&stuff[1], origSegs, nsegs * sizeof(xSegment));

            if (isRoot) {
                int x_off = walkScreen->x;
                int y_off = walkScreen->y;

                if (x_off || y_off) {
                    xSegment *segs = (xSegment *) &stuff[1];

                    for (i = nsegs; i--; segs++) {
                        segs->x1 -= x_off;
                        segs->x2 -= x_off;
                        segs->y1 -= y_off;
                        segs->y2 -= y_off;
                    }
                }
            }

            stuff->drawable = draw->info[walkScreenIdx].id;
            stuff->gc = gc->info[walkScreenIdx].id;
            result = (*SavedProcVector[X_PolySegment]) (client);
            if (result != Success)
                break;
        });

        free(origSegs);
        return result;
    }
    else
        return Success;
}

int
PanoramiXPolyRectangle(ClientPtr client)
{
    int result, nrects, i;
    PanoramiXRes *gc, *draw;
    Bool isRoot;

    REQUEST(xPolyRectangleReq);

    REQUEST_AT_LEAST_SIZE(xPolyRectangleReq);

    result = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                      XRC_DRAWABLE, client, DixWriteAccess);
    if (result != Success)
        return (result == BadValue) ? BadDrawable : result;

    if (IS_SHARED_PIXMAP(draw))
        return (*SavedProcVector[X_PolyRectangle]) (client);

    result = dixLookupResourceByType((void **) &gc, stuff->gc, XRT_GC,
                                     client, DixReadAccess);
    if (result != Success)
        return result;

    isRoot = IS_ROOT_DRAWABLE(draw);

    nrects = (client->req_len << 2) - sizeof(xPolyRectangleReq);
    if (nrects & 4)
        return BadLength;
    nrects >>= 3;
    if (nrects > 0) {
        xRectangle *origRecs = calloc(nrects, sizeof(xRectangle));
        if (!origRecs)
            return BadAlloc;
        memcpy((char *) origRecs, (char *) &stuff[1],
               nrects * sizeof(xRectangle));

        XINERAMA_FOR_EACH_SCREEN_FORWARD({
            if (walkScreenIdx) /* skip on screen #0 */
                memcpy(&stuff[1], origRecs, nrects * sizeof(xRectangle));

            if (isRoot) {
                int x_off = walkScreen->x;
                int y_off = walkScreen->y;

                if (x_off || y_off) {
                    xRectangle *rects = (xRectangle *) &stuff[1];

                    for (i = nrects; i--; rects++) {
                        rects->x -= x_off;
                        rects->y -= y_off;
                    }
                }
            }

            stuff->drawable = draw->info[walkScreenIdx].id;
            stuff->gc = gc->info[walkScreenIdx].id;
            result = (*SavedProcVector[X_PolyRectangle]) (client);
            if (result != Success)
                break;
        });

        free(origRecs);
        return result;
    }
    else
        return Success;
}

int
PanoramiXPolyArc(ClientPtr client)
{
    int result, narcs, i;
    PanoramiXRes *gc, *draw;
    Bool isRoot;

    REQUEST(xPolyArcReq);

    REQUEST_AT_LEAST_SIZE(xPolyArcReq);

    result = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                      XRC_DRAWABLE, client, DixWriteAccess);
    if (result != Success)
        return (result == BadValue) ? BadDrawable : result;

    if (IS_SHARED_PIXMAP(draw))
        return (*SavedProcVector[X_PolyArc]) (client);

    result = dixLookupResourceByType((void **) &gc, stuff->gc, XRT_GC,
                                     client, DixReadAccess);
    if (result != Success)
        return result;

    isRoot = IS_ROOT_DRAWABLE(draw);

    narcs = (client->req_len << 2) - sizeof(xPolyArcReq);
    if (narcs % sizeof(xArc))
        return BadLength;
    narcs /= sizeof(xArc);
    if (narcs > 0) {
        xArc *origArcs = calloc(narcs, sizeof(xArc));
        if (!origArcs)
            return BadAlloc;
        memcpy((char *) origArcs, (char *) &stuff[1], narcs * sizeof(xArc));

        XINERAMA_FOR_EACH_SCREEN_FORWARD({
            if (walkScreenIdx) /* skip screen #0 */
                memcpy(&stuff[1], origArcs, narcs * sizeof(xArc));

            if (isRoot) {
                int x_off = walkScreen->x;
                int y_off = walkScreen->y;

                if (x_off || y_off) {
                    xArc *arcs = (xArc *) &stuff[1];

                    for (i = narcs; i--; arcs++) {
                        arcs->x -= x_off;
                        arcs->y -= y_off;
                    }
                }
            }
            stuff->drawable = draw->info[walkScreenIdx].id;
            stuff->gc = gc->info[walkScreenIdx].id;
            result = (*SavedProcVector[X_PolyArc]) (client);
            if (result != Success)
                break;
        });

        free(origArcs);
        return result;
    }
    else
        return Success;
}

int
PanoramiXFillPoly(ClientPtr client)
{
    int result, count;
    PanoramiXRes *gc, *draw;
    Bool isRoot;

    REQUEST(xFillPolyReq);

    REQUEST_AT_LEAST_SIZE(xFillPolyReq);

    result = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                      XRC_DRAWABLE, client, DixWriteAccess);
    if (result != Success)
        return (result == BadValue) ? BadDrawable : result;

    if (IS_SHARED_PIXMAP(draw))
        return (*SavedProcVector[X_FillPoly]) (client);

    result = dixLookupResourceByType((void **) &gc, stuff->gc, XRT_GC,
                                     client, DixReadAccess);
    if (result != Success)
        return result;

    isRoot = IS_ROOT_DRAWABLE(draw);

    count = bytes_to_int32((client->req_len << 2) - sizeof(xFillPolyReq));
    if (count > 0) {
        DDXPointPtr locPts = calloc(count, sizeof(DDXPointRec));
        if (!locPts)
            return BadAlloc;
        memcpy((char *) locPts, (char *) &stuff[1],
               count * sizeof(DDXPointRec));

        XINERAMA_FOR_EACH_SCREEN_FORWARD({
            if (walkScreenIdx) /* skip screen #0 */
                memcpy(&stuff[1], locPts, count * sizeof(DDXPointRec));

            if (isRoot) {
                int x_off = walkScreen->x;
                int y_off = walkScreen->y;

                if (x_off || y_off) {
                    DDXPointPtr pnts = (DDXPointPtr) &stuff[1];
                    int i = (stuff->coordMode == CoordModePrevious) ? 1 : count;

                    while (i--) {
                        pnts->x -= x_off;
                        pnts->y -= y_off;
                        pnts++;
                    }
                }
            }

            stuff->drawable = draw->info[walkScreenIdx].id;
            stuff->gc = gc->info[walkScreenIdx].id;
            result = (*SavedProcVector[X_FillPoly]) (client);
            if (result != Success)
                break;
        });

        free(locPts);
        return result;
    }
    else
        return Success;
}

int
PanoramiXPolyFillRectangle(ClientPtr client)
{
    int result, things, i;
    PanoramiXRes *gc, *draw;
    Bool isRoot;
    REQUEST(xPolyFillRectangleReq);

    REQUEST_AT_LEAST_SIZE(xPolyFillRectangleReq);

    result = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                      XRC_DRAWABLE, client, DixWriteAccess);
    if (result != Success)
        return (result == BadValue) ? BadDrawable : result;

    if (IS_SHARED_PIXMAP(draw))
        return (*SavedProcVector[X_PolyFillRectangle]) (client);

    result = dixLookupResourceByType((void **) &gc, stuff->gc, XRT_GC,
                                     client, DixReadAccess);
    if (result != Success)
        return result;

    isRoot = IS_ROOT_DRAWABLE(draw);

    things = (client->req_len << 2) - sizeof(xPolyFillRectangleReq);
    if (things & 4)
        return BadLength;
    things >>= 3;
    if (things > 0) {
        xRectangle *origRects = calloc(things, sizeof(xRectangle));
        if (!origRects)
            return BadAlloc;
        memcpy((char *) origRects, (char *) &stuff[1],
               things * sizeof(xRectangle));

        XINERAMA_FOR_EACH_SCREEN_FORWARD({
            if (walkScreenIdx) /* skip screen #0 */
                memcpy(&stuff[1], origRects, things * sizeof(xRectangle));

            if (isRoot) {
                int x_off = walkScreen->x;
                int y_off = walkScreen->y;

                if (x_off || y_off) {
                    xRectangle *rects = (xRectangle *) &stuff[1];

                    for (i = things; i--; rects++) {
                        rects->x -= x_off;
                        rects->y -= y_off;
                    }
                }
            }

            stuff->drawable = draw->info[walkScreenIdx].id;
            stuff->gc = gc->info[walkScreenIdx].id;
            result = (*SavedProcVector[X_PolyFillRectangle]) (client);
            if (result != Success)
                break;
        });

        free(origRects);
        return result;
    }
    else
        return Success;
}

int
PanoramiXPolyFillArc(ClientPtr client)
{
    PanoramiXRes *gc, *draw;
    Bool isRoot;
    int result, narcs, i;

    REQUEST(xPolyFillArcReq);

    REQUEST_AT_LEAST_SIZE(xPolyFillArcReq);

    result = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                      XRC_DRAWABLE, client, DixWriteAccess);
    if (result != Success)
        return (result == BadValue) ? BadDrawable : result;

    if (IS_SHARED_PIXMAP(draw))
        return (*SavedProcVector[X_PolyFillArc]) (client);

    result = dixLookupResourceByType((void **) &gc, stuff->gc, XRT_GC,
                                     client, DixReadAccess);
    if (result != Success)
        return result;

    isRoot = IS_ROOT_DRAWABLE(draw);

    narcs = (client->req_len << 2) - sizeof(xPolyFillArcReq);
    if (narcs % sizeof(xArc))
        return BadLength;
    narcs /= sizeof(xArc);
    if (narcs > 0) {
        xArc *origArcs = calloc(narcs, sizeof(xArc));
        if (!origArcs)
            return BadAlloc;
        memcpy((char *) origArcs, (char *) &stuff[1], narcs * sizeof(xArc));

        XINERAMA_FOR_EACH_SCREEN_FORWARD({
            if (walkScreenIdx) /* skip screen #0 */
                memcpy(&stuff[1], origArcs, narcs * sizeof(xArc));

            if (isRoot) {
                int x_off = walkScreen->x;
                int y_off = walkScreen->y;

                if (x_off || y_off) {
                    xArc *arcs = (xArc *) &stuff[1];

                    for (i = narcs; i--; arcs++) {
                        arcs->x -= x_off;
                        arcs->y -= y_off;
                    }
                }
            }

            stuff->drawable = draw->info[walkScreenIdx].id;
            stuff->gc = gc->info[walkScreenIdx].id;
            result = (*SavedProcVector[X_PolyFillArc]) (client);
            if (result != Success)
                break;
        });

        free(origArcs);
        return result;
    }
    else
        return Success;
}

int
PanoramiXPutImage(ClientPtr client)
{
    PanoramiXRes *gc, *draw;
    Bool isRoot;
    int result, orig_x, orig_y;

    REQUEST(xPutImageReq);

    REQUEST_AT_LEAST_SIZE(xPutImageReq);

    result = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                      XRC_DRAWABLE, client, DixWriteAccess);
    if (result != Success)
        return (result == BadValue) ? BadDrawable : result;

    if (IS_SHARED_PIXMAP(draw))
        return (*SavedProcVector[X_PutImage]) (client);

    result = dixLookupResourceByType((void **) &gc, stuff->gc, XRT_GC,
                                     client, DixReadAccess);
    if (result != Success)
        return result;

    isRoot = IS_ROOT_DRAWABLE(draw);

    orig_x = stuff->dstX;
    orig_y = stuff->dstY;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        if (isRoot) {
            stuff->dstX = orig_x - walkScreen->x;
            stuff->dstY = orig_y - walkScreen->y;
        }
        stuff->drawable = draw->info[walkScreenIdx].id;
        stuff->gc = gc->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_PutImage]) (client);
        if (result != Success)
            break;
    });

    return result;
}

int
PanoramiXGetImage(ClientPtr client)
{
    DrawablePtr drawables[MAXSCREENS];
    DrawablePtr pDraw;
    PanoramiXRes *draw;
    Bool isRoot;
    int x, y, w, h, format, rc;
    Mask plane = 0, planemask;
    int linesDone, nlines, linesPerBuf;
    long widthBytesLine;

    REQUEST(xGetImageReq);

    REQUEST_SIZE_MATCH(xGetImageReq);

    if ((stuff->format != XYPixmap) && (stuff->format != ZPixmap)) {
        client->errorValue = stuff->format;
        return BadValue;
    }

    rc = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                  XRC_DRAWABLE, client, DixReadAccess);
    if (rc != Success)
        return (rc == BadValue) ? BadDrawable : rc;

    if (draw->type == XRT_PIXMAP)
        return (*SavedProcVector[X_GetImage]) (client);

    rc = dixLookupDrawable(&pDraw, stuff->drawable, client, 0, DixReadAccess);
    if (rc != Success)
        return rc;

    if (!((WindowPtr) pDraw)->realized)
        return BadMatch;

    x = stuff->x;
    y = stuff->y;
    w = stuff->width;
    h = stuff->height;
    format = stuff->format;
    planemask = stuff->planeMask;

    isRoot = IS_ROOT_DRAWABLE(draw);

    if (isRoot) {
        /* check for being onscreen */
        if (x < 0 || x + w > PanoramiXPixWidth ||
            y < 0 || y + h > PanoramiXPixHeight)
            return BadMatch;
    }
    else {
        ScreenPtr masterScreen = dixGetMasterScreen();
        /* check for being onscreen and inside of border */
        if (masterScreen->x + pDraw->x + x < 0 ||
            masterScreen->x + pDraw->x + x + w > PanoramiXPixWidth ||
            masterScreen->y + pDraw->y + y < 0 ||
            masterScreen->y + pDraw->y + y + h > PanoramiXPixHeight ||
            x < -wBorderWidth((WindowPtr) pDraw) ||
            x + w > wBorderWidth((WindowPtr) pDraw) + (int) pDraw->width ||
            y < -wBorderWidth((WindowPtr) pDraw) ||
            y + h > wBorderWidth((WindowPtr) pDraw) + (int) pDraw->height)
            return BadMatch;
    }

    drawables[0] = pDraw;

    XINERAMA_FOR_EACH_SCREEN_FORWARD_SKIP0({
        rc = dixLookupDrawable(drawables + walkScreenIdx,
                               draw->info[walkScreenIdx].id,
                               client, 0,
                               DixGetAttrAccess);
        if (rc != Success)
            return rc;
    });

    XINERAMA_FOR_EACH_SCREEN_FORWARD({
        DrawablePtr d = drawables[walkScreenIdx];
        d->pScreen->SourceValidate(d, 0, 0, d->width, d->height, IncludeInferiors);
    });

    size_t length;
    if (format == ZPixmap) {
        widthBytesLine = PixmapBytePad(w, pDraw->depth);
        length = widthBytesLine * h;
    }
    else {
        widthBytesLine = BitmapBytePad(w);
        plane = ((Mask) 1) << (pDraw->depth - 1);
        /* only planes asked for */
        length = widthBytesLine * h * Ones(planemask & (plane | (plane - 1)));
    }

    if (widthBytesLine == 0 || h == 0)
        linesPerBuf = 0;
    else if (widthBytesLine >= XINERAMA_IMAGE_BUFSIZE)
        linesPerBuf = 1;
    else {
        linesPerBuf = XINERAMA_IMAGE_BUFSIZE / widthBytesLine;
        if (linesPerBuf > h)
            linesPerBuf = h;
    }


    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    /* can become quite big, so make enough room so we don't need to relloc */
    if (!x_rpcbuf_makeroom(&rpcbuf, length))
        return BadAlloc;

    if (linesPerBuf == 0) {
        /* nothing to do */
    }
    else if (format == ZPixmap) {
        linesDone = 0;
        while (h - linesDone > 0) {
            nlines = MIN(linesPerBuf, h - linesDone);

            char *pBuf = x_rpcbuf_reserve(&rpcbuf, nlines * widthBytesLine);
            if (!pBuf)
                return BadAlloc;
            XineramaGetImageData(drawables, x, y + linesDone, w, nlines,
                                 format, planemask, pBuf, widthBytesLine,
                                 isRoot);

            linesDone += nlines;
        }
    }
    else {                      /* XYPixmap */
        for (; plane; plane >>= 1) {
            if (planemask & plane) {
                linesDone = 0;
                while (h - linesDone > 0) {
                    nlines = MIN(linesPerBuf, h - linesDone);

                    char *pBuf = x_rpcbuf_reserve(&rpcbuf, nlines * widthBytesLine);
                    if (!pBuf)
                        return BadAlloc;
                    XineramaGetImageData(drawables, x, y + linesDone, w,
                                         nlines, format, plane, pBuf,
                                         widthBytesLine, isRoot);

                    linesDone += nlines;
                }
            }
        }
    }

    xGetImageReply reply = {
        .visual = wVisual(((WindowPtr) pDraw)),
        .depth = pDraw->depth,
    };

    if (client->swapped) {
        swaps(&reply.sequenceNumber);
        swapl(&reply.visual);
    }

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

/* The text stuff should be rewritten so that duplication happens
   at the GlyphBlt level.  That is, loading the font and getting
   the glyphs should only happen once */

int
PanoramiXPolyText8(ClientPtr client)
{
    PanoramiXRes *gc, *draw;
    Bool isRoot;
    int result;
    int orig_x, orig_y;

    REQUEST(xPolyTextReq);

    REQUEST_AT_LEAST_SIZE(xPolyTextReq);

    result = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                      XRC_DRAWABLE, client, DixWriteAccess);
    if (result != Success)
        return (result == BadValue) ? BadDrawable : result;

    if (IS_SHARED_PIXMAP(draw))
        return (*SavedProcVector[X_PolyText8]) (client);

    result = dixLookupResourceByType((void **) &gc, stuff->gc, XRT_GC,
                                     client, DixReadAccess);
    if (result != Success)
        return result;

    isRoot = IS_ROOT_DRAWABLE(draw);

    orig_x = stuff->x;
    orig_y = stuff->y;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->drawable = draw->info[walkScreenIdx].id;
        stuff->gc = gc->info[walkScreenIdx].id;
        if (isRoot) {
            stuff->x = orig_x - walkScreen->x;
            stuff->y = orig_y - walkScreen->y;
        }
        result = (*SavedProcVector[X_PolyText8]) (client);
        if (result != Success)
            break;
    });

    return result;
}

int
PanoramiXPolyText16(ClientPtr client)
{
    PanoramiXRes *gc, *draw;
    Bool isRoot;
    int result;
    int orig_x, orig_y;

    REQUEST(xPolyTextReq);

    REQUEST_AT_LEAST_SIZE(xPolyTextReq);

    result = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                      XRC_DRAWABLE, client, DixWriteAccess);
    if (result != Success)
        return (result == BadValue) ? BadDrawable : result;

    if (IS_SHARED_PIXMAP(draw))
        return (*SavedProcVector[X_PolyText16]) (client);

    result = dixLookupResourceByType((void **) &gc, stuff->gc, XRT_GC,
                                     client, DixReadAccess);
    if (result != Success)
        return result;

    isRoot = IS_ROOT_DRAWABLE(draw);

    orig_x = stuff->x;
    orig_y = stuff->y;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->drawable = draw->info[walkScreenIdx].id;
        stuff->gc = gc->info[walkScreenIdx].id;
        if (isRoot) {
            stuff->x = orig_x - walkScreen->x;
            stuff->y = orig_y - walkScreen->y;
        }
        result = (*SavedProcVector[X_PolyText16]) (client);
        if (result != Success)
            break;
    });

    return result;
}

int
PanoramiXImageText8(ClientPtr client)
{
    int result;
    PanoramiXRes *gc, *draw;
    Bool isRoot;
    int orig_x, orig_y;

    REQUEST(xImageTextReq);

    REQUEST_FIXED_SIZE(xImageTextReq, stuff->nChars);

    result = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                      XRC_DRAWABLE, client, DixWriteAccess);
    if (result != Success)
        return (result == BadValue) ? BadDrawable : result;

    if (IS_SHARED_PIXMAP(draw))
        return (*SavedProcVector[X_ImageText8]) (client);

    result = dixLookupResourceByType((void **) &gc, stuff->gc, XRT_GC,
                                     client, DixReadAccess);
    if (result != Success)
        return result;

    isRoot = IS_ROOT_DRAWABLE(draw);

    orig_x = stuff->x;
    orig_y = stuff->y;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->drawable = draw->info[walkScreenIdx].id;
        stuff->gc = gc->info[walkScreenIdx].id;
        if (isRoot) {
            stuff->x = orig_x - walkScreen->x;
            stuff->y = orig_y - walkScreen->y;
        }
        result = (*SavedProcVector[X_ImageText8]) (client);
        if (result != Success)
            break;
    });

    return result;
}

int
PanoramiXImageText16(ClientPtr client)
{
    int result;
    PanoramiXRes *gc, *draw;
    Bool isRoot;
    int orig_x, orig_y;

    REQUEST(xImageTextReq);

    REQUEST_FIXED_SIZE(xImageTextReq, stuff->nChars << 1);

    result = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                      XRC_DRAWABLE, client, DixWriteAccess);
    if (result != Success)
        return (result == BadValue) ? BadDrawable : result;

    if (IS_SHARED_PIXMAP(draw))
        return (*SavedProcVector[X_ImageText16]) (client);

    result = dixLookupResourceByType((void **) &gc, stuff->gc, XRT_GC,
                                     client, DixReadAccess);
    if (result != Success)
        return result;

    isRoot = IS_ROOT_DRAWABLE(draw);

    orig_x = stuff->x;
    orig_y = stuff->y;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->drawable = draw->info[walkScreenIdx].id;
        stuff->gc = gc->info[walkScreenIdx].id;
        if (isRoot) {
            stuff->x = orig_x - walkScreen->x;
            stuff->y = orig_y - walkScreen->y;
        }
        result = (*SavedProcVector[X_ImageText16]) (client);
        if (result != Success)
            break;
    });

    return result;
}

int
PanoramiXCreateColormap(ClientPtr client)
{
    PanoramiXRes *win, *newCmap;
    int result, orig_visual;

    REQUEST(xCreateColormapReq);

    REQUEST_SIZE_MATCH(xCreateColormapReq);

    result = dixLookupResourceByType((void **) &win, stuff->window,
                                     XRT_WINDOW, client, DixReadAccess);
    if (result != Success)
        return result;

    if (!(newCmap = calloc(1, sizeof(PanoramiXRes))))
        return BadAlloc;

    newCmap->type = XRT_COLORMAP;
    panoramix_setup_ids(newCmap, client, stuff->mid);

    orig_visual = stuff->visual;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->mid = newCmap->info[walkScreenIdx].id;
        stuff->window = win->info[walkScreenIdx].id;
        stuff->visual = PanoramiXTranslateVisualID(walkScreenIdx, orig_visual);
        result = (*SavedProcVector[X_CreateColormap]) (client);
        if (result != Success)
            break;
    });

    if (result == Success)
        AddResource(newCmap->info[0].id, XRT_COLORMAP, newCmap);
    else
        free(newCmap);

    return result;
}

int
PanoramiXFreeColormap(ClientPtr client)
{
    PanoramiXRes *cmap;
    int result;

    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);

    client->errorValue = stuff->id;

    result = dixLookupResourceByType((void **) &cmap, stuff->id, XRT_COLORMAP,
                                     client, DixDestroyAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->id = cmap->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_FreeColormap]) (client);
        if (result != Success)
            break;
    });

    /* Since ProcFreeColormap is using FreeResource, it will free
       our resource for us on the last pass through the loop above */

    return result;
}

int
PanoramiXCopyColormapAndFree(ClientPtr client)
{
    PanoramiXRes *cmap, *newCmap;
    int result;

    REQUEST(xCopyColormapAndFreeReq);

    REQUEST_SIZE_MATCH(xCopyColormapAndFreeReq);

    client->errorValue = stuff->srcCmap;

    result = dixLookupResourceByType((void **) &cmap, stuff->srcCmap,
                                     XRT_COLORMAP, client,
                                     DixReadAccess | DixWriteAccess);
    if (result != Success)
        return result;

    if (!(newCmap = calloc(1, sizeof(PanoramiXRes))))
        return BadAlloc;

    newCmap->type = XRT_COLORMAP;
    panoramix_setup_ids(newCmap, client, stuff->mid);

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->srcCmap = cmap->info[walkScreenIdx].id;
        stuff->mid = newCmap->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_CopyColormapAndFree]) (client);
        if (result != Success)
            break;
    });

    if (result == Success)
        AddResource(newCmap->info[0].id, XRT_COLORMAP, newCmap);
    else
        free(newCmap);

    return result;
}

int
PanoramiXInstallColormap(ClientPtr client)
{
    REQUEST(xResourceReq);
    int result;
    PanoramiXRes *cmap;

    REQUEST_SIZE_MATCH(xResourceReq);

    client->errorValue = stuff->id;

    result = dixLookupResourceByType((void **) &cmap, stuff->id, XRT_COLORMAP,
                                     client, DixReadAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->id = cmap->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_InstallColormap]) (client);
        if (result != Success)
            break;
    });

    return result;
}

int
PanoramiXUninstallColormap(ClientPtr client)
{
    REQUEST(xResourceReq);
    int result;
    PanoramiXRes *cmap;

    REQUEST_SIZE_MATCH(xResourceReq);

    client->errorValue = stuff->id;

    result = dixLookupResourceByType((void **) &cmap, stuff->id, XRT_COLORMAP,
                                     client, DixReadAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->id = cmap->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_UninstallColormap]) (client);
        if (result != Success)
            break;
    });

    return result;
}

int
PanoramiXAllocColor(ClientPtr client)
{
    int result;
    PanoramiXRes *cmap;

    REQUEST(xAllocColorReq);
    REQUEST_SIZE_MATCH(xAllocColorReq);

    if (client->swapped) {
        swapl(&stuff->cmap);
        swaps(&stuff->red);
        swaps(&stuff->green);
        swaps(&stuff->blue);
    }

    client->errorValue = stuff->cmap;

    result = dixLookupResourceByType((void **) &cmap, stuff->cmap,
                                     XRT_COLORMAP, client, DixWriteAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        Colormap childCmap = cmap->info[walkScreenIdx].id;

        CARD16 red = stuff->red;
        CARD16 green = stuff->green;
        CARD16 blue = stuff->blue;
        CARD32 pixel = 0;

        result = dixAllocColor(client, childCmap, &red, &green, &blue, &pixel);
        if (result != Success)
            return result;

        /* only send out reply for on first screen */
        if (!walkScreenIdx) {
            xAllocColorReply reply; /* static init would confuse preprocessor */
            reply.red = red;
            reply.green = green;
            reply.blue = blue;
            reply.pixel = pixel;

            if (client->swapped) {
                swaps(&reply.red);
                swaps(&reply.green);
                swaps(&reply.blue);
                swapl(&reply.pixel);
            }

            /* iterating backwards, first screen comes last, so we can return here */
            return X_SEND_REPLY_SIMPLE(client, reply);
        }
    });

    /* shouldn't ever reach here, because we already returned from within the loop
       if this ever happens, PanoramiXNumScreens must be 0 */
    return BadImplementation;
}

int
PanoramiXAllocNamedColor(ClientPtr client)
{
    int result;
    PanoramiXRes *cmap;

    REQUEST(xAllocNamedColorReq);

    REQUEST_FIXED_SIZE(xAllocNamedColorReq, stuff->nbytes);

    client->errorValue = stuff->cmap;

    result = dixLookupResourceByType((void **) &cmap, stuff->cmap,
                                     XRT_COLORMAP, client, DixWriteAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->cmap = cmap->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_AllocNamedColor]) (client);
        if (result != Success)
            break;
    });

    return result;
}

int
PanoramiXAllocColorCells(ClientPtr client)
{
    int result;
    PanoramiXRes *cmap;

    REQUEST(xAllocColorCellsReq);

    REQUEST_SIZE_MATCH(xAllocColorCellsReq);

    client->errorValue = stuff->cmap;

    result = dixLookupResourceByType((void **) &cmap, stuff->cmap,
                                     XRT_COLORMAP, client, DixWriteAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->cmap = cmap->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_AllocColorCells]) (client);
        if (result != Success)
            break;
    });

    return result;
}

int
PanoramiXAllocColorPlanes(ClientPtr client)
{
    int result;
    PanoramiXRes *cmap;

    REQUEST(xAllocColorPlanesReq);

    REQUEST_SIZE_MATCH(xAllocColorPlanesReq);

    client->errorValue = stuff->cmap;

    result = dixLookupResourceByType((void **) &cmap, stuff->cmap,
                                     XRT_COLORMAP, client, DixWriteAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->cmap = cmap->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_AllocColorPlanes]) (client);
        if (result != Success)
            break;
    });

    return result;
}

int
PanoramiXFreeColors(ClientPtr client)
{
    int result;
    PanoramiXRes *cmap;

    REQUEST(xFreeColorsReq);

    REQUEST_AT_LEAST_SIZE(xFreeColorsReq);

    client->errorValue = stuff->cmap;

    result = dixLookupResourceByType((void **) &cmap, stuff->cmap,
                                     XRT_COLORMAP, client, DixWriteAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->cmap = cmap->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_FreeColors]) (client);
    });

    return result;
}

int
PanoramiXStoreColors(ClientPtr client)
{
    int result;
    PanoramiXRes *cmap;

    REQUEST(xStoreColorsReq);

    REQUEST_AT_LEAST_SIZE(xStoreColorsReq);

    client->errorValue = stuff->cmap;

    result = dixLookupResourceByType((void **) &cmap, stuff->cmap,
                                     XRT_COLORMAP, client, DixWriteAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->cmap = cmap->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_StoreColors]) (client);
        if (result != Success)
            break;
    });

    return result;
}

int
PanoramiXStoreNamedColor(ClientPtr client)
{
    int result;
    PanoramiXRes *cmap;

    REQUEST(xStoreNamedColorReq);

    REQUEST_FIXED_SIZE(xStoreNamedColorReq, stuff->nbytes);

    client->errorValue = stuff->cmap;

    result = dixLookupResourceByType((void **) &cmap, stuff->cmap,
                                     XRT_COLORMAP, client, DixWriteAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        stuff->cmap = cmap->info[walkScreenIdx].id;
        result = (*SavedProcVector[X_StoreNamedColor]) (client);
        if (result != Success)
            break;
    });

    return result;
}
