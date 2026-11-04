#include "xaverius.h"
#include "xaverius_device.h"
#include "xaverius_proto.h"
#include "dix.h"
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
    case XAVERIUS_SUBOPCODE_ENUMERATE_PHYSICAL_DEVICES:
        return ProcXAveriusEnumeratePhysicalDevices(client);
    case XAVERIUS_SUBOPCODE_GET_PHYSICAL_DEVICE_PROPERTY:
        return ProcXAveriusGetPhysicalDeviceProperty(client);
    case XAVERIUS_SUBOPCODE_GET_PHYSICAL_DEVICE_QUEUE_FAMILY_PROPERTIES:
        return ProcXAveriusGetPhysicalDeviceQueueFamilyProperties(client);
    case XAVERIUS_SUBOPCODE_CREATE_DEVICE:
        return ProcXAveriusCreateDevice(client);
    case XAVERIUS_SUBOPCODE_CREATE_DEFAULT_DEVICE:
        return ProcXAveriusCreateDefaultDevice(client);
    case XAVERIUS_SUBOPCODE_DESTROY_DEVICE:
        return ProcXAveriusDestroyDevice(client);
    case XAVERIUS_SUBOPCODE_GET_PHYSICAL_DEVICE_MEMORY_PROPERTIES:
        return ProcXAveriusGetPhysicalDeviceMemoryProperties(client);
    case XAVERIUS_SUBOPCODE_CREATE_COMMAND_POOL:
        return ProcXAveriusCreateCommandPool(client);
    case XAVERIUS_SUBOPCODE_DESTROY_COMMAND_POOL:
        return ProcXAveriusDestroyCommandPool(client);
    case XAVERIUS_SUBOPCODE_RESET_COMMAND_BUFFER:
        return ProcXAveriusResetCommandBuffer(client);
    case XAVERIUS_SUBOPCODE_COMMAND_BUFFER_DATA:
        return ProcXAveriusCommandBufferData(client);
    case XAVERIUS_SUBOPCODE_QUEUE_SUBMIT:
        return ProcXAveriusQueueSubmit(client);
    case XAVERIUS_SUBOPCODE_CREATE_SEMAPHORE:
        return ProcXAveriusCreateSemaphore(client);
    case XAVERIUS_SUBOPCODE_GET_SEMAPHORE_COUNTER_VALUE:
        return ProcXAveriusGetSemaphoreCounterValue(client);
    case XAVERIUS_SUBOPCODE_WAIT_SEMAPHORES:
        return ProcXAveriusWaitSemaphores(client);
    case XAVERIUS_SUBOPCODE_SIGNAL_SEMAPHORE:
        return ProcXAveriusSignalSemaphore(client);
    case XAVERIUS_SUBOPCODE_DESTROY_SEMAPHORE:
        return ProcXAveriusDestroySemaphore(client);
    case XAVERIUS_SUBOPCODE_CREATE_FENCE:
        return ProcXAveriusCreateFence(client);
    case XAVERIUS_SUBOPCODE_GET_FENCE_STATUS:
        return ProcXAveriusGetFenceStatus(client);
    case XAVERIUS_SUBOPCODE_WAIT_FOR_FENCE:
        return ProcXAveriusWaitForFence(client);
    case XAVERIUS_SUBOPCODE_RESET_FENCE:
        return ProcXAveriusResetFence(client);
    case XAVERIUS_SUBOPCODE_DESTROY_FENCE:
        return ProcXAveriusDestroyFence(client);
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

    return ProcXAveriusDispatchInternal(client);
}

int
SProcXAveriusDispatch(ClientPtr client)
{
    REQUEST(xAveriusReq);

    if (stuff->data != 0)
        return BadValue;

    return ProcXAveriusDispatchInternal(client);
}