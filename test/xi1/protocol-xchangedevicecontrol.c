/**
 * Copyright (c) 2014, Oracle and/or its affiliates.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* Test relies on assert() */
#undef NDEBUG

#include <dix-config.h>

/*
 * Protocol testing for ChangeDeviceControl request.
 */
#include <stdint.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/extensions/XIproto.h>
#include "inputstr.h"
#include "chgdctl.h"

#include "protocol-common.h"

DECLARE_WRAP_FUNCTION(WriteToClient, void, ClientPtr client, int len, void *data);

extern ClientRec client_window;
static ClientRec client_request;

static void
reply_ChangeDeviceControl(ClientPtr client, int len, void *data)
{
    xChangeDeviceControlReply *rep = (xChangeDeviceControlReply *) data;

    if (client->swapped) {
        swapl(&rep->length);
        swaps(&rep->sequenceNumber);
    }

    reply_check_defaults(rep, len, ChangeDeviceControl);

    /* XXX: check status code in reply */
}

static void
request_ChangeDeviceControl(ClientPtr client, xChangeDeviceControlReq * req,
                            xDeviceCtl *ctl, int error)
{
    int rc;

    client_request.req_len = req->length;
    rc = ProcXChangeDeviceControl(&client_request);
    assert(rc == error);

    /* XXX: ChangeDeviceControl doesn't seem to fill in errorValue to check */

    client_request.swapped = TRUE;
    swaps(&req->length);
    swaps(&req->control);
    swaps(&ctl->length);
    swaps(&ctl->control);
    /* XXX: swap other contents of ctl, depending on type */
    rc = ProcXChangeDeviceControl(&client_request);
    assert(rc == error);
}

static unsigned char *data[4096];       /* the request buffer */

static void
test_ChangeDeviceControl(void)
{
    init_simple();

    xChangeDeviceControlReq *request = (xChangeDeviceControlReq *) data;
    xDeviceCtl *control = (xDeviceCtl *) (&request[1]);

    request_init(request, ChangeDeviceControl);

    wrapped_WriteToClient  = reply_ChangeDeviceControl;

    client_request = init_client(request->length, request);

    dbg("Testing invalid lengths:\n");
    dbg(" -- no control struct\n");
    request_ChangeDeviceControl(&client_request, request, control, BadLength);

    dbg(" -- xDeviceResolutionCtl\n");
    request_init(request, ChangeDeviceControl);
    request->control = DEVICE_RESOLUTION;
    control->length = (sizeof(xDeviceResolutionCtl) >> 2);
    request->length += control->length - 2;
    request_ChangeDeviceControl(&client_request, request, control, BadLength);

    dbg(" -- xDeviceEnableCtl\n");
    request_init(request, ChangeDeviceControl);
    request->control = DEVICE_ENABLE;
    control->length = (sizeof(xDeviceEnableCtl) >> 2);
    request->length += control->length - 2;
    request_ChangeDeviceControl(&client_request, request, control, BadLength);

    /* XXX: Test functionality! */
}

const testfunc_t*
protocol_xchangedevicecontrol_test(void)
{
    static const testfunc_t testfuncs[] = {
        test_ChangeDeviceControl,
        NULL,
    };
    return testfuncs;
}
