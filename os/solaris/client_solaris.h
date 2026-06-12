#pragma once

#ifndef OS_SOLARIS_CLIENT_H
#define OS_SOLARIS_CLIENT_H 1

#include <sys/types.h> /* pid_t */

/* Implementation of DetermineClientCmd for Solaris/SunOS systems */
void DetermineClientCmdSolaris(pid_t pid, const char **cmdname, const char **cmdargs);

#endif /* OS_SOLARIS_CLIENT_H */
