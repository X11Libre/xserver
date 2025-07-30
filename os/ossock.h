/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_OS_OSSOCK_H_
#define _XSERVER_OS_OSSOCK_H_

#include <errno.h>

/*
 * os specific initialization of the socket layer
 */
void ossock_init(void);

/*
 * os specific socket ioctl function
 */
int ossock_ioctl(int fd, unsigned long request, void *arg);

#endif /* _XSERVER_OS_OSSOCK_H_ */
