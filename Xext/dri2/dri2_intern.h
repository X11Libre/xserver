/* SPDX-License-Identifier: MIT OR X11 OR AGPLv3+
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * This header holds definitions that are only used within the dri2
 * extensions - other parts of the Xserver should never include it.
 */
#ifndef _XSERVER_DRI2_INTERN_H_
#define _XSERVER_DRI2_INTERN_H_

#include <stdbool.h>

#include "include/xlibre_ptrtypes.h"

bool DRI2ModuleSetup(void);

#endif /* _XSERVER_DRI2_INTERN_H_ */
