/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111github@gmail.com>
 */

#ifndef _KSTUB_H_
#define _KSTUB_H_

#include "kdrive.h"

extern KdPointerDriver StubMouseDriver;
extern KdKeyboardDriver StubKeyboardDriver;

extern KdOsFuncs StubOsFuncs;

/* Register the stub input drivers */
void StubAddInputDrivers(void);

#endif /* _KSTUB_H_ */
