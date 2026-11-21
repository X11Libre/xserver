#include "xaverius.h"
#include "xaverius_device.h"
#include "xaverius_proto.h"
#include "dix.h"
#include "resource.h"
#include "extnsionst.h"
#include "globals.h"
#include "dix/dix_priv.h"
#include "dix/request_priv.h"

int
ProcXAveriusCreateSemaphore(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)payload;
    XID semaphore_xid = *(XID *)(payload + 4);
    uint32_t semaphore_type = *(uint32_t *)(payload + 8);
    uint64_t initial_value = *(uint64_t *)(payload + 12);
    void *result = NULL;
    int rc;

    if (client->req_len != 7)
        return BadLength;

    if (semaphore_type > XAVERIUS_SEMAPHORE_TYPE_TIMELINE)
        return BadValue;
    if (semaphore_type == XAVERIUS_SEMAPHORE_TYPE_BINARY && initial_value != 0)
        return BadValue;

    rc = dixLookupResourceByType(&result, device_xid,
                                 XaveriusLogicalDeviceType, client,
                                 DixReadAccess);
    if (rc != Success)
        return BadValue;
    if (!result)
        return BadMatch;

    return Success;
}

int
ProcXAveriusGetSemaphoreCounterValue(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)payload;
    XID semaphore_xid = *(XID *)(payload + 4);
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

    rc = dixLookupResourceByType(&result, semaphore_xid,
                                 XaveriusSemaphoreType, client,
                                 DixReadAccess);
    if (rc != Success)
        return BadValue;
    if (!result)
        return BadMatch;

    return Success;
}

int
ProcXAveriusWaitSemaphores(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)payload;
    uint32_t semaphore_count = *(uint32_t *)(payload + 4);
    uint32_t wait_mode = *(uint32_t *)(payload + 8);
    uint64_t timeout_ns = *(uint64_t *)(payload + 12);
    uint32_t expected_len;
    void *result = NULL;
    int rc;

    if (client->req_len < 7)
        return BadLength;

    if (semaphore_count == 0)
        return BadValue;
    if (wait_mode > XAVERIUS_WAIT_MODE_ALL)
        return BadValue;

    expected_len = 7 + semaphore_count * 3;
    if ((uint32_t)client->req_len != expected_len)
        return BadLength;

    rc = dixLookupResourceByType(&result, device_xid,
                                 XaveriusLogicalDeviceType, client,
                                 DixReadAccess);
    if (rc != Success)
        return BadValue;
    if (!result)
        return BadMatch;

    return Success;
}

int
ProcXAveriusSignalSemaphore(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)payload;
    XID semaphore_xid = *(XID *)(payload + 4);
    void *result = NULL;
    int rc;

    if (client->req_len != 6)
        return BadLength;

    rc = dixLookupResourceByType(&result, device_xid,
                                 XaveriusLogicalDeviceType, client,
                                 DixReadAccess);
    if (rc != Success)
        return BadValue;
    if (!result)
        return BadMatch;

    rc = dixLookupResourceByType(&result, semaphore_xid,
                                 XaveriusSemaphoreType, client,
                                 DixReadAccess);
    if (rc != Success)
        return BadValue;
    if (!result)
        return BadMatch;

    return Success;
}

int
ProcXAveriusDestroySemaphore(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)payload;
    XID semaphore_xid = *(XID *)(payload + 4);
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

    rc = dixLookupResourceByType(&result, semaphore_xid,
                                 XaveriusSemaphoreType, client,
                                 DixReadAccess);
    if (rc != Success)
        return BadValue;
    if (!result)
        return BadMatch;

    return Success;
}