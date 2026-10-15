/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include <X11/Xproto.h>
#include <X11/extensions/Xvproto.h>

#include "include/misc.h"

#include "xv_intern.h"

void XvSwapVideoNotifyEvent(xEvent *_from, xEvent *_to)
{
    xvEvent *from = (xvEvent*)_from;
    xvEvent *to = (xvEvent*)_to;
    *to = *from;

    swaps(&to->u.videoNotify.sequenceNumber);
    swapl(&to->u.videoNotify.time);
    swapl(&to->u.videoNotify.drawable);
    swapl(&to->u.videoNotify.port);
}

void XvSwapPortNotifyEvent(xEvent* _from, xEvent *_to)
{
    xvEvent *from = (xvEvent*)_from;
    xvEvent *to = (xvEvent*)_to;
    *to = *from;

    swaps(&to->u.portNotify.sequenceNumber);
    swapl(&to->u.portNotify.time);
    swapl(&to->u.portNotify.port);
    swapl(&to->u.portNotify.value);
}
