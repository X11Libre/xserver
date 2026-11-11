/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * Xaverius internal definitions shared between source files within the extension.
 */
#ifndef _XLIBRE_XAVERIUS_INTERN_H
#define _XLIBRE_XAVERIUS_INTERN_H

#include <stdbool.h>
#include <stdint.h>

#include <X11/Xdefs.h>
#include <X11/Xfuncproto.h>

#include "include/list.h"
#include "include/privates.h"
#include "include/xlibre_ptrtypes.h"

#include "xaverius_proto.h"

/* Forward declarations for internal types */
struct _XaveriusPhysicalDevice;
typedef struct _XaveriusPhysicalDevice XaveriusPhysicalDevice;
/* XaveriusPhysicalDevice is fully defined in xaverius_priv.h (DDX-facing).
 * Only forward declaration here to avoid duplicate typedef. */

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

/* ModuleRole quiescence result */
typedef enum {
    XAVERIUS_MODULE_QUIESCE_PENDING  = 0,
    XAVERIUS_MODULE_QUIESCE_SUCCESS  = 1,
    XAVERIUS_MODULE_QUIESCE_FAILURE  = 2,
} XaveriusModuleQuiescenceResult;

/* Forward declarations */
struct _XaveriusModuleRole;
typedef struct _XaveriusModuleRole XaveriusModuleRole;

struct _XaveriusModuleLease;
typedef struct _XaveriusModuleLease XaveriusModuleLease;

/* ---- DDX module-role descriptor (v1, size/versioned) ---- */
typedef struct _XaveriusModuleRoleDescriptor {
    uint32_t struct_size;
    uint32_t abi_version;
    const char *owner_key;          /* stable role key across reloads */
    XaveriusModuleLease *module_lease;  /* server-owned module lease */
    void (*retain_module_role)(XaveriusModuleRole *role);
    void (*release_module_role)(XaveriusModuleRole *role);
    XaveriusModuleQuiescenceResult (*quiesce_module_role)(XaveriusModuleRole *role);
} XaveriusModuleRoleDescriptor;

/* ---- Canonical PhysicalDevice identity (shared with DDX-facing header) ----
 * Matches spec/device-registry-architecture.md §2.2 identity evidence classes.
 * This is defined here (not in xaverius_priv.h) so internal code and tests
 * can use it without requiring Xorg headers. */
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

/* ---- DDX device backend registration descriptor (v1, size/versioned) ---- */
typedef struct _XaveriusDeviceBackendRegistration {
    uint32_t struct_size;
    uint32_t abi_version;
    const XaveriusDeviceIdentity *identity;
    const struct XaveriusDeviceBackendOps *ops;
    void *device_context;
    void *native_physical_device;
    XaveriusModuleRole *owner_role;
    uint64_t owner_role_generation;
} XaveriusDeviceBackendRegistration;

#include "xaverius_proto.h"

/* Forward declarations for internal types */
struct _XaveriusPhysicalDevice;
/* XaveriusPhysicalDevice is fully defined in xaverius_priv.h (DDX-facing).
 * Only forward declaration here to avoid duplicate typedef. */

extern int XaveriusEventBase;

/* Global property atoms (initialized in init.c) */
extern Atom xaverius_atom_device_uuid;
extern Atom xaverius_atom_driver_uuid;
extern Atom xaverius_atom_max_work_group_invocations;
extern Atom xaverius_atom_max_work_group_size;
extern Atom xaverius_atom_timeline_semaphore;
extern DevPrivateKeyRec XaveriusScreenPrivateKeyRec;

/*
 * Device type values (xserver-internal, no separate xaverius_device.h header).
 */
#define XAVERIUS_DEVICE_TYPE_PHYSICAL    1
#define XAVERIUS_DEVICE_TYPE_LOGICAL     2

/* ---- DDX backend ABI version (v0.1) ---- */
#define XAVERIUS_DEVICE_BACKEND_OPS_ABI_VERSION 1

/* ---- DDX device backend registration ABI version (v1) ---- */
#define XAVERIUS_DEVICE_BACKEND_REGISTRATION_ABI_VERSION 1

/* ---- DDX module-role descriptor ABI version (v1) ---- */
#define XAVERIUS_MODULE_ROLE_DESCRIPTOR_ABI_VERSION 1

/* ---- DDX backend operation result ---- */
typedef enum {
    XAVERIUS_BACKEND_SUCCESS             = 0,
    XAVERIUS_BACKEND_UNSUPPORTED         = 1,
    XAVERIUS_BACKEND_STATUS              = 2,
    XAVERIUS_BACKEND_UNAVAILABLE         = 3,
    XAVERIUS_BACKEND_DEVICE_LOST         = 4,
    XAVERIUS_BACKEND_OUT_OF_HOST_MEMORY  = 5,
    XAVERIUS_BACKEND_OUT_OF_DEVICE_MEMORY = 6,
    XAVERIUS_BACKEND_TRANSFER_FAILURE    = 7,
    XAVERIUS_BACKEND_EXECUTION_FAILURE   = 8,
    XAVERIUS_BACKEND_CANCELLED           = 9,
    XAVERIUS_BACKEND_FATAL               = 10,
} XaveriusBackendResult;

/* ---- Typed DDX backend operation tables (provider-operation-matrix.md §3) ---- */
struct XaveriusDeviceBackendBufferMemoryOps {
    XaveriusBackendResult (*create_buffer)(void *, void *, void **);
    XaveriusBackendResult (*free_buffer)(void *, void *);
    XaveriusBackendResult (*get_buffer_memory_requirements)(void *, void *, void *);
    XaveriusBackendResult (*allocate_memory)(void *, void *, void **);
    XaveriusBackendResult (*free_memory)(void *, void *);
    XaveriusBackendResult (*bind_buffer_memory)(void *, void *, void *, uint64_t);
    XaveriusBackendResult (*write_buffer)(void *, void *, void *, const void *, void *);
    XaveriusBackendResult (*read_buffer)(void *, void *, void *, const void *, void *);
};

struct XaveriusDeviceBackendShaderPipelineOps {
    XaveriusBackendResult (*create_shader_module)(void *, void *, void **);
    XaveriusBackendResult (*create_descriptor_set_layout)(void *, void *, void **);
    XaveriusBackendResult (*create_descriptor_pool)(void *, void *, void **);
    XaveriusBackendResult (*allocate_descriptor_sets)(void *, void *, const void *, uint32_t, void **);
    XaveriusBackendResult (*update_descriptor_sets)(void *, const void *, uint32_t);
    XaveriusBackendResult (*free_descriptor_sets)(void *, void *, void * const *, uint32_t);
    XaveriusBackendResult (*create_pipeline_layout)(void *, void *, void **);
    XaveriusBackendResult (*create_compute_pipeline)(void *, void *, void **);
    XaveriusBackendResult (*destroy_shader_module)(void *, void *);
    XaveriusBackendResult (*destroy_descriptor_set_layout)(void *, void *);
    XaveriusBackendResult (*destroy_descriptor_pool)(void *, void *);
    XaveriusBackendResult (*destroy_pipeline_layout)(void *, void *);
    XaveriusBackendResult (*destroy_compute_pipeline)(void *, void *);
};

struct XaveriusDeviceBackendCommandOps {
    XaveriusBackendResult (*create_command_pool)(void *, void *, void **);
    XaveriusBackendResult (*destroy_command_pool)(void *, void *);
    XaveriusBackendResult (*allocate_command_buffer)(void *, void *, const void *, void **);
    XaveriusBackendResult (*free_command_buffer)(void *, void *, void *);
    XaveriusBackendResult (*reset_command_buffer)(void *, void *);
    XaveriusBackendResult (*begin_command_buffer)(void *, void *, const void *);
    XaveriusBackendResult (*record_commands)(void *, void *, const void *, uint32_t);
    XaveriusBackendResult (*end_command_buffer)(void *, void *);
};

struct XaveriusDeviceBackendSynchronizationOps {
    XaveriusBackendResult (*create_semaphore)(void *, void *, void **);
    XaveriusBackendResult (*host_signal_semaphore)(void *, void *, uint64_t, void *);
    XaveriusBackendResult (*destroy_semaphore)(void *, void *);
    XaveriusBackendResult (*create_fence)(void *, void *, void **);
    XaveriusBackendResult (*reset_fence)(void *, void *);
    XaveriusBackendResult (*destroy_fence)(void *, void *);
};

/* ---- Snapshot type (W0-138: ABI v1) ---- */
typedef struct {
    void *data;
    uint64_t size;
} XaveriusPhysicalDeviceSnapshot;

/* Forward declarations for XaveriusDeviceBackendOps */
struct _XaveriusDeviceBackendBinding;
typedef struct _XaveriusDeviceBackendBinding XaveriusDeviceBackendBinding;

/* ---- Top-level DDX backend operations descriptor (v0.1) ---- */
struct XaveriusDeviceBackendOps {
    uint64_t struct_size;
    uint64_t abi_version;

    /* Physical snapshot query */
    XaveriusBackendResult (*query_physical_snapshot)(XaveriusDeviceBackendBinding *, XaveriusPhysicalDeviceSnapshot *);

    /* Logical device */
    XaveriusBackendResult (*create_logical_device)(void *, void *, void **);
    XaveriusBackendResult (*get_queue)(void *, void *, void **);
    XaveriusBackendResult (*destroy_logical_device)(void *, void *);

    /* Queue submission */
    XaveriusBackendResult (*queue_submit)(void *, void *);

    /* Surface / presentation route / swapchain */
    XaveriusBackendResult (*create_surface)(void *, void *, void **);
    XaveriusBackendResult (*destroy_surface)(void *, void *);
    XaveriusBackendResult (*query_surface_support)(void *, void *, void *);
    XaveriusBackendResult (*create_presentation_route)(void *, void *, void **);
    XaveriusBackendResult (*destroy_presentation_route)(void *, void *);
    XaveriusBackendResult (*create_swapchain)(void *, void *, void **);
    XaveriusBackendResult (*destroy_swapchain)(void *, void *);
    XaveriusBackendResult (*acquire_next_image)(void *, void *, void **);
    XaveriusBackendResult (*queue_present)(void *, void *);
    XaveriusBackendResult (*cancel_acquire)(void *, void *);
    XaveriusBackendResult (*release_late_acquired_image)(void *, void *);

    /* Operation retirement */
    XaveriusBackendResult (*retire_operation)(void *, void *);

    /* Typed operation tables */
    struct XaveriusDeviceBackendBufferMemoryOps *buffer_memory;
    struct XaveriusDeviceBackendShaderPipelineOps *shader_pipeline;
    struct XaveriusDeviceBackendCommandOps *commands;
    struct XaveriusDeviceBackendSynchronizationOps *synchronization;
};

/* ---- Direct DDX device backend binding ---- */
struct _XaveriusDeviceBackendBinding {
    struct XaveriusDeviceBackendOps *ops;     /* server-owned deep-copied ops tables */
    void *native_handle;                         /* borrowed DDX-owned */
    void *context;                             /* borrowed DDX-owned */
    uint64_t binding_generation;
    uint32_t availability;
    struct _XaveriusModuleRole *module_role;   /* retained owning role */
    struct xorg_list list;
};

/* ---- PhysicalDevice internal representation ---- */
struct _XaveriusPhysicalDevice {
    /* Canonical identity evidence (structured, compared per §2.2). */
    XaveriusDeviceIdentity identity;

    /* Immutable, server-owned identity evidence copy */
    void *identity_evidence;
    uint64_t identity_evidence_size;

    /* Immutable, server-owned capability/metadata snapshot (member/value) */
    XaveriusPhysicalDeviceSnapshot snapshot;

    /* Selected executable DDX device binding (direct dispatch target). */
    struct _XaveriusDeviceBackendBinding *binding;

    /* Current discovery/availability status */
    uint32_t status;

    /* Exact device incarnation generation */
    uint64_t device_generation;

    /* Server-private DIX resource XID assigned on publication (0 if unpublished). */
    XID xid;

    char ordering_key[128];

    /* Caller-owned reference count (from XaveriusRegistryRegisterDdxDevice).
     * Each successful registration returns a caller-owned reference that must
     * be released via XaveriusPhysicalDeviceRelease(). The registry itself
     * holds one permanent reference for the lifetime of the record. */
    uint32_t caller_refcount;

    struct xorg_list list;
};

/* ---- ModuleRole control record (shared code/module lifetime & quiescence) ---- */
struct _XaveriusModuleRole {
    char *owner_key;
    XaveriusModuleLease *module_lease;            /* server-owned module lease */
    uint64_t role_generation;
    uint32_t admission;                /* open / quiescing / closed */
    uint32_t quiesce_state;            /* PENDING / SUCCESS / FAILURE */
    void (*retain_module_role)(XaveriusModuleRole *role);
    void (*release_module_role)(XaveriusModuleRole *role);
    XaveriusModuleQuiescenceResult (*quiesce_module_role)(XaveriusModuleRole *role);
    struct xorg_list list;
};

/* Global device registry (defined in xaverius_registry.c). Only one registry
 * exists per server process. */
extern struct xorg_list xaverius_physical_devices;
extern uint32_t xaverius_physical_device_count;

/* DIX resource type for published PhysicalDevices (server-private). */
extern RESTYPE XaveriusPhysicalDeviceType;

/* ---- Internal registry API ---- */
bool XaveriusRegistryInit(void);
void XaveriusRegistryFini(void);

/* ---- ModuleRole substrate (Stage 2) ---- */
/* ModuleRole lifecycle states. */
typedef enum {
    XAVERIUS_MODULE_ROLE_ADMISSION_OPEN   = 0,
    XAVERIUS_MODULE_ROLE_ADMISSION_CLOSED = 1,
} XaveriusModuleRoleAdmission;

/* Create / retain / release / quiesce / finalize a ModuleRole control record.
 * These are infallible, non-blocking lifetime primitives except XaveriusModuleRoleCreate
 * which may fail on allocation/validation before any retain. */
XaveriusRegistryResult XaveriusModuleRoleCreate(const XaveriusModuleRoleDescriptor *desc,
                                      XaveriusModuleRole **out_role);
void XaveriusModuleRoleRetain(XaveriusModuleRole *role);
void XaveriusModuleRoleRelease(XaveriusModuleRole *role);
XaveriusModuleQuiescenceResult XaveriusModuleRoleQuiesce(XaveriusModuleRole *role);
void XaveriusModuleRoleFinalize(XaveriusModuleRole *role);

/* Is admission still open for this role? */
bool XaveriusModuleRoleAdmissionOpen(XaveriusModuleRole *role);

/* ---- XaveriusPhysicalDevicePublish — separate server-internal publication operation.
 * Allocates a server-private XID, prepares the DIX wrapper, and atomically
 * exposes it once AddResource() succeeds. */
bool XaveriusPhysicalDevicePublish(XaveriusPhysicalDevice *dev);

/* DIX resource destructor — canonical path, runs exactly once per published record. */
int XaveriusPhysicalDeviceDIXDestroy(void *value, XID xid);

/* Lookup / iteration */
XaveriusPhysicalDevice *XaveriusFindDeviceByBindingGeneration(uint64_t binding_generation);
XaveriusPhysicalDevice *XaveriusFirstDevice(void);
XaveriusPhysicalDevice *XaveriusNextDevice(XaveriusPhysicalDevice *device);
uint32_t XaveriusDeviceCount(void);

/* Internal validation and comparison functions (exposed for testing) */
XaveriusRegistryResult xaverius_module_role_descriptor_validate(
    const XaveriusModuleRoleDescriptor *desc);
bool xaverius_identity_equal(const XaveriusDeviceIdentity *a, const XaveriusDeviceIdentity *b);
bool xaverius_identity_has_pci(const XaveriusDeviceIdentity *id);

/* Canonical ordering key functions (per normative spec device-registry-architecture.md §6) */
XaveriusRegistryResult xaverius_create_ordering_key(
    char *out_buf, size_t out_buf_size,
    const XaveriusDeviceIdentity *identity,
    const char *owner_key);
int xaverius_compare_ordering_key(const char *a, const char *b);

/* ---- Observation / removal (Stage 3) ---- */
/* Discovery source and observation bookkeeping (minimal, server-side only).
 * Generation-checked, non-reusable generations for source and observation. */
typedef struct _XaveriusDiscoverySource {
    void *source_key;
    uint64_t source_key_size;
    uint64_t source_generation;
    struct xorg_list list;
} XaveriusDiscoverySource;

typedef struct _XaveriusObservation {
    void *source_key;
    uint64_t source_key_size;
    uint64_t source_generation;
    uint64_t observation_id;
    XaveriusDeviceIdentity identity;
    XaveriusPhysicalDevice *device; /* canonical record reference */
    uint64_t device_generation; /* generation of the device at observation time */
    struct xorg_list list;
} XaveriusObservation;

/* Submit or update an identity observation from a discovery source.
 * Returns a generation-checked canonical record reference only when identity
 * matching is unambiguous; never allocates a protocol XID. */
XaveriusRegistryResult XaveriusRegistryObserveDevice(void *registry,
    const XaveriusDeviceIdentity *identity_evidence,
    uint64_t identity_evidence_size,
    const void *source_key,
    uint64_t source_key_size,
    uint64_t source_generation,
    uint64_t observation_id,
    XaveriusPhysicalDevice **out_device,
    uint64_t *out_device_generation);

/* Withdraw only this source's evidence. Must not mark hardware REMOVED
 * while other authoritative evidence still establishes the same incarnation. */
XaveriusRegistryResult XaveriusRegistryRemoveObservation(void *registry,
    const void *source_key,
    uint64_t source_generation,
    uint64_t observation_id);

/* Commit terminal REMOVED only on authoritative evidence that the physical
 * incarnation was removed. */
XaveriusRegistryResult XaveriusRegistryConfirmDeviceRemoved(void *registry,
    XaveriusPhysicalDevice *device,
    uint64_t device_generation,
    const void *removal_evidence,
    uint64_t removal_evidence_size);

/* Dispatch */
int ProcXaveriusDispatch(ClientPtr client);

/* ---- Legacy names kept only for transitional reference; do not reuse ---- */
/* XaveriusDevice, XaveriusDeviceCapabilities, XaveriusRegisterDevice,
 * XaveriusRegisterProvider are REMOVED — replaced by Xaverius* model above. */

#endif /* _XLIBRE_XAVERIUS_INTERN_H */
