/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XLIBRE_XAVERIUS_H
#define _XLIBRE_XAVERIUS_H

#include "include/xlibre_ptrtypes.h"

#define XAVERIUS_EXTENSION_NAME "XAVERIUS"

extern int XaveriusEventBase;
int ProcXaveriusDispatch(ClientPtr client);

#endif /* _XLIBRE_XAVERIUS_H */
