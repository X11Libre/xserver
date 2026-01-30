/* SPDX-License-Identifier: MIT OR X11
 *
 * @copyright Enrico Weigelt, metux IT consult <info@metux.net>
 * @brief console driver interface
 */
#include <xorg-config.h>

#include <stddef.h>

#include "xf86_console_priv.h"

/* user requested VT number (-1 = unspecified) */
int xf86_console_requested_vt = -1;

/* close callback of current console backend - may be NULL */
void (*xf86_console_proc_close)(void) = NULL;

/* reactivation callback (eg. on server regeneration) - may be NULL */
void (*xf86_console_proc_reactivate)(void) = NULL;

/* ring the system bell */
void (*xf86_console_proc_bell)(int loudness, int pitch, int duration) = NULL;

/* switch away from VT */
bool (*xf86_console_proc_switch_away)(void) = NULL;
