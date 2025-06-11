/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */

#include "dix/dix_priv.h"
#include "include/dix.h"
#include "include/screenint.h"

const char *display = "0";
int displayfd = -1;

const char *dixGetDisplayName(ScreenPtr *pScreen)
{
    // pScreen currently is ignored as the value is global,
    // but this might perhaps change in the future.
    return display;
}
