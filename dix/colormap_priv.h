/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_COLORMAP_PRIV_H
#define _XSERVER_DIX_COLORMAP_PRIV_H

#include <X11/Xdefs.h>
#include <X11/Xproto.h>

#include "dix/screenint_priv.h"
#include "include/colormap.h"
#include "include/dix.h"
#include "include/window.h"

typedef struct _CMEntry *EntryPtr;

int CreateColormap(Colormap mid, ScreenPtr pScreen, VisualPtr pVisual,
                   ColormapPtr *ppcmap, int alloc, int client);

/* should only be called via resource type's destructor */
int FreeColormap(void *pmap, XID mid);

int TellLostMap(WindowPtr pwin, void *value);

int TellGainedMap(WindowPtr pwin, void *value);

int CopyColormapAndFree(Colormap mid, ColormapPtr pSrc, int client);

int AllocColor(ColormapPtr pmap, unsigned short *pred, unsigned short *pgreen,
               unsigned short *pblue, Pixel *pPix, int client );

void FakeAllocColor(ColormapPtr pmap, xColorItem *item);

void FakeFreeColor(ColormapPtr pmap, Pixel pixel);

int QueryColors(ColormapPtr pmap, int count, Pixel *ppixIn,
                xrgb *prgbList, ClientPtr client);

#endif /* _XSERVER_DIX_COLORMAP_PRIV_H */
