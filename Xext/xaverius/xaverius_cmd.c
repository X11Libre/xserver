#include "xaverius.h"
#include "xaverius_device.h"
#include "xaverius_proto.h"
#include "dix.h"
#include "resource.h"
#include "extnsionst.h"
#include "globals.h"
#include "dix/dix_priv.h"
#include "dix/request_priv.h"

static int
xaverius_lookup_command_pool(ClientPtr client, XID xid,
                             XaveriusCommandPoolPtr *out)
{
    void *result = NULL;
    int rc = dixLookupResourceByType(&result, xid, XaveriusCommandPoolType,
                                     client, DixReadAccess);
    if (rc != Success)
        return BadValue;
    *out = (XaveriusCommandPoolPtr)result;
    if (!*out)
        return BadMatch;
    return Success;
}

static int
xaverius_lookup_command_buffer(ClientPtr client, XID xid,
                               XaveriusCommandBufferPtr *out)
{
    void *result = NULL;
    int rc = dixLookupResourceByType(&result, xid, XaveriusCommandBufferType,
                                     client, DixReadAccess);
    if (rc != Success)
        return BadValue;
    *out = (XaveriusCommandBufferPtr)result;
    if (!*out)
        return BadMatch;
    return Success;
}

int
ProcXAveriusDestroyCommandPool(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)payload;
    XID command_pool_xid = *(XID *)(payload + 4);
    XaveriusCommandPoolPtr pool;
    void *result = NULL;
    int rc;

    if (client->req_len != 4)
        return BadLength;

    rc = dixLookupResourceByType(&result, device_xid,
                                 XaveriusLogicalDeviceType, client,
                                 DixReadAccess);
    if (rc != Success)
        return BadValue;
    if (!result)
        return BadMatch;

    rc = xaverius_lookup_command_pool(client, command_pool_xid, &pool);
    if (rc != Success)
        return rc;

    if (pool->device_xid != device_xid)
        return BadMatch;

    return Success;
}

int
ProcXAveriusResetCommandBuffer(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)payload;
    XID command_buffer_xid = *(XID *)(payload + 4);
    XaveriusCommandBufferPtr buf;
    void *result = NULL;
    int rc;

    if (client->req_len != 4)
        return BadLength;

    rc = dixLookupResourceByType(&result, device_xid,
                                 XaveriusLogicalDeviceType, client,
                                 DixReadAccess);
    if (rc != Success)
        return BadValue;
    if (!result)
        return BadMatch;

    rc = xaverius_lookup_command_buffer(client, command_buffer_xid, &buf);
    if (rc != Success)
        return rc;

    if (buf->device_xid != device_xid)
        return BadMatch;

    return Success;
}

int
ProcXAveriusCommandBufferData(ClientPtr client)
{
    REQUEST(xAveriusReq);
    uint8_t flags;

    if (client->req_len < 4)
        return BadLength;

    flags = stuff->flags;

    if ((flags & XAVERIUS_CMD_DATA_FLAG_ABORT) &&
        (flags & ~XAVERIUS_CMD_DATA_FLAG_ABORT) != 0)
        return BadValue;

    if ((flags & (XAVERIUS_CMD_DATA_FLAG_CREATE |
                  XAVERIUS_CMD_DATA_FLAG_FINAL |
                  XAVERIUS_CMD_DATA_FLAG_ABORT)) == 0)
        return BadValue;

    return BadValue;
}