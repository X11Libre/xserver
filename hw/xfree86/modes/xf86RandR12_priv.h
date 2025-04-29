/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_XF86RANDR12_PRIV_H_
#define _XSERVER_XF86RANDR12_PRIV_H_

#include <X11/extensions/render.h>

#include "randrstr.h"
#include "xf86RandR12.h"

void xf86RandR12LoadPalette(ScrnInfoPtr pScrn, int numColors,
                            int *indices, LOCO *colors,
                            VisualPtr pVisual);
Bool xf86RandR12InitGamma(ScrnInfoPtr pScrn, unsigned gammaSize);

void xf86RandR12CloseScreen(ScreenPtr pScreen);

#endif /* _XSERVER_XF86RANDR12_PRIV_H_ */
