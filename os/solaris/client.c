/**
 * @file client.c
 *
 * This file contains X11 client identification code implemented specifically
 * for SunOS/Solaris targets. Because pre-11.3.5 Solaris has no /proc/n/cmdline API,
 * two cases for implementing DetermineClientCmd are done - One using the former, and one
 * using the /proc/n/psinfo file. Detection is done at runtime to choose either. Code here
 * was originally written by Alan Coopersmith.
 *
 * Author: Aggelos Tselios <aggelostselios777@gmail.com>
 * Date: 06/2026
 */


#include "client_solaris.h"
#include "os.h"
#include <errno.h>
#include <dix-config.h>
#include <fcntl.h>
#include <procfs.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

static void determine_client_cmd_using_cmdline(char *path, int fd, const char **cmdname,
                                               const char **cmdargs) {
        /* Read the contents of /proc/pid/cmdline. It should contain the
         * process name and arguments. */
        ssize_t totsize = read(fd, path, PATH_MAX + 1);
        if (totsize <= 0)
                return;
        path[totsize - 1] = '\0';

        /* Construct the process name without arguments. */
        if (cmdname) {
                *cmdname = strdup(path);
        }

        /* Construct the arguments for client process. */
        if (cmdargs) {
                size_t cmdsize = strlen(path) + 1;
                size_t argsize = totsize - cmdsize;
                char *args = NULL;

                if (argsize > 0)
                        args = calloc(1, argsize);
                if (args) {
                        int i = 0;

                        for (i = 0; i < (argsize - 1); ++i) {
                                const char c = path[cmdsize + i];

                                args[i] = (c == '\0') ? ' ' : c;
                        }
                        args[argsize - 1] = '\0';
                        *cmdargs = args;
                }
        }
}

/* Solaris prior to 11.3.5 does not support /proc/pid/cmdline, but
 * makes information similar to what ps shows available in a binary
 * structure in the /proc/pid/psinfo file. */
static void determine_client_cmd_using_psinfo(const char *path, const char **cmdname,
                                              const char **cmdargs) {
        int fd = open(path, O_RDONLY);
        if (fd < 0) {
                ErrorF("Failed to open %s: %s\n", path, strerror(errno));
                return;
        } else {
                psinfo_t psinfo = {0};

                ssize_t totsize = read(fd, &psinfo, sizeof(psinfo_t));
                close(fd);
                if (totsize <= 0)
                        return;

                /* pr_psargs is the first PRARGSZ (80) characters of the command
                 * line string - assume up to the first space is the command
                 * name, since it's not delimited.   While there is also
                 * pr_fname, that's more limited, giving only the first 16 chars
                 * of the basename of the file that was exec'ed, thus cutting
                 * off many long gnome command names, or returning
                 * "isapython2.6" for all python scripts.
                 */
                psinfo.pr_psargs[PRARGSZ - 1] = '\0';
                char *sp = strchr(psinfo.pr_psargs, ' ');
                if (sp)
                        *sp++ = '\0';

                if (cmdname)
                        *cmdname = strdup(psinfo.pr_psargs);

                if (cmdargs && sp)
                        *cmdargs = strdup(sp);
        }
}

void DetermineClientCmdSolaris(pid_t pid, const char **cmdname,
                               const char **cmdargs) {
        /*
         * see comment in determine_client_cmd_using_psinfo
         * to avoid allocating the same path twice, we are
         * formatting it here and passing it to the function that will actually
         * be used
         */
        char path[PATH_MAX + 1] = { 0 };
        if (snprintf(path, sizeof(path), "/proc/%d/cmdline", pid) < 0)
                return;
        int fd = open(path, O_RDONLY);
        if (fd < 0) {
                if (snprintf(path, sizeof(path), "/proc/%d/psinfo", pid) >= 0)
                        determine_client_cmd_using_psinfo(path, cmdname, cmdargs);
        } else {
                determine_client_cmd_using_cmdline(path, fd, cmdname, cmdargs);
                close(fd);
        }
}
