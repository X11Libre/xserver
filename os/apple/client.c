/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies). All
 * rights reserved.
 * Copyright (c) 1993, 2010, Oracle and/or its affiliates.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <dix-config.h>
#include "client_apple.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "os.h"

/* TODO: Have non libdispatch code as well */
#include <dispatch/dispatch.h>

#include <errno.h>
#include <sys/sysctl.h>

static void get_argmax_from_kern(void *arg) {
        int *argmax = arg;
        int mib[2];
        size_t len;

        mib[0] = CTL_KERN;
        mib[1] = KERN_ARGMAX;

        len = sizeof(int);
        if (sysctl(mib, 2, argmax, &len, NULL, 0) == -1) {
                ErrorF("Unable to dynamically determine kern.argmax, using ARG_MAX (%d)\n", ARG_MAX);
                *argmax = ARG_MAX;
        }
}

/*
 * This is the libdispatch-based way to implement DetermineClientCmd. It only works on macOS 10.6 and later. 
 * So TODO, have a version of this function without libdispatch (and wrap this in an ifdef to avoid compiling any libdispatch code)
 * The code of this function was taken from the original DetermineClientCmd, written by Jeremy Huddleston Sequoia.
*/
static void determine_client_cmd_using_libdispatch(pid_t pid, const char **cmdname, const char **cmdargs) {
        static dispatch_once_t once;
        static int argmax;
        dispatch_once_f(&once, &argmax, get_argmax_from_kern);

        int mib[3];
        size_t len = argmax;
        int32_t argc = -1;

        char * const procargs = calloc(1, len);
        if (!procargs) {
            ErrorF("Failed to allocate memory (%lu bytes) for KERN_PROCARGS2 result for pid %d: %s\n", len, pid, strerror(errno));
            return;
        }

        mib[0] = CTL_KERN;
        mib[1] = KERN_PROCARGS2;
        mib[2] = pid;

        if (sysctl(mib, 3, procargs, &len, NULL, 0) == -1) {
            ErrorF("Failed to determine KERN_PROCARGS2 for pid %d: %s\n", pid, strerror(errno));
            free(procargs);
            return;
        }

        if (len < sizeof(argc) || len > argmax) {
            ErrorF("Erroneous length returned when querying KERN_PROCARGS2 for pid %d: %zu\n", pid, len);
            free(procargs);
            return;
        }

        /* Ensure we have a failsafe NUL termination just in case the last entry
         * was not actually NUL terminated.
         */
        procargs[len-1] = '\0';

        /* Setup our iterator */
        char *is = procargs;

        /* The first element in the buffer is argc as a 32bit int. When using
         * the older KERN_PROCARGS, this is omitted, and one needs to guess
         * (usually by checking for an `=` character) when we start seeing
         * envvars instead of arguments.
         */
        argc = *(int32_t *)is;
        is += sizeof(argc);

        /* The very next string is the executable path.  Skip over it since
         * this function wants to return argv[0] and argv[1...n].
         */
        is += strlen(is) + 1;

        /* Skip over extra NUL characters to get to the start of argv[0] */
        for (; (is < &procargs[len]) && !(*is); is++);

        if (! (is < &procargs[len])) {
            ErrorF("Arguments were not returned when querying KERN_PROCARGS2 for pid %d: %zu\n", pid, len);
            free(procargs);
            return;
        }

        if (cmdname) {
            *cmdname = strdup(is);
        }

        /* Jump over argv[0] and point to argv[1] */
        is += strlen(is) + 1;

        if (cmdargs && is < &procargs[len]) {
            char *args = is;

            /* Remove the NUL terminators except the last one */
            for (int i = 1; i < argc - 1; i++) {
                /* Advance to the NUL terminator */
                is += strlen(is);

                /* Change the NUL to a space, ensuring we don't accidentally remove the terminal NUL */
                if (is < &procargs[len-1]) {
                    *is = ' ';
                }
            }

            *cmdargs = strdup(args);
        }

        free(procargs);
}

void DetermineClientCmdApple(pid_t pid, const char **cmdname, const char **cmdargs) {
        /* There should be a check here for the macOS version.
         * We are assuming that it's recent enough like the code previously did. */
        determine_client_cmd_using_libdispatch(pid, cmdname, cmdargs);
}
