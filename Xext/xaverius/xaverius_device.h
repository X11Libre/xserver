#ifndef XAVERIUS_DEVICE_H
#define XAVERIUS_DEVICE_H

#include "xaverius.h"

extern void xaverius_device_init(void);
extern void xaverius_device_fini(void);
extern int ProcXAveriusCreateDevice(ClientPtr client);
extern int ProcXAveriusCreateDefaultDevice(ClientPtr client);
extern int ProcXAveriusDestroyDevice(ClientPtr client);
extern int ProcXAveriusDispatchDeviceLifecycle(ClientPtr client);
extern int ProcXAveriusGetPhysicalDeviceMemoryProperties(ClientPtr client);
extern int ProcXAveriusEnumeratePhysicalDevices(ClientPtr client);
extern int ProcXAveriusGetPhysicalDeviceProperty(ClientPtr client);
extern int ProcXAveriusGetPhysicalDeviceQueueFamilyProperties(ClientPtr client);

/* Presentation request handlers (subopcodes 41-47) */
extern int ProcXAveriusCreateSurface(ClientPtr client);
extern int ProcXAveriusDestroySurface(ClientPtr client);
extern int ProcXAveriusGetSurfaceCapabilities(ClientPtr client);
extern int ProcXAveriusCreateSwapchain(ClientPtr client);
extern int ProcXAveriusDestroySwapchain(ClientPtr client);
extern int ProcXAveriusAcquireNextImage(ClientPtr client);
extern int ProcXAveriusQueuePresent(ClientPtr client);

#define XAVERIUS_DEVICE_TYPE_PHYSICAL    1
#define XAVERIUS_DEVICE_TYPE_LOGICAL     2
#define XAVERIUS_DEVICE_TYPE_SURFACE     3
#define XAVERIUS_DEVICE_TYPE_SWAPCHAIN   4

extern RESTYPE XaveriusSurfaceType;
extern RESTYPE XaveriusSwapchainType;

#endif