#ifndef XAVERIUS_PROTO_H
#define XAVERIUS_PROTO_H

#include <X11/X.h>
#include <X11/Xproto.h>

#ifndef FLOAT32
#define FLOAT32 float
#endif

typedef struct {
    CARD8 reqType;
    CARD8 data;
    CARD16 length;
    CARD8 subopcode;
    CARD8 flags;
    CARD16 reserved;
} xAveriusReq;

#define sz_xAveriusReq 8

typedef struct {
    CARD8 reqType;
    CARD8 data;
    CARD16 length;
    CARD8 subopcode;
    CARD8 device_type;
    CARD16 reserved;
} xAveriusEnumeratePhysicalDevicesReq;

#define sz_xAveriusEnumeratePhysicalDevicesReq 8

typedef struct {
    BYTE type;
    BYTE pad1;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 nDevices;
    CARD32 pad2;
    CARD32 pad3;
} xAveriusEnumeratePhysicalDevicesReply;

#define sz_xAveriusEnumeratePhysicalDevicesReply 32

typedef struct {
    CARD8 reqType;
    CARD8 data;
    CARD16 length;
    CARD8 subopcode;
    CARD8 flags;
    CARD16 reserved;
    XID physical_device_xid;
    ATOM property_atom;
} xAveriusGetPhysicalDevicePropertyReq;

#define sz_xAveriusGetPhysicalDevicePropertyReq 16

typedef struct {
    BYTE type;
    BYTE pad1;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 status;
    CARD32 property_type;
    CARD32 property_value;
    CARD32 reserved[2];
} xAveriusGetPhysicalDevicePropertyReply;

#define sz_xAveriusGetPhysicalDevicePropertyReply 32

typedef struct {
    CARD8 reqType;
    CARD8 data;
    CARD16 length;
    CARD8 subopcode;
    CARD8 flags;
    CARD16 reserved;
    XID physical_device_xid;
} xAveriusGetPhysicalDeviceQueueFamilyPropertiesReq;

#define sz_xAveriusGetPhysicalDeviceQueueFamilyPropertiesReq 12

typedef struct {
    BYTE type;
    BYTE pad1;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 queue_family_index;
    CARD32 queue_count;
    CARD32 flags;
    CARD32 reserved;
} xAveriusGetPhysicalDeviceQueueFamilyPropertiesReply;

#define sz_xAveriusGetPhysicalDeviceQueueFamilyPropertiesReply 32

typedef struct {
    CARD8 reqType;
    CARD8 data;
    CARD16 length;
    CARD8 subopcode;
    CARD8 flags;
    CARD16 reserved;
    XID device_xid;
    XID physical_device_xid;
    CARD16 queue_family_request_count;
    CARD16 feature_request_count;
    CARD16 device_extension_count;
    CARD16 reserved2;
} xAveriusCreateDeviceReq;

#define sz_xAveriusCreateDeviceReq 24

typedef struct {
    CARD8 reqType;
    CARD8 data;
    CARD16 length;
    CARD8 subopcode;
    CARD8 flags;
    CARD16 reserved;
    XID device_xid;
    CARD16 queue_count;
    CARD16 reserved2;
    FLOAT32 priorities[];
} xAveriusCreateDefaultDeviceReq;

#define sz_xAveriusCreateDefaultDeviceReq 16

typedef struct {
    CARD8 reqType;
    CARD8 data;
    CARD16 length;
    CARD8 subopcode;
    CARD8 flags;
    CARD16 reserved;
    XID device_xid;
} xAveriusDestroyDeviceReq;

#define sz_xAveriusDestroyDeviceReq 12

typedef struct {
    CARD8 reqType;
    CARD8 data;
    CARD16 length;
    CARD8 subopcode;
    CARD8 flags;
    CARD16 reserved;
    XID physical_device_xid;
} xAveriusGetPhysicalDeviceMemoryPropertiesReq;

#define sz_xAveriusGetPhysicalDeviceMemoryPropertiesReq 12

typedef struct {
    XID physical_device_xid;
    CARD32 device_type;
    CARD32 vendor_id;
    CARD32 device_id;
    CARD32 api_version;
    CARD32 driver_version;
    CARD32 compute_queue_family_count;
    CARD16 device_name_length;
    CARD16 reserved;
} xAveriusPhysicalDeviceInfo;

#define sz_xAveriusPhysicalDeviceInfo 32

/* Sync subopcodes 34-40 */
#define XAVERIUS_SUBOPCODE_CREATE_SEMAPHORE           34
#define XAVERIUS_SUBOPCODE_GET_SEMAPHORE_COUNTER_VALUE 35
#define XAVERIUS_SUBOPCODE_WAIT_SEMAPHORES             36
#define XAVERIUS_SUBOPCODE_SIGNAL_SEMAPHORE            37
#define XAVERIUS_SUBOPCODE_DESTROY_SEMAPHORE           38
#define XAVERIUS_SUBOPCODE_CREATE_FENCE                39
#define XAVERIUS_SUBOPCODE_GET_FENCE_STATUS            40

/* Semaphore types */
#define XAVERIUS_SEMAPHORE_TYPE_BINARY     0
#define XAVERIUS_SEMAPHORE_TYPE_TIMELINE   1

/* Semaphore states */
#define XAVERIUS_SEMAPHORE_STATE_UNSIGNALED 0
#define XAVERIUS_SEMAPHORE_STATE_PENDING    1
#define XAVERIUS_SEMAPHORE_STATE_SIGNALED   2
#define XAVERIUS_SEMAPHORE_STATE_FAILED     3

/* Wait modes */
#define XAVERIUS_WAIT_MODE_ANY   0
#define XAVERIUS_WAIT_MODE_ALL   1

/* Fence states */
#define XAVERIUS_FENCE_STATE_UNSIGNALED 0
#define XAVERIUS_FENCE_STATE_PENDING    1
#define XAVERIUS_FENCE_STATE_SIGNALED   2
#define XAVERIUS_FENCE_STATE_FAILED     3

typedef struct {
    CARD8 reqType;
    CARD8 data;
    CARD16 length;
    CARD8 subopcode;
    CARD8 flags;
    CARD16 reserved;
    XID device_xid;
    XID semaphore_xid;
    CARD32 semaphore_type;
    CARD64 initial_value;
} xAveriusCreateSemaphoreReq;

#define sz_xAveriusCreateSemaphoreReq 28

typedef struct {
    CARD8 reqType;
    CARD8 data;
    CARD16 length;
    CARD8 subopcode;
    CARD8 flags;
    CARD16 reserved;
    XID device_xid;
    XID semaphore_xid;
} xAveriusGetSemaphoreCounterValueReq;

#define sz_xAveriusGetSemaphoreCounterValueReq 16

typedef struct {
    BYTE type;
    BYTE pad1;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD64 counter_value;
    CARD32 reserved[2];
} xAveriusGetSemaphoreCounterValueReply;

#define sz_xAveriusGetSemaphoreCounterValueReply 32

typedef struct {
    CARD8 reqType;
    CARD8 data;
    CARD16 length;
    CARD8 subopcode;
    CARD8 flags;
    CARD16 reserved;
    XID device_xid;
    CARD32 semaphore_count;
    CARD32 wait_mode;
    CARD64 timeout_ns;
} xAveriusWaitSemaphoresReq;

#define sz_xAveriusWaitSemaphoresReq 28

typedef struct {
    BYTE type;
    BYTE pad1;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 status;
    CARD32 reserved[3];
} xAveriusWaitSemaphoresReply;

#define sz_xAveriusWaitSemaphoresReply 32

typedef struct {
    CARD8 reqType;
    CARD8 data;
    CARD16 length;
    CARD8 subopcode;
    CARD8 flags;
    CARD16 reserved;
    XID device_xid;
    XID semaphore_xid;
    CARD64 signal_value;
} xAveriusSignalSemaphoreReq;

#define sz_xAveriusSignalSemaphoreReq 24

typedef struct {
    CARD8 reqType;
    CARD8 data;
    CARD16 length;
    CARD8 subopcode;
    CARD8 flags;
    CARD16 reserved;
    XID device_xid;
    XID semaphore_xid;
} xAveriusDestroySemaphoreReq;

#define sz_xAveriusDestroySemaphoreReq 16

typedef struct {
    CARD8 reqType;
    CARD8 data;
    CARD16 length;
    CARD8 subopcode;
    CARD8 flags;
    CARD16 reserved;
    XID device_xid;
    XID fence_xid;
    CARD32 create_flags;
    CARD32 reserved2;
} xAveriusCreateFenceReq;

#define sz_xAveriusCreateFenceReq 24

typedef struct {
    CARD8 reqType;
    CARD8 data;
    CARD16 length;
    CARD8 subopcode;
    CARD8 flags;
    CARD16 reserved;
    XID device_xid;
    XID fence_xid;
} xAveriusGetFenceStatusReq;

#define sz_xAveriusGetFenceStatusReq 16

typedef struct {
    BYTE type;
    BYTE fence_state;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 reserved[6];
} xAveriusGetFenceStatusReply;

#define sz_xAveriusGetFenceStatusReply 32
#endif