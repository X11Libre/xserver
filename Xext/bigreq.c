/*

Copyright 1992, 1998  The Open Group

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

#include <dix-config.h>

#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/extensions/bigreqsproto.h>

#include "dix/dix_priv.h"
#include "dix/request_priv.h"
#include "miext/extinit_priv.h"

#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "opaque.h"

static int
ProcBigReqDispatch(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xBigReqEnableReq);

    if (stuff->brReqType != X_BigReqEnable)
        return BadRequest;
    client->big_requests = TRUE;

    xBigReqEnableReply reply = {
        .max_request_size = maxBigRequestSize
    };

    REPLY_FIELD_CARD32(max_request_size);

    return X_SEND_REPLY_SIMPLE(client, reply);
}

void
BigReqExtensionInit(void)
{
    AddExtension(XBigReqExtensionName, 0, 0,
                 ProcBigReqDispatch, ProcBigReqDispatch,
                 NULL, StandardMinorOpcode);
}
