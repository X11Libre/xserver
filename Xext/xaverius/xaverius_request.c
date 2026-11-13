/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.>

#include "xaverius.h"
#include "xaverius_compute.h"
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
    case XAVERIUS_SUBOPCODE_CREATE_DESCRIPTOR_SET_LAYOUT:
        return ProcXAveriusCreateDescriptorSetLayout(client);
    case XAVERIUS_SUBOPCODE_CREATE_DESCRIPTOR_POOL:
        return ProcXAveriusCreateDescriptorPool(client);
    case XAVERIUS_SUBOPCODE_DESTROY_DESCRIPTOR_POOL:
        return ProcXAveriusDestroyDescriptorPool(client);
    case XAVERIUS_SUBOPCODE_ALLOCATE_DESCRIPTOR_SETS:
        return ProcXAveriusAllocateDescriptorSets(client);
    case XAVERIUS_SUBOPCODE_UPDATE_DESCRIPTOR_SETS:
        return ProcXAveriusUpdateDescriptorSets(client);
    case XAVERIUS_SUBOPCODE_FREE_DESCRIPTOR_SETS:
        return ProcXAveriusFreeDescriptorSets(client);
    case XAVERIUS_SUBOPCODE_CREATE_PIPELINE_LAYOUT:
        return ProcXAveriusCreatePipelineLayout(client);
    case XAVERIUS_SUBOPCODE_CREATE_COMPUTE_PIPELINE:
        return ProcXAveriusCreateComputePipeline(client);
    case XAVERIUS_SUBOPCODE_CREATE_COMMAND_POOL:
        return ProcXAveriusCreateCommandPool(client);
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