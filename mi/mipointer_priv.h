/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_MI_MIPOINTER_PRIV_H
#define _XSERVER_MI_MIPOINTER_PRIV_H

#include <X11/Xdefs.h>

#include "dix/screenint_priv.h"
#include "mi/mipointer.h"

Bool miPointerInitialize(ScreenPtr pScreen, miPointerSpriteFuncPtr spriteFuncs,
                         miPointerScreenFuncPtr screenFuncs, Bool waitForUpdate);

#endif /* _XSERVER_MI_MIPOINTER_PRIV_H */
