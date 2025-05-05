/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef XORG_FBPICT_PRIV_H
#define XORG_FBPICT_PRIV_H

#include <X11/extensions/renderproto.h>

#include "fb/fbpict.h"
#include "render/picture.h"

void fbRasterizeTrapezoid(PicturePtr alpha, xTrapezoid *trap,
                          int x_off, int y_off);

#endif /* XORG_FBPICT_PRIV_H */
