/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIXFONT_PRIV_H
#define _XSERVER_DIXFONT_PRIV_H

#include "include/dixfont.h"

Bool SetDefaultFont(const char *defaultfontname);

int SetDefaultFontPath(const char *path);

int OpenFont(ClientPtr client,
             XID fid,
             Mask flags,
             unsigned lenfname,
             const char *pfontname);

int CloseFont(void *pfont, XID fid);

int SetFontPath(ClientPtr client, int npaths, unsigned char *paths);

void DeleteClientFontStuff(ClientPtr client);

/* Quartz support on Mac OS X pulls in the QuickDraw
    framework whose InitFonts function conflicts here. */
void dixInitFonts(void);
void dixFreeFonts(void);

int ListFonts(ClientPtr client,
              unsigned char *pattern,
              unsigned int length,
              unsigned int max_names);

int ImageText(ClientPtr client,
              DrawablePtr pDraw,
              GCPtr pGC,
              int nChars,
              unsigned char *data,
              int xorg,
              int yorg,
              int reqType,
              XID did);

int PolyText(ClientPtr client,
             DrawablePtr pDraw,
             GCPtr pGC,
             unsigned char *pElt,
             unsigned char *endReq,
             int xorg,
             int yorg,
             int reqType,
             XID did);

#endif /* _XSERVER_DIXFONT_PRIV_H */
