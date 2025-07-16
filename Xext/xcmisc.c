/*

Copyright 1993, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

#include <dix-config.h>

#include <stdint.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/extensions/xcmiscproto.h>

#include "dix/dix_priv.h"
#include "dix/resource_priv.h"
#include "dix/rpcbuf_priv.h"
#include "miext/extinit_priv.h"

#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "swaprep.h"

static int
ProcXCMiscGetVersion(ClientPtr client)
{
    REQUEST(xXCMiscGetVersionReq);
    REQUEST_SIZE_MATCH(xXCMiscGetVersionReq);

    if (client->swapped) {
        swaps(&stuff->majorVersion);
        swaps(&stuff->minorVersion);
    }

    xXCMiscGetVersionReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .majorVersion = XCMiscMajorVersion,
        .minorVersion = XCMiscMinorVersion
    };

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swaps(&rep.majorVersion);
        swaps(&rep.minorVersion);
    }
    WriteToClient(client, sizeof(xXCMiscGetVersionReply), &rep);
    return Success;
}

static int
ProcXCMiscGetXIDRange(ClientPtr client)
{
    REQUEST_SIZE_MATCH(xXCMiscGetXIDRangeReq);

    XID min_id, max_id;
    GetXIDRange(client->index, FALSE, &min_id, &max_id);

    xXCMiscGetXIDRangeReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .start_id = min_id,
        .count = max_id - min_id + 1
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.start_id);
        swapl(&rep.count);
    }
    WriteToClient(client, sizeof(xXCMiscGetXIDRangeReply), &rep);
    return Success;
}

static int
ProcXCMiscGetXIDList(ClientPtr client)
{
    REQUEST(xXCMiscGetXIDListReq);
    REQUEST_SIZE_MATCH(xXCMiscGetXIDListReq);

    if (client->swapped)
        swapl(&stuff->count);

    if (stuff->count > UINT32_MAX / sizeof(XID))
        return BadAlloc;

    XID *pids = calloc(stuff->count, sizeof(XID));
    if (!pids) {
        return BadAlloc;
    }

    size_t count = GetXIDList(client, stuff->count, pids);

    struct x_rpcbuf rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    x_rpcbuf_write_CARD32s(&rpcbuf, pids, count);
    free(pids);

    if (rpcbuf.error)
        return BadAlloc;

    xXCMiscGetXIDListReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = count,
        .count = count
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.count);
    }

    WriteToClient(client, sizeof(xXCMiscGetXIDListReply), &rep);
    WriteRpcbufToClient(client, &rpcbuf);
    return Success;
}

static int
ProcXCMiscDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
    case X_XCMiscGetVersion:
        return ProcXCMiscGetVersion(client);
    case X_XCMiscGetXIDRange:
        return ProcXCMiscGetXIDRange(client);
    case X_XCMiscGetXIDList:
        return ProcXCMiscGetXIDList(client);
    default:
        return BadRequest;
    }
}

static int _X_COLD
SProcXCMiscDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
    case X_XCMiscGetVersion:
        return ProcXCMiscGetVersion(client);
    case X_XCMiscGetXIDRange:
        return ProcXCMiscGetXIDRange(client);
    case X_XCMiscGetXIDList:
        return ProcXCMiscGetXIDList(client);
    default:
        return BadRequest;
    }
}

void
XCMiscExtensionInit(void)
{
    AddExtension(XCMiscExtensionName, 0, 0,
                 ProcXCMiscDispatch, SProcXCMiscDispatch,
                 NULL, StandardMinorOpcode);
}
