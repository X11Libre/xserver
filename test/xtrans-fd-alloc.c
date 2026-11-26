/* SPDX-License-Identifier: MIT */

/* Test relies on assert(). */
#undef NDEBUG

#include <dix-config.h>

#include "tests-common.h"

#ifdef LDWRAP_TESTS
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>

void *__real_malloc(size_t size);
void *__wrap_malloc(size_t size);

static int malloc_fail_after = -1;

void *
__wrap_malloc(size_t size)
{
    if (malloc_fail_after == 0) {
        malloc_fail_after = -1;
        errno = ENOMEM;
        return NULL;
    }
    if (malloc_fail_after > 0)
        malloc_fail_after--;
    return __real_malloc(size);
}
#endif

#if defined(__linux__) && defined(XTRANS_SEND_FDS) && defined(LDWRAP_TESTS)

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "../os/Xtrans.h"

static void
open_xtrans_local_connection(XtransConnInfo *server, int *client_fd)
{
    XtransConnInfo listener;
    struct sockaddr_un addr;
    char path[sizeof(addr.sun_path)];

    snprintf(path, sizeof(path), "/tmp/xtrans-fd-alloc-%ld.sock",
             (long) getpid());

    listener = _XSERVTransOpenCOTSServer(path);
    assert(listener != NULL);
    assert(_XSERVTransCreateListener(listener, path, 0) == 0);

    *client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    assert(*client_fd >= 0);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    assert(strlen(path) < sizeof(addr.sun_path));
    strcpy(addr.sun_path, path);
    assert(connect(*client_fd, (struct sockaddr *) &addr,
                   sizeof(addr)) == 0);

    *server = _XSERVTransAccept(listener);
    assert(*server != NULL);

    assert(_XSERVTransClose(listener) == 0);
    unlink(path);
}

static int
first_free_fd(void)
{
    int fd;

    for (fd = 3; fd < 1024; fd++) {
        errno = 0;
        if (fcntl(fd, F_GETFD) < 0 && errno == EBADF)
            return fd;
    }
    assert(!"no free descriptor slot found");
    return -1;
}

static void
send_fds(int sock, const int *fds, size_t nfds)
{
    char byte = 'F';
    char control[CMSG_SPACE(2 * sizeof(int))] = { 0 };
    struct iovec iov = {
        .iov_base = &byte,
        .iov_len = sizeof(byte),
    };
    struct msghdr msg = {
        .msg_iov = &iov,
        .msg_iovlen = 1,
        .msg_control = control,
        .msg_controllen = CMSG_SPACE(nfds * sizeof(int)),
    };
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);

    assert(nfds > 0 && nfds <= 2);
    assert(cmsg != NULL);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(nfds * sizeof(int));
    memcpy(CMSG_DATA(cmsg), fds, nfds * sizeof(int));
    assert(sendmsg(sock, &msg, 0) == 1);
}

/*
 * SendFd() must report ENOMEM without consuming or closing the caller's fd.
 * The do_close flag only transfers cleanup responsibility after queueing
 * succeeds.
 */
static void
xtrans_send_fd_alloc_failure_test(void)
{
    XtransConnInfo server;
    int client, pipefd[2];

    open_xtrans_local_connection(&server, &client);
    assert(pipe(pipefd) == 0);

    malloc_fail_after = 0;
    errno = 0;
    assert(_XSERVTransSendFd(server, pipefd[0], 1) == -1);
    assert(errno == ENOMEM);
    assert(fcntl(pipefd[0], F_GETFD) >= 0);

    close(pipefd[0]);
    close(pipefd[1]);
    close(client);
    assert(_XSERVTransClose(server) == 0);
}

/*
 * Fail while appending the second descriptor from a recvmsg().  The
 * connection's previously queued descriptors, the first descriptor from
 * this message, and the descriptor whose queue-node allocation failed must
 * all be closed; no partial queue may remain consumable.
 */
static void
xtrans_recv_fd_alloc_failure_test(void)
{
    XtransConnInfo server;
    int client, first_pipe[2], second_pipe[2], third_pipe[2];
    int first_received, second_received, third_received;
    int first_group[1], second_group[2];
    char byte;

    open_xtrans_local_connection(&server, &client);
    assert(pipe(first_pipe) == 0);
    assert(pipe(second_pipe) == 0);
    assert(pipe(third_pipe) == 0);

    first_received = first_free_fd();
    first_group[0] = first_pipe[0];
    send_fds(client, first_group, 1);
    assert(_XSERVTransRead(server, &byte, 1) == 1);
    assert(byte == 'F');
    assert(fcntl(first_received, F_GETFD) >= 0);

    second_received = first_free_fd();
    third_received = second_received + 1;
    while (fcntl(third_received, F_GETFD) >= 0)
        third_received++;
    second_group[0] = second_pipe[0];
    second_group[1] = third_pipe[0];
    send_fds(client, second_group, 2);

    /* First append succeeds; the second allocation fails. */
    malloc_fail_after = 1;
    errno = 0;
    assert(_XSERVTransRead(server, &byte, 1) == -1);
    assert(errno == ENOMEM);

    assert(fcntl(first_received, F_GETFD) == -1 && errno == EBADF);
    errno = 0;
    assert(fcntl(second_received, F_GETFD) == -1 && errno == EBADF);
    errno = 0;
    assert(fcntl(third_received, F_GETFD) == -1 && errno == EBADF);
    assert(_XSERVTransRecvFd(server) == -1);

    close(first_pipe[0]);
    close(first_pipe[1]);
    close(second_pipe[0]);
    close(second_pipe[1]);
    close(third_pipe[0]);
    close(third_pipe[1]);
    close(client);
    assert(_XSERVTransClose(server) == 0);
}

#endif /* __linux__ && XTRANS_SEND_FDS && LDWRAP_TESTS */

const testfunc_t *
xtrans_fd_alloc_test(void)
{
    static const testfunc_t testfuncs[] = {
#if defined(__linux__) && defined(XTRANS_SEND_FDS) && defined(LDWRAP_TESTS)
        xtrans_send_fd_alloc_failure_test,
        xtrans_recv_fd_alloc_failure_test,
#endif
        NULL,
    };
    return testfuncs;
}
