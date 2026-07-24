/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_PANORAMIX_PRIV_H_
#define _XSERVER_PANORAMIX_PRIV_H_

#include <stdbool.h>

#include "include/extinit.h"

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

static inline void PanoramiXDisable() {
#ifdef XINERAMA
    noPanoramiXExtension = TRUE;
#endif
}

static inline void PanoramiXEnable() {
#ifdef XINERAMA
    noPanoramiXExtension = FALSE;
#endif
}

static inline bool PanoramiXIsMasterScreen(ScreenPtr pScreen) {
#ifdef XINERAMA
    return ((!PanoramiXIsEnabled()) || (pScreen && pScreen->myNum == 0));
#else
    return TRUE;
#endif
}

static inline bool PanoramiXIsSlaveScreen(ScreenPr pScreen) {
#ifdef XINERAMA
    return (PanoramiXIsEnabled() && pScreen && pScreen->myNum != 0);
#else
    return FALSE;
#endif
}

#endif /* _XSERVER_PANORAMIX_PRIV_H_ */
