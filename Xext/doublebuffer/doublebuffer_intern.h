/* SPDX-License-Identifier: MIT OR X11 OR AGPLv3+
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * This header holds definitions that are only used within the composite
 * extensions - other parts of the Xserver should never include it.
 */
#ifndef _XSERVER_DOUBLEBUFFER_INTERN_H_
#define _XSERVER_DOUBLEBUFFER_INTERN_H_

#include <stdbool.h>

#include "include/xlibre_ptrtypes.h"

#include "dbestruct.h"

bool miDbeInit(ScreenPtr pScreen, DbeScreenPrivPtr pDbeScreenPriv);

#endif /* _XSERVER_DOUBLEBUFFER_INTERN_H_ */
