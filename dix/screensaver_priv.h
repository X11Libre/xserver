/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_SCREENSAVER_PRIV_H
#define _XSERVER_DIX_SCREENSAVER_PRIV_H

#include <X11/Xdefs.h>
#include <X11/Xmd.h>

extern CARD32 defaultScreenSaverTime;
extern CARD32 defaultScreenSaverInterval;
extern CARD32 ScreenSaverTime;
extern CARD32 ScreenSaverInterval;
extern Bool screenSaverSuspended;

#endif /* _XSERVER_DIX_SCREENSAVER_PRIV_H */
