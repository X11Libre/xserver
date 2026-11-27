/* SPDX-License-Identifier: MIT */
/* Copyright 2026 Enrico Weigelt <info@metux.net> */
#undef NDEBUG
#include <dix-config.h>

#include <stddef.h>
#include "tests-common.h"
#if defined(__linux__) && defined(XTRANS_SEND_FDS)
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "../os/Xtrans.h"
#define TEST_FD_COUNT 129
static void open_xtrans_local_connection(XtransConnInfo *server, int *client_fd)
{
    XtransConnInfo listener;
    struct sockaddr_un addr;
    char path[sizeof(addr.sun_path)];
    snprintf(path, sizeof(path), "/tmp/xtrans-fd-trunc-%ld.sock",
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
    assert(connect(*client_fd, (struct sockaddr *) &addr, sizeof(addr)) == 0);
    *server = _XSERVTransAccept(listener);
    assert(*server != NULL);
    assert(_XSERVTransClose(listener) == 0);
    unlink(path);
}
static void collect_free_fds(int *fds, int count)
{
    int fd;
    int n = 0;

    for (fd = 3; fd < 1024 && n < count; fd++) {
        errno = 0;
        if (fcntl(fd, F_GETFD) < 0 && errno == EBADF) {
            fds[n++] = fd;
        }
    }
    assert(n == count);
}
static void send_fds(int sock, const int *fds, int count)
{
    char byte = 'T';
    char control[CMSG_SPACE(TEST_FD_COUNT * sizeof(int))] = { 0 };
    struct iovec iov = { .iov_base = &byte, .iov_len = sizeof(byte) };
    struct msghdr msg = {
        .msg_iov = &iov, .msg_iovlen = 1,
        .msg_control = control, .msg_controllen = CMSG_SPACE(count * sizeof(int)),
    };
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    assert(cmsg != NULL);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(count * sizeof(int));
    memcpy(CMSG_DATA(cmsg), fds, count * sizeof(int));
    assert(sendmsg(sock, &msg, 0) == 1);
}
static void xtrans_recv_fd_truncated_control_test(void)
{
    XtransConnInfo server;
    int client, pipes[TEST_FD_COUNT][2], fds[TEST_FD_COUNT];
    int expected_fds[TEST_FD_COUNT], i;
    int prior_pipe[2], prior_received_fd;
    char byte;
    open_xtrans_local_connection(&server, &client);
    assert(pipe(prior_pipe) == 0);
    collect_free_fds(&prior_received_fd, 1);
    send_fds(client, &prior_pipe[0], 1);
    assert(_XSERVTransRead(server, &byte, 1) == 1);
    assert(byte == 'T');
    for (i = 0; i < TEST_FD_COUNT; i++) {
        assert(pipe(pipes[i]) == 0);
        fds[i] = pipes[i][0];
    }
    /*
     * recvmsg() installs received descriptors in the lowest available slots.
     * Record the actual free slots rather than assuming they are contiguous.
     */
    collect_free_fds(expected_fds, TEST_FD_COUNT);
    send_fds(client, fds, TEST_FD_COUNT);
    errno = 0;
    assert(_XSERVTransRead(server, &byte, 1) == -1);
    assert(errno == EMSGSIZE);
    assert(_XSERVTransRecvFd(server) == -1);
    errno = 0;
    assert(fcntl(prior_received_fd, F_GETFD) == -1);
    assert(errno == EBADF);
    for (i = 0; i < TEST_FD_COUNT; i++) {
        errno = 0;
        assert(fcntl(expected_fds[i], F_GETFD) == -1);
        assert(errno == EBADF);
    }
    for (i = 0; i < TEST_FD_COUNT; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    close(prior_pipe[0]);
    close(prior_pipe[1]);
    close(client);
    assert(_XSERVTransClose(server) == 0);
}
#endif /* __linux__ && XTRANS_SEND_FDS */
const testfunc_t *xtrans_fd_trunc_test(void)
{
    static const testfunc_t testfuncs[] = {
#if defined(__linux__) && defined(XTRANS_SEND_FDS)
        xtrans_recv_fd_truncated_control_test,
#endif
        NULL,
    };
    return testfuncs;
}
