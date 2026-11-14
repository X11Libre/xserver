/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * Xaverius QueryVersion protocol contract tests.
 * Exercises the real dispatch path (ProcXaveriusDispatch) and the
 * request parser, not only a direct helper call.
 */
#undef NDEBUG

#include <dix-config.h>

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include <X11/X.h>
#include <X11/Xproto.h>

#include "dix/request_priv.h"
#include "tests.h"

#include "xaverius_intern.h"
#include "xaverius_proto.h"

/* Intercept dixWriteToClient so the success-path reply can be inspected.
 * Verifies the QueryVersion reply layout per wire-protocol.md §36-48:
 * 32-byte standard reply, reply.length=0, major_version=0, minor_version=1,
 * reserved bytes 12–31 all zero. */
DECLARE_WRAP_FUNCTION(dixWriteToClient, void, ClientPtr, int, void *);

static XaveriusQueryVersionReply captured_reply;
static bool reply_captured;

static void
check_dixWriteToClient(ClientPtr client, int len, void *data)
{
    (void)client;
    (void)len;

    if (len == sizeof(XaveriusQueryVersionReply)) {
        memcpy(&captured_reply, data, sizeof(captured_reply));
        reply_captured = true;
    }
}

/* Build a minimal 8-byte XAverius QueryVersion request header. */
static XaveriusReqHeader
make_qv_header(CARD8 subopcode, CARD8 flags, CARD16 reserved, CARD8 data)
{
    XaveriusReqHeader h = { 0 };

    h.reqType = 128;   /* dynamic major opcode; test placeholder */
    h.data = data;
    h.length = sz_XaveriusQueryVersionReq / 4;  /* exactly 8 bytes */
    h.subopcode = subopcode;
    h.flags = flags;
    h.reserved = reserved;
    return h;
}

/* Dispatch an 8-byte request through the real XAverius dispatch entry point.
 * Returns the ProcXaveriusDispatch result code. */
static int
dispatch_qv(XaveriusReqHeader h)
{
    ClientRec client = { 0 };

    client.index = 1;
    client.requestBuffer = &h;
    client.req_len = sz_XaveriusQueryVersionReq / 4;

    reply_captured = false;
    wrapped_dixWriteToClient = check_dixWriteToClient;
    return ProcXaveriusDispatch(&client);
}

static void
xaverius_qv_subopcode_zero_routes_to_queryversion(void)
{
    XaveriusReqHeader h = make_qv_header(XAVERIUS_REQ_QUERY_VERSION, 0, 0, 0);
    int rc = dispatch_qv(h);
    assert(rc == Success);
    assert(reply_captured);
    assert(captured_reply.response_type == X_Reply);
    assert(captured_reply.data == 0);
    assert(captured_reply.length == 0);
    assert(captured_reply.major_version == XAVERIUS_MAJOR_VERSION);
    assert(captured_reply.minor_version == XAVERIUS_MINOR_VERSION);
    /* reserved[20] bytes 12–31 must all be zero per wire-protocol.md §48 */
    for (size_t i = 0; i < sizeof(captured_reply.reserved); i++)
        assert(captured_reply.reserved[i] == 0);
}

static void
xaverius_qv_subopcode_forty_eight_does_not_route_to_queryversion(void)
{
    XaveriusReqHeader h = make_qv_header(48, 0, 0, 0);
    int rc = dispatch_qv(h);
    assert(rc == BadRequest);
}

static void
xaverius_qv_unsupported_subopcode_returns_badrequest(void)
{
    XaveriusReqHeader h = make_qv_header(255, 0, 0, 0);
    int rc = dispatch_qv(h);
    assert(rc == BadRequest);
}

static void
xaverius_qv_nonzero_data_returns_badvalue(void)
{
    XaveriusReqHeader h = make_qv_header(XAVERIUS_REQ_QUERY_VERSION, 0, 0, 1);
    int rc = dispatch_qv(h);
    assert(rc == BadValue);
}

static void
xaverius_qv_nonzero_flags_returns_badvalue(void)
{
    XaveriusReqHeader h = make_qv_header(XAVERIUS_REQ_QUERY_VERSION, 1, 0, 0);
    int rc = dispatch_qv(h);
    assert(rc == BadValue);
}

static void
xaverius_qv_nonzero_reserved_returns_badvalue(void)
{
    XaveriusReqHeader h = make_qv_header(XAVERIUS_REQ_QUERY_VERSION, 0, 1, 0);
    int rc = dispatch_qv(h);
    assert(rc == BadValue);
}

static void
xaverius_qv_wrong_length_returns_badlength(void)
{
    XaveriusReqHeader h = make_qv_header(XAVERIUS_REQ_QUERY_VERSION, 0, 0, 0);
    h.length = (sz_XaveriusQueryVersionReq / 4) + 1; /* 9-byte claim */
    int rc = dispatch_qv(h);
    assert(rc == BadLength);
}

const testfunc_t*
xaverius_qv_protocol_test(void)
{
    static const testfunc_t testfuncs[] = {
        xaverius_qv_subopcode_zero_routes_to_queryversion,
        xaverius_qv_subopcode_forty_eight_does_not_route_to_queryversion,
        xaverius_qv_unsupported_subopcode_returns_badrequest,
        xaverius_qv_nonzero_data_returns_badvalue,
        xaverius_qv_nonzero_flags_returns_badvalue,
        xaverius_qv_nonzero_reserved_returns_badvalue,
        xaverius_qv_wrong_length_returns_badlength,
        NULL,
    };

    return testfuncs;
}
