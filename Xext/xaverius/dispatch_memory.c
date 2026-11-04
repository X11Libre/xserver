/* SPDX-License-Identifier: MIT OR X11 */

#include "xaverius.h"
#include "xaverius_device.h"
#include "xaverius_proto.h"
#include "dispatch_memory.h"
#include "dix.h"
#include "resource.h"
#include "extnsionst.h"
#include "globals.h"
#include "dix/dix_priv.h"
#include "dix/request_priv.h"

static int xaverius_lookup_buffer(ClientPtr client, XID xid, XaveriusBufferPtr *out)
{
    void *result = NULL;
    int rc = dixLookupResourceByType(&result, xid, XaveriusBufferType, client, DixReadAccess);
    if (rc != Success)
        return BadValue;
    *out = (XaveriusBufferPtr)result;
    if (!*out)
        return BadMatch;
    return Success;
}

static int xaverius_lookup_memory(ClientPtr client, XID xid, XaveriusMemoryPtr *out)
{
    void *result = NULL;
    int rc = dixLookupResourceByType(&result, xid, XaveriusMemoryType, client, DixReadAccess);
    if (rc != Success)
        return BadValue;
    *out = (XaveriusMemoryPtr)result;
    if (!*out)
        return BadMatch;
    return Success;
}

int ProcXAveriusCreateBuffer(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)payload;
    XID buffer_xid = *(XID *)(payload + 4);
    uint64_t size = *(uint64_t *)(payload + 8);
    uint32_t usage_flags = *(uint32_t *)(payload + 16);
    uint32_t reserved = *(uint32_t *)(payload + 20);
    void *result = NULL;
    int rc;

    if (stuff->data != 0)
        return BadValue;

    if (client->req_len != 8)
        return BadLength;

    if (size == 0)
        return BadValue;
    if (usage_flags == 0)
        return BadValue;
    if (reserved != 0)
        return BadValue;

    rc = dixLookupResourceByType(&result, device_xid, XaveriusLogicalDeviceType, client, DixReadAccess);
    if (rc != Success)
        return BadValue;
    if (!result)
        return BadMatch;

    if (!ValidateXID(buffer_xid))
        return BadIDChoice;

    XaveriusBufferPtr existing;
    if (dixLookupResourceByType((void **)&existing, buffer_xid, XaveriusBufferType, client, DixWriteAccess) == Success)
        return BadIDChoice;

    XaveriusBufferPtr buffer = calloc(1, sizeof(XaveriusBufferRec));
    if (!buffer)
        return BadAlloc;

    buffer->xid = buffer_xid;
    buffer->device_xid = device_xid;
    buffer->size = size;
    buffer->usage_flags = usage_flags;
    buffer->bound_memory = NULL;
    buffer->memory_offset = 0;

    if (!AddResource(buffer_xid, XaveriusBufferType, buffer)) {
        free(buffer);
        return BadAlloc;
    }

    return Success;
}

int ProcXAveriusGetBufferMemoryRequirements(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)payload;
    XID buffer_xid = *(XID *)(payload + 4);
    XaveriusBufferPtr buffer;
    void *result = NULL;
    int rc;

    if (stuff->data != 0)
        return BadValue;

    if (client->req_len != 4)
        return BadLength;

    rc = dixLookupResourceByType(&result, device_xid, XaveriusLogicalDeviceType, client, DixReadAccess);
    if (rc != Success)
        return BadValue;
    if (!result)
        return BadMatch;

    rc = xaverius_lookup_buffer(client, buffer_xid, &buffer);
    if (rc != Success)
        return rc;

    if (buffer->device_xid != device_xid)
        return BadMatch;

    xAveriusGetBufferMemoryRequirementsReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .size = buffer->size,
        .alignment = 256,
        .memory_type_bits = 0xFFFFFFFF,
    };

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapll(&rep.size);
        swapll(&rep.alignment);
        swapl(&rep.memory_type_bits);
    }

    WriteToClient(client, sizeof(rep), &rep);
    return Success;
}

int ProcXAveriusAllocateMemory(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)payload;
    XID memory_xid = *(XID *)(payload + 4);
    uint64_t allocation_size = *(uint64_t *)(payload + 8);
    uint32_t memory_type_index = *(uint32_t *)(payload + 16);
    uint32_t reserved = *(uint32_t *)(payload + 20);
    void *result = NULL;
    int rc;

    if (stuff->data != 0)
        return BadValue;

    if (client->req_len != 8)
        return BadLength;

    if (allocation_size == 0)
        return BadValue;

    if (reserved != 0)
        return BadValue;

    rc = dixLookupResourceByType(&result, device_xid, XaveriusLogicalDeviceType, client, DixReadAccess);
    if (rc != Success)
        return BadValue;
    if (!result)
        return BadMatch;

    if (!ValidateXID(memory_xid))
        return BadIDChoice;

    XaveriusMemoryPtr existing;
    if (dixLookupResourceByType((void **)&existing, memory_xid, XaveriusMemoryType, client, DixWriteAccess) == Success)
        return BadIDChoice;

    XaveriusMemoryPtr memory = calloc(1, sizeof(XaveriusMemoryRec));
    if (!memory)
        return BadAlloc;

    memory->xid = memory_xid;
    memory->device_xid = device_xid;
    memory->size = allocation_size;
    memory->type_index = memory_type_index;

    if (!AddResource(memory_xid, XaveriusMemoryType, memory)) {
        free(memory);
        return BadAlloc;
    }

    return Success;
}

int ProcXAveriusFreeMemory(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)payload;
    XID memory_xid = *(XID *)(payload + 4);
    XaveriusMemoryPtr memory;
    void *result = NULL;
    int rc;

    if (stuff->data != 0)
        return BadValue;

    if (client->req_len != 4)
        return BadLength;

    rc = dixLookupResourceByType(&result, device_xid, XaveriusLogicalDeviceType, client, DixReadAccess);
    if (rc != Success)
        return BadValue;
    if (!result)
        return BadMatch;

    rc = xaverius_lookup_memory(client, memory_xid, &memory);
    if (rc != Success)
        return rc;

    if (memory->device_xid != device_xid)
        return BadMatch;

    if (memory->bound_buffer != NULL)
        return BadMatch;

    FreeResource(memory_xid, XaveriusMemoryType);
    return Success;
}

int ProcXAveriusBindBufferMemory(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)payload;
    XID buffer_xid = *(XID *)(payload + 4);
    XID memory_xid = *(XID *)(payload + 8);
    uint64_t memory_offset = *(uint64_t *)(payload + 12);
    XaveriusBufferPtr buffer;
    XaveriusMemoryPtr memory;
    int rc;

    if (stuff->data != 0)
        return BadValue;

    if (client->req_len != 7)
        return BadLength;

    rc = dixLookupResourceByType(NULL, device_xid, XaveriusLogicalDeviceType, client, DixReadAccess);
    if (rc != Success)
        return BadValue;

    rc = xaverius_lookup_buffer(client, buffer_xid, &buffer);
    if (rc != Success)
        return rc;

    rc = xaverius_lookup_memory(client, memory_xid, &memory);
    if (rc != Success)
        return rc;

    if (buffer->device_xid != device_xid || memory->device_xid != device_xid)
        return BadMatch;

    if (buffer->bound_memory != NULL)
        return BadMatch;

    if (memory_offset % 256 != 0)
        return BadMatch;

    if (memory_offset > memory->size)
        return BadMatch;

    buffer->bound_memory = memory;
    buffer->memory_offset = memory_offset;
    memory->bound_buffer = buffer;

    return Success;
}

int ProcXAveriusWriteBuffer(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)payload;
    XID buffer_xid = *(XID *)(payload + 4);
    uint64_t buffer_offset = *(uint64_t *)(payload + 8);
    uint32_t data_length = *(uint32_t *)(payload + 16);
    XaveriusBufferPtr buffer;
    void *result = NULL;
    int rc;

    if (stuff->data != 0)
        return BadValue;

    if (client->req_len < 7)
        return BadLength;

    if (data_length == 0)
        return BadValue;

    rc = dixLookupResourceByType(&result, device_xid, XaveriusLogicalDeviceType, client, DixReadAccess);
    if (rc != Success)
        return BadValue;
    if (!result)
        return BadMatch;

    rc = xaverius_lookup_buffer(client, buffer_xid, &buffer);
    if (rc != Success)
        return rc;

    if (buffer->device_xid != device_xid)
        return BadMatch;

    if (!buffer->bound_memory)
        return BadMatch;

    if (!(buffer->usage_flags & 0x02))
        return BadMatch;

    if (buffer_offset > buffer->size)
        return BadMatch;

    if (data_length > buffer->size - buffer_offset)
        return BadMatch;

    return Success;
}

int ProcXAveriusReadBuffer(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)payload;
    XID buffer_xid = *(XID *)(payload + 4);
    uint64_t buffer_offset = *(uint64_t *)(payload + 8);
    uint32_t data_length = *(uint32_t *)(payload + 16);
    XaveriusBufferPtr buffer;
    void *result = NULL;
    int rc;

    if (stuff->data != 0)
        return BadValue;

    if (client->req_len != 7)
        return BadLength;

    if (data_length == 0)
        return BadValue;

    rc = dixLookupResourceByType(&result, device_xid, XaveriusLogicalDeviceType, client, DixReadAccess);
    if (rc != Success)
        return BadValue;
    if (!result)
        return BadMatch;

    rc = dixLookupResourceByType(&result, buffer_xid, XaveriusBufferType, client, DixReadAccess);
    if (rc != Success)
        return BadValue;

    buffer = (XaveriusBufferPtr)result;
    if (!buffer)
        return BadMatch;

    if (buffer->device_xid != device_xid)
        return BadMatch;

    if (!buffer->bound_memory)
        return BadMatch;

    if (!(buffer->usage_flags & 0x01))
        return BadMatch;

    if (buffer_offset > buffer->size)
        return BadMatch;

    if (data_length > buffer->size - buffer_offset)
        return BadMatch;

    return Success;
}

int ProcXAveriusShaderModuleData(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)payload;
    XID shader_module_xid = *(XID *)(payload + 4);
    uint32_t data_length = *(uint32_t *)(payload + 8);
    uint8_t flags = ((CARD8 *)client->requestBuffer)[1];
    void *result = NULL;
    int rc;

    if (stuff->data != 0)
        return BadValue;

    if (client->req_len < 5)
        return BadLength;

    if (flags & XAVERIUS_CMD_DATA_FLAG_ABORT) {
        if (data_length != 0)
            return BadValue;
    } else {
        if (data_length == 0)
            return BadValue;
    }

    if (flags & ~(XAVERIUS_CMD_DATA_FLAG_CREATE | XAVERIUS_CMD_DATA_FLAG_FINAL | XAVERIUS_CMD_DATA_FLAG_ABORT))
        return BadValue;

    (void)shader_module_xid;

    rc = dixLookupResourceByType(&result, device_xid, XaveriusLogicalDeviceType, client, DixReadAccess);
    if (rc != Success)
        return BadValue;
    if (!result)
        return BadMatch;

    return BadValue;
}

int ProcXAveriusCreateCommandPool(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)payload;
    XID command_pool_xid = *(XID *)(payload + 4);
    uint32_t queue_family_index = *(uint32_t *)(payload + 8);
    uint32_t flags = *(uint32_t *)(payload + 12);
    void *result = NULL;
    int rc;

    if (stuff->data != 0)
        return BadValue;

    if (client->req_len != 6)
        return BadLength;

    if (flags & ~0x03)
        return BadValue;

    if (queue_family_index >= 32)
        return BadValue;

    if (!ValidateXID(command_pool_xid))
        return BadIDChoice;

    rc = dixLookupResourceByType(&result, device_xid, XaveriusLogicalDeviceType, client, DixReadAccess);
    if (rc != Success)
        return BadValue;
    if (!result)
        return BadMatch;

    XaveriusCommandPoolPtr pool = calloc(1, sizeof(XaveriusCommandPoolRec));
    if (!pool)
        return BadAlloc;

    pool->xid = command_pool_xid;
    pool->device_xid = device_xid;
    pool->queue_family_index = queue_family_index;
    pool->flags = flags;
    pool->command_buffers = NULL;
    pool->command_buffer_count = 0;

    if (!AddResource(command_pool_xid, XaveriusCommandPoolType, pool)) {
        free(pool);
        return BadAlloc;
    }

    return Success;
}
