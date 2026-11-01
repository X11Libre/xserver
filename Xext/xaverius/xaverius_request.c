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
    case XAVERIUS_SUBOPCODE_CREATE_SURFACE:
        return ProcXAveriusCreateSurface(client);
    case XAVERIUS_SUBOPCODE_DESTROY_SURFACE:
        return ProcXAveriusDestroySurface(client);
    case XAVERIUS_SUBOPCODE_GET_SURFACE_CAPABILITIES:
        return ProcXAveriusGetSurfaceCapabilities(client);
    case XAVERIUS_SUBOPCODE_CREATE_SWAPCHAIN:
        return ProcXAveriusCreateSwapchain(client);
    case XAVERIUS_SUBOPCODE_DESTROY_SWAPCHAIN:
        return ProcXAveriusDestroySwapchain(client);
    case XAVERIUS_SUBOPCODE_ACQUIRE_NEXT_IMAGE:
        return ProcXAveriusAcquireNextImage(client);
    case XAVERIUS_SUBOPCODE_QUEUE_PRESENT:
        return ProcXAveriusQueuePresent(client);
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