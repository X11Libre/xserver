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
#endif