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

#include "include/cursor.h"
#include "include/dix.h"

extern CursorPtr rootCursor;

/* reference counting */
CursorPtr RefCursor(CursorPtr cursor);
CursorPtr UnrefCursor(CursorPtr cursor);
int CursorRefCount(const CursorPtr cursor);

int AllocARGBCursor(unsigned char *psrcbits,
                    unsigned char *pmaskbits,
                    CARD32 *argb,
                    CursorMetricPtr cm,
                    unsigned foreRed,
                    unsigned foreGreen,
                    unsigned foreBlue,
                    unsigned backRed,
                    unsigned backGreen,
                    unsigned backBlue,
                    CursorPtr *ppCurs,
                    ClientPtr client,
                    XID cid);

int AllocGlyphCursor(Font source,
                     unsigned int sourceChar,
                     Font mask,
                     unsigned int maskChar,
                     unsigned foreRed,
                     unsigned foreGreen,
                     unsigned foreBlue,
                     unsigned backRed,
                     unsigned backGreen,
                     unsigned backBlue,
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

#endif /* _XSERVER_DIX_CURSOR_PRIV_H */
