/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef XNEST_SCREEN_PRIV_H
#define XNEST_SCREEN_PRIV_H

#include <xcb/xcb.h>

#include "include/privates.h"
#include "include/scrnintstr.h"

typedef struct _xnestScreenPrivate {
    xcb_window_t defaultWindow;
    xcb_window_t screenSaverWindow;
} XnestScreenPrivate;

extern DevPrivateKeyRec xnestScreenPrivateKeyRec;

static inline XnestScreenPrivate *xnestGetScreenPrivate(ScreenPtr pScreen) {
    return dixLookupPrivate(&pScreen->devPrivates, &xnestScreenPrivateKeyRec);
}

#endif /* XNEST_SCREEN_PRIV_H */
