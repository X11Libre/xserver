/* SPDX-License-Identifier: MIT */

/* Test relies on assert() */
#undef NDEBUG

/*
 * Exercise the Linux AF_UNIX SOCK_STREAM ancillary-data framing rules used
 * by Xtrans FD passing.  These tests deliberately cover read-ahead across
 * ordinary bytes, multiple SCM_RIGHTS markers, and control-buffer truncation.
 */

#include <dix-config.h>

#include <assert.h>
#include <stddef.h>

#include "tests-common.h"

#if defined(__linux__)
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static void
send_with_fds(int sock, const char *data, size_t len,
              const int *fds, size_t nfds)
{
    char control[CMSG_SPACE(4 * sizeof(int))];
    struct iovec iov = {
        .iov_base = (char *) data,
        .iov_len = len,
    };
    struct msghdr msg = {
        .msg_iov = &iov,
        .msg_iovlen = 1,
        .msg_control = control,
        .msg_controllen = CMSG_SPACE(nfds * sizeof(int)),
    };
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);

    assert(nfds > 0 && nfds <= 4);
    assert(cmsg != NULL);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(nfds * sizeof(int));
    memcpy(CMSG_DATA(cmsg), fds, nfds * sizeof(int));

    assert(sendmsg(sock, &msg, 0) == (ssize_t) len);
}

static int
recv_one_byte_with_fd(int sock, char expected)
{
    char data = 0;
    char control[CMSG_SPACE(sizeof(int))];
    struct iovec iov = {
        .iov_base = &data,
        .iov_len = sizeof(data),
    };
    struct msghdr msg = {
        .msg_iov = &iov,
        .msg_iovlen = 1,
        .msg_control = control,
        .msg_controllen = sizeof(control),
    };
    struct cmsghdr *cmsg;
    int fd = -1;

    assert(recvmsg(sock, &msg, 0) == 1);
    assert(data == expected);
    assert((msg.msg_flags & MSG_CTRUNC) == 0);

    cmsg = CMSG_FIRSTHDR(&msg);
    assert(cmsg != NULL);
    assert(cmsg->cmsg_level == SOL_SOCKET);
    assert(cmsg->cmsg_type == SCM_RIGHTS);
    assert(cmsg->cmsg_len == CMSG_LEN(sizeof(int)));
    memcpy(&fd, CMSG_DATA(cmsg), sizeof(fd));
    assert(fd >= 0);
    assert(CMSG_NXTHDR(&msg, cmsg) == NULL);

    return fd;
}

/*
 * Linux AF_UNIX stream sockets may return ordinary bytes preceding an
 * SCM_RIGHTS-bearing sendmsg() together with the complete data from that
 * sendmsg().  The ancillary data is not accompanied by a byte offset, so
 * the receiver cannot infer the exact marker position from the read length.
 */
static void
xtrans_fd_read_ahead_batch_test(void)
{
    int sockets[2];
    int pipefd[2];
    char data[16] = { 0 };
    char control[CMSG_SPACE(sizeof(int))];
    struct iovec iov = {
        .iov_base = data,
        .iov_len = sizeof(data),
    };
    struct msghdr msg = {
        .msg_iov = &iov,
        .msg_iovlen = 1,
        .msg_control = control,
        .msg_controllen = sizeof(control),
    };
    struct cmsghdr *cmsg;
    int received_fd = -1;
    ssize_t n;

    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    assert(pipe(pipefd) == 0);

    assert(write(sockets[0], "prior", 5) == 5);
    send_with_fds(sockets[0], "Mtail", 5, &pipefd[0], 1);

    n = recvmsg(sockets[1], &msg, 0);
    assert(n == 10);
    assert(memcmp(data, "priorMtail", 10) == 0);
    assert((msg.msg_flags & MSG_CTRUNC) == 0);

    cmsg = CMSG_FIRSTHDR(&msg);
    assert(cmsg != NULL);
    assert(cmsg->cmsg_level == SOL_SOCKET);
    assert(cmsg->cmsg_type == SCM_RIGHTS);
    assert(cmsg->cmsg_len == CMSG_LEN(sizeof(int)));
    memcpy(&received_fd, CMSG_DATA(cmsg), sizeof(received_fd));
    assert(received_fd >= 0);
    assert(CMSG_NXTHDR(&msg, cmsg) == NULL);

    close(received_fd);
    close(pipefd[0]);
    close(pipefd[1]);
    close(sockets[0]);
    close(sockets[1]);
}

/* Each SCM_RIGHTS sendmsg() creates a distinct marker boundary. */
static void
xtrans_fd_multiple_markers_test(void)
{
    int sockets[2];
    int first_pipe[2], second_pipe[2];
    int first_fd, second_fd;

    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    assert(pipe(first_pipe) == 0);
    assert(pipe(second_pipe) == 0);

    send_with_fds(sockets[0], "A", 1, &first_pipe[0], 1);
    send_with_fds(sockets[0], "B", 1, &second_pipe[0], 1);

    first_fd = recv_one_byte_with_fd(sockets[1], 'A');
    second_fd = recv_one_byte_with_fd(sockets[1], 'B');
    assert(first_fd != second_fd);

    close(first_fd);
    close(second_fd);
    close(first_pipe[0]);
    close(first_pipe[1]);
    close(second_pipe[0]);
    close(second_pipe[1]);
    close(sockets[0]);
    close(sockets[1]);
}

/* A short ancillary buffer must be reported as truncated, never accepted. */
static void
xtrans_fd_truncated_control_test(void)
{
    int sockets[2];
    int pipes[4][2], fds[4];
    int i;
    char byte;
    char control[CMSG_SPACE(sizeof(int))];
    struct iovec iov = {
        .iov_base = &byte,
        .iov_len = sizeof(byte),
    };
    struct msghdr msg = {
        .msg_iov = &iov,
        .msg_iovlen = 1,
        .msg_control = control,
        .msg_controllen = sizeof(control),
    };
    struct cmsghdr *cmsg;

    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    for (i = 0; i < 4; i++) {
        assert(pipe(pipes[i]) == 0);
        fds[i] = pipes[i][0];
    }

    /* One-FD CMSG_SPACE is too small for this four-FD control message. */
    send_with_fds(sockets[0], "T", 1, fds, 4);
    assert(recvmsg(sockets[1], &msg, 0) == 1);
    assert(byte == 'T');
    assert((msg.msg_flags & MSG_CTRUNC) != 0);

    /* Close every descriptor the kernel did deliver. */
    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg;
         cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_level == SOL_SOCKET &&
            cmsg->cmsg_type == SCM_RIGHTS) {
            size_t bytes = cmsg->cmsg_len - CMSG_LEN(0);
            size_t fd_index;
            int *received = (int *) CMSG_DATA(cmsg);

            assert(bytes % sizeof(int) == 0);
            for (fd_index = 0; fd_index < bytes / sizeof(int); fd_index++)
                close(received[fd_index]);
        }
    }

    for (i = 0; i < 4; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    close(sockets[0]);
    close(sockets[1]);
}
#endif /* __linux__ */

const testfunc_t *
xtrans_fd_test(void)
{
    static const testfunc_t testfuncs[] = {
#if defined(__linux__)
        xtrans_fd_read_ahead_batch_test,
        xtrans_fd_multiple_markers_test,
        xtrans_fd_truncated_control_test,
#endif
        NULL,
    };
    return testfuncs;
}
