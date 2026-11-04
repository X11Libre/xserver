/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Xaverius physical device management handlers.
 */

#include <dix-config.h>

#include <stdbool.h>
#include <string.h>

#include "include/xlibre_ptrtypes.h"
#include "dix/dix_priv.h"
#include "dix/request_priv.h"
#include "dix/rpcbuf_priv.h"

#include "xaverius_priv.h"
#include "xaverius_proto.h"
#include "xaverius_intern.h"

int ProcXaveriusEnumeratePhysicalDevices(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xavEnumeratePhysicalDevicesReq);

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped };

    XaveriusDevicePtr dev;
    xorg_list_for_each_entry(dev, &xaverius_devices, list) {
        if (dev->status != XAVERIUS_DEVICE_STATUS_AVAILABLE)
            continue;

        const XaveriusDeviceCapabilities *caps = dev->callbacks;
        if (!caps)
            continue;

        x_rpcbuf_write_CARD32(&rpcbuf, dev->xid);
        x_rpcbuf_write_CARD32(&rpcbuf, caps->device_type);
        x_rpcbuf_write_CARD32(&rpcbuf, caps->vendor_id);
        x_rpcbuf_write_CARD32(&rpcbuf, caps->device_id);
        x_rpcbuf_write_CARD32(&rpcbuf, caps->api_version);
        x_rpcbuf_write_CARD32(&rpcbuf, caps->driver_version);
        x_rpcbuf_write_CARD32(&rpcbuf, caps->compute_queue_family_count);

        size_t nlen = caps->device_name ? strlen(caps->device_name) : 0;
        x_rpcbuf_write_CARD16(&rpcbuf, (CARD16)nlen);
        x_rpcbuf_write_CARD16(&rpcbuf, 0);
        if (caps->device_name)
            x_rpcbuf_write_string_pad(&rpcbuf, caps->device_name);
        else
            x_rpcbuf_pad(&rpcbuf);
    }

    xavEnumeratePhysicalDevicesReply rep = { 0 };
    X_SEND_REPLY_WITH_RPCBUF(client, rep, rpcbuf);
    x_rpcbuf_clear(&rpcbuf);
    return Success;
}

int ProcXaveriusGetPhysicalDeviceProperty(ClientPtr client)
{
    X_REQUEST_HEAD_AT_LEAST(xavGetPhysicalDevicePropertyReq);
    X_REQUEST_FIELD_CARD32(physical_device_xid);
    X_REQUEST_FIELD_CARD32(property_atom);

    XaveriusDevicePtr dev = XaveriusFindDeviceByXid(stuff->physical_device_xid);
    if (!dev || dev->status != XAVERIUS_DEVICE_STATUS_AVAILABLE)
        return BadMatch;

    const XaveriusDeviceCapabilities *caps = dev->callbacks;
    if (!caps)
        return BadImplementation;

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped };
    CARD32 property_type = 0, value_length = 0;
    bool found = false;

    if (stuff->property_atom == xav_atom_device_uuid) {
        value_length = 16;
        property_type = XAV_PROPERTY_BYTE_ARRAY;
        x_rpcbuf_write_binary_pad(&rpcbuf, (void *)calloc(1, 16), 16);
        found = true;
    }
    else if (stuff->property_atom == xav_atom_driver_uuid) {
        value_length = 16;
        property_type = XAV_PROPERTY_BYTE_ARRAY;
        x_rpcbuf_write_binary_pad(&rpcbuf, (void *)calloc(1, 16), 16);
        found = true;
    }
    else if (stuff->property_atom == xav_atom_max_work_group_invocations) {
        value_length = 4;
        property_type = XAV_PROPERTY_CARD32;
        CARD32 val = caps->max_work_group_invocations ? caps->max_work_group_invocations : 256;
        x_rpcbuf_write_CARD32(&rpcbuf, val);
        found = true;
    }
    else if (stuff->property_atom == xav_atom_max_work_group_size) {
        value_length = 12;
        property_type = XAV_PROPERTY_CARD32_ARRAY;
        CARD32 val[3] = {
            caps->max_work_group_size[0] ? caps->max_work_group_size[0] : 256,
            caps->max_work_group_size[1] ? caps->max_work_group_size[1] : 256,
            caps->max_work_group_size[2] ? caps->max_work_group_size[2] : 256
        };
        x_rpcbuf_write_binary_pad(&rpcbuf, (void *)val, 12);
        found = true;
    }
    else if (stuff->property_atom == xav_atom_timeline_semaphore) {
        value_length = 4;
        property_type = XAV_PROPERTY_CARD32;
        x_rpcbuf_write_CARD32(&rpcbuf, caps->has_timeline_semaphore ? 1 : 0);
        found = true;
    }

    if (!found)
        return BadImplementation;

    xavGetPhysicalDevicePropertyReply rep = {
        .status = XAV_PROPERTY_STATUS_SUCCESS,
        .property_type = property_type,
        .value_length = value_length,
    };
    X_SEND_REPLY_WITH_RPCBUF(client, rep, rpcbuf);
    x_rpcbuf_clear(&rpcbuf);
    return Success;
}

int ProcXaveriusGetPhysicalDeviceQueueFamilyProperties(ClientPtr client)
{
    X_REQUEST_HEAD_AT_LEAST(xavGetPhysicalDeviceQueueFamilyPropertiesReq);
    X_REQUEST_FIELD_CARD32(physical_device_xid);

    XaveriusDevicePtr dev = XaveriusFindDeviceByXid(stuff->physical_device_xid);
    if (!dev || dev->status != XAVERIUS_DEVICE_STATUS_AVAILABLE)
        return BadMatch;

    const XaveriusDeviceCapabilities *caps = dev->callbacks;
    if (!caps || !caps->queue_families || caps->queue_family_count == 0)
        return BadImplementation;

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped };

    CARD32 j;
    for (j = 0; j < caps->queue_family_count; j++) {
        x_rpcbuf_write_CARD32(&rpcbuf, caps->queue_families[j].queue_flags);
        x_rpcbuf_write_CARD32(&rpcbuf, caps->queue_families[j].queue_count);
        x_rpcbuf_write_CARD32(&rpcbuf, caps->queue_families[j].reserved);
    }

    xavGetPhysicalDeviceQueueFamilyPropertiesReply rep = { 0 };
    X_SEND_REPLY_WITH_RPCBUF(client, rep, rpcbuf);
    x_rpcbuf_clear(&rpcbuf);
    return Success;
}
