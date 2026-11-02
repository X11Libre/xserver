#ifndef XAVERIUS_H
#define XAVERIUS_H

/* SPDX-License-Identifier: MIT OR X11 */

#include "xorg-server.h"
#include "xf86.h"
#include "xf86_OSproc.h"
#include "compiler.h"
#include "xf86int10.h"

#define XAVERIUS_EXTENSION_NAME "XAVERIUS"

#define XAVERIUS_MAJOR_VERSION 0
#define XAVERIUS_MINOR_VERSION 1

/* Subopcodes 1-7: Device Discovery */
#define XAVERIUS_SUBOPCODE_ENUMERATE_PHYSICAL_DEVICES       1
#define XAVERIUS_SUBOPCODE_GET_PHYSICAL_DEVICE_PROPERTY     2
#define XAVERIUS_SUBOPCODE_GET_PHYSICAL_DEVICE_QUEUE_FAMILY_PROPERTIES 3
#define XAVERIUS_SUBOPCODE_CREATE_DEVICE                    4
#define XAVERIUS_SUBOPCODE_CREATE_DEFAULT_DEVICE            5
#define XAVERIUS_SUBOPCODE_DESTROY_DEVICE                   6
#define XAVERIUS_SUBOPCODE_GET_PHYSICAL_DEVICE_MEMORY_PROPERTIES 7

/* Subopcodes 25-33: Compute Lifecycle */
#define XAVERIUS_SUBOPCODE_DESTROY_COMMAND_POOL             25
#define XAVERIUS_SUBOPCODE_RESET_COMMAND_BUFFER             26
#define XAVERIUS_SUBOPCODE_COMMAND_BUFFER_DATA              27
#define XAVERIUS_SUBOPCODE_QUEUE_SUBMIT                     28
#define XAVERIUS_SUBOPCODE_CREATE_SEMAPHORE                 29
#define XAVERIUS_SUBOPCODE_GET_SEMAPHORE_COUNTER_VALUE      30
#define XAVERIUS_SUBOPCODE_WAIT_SEMAPHORES                  31
#define XAVERIUS_SUBOPCODE_SIGNAL_SEMAPHORE                 32
#define XAVERIUS_SUBOPCODE_DESTROY_SEMAPHORE                33

#define XAVERIUS_ERROR_BASE 100

#define XAVERIUS_ERROR_DEVICE_UNAVAILABLE     (XAVERIUS_ERROR_BASE + 0)
#define XAVERIUS_ERROR_DEVICE_LOST            (XAVERIUS_ERROR_BASE + 1)
#define XAVERIUS_ERROR_EXECUTION_FAILED       (XAVERIUS_ERROR_BASE + 2)
#define XAVERIUS_ERROR_DEVICE_DESTROYED       (XAVERIUS_ERROR_BASE + 3)
#define XAVERIUS_ERROR_INTERNAL_SERVER_ERROR  (XAVERIUS_ERROR_BASE + 4)
#define XAVERIUS_ERROR_INVALID_SYNCHRONIZATION_STATE (XAVERIUS_ERROR_BASE + 5)
#define XAVERIUS_ERROR_TRANSFER_FAILED        (XAVERIUS_ERROR_BASE + 6)
#define XAVERIUS_ERROR_ALLOCATION_FAILED      (XAVERIUS_ERROR_BASE + 7)
#define XAVERIUS_ERROR_DEPENDENCY_FAILED      (XAVERIUS_ERROR_BASE + 8)

#define XAVERIUS_EVENT_BASE 100

#define XAVERIUS_EVENT_DEVICE_LOST            (XAVERIUS_EVENT_BASE + 0)
#define XAVERIUS_EVENT_SUBMISSION_COMPLETE    (XAVERIUS_EVENT_BASE + 1)

/* Property types */
#define XAVERIUS_PROP_TYPE_NONE           0
#define XAVERIUS_PROP_TYPE_CARD32         1
#define XAVERIUS_PROP_TYPE_CARD64         2
#define XAVERIUS_PROP_TYPE_ATOM           3
#define XAVERIUS_PROP_TYPE_STRING8        4
#define XAVERIUS_PROP_TYPE_BYTE_ARRAY     5
#define XAVERIUS_PROP_TYPE_CARD32_ARRAY   6
#define XAVERIUS_PROP_TYPE_CARD64_ARRAY   7

/* Property status */
#define XAVERIUS_PROP_STATUS_SUCCESS      0
#define XAVERIUS_PROP_STATUS_NOT_SUPPORTED 1
#define XAVERIUS_PROP_STATUS_NOT_AVAILABLE 2

/* Device types */
#define XAVERIUS_DEVICE_TYPE_PHYSICAL    1
#define XAVERIUS_DEVICE_TYPE_LOGICAL     2

/* Queue flags */
#define XAVERIUS_QUEUE_FLAG_GRAPHICS     0x00000001
#define XAVERIUS_QUEUE_FLAG_COMPUTE      0x00000002
#define XAVERIUS_QUEUE_FLAG_TRANSFER     0x00000004
#define XAVERIUS_QUEUE_FLAG_SPARSE_BINDING 0x00000008
#define XAVERIUS_QUEUE_FLAG_PROTECTED    0x00000010

/* Command pool flags */
#define XAVERIUS_COMMAND_POOL_FLAG_TRANSIENT         0x00000001
#define XAVERIUS_COMMAND_POOL_FLAG_RESET_COMMAND_BUFFER 0x00000002

/* Command buffer states */
#define XAVERIUS_CMD_BUFFER_STATE_INITIAL     0
#define XAVERIUS_CMD_BUFFER_STATE_RECORDING   1
#define XAVERIUS_CMD_BUFFER_STATE_EXECUTABLE  2
#define XAVERIUS_CMD_BUFFER_STATE_PENDING     3
#define XAVERIUS_CMD_BUFFER_STATE_INVALID     4

/* CommandBufferData flags */
#define XAVERIUS_CMD_DATA_FLAG_CREATE  0x01
#define XAVERIUS_CMD_DATA_FLAG_FINAL   0x02
#define XAVERIUS_CMD_DATA_FLAG_ABORT   0x04

/* Semaphore types */
#define XAVERIUS_SEMAPHORE_TYPE_BINARY   0
#define XAVERIUS_SEMAPHORE_TYPE_TIMELINE 1

/* Wait modes */
#define XAVERIUS_WAIT_MODE_ANY  0
#define XAVERIUS_WAIT_MODE_ALL  1

/* Fence states */
#define XAVERIUS_FENCE_STATE_UNSIGNALED   0
#define XAVERIUS_FENCE_STATE_PENDING      1
#define XAVERIUS_FENCE_STATE_SIGNALED     2
#define XAVERIUS_FENCE_STATE_FAILED       3

/* WaitSemaphores reply status */
#define XAVERIUS_WAIT_STATUS_SUCCESS      0
#define XAVERIUS_WAIT_STATUS_TIMEOUT      1

/* WaitForFence reply status */
#define XAVERIUS_FENCE_WAIT_SUCCESS       0
#define XAVERIUS_FENCE_WAIT_TIMEOUT       1
#define XAVERIUS_FENCE_WAIT_EXECUTION_FAILED 2
#define XAVERIUS_FENCE_WAIT_DEVICE_LOST   3
#define XAVERIUS_FENCE_WAIT_DEVICE_DESTROYED 4

typedef struct {
    XID xid;
    uint32_t type;
} XaveriusResourceRec, *XaveriusResourcePtr;

/* Phase 1: Physical device info (used by EnumeratePhysicalDevices) */
typedef struct {
    XID physical_device_xid;
    uint32_t device_type;
    uint32_t vendor_id;
    uint32_t device_id;
    uint32_t api_version;
    uint32_t driver_version;
    uint32_t compute_queue_family_count;
    char name[256];
} PhysicalDeviceInfoRec, *PhysicalDeviceInfoPtr;

/* Phase 1: Device creation requests */
typedef struct {
    XID device_xid;
    XID physical_device_xid;
    uint32_t queue_family_index;
    uint32_t queue_count;
    float *queue_priorities;
    uint32_t feature_count;
    uint32_t *feature_ids;
    uint32_t extension_count;
    char **extension_names;
} CreateDeviceRequestRec, *CreateDeviceRequestPtr;

typedef struct {
    XID device_xid;
    uint32_t queue_count;
    float *priorities;
} CreateDefaultDeviceRequestRec, *CreateDefaultDeviceRequestPtr;

typedef struct {
    XID device_xid;
} DestroyDeviceRequestRec, *DestroyDeviceRequestPtr;

/* Phase 2: Extended physical device */
typedef struct {
    XID physical_device_xid;
    uint32_t vendor_id;
    uint32_t device_id;
    uint32_t api_version;
    uint32_t driver_version;
    uint32_t compute_queue_family_count;
    uint32_t queue_flags;
    uint32_t queue_count;
    char name[256];
    uint32_t max_work_group_invocations;
    uint32_t max_work_group_size[3];
    uint8_t device_uuid[16];
    uint8_t driver_uuid[16];
    Bool timeline_semaphore_supported;
} XaveriusPhysicalDeviceRec, *XaveriusPhysicalDevicePtr;

/* Phase 2: Logical device */
typedef struct {
    XID xid;
    XID physical_device_xid;
    uint32_t queue_family_index;
    uint32_t queue_count;
    float *queue_priorities;
    uint32_t feature_count;
    uint32_t *feature_ids;
    uint32_t extension_count;
    char **extension_names;
} XaveriusLogicalDeviceRec, *XaveriusLogicalDevicePtr;

/* Phase 2: Command pool */
typedef struct {
    XID xid;
    XID device_xid;
    uint32_t queue_family_index;
    uint32_t flags;
    XID *command_buffers;
    uint32_t command_buffer_count;
} XaveriusCommandPoolRec, *XaveriusCommandPoolPtr;

/* Phase 2: Command buffer */
typedef struct {
    XID xid;
    XID device_xid;
    XID command_pool_xid;
    uint32_t state;
    Bool has_recorded_entries;
} XaveriusCommandBufferRec, *XaveriusCommandBufferPtr;

/* Phase 2: Semaphore */
typedef struct {
    XID xid;
    XID device_xid;
    uint32_t type;
    uint64_t counter;
    int signaled;
    int wait_pending;
    int signal_pending;
} XaveriusSemaphoreRec, *XaveriusSemaphorePtr;

/* Phase 2: Fence */
typedef struct {
    XID xid;
    XID device_xid;
    int state;
} XaveriusFenceRec, *XaveriusFencePtr;

/* Phase 2: Wait/Signal entries */
typedef struct {
    XID semaphore_xid;
    uint64_t wait_value;
    uint32_t wait_stage_mask;
} XaveriusWaitEntryRec;

typedef struct {
    XID semaphore_xid;
    uint64_t signal_value;
} XaveriusSignalEntryRec;

int xaverius_extension_init(void);
void xaverius_extension_fini(void);

int ProcXAveriusDispatch(ClientPtr client);
int SProcXAveriusDispatch(ClientPtr client);

int ProcXAveriusGetPhysicalDeviceProperty(ClientPtr client);
int ProcXAveriusGetPhysicalDeviceQueueFamilyProperties(ClientPtr client);
int ProcXAveriusDestroyCommandPool(ClientPtr client);
int ProcXAveriusResetCommandBuffer(ClientPtr client);
int ProcXAveriusCommandBufferData(ClientPtr client);
int ProcXAveriusQueueSubmit(ClientPtr client);
int ProcXAveriusCreateSemaphore(ClientPtr client);
int ProcXAveriusGetSemaphoreCounterValue(ClientPtr client);
int ProcXAveriusWaitSemaphores(ClientPtr client);
int ProcXAveriusSignalSemaphore(ClientPtr client);
int ProcXAveriusDestroySemaphore(ClientPtr client);

void xaverius_device_init(void);
void xaverius_device_fini(void);
void xaverius_init_property_atoms(void);

int xaverius_register_physical_device(XaveriusPhysicalDevicePtr dev);
XaveriusPhysicalDevicePtr xaverius_lookup_physical_device(XID xid);

extern RESTYPE XaveriusPhysicalDeviceType;
extern RESTYPE XaveriusLogicalDeviceType;
extern RESTYPE XaveriusCommandPoolType;
extern RESTYPE XaveriusCommandBufferType;
extern RESTYPE XaveriusSemaphoreType;
extern RESTYPE XaveriusFenceType;

#endif