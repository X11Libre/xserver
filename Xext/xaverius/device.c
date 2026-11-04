/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * Xaverius physical device management handlers.
 * Uses global registry (xaverius_registry.c) for device lookup —
 * no Screen coupling, no hardcoded devices.
 */

#include "xaverius.h"
#include "xaverius_device.h"
#include "xaverius_proto.h"
#include "xaverius_intern.h"
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

RESTYPE XaveriusLogicalDeviceType = 0;
RESTYPE XaveriusPhysicalDeviceType = 0;
RESTYPE XaveriusCommandPoolType = 0;
RESTYPE XaveriusCommandBufferType = 0;
RESTYPE XaveriusSemaphoreType = 0;
RESTYPE XaveriusFenceType = 0;
RESTYPE XaveriusBufferType = 0;
RESTYPE XaveriusMemoryType = 0;

static int
xaverius_free_logical_device(void *value, XID id)
{
    XaveriusLogicalDevicePtr device = (XaveriusLogicalDevicePtr)value;

    if (device->queue_priorities)
        free(device->queue_priorities);

    free(device);
    return Success;
}

static int
xaverius_free_physical_device(void *value, XID id)
{
    XaveriusDevicePtr dev = (XaveriusDevicePtr)value;

    free(dev);
    return Success;
}

int ProcXAveriusEnumeratePhysicalDevices(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xAveriusEnumeratePhysicalDevicesReq);

    XaveriusDevicePtr dev;
    uint32_t count = 0;

    /* Count available devices first */
    xorg_list_for_each_entry(dev, &xaverius_devices, list) {
        if (dev->status == XAVERIUS_DEVICE_STATUS_AVAILABLE)
            count++;
    }

    xAveriusEnumeratePhysicalDevicesReply reply = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = bytes_to_int32(count * sz_xAveriusPhysicalDeviceInfo),
        .nDevices = count
    };

    if (client->swapped) {
        swaps(&reply.sequenceNumber);
        swapl(&reply.length);
        swapl(&reply.nDevices);
    }
    dixWriteToClient(client, sz_xAveriusEnumeratePhysicalDevicesReply, &reply);

    xorg_list_for_each_entry(dev, &xaverius_devices, list) {
        if (dev->status != XAVERIUS_DEVICE_STATUS_AVAILABLE)
            continue;

        const XaveriusDeviceCapabilities *caps = &dev->caps;
        if (!caps)
            continue;

        xAveriusPhysicalDeviceInfo info = {
            .physical_device_xid = dev->xid,
            .device_type = caps ? caps->device_type : XAVERIUS_DEVICE_TYPE_PHYSICAL,
            .vendor_id = caps->vendor_id,
            .device_id = caps->device_id,
            .api_version = caps->api_version,
            .driver_version = caps->driver_version,
            .compute_queue_family_count = caps->compute_queue_family_count,
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

int ProcXAveriusGetPhysicalDeviceProperty(ClientPtr client)
{
    X_REQUEST_HEAD_AT_LEAST(xAveriusGetPhysicalDevicePropertyReq);
    X_REQUEST_FIELD_CARD32(physical_device_xid);
    X_REQUEST_FIELD_CARD32(property_id);

    XaveriusDevicePtr dev = XaveriusFindDeviceByXid(stuff->physical_device_xid);
    if (!dev || dev->status != XAVERIUS_DEVICE_STATUS_AVAILABLE)
        return BadMatch;

    const XaveriusDeviceCapabilities *caps = &dev->caps;
    if (!caps)
        return BadImplementation;

    xAveriusGetPhysicalDevicePropertyReply reply = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .property_id = stuff->property_id,
        .property_value = 0
    };

    switch (stuff->property_id) {
    case 0: /* vendor_id */
        reply.property_value = caps->vendor_id;
        break;
    case 1: /* device_id */
        reply.property_value = caps->device_id;
        break;
    case 2: /* api_version */
        reply.property_value = caps->api_version;
        break;
    case 3: /* driver_version */
        reply.property_value = caps->driver_version;
        break;
    case 4: /* compute_queue_family_count */
        reply.property_value = caps->compute_queue_family_count;
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

int ProcXAveriusGetPhysicalDeviceQueueFamilyProperties(ClientPtr client)
{
    X_REQUEST_HEAD_AT_LEAST(xAveriusGetPhysicalDeviceQueueFamilyPropertiesReq);
    X_REQUEST_FIELD_CARD32(physical_device_xid);

    XaveriusDevicePtr dev = XaveriusFindDeviceByXid(stuff->physical_device_xid);
    if (!dev || dev->status != XAVERIUS_DEVICE_STATUS_AVAILABLE)
        return BadMatch;

    const XaveriusDeviceCapabilities *caps = &dev->caps;
    if (!caps || !caps->queue_families || caps->queue_family_count == 0)
        return BadImplementation;

    xAveriusGetPhysicalDeviceQueueFamilyPropertiesReply reply = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .queue_family_index = 0,
        .queue_count = caps->queue_family_count,
        .flags = 0
    };

    /* Write queue family records after the fixed reply */
    size_t records_len = caps->queue_family_count * sizeof(xavQueueFamilyInfo);
    reply.length = bytes_to_int32(records_len);

    if (client->swapped) {
        swaps(&reply.sequenceNumber);
        swapl(&reply.length);
        swapl(&reply.queue_family_index);
        swapl(&reply.queue_count);
        swapl(&reply.flags);
    }
    dixWriteToClient(client, sz_xAveriusGetPhysicalDeviceQueueFamilyPropertiesReply,
                     &reply);

    /* Write variable queue family records */
    for (uint32_t j = 0; j < caps->queue_family_count; j++) {
        xavQueueFamilyInfo info = {
            .queue_family_index = caps->queue_families[j].queue_family_index,
            .queue_flags = caps->queue_families[j].queue_flags,
            .queue_count = caps->queue_families[j].queue_count,
        };

        if (client->swapped) {
            swapl(&info.queue_family_index);
            swapl(&info.queue_flags);
            swapl(&info.queue_count);
        }
        dixWriteToClient(client, sizeof(xavQueueFamilyInfo), &info);
    }

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

    /* Validate physical device exists */
    XaveriusDevicePtr pdev = XaveriusFindDeviceByXid(physical_device_xid);
    if (!pdev || pdev->status != XAVERIUS_DEVICE_STATUS_AVAILABLE)
        return BadMatch;

    XaveriusLogicalDevicePtr existing;
    if (dixLookupResourceByType((void **)&existing, device_xid,
                                XaveriusLogicalDeviceType, client,
                                DixWriteAccess) == Success)
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
    if (dixLookupResourceByType((void **)&existing, device_xid,
                                XaveriusLogicalDeviceType, client,
                                DixWriteAccess) == Success)
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
    if (dixLookupResourceByType((void **)&device, device_xid,
                                XaveriusLogicalDeviceType, client,
                                DixWriteAccess) != Success)
        return BadValue;

    if (device->type != XAVERIUS_DEVICE_TYPE_LOGICAL)
        return BadMatch;

    FreeResource(device_xid, XaveriusLogicalDeviceType);

    return Success;
}

void
xaverius_device_init(void)
{
    XaveriusLogicalDeviceType = CreateNewResourceType(xaverius_free_logical_device,
                                                      "XAveriusLogicalDevice");
    XaveriusPhysicalDeviceType = CreateNewResourceType(xaverius_free_physical_device,
                                                       "XAveriusPhysicalDevice");
    XaveriusCommandPoolType = CreateNewResourceType(NULL, "XAveriusCommandPool");
    XaveriusCommandBufferType = CreateNewResourceType(NULL, "XAveriusCommandBuffer");
    XaveriusSemaphoreType = CreateNewResourceType(NULL, "XAveriusSemaphore");
    XaveriusFenceType = CreateNewResourceType(NULL, "XAveriusFence");
}

void
xaverius_device_fini(void)
{
    if (XaveriusLogicalDeviceType)
        FreeResourceByType(0, XaveriusLogicalDeviceType, TRUE);
    if (XaveriusPhysicalDeviceType)
        FreeResourceByType(0, XaveriusPhysicalDeviceType, TRUE);
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
