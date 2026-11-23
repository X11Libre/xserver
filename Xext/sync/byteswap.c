/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include <X11/Xdefs.h>
#include <X11/Xproto.h>
#include <X11/extensions/syncproto.h>

#include "include/misc.h"

#include "sync_intern.h"

void SyncSwapCounterNotifyEvent(xEvent *_from, xEvent *_to)
{
    xSyncCounterNotifyEvent *from = (xSyncCounterNotifyEvent*)_from;
    xSyncCounterNotifyEvent *to = (xSyncCounterNotifyEvent*)_to;

    *to = *from;
    swaps(&to->sequenceNumber);
    swapl(&to->counter);
    swapl(&to->wait_value_lo);
    swapl(&to->wait_value_hi);
    swapl(&to->counter_value_lo);
    swapl(&to->counter_value_hi);
    swapl(&to->time);
    swaps(&to->count);
}

void SyncSwapAlarmNotifyEvent(xEvent *_from, xEvent *_to)
{
    xSyncAlarmNotifyEvent *from = (xSyncAlarmNotifyEvent*)_from;
    xSyncAlarmNotifyEvent *to = (xSyncAlarmNotifyEvent*)_to;

    *to = *from;
    swaps(&to->sequenceNumber);
    swapl(&to->alarm);
    swapl(&to->counter_value_lo);
    swapl(&to->counter_value_hi);
    swapl(&to->alarm_value_lo);
    swapl(&to->alarm_value_hi);
    swapl(&to->time);
}
