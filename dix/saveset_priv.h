/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_SAVESET_PRIV_H
#define _XSERVER_DIX_SAVESET_PRIV_H

#include <stdbool.h>

#include "include/list.h"
#include "include/window.h"

typedef struct {
    struct xorg_list entry;
    WindowPtr windowPtr;
    bool toRoot;
    bool map;
} SaveSetEntry;

#endif /*_XSERVER_DIX_SAVESET_PRIV_H */
