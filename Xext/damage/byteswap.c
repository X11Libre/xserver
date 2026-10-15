/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include <stdbool.h>
#include <X11/Xproto.h>
#include <X11/extensions/damageproto.h>

#include "include/misc.h"

#include "damageext_intern.h"

void DamageSwapNotifyEvent(xEvent *_from, xEvent *_to)
{
    xDamageNotifyEvent *from = (xDamageNotifyEvent*)_from;
    xDamageNotifyEvent *to = (xDamageNotifyEvent*)_to;

    to->type = from->type;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->drawable, to->drawable);
    cpswapl(from->damage, to->damage);
    cpswaps(from->area.x, to->area.x);
    cpswaps(from->area.y, to->area.y);
    cpswaps(from->area.width, to->area.width);
    cpswaps(from->area.height, to->area.height);
    cpswaps(from->geometry.x, to->geometry.x);
    cpswaps(from->geometry.y, to->geometry.y);
    cpswaps(from->geometry.width, to->geometry.width);
    cpswaps(from->geometry.height, to->geometry.height);
}
