/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * Xaverius per-screen private API for DDX drivers.
 * This header is installed and can be included by DDX drivers (e.g. modesetting)
 * to hook into Xaverius per-screen functionality.
 *
 * Xaverius allocates the per-screen structure and calls into the DDX driver
 * through the ops callbacks.
 */

#ifndef _XLIBRE_XAVERIUS_PRIV_H
#define _XLIBRE_XAVERIUS_PRIV_H

#include <stdbool.h>
#include <stdint.h>

#include "include/xlibre_ptrtypes.h"

#include "xaverius_proto.h"

/* Device status (matches XAverius spec) */
typedef enum {
    XAVERIUS_DEVICE_STATUS_AVAILABLE   = 0,
    XAVERIUS_DEVICE_STATUS_UNAVAILABLE = 1,
    XAVERIUS_DEVICE_STATUS_REMOVED     = 2
} XaveriusDeviceStatus;

/* Device identity for canonicalization */
typedef struct _XaveriusDeviceIdentity {
    /* DRM/PCI bus identity */
    uint32_t domain;
    uint32_t bus;
    uint32_t device;
    uint32_t function;

    /* Optional Vulkan device UUID (16 bytes) */
    uint8_t device_uuid[16];
    bool has_device_uuid;

    /* Driver UUID (16 bytes) */
    uint8_t driver_uuid[16];

    /* Kernel DRM version info */
    uint32_t drm_major;
    uint32_t drm_minor;

    /* Optional: render node vs primary node tracking */
    bool is_render_node;
    int drm_fd; /* -1 if not applicable */
} XaveriusDeviceIdentity;

/* Provider capabilities (immutable snapshot, taken at registration time) */
typedef struct _XaveriusDeviceCapabilities {
    /* Device properties */
    uint32_t device_type;
    uint32_t vendor_id;
    uint32_t device_id;
    uint32_t api_version;
    uint32_t driver_version;
    uint32_t compute_queue_family_count;

    /* Queue families */
    XaveriusQueueFamilyInfo *queue_families;
    uint32_t queue_family_count;

    /* Memory properties */
    XaveriusMemoryTypeInfo *memory_types;
    uint32_t memory_type_count;
    XaveriusMemoryHeapInfo *memory_heaps;
    uint32_t memory_heap_count;

    /* Properties */
    uint32_t max_work_group_invocations;
    uint32_t max_work_group_size[3];
    bool has_timeline_semaphore;

    /* Device name */
    char *device_name;

    /* Provider-specific handle (opaque) */
    void *provider_handle;
} XaveriusDeviceCapabilities;

/* Forward declaration for callback signatures */
typedef struct _XaveriusScreen XaveriusScreenRec, *XaveriusScreenPtr;

/* Called by DDX during screen initialization to register its callbacks.
 * Xaverius allocates the per-screen structure, DDX fills in the ops.
 */
typedef struct _XaveriusDDXOps
{
    bool (*ScreenInit)(XaveriusScreenPtr xav_screen);
    void (*ScreenFini)(XaveriusScreenPtr xav_screen);
} XaveriusDDXOps;

/* Per-screen state (internal, not exposed to DDX beyond the pointer) */
struct _XaveriusScreen
{
    ScreenPtr pScreen;
    const XaveriusDDXOps *ops;   /* callbacks provided by DDX, called by Xaverius */
    void *driver_private;        /* DDX-owned private data */
    bool initialized;
};

/* Called by DDX during screen initialization to register its callbacks.
 * Xaverius allocates the per-screen structure, DDX fills in the ops.
 * Screen close cleanup is handled automatically via the screen close hook.
 */
_X_EXPORT bool XaveriusScreenInit(ScreenPtr pScreen, const XaveriusDDXOps *ops);

_X_EXPORT XaveriusScreenPtr XaveriusGetScreen(ScreenPtr pScreen);

/* DDX-wide device registration entry point (Phase A).
 * DDX drivers call this once per provider to register all served
 * PhysicalDevices and their provider capabilities.
 * Strictly DDX-wide; no ScreenPtr, no per-screen state.
 */
_X_EXPORT bool XaveriusRegisterDevice(const XaveriusDeviceIdentity identity,
                                      const XaveriusDeviceCapabilities *caps,
                                      int native_handle,
                                      void *context);

#endif /* _XLIBRE_XAVERIUS_PRIV_H */
