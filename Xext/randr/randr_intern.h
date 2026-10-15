/* SPDX-License-Identifier: MIT OR X11 OR AGPLv3
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * @brief: extension-internal definitions
 */
#ifndef _XLIBRE_RANDR_INTERN_H
#define _XLIBRE_RANDR_INTERN_H

#include <X11/Xproto.h>

void RRSwapScreenChangeNotifyEvent(xEvent *_from, xEvent *_to);
void RRSwapNotifyEvent(xEvent *from, xEvent *to);

#endif /* _XLIBRE_RANDR_INTERN_H */
