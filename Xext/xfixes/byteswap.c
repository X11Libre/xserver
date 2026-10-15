/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include <X11/Xproto.h>
#include <X11/extensions/xfixesproto.h>

#include "include/misc.h"

#include "xfixes_intern.h"

void XFixesSwapSelectionNotifyEvent(xEvent* _from, xEvent* _to)
{
    xXFixesSelectionNotifyEvent *from = (xXFixesSelectionNotifyEvent*) _from;
    xXFixesSelectionNotifyEvent *to = (xXFixesSelectionNotifyEvent*) _to;

    to->type = from->type;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->window, to->window);
    cpswapl(from->owner, to->owner);
    cpswapl(from->selection, to->selection);
    cpswapl(from->timestamp, to->timestamp);
    cpswapl(from->selectionTimestamp, to->selectionTimestamp);
}

void XFixesSwapCursorNotifyEvent(xEvent* _from, xEvent* _to)
{
    xXFixesCursorNotifyEvent *from = (xXFixesCursorNotifyEvent*) _from;
    xXFixesCursorNotifyEvent *to = (xXFixesCursorNotifyEvent*) _to;

    to->type = from->type;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->window, to->window);
    cpswapl(from->cursorSerial, to->cursorSerial);
    cpswapl(from->timestamp, to->timestamp);
    cpswapl(from->name, to->name);
}
