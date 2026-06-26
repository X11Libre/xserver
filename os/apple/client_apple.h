/* SPDX-License-Identifier: MIT OR X11 OR AGPLv3
 *
 * Copyright © 2026 Aggelos Tselios <aggelostselios777@gmail.com>
 */
 
#pragma once

#ifndef OS_APPLE_CLIENT_H
#define OS_APPLE_CLIENT_H 1

#include <sys/types.h> /* pid_t */

/* Implementation of DetermineClientCmd for Apple targets. */
void DetermineClientCmdApple(pid_t pid, const char **cmdname, const char **cmdargs);

#endif /* OS_APPLE_CLIENT_H */
