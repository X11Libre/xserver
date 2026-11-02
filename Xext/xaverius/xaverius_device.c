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
#include "extension.h"

#define XAVERIUS_DEVICE_TYPE_PHYSICAL    1
#define XAVERIUS_DEVICE_TYPE_LOGICAL     2

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

static RESTYPE XaveriusLogicalDeviceType = 0;
static RESTYPE XaveriusPhysicalDeviceType = 0;
static RESTYPE XaveriusCommandPoolType = 0;
static RESTYPE XaveriusCommandBufferType = 0;
static RESTYPE XaveriusSemaphoreType = 0;
static RESTYPE XaveriusFenceType = 0;

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
xaverius_free_physical_device(void *value, XID id)
{
    XaveriusPhysicalDevicePtr dev = (XaveriusPhysicalDevicePtr)value;

    free(dev);
    return Success;
}

int
xaverius_register_physical_device(XaveriusPhysicalDevicePtr dev)
{
    if (!AddResource(dev->physical_device_xid, XaveriusPhysicalDeviceType, dev))
        return BadAlloc;
    return Success;
}

XaveriusPhysicalDevicePtr
xaverius_lookup_physical_device(XID xid)
{
    XaveriusPhysicalDevicePtr dev;
    void *result = NULL;
    int rc = dixLookupResourceByType(&result, xid, XaveriusPhysicalDeviceType,
                                     NULL, DixReadAccess);
    if (rc != Success)
        return NULL;
    dev = (XaveriusPhysicalDevicePtr)result;
    return dev;
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
    XaveriusPhysicalDeviceType = CreateNewResourceType(xaverius_free_physical_device, "XAveriusPhysicalDevice");
    XaveriusCommandPoolType = CreateNewResourceType(NULL, "XAveriusCommandPool");
    XaveriusCommandBufferType = CreateNewResourceType(NULL, "XAveriusCommandBuffer");
    XaveriusSemaphoreType = CreateNewResourceType(NULL, "XAveriusSemaphore");
    XaveriusFenceType = CreateNewResourceType(NULL, "XAveriusFence");

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