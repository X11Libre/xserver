/**
 * @file client.c
 *
 * This file contains X11 client identification code implemented specifically
 * for FreeBSD-based targets (that also includes GhostBSD and other operating
 * systems). Much of this code comes from os/client.c, but moved into a separate
 * file to clean up the former. This file is part of the XLibre project and
 * provided under the same license.
 *
 * Author: Aggelos Tselios <aggelostselios777@gmail.com>
 * Date: 06/2026
 */

#include "client_freebsd.h"
#include <dix-config.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "dix.h"
#include "os.h"
#include <errno.h>
#include <sys/sysctl.h>

/*
 * Originally implemented by Rami Ylimäki. This implementation is also compatible with DragonflyBSD. 
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
