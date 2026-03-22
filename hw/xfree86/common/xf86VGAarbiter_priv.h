/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_XF86VGAARBITERPRIV_H
#define _XSERVER_XF86VGAARBITERPRIV_H

#include <X11/Xdefs.h>

#include "xf86str.h"

#ifdef XSERVER_LIBPCIACCESS

void xf86VGAarbiterInit(void);
void xf86VGAarbiterFini(void);
void xf86VGAarbiterScrnInit(ScrnInfoPtr pScrn);
Bool xf86VGAarbiterWrapFunctions(void);
void xf86VGAarbiterLock(ScrnInfoPtr pScrn);
void xf86VGAarbiterUnlock(ScrnInfoPtr pScrn);

#else /* XSERVER_LIBPCIACCESS */

static inline void xf86VGAarbiterInit(void) {}
static inline void xf86VGAarbiterFini(void) {}
static inline void xf86VGAarbiterScrnInit(ScrnInfoPtr pScrn) {}
static inline void xf86VGAarbiterWrapFunctions(void) {}
static inline void xf86VGAarbiterLock(ScrnInfoPtr pScrn) {}
static inline void xf86VGAarbiterUnlock(ScrnInfoPtr pScrn) {}

#endif /* XSERVER_LIBPCIACCESS */

Bool xf86VGAarbiterAllowDRI(ScreenPtr pScreen);

#endif /* _XSERVER_XF86VGAARBITERPRIV_H */
