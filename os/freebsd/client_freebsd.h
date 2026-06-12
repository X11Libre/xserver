#pragma once

#ifndef OS_FREEBSD_CLIENT_H
#define OS_FREEBSD_CLIENT_H 1

#include <sys/types.h> /* pid_t */

/* Implementation of DetermineClientCmd for FreeBSD based operating systems */
void DetermineClientCmdFreeBSD(pid_t pid, const char **cmdname, const char **cmdargs);

#endif /* OS_FREEBSD_CLIENT_H */
