/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_REGION_PRIV_H
#define _XSERVER_DIX_REGION_PRIV_H

#include "include/regionstr.h"

void InitRegions(void);
Bool RegionRectAlloc(RegionPtr pRgn, int n);
Bool RegionIsValid(RegionPtr prgn);
void RegionPrint(RegionPtr pReg);

#endif /* _XSERVER_DIX_REGION_PRIV_H */
