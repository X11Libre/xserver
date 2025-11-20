/**
 * Copyright © 2011 Red Hat, Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice (including the next
 *  paragraph) shall be included in all copies or substantial portions of the
 *  Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 */

/* Test relies on assert() */
#undef NDEBUG

#include <dix-config.h>

/*
 * Protocol testing for XIPassiveGrab request.
 */
#include <stdint.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/extensions/XI2proto.h>

#include "dix/exevents_priv.h"
#include "os/osdep.h"
#include "Xi/handlers.h"

#include "inputstr.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "exglobals.h"

#include "protocol-common.h"

DECLARE_WRAP_FUNCTION(WriteToClient, void, ClientPtr client, int len, void *data);
DECLARE_WRAP_FUNCTION(GrabButton, int,
                      ClientPtr client, DeviceIntPtr dev,
                      DeviceIntPtr modifier_device, int button,
                      GrabParameters *param, enum InputLevel grabtype,
                      GrabMask *mask);

extern ClientRec client_window;
static ClientRec client_request;

#define N_MODS 7
static uint32_t modifiers[N_MODS] = { 1, 2, 3, 4, 5, 6, 7 };

static struct test_data {
    int num_modifiers;
} testdata;


static void reply_XIPassiveGrabDevice_data(ClientPtr client, int len,
                                           void *data);

static int
override_GrabButton(ClientPtr client, DeviceIntPtr dev,
                  DeviceIntPtr modifier_device, int button,
                  GrabParameters *param, enum InputLevel grabtype,
                  GrabMask *mask)
{
    /* Fail every odd modifier */
    if (param->modifiers % 2)
        return BadAccess;

    return Success;
}

static void
reply_XIPassiveGrabDevice(ClientPtr client, int len, void *data)
{
    xXIPassiveGrabDeviceReply *repptr = (xXIPassiveGrabDeviceReply *) data;
    xXIPassiveGrabDeviceReply reply = *repptr; /* copy so swapping doesn't touch the real reply */

    assert(len < 0xffff); /* suspicious size, swapping bug */

    if (client->swapped) {
        swaps(&reply.sequenceNumber);
        swapl(&reply.length);
        swaps(&reply.num_modifiers);

        testdata.num_modifiers = reply.num_modifiers;
    }

    reply_check_defaults(&reply, len, XIPassiveGrabDevice);

    /* ProcXIPassiveGrabDevice sends the data in two batches, let the second
     * handler handle the modifier data */
    if (reply.num_modifiers > 0)
        wrapped_WriteToClient = reply_XIPassiveGrabDevice_data;
}

static void
reply_XIPassiveGrabDevice_data(ClientPtr client, int len, void *data)
{
    int i;

    xXIGrabModifierInfo *mods = (xXIGrabModifierInfo *) data;

    assert(len < 0xffff); /* suspicious size, swapping bug */

    for (i = 0; i < testdata.num_modifiers; i++, mods++) {
        if (client->swapped)
            swapl(&mods->modifiers);

        /* 1 - 7 is the range we use for the global modifiers array
         * above */
        assert(mods->modifiers > 0);
        assert(mods->modifiers <= 7);
        assert(mods->modifiers % 2 == 1);       /* because we fail odd ones */
        assert(mods->status != Success);
        assert(mods->pad0 == 0);
        assert(mods->pad1 == 0);
    }

    wrapped_WriteToClient = reply_XIPassiveGrabDevice;
}

static void
request_XIPassiveGrabDevice(ClientPtr client, xXIPassiveGrabDeviceReq * req,
                            int error, int errval)
{
    int rc;
    int local_modifiers;
    int mask_len;

    client_request.req_len = req->length;
    client_request.swapped = FALSE;
    rc = ProcXIPassiveGrabDevice(&client_request);
    assert(rc == error);

    if (rc != Success)
        assert(client_request.errorValue == errval);

    client_request.swapped = TRUE;

    /* MUST NOT swap req->length here !

       The handler proc's don't use that field anymore, thus also SProc's
       wont swap it. But this test program uses that field to initialize
       client->req_len (see above). We previously had to swap it here, so
       that ProcXIPassiveGrabDevice() will swap it back. Since that's gone
       now, still swapping itself would break if this function is called
       again and writing back a errornously swapped value
    */

    swapl(&req->time);
    swapl(&req->grab_window);
    swapl(&req->cursor);
    swapl(&req->detail);
    swaps(&req->deviceid);
    local_modifiers = req->num_modifiers;
    swaps(&req->num_modifiers);
    mask_len = req->mask_len;
    swaps(&req->mask_len);

    while (local_modifiers--) {
        CARD32 *mod = (CARD32 *) (req + 1) + mask_len + local_modifiers;

        swapl(mod);
    }

    rc = ProcXIPassiveGrabDevice(&client_request);
    assert(rc == error);

    if (rc != Success)
        assert(client_request.errorValue == errval);
}

static unsigned char *data[4096];       /* the request buffer */
static void
test_XIPassiveGrabDevice(void)
{
    int i;
    xXIPassiveGrabDeviceReq *request = (xXIPassiveGrabDeviceReq *) data;
    unsigned char *mask;

    wrapped_GrabButton = override_GrabButton;

    init_simple();

    request_init(request, XIPassiveGrabDevice);

    request->grab_window = CLIENT_WINDOW_ID;

    wrapped_WriteToClient = reply_XIPassiveGrabDevice;
    client_request = init_client(request->length, request);

    dbg("Testing invalid device\n");
    request->deviceid = 12;
    request_XIPassiveGrabDevice(&client_request, request, BadDevice,
                                request->deviceid);

    dbg("Testing invalid length\n");
    request->length -= 2;
    request_XIPassiveGrabDevice(&client_request, request, BadLength,
                                client_request.errorValue);
    /* re-init request since swapped length test leaves some values swapped */
    request_init(request, XIPassiveGrabDevice);
    request->grab_window = CLIENT_WINDOW_ID;
    request->deviceid = XIAllMasterDevices;

    dbg("Testing invalid grab types\n");
    for (i = XIGrabtypeGestureSwipeBegin + 1; i < 0xFF; i++) {
        request->grab_type = i;
        request_XIPassiveGrabDevice(&client_request, request, BadValue,
                                    request->grab_type);
    }

    dbg("Testing invalid grab type + detail combinations\n");
    request->grab_type = XIGrabtypeEnter;
    request->detail = 1;
    request_XIPassiveGrabDevice(&client_request, request, BadValue,
                                request->detail);

    request->grab_type = XIGrabtypeFocusIn;
    request_XIPassiveGrabDevice(&client_request, request, BadValue,
                                request->detail);

    request->detail = 0;

    dbg("Testing invalid masks\n");
    mask = (unsigned char *) &request[1];

    request->mask_len = bytes_to_int32(XI2LASTEVENT + 1);
    request->length += request->mask_len;
    SetBit(mask, XI2LASTEVENT + 1);
    request_XIPassiveGrabDevice(&client_request, request, BadValue,
                                XI2LASTEVENT + 1);

    ClearBit(mask, XI2LASTEVENT + 1);

    /* tested all special cases now, test a few valid cases */

    /* no modifiers */
    request->deviceid = XIAllDevices;
    request->grab_type = XIGrabtypeButton;
    request->detail = XIAnyButton;
    request_XIPassiveGrabDevice(&client_request, request, Success, 0);

    /* Set a few random masks to make sure we handle modifiers correctly */
    SetBit(mask, XI_ButtonPress);
    SetBit(mask, XI_KeyPress);
    SetBit(mask, XI_Enter);

    /* some modifiers */
    request->num_modifiers = N_MODS;
    request->length += N_MODS;
    memcpy((uint32_t *) (request + 1) + request->mask_len, modifiers,
           sizeof(modifiers));
    request_XIPassiveGrabDevice(&client_request, request, Success, 0);
}

const testfunc_t*
protocol_xipassivegrabdevice_test(void)
{
    static const testfunc_t testfuncs[] = {
        test_XIPassiveGrabDevice,
        NULL,
    };

    return testfuncs;
}
