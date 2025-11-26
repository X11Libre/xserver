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
#include "Xi/handlers.h"

#include "inputstr.h"
#include "windowstr.h"
#include "scrnintstr.h"

#include "protocol-common.h"

DECLARE_WRAP_FUNCTION(WriteToClient, void, ClientPtr client, int len, void *data);
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
    xXIGetSelectedEventsReply *reply = (xXIGetSelectedEventsReply *) data;
    xXIGetSelectedEventsReply rep = *reply; /* copy so swapping doesn't touch the real reply */

    assert(len < 0xffff); /* suspicious size, swapping bug */

    if (client->swapped) {
        swapl(&rep.length);
        swaps(&rep.sequenceNumber);
        swaps(&rep.num_masks);
    }

    reply_check_defaults(&rep, len, XIGetSelectedEvents);

    assert(rep.num_masks == test_data.num_masks_expected);

    wrapped_WriteToClient = reply_XIGetSelectedEvents_data;
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

    wrapped_WriteToClient = reply_XIGetSelectedEvents;

    rc = ProcXIGetSelectedEvents(&client);
    assert(rc == error);

    wrapped_WriteToClient = reply_XIGetSelectedEvents;
    client.swapped = TRUE;

    /* MUST NOT swap req->length here !

       The handler proc's don't use that field anymore, thus also SProc's
       wont swap it. But this test program uses that field to initialize
       client->req_len (see above). We previously had to swap it here, so
       that ProcXIPassiveGrabDevice() will swap it back. Since that's gone
       now, still swapping itself would break if this function is called
       again and writing back a errornously swapped value
    */

    swapl(&req->win);
    rc = ProcXIGetSelectedEvents(&client);
    assert(rc == error);
}

static void
test_XIGetSelectedEvents(void)
{
    int i, j;
    xXIGetSelectedEventsReq request;
    ClientRec client;
    unsigned char *mask;
    DeviceIntRec dev;

    wrapped_AddResource = override_AddResource;

    init_simple();
    client = init_client(0, NULL);

    request_init(&request, XIGetSelectedEvents);

    dbg("Testing for BadWindow on invalid window.\n");
    request.win = None;
    request_XIGetSelectedEvents(&request, BadWindow);

    dbg("Testing for zero-length (unset) masks.\n");
    /* No masks set yet */
    test_data.num_masks_expected = 0;
    request.win = ROOT_WINDOW_ID;
    request_XIGetSelectedEvents(&request, Success);

    request.win = CLIENT_WINDOW_ID;
    request_XIGetSelectedEvents(&request, Success);

    memset(test_data.mask, 0, sizeof(test_data.mask));

    dbg("Testing for valid masks\n");
    memset(&dev, 0, sizeof(dev));       /* dev->id is enough for XISetEventMask */
    request.win = ROOT_WINDOW_ID;

    /* devices 6 - MAXDEVICES don't exist, they mustn't be included in the
     * reply even if a mask is set */
    for (j = 0; j < MAXDEVICES; j++) {
        test_data.num_masks_expected = MIN(j + 1, devices.num_devices + 2);
        dev.id = j;
        mask = test_data.mask[j];
        /* bits one-by-one */
        for (i = 0; i < XI2LASTEVENT; i++) {
            SetBit(mask, i);
            XISetEventMask(&dev, &root, &client, (i + 8) / 8, mask);
            request_XIGetSelectedEvents(&request, Success);
            ClearBit(mask, i);
        }

        /* all valid mask bits */
        for (i = 0; i < XI2LASTEVENT; i++) {
            SetBit(mask, i);
            XISetEventMask(&dev, &root, &client, (i + 8) / 8, mask);
            request_XIGetSelectedEvents(&request, Success);
        }
    }

    dbg("Testing removing all masks\n");
    /* Unset all masks one-by-one */
    for (j = MAXDEVICES - 1; j >= 0; j--) {
        if (j < devices.num_devices + 2)
            test_data.num_masks_expected--;

        mask = test_data.mask[j];
        memset(mask, 0, XI2LASTEVENT);

        dev.id = j;
        XISetEventMask(&dev, &root, &client, 0, NULL);

        request_XIGetSelectedEvents(&request, Success);
    }
}

const testfunc_t*
protocol_xigetselectedevents_test(void)
{
    static const testfunc_t testfuncs[] = {
        test_XIGetSelectedEvents,
        NULL,
    };
    return testfuncs;

    return 0;
}
