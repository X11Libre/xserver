/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_MI_MIPOINTER_PRIV_H
#define _XSERVER_MI_MIPOINTER_PRIV_H

#include <X11/Xdefs.h>

#include "dix/screenint_priv.h"
#include "include/input.h"
#include "mi/mipointer.h"

Bool miPointerInitialize(ScreenPtr pScreen, miPointerSpriteFuncPtr spriteFuncs,
                         miPointerScreenFuncPtr screenFuncs, Bool waitForUpdate);
void miPointerWarpCursor(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y);
void miPointerSetScreen(DeviceIntPtr pDev, int screen_num, int x, int y);
void miPointerUpdateSprite(DeviceIntPtr pDev);

 /* Invalidate current sprite, forcing reload on next
  * sprite setting (window crossing, grab action, etc)
  */
void miPointerInvalidateSprite(DeviceIntPtr pDev);

#endif /* _XSERVER_MI_MIPOINTER_PRIV_H */
