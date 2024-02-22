/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_OS_DDX_H
#define _XSERVER_OS_DDX_H

#include "os.h"

/* callbacks of the DDX, which are called by DIX or OS layer.
   DDX's need to implement these in order to handle DDX specific things.
*/

/* called before server reset */
void ddxBeforeReset(void);

/* called by ProcessCommandLine, so DDX can catch cmdline args */
int ddxProcessArgument(int argc, char *argv[], int i);

/* print DDX specific usage message */
void ddxUseMsg(void);

void ddxGiveUp(enum ExitCode error);

void ddxInputThreadInit(void);

#endif /* _XSERVER_OS_DDX_H */
