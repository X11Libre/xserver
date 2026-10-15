/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include <X11/Xproto.h>
#include <X11/extensions/saverproto.h>

#include "include/misc.h"

#include "saver_intern.h"

void ScreenSaverSwapNotifyEvent(xEvent *_from, xEvent *_to)
{
    xScreenSaverNotifyEvent *from = (xScreenSaverNotifyEvent*)_from;
    xScreenSaverNotifyEvent *to = (xScreenSaverNotifyEvent*)_to;

    *to = *from;
    swaps(&to->sequenceNumber);
    swapl(&to->timestamp);
    swapl(&to->root);
    swapl(&to->window);
}
