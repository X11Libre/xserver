#include "xaverius.h"
#include "xaverius_device.h"
#include "xaverius_proto.h"
#include "dix.h"
#include "resource.h"
#include "extnsionst.h"
#include "globals.h"
#include "dixstruct.h"
#include "dix/dix_priv.h"
#include "dix/request_priv.h"
#include "os/fmt.h"
#include <string.h>

#define XAVERIUS_DEVICE_TYPE_PHYSICAL    1
#define XAVERIUS_DEVICE_TYPE_LOGICAL     2
#define XAVERIUS_DEVICE_TYPE_SURFACE     3
#define XAVERIUS_DEVICE_TYPE_SWAPCHAIN   4

#define ValidateXID(x) ((x) != 0)

typedef struct {
    XID xid;
    uint32_t type;
    XID physical_device_xid;
    uint32_t *queue_family_indices;
    uint32_t queue_family_count;
    float *queue_priorities;
    void *vulkan_device;
} XaveriusLogicalDeviceRec, *XaveriusLogicalDevicePtr;

typedef struct {
    XID xid;
    uint32_t vendor_id;
    uint32_t device_id;
    uint32_t device_type;
    uint32_t api_version;
    uint32_t driver_version;
    uint32_t compute_queue_family_count;
    char name[256];
    uint32_t *queue_family_indices;
    uint32_t queue_family_count;
} XaveriusPhysicalDeviceRec, *XaveriusPhysicalDevicePtr;

typedef struct {
    XID xid;
    uint32_t type;
    XID window_xid;
    XID owner_device_xid;
} XaveriusSurfaceRec, *XaveriusSurfacePtr;

typedef struct {
    XID xid;
    uint32_t type;
    XID surface_xid;
    uint32_t image_count;
    uint32_t format_id;
    uint32_t color_space_id;
    uint32_t width;
    uint32_t height;
    uint32_t present_mode_id;
    uint32_t image_usage_flags;
} XaveriusSwapchainRec, *XaveriusSwapchainPtr;

static RESTYPE XaveriusLogicalDeviceType = 0;
static RESTYPE XaveriusPhysicalDeviceType = 0;
static RESTYPE XaveriusSurfaceType = 0;
static RESTYPE XaveriusSwapchainType = 0;

#define XAVERIUS_MAX_PHYSICAL_DEVICES 4

static XaveriusPhysicalDeviceRec xaverius_physical_devices[XAVERIUS_MAX_PHYSICAL_DEVICES];
static int xaverius_physical_device_count = 0;

static void
xaverius_register_physical_device(XID xid, uint32_t vendor_id, uint32_t device_id,
                                  uint32_t api_version, uint32_t driver_version,
                                  uint32_t queue_family_count, const char *name)
{
    XaveriusPhysicalDevicePtr dev;

    if (xaverius_physical_device_count >= XAVERIUS_MAX_PHYSICAL_DEVICES)
        return;

    dev = &xaverius_physical_devices[xaverius_physical_device_count++];
    dev->xid = xid;
    dev->vendor_id = vendor_id;
    dev->device_id = device_id;
    dev->api_version = api_version;
    dev->driver_version = driver_version;
    dev->compute_queue_family_count = queue_family_count;
    dev->queue_family_indices = NULL;
    dev->queue_family_count = 0;
    strncpy(dev->name, name, sizeof(dev->name) - 1);
    dev->name[sizeof(dev->name) - 1] = '\0';

    AddResource(xid, XaveriusPhysicalDeviceType, dev);
}

int
ProcXAveriusEnumeratePhysicalDevices(ClientPtr client)
{
    xAveriusEnumeratePhysicalDevicesReply reply = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = bytes_to_int32(xaverius_physical_device_count *
                                 sz_xAveriusPhysicalDeviceInfo),
        .nDevices = xaverius_physical_device_count
    };
    int i;

    if (client->swapped) {
        swaps(&reply.sequenceNumber);
        swapl(&reply.length);
        swapl(&reply.nDevices);
    }
    dixWriteToClient(client, sz_xAveriusEnumeratePhysicalDevicesReply, &reply);

    for (i = 0; i < xaverius_physical_device_count; i++) {
        xAveriusPhysicalDeviceInfo info = {
            .physical_device_xid = xaverius_physical_devices[i].xid,
            .device_type = xaverius_physical_devices[i].device_type,
            .vendor_id = xaverius_physical_devices[i].vendor_id,
            .device_id = xaverius_physical_devices[i].device_id,
            .api_version = xaverius_physical_devices[i].api_version,
            .driver_version = xaverius_physical_devices[i].driver_version,
            .compute_queue_family_count = xaverius_physical_devices[i].compute_queue_family_count,
        };

        if (client->swapped) {
            swapl(&info.physical_device_xid);
            swapl(&info.device_type);
            swapl(&info.vendor_id);
            swapl(&info.device_id);
            swapl(&info.api_version);
            swapl(&info.driver_version);
            swapl(&info.compute_queue_family_count);
        }
        dixWriteToClient(client, sz_xAveriusPhysicalDeviceInfo, &info);
    }

    return Success;
}

int
ProcXAveriusGetPhysicalDeviceProperty(ClientPtr client)
{
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)payload;
    CARD32 property_id = *(CARD32 *)(payload + 4);
    xAveriusGetPhysicalDevicePropertyReply reply = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .property_id = property_id,
        .property_value = 0
    };
    XaveriusPhysicalDevicePtr dev = NULL;
    int i;

    if (!ValidateXID(device_xid))
        return BadValue;

    for (i = 0; i < xaverius_physical_device_count; i++) {
        if (xaverius_physical_devices[i].xid == device_xid) {
            dev = &xaverius_physical_devices[i];
            break;
        }
    }

    if (!dev)
        return BadValue;

    switch (property_id) {
    case 0:
        reply.property_value = dev->vendor_id;
        break;
    case 1:
        reply.property_value = dev->device_id;
        break;
    case 2:
        reply.property_value = dev->api_version;
        break;
    case 3:
        reply.property_value = dev->driver_version;
        break;
    case 4:
        reply.property_value = dev->compute_queue_family_count;
        break;
    default:
        reply.property_value = 0;
        break;
    }

    if (client->swapped) {
        swaps(&reply.sequenceNumber);
        swapl(&reply.length);
        swapl(&reply.property_id);
        swapl(&reply.property_value);
    }
    dixWriteToClient(client, sz_xAveriusGetPhysicalDevicePropertyReply, &reply);

    return Success;
}

int
ProcXAveriusGetPhysicalDeviceQueueFamilyProperties(ClientPtr client)
{
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)payload;
    xAveriusGetPhysicalDeviceQueueFamilyPropertiesReply reply = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .queue_family_index = 0,
        .queue_count = 0,
        .flags = 0
    };
    XaveriusPhysicalDevicePtr dev = NULL;
    int i;

    if (!ValidateXID(device_xid))
        return BadValue;

    for (i = 0; i < xaverius_physical_device_count; i++) {
        if (xaverius_physical_devices[i].xid == device_xid) {
            dev = &xaverius_physical_devices[i];
            break;
        }
    }

    if (!dev)
        return BadValue;

    reply.queue_family_index = 0;
    reply.queue_count = dev->compute_queue_family_count;
    reply.flags = 0;

    if (client->swapped) {
        swaps(&reply.sequenceNumber);
        swapl(&reply.length);
        swapl(&reply.queue_family_index);
        swapl(&reply.queue_count);
        swapl(&reply.flags);
    }
    dixWriteToClient(client, sz_xAveriusGetPhysicalDeviceQueueFamilyPropertiesReply,
                     &reply);

    return Success;
}

static int
xaverius_free_logical_device(void *value, XID id)
{
    XaveriusLogicalDevicePtr device = (XaveriusLogicalDevicePtr)value;

    if (device->queue_family_indices)
        free(device->queue_family_indices);
    if (device->queue_priorities)
        free(device->queue_priorities);

    free(device);
    return Success;
}

static int
xaverius_free_surface(void *value, XID id)
{
    XaveriusSurfacePtr surface = (XaveriusSurfacePtr)value;
    free(surface);
    return Success;
}

static int
xaverius_free_swapchain(void *value, XID id)
{
    XaveriusSwapchainPtr swapchain = (XaveriusSwapchainPtr)value;
    free(swapchain);
    return Success;
}

int
ProcXAveriusCreateDevice(ClientPtr client)
{
    REQUEST(xAveriusReq);

    if (stuff->data != 0)
        return BadValue;

    /* Fixed header (8 bytes) + fixed payload fields (24 bytes) = 32 bytes total */
    if (client->req_len < 32 / 4)
        return BadLength;

    /* Payload starts at offset 8 */
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)(payload + 0);
    XID physical_device_xid = *(XID *)(payload + 4);
    CARD16 queue_family_count = *(CARD16 *)(payload + 8);
    CARD16 reserved = *(CARD16 *)(payload + 14);

    if (reserved != 0)
        return BadValue;

    if (queue_family_count == 0)
        return BadValue;

    if (device_xid == 0 || physical_device_xid == 0)
        return BadValue;

    XaveriusLogicalDevicePtr existing;
    if (dixLookupResourceByType((void **)&existing, device_xid, XaveriusLogicalDeviceType, client, DixWriteAccess) == Success)
        return BadIDChoice;

    XaveriusLogicalDevicePtr new_device = calloc(1, sizeof(XaveriusLogicalDeviceRec));
    if (!new_device)
        return BadAlloc;

    new_device->xid = device_xid;
    new_device->type = XAVERIUS_DEVICE_TYPE_LOGICAL;
    new_device->physical_device_xid = physical_device_xid;

    if (!AddResource(device_xid, XaveriusLogicalDeviceType, new_device)) {
        free(new_device);
        return BadAlloc;
    }

    return Success;
}

int
ProcXAveriusCreateDefaultDevice(ClientPtr client)
{
    REQUEST(xAveriusReq);

    if (stuff->data != 0)
        return BadValue;

    /* Fixed header (8 bytes) + fixed payload (8 bytes) = 16 bytes total */
    if (client->req_len < 16 / 4)
        return BadLength;

    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)(payload + 0);
    CARD16 queue_count = *(CARD16 *)(payload + 4);
    CARD16 reserved = *(CARD16 *)(payload + 6);

    if (reserved != 0)
        return BadValue;

    if (queue_count == 0)
        return BadValue;

    if (!ValidateXID(device_xid))
        return BadIDChoice;

    XaveriusLogicalDevicePtr existing;
    if (dixLookupResourceByType((void **)&existing, device_xid, XaveriusLogicalDeviceType, client, DixWriteAccess) == Success)
        return BadIDChoice;

    XaveriusLogicalDevicePtr new_device = calloc(1, sizeof(XaveriusLogicalDeviceRec));
    if (!new_device)
        return BadAlloc;

    new_device->xid = device_xid;
    new_device->type = XAVERIUS_DEVICE_TYPE_LOGICAL;

    if (!AddResource(device_xid, XaveriusLogicalDeviceType, new_device)) {
        free(new_device);
        return BadAlloc;
    }

    return Success;
}

int
ProcXAveriusDestroyDevice(ClientPtr client)
{
    REQUEST(xAveriusReq);

    if (stuff->data != 0)
        return BadValue;

    /* Fixed header (8 bytes) + device XID (4 bytes) = 12 bytes total */
    if (client->req_len < 12 / 4)
        return BadLength;

    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)(payload + 0);

    if (!ValidateXID(device_xid))
        return BadValue;

    XaveriusLogicalDevicePtr device;
    if (dixLookupResourceByType((void **)&device, device_xid, XaveriusLogicalDeviceType, client, DixWriteAccess) != Success)
        return BadValue;

    if (device->type != XAVERIUS_DEVICE_TYPE_LOGICAL)
        return BadMatch;

    FreeResource(device_xid, XaveriusLogicalDeviceType);

    return Success;
}

void
xaverius_device_init(void)
{
    XaveriusLogicalDeviceType = CreateNewResourceType(xaverius_free_logical_device, "XAveriusLogicalDevice");
    XaveriusPhysicalDeviceType = CreateNewResourceType(NULL, "XAveriusPhysicalDevice");
    XaveriusSurfaceType = CreateNewResourceType(xaverius_free_surface, "XAveriusSurface");
    XaveriusSwapchainType = CreateNewResourceType(xaverius_free_swapchain, "XAveriusSwapchain");

    xaverius_register_physical_device(0x1001, 0x10DE, 0x2204, 0x00010000, 0x000F0000,
                                      1, "NVIDIA GV100");
    xaverius_register_physical_device(0x1002, 0x10DE, 0x2206, 0x00010000, 0x000F0000,
                                      2, "NVIDIA GV100");
    xaverius_register_physical_device(0x1003, 0x1002, 0x15D6, 0x00010006, 0x00170000,
                                      1, "AMD Radeon RX 6800");
    xaverius_register_physical_device(0x1004, 0x8086, 0x5668, 0x00020000, 0x000A0000,
                                      1, "Intel Iris Xe");
}

void
xaverius_device_fini(void)
{
    int i;

    if (XaveriusLogicalDeviceType)
        FreeResourceByType(0, XaveriusLogicalDeviceType, TRUE);
    if (XaveriusPhysicalDeviceType)
        FreeResourceByType(0, XaveriusPhysicalDeviceType, TRUE);
    if (XaveriusSurfaceType)
        FreeResourceByType(0, XaveriusSurfaceType, TRUE);
    if (XaveriusSwapchainType)
        FreeResourceByType(0, XaveriusSwapchainType, TRUE);

    for (i = 0; i < xaverius_physical_device_count; i++) {
        free(xaverius_physical_devices[i].queue_family_indices);
    }
    xaverius_physical_device_count = 0;
}

int
ProcXAveriusGetPhysicalDeviceMemoryProperties(ClientPtr client)
{
    REQUEST(xAveriusReq);

    if (stuff->data != 0)
        return BadValue;

    /* Fixed header (8 bytes) + physical-device XID (4 bytes) = 12 bytes total */
    if (client->req_len < 12 / 4)
        return BadLength;

    /* Physical-device XID at wire offset 8, right after the 8-byte header. */
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    CARD32 physical_device_xid = *(CARD32 *)(payload + 0);

    CARD32 memory_type_count = 0;
    CARD32 memory_heap_count = 0;

    if (!ValidateXID(physical_device_xid))
        return BadValue;

    /* Validate physical-device XID: unknown or non-live -> BadValue,
     * wrong live type -> BadMatch. */
    void *value;
    int rc = dixLookupResourceByType((void **)&value, physical_device_xid,
                                     XaveriusPhysicalDeviceType,
                                     client, DixReadAccess);
    if (rc != Success)
        return rc;

    size_t payload_len = 16
        + (size_t)memory_type_count * 8
        + (size_t)memory_heap_count * 16;

    xGenericReply reply = {
        .type = X_Reply,
        .sequenceNumber = (CARD16)client->sequence,
        .length = bytes_to_int32(payload_len),
    };

    if (client->swapped) {
        swaps(&reply.sequenceNumber);
        swapl(&reply.length);
    }

    dixWriteToClient(client, sizeof(reply), &reply);

    /* Fixed reply fields after the 32-byte header: */
    CARD32 header_payload[4] = {
        memory_type_count,
        memory_heap_count,
        0, /* memory_properties_flags */
        0, /* reserved */
    };

    if (client->swapped) {
        for (int i = 0; i < 4; i++)
            swapl(&header_payload[i]);
    }

    dixWriteToClient(client, sizeof(header_payload), header_payload);

    /* MEMORYTYPEINFO[] and MEMORYHEAPINFO[] are empty in v0.1 stub. */

    return Success;
}

int
ProcXAveriusCreateSurface(ClientPtr client)
{
    REQUEST(xAveriusReq);

    if (stuff->data != 0)
        return BadValue;

    /* Fixed header (8 bytes) + surface_xid + window_xid (8 bytes) = 16 bytes */
    if (client->req_len < 16 / 4)
        return BadLength;

    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    CARD32 surface_xid = *(CARD32 *)(payload + 0);
    CARD32 window_xid = *(CARD32 *)(payload + 4);

    if (client->swapped) {
        swapl(&surface_xid);
        swapl(&window_xid);
    }

    if (!ValidateXID(surface_xid))
        return BadIDChoice;

    /* Validate surface_xid as create-XID: not already occupied */
    void *existing;
    if (dixLookupResourceByType((void **)&existing, surface_xid,
                                XaveriusSurfaceType, client, DixWriteAccess) == Success)
        return BadIDChoice;

    /* Validate window_xid as a live core Window owned by this connection */
    void *window;
    int rc = dixLookupResourceByType((void **)&window, window_xid,
                                     XRT_WINDOW, client, DixReadAccess);
    if (rc != Success)
        return BadWindow;

    XaveriusSurfacePtr surface = calloc(1, sizeof(XaveriusSurfaceRec));
    if (!surface)
        return BadAlloc;

    surface->xid = surface_xid;
    surface->type = XAVERIUS_DEVICE_TYPE_SURFACE;
    surface->window_xid = window_xid;
    surface->owner_device_xid = 0; /* connection-owned, not device-owned */

    if (!AddResource(surface_xid, XaveriusSurfaceType, surface)) {
        free(surface);
        return BadAlloc;
    }

    return Success;
}

int
ProcXAveriusDestroySurface(ClientPtr client)
{
    REQUEST(xAveriusReq);

    if (stuff->data != 0)
        return BadValue;

    /* Fixed header (8 bytes) + surface_xid (4 bytes) = 12 bytes */
    if (client->req_len < 12 / 4)
        return BadLength;

    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    CARD32 surface_xid = *(CARD32 *)(payload + 0);

    if (client->swapped)
        swapl(&surface_xid);

    if (!ValidateXID(surface_xid))
        return BadValue;

    void *surface;
    int rc = dixLookupResourceByType((void **)&surface, surface_xid,
                                     XaveriusSurfaceType, client, DixReadAccess);
    if (rc != Success)
        return rc;

    FreeResource(surface_xid, XaveriusSurfaceType);
    return Success;
}

int
ProcXAveriusGetSurfaceCapabilities(ClientPtr client)
{
    REQUEST(xAveriusReq);

    if (stuff->data != 0)
        return BadValue;

    /* Fixed header (8 bytes) + device_xid + surface_xid (8 bytes) = 16 bytes */
    if (client->req_len < 16 / 4)
        return BadLength;

    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    CARD32 device_xid = *(CARD32 *)(payload + 0);
    CARD32 surface_xid = *(CARD32 *)(payload + 4);

    if (client->swapped) {
        swapl(&device_xid);
        swapl(&surface_xid);
    }

    if (!ValidateXID(device_xid) || !ValidateXID(surface_xid))
        return BadValue;

    void *device;
    int rc = dixLookupResourceByType((void **)&device, device_xid,
                                     XaveriusLogicalDeviceType, client, DixReadAccess);
    if (rc != Success)
        return rc;

    void *surface;
    rc = dixLookupResourceByType((void **)&surface, surface_xid,
                                 XaveriusSurfaceType, client, DixReadAccess);
    if (rc != Success)
        return rc;

    /* Stub: return success with zero capabilities (no presentation support yet) */
    xAveriusGetSurfaceCapabilitiesReply reply = {
        .type = X_Reply,
        .sequenceNumber = (CARD16)client->sequence,
        .length = bytes_to_int32(32), /* fixed part only, no variable records */
        .status = 0, /* SUCCESS */
        .present_supported = 0,
        .current_width = 0,
        .current_height = 0,
        .min_image_count = 0,
        .max_image_count = 0,
        .min_width = 0,
        .min_height = 0,
        .max_width = 0,
        .max_height = 0,
        .format_pair_count = 0,
        .present_mode_count = 0,
        .image_usage_flags = 0,
        .queue_family_count = 0,
    };

    if (client->swapped) {
        swaps(&reply.sequenceNumber);
        swapl(&reply.length);
        swapl(&reply.status);
        swapl(&reply.present_supported);
        swapl(&reply.current_width);
        swapl(&reply.current_height);
        swapl(&reply.min_image_count);
        swapl(&reply.max_image_count);
        swapl(&reply.min_width);
        swapl(&reply.min_height);
        swapl(&reply.max_width);
        swapl(&reply.max_height);
        swapl(&reply.format_pair_count);
        swapl(&reply.present_mode_count);
        swapl(&reply.image_usage_flags);
        swapl(&reply.queue_family_count);
    }

    dixWriteToClient(client, sizeof(reply), &reply);
    return Success;
}

int
ProcXAveriusCreateSwapchain(ClientPtr client)
{
    REQUEST(xAveriusReq);

    if (stuff->data != 0)
        return BadValue;

    /* Fixed header (8 bytes) + 10 CARD32 fields (40 bytes) = 48 bytes */
    if (client->req_len < 48 / 4)
        return BadLength;

    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    CARD32 device_xid = *(CARD32 *)(payload + 0);
    CARD32 swapchain_xid = *(CARD32 *)(payload + 4);
    CARD32 surface_xid = *(CARD32 *)(payload + 8);
    CARD32 image_count = *(CARD32 *)(payload + 12);
    CARD32 format_id = *(CARD32 *)(payload + 16);
    CARD32 color_space_id = *(CARD32 *)(payload + 20);
    CARD32 width = *(CARD32 *)(payload + 24);
    CARD32 height = *(CARD32 *)(payload + 28);
    CARD32 present_mode_id = *(CARD32 *)(payload + 32);
    CARD32 image_usage_flags = *(CARD32 *)(payload + 36);

    if (client->swapped) {
        swapl(&device_xid);
        swapl(&swapchain_xid);
        swapl(&surface_xid);
        swapl(&image_count);
        swapl(&format_id);
        swapl(&color_space_id);
        swapl(&width);
        swapl(&height);
        swapl(&present_mode_id);
        swapl(&image_usage_flags);
    }

    if (!ValidateXID(device_xid) || !ValidateXID(swapchain_xid) || !ValidateXID(surface_xid))
        return BadValue;

    if (width == 0 || height == 0)
        return BadValue;

    void *device;
    int rc = dixLookupResourceByType((void **)&device, device_xid,
                                     XaveriusLogicalDeviceType, client, DixReadAccess);
    if (rc != Success)
        return rc;

    void *surface;
    rc = dixLookupResourceByType((void **)&surface, surface_xid,
                                 XaveriusSurfaceType, client, DixReadAccess);
    if (rc != Success)
        return rc;

    /* Validate swapchain_xid as create-XID */
    void *existing;
    if (dixLookupResourceByType((void **)&existing, swapchain_xid,
                                XaveriusSwapchainType, client, DixWriteAccess) == Success)
        return BadIDChoice;

    /* Stub: no presentation support → UNSUPPORTED_CONFIGURATION status reply */
    xAveriusCreateSwapchainReply reply = {
        .type = X_Reply,
        .sequenceNumber = (CARD16)client->sequence,
        .length = bytes_to_int32(8),
        .status = XAVERIUS_SWAPCHAIN_UNSUPPORTED_CONFIG,
        .actual_image_count = 0,
    };

    if (client->swapped) {
        swaps(&reply.sequenceNumber);
        swapl(&reply.length);
        swapl(&reply.status);
        swapl(&reply.actual_image_count);
    }

    dixWriteToClient(client, sizeof(reply), &reply);
    return Success;
}

int
ProcXAveriusDestroySwapchain(ClientPtr client)
{
    REQUEST(xAveriusReq);

    if (stuff->data != 0)
        return BadValue;

    /* Fixed header (8 bytes) + swapchain_xid (4 bytes) = 12 bytes */
    if (client->req_len < 12 / 4)
        return BadLength;

    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    CARD32 swapchain_xid = *(CARD32 *)(payload + 0);

    if (client->swapped)
        swapl(&swapchain_xid);

    if (!ValidateXID(swapchain_xid))
        return BadValue;

    void *swapchain;
    int rc = dixLookupResourceByType((void **)&swapchain, swapchain_xid,
                                     XaveriusSwapchainType, client, DixReadAccess);
    if (rc != Success)
        return rc;

    /* Stub: no pending operations, accept destruction */
    FreeResource(swapchain_xid, XaveriusSwapchainType);
    return Success;
}

int
ProcXAveriusAcquireNextImage(ClientPtr client)
{
    REQUEST(xAveriusReq);

    if (stuff->data != 0)
        return BadValue;

    /* Fixed header (8 bytes) + 5 fields (24 bytes) = 32 bytes */
    if (client->req_len < 32 / 4)
        return BadLength;

    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    CARD32 device_xid = *(CARD32 *)(payload + 0);
    CARD32 swapchain_xid = *(CARD32 *)(payload + 4);
    CARD64 timeout_ns = *(CARD64 *)(payload + 8);
    CARD32 semaphore_xid = *(CARD32 *)(payload + 16);
    CARD32 fence_xid = *(CARD32 *)(payload + 20);

    if (client->swapped) {
        swapl(&device_xid);
        swapl(&swapchain_xid);
        swapll(&timeout_ns);
        swapl(&semaphore_xid);
        swapl(&fence_xid);
    }

    if (!ValidateXID(device_xid) || !ValidateXID(swapchain_xid))
        return BadValue;

    void *device;
    int rc = dixLookupResourceByType((void **)&device, device_xid,
                                     XaveriusLogicalDeviceType, client, DixReadAccess);
    if (rc != Success)
        return rc;

    void *swapchain;
    rc = dixLookupResourceByType((void **)&swapchain, swapchain_xid,
                                 XaveriusSwapchainType, client, DixReadAccess);
    if (rc != Success)
        return rc;

    /* Stub: no images available → NOT_READY status reply (zero timeout poll) */
    xAveriusAcquireNextImageReply reply = {
        .type = X_Reply,
        .sequenceNumber = (CARD16)client->sequence,
        .length = bytes_to_int32(12),
        .status = XAVERIUS_ACQUIRE_NOT_READY,
        .image_index = 0,
        .reserved = 0,
    };

    if (client->swapped) {
        swaps(&reply.sequenceNumber);
        swapl(&reply.length);
        swapl(&reply.status);
        swapl(&reply.image_index);
        swapl(&reply.reserved);
    }

    dixWriteToClient(client, sizeof(reply), &reply);
    return Success;
}

int
ProcXAveriusQueuePresent(ClientPtr client)
{
    REQUEST_AT_LEAST_SIZE(xAveriusReq);

    if (stuff->data != 0)
        return BadValue;

    /* Fixed header (8 bytes) + 7 fields (28 bytes) + wait semaphore XIDs */
    if (client->req_len < 40 / 4)
        return BadLength;

    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    CARD32 device_xid = *(CARD32 *)(payload + 0);
    CARD32 queue_family_index = *(CARD32 *)(payload + 4);
    CARD32 queue_index = *(CARD32 *)(payload + 8);
    CARD32 swapchain_xid = *(CARD32 *)(payload + 12);
    CARD32 image_index = *(CARD32 *)(payload + 16);
    CARD32 wait_semaphore_count = *(CARD32 *)(payload + 20);
    CARD64 presentation_token = *(CARD64 *)(payload + 24);

    if (client->swapped) {
        swapl(&device_xid);
        swapl(&queue_family_index);
        swapl(&queue_index);
        swapl(&swapchain_xid);
        swapl(&image_index);
        swapl(&wait_semaphore_count);
        swapll(&presentation_token);
    }

    if (!ValidateXID(device_xid) || !ValidateXID(swapchain_xid))
        return BadValue;

    if (presentation_token == 0)
        return BadValue;

    if (wait_semaphore_count > 0) {
        /* Variable-length wait semaphore XIDs at offset 40 */
        size_t needed = 40 + (size_t)wait_semaphore_count * 4;
        if ((size_t)(client->req_len << 2) < needed)
            return BadLength;

        /* Check for duplicate semaphore XIDs */
        for (uint32_t i = 0; i < wait_semaphore_count; i++) {
            CARD32 sem_xid;
            memcpy(&sem_xid, payload + 28 + i * 4, 4);
            if (client->swapped)
                swapl(&sem_xid);
            for (uint32_t j = i + 1; j < wait_semaphore_count; j++) {
                CARD32 sem_xid2;
                memcpy(&sem_xid2, payload + 28 + j * 4, 4);
                if (client->swapped)
                    swapl(&sem_xid2);
                if (sem_xid == sem_xid2)
                    return BadValue;
            }
        }
    }

    void *device;
    int rc = dixLookupResourceByType((void **)&device, device_xid,
                                     XaveriusLogicalDeviceType, client, DixReadAccess);
    if (rc != Success)
        return rc;

    void *swapchain;
    rc = dixLookupResourceByType((void **)&swapchain, swapchain_xid,
                                 XaveriusSwapchainType, client, DixReadAccess);
    if (rc != Success)
        return rc;

    /* Stub: no presentation support → OUT_OF_DATE status reply */
    xAveriusQueuePresentReply reply = {
        .type = X_Reply,
        .sequenceNumber = (CARD16)client->sequence,
        .length = bytes_to_int32(4),
        .status = XAVERIUS_PRESENT_OUT_OF_DATE,
    };

    if (client->swapped) {
        swaps(&reply.sequenceNumber);
        swapl(&reply.length);
        swapl(&reply.status);
    }

    dixWriteToClient(client, sizeof(reply), &reply);
    return Success;
}