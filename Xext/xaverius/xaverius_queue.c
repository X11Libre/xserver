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
ProcXAveriusQueueSubmit(ClientPtr client)
{
    REQUEST(xAveriusReq);
    CARD8 *payload = (CARD8 *)client->requestBuffer + 8;
    XID device_xid = *(XID *)payload;
    uint32_t queue_family_index = *(uint32_t *)(payload + 4);
    uint32_t queue_index = *(uint32_t *)(payload + 8);
    XID fence_xid = *(XID *)(payload + 12);
    uint16_t cmd_buf_count = *(uint16_t *)(payload + 16);
    uint16_t wait_count = *(uint16_t *)(payload + 18);
    uint16_t signal_count = *(uint16_t *)(payload + 20);
    uint16_t reserved = *(uint16_t *)(payload + 22);
    uint64_t submission_token = *(uint64_t *)(payload + 24);
    uint32_t expected_len;
    void *result = NULL;
    int rc;

    if (client->req_len < 10)
        return BadLength;

    if (reserved != 0)
        return BadValue;
    if (submission_token == 0)
        return BadValue;

    expected_len = 10 + cmd_buf_count + wait_count * 4 + signal_count * 3;
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