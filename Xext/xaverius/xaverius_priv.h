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

#include <X11/Xfuncproto.h>

#ifndef _X_EXPORT
# if defined(__GNUC__) && __GNUC__ >= 4
#  define _X_EXPORT __attribute__((visibility("default")))
# else
#  define _X_EXPORT
# endif
#endif

#include "xaverius_intern.h"
#include "xaverius_proto.h"

/* ---- DDX device backend registration ABI version (v1) ---- */
#define XAVERIUS_DEVICE_BACKEND_REGISTRATION_ABI_VERSION 1

/* ---- DDX module-role descriptor ABI version (v1) ---- */
#define XAVERIUS_MODULE_ROLE_DESCRIPTOR_ABI_VERSION 1

/* Forward declaration for callback signatures */
typedef struct _XaveriusScreen XaveriusScreenRec, *XaveriusScreenPtr;

/* Called by DDX during screen initialization to register its callbacks.
 * Xaverius allocates the per-screen structure, DDX fills in the ops.
 */
typedef struct _XaveriusDDXOps
{
    int (*ScreenInit)(XaveriusScreenPtr xav_screen);
    void (*ScreenFini)(XaveriusScreenPtr xav_screen);
} XaveriusDDXOps;

/* Per-screen state (internal, not exposed to DDX beyond the pointer) */
struct _XaveriusScreen
{
    ScreenPtr pScreen;
    const XaveriusDDXOps *ops;   /* callbacks provided by DDX, called by Xaverius */
    void *driver_private;        /* DDX-owned private data */
    int initialized;
};

/* Called by DDX during screen initialization to register its callbacks.
 * Xaverius allocates the per-screen structure, DDX fills in the ops.
 * Screen close cleanup is handled automatically via the screen close hook.
 */
int XaveriusScreenInit(ScreenPtr pScreen, const XaveriusDDXOps *ops);

XaveriusScreenPtr XaveriusGetScreen(ScreenPtr pScreen);

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
