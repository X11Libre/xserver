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
 * Request to send an extension event.
 *
 */

#include <dix-config.h>

#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>

#include "dix/dix_priv.h"
#include "dix/exevents_priv.h"
#include "dix/extension_priv.h"
#include "dix/inpututils_priv.h"
#include "os/osdep.h"
#include "Xi/handlers.h"

#include "inputstr.h"           /* DeviceIntPtr      */
#include "windowstr.h"          /* Window            */
#include "extnsionst.h"         /* EventSwapPtr      */
#include "grabdev.h"

extern int lastEvent;           /* Defined in extension.c */

/***********************************************************************
 *
 * Send an event to some client, as if it had come from an extension input
 * device.
 *
 */

int
ProcXSendExtensionEvent(ClientPtr client)
{
    REQUEST(xSendExtensionEventReq);
    REQUEST_AT_LEAST_SIZE(xSendExtensionEventReq);

    if (client->swapped) {
        swapl(&stuff->destination);
        swaps(&stuff->count);
    }

    if (client->req_len !=
        bytes_to_int32(sizeof(xSendExtensionEventReq)) + stuff->count +
        (stuff->num_events * bytes_to_int32(sizeof(xEvent))))
        return BadLength;

    if (client->swapped) {
        xEvent *eventP = (xEvent *) &stuff[1];
        for (int i = 0; i < stuff->num_events; i++, eventP++) {
            if (eventP->u.u.type == GenericEvent) {
                client->errorValue = eventP->u.u.type;
                return BadValue;
            }

            EventSwapPtr proc = EventSwapVector[eventP->u.u.type & 0177];
            /* no swapping proc; invalid event type? */
            if (proc == NotImplemented) {
                client->errorValue = eventP->u.u.type;
                return BadValue;
            }
            xEvent eventT = { 0 };
            (*proc) (eventP, &eventT);
            *eventP = eventT;
        }

        CARD32 *p = (CARD32 *) (((xEvent *) &stuff[1]) + stuff->num_events);
        SwapLongs(p, stuff->count);
    }

    int ret, i;
    DeviceIntPtr dev;
    xEvent *first;
    XEventClass *list;
    struct tmask tmp[EMASKSIZE];

    ret = dixLookupDevice(&dev, stuff->deviceid, client, DixWriteAccess);
    if (ret != Success)
        return ret;

    if (stuff->num_events == 0)
        return ret;

    /* The client's event type must be one defined by an extension. */

    first = ((xEvent *) &stuff[1]);
    for (i = 0; i < stuff->num_events; i++) {
        if (!((EXTENSION_EVENT_BASE <= first[i].u.u.type) &&
            (first[i].u.u.type < lastEvent))) {
            client->errorValue = first[i].u.u.type;
            return BadValue;
        }
    }

    list = (XEventClass *) (first + stuff->num_events);
    if ((ret = CreateMaskFromList(client, list, stuff->count, tmp, dev,
                                  X_SendExtensionEvent)) != Success)
        return ret;

    ret = (SendEvent(client, dev, stuff->destination,
                     stuff->propagate, (xEvent *) &stuff[1],
                     tmp[stuff->deviceid].mask, stuff->num_events));

    return ret;
}
