/* SPDX-License-Identifier: MIT OR X11
 *
 * @copyright Enrico Weigelt, metux IT consult <info@metux.net>
 * @brief definitions for XF86 console driver interface
 */
#ifndef __XSERVER_XFREE86_XF86_CONSOLE_PRIV_H
#define __XSERVER_XFREE86_XF86_CONSOLE_PRIV_H

#include <stdbool.h>

/* user requested VT number (-1 = unspecified) */
extern int xf86_console_requested_vt;

/* close callback of current console backend - may be NULL */
extern void (*xf86_console_proc_close)(void);

/* reactivation callback (eg. on server regeneration) - may be NULL */
extern void (*xf86_console_proc_reactivate)(void);

/* ring the system bell */
extern void (*xf86_console_proc_bell)(int loudness, int pitch, int duration);

/* switch away from VT */
extern bool (*xf86_console_proc_switch_away)(void);

#endif /* __XSERVER_XFREE86_XF86_CONSOLE_PRIV_H */
