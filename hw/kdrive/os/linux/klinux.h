/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111github@gmail.com>
 */

#ifndef _KLINUX_H_
#define _KLINUX_H_

#include "kdrive.h"

#ifdef KDRIVE_MOUSE
extern KdPointerDriver LinuxMouseDriver;
extern KdPointerDriver Ps2MouseDriver;
extern KdPointerDriver MsMouseDriver;
extern KdPointerDriver BusMouseDriver;
#endif
#ifdef KDRIVE_TSLIB
extern KdPointerDriver TsDriver;
#endif
#ifdef KDRIVE_EVDEV
extern KdPointerDriver LinuxEvdevMouseDriver;
extern KdKeyboardDriver LinuxEvdevKeyboardDriver;
#endif
#ifdef KDRIVE_KBD
extern KdKeyboardDriver LinuxKeyboardDriver;
#endif

extern int LinuxConsoleFd;
extern int LinuxApmFd;

extern KdOsFuncs LinuxFuncs;

/* Register the linux input drivers */
void LinuxAddInputDrivers(void);

#endif /* _KLINUX_H_ */
