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

#include "inputstr.h"           /* DeviceIntPtr      */
#include "exglobals.h"
#include "gtmotion.h"

/***********************************************************************
 *
 * Swap the request if server and client have different byte ordering.
 *
 */

int _X_COLD
SProcXGetDeviceMotionEvents(ClientPtr client)
{
    REQUEST(xGetDeviceMotionEventsReq);
    REQUEST_SIZE_MATCH(xGetDeviceMotionEventsReq);
    swapl(&stuff->start);
    swapl(&stuff->stop);
    return (ProcXGetDeviceMotionEvents(client));
}

/****************************************************************************
 *
 * Get the motion history for an extension pointer devices.
 *
 */

int
ProcXGetDeviceMotionEvents(ClientPtr client)
{
    REQUEST(xGetDeviceMotionEventsReq);
    REQUEST_SIZE_MATCH(xGetDeviceMotionEventsReq);

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
        .repType = X_Reply,
        .RepType = X_GetDeviceMotionEvents,
        .sequenceNumber = client->sequence,
        .length = 0,
        .nEvents = 0,
        .axes = v->numAxes,
        .mode = Absolute        /* XXX we don't do relative at the moment */
    };

    TimeStamp start = ClientTimeToServerTime(stuff->start);
    TimeStamp stop = ClientTimeToServerTime(stuff->stop);
    if (CompareTimeStamps(start, stop) == LATER ||
        CompareTimeStamps(start, currentTime) == LATER) {
        WriteReplyToClient(client, sizeof(xGetDeviceMotionEventsReply), &rep);
        return Success;
    }
    if (CompareTimeStamps(stop, currentTime) == LATER)
        stop = currentTime;

    int length = 0;
    INT32 *coords = NULL;
    if (v->numMotionEvents) {
        rep.nEvents = GetMotionHistory(dev, (xTimecoord **) &coords,   /* XXX */
                                       start.milliseconds, stop.milliseconds,
                                       (ScreenPtr) NULL, FALSE);
        length = rep.nEvents * (sizeof(Time) + (v->numAxes * sizeof(INT32)));
    }

    rep.length = bytes_to_int32(length);

    if (client->swapped) {
        SwapLongs((CARD32*) coords, rep.length);
    }

    WriteReplyToClient(client, sizeof(xGetDeviceMotionEventsReply), &rep);
    WriteToClient(client, length, coords);
    free(coords);
    return Success;
}

/***********************************************************************
 *
 * This procedure writes the reply for the XGetDeviceMotionEvents function,
 * if the client and server have a different byte ordering.
 *
 */

void _X_COLD
SRepXGetDeviceMotionEvents(ClientPtr client, int size,
                           xGetDeviceMotionEventsReply * rep)
{
    swaps(&rep->sequenceNumber);
    swapl(&rep->length);
    swapl(&rep->nEvents);
    WriteToClient(client, size, rep);
}
