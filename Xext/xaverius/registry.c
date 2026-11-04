/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * Xaverius Physical Device Registry — global server-wide registry
 * Only one registry exists per server process. Devices register with call vectors.
 */
#include <dix-config.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "xaverius_intern.h"
#include "xaverius_proto.h"
#include "dix/dix_priv.h"
#include "resource.h"

/* Global registry state (property atoms are defined in init.c) */
struct xorg_list xaverius_devices = { &xaverius_devices, &xaverius_devices };
uint32_t xaverius_device_count = 0;

void XaveriusRegistryInit(void)
{
    /* Atoms initialized in init.c */
    xorg_list_init(&xaverius_devices);
    xaverius_device_count = 0;
}

void XaveriusRegistryFini(void)
{
    XaveriusDevicePtr dev, next;
    xorg_list_for_each_entry_safe(dev, next, &xaverius_devices, list) {
        xorg_list_del(&dev->list);
        free(dev);
    }
    xorg_list_init(&xaverius_devices);
    xaverius_device_count = 0;
}

static bool
identity_equal(const XaveriusDeviceIdentity *a, const XaveriusDeviceIdentity *b)
{
    if (a->domain != b->domain || a->bus != b->bus ||
        a->device != b->device || a->function != b->function)
        return false;

    if (a->has_device_uuid && b->has_device_uuid) {
        if (memcmp(a->device_uuid, b->device_uuid, 16) != 0)
            return false;
    } else if (a->has_device_uuid != b->has_device_uuid) {
        return false;
    }

    if (memcmp(a->driver_uuid, b->driver_uuid, 16) != 0)
        return false;

    return true;
}

XaveriusDevicePtr XaveriusRegisterDevice(XaveriusDeviceIdentity identity,
                                         const XaveriusDeviceCapabilities *caps)
{
    /* Check if already exists */
    XaveriusDevicePtr dev;
    xorg_list_for_each_entry(dev, &xaverius_devices, list) {
        if (identity_equal(&dev->identity, &identity)) {
            /* Update callbacks if provided */
            if (caps)
                dev->caps = caps;
            return dev;
        }
    }

    /* Create new device */
    dev = calloc(1, sizeof(XaveriusDeviceRec));
    if (!dev)
        return NULL;

    dev->xid = dixAllocServerXID();
    dev->identity = identity;
    dev->status = XAVERIUS_DEVICE_STATUS_AVAILABLE;
    dev->generation = 1;
    dev->caps = caps;

    xorg_list_init(&dev->list);
    xorg_list_append(&dev->list, &xaverius_devices);
    xaverius_device_count++;

    return dev;
}

XaveriusDevicePtr XaveriusFindDeviceByXid(XID xid)
{
    if (!xid)
        return NULL;

    XaveriusDevicePtr dev;
    xorg_list_for_each_entry(dev, &xaverius_devices, list) {
        if (dev->xid == xid)
            return dev;
    }
    return NULL;
}

XaveriusDevicePtr XaveriusFindDeviceByIdentity(XaveriusDeviceIdentity identity)
{
    if (identity.domain == 0 && identity.bus == 0 && identity.device == 0 && identity.function == 0)
        return NULL;

    XaveriusDevicePtr dev;
    xorg_list_for_each_entry(dev, &xaverius_devices, list) {
        if (identity_equal(&dev->identity, &identity))
            return dev;
    }
    return NULL;
}

void XaveriusSetDeviceStatus(XaveriusDevicePtr device, XaveriusDeviceStatus status)
{
    if (!device || device->status == status)
        return;
    device->status = status;
    device->generation++;
}

XaveriusDevicePtr XaveriusFirstDevice(void)
{
    if (xorg_list_is_empty(&xaverius_devices))
        return NULL;
    return xorg_list_first_entry(&xaverius_devices, XaveriusDeviceRec, list);
}

XaveriusDevicePtr XaveriusNextDevice(XaveriusDevicePtr device)
{
    if (!device)
        return NULL;
    struct xorg_list *next = device->list.next;
    if (next == &xaverius_devices)
        return NULL;
    return xorg_list_entry(next, XaveriusDeviceRec, list);
}

uint32_t XaveriusDeviceCount(void)
{
    return xaverius_device_count;
}
