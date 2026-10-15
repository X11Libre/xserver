/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef __XLIBRE_XV_INTERN_H
#define __XLIBRE_XV_INTERN_H

#include <X11/Xproto.h>

void XvSwapVideoNotifyEvent(xEvent *_from, xEvent *_to);
void XvSwapPortNotifyEvent(xEvent* _from, xEvent *_to);

#endif /* __XLIBRE_XV_INTERN_H */
