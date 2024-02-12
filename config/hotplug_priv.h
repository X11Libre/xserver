/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 * Copyright © 2006-2007 Daniel Stone
 */
#ifndef _XSERVER_HOTPLUG_PRIV_H
#define _XSERVER_HOTPLUG_PRIV_H

#include <X11/Xfuncproto.h>

#include "hotplug.h"
#include "list.h"

/* Bump this each time you add something to the struct
 * so that drivers can easily tell what is available
 */
#define ODEV_ATTRIBUTES_VERSION         1

struct OdevAttributes {
    /* path to kernel device node - Linux e.g. /dev/dri/card0 */
    char        *path;

    /* system device path - Linux e.g. /sys/devices/pci0000:00/0000:00:01.0/0000:01:00.0/drm/card1 */
    char        *syspath;

    /* DRI-style bus id */
    char        *busid;

    /* Server managed FD */
    int         fd;

    /* Major number of the device node pointed to by ODEV_ATTRIB_PATH */
    int         major;

    /* Minor number of the device node pointed to by ODEV_ATTRIB_PATH */
    int         minor;

    /* kernel driver name */
    char        *driver;
};

/* Note starting with xserver 1.16 this function never fails */
struct OdevAttributes *
config_odev_allocate_attributes(void);

void
config_odev_free_attributes(struct OdevAttributes *attribs);

typedef void (*config_odev_probe_proc_ptr)(struct OdevAttributes *attribs);
void config_odev_probe(config_odev_probe_proc_ptr probe_callback);

#ifdef CONFIG_UDEV_KMS
void NewGPUDeviceRequest(struct OdevAttributes *attribs);
void DeleteGPUDeviceRequest(struct OdevAttributes *attribs);
#endif

#define ServerIsNotSeat0() (SeatId && strcmp(SeatId, "seat0"))

struct xf86_platform_device *
xf86_find_platform_device_by_devnum(int major, int minor);

#endif /* _XSERVER_HOTPLUG_PRIV_H */
