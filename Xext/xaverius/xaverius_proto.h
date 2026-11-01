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

/* --- Presentation subopcodes 41-47 reply structs --- */

/* GetSurfaceCapabilities reply:
   Fixed part is the standard 32-byte X11 reply header (type, pad1,
   sequenceNumber, length) followed by fields at offsets 8..63.
   Variable payload follows at offset 64. */
typedef struct {
    BYTE type;
    BYTE pad1;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 status;
    CARD32 present_supported;
    CARD32 current_width;
    CARD32 current_height;
    CARD32 min_image_count;
    CARD32 max_image_count;
    CARD32 min_width;
    CARD32 min_height;
    CARD32 max_width;
    CARD32 max_height;
    CARD32 format_pair_count;
    CARD32 present_mode_count;
    CARD32 image_usage_flags;
    CARD32 queue_family_count;
} xAveriusGetSurfaceCapabilitiesReply;

#define sz_xAveriusGetSurfaceCapabilitiesReply 64

/* CreateSwapchain reply: status, actual_image_count after 32-byte header */
typedef struct {
    BYTE type;
    BYTE pad1;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 status;
    CARD32 actual_image_count;
} xAveriusCreateSwapchainReply;

#define sz_xAveriusCreateSwapchainReply 40

/* AcquireNextImage reply: status, image_index, reserved after 32-byte header */
typedef struct {
    BYTE type;
    BYTE pad1;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 status;
    CARD32 image_index;
    CARD32 reserved;
} xAveriusAcquireNextImageReply;

#define sz_xAveriusAcquireNextImageReply 44

/* QueuePresent reply: status after 32-byte header */
typedef struct {
    BYTE type;
    BYTE pad1;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 status;
} xAveriusQueuePresentReply;

#define sz_xAveriusQueuePresentReply 36

/* Variable-length payload records for GetSurfaceCapabilities */
typedef struct {
    CARD32 format_id;
    CARD32 color_space_id;
} xAveriusFormatColorSpacePair;

#define sz_xAveriusFormatColorSpacePair 8

typedef struct {
    CARD32 queue_family_index;
    CARD32 present_supported;
} xAveriusQueueFamilyPresentInfo;

#define sz_xAveriusQueueFamilyPresentInfo 8

#endif