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
 * Function to modify the dont-propagate-list for an extension input device.
 *
 */

#include <dix-config.h>

#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>

#include "dix/dix_priv.h"
#include "dix/exevents_priv.h"
#include "dix/request_priv.h"
#include "dix/window_priv.h"
#include "Xi/handlers.h"

#include "inputstr.h"           /* DeviceIntPtr      */
#include "windowstr.h"
#include "exevents.h"
#include "exglobals.h"
#include "grabdev.h"

/***********************************************************************
 *
 * This procedure changes the dont-propagate list for the specified window.
 *
 */

int
ProcXChangeDeviceDontPropagateList(ClientPtr client)
{
    X_REQUEST_HEAD_AT_LEAST(xChangeDeviceDontPropagateListReq);
    X_REQUEST_FIELD_CARD32(window);
    X_REQUEST_FIELD_CARD16(count);
    X_REQUEST_REST_COUNT_CARD32(stuff->count);

    int i, rc;
    WindowPtr pWin;
    struct tmask tmp[EMASKSIZE];
    OtherInputMasks *others;

    rc = dixLookupWindow(&pWin, stuff->window, client, DixSetAttrAccess);
    if (rc != Success)
        return rc;

    if (stuff->mode != AddToList && stuff->mode != DeleteFromList) {
        client->errorValue = stuff->window;
        return BadMode;
    }

    if ((rc = CreateMaskFromList(client, (XEventClass *) &stuff[1],
                                 stuff->count, tmp, NULL,
                                 X_ChangeDeviceDontPropagateList)) != Success)
        return rc;

    others = wOtherInputMasks(pWin);
    if (!others && stuff->mode == DeleteFromList)
        return Success;
    for (i = 0; i < EMASKSIZE; i++) {
        if (tmp[i].mask == 0)
            continue;

        if (stuff->mode == DeleteFromList)
            tmp[i].mask = (others->dontPropagateMask[i] & ~tmp[i].mask);
        else if (others)
            tmp[i].mask |= others->dontPropagateMask[i];

        if (DeviceEventSuppressForWindow(pWin, client, tmp[i].mask, i) !=
            Success)
            return BadClass;
    }

    return Success;
}
