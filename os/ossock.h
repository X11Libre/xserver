/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_OS_OSSOCK_H_
#define _XSERVER_OS_OSSOCK_H_

#include <errno.h>
#include <stdbool.h>

/*
 * os specific initialization of the socket layer
 */
void ossock_init(void);

/*
 * os specific socket ioctl function
 */
int ossock_ioctl(int fd, unsigned long request, void *arg);

/*
 * os specific socket close function
 */
int ossock_close(int fd);

/*
 * os specific check for errno indicating operation would block
 */
int ossock_wouldblock(int err);

/*
 * os specific check for errno indicating operation interrupted
 */
bool ossock_eintr(int err);

/*
 * os specific retrieval of last socket operation error
 * on Unix: errno, on Win32: GetWSALastError()
 */
int ossock_errno(void);

#endif /* _XSERVER_OS_OSSOCK_H_ */
