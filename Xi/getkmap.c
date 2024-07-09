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

/********************************************************************
 *
 *  Get the key mapping for an extension device.
 *
 */

#include <dix-config.h>

#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>

#include "dix/request_priv.h"

#include "inputstr.h"           /* DeviceIntPtr      */
#include "exglobals.h"
#include "swaprep.h"
#include "xkbsrv.h"
#include "xkbstr.h"

#include "getkmap.h"

/***********************************************************************
 *
 * Get the device key mapping.
 *
 */

int
ProcXGetDeviceKeyMapping(ClientPtr client)
{
    DeviceIntPtr dev;
    XkbDescPtr xkb;
    KeySymsPtr syms;
    int rc;

    REQUEST_HEAD_STRUCT(xGetDeviceKeyMappingReq);

    rc = dixLookupDevice(&dev, stuff->deviceid, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;
    if (dev->key == NULL)
        return BadMatch;
    xkb = dev->key->xkbInfo->desc;

    if (stuff->firstKeyCode < xkb->min_key_code ||
        stuff->firstKeyCode > xkb->max_key_code) {
        client->errorValue = stuff->firstKeyCode;
        return BadValue;
    }

    if (stuff->firstKeyCode + stuff->count > xkb->max_key_code + 1) {
        client->errorValue = stuff->count;
        return BadValue;
    }

    syms = XkbGetCoreMap(dev);
    if (!syms)
        return BadAlloc;

    xGetDeviceKeyMappingReply rep = {
        .RepType = X_GetDeviceKeyMapping,
        .keySymsPerKeyCode = syms->mapWidth,
        .length = (syms->mapWidth * stuff->count) /* KeySyms are 4 bytes */
    };

    REPLY_SEND();

    client->pSwapReplyFunc = (ReplySwapPtr) CopySwap32Write;
    WriteSwappedDataToClient(client,
                             syms->mapWidth * stuff->count * sizeof(KeySym),
                             &syms->map[syms->mapWidth * (stuff->firstKeyCode -
                                                          syms->minKeyCode)]);
    free(syms->map);
    free(syms);

    return Success;
}
