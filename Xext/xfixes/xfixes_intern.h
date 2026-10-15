/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * this file holds only extension-internal definitions, it shall never
 * be included outside the extension itself.
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XLIBRE_XFIXES_INTERN_H
#define _XLIBRE_XFIXES_INTERN_H

#include <X11/Xproto.h>

void XFixesSwapSelectionNotifyEvent(xEvent* _from, xEvent* _to);
void XFixesSwapCursorNotifyEvent(xEvent* _from, xEvent* _to);

#endif /* _XLIBRE_XFIXES_INTERN_H */
