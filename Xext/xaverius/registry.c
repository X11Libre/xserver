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
#include "xaverius_priv.h"

/* Forward declarations for internal functions */
static XaveriusObservation *
xaverius_find_observation_by_removal_evidence(const XaveriusDeviceRemovalEvidence *evidence);
XaveriusDiscoverySource *
xaverius_find_discovery_source(const char *source_key);

uint64_t xaverius_next_generation_id(void);
bool XaveriusPhysicalDeviceTypeInit(void);
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

/* ---- Process-wide observation ID counter (non-reusable) ---- */
uint64_t xaverius_next_observation_id = 1;

uint64_t xaverius_next_observation_id_fn(void)
{
    return xaverius_next_observation_id++;
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
    XaveriusPhysicalDeviceDIXWrapper *wrapper = value;

    /* The DIX destructor must not modify XaveriusPhysicalDeviceType (the
     * resource type ID). It releases only the DIX wrapper reference. The
     * PhysicalDevice record remains owned by the registry until explicit
     * removal via XaveriusRegistryFini() or removal path. */
    if (wrapper != NULL) {
        XaveriusPhysicalDevice *dev = wrapper->dev;
        if (wrapper->refcount > 0) {
            wrapper->refcount--;
            if (wrapper->refcount == 0) {
                /* Release PhysicalDevice's DIX-resource ownership reference.
                 * The PhysicalDevice tracks its wrapper; clear the pointer
                 * so the device knows its DIX resource is gone. */
                if (dev != NULL) {
                    dev->dix_wrapper = NULL;
                    /* Release the separate PhysicalDevice DIX-resource
                     * ownership refcount. Invariant: must be >0 here;
                     * double-release is a logic error. */
                    assert(dev->dix_resource_refcount > 0);
                    dev->dix_resource_refcount--;
                }
                free(wrapper);
            }
        }
    }
    (void)xid;
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

/* Idempotent registry initialization.
 * Repeated calls must not reset live lists, counts, roles, devices,
 * tombstones, sources, or observations. The first call initializes
 * empty state; subsequent calls are no-ops (return true without mutating).
 */
static bool xaverius_registry_initialized = false;

bool XaveriusRegistryInit(void)
{
    if (!XaveriusPhysicalDeviceTypeInit())
        return false;

    if (xaverius_registry_initialized)
        return true;

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
    xaverius_registry_initialized = true;
    return true;
}

/* Helper to free a binding and all its nested operation tables.
 * Releases the binding's retained ModuleRole reference, if any. */
static void xaverius_binding_free(struct _XaveriusDeviceBackendBinding *binding)
{
    if (binding == NULL)
        return;

    if (binding->module_role != NULL) {
        XaveriusModuleRoleRelease(binding->module_role);
        binding->module_role = NULL;
    }

    if (binding->ops != NULL) {
        free((void *)binding->ops->buffer_memory);
        free((void *)binding->ops->shader_pipeline);
        free((void *)binding->ops->commands);
        free((void *)binding->ops->synchronization);
        free(binding->ops);
    }
    free(binding);
}

void XaveriusRegistryFini(void)
{
    XaveriusPhysicalDevice *dev, *next_dev;
    struct _XaveriusModuleRole *role, *next_role;
    struct _XaveriusDiscoverySource *src, *next_src;
    XaveriusObservation *obs, *next_obs;

    /* Step 1: Close admission on all roles — stop new callbacks.
     * Use the canonical XaveriusModuleRoleAdmissionClose() to ensure
     * callback_admission_open is also cleared (checked by AcquireExecRef). */
    xorg_list_for_each_entry(role, &xaverius_module_roles, list) {
        XaveriusModuleRoleAdmissionClose(role);
        role->admission = XAVERIUS_MODULE_ROLE_ADMISSION_CLOSED;
    }

    /* Step 2: Stop discovery sources — release discovery source records
     * before role finalization so no new observations can be registered
     * during quiescence. */
    xorg_list_for_each_entry_safe(src, next_src, &xaverius_discovery_sources, list) {
        xorg_list_del(&src->list);
        free((void *)src->source_key);
        free(src);
    }
    xaverius_discovery_source_count = 0;

    /* Step 3: Quiesce all roles — drain accepted callbacks/operations.
     * Record result in quiesce_state for later use. PENDING/FAILURE roles
     * are retained/quarantined — their code and references stay. */
    xorg_list_for_each_entry(role, &xaverius_module_roles, list) {
        XaveriusModuleQuiescenceResult qr = XaveriusModuleRoleQuiesce(role);
        if (qr == XAVERIUS_MODULE_QUIESCE_SUCCESS)
            role->quiesce_state = XAVERIUS_MODULE_QUIESCE_SUCCESS;
        else if (qr == XAVERIUS_MODULE_QUIESCE_FAILURE)
            role->quiesce_state = XAVERIUS_MODULE_QUIESCE_FAILURE;
        /* PENDING/ACTIVE: leave state unchanged — role is retained. */
    }

    /* Step 4: Release devices/bindings only for successfully quiesced roles.
     * PENDING/FAILURE roles retain code and references. */
    {
        unsigned int freed_devices = 0;
        xorg_list_for_each_entry_safe(dev, next_dev, &xaverius_physical_devices, list) {
            if (dev->binding != NULL && dev->binding->module_role != NULL &&
                dev->binding->module_role->quiesce_state != XAVERIUS_MODULE_QUIESCE_SUCCESS) {
                continue; /* Role not quiesced — retain/quarantine device. */
            }
            xorg_list_del(&dev->list);
            if (dev->xid != 0) {
                FreeResource(dev->xid, RT_NONE);
                dev->xid = 0;
            }
            /* Clear DIX wrapper reference if present — the DIX resource
             * was freed by FreeResource above, which calls DIXDestroy. */
            dev->dix_wrapper = NULL;
            free(dev->identity_evidence);
            free(dev->snapshot.data);
            xaverius_binding_free(dev->binding);
            dev->binding = NULL;
            free(dev);
            freed_devices++;
        }
        if (freed_devices >= xaverius_physical_device_count)
            xaverius_physical_device_count = 0;
        else
            xaverius_physical_device_count -= freed_devices;
    }

    {
        unsigned int freed_tombstones = 0;
        xorg_list_for_each_entry_safe(dev, next_dev, &xaverius_tombstones, list) {
            if (dev->binding != NULL && dev->binding->module_role != NULL &&
                dev->binding->module_role->quiesce_state != XAVERIUS_MODULE_QUIESCE_SUCCESS) {
                continue; /* Role not quiesced — retain/quarantine tombstone. */
            }
            xorg_list_del(&dev->list);
            if (dev->xid != 0) {
                FreeResource(dev->xid, RT_NONE);
                dev->xid = 0;
            }
            dev->dix_wrapper = NULL;
            free(dev->identity_evidence);
            free(dev->snapshot.data);
            xaverius_binding_free(dev->binding);
            dev->binding = NULL;
            free(dev);
            freed_tombstones++;
        }
        if (freed_tombstones >= xaverius_tombstone_count)
            xaverius_tombstone_count = 0;
        else
            xaverius_tombstone_count -= freed_tombstones;
    }

    /* Step 5: Finalize only successfully quiesced roles. PENDING/FAILURE/
     * ACTIVE roles are retained/quarantined — their code and references stay. */
    xorg_list_for_each_entry_safe(role, next_role, &xaverius_module_roles, list) {
        if (role->quiesce_state == XAVERIUS_MODULE_QUIESCE_SUCCESS) {
            XaveriusModuleRoleFinalize(role);
            continue;
        }
        /* PENDING/FAILURE/ACTIVE — retain role but mark it quarantined so
         * XaveriusModuleRoleFinalize() will refuse to free it while retained
         * bindings/devices still reference it via binding->module_role. */
        XaveriusModuleRoleQuarantine(role);
    }

    xorg_list_for_each_entry_safe(obs, next_obs, &xaverius_observations, list) {
        xorg_list_del(&obs->list);
        free((void *)obs->source_key);
        free(obs);
    }
    xaverius_observation_count = 0;

    /* Only reset the initialization guard if all registry state has been
     * fully released. If PENDING/FAILURE roles, devices, tombstones,
     * discovery sources, or observations remain, a subsequent Init must
     * not clobber list heads or reset counts — the retained objects must
     * stay reachable. */
    bool all_empty =
        xorg_list_is_empty(&xaverius_physical_devices) &&
        xorg_list_is_empty(&xaverius_tombstones) &&
        xorg_list_is_empty(&xaverius_module_roles) &&
        xorg_list_is_empty(&xaverius_discovery_sources) &&
        xorg_list_is_empty(&xaverius_observations) &&
        xaverius_physical_device_count == 0 &&
        xaverius_tombstone_count == 0 &&
        xaverius_module_role_count == 0 &&
        xaverius_discovery_source_count == 0 &&
        xaverius_observation_count == 0;

    if (all_empty)
        xaverius_registry_initialized = false;
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

    /* UUID presence must be consistent: if one side has a UUID and
     * the other doesn't, they cannot be the same device. */
    if (a->has_device_uuid != b->has_device_uuid)
        return false;

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

    /* If both sides have UUIDs, compare them. UUID presence was already
     * validated as consistent above. */
    if (a->has_device_uuid && b->has_device_uuid &&
        memcmp(a->device_uuid, b->device_uuid, 16) != 0)
        return false;

    return true;
}

/* ---- ModuleRole substrate ---- */

/* ModuleRole lifecycle: infallible, non-blocking lifetime primitives except
 * XaveriusModuleRoleCreate which may fail on allocation/validation before any
 * retain. */

/* Validate a XaveriusModuleRoleDescriptor header before reading later fields.
 * Returns XAVERIUS_REGISTRY_SUCCESS on success, or appropriate error code:
 * - INVALID_ARGUMENT: struct_size too small or too large for known ABI version
 * - UNSUPPORTED: unknown ABI version
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
XaveriusRegistryResult xaverius_module_role_descriptor_validate(
    const XaveriusModuleRoleDescriptor *desc)
{
    if (desc == NULL)
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    /* Size/version validation, exactly as specified for v1 descriptors. */
    if (desc->abi_version == XAVERIUS_MODULE_ROLE_DESCRIPTOR_ABI_VERSION) {
        if (desc->struct_size != sizeof(*desc))
            return XAVERIUS_REGISTRY_INVALID_ARGUMENT;
    } else {
        return XAVERIUS_REGISTRY_UNSUPPORTED;
    }

    return XAVERIUS_REGISTRY_SUCCESS;
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
XaveriusRegistryResult XaveriusModuleRoleCreate(
    const XaveriusModuleRoleDescriptor *desc,
    XaveriusModuleRole **out_role)
{
    struct _XaveriusModuleRole *role;

    if (desc == NULL || out_role == NULL)
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    /* Validate header before reading later fields. */
    XaveriusRegistryResult valid_result = xaverius_module_role_descriptor_validate(desc);
    if (valid_result != XAVERIUS_REGISTRY_SUCCESS)
        return valid_result;

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

    role->module_lease = desc->module_lease;  /* Lease from descriptor */
    role->role_generation = xaverius_next_generation_id();
    role->admission = XAVERIUS_MODULE_ROLE_ADMISSION_OPEN;
    role->quiesce_state = XAVERIUS_MODULE_QUIESCE_ACTIVE;
    role->exec_ref = 0;
    role->callback_admission_open = 1;
    role->quarantined = 0;
    role->retain_module_role = desc->retain_module_role;
    role->release_module_role = desc->release_module_role;
    role->quiesce_module_role = desc->quiesce_module_role;

    xorg_list_init(&role->list);

    xorg_list_append(&role->list, &xaverius_module_roles);
    xaverius_module_role_count++;

    *out_role = role;
    return XAVERIUS_REGISTRY_SUCCESS;
}
#pragma GCC diagnostic pop

void XaveriusModuleRoleRetain(XaveriusModuleRole *role)
{
    struct _XaveriusModuleRole *r = (struct _XaveriusModuleRole *)role;

    if (r == NULL || r->retain_module_role == NULL)
        return;

    /* Infallible, non-blocking lifetime primitive. */
    r->retain_module_role(r);
}

void XaveriusModuleRoleRelease(XaveriusModuleRole *role)
{
    struct _XaveriusModuleRole *r = (struct _XaveriusModuleRole *)role;

    if (r == NULL || r->release_module_role == NULL)
        return;

    /* Infallible, non-blocking lifetime primitive; does NOT initiate
     * quiescence. */
    r->release_module_role(r);
}

/* Acquire a callback-execution reference before entering DDX backend module
 * code. A quarantined role or a role with closed admission rejects new
 * execution references. Returns true on success, false if admission is closed
 * or role is quarantined. */
bool XaveriusModuleRoleAcquireExecRef(XaveriusModuleRole *role)
{
    struct _XaveriusModuleRole *r = (struct _XaveriusModuleRole *)role;

    if (r == NULL || r->quarantined || r->callback_admission_open == 0)
        return false;

    r->exec_ref++;
    return true;
}

/* Release a callback-execution reference after XaveriusBackendCompletionDispatch
 * has copied/enqueued the immutable completion and returned. */
void XaveriusModuleRoleReleaseExecRef(XaveriusModuleRole *role)
{
    struct _XaveriusModuleRole *r = (struct _XaveriusModuleRole *)role;

    if (r == NULL || r->exec_ref == 0)
        return;

    r->exec_ref--;
}

/* Close admission for callbacks. Atomically marks the role as not accepting
 * new callbacks; does not cancel accepted operations. */
void XaveriusModuleRoleAdmissionClose(XaveriusModuleRole *role)
{
    struct _XaveriusModuleRole *r = (struct _XaveriusModuleRole *)role;

    if (r == NULL)
        return;

    r->callback_admission_open = 0;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
XaveriusModuleQuiescenceResult XaveriusModuleRoleQuiesce(XaveriusModuleRole *role)
{
    struct _XaveriusModuleRole *r = (struct _XaveriusModuleRole *)role;

    if (r == NULL || r->quiesce_module_role == NULL)
        return XAVERIUS_MODULE_QUIESCE_FAILURE;

    /* Quiescence SUCCESS requires no active callback executions.
     * If exec_ref > 0, the role is still in use and cannot quiesce successfully. */
    if (r->exec_ref > 0)
        return XAVERIUS_MODULE_QUIESCE_PENDING;

    /* Returns PENDING/SUCCESS/FAILURE. */
    XaveriusModuleQuiescenceResult qr = r->quiesce_module_role(r);

    if (qr == XAVERIUS_MODULE_QUIESCE_SUCCESS)
        r->quiesce_state = XAVERIUS_MODULE_QUIESCE_SUCCESS;
    else if (qr == XAVERIUS_MODULE_QUIESCE_FAILURE)
        r->quiesce_state = XAVERIUS_MODULE_QUIESCE_FAILURE;
    /* PENDING/ACTIVE: leave state unchanged — role is retained. */

    return qr;
}
#pragma GCC diagnostic pop

/* Quarantine the role. A quarantined role cannot be unloaded; its code and
 * references are retained until server shutdown or a later proven-safe
 * retirement. */
void XaveriusModuleRoleQuarantine(XaveriusModuleRole *role)
{
    struct _XaveriusModuleRole *r = (struct _XaveriusModuleRole *)role;

    if (r == NULL)
        return;

    r->quarantined = 1;
}

void XaveriusModuleRoleFinalize(XaveriusModuleRole *role)
{
    struct _XaveriusModuleRole *r = (struct _XaveriusModuleRole *)role;

    if (r == NULL)
        return;

    /* A quarantined role cannot be finalized; it is retained until server
     * shutdown or a later proven-safe retirement. */
    if (r->quarantined)
        return;

    /* A role with active callback-execution references cannot be finalized;
     * DDX backend code may still be executing. */
    if (r->exec_ref > 0)
        return;

    /* Finalization releases the module owner's original role reference via
     * the role hook and frees server-owned role storage. The caller (module
     * owner/registry shutdown path) is responsible for invoking this exactly
     * once after all binding references have been released. */
    if (r->release_module_role != NULL)
        r->release_module_role(r);

    if (!xorg_list_is_empty(&r->list)) {
        xorg_list_del(&r->list);
        if (xaverius_module_role_count > 0)
            xaverius_module_role_count--;
    }

    free(r->owner_key);
    free(r);
}

bool XaveriusModuleRoleAdmissionOpen(XaveriusModuleRole *role)
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
 *
 * On success, the binding holds a retained reference to the ModuleRole.
 * Callers must release that reference via xaverius_binding_free() on failure
 * paths after a successful allocation.
 */
struct _XaveriusDeviceBackendBinding *xaverius_binding_alloc(
    const XaveriusDeviceBackendRegistration *registration,
    struct _XaveriusModuleRole *role)
{
    struct _XaveriusDeviceBackendBinding *binding;

    binding = calloc(1, sizeof(*binding));
    if (binding == NULL)
        return NULL;

    binding->native_handle = registration->native_physical_device;
    binding->context = registration->device_context;
    binding->binding_generation = xaverius_next_generation_id();
    binding->availability = XAVERIUS_REGISTRY_SUCCESS;
    binding->module_role = role;
    XaveriusModuleRoleRetain(role);

    /* Deep-copy the ops tables into server-owned immutable storage.
     * The DDX may unload its module after registration; the binding must
     * not hold a pointer into DDX memory. */
    binding->ops = calloc(1, sizeof(*registration->ops));
    if (binding->ops == NULL) {
        XaveriusModuleRoleRelease(role);
        free(binding);
        return NULL;
    }
    /* Copy top-level ops table contents into server-owned immutable storage. */
    memcpy((void *)binding->ops, registration->ops, sizeof(*registration->ops));

    /* Deep-copy all four nested operation tables. Each table must be
     * validated and copied by-value into server-owned storage. The binding
     * must not retain any pointers into DDX-owned memory. */
    if (registration->ops->buffer_memory != NULL) {
        binding->ops->buffer_memory = calloc(1, sizeof(*registration->ops->buffer_memory));
        if (binding->ops->buffer_memory == NULL) {
            xaverius_binding_free(binding);
            return NULL;
        }
        memcpy((void *)binding->ops->buffer_memory, registration->ops->buffer_memory,
               sizeof(*registration->ops->buffer_memory));
    }

    if (registration->ops->shader_pipeline != NULL) {
        binding->ops->shader_pipeline = calloc(1, sizeof(*registration->ops->shader_pipeline));
        if (binding->ops->shader_pipeline == NULL) {
            xaverius_binding_free(binding);
            return NULL;
        }
        memcpy((void *)binding->ops->shader_pipeline, registration->ops->shader_pipeline,
               sizeof(*registration->ops->shader_pipeline));
    }

    if (registration->ops->commands != NULL) {
        binding->ops->commands = calloc(1, sizeof(*registration->ops->commands));
        if (binding->ops->commands == NULL) {
            xaverius_binding_free(binding);
            return NULL;
        }
        memcpy((void *)binding->ops->commands, registration->ops->commands,
               sizeof(*registration->ops->commands));
    }

    if (registration->ops->synchronization != NULL) {
        binding->ops->synchronization = calloc(1, sizeof(*registration->ops->synchronization));
        if (binding->ops->synchronization == NULL) {
            xaverius_binding_free(binding);
            return NULL;
        }
        memcpy((void *)binding->ops->synchronization, registration->ops->synchronization,
               sizeof(*registration->ops->synchronization));
    }

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
/* Create a canonical ordering tuple from identity and owner_key.
 * Format: <identity_class>:<pci_or_drm>:<device_uuid>:<driver_uuid>:<owner_key>
 * Returns XAVERIUS_REGISTRY_SUCCESS on success, or INVALID_ARGUMENT if the
 * output buffer is too small (caller must provide adequate space; 128 bytes min).
 * No silent truncation - a truncated key is never a valid ordering key.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
XaveriusRegistryResult xaverius_create_ordering_key(
    char *out_buf, size_t out_buf_size,
    const XaveriusDeviceIdentity *identity,
    const char *owner_key)
{
    char identity_class[8] = {0};
    char pci_or_drm[32] = {0};
    char device_uuid_str[33] = {0};  /* 32 hex + null */
    char driver_uuid_str[33] = {0};  /* 32 hex + null */

    /* Determine identity class and encode PCI/DRM component */
    if (identity->domain != 0 || identity->bus != 0 ||
        identity->device != 0 || identity->function != 0) {
        /* PCI identity */
        strcpy(identity_class, "pci");
        snprintf(pci_or_drm, sizeof(pci_or_drm), "%04x:%02x:%02x.%x",
                 identity->domain, identity->bus, identity->device, identity->function);
    } else if (identity->drm_major != 0 || identity->drm_minor != 0) {
        /* DRM identity */
        strcpy(identity_class, "drm");
        snprintf(pci_or_drm, sizeof(pci_or_drm), "%u.%u",
                 identity->drm_major, identity->drm_minor);
    } else {
        /* UUID-only identity */
        strcpy(identity_class, "uuid");
        pci_or_drm[0] = '\0';
    }

    /* Encode device UUID if present */
    if (identity->has_device_uuid) {
        for (int i = 0; i < 16; i++) {
            snprintf(device_uuid_str + i * 2, 3, "%02x", identity->device_uuid[i]);
        }
    } else {
        device_uuid_str[0] = '\0';
    }

    /* Encode driver UUID (always present) */
    for (int i = 0; i < 16; i++) {
        snprintf(driver_uuid_str + i * 2, 3, "%02x", identity->driver_uuid[i]);
    }

    /* Check buffer size before writing - no silent truncation */
    int needed = snprintf(NULL, 0,
                          "%s:%s:%s:%s:%s",
                          identity_class, pci_or_drm, device_uuid_str, driver_uuid_str,
                          owner_key ? owner_key : "");

    if (needed < 0 || (size_t)needed >= out_buf_size)
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    snprintf(out_buf, out_buf_size,
             "%s:%s:%s:%s:%s",
             identity_class, pci_or_drm, device_uuid_str, driver_uuid_str,
             owner_key ? owner_key : "");

    return XAVERIUS_REGISTRY_SUCCESS;
}
#pragma GCC diagnostic pop

/* Bytewise lexicographic comparator (per normative spec):
 * - Compares ordering keys without involving Pointer/XID/DRM-minor/
 *   Screen-/Registration-order fields.
 * - Returns negative if a < b, zero if a == b, positive if a > b.
 * - Uses strcmp for safe comparison of null-terminated strings.
 */
int xaverius_compare_ordering_key(const char *a, const char *b)
{
    return strcmp(a, b);
}

/* ---- Owner-key preference selection (spec: binding-selection and registry
 * result closure) ----
 *
 * The server may configure an ordered preference list over stable owner_key
 * values of XAVERIUS_MODULE_ROLE_DDX_BACKEND roles. Selection rules:
 *   - The first eligible binding whose owner_key matches a preference entry
 *     wins (lowest index = highest preference).
 *   - If no preference entry matches any eligible binding's owner_key:
 *     * A sole eligible binding is selected.
 *     * Multiple eligible unlisted bindings remain unpublished as AMBIGUOUS.
 * Ineligible bindings (failed snapshot, invalid descriptor, etc.) never block
 * a later eligible preference.
 *
 * The preference list is NULL-terminated. It is NULL (unset) by default, in
 * which case all owner_keys have equal rank.
 */

/* Server-configured ordered preference list of owner_key values (NULL-term).
 * Set via xaverius_binding_preference_set(). */
static const char * const *xaverius_binding_preference_owner_keys = NULL;

/* Set the ordered owner_key preference list. Caller retains ownership of the
 * array and its strings; they must outlive the registry. Passing NULL clears
 * the preference list (all owner_keys equal rank). */
void
xaverius_binding_preference_set(const char * const *preference_keys)
{
    xaverius_binding_preference_owner_keys = preference_keys;
}

/* Return the preference rank of owner_key: index into the preference list
 * (0 = highest preference), or a large value if not listed. */
static int
xaverius_owner_key_rank(const char *owner_key)
{
    int i;

    if (owner_key == NULL || xaverius_binding_preference_owner_keys == NULL)
        return 1000000;

    for (i = 0; xaverius_binding_preference_owner_keys[i] != NULL; i++) {
        if (strcmp(owner_key, xaverius_binding_preference_owner_keys[i]) == 0)
            return i;
    }
    return 1000000;
}

static XaveriusPhysicalDevice *xaverius_find_device_by_identity(
    const XaveriusDeviceIdentity *identity)
{
    XaveriusPhysicalDevice *dev;

    xorg_list_for_each_entry(dev, &xaverius_physical_devices, list) {
        if (xaverius_identity_equal(&dev->identity, identity))
            return dev;
    }

    return NULL;
}

/* ---- Registry registration ---- */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
XaveriusRegistryResult XaveriusRegistryRegisterDdxDevice(
    void *registry,
    const XaveriusDeviceBackendRegistration *registration,
    XaveriusPhysicalDevice **out_device,
    uint64_t *out_device_generation,
    uint64_t *out_binding_generation)
{
    XaveriusPhysicalDevice *dev = NULL;
    XaveriusPhysicalDevice *existing;
    struct _XaveriusModuleRole *role;
    struct _XaveriusDeviceBackendBinding *binding;
    XaveriusBackendResult snap_result;

    if (registration == NULL || out_device == NULL || registration->ops == NULL)
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    /* Validate descriptor header before reading later fields.
     * Normative: any v1 size mismatch (undersized OR oversized) is
     * INVALID_ARGUMENT; unknown ABI version is UNSUPPORTED. */
    if (registration->struct_size != sizeof(*registration))
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;
    if (registration->abi_version != XAVERIUS_DEVICE_BACKEND_REGISTRATION_ABI_VERSION)
        return XAVERIUS_REGISTRY_UNSUPPORTED;

    /* Validate the top-level ops descriptor header before reading callback
     * fields and before any Hook/Lease side effect. Invalid descriptors
     * must cause neither callback invocation nor lease retention. */
    if (registration->ops->struct_size != sizeof(*registration->ops))
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;
    if (registration->ops->abi_version != XAVERIUS_DEVICE_BACKEND_OPS_ABI_VERSION)
        return XAVERIUS_REGISTRY_UNSUPPORTED;

    /* query_physical_snapshot is required for registration. Validate before
     * allocating/retaining binding-side resources. */
    if (registration->ops->query_physical_snapshot == NULL)
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    /* Resolve the exact ModuleRole generation. No retain happens on lookup;
     * binding allocation retains the role for binding lifetime. */
    role = xaverius_find_module_role_by_generation(
        registration->owner_role_generation);
    if (role == NULL)
        return XAVERIUS_REGISTRY_NOT_REGISTERED;

    /* owner_key belongs to ModuleRole, resolved through role. Validate
     * that role has a non-empty owner_key. */
    if (role->owner_key == NULL || role->owner_key[0] == '\0')
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    /* Per spec §1041: validate role state before installing binding.
     * Order: most specific first (QUARANTINED, STALE_GENERATION),
     * then admission-based (SHUTTING_DOWN, UNAVAILABLE). */
    if (role->quiesce_state == XAVERIUS_MODULE_QUIESCE_FAILURE)
        return XAVERIUS_REGISTRY_QUARANTINED;

    if (role->quiesce_state == XAVERIUS_MODULE_QUIESCE_SUCCESS)
        return XAVERIUS_REGISTRY_STALE_GENERATION;

    if (role->admission == XAVERIUS_MODULE_ROLE_ADMISSION_CLOSED)
        return XAVERIUS_REGISTRY_SHUTTING_DOWN;

    if (role->quiesce_state == XAVERIUS_MODULE_QUIESCE_PENDING)
        return XAVERIUS_REGISTRY_UNAVAILABLE;

    /* Check callback_admission_open flag (checked by AcquireExecRef).
     * This must be checked before allocating binding to avoid provisional
     * binding/role reference leaks when admission is closed. */
    if (!role->callback_admission_open)
        return XAVERIUS_REGISTRY_UNAVAILABLE;

    if (!XaveriusModuleRoleAdmissionOpen(role))
        return XAVERIUS_REGISTRY_UNAVAILABLE;

    /* Canonicalize identity: check for an existing exact incarnation.
     * registration->struct_size/abi_version validate the registration
     * descriptor header; registration->identity is a typed
     * XaveriusDeviceIdentity pointer (no separate length parameter). */
    existing = NULL;
    if (registration->identity != NULL) {
        existing = xaverius_find_device_by_identity(
            (const XaveriusDeviceIdentity *)registration->identity);
    }

    int is_replacement = 0;

    if (existing != NULL) {
        /* Exact replay of the same live registration/binding ownership is
         * idempotent: return an additional caller-owned reference.
         * If binding was removed (NULL), allow re-registration with a new
         * binding on the SAME device. Same hardware identity but different
         * binding ownership is resolved by owner-key preference selection:
         * the eligible binding with the highest-configured preference wins;
         * if no preference is configured, multiple eligible bindings remain
         * unpublished as AMBIGUOUS; ineligible bindings never block a later
         * eligible preference.
         *
         * Exact replay compares the existing binding's owning role/generation.
         * No new binding is created and no extra role retain is taken. */
        if (existing->binding != NULL) {
            if (existing->binding->module_role == role &&
                existing->binding->module_role->role_generation ==
                    registration->owner_role_generation) {
                existing->caller_refcount++;
                *out_device = existing;
                if (out_device_generation != NULL)
                    *out_device_generation = existing->device_generation;
                if (out_binding_generation != NULL)
                    *out_binding_generation = existing->binding->binding_generation;
                return XAVERIUS_REGISTRY_SUCCESS;
            }
            /* Conflict: same identity, different role. Apply owner-key
             * preference selection (W0-153 item 4). */
            {
                int existing_rank = xaverius_owner_key_rank(
                    existing->binding->module_role->owner_key);
                int new_rank = xaverius_owner_key_rank(role->owner_key);

                if (existing_rank <= new_rank) {
                    /* Existing binding wins (higher preference or equal
                     * rank → ambiguous). New binding not selected. */
                    return XAVERIUS_REGISTRY_AMBIGUOUS;
                }
                /* New binding wins: defer replacement until after the
                 * provisional binding is fully validated. Never replace a
                 * published binding (dev->xid != 0). */
                if (existing->xid != 0)
                    return XAVERIUS_REGISTRY_AMBIGUOUS;
                is_replacement = 1;
            }
        }
        /* binding == NULL (or was just freed): re-register on existing device. */
        dev = existing;
    }

    /* Allocate the direct DDX device backend binding. The binding retains
     * the ModuleRole for its lifetime. Built provisionally; installed only
     * after snapshot validation succeeds. */
    binding = xaverius_binding_alloc(registration, role);
    if (binding == NULL)
        return XAVERIUS_REGISTRY_OUT_OF_MEMORY;

    if (existing == NULL) {
        /* New device: allocate and initialize. dev is explicitly assigned
         * here so every compiler path provably initializes it before use. */
        dev = calloc(1, sizeof(*dev));
        if (dev == NULL) {
            xaverius_binding_free(binding);
            return XAVERIUS_REGISTRY_OUT_OF_MEMORY;
        }
        dev->device_generation = xaverius_next_generation_id();
        dev->xid = 0;

        /* Copy and validate identity evidence, then create ordering key from the
         * canonical copied identity (never from an uninitialized device identity). */
        if (registration->identity != NULL) {
            dev->identity_evidence = calloc(1, sizeof(XaveriusDeviceIdentity));
            if (dev->identity_evidence == NULL) {
                xaverius_binding_free(binding);
                free(dev);
                return XAVERIUS_REGISTRY_OUT_OF_MEMORY;
            }
            memcpy(dev->identity_evidence, registration->identity,
                   sizeof(XaveriusDeviceIdentity));
            dev->identity_evidence_size = sizeof(XaveriusDeviceIdentity);
            dev->identity = *(const XaveriusDeviceIdentity *)registration->identity;
        } else {
            xaverius_binding_free(binding);
            free(dev);
            return XAVERIUS_REGISTRY_INVALID_ARGUMENT;
        }

        {
            XaveriusRegistryResult key_result = xaverius_create_ordering_key(
                dev->ordering_key,
                sizeof(dev->ordering_key),
                &dev->identity,
                role->owner_key);
            if (key_result != XAVERIUS_REGISTRY_SUCCESS) {
                xaverius_binding_free(binding);
                free(dev->identity_evidence);
                free(dev);
                return key_result;
            }
        }

        /* Initialize caller reference count: registration returns a caller-owned
         * reference that must be released via XaveriusPhysicalDeviceRelease().
         * The registry holds its own permanent reference via list membership. */
        dev->caller_refcount = 1;
    }
    /* else: re-registration on existing device; dev == existing and is already
     * fully initialized. Do NOT mutate caller_refcount/device_generation/binding
     * until after snapshot validation succeeds (failure-atomic installation). */

    /* Query the physical snapshot through the exact provisional binding's
     * query_physical_snapshot callback. Write into a temporary snapshot so
     * the callback-owned memory is never freed before being deep-copied
     * into server-owned storage (avoids use-after-free).
     * Protect with execution-ref protocol: acquire before callback, release
     * after, regardless of success/failure.
     * If execution ref cannot be acquired (admission closed/quarantined),
     * reject the registration without invoking the callback. */
    XaveriusPhysicalDeviceSnapshot snap = { 0 };
    XaveriusModuleRole *role_for_exec = binding->module_role;
    bool exec_acquired = false;
    if (role_for_exec != NULL) {
        exec_acquired = XaveriusModuleRoleAcquireExecRef(role_for_exec);
    }
    if (!exec_acquired) {
        xaverius_binding_free(binding);
        if (existing == NULL) {
            free(dev->identity_evidence);
            free(dev);
        }
        return XAVERIUS_REGISTRY_UNAVAILABLE;
    }
    snap_result = registration->ops->query_physical_snapshot(binding, &snap);
    XaveriusModuleRoleReleaseExecRef(role_for_exec);
    if (snap_result != XAVERIUS_BACKEND_SUCCESS) {
        free(snap.data);
        xaverius_binding_free(binding);
        if (existing == NULL) {
            free(dev->identity_evidence);
            free(dev);
        }
        /* Re-registration path: dev not mutated, binding NULL, clean state. */
        return XAVERIUS_REGISTRY_UNAVAILABLE;
    }

    /* Validate snapshot fields per W0-022 D:
     * - Pointer/count pairs must be consistent
     * - Overflow/Maximalgrenzen prüfen
     * - UUID-Validierung
     * - Queue/Memory-Konsistenz prüfen
     * - timelineSemaphore = 1 ist verpflichtend */
    if (snap.data == NULL || snap.size == 0) {
        free(snap.data);
        xaverius_binding_free(binding);
        if (existing == NULL) {
            free(dev->identity_evidence);
            free(dev);
        }
        return XAVERIUS_REGISTRY_INVALID_EVIDENCE;
    }

    /* Deep copy snapshot data into server-owned immutable storage.
     * Keep snapshot_copy local until after if/else to ensure atomic swap
     * and proper old-snapshot cleanup in re-registration path. */
    void *snapshot_copy = NULL;
    size_t snapshot_copy_size = 0;
    {
        snapshot_copy = calloc(1, snap.size);
        if (snapshot_copy == NULL) {
            free(snap.data);
            xaverius_binding_free(binding);
            if (existing == NULL) {
                free(dev->identity_evidence);
                free(dev);
            }
            return XAVERIUS_REGISTRY_OUT_OF_MEMORY;
        }
        memcpy(snapshot_copy, snap.data, snap.size);
        free(snap.data);
        snapshot_copy_size = snap.size;
    }

    /* The device's binding pointer will be set after the new/existing
     * device logic, once we know which binding to install. */
    if (existing == NULL) {
        /* Install new device into internal (not yet published) registry. */
        xorg_list_append(&dev->list, &xaverius_physical_devices);
        xaverius_physical_device_count++;
        dev->binding = binding;
        dev->snapshot.data = snapshot_copy;
        dev->snapshot.size = snapshot_copy_size;
    } else {
        /* Re-registration: mutate existing device atomically now that all
         * validation has succeeded. No partial state is visible on failure.
         * For a preference-driven binding replacement (is_replacement), free
         * the old binding (kept intact during validation), set a fresh caller
         * reference instead of incrementing, since this is a new selection
         * through a different role, not a replay.
         * Free the OLD snapshot data before installing the new one. */
        if (is_replacement) {
            xaverius_binding_free(dev->binding);
            dev->binding = NULL;
            dev->caller_refcount = 1;
        } else {
            dev->caller_refcount++;
        }
        /* Free old snapshot data before installing new snapshot. */
        if (dev->snapshot.data != NULL) {
            free(dev->snapshot.data);
        }
        dev->snapshot.data = snapshot_copy;
        dev->snapshot.size = snapshot_copy_size;
        dev->device_generation = xaverius_next_generation_id();
        binding->binding_generation = xaverius_next_generation_id(); /* independent generation */
        dev->binding = binding;
    }

    *out_device = dev;
    if (out_device_generation != NULL)
        *out_device_generation = dev->device_generation;
    if (out_binding_generation != NULL)
        *out_binding_generation = binding->binding_generation;

    return XAVERIUS_REGISTRY_SUCCESS;
}
#pragma GCC diagnostic pop

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
     * Decrement the caller reference count. The registry holds its own
     * permanent reference via list membership. */
    if (device->caller_refcount > 0) {
        device->caller_refcount--;
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
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

    if (device->device_generation != device_generation)
        return XAVERIUS_REGISTRY_STALE_GENERATION;

    if (device->binding->module_role->role_generation != module_role_generation ||
        device->binding->binding_generation != binding_generation)
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    /* Mark binding unavailable. The binding record itself remains owned by
     * the PhysicalDevice; only its availability changes. */
    device->binding->availability = XAVERIUS_REGISTRY_UNAVAILABLE;
    device->status = XAVERIUS_REGISTRY_UNAVAILABLE;

    /* Release the binding's retained ModuleRole reference and free all
     * server-owned binding copies via the central cleanup path. */
    xaverius_binding_free(device->binding);
    device->binding = NULL;

    return XAVERIUS_REGISTRY_SUCCESS;
}

/* ---- Observation / removal ---- */

/* Process-wide discovery source and observation registries */

/* Find observation matching the removal evidence (source + sequence + ID). */
static XaveriusObservation *
xaverius_find_observation_by_removal_evidence(
    const XaveriusDeviceRemovalEvidence *evidence)
{
    XaveriusObservation *obs;

    if (evidence == NULL)
        return NULL;

    xorg_list_for_each_entry(obs, &xaverius_observations, list) {
        if (obs->device == NULL)
            continue;

        if (strcmp(obs->source_key, evidence->source_key) == 0 &&
            obs->source_generation == evidence->source_generation &&
            obs->observation_id == evidence->observation_id &&
            obs->source_event_sequence == evidence->source_event_sequence)
            return obs;
    }

    return NULL;
}

XaveriusDiscoverySource *xaverius_find_discovery_source(
    const char *source_key)
{
    XaveriusDiscoverySource *src;

    xorg_list_for_each_entry(src, &xaverius_discovery_sources, list) {
        if (strcmp(src->source_key, source_key) == 0)
            return src;
    }

    return NULL;
}

/* Find the latest observation for a given source + event sequence, used for
 * idempotent replay detection. */
static XaveriusObservation *
xaverius_find_observation_by_sequence(
    const char *source_key,
    uint64_t source_generation,
    uint64_t source_event_sequence)
{
    XaveriusObservation *obs;

    xorg_list_for_each_entry(obs, &xaverius_observations, list) {
        if (strcmp(obs->source_key, source_key) == 0 &&
            obs->source_generation == source_generation &&
            obs->source_event_sequence == source_event_sequence)
            return obs;
    }

    return NULL;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
XaveriusRegistryResult XaveriusRegistryObserveDevice(
    void *registry,
    const char *source_key,
    uint64_t source_generation,
    uint64_t source_event_sequence,
    const XaveriusDeviceIdentity *identity_evidence,
    XaveriusPhysicalDevice **out_device,
    uint64_t *out_device_generation,
    uint64_t *out_observation_id)
{
    XaveriusPhysicalDevice *dev;
    XaveriusObservation *obs;
    XaveriusDiscoverySource *src;
    XaveriusRegistryResult result = XAVERIUS_REGISTRY_INTERNAL_ERROR;
    uint64_t new_obs_id;

    if (source_key == NULL || out_device == NULL || out_observation_id == NULL)
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    /* source_event_sequence must be nonzero per spec §1037. */
    if (source_event_sequence == 0)
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    /* Resolve or register the discovery source. */
    src = xaverius_find_discovery_source(source_key);
    if (src == NULL) {
        src = calloc(1, sizeof(*src));
        if (src == NULL)
            return XAVERIUS_REGISTRY_OUT_OF_MEMORY;
        src->source_key = strdup(source_key);
        if (src->source_key == NULL) {
            free(src);
            return XAVERIUS_REGISTRY_OUT_OF_MEMORY;
        }
        src->source_generation = source_generation;
        src->last_event_sequence = 0;
        xorg_list_init(&src->list);
        xorg_list_append(&src->list, &xaverius_discovery_sources);
        xaverius_discovery_source_count++;
    } else {
        /* Validate source generation: only the exact active generation is
         * accepted. Stale or future generations are rejected. */
        if (source_generation != src->source_generation)
            return XAVERIUS_REGISTRY_STALE_EVENT;

        /* Exhaustion check: if the source has reached UINT64_MAX, no further
         * observations are accepted (including replays) per spec §1037:
         * "On exhaustion the source generation is retired". */
        if (src->last_event_sequence == UINT64_MAX)
            return XAVERIUS_REGISTRY_STALE_EVENT; /* exhausted */

        /* Idempotent replay detection: same source + same event sequence.
         * This MUST come before sequence validation to allow valid replays
         * of the same sequence number. */
        if (src->last_event_sequence == source_event_sequence) {
            XaveriusObservation *prev_obs =
                xaverius_find_observation_by_sequence(source_key, source_generation,
                                                      source_event_sequence);
            if (prev_obs != NULL) {
                /* Same payload → idempotent: return existing observation ID. */
                if (identity_evidence != NULL &&
                    prev_obs->device != NULL &&
                    memcmp(&prev_obs->identity, identity_evidence,
                           sizeof(XaveriusDeviceIdentity)) == 0) {
                    *out_observation_id = prev_obs->observation_id;
                    *out_device = prev_obs->device;
                    if (out_device_generation != NULL)
                        *out_device_generation = prev_obs->device_generation;
                    /* Additional caller-owned reference for repeated success. */
                    prev_obs->device->caller_refcount++;
                    return XAVERIUS_REGISTRY_SUCCESS;
                }
                /* Different payload for same sequence → conflict. */
                return XAVERIUS_REGISTRY_IDENTITY_CONFLICT;
            }
        }

        /* Validate source event sequence: must be strictly increasing
         * within a source generation per spec §1037. Sequences start at 1,
         * MUST NOT wrap or be reused. */
        if (source_event_sequence < src->last_event_sequence + 1)
            return XAVERIUS_REGISTRY_STALE_EVENT;
    }

    /* Canonicalize identity: find existing exact incarnation. */
    dev = NULL;
    if (identity_evidence != NULL) {
        dev = xaverius_find_device_by_identity(identity_evidence);
    }

    /* Allocate a unique nonzero observation ID. */
    new_obs_id = xaverius_next_observation_id_fn();

    obs = calloc(1, sizeof(*obs));
    if (obs == NULL)
        return XAVERIUS_REGISTRY_OUT_OF_MEMORY;

    obs->source_key = strdup(source_key);
    if (obs->source_key == NULL) {
        free(obs);
        return XAVERIUS_REGISTRY_OUT_OF_MEMORY;
    }
    obs->source_generation = source_generation;
    obs->source_event_sequence = source_event_sequence;
    obs->observation_id = new_obs_id;
    if (identity_evidence != NULL)
        obs->identity = *identity_evidence;
    else
        memset(&obs->identity, 0, sizeof(obs->identity));
    obs->device = dev;
    obs->device_generation = dev ? dev->device_generation : 0;
    xorg_list_init(&obs->list);
    xorg_list_append(&obs->list, &xaverius_observations);
    xaverius_observation_count++;

    /* Update the source's last used event sequence. */
    src->last_event_sequence = source_event_sequence;

    *out_observation_id = new_obs_id;

    if (dev != NULL) {
        /* Unambiguous match: return canonical record reference. */
        *out_device = dev;
        if (out_device_generation != NULL)
            *out_device_generation = dev->device_generation;
        /* Additional caller-owned reference on success. */
        dev->caller_refcount++;
        /* Track active observations per spec §1038: each source's
         * evidence contributes to availability. */
        dev->active_observation_count++;
        result = XAVERIUS_REGISTRY_SUCCESS;
    } else {
        /* Ambiguous or no match: observation retained but device unpublished.
         * Observation retained for provenance; no XID allocated. */
        *out_device = NULL;
        if (out_device_generation != NULL)
            *out_device_generation = 0;
        result = XAVERIUS_REGISTRY_PENDING;
    }

    return result;
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
XaveriusRegistryResult XaveriusRegistryRemoveObservation(
    void *registry,
    const char *source_key,
    uint64_t source_generation,
    uint64_t observation_id)
{
    XaveriusObservation *obs = NULL, *next_obs = NULL;
    XaveriusRegistryResult result = XAVERIUS_REGISTRY_NOT_FOUND;

    if (source_key == NULL)
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    xorg_list_for_each_entry_safe(obs, next_obs, &xaverius_observations, list) {
        if (strcmp(obs->source_key, source_key) == 0 &&
            obs->source_generation == source_generation &&
            obs->observation_id == observation_id) {
            /* Decrement active observation count on referenced device. */
            if (obs->device != NULL && obs->device->active_observation_count > 0)
                obs->device->active_observation_count--;
            xorg_list_del(&obs->list);
            xaverius_observation_count--;
            free((void *)obs->source_key);
            free(obs);
            result = XAVERIUS_REGISTRY_SUCCESS;
            break;
        }
    }

    return result;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
XaveriusRegistryResult XaveriusRegistryConfirmDeviceRemoved(
    void *registry,
    XaveriusPhysicalDevice *device,
    uint64_t device_generation,
    const XaveriusDeviceRemovalEvidence *removal_evidence)
{
    XaveriusObservation *obs = NULL;

    if (device == NULL)
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    /* Removal evidence must be present and well-formed. */
    if (removal_evidence == NULL)
        return XAVERIUS_REGISTRY_INVALID_EVIDENCE;

    /* Generation must identify the exact current device incarnation. */
    if (device->device_generation != device_generation)
        return XAVERIUS_REGISTRY_STALE_GENERATION;

    /* Terminal REMOVED is committed exactly once per incarnation. */
    if (device->status == XAVERIUS_REGISTRY_REMOVED)
        return XAVERIUS_REGISTRY_STALE_EVENT;

    /* The evidence must reference a registered observation that references
     * this exact device; authority comes from the registered source, not
     * from any caller-supplied flag. */
    obs = xaverius_find_observation_by_removal_evidence(removal_evidence);
    if (obs == NULL ||
        obs->device != device ||
        obs->device_generation != device_generation)
        return XAVERIUS_REGISTRY_INVALID_EVIDENCE;

    /* Per spec §1038: device removal requires ALL active sources to have
     * withdrawn their evidence. Check if there are other active observations
     * for this device (besides the one referenced by removal_evidence).
     * If so, removal is deferred and returns BUSY. */
    {
        XaveriusObservation *other_obs;
        xorg_list_for_each_entry(other_obs, &xaverius_observations, list) {
            if (other_obs->device == device &&
                other_obs->observation_id != removal_evidence->observation_id) {
                return XAVERIUS_REGISTRY_BUSY;
            }
        }
    }

    /* Move the record from the active device list to the tombstone list
     * exactly once. The record (identity, generation, XID if published)
     * remains owned by the registry until ordered server shutdown. */
    xorg_list_del(&device->list);
    if (xaverius_physical_device_count > 0)
        xaverius_physical_device_count--;
    xorg_list_append(&device->list, &xaverius_tombstones);
    xaverius_tombstone_count++;
    device->status = XAVERIUS_REGISTRY_REMOVED;

    return XAVERIUS_REGISTRY_SUCCESS;
}

/* XaveriusPhysicalDevicePublish — separate server-internal publication operation
 * Allocates a server-private XID, prepares the DIX wrapper, and
 * atomically exposes it once AddResource() succeeds. */
XaveriusRegistryResult XaveriusPhysicalDevicePublish(XaveriusPhysicalDevice *dev,
                                                     uint64_t device_generation,
                                                     XID *out_xid)
{
    XID xid;

    if (dev == NULL || XaveriusPhysicalDeviceType == 0)
        return XAVERIUS_REGISTRY_INVALID_ARGUMENT;

    /* Publication is separate from registration and must happen at most once.
     * A published (or tombstoned) device retains its XID for the server lifetime. */
    if (dev->published)
        return XAVERIUS_REGISTRY_ALREADY_EXISTS;

    /* Validate device_generation matches the current incarnation. */
    if (dev->device_generation != device_generation)
        return XAVERIUS_REGISTRY_STALE_GENERATION;

    /* Allocate/reserve the XID using the established Xorg resource/XID rules.
     * Publication is separate from registration: no XID is allocated during
     * registration itself. */
    xid = dixAllocServerXID();
    if (xid == 0)
        return XAVERIUS_REGISTRY_OUT_OF_MEMORY;

    /* Allocate DIX wrapper — separate server-private object referenced by the
     * DIX resource DB. The registry retains ownership of the PhysicalDevice
     * record itself; the wrapper only holds a back-reference.
     * Initialize wrapper refcount to 1 for DIX-side ownership. */
    XaveriusPhysicalDeviceDIXWrapper *wrapper =
        calloc(1, sizeof(*wrapper));
    if (wrapper == NULL) {
        return XAVERIUS_REGISTRY_OUT_OF_MEMORY;
    }
    wrapper->dev = dev;
    wrapper->refcount = 1;

    /* Register the resource exactly once with the PhysicalDevice resource
     * type and its canonical destructor. Only persist the XID after
     * successful resource registration so registry and DIX stay consistent. */
    if (!AddResource(xid, XaveriusPhysicalDeviceType, wrapper)) {
        free(wrapper);
        return XAVERIUS_REGISTRY_INTERNAL_ERROR;
    }

    /* Acquire PhysicalDevice DIX-resource ownership reference.
     * This is a separate ref from caller_refcount and wrapper refcount.
     * Acquired exactly once after successful AddResource/publication.
     * Released exactly once in DIXDestroy when wrapper refcount reaches 0. */
    dev->xid = xid;
    dev->dix_wrapper = wrapper;
    dev->dix_resource_refcount = 1;
    dev->published = true;

    if (out_xid != NULL)
        *out_xid = xid;

    return XAVERIUS_REGISTRY_SUCCESS;
}
