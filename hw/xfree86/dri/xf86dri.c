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

#include "xf86.h"
#include "misc.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "cursorstr.h"
#include "scrnintstr.h"
#include "servermd.h"
#define _XF86DRI_SERVER_
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
    REQUEST_HEAD_STRUCT(xXF86DRIQueryVersionReq);

    xXF86DRIQueryVersionReply rep = {
        .majorVersion = SERVER_XF86DRI_MAJOR_VERSION,
        .minorVersion = SERVER_XF86DRI_MINOR_VERSION,
        .patchVersion = SERVER_XF86DRI_PATCH_VERSION
    };

    REPLY_FIELD_CARD16(majorVersion);
    REPLY_FIELD_CARD16(minorVersion);
    REPLY_FIELD_CARD32(patchVersion);
    REPLY_SEND();
    return Success;
}

static int
ProcXF86DRIQueryDirectRenderingCapable(register ClientPtr client)
{
    REQUEST_HEAD_STRUCT(xXF86DRIQueryDirectRenderingCapableReq);
    REQUEST_FIELD_CARD32(screen);

    Bool isCapable;

    if (stuff->screen >= screenInfo.numScreens) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    if (!DRIQueryDirectRenderingCapable(screenInfo.screens[stuff->screen],
                                        &isCapable)) {
        return BadValue;
    }

    if (!client->local || client->swapped)
        isCapable = 0;

    xXF86DRIQueryDirectRenderingCapableReply rep = {
        .isCapable = isCapable
    };

    REPLY_SEND();
    return Success;
}

static int
ProcXF86DRIOpenConnection(register ClientPtr client)
{
    REQUEST_HEAD_STRUCT(xXF86DRIOpenConnectionReq);

    drm_handle_t hSAREA;
    char *busIdString;
    CARD32 busIdStringLength = 0;

    if (stuff->screen >= screenInfo.numScreens) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    if (!DRIOpenConnection(screenInfo.screens[stuff->screen],
                           &hSAREA, &busIdString)) {
        return BadValue;
    }

    if (busIdString)
        busIdStringLength = strlen(busIdString);

    xXF86DRIOpenConnectionReply rep = {
        .length = bytes_to_int32(SIZEOF(xXF86DRIOpenConnectionReply) -
                                 SIZEOF(xGenericReply) +
                                 pad_to_int32(busIdStringLength)),
        .busIdStringLength = busIdStringLength,

        .hSAREALow = (CARD32) (hSAREA & 0xffffffff),
#if defined(LONG64) && !defined(__linux__)
        .hSAREAHigh = (CARD32) (hSAREA >> 32),
#endif
    };

    REPLY_SEND_EXTRA(busIdString, busIdStringLength);
    return Success;
}

static int
ProcXF86DRIAuthConnection(register ClientPtr client)
{
    REQUEST_HEAD_STRUCT(xXF86DRIAuthConnectionReq);

    xXF86DRIAuthConnectionReply rep = {
        .authenticated = 1
    };

    if (stuff->screen >= screenInfo.numScreens) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    if (!DRIAuthConnection(screenInfo.screens[stuff->screen], stuff->magic)) {
        ErrorF("Failed to authenticate %lu\n", (unsigned long) stuff->magic);
        rep.authenticated = 0;
    }

    REPLY_SEND();
    return Success;
}

static int
ProcXF86DRICloseConnection(register ClientPtr client)
{
    REQUEST_HEAD_STRUCT(xXF86DRICloseConnectionReq);

    if (stuff->screen >= screenInfo.numScreens) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    DRICloseConnection(screenInfo.screens[stuff->screen]);

    return Success;
}

static int
ProcXF86DRIGetClientDriverName(register ClientPtr client)
{
    REQUEST_HEAD_STRUCT(xXF86DRIGetClientDriverNameReq);

    xXF86DRIGetClientDriverNameReply rep = { 0 };
    char *clientDriverName;

    if (stuff->screen >= screenInfo.numScreens) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    DRIGetClientDriverName(screenInfo.screens[stuff->screen],
                           (int *) &rep.ddxDriverMajorVersion,
                           (int *) &rep.ddxDriverMinorVersion,
                           (int *) &rep.ddxDriverPatchVersion,
                           &clientDriverName);

    if (clientDriverName)
        rep.clientDriverNameLength = strlen(clientDriverName);
    rep.length = bytes_to_int32(SIZEOF(xXF86DRIGetClientDriverNameReply) -
                                SIZEOF(xGenericReply) +
                                pad_to_int32(rep.clientDriverNameLength));

    REPLY_SEND_EXTRA(clientDriverName, rep.clientDriverNameLength);
    return Success;
}

static int
ProcXF86DRICreateContext(register ClientPtr client)
{
    REQUEST_HEAD_STRUCT(xXF86DRICreateContextReq);

    xXF86DRICreateContextReply rep = { 0 };
    ScreenPtr pScreen;

    if (stuff->screen >= screenInfo.numScreens) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    pScreen = screenInfo.screens[stuff->screen];

    if (!DRICreateContext(pScreen,
                          NULL,
                          stuff->context, (drm_context_t *) &rep.hHWContext)) {
        return BadValue;
    }

    REPLY_SEND();
    return Success;
}

static int
ProcXF86DRIDestroyContext(register ClientPtr client)
{
    REQUEST_HEAD_STRUCT(xXF86DRIDestroyContextReq);

    if (stuff->screen >= screenInfo.numScreens) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    if (!DRIDestroyContext(screenInfo.screens[stuff->screen], stuff->context)) {
        return BadValue;
    }

    return Success;
}

static int
ProcXF86DRICreateDrawable(ClientPtr client)
{
    REQUEST_HEAD_STRUCT(xXF86DRICreateDrawableReq);

    xXF86DRICreateDrawableReply rep = { 0 };
    DrawablePtr pDrawable;
    int rc;

    if (stuff->screen >= screenInfo.numScreens) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    rc = dixLookupDrawable(&pDrawable, stuff->drawable, client, 0,
                           DixReadAccess);
    if (rc != Success)
        return rc;

    if (!DRICreateDrawable(screenInfo.screens[stuff->screen], client,
                           pDrawable, (drm_drawable_t *) &rep.hHWDrawable)) {
        return BadValue;
    }

    REPLY_SEND();
    return Success;
}

static int
ProcXF86DRIDestroyDrawable(register ClientPtr client)
{
    REQUEST_HEAD_STRUCT(xXF86DRIDestroyDrawableReq);

    DrawablePtr pDrawable;
    int rc;

    if (stuff->screen >= screenInfo.numScreens) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    rc = dixLookupDrawable(&pDrawable, stuff->drawable, client, 0,
                           DixReadAccess);
    if (rc != Success)
        return rc;

    if (!DRIDestroyDrawable(screenInfo.screens[stuff->screen], client,
                            pDrawable)) {
        return BadValue;
    }

    return Success;
}

static int
ProcXF86DRIGetDrawableInfo(register ClientPtr client)
{
    REQUEST_HEAD_STRUCT(xXF86DRIGetDrawableInfoReq);

    xXF86DRIGetDrawableInfoReply rep = { 0 };
    DrawablePtr pDrawable;
    int X, Y, W, H;
    drm_clip_rect_t *pClipRects, *pClippedRects = NULL;
    drm_clip_rect_t *pBackClipRects;
    int backX, backY, rc;

    if (stuff->screen >= screenInfo.numScreens) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    rc = dixLookupDrawable(&pDrawable, stuff->drawable, client, 0,
                           DixReadAccess);
    if (rc != Success)
        return rc;

    if (!DRIGetDrawableInfo(screenInfo.screens[stuff->screen],
                            pDrawable,
                            (unsigned int *) &rep.drawableTableIndex,
                            (unsigned int *) &rep.drawableTableStamp,
                            (int *) &X,
                            (int *) &Y,
                            (int *) &W,
                            (int *) &H,
                            (int *) &rep.numClipRects,
                            &pClipRects,
                            &backX,
                            &backY,
                            (int *) &rep.numBackClipRects, &pBackClipRects)) {
        return BadValue;
    }

    rep.drawableX = X;
    rep.drawableY = Y;
    rep.drawableWidth = W;
    rep.drawableHeight = H;

    rep.backX = backX;
    rep.backY = backY;

    if (rep.numBackClipRects)
        rep.length += sizeof(drm_clip_rect_t) * rep.numBackClipRects;

    if (rep.numClipRects) {
        /* Clip cliprects to screen dimensions (redirected windows) */
        pClippedRects = calloc(rep.numClipRects, sizeof(drm_clip_rect_t));

        if (!pClippedRects)
            return BadAlloc;

        ScreenPtr pScreen = screenInfo.screens[stuff->screen];
        int i, j;

        for (i = 0, j = 0; i < rep.numClipRects; i++) {
            pClippedRects[j] = (drm_clip_rect_t) {
                .x1 = max(pClipRects[i].x1, 0),
                .y1 = max(pClipRects[i].y1, 0),
                .x2 = min(pClipRects[i].x2, pScreen->width),
                .y2 = min(pClipRects[i].y2, pScreen->height),
            };

            if (pClippedRects[j].x1 < pClippedRects[j].x2 &&
                pClippedRects[j].y1 < pClippedRects[j].y2) {
                j++;
            }
        }
        rep.numClipRects = j;
    }

    REPLY_SEND_EXTRA_2(pClippedRects, sizeof(drm_clip_rect_t) * rep.numClipRects,
                       pBackClipRects, sizeof(drm_clip_rect_t) * rep.numBackClipRects);
    return Success;
}

static int
ProcXF86DRIGetDeviceInfo(register ClientPtr client)
{
    REQUEST_HEAD_STRUCT(xXF86DRIGetDeviceInfoReq);

    xXF86DRIGetDeviceInfoReply rep = { 0 };
    drm_handle_t hFrameBuffer;
    void *pDevPrivate = NULL;

    if (stuff->screen >= screenInfo.numScreens) {
        client->errorValue = stuff->screen;
        return BadValue;
    }

    if (!DRIGetDeviceInfo(screenInfo.screens[stuff->screen],
                          &hFrameBuffer,
                          (int *) &rep.framebufferOrigin,
                          (int *) &rep.framebufferSize,
                          (int *) &rep.framebufferStride,
                          (int *) &rep.devPrivateSize, &pDevPrivate)) {
        return BadValue;
    }

    rep.hFrameBufferLow = (CARD32) (hFrameBuffer & 0xffffffff);
#if defined(LONG64) && !defined(__linux__)
    rep.hFrameBufferHigh = (CARD32) (hFrameBuffer >> 32);
#endif

    REPLY_SEND_EXTRA(pDevPrivate, rep.devPrivateSize);
    return Success;
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

static int _X_COLD
SProcXF86DRIDispatch(register ClientPtr client)
{
    REQUEST(xReq);

    /*
     * Only local clients are allowed DRI access, but remote clients still need
     * these requests to find out cleanly.
     */
    switch (stuff->data) {
    case X_XF86DRIQueryVersion:
        return ProcXF86DRIQueryVersion(client);
    case X_XF86DRIQueryDirectRenderingCapable:
        return ProcXF86DRIQueryDirectRenderingCapable(client);
    default:
        return DRIErrorBase + XF86DRIClientNotLocal;
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
                                 SProcXF86DRIDispatch,
                                 XF86DRIResetProc, StandardMinorOpcode))) {
        DRIErrorBase = extEntry->errorBase;
    }
}
