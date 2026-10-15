/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include <X11/Xproto.h>
#include <X11/extensions/shapeproto.h>

#include "include/misc.h"

#include "shape_intern.h"

void ShapeSwapNotifyEvent(xEvent *_from, xEvent *_to)
{
    xShapeNotifyEvent *from = (xShapeNotifyEvent*)_from;
    xShapeNotifyEvent *to = (xShapeNotifyEvent*)_to;

    *to = *from;

    swapl(&to->window);
    swaps(&to->sequenceNumber);
    swaps(&to->x);
    swaps(&to->y);
    swaps(&to->width);
    swaps(&to->height);
    swapl(&to->time);
}
