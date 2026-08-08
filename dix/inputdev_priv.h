/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */

#ifndef __XLIBRE_DIX_INPUTDEV_PRIV_H
#define __XLIBRE_DIX_INPUTDEV_PRIV_H

#include "include/xlibre_ptrtypes.h"
#include "include/inputstr.h"

/*
 * @brief get input device by config_info string
 *
 * @return pointer to DeviceIntRec or NULL (if none found)
 */
DeviceIntPtr dixInputDeviceFindByConf(const char *config_info);

/*
 * @brief get disabled input device by config_info string
 *
 * @return pointer to DeviceIntRec or NULL (if none found)
 */
DeviceIntPtr dixInputDeviceOffFindByConf(const char *config_info);

#endif /* __XLIBRE_DIX_INPUTDEV_PRIV_H */
