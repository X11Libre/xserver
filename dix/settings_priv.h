/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_SETTINGS_H
#define _XSERVER_DIX_SETTINGS_H

#include <stdbool.h>

/* This file holds global DIX *settings*, which might be needed by other
 * parts, e.g. OS layer or DDX'es.
 *
 * Some of them might be influenced by command line args, some by xf86's
 * config files.
 */

extern bool dixSettingAllowByteSwappedClients;
extern char *dixSettingSeatId;

#endif
