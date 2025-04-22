/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef XORG_FB_PRIV_H
#define XORG_FB_PRIV_H

#include <X11/Xdefs.h>

#include "include/scrnintstr.h"
#include "fb/fb.h"

#ifdef FB_DEBUG

void fbValidateDrawable(DrawablePtr d);
void fbSetBits(FbStip * bits, int stride, FbStip data);

#else

static inline void fbValidateDrawable(DrawablePtr d) {}

#endif /* FB_DEBUG */

Bool fbAllocatePrivates(ScreenPtr pScreen);

#endif /* XORG_FB_PRIV_H */
