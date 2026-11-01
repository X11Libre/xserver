#include "xaverius.h"
#include "xaverius_device.h"
#include "xaverius_proto.h"
#include "dixstruct.h"

static int
ProcXAveriusDispatchInternal(ClientPtr client)
{
    REQUEST(xAveriusReq);

    if (stuff->data != 0)
        return BadValue;

    if (!XAveriusExtensionMajorOpcode)
        return BadRequest;

    uint8_t subopcode = stuff->subopcode;

    switch (subopcode) {
    case XAVERIUS_SUBOPCODE_CREATE_DEVICE:
    case XAVERIUS_SUBOPCODE_CREATE_DEFAULT_DEVICE:
    case XAVERIUS_SUBOPCODE_DESTROY_DEVICE:
        return ProcXAveriusDispatchDeviceLifecycle(client);
    case XAVERIUS_SUBOPCODE_CREATE_SEMAPHORE:
    case XAVERIUS_SUBOPCODE_GET_SEMAPHORE_COUNTER_VALUE:
    case XAVERIUS_SUBOPCODE_WAIT_SEMAPHORES:
    case XAVERIUS_SUBOPCODE_SIGNAL_SEMAPHORE:
    case XAVERIUS_SUBOPCODE_DESTROY_SEMAPHORE:
    case XAVERIUS_SUBOPCODE_CREATE_FENCE:
    case XAVERIUS_SUBOPCODE_GET_FENCE_STATUS:
        return ProcXAveriusDispatchComputeSync(client);
    default:
        return BadRequest;
    }
}

int
ProcXAveriusDispatch(ClientPtr client)
{
    REQUEST(xAveriusReq);

    if (stuff->data != 0)
        return BadValue;

    if (!XAveriusExtensionMajorOpcode)
        return BadRequest;

    return ProcXAveriusDispatchInternal(client);
}

int
SProcXAveriusDispatch(ClientPtr client)
{
    REQUEST(xAveriusReq);

    if (stuff->data != 0)
        return BadValue;

    if (!XAveriusExtensionMajorOpcode)
        return BadRequest;

    return ProcXAveriusDispatchInternal(client);
}