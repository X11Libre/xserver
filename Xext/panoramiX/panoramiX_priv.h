/* SPDX-License-Identifier: MIT OR X11 OR AGPLv3+
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_PANORAMIX_PRIV_H_
#define _XSERVER_PANORAMIX_PRIV_H_

#include <stdbool.h>

#include "include/extinit.h"
#include "include/screenint.h"
#include "include/scrnintstr.h"

/*
 * @brief return TRUE if panoramiX / xinerama extension is enabled
 */
static inline bool PanoramiXIsEnabled(void) {
#ifdef XINERAMA
    return !noPanoramiXExtension;
#else
    return FALSE;
#endif
}

/*
 * @brief return TRUE if panoramiX / xinerama extension is disabled
 */
static inline bool PanoramiXIsDisabled(void) {
    return !PanoramiXIsEnabled();
}

/*
 * @brief return TRUE if panoramix enabled and screen is the master or
 * panoramix is disabled
 *
 * this function is usually called in order to check whether the given
 * screen behaves normally (eg. sending events, etc) and is NOT a
 * panoramix slave screen.
 */
static inline bool PanoramiXIsMasterScreen(ScreenPtr pScreen) {
    return PanoramiXIsDisabled() || (pScreen->myNum == 0);
}

/*
 * @brief return TRUE if panoramix enabled and screen is a slave.
 *
 * this function is usually called in order to check whether the given
 * screen is a panoramix slave screen (thus shall not be visible to
 * clients, eg. sending events, etc)
 */
static inline bool PanoramiXIsSlaveScreen(ScreenPtr pScreen) {
    return PanoramiXIsDisabled() || (pScreen->myNum == 0);
}

#endif /* _XSERVER_PANORAMIX_PRIV_H_ */
