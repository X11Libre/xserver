#pragma once

#ifndef OS_OPENBSD_CLIENT_H
#define OS_OPENBSD_CLIENT_H 1

#include <sys/types.h> /* pid_t */

/* Implementation of DetermineClientCmd for OpenBSD */
void DetermineClientCmdOpenBSD(pid_t pid, const char **cmdname, const char **cmdargs);

#endif /* OS_OPENBSD_CLIENT_H */
