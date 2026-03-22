/************************************************************

Copyright 1989, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

Copyright 1989 by Hewlett-Packard Company, Palo Alto, California.

			All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Hewlett-Packard not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

HEWLETT-PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
HEWLETT-PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

********************************************************/

/***********************************************************************
 *
 * Extension function to grab a button on an extension device.
 *
 */

#include <dix-config.h>

#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>

#include "dix/dix_priv.h"
#include "dix/devices_priv.h"
#include "dix/exevents_priv.h"
#include "dix/input_priv.h"
#include "dix/request_priv.h"
#include "Xi/handlers.h"

#include "inputstr.h"           /* DeviceIntPtr      */
#include "windowstr.h"          /* window structure  */
#include "grabdev.h"

/***********************************************************************
 *
 * Grab a button on an extension device.
 *
 */

int
ProcXGrabDeviceButton(ClientPtr client)
{
    int ret;
    DeviceIntPtr dev;
    DeviceIntPtr mdev;
    XEventClass *class;
    struct tmask tmp[EMASKSIZE];
    GrabMask mask;

    X_REQUEST_HEAD_AT_LEAST(xGrabDeviceButtonReq);
    X_REQUEST_FIELD_CARD32(grabWindow);
    X_REQUEST_FIELD_CARD16(modifiers);
    X_REQUEST_FIELD_CARD16(event_count);
    X_REQUEST_REST_COUNT_CARD32(stuff->event_count);

    ret = dixLookupDevice(&dev, stuff->grabbed_device, client, DixGrabAccess);
    if (ret != Success)
        return ret;

    if (stuff->modifier_device != UseXKeyboard) {
        ret = dixLookupDevice(&mdev, stuff->modifier_device, client,
                              DixUseAccess);
        if (ret != Success)
            return ret;
        if (mdev->key == NULL)
            return BadMatch;
    }
    else {
        mdev = PickKeyboard(client);
        ret = dixCallDeviceAccessCallback(client, mdev, DixUseAccess);
        if (ret != Success)
            return ret;
    }

    class = (XEventClass *) (&stuff[1]);        /* first word of values */

    if ((ret = CreateMaskFromList(client, class,
                                  stuff->event_count, tmp, dev,
                                  X_GrabDeviceButton)) != Success)
        return ret;

    GrabParameters param = {
        .grabtype = XI,
        .ownerEvents = stuff->ownerEvents,
        .this_device_mode = stuff->this_device_mode,
        .other_devices_mode = stuff->other_devices_mode,
        .grabWindow = stuff->grabWindow,
        .modifiers = stuff->modifiers
    };
    mask.xi = tmp[stuff->grabbed_device].mask;

    ret = GrabButton(client, dev, mdev, stuff->button, &param, XI, &mask);

    return ret;
}
