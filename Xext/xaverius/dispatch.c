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
static int
ProcXaveriusQueryVersion(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(XaveriusQueryVersionReq);

    /* Common-header invariants: data, flags and reserved must be zero. */
    if (stuff->header.data != 0 || stuff->header.flags != 0 ||
        stuff->header.reserved != 0)
        return BadValue;

    /* Request-specific exact length: QueryVersion is exactly 8 bytes. */
    if (stuff->header.length != sz_XaveriusQueryVersionReq / 4)
        return BadLength;

    XaveriusQueryVersionReply rep = {
        .major_version  = XAVERIUS_MAJOR_VERSION,
        .minor_version  = XAVERIUS_MINOR_VERSION,
    };

    X_SEND_REPLY_SIMPLE(client, rep);
    return Success;
}

int ProcXaveriusDispatch(ClientPtr client)
{
    REQUEST(XaveriusReqHeader);

    switch (stuff->subopcode) {
    case XAVERIUS_REQ_QUERY_VERSION:
        return ProcXaveriusQueryVersion(client);

    default:
        return BadRequest;
    }
}
