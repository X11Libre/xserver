
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "config/hotplug_priv.h"

#ifdef XSERVER_PLATFORM_BUS
/* noop platform device support */
#include "xf86_OSproc.h"

#include "xf86.h"
#include "xf86_os_support.h"
#include "xf86platformBus_priv.h"

Bool
xf86PlatformDeviceCheckBusID(struct xf86_platform_device *device, const char *busid)
{
    return FALSE;
}

void xf86PlatformDeviceProbe(struct OdevAttributes *attribs)
{
}

void xf86PlatformReprobeDevice(int index, struct OdevAttributes *attribs)
{
}

void DeleteGPUDeviceRequest(struct OdevAttributes *attribs)
{
}

void NewGPUDeviceRequest(struct OdevAttributes *attribs)
{
}

#endif
