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

struct _XaveriusPhysicalDeviceDIXWrapper;
typedef struct _XaveriusPhysicalDeviceDIXWrapper XaveriusPhysicalDeviceDIXWrapper;

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
    XAVERIUS_MODULE_QUIESCE_ACTIVE   = 3,  /* role is active, accepting registrations */
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
/* ---- Snapshot type (W0-138: ABI v1) ---- */
typedef struct {
    void *data;
    uint64_t size;
} XaveriusPhysicalDeviceSnapshot;

/* ---- Opaque DDX backend object handles (ABI v1) ---- */
/* DDX-owned object instances; server refers to them only through these
 * opaque handles. Concrete types are defined by each DDX backend. */
typedef struct _XaveriusDeviceBackendDevice XaveriusDeviceBackendDevice;
typedef struct _XaveriusDeviceBackendQueue XaveriusDeviceBackendQueue;
typedef struct _XaveriusDeviceBackendSurface XaveriusDeviceBackendSurface;
typedef struct _XaveriusDeviceBackendSwapchain XaveriusDeviceBackendSwapchain;
typedef struct _XaveriusDeviceBackendRoute XaveriusDeviceBackendRoute;
typedef struct _XaveriusDeviceBackendSemaphore XaveriusDeviceBackendSemaphore;
typedef struct _XaveriusDeviceBackendFence XaveriusDeviceBackendFence;
typedef struct _XaveriusDeviceBackendCommandPool XaveriusDeviceBackendCommandPool;
typedef struct _XaveriusDeviceBackendCommandBuffer XaveriusDeviceBackendCommandBuffer;
typedef struct _XaveriusDeviceBackendShaderModule XaveriusDeviceBackendShaderModule;
typedef struct _XaveriusDeviceBackendDescriptorSetLayout XaveriusDeviceBackendDescriptorSetLayout;
typedef struct _XaveriusDeviceBackendDescriptorPool XaveriusDeviceBackendDescriptorPool;
typedef struct _XaveriusDeviceBackendPipelineLayout XaveriusDeviceBackendPipelineLayout;
typedef struct _XaveriusDeviceBackendComputePipeline XaveriusDeviceBackendComputePipeline;
typedef struct _XaveriusDeviceBackendBuffer XaveriusDeviceBackendBuffer;
typedef struct _XaveriusDeviceBackendDeviceMemory XaveriusDeviceBackendDeviceMemory;
typedef struct _XaveriusDeviceBackendDescriptorSet XaveriusDeviceBackendDescriptorSet;
typedef struct _XaveriusDeviceBackendOperation XaveriusDeviceBackendOperation;

/* Forward declarations for XaveriusDeviceBackendOps */
struct _XaveriusDeviceBackendBinding;
typedef struct _XaveriusDeviceBackendBinding XaveriusDeviceBackendBinding;

struct XaveriusDeviceBackendBufferMemoryOps {
    XaveriusBackendResult (*create_buffer)(XaveriusDeviceBackendDevice *, const void *, XaveriusDeviceBackendBuffer *);
    XaveriusBackendResult (*free_buffer)(XaveriusDeviceBackendBuffer *);
    XaveriusBackendResult (*get_buffer_memory_requirements)(XaveriusDeviceBackendBuffer *, void *, void *);
    XaveriusBackendResult (*allocate_memory)(XaveriusDeviceBackendDevice *, const void *, XaveriusDeviceBackendDeviceMemory *);
    XaveriusBackendResult (*free_memory)(XaveriusDeviceBackendDeviceMemory *);
    XaveriusBackendResult (*bind_buffer_memory)(XaveriusDeviceBackendBuffer *, XaveriusDeviceBackendDeviceMemory *, uint64_t);
    XaveriusBackendResult (*write_buffer)(XaveriusDeviceBackendBuffer *, const void *, void *, const void *, void *);
    XaveriusBackendResult (*read_buffer)(XaveriusDeviceBackendBuffer *, const void *, void *, const void *, void *);
};

struct XaveriusDeviceBackendShaderPipelineOps {
    XaveriusBackendResult (*create_shader_module)(XaveriusDeviceBackendDevice *, const void *, XaveriusDeviceBackendShaderModule *);
    XaveriusBackendResult (*create_descriptor_set_layout)(XaveriusDeviceBackendDevice *, const void *, XaveriusDeviceBackendDescriptorSetLayout *);
    XaveriusBackendResult (*create_descriptor_pool)(XaveriusDeviceBackendDevice *, const void *, XaveriusDeviceBackendDescriptorPool *);
    XaveriusBackendResult (*allocate_descriptor_sets)(XaveriusDeviceBackendDescriptorPool *, const void *, uint32_t, XaveriusDeviceBackendDescriptorSet *);
    XaveriusBackendResult (*update_descriptor_sets)(XaveriusDeviceBackendDevice *, const void *, uint32_t);
    XaveriusBackendResult (*free_descriptor_sets)(XaveriusDeviceBackendDescriptorPool *, void * const *, uint32_t);
    XaveriusBackendResult (*create_pipeline_layout)(XaveriusDeviceBackendDevice *, const void *, XaveriusDeviceBackendPipelineLayout *);
    XaveriusBackendResult (*create_compute_pipeline)(XaveriusDeviceBackendDevice *, const void *, XaveriusDeviceBackendComputePipeline *);
    XaveriusBackendResult (*destroy_shader_module)(XaveriusDeviceBackendShaderModule *);
    XaveriusBackendResult (*destroy_descriptor_set_layout)(XaveriusDeviceBackendDescriptorSetLayout *);
    XaveriusBackendResult (*destroy_descriptor_pool)(XaveriusDeviceBackendDescriptorPool *);
    XaveriusBackendResult (*destroy_pipeline_layout)(XaveriusDeviceBackendPipelineLayout *);
    XaveriusBackendResult (*destroy_compute_pipeline)(XaveriusDeviceBackendComputePipeline *);
};

struct XaveriusDeviceBackendCommandOps {
    XaveriusBackendResult (*create_command_pool)(XaveriusDeviceBackendDevice *, const void *, XaveriusDeviceBackendCommandPool *);
    XaveriusBackendResult (*destroy_command_pool)(XaveriusDeviceBackendCommandPool *);
    XaveriusBackendResult (*allocate_command_buffer)(XaveriusDeviceBackendCommandPool *, const void *, XaveriusDeviceBackendCommandBuffer *);
    XaveriusBackendResult (*free_command_buffer)(XaveriusDeviceBackendCommandBuffer *);
    XaveriusBackendResult (*reset_command_buffer)(XaveriusDeviceBackendCommandBuffer *);
    XaveriusBackendResult (*begin_command_buffer)(XaveriusDeviceBackendCommandBuffer *, const void *);
    XaveriusBackendResult (*record_commands)(XaveriusDeviceBackendCommandBuffer *, const void *, uint32_t);
    XaveriusBackendResult (*end_command_buffer)(XaveriusDeviceBackendCommandBuffer *);
};

struct XaveriusDeviceBackendSynchronizationOps {
    XaveriusBackendResult (*create_semaphore)(XaveriusDeviceBackendDevice *, const void *, XaveriusDeviceBackendSemaphore *);
    XaveriusBackendResult (*host_signal_semaphore)(XaveriusDeviceBackendSemaphore *, uint64_t, void *);
    XaveriusBackendResult (*destroy_semaphore)(XaveriusDeviceBackendSemaphore *);
    XaveriusBackendResult (*create_fence)(XaveriusDeviceBackendDevice *, const void *, XaveriusDeviceBackendFence *);
    XaveriusBackendResult (*reset_fence)(XaveriusDeviceBackendFence *);
    XaveriusBackendResult (*destroy_fence)(XaveriusDeviceBackendFence *);
};

/* ---- Direct DDX device backend binding ---- */
struct XaveriusDeviceBackendOps {
    uint32_t struct_size;
    uint32_t abi_version;

    /* Physical snapshot query (two-argument form, unchanged per W0-153). */
    XaveriusBackendResult (*query_physical_snapshot)(XaveriusDeviceBackendBinding *, XaveriusPhysicalDeviceSnapshot *);

    /* Logical device */
    XaveriusBackendResult (*create_logical_device)(XaveriusDeviceBackendDevice *, const void *, XaveriusDeviceBackendDevice *);
    XaveriusBackendResult (*get_queue)(XaveriusDeviceBackendDevice *, uint32_t, uint32_t, XaveriusDeviceBackendQueue *);
    XaveriusBackendResult (*destroy_logical_device)(XaveriusDeviceBackendDevice *);

    /* Queue submission */
    XaveriusBackendResult (*queue_submit)(XaveriusDeviceBackendQueue *, XaveriusDeviceBackendOperation *);

    /* Surface / presentation route / swapchain */
    XaveriusBackendResult (*create_surface)(XaveriusDeviceBackendSurface *, const void *, void **);
    XaveriusBackendResult (*destroy_surface)(XaveriusDeviceBackendSurface *);
    XaveriusBackendResult (*query_surface_support)(XaveriusDeviceBackendDevice *, XaveriusDeviceBackendSurface *, uint32_t, void *);
    XaveriusBackendResult (*create_presentation_route)(XaveriusDeviceBackendRoute *, XaveriusDeviceBackendDevice *, XaveriusDeviceBackendSurface *, uint64_t);
    XaveriusBackendResult (*destroy_presentation_route)(XaveriusDeviceBackendRoute *);
    XaveriusBackendResult (*create_swapchain)(XaveriusDeviceBackendSwapchain *, XaveriusDeviceBackendDevice *, XaveriusDeviceBackendRoute *, const void *);
    XaveriusBackendResult (*destroy_swapchain)(XaveriusDeviceBackendSwapchain *);
    XaveriusBackendResult (*acquire_next_image)(XaveriusDeviceBackendSwapchain *, void *);
    XaveriusBackendResult (*queue_present)(XaveriusDeviceBackendQueue *, XaveriusDeviceBackendSwapchain *, uint32_t);
    XaveriusBackendResult (*cancel_acquire)(XaveriusDeviceBackendSwapchain *, void *);
    XaveriusBackendResult (*release_late_acquired_image)(XaveriusDeviceBackendSwapchain *, uint32_t);

    /* Operation retirement */
    XaveriusBackendResult (*retire_operation)(XaveriusDeviceBackendOperation *);

    /* Typed operation tables (immutable, const-qualified per ABI v1). */
    const struct XaveriusDeviceBackendBufferMemoryOps *buffer_memory;
    const struct XaveriusDeviceBackendShaderPipelineOps *shader_pipeline;
    const struct XaveriusDeviceBackendCommandOps *commands;
    const struct XaveriusDeviceBackendSynchronizationOps *synchronization;
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

    /* DIX wrapper pointer — tracks the wrapper owned by the DIX resource DB.
     * The PhysicalDevice holds a DIX-resource ownership reference separate from
     * wrapper refcount and caller_refcount. This ref is acquired exactly once
     * after successful AddResource/publication and released exactly once in
     * XaveriusPhysicalDeviceDIXDestroy when wrapper refcount reaches 0.
     * This ensures the PhysicalDevice record persists until both the DIX
     * resource and all registry references are gone. */
    XaveriusPhysicalDeviceDIXWrapper *dix_wrapper;

    /* DIX-resource ownership reference count. Acquired exactly once after
     * successful AddResource/publication in XaveriusPhysicalDevicePublish.
     * Released exactly once in XaveriusPhysicalDeviceDIXDestroy when the
     * wrapper refcount reaches 0. This is distinct from caller_refcount
     * (DDX-owned caller references) and wrapper->refcount (DIX-side
     * wrapper ownership). The PhysicalDevice record persists while this
     * refcount > 0 OR caller_refcount > 0 OR registry list membership. */
    uint32_t dix_resource_refcount;

    /* Publication flag: set to true after first successful Publish, never cleared.
     * Ensures a device can only be published once per server lifetime (spec:
     * "A published (or tombstoned) device retains its XID for the server lifetime"). */
    bool published;

    char ordering_key[128];

    /* Caller-owned reference count (from XaveriusRegistryRegisterDdxDevice).
     * Each successful registration returns a caller-owned reference that must
     * be released via XaveriusPhysicalDeviceRelease(). The registry itself
     * holds one permanent reference for the lifetime of the record. */
    uint32_t caller_refcount;

    /* Number of active observations referencing this device from all sources.
     * Incremented on successful ObserveDevice match, decremented on
     * RemoveObservation. Per spec §1038 and §1037: ConfirmDeviceRemoved
     * MUST NOT transition to REMOVED while active_observation_count > 0. */
    uint32_t active_observation_count;

    struct xorg_list list;
};

/* ---- ModuleRole control record (shared code/module lifetime & quiescence) ---- */
struct _XaveriusModuleRole {
    char *owner_key;
    XaveriusModuleLease *module_lease;            /* server-owned module lease */
    uint64_t role_generation;
    uint32_t admission;                /* open / quiescing / closed */
    uint32_t quiesce_state;            /* PENDING / SUCCESS / FAILURE */
    uint32_t exec_ref;                 /* active callback-execution references */
    uint32_t callback_admission_open;  /* callback admission gate */
    uint32_t quarantined;              /* role is quarantined */
    void (*retain_module_role)(XaveriusModuleRole *role);
    void (*release_module_role)(XaveriusModuleRole *role);
    XaveriusModuleQuiescenceResult (*quiesce_module_role)(XaveriusModuleRole *role);
    struct xorg_list list;
};

/* Global device registry (defined in xaverius_registry.c). Only one registry
 * exists per server process. */
extern struct xorg_list xaverius_physical_devices;
extern uint32_t xaverius_physical_device_count;
extern struct xorg_list xaverius_module_roles;
extern uint32_t xaverius_module_role_count;
extern struct xorg_list xaverius_tombstones;
extern uint32_t xaverius_tombstone_count;

/* Observation ID counter (defined in xaverius_registry.c). */
extern uint64_t xaverius_next_observation_id;
uint64_t xaverius_next_observation_id_fn(void);

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
 * which may fail on allocation/validation before any retain.
 *
 * Generation-checked callback admission: XaveriusModuleRoleAcquireCallbackRef()
 * acquires an execution reference before entering DDX backend module code;
 * XaveriusModuleRoleReleaseExecRef() releases it. Quiescence SUCCESS requires
 * exec_ref == 0 (no callback executing). XaveriusModuleRoleQuarantine() marks
 * the role as quarantined; a quarantined role cannot be unloaded.
 */
XaveriusRegistryResult XaveriusModuleRoleCreate(const XaveriusModuleRoleDescriptor *desc,
                                       XaveriusModuleRole **out_role);
void XaveriusModuleRoleRetain(XaveriusModuleRole *role);
void XaveriusModuleRoleRelease(XaveriusModuleRole *role);
XaveriusModuleQuiescenceResult XaveriusModuleRoleQuiesce(XaveriusModuleRole *role);
void XaveriusModuleRoleFinalize(XaveriusModuleRole *role);

/* Callback-execution reference management. Acquire before entering DDX backend
 * module code; release after XaveriusBackendCompletionDispatch returns. */
bool XaveriusModuleRoleAcquireExecRef(XaveriusModuleRole *role);
void XaveriusModuleRoleReleaseExecRef(XaveriusModuleRole *role);

/* Admission gate for callbacks. XaveriusModuleRoleAdmissionClose() atomically
 * marks the role as not accepting new callbacks; it does not cancel accepted
 * operations. XaveriusModuleRoleAdmissionOpen() queries the current state. */
void XaveriusModuleRoleAdmissionClose(XaveriusModuleRole *role);
bool XaveriusModuleRoleAdmissionOpen(XaveriusModuleRole *role);

/* Quarantine the role. A quarantined role cannot be unloaded and its code
 * and references are retained until server shutdown or a later proven-safe
 * retirement. */
void XaveriusModuleRoleQuarantine(XaveriusModuleRole *role);

/* ---- XaveriusPhysicalDeviceDIXWrapper — server-private DIX wrapper object.
 * The DIX resource DB holds a wrapper; the registry retains ownership of the
 * PhysicalDevice record itself. Freeing the wrapper (via XaveriusPhysicalDeviceDIXDestroy)
 * releases only DIX-side ownership, never registry membership or tombstone state.
 * The wrapper has its own reference count for DIX-side ownership tracking. */
struct _XaveriusPhysicalDeviceDIXWrapper {
    XaveriusPhysicalDevice *dev;
    uint32_t refcount;  /* DIX-side ownership reference count */
};

/* ---- XaveriusPhysicalDevicePublish — separate server-internal publication operation.
 * Allocates a server-private XID, prepares the DIX wrapper, and atomically
 * exposes it once AddResource() succeeds. */
XaveriusRegistryResult XaveriusPhysicalDevicePublish(XaveriusPhysicalDevice *dev,
                                                     uint64_t device_generation,
                                                     XID *out_xid);

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

/* Set the ordered owner_key preference list for binding selection (W0-153).
 * Caller retains ownership of the array and its strings. Passing NULL clears
 * the preference list (all owner_keys equal rank). */
void xaverius_binding_preference_set(const char * const *preference_keys);

/* ---- Observation / removal (Stage 3) ---- */
/* Discovery source and observation bookkeeping (minimal, server-side only).
 * Generation-checked, non-reusable generations for source and observation. */
typedef struct _XaveriusDiscoverySource {
    const char *source_key;   /* borrowed for the call only */
    uint64_t source_generation;
    uint64_t last_event_sequence; /* last source_event_sequence accepted */
    struct xorg_list list;
} XaveriusDiscoverySource;

typedef struct _XaveriusObservation {
    const char *source_key;   /* borrowed for the call only */
    uint64_t source_generation;
    uint64_t source_event_sequence; /* monotonically increasing per source */
    uint64_t observation_id;
    XaveriusDeviceIdentity identity;
    XaveriusPhysicalDevice *device; /* canonical record reference */
    uint64_t device_generation; /* generation of the device at observation time */
    struct xorg_list list;
} XaveriusObservation;

/* Removal evidence submitted to ConfirmDeviceRemoved. source_key is borrowed
 * for this call only; the registry resolves it to its registered source
 * identity before returning. */
typedef struct {
    const char *source_key;
    uint64_t source_generation;
    uint64_t observation_id;
    uint64_t source_event_sequence;
    uint64_t physical_device_generation;
} XaveriusDeviceRemovalEvidence;

/* Submit or update an identity observation from a discovery source.
 * Returns a generation-checked canonical record reference only when identity
 * matching is unambiguous; never allocates a protocol XID.
 * source_event_sequence is the monotonically increasing per-source sequence
 * captured when the source event was observed. out_observation_id returns a
 * unique nonzero observation ID per new observation.
 */
XaveriusRegistryResult XaveriusRegistryObserveDevice(void *registry,
    const char *source_key,
    uint64_t source_generation,
    uint64_t source_event_sequence,
    const XaveriusDeviceIdentity *identity_evidence,
    XaveriusPhysicalDevice **out_device,
    uint64_t *out_device_generation,
    uint64_t *out_observation_id);

/* Withdraw only this source's evidence. Must not mark hardware REMOVED
 * while other authoritative evidence still establishes the same incarnation. */
XaveriusRegistryResult XaveriusRegistryRemoveObservation(void *registry,
    const char *source_key,
    uint64_t source_generation,
    uint64_t observation_id);

/* Commit terminal REMOVED only on authoritative evidence that the physical
 * incarnation was removed. */
XaveriusRegistryResult XaveriusRegistryConfirmDeviceRemoved(void *registry,
    XaveriusPhysicalDevice *device,
    uint64_t device_generation,
    const XaveriusDeviceRemovalEvidence *removal_evidence);

/* Dispatch */
int ProcXaveriusDispatch(ClientPtr client);

/* ---- Legacy names kept only for transitional reference; do not reuse ---- */
/* XaveriusDevice, XaveriusDeviceCapabilities, XaveriusRegisterDevice,
 * XaveriusRegisterProvider are REMOVED — replaced by Xaverius* model above. */

#endif /* _XLIBRE_XAVERIUS_INTERN_H */
