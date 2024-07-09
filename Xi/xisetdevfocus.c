/*
 * Copyright 2008 Red Hat, Inc.
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
 * Request to set and get an input device's focus.
 *
 */

#include <dix-config.h>

#include <X11/extensions/XI2.h>
#include <X11/extensions/XI2proto.h>

#include "dix/dix_priv.h"
#include "dix/request_priv.h"

#include "inputstr.h"           /* DeviceIntPtr      */
#include "windowstr.h"          /* window structure  */
#include "exglobals.h"          /* BadDevice */
#include "xisetdevfocus.h"

int
ProcXISetFocus(ClientPtr client)
{
    REQUEST_HEAD_AT_LEAST(xXISetFocusReq);
    REQUEST_FIELD_CARD16(deviceid);
    REQUEST_FIELD_CARD32(focus);
    REQUEST_FIELD_CARD32(time);

    DeviceIntPtr dev;
    int ret;

    ret = dixLookupDevice(&dev, stuff->deviceid, client, DixSetFocusAccess);
    if (ret != Success)
        return ret;
    if (!dev->focus)
        return BadDevice;

    return SetInputFocus(client, dev, stuff->focus, RevertToParent,
                         stuff->time, TRUE);
}

int
ProcXIGetFocus(ClientPtr client)
{
    REQUEST_HEAD_AT_LEAST(xXIGetFocusReq);
    REQUEST_FIELD_CARD16(deviceid);

    DeviceIntPtr dev;
    int ret;

    ret = dixLookupDevice(&dev, stuff->deviceid, client, DixGetFocusAccess);
    if (ret != Success)
        return ret;
    if (!dev->focus)
        return BadDevice;

    xXIGetFocusReply rep = {
        .repType = X_Reply,
        .RepType = X_XIGetFocus,
        .sequenceNumber = client->sequence,
    };

    if (dev->focus->win == NoneWin)
        rep.focus = None;
    else if (dev->focus->win == PointerRootWin)
        rep.focus = PointerRoot;
    else if (dev->focus->win == FollowKeyboardWin)
        rep.focus = FollowKeyboard;
    else
        rep.focus = dev->focus->win->drawable.id;

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.focus);
    }
    WriteToClient(client, sizeof(xXIGetFocusReply), &rep);
    return Success;
}
