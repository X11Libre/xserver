/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_COLORMAP_PRIV_H
#define _XSERVER_DIX_COLORMAP_PRIV_H

#include <X11/Xdefs.h>

#include "dix/screenint_priv.h"
#include "include/colormap.h"

typedef struct _CMEntry *EntryPtr;

int CreateColormap(Colormap mid, ScreenPtr pScreen, VisualPtr pVisual,
                   ColormapPtr *ppcmap, int alloc, int client);

/* should only be called via resource type's destructor */
int FreeColormap(void *pmap, XID mid);

#endif /* _XSERVER_DIX_COLORMAP_PRIV_H */
