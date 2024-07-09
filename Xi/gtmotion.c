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
 * Request to get the motion history from an extension device.
 *
 */

#include <dix-config.h>

#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>

#include "dix/exevents_priv.h"
#include "dix/request_priv.h"

#include "inputstr.h"           /* DeviceIntPtr      */
#include "exglobals.h"
#include "gtmotion.h"

/****************************************************************************
 *
 * Get the motion history for an extension pointer devices.
 *
 */

int
ProcXGetDeviceMotionEvents(ClientPtr client)
{
    REQUEST_HEAD_STRUCT(xGetDeviceMotionEventsReq);
    REQUEST_FIELD_CARD32(start);
    REQUEST_FIELD_CARD32(stop);

    DeviceIntPtr dev;
    int rc = dixLookupDevice(&dev, stuff->deviceid, client, DixReadAccess);
    if (rc != Success)
        return rc;

    const ValuatorClassPtr v = dev->valuator;
    if (v == NULL || v->numAxes == 0)
        return BadMatch;

    if (dev->valuator->motionHintWindow)
        MaybeStopDeviceHint(dev, client);

    xGetDeviceMotionEventsReply rep = {
        .RepType = X_GetDeviceMotionEvents,
        .axes = v->numAxes,
        .mode = Absolute        /* XXX we don't do relative at the moment */
    };

    TimeStamp start = ClientTimeToServerTime(stuff->start);
    TimeStamp stop = ClientTimeToServerTime(stuff->stop);
    int length = 0;
    INT32 *coords = NULL;

    if (CompareTimeStamps(start, stop) != LATER &&
        CompareTimeStamps(start, currentTime) != LATER) {
        if (CompareTimeStamps(stop, currentTime) == LATER)
            stop = currentTime;
        if (v->numMotionEvents) {
            const int size = sizeof(Time) + (v->numAxes * sizeof(INT32));
            rep.nEvents = GetMotionHistory(dev, (xTimecoord **) &coords,   /* XXX */
                                           start.milliseconds, stop.milliseconds,
                                           (ScreenPtr) NULL, FALSE);
            if (rep.nEvents < 0)
                rep.nEvents = 0;

            length = rep.nEvents * size;
        }
    }

    if (client->swapped) {
        SwapLongs((CARD32*) coords, rep.length);
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.nEvents);
    }

    REPLY_BUF_CARD32(coords, bytes_to_int32(length));
    REPLY_FIELD_CARD32(nEvents);
    REPLY_SEND_EXTRA(coords, length);
    free(coords);
    return Success;
}
