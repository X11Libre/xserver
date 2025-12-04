/**************************************************************************

   Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
   Copyright 2000 VA Linux Systems, Inc.
   Copyright (c) 2002, 2009-2012 Apple Inc.
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
 *   Jens Owen <jens@valinux.com>
 *   Rickard E. (Rik) Faith <faith@valinux.com>
 *   Jeremy Huddleston <jeremyhu@apple.com>
 *
 */

#include <dix-config.h>

#include <X11/X.h>
#include <X11/Xproto.h>

#include "dix/dix_priv.h"
#include "dix/request_priv.h"
#include "dix/screenint_priv.h"

#include "misc.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "cursorstr.h"
#include "scrnintstr.h"
#include "servermd.h"
#define _APPLEDRI_SERVER_
#include "appledristr.h"
#include "swaprep.h"
#include "dri.h"
#include "dristruct.h"
#include "xpr.h"
#include "x-hash.h"
#include "protocol-versions.h"

static int DRIErrorBase = 0;

static void
AppleDRIResetProc(ExtensionEntry* extEntry);
static int
ProcAppleDRICreatePixmap(ClientPtr client);

static int DRIEventBase = 0;

static void
SNotifyEvent(xAppleDRINotifyEvent *from, xAppleDRINotifyEvent *to);

typedef struct _DRIEvent *DRIEventPtr;
typedef struct _DRIEvent {
    DRIEventPtr next;
    ClientPtr client;
    XID clientResource;
    unsigned int mask;
} DRIEventRec;

/*ARGSUSED*/
static void
AppleDRIResetProc(ExtensionEntry* extEntry)
{
    DRIReset();
}

static int
ProcAppleDRIQueryVersion(register ClientPtr client)
{
    REQUEST_SIZE_MATCH(xAppleDRIQueryVersionReq);

    xAppleDRIQueryVersionReply reply = {
        .majorVersion = SERVER_APPLEDRI_MAJOR_VERSION,
        .minorVersion = SERVER_APPLEDRI_MINOR_VERSION,
        .patchVersion = SERVER_APPLEDRI_PATCH_VERSION,
    };

    if (client->swapped) {
        swaps(&reply.majorVersion);
        swaps(&reply.minorVersion);
        swapl(&reply.patchVersion);
    }
    return X_SEND_REPLY_SIMPLE(client, reply);
}

/* surfaces */

static int
ProcAppleDRIQueryDirectRenderingCapable(register ClientPtr client)
{
    REQUEST(xAppleDRIQueryDirectRenderingCapableReq);
    REQUEST_SIZE_MATCH(xAppleDRIQueryDirectRenderingCapableReq);

    if (client->swapped)
        swapl(&stuff->screen);

    Bool isCapable;

    ScreenPtr pScreen = dixGetScreenPtr(stuff->screen);
    if (!pScreen) {
        return BadValue;
    }

    if (!DRIQueryDirectRenderingCapable(screenInfo.screens[stuff->screen],
                                        &isCapable)) {
        return BadValue;
    }

    if (!client->local)
        isCapable = FALSE;

    xAppleDRIQueryDirectRenderingCapableReply reply = {
        .isCapable = isCapable,
    };

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
ProcAppleDRIAuthConnection(register ClientPtr client)
{
    REQUEST(xAppleDRIAuthConnectionReq);
    REQUEST_SIZE_MATCH(xAppleDRIAuthConnectionReq);

    if (client->swapped) {
        swapl(&stuff->screen);
        swapl(&stuff->magic);
    }

    xAppleDRIAuthConnectionReply reply = {
        .authenticated = 1
    };

    if (!DRIAuthConnection(screenInfo.screens[stuff->screen],
                           stuff->magic)) {
        ErrorF("Failed to authenticate %u\n", (unsigned int)stuff->magic);
        reply.authenticated = 0;
    }

    if (client->swapped) {
        swapl(&reply.authenticated); /* Yes, this is a CARD32 ... sigh */
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static void
surface_notify(void *_arg,
               void *data)
{
    DRISurfaceNotifyArg *arg = _arg;
    int client_index = (int)x_cvt_vptr_to_uint(data);
    xAppleDRINotifyEvent se;

    if (client_index < 0 || client_index >= currentMaxClients)
        return;

    se.type = DRIEventBase + AppleDRISurfaceNotify;
    se.kind = arg->kind;
    se.arg = arg->id;
    se.time = currentTime.milliseconds;
    WriteEventsToClient(clients[client_index], 1, (xEvent *)&se);
}

static int
ProcAppleDRICreateSurface(ClientPtr client)
{
    REQUEST(xAppleDRICreateSurfaceReq);
    REQUEST_SIZE_MATCH(xAppleDRICreateSurfaceReq);

    if (client->swapped) {
        swapl(&stuff->screen);
        swapl(&stuff->drawable);
        swapl(&stuff->client_id);
    }

    DrawablePtr pDrawable;
    xp_surface_id sid;
    unsigned int key[2];
    int rc;

    rc = dixLookupDrawable(&pDrawable, stuff->drawable, client, 0,
                           DixReadAccess);
    if (rc != Success)
        return rc;


    if (!DRICreateSurface(screenInfo.screens[stuff->screen],
                          (Drawable)stuff->drawable, pDrawable,
                          stuff->client_id, &sid, key,
                          surface_notify,
                          x_cvt_uint_to_vptr(client->index))) {
        return BadValue;
    }

    xAppleDRICreateSurfaceReply reply = {
        .key_0 = key[0],
        .key_1 = key[1],
        .uid = sid,
    };

    if (client->swapped) {
        swapl(&reply.key_0);
        swapl(&reply.key_1);
        swapl(&reply.uid);
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
ProcAppleDRIDestroySurface(register ClientPtr client)
{
    REQUEST(xAppleDRIDestroySurfaceReq);
    REQUEST_SIZE_MATCH(xAppleDRIDestroySurfaceReq);

    if (client->swapped) {
        swapl(&stuff->screen);
        swapl(&stuff->drawable);
    }

    int rc;
    DrawablePtr pDrawable;

    rc = dixLookupDrawable(&pDrawable, stuff->drawable, client, 0,
                           DixReadAccess);
    if (rc != Success)
        return rc;

    if (!DRIDestroySurface(screenInfo.screens[stuff->screen],
                           (Drawable)stuff->drawable,
                           pDrawable, NULL, NULL)) {
        return BadValue;
    }

    return Success;
}

static int
ProcAppleDRICreatePixmap(ClientPtr client)
{
    REQUEST(xAppleDRICreatePixmapReq);
    REQUEST_SIZE_MATCH(xAppleDRICreatePixmapReq);

    if (client->swapped) {
        swapl(&stuff->screen);
        swapl(&stuff->drawable);
    }

    DrawablePtr pDrawable;
    int rc;
    char path[PATH_MAX];
    int width, height, pitch, bpp;
    void *ptr;

    REQUEST_SIZE_MATCH(xAppleDRICreatePixmapReq);

    rc = dixLookupDrawable(&pDrawable, stuff->drawable, client, 0,
                           DixReadAccess);

    if (rc != Success)
        return rc;

    if (!DRICreatePixmap(screenInfo.screens[stuff->screen],
                         (Drawable)stuff->drawable,
                         pDrawable,
                         path, PATH_MAX)) {
        return BadValue;
    }

    if (!DRIGetPixmapData(pDrawable, &width, &height,
                          &pitch, &bpp, &ptr)) {
        return BadValue;
    }

    CARD32 stringLength = strlen(path) + 1;

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };
    x_rpcbuf_write_CARD8s(&rpcbuf, path, stringLength);

    xAppleDRICreatePixmapReply reply = {
        .stringLength = stringLength,
        .width = width,
        .height = height,
        .pitch = pitch,
        .bpp = bpp,
        .size = pitch * height,
    };

    if (client->swapped) {
        swapl(&reply.stringLength);
        swapl(&reply.width);
        swapl(&reply.height);
        swapl(&reply.pitch);
        swapl(&reply.bpp);
        swapl(&reply.size);
    }

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

static int
ProcAppleDRIDestroyPixmap(ClientPtr client)
{
    REQUEST(xAppleDRIDestroyPixmapReq);
    REQUEST_SIZE_MATCH(xAppleDRIDestroyPixmapReq);

    if (client->swapped)
        swapl(&stuff->drawable);

    DrawablePtr pDrawable;
    int rc;

    rc = dixLookupDrawable(&pDrawable, stuff->drawable, client, 0,
                           DixReadAccess);

    if (rc != Success)
        return rc;

    DRIDestroyPixmap(pDrawable);

    return Success;
}

/* dispatch */

static int
ProcAppleDRIDispatch(register ClientPtr client)
{
    REQUEST(xReq);

    switch (stuff->data) {
    case X_AppleDRIQueryVersion:
        return ProcAppleDRIQueryVersion(client);

    case X_AppleDRIQueryDirectRenderingCapable:
        return ProcAppleDRIQueryDirectRenderingCapable(client);
    }

    if (!client->local)
        return DRIErrorBase + AppleDRIClientNotLocal;

    switch (stuff->data) {
    case X_AppleDRIAuthConnection:
        return ProcAppleDRIAuthConnection(client);

    case X_AppleDRICreateSurface:
        return ProcAppleDRICreateSurface(client);

    case X_AppleDRIDestroySurface:
        return ProcAppleDRIDestroySurface(client);

    case X_AppleDRICreatePixmap:
        return ProcAppleDRICreatePixmap(client);

    case X_AppleDRIDestroyPixmap:
        return ProcAppleDRIDestroyPixmap(client);

    default:
        return BadRequest;
    }
}

static void
SNotifyEvent(xAppleDRINotifyEvent *from,
             xAppleDRINotifyEvent *to)
{
    to->type = from->type;
    to->kind = from->kind;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->time, to->time);
    cpswapl(from->arg, to->arg);
}

void
AppleDRIExtensionInit(void)
{
    ExtensionEntry* extEntry;

    if (DRIExtensionInit() &&
        (extEntry = AddExtension(APPLEDRINAME,
                                 AppleDRINumberEvents,
                                 AppleDRINumberErrors,
                                 ProcAppleDRIDispatch,
                                 ProcAppleDRIDispatch,
                                 AppleDRIResetProc,
                                 StandardMinorOpcode))) {
        size_t i;
        DRIErrorBase = extEntry->errorBase;
        DRIEventBase = extEntry->eventBase;
        for (i = 0; i < AppleDRINumberEvents; i++)
            EventSwapVector[DRIEventBase + i] = (EventSwapPtr)SNotifyEvent;
    }
}
