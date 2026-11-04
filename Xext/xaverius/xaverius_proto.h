#ifndef XAVERIUS_PROTO_H
#define XAVERIUS_PROTO_H

#include <X11/X.h>
#include <X11/Xproto.h>

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
    BYTE type;
    BYTE pad1;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 nDevices;
} xAveriusEnumeratePhysicalDevicesReply;

#define sz_xAveriusEnumeratePhysicalDevicesReply 16

typedef struct {
    XID physical_device_xid;
    CARD32 device_type;
    CARD32 vendor_id;
    CARD32 device_id;
    CARD32 api_version;
    CARD32 driver_version;
    CARD32 compute_queue_family_count;
} xAveriusPhysicalDeviceInfo;

#define sz_xAveriusPhysicalDeviceInfo 28

typedef struct {
    BYTE type;
    BYTE pad1;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 property_id;
    CARD32 property_value;
} xAveriusGetPhysicalDevicePropertyReply;

#define sz_xAveriusGetPhysicalDevicePropertyReply 16

typedef struct {
    BYTE type;
    BYTE pad1;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 queue_family_index;
    CARD32 queue_count;
    CARD32 flags;
} xAveriusGetPhysicalDeviceQueueFamilyPropertiesReply;

#define sz_xAveriusGetPhysicalDeviceQueueFamilyPropertiesReply 16

/* Request structs for Physical Device operations (subopcodes 1-3). */

typedef struct {
    xAveriusReq header;
    XID physical_device_xid;
    CARD32 property_id;
} xAveriusGetPhysicalDevicePropertyReq;

#define sz_xAveriusGetPhysicalDevicePropertyReq 16

typedef struct {
    xAveriusReq header;
    XID physical_device_xid;
} xAveriusGetPhysicalDeviceQueueFamilyPropertiesReq;

#define sz_xAveriusGetPhysicalDeviceQueueFamilyPropertiesReq 12

typedef struct {
    xAveriusReq header;
} xAveriusEnumeratePhysicalDevicesReq;

#define sz_xAveriusEnumeratePhysicalDevicesReq 8

/* Queue family description as reported by a DDX backend. */
typedef struct _xavQueueFamilyInfo {
    uint32_t queue_family_index;
    uint32_t queue_flags;
    uint32_t queue_count;
} xavQueueFamilyInfo;

/* Memory type description as reported by a DDX backend. */
typedef struct _xavMemoryTypeInfo {
    uint32_t memory_type_index;
    uint32_t heap_index;
    uint32_t property_flags;
} xavMemoryTypeInfo;

/* Memory heap description as reported by a DDX backend. */
typedef struct _xavMemoryHeapInfo {
    uint32_t memory_heap_index;
    uint64_t size;
    uint32_t memory_heap_flags;
} xavMemoryHeapInfo;

/* Reply struct for GetBufferMemoryRequirements (subopcode 17). */
typedef struct {
    BYTE type;
    BYTE pad1;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD64 size;
    CARD64 alignment;
    CARD32 memory_type_bits;
    CARD32 reserved;
} xAveriusGetBufferMemoryRequirementsReply;

#define sz_xAveriusGetBufferMemoryRequirementsReply 32

#endif