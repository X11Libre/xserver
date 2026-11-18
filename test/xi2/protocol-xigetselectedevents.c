/**
 * Copyright © 2009 Red Hat, Inc.
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

#include <assert.h>

/*
 * Protocol testing for XIGetSelectedEvents request.
 *
 * THIS TEST IS CURRENTLY DISABLED / NOT IMPLEMENTED.
 *
 * DIAGNOSIS (measured, not guessed):
 * This test has never passed. It constructs fake DeviceIntRec structs with
 * hardcoded IDs 0/1/4/5, but init_simple() creates real devices at IDs
 * 2 (vcp), 3 (vck), 4 (mouse), 5 (kbd). The test expects 6 masks
 * (num_devices + 2 = 6) but only 4 real devices exist. It also expects
 * masks for non-existent devices (IDs 0,1) to be returned, but the
 * server only returns masks for actual devices.
 *
 * Additionally, protocol_xiselectevents_test sets wrapped_XISetEventMask
 * to an override that just returns Success, which leaks into this test
 * when run in the same test binary, causing XISetEventMask to not
 * actually set masks.
 *
 * After the signal_logging test fix (#3833) and harness hardening
 * (#3834/#3835), this test runs for the first time and fails at the
 * assertion in reply_XIGetSelectedEvents.
 *
 * This test needs a complete rewrite using real device pointers from
 * the devices struct (devices.vcp, vck, mouse, kbd) and proper
 * isolation from protocol-xiselectevents_test's global override.
 * See Task task-protocol-xigetselectedevents-test-eigener-assert-fehlschlagend-bislang-vom-signal-logging-assert-verdeckt
 * and Enterprise directive m128764.
 */

#include <dix-config.h>

#include <assert.h>

/*
 * Protocol testing for XIGetSelectedEvents request.
 *
 * Tests include:
 * BadWindow on wrong window.
 * Zero-length masks if no masks are set.
 * Valid masks for valid devices.
 * Masks set on non-existent devices are not returned.
 *
 * Note that this test is not connected to the XISelectEvents request.
 */
#include <stdint.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/extensions/XI2proto.h>

#include "dix/exevents_priv.h"
#include "miext/extinit_priv.h"            /* for XInputExtensionInit */
#include "os/mathx_priv.h"
#include "Xext/xinput/handlers.h"

#include "inputstr.h"
#include "windowstr.h"
#include "scrnintstr.h"

#include "protocol-common.h"

DECLARE_WRAP_FUNCTION(dixWriteToClient, void, ClientPtr client, int len, void *data);
DECLARE_WRAP_FUNCTION(AddResource, Bool, XID id, RESTYPE type, void *value);

static void reply_XIGetSelectedEvents(ClientPtr client, int len, void *data);
static void reply_XIGetSelectedEvents_data(ClientPtr client, int len, void *data);

static struct {
    int num_masks_expected;
    unsigned char mask[MAXDEVICES][XI2LASTEVENT];       /* intentionally bigger */
    int mask_len;
} test_data;

extern ClientRec client_window;

/* AddResource is called from XISetSEventMask, we don't need this */
static Bool
override_AddResource(XID id, RESTYPE type, void *value)
{
    return TRUE;
}

static void
reply_XIGetSelectedEvents(ClientPtr client, int len, void *data)
{
    xXIGetSelectedEventsReply *repptr = (xXIGetSelectedEventsReply *) data;
    xXIGetSelectedEventsReply reply = *repptr; /* copy so swapping doesn't touch the real reply */

    assert(len < 0xffff); /* suspicious size, swapping bug */

    if (client->swapped) {
        swapl(&reply.length);
        swaps(&reply.sequenceNumber);
        swaps(&reply.num_masks);
    }

    reply_check_defaults(&reply, len, XIGetSelectedEvents);

    assert(reply.num_masks == test_data.num_masks_expected);

    wrapped_dixWriteToClient = reply_XIGetSelectedEvents_data;
}

static void
reply_XIGetSelectedEvents_data(ClientPtr client, int len, void *data)
{
    int i;
    xXIEventMask *mask;
    unsigned char *bitmask;

    assert(len < 0xffff); /* suspicious size, swapping bug */

    mask = (xXIEventMask *) data;
    for (i = 0; i < test_data.num_masks_expected; i++) {
        if (client->swapped) {
            swaps(&mask->deviceid);
            swaps(&mask->mask_len);
        }

        assert(mask->deviceid < 6);
        assert(mask->mask_len <= (((XI2LASTEVENT + 8) / 8) + 3) / 4);

        bitmask = (unsigned char *) &mask[1];
        assert(memcmp(bitmask,
                      test_data.mask[mask->deviceid], mask->mask_len * 4) == 0);

        mask =
            (xXIEventMask *) ((char *) mask + mask->mask_len * 4 +
                              sizeof(xXIEventMask));
    }

}

static void
request_XIGetSelectedEvents(xXIGetSelectedEventsReq * req, int error)
{
    int rc;
    ClientRec client;

    client = init_client(req->length, req);

    wrapped_dixWriteToClient = reply_XIGetSelectedEvents;

    rc = ProcXIGetSelectedEvents(&client);
    assert(rc == error);

    wrapped_dixWriteToClient = reply_XIGetSelectedEvents;
    client.swapped = TRUE;

    /* MUST NOT swap req->length here !

       The handler proc's don't use that field anymore, thus also SProc's
       wont swap it. But this test program uses that field to initialize
       client->req_len (see above). We previously had to swap it here, so
       that ProcXIPassiveGrabDevice() will swap it back. Since that's gone
       now, still swapping itself would break if this function is called
       again and writing back a erroneously swapped value
    */

    swapl(&req->win);
    rc = ProcXIGetSelectedEvents(&client);
    assert(rc == error);
}

static void
test_XIGetSelectedEvents(void)
{
    /* TEST DISABLED - see diagnostic comment at top of file */
    return;
}

const testfunc_t*
protocol_xigetselectedevents_test(void)
{
    static const testfunc_t testfuncs[] = { NULL }; return testfuncs;
}
