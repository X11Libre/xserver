/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */

#ifndef __XLIBRE_XAVERIUS_PROTO_H
#define __XLIBRE_XAVERIUS_PROTO_H

#include <stddef.h>
#include <float.h>
#include <X11/X.h>
#include <X11/Xmd.h>

/*
 * XAverius v0.1 shared X11 wire protocol declarations.
 * This is the single public protocol header for both server and clients.
 */

/* Constants, registries, request subopcodes, event IDs and flags. */
#define XAVERIUS_MAJOR_VERSION 0
#define XAVERIUS_MINOR_VERSION 1
#define XAVERIUS_EXTENSION_NAME "XAVERIUS"

#define XAV_REQ_QUERY_VERSION 0
#define XAV_REQ_ENUMERATE_PHYSICAL_DEVICES 1
#define XAV_REQ_GET_PHYSICAL_DEVICE_PROPERTY 2
#define XAV_REQ_GET_PHYSICAL_DEVICE_QUEUE_FAMILY_PROPERTIES 3
#define XAV_REQ_CREATE_DEVICE 4
#define XAV_REQ_CREATE_DEFAULT_DEVICE 5
#define XAV_REQ_DESTROY_DEVICE 6
#define XAV_REQ_GET_PHYSICAL_DEVICE_MEMORY_PROPERTIES 7
#define XAV_REQ_CREATE_BUFFER 8
#define XAV_REQ_GET_BUFFER_MEMORY_REQUIREMENTS 9
#define XAV_REQ_ALLOCATE_MEMORY 10
#define XAV_REQ_FREE_MEMORY 11
#define XAV_REQ_BIND_BUFFER_MEMORY 12
#define XAV_REQ_WRITE_BUFFER 13
#define XAV_REQ_READ_BUFFER 14
#define XAV_REQ_SHADER_MODULE_DATA 15
#define XAV_REQ_CREATE_DESCRIPTOR_SET_LAYOUT 16
#define XAV_REQ_CREATE_DESCRIPTOR_POOL 17
#define XAV_REQ_DESTROY_DESCRIPTOR_POOL 18
#define XAV_REQ_ALLOCATE_DESCRIPTOR_SETS 19
#define XAV_REQ_UPDATE_DESCRIPTOR_SETS 20
#define XAV_REQ_FREE_DESCRIPTOR_SETS 21
#define XAV_REQ_CREATE_PIPELINE_LAYOUT 22
#define XAV_REQ_CREATE_COMPUTE_PIPELINE 23
#define XAV_REQ_CREATE_COMMAND_POOL 24
#define XAV_REQ_DESTROY_COMMAND_POOL 25
#define XAV_REQ_RESET_COMMAND_BUFFER 26
#define XAV_REQ_COMMAND_BUFFER_DATA 27
#define XAV_REQ_QUEUE_SUBMIT 28
#define XAV_REQ_CREATE_SEMAPHORE 29
#define XAV_REQ_GET_SEMAPHORE_COUNTER_VALUE 30
#define XAV_REQ_WAIT_SEMAPHORES 31
#define XAV_REQ_SIGNAL_SEMAPHORE 32
#define XAV_REQ_DESTROY_SEMAPHORE 33
#define XAV_REQ_CREATE_FENCE 34
#define XAV_REQ_GET_FENCE_STATUS 35
#define XAV_REQ_WAIT_FOR_FENCE 36
#define XAV_REQ_RESET_FENCE 37
#define XAV_REQ_DESTROY_FENCE 38
#define XAV_REQ_DESTROY_RESOURCE 39
#define XAV_REQ_SELECT_EVENTS 40
#define XAV_REQ_CREATE_SURFACE 41
#define XAV_REQ_DESTROY_SURFACE 42
#define XAV_REQ_GET_SURFACE_CAPABILITIES 43
#define XAV_REQ_CREATE_SWAPCHAIN 44
#define XAV_REQ_DESTROY_SWAPCHAIN 45
#define XAV_REQ_ACQUIRE_NEXT_IMAGE 46
#define XAV_REQ_QUEUE_PRESENT 47
#define XAV_REQ_LAST_COMPUTE 40
#define XAV_REQ_FIRST_PRESENTATION 41
#define XAV_REQ_LAST_PRESENTATION 47
#define XAV_ERROR_DEVICE_UNAVAILABLE 0
#define XAV_ERROR_DEVICE_LOST 1
#define XAV_ERROR_INVALID_SYNCHRONIZATION_STATE 2
#define XAV_ERROR_EXECUTION_FAILED 3
#define XAV_ERROR_TRANSFER_FAILED 4
#define XAV_ERROR_ALLOCATION_FAILED 5
#define XAV_ERROR_DEPENDENCY_FAILED 6
#define XAV_ERROR_INTERNAL_SERVER_ERROR 7
#define XAV_ERROR_DEVICE_DESTROYED 8
#define XAV_EVENT_ASYNC_ERROR 0
#define XAV_EVENT_DEVICE_LOST 1
#define XAV_EVENT_DEVICE_STATUS_CHANGED 2
#define XAV_EVENT_DEVICE_MEMORY_PRESSURE 3
#define XAV_EVENT_DEVICE_IDLE 4
#define XAV_EVENT_SUBMISSION_COMPLETE 5
#define XAV_EVENT_SUBMISSION_FAILED 6
#define XAV_EVENT_IMAGE_RELEASED 7
#define XAV_EVENT_PRESENTATION_COMPLETE 8
#define XAV_CMD_BIND_PIPELINE 1
#define XAV_CMD_BIND_DESCRIPTOR_SETS 2
#define XAV_CMD_DISPATCH 3
#define XAV_CMD_PUSH_CONSTANTS 4
#define XAV_CMD_COPY_BUFFER_TO_SWAPCHAIN_IMAGE 5
#define XAV_FLAG_CREATE 0x01
#define XAV_FLAG_FINAL 0x02
#define XAV_FLAG_ABORT 0x04
#define XAV_BUFFER_USAGE_STORAGE_BUFFER 0x00000001U
#define XAV_BUFFER_USAGE_TRANSFER_SRC 0x00000002U
#define XAV_BUFFER_USAGE_TRANSFER_DST 0x00000004U
#define XAV_BUFFER_USAGE_UNIFORM_BUFFER 0x00000008U
#define XAV_BUFFER_USAGE_INDEX_BUFFER 0x00000010U
#define XAV_BUFFER_USAGE_VERTEX_BUFFER 0x00000020U
#define XAV_BUFFER_USAGE_INDIRECT_BUFFER 0x00000040U
#define XAV_QUEUE_GRAPHICS 0x00000001U
#define XAV_QUEUE_COMPUTE 0x00000002U
#define XAV_QUEUE_TRANSFER 0x00000004U
#define XAV_QUEUE_SPARSE_BINDING 0x00000008U
#define XAV_QUEUE_PROTECTED 0x00000010U
#define XAV_MEMORY_DEVICE_LOCAL 0x00000001U
#define XAV_MEMORY_HOST_VISIBLE 0x00000002U
#define XAV_MEMORY_HOST_COHERENT 0x00000004U
#define XAV_MEMORY_HOST_CACHED 0x00000008U
#define XAV_MEMORY_LAZILY_ALLOCATED 0x00000010U
#define XAV_MEMORY_PROTECTED 0x00000020U
#define XAV_MEMORY_HEAP_DEVICE_LOCAL 0x00000001U
#define XAV_STAGE_COMPUTE 0x00000001U
#define XAV_WAIT_STAGE_DRAW_INDIRECT 0x00000001U
#define XAV_WAIT_STAGE_COMPUTE_SHADER 0x00000002U
#define XAV_WAIT_STAGE_TRANSFER 0x00000004U
#define XAV_WAIT_STAGE_ALL_COMMANDS 0x00000008U
#define XAV_DESCRIPTOR_TYPE_UNSPECIFIED 0
#define XAV_DESCRIPTOR_TYPE_STORAGE_BUFFER 1
#define XAV_DEVICE_TYPE_OTHER 0
#define XAV_DEVICE_TYPE_INTEGRATED_GPU 1
#define XAV_DEVICE_TYPE_DISCRETE_GPU 2
#define XAV_DEVICE_TYPE_VIRTUAL_GPU 3
#define XAV_DEVICE_TYPE_CPU 4
#define XAV_PROPERTY_NONE 0
#define XAV_PROPERTY_CARD32 1
#define XAV_PROPERTY_CARD64 2
#define XAV_PROPERTY_ATOM 3
#define XAV_PROPERTY_STRING8 4
#define XAV_PROPERTY_BYTE_ARRAY 5
#define XAV_PROPERTY_CARD32_ARRAY 6
#define XAV_PROPERTY_CARD64_ARRAY 7
#define XAV_PROPERTY_STATUS_SUCCESS 0
#define XAV_PROPERTY_STATUS_NOT_SUPPORTED 1
#define XAV_PROPERTY_STATUS_NOT_AVAILABLE 2
#define XAV_SEMAPHORE_BINARY 0
#define XAV_SEMAPHORE_TIMELINE 1
#define XAV_WAIT_ANY 0
#define XAV_WAIT_ALL 1
#define XAV_FENCE_UNSIGNALED 0
#define XAV_FENCE_PENDING 1
#define XAV_FENCE_SIGNALED 2
#define XAV_FENCE_FAILED 3
#define XAV_WAIT_STATUS_SUCCESS 0
#define XAV_WAIT_STATUS_TIMEOUT 1
#define XAV_FENCE_WAIT_EXECUTION_FAILED 2
#define XAV_FENCE_WAIT_DEVICE_LOST 3
#define XAV_FENCE_WAIT_DEVICE_DESTROYED 4
#define XAV_COMMAND_POOL_TRANSIENT 0x00000001U
#define XAV_COMMAND_POOL_RESET_COMMAND_BUFFER 0x00000002U
#define XAV_COMMAND_BUFFER_INITIAL 0
#define XAV_COMMAND_BUFFER_RECORDING 1
#define XAV_COMMAND_BUFFER_EXECUTABLE 2
#define XAV_COMMAND_BUFFER_PENDING 3
#define XAV_COMMAND_BUFFER_INVALID 4
#define XAV_SCOPE_CONNECTION 0
#define XAV_SCOPE_DEVICE 1
#define XAV_SCOPE_QUEUE 2
#define XAV_EVENT_MASK_CONNECTION_DEVICE_STATUS 0x00000001U
#define XAV_EVENT_MASK_DEVICE_MEMORY_PRESSURE 0x00000001U
#define XAV_EVENT_MASK_DEVICE_IDLE 0x00000002U
#define XAV_EVENT_MASK_QUEUE_SUBMISSION_COMPLETE 0x00000001U
#define XAV_EVENT_MASK_QUEUE_SUBMISSION_FAILED 0x00000002U
#define XAV_EVENT_MASK_QUEUE_IMAGE_RELEASED 0x00000004U
#define XAV_EVENT_MASK_QUEUE_PRESENTATION_COMPLETE 0x00000008U
#define XAV_ASYNC_ORIGIN_UNSPECIFIED 0
#define XAV_ASYNC_ORIGIN_QUEUE_SUBMIT 1
#define XAV_ASYNC_ORIGIN_WRITE_BUFFER 2
#define XAV_ASYNC_ORIGIN_READ_BUFFER 3
#define XAV_ASYNC_ORIGIN_SEMAPHORE 4
#define XAV_ASYNC_ORIGIN_FENCE 5
#define XAV_ASYNC_ORIGIN_DEVICE 6
#define XAV_SUBMISSION_FAILURE_EXECUTION_FAILED 0
#define XAV_SUBMISSION_FAILURE_DEVICE_LOST 1
#define XAV_DEVICE_STATUS_AVAILABLE 0
#define XAV_DEVICE_STATUS_UNAVAILABLE 1
#define XAV_DEVICE_STATUS_REMOVED 2
#define XAV_MEMORY_PRESSURE_NORMAL 0
#define XAV_MEMORY_PRESSURE_LOW 1
#define XAV_MEMORY_PRESSURE_MEDIUM 2
#define XAV_MEMORY_PRESSURE_HIGH 3
#define XAV_IDLE_REASON_UNSPECIFIED 0
#define XAV_IDLE_REASON_NO_PENDING_WORK 1
#define XAV_IDLE_REASON_WORK_COMPLETED 2
#define XAV_REASON_UNSPECIFIED 0
#define XAV_REASON_HOTPLUG_REMOVAL 1
#define XAV_REASON_DRIVER_FAILURE 2
#define XAV_REASON_VULKAN_DEVICE_LOST 3
#define XAV_REASON_SERVER_POLICY 4
#define XAV_REASON_SERVER_SHUTDOWN 5
#define XAV_PRESENT_STATUS_SUCCESS 0
#define XAV_PRESENT_STATUS_OUT_OF_DATE 1
#define XAV_PRESENT_STATUS_SURFACE_LOST 2
#define XAV_PRESENT_STATUS_DEVICE_LOST 3
#define XAV_PRESENT_STATUS_PRESENTATION_FAILED 4
#define XAV_SURFACE_STATUS_SUCCESS 0
#define XAV_SURFACE_STATUS_TEMPORARILY_NOT_PRESENTABLE 1
#define XAV_SURFACE_STATUS_LOST 2
#define XAV_SURFACE_STATUS_DEVICE_LOST 3
#define XAV_SWAPCHAIN_STATUS_SUCCESS 0
#define XAV_SWAPCHAIN_STATUS_OUT_OF_DATE 1
#define XAV_SWAPCHAIN_STATUS_SURFACE_LOST 2
#define XAV_SWAPCHAIN_STATUS_DEVICE_LOST 3
#define XAV_SWAPCHAIN_STATUS_UNSUPPORTED_CONFIGURATION 4
#define XAV_SWAPCHAIN_STATUS_ALLOCATION_FAILED 5
#define XAV_QUEUE_PRESENT_STATUS_SUCCESS 0
#define XAV_QUEUE_PRESENT_STATUS_OUT_OF_DATE 1
#define XAV_QUEUE_PRESENT_STATUS_SURFACE_LOST 2
#define XAV_QUEUE_PRESENT_STATUS_DEVICE_LOST 3
#define XAV_FORMAT_RGBA8_UNORM 1
#define XAV_FORMAT_BGRA8_UNORM 2
#define XAV_COLOR_SPACE_SRGB_NONLINEAR 1
#define XAV_PRESENT_MODE_FIFO 1
#define XAV_ACQUIRE_STATUS_SUCCESS 0
#define XAV_ACQUIRE_STATUS_TIMEOUT 1
#define XAV_ACQUIRE_STATUS_NOT_READY 2
#define XAV_ACQUIRE_STATUS_OUT_OF_DATE 3
#define XAV_ACQUIRE_STATUS_SURFACE_LOST 4
#define XAV_ACQUIRE_STATUS_DEVICE_LOST 5
#define XAV_ACQUIRE_STATUS_DEVICE_DESTROYED 6
#define XAV_PROPERTY_NAME_DEVICE_UUID "XAverius.deviceUUID"
#define XAV_PROPERTY_NAME_DRIVER_UUID "XAverius.driverUUID"
#define XAV_PROPERTY_NAME_MAX_WORK_GROUP_INVOCATIONS "XAverius.compute.maxWorkGroupInvocations"
#define XAV_PROPERTY_NAME_MAX_WORK_GROUP_SIZE "XAverius.compute.maxWorkGroupSize"
#define XAV_PROPERTY_NAME_TIMELINE_SEMAPHORE "XAverius.timelineSemaphore"
#define XAV_IMAGE_USAGE_TRANSFER_DST 0x00000001U
#define XAV_MAX_REQUEST_BYTES 262140U
#define XAV_REQUEST_HEADER_BYTES 8U
#define XAV_REPLY_HEADER_BYTES 32U
#define XAV_XGE_HEADER_BYTES 32U
#define XAV_XGE_GENERIC_EVENT 35U
#define XAV_MAX_REQUEST_WORDS 65535U

/* Wire XIDs/Atoms are always CARD32; Xlib XID/Atom may be wider on LP64. */
typedef float xavFloat32;
#pragma pack(push, 4)

typedef struct { CARD8 reqType; CARD8 data; CARD16 length; CARD8 subopcode; CARD8 flags; CARD16 reserved; } xavReqHeader;
typedef struct { CARD8 response_type; CARD8 data; CARD16 sequenceNumber; CARD32 length; CARD32 pad[6]; } xavReplyHeader;
typedef struct {
    CARD8 type; CARD8 errorCode; CARD16 sequenceNumber; CARD32 resourceID;
    CARD16 minorCode; CARD8 majorCode; CARD8 pad0; CARD32 pad[5];
} xavErrorPacket;
typedef struct { CARD8 response_type; CARD8 extension; CARD16 sequenceNumber; CARD32 length; CARD16 evtype; CARD8 pad[22]; } xavXGEHeader;

typedef struct { xavReqHeader header; } xavQueryVersionReq;
typedef struct { xavReqHeader header; } xavEnumeratePhysicalDevicesReq;
typedef struct { xavReqHeader header; CARD32 physical_device_xid; CARD32 property_atom; } xavGetPhysicalDevicePropertyReq;
typedef struct { xavReqHeader header; CARD32 physical_device_xid; } xavGetPhysicalDeviceQueueFamilyPropertiesReq;
typedef struct { CARD32 queue_family_index; CARD16 queue_count; CARD16 reserved; xavFloat32 queue_priorities[1]; } xavQueueFamilyRequest;
typedef struct { CARD32 feature_id; CARD32 enabled; } xavDeviceFeatureRequest;
typedef struct { CARD16 name_length; CARD16 reserved; CARD8 name[1]; } xavDeviceExtensionRequest;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 physical_device_xid; CARD16 queue_family_request_count; CARD16 feature_request_count; CARD16 device_extension_count; CARD16 reserved; CARD8 requests[1]; } xavCreateDeviceReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD16 queue_count; CARD16 reserved; xavFloat32 priorities[1]; } xavCreateDefaultDeviceReq;
typedef struct { xavReqHeader header; CARD32 device_xid; } xavDestroyDeviceReq;
typedef struct { xavReqHeader header; CARD32 physical_device_xid; } xavGetPhysicalDeviceMemoryPropertiesReq;

typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 buffer_xid; CARD64 size; CARD32 usage_flags; CARD32 reserved; } xavCreateBufferReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 buffer_xid; } xavGetBufferMemoryRequirementsReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 memory_xid; CARD64 allocation_size; CARD32 memory_type_index; CARD32 reserved; } xavAllocateMemoryReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 memory_xid; } xavFreeMemoryReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 buffer_xid; CARD32 memory_xid; CARD64 memory_offset; } xavBindBufferMemoryReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 buffer_xid; CARD64 buffer_offset; CARD32 data_length; CARD8 data[1]; } xavWriteBufferReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 buffer_xid; CARD64 buffer_offset; CARD32 data_length; } xavReadBufferReq;

typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 shader_module_xid; CARD32 data_length; CARD8 data[1]; } xavShaderModuleDataReq;
typedef struct { CARD32 binding; CARD32 descriptor_type; CARD32 descriptor_count; CARD32 stage_flags; } xavDescriptorSetLayoutBinding;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 descriptor_set_layout_xid; CARD32 flags; CARD32 binding_count; xavDescriptorSetLayoutBinding bindings[1]; } xavCreateDescriptorSetLayoutReq;
typedef struct { CARD32 descriptor_type; CARD32 descriptor_count; } xavDescriptorPoolSize;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 descriptor_pool_xid; CARD32 max_sets; CARD16 pool_size_count; CARD16 reserved; xavDescriptorPoolSize pool_sizes[1]; } xavCreateDescriptorPoolReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 descriptor_pool_xid; } xavDestroyDescriptorPoolReq;
typedef struct { CARD32 descriptor_set_xid; CARD32 descriptor_set_layout_xid; } xavDescriptorSetAllocation;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 descriptor_pool_xid; CARD16 descriptor_set_count; CARD16 reserved; xavDescriptorSetAllocation allocations[1]; } xavAllocateDescriptorSetsReq;
typedef struct { CARD32 descriptor_set_xid; CARD32 binding; CARD32 array_element; CARD32 descriptor_type; CARD32 buffer_xid; CARD64 buffer_offset; CARD64 range; } xavDescriptorWrite;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD16 write_count; CARD16 reserved0; CARD32 reserved1; xavDescriptorWrite writes[1]; } xavUpdateDescriptorSetsReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 descriptor_pool_xid; CARD16 descriptor_set_count; CARD16 reserved; CARD32 descriptor_set_xids[1]; } xavFreeDescriptorSetsReq;
typedef struct { CARD32 stage_flags; CARD32 offset; CARD32 size; } xavPushConstantRange;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 pipeline_layout_xid; CARD16 set_layout_count; CARD16 push_constant_range_count; CARD8 data[1]; } xavCreatePipelineLayoutReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 pipeline_xid; CARD32 pipeline_layout_xid; CARD32 shader_module_xid; CARD16 entry_point_length; CARD16 reserved; CARD8 entry_point[1]; } xavCreateComputePipelineReq;

typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 command_pool_xid; CARD32 queue_family_index; CARD32 flags; } xavCreateCommandPoolReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 command_pool_xid; } xavDestroyCommandPoolReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 command_buffer_xid; } xavResetCommandBufferReq;
typedef struct { xavReqHeader header; CARD32 command_buffer_xid; CARD32 command_pool_xid; CARD8 entries[1]; } xavCommandBufferDataReq;
typedef struct { CARD16 command_id; CARD16 entry_length; } xavCommandEntryHeader;
typedef struct { CARD16 command_id; CARD16 entry_length; CARD32 pipeline_xid; } xavCmdBindPipeline;
typedef struct { CARD16 command_id; CARD16 entry_length; CARD32 pipeline_layout_xid; CARD32 first_set; CARD16 descriptor_set_count; CARD16 reserved; CARD32 descriptor_set_xids[1]; } xavCmdBindDescriptorSets;
typedef struct { CARD16 command_id; CARD16 entry_length; CARD32 group_count_x; CARD32 group_count_y; CARD32 group_count_z; } xavCmdDispatch;
typedef struct { CARD16 command_id; CARD16 entry_length; CARD32 stage_flags; CARD32 pipeline_layout_xid; CARD32 offset; CARD32 data_length; CARD8 data[1]; } xavCmdPushConstants;
typedef struct { CARD16 command_id; CARD16 entry_length; CARD32 buffer_xid; CARD32 swapchain_xid; CARD32 image_index; CARD64 buffer_offset; CARD32 row_length; CARD32 image_height; CARD32 reserved; } xavCmdCopyBufferToSwapchainImage;

typedef struct { CARD32 semaphore_xid; CARD64 wait_value; CARD32 wait_stage_mask; } xavQueueSubmitWait;
typedef struct { CARD32 semaphore_xid; CARD64 signal_value; } xavQueueSubmitSignal;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 queue_family_index; CARD32 queue_index; CARD32 fence_xid; CARD16 command_buffer_count; CARD16 wait_semaphore_count; CARD16 signal_semaphore_count; CARD16 reserved; CARD64 submission_token; CARD8 arrays[1]; } xavQueueSubmitReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 semaphore_xid; CARD32 semaphore_type; CARD64 initial_value; } xavCreateSemaphoreReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 semaphore_xid; } xavGetSemaphoreCounterValueReq;
typedef struct { CARD32 semaphore_xid; CARD64 wait_value; } xavWaitSemaphoreEntry;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 semaphore_count; CARD32 wait_mode; CARD64 timeout_ns; xavWaitSemaphoreEntry entries[1]; } xavWaitSemaphoresReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 semaphore_xid; CARD64 signal_value; } xavSignalSemaphoreReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 semaphore_xid; } xavDestroySemaphoreReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 fence_xid; CARD32 create_flags; CARD32 reserved; } xavCreateFenceReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 fence_xid; } xavGetFenceStatusReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 fence_xid; CARD64 timeout_ns; } xavWaitForFenceReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 fence_xid; } xavResetFenceReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 fence_xid; } xavDestroyFenceReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 resource_xid; } xavDestroyResourceReq;
typedef struct { xavReqHeader header; CARD32 scope; CARD32 device_xid; CARD32 queue_family_index; CARD32 queue_index; CARD32 event_mask; CARD32 reserved; } xavSelectEventsReq;

typedef struct { xavReqHeader header; CARD32 surface_xid; CARD32 window_xid; } xavCreateSurfaceReq;
typedef struct { xavReqHeader header; CARD32 surface_xid; } xavDestroySurfaceReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 surface_xid; } xavGetSurfaceCapabilitiesReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 swapchain_xid; CARD32 surface_xid; CARD32 image_count; CARD32 format_id; CARD32 color_space_id; CARD32 width; CARD32 height; CARD32 present_mode_id; CARD32 image_usage_flags; } xavCreateSwapchainReq;
typedef struct { xavReqHeader header; CARD32 swapchain_xid; } xavDestroySwapchainReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 swapchain_xid; CARD64 timeout_ns; CARD32 semaphore_xid; CARD32 fence_xid; } xavAcquireNextImageReq;
typedef struct { xavReqHeader header; CARD32 device_xid; CARD32 queue_family_index; CARD32 queue_index; CARD32 swapchain_xid; CARD32 image_index; CARD32 wait_semaphore_count; CARD64 presentation_token; CARD32 wait_semaphore_xids[1]; } xavQueuePresentReq;

/* Replies whose fields occupy the standard 32-byte reply header's data area. */
typedef struct { CARD8 response_type; CARD8 data; CARD16 sequenceNumber; CARD32 length; CARD16 major_version; CARD16 minor_version; CARD8 reserved[20]; } xavQueryVersionReply;
typedef struct { CARD8 response_type; CARD8 data; CARD16 sequenceNumber; CARD32 length; CARD32 status; CARD32 property_type; CARD32 value_length; CARD8 reserved[12]; CARD8 value[1]; } xavGetPhysicalDevicePropertyReply;
typedef struct { CARD8 response_type; CARD8 data; CARD16 sequenceNumber; CARD32 length; CARD32 memory_type_count; CARD32 memory_heap_count; CARD32 memory_properties_flags; CARD32 reserved; CARD8 pad[8]; CARD8 records[1]; } xavGetPhysicalDeviceMemoryPropertiesReply;
typedef struct { CARD32 property_flags; CARD32 heap_index; } xavMemoryTypeInfo;
typedef struct { CARD64 size; CARD32 heap_flags; CARD32 reserved; } xavMemoryHeapInfo;
typedef struct { CARD8 response_type; CARD8 data; CARD16 sequenceNumber; CARD32 length; CARD64 size; CARD64 alignment; CARD32 memory_type_bits; CARD32 reserved; } xavGetBufferMemoryRequirementsReply;
typedef struct { CARD32 queue_flags; CARD32 queue_count; CARD32 reserved; } xavQueueFamilyInfo;
typedef struct { xavReplyHeader header; xavQueueFamilyInfo records[1]; } xavGetPhysicalDeviceQueueFamilyPropertiesReply;
typedef struct { CARD32 physical_device_xid; CARD32 device_type; CARD32 vendor_id; CARD32 device_id; CARD32 api_version; CARD32 driver_version; CARD32 compute_queue_family_count; CARD16 device_name_length; CARD16 reserved; CARD8 device_name[1]; } xavPhysicalDeviceInfo;
typedef struct { xavReplyHeader header; xavPhysicalDeviceInfo records[1]; } xavEnumeratePhysicalDevicesReply;
typedef struct { CARD8 response_type; CARD8 data; CARD16 sequenceNumber; CARD32 length; CARD64 counter_value; CARD8 reserved[16]; } xavGetSemaphoreCounterValueReply;
typedef struct { xavReplyHeader header; CARD8 data[1]; } xavReadBufferReply;
typedef struct { CARD8 response_type; CARD8 data; CARD16 sequenceNumber; CARD32 length; CARD32 status; CARD8 reserved[20]; } xavWaitSemaphoresReply;
typedef struct { CARD8 response_type; CARD8 fence_status; CARD16 sequenceNumber; CARD32 length; CARD8 reserved[24]; } xavGetFenceStatusReply;
typedef struct { CARD8 response_type; CARD8 data; CARD16 sequenceNumber; CARD32 length; CARD32 status; CARD8 reserved[20]; } xavWaitForFenceReply;

/* XGE events. */
typedef struct { xavXGEHeader header; CARD32 device_xid; CARD32 error_code; CARD32 origin; CARD64 submission_token; CARD32 resource_xid; CARD32 reserved; } xavAsyncErrorEvent;
typedef struct { xavXGEHeader header; CARD32 device_xid; CARD32 reason_code; CARD64 device_generation; } xavDeviceLostEvent;
typedef struct { xavXGEHeader header; CARD32 physical_device_xid; CARD32 new_status; CARD32 reason_code; CARD32 reserved; } xavDeviceStatusChangedEvent;
typedef struct { xavXGEHeader header; CARD32 device_xid; CARD32 heap_index; CARD32 pressure_level; CARD32 reserved; } xavDeviceMemoryPressureEvent;
typedef struct { xavXGEHeader header; CARD32 device_xid; CARD32 idle_reason; CARD64 device_generation; } xavDeviceIdleEvent;
typedef struct { xavXGEHeader header; CARD32 device_xid; CARD32 queue_family_index; CARD32 queue_index; CARD32 reserved; CARD64 submission_token; } xavSubmissionCompleteEvent;
typedef struct { xavXGEHeader header; CARD32 device_xid; CARD32 queue_family_index; CARD32 queue_index; CARD32 failure_class; CARD64 submission_token; } xavSubmissionFailedEvent;
typedef struct { xavXGEHeader header; CARD32 device_xid; CARD32 queue_family_index; CARD32 queue_index; CARD32 swapchain_xid; CARD32 image_index; CARD32 reserved; CARD64 presentation_token; } xavImageReleasedEvent;
typedef struct { xavXGEHeader header; CARD32 device_xid; CARD32 queue_family_index; CARD32 queue_index; CARD32 swapchain_xid; CARD32 image_index; CARD32 status; CARD64 presentation_token; } xavPresentationCompleteEvent;

/* Variable-length presentation capability reply starts at offset 64. */
typedef struct {
    CARD8 response_type; CARD8 data; CARD16 sequenceNumber; CARD32 length;
    CARD32 status; CARD32 present_supported; CARD32 current_width; CARD32 current_height;
    CARD32 min_image_count; CARD32 max_image_count; CARD32 min_width; CARD32 min_height;
    CARD32 max_width; CARD32 max_height; CARD32 format_pair_count; CARD32 present_mode_count;
    CARD32 image_usage_flags; CARD32 queue_family_count; CARD8 records[1];
} xavGetSurfaceCapabilitiesReply;
typedef struct { CARD32 format_id; CARD32 color_space_id; } xavSurfaceFormatPair;
typedef struct { CARD32 queue_family_index; CARD32 present_supported; } xavSurfaceQueueFamilyInfo;
typedef struct { xavReplyHeader header; CARD32 status; CARD32 actual_image_count; } xavCreateSwapchainReply;
typedef struct { xavReplyHeader header; CARD32 status; CARD32 image_index; CARD32 reserved; } xavAcquireNextImageReply;
typedef struct { xavReplyHeader header; CARD32 status; } xavQueuePresentReply;

#pragma pack(pop)

#define sz_xavReqHeader 8
#define sz_xavReplyHeader 32
#define sz_xavXGEHeader 32
#define sz_xavQueryVersionReq 8
#define sz_xavEnumeratePhysicalDevicesReq 8
#define sz_xavGetPhysicalDevicePropertyReq 16
#define sz_xavGetPhysicalDeviceQueueFamilyPropertiesReq 12
#define sz_xavCreateDeviceReq 24
#define sz_xavCreateDefaultDeviceReq 16
#define sz_xavDestroyDeviceReq 12
#define sz_xavGetPhysicalDeviceMemoryPropertiesReq 12
#define sz_xavCreateBufferReq 32
#define sz_xavGetBufferMemoryRequirementsReq 16
#define sz_xavAllocateMemoryReq 32
#define sz_xavFreeMemoryReq 16
#define sz_xavBindBufferMemoryReq 28
#define sz_xavWriteBufferReq 28
#define sz_xavReadBufferReq 28
#define sz_xavShaderModuleDataReq 20
#define sz_xavCreateDescriptorSetLayoutReq 24
#define sz_xavCreateDescriptorPoolReq 24
#define sz_xavDestroyDescriptorPoolReq 16
#define sz_xavAllocateDescriptorSetsReq 20
#define sz_xavUpdateDescriptorSetsReq 20
#define sz_xavFreeDescriptorSetsReq 20
#define sz_xavCreatePipelineLayoutReq 20
#define sz_xavCreateComputePipelineReq 28
#define sz_xavCreateCommandPoolReq 24
#define sz_xavDestroyCommandPoolReq 16
#define sz_xavResetCommandBufferReq 16
#define sz_xavCommandBufferDataReq 16
#define sz_xavQueueSubmitReq 40
#define sz_xavCreateSemaphoreReq 28
#define sz_xavGetSemaphoreCounterValueReq 16
#define sz_xavWaitSemaphoresReq 28
#define sz_xavSignalSemaphoreReq 24
#define sz_xavDestroySemaphoreReq 16
#define sz_xavCreateFenceReq 24
#define sz_xavGetFenceStatusReq 16
#define sz_xavWaitForFenceReq 24
#define sz_xavResetFenceReq 16
#define sz_xavDestroyFenceReq 16
#define sz_xavDestroyResourceReq 16
#define sz_xavSelectEventsReq 32
#define sz_xavCreateSurfaceReq 16
#define sz_xavDestroySurfaceReq 12
#define sz_xavGetSurfaceCapabilitiesReq 16
#define sz_xavCreateSwapchainReq 48
#define sz_xavDestroySwapchainReq 12
#define sz_xavAcquireNextImageReq 32
#define sz_xavQueuePresentReq 40
#define sz_xavErrorPacket 32
#define sz_xavQueryVersionReply 32
#define sz_xavEnumeratePhysicalDevicesReply 32
#define sz_xavGetPhysicalDevicePropertyReply 32
#define sz_xavGetPhysicalDeviceQueueFamilyPropertiesReply 32
#define sz_xavGetPhysicalDeviceMemoryPropertiesReply 32
#define sz_xavGetBufferMemoryRequirementsReply 32
#define sz_xavReadBufferReply 32
#define sz_xavGetSemaphoreCounterValueReply 32
#define sz_xavWaitSemaphoresReply 32
#define sz_xavGetFenceStatusReply 32
#define sz_xavWaitForFenceReply 32
#define sz_xavGetSurfaceCapabilitiesReply 64
#define sz_xavCreateSwapchainReply 40
#define sz_xavAcquireNextImageReply 44
#define sz_xavQueuePresentReply 36
#define sz_xavPhysicalDeviceInfo 32
#define sz_xavQueueFamilyInfo 12
#define sz_xavMemoryTypeInfo 8
#define sz_xavMemoryHeapInfo 16
#define sz_xavDescriptorSetLayoutBinding 16
#define sz_xavDescriptorPoolSize 8
#define sz_xavDescriptorSetAllocation 8
#define sz_xavDescriptorWrite 36
#define sz_xavPushConstantRange 12
#define sz_xavQueueSubmitWait 16
#define sz_xavQueueSubmitSignal 12
#define sz_xavWaitSemaphoreEntry 12
#define sz_xavAsyncErrorEvent 60
#define sz_xavDeviceLostEvent 48
#define sz_xavDeviceStatusChangedEvent 48
#define sz_xavDeviceMemoryPressureEvent 48
#define sz_xavDeviceIdleEvent 48
#define sz_xavSubmissionCompleteEvent 56
#define sz_xavSubmissionFailedEvent 56
#define sz_xavImageReleasedEvent 64
#define sz_xavPresentationCompleteEvent 64

#if defined(__cplusplus)
# define XAV_STATIC_ASSERT(c, m) static_assert((c), m)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
# define XAV_STATIC_ASSERT(c, m) _Static_assert((c), m)
#else
# define XAV_STATIC_ASSERT(c, m) typedef char xav_static_assert_##__LINE__[(c) ? 1 : -1]
#endif
XAV_STATIC_ASSERT(sizeof(xavReqHeader) == 8, "request header size");
XAV_STATIC_ASSERT(sizeof(xavReplyHeader) == 32, "reply header size");
XAV_STATIC_ASSERT(sizeof(xavXGEHeader) == 32, "XGE header size");
XAV_STATIC_ASSERT(sizeof(xavQueryVersionReply) == 32, "QueryVersion reply size");
XAV_STATIC_ASSERT(offsetof(xavGetPhysicalDevicePropertyReply, value) == 32, "property reply payload offset");
XAV_STATIC_ASSERT(sizeof(xavGetBufferMemoryRequirementsReply) == 32, "memory requirements reply size");
XAV_STATIC_ASSERT(offsetof(xavAsyncErrorEvent, submission_token) == 44, "ASYNC_ERROR token offset");
XAV_STATIC_ASSERT(sizeof(xavAsyncErrorEvent) == 60, "ASYNC_ERROR event size");
XAV_STATIC_ASSERT(offsetof(xavImageReleasedEvent, presentation_token) == 56, "IMAGE_RELEASED token offset");
XAV_STATIC_ASSERT(sizeof(xavImageReleasedEvent) == 64, "IMAGE_RELEASED event size");
XAV_STATIC_ASSERT(offsetof(xavGetSurfaceCapabilitiesReply, records) == 64, "capabilities variable payload offset");
XAV_STATIC_ASSERT(offsetof(xavGetPhysicalDeviceMemoryPropertiesReply, records) == 32, "memory properties payload offset");
XAV_STATIC_ASSERT(sizeof(xavErrorPacket) == 32, "X11 error packet size");

/* FLOAT32 is an IEEE-754 binary32 wire value, not an arbitrary C float. */
XAV_STATIC_ASSERT(sizeof(xavFloat32) == 4, "FLOAT32 size");
XAV_STATIC_ASSERT(FLT_RADIX == 2, "FLOAT32 radix");
XAV_STATIC_ASSERT(FLT_MANT_DIG == 24, "FLOAT32 precision");
XAV_STATIC_ASSERT(FLT_MAX_EXP == 128, "FLOAT32 maximum exponent");
XAV_STATIC_ASSERT(FLT_MIN_EXP == -125, "FLOAT32 minimum exponent");

/* Fixed request structures must match their normative total wire lengths. */
#define XAV_ASSERT_REQ_SIZE(type) \
    XAV_STATIC_ASSERT(sizeof(type) == sz_##type, #type " wire size")
#define XAV_ASSERT_REQ_PREFIX(type, field) \
    XAV_STATIC_ASSERT(offsetof(type, field) == sz_##type, #type " fixed prefix")

XAV_ASSERT_REQ_SIZE(xavQueryVersionReq);
XAV_ASSERT_REQ_SIZE(xavEnumeratePhysicalDevicesReq);
XAV_ASSERT_REQ_SIZE(xavGetPhysicalDevicePropertyReq);
XAV_ASSERT_REQ_SIZE(xavGetPhysicalDeviceQueueFamilyPropertiesReq);
XAV_ASSERT_REQ_PREFIX(xavCreateDeviceReq, requests);
XAV_ASSERT_REQ_PREFIX(xavCreateDefaultDeviceReq, priorities);
XAV_ASSERT_REQ_SIZE(xavDestroyDeviceReq);
XAV_ASSERT_REQ_SIZE(xavGetPhysicalDeviceMemoryPropertiesReq);
XAV_ASSERT_REQ_SIZE(xavCreateBufferReq);
XAV_ASSERT_REQ_SIZE(xavGetBufferMemoryRequirementsReq);
XAV_ASSERT_REQ_SIZE(xavAllocateMemoryReq);
XAV_ASSERT_REQ_SIZE(xavFreeMemoryReq);
XAV_ASSERT_REQ_SIZE(xavBindBufferMemoryReq);
XAV_ASSERT_REQ_PREFIX(xavWriteBufferReq, data);
XAV_ASSERT_REQ_SIZE(xavReadBufferReq);
XAV_ASSERT_REQ_PREFIX(xavShaderModuleDataReq, data);
XAV_ASSERT_REQ_PREFIX(xavCreateDescriptorSetLayoutReq, bindings);
XAV_ASSERT_REQ_PREFIX(xavCreateDescriptorPoolReq, pool_sizes);
XAV_ASSERT_REQ_SIZE(xavDestroyDescriptorPoolReq);
XAV_ASSERT_REQ_PREFIX(xavAllocateDescriptorSetsReq, allocations);
XAV_ASSERT_REQ_PREFIX(xavUpdateDescriptorSetsReq, writes);
XAV_ASSERT_REQ_PREFIX(xavFreeDescriptorSetsReq, descriptor_set_xids);
XAV_ASSERT_REQ_PREFIX(xavCreatePipelineLayoutReq, data);
XAV_ASSERT_REQ_PREFIX(xavCreateComputePipelineReq, entry_point);
XAV_ASSERT_REQ_SIZE(xavCreateCommandPoolReq);
XAV_ASSERT_REQ_SIZE(xavDestroyCommandPoolReq);
XAV_ASSERT_REQ_SIZE(xavResetCommandBufferReq);
XAV_ASSERT_REQ_PREFIX(xavCommandBufferDataReq, entries);
XAV_ASSERT_REQ_PREFIX(xavQueueSubmitReq, arrays);
XAV_ASSERT_REQ_SIZE(xavCreateSemaphoreReq);
XAV_ASSERT_REQ_SIZE(xavGetSemaphoreCounterValueReq);
XAV_ASSERT_REQ_PREFIX(xavWaitSemaphoresReq, entries);
XAV_ASSERT_REQ_SIZE(xavSignalSemaphoreReq);
XAV_ASSERT_REQ_SIZE(xavDestroySemaphoreReq);
XAV_ASSERT_REQ_SIZE(xavCreateFenceReq);
XAV_ASSERT_REQ_SIZE(xavGetFenceStatusReq);
XAV_ASSERT_REQ_SIZE(xavWaitForFenceReq);
XAV_ASSERT_REQ_SIZE(xavResetFenceReq);
XAV_ASSERT_REQ_SIZE(xavDestroyFenceReq);
XAV_ASSERT_REQ_SIZE(xavDestroyResourceReq);
XAV_ASSERT_REQ_SIZE(xavSelectEventsReq);
XAV_ASSERT_REQ_SIZE(xavCreateSurfaceReq);
XAV_ASSERT_REQ_SIZE(xavDestroySurfaceReq);
XAV_ASSERT_REQ_SIZE(xavGetSurfaceCapabilitiesReq);
XAV_ASSERT_REQ_SIZE(xavCreateSwapchainReq);
XAV_ASSERT_REQ_SIZE(xavDestroySwapchainReq);
XAV_ASSERT_REQ_SIZE(xavAcquireNextImageReq);
XAV_ASSERT_REQ_PREFIX(xavQueuePresentReq, wait_semaphore_xids);

/* Nested variable records and reply payload boundaries. */
XAV_STATIC_ASSERT(offsetof(xavQueueFamilyRequest, queue_priorities) == 8, "queue priorities offset");
XAV_STATIC_ASSERT(offsetof(xavDeviceExtensionRequest, name) == 4, "device extension name offset");
XAV_STATIC_ASSERT(offsetof(xavCmdBindDescriptorSets, descriptor_set_xids) == 16, "descriptor-set command tail offset");
XAV_STATIC_ASSERT(offsetof(xavCmdPushConstants, data) == 20, "push-constant command tail offset");
XAV_STATIC_ASSERT(offsetof(xavPhysicalDeviceInfo, device_name) == 32, "physical-device name offset");
XAV_STATIC_ASSERT(offsetof(xavEnumeratePhysicalDevicesReply, records) == 32, "physical-device reply records offset");
XAV_STATIC_ASSERT(offsetof(xavGetPhysicalDeviceQueueFamilyPropertiesReply, records) == 32, "queue-family reply records offset");
XAV_STATIC_ASSERT(offsetof(xavGetPhysicalDeviceMemoryPropertiesReply, records) == 32, "memory-property reply records offset");
XAV_STATIC_ASSERT(offsetof(xavReadBufferReply, data) == 32, "ReadBuffer payload offset");
XAV_STATIC_ASSERT(offsetof(xavGetSurfaceCapabilitiesReply, records) == 64, "surface-capability payload offset");

/* Every fixed-size reply and every XGE event must have its specified size. */
XAV_STATIC_ASSERT(sizeof(xavQueryVersionReply) == sz_xavQueryVersionReply, "QueryVersion reply size");
XAV_STATIC_ASSERT(sizeof(xavGetBufferMemoryRequirementsReply) == sz_xavGetBufferMemoryRequirementsReply, "buffer requirements reply size");
XAV_STATIC_ASSERT(sizeof(xavGetSemaphoreCounterValueReply) == sz_xavGetSemaphoreCounterValueReply, "semaphore counter reply size");
XAV_STATIC_ASSERT(sizeof(xavWaitSemaphoresReply) == sz_xavWaitSemaphoresReply, "WaitSemaphores reply size");
XAV_STATIC_ASSERT(sizeof(xavGetFenceStatusReply) == sz_xavGetFenceStatusReply, "GetFenceStatus reply size");
XAV_STATIC_ASSERT(sizeof(xavWaitForFenceReply) == sz_xavWaitForFenceReply, "WaitForFence reply size");
XAV_STATIC_ASSERT(sizeof(xavCreateSwapchainReply) == sz_xavCreateSwapchainReply, "CreateSwapchain reply size");
XAV_STATIC_ASSERT(sizeof(xavAcquireNextImageReply) == sz_xavAcquireNextImageReply, "AcquireNextImage reply size");
XAV_STATIC_ASSERT(sizeof(xavQueuePresentReply) == sz_xavQueuePresentReply, "QueuePresent reply size");
XAV_STATIC_ASSERT(sizeof(xavAsyncErrorEvent) == sz_xavAsyncErrorEvent, "ASYNC_ERROR event size");
XAV_STATIC_ASSERT(sizeof(xavDeviceLostEvent) == sz_xavDeviceLostEvent, "DEVICE_LOST event size");
XAV_STATIC_ASSERT(sizeof(xavDeviceStatusChangedEvent) == sz_xavDeviceStatusChangedEvent, "DEVICE_STATUS_CHANGED event size");
XAV_STATIC_ASSERT(sizeof(xavDeviceMemoryPressureEvent) == sz_xavDeviceMemoryPressureEvent, "DEVICE_MEMORY_PRESSURE event size");
XAV_STATIC_ASSERT(sizeof(xavDeviceIdleEvent) == sz_xavDeviceIdleEvent, "DEVICE_IDLE event size");
XAV_STATIC_ASSERT(sizeof(xavSubmissionCompleteEvent) == sz_xavSubmissionCompleteEvent, "SUBMISSION_COMPLETE event size");
XAV_STATIC_ASSERT(sizeof(xavSubmissionFailedEvent) == sz_xavSubmissionFailedEvent, "SUBMISSION_FAILED event size");
XAV_STATIC_ASSERT(sizeof(xavImageReleasedEvent) == sz_xavImageReleasedEvent, "IMAGE_RELEASED event size");
XAV_STATIC_ASSERT(sizeof(xavPresentationCompleteEvent) == sz_xavPresentationCompleteEvent, "PRESENTATION_COMPLETE event size");

#undef XAV_ASSERT_REQ_SIZE
#undef XAV_ASSERT_REQ_PREFIX
#undef XAV_STATIC_ASSERT

#endif /* __XLIBRE_XAVERIUS_PROTO_H */
