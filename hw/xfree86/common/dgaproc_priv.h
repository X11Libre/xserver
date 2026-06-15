/* SPDX-License-Identifier: MIT OR X11 OR AGPL-3.0-or-later
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef __XSERVER_XFREE86_DGAPROC_H
#define __XSERVER_XFREE86_DGAPROC_H

#include "screenint.h"
#include "input.h"

Bool DGAScreenAvailable(ScreenPtr pScreen);
Bool DGAActive(int Index);

Bool DGAVTSwitch(void);
Bool DGAStealButtonEvent(DeviceIntPtr dev, int Index, int button, int is_down);
Bool DGAStealMotionEvent(DeviceIntPtr dev, int Index, int dx, int dy);
Bool DGAStealKeyEvent(DeviceIntPtr dev, int Index, int key_code, int is_down);

#endif /* __XSERVER_XFREE86_DGAPROC_H */
