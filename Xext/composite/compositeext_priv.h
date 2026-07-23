/* SPDX-License-Identifier: MIT OR X11 OR AGPLv3+
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_COMPOSITEEXT_PRIV_H_
#define _XSERVER_COMPOSITEEXT_PRIV_H_

#include <X11/X.h>

#include "include/xlibre_ptrtypes.h"

void compSetRedirectBorderClip(WindowPtr pWin, RegionPtr pRegion);
RegionPtr compGetRedirectBorderClip(WindowPtr pWin);

void PanoramiXCompositeInit(void);
void PanoramiXCompositeReset(void);

#endif /* _XSERVER_COMPOSITEEXT_PRIV_H_ */
