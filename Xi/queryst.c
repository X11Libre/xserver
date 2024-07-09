/*

Copyright 1998, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

/***********************************************************************
 *
 * Request to query the state of an extension input device.
 *
 */

#include <dix-config.h>

#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>

#include "dix/dix_priv.h"
#include "dix/exevents_priv.h"
#include "dix/input_priv.h"
#include "dix/request_priv.h"
#include "dix/rpcbuf_priv.h"
#include "Xi/handlers.h"

#include "inputstr.h"           /* DeviceIntPtr      */
#include "windowstr.h"          /* window structure  */
#include "xkbsrv.h"
#include "xkbstr.h"

/***********************************************************************
 *
 * This procedure allows frozen events to be routed.
 *
 */

int
ProcXQueryDeviceState(ClientPtr client)
{
    int rc, i;
    int num_classes = 0;
    int total_length = 0;
    KeyClassPtr k;
    xKeyState *tk;
    ButtonClassPtr b;
    xButtonState *tb;
    ValuatorClassPtr v;
    xValuatorState *tv;
    DeviceIntPtr dev;

    X_REQUEST_HEAD_STRUCT(xQueryDeviceStateReq);

    rc = dixLookupDevice(&dev, stuff->deviceid, client, DixReadAccess);
    if (rc != Success && rc != BadAccess)
        return rc;

    v = dev->valuator;
    if (v != NULL && v->motionHintWindow != NULL)
        MaybeStopDeviceHint(dev, client);

    k = dev->key;
    if (k != NULL) {
        total_length += sizeof(xKeyState);
        num_classes++;
    }

    b = dev->button;
    if (b != NULL) {
        total_length += sizeof(xButtonState);
        num_classes++;
    }

    if (v != NULL) {
        total_length += (sizeof(xValuatorState) + (v->numAxes * sizeof(int)));
        num_classes++;
    }

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };
    char *buf = x_rpcbuf_reserve(&rpcbuf, total_length);
    if (!buf)
        return BadAlloc;

    if (k != NULL) {
        tk = (xKeyState *) buf;
        tk->class = KeyClass;
        tk->length = sizeof(xKeyState);
        tk->num_keys = k->xkbInfo->desc->max_key_code -
            k->xkbInfo->desc->min_key_code + 1;
        for (i = 0; i < 32; i++)
            tk->keys[i] = k->down[i];
        buf += sizeof(xKeyState);
    }

    if (b != NULL) {
        tb = (xButtonState *) buf;
        tb->class = ButtonClass;
        tb->length = sizeof(xButtonState);
        tb->num_buttons = b->numButtons;
        if (rc != BadAccess)
            memcpy(tb->buttons, b->down, sizeof(b->down));
        buf += sizeof(xButtonState);
    }

    if (v != NULL) {
        tv = (xValuatorState *) buf;
        tv->class = ValuatorClass;
        tv->length = sizeof(xValuatorState) + v->numAxes * 4;
        tv->num_valuators = v->numAxes;
        tv->mode = valuator_get_mode(dev, 0);
        tv->mode |= (dev->proximity &&
                     !dev->proximity->in_proximity) ? OutOfProximity : 0;
        buf += sizeof(xValuatorState);
        int *buf2 = (int *)buf;
        for (i = 0; i < v->numAxes; i++) {
            if (rc != BadAccess)
                buf2[i] = v->axisVal[i];
        }
        REPLY_BUF_CARD32(buf2, v->numAxes);
    }

    xQueryDeviceStateReply reply = {
        .RepType = X_QueryDeviceState,
        .num_classes = num_classes
    };

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}
