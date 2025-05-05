/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef XORG_FB_PRIV_H
#define XORG_FB_PRIV_H

#include <X11/Xdefs.h>

#include "include/scrnintstr.h"
#include "fb/fb.h"

#define FbBitsStrideToStipStride(s) (((s) << (FB_SHIFT - FB_STIP_SHIFT)))

#define fbGetGCPrivateKey(pGC) (&fbGetScreenPrivate((pGC)->pScreen)->gcPrivateKeyRec)
#define fbGetGCPrivate(pGC) ((FbGCPrivPtr)dixLookupPrivate(&(pGC)->devPrivates, fbGetGCPrivateKey(pGC)))

#define fbGetScreenPixmap(s)    ((PixmapPtr) (s)->devPrivate)

#ifdef FB_DEBUG

#define FB_HEAD_BITS   (FbStip) (0xbaadf00d)
#define FB_TAIL_BITS   (FbStip) (0xbaddf0ad)

void fbValidateDrawable(DrawablePtr d);
void fbSetBits(FbStip * bits, int stride, FbStip data);

#else

static inline void fbValidateDrawable(DrawablePtr d) {}

#endif /* FB_DEBUG */

Bool fbAllocatePrivates(ScreenPtr pScreen);
int  fbListInstalledColormaps(ScreenPtr pScreen, Colormap* pmaps);

#endif /* XORG_FB_PRIV_H */
