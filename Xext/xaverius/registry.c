/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * Xaverius process-wide PhysicalDevice registry implementation.
 *
 * The registry owns the process-wide inventory of PhysicalDevices served
 * by any DDX.
 */
#include <dix-config.h>

#include <string.h>

#include "include/dix.h"
#include "include/dixstruct.h"
#include "os/fmt.h"
#include "include/extnsionst.h"
#include "include/globals.h"

#include "xaverius_intern.h"

struct xorg_list xaverius_physical_devices = { 0 };
uint32_t xaverius_physical_device_count = 0;

RESTYPE XaveriusPhysicalDeviceType = 0;

static int XaveriusPhysicalDeviceDestroy(void *value, XID xid)
{
    XaveriusDevicePtr dev = (XaveriusDevicePtr)value;

    /* Drop list membership; the DIX resource table owns the final cleanup. */
    xorg_list_del(&dev->list);
    xaverius_physical_device_count--;

    /* Free every deep-copied field owned by this record. */
    free(dev->capabilities.queue_families);
    free(dev->capabilities.memory_types);
    free(dev->capabilities.memory_heaps);
    free(dev->capabilities.device_name);

    free(dev);

    return 1;
}

static bool XaveriusPhysicalDeviceTypeInit(void)
{
    if (XaveriusPhysicalDeviceType != 0)
        return true;

    XaveriusPhysicalDeviceType = CreateNewResourceType(
        XaveriusPhysicalDeviceDestroy, "XaveriusPhysicalDevice");
    if (XaveriusPhysicalDeviceType == 0)
        return false;

    return true;
}

/*
 * Canonical identity comparison.
 *
 * Phase A uses the normative evidence hierarchy without inventing a
 * competing arbitration policy:
 *
 * - PCI devices: match on domain/bus/device/function; corroborate with
 *   Vulkan device UUID when both sides report it.
 * - Non-PCI devices (DRM render nodes, platform devices, etc.): fall back
 *   to DRM device identity when PCI coordinates are all zero.
 *
 * A pathname, minor number, fd identity, or node basename alone is not
 * sufficient for canonical identity.
 */
static bool xaverius_identity_equal(const XaveriusDeviceIdentity *a,
                        const XaveriusDeviceIdentity *b)
{
    bool a_has_pci = (a->domain != 0 || a->bus != 0 ||
                      a->device != 0 || a->function != 0);
    bool b_has_pci = (b->domain != 0 || b->bus != 0 ||
                      b->device != 0 || b->function != 0);

    if (a_has_pci && b_has_pci) {
        if (a->domain != b->domain || a->bus != b->bus ||
            a->device != b->device || a->function != b->function)
            return false;
    } else if (a_has_pci != b_has_pci) {
        /* PCI vs non-PCI: never the same incarnation. */
        return false;
    } else {
        /* Both non-PCI: fall back to DRM device identity (major:minor). */
        if (a->drm_major != 0 || a->drm_minor != 0 ||
            b->drm_major != 0 || b->drm_minor != 0) {
            if (a->drm_major != b->drm_major ||
                a->drm_minor != b->drm_minor)
                return false;
        } else if (!a->has_device_uuid || !b->has_device_uuid) {
            /* Neither has PCI, DRM, or UUID evidence: cannot be same. */
            return false;
        }
    }

    if (a->has_device_uuid && b->has_device_uuid &&
        memcmp(a->device_uuid, b->device_uuid, 16) != 0)
        return false;

    return true;
}

static XaveriusDevicePtr xaverius_find_device_locked(const XaveriusDeviceIdentity *identity)
{
    XaveriusDevicePtr dev;

    xorg_list_for_each_entry(dev, &xaverius_physical_devices, list) {
        if (xaverius_identity_equal(&dev->identity, identity))
            return dev;
    }

    return NULL;
}

bool XaveriusRegistryInit(void)
{
    if (!XaveriusPhysicalDeviceTypeInit())
        return false;

    xorg_list_init(&xaverius_physical_devices);
    xaverius_physical_device_count = 0;
    return true;
}

void XaveriusRegistryFini(void)
{
    XaveriusDevicePtr dev, next;

    /* Retire every record through the DIX resource table so the canonical
     * destructor runs exactly once per record. */
    xorg_list_for_each_entry_safe(dev, next, &xaverius_physical_devices, list) {
        FreeResource(dev->xid, 0);
    }

    xaverius_physical_device_count = 0;
}

bool XaveriusRegisterDevice(const XaveriusDeviceIdentity identity,
                            const XaveriusDeviceCapabilities *caps,
                            int native_handle,
                            void *context)
{
    XID xid = 0;

    if (caps == NULL)
        return false;

    /* Idempotency: an already-registered device with the same identity
     * is not re-registered or overwritten ("last writer wins" forbidden). */
    XaveriusDevicePtr dev = xaverius_find_device_locked(&identity);
    if (dev != NULL)
        return true;

    /* Validate all input pointers/counts/lengths. */
    if (caps->queue_families != NULL && caps->queue_family_count == 0)
        return false;
    if (caps->memory_types != NULL && caps->memory_type_count == 0)
        return false;
    if (caps->memory_heaps != NULL && caps->memory_heap_count == 0)
        return false;

    /* Allocate the provisional PhysicalDevice record and deep-copy every
     * owned field; initialize its list links, generations, status, and
     * complete binding before it is visible to DIX or registry iteration. */
    dev = calloc(1, sizeof(*dev));
    if (dev == NULL)
        return false;

    dev->identity = identity;
    dev->status = XAVERIUS_DEVICE_STATUS_AVAILABLE;
    dev->native_handle = native_handle;
    dev->context = context;
    dev->capabilities = *caps;

    /* Allocate/reserve the XID using the established Xorg resource/XID rules. */
    dev->xid = dixAllocServerXID();
    if (dev->xid == 0) {
        free(dev);
        return false;
    }

    /* Deep-copy dynamically owned fields from the snapshot. */
    if (caps->queue_families != NULL && caps->queue_family_count > 0) {
        dev->capabilities.queue_families =
            calloc(caps->queue_family_count, sizeof(XaveriusQueueFamilyInfo));
        if (dev->capabilities.queue_families == NULL) {
            free(dev);
            return false;
        }
        memcpy(dev->capabilities.queue_families, caps->queue_families,
               caps->queue_family_count * sizeof(XaveriusQueueFamilyInfo));
    }

    if (caps->memory_types != NULL && caps->memory_type_count > 0) {
        dev->capabilities.memory_types =
            calloc(caps->memory_type_count, sizeof(XaveriusMemoryTypeInfo));
        if (dev->capabilities.memory_types == NULL) {
            free(dev->capabilities.queue_families);
            free(dev);
            return false;
        }
        memcpy(dev->capabilities.memory_types, caps->memory_types,
               caps->memory_type_count * sizeof(XaveriusMemoryTypeInfo));
    }

    if (caps->memory_heaps != NULL && caps->memory_heap_count > 0) {
        dev->capabilities.memory_heaps =
            calloc(caps->memory_heap_count, sizeof(XaveriusMemoryHeapInfo));
        if (dev->capabilities.memory_heaps == NULL) {
            free(dev->capabilities.queue_families);
            free(dev->capabilities.memory_types);
            free(dev);
            return false;
        }
        memcpy(dev->capabilities.memory_heaps, caps->memory_heaps,
               caps->memory_heap_count * sizeof(XaveriusMemoryHeapInfo));
    }

    if (caps->device_name != NULL) {
        dev->capabilities.device_name = strdup(caps->device_name);
        if (dev->capabilities.device_name == NULL) {
            free(dev->capabilities.queue_families);
            free(dev->capabilities.memory_types);
            free(dev->capabilities.memory_heaps);
            free(dev);
            return false;
        }
    }

    /* Fail closed: registry must be initialized before any registration. */
    if (XaveriusPhysicalDeviceType == 0)
        return false;

    /* Register the resource exactly once with the PhysicalDevice resource
     * type and its canonical destructor. */
    if (!AddResource(dev->xid, XaveriusPhysicalDeviceType, dev)) {
        free(dev->capabilities.queue_families);
        free(dev->capabilities.memory_types);
        free(dev->capabilities.memory_heaps);
        free(dev->capabilities.device_name);
        free(dev);
        return false;
    }

    /* Only after successful resource registration, link the record into the
     * process-wide registry and mark it published. The registry list and DIX
     * resource table must never disagree about whether the object is published. */
    xorg_list_append(&dev->list, &xaverius_physical_devices);
    xaverius_physical_device_count++;

    return true;
}

XaveriusDevicePtr XaveriusFindDeviceByXid(XID xid)
{
    XaveriusDevicePtr dev;

    xorg_list_for_each_entry(dev, &xaverius_physical_devices, list) {
        if (dev->xid == xid)
            return dev;
    }

    return NULL;
}

XaveriusDevicePtr XaveriusFirstDevice(void)
{
    if (xorg_list_is_empty(&xaverius_physical_devices))
        return NULL;

    return xorg_list_first_entry(&xaverius_physical_devices,
                                 XaveriusDeviceRec, list);
}

XaveriusDevicePtr XaveriusNextDevice(XaveriusDevicePtr device)
{
    if (device == NULL)
        return XaveriusFirstDevice();

    struct xorg_list *next = device->list.next;
    if (next == &xaverius_physical_devices)
        return NULL;

    return xorg_list_first_entry(next, XaveriusDeviceRec, list);
}

uint32_t XaveriusDeviceCount(void)
{
    return xaverius_physical_device_count;
}

void XaveriusSetDeviceStatus(XaveriusDevicePtr device, XaveriusDeviceStatus status)
{
    if (device != NULL)
        device->status = status;
}
