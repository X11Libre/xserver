/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * this file holds only extension-internal definitions, it shall never
 * be included outside the extension itself.
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef __XLIBRE_SYNC_INTERN_H
#define __XLIBRE_SYNC_INTERN_H

#include <X11/Xproto.h>

void SyncSwapCounterNotifyEvent(xEvent *_from, xEvent *_to);
void SyncSwapAlarmNotifyEvent(xEvent *_from, xEvent *_to);

#endif /* __XLIBRE_SYNC_INTERN_H */
