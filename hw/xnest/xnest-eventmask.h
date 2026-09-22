/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef __XLIBRE_XNEST_EVENTMASK_H
#define __XLIBRE_XNEST_EVENTMASK_H

#include <stdint.h>

extern uint32_t xnestEventMask;

void xnestUpdateEventMask(void);

#endif /* __XLIBRE_XNEST_EVENTMASK_H */
