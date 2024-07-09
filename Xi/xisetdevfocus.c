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
#include "Xi/handlers.h"

#include "inputstr.h"           /* DeviceIntPtr      */
#include "windowstr.h"          /* window structure  */
#include "exglobals.h"          /* BadDevice */

int
ProcXISetFocus(ClientPtr client)
{
    X_REQUEST_HEAD_AT_LEAST(xXISetFocusReq);
    X_REQUEST_FIELD_CARD16(deviceid);
    X_REQUEST_FIELD_CARD32(focus);
    X_REQUEST_FIELD_CARD32(time);

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
    X_REQUEST_HEAD_AT_LEAST(xXIGetFocusReq);
    X_REQUEST_FIELD_CARD16(deviceid);

    DeviceIntPtr dev;
    int ret;

    ret = dixLookupDevice(&dev, stuff->deviceid, client, DixGetFocusAccess);
    if (ret != Success)
        return ret;
    if (!dev->focus)
        return BadDevice;

    xXIGetFocusReply reply = {
        .RepType = X_XIGetFocus,
    };

    if (dev->focus->win == NoneWin)
        reply.focus = None;
    else if (dev->focus->win == PointerRootWin)
        reply.focus = PointerRoot;
    else if (dev->focus->win == FollowKeyboardWin)
        reply.focus = FollowKeyboard;
    else
        reply.focus = dev->focus->win->drawable.id;

    REPLY_FIELD_CARD32(focus);

    return X_SEND_REPLY_SIMPLE(client, reply);
}
