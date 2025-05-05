/*
 *
 * Copyright © 2000 SuSE, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */

#ifndef _FBOVERLAY_H_
#define _FBOVERLAY_H_

#include "privates.h"

extern _X_EXPORT DevPrivateKey fbOverlayGetScreenPrivateKey(void);

#ifndef FB_OVERLAY_MAX
#define FB_OVERLAY_MAX	2
#endif

typedef void (*fbOverlayPaintKeyProc) (DrawablePtr, RegionPtr, CARD32, int);

typedef struct _fbOverlayLayer {
    union {
        struct {
            void *pbits;
            int width;
            int depth;
        } init;
        struct {
            PixmapPtr pixmap;
            RegionRec region;
        } run;
    } u;
    CARD32 key;                 /* special pixel value */
} FbOverlayLayer;

typedef struct _fbOverlayScrPriv {
    int nlayers;
    fbOverlayPaintKeyProc PaintKey;
    miCopyProc CopyWindow;
    FbOverlayLayer layer[FB_OVERLAY_MAX];
} FbOverlayScrPrivRec, *FbOverlayScrPrivPtr;

#define fbOverlayGetScrPriv(s) \
    dixLookupPrivate(&(s)->devPrivates, fbOverlayGetScreenPrivateKey())

extern _X_EXPORT Bool

fbOverlayFinishScreenInit(ScreenPtr pScreen,
                          void *pbits1,
                          void *pbits2,
                          int xsize,
                          int ysize,
                          int dpix,
                          int dpiy,
                          int width1,
                          int width2,
                          int bpp1, int bpp2, int depth1, int depth2);

#endif                          /* _FBOVERLAY_H_ */
