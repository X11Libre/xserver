/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * @brief only to be used internally by shape extension
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef __XLIBRE_SHAPE_INTERN_H
#define __XLIBRE_SHAPE_INTERN_H

#include <X11/Xproto.h>

void ShapeSwapNotifyEvent(xEvent *_from, xEvent *_to);

#endif /* __XLIBRE_SHAPE_INTERN_H */
