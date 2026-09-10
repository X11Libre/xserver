/*
 * Copyright © 2004 Keith Packard
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

#ifndef _FBDEV_H_
#define _FBDEV_H_
#include <stdio.h>
#include <unistd.h>
#include "kglamor.h"

#ifdef RANDR
#include "randrstr.h"
#endif

typedef struct _fakePriv {
    CARD8 *base;
    int bytes_per_line;
} FakePriv;

typedef struct _fakeScrPriv {
    Rotation randr;
    Bool shadow;
#ifdef GLAMOR
    int dri_fd;
#endif
} FakeScrPriv;

typedef struct _fakeScreenConf {
    int shadow;
    KdGlamorInfo glamor_info;
    const char *dri_path;
} FakeScreenConf;

extern KdCardFuncs fakeFuncs;

Bool
 fakeInitialize(KdCardInfo * card, FakePriv * priv);

Bool
 fakeCardInit(KdCardInfo * card);

Bool
 fakeScreenInit(KdScreenInfo * screen);

Bool
 fakeScreenInitialize(KdScreenInfo * screen, FakeScrPriv * scrpriv);

Bool
 fakeInitScreen(ScreenPtr pScreen);

Bool
 fakeFinishInitScreen(ScreenPtr pScreen);

Bool
 fakeCreateResources(ScreenPtr pScreen);

void
 fakePreserve(KdCardInfo * card);

Bool
 fakeEnable(ScreenPtr pScreen);

Bool
 fakeDPMS(ScreenPtr pScreen, int mode);

void
 fakeDisable(ScreenPtr pScreen);

void
 fakeRestore(KdCardInfo * card);

void
 fakeScreenFini(KdScreenInfo * screen);

void
 fakeCardFini(KdCardInfo * card);

void
 fakeGetColors(ScreenPtr pScreen, int n, xColorItem * pdefs);

void
 fakePutColors(ScreenPtr pScreen, int n, xColorItem * pdefs);

Bool
 fakeMapFramebuffer(KdScreenInfo * screen);

void *fakeWindowLinear(ScreenPtr pScreen,
                       CARD32 row,
                       CARD32 offset, int mode, CARD32 *size, void *closure);

void
 fakeSetScreenSizes(ScreenPtr pScreen);

Bool
 fakeUnmapFramebuffer(KdScreenInfo * screen);

Bool
 fakeSetShadow(ScreenPtr pScreen);

Bool
 fakeCreateColormap(ColormapPtr pmap);

#ifdef RANDR
Bool
 fakeRandRGetInfo(ScreenPtr pScreen, Rotation * rotations);

Bool

fakeRandRSetConfig(ScreenPtr pScreen,
                   Rotation randr, int rate, RRScreenSizePtr pSize);
Bool
 fakeRandRInit(ScreenPtr pScreen);

#endif

#ifdef GLAMOR
Bool fakeInitAccel(ScreenPtr screen);

void fakeEnableAccel(ScreenPtr screen);

void fakeDisableAccel(ScreenPtr screen);

void fakeFiniAccel(ScreenPtr screen);
#endif

#endif                          /* _FBDEV_H_ */
