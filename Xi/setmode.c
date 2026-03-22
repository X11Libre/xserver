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
 * Request to change the mode of an extension input device.
 *
 */

#include <dix-config.h>

#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>

#include "dix/dix_priv.h"
#include "dix/input_priv.h"
#include "dix/request_priv.h"
#include "dix/resource_priv.h"
#include "Xi/handlers.h"

#include "inputstr.h"           /* DeviceIntPtr      */
#include "XIstubs.h"
#include "exglobals.h"

/***********************************************************************
 *
 * This procedure sets the mode of a device.
 *
 */

int
ProcXSetDeviceMode(ClientPtr client)
{
    DeviceIntPtr dev;
    int rc;

    X_REQUEST_HEAD_STRUCT(xSetDeviceModeReq);

    xSetDeviceModeReply reply = {
        .RepType = X_SetDeviceMode,
    };

    rc = dixLookupDevice(&dev, stuff->deviceid, client, DixSetAttrAccess);
    if (rc != Success)
        return rc;
    if (dev->valuator == NULL)
        return BadMatch;

    if (IsXTestDevice(dev, NULL))
        return BadMatch;

    if ((dev->deviceGrab.grab) && !SameClient(dev->deviceGrab.grab, client))
        reply.status = AlreadyGrabbed;
    else
        reply.status = SetDeviceMode(client, dev, stuff->mode);

    if (reply.status == Success)
        valuator_set_mode(dev, VALUATOR_MODE_ALL_AXES, stuff->mode);
    else if (reply.status != AlreadyGrabbed) {
        switch (reply.status) {
        case BadMatch:
        case BadImplementation:
        case BadAlloc:
            break;
        default:
            reply.status = BadMode;
        }
        return reply.status;
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}
