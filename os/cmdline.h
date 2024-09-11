/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_OS_CMDLINE_H
#define _XSERVER_OS_CMDLINE_H

#include "include/os.h"

#define CHECK_FOR_REQUIRED_ARGUMENTS(num)  \
    do if (((i + num) >= argc) || (!argv[i + num])) {                   \
        UseMsg();                                                       \
        FatalError("Required argument to %s not specified\n", argv[i]); \
    } while (0)

void UseMsg(void);
void ProcessCommandLine(int argc, char * argv[]);
void CheckUserParameters(int argc, char **argv, char **envp);

/*
 * @brief check for and parse an counting-flag or value-flag option
 *
 * Parses an option that may either be used for setting an integer value or
 * given one or multiple times (without argument) to increase an value
 *
 * @param argc total number of elements in argv
 * @param argv array of pointers to cmdline argument strings
 * @param idx pointer to current index in argv -- eventually will be modified
 * @param name the command line argument name
 * @param value pointer to the field holding the setting value
 * @return non-zero if the flag was found and parsed
 */
int ProcessCmdLineMultiInt(int argc, char *argv[], int *idx, const char* name, int *value);

#endif /* _XSERVER_OS_CMELINE_H */
