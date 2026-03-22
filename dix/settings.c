/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * This file holds global DIX *settings*, which might be needed by other
 * parts, e.g. OS layer or DDX'es.
 *
 * Some of them might be influenced by command line args, some by xf86's
 * config files.
 */
#include <dix-config.h>

#include <stdbool.h>
#include <stddef.h>

#include "dix/settings_priv.h"

bool dixSettingAllowByteSwappedClients = false;
char *dixSettingSeatId = NULL;
