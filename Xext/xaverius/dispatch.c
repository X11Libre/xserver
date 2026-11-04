/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include <X11/X.h>

#include "include/xlibre_ptrtypes.h"
#include "dix/request_priv.h"

#include "xaverius_intern.h"
#include "xaverius_proto.h"

static int ProcXaveriusQueryVersion(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xavQueryVersionReq);

    /* Common-header invariants: data and reserved must be zero. */
    if (stuff->header.data != 0 || stuff->header.reserved != 0)
        return BadValue;

    xavQueryVersionReply rep = {
        .major_version  = XAVERIUS_MAJOR_VERSION,
        .minor_version  = XAVERIUS_MINOR_VERSION,
    };

    X_SEND_REPLY_SIMPLE(client, rep);
    return Success;
}

int ProcXaveriusDispatch(ClientPtr client)
{
    REQUEST(xavReqHeader);

    switch (stuff->subopcode) {
    case XAV_REQ_QUERY_VERSION:
        return ProcXaveriusQueryVersion(client);

    case XAV_REQ_ENUMERATE_PHYSICAL_DEVICES:
        return ProcXaveriusEnumeratePhysicalDevices(client);

    case XAV_REQ_GET_PHYSICAL_DEVICE_PROPERTY:
        return ProcXaveriusGetPhysicalDeviceProperty(client);

    case XAV_REQ_GET_PHYSICAL_DEVICE_QUEUE_FAMILY_PROPERTIES:
        return ProcXaveriusGetPhysicalDeviceQueueFamilyProperties(client);

    default:
        return BadRequest;
    }
}
