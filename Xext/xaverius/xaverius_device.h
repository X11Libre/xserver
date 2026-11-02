/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef XAVERIUS_DEVICE_H
#define XAVERIUS_DEVICE_H

#include "xaverius.h"

extern void xaverius_device_init(void);
extern void xaverius_device_fini(void);
extern int ProcXAveriusEnumeratePhysicalDevices(ClientPtr client);
extern int ProcXAveriusGetPhysicalDeviceProperty(ClientPtr client);
extern int ProcXAveriusGetPhysicalDeviceQueueFamilyProperties(ClientPtr client);
extern int ProcXAveriusCreateDevice(ClientPtr client);
extern int ProcXAveriusCreateDefaultDevice(ClientPtr client);
extern int ProcXAveriusDestroyDevice(ClientPtr client);
extern int ProcXAveriusGetPhysicalDeviceMemoryProperties(ClientPtr client);
extern int ProcXAveriusDispatchDeviceLifecycle(ClientPtr client);

#define XAVERIUS_DEVICE_TYPE_PHYSICAL    1
#define XAVERIUS_DEVICE_TYPE_LOGICAL     2
#endif