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
#include "client_freebsd.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dix.h"
#include "os.h"
#include <errno.h>
#include <sys/sysctl.h>

/*
 * Originally implemented by Jan Beich. This implementation is also compatible with DragonflyBSD. 
 */
void DetermineClientCmdFreeBSD(pid_t pid, const char **cmdname,
                               const char **cmdargs) {
        int mib[] = {
            CTL_KERN,
            KERN_PROC,
            KERN_PROC_ARGS,
            pid,
        };

        /* Determine exact size instead of relying on kern.argmax */
        size_t len;
        if (sysctl(mib, ARRAY_SIZE(mib), NULL, &len, NULL, 0) != 0) {
                ErrorF("Failed to query KERN_PROC_ARGS length for PID %d: %s\n",
                       pid, strerror(errno));
                return;
        }

        /* Read KERN_PROC_ARGS contents. Similar to /proc/pid/cmdline
         * the process name and each argument are separated by NUL byte. */
        char *const procargs = calloc(1, len);
        if (!procargs) {
                ErrorF("Failed to allocate memory (%zu bytes) for KERN_PROC_ARGS result for pid %d: %s\n", len, pid, strerror(errno));
                return;
        }
                
        if (sysctl(mib, ARRAY_SIZE(mib), procargs, &len, NULL, 0) != 0) {
                ErrorF("Failed to get KERN_PROC_ARGS for PID %d: %s\n", pid,
                       strerror(errno));
                free(procargs);
                return;
        }

        /* Construct the process name without arguments. */
        if (cmdname) {
                *cmdname = strdup(procargs);
        }

        /* Construct the arguments for client process. */
        if (cmdargs) {
                size_t cmdsize = strlen(procargs) + 1;
                size_t argsize = len - cmdsize;
                char *args = NULL;

                if (argsize > 0)
                        args = procargs + cmdsize;
                if (args) {
                        /* Replace NUL with space except terminating NUL */
                        for (size_t i = 0; i < (argsize - 1); i++) {
                                if (args[i] == '\0')
                                        args[i] = ' ';
                        }
                        *cmdargs = strdup(args);
                }
        }
        free(procargs);
}
