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

#endif