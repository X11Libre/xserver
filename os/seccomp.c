#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <errno.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>

#include "os.h"

#ifdef HAVE_SECCOMP
#include <sys/prctl.h>
#include <linux/seccomp.h>
#include <linux/filter.h>
#include <linux/audit.h>
#include <stddef.h>

#ifndef SECCOMP_MODE_FILTER
#define SECCOMP_MODE_FILTER	2 /* uses user-supplied filter. */
#endif
#ifndef PR_SET_NO_NEW_PRIVS
#define PR_SET_NO_NEW_PRIVS 38
#endif
#ifndef SYS_socket
#if defined(__i386__)
#define SYS_socket 359
#elif defined(__x86_64__)
#define SYS_socket 41
#elif defined(__arm__)
#define SYS_socket 281
#elif defined(__aarch64__)
#define SYS_socket 198
#else
#error "Unsupported architecture"
#endif
#endif

static int
seccomp_set_filter(void)
{
    struct sock_filter filter[] = {
        /* 1. Load syscall number */
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (offsetof(struct seccomp_data, nr))),
        /* 2. Is it socket()? if not, jump to allow. */
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_socket, 0, 3),
        /* 3. It is socket(). Load domain argument. */
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (offsetof(struct seccomp_data, args[0]))),
        /* 4. Is it AF_UNIX? if so, jump to allow. */
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, AF_UNIX, 1, 0),
        /* 5. Not AF_UNIX. Kill. */
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL),
        /* 6. Allow. */
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
    };

    struct sock_fprog prog = {
        .len = (unsigned short)(sizeof(filter) / sizeof(filter[0])),
        .filter = filter,
    };

    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)) {
        ErrorF("prctl(PR_SET_NO_NEW_PRIVS) failed: %s\n", strerror(errno));
        return -1;
    }

    if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog)) {
        ErrorF("prctl(PR_SET_SECCOMP) failed: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

#endif /* HAVE_SECCOMP */

void
OsSeccompInit(void)
{
#ifdef HAVE_SECCOMP
    if (seccomp_set_filter()) {
        FatalError("Failed to set seccomp filter\n");
    }
#endif
}
