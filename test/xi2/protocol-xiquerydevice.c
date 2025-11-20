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

#include <stdint.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/extensions/XI2proto.h>
#include <X11/Xatom.h>

#include "miext/extinit_priv.h"
#include "os/osdep.h"
#include "Xi/handlers.h"

#include "inputstr.h"
#include "exglobals.h"
#include "scrnintstr.h"
#include "xkbsrv.h"

#include "xiquerydevice.h"

#include "protocol-common.h"

DECLARE_WRAP_FUNCTION(WriteToClient, void, ClientPtr client, int len, void *data);
/*
 * Protocol testing for XIQueryDevice request and reply.
 *
 * Test approach:
 * Wrap WriteToClient to intercept server's reply. ProcXIQueryDevice returns
 * data in two batches, once for the request, once for the trailing data
 * with the device information.
 * Repeatedly test with varying deviceids and check against data in reply.
 */

static struct test_data {
    int which_device;
    int num_devices_in_reply;
} test_data;

extern ClientRec client_window;

static void reply_XIQueryDevice_data(ClientPtr client, int len, void *data);

/* reply handling for the first bytes that constitute the reply */
static void
reply_XIQueryDevice(ClientPtr client, int len, void *data)
{
    xXIQueryDeviceReply *repptr = (xXIQueryDeviceReply *) data;
    xXIQueryDeviceReply reply = *repptr; /* copy so swapping doesn't touch the real reply */

    assert(len < 0xffff); /* suspicious size, swapping bug */

    if (client->swapped) {
        swapl(&reply.length);
        swaps(&reply.sequenceNumber);
        swaps(&reply.num_devices);
    }

    reply_check_defaults(&reply, len, XIQueryDevice);

    if (test_data.which_device == XIAllDevices)
        assert(reply.num_devices == devices.num_devices);
    else if (test_data.which_device == XIAllMasterDevices)
        assert(reply.num_devices == devices.num_master_devices);
    else
        assert(reply.num_devices == 1);

    test_data.num_devices_in_reply = reply.num_devices;

    wrapped_WriteToClient = reply_XIQueryDevice_data;
}

/* reply handling for the trailing bytes that constitute the device info */
static void
reply_XIQueryDevice_data(ClientPtr client, int len, void *data)
{
    int i, j;

    DeviceIntPtr dev;
    xXIDeviceInfo *info = (xXIDeviceInfo *) data;
    xXIAnyInfo *any;

    assert(len < 0xffff); /* suspicious size, swapping bug */

    for (i = 0; i < test_data.num_devices_in_reply; i++) {
        if (client->swapped) {
            swaps(&info->deviceid);
            swaps(&info->attachment);
            swaps(&info->use);
            swaps(&info->num_classes);
            swaps(&info->name_len);
        }

        if (test_data.which_device > XIAllMasterDevices)
            assert(info->deviceid == test_data.which_device);

        assert(info->deviceid >= 2);    /* 0 and 1 is reserved */

        switch (info->deviceid) {
        case 2:                /* VCP */
            dev = devices.vcp;
            assert(info->use == XIMasterPointer);
            assert(info->attachment == devices.vck->id);
            assert(info->num_classes == 3);     /* 2 axes + button */
            break;
        case 3:                /* VCK */
            dev = devices.vck;
            assert(info->use == XIMasterKeyboard);
            assert(info->attachment == devices.vcp->id);
            assert(info->num_classes == 1);
            break;
        case 4:                /* mouse */
            dev = devices.mouse;
            assert(info->use == XISlavePointer);
            assert(info->attachment == devices.vcp->id);
            assert(info->num_classes == 7);     /* 4 axes + button + 2 scroll */
            break;
        case 5:                /* keyboard */
            dev = devices.kbd;
            assert(info->use == XISlaveKeyboard);
            assert(info->attachment == devices.vck->id);
            assert(info->num_classes == 1);
            break;

        default:
            /* We shouldn't get here */
            assert(0);
            break;
        }
        assert(info->enabled == dev->enabled);
        assert(info->name_len == strlen(dev->name));
        assert(strncmp((char *) &info[1], dev->name, info->name_len) == 0);

        any =
            (xXIAnyInfo *) ((char *) &info[1] + ((info->name_len + 3) / 4) * 4);
        for (j = 0; j < info->num_classes; j++) {
            if (client->swapped) {
                swaps(&any->type);
                swaps(&any->length);
                swaps(&any->sourceid);
            }

            switch (info->deviceid) {
            case 3:            /* VCK and kbd have the same properties */
            case 5:
            {
                int k;
                xXIKeyInfo *ki = (xXIKeyInfo *) any;
                XkbDescPtr xkb = devices.vck->key->xkbInfo->desc;
                uint32_t *kc;

                if (client->swapped)
                    swaps(&ki->num_keycodes);

                assert(any->type == XIKeyClass);
                assert(ki->num_keycodes ==
                       (xkb->max_key_code - xkb->min_key_code + 1));
                assert(any->length == (2 + ki->num_keycodes));

                kc = (uint32_t *) &ki[1];
                for (k = 0; k < ki->num_keycodes; k++, kc++) {
                    if (client->swapped)
                        swapl(kc);

                    assert(*kc >= xkb->min_key_code);
                    assert(*kc <= xkb->max_key_code);
                }
                break;
            }
            case 4:
            {
                assert(any->type == XIButtonClass ||
                       any->type == XIValuatorClass ||
                       any->type == XIScrollClass);

                if (any->type == XIScrollClass) {
                    xXIScrollInfo *si = (xXIScrollInfo *) any;

                    if (client->swapped) {
                        swaps(&si->number);
                        swaps(&si->scroll_type);
                        swapl(&si->increment.integral);
                        swapl(&si->increment.frac);
                    }
                    assert(si->length == 6);
                    assert(si->number == 2 || si->number == 3);
                    if (si->number == 2) {
                        assert(si->scroll_type == XIScrollTypeVertical);
                        assert(!si->flags);
                    }
                    if (si->number == 3) {
                        assert(si->scroll_type == XIScrollTypeHorizontal);
                        assert(si->flags & XIScrollFlagPreferred);
                        assert(!(si->flags & ~XIScrollFlagPreferred));
                    }

                    assert(si->increment.integral == si->number);
                    /* protocol-common.c sets up increments of 2.4 and 3.5 */
                    assert(si->increment.frac > 0.3 * (1ULL << 32));
                    assert(si->increment.frac < 0.6 * (1ULL << 32));
                }

            }
                /* fall through */
            case 2:            /* VCP and mouse have the same properties except for scroll */
            {
                if (info->deviceid == 2)        /* VCP */
                    assert(any->type == XIButtonClass ||
                           any->type == XIValuatorClass);

                if (any->type == XIButtonClass) {
                    int l;
                    xXIButtonInfo *bi = (xXIButtonInfo *) any;

                    if (client->swapped)
                        swaps(&bi->num_buttons);

                    assert(bi->num_buttons == devices.vcp->button->numButtons);

                    l = 2 + bi->num_buttons +
                        bytes_to_int32(bits_to_bytes(bi->num_buttons));
                    assert(bi->length == l);
                }
                else if (any->type == XIValuatorClass) {
                    xXIValuatorInfo *vi = (xXIValuatorInfo *) any;

                    if (client->swapped) {
                        swaps(&vi->number);
                        swapl(&vi->label);
                        swapl(&vi->min.integral);
                        swapl(&vi->min.frac);
                        swapl(&vi->max.integral);
                        swapl(&vi->max.frac);
                        swapl(&vi->resolution);
                    }

                    assert(vi->length == 11);
                    assert(vi->number >= 0);
                    assert(vi->number < 4);
                    if (info->deviceid == 2)    /* VCP */
                        assert(vi->number < 2);

                    assert(vi->mode == XIModeRelative);
                    /* device was set up as relative, so standard
                     * values here. */
                    assert(vi->min.integral == -1);
                    assert(vi->min.frac == 0);
                    assert(vi->max.integral == -1);
                    assert(vi->max.frac == 0);
                    assert(vi->resolution == 0);
                }
            }
                break;
            }
            any = (xXIAnyInfo *) (((char *) any) + any->length * 4);
        }

        info = (xXIDeviceInfo *) any;
    }
}

static void
request_XIQueryDevice(struct test_data *querydata, int deviceid, int error)
{
    int rc;
    ClientRec client;
    xXIQueryDeviceReq request;

    request_init(&request, XIQueryDevice);
    client = init_client(request.length, &request);
    wrapped_WriteToClient = reply_XIQueryDevice;

    querydata->which_device = deviceid;

    request.deviceid = deviceid;
    rc = ProcXIQueryDevice(&client);
    assert(rc == error);

    if (rc != Success)
        assert(client.errorValue == deviceid);

    wrapped_WriteToClient = reply_XIQueryDevice;

    client.swapped = TRUE;
    swaps(&request.length);
    swaps(&request.deviceid);
    rc = ProcXIQueryDevice(&client);
    assert(rc == error);

    if (rc != Success)
        assert(client.errorValue == deviceid);
}

static void
test_XIQueryDevice(void)
{
    int i;
    xXIQueryDeviceReq request;

    init_simple();

    wrapped_WriteToClient = reply_XIQueryDevice;
    request_init(&request, XIQueryDevice);

    dbg("Testing XIAllDevices.\n");
    request_XIQueryDevice(&test_data, XIAllDevices, Success);
    dbg("Testing XIAllMasterDevices.\n");
    request_XIQueryDevice(&test_data, XIAllMasterDevices, Success);

    dbg("Testing existing device ids.\n");
    for (i = 2; i < 6; i++)
        request_XIQueryDevice(&test_data, i, Success);

    dbg("Testing non-existing device ids.\n");
    for (i = 6; i <= 0xFFFF; i++)
        request_XIQueryDevice(&test_data, i, BadDevice);
}

const testfunc_t*
protocol_xiquerydevice_test(void)
{
    static const testfunc_t testfuncs[] = {
        test_XIQueryDevice,
        NULL,
    };

    return testfuncs;
}
