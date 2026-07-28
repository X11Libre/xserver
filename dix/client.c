/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include <stddef.h>

#include "include/callback.h"

CallbackListPtr ClientDestroyCallback = NULL;
