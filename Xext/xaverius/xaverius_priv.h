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

/* Forward declaration for callback signatures */
typedef struct _XaveriusScreen XaveriusScreenRec, *XaveriusScreenPtr;

/* ---- Registry result (DDX-facing) ---- */
/* Matches spec/device-registry-architecture.md §7 (XaveriusRegistryResult). */
typedef enum {
    XAVERIUS_REGISTRY_SUCCESS            = 0,
    XAVERIUS_REGISTRY_PENDING            = 1,
    XAVERIUS_REGISTRY_AMBIGUOUS          = 2,
    XAVERIUS_REGISTRY_STALE_GENERATION   = 3,
    XAVERIUS_REGISTRY_INVALID_EVIDENCE   = 4,
    XAVERIUS_REGISTRY_IDENTITY_CONFLICT  = 5,
    XAVERIUS_REGISTRY_OUT_OF_MEMORY      = 6,
    XAVERIUS_REGISTRY_INVALID_ARGUMENT   = 7,
    XAVERIUS_REGISTRY_NOT_FOUND          = 8,
    XAVERIUS_REGISTRY_ALREADY_EXISTS     = 9,
    XAVERIUS_REGISTRY_NOT_REGISTERED     = 10,
    XAVERIUS_REGISTRY_SHUTTING_DOWN      = 11,
    XAVERIUS_REGISTRY_BUSY               = 12,
    XAVERIUS_REGISTRY_UNAVAILABLE        = 13,
    XAVERIUS_REGISTRY_UNSUPPORTED        = 14,
    XAVERIUS_REGISTRY_QUARANTINED        = 15,
    XAVERIUS_REGISTRY_STALE_EVENT        = 16,
    XAVERIUS_REGISTRY_REMOVED            = 17,
    XAVERIUS_REGISTRY_INTERNAL_ERROR     = 18,
} XaveriusRegistryResult;

/* ---- Canonical PhysicalDevice identity (DDX-facing) ---- */
/* Matches spec/device-registry-architecture.md §2.2 identity evidence classes. */
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

/* ---- DDX module-role descriptor (v1, size/versioned) ---- */
typedef struct _XaveriusModuleRoleDescriptor {
    uint64_t struct_size;
    uint64_t abi_version;
    const char *owner_key;          /* stable role key across reloads */
    bool is_permanent;              /* no loader lease; active during server operation */
    void (*retain_module_role)(void *role);
    void (*release_module_role)(void *role);
    int  (*quiesce_module_role)(void *role); /* PENDING/SUCCESS/FAILURE */
    void *role_userdata;            /* DDX-owned */
} XaveriusModuleRoleDescriptor;

/* ---- DDX device backend registration descriptor (v1, size/versioned) ---- */
typedef struct _XaveriusDeviceBackendRegistration {
    uint64_t struct_size;
    uint64_t abi_version;
    const void *identity_evidence;      /* structured canonical identity */
    uint64_t identity_evidence_size;
    int native_handle;                  /* borrowed DDX-owned, exact native fd/handle */
    void *context;                      /* borrowed DDX-owned per-device context */
    const struct XaveriusDeviceBackendOps *ops; /* immutable typed ops tables */
    uint64_t module_role_generation;    /* owning ModuleRole generation */
    const char *owner_key;              /* stable role key across reloads */
    uint64_t owner_key_size;
} XaveriusDeviceBackendRegistration;

struct _XaveriusPhysicalDevice;
typedef struct _XaveriusPhysicalDevice XaveriusPhysicalDevice;

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

/*
 * Register one PhysicalDevice served by this DDX.
 *
 * Supplies structured identity evidence, the exact native PhysicalDevice handle,
 * per-device backend context, immutable typed operation vectors, and the owning
 * DDX module-role generation.
 *
 * On XAVERIUS_REGISTRY_SUCCESS returns an immediate caller-owned opaque PhysicalDevice
 * reference in *out_device plus exact device_generation and binding_generation.
 * The caller must release the reference with XaveriusPhysicalDeviceRelease() and must
 * NOT retain or dereference the bare pointer after this call returns.
 *
 * Registration does NOT publish the PhysicalDevice or allocate its XID;
 * publication is a separate server-internal operation (XaveriusPhysicalDevicePublish)
 * after identity resolution, eligible-binding selection, and the startup gate.
 */
XaveriusRegistryResult XaveriusRegistryRegisterDdxDevice(
    void *registry,
    const XaveriusDeviceBackendRegistration *registration,
    XaveriusPhysicalDevice **out_device,
    uint64_t *out_device_generation,
    uint64_t *out_binding_generation);

/*
 * Release the caller-owned PhysicalDevice reference returned by registration.
 * Not a retained DDX handle; NULL-safe.
 */
void XaveriusPhysicalDeviceRelease(XaveriusPhysicalDevice *device);

/*
 * Stop new calls through the exact DDX/device binding and apply the selected
 * binding unavailability rules; not physical removal.
 */
XaveriusRegistryResult XaveriusRegistryRemoveDeviceBackendBinding(
    void *registry,
    XaveriusPhysicalDevice *device,
    uint64_t device_generation,
    uint64_t module_role_generation,
    uint64_t binding_generation);

#endif /* _XLIBRE_XAVERIUS_PRIV_H */
