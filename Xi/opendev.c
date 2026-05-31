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

#include "dix/dix_priv.h"
#include "dix/input_priv.h"
#include "dix/request_priv.h"
#include "dix/rpcbuf_priv.h"
#include "Xi/handlers.h"

#include "inputstr.h"           /* DeviceIntPtr      */
#include "XIstubs.h"
#include "windowstr.h"          /* window structure  */
#include "exglobals.h"
#include "exevents.h"

extern CARD8 event_base[];

/***********************************************************************
 *
 * This procedure causes the server to open an input device.
 *
 */

#define WRITE_ICI(cls) do { \
        x_rpcbuf_write_CARD8(&rpcbuf, cls); \
        x_rpcbuf_write_CARD8(&rpcbuf, event_base[cls]); \
        num_classes++; \
    } while (0)

int
ProcXOpenDevice(ClientPtr client)
{
    int num_classes = 0;
    int status = Success;
    DeviceIntPtr dev;

    X_REQUEST_HEAD_STRUCT(xOpenDeviceReq);

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

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    if (dev->key != NULL) {
        WRITE_ICI(KeyClass);
    }
    if (dev->button != NULL) {
        WRITE_ICI(ButtonClass);
    }
    if (dev->valuator != NULL) {
        WRITE_ICI(ValuatorClass);
    }
    if (dev->kbdfeed != NULL || dev->ptrfeed != NULL || dev->leds != NULL ||
        dev->intfeed != NULL || dev->bell != NULL || dev->stringfeed != NULL) {
        WRITE_ICI(FeedbackClass);
    }
    if (dev->focus != NULL) {
        WRITE_ICI(FocusClass);
    }
    if (dev->proximity != NULL) {
        WRITE_ICI(ProximityClass);
    }
    WRITE_ICI(OtherClass);

    xOpenDeviceReply reply = {
        .RepType = X_OpenDevice,
        .num_classes = num_classes
    };

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}
