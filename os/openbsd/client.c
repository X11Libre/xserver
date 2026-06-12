/**
 * @file client.c
 *
 * This file contains X11 client identification code implemented specifically
 * for OpenBSD targets. It is compiled only on OpenBSD targets. A subset of the
 * code here (Mostly kvm calls) is also compatible with FreeBSD.
 *
 * Author: Aggelos Tselios <aggelostselios777@gmail.com>
 * Date: 06/2026
 */

#include "client_openbsd.h"
#include <dix-config.h>

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/types.h>

#include <kvm.h>
#include <limits.h>

void DetermineClientCmdOpenBSD(pid_t pid, const char **cmdname,
                               const char **cmdargs) {
        kvm_t *kd;
        char errbuf[_POSIX2_LINE_MAX];
        char **argv;
        struct kinfo_proc *kp;
        size_t len = 0;
        int i, n;

        kd = kvm_open(NULL, NULL, NULL, KVM_NO_FILES, errbuf);
        if (kd == NULL)
                return;
        kp =
            kvm_getprocs(kd, KERN_PROC_PID, pid, sizeof(struct kinfo_proc), &n);
        if (n != 1)
                goto done_kvm;
        argv = kvm_getargv(kd, kp, 0);
        if (argv == NULL)
                goto done_kvm;
        if (cmdname) {
                if (argv[0] == NULL)
                        goto done_kvm;
                else
                        *cmdname = strdup(argv[0]);
        }
        if (cmdargs) {
                i = 1;
                while (argv[i] != NULL) {
                        len += strlen(argv[i]) + 1;
                        i++;
                }
                *cmdargs = calloc(1, len);
                if (*cmdargs) {
                        i = 1;
                        while (argv[i] != NULL) {
                                strlcat(*(char **)cmdargs, argv[i], len);
                                strlcat(*(char **)cmdargs, " ", len);
                                i++;
                        }
                }
        }
done_kvm:
        kvm_close(kd);
}
