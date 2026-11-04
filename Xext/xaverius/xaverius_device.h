#ifndef XAVERIUS_DEVICE_H
#define XAVERIUS_DEVICE_H

#include "xaverius.h"

extern int XAveriusExtensionMajorOpcode;

extern int ProcXAveriusEnumeratePhysicalDevices(ClientPtr client);
extern int ProcXAveriusCreateDevice(ClientPtr client);
extern int ProcXAveriusCreateDefaultDevice(ClientPtr client);
extern int ProcXAveriusDestroyDevice(ClientPtr client);
extern int ProcXAveriusGetPhysicalDeviceMemoryProperties(ClientPtr client);

#define XAVERIUS_DEVICE_TYPE_PHYSICAL    1
#define XAVERIUS_DEVICE_TYPE_LOGICAL     2
#endif