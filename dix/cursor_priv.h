/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_CURSOR_PRIV_H
#define _XSERVER_DIX_CURSOR_PRIV_H

#include <X11/fonts/font.h>
#include <X11/X.h>
#include <X11/Xdefs.h>
#include <X11/Xmd.h>

#include "dix/screenint_priv.h"
#include "include/cursor.h"
#include "include/dix.h"
#include "include/input.h"
#include "include/window.h"

extern CursorPtr rootCursor;

/* reference counting */
CursorPtr RefCursor(CursorPtr cursor);
CursorPtr UnrefCursor(CursorPtr cursor);
int CursorRefCount(ConstCursorPtr cursor);

int AllocARGBCursor(unsigned char *psrcbits,
                    unsigned char *pmaskbits,
                    CARD32 *argb,
                    CursorMetricPtr cm,
                    unsigned short foreRed,
                    unsigned short foreGreen,
                    unsigned short foreBlue,
                    unsigned short backRed,
                    unsigned short backGreen,
                    unsigned short backBlue,
                    CursorPtr *ppCurs,
                    ClientPtr client,
                    XID cid);

int AllocGlyphCursor(Font source,
                     unsigned short sourceChar,
                     Font mask,
                     unsigned short maskChar,
                     unsigned short foreRed,
                     unsigned short foreGreen,
                     unsigned short foreBlue,
                     unsigned short backRed,
                     unsigned short backGreen,
                     unsigned short backBlue,
                     CursorPtr *ppCurs,
                     ClientPtr client,
                     XID cid);

CursorPtr CreateRootCursor(void);

int ServerBitsFromGlyph(FontPtr pfont,
                        unsigned int ch,
                        CursorMetricPtr cm,
                        unsigned char **ppbits);

Bool CursorMetricsFromGlyph(FontPtr pfont,
                            unsigned ch,
                            CursorMetricPtr cm);

void CheckCursorConfinement(WindowPtr pWin);

void NewCurrentScreen(DeviceIntPtr pDev,
                      ScreenPtr newScreen,
                      int x,
                      int y);

Bool PointerConfinedToScreen(DeviceIntPtr pDev);

void GetSpritePosition(DeviceIntPtr pDev, int *px, int *py);

#endif /* _XSERVER_DIX_CURSOR_PRIV_H */
