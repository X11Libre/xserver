/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_SCREENSAVER_PRIV_H
#define _XSERVER_DIX_SCREENSAVER_PRIV_H

#include <stdbool.h>
#include <X11/Xdefs.h>
#include <X11/Xmd.h>

#include "include/callback.h"
#include "include/dix.h"
#include "include/screenint.h"
#include "include/scrnintstr.h"

extern CARD32 defaultScreenSaverTime;
extern CARD32 defaultScreenSaverInterval;
extern CARD32 ScreenSaverTime;
extern CARD32 ScreenSaverInterval;
extern Bool screenSaverSuspended;

extern CallbackListPtr ScreenSaverAccessCallback;

typedef struct {
    ClientPtr client;
    ScreenPtr screen;
    Mask access_mode;
    int status;
} ScreenSaverAccessCallbackParam;

static inline int dixCallScreensaverAccessCallback(ClientPtr client,
                                                   ScreenPtr screen,
                                                   Mask access_mode)
{
    ScreenSaverAccessCallbackParam rec = { client, screen, access_mode, Success };
    CallCallbacks(&ScreenSaverAccessCallback, &rec);
    return rec.status;
}

extern int screenIsSaved;

static inline bool HasSaverWindow(ScreenPtr pScreen) {
    return (pScreen->screensaver.pWindow != NullWindow);
}

#endif /* _XSERVER_DIX_SCREENSAVER_PRIV_H */
