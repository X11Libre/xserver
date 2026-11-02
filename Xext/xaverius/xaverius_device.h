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
extern int ProcXAveriusCreateSemaphore(ClientPtr client);
extern int ProcXAveriusGetSemaphoreCounterValue(ClientPtr client);
extern int ProcXAveriusWaitSemaphores(ClientPtr client);
extern int ProcXAveriusSignalSemaphore(ClientPtr client);
extern int ProcXAveriusDestroySemaphore(ClientPtr client);
extern int ProcXAveriusCreateFence(ClientPtr client);
extern int ProcXAveriusGetFenceStatus(ClientPtr client);
extern int ProcXAveriusDispatchComputeSync(ClientPtr client);

#define XAVERIUS_DEVICE_TYPE_PHYSICAL    1
#define XAVERIUS_DEVICE_TYPE_LOGICAL     2
#define XAVERIUS_SEMAPHORE_TYPE_BINARY   0
#define XAVERIUS_SEMAPHORE_TYPE_TIMELINE 1
#define XAVERIUS_SEMAPHORE_STATE_UNSIGNALED 0
#define XAVERIUS_SEMAPHORE_STATE_PENDING    1
#define XAVERIUS_SEMAPHORE_STATE_SIGNALED   2
#define XAVERIUS_SEMAPHORE_STATE_FAILED     3
#define XAVERIUS_WAIT_MODE_ANY   0
#define XAVERIUS_WAIT_MODE_ALL   1
#define XAVERIUS_FENCE_STATE_UNSIGNALED 0
#define XAVERIUS_FENCE_STATE_PENDING    1
#define XAVERIUS_FENCE_STATE_SIGNALED   2
#define XAVERIUS_FENCE_STATE_FAILED     3
#endif