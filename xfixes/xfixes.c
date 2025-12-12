/*
 * Copyright (c) 2006, Oracle and/or its affiliates.
 * Copyright 2010, 2021 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Copyright © 2002 Keith Packard
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
#include "miext/extinit_priv.h"
#include "os/fmt.h"
#include "os/mathx_priv.h"

#include "xfixesint.h"
#include "protocol-versions.h"

Bool noXFixesExtension = FALSE;

static unsigned char XFixesReqCode;
int XFixesEventBase;
int XFixesErrorBase;

static DevPrivateKeyRec XFixesClientPrivateKeyRec;

#define XFixesClientPrivateKey (&XFixesClientPrivateKeyRec)

static int
ProcXFixesQueryVersion(ClientPtr client)
{
    int major, minor;
    XFixesClientPtr pXFixesClient = GetXFixesClient(client);

    REQUEST(xXFixesQueryVersionReq);
    REQUEST_SIZE_MATCH(xXFixesQueryVersionReq);

    if (client->swapped) {
        swapl(&stuff->majorVersion);
        swapl(&stuff->minorVersion);
    }

    if (version_compare(stuff->majorVersion, stuff->minorVersion,
                        SERVER_XFIXES_MAJOR_VERSION,
                        SERVER_XFIXES_MINOR_VERSION) < 0) {
        major = MAX(pXFixesClient->major_version, stuff->majorVersion);
        minor = stuff->minorVersion;
    }
    else {
        major = SERVER_XFIXES_MAJOR_VERSION;
        minor = SERVER_XFIXES_MINOR_VERSION;
    }

    pXFixesClient->major_version = major;

    xXFixesQueryVersionReply reply = {
        .majorVersion = MIN(stuff->majorVersion, major),
        .minorVersion = minor
    };

    if (client->swapped) {
        swapl(&reply.majorVersion);
        swapl(&reply.minorVersion);
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

/* Major version controls available requests */
static const int version_requests[] = {
    X_XFixesQueryVersion,       /* before client sends QueryVersion */
    X_XFixesGetCursorImage,     /* Version 1 */
    X_XFixesChangeCursorByName, /* Version 2 */
    X_XFixesExpandRegion,       /* Version 3 */
    X_XFixesShowCursor,         /* Version 4 */
    X_XFixesDestroyPointerBarrier,      /* Version 5 */
    X_XFixesGetClientDisconnectMode,    /* Version 6 */
};

static int
ProcXFixesDispatch(ClientPtr client)
{
    REQUEST(xReq);
    XFixesClientPtr pXFixesClient = GetXFixesClient(client);

    if (pXFixesClient->major_version >= ARRAY_SIZE(version_requests))
        return BadRequest;
    if (stuff->data > version_requests[pXFixesClient->major_version])
        return BadRequest;

    switch (stuff->data) {
        /*************** Version 1 ******************/
        case X_XFixesQueryVersion:
            return ProcXFixesQueryVersion(client);
        case X_XFixesChangeSaveSet:
            return ProcXFixesChangeSaveSet(client);
        case X_XFixesSelectSelectionInput:
            return ProcXFixesSelectSelectionInput(client);
        case X_XFixesSelectCursorInput:
            return ProcXFixesSelectCursorInput(client);
        case X_XFixesGetCursorImage:
            return ProcXFixesGetCursorImage(client);

        /*************** Version 2 ******************/
        case X_XFixesCreateRegion:
            return ProcXFixesCreateRegion(client);
        case X_XFixesCreateRegionFromBitmap:
            return ProcXFixesCreateRegionFromBitmap(client);
        case X_XFixesCreateRegionFromWindow:
            return ProcXFixesCreateRegionFromWindow(client);
        case X_XFixesCreateRegionFromGC:
            return ProcXFixesCreateRegionFromGC(client);
        case X_XFixesCreateRegionFromPicture:
            return ProcXFixesCreateRegionFromPicture(client);
        case X_XFixesDestroyRegion:
            return ProcXFixesDestroyRegion(client);
        case X_XFixesSetRegion:
            return ProcXFixesSetRegion(client);
        case X_XFixesCopyRegion:
            return ProcXFixesCopyRegion(client);
        case X_XFixesUnionRegion:
            return ProcXFixesCombineRegion(client);
        case X_XFixesIntersectRegion:
            return ProcXFixesCombineRegion(client);
        case X_XFixesSubtractRegion:
            return ProcXFixesCombineRegion(client);
        case X_XFixesInvertRegion:
            return ProcXFixesInvertRegion(client);
        case X_XFixesTranslateRegion:
            return ProcXFixesTranslateRegion(client);
        case X_XFixesRegionExtents:
            return ProcXFixesRegionExtents(client);
        case X_XFixesFetchRegion:
            return ProcXFixesFetchRegion(client);
        case X_XFixesSetGCClipRegion:
            return ProcXFixesSetGCClipRegion(client);
        case X_XFixesSetWindowShapeRegion:
            return ProcXFixesSetWindowShapeRegion(client);
        case X_XFixesSetPictureClipRegion:
            return ProcXFixesSetPictureClipRegion(client);
        case X_XFixesSetCursorName:
            return ProcXFixesSetCursorName(client);
        case X_XFixesGetCursorName:
            return ProcXFixesGetCursorName(client);
        case X_XFixesGetCursorImageAndName:
            return ProcXFixesGetCursorImageAndName(client);
        case X_XFixesChangeCursor:
            return ProcXFixesChangeCursor(client);
        case X_XFixesChangeCursorByName:
            return ProcXFixesChangeCursorByName(client);

        /*************** Version 3 ******************/
        case X_XFixesExpandRegion:
            return ProcXFixesExpandRegion(client);
        /*************** Version 4 ******************/
        case X_XFixesHideCursor:
            return ProcXFixesHideCursor(client);
        case X_XFixesShowCursor:
            return ProcXFixesShowCursor(client);
        /*************** Version 5 ******************/
        case X_XFixesCreatePointerBarrier:
            return ProcXFixesCreatePointerBarrier(client);
        case X_XFixesDestroyPointerBarrier:
            return ProcXFixesDestroyPointerBarrier(client);
        /*************** Version 6 ******************/
        case X_XFixesSetClientDisconnectMode:
            return ProcXFixesSetClientDisconnectMode(client);
        case X_XFixesGetClientDisconnectMode:
            return ProcXFixesGetClientDisconnectMode(client);
        default:
            return BadRequest;
    }
}

void
XFixesExtensionInit(void)
{
    ExtensionEntry *extEntry;

    if (!dixRegisterPrivateKey
        (&XFixesClientPrivateKeyRec, PRIVATE_CLIENT, sizeof(XFixesClientRec)))
        return;

    if (XFixesSelectionInit() &&
        XFixesCursorInit() &&
        XFixesRegionInit() &&
        XFixesClientDisconnectInit() &&
        (extEntry = AddExtension(XFIXES_NAME, XFixesNumberEvents,
                                 XFixesNumberErrors,
                                 ProcXFixesDispatch, ProcXFixesDispatch,
                                 NULL, StandardMinorOpcode)) != 0) {
        XFixesReqCode = (unsigned char) extEntry->base;
        XFixesEventBase = extEntry->eventBase;
        XFixesErrorBase = extEntry->errorBase;
        EventSwapVector[XFixesEventBase + XFixesSelectionNotify] =
            (EventSwapPtr) SXFixesSelectionNotifyEvent;
        EventSwapVector[XFixesEventBase + XFixesCursorNotify] =
            (EventSwapPtr) SXFixesCursorNotifyEvent;
        SetResourceTypeErrorValue(RegionResType, XFixesErrorBase + BadRegion);
        SetResourceTypeErrorValue(PointerBarrierType,
                                  XFixesErrorBase + BadBarrier);
    }
}

#ifdef XINERAMA

int XFixesUseXinerama = 0;

void
PanoramiXFixesInit(void)
{
    XFixesUseXinerama = 1;
}

void
PanoramiXFixesReset(void)
{
    XFixesUseXinerama = 0;
}

#endif /* XINERAMA */
