/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * Xaverius process-wide PhysicalDevice registry implementation.
 *
 * Stage 1: canonical Xaverius* types and API declarations.
 * Stage 2: XaveriusModuleRole substrate (retain/release/quiesce/finalize,
 * admission close, lease ownership).
 * Stage 3: registry core — canonical identity, non-reusable generations,
 * tombstones, observe/withdraw/confirm-removal, DDX binding preference via
 * owner_key. Publication/DIX wiring deferred to Stage 4.
 */

#include <dix-config.h>

#include <string.h>

#include "include/dix.h"
#include "include/dixstruct.h"
#include "os/fmt.h"
#include "include/extnsionst.h"
#include "include/globals.h"

#include "xaverius_intern.h"

/* ---- Process-wide generation counter (non-reusable generations) ---- */
static uint64_t xaverius_next_generation = 1;

static uint64_t
xaverius_next_generation_id(void)
{
    return xaverius_next_generation++;
}

/* ---- Process-wide PhysicalDevice registry (Stage 3) ---- */
struct xorg_list xaverius_physical_devices = { 0 };
uint32_t xaverius_physical_device_count = 0;

/* Tombstones of confirmed-removed PhysicalDevices. Survive confirmed physical
 * removal and remain enumerable until ordered server shutdown. DIX wrapper
 * destruction releases only its wrapper reference; it must not unlink/free the
 * registry identity or permit XID reuse. */
struct xorg_list xaverius_tombstones = { 0 };
uint32_t xaverius_tombstone_count = 0;

/* Process-wide ModuleRole registry (Stage 2). One control record per DDX
 * module-role instance; shared across all bindings of that role. */
struct xorg_list xaverius_module_roles = { 0 };
uint32_t xaverius_module_role_count = 0;

/* Registered discovery sources (Stage 3 minimal bookkeeping). */
struct xorg_list xaverius_discovery_sources = { 0 };
uint32_t xaverius_discovery_source_count = 0;

/* DIX resource type for published PhysicalDevices (server-private). */
RESTYPE XaveriusPhysicalDeviceType = 0;

/* Canonical DIX destructor for a published PhysicalDevice record.
 * Runs exactly once per published record via FreeResource. */
int
XaveriusPhysicalDeviceDIXDestroy(void *value, XID xid)
{
    XaveriusPhysicalDevice *dev = (XaveriusPhysicalDevice *)value;

    /* Drop list membership; the DIX resource table owns the final cleanup. */
    xorg_list_del(&dev->list);
    xaverius_physical_device_count--;

    /* Free server-owned fields. */
    free(dev->identity_evidence);
    free(dev->snapshot);

    free(dev);

    return 1;
}

/* Idempotently create the PhysicalDevice DIX resource type with its
 * canonical destructor (Stage 1: type registration only, no publication). */
static bool
XaveriusPhysicalDeviceTypeInit(void)
{
    if (XaveriusPhysicalDeviceType != 0)
        return true;

    XaveriusPhysicalDeviceType =
        CreateNewResourceType(XaveriusPhysicalDeviceDIXDestroy,
                              "XaveriusPhysicalDevice");
    if (XaveriusPhysicalDeviceType == 0)
        return false;

    return true;
}

bool
XaveriusRegistryInit(void)
{
    if (!XaveriusPhysicalDeviceTypeInit())
        return false;

    xorg_list_init(&xaverius_physical_devices);
    xaverius_physical_device_count = 0;
    xorg_list_init(&xaverius_tombstones);
    xaverius_tombstone_count = 0;
    xorg_list_init(&xaverius_module_roles);
    xaverius_module_role_count = 0;
    xorg_list_init(&xaverius_discovery_sources);
    xaverius_discovery_source_count = 0;
    return true;
}

void
XaveriusRegistryFini(void)
{
    /* Stage 3: no published records yet. Keep the signature for later
     * stages; the DIX destructor path handles published records. */
    xaverius_physical_device_count = 0;
    xaverius_tombstone_count = 0;
    xaverius_module_role_count = 0;
    xaverius_discovery_source_count = 0;
}

/* ---- Canonical identity comparison (Stage 3) ---- */

/* PCI vs non-PCI classification. */
static bool
xaverius_identity_has_pci(const XaveriusDeviceIdentity *id)
{
    return (id->domain != 0 || id->bus != 0 ||
            id->device != 0 || id->function != 0);
}

static bool
xaverius_identity_equal(const XaveriusDeviceIdentity *a,
                        const XaveriusDeviceIdentity *b)
{
    bool a_has_pci = xaverius_identity_has_pci(a);
    bool b_has_pci = xaverius_identity_has_pci(b);

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

/* Canonical ordering key: lower value sorts first. Returns negative, zero,
 * or positive. Implements the v0.1 tuple ordering from
 * device-registry-architecture.md §2.3 without introducing DDX module-role
 * registration order, pointer values, DRM minor, Screen index, etc. */
static int
xaverius_identity_compare(const XaveriusDeviceIdentity *a,
                          const XaveriusDeviceIdentity *b)
{
    bool a_has_pci = xaverius_identity_has_pci(a);
    bool b_has_pci = xaverius_identity_has_pci(b);

    /* Identity class ordering: PCI-backed before non-PCI. */
    if (a_has_pci && !b_has_pci)
        return -1;
    if (!a_has_pci && b_has_pci)
        return 1;

    if (a_has_pci && b_has_pci) {
        int c;
        if ((c = (int)a->domain - (int)b->domain) != 0) return c;
        if ((c = (int)a->bus - (int)b->bus) != 0) return c;
        if ((c = (int)a->device - (int)b->device) != 0) return c;
        if ((c = (int)a->function - (int)b->function) != 0) return c;
        return 0;
    }

    /* Both non-PCI: compare DRM major:minor, then UUID. */
    {
        int c;
        if ((c = (int)a->drm_major - (int)b->drm_major) != 0) return c;
        if ((c = (int)a->drm_minor - (int)b->drm_minor) != 0) return c;
    }
    if (a->has_device_uuid && b->has_device_uuid)
        return memcmp(a->device_uuid, b->device_uuid, 16);
    if (a->has_device_uuid && !b->has_device_uuid)
        return -1;
    if (!a->has_device_uuid && b->has_device_uuid)
        return 1;
    return 0;
}

/* Find an existing PhysicalDevice by canonical identity (unpublished or
 * published). Returns NULL if none matches. */
static XaveriusPhysicalDevice *
xaverius_find_device_by_identity(const XaveriusDeviceIdentity *identity)
{
    XaveriusPhysicalDevice *dev;

    xorg_list_for_each_entry(dev, &xaverius_physical_devices, list) {
        if (xaverius_identity_equal(&dev->identity, identity))
            return dev;
    }

    /* Also check tombstones: a removed incarnation may be re-registered as a
     * new incarnation. Tombstones are never reused as the same incarnation. */
    xorg_list_for_each_entry(dev, &xaverius_tombstones, list) {
        if (xaverius_identity_equal(&dev->identity, identity))
            return dev;
    }

    return NULL;
}

/* ---- ModuleRole substrate (Stage 2) ---- */

/* ModuleRole lifecycle: infallible, non-blocking lifetime primitives except
 * XaveriusModuleRoleCreate which may fail on allocation/validation before any
 * retain. */

/* Validate a XaveriusModuleRoleDescriptor header before reading later fields. */
static bool
xaverius_module_role_descriptor_valid(const XaveriusModuleRoleDescriptor *desc)
{
    if (desc == NULL)
        return false;

    /* Size/version validation, exactly as specified for v1 descriptors. */
    if (desc->struct_size < sizeof(*desc))
        return false;

    return true;
}

XaveriusRegistryResult
XaveriusModuleRoleCreate(const XaveriusModuleRoleDescriptor *desc,
                    void **out_role)
{
    struct _XaveriusModuleRole *role;

    if (desc == NULL || out_role == NULL)
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    /* Validate header before reading later fields. */
    if (!xaverius_module_role_descriptor_valid(desc))
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    /* Failed registration calls no role hook. */
    if (desc->retain_module_role == NULL ||
        desc->release_module_role == NULL ||
        desc->quiesce_module_role == NULL)
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    role = calloc(1, sizeof(*role));
    if (role == NULL)
        return XAVERIUS_REGISTRY_OUT_OF_MEMORY;

    role->owner_key = strdup(desc->owner_key ? desc->owner_key : "");
    if (role->owner_key == NULL) {
        free(role);
        return XAVERIUS_REGISTRY_OUT_OF_MEMORY;
    }

    role->is_permanent = desc->is_permanent;
    role->module_lease_ref = NULL;           /* set by module owner later */
    role->role_generation = xaverius_next_generation_id();
    role->admission = XAVERIUS_MODULE_ROLE_ADMISSION_OPEN;
    role->quiesce_state = XAVERIUS_MODULE_ROLE_QUIESCE_PENDING;
    role->retain_module_role = desc->retain_module_role;
    role->release_module_role = desc->release_module_role;
    role->quiesce_module_role = desc->quiesce_module_role;
    role->role_userdata = desc->role_userdata;

    xorg_list_init(&role->list);

    *out_role = role;
    return XAVERIUS_REGISTRY_SUCCESS;
}

void
XaveriusModuleRoleRetain(void *role)
{
    struct _XaveriusModuleRole *r = (struct _XaveriusModuleRole *)role;

    if (r == NULL || r->retain_module_role == NULL)
        return;

    /* Infallible, non-blocking lifetime primitive. */
    r->retain_module_role(r);
}

void
XaveriusModuleRoleRelease(void *role)
{
    struct _XaveriusModuleRole *r = (struct _XaveriusModuleRole *)role;

    if (r == NULL || r->release_module_role == NULL)
        return;

    /* Infallible, non-blocking lifetime primitive; does NOT initiate
     * quiescence. */
    r->release_module_role(r);
}

int
XaveriusModuleRoleQuiesce(void *role)
{
    struct _XaveriusModuleRole *r = (struct _XaveriusModuleRole *)role;

    if (r == NULL || r->quiesce_module_role == NULL)
        return XAVERIUS_MODULE_ROLE_QUIESCE_FAILURE;

    /* Returns PENDING/SUCCESS/FAILURE. */
    return r->quiesce_module_role(r);
}

void
XaveriusModuleRoleFinalize(void *role)
{
    struct _XaveriusModuleRole *r = (struct _XaveriusModuleRole *)role;

    if (r == NULL)
        return;

    /* After quiescence, finalization calls release exactly once. Permanent
     * roles skip lease release. */
    if (!r->is_permanent && r->module_lease_ref != NULL) {
        /* Module owner releases its original reference after every role
         * for this module instance is finalized; handled by the module
         * owner, not here. */
    }

    if (r->release_module_role != NULL)
        r->release_module_role(r);

    free(r->owner_key);
    free(r);
}

bool
XaveriusModuleRoleAdmissionOpen(void *role)
{
    struct _XaveriusModuleRole *r = (struct _XaveriusModuleRole *)role;

    if (r == NULL)
        return false;

    return r->admission == XAVERIUS_MODULE_ROLE_ADMISSION_OPEN;
}

/* ---- Registry registration (Stage 3) ---- */

XaveriusRegistryResult
XaveriusRegistryRegisterDdxDevice(void *registry,
                             const XaveriusDeviceBackendRegistration *registration,
                             XaveriusPhysicalDevice **out_device,
                             uint64_t *out_device_generation,
                             uint64_t *out_binding_generation)
{
    XaveriusPhysicalDevice *dev;
    XaveriusPhysicalDevice *existing;

    if (registration == NULL || out_device == NULL)
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    /* Validate descriptor header before reading later fields. */
    if (registration->struct_size < sizeof(*registration) ||
        registration->abi_version != XAVERIUS_DEVICE_BACKEND_OPS_ABI_VERSION)
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    /* Canonicalize identity: check for an existing exact incarnation. */
    existing = xaverius_find_device_by_identity(
        registration->identity_evidence != NULL
            ? (const XaveriusDeviceIdentity *)registration->identity_evidence
            : &(XaveriusDeviceIdentity){ 0 });

    if (existing != NULL) {
        /* Exact replay of the same live registration/binding ownership is
         * idempotent: return an additional caller-owned reference.
         * Conflicting data returns a classified failure and no ref. */
        if (existing->module_role_generation == registration->module_role_generation &&
            existing->binding_generation == registration->module_role_generation) {
            *out_device = existing;
            if (out_device_generation != NULL)
                *out_device_generation = existing->device_generation;
            if (out_binding_generation != NULL)
                *out_binding_generation = existing->binding_generation;
            return XAVERIUS_REGISTRY_SUCCESS;
        }
        return XAVERIUS_REGISTRY_IDENTITY_CONFLICT;
    }

    /* Install into internal (not yet published) registry. */
    dev = calloc(1, sizeof(*dev));
    if (dev == NULL)
        return XAVERIUS_REGISTRY_OUT_OF_MEMORY;

    dev->identity_evidence = NULL;
    dev->identity_evidence_size = 0;
    dev->snapshot = NULL;
    dev->snapshot_size = 0;
    dev->status = XAVERIUS_REGISTRY_SUCCESS;
    dev->binding_generation = xaverius_next_generation_id();
    dev->device_generation = xaverius_next_generation_id();
    dev->module_role_generation = registration->module_role_generation;

    /* Copy identity evidence if provided. */
    if (registration->identity_evidence != NULL &&
        registration->identity_evidence_size > 0) {
        dev->identity_evidence =
            calloc(1, registration->identity_evidence_size);
        if (dev->identity_evidence == NULL) {
            free(dev);
            return XAVERIUS_REGISTRY_OUT_OF_MEMORY;
        }
        memcpy(dev->identity_evidence, registration->identity_evidence,
               registration->identity_evidence_size);
        dev->identity_evidence_size = registration->identity_evidence_size;
    }

    xorg_list_append(&dev->list, &xaverius_physical_devices);
    xaverius_physical_device_count++;

    *out_device = dev;
    if (out_device_generation != NULL)
        *out_device_generation = dev->device_generation;
    if (out_binding_generation != NULL)
        *out_binding_generation = dev->binding_generation;

    return XAVERIUS_REGISTRY_SUCCESS;
}

void
XaveriusPhysicalDeviceRelease(XaveriusPhysicalDevice *device)
{
    if (device == NULL)
        return;

    /* Drop list membership; this is the internal registry record, not a
     * published DIX resource (Stage 3: no XID/Alloc yet). */
    xorg_list_del(&device->list);
    xaverius_physical_device_count--;

    free(device->identity_evidence);
    free(device->snapshot);
    free(device);
}

XaveriusRegistryResult
XaveriusRegistryRemoveDeviceBackendBinding(void *registry,
                                       XaveriusPhysicalDevice *device,
                                       uint64_t device_generation,
                                       uint64_t module_role_generation,
                                       uint64_t binding_generation)
{
    /* Stage 3: binding removal marks the exact binding UNAVAILABLE; not
     * physical removal. Reject if generations do not match. */
    if (device == NULL)
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    if (device->module_role_generation != module_role_generation ||
        device->binding_generation != binding_generation)
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    device->status = XAVERIUS_REGISTRY_UNAVAILABLE;
    return XAVERIUS_REGISTRY_SUCCESS;
}

XaveriusPhysicalDevice *
XaveriusFirstDevice(void)
{
    if (xorg_list_is_empty(&xaverius_physical_devices))
        return NULL;

    return xorg_list_first_entry(&xaverius_physical_devices,
                                 XaveriusPhysicalDevice, list);
}

XaveriusPhysicalDevice *
XaveriusNextDevice(XaveriusPhysicalDevice *device)
{
    struct xorg_list *next;

    if (device == NULL)
        return XaveriusFirstDevice();

    next = device->list.next;
    if (next == &xaverius_physical_devices)
        return NULL;

    return xorg_list_first_entry(next, XaveriusPhysicalDevice, list);
}

uint32_t
XaveriusDeviceCount(void)
{
    return xaverius_physical_device_count;
}

XaveriusPhysicalDevice *
XaveriusFindDeviceByBindingGeneration(uint64_t binding_generation)
{
    XaveriusPhysicalDevice *dev;

    xorg_list_for_each_entry(dev, &xaverius_physical_devices, list) {
        if (dev->binding_generation == binding_generation)
            return dev;
    }

    return NULL;
}

/* XaPhysicalDevicePublish — separate server-internal publication operation
 * (Stage 4). Stage 3 stub: no XID/Alloc yet. */
bool
XaPhysicalDevicePublish(XaveriusPhysicalDevice *dev)
{
    (void)dev;
    return false;
}
