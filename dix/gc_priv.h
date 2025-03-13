/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 * Copyright © 1987 by Digital Equipment Corporation, Maynard, Massachusetts.
 * Copyright © 1987, 1998  The Open Group
 */
#ifndef _XSERVER_DIX_GC_PRIV_H
#define _XSERVER_DIX_GC_PRIV_H

#include "include/gc.h"

int ChangeGCXIDs(ClientPtr client, GCPtr pGC, BITS32 mask, CARD32 * pval);

GCPtr CreateGC(DrawablePtr pDrawable,
               BITS32 mask,
               XID *pval,
               int *pStatus,
               XID gcid,
               ClientPtr client);

int CopyGC(GCPtr pgcSrc, GCPtr pgcDst, BITS32 mask);

int FreeGC(void *pGC, XID gid);

void FreeGCperDepth(ScreenPtr pScreen);

Bool CreateGCperDepth(int screenNum);

Bool CreateDefaultStipple(int screenNum);

void FreeDefaultStipple(int screenNum);

int SetDashes(GCPtr pGC, unsigned offset, unsigned ndash, unsigned char *pdash);

int VerifyRectOrder(int nrects, xRectangle *prects, int ordering);

int SetClipRects(GCPtr pGC,
                int xOrigin,
                int yOrigin,
                int nrects,
                xRectangle *prects,
                int ordering);

#endif /* _XSERVER_DIX_GC_PRIV_H */
