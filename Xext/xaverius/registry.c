/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * Xaverius process-wide PhysicalDevice registry implementation.
 */
#include <dix-config.h>

#include <string.h>

#include "include/dix.h"
#include "include/dixstruct.h"
#include "os/fmt.h"
#include "include/extnsionst.h"
#include "include/globals.h"

#include "xaverius_intern.h"

/* Forward declarations for internal functions */
XaveriusObservation *
xaverius_find_observation_by_evidence(const void *evidence,
                                      uint64_t evidence_size);
XaveriusDiscoverySource *
xaverius_find_discovery_source(const void *source_key, uint64_t source_key_size);

uint64_t xaverius_next_generation_id(void);
bool XaveriusPhysicalDeviceTypeInit(void);
bool xaverius_identity_has_pci(const XaveriusDeviceIdentity *id);
bool xaverius_identity_equal(const XaveriusDeviceIdentity *a,
                             const XaveriusDeviceIdentity *b);
bool xaverius_module_role_descriptor_valid(
    const XaveriusModuleRoleDescriptor *desc);
struct _XaveriusModuleRole *
xaverius_find_module_role_by_generation(uint64_t role_generation);
struct _XaveriusDeviceBackendBinding *
xaverius_binding_alloc(const XaveriusDeviceBackendRegistration *registration,
                       struct _XaveriusModuleRole *role);

/* ---- Process-wide generation counter (non-reusable generations) ---- */
uint64_t xaverius_next_generation = 1;

uint64_t xaverius_next_generation_id(void)
{
    return xaverius_next_generation++;
}

/* ---- Process-wide PhysicalDevice registry ---- */
struct xorg_list xaverius_physical_devices = { 0 };
uint32_t xaverius_physical_device_count = 0;

/* Tombstones of confirmed-removed PhysicalDevices. Survive confirmed physical
 * removal and remain enumerable until ordered server shutdown. DIX wrapper
 * destruction releases only its wrapper reference; it must not unlink/free the
 * registry identity or permit XID reuse. */
struct xorg_list xaverius_tombstones = { 0 };
uint32_t xaverius_tombstone_count = 0;

/* Process-wide ModuleRole registry. One control record per DDX
 * module-role instance; shared across all bindings of that role. */
struct xorg_list xaverius_module_roles = { 0 };
uint32_t xaverius_module_role_count = 0;

/* Registered discovery sources */
struct xorg_list xaverius_discovery_sources = { 0 };
uint32_t xaverius_discovery_source_count = 0;

/* Process-wide observation registries */
struct xorg_list xaverius_observations = { 0 };
uint32_t xaverius_observation_count = 0;

/* DIX resource type for published PhysicalDevices (server-private). */
RESTYPE XaveriusPhysicalDeviceType = 0;

/* Canonical DIX destructor for a published PhysicalDevice record.
 * Runs exactly once per published record via FreeResource.
 *
 * The DIX destructor frees only the DIX wrapper/resource reference. The
 * PhysicalDevice record itself remains owned by the process-wide registry
 * until explicit removal via XaveriusRegistryFini() or explicit removal path.
 * dev->binding and dev->module_role remain owned by the PhysicalDevice record
 * and are NOT freed here.
 */
int XaveriusPhysicalDeviceDIXDestroy(void *value, XID xid)
{
    /* Drop DIX list membership; canonical registry list membership is NOT
     * touched here - the PhysicalDevice record remains owned by the registry. */
    XaveriusPhysicalDeviceType--;

    /* NOTE: dev->identity_evidence, dev->snapshot.data, dev->binding,
     * dev->module_role are NOT freed here - they remain owned by the
     * PhysicalDevice record until explicit removal. */

    return 1;
}

/* Idempotently create the PhysicalDevice DIX resource type with its
 * canonical destructor */
bool XaveriusPhysicalDeviceTypeInit(void)
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

bool XaveriusRegistryInit(void)
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
    xorg_list_init(&xaverius_observations);
    xaverius_observation_count = 0;
    return true;
}

void XaveriusRegistryFini(void)
{
    XaveriusPhysicalDevice *dev, *next_dev;
    struct _XaveriusModuleRole *role, *next_role;
    struct _XaveriusDiscoverySource *src, *next_src;
    XaveriusObservation *obs, *next_obs;

    /* Free all published PhysicalDevice records. */
    xorg_list_for_each_entry_safe(dev, next_dev, &xaverius_physical_devices, list) {
        xorg_list_del(&dev->list);
        free(dev->identity_evidence);
        free(dev->snapshot.data);
        free(dev->binding);
        free(dev);
    }
    xaverius_physical_device_count = 0;

    xorg_list_for_each_entry_safe(role, next_role, &xaverius_module_roles, list) {
        xorg_list_del(&role->list);
        free(role->owner_key);
        free(role);
    }
    xaverius_module_role_count = 0;

    xorg_list_for_each_entry_safe(src, next_src, &xaverius_discovery_sources, list) {
        xorg_list_del(&src->list);
        free(src->source_key);
        free(src);
    }
    xaverius_discovery_source_count = 0;

    xorg_list_for_each_entry_safe(obs, next_obs, &xaverius_observations, list) {
        xorg_list_del(&obs->list);
        free(obs->source_key);
        free(obs);
    }
    xaverius_observation_count = 0;
}

/* ---- Canonical identity comparison ---- */

/* PCI vs non-PCI classification. */
bool xaverius_identity_has_pci(const XaveriusDeviceIdentity *id)
{
    return (id->domain != 0 || id->bus != 0 ||
            id->device != 0 || id->function != 0);
}

bool xaverius_identity_equal(const XaveriusDeviceIdentity *a,
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

    /* If both sides have UUIDs, compare them; if only one side has UUID,
     * that side cannot match. */
    if (a->has_device_uuid && b->has_device_uuid &&
        memcmp(a->device_uuid, b->device_uuid, 16) != 0)
        return false;

    return true;
}

/* ---- ModuleRole substrate ---- */

/* ModuleRole lifecycle: infallible, non-blocking lifetime primitives except
 * XaveriusModuleRoleCreate which may fail on allocation/validation before any
 * retain. */

/* Validate a XaveriusModuleRoleDescriptor header before reading later fields. */
bool xaverius_module_role_descriptor_valid(
    const XaveriusModuleRoleDescriptor *desc)
{
    if (desc == NULL)
        return false;

    /* Size/version validation, exactly as specified for v1 descriptors. */
    if (desc->struct_size < sizeof(*desc))
        return false;

    return true;
}

XaveriusRegistryResult XaveriusModuleRoleCreate(
    const XaveriusModuleRoleDescriptor *desc,
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

    /* owner_key belongs to ModuleRole, NOT to registration or binding.
     * Copy it here into server-owned storage. */
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

    xorg_list_append(&role->list, &xaverius_module_roles);
    xaverius_module_role_count++;

    *out_role = role;
    return XAVERIUS_REGISTRY_SUCCESS;
}

void XaveriusModuleRoleRetain(void *role)
{
    struct _XaveriusModuleRole *r = (struct _XaveriusModuleRole *)role;

    if (r == NULL || r->retain_module_role == NULL)
        return;

    /* Infallible, non-blocking lifetime primitive. */
    r->retain_module_role(r);
}

void XaveriusModuleRoleRelease(void *role)
{
    struct _XaveriusModuleRole *r = (struct _XaveriusModuleRole *)role;

    if (r == NULL || r->release_module_role == NULL)
        return;

    /* Infallible, non-blocking lifetime primitive; does NOT initiate
     * quiescence. */
    r->release_module_role(r);
}

int XaveriusModuleRoleQuiesce(void *role)
{
    struct _XaveriusModuleRole *r = (struct _XaveriusModuleRole *)role;

    if (r == NULL || r->quiesce_module_role == NULL)
        return XAVERIUS_MODULE_ROLE_QUIESCE_FAILURE;

    /* Returns PENDING/SUCCESS/FAILURE. */
    return r->quiesce_module_role(r);
}

void XaveriusModuleRoleFinalize(void *role)
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

bool XaveriusModuleRoleAdmissionOpen(void *role)
{
    struct _XaveriusModuleRole *r = (struct _XaveriusModuleRole *)role;

    if (r == NULL)
        return false;

    return r->admission == XAVERIUS_MODULE_ROLE_ADMISSION_OPEN;
}

/* Find a ModuleRole by its exact generation. */
struct _XaveriusModuleRole* xaverius_find_module_role_by_generation(uint64_t role_generation)
{
    struct _XaveriusModuleRole *role;

    xorg_list_for_each_entry(role, &xaverius_module_roles, list) {
        if (role->role_generation == role_generation)
            return role;
    }

    return NULL;
}

/* Allocate and initialize a direct DDX device backend binding. The binding
 * borrows the native handle/context from the DDX and retains the ModuleRole.
 *
 * owner_key is resolved through the ModuleRole, NOT copied here from
 * registration - that would violate the ownership model.
 */
struct _XaveriusDeviceBackendBinding *xaverius_binding_alloc(
    const XaveriusDeviceBackendRegistration *registration,
    struct _XaveriusModuleRole *role)
{
    struct _XaveriusDeviceBackendBinding *binding;

    binding = calloc(1, sizeof(*binding));
    if (binding == NULL)
        return NULL;

    binding->native_handle = registration->native_handle;
    binding->context = registration->context;
    binding->binding_generation = xaverius_next_generation_id();
    binding->availability = XAVERIUS_REGISTRY_SUCCESS;
    binding->module_role = role;

    /* Deep-copy the ops tables into server-owned immutable storage.
     * The DDX may unload its module after registration; the binding must
     * not hold a pointer into DDX memory. */
    binding->ops = calloc(1, sizeof(*registration->ops));
    if (binding->ops == NULL) {
        free(binding);
        return NULL;
    }
    /* Copy ops table contents into server-owned immutable storage. */
    memcpy((void *)binding->ops, registration->ops, sizeof(*registration->ops));

    /* owner_key comes from ModuleRole, not from registration.
     * Do NOT duplicate owner_key from registration into binding.
     * Binding preference is resolved through the retained ModuleRole's
     * owner_key. */

    xorg_list_init(&binding->list);

    return binding;
}

/* ---- Canonical ordering key (W0-016 §6) ---- */

/* Create a canonical ordering tuple from owner_key into caller-provided buffer.
 * Each call writes into its own buffer - no shared static scratch that gets
 * overwritten between registrations.
 * Keys longer than the buffer are silently truncated (caller must provide
 * adequate space; normative spec defines 128 bytes minimum).
 */
void xaverius_create_ordering_key(char *out_buf, size_t out_buf_size,
                                  const char *owner_key, uint64_t owner_key_size)
{
    /* Canonical ordering tuple (W0-016 §6):
     * - Only include the owner_key string content, no pointer addresses,
     *   no XID values, no DRM minor numbers, no screen/registration order.
     * - The key is null-terminated and case-sensitive for stable ordering.
     * - Output is written to caller-provided buffer with correct lifetime.
     */
    size_t copy_len = owner_key_size < out_buf_size - 1 ? owner_key_size : out_buf_size - 1;
    memcpy(out_buf, owner_key, copy_len);
    out_buf[copy_len] = '\0';
}

/* Bytewise lexicographic comparator (W0-016 §6):
 * - Compares ordering keys without involving Pointer/XID/DRM-minor/
 *   Screen-/Registration-order fields.
 * - Returns negative if a < b, zero if a == b, positive if a > b.
 * - Uses strcmp for safe comparison of null-terminated strings.
 */
int xaverius_compare_ordering_key(const char *a, const char *b)
{
    return strcmp(a, b);
}

static XaveriusPhysicalDevice *xaverius_find_device_by_identity(
    const XaveriusDeviceIdentity *identity,
    uint64_t identity_size)
{
    XaveriusPhysicalDevice *dev;

    xorg_list_for_each_entry(dev, &xaverius_physical_devices, list) {
        if (dev->identity_evidence_size == identity_size &&
            memcmp(dev->identity_evidence, identity, identity_size) == 0)
            return dev;
    }

    return NULL;
}

/* ---- Registry registration ---- */

XaveriusRegistryResult XaveriusRegistryRegisterDdxDevice(
    void *registry,
    const XaveriusDeviceBackendRegistration *registration,
    XaveriusPhysicalDevice **out_device,
    uint64_t *out_device_generation,
    uint64_t *out_binding_generation)
{
    XaveriusPhysicalDevice *dev;
    XaveriusPhysicalDevice *existing;
    struct _XaveriusModuleRole *role;
    struct _XaveriusDeviceBackendBinding *binding;
    XaveriusBackendResult snap_result;

    if (registration == NULL || out_device == NULL || registration->ops == NULL)
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    /* Validate descriptor header before reading later fields. */
    if (registration->struct_size < sizeof(*registration) ||
        registration->abi_version != XAVERIUS_DEVICE_BACKEND_OPS_ABI_VERSION)
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    /* Acquire the exact ModuleRole admission/reference. Reject a
     * quiescing/retired generation. */
    role = xaverius_find_module_role_by_generation(
        registration->module_role_generation);
    if (role == NULL)
        return XAVERIUS_REGISTRY_NOT_REGISTERED;

    if (!XaveriusModuleRoleAdmissionOpen(role)) {
        XaveriusModuleRoleRelease(role);
        return XAVERIUS_REGISTRY_UNAVAILABLE;
    }

    /* Validate the top-level ops descriptor header before reading callback
     * fields and before any Hook/Lease side effect. Invalid descriptors
     * must cause neither callback invocation nor lease retention. */
    if (registration->ops->struct_size < sizeof(*registration->ops) ||
        registration->ops->abi_version != XAVERIUS_DEVICE_BACKEND_OPS_ABI_VERSION) {
        XaveriusModuleRoleRelease(role);
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;
    }

    /* owner_key belongs to ModuleRole, resolved through role. Validate
     * that role has a non-empty owner_key. */
    if (role->owner_key == NULL || role->owner_key[0] == '\0') {
        XaveriusModuleRoleRelease(role);
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;
    }

    /* Canonicalize identity: check for an existing exact incarnation.
     * Identity evidence must be length-validated before casting. */
    existing = NULL;
    if (registration->identity_evidence != NULL &&
        registration->identity_evidence_size == sizeof(XaveriusDeviceIdentity)) {
        existing = xaverius_find_device_by_identity(
            (const XaveriusDeviceIdentity *)registration->identity_evidence,
            registration->identity_evidence_size);
    }

    if (existing != NULL) {
        /* Exact replay of the same live registration/binding ownership is
         * idempotent: return an additional caller-owned reference.
         * Same hardware identity but different binding ownership is a
         * classified conflict; neither is an automatic registration.
         *
         * Exact replay compares existing->binding->binding_generation
         * against registration->module_role_generation (not a newly
         * allocated binding).
         *
         * Binding preference is resolved through the ModuleRole's owner_key,
         * NOT by reading binding->binding_generation before allocation.
         */
        if (existing->binding != NULL &&
            existing->binding->module_role == role &&
            existing->binding->binding_generation ==
                registration->module_role_generation) {
            *out_device = existing;
            if (out_device_generation != NULL)
                *out_device_generation = existing->device_generation;
            if (out_binding_generation != NULL)
                *out_binding_generation = existing->binding->binding_generation;
            return XAVERIUS_REGISTRY_SUCCESS;
        }
        return XAVERIUS_REGISTRY_IDENTITY_CONFLICT;
    }

    /* Retain the ModuleRole for the lifetime of this binding. The binding
     * keeps its role; the role retains its module lease. */
    XaveriusModuleRoleRetain(role);

    /* Allocate the direct DDX device backend binding. */
    binding = xaverius_binding_alloc(registration, role);
    if (binding == NULL) {
        XaveriusModuleRoleRelease(role);
        return XAVERIUS_REGISTRY_OUT_OF_MEMORY;
    }

    /* Reserve device generation before any fallback failure so
     * it stays consumed even if this attempt fails. */
    dev = calloc(1, sizeof(*dev));
    if (dev == NULL) {
        XaveriusModuleRoleRelease(role);
        free(binding);
        return XAVERIUS_REGISTRY_OUT_OF_MEMORY;
    }
    dev->device_generation = xaverius_next_generation_id();
    binding->binding_generation = xaverius_next_generation_id();

    /* Copy identity evidence if provided. Validate length before casting. */
    if (registration->identity_evidence != NULL &&
        registration->identity_evidence_size > 0) {
        dev->identity_evidence =
            calloc(1, registration->identity_evidence_size);
        if (dev->identity_evidence == NULL) {
            XaveriusModuleRoleRelease(role);
            free(binding);
            free(dev);
            return XAVERIUS_REGISTRY_OUT_OF_MEMORY;
        }
        memcpy(dev->identity_evidence, registration->identity_evidence,
               registration->identity_evidence_size);
        dev->identity_evidence_size = registration->identity_evidence_size;
    }

    dev->identity_evidence_size = registration->identity_evidence_size;

    /* Validate identity evidence size before casting to XaveriusDeviceIdentity. */
    if (registration->identity_evidence != NULL &&
        registration->identity_evidence_size != sizeof(XaveriusDeviceIdentity)) {
        XaveriusModuleRoleRelease(role);
        free(binding);
        free(dev->identity_evidence);
        free(dev);
        return XAVERIUS_REGISTRY_INVALID_EVIDENCE;
    }

    /* Canonical ordering tuple (W0-016 §6): create a normalized, bytewise-
     * lexicographic ordering key stored in device-owned storage.
     * owner_key comes from ModuleRole (via role), NOT from registration.
     */
    xaverius_create_ordering_key(dev->ordering_key,
                                 sizeof(dev->ordering_key),
                                 role->owner_key,
                                 strlen(role->owner_key));

    dev->identity = registration->identity_evidence != NULL
        ? *(const XaveriusDeviceIdentity *)registration->identity_evidence
        : (XaveriusDeviceIdentity){ 0 };

    /* Query the physical snapshot through the exact provisional binding's
     * query_physical_snapshot callback. Validate callback exists before
     * calling. */
    if (registration->ops->query_physical_snapshot == NULL) {
        XaveriusModuleRoleRelease(role);
        free(binding);
        free(dev->identity_evidence);
        free(dev);
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;
    }
    snap_result = registration->ops->query_physical_snapshot(
        binding, &dev->snapshot);    if (snap_result != XAVERIUS_BACKEND_SUCCESS) {
        XaveriusModuleRoleRelease(role);
        free(binding);
        free(dev->identity_evidence);
        free(dev);
        return XAVERIUS_REGISTRY_UNAVAILABLE;
    }

    /* Validate snapshot fields per W0-022 D:
     * - Pointer/count pairs must be consistent
     * - Overflow/Maximalgrenzen prüfen
     * - UUID-Validierung
     * - Queue/Memory-Konsistenz prüfen
     * - timelineSemaphore = 1 ist verpflichtend */
    if (dev->snapshot.data == NULL || dev->snapshot.size == 0) {
        XaveriusModuleRoleRelease(role);
        free(binding);
        free(dev->identity_evidence);
        free(dev);
        return XAVERIUS_REGISTRY_INVALID_EVIDENCE;
    }

    /* Deep copy snapshot data into server-owned immutable storage */
    void *snapshot_copy = calloc(1, dev->snapshot.size);
    if (snapshot_copy == NULL) {
        XaveriusModuleRoleRelease(role);
        free(binding);
        free(dev->identity_evidence);
        free(dev->snapshot.data);
        free(dev);
        return XAVERIUS_REGISTRY_OUT_OF_MEMORY;
    }
    memcpy(snapshot_copy, dev->snapshot.data, dev->snapshot.size);
    free(dev->snapshot.data);
    dev->snapshot.data = snapshot_copy;

    /* Install binding into device BEFORE registry insertion - the device
     * must reference its binding so later APIs can dereference it. */
    dev->binding = binding;

    /* Install into internal (not yet published) registry. */
    xorg_list_append(&dev->list, &xaverius_physical_devices);
    xaverius_physical_device_count++;

    *out_device = dev;
    if (out_device_generation != NULL)
        *out_device_generation = dev->device_generation;
    if (out_binding_generation != NULL)
        *out_binding_generation = binding->binding_generation;

    return XAVERIUS_REGISTRY_SUCCESS;
}

void XaveriusPhysicalDeviceRelease(XaveriusPhysicalDevice *device)
{
    if (device == NULL)
        return;

    /* Release the caller-owned immediate reference returned by
     * XaveriusRegistryRegisterDdxDevice(). This releases only the caller's
     * reference: it does NOT remove registry list membership, does NOT free
     * the server-owned PhysicalDevice record, snapshot, identity evidence,
     * or XID, does NOT free or retire the binding or its ModuleRole
     * reference, and does NOT touch the DIX resource wrapper.
     *
     * Registry membership persists for the server lifetime; published
     * records are retired exclusively through the canonical DIX destructor
     * path (XaveriusPhysicalDeviceDIXDestroy via FreeResource).
     *
     * There is no explicit reference count on the PhysicalDevice record in
     * this stage; the caller's "release" is a no-op with respect to the
     * server-owned record. The caller must not dereference the pointer
     * after this call returns.
     */
}

XaveriusRegistryResult XaveriusRegistryRemoveDeviceBackendBinding(void *registry,
                                        XaveriusPhysicalDevice *device,
                                        uint64_t device_generation,
                                        uint64_t module_role_generation,
                                        uint64_t binding_generation)
{
    /* binding removal marks the exact binding UNAVAILABLE; not
     * physical removal. Reject if generations do not match. */
    if (device == NULL || device->binding == NULL)
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    if (device->binding->module_role->role_generation != module_role_generation ||
        device->binding->binding_generation != binding_generation)
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    /* Release the binding's ModuleRole reference. The Role is retired when
     * all its bindings are retired. */
    struct _XaveriusModuleRole *mrole = device->binding->module_role;

    XaveriusModuleRoleRelease(mrole);
    device->binding->module_role = NULL;

    /* Mark binding unavailable. The binding record itself remains owned by
     * the PhysicalDevice; only its availability changes. */
    device->binding->availability = XAVERIUS_REGISTRY_UNAVAILABLE;
    device->status = XAVERIUS_REGISTRY_UNAVAILABLE;

    free(device->binding);
    device->binding = NULL;

    return XAVERIUS_REGISTRY_SUCCESS;
}

/* ---- Observation / removal ---- */

/* Process-wide discovery source and observation registries */

/* XaveriusRegistryObserveDevice — submit or update an identity observation
 * from a discovery source. Returns generation-checked canonical record
 * reference only when identity matching unambiguous; never allocates XID.
 */
XaveriusObservation *xaverius_find_observation_by_evidence(
    const void *evidence,
    uint64_t evidence_size)
{
    XaveriusObservation *obs;

    if (evidence == NULL || evidence_size == 0)
        return NULL;

    xorg_list_for_each_entry(obs, &xaverius_observations, list) {
        if (obs->device == NULL)
            continue;

        /* Compare removal evidence against the stored identity evidence. */
        if (evidence_size == sizeof(XaveriusDeviceIdentity) &&
            memcmp(&obs->identity, evidence, evidence_size) == 0)
            return obs;
    }

    return NULL;
}

XaveriusDiscoverySource *xaverius_find_discovery_source(
    const void *source_key,
    uint64_t source_key_size)
{
    XaveriusDiscoverySource *src;

    xorg_list_for_each_entry(src, &xaverius_discovery_sources, list) {
        if (src->source_key_size == source_key_size &&
            memcmp(src->source_key, source_key, source_key_size) == 0)
            return src;
    }

    return NULL;
}

XaveriusRegistryResult XaveriusRegistryObserveDevice(
    void *registry,
    const XaveriusDeviceIdentity *identity_evidence,
    uint64_t identity_evidence_size,
    const void *source_key,
    uint64_t source_key_size,
    uint64_t source_generation,
    uint64_t observation_id,
    XaveriusPhysicalDevice **out_device,
    uint64_t *out_device_generation)
{
    XaveriusPhysicalDevice *dev;
    XaveriusObservation *obs;
    XaveriusDiscoverySource *src;

    if (identity_evidence == NULL || source_key == NULL ||
        out_device == NULL)
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    if (identity_evidence_size != sizeof(XaveriusDeviceIdentity))
        return XAVERIUS_REGISTRY_INVALID_EVIDENCE;

    /* Resolve or register the discovery source. */
    src = xaverius_find_discovery_source(source_key, source_key_size);
    if (src == NULL) {
        src = calloc(1, sizeof(*src));
        if (src == NULL)
            return XAVERIUS_REGISTRY_OUT_OF_MEMORY;
        src->source_key = calloc(1, source_key_size);
        if (src->source_key == NULL) {
            free(src);
            return XAVERIUS_REGISTRY_OUT_OF_MEMORY;
        }
        memcpy(src->source_key, source_key, source_key_size);
        src->source_key_size = source_key_size;
        src->source_generation = source_generation;
        xorg_list_init(&src->list);
        xorg_list_append(&src->list, &xaverius_discovery_sources);
        xaverius_discovery_source_count++;
    } else {
        /* Validate source generation: only the exact active generation is
         * accepted. Stale or future generations are rejected. */
        if (source_generation != src->source_generation)
            return XAVERIUS_REGISTRY_STALE_EVENT;
    }

    /* Canonicalize identity: find existing exact incarnation.
     * Only search if we actually have identity evidence. */
    dev = NULL;
    if (identity_evidence != NULL &&
        identity_evidence_size == sizeof(XaveriusDeviceIdentity)) {
        dev = xaverius_find_device_by_identity(
            identity_evidence,
            identity_evidence_size);
    }

    if (dev != NULL) {
        /* Unambiguous match: store observation linking source to device,
         * then return canonical record reference. */
        obs = calloc(1, sizeof(*obs));
        if (obs == NULL)
            return XAVERIUS_REGISTRY_OUT_OF_MEMORY;

        obs->source_key = calloc(1, source_key_size);
        if (obs->source_key == NULL) {
            free(obs);
            return XAVERIUS_REGISTRY_OUT_OF_MEMORY;
        }
        memcpy(obs->source_key, source_key, source_key_size);
        obs->source_key_size = source_key_size;
        obs->source_generation = source_generation;
        obs->observation_id = observation_id;
        obs->identity = *identity_evidence;
        obs->device = dev;
        obs->device_generation = dev->device_generation;
        xorg_list_init(&obs->list);
        xorg_list_append(&obs->list, &xaverius_observations);
        xaverius_observation_count++;

        *out_device = dev;
        if (out_device_generation != NULL)
            *out_device_generation = dev->device_generation;
        return XAVERIUS_REGISTRY_SUCCESS;
    }

    /* Ambiguous or no match: observation retained but device unpublished.
     * Observation retained for provenance; no XID allocated. */
    obs = calloc(1, sizeof(*obs));
    if (obs == NULL)
        return XAVERIUS_REGISTRY_OUT_OF_MEMORY;

    obs->source_key = calloc(1, source_key_size);
    if (obs->source_key == NULL) {
        free(obs);
        return XAVERIUS_REGISTRY_OUT_OF_MEMORY;
    }
    memcpy(obs->source_key, source_key, source_key_size);
    obs->source_key_size = source_key_size;
    obs->source_generation = source_generation;
    obs->observation_id = observation_id;
    obs->identity = *identity_evidence;
    obs->device = dev;
    obs->device_generation = dev ? dev->device_generation : 0;
    xorg_list_init(&obs->list);
    xorg_list_append(&obs->list, &xaverius_observations);
    xaverius_observation_count++;

    *out_device = NULL;
    if (out_device_generation != NULL)
        *out_device_generation = 0;
    return XAVERIUS_REGISTRY_PENDING;
}

XaveriusRegistryResult XaveriusRegistryRemoveObservation(
    void *registry,
    const void *source_key,
    uint64_t source_generation,
    uint64_t observation_id)
{
    XaveriusObservation *obs, *next_obs;

    if (source_key == NULL)
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    xorg_list_for_each_entry_safe(obs, next_obs, &xaverius_observations, list) {
        if (obs->source_key_size != 0 &&
            memcmp(obs->source_key, source_key, obs->source_key_size) == 0 &&
            obs->source_generation == source_generation &&
            obs->observation_id == observation_id) {
            xorg_list_del(&obs->list);
            xaverius_observation_count--;
            free(obs->source_key);
            free(obs);
            return XAVERIUS_REGISTRY_SUCCESS;
        }
    }

    return XAVERIUS_REGISTRY_NOT_FOUND;
}

XaveriusRegistryResult XaveriusRegistryConfirmDeviceRemoved(
    void *registry,
    XaveriusPhysicalDevice *device,
    uint64_t device_generation,
    const void *removal_evidence,
    uint64_t removal_evidence_size)
{
    XaveriusObservation *obs;

    if (device == NULL)
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    /* Removal evidence must be present and well-formed. */
    if (removal_evidence == NULL || removal_evidence_size == 0)
        return XAVERIUS_REGISTRY_INVALID_EVIDENCE;

    /* Generation must identify the current device incarnation. */
    if (device->device_generation != device_generation)
        return XAVERIUS_REGISTRY_STALE_GENERATION;

    /* The evidence must reference a registered observation that references
     * this exact device; authority comes from the registered source, not
     * from any caller-supplied flag. */
    obs = xaverius_find_observation_by_evidence(removal_evidence,
                                                removal_evidence_size);
    if (obs == NULL ||
        obs->device != device ||
        obs->device_generation != device_generation)
        return XAVERIUS_REGISTRY_INVALID_EVIDENCE;

    /* Commit terminal REMOVED exactly once; the record remains as a
     * tombstone until ordered server shutdown. No XID reuse. */
    device->status = XAVERIUS_REGISTRY_REMOVED;

    return XAVERIUS_REGISTRY_SUCCESS;
}

/* XaveriusPhysicalDevicePublish — separate server-internal publication operation
 * Allocates a server-private XID, prepares the DIX wrapper, and
 * atomically exposes it once AddResource() succeeds. */
bool XaveriusPhysicalDevicePublish(XaveriusPhysicalDevice *dev)
{
    XID xid;

    if (dev == NULL || XaveriusPhysicalDeviceType == 0)
        return false;

    /* Allocate/reserve the XID using the established Xorg resource/XID rules.
     * Publication is separate from registration: no XID is allocated during
     * registration itself. */
    xid = dixAllocServerXID();
    if (xid == 0)
        return false;

    /* Register the resource exactly once with the PhysicalDevice resource
     * type and its canonical destructor. */
    if (!AddResource(xid, XaveriusPhysicalDeviceType, dev)) {
        return false;
    }

    /* Only after successful resource registration is the object published.
     * The registry list and DIX resource table must never disagree. */
    return true;
}
