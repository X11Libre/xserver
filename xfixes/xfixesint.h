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
#ifndef _XFIXESINT_H_
#define _XFIXESINT_H_

#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/extensions/xfixesproto.h>

#include "dix/selection_priv.h"

#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "windowstr.h"
#include "xfixes.h"

extern int XFixesEventBase;
extern int XFixesUseXinerama;

typedef struct _XFixesClient {
    CARD32 major_version;
} XFixesClientRec, *XFixesClientPtr;

#define GetXFixesClient(pClient) ((XFixesClientPtr)dixLookupPrivate(&(pClient)->devPrivates, XFixesClientPrivateKey))

/* Save set */
int
 ProcXFixesChangeSaveSet(ClientPtr client);

/* Selection events */
int
 ProcXFixesSelectSelectionInput(ClientPtr client);

void

SXFixesSelectionNotifyEvent(xXFixesSelectionNotifyEvent * from,
                            xXFixesSelectionNotifyEvent * to);
Bool
 XFixesSelectionInit(void);

/* Cursor notification */
Bool
 XFixesCursorInit(void);

int
 ProcXFixesSelectCursorInput(ClientPtr client);

void

SXFixesCursorNotifyEvent(xXFixesCursorNotifyEvent * from,
                         xXFixesCursorNotifyEvent * to);

int
 ProcXFixesGetCursorImage(ClientPtr client);

/* Cursor names (Version 2) */

int
 ProcXFixesSetCursorName(ClientPtr client);

int
 ProcXFixesGetCursorName(ClientPtr client);

int
 ProcXFixesGetCursorImageAndName(ClientPtr client);

/* Cursor replacement (Version 2) */

int
 ProcXFixesChangeCursor(ClientPtr client);

int
 ProcXFixesChangeCursorByName(ClientPtr client);

/* Region objects (Version 2* */
Bool
 XFixesRegionInit(void);

int
 ProcXFixesCreateRegion(ClientPtr client);

int
 ProcXFixesCreateRegionFromBitmap(ClientPtr client);

int
 ProcXFixesCreateRegionFromWindow(ClientPtr client);

int
 ProcXFixesCreateRegionFromGC(ClientPtr client);

int
 ProcXFixesCreateRegionFromPicture(ClientPtr client);

int
 ProcXFixesDestroyRegion(ClientPtr client);

int
 ProcXFixesSetRegion(ClientPtr client);

int
 ProcXFixesCopyRegion(ClientPtr client);

int
 ProcXFixesCombineRegion(ClientPtr client);

int
 ProcXFixesInvertRegion(ClientPtr client);

int
 ProcXFixesTranslateRegion(ClientPtr client);

int
 ProcXFixesRegionExtents(ClientPtr client);

int
 ProcXFixesFetchRegion(ClientPtr client);

int
 ProcXFixesSetGCClipRegion(ClientPtr client);

int
 ProcXFixesSetWindowShapeRegion(ClientPtr client);

int
 ProcXFixesSetPictureClipRegion(ClientPtr client);

int
 ProcXFixesExpandRegion(ClientPtr client);

/* Cursor Visibility (Version 4) */

int
 ProcXFixesHideCursor(ClientPtr client);

int
 ProcXFixesShowCursor(ClientPtr client);

/* Version 5 */

int
 ProcXFixesCreatePointerBarrier(ClientPtr client);

int
 ProcXFixesDestroyPointerBarrier(ClientPtr client);

/* Version 6 */

Bool
 XFixesClientDisconnectInit(void);

int
 ProcXFixesSetClientDisconnectMode(ClientPtr client);

int
 ProcXFixesGetClientDisconnectMode(ClientPtr client);

Bool
 XFixesShouldDisconnectClient(ClientPtr client);

/* Xinerama */
#ifdef XINERAMA
void PanoramiXFixesInit(void);
void PanoramiXFixesReset(void);
#endif /* XINERAMA */

#endif                          /* _XFIXESINT_H_ */
