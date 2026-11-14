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
#include <stdint.h>

#include <X11/X.h>
#include <X11/Xproto.h>

#include "dix/request_priv.h"
#include "tests.h"

#include "xaverius_intern.h"
#include "xaverius_proto.h"

/* Intercept dixWriteToClient so the success-path reply does not touch
 * an uninitialised client OS private area. */
DECLARE_WRAP_FUNCTION(dixWriteToClient, void, ClientPtr, int, void *);

static void
noop_dixWriteToClient(ClientPtr client, int len, void *data)
{
    (void)client;
    (void)len;
    (void)data;
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

    wrapped_dixWriteToClient = noop_dixWriteToClient;
    return ProcXaveriusDispatch(&client);
}

static void
xaverius_qv_subopcode_zero_routes_to_queryversion(void)
{
    XaveriusReqHeader h = make_qv_header(XAVERIUS_REQ_QUERY_VERSION, 0, 0, 0);
    int rc = dispatch_qv(h);
    assert(rc == Success);
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
