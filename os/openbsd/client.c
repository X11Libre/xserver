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
#include "client_openbsd.h"

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
