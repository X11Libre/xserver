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

#include "include/list.h"
#include "include/privates.h"
#include "include/xlibre_ptrtypes.h"

#include "xaverius_priv.h"
#include "xaverius_proto.h"

/* Forward declarations for internal types */
struct _XaveriusPhysicalDevice;
typedef struct _XaveriusPhysicalDevice XaveriusPhysicalDevice;

extern int XaveriusEventBase;

extern DevPrivateKeyRec XaveriusScreenPrivateKeyRec;

/* Global property atoms (initialized in init.c) */
extern Atom xaverius_atom_device_uuid;
extern Atom xaverius_atom_driver_uuid;
extern Atom xaverius_atom_max_work_group_invocations;
extern Atom xaverius_atom_max_work_group_size;
extern Atom xaverius_atom_timeline_semaphore;

/*
 * Device type values (xserver-internal, no separate xaverius_device.h header).
 */
#define XAVERIUS_DEVICE_TYPE_PHYSICAL    1
#define XAVERIUS_DEVICE_TYPE_LOGICAL     2

/* ---- DDX backend ABI version (v0.1) ---- */
#define XAVERIUS_DEVICE_BACKEND_OPS_ABI_VERSION 1

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

/* ---- Top-level DDX backend operations descriptor (v0.1) ---- */
struct XaveriusDeviceBackendOps {
    uint64_t struct_size;
    uint64_t abi_version;

    /* Physical snapshot query */
    XaveriusBackendResult (*query_physical_snapshot)(void *, void *);

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
    const struct XaveriusDeviceBackendBufferMemoryOps *buffer_memory;
    const struct XaveriusDeviceBackendShaderPipelineOps *shader_pipeline;
    const struct XaveriusDeviceBackendCommandOps *commands;
    const struct XaveriusDeviceBackendSynchronizationOps *synchronization;
};

/* ---- PhysicalDevice internal representation ---- */
struct _XaveriusPhysicalDevice {
    /* Canonical identity evidence (structured, compared per §2.2). */
    XaveriusDeviceIdentity identity;

    /* Immutable, server-owned identity evidence copy */
    void *identity_evidence;
    uint64_t identity_evidence_size;

    /* Immutable, server-owned capability/metadata snapshot (member/value) */
    void *snapshot;
    uint64_t snapshot_size;

    /* Current discovery/availability status */
    uint32_t status;

    /* Exact selected DDX binding generation */
    uint64_t binding_generation;

    /* Exact device incarnation generation */
    uint64_t device_generation;

    /* Owning ModuleRole generation */
    uint64_t module_role_generation;

    struct xorg_list list;
};

/* ---- Direct DDX device backend binding ---- */
struct _XaveriusDeviceBackendBinding {
    const struct XaveriusDeviceBackendOps *ops;     /* immutable typed ops tables */
    int native_handle;                         /* borrowed DDX-owned */
    void *context;                             /* borrowed DDX-owned */
    uint64_t binding_generation;
    uint32_t availability;
    /* Retained references to reachable code/module/workers */
    void *module_role_ref;
    struct xorg_list list;
};

/* ---- ModuleRole control record (shared code/module lifetime & quiescence) ---- */
struct _XaveriusModuleRole {
    char *owner_key;
    bool is_permanent;
    void *module_lease_ref;            /* loader/module pin */
    uint64_t role_generation;
    uint32_t admission;                /* open / quiescing / closed */
    uint32_t quiesce_state;            /* PENDING / SUCCESS / FAILURE */
    void (*retain_module_role)(void *role);
    void (*release_module_role)(void *role);
    int  (*quiesce_module_role)(void *role);
    void *role_userdata;
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

typedef enum {
    XAVERIUS_MODULE_ROLE_QUIESCE_PENDING  = 0,
    XAVERIUS_MODULE_ROLE_QUIESCE_SUCCESS  = 1,
    XAVERIUS_MODULE_ROLE_QUIESCE_FAILURE  = 2,
} XaveriusModuleRoleQuiesceState;

/* Create / retain / release / quiesce / finalize a ModuleRole control record.
 * These are infallible, non-blocking lifetime primitives except XaveriusModuleRoleCreate
 * which may fail on allocation/validation before any retain. */
XaveriusRegistryResult XaveriusModuleRoleCreate(const XaveriusModuleRoleDescriptor *desc,
                                      void **out_role);
void XaveriusModuleRoleRetain(void *role);
void XaveriusModuleRoleRelease(void *role);
int XaveriusModuleRoleQuiesce(void *role); /* returns XaveriusModuleRoleQuiesceState */
void XaveriusModuleRoleFinalize(void *role);

/* Is admission still open for this role? */
bool XaveriusModuleRoleAdmissionOpen(void *role);

/* ---- XaPhysicalDevicePublish — separate server-internal publication operation.
 * Allocates a server-private XID, prepares the DIX wrapper, and atomically
 * exposes it once AddResource() succeeds. */
bool XaPhysicalDevicePublish(XaveriusPhysicalDevice *dev);

/* DIX resource destructor — canonical path, runs exactly once per published record. */
int XaveriusPhysicalDeviceDIXDestroy(void *value, XID xid);

/* Lookup / iteration */
XaveriusPhysicalDevice *XaveriusFindDeviceByBindingGeneration(uint64_t binding_generation);
XaveriusPhysicalDevice *XaveriusFirstDevice(void);
XaveriusPhysicalDevice *XaveriusNextDevice(XaveriusPhysicalDevice *device);
uint32_t XaveriusDeviceCount(void);

/* Normative DDX-facing registration API (declared in xaverius_priv.h). */
XaveriusRegistryResult XaveriusRegistryRegisterDdxDevice(void *registry,
                                               const XaveriusDeviceBackendRegistration *registration,
                                               XaveriusPhysicalDevice **out_device,
                                               uint64_t *out_device_generation,
                                               uint64_t *out_binding_generation);
void XaveriusPhysicalDeviceRelease(XaveriusPhysicalDevice *device);
XaveriusRegistryResult XaveriusRegistryRemoveDeviceBackendBinding(void *registry,
                                                        XaveriusPhysicalDevice *device,
                                                        uint64_t device_generation,
                                                        uint64_t module_role_generation,
                                                        uint64_t binding_generation);

/* Dispatch */
int ProcXaveriusDispatch(ClientPtr client);

/* ---- Legacy names kept only for transitional reference; do not reuse ---- */
/* XaveriusDevice, XaveriusDeviceCapabilities, XaveriusRegisterDevice,
 * XaveriusRegisterProvider are REMOVED — replaced by Xaverius* model above. */

#endif /* _XLIBRE_XAVERIUS_INTERN_H */
