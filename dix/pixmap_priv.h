/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_PIXMAP_PRIV_H
#define _XSERVER_DIX_PIXMAP_PRIV_H

#include "include/pixmap.h"
#include "include/regionstr.h"
#include "include/screenint.h"

Bool PixmapScreenInit(ScreenPtr pScreen);

/* for DRI2 module */ _X_EXPORT
PixmapPtr PixmapShareToSecondary(PixmapPtr pixmap, ScreenPtr secondary);

/* for DRI2 module */ _X_EXPORT
void PixmapUnshareSecondaryPixmap(PixmapPtr secondary_pixmap);

/* for modesetting module */ _X_EXPORT
void PixmapDirtyCopyArea(PixmapPtr dst, DrawablePtr src,
                         int x, int y, int dst_x, int dst_y,
                         RegionPtr dirty_region);

#endif /* _XSERVER_DIX_PIXMAP_PRIV_H */
