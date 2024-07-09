/*
 * Copyright © 2009 Red Hat, Inc.
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
 *
 * Author: Peter Hutterer
 */

/***********************************************************************
 *
 * Request to allow some device events.
 *
 */

#include <dix-config.h>

#include <X11/extensions/XI2.h>
#include <X11/extensions/XI2proto.h>

#include "dix/dix_priv.h"
#include "dix/exevents_priv.h"
#include "dix/input_priv.h"
#include "dix/request_priv.h"

#include "inputstr.h"           /* DeviceIntPtr      */
#include "windowstr.h"          /* window structure  */
#include "mi.h"
#include "eventstr.h"
#include "exglobals.h"          /* BadDevice */
#include "xiallowev.h"

static int allowEvents(ClientPtr client, xXIAllowEventsReq *stuff)
{
    DeviceIntPtr dev;

    int ret = dixLookupDevice(&dev, stuff->deviceid, client, DixGetAttrAccess);
    if (ret != Success)
        return ret;

    TimeStamp time = ClientTimeToServerTime(stuff->time);

    switch (stuff->mode) {
        case XIReplayDevice:
            AllowSome(client, time, dev, NOT_GRABBED);
            break;
        case XISyncDevice:
            AllowSome(client, time, dev, FREEZE_NEXT_EVENT);
            break;
        case XIAsyncDevice:
            AllowSome(client, time, dev, THAWED);
            break;
        case XIAsyncPairedDevice:
            if (IsMaster(dev))
                AllowSome(client, time, dev, THAW_OTHERS);
            break;
        case XISyncPair:
            if (IsMaster(dev))
                AllowSome(client, time, dev, FREEZE_BOTH_NEXT_EVENT);
            break;
        case XIAsyncPair:
            if (IsMaster(dev))
                AllowSome(client, time, dev, THAWED_BOTH);
            break;
        default:
            client->errorValue = stuff->mode;
            ret = BadValue;
    }
    return ret;
}

int
ProcXIAllowEvents(ClientPtr client)
{
    XIClientPtr xi_client = dixLookupPrivate(&client->devPrivates, XIClientPrivateKey);

    if (version_compare(xi_client->major_version,
                        xi_client->minor_version, 2, 2) >= 0) {
        REQUEST_HEAD_AT_LEAST(xXI2_2AllowEventsReq);
        REQUEST_FIELD_CARD16(length);
        REQUEST_FIELD_CARD16(deviceid);
        REQUEST_FIELD_CARD32(time);
        REQUEST_FIELD_CARD32(touchid);
        REQUEST_FIELD_CARD32(grab_window);

        if ((stuff->mode == XIRejectTouch) || (stuff->mode == XIAcceptTouch)) {
            DeviceIntPtr dev;
            int rc;
            WindowPtr win;

            int ret = dixLookupDevice(&dev, stuff->deviceid, client, DixGetAttrAccess);
            if (ret != Success)
                return ret;

            rc = dixLookupWindow(&win, stuff->grab_window, client, DixReadAccess);
            if (rc != Success)
                return rc;

            return TouchAcceptReject(client, dev, stuff->mode, stuff->touchid,
                                     stuff->grab_window, &client->errorValue);
        }
        return allowEvents(client, (xXIAllowEventsReq*)stuff);
    } else {
        REQUEST_HEAD_AT_LEAST(xXIAllowEventsReq);
        REQUEST_FIELD_CARD16(length);
        REQUEST_FIELD_CARD16(deviceid);
        REQUEST_FIELD_CARD32(time);
        return allowEvents(client, stuff);
    }
}
