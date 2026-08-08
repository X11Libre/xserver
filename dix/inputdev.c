/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */

#include <dix-config.h>

#include "dix/inputdev_priv.h"
#include "include/xlibre_ptrtypes.h"
#include "include/inputstr.h"

DeviceIntPtr dixInputDeviceFindByConf(const char *config_info) {
    DeviceIntPtr dev;
    for (dev = inputInfo.devices; dev; dev = dev->next) {
        if (dev->config_info && (strcmp(dev->config_info, config_info) == 0))
            return dev;
    }
    return NULL;
}

DeviceIntPtr dixInputDeviceOffFindByConf(const char *config_info) {
    DeviceIntPtr dev;
    for (dev = inputInfo.off_devices; dev; dev = dev->next) {
        if (dev->config_info && (strcmp(dev->config_info, config_info) == 0))
            return dev;
    }
    return NULL;
}
