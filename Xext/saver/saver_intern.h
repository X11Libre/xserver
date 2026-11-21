/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * this file holds only extension-internal definitions, it shall never
 * be included outside the extension itself.
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef __XLIBRE_SAVER_INTERN_H
#define __XLIBRE_SAVER_INTERN_H

#include <X11/Xproto.h>

void ScreenSaverSwapNotifyEvent(xEvent *_from, xEvent *_to);

#endif /* __XLIBRE_SAVER_INTERN_H */
