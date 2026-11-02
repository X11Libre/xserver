/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.>

#include "xaverius.h"
#include "xaverius_device.h"
#include "xaverius_proto.h"
#include "dix.h"
#include "dixstruct.h"

extern int ProcXAveriusDestroyCommandPool(ClientPtr client);
extern int ProcXAveriusResetCommandBuffer(ClientPtr client);
extern int ProcXAveriusCommandBufferData(ClientPtr client);
extern int ProcXAveriusQueueSubmit(ClientPtr client);
extern int ProcXAveriusCreateSemaphore(ClientPtr client);
extern int ProcXAveriusGetSemaphoreCounterValue(ClientPtr client);
extern int ProcXAveriusWaitSemaphores(ClientPtr client);
extern int ProcXAveriusSignalSemaphore(ClientPtr client);
extern int ProcXAveriusDestroySemaphore(ClientPtr client);

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