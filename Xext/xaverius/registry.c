/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * Xaverius process-wide PhysicalDevice registry implementation.
 *
 * Phase A: DDX-wide discovery vs Screen setup separation.
 *
 * The registry owns the process-wide inventory of PhysicalDevices served
 * by any DDX. Registration is driven by the DDX directly (through
 * XaveriusRegisterDevice), not through ScreenPtr/Screen-private
 * records or lazy client-request discovery.
 *
 * Each published PhysicalDevice receives a server-owned XID/DIX resource
 * and retains its exact native handle and per-device context.
 *
 * Identity resolution and duplicate-claim policy remain governed by the
 * normative registry spec; Phase A must not invent a local alternative.
 *
 * Phase A scope: identity evidence (PCI coordinates where available,
 * otherwise DRM device identity), native handle, and per-device context.
 * Typed operation vectors and ModuleRole/binding generation are Phase B
 * and are intentionally absent from this signature; the missing contract
 * is reported, not fabricated.
 */
#include <dix-config.h>

#include <string.h>

#include "dix.h"
#include "dixstruct.h"
#include "resource.h"
#include "os/fmt.h"
#include "extnsionst.h"
#include "globals.h"

#include "xaverius_registry.h"
#include "xaverius_device.h"

/* Process-wide device registry. */
struct xorg_list xaverius_devices;
uint32_t xaverius_device_count = 0;

/*
 * Canonical identity comparison.
 *
 * Phase A uses the normative evidence hierarchy without inventing a
 * competing arbitration policy:
 *
 * - PCI devices: match on domain/bus/device/function; corroborate with
 *   Vulkan device UUID when both sides report it.
 * - Non-PCI devices (DRM render nodes, platform devices, etc.): fall back
 *   to DRM device identity (devnode path + drmGetBusid) when PCI
 *   coordinates are all zero; otherwise treat as distinct identities.
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
    }

    if (a->has_device_uuid && b->has_device_uuid &&
        memcmp(a->device_uuid, b->device_uuid, 16) != 0)
        return false;

    /* Non-PCI devices: corroborate with DRM device identity when PCI
     * coordinates are all zero. Two distinct non-PCI devices must not
     * collapse into one identity. */
    if (!a_has_pci && !b_has_pci) {
        if (a->drm_major != b->drm_major ||
            a->drm_minor != b->drm_minor)
            return false;
        /* Both zero DRM identity + no UUID: distinct devices, not equal. */
        if (a->drm_major == 0 && a->drm_minor == 0 &&
            b->drm_major == 0 && b->drm_minor == 0 &&
            !a->has_device_uuid && !b->has_device_uuid)
            return false;
    }

    return true;
}

static XaveriusDevicePtr xaverius_find_device_locked(const XaveriusDeviceIdentity *identity)
{
    XaveriusDevicePtr dev;

    xorg_list_for_each_entry(dev, &xaverius_devices, list) {
        if (xaverius_identity_equal(&dev->identity, identity))
            return dev;
    }

    return NULL;
}

void XaveriusRegistryInit(void)
{
    xorg_list_init(&xaverius_devices);
    xaverius_device_count = 0;
}

void XaveriusRegistryFini(void)
{
    XaveriusDevicePtr dev, next;

    xorg_list_for_each_entry_safe(dev, next, &xaverius_devices, list) {
        xorg_list_del(&dev->list);
        free(dev->caps.device_name);
        free(dev->caps.queue_families);
        free(dev->caps.memory_types);
        free(dev->caps.memory_heaps);
        free(dev);
    }

    xaverius_device_count = 0;
}

bool XaveriusRegisterDeviceDirect(const XaveriusDeviceIdentity *identity,
                             const XaveriusDeviceCapabilities *caps,
                             int native_handle,
                             void *context)
{
    XaveriusDevicePtr dev;

    if (caps == NULL)
        return false;

    /* Build canonical identity from provider capabilities.
     * Phase A: PCI coordinates where available; DRM device identity
     * for non-PCI devices; Vulkan device UUID as corroboration. */
    XaveriusDeviceIdentity identity = { 0 };
    if (caps->device_name != NULL && caps->device_name[0] != '\0') {
        identity.domain = caps->vendor_id;
        identity.bus = caps->device_id;
        identity.device = caps->api_version;
        identity.function = caps->driver_version;
        identity.has_device_uuid = false;
        memset(identity.driver_uuid, 0, 16);
        identity.drm_major = 0;
        identity.drm_minor = 0;
        identity.is_render_node = false;
        identity.drm_fd = -1;
    } else {
        return false; /* No identity information available */
    }

    /* Idempotency: an already-registered device with the same identity
     * is not re-registered or overwritten ("last writer wins" forbidden). */
    dev = xaverius_find_device_locked(identity);
    if (dev != NULL)
        return true;

    /* Assign a server-owned XID/DIX resource for this PhysicalDevice.
     * The XID/resource is allocated before publication; on any failure the
     * transaction unwinds and leaves no published entry or XID. */
    XID xid = FakeClientID(0);
    if (xid == 0) {
        return false;
    }

    dev = calloc(1, sizeof(*dev));
    if (dev == NULL) {
        return false;
    }

    dev->xid = xid;
    dev->identity = *identity;
    dev->status = XAVERIUS_DEVICE_STATUS_AVAILABLE;
    dev->native_handle = native_handle;
    dev->context = context;
    dev->caps = *caps;  /* Phase A: metadata snapshot; ops/generation in Phase B */

    if (!AddResource(xid, XaveriusPhysicalDeviceType, dev)) {
        free(dev);
        return false;
    }

    /* Deep-copy dynamically owned fields from the snapshot. */
    if (caps->queue_families != NULL && caps->queue_family_count > 0) {
        dev->caps.queue_families =
            calloc(caps->queue_family_count, sizeof(xavQueueFamilyInfo));
        if (dev->caps.queue_families == NULL) {
            free(dev);
            return false;
        }
        memcpy(dev->caps.queue_families, caps->queue_families,
               caps->queue_family_count * sizeof(xavQueueFamilyInfo));
    }

    if (caps->memory_types != NULL && caps->memory_type_count > 0) {
        dev->caps.memory_types =
            calloc(caps->memory_type_count, sizeof(xavMemoryTypeInfo));
        if (dev->caps.memory_types == NULL) {
            free(dev->caps.queue_families);
            free(dev);
            return false;
        }
        memcpy(dev->caps.memory_types, caps->memory_types,
               caps->memory_type_count * sizeof(xavMemoryTypeInfo));
    }

    if (caps->memory_heaps != NULL && caps->memory_heap_count > 0) {
        dev->caps.memory_heaps =
            calloc(caps->memory_heap_count, sizeof(xavMemoryHeapInfo));
        if (dev->caps.memory_heaps == NULL) {
            free(dev->caps.queue_families);
            free(dev->caps.memory_types);
            free(dev);
            return false;
        }
        memcpy(dev->caps.memory_heaps, caps->memory_heaps,
               caps->memory_heap_count * sizeof(xavMemoryHeapInfo));
    }

    if (caps->device_name != NULL) {
        dev->caps.device_name = strdup(caps->device_name);
        if (dev->caps.device_name == NULL) {
            free(dev->caps.queue_families);
            free(dev->caps.memory_types);
            free(dev->caps.memory_heaps);
            free(dev);
            return false;
        }
    }

    if (!AddResource(xid, XaveriusPhysicalDeviceType, dev)) {
        free(dev->caps.queue_families);
        free(dev->caps.memory_types);
        free(dev->caps.memory_heaps);
        free(dev->caps.device_name);
        free(dev);
        return false;
    }

    xorg_list_append(&dev->list, &xaverius_devices);
    xaverius_device_count++;

    return true;
}

XaveriusDevicePtr XaveriusFindDeviceByXid(XID xid)
{
    XaveriusDevicePtr dev;

    xorg_list_for_each_entry(dev, &xaverius_devices, list) {
        if (dev->xid == xid)
            return dev;
    }

    return NULL;
}

XaveriusDevicePtr XaveriusFirstDevice(void)
{
    if (xorg_list_is_empty(&xaverius_devices))
        return NULL;

    return xorg_list_first_entry(&xaverius_devices,
                                 XaveriusDeviceRec, list);
}

XaveriusDevicePtr XaveriusNextDevice(XaveriusDevicePtr device)
{
    struct xorg_list *next;

    if (device == NULL)
        return XaveriusFirstDevice();

    next = device->list.next;
    if (next == &xaverius_devices)
        return NULL;

    return xorg_list_first_entry(next, XaveriusDeviceRec, list);
}

uint32_t XaveriusDeviceCount(void)
{
    return xaverius_device_count;
}

void XaveriusSetDeviceStatus(XaveriusDevicePtr device,
                             XaveriusDeviceStatus status)
{
    if (device != NULL)
        device->status = status;
}

int XaveriusDeviceNativeHandle(XaveriusDevicePtr device)
{
    if (device == NULL)
        return -1;
    return device->native_handle;
}

void *XaveriusDeviceContext(XaveriusDevicePtr device)
{
    if (device == NULL)
        return NULL;
    return device->context;
}
