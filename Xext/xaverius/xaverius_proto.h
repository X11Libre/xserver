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

#define XAVERIUS_REQ_QUERY_VERSION 0
#define XAVERIUS_REQ_ENUMERATE_PHYSICAL_DEVICES 1
#define XAVERIUS_REQ_GET_PHYSICAL_DEVICE_PROPERTY 2
#define XAVERIUS_REQ_GET_PHYSICAL_DEVICE_QUEUE_FAMILY_PROPERTIES 3
#define XAVERIUS_REQ_CREATE_DEVICE 4
#define XAVERIUS_REQ_CREATE_DEFAULT_DEVICE 5
#define XAVERIUS_REQ_DESTROY_DEVICE 6
#define XAVERIUS_REQ_GET_PHYSICAL_DEVICE_MEMORY_PROPERTIES 7
#define XAVERIUS_REQ_CREATE_BUFFER 8
#define XAVERIUS_REQ_GET_BUFFER_MEMORY_REQUIREMENTS 9
#define XAVERIUS_REQ_ALLOCATE_MEMORY 10
#define XAVERIUS_REQ_FREE_MEMORY 11
#define XAVERIUS_REQ_BIND_BUFFER_MEMORY 12
#define XAVERIUS_REQ_WRITE_BUFFER 13
#define XAVERIUS_REQ_READ_BUFFER 14
#define XAVERIUS_REQ_SHADER_MODULE_DATA 15
#define XAVERIUS_REQ_CREATE_DESCRIPTOR_SET_LAYOUT 16
#define XAVERIUS_REQ_CREATE_DESCRIPTOR_POOL 17
#define XAVERIUS_REQ_DESTROY_DESCRIPTOR_POOL 18
#define XAVERIUS_REQ_ALLOCATE_DESCRIPTOR_SETS 19
#define XAVERIUS_REQ_UPDATE_DESCRIPTOR_SETS 20
#define XAVERIUS_REQ_FREE_DESCRIPTOR_SETS 21
#define XAVERIUS_REQ_CREATE_PIPELINE_LAYOUT 22
#define XAVERIUS_REQ_CREATE_COMPUTE_PIPELINE 23
#define XAVERIUS_REQ_CREATE_COMMAND_POOL 24
#define XAVERIUS_REQ_DESTROY_COMMAND_POOL 25
#define XAVERIUS_REQ_RESET_COMMAND_BUFFER 26
#define XAVERIUS_REQ_COMMAND_BUFFER_DATA 27
#define XAVERIUS_REQ_QUEUE_SUBMIT 28
#define XAVERIUS_REQ_CREATE_SEMAPHORE 29
#define XAVERIUS_REQ_GET_SEMAPHORE_COUNTER_VALUE 30
#define XAVERIUS_REQ_WAIT_SEMAPHORES 31
#define XAVERIUS_REQ_SIGNAL_SEMAPHORE 32
#define XAVERIUS_REQ_DESTROY_SEMAPHORE 33
#define XAVERIUS_REQ_CREATE_FENCE 34
#define XAVERIUS_REQ_GET_FENCE_STATUS 35
#define XAVERIUS_REQ_WAIT_FOR_FENCE 36
#define XAVERIUS_REQ_RESET_FENCE 37
#define XAVERIUS_REQ_DESTROY_FENCE 38
#define XAVERIUS_REQ_DESTROY_RESOURCE 39
#define XAVERIUS_REQ_SELECT_EVENTS 40
#define XAVERIUS_REQ_CREATE_SURFACE 41
#define XAVERIUS_REQ_DESTROY_SURFACE 42
#define XAVERIUS_REQ_GET_SURFACE_CAPABILITIES 43
#define XAVERIUS_REQ_CREATE_SWAPCHAIN 44
#define XAVERIUS_REQ_DESTROY_SWAPCHAIN 45
#define XAVERIUS_REQ_ACQUIRE_NEXT_IMAGE 46
#define XAVERIUS_REQ_QUEUE_PRESENT 47
#define XAVERIUS_REQ_LAST_COMPUTE 40
#define XAVERIUS_REQ_FIRST_PRESENTATION 41
#define XAVERIUS_REQ_LAST_PRESENTATION 47
#define XAVERIUS_ERROR_DEVICE_UNAVAILABLE 0
#define XAVERIUS_ERROR_DEVICE_LOST 1
#define XAVERIUS_ERROR_INVALID_SYNCHRONIZATION_STATE 2
#define XAVERIUS_ERROR_EXECUTION_FAILED 3
#define XAVERIUS_ERROR_TRANSFER_FAILED 4
#define XAVERIUS_ERROR_ALLOCATION_FAILED 5
#define XAVERIUS_ERROR_DEPENDENCY_FAILED 6
#define XAVERIUS_ERROR_INTERNAL_SERVER_ERROR 7
#define XAVERIUS_ERROR_DEVICE_DESTROYED 8
#define XAVERIUS_EVENT_ASYNC_ERROR 0
#define XAVERIUS_EVENT_DEVICE_LOST 1
#define XAVERIUS_EVENT_DEVICE_STATUS_CHANGED 2
#define XAVERIUS_EVENT_DEVICE_MEMORY_PRESSURE 3
#define XAVERIUS_EVENT_DEVICE_IDLE 4
#define XAVERIUS_EVENT_SUBMISSION_COMPLETE 5
#define XAVERIUS_EVENT_SUBMISSION_FAILED 6
#define XAVERIUS_EVENT_IMAGE_RELEASED 7
#define XAVERIUS_EVENT_PRESENTATION_COMPLETE 8
#define XAVERIUS_CMD_BIND_PIPELINE 1
#define XAVERIUS_CMD_BIND_DESCRIPTOR_SETS 2
#define XAVERIUS_CMD_DISPATCH 3
#define XAVERIUS_CMD_PUSH_CONSTANTS 4
#define XAVERIUS_CMD_COPY_BUFFER_TO_SWAPCHAIN_IMAGE 5
#define XAVERIUS_FLAG_CREATE 0x01
#define XAVERIUS_FLAG_FINAL 0x02
#define XAVERIUS_FLAG_ABORT 0x04
#define XAVERIUS_BUFFER_USAGE_STORAGE_BUFFER 0x00000001U
#define XAVERIUS_BUFFER_USAGE_TRANSFER_SRC 0x00000002U
#define XAVERIUS_BUFFER_USAGE_TRANSFER_DST 0x00000004U
#define XAVERIUS_BUFFER_USAGE_UNIFORM_BUFFER 0x00000008U
#define XAVERIUS_BUFFER_USAGE_INDEX_BUFFER 0x00000010U
#define XAVERIUS_BUFFER_USAGE_VERTEX_BUFFER 0x00000020U
#define XAVERIUS_BUFFER_USAGE_INDIRECT_BUFFER 0x00000040U
#define XAVERIUS_QUEUE_GRAPHICS 0x00000001U
#define XAVERIUS_QUEUE_COMPUTE 0x00000002U
#define XAVERIUS_QUEUE_TRANSFER 0x00000004U
#define XAVERIUS_QUEUE_SPARSE_BINDING 0x00000008U
#define XAVERIUS_QUEUE_PROTECTED 0x00000010U
#define XAVERIUS_MEMORY_DEVICE_LOCAL 0x00000001U
#define XAVERIUS_MEMORY_HOST_VISIBLE 0x00000002U
#define XAVERIUS_MEMORY_HOST_COHERENT 0x00000004U
#define XAVERIUS_MEMORY_HOST_CACHED 0x00000008U
#define XAVERIUS_MEMORY_LAZILY_ALLOCATED 0x00000010U
#define XAVERIUS_MEMORY_PROTECTED 0x00000020U
#define XAVERIUS_MEMORY_HEAP_DEVICE_LOCAL 0x00000001U
#define XAVERIUS_STAGE_COMPUTE 0x00000001U
#define XAVERIUS_WAIT_STAGE_DRAW_INDIRECT 0x00000001U
#define XAVERIUS_WAIT_STAGE_COMPUTE_SHADER 0x00000002U
#define XAVERIUS_WAIT_STAGE_TRANSFER 0x00000004U
#define XAVERIUS_WAIT_STAGE_ALL_COMMANDS 0x00000008U
#define XAVERIUS_DESCRIPTOR_TYPE_UNSPECIFIED 0
#define XAVERIUS_DESCRIPTOR_TYPE_STORAGE_BUFFER 1
#define XAVERIUS_DEVICE_TYPE_OTHER 0
#define XAVERIUS_DEVICE_TYPE_INTEGRATED_GPU 1
#define XAVERIUS_DEVICE_TYPE_DISCRETE_GPU 2
#define XAVERIUS_DEVICE_TYPE_VIRTUAL_GPU 3
#define XAVERIUS_DEVICE_TYPE_CPU 4
#define XAVERIUS_PROPERTY_NONE 0
#define XAVERIUS_PROPERTY_CARD32 1
#define XAVERIUS_PROPERTY_CARD64 2
#define XAVERIUS_PROPERTY_ATOM 3
#define XAVERIUS_PROPERTY_STRING8 4
#define XAVERIUS_PROPERTY_BYTE_ARRAY 5
#define XAVERIUS_PROPERTY_CARD32_ARRAY 6
#define XAVERIUS_PROPERTY_CARD64_ARRAY 7
#define XAVERIUS_PROPERTY_STATUS_SUCCESS 0
#define XAVERIUS_PROPERTY_STATUS_NOT_SUPPORTED 1
#define XAVERIUS_PROPERTY_STATUS_NOT_AVAILABLE 2
#define XAVERIUS_SEMAPHORE_BINARY 0
#define XAVERIUS_SEMAPHORE_TIMELINE 1
#define XAVERIUS_WAIT_ANY 0
#define XAVERIUS_WAIT_ALL 1
#define XAVERIUS_FENCE_UNSIGNALED 0
#define XAVERIUS_FENCE_PENDING 1
#define XAVERIUS_FENCE_SIGNALED 2
#define XAVERIUS_FENCE_FAILED 3
#define XAVERIUS_WAIT_STATUS_SUCCESS 0
#define XAVERIUS_WAIT_STATUS_TIMEOUT 1
#define XAVERIUS_FENCE_WAIT_EXECUTION_FAILED 2
#define XAVERIUS_FENCE_WAIT_DEVICE_LOST 3
#define XAVERIUS_FENCE_WAIT_DEVICE_DESTROYED 4
#define XAVERIUS_COMMAND_POOL_TRANSIENT 0x00000001U
#define XAVERIUS_COMMAND_POOL_RESET_COMMAND_BUFFER 0x00000002U
#define XAVERIUS_COMMAND_BUFFER_INITIAL 0
#define XAVERIUS_COMMAND_BUFFER_RECORDING 1
#define XAVERIUS_COMMAND_BUFFER_EXECUTABLE 2
#define XAVERIUS_COMMAND_BUFFER_PENDING 3
#define XAVERIUS_COMMAND_BUFFER_INVALID 4
#define XAVERIUS_SCOPE_CONNECTION 0
#define XAVERIUS_SCOPE_DEVICE 1
#define XAVERIUS_SCOPE_QUEUE 2
#define XAVERIUS_EVENT_MASK_CONNECTION_DEVICE_STATUS 0x00000001U
#define XAVERIUS_EVENT_MASK_DEVICE_MEMORY_PRESSURE 0x00000001U
#define XAVERIUS_EVENT_MASK_DEVICE_IDLE 0x00000002U
#define XAVERIUS_EVENT_MASK_QUEUE_SUBMISSION_COMPLETE 0x00000001U
#define XAVERIUS_EVENT_MASK_QUEUE_SUBMISSION_FAILED 0x00000002U
#define XAVERIUS_EVENT_MASK_QUEUE_IMAGE_RELEASED 0x00000004U
#define XAVERIUS_EVENT_MASK_QUEUE_PRESENTATION_COMPLETE 0x00000008U
#define XAVERIUS_ASYNC_ORIGIN_UNSPECIFIED 0
#define XAVERIUS_ASYNC_ORIGIN_QUEUE_SUBMIT 1
#define XAVERIUS_ASYNC_ORIGIN_WRITE_BUFFER 2
#define XAVERIUS_ASYNC_ORIGIN_READ_BUFFER 3
#define XAVERIUS_ASYNC_ORIGIN_SEMAPHORE 4
#define XAVERIUS_ASYNC_ORIGIN_FENCE 5
#define XAVERIUS_ASYNC_ORIGIN_DEVICE 6
#define XAVERIUS_SUBMISSION_FAILURE_EXECUTION_FAILED 0
#define XAVERIUS_SUBMISSION_FAILURE_DEVICE_LOST 1
#define XAVERIUS_MEMORY_PRESSURE_NORMAL 0
#define XAVERIUS_MEMORY_PRESSURE_LOW 1
#define XAVERIUS_MEMORY_PRESSURE_MEDIUM 2
#define XAVERIUS_MEMORY_PRESSURE_HIGH 3
#define XAVERIUS_IDLE_REASON_UNSPECIFIED 0
#define XAVERIUS_IDLE_REASON_NO_PENDING_WORK 1
#define XAVERIUS_IDLE_REASON_WORK_COMPLETED 2
#define XAVERIUS_REASON_UNSPECIFIED 0
#define XAVERIUS_REASON_HOTPLUG_REMOVAL 1
#define XAVERIUS_REASON_DRIVER_FAILURE 2
#define XAVERIUS_REASON_VULKAN_DEVICE_LOST 3
#define XAVERIUS_REASON_SERVER_POLICY 4
#define XAVERIUS_REASON_SERVER_SHUTDOWN 5
#define XAVERIUS_PRESENT_STATUS_SUCCESS 0
#define XAVERIUS_PRESENT_STATUS_OUT_OF_DATE 1
#define XAVERIUS_PRESENT_STATUS_SURFACE_LOST 2
#define XAVERIUS_PRESENT_STATUS_DEVICE_LOST 3
#define XAVERIUS_PRESENT_STATUS_PRESENTATION_FAILED 4
#define XAVERIUS_SURFACE_STATUS_SUCCESS 0
#define XAVERIUS_SURFACE_STATUS_TEMPORARILY_NOT_PRESENTABLE 1
#define XAVERIUS_SURFACE_STATUS_LOST 2
#define XAVERIUS_SURFACE_STATUS_DEVICE_LOST 3
#define XAVERIUS_SWAPCHAIN_STATUS_SUCCESS 0
#define XAVERIUS_SWAPCHAIN_STATUS_OUT_OF_DATE 1
#define XAVERIUS_SWAPCHAIN_STATUS_SURFACE_LOST 2
#define XAVERIUS_SWAPCHAIN_STATUS_DEVICE_LOST 3
#define XAVERIUS_SWAPCHAIN_STATUS_UNSUPPORTED_CONFIGURATION 4
#define XAVERIUS_SWAPCHAIN_STATUS_ALLOCATION_FAILED 5
#define XAVERIUS_QUEUE_PRESENT_STATUS_SUCCESS 0
#define XAVERIUS_QUEUE_PRESENT_STATUS_OUT_OF_DATE 1
#define XAVERIUS_QUEUE_PRESENT_STATUS_SURFACE_LOST 2
#define XAVERIUS_QUEUE_PRESENT_STATUS_DEVICE_LOST 3
#define XAVERIUS_FORMAT_RGBA8_UNORM 1
#define XAVERIUS_FORMAT_BGRA8_UNORM 2
#define XAVERIUS_COLOR_SPACE_SRGB_NONLINEAR 1
#define XAVERIUS_PRESENT_MODE_FIFO 1
#define XAVERIUS_ACQUIRE_STATUS_SUCCESS 0
#define XAVERIUS_ACQUIRE_STATUS_TIMEOUT 1
#define XAVERIUS_ACQUIRE_STATUS_NOT_READY 2
#define XAVERIUS_ACQUIRE_STATUS_OUT_OF_DATE 3
#define XAVERIUS_ACQUIRE_STATUS_SURFACE_LOST 4
#define XAVERIUS_ACQUIRE_STATUS_DEVICE_LOST 5
#define XAVERIUS_ACQUIRE_STATUS_DEVICE_DESTROYED 6
#define XAVERIUS_PROPERTY_NAME_DEVICE_UUID "XAverius.deviceUUID"
#define XAVERIUS_PROPERTY_NAME_DRIVER_UUID "XAverius.driverUUID"
#define XAVERIUS_PROPERTY_NAME_MAX_WORK_GROUP_INVOCATIONS "XAverius.compute.maxWorkGroupInvocations"
#define XAVERIUS_PROPERTY_NAME_MAX_WORK_GROUP_SIZE "XAverius.compute.maxWorkGroupSize"
#define XAVERIUS_PROPERTY_NAME_TIMELINE_SEMAPHORE "XAverius.timelineSemaphore"
#define XAVERIUS_IMAGE_USAGE_TRANSFER_DST 0x00000001U
#define XAVERIUS_MAX_REQUEST_BYTES 262140U
#define XAVERIUS_REQUEST_HEADER_BYTES 8U
#define XAVERIUS_REPLY_HEADER_BYTES 32U
#define XAVERIUS_XGE_HEADER_BYTES 32U
#define XAVERIUS_XGE_GENERIC_EVENT 35U
#define XAVERIUS_MAX_REQUEST_WORDS 65535U

/* Wire XIDs/Atoms are always CARD32; Xlib XID/Atom may be wider on LP64. */
typedef float XaveriusFloat32;
#pragma pack(push, 4)

typedef struct { CARD8 reqType; CARD8 data; CARD16 length; CARD8 subopcode; CARD8 flags; CARD16 reserved; } XaveriusReqHeader;
typedef struct { CARD8 response_type; CARD8 data; CARD16 sequenceNumber; CARD32 length; CARD32 pad[6]; } XaveriusReplyHeader;
typedef struct {
    CARD8 type; CARD8 errorCode; CARD16 sequenceNumber; CARD32 resourceID;
    CARD16 minorCode; CARD8 majorCode; CARD8 pad0; CARD32 pad[5];
} XaveriusErrorPacket;
typedef struct { CARD8 response_type; CARD8 extension; CARD16 sequenceNumber; CARD32 length; CARD16 evtype; CARD8 pad[22]; } XaveriusXGEHeader;

typedef struct { XaveriusReqHeader header; } XaveriusQueryVersionReq;
typedef struct { XaveriusReqHeader header; } XaveriusEnumeratePhysicalDevicesReq;
typedef struct { XaveriusReqHeader header; CARD32 physical_device_xid; CARD32 property_atom; } XaveriusGetPhysicalDevicePropertyReq;
typedef struct { XaveriusReqHeader header; CARD32 physical_device_xid; } XaveriusGetPhysicalDeviceQueueFamilyPropertiesReq;
typedef struct { CARD32 queue_family_index; CARD16 queue_count; CARD16 reserved; XaveriusFloat32 queue_priorities[1]; } XaveriusQueueFamilyRequest;
typedef struct { CARD32 feature_id; CARD32 enabled; } XaveriusDeviceFeatureRequest;
typedef struct { CARD16 name_length; CARD16 reserved; CARD8 name[1]; } XaveriusDeviceExtensionRequest;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 physical_device_xid; CARD16 queue_family_request_count; CARD16 feature_request_count; CARD16 device_extension_count; CARD16 reserved; CARD8 requests[1]; } XaveriusCreateDeviceReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD16 queue_count; CARD16 reserved; XaveriusFloat32 priorities[1]; } XaveriusCreateDefaultDeviceReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; } XaveriusDestroyDeviceReq;
typedef struct { XaveriusReqHeader header; CARD32 physical_device_xid; } XaveriusGetPhysicalDeviceMemoryPropertiesReq;

typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 buffer_xid; CARD64 size; CARD32 usage_flags; CARD32 reserved; } XaveriusCreateBufferReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 buffer_xid; } XaveriusGetBufferMemoryRequirementsReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 memory_xid; CARD64 allocation_size; CARD32 memory_type_index; CARD32 reserved; } XaveriusAllocateMemoryReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 memory_xid; } XaveriusFreeMemoryReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 buffer_xid; CARD32 memory_xid; CARD64 memory_offset; } XaveriusBindBufferMemoryReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 buffer_xid; CARD64 buffer_offset; CARD32 data_length; CARD8 data[1]; } XaveriusWriteBufferReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 buffer_xid; CARD64 buffer_offset; CARD32 data_length; } XaveriusReadBufferReq;

typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 shader_module_xid; CARD32 data_length; CARD8 data[1]; } XaveriusShaderModuleDataReq;
typedef struct { CARD32 binding; CARD32 descriptor_type; CARD32 descriptor_count; CARD32 stage_flags; } XaveriusDescriptorSetLayoutBinding;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 descriptor_set_layout_xid; CARD32 flags; CARD32 binding_count; XaveriusDescriptorSetLayoutBinding bindings[1]; } XaveriusCreateDescriptorSetLayoutReq;
typedef struct { CARD32 descriptor_type; CARD32 descriptor_count; } XaveriusDescriptorPoolSize;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 descriptor_pool_xid; CARD32 max_sets; CARD16 pool_size_count; CARD16 reserved; XaveriusDescriptorPoolSize pool_sizes[1]; } XaveriusCreateDescriptorPoolReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 descriptor_pool_xid; } XaveriusDestroyDescriptorPoolReq;
typedef struct { CARD32 descriptor_set_xid; CARD32 descriptor_set_layout_xid; } XaveriusDescriptorSetAllocation;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 descriptor_pool_xid; CARD16 descriptor_set_count; CARD16 reserved; XaveriusDescriptorSetAllocation allocations[1]; } XaveriusAllocateDescriptorSetsReq;
typedef struct { CARD32 descriptor_set_xid; CARD32 binding; CARD32 array_element; CARD32 descriptor_type; CARD32 buffer_xid; CARD64 buffer_offset; CARD64 range; } XaveriusDescriptorWrite;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD16 write_count; CARD16 reserved0; CARD32 reserved1; XaveriusDescriptorWrite writes[1]; } XaveriusUpdateDescriptorSetsReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 descriptor_pool_xid; CARD16 descriptor_set_count; CARD16 reserved; CARD32 descriptor_set_xids[1]; } XaveriusFreeDescriptorSetsReq;
typedef struct { CARD32 stage_flags; CARD32 offset; CARD32 size; } XaveriusPushConstantRange;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 pipeline_layout_xid; CARD16 set_layout_count; CARD16 push_constant_range_count; CARD8 data[1]; } XaveriusCreatePipelineLayoutReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 pipeline_xid; CARD32 pipeline_layout_xid; CARD32 shader_module_xid; CARD16 entry_point_length; CARD16 reserved; CARD8 entry_point[1]; } XaveriusCreateComputePipelineReq;

typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 command_pool_xid; CARD32 queue_family_index; CARD32 flags; } XaveriusCreateCommandPoolReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 command_pool_xid; } XaveriusDestroyCommandPoolReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 command_buffer_xid; } XaveriusResetCommandBufferReq;
typedef struct { XaveriusReqHeader header; CARD32 command_buffer_xid; CARD32 command_pool_xid; CARD8 entries[1]; } XaveriusCommandBufferDataReq;
typedef struct { CARD16 command_id; CARD16 entry_length; } XaveriusCommandEntryHeader;
typedef struct { CARD16 command_id; CARD16 entry_length; CARD32 pipeline_xid; } XaveriusCmdBindPipeline;
typedef struct { CARD16 command_id; CARD16 entry_length; CARD32 pipeline_layout_xid; CARD32 first_set; CARD16 descriptor_set_count; CARD16 reserved; CARD32 descriptor_set_xids[1]; } XaveriusCmdBindDescriptorSets;
typedef struct { CARD16 command_id; CARD16 entry_length; CARD32 group_count_x; CARD32 group_count_y; CARD32 group_count_z; } XaveriusCmdDispatch;
typedef struct { CARD16 command_id; CARD16 entry_length; CARD32 stage_flags; CARD32 pipeline_layout_xid; CARD32 offset; CARD32 data_length; CARD8 data[1]; } XaveriusCmdPushConstants;
typedef struct { CARD16 command_id; CARD16 entry_length; CARD32 buffer_xid; CARD32 swapchain_xid; CARD32 image_index; CARD64 buffer_offset; CARD32 row_length; CARD32 image_height; CARD32 reserved; } XaveriusCmdCopyBufferToSwapchainImage;

typedef struct { CARD32 semaphore_xid; CARD64 wait_value; CARD32 wait_stage_mask; } XaveriusQueueSubmitWait;
typedef struct { CARD32 semaphore_xid; CARD64 signal_value; } XaveriusQueueSubmitSignal;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 queue_family_index; CARD32 queue_index; CARD32 fence_xid; CARD16 command_buffer_count; CARD16 wait_semaphore_count; CARD16 signal_semaphore_count; CARD16 reserved; CARD64 submission_token; CARD8 arrays[1]; } XaveriusQueueSubmitReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 semaphore_xid; CARD32 semaphore_type; CARD64 initial_value; } XaveriusCreateSemaphoreReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 semaphore_xid; } XaveriusGetSemaphoreCounterValueReq;
typedef struct { CARD32 semaphore_xid; CARD64 wait_value; } XaveriusWaitSemaphoreEntry;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 semaphore_count; CARD32 wait_mode; CARD64 timeout_ns; XaveriusWaitSemaphoreEntry entries[1]; } XaveriusWaitSemaphoresReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 semaphore_xid; CARD64 signal_value; } XaveriusSignalSemaphoreReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 semaphore_xid; } XaveriusDestroySemaphoreReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 fence_xid; CARD32 create_flags; CARD32 reserved; } XaveriusCreateFenceReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 fence_xid; } XaveriusGetFenceStatusReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 fence_xid; CARD64 timeout_ns; } XaveriusWaitForFenceReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 fence_xid; } XaveriusResetFenceReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 fence_xid; } XaveriusDestroyFenceReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 resource_xid; } XaveriusDestroyResourceReq;
typedef struct { XaveriusReqHeader header; CARD32 scope; CARD32 device_xid; CARD32 queue_family_index; CARD32 queue_index; CARD32 event_mask; CARD32 reserved; } XaveriusSelectEventsReq;

typedef struct { XaveriusReqHeader header; CARD32 surface_xid; CARD32 window_xid; } XaveriusCreateSurfaceReq;
typedef struct { XaveriusReqHeader header; CARD32 surface_xid; } XaveriusDestroySurfaceReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 surface_xid; } XaveriusGetSurfaceCapabilitiesReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 swapchain_xid; CARD32 surface_xid; CARD32 image_count; CARD32 format_id; CARD32 color_space_id; CARD32 width; CARD32 height; CARD32 present_mode_id; CARD32 image_usage_flags; } XaveriusCreateSwapchainReq;
typedef struct { XaveriusReqHeader header; CARD32 swapchain_xid; } XaveriusDestroySwapchainReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 swapchain_xid; CARD64 timeout_ns; CARD32 semaphore_xid; CARD32 fence_xid; } XaveriusAcquireNextImageReq;
typedef struct { XaveriusReqHeader header; CARD32 device_xid; CARD32 queue_family_index; CARD32 queue_index; CARD32 swapchain_xid; CARD32 image_index; CARD32 wait_semaphore_count; CARD64 presentation_token; CARD32 wait_semaphore_xids[1]; } XaveriusQueuePresentReq;

/* Replies whose fields occupy the standard 32-byte reply header's data area. */
typedef struct { CARD8 response_type; CARD8 data; CARD16 sequenceNumber; CARD32 length; CARD16 major_version; CARD16 minor_version; CARD8 reserved[20]; } XaveriusQueryVersionReply;
typedef struct { CARD8 response_type; CARD8 data; CARD16 sequenceNumber; CARD32 length; CARD32 status; CARD32 property_type; CARD32 value_length; CARD8 reserved[12]; CARD8 value[1]; } XaveriusGetPhysicalDevicePropertyReply;
typedef struct { CARD8 response_type; CARD8 data; CARD16 sequenceNumber; CARD32 length; CARD32 memory_type_count; CARD32 memory_heap_count; CARD32 memory_properties_flags; CARD32 reserved; CARD8 pad[8]; CARD8 records[1]; } XaveriusGetPhysicalDeviceMemoryPropertiesReply;
typedef struct { CARD32 property_flags; CARD32 heap_index; } XaveriusMemoryTypeInfo;
typedef struct { CARD64 size; CARD32 heap_flags; CARD32 reserved; } XaveriusMemoryHeapInfo;
typedef struct { CARD8 response_type; CARD8 data; CARD16 sequenceNumber; CARD32 length; CARD64 size; CARD64 alignment; CARD32 memory_type_bits; CARD32 reserved; } XaveriusGetBufferMemoryRequirementsReply;
typedef struct { CARD32 queue_flags; CARD32 queue_count; CARD32 reserved; } XaveriusQueueFamilyInfo;
typedef struct { XaveriusReplyHeader header; XaveriusQueueFamilyInfo records[1]; } XaveriusGetPhysicalDeviceQueueFamilyPropertiesReply;
typedef struct { CARD32 physical_device_xid; CARD32 device_type; CARD32 vendor_id; CARD32 device_id; CARD32 api_version; CARD32 driver_version; CARD32 compute_queue_family_count; CARD16 device_name_length; CARD16 reserved; CARD8 device_name[1]; } XaveriusPhysicalDeviceInfo;
typedef struct { XaveriusReplyHeader header; XaveriusPhysicalDeviceInfo records[1]; } XaveriusEnumeratePhysicalDevicesReply;
typedef struct { CARD8 response_type; CARD8 data; CARD16 sequenceNumber; CARD32 length; CARD64 counter_value; CARD8 reserved[16]; } XaveriusGetSemaphoreCounterValueReply;
typedef struct { XaveriusReplyHeader header; CARD8 data[1]; } XaveriusReadBufferReply;
typedef struct { CARD8 response_type; CARD8 data; CARD16 sequenceNumber; CARD32 length; CARD32 status; CARD8 reserved[20]; } XaveriusWaitSemaphoresReply;
typedef struct { CARD8 response_type; CARD8 fence_status; CARD16 sequenceNumber; CARD32 length; CARD8 reserved[24]; } XaveriusGetFenceStatusReply;
typedef struct { CARD8 response_type; CARD8 data; CARD16 sequenceNumber; CARD32 length; CARD32 status; CARD8 reserved[20]; } XaveriusWaitForFenceReply;

/* XGE events. */
typedef struct { XaveriusXGEHeader header; CARD32 device_xid; CARD32 error_code; CARD32 origin; CARD64 submission_token; CARD32 resource_xid; CARD32 reserved; } XaveriusAsyncErrorEvent;
typedef struct { XaveriusXGEHeader header; CARD32 device_xid; CARD32 reason_code; CARD64 device_generation; } XaveriusDeviceLostEvent;
typedef struct { XaveriusXGEHeader header; CARD32 physical_device_xid; CARD32 new_status; CARD32 reason_code; CARD32 reserved; } XaveriusDeviceStatusChangedEvent;
typedef struct { XaveriusXGEHeader header; CARD32 device_xid; CARD32 heap_index; CARD32 pressure_level; CARD32 reserved; } XaveriusDeviceMemoryPressureEvent;
typedef struct { XaveriusXGEHeader header; CARD32 device_xid; CARD32 idle_reason; CARD64 device_generation; } XaveriusDeviceIdleEvent;
typedef struct { XaveriusXGEHeader header; CARD32 device_xid; CARD32 queue_family_index; CARD32 queue_index; CARD32 reserved; CARD64 submission_token; } XaveriusSubmissionCompleteEvent;
typedef struct { XaveriusXGEHeader header; CARD32 device_xid; CARD32 queue_family_index; CARD32 queue_index; CARD32 failure_class; CARD64 submission_token; } XaveriusSubmissionFailedEvent;
typedef struct { XaveriusXGEHeader header; CARD32 device_xid; CARD32 queue_family_index; CARD32 queue_index; CARD32 swapchain_xid; CARD32 image_index; CARD32 reserved; CARD64 presentation_token; } XaveriusImageReleasedEvent;
typedef struct { XaveriusXGEHeader header; CARD32 device_xid; CARD32 queue_family_index; CARD32 queue_index; CARD32 swapchain_xid; CARD32 image_index; CARD32 status; CARD64 presentation_token; } XaveriusPresentationCompleteEvent;

/* Variable-length presentation capability reply starts at offset 64. */
typedef struct {
    CARD8 response_type; CARD8 data; CARD16 sequenceNumber; CARD32 length;
    CARD32 status; CARD32 present_supported; CARD32 current_width; CARD32 current_height;
    CARD32 min_image_count; CARD32 max_image_count; CARD32 min_width; CARD32 min_height;
    CARD32 max_width; CARD32 max_height; CARD32 format_pair_count; CARD32 present_mode_count;
    CARD32 image_usage_flags; CARD32 queue_family_count; CARD8 records[1];
} XaveriusGetSurfaceCapabilitiesReply;
typedef struct { CARD32 format_id; CARD32 color_space_id; } XaveriusSurfaceFormatPair;
typedef struct { CARD32 queue_family_index; CARD32 present_supported; } XaveriusSurfaceQueueFamilyInfo;
typedef struct { XaveriusReplyHeader header; CARD32 status; CARD32 actual_image_count; } XaveriusCreateSwapchainReply;
typedef struct { XaveriusReplyHeader header; CARD32 status; CARD32 image_index; CARD32 reserved; } XaveriusAcquireNextImageReply;
typedef struct { XaveriusReplyHeader header; CARD32 status; } XaveriusQueuePresentReply;

#pragma pack(pop)

#define sz_XaveriusReqHeader 8
#define sz_XaveriusReplyHeader 32
#define sz_XaveriusXGEHeader 32
#define sz_XaveriusQueryVersionReq 8
#define sz_XaveriusEnumeratePhysicalDevicesReq 8
#define sz_XaveriusGetPhysicalDevicePropertyReq 16
#define sz_XaveriusGetPhysicalDeviceQueueFamilyPropertiesReq 12
#define sz_XaveriusCreateDeviceReq 24
#define sz_XaveriusCreateDefaultDeviceReq 16
#define sz_XaveriusDestroyDeviceReq 12
#define sz_XaveriusGetPhysicalDeviceMemoryPropertiesReq 12
#define sz_XaveriusCreateBufferReq 32
#define sz_XaveriusGetBufferMemoryRequirementsReq 16
#define sz_XaveriusAllocateMemoryReq 32
#define sz_XaveriusFreeMemoryReq 16
#define sz_XaveriusBindBufferMemoryReq 28
#define sz_XaveriusWriteBufferReq 28
#define sz_XaveriusReadBufferReq 28
#define sz_XaveriusShaderModuleDataReq 20
#define sz_XaveriusCreateDescriptorSetLayoutReq 24
#define sz_XaveriusCreateDescriptorPoolReq 24
#define sz_XaveriusDestroyDescriptorPoolReq 16
#define sz_XaveriusAllocateDescriptorSetsReq 20
#define sz_XaveriusUpdateDescriptorSetsReq 20
#define sz_XaveriusFreeDescriptorSetsReq 20
#define sz_XaveriusCreatePipelineLayoutReq 20
#define sz_XaveriusCreateComputePipelineReq 28
#define sz_XaveriusCreateCommandPoolReq 24
#define sz_XaveriusDestroyCommandPoolReq 16
#define sz_XaveriusResetCommandBufferReq 16
#define sz_XaveriusCommandBufferDataReq 16
#define sz_XaveriusQueueSubmitReq 40
#define sz_XaveriusCreateSemaphoreReq 28
#define sz_XaveriusGetSemaphoreCounterValueReq 16
#define sz_XaveriusWaitSemaphoresReq 28
#define sz_XaveriusSignalSemaphoreReq 24
#define sz_XaveriusDestroySemaphoreReq 16
#define sz_XaveriusCreateFenceReq 24
#define sz_XaveriusGetFenceStatusReq 16
#define sz_XaveriusWaitForFenceReq 24
#define sz_XaveriusResetFenceReq 16
#define sz_XaveriusDestroyFenceReq 16
#define sz_XaveriusDestroyResourceReq 16
#define sz_XaveriusSelectEventsReq 32
#define sz_XaveriusCreateSurfaceReq 16
#define sz_XaveriusDestroySurfaceReq 12
#define sz_XaveriusGetSurfaceCapabilitiesReq 16
#define sz_XaveriusCreateSwapchainReq 48
#define sz_XaveriusDestroySwapchainReq 12
#define sz_XaveriusAcquireNextImageReq 32
#define sz_XaveriusQueuePresentReq 40
#define sz_XaveriusErrorPacket 32
#define sz_XaveriusQueryVersionReply 32
#define sz_XaveriusEnumeratePhysicalDevicesReply 32
#define sz_XaveriusGetPhysicalDevicePropertyReply 32
#define sz_XaveriusGetPhysicalDeviceQueueFamilyPropertiesReply 32
#define sz_XaveriusGetPhysicalDeviceMemoryPropertiesReply 32
#define sz_XaveriusGetBufferMemoryRequirementsReply 32
#define sz_XaveriusReadBufferReply 32
#define sz_XaveriusGetSemaphoreCounterValueReply 32
#define sz_XaveriusWaitSemaphoresReply 32
#define sz_XaveriusGetFenceStatusReply 32
#define sz_XaveriusWaitForFenceReply 32
#define sz_XaveriusGetSurfaceCapabilitiesReply 64
#define sz_XaveriusCreateSwapchainReply 40
#define sz_XaveriusAcquireNextImageReply 44
#define sz_XaveriusQueuePresentReply 36
#define sz_XaveriusPhysicalDeviceInfo 32
#define sz_XaveriusQueueFamilyInfo 12
#define sz_XaveriusMemoryTypeInfo 8
#define sz_XaveriusMemoryHeapInfo 16
#define sz_XaveriusDescriptorSetLayoutBinding 16
#define sz_XaveriusDescriptorPoolSize 8
#define sz_XaveriusDescriptorSetAllocation 8
#define sz_XaveriusDescriptorWrite 36
#define sz_XaveriusPushConstantRange 12
#define sz_XaveriusQueueSubmitWait 16
#define sz_XaveriusQueueSubmitSignal 12
#define sz_XaveriusWaitSemaphoreEntry 12
#define sz_XaveriusAsyncErrorEvent 60
#define sz_XaveriusDeviceLostEvent 48
#define sz_XaveriusDeviceStatusChangedEvent 48
#define sz_XaveriusDeviceMemoryPressureEvent 48
#define sz_XaveriusDeviceIdleEvent 48
#define sz_XaveriusSubmissionCompleteEvent 56
#define sz_XaveriusSubmissionFailedEvent 56
#define sz_XaveriusImageReleasedEvent 64
#define sz_XaveriusPresentationCompleteEvent 64

#if defined(__cplusplus)
# define XAVERIUS_STATIC_ASSERT(c, m) static_assert((c), m)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
# define XAVERIUS_STATIC_ASSERT(c, m) _Static_assert((c), m)
#else
# define XAVERIUS_STATIC_ASSERT_HELPER(c, m, line) typedef char xav_static_assert_##line[(c) ? 1 : -1]
# define XAVERIUS_STATIC_ASSERT(c, m) XAVERIUS_STATIC_ASSERT_HELPER(c, m, __LINE__)
#endif
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusReqHeader) == 8, "request header size");
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusReplyHeader) == 32, "reply header size");
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusXGEHeader) == 32, "XGE header size");
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusQueryVersionReply) == 32, "QueryVersion reply size");
XAVERIUS_STATIC_ASSERT(offsetof(XaveriusGetPhysicalDevicePropertyReply, value) == 32, "property reply payload offset");
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusGetBufferMemoryRequirementsReply) == 32, "memory requirements reply size");
XAVERIUS_STATIC_ASSERT(offsetof(XaveriusAsyncErrorEvent, submission_token) == 44, "ASYNC_ERROR token offset");
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusAsyncErrorEvent) == 60, "ASYNC_ERROR event size");
XAVERIUS_STATIC_ASSERT(offsetof(XaveriusImageReleasedEvent, presentation_token) == 56, "IMAGE_RELEASED token offset");
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusImageReleasedEvent) == 64, "IMAGE_RELEASED event size");
XAVERIUS_STATIC_ASSERT(offsetof(XaveriusGetSurfaceCapabilitiesReply, records) == 64, "capabilities variable payload offset");
XAVERIUS_STATIC_ASSERT(offsetof(XaveriusGetPhysicalDeviceMemoryPropertiesReply, records) == 32, "memory properties payload offset");
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusErrorPacket) == 32, "X11 error packet size");

/* FLOAT32 is an IEEE-754 binary32 wire value, not an arbitrary C float. */
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusFloat32) == 4, "FLOAT32 size");
XAVERIUS_STATIC_ASSERT(FLT_RADIX == 2, "FLOAT32 radix");
XAVERIUS_STATIC_ASSERT(FLT_MANT_DIG == 24, "FLOAT32 precision");
XAVERIUS_STATIC_ASSERT(FLT_MAX_EXP == 128, "FLOAT32 maximum exponent");
XAVERIUS_STATIC_ASSERT(FLT_MIN_EXP == -125, "FLOAT32 minimum exponent");

/* Fixed request structures must match their normative total wire lengths. */
#define XAVERIUS_ASSERT_REQ_SIZE(type) \
    XAVERIUS_STATIC_ASSERT(sizeof(type) == sz_##type, #type " wire size")
#define XAVERIUS_ASSERT_REQ_PREFIX(type, field) \
    XAVERIUS_STATIC_ASSERT(offsetof(type, field) == sz_##type, #type " fixed prefix")

XAVERIUS_ASSERT_REQ_SIZE(XaveriusQueryVersionReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusEnumeratePhysicalDevicesReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusGetPhysicalDevicePropertyReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusGetPhysicalDeviceQueueFamilyPropertiesReq);
XAVERIUS_ASSERT_REQ_PREFIX(XaveriusCreateDeviceReq, requests);
XAVERIUS_ASSERT_REQ_PREFIX(XaveriusCreateDefaultDeviceReq, priorities);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusDestroyDeviceReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusGetPhysicalDeviceMemoryPropertiesReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusCreateBufferReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusGetBufferMemoryRequirementsReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusAllocateMemoryReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusFreeMemoryReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusBindBufferMemoryReq);
XAVERIUS_ASSERT_REQ_PREFIX(XaveriusWriteBufferReq, data);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusReadBufferReq);
XAVERIUS_ASSERT_REQ_PREFIX(XaveriusShaderModuleDataReq, data);
XAVERIUS_ASSERT_REQ_PREFIX(XaveriusCreateDescriptorSetLayoutReq, bindings);
XAVERIUS_ASSERT_REQ_PREFIX(XaveriusCreateDescriptorPoolReq, pool_sizes);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusDestroyDescriptorPoolReq);
XAVERIUS_ASSERT_REQ_PREFIX(XaveriusAllocateDescriptorSetsReq, allocations);
XAVERIUS_ASSERT_REQ_PREFIX(XaveriusUpdateDescriptorSetsReq, writes);
XAVERIUS_ASSERT_REQ_PREFIX(XaveriusFreeDescriptorSetsReq, descriptor_set_xids);
XAVERIUS_ASSERT_REQ_PREFIX(XaveriusCreatePipelineLayoutReq, data);
XAVERIUS_ASSERT_REQ_PREFIX(XaveriusCreateComputePipelineReq, entry_point);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusCreateCommandPoolReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusDestroyCommandPoolReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusResetCommandBufferReq);
XAVERIUS_ASSERT_REQ_PREFIX(XaveriusCommandBufferDataReq, entries);
XAVERIUS_ASSERT_REQ_PREFIX(XaveriusQueueSubmitReq, arrays);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusCreateSemaphoreReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusGetSemaphoreCounterValueReq);
XAVERIUS_ASSERT_REQ_PREFIX(XaveriusWaitSemaphoresReq, entries);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusSignalSemaphoreReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusDestroySemaphoreReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusCreateFenceReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusGetFenceStatusReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusWaitForFenceReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusResetFenceReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusDestroyFenceReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusDestroyResourceReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusSelectEventsReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusCreateSurfaceReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusDestroySurfaceReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusGetSurfaceCapabilitiesReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusCreateSwapchainReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusDestroySwapchainReq);
XAVERIUS_ASSERT_REQ_SIZE(XaveriusAcquireNextImageReq);
XAVERIUS_ASSERT_REQ_PREFIX(XaveriusQueuePresentReq, wait_semaphore_xids);

/* Nested variable records and reply payload boundaries. */
XAVERIUS_STATIC_ASSERT(offsetof(XaveriusQueueFamilyRequest, queue_priorities) == 8, "queue priorities offset");
XAVERIUS_STATIC_ASSERT(offsetof(XaveriusDeviceExtensionRequest, name) == 4, "device extension name offset");
XAVERIUS_STATIC_ASSERT(offsetof(XaveriusCmdBindDescriptorSets, descriptor_set_xids) == 16, "descriptor-set command tail offset");
XAVERIUS_STATIC_ASSERT(offsetof(XaveriusCmdPushConstants, data) == 20, "push-constant command tail offset");
XAVERIUS_STATIC_ASSERT(offsetof(XaveriusPhysicalDeviceInfo, device_name) == 32, "physical-device name offset");
XAVERIUS_STATIC_ASSERT(offsetof(XaveriusEnumeratePhysicalDevicesReply, records) == 32, "physical-device reply records offset");
XAVERIUS_STATIC_ASSERT(offsetof(XaveriusGetPhysicalDeviceQueueFamilyPropertiesReply, records) == 32, "queue-family reply records offset");
XAVERIUS_STATIC_ASSERT(offsetof(XaveriusGetPhysicalDeviceMemoryPropertiesReply, records) == 32, "memory-property reply records offset");
XAVERIUS_STATIC_ASSERT(offsetof(XaveriusReadBufferReply, data) == 32, "ReadBuffer payload offset");
XAVERIUS_STATIC_ASSERT(offsetof(XaveriusGetSurfaceCapabilitiesReply, records) == 64, "surface-capability payload offset");

/* Every fixed-size reply and every XGE event must have its specified size. */
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusQueryVersionReply) == sz_XaveriusQueryVersionReply, "QueryVersion reply size");
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusGetBufferMemoryRequirementsReply) == sz_XaveriusGetBufferMemoryRequirementsReply, "buffer requirements reply size");
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusGetSemaphoreCounterValueReply) == sz_XaveriusGetSemaphoreCounterValueReply, "semaphore counter reply size");
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusWaitSemaphoresReply) == sz_XaveriusWaitSemaphoresReply, "WaitSemaphores reply size");
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusGetFenceStatusReply) == sz_XaveriusGetFenceStatusReply, "GetFenceStatus reply size");
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusWaitForFenceReply) == sz_XaveriusWaitForFenceReply, "WaitForFence reply size");
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusCreateSwapchainReply) == sz_XaveriusCreateSwapchainReply, "CreateSwapchain reply size");
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusAcquireNextImageReply) == sz_XaveriusAcquireNextImageReply, "AcquireNextImage reply size");
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusQueuePresentReply) == sz_XaveriusQueuePresentReply, "QueuePresent reply size");
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusAsyncErrorEvent) == sz_XaveriusAsyncErrorEvent, "ASYNC_ERROR event size");
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusDeviceLostEvent) == sz_XaveriusDeviceLostEvent, "DEVICE_LOST event size");
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusDeviceStatusChangedEvent) == sz_XaveriusDeviceStatusChangedEvent, "DEVICE_STATUS_CHANGED event size");
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusDeviceMemoryPressureEvent) == sz_XaveriusDeviceMemoryPressureEvent, "DEVICE_MEMORY_PRESSURE event size");
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusDeviceIdleEvent) == sz_XaveriusDeviceIdleEvent, "DEVICE_IDLE event size");
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusSubmissionCompleteEvent) == sz_XaveriusSubmissionCompleteEvent, "SUBMISSION_COMPLETE event size");
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusSubmissionFailedEvent) == sz_XaveriusSubmissionFailedEvent, "SUBMISSION_FAILED event size");
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusImageReleasedEvent) == sz_XaveriusImageReleasedEvent, "IMAGE_RELEASED event size");
XAVERIUS_STATIC_ASSERT(sizeof(XaveriusPresentationCompleteEvent) == sz_XaveriusPresentationCompleteEvent, "PRESENTATION_COMPLETE event size");

#undef XAVERIUS_ASSERT_REQ_SIZE
#undef XAVERIUS_ASSERT_REQ_PREFIX
#undef XAVERIUS_STATIC_ASSERT

#endif /* __XLIBRE_XAVERIUS_PROTO_H */
