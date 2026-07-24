/* SPDX-License-Identifier: MIT OR X11 OR AGPLv3+
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

#endif /* _XSERVER_PANORAMIX_PRIV_H_ */
