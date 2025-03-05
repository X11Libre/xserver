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

/* Values for the flags field of a colormap. These should have 1 bit set
 * and not overlap */
#define CM_IsDefault 1
#define CM_AllAllocated 2
#define CM_BeingCreated 4

typedef struct _CMEntry *EntryPtr;

int dixCreateColormap(Colormap mid, ScreenPtr pScreen, VisualPtr pVisual,
                      ColormapPtr *ppcmap, int alloc, ClientPtr client);

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

/* should only be called via resource type's destructor */
int FreeClientPixels(void *pcr, XID fakeid);

int AllocColorCells(ClientPtr pClient, ColormapPtr pmap, int colors, int planes,
                    Bool contig, Pixel *ppix, Pixel *masks);

int AllocColorPlanes(int client, ColormapPtr pmap, int colors, int r, int g,
                     int b, Bool contig, Pixel *pixels, Pixel *prmask,
                     Pixel *pgmask, Pixel *pbmask);

int FreeColors(ColormapPtr pmap, int client, int count, Pixel *pixels, Pixel mask);

int StoreColors(ColormapPtr pmap, int count, xColorItem * defs, ClientPtr client);

int IsMapInstalled(Colormap map, WindowPtr pWin);

#endif /* _XSERVER_DIX_COLORMAP_PRIV_H */
