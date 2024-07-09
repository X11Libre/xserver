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
 * Request to open an extension input device.
 *
 */

#include <dix-config.h>

#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>

#include "dix/input_priv.h"
#include "dix/request_priv.h"

#include "inputstr.h"           /* DeviceIntPtr      */
#include "XIstubs.h"
#include "windowstr.h"          /* window structure  */
#include "exglobals.h"
#include "exevents.h"

#include "opendev.h"

extern CARD8 event_base[];

/***********************************************************************
 *
 * This procedure causes the server to open an input device.
 *
 */

int
ProcXOpenDevice(ClientPtr client)
{
    xInputClassInfo evbase[numInputClasses];
    int j = 0;
    int status = Success;
    DeviceIntPtr dev;

    REQUEST_HEAD_STRUCT(xOpenDeviceReq);

    status = dixLookupDevice(&dev, stuff->deviceid, client, DixUseAccess);

    if (status == BadDevice) {  /* not open */
        for (dev = inputInfo.off_devices; dev; dev = dev->next)
            if (dev->id == stuff->deviceid)
                break;
        if (dev == NULL)
            return BadDevice;
    }
    else if (status != Success)
        return status;

    if (InputDevIsMaster(dev))
        return BadDevice;

    if (status != Success)
        return status;

    if (dev->key != NULL) {
        evbase[j].class = KeyClass;
        evbase[j++].event_type_base = event_base[KeyClass];
    }
    if (dev->button != NULL) {
        evbase[j].class = ButtonClass;
        evbase[j++].event_type_base = event_base[ButtonClass];
    }
    if (dev->valuator != NULL) {
        evbase[j].class = ValuatorClass;
        evbase[j++].event_type_base = event_base[ValuatorClass];
    }
    if (dev->kbdfeed != NULL || dev->ptrfeed != NULL || dev->leds != NULL ||
        dev->intfeed != NULL || dev->bell != NULL || dev->stringfeed != NULL) {
        evbase[j].class = FeedbackClass;
        evbase[j++].event_type_base = event_base[FeedbackClass];
    }
    if (dev->focus != NULL) {
        evbase[j].class = FocusClass;
        evbase[j++].event_type_base = event_base[FocusClass];
    }
    if (dev->proximity != NULL) {
        evbase[j].class = ProximityClass;
        evbase[j++].event_type_base = event_base[ProximityClass];
    }
    evbase[j].class = OtherClass;
    evbase[j++].event_type_base = event_base[OtherClass];

    xOpenDeviceReply rep = {
        .repType = X_Reply,
        .RepType = X_OpenDevice,
        .sequenceNumber = client->sequence,
        .length = bytes_to_int32(j * sizeof(xInputClassInfo)),
        .num_classes = j
    };

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
    }

    WriteToClient(client, sizeof(xOpenDeviceReply), &rep);
    WriteToClient(client, j * sizeof(xInputClassInfo), evbase);
    return Success;
}
