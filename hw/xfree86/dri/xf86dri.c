/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
Copyright 2000 VA Linux Systems, Inc.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Jens Owen <jens@tungstengraphics.com>
 *   Rickard E. (Rik) Faith <faith@valinux.com>
 *
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <string.h>
#include <X11/X.h>
#include <X11/Xproto.h>

#include "dix/dix_priv.h"
#include "dix/request_priv.h"
#include "dix/screenint_priv.h"

#include "xf86.h"
#include "misc.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "cursorstr.h"
#include "scrnintstr.h"
#include "servermd.h"
#include <X11/dri/xf86driproto.h>
#include "swaprep.h"
#include "xf86str.h"
#include "dri_priv.h"
#include "sarea.h"
#include "dristruct.h"
#include "xf86drm.h"
#include "protocol-versions.h"
#include "xf86Extensions.h"

static int DRIErrorBase;

static void XF86DRIResetProc(ExtensionEntry *extEntry);

/*ARGSUSED*/
static void
XF86DRIResetProc(ExtensionEntry *extEntry)
{
    DRIReset();
}

static int
ProcXF86DRIQueryVersion(register ClientPtr client)
{
    xXF86DRIQueryVersionReply reply = {
        .majorVersion = SERVER_XF86DRI_MAJOR_VERSION,
        .minorVersion = SERVER_XF86DRI_MINOR_VERSION,
        .patchVersion = SERVER_XF86DRI_PATCH_VERSION
    };

    REQUEST_SIZE_MATCH(xXF86DRIQueryVersionReq);
    if (client->swapped) {
        swaps(&reply.majorVersion);
        swaps(&reply.minorVersion);
        swapl(&reply.patchVersion);
    }
    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
ProcXF86DRIQueryDirectRenderingCapable(register ClientPtr client)
{
    Bool isCapable;

    REQUEST(xXF86DRIQueryDirectRenderingCapableReq);
    REQUEST_SIZE_MATCH(xXF86DRIQueryDirectRenderingCapableReq);

    if (client->swapped)
        swapl(&stuff->screen);

    ScreenPtr pScreen = dixGetScreenPtr(stuff->screen);

    if (!pScreen) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    if (!DRIQueryDirectRenderingCapable(pScreen,
                                        &isCapable)) {
        return BadValue;
    }

    if (!client->local || client->swapped)
        isCapable = 0;

    xXF86DRIQueryDirectRenderingCapableReply reply = {
        .isCapable = isCapable
    };

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
ProcXF86DRIOpenConnection(register ClientPtr client)
{
    drm_handle_t hSAREA;
    char *busIdString;
    CARD32 busIdStringLength = 0;

    REQUEST(xXF86DRIOpenConnectionReq);
    REQUEST_SIZE_MATCH(xXF86DRIOpenConnectionReq);

    ScreenPtr pScreen = dixGetScreenPtr(stuff->screen);
    if (!pScreen) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    if (!DRIOpenConnection(pScreen,
                           &hSAREA, &busIdString)) {
        return BadValue;
    }

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };
    if (busIdString) {
        busIdStringLength = strlen(busIdString);
        x_rpcbuf_write_CARD8s(&rpcbuf, (CARD8*)busIdString, strlen(busIdString));
    }

    xXF86DRIOpenConnectionReply reply = {
        .busIdStringLength = busIdStringLength,
        .hSAREALow = (CARD32) (hSAREA & 0xffffffff),
#if defined(LONG64) && !defined(__linux__)
        .hSAREAHigh = (CARD32) (hSAREA >> 32),
#endif
    };

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

static int
ProcXF86DRIAuthConnection(register ClientPtr client)
{
    REQUEST(xXF86DRIAuthConnectionReq);
    REQUEST_SIZE_MATCH(xXF86DRIAuthConnectionReq);

    ScreenPtr pScreen = dixGetScreenPtr(stuff->screen);
    if (!pScreen) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    CARD8 authenticated = 1;
    if (!DRIAuthConnection(pScreen, stuff->magic)) {
        ErrorF("Failed to authenticate %lu\n", (unsigned long) stuff->magic);
        authenticated = 0;
    }

    xXF86DRIAuthConnectionReply reply = {
        .authenticated = authenticated
    };

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
ProcXF86DRICloseConnection(register ClientPtr client)
{
    REQUEST(xXF86DRICloseConnectionReq);
    REQUEST_SIZE_MATCH(xXF86DRICloseConnectionReq);

    ScreenPtr pScreen = dixGetScreenPtr(stuff->screen);
    if (!pScreen) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    DRICloseConnection(pScreen);
    return Success;
}

static int
ProcXF86DRIGetClientDriverName(register ClientPtr client)
{
    char *clientDriverName = NULL;

    REQUEST(xXF86DRIGetClientDriverNameReq);
    REQUEST_SIZE_MATCH(xXF86DRIGetClientDriverNameReq);

    ScreenPtr pScreen = dixGetScreenPtr(stuff->screen);
    if (!pScreen) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    xXF86DRIGetClientDriverNameReply reply = { 0 };

    DRIGetClientDriverName(pScreen,
                           (int *) &reply.ddxDriverMajorVersion,
                           (int *) &reply.ddxDriverMinorVersion,
                           (int *) &reply.ddxDriverPatchVersion,
                           &clientDriverName);

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };
    if (clientDriverName) {
        reply.clientDriverNameLength = strlen(clientDriverName);
        x_rpcbuf_write_CARD8s(&rpcbuf, (CARD8*)clientDriverName, reply.clientDriverNameLength);
    }

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

static int
ProcXF86DRICreateContext(register ClientPtr client)
{
    REQUEST(xXF86DRICreateContextReq);
    REQUEST_SIZE_MATCH(xXF86DRICreateContextReq);

    ScreenPtr pScreen = dixGetScreenPtr(stuff->screen);
    if (!pScreen) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    xXF86DRICreateContextReply reply = { 0 };

    if (!DRICreateContext(pScreen,
                          NULL,
                          stuff->context,
                          (drm_context_t *) &reply.hHWContext)) {
        return BadValue;
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
ProcXF86DRIDestroyContext(register ClientPtr client)
{
    REQUEST(xXF86DRIDestroyContextReq);
    REQUEST_SIZE_MATCH(xXF86DRIDestroyContextReq);

    ScreenPtr pScreen = dixGetScreenPtr(stuff->screen);
    if (!pScreen) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    if (!DRIDestroyContext(pScreen, stuff->context)) {
        return BadValue;
    }

    return Success;
}

static int
ProcXF86DRICreateDrawable(ClientPtr client)
{
    DrawablePtr pDrawable;
    int rc;

    REQUEST(xXF86DRICreateDrawableReq);
    REQUEST_SIZE_MATCH(xXF86DRICreateDrawableReq);

    ScreenPtr pScreen = dixGetScreenPtr(stuff->screen);
    if (!pScreen) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    rc = dixLookupDrawable(&pDrawable, stuff->drawable, client, 0,
                           DixReadAccess);
    if (rc != Success)
        return rc;

    xXF86DRICreateDrawableReply reply = { 0 };
    if (!DRICreateDrawable(pScreen, client,
                           pDrawable,
                           (drm_drawable_t *) &reply.hHWDrawable)) {
        return BadValue;
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
ProcXF86DRIDestroyDrawable(register ClientPtr client)
{
    REQUEST(xXF86DRIDestroyDrawableReq);
    DrawablePtr pDrawable;
    int rc;

    REQUEST_SIZE_MATCH(xXF86DRIDestroyDrawableReq);

    ScreenPtr pScreen = dixGetScreenPtr(stuff->screen);
    if (!pScreen) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    rc = dixLookupDrawable(&pDrawable, stuff->drawable, client, 0,
                           DixReadAccess);
    if (rc != Success)
        return rc;

    if (!DRIDestroyDrawable(pScreen, client,
                            pDrawable)) {
        return BadValue;
    }

    return Success;
}

static int
ProcXF86DRIGetDrawableInfo(register ClientPtr client)
{
    DrawablePtr pDrawable;
    int X, Y, W, H;
    drm_clip_rect_t *pClipRects;
    drm_clip_rect_t *pBackClipRects;
    int backX, backY, rc;

    REQUEST(xXF86DRIGetDrawableInfoReq);
    REQUEST_SIZE_MATCH(xXF86DRIGetDrawableInfoReq);

    ScreenPtr pScreen = dixGetScreenPtr(stuff->screen);
    if (!pScreen) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    rc = dixLookupDrawable(&pDrawable, stuff->drawable, client, 0,
                           DixReadAccess);
    if (rc != Success)
        return rc;

    xXF86DRIGetDrawableInfoReply reply = { 0 };

    if (!DRIGetDrawableInfo(pScreen,
                            pDrawable,
                            (unsigned int *) &reply.drawableTableIndex,
                            (unsigned int *) &reply.drawableTableStamp,
                            (int *) &X,
                            (int *) &Y,
                            (int *) &W,
                            (int *) &H,
                            (int *) &reply.numClipRects,
                            &pClipRects,
                            &backX,
                            &backY,
                            (int *) &reply.numBackClipRects,
                            &pBackClipRects)) {
        return BadValue;
    }

    reply.drawableX = X;
    reply.drawableY = Y;
    reply.drawableWidth = W;
    reply.drawableHeight = H;
    reply.backX = backX;
    reply.backY = backY;

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    if (reply.numClipRects) {
        int j = 0;

        for (int i = 0; i < reply.numClipRects; i++) {
            /* Clip cliprects to screen dimensions (redirected windows) */
            CARD16 x1 = max(pClipRects[i].x1, 0);
            CARD16 y1 = max(pClipRects[i].y1, 0);
            CARD16 x2 = min(pClipRects[i].x2, pScreen->width);
            CARD16 y2 = min(pClipRects[i].y2, pScreen->height);

            /* only write visible ones */
            if (x1 < x2 && y1 < y2) {
                x_rpcbuf_write_CARD16(&rpcbuf, x1);
                x_rpcbuf_write_CARD16(&rpcbuf, y1);
                x_rpcbuf_write_CARD16(&rpcbuf, x2);
                x_rpcbuf_write_CARD16(&rpcbuf, y2);
                j++;
            }
        }

        reply.numClipRects = j;
    }

    for (int i = 0; i < reply.numBackClipRects; i++) {
        x_rpcbuf_write_CARD16(&rpcbuf, pBackClipRects[i].x1);
        x_rpcbuf_write_CARD16(&rpcbuf, pBackClipRects[i].y1);
        x_rpcbuf_write_CARD16(&rpcbuf, pBackClipRects[i].x2);
        x_rpcbuf_write_CARD16(&rpcbuf, pBackClipRects[i].y2);
    }

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

static int
ProcXF86DRIGetDeviceInfo(register ClientPtr client)
{
    drm_handle_t hFrameBuffer;
    void *pDevPrivate;

    REQUEST(xXF86DRIGetDeviceInfoReq);
    REQUEST_SIZE_MATCH(xXF86DRIGetDeviceInfoReq);

    ScreenPtr pScreen = dixGetScreenPtr(stuff->screen);
    if (!pScreen) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    xXF86DRIGetDeviceInfoReply reply = { 0 };

    if (!DRIGetDeviceInfo(pScreen,
                          &hFrameBuffer,
                          (int *) &reply.framebufferOrigin,
                          (int *) &reply.framebufferSize,
                          (int *) &reply.framebufferStride,
                          (int *) &reply.devPrivateSize,
                          &pDevPrivate)) {
        return BadValue;
    }

    reply.hFrameBufferLow = (CARD32) (hFrameBuffer & 0xffffffff);
#if defined(LONG64) && !defined(__linux__)
    reply.hFrameBufferHigh = (CARD32) (hFrameBuffer >> 32);
#endif

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };
    x_rpcbuf_write_CARD8s(&rpcbuf, pDevPrivate, reply.devPrivateSize);

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

static int
ProcXF86DRIDispatch(register ClientPtr client)
{
    REQUEST(xReq);

    switch (stuff->data) {
    case X_XF86DRIQueryVersion:
        return ProcXF86DRIQueryVersion(client);
    case X_XF86DRIQueryDirectRenderingCapable:
        return ProcXF86DRIQueryDirectRenderingCapable(client);
    }

    if (!client->local)
        return DRIErrorBase + XF86DRIClientNotLocal;

    switch (stuff->data) {
    case X_XF86DRIOpenConnection:
        return ProcXF86DRIOpenConnection(client);
    case X_XF86DRICloseConnection:
        return ProcXF86DRICloseConnection(client);
    case X_XF86DRIGetClientDriverName:
        return ProcXF86DRIGetClientDriverName(client);
    case X_XF86DRICreateContext:
        return ProcXF86DRICreateContext(client);
    case X_XF86DRIDestroyContext:
        return ProcXF86DRIDestroyContext(client);
    case X_XF86DRICreateDrawable:
        return ProcXF86DRICreateDrawable(client);
    case X_XF86DRIDestroyDrawable:
        return ProcXF86DRIDestroyDrawable(client);
    case X_XF86DRIGetDrawableInfo:
        return ProcXF86DRIGetDrawableInfo(client);
    case X_XF86DRIGetDeviceInfo:
        return ProcXF86DRIGetDeviceInfo(client);
    case X_XF86DRIAuthConnection:
        return ProcXF86DRIAuthConnection(client);
        /* {Open,Close}FullScreen are deprecated now */
    default:
        return BadRequest;
    }
}

void
XFree86DRIExtensionInit(void)
{
    ExtensionEntry *extEntry;

    if (DRIExtensionInit() &&
        (extEntry = AddExtension(XF86DRINAME,
                                 XF86DRINumberEvents,
                                 XF86DRINumberErrors,
                                 ProcXF86DRIDispatch,
                                 ProcXF86DRIDispatch,
                                 XF86DRIResetProc, StandardMinorOpcode))) {
        DRIErrorBase = extEntry->errorBase;
    }
}
