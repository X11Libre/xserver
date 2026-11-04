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

#include "include/xlibre_ptrtypes.h"

#include "xaverius_proto.h"

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
    xavQueueFamilyInfo *queue_families;
    uint32_t queue_family_count;

    /* Memory properties */
    xavMemoryTypeInfo *memory_types;
    uint32_t memory_type_count;
    xavMemoryHeapInfo *memory_heaps;
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

typedef struct _XaveriusScreen XaveriusScreenRec, *XaveriusScreenPtr;

typedef struct _XaveriusDDXOps
{
    bool (*ScreenInit)(XaveriusScreenPtr xav_screen);
    void (*ScreenFini)(XaveriusScreenPtr xav_screen);
} XaveriusDDXOps;

/* Per-screen state (internal, not exposed to DDX beyond the pointer).
 * Screen-local only; PhysicalDevice discovery/registration is handled
 * by the DDX-wide registration entry point (XaveriusRegisterDevice in
 * xaverius_intern.h), not through Screen callbacks.
 */
struct _XaveriusScreen
{
    ScreenPtr pScreen;
    const XaveriusDDXOps *ops;   /* DDX callbacks, called by Xaverius */
    void *driver_private;        /* DDX-owned private data */
    bool initialized;
};

/* Called by DDX during ScreenInit to register screen-local state.
 * Strictly Screen-local; does not touch PhysicalDevice registration.
 * Device discovery/registration is DDX-wide via XaveriusRegisterDevice().
 */
_X_EXPORT bool XaveriusScreenInit(ScreenPtr pScreen);

_X_EXPORT XaveriusScreenPtr XaveriusGetScreen(ScreenPtr pScreen);

/* DDX-wide provider registration entry point (Phase A).
 * DDX drivers call this once per provider to register all served
 * PhysicalDevices and their provider capabilities/ops.
 * Strictly DDX-wide; no ScreenPtr, no per-screen state.
 */
_X_EXPORT bool XaveriusRegisterDevice(const XaveriusDeviceCapabilities *caps);

/* ===== DDX backend operation tables (spec/provider-operation-matrix.md §3) =====
 *
 * These declarations are the mandatory DDX/backend API contract.
 * Exact C type names follow the spec; opaque types are forward-declared
 * here and their payload fields are defined by the corresponding request
 * chapter's validated semantic data, not copied wire structs.
 */

/* Opaque DDX backend object handles. */
typedef struct _GpuBackendDevice          *GpuBackendDevice;
typedef struct _GpuBackendBuffer          *GpuBackendBuffer;
typedef struct _GpuBackendMemory          *GpuBackendMemory;
typedef struct _GpuBackendShaderModule    *GpuBackendShaderModule;
typedef struct _GpuBackendDescriptorSetLayout *GpuBackendDescriptorSetLayout;
typedef struct _GpuBackendDescriptorPool  *GpuBackendDescriptorPool;
typedef struct _GpuBackendDescriptorSet   *GpuBackendDescriptorSet;
typedef struct _GpuBackendPipelineLayout  *GpuBackendPipelineLayout;
typedef struct _GpuBackendPipeline        *GpuBackendPipeline;
typedef struct _GpuBackendCommandPool     *GpuBackendCommandPool;
typedef struct _GpuBackendCommandBuffer   *GpuBackendCommandBuffer;
typedef struct _GpuBackendSemaphore       *GpuBackendSemaphore;
typedef struct _GpuBackendFence           *GpuBackendFence;

/* Opaque operation token for the six asynchronous operations. */
typedef struct _GpuBackendOperationToken  *GpuBackendOperationToken;

/* DDX backend result codes (spec §6). */
typedef enum {
    GPU_BACKEND_SUCCESS                  = 0,
    GPU_BACKEND_UNSUPPORTED              = 1,
    GPU_BACKEND_STATUS                   = 2,
    GPU_BACKEND_UNAVAILABLE              = 3,
    GPU_BACKEND_DEVICE_LOST              = 4,
    GPU_BACKEND_OUT_OF_HOST_MEMORY       = 5,
    GPU_BACKEND_OUT_OF_DEVICE_MEMORY     = 6,
    GPU_BACKEND_TRANSFER_FAILURE         = 7,
    GPU_BACKEND_EXECUTION_FAILURE        = 8,
    GPU_BACKEND_CANCELLED                = 9,
    GPU_BACKEND_FATAL                    = 10,
} GpuBackendResult;

/* ----- Info/description structs (immutable, server-owned) ----- */

typedef struct _GpuBufferCreateInfo {
    uint64_t size;
    uint32_t usage_flags;
    uint32_t memory_type_bits;
} GpuBufferCreateInfo;

typedef struct _GpuBufferMemoryRequirements {
    uint64_t size;
    uint64_t alignment;
    uint32_t memory_type_bits;
} GpuBufferMemoryRequirements;

typedef struct _GpuMemoryAllocateInfo {
    uint64_t allocation_size;
    uint32_t memory_type_index;
} GpuMemoryAllocateInfo;

typedef struct _GpuBufferTransferInfo {
    GpuBackendBuffer  buffer;
    GpuBackendMemory  memory;
    uint64_t offset;
    uint64_t size;
} GpuBufferTransferInfo;

typedef struct _GpuShaderModuleCreateInfo {
    const uint32_t *code;
    uint32_t code_size;
} GpuShaderModuleCreateInfo;

typedef struct _GpuDescriptorSetLayoutCreateInfo {
    uint32_t binding_count;
} GpuDescriptorSetLayoutCreateInfo;

typedef struct _GpuDescriptorPoolCreateInfo {
    uint32_t max_sets;
} GpuDescriptorPoolCreateInfo;

typedef struct _GpuDescriptorSetAllocateInfo {
    GpuBackendDescriptorPool pool;
    GpuBackendDescriptorSetLayout *layouts;
    uint32_t layout_count;
} GpuDescriptorSetAllocateInfo;

typedef struct _GpuDescriptorUpdateInfo {
    GpuBackendDescriptorSet set;
    uint32_t binding;
    uint32_t array_element;
    uint32_t descriptor_count;
} GpuDescriptorUpdateInfo;

typedef struct _GpuPipelineLayoutCreateInfo {
    uint32_t set_layout_count;
} GpuPipelineLayoutCreateInfo;

typedef struct _GpuComputePipelineCreateInfo {
    GpuBackendPipelineLayout layout;
    GpuBackendShaderModule shader;
} GpuComputePipelineCreateInfo;

typedef struct _GpuCommandPoolCreateInfo {
    uint32_t queue_family_index;
    uint32_t flags;
} GpuCommandPoolCreateInfo;

typedef struct _GpuCommandBufferAllocateInfo {
    GpuBackendCommandPool pool;
    uint32_t level;
    uint32_t command_buffer_count;
} GpuCommandBufferAllocateInfo;

typedef struct _GpuCommandBufferBeginInfo {
    uint32_t flags;
} GpuCommandBufferBeginInfo;

typedef struct _GpuValidatedCommand {
    uint32_t opcode;
    const void *data;
    uint32_t data_size;
} GpuValidatedCommand;

typedef struct _GpuSemaphoreCreateInfo {
    uint32_t type;
    uint64_t initial_value;
} GpuSemaphoreCreateInfo;

typedef struct _GpuFenceCreateInfo {
    uint32_t flags;
} GpuFenceCreateInfo;

/* ----- The four typed operation tables ----- */

struct GpuBackendBufferMemoryOps {
    GpuBackendResult (*create_buffer)(
        GpuBackendDevice, const GpuBufferCreateInfo *,
        GpuBackendBuffer **out_buffer);
    GpuBackendResult (*free_buffer)(
        GpuBackendDevice, GpuBackendBuffer *);
    GpuBackendResult (*get_buffer_memory_requirements)(
        GpuBackendDevice, GpuBackendBuffer *,
        GpuBufferMemoryRequirements *out_requirements);
    GpuBackendResult (*allocate_memory)(
        GpuBackendDevice, const GpuMemoryAllocateInfo *,
        GpuBackendMemory **out_memory);
    GpuBackendResult (*free_memory)(
        GpuBackendDevice, GpuBackendMemory *);
    GpuBackendResult (*bind_buffer_memory)(
        GpuBackendDevice, GpuBackendBuffer *,
        GpuBackendMemory *, uint64_t offset);
    GpuBackendResult (*write_buffer)(
        GpuBackendDevice, GpuBackendBuffer *, GpuBackendMemory *,
        const GpuBufferTransferInfo *, GpuBackendOperationToken *);
    GpuBackendResult (*read_buffer)(
        GpuBackendDevice, GpuBackendBuffer *, GpuBackendMemory *,
        const GpuBufferTransferInfo *, GpuBackendOperationToken *);
};

struct GpuBackendShaderPipelineOps {
    GpuBackendResult (*create_shader_module)(
        GpuBackendDevice, const GpuShaderModuleCreateInfo *,
        GpuBackendShaderModule **out_shader);
    GpuBackendResult (*create_descriptor_set_layout)(
        GpuBackendDevice, const GpuDescriptorSetLayoutCreateInfo *,
        GpuBackendDescriptorSetLayout **out_layout);
    GpuBackendResult (*create_descriptor_pool)(
        GpuBackendDevice, const GpuDescriptorPoolCreateInfo *,
        GpuBackendDescriptorPool **out_pool);
    GpuBackendResult (*allocate_descriptor_sets)(
        GpuBackendDevice, GpuBackendDescriptorPool *,
        const GpuDescriptorSetAllocateInfo *, uint32_t set_count,
        GpuBackendDescriptorSet **out_sets);
    GpuBackendResult (*update_descriptor_sets)(
        GpuBackendDevice, const GpuDescriptorUpdateInfo *,
        uint32_t update_count);
    GpuBackendResult (*free_descriptor_sets)(
        GpuBackendDevice, GpuBackendDescriptorPool *,
        GpuBackendDescriptorSet *const *sets, uint32_t set_count);
    GpuBackendResult (*create_pipeline_layout)(
        GpuBackendDevice, const GpuPipelineLayoutCreateInfo *,
        GpuBackendPipelineLayout **out_layout);
    GpuBackendResult (*create_compute_pipeline)(
        GpuBackendDevice, const GpuComputePipelineCreateInfo *,
        GpuBackendPipeline **out_pipeline);
    GpuBackendResult (*destroy_shader_module)(
        GpuBackendDevice, GpuBackendShaderModule *);
    GpuBackendResult (*destroy_descriptor_set_layout)(
        GpuBackendDevice, GpuBackendDescriptorSetLayout *);
    GpuBackendResult (*destroy_descriptor_pool)(
        GpuBackendDevice, GpuBackendDescriptorPool *);
    GpuBackendResult (*destroy_pipeline_layout)(
        GpuBackendDevice, GpuBackendPipelineLayout *);
    GpuBackendResult (*destroy_compute_pipeline)(
        GpuBackendDevice, GpuBackendPipeline *);
};

struct GpuBackendCommandOps {
    GpuBackendResult (*create_command_pool)(
        GpuBackendDevice, const GpuCommandPoolCreateInfo *,
        GpuBackendCommandPool **out_pool);
    GpuBackendResult (*destroy_command_pool)(
        GpuBackendDevice, GpuBackendCommandPool *);
    GpuBackendResult (*allocate_command_buffer)(
        GpuBackendDevice, GpuBackendCommandPool *,
        const GpuCommandBufferAllocateInfo *,
        GpuBackendCommandBuffer **out_command_buffer);
    GpuBackendResult (*free_command_buffer)(
        GpuBackendDevice, GpuBackendCommandPool *,
        GpuBackendCommandBuffer *);
    GpuBackendResult (*reset_command_buffer)(
        GpuBackendDevice, GpuBackendCommandBuffer *);
    GpuBackendResult (*begin_command_buffer)(
        GpuBackendDevice, GpuBackendCommandBuffer *,
        const GpuCommandBufferBeginInfo *);
    GpuBackendResult (*record_commands)(
        GpuBackendDevice, GpuBackendCommandBuffer *,
        const GpuValidatedCommand *, uint32_t command_count);
    GpuBackendResult (*end_command_buffer)(
        GpuBackendDevice, GpuBackendCommandBuffer *);
};

struct GpuBackendSynchronizationOps {
    GpuBackendResult (*create_semaphore)(
        GpuBackendDevice, const GpuSemaphoreCreateInfo *,
        GpuBackendSemaphore **out_semaphore);
    GpuBackendResult (*host_signal_semaphore)(
        GpuBackendDevice, GpuBackendSemaphore *, uint64_t value,
        GpuBackendOperationToken *);
    GpuBackendResult (*destroy_semaphore)(
        GpuBackendDevice, GpuBackendSemaphore *);
    GpuBackendResult (*create_fence)(
        GpuBackendDevice, const GpuFenceCreateInfo *,
        GpuBackendFence **out_fence);
    GpuBackendResult (*reset_fence)(
        GpuBackendDevice, GpuBackendFence *);
    GpuBackendResult (*destroy_fence)(
        GpuBackendDevice, GpuBackendFence *);
};

/* ----- Top-level DDX device backend ops ----- */

typedef struct _GpuDeviceBackendOps GpuDeviceBackendOps;

struct _GpuDeviceBackendOps {
    /* Physical device snapshot */
    GpuBackendResult (*query_physical_snapshot)(
        GpuBackendDevice, void **out_snapshot);

    /* Logical device lifecycle */
    GpuBackendResult (*create_logical_device)(
        GpuBackendDevice, void **out_logical_device);
    GpuBackendResult (*get_queue)(
        GpuBackendDevice, uint32_t queue_family_index,
        uint32_t queue_index, void **out_queue);
    GpuBackendResult (*destroy_logical_device)(
        GpuBackendDevice, void *logical_device);

    /* Queue submission (async) */
    GpuBackendResult (*queue_submit)(
        GpuBackendDevice, void *queue,
        const GpuValidatedCommand *, uint32_t command_count,
        GpuBackendOperationToken *);

    /* Surface lifecycle */
    GpuBackendResult (*create_surface)(
        GpuBackendDevice, void **out_surface);
    GpuBackendResult (*destroy_surface)(
        GpuBackendDevice, void *surface);
    GpuBackendResult (*query_surface_support)(
        GpuBackendDevice, void *surface, void **out_capabilities);

    /* Presentation route */
    GpuBackendResult (*create_presentation_route)(
        GpuBackendDevice, void *surface, void **out_route);
    GpuBackendResult (*destroy_presentation_route)(
        GpuBackendDevice, void *route);

    /* Swapchain */
    GpuBackendResult (*create_swapchain)(
        GpuBackendDevice, void *route, void **out_swapchain);
    GpuBackendResult (*destroy_swapchain)(
        GpuBackendDevice, void *swapchain);
    GpuBackendResult (*acquire_next_image)(
        GpuBackendDevice, void *swapchain, GpuBackendOperationToken *);
    GpuBackendResult (*queue_present)(
        GpuBackendDevice, void *queue, void *swapchain,
        GpuBackendOperationToken *);
    GpuBackendResult (*cancel_acquire)(
        GpuBackendDevice, GpuBackendOperationToken *);
    GpuBackendResult (*release_late_acquired_image)(
        GpuBackendDevice, void *swapchain, uint32_t image_index);

    /* Typed operation tables */
    const struct GpuBackendBufferMemoryOps *buffer_memory;
    const struct GpuBackendShaderPipelineOps *shader_pipeline;
    const struct GpuBackendCommandOps *commands;
    const struct GpuBackendSynchronizationOps *synchronization;

    /* DDX backend/module lifetime hooks */
    void (*retire_operation)(void *token_userdata, GpuBackendResult result);
    void (*quiesce)(GpuBackendDevice);
};

#endif /* _XLIBRE_XAVERIUS_PRIV_H */
