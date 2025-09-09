/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_XF86_PLATFORM_BUS_PRIV_H
#define _XSERVER_XF86_PLATFORM_BUS_PRIV_H

#include "xf86platformBus.h"

#ifdef XSERVER_PLATFORM_BUS

int xf86platformProbe(void);
int xf86platformProbeDev(DriverPtr drvp);
int xf86platformAddGPUDevices(DriverPtr drvp);
void xf86MergeOutputClassOptions(int entityIndex, void **options);
void xf86PlatformScanPciDev(void);
const char *xf86PlatformFindHotplugDriver(int dev_index);

int xf86_add_platform_device(struct OdevAttributes *attribs, Bool unowned);
int xf86_remove_platform_device(int dev_index);
Bool xf86_get_platform_device_unowned(int index);

int xf86platformAddDevice(const char *driver_name, int index);
void xf86platformRemoveDevice(int index);

void xf86platformVTProbe(void);
void xf86platformPrimary(void);

#else /* XSERVER_PLATFORM_BUS */

static inline int xf86platformAddGPUDevices(DriverPtr drvp) { return FALSE; }
static inline void xf86MergeOutputClassOptions(int index, void **options) {}

#endif /* XSERVER_PLATFORM_BUS */

#endif /* _XSERVER_XF86_PLATFORM_BUS_PRIV_H */
