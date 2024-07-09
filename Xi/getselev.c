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
 * Extension function to get the current selected events for a given window.
 *
 */

#include <dix-config.h>

#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>

#include "dix/request_priv.h"
#include "dix/resource_priv.h"

#include "inputstr.h"           /* DeviceIntPtr      */
#include "windowstr.h"          /* window struct     */
#include "exglobals.h"
#include "swaprep.h"
#include "getprop.h"
#include "getselev.h"

/***********************************************************************
 *
 * This procedure gets the current device select mask,
 * if the client and server have a different byte ordering.
 *
 */

int
ProcXGetSelectedExtensionEvents(ClientPtr client)
{
    int i, rc, total_length = 0;
    WindowPtr pWin;
    XEventClass *buf = NULL;
    XEventClass *tclient;
    XEventClass *aclient;
    OtherInputMasks *pOthers;
    InputClientsPtr others;

    REQUEST_HEAD_STRUCT(xGetSelectedExtensionEventsReq);
    REQUEST_FIELD_CARD32(window);

    xGetSelectedExtensionEventsReply rep = {
        .repType = X_Reply,
        .RepType = X_GetSelectedExtensionEvents,
        .sequenceNumber = client->sequence,
    };

    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    if ((pOthers = wOtherInputMasks(pWin)) != 0) {
        for (others = pOthers->inputClients; others; others = others->next)
            for (i = 0; i < EMASKSIZE; i++)
                ClassFromMask(NULL, others->mask[i], i,
                              &rep.all_clients_count, COUNT);

        for (others = pOthers->inputClients; others; others = others->next)
            if (SameClient(others, client)) {
                for (i = 0; i < EMASKSIZE; i++)
                    ClassFromMask(NULL, others->mask[i], i,
                                  &rep.this_client_count, COUNT);
                break;
            }

        total_length = (rep.all_clients_count + rep.this_client_count) *
            sizeof(XEventClass);
        rep.length = bytes_to_int32(total_length);
        buf = calloc(1, total_length);

        tclient = buf;
        aclient = buf + rep.this_client_count;
        if (others)
            for (i = 0; i < EMASKSIZE; i++)
                tclient =
                    ClassFromMask(tclient, others->mask[i], i, NULL, CREATE);

        for (others = pOthers->inputClients; others; others = others->next)
            for (i = 0; i < EMASKSIZE; i++)
                aclient =
                    ClassFromMask(aclient, others->mask[i], i, NULL, CREATE);
    }

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swaps(&rep.this_client_count);
        swaps(&rep.all_clients_count);
    }
    WriteToClient(client, sizeof(xGetSelectedExtensionEventsReply), &rep);

    if (total_length) {
        client->pSwapReplyFunc = (ReplySwapPtr) Swap32Write;
        WriteSwappedDataToClient(client, total_length, buf);
    }
    free(buf);
    return Success;
}
