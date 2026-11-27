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

static void open_xtrans_local_connection(XtransConnInfo *server, int *client_fd)
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
    assert(connect(*client_fd, (struct sockaddr *) &addr, sizeof(addr)) == 0);
    *server = _XSERVTransAccept(listener);
    assert(*server != NULL);
    assert(_XSERVTransClose(listener) == 0);
    unlink(path);
}

static int first_free_fd(void)
{
    int fd;

    for (fd = 3; fd < 1024; fd++) {
        errno = 0;
        if (fcntl(fd, F_GETFD) < 0 && errno == EBADF) {
            return fd;
        }
    }
    assert(!"no free descriptor slot found");
    return -1;
}

static void send_bytes_with_fds(int sock, const char *data, size_t size,
                    const int *fds, size_t nfds)
{
    char control[CMSG_SPACE(2 * sizeof(int))] = { 0 };
    struct iovec iov = { .iov_base = (void *) data, .iov_len = size };
    struct msghdr msg = {
        .msg_iov = &iov,
        .msg_iovlen = 1,
        .msg_control = control,
        .msg_controllen = CMSG_SPACE(nfds * sizeof(int)),
    };
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);

    assert(size > 0);
    assert(nfds > 0 && nfds <= 2);
    assert(cmsg != NULL);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(nfds * sizeof(int));
    memcpy(CMSG_DATA(cmsg), fds, nfds * sizeof(int));
    assert(sendmsg(sock, &msg, 0) == (ssize_t) size);
}

static void send_fds(int sock, const int *fds, size_t nfds)
{
    send_bytes_with_fds(sock, "F", 1, fds, nfds);
}

/* The getter reports the number of descriptors received by this read only. */
static void xtrans_read_fd_count_test(void)
{
    XtransConnInfo server;
    int client, first_pipe[2], second_pipe[2], third_pipe[2];
    int first_fds[2], second_fds[1], received_fd;
    char data[4];

    open_xtrans_local_connection(&server, &client);
    assert(pipe(first_pipe) == 0);
    assert(pipe(second_pipe) == 0);
    assert(pipe(third_pipe) == 0);
    first_fds[0] = first_pipe[0];
    first_fds[1] = second_pipe[0];
    second_fds[0] = third_pipe[0];

    assert(write(client, "REQ0", 4) == 4);
    send_bytes_with_fds(client, "FD", 2, first_fds, 2);
    send_bytes_with_fds(client, "RE", 2, second_fds, 1);
    assert(write(client, "Q!!!", 4) == 4);

    assert(_XSERVTransRead(server, data, 4) == 4);
    assert(memcmp(data, "REQ0", 4) == 0);
    assert(_XSERVTransGetLastReadFdCount(server) == 0);
    assert(_XSERVTransRecvFd(server) < 0);

    assert(_XSERVTransRead(server, data, 4) == 2);
    assert(memcmp(data, "FD", 2) == 0);
    assert(_XSERVTransGetLastReadFdCount(server) == 2);
    received_fd = _XSERVTransRecvFd(server);
    assert(received_fd >= 0);
    close(received_fd);
    received_fd = _XSERVTransRecvFd(server);
    assert(received_fd >= 0);
    close(received_fd);
    assert(_XSERVTransRecvFd(server) < 0);

    assert(_XSERVTransRead(server, data, 4) == 2);
    assert(memcmp(data, "RE", 2) == 0);
    assert(_XSERVTransGetLastReadFdCount(server) == 1);
    received_fd = _XSERVTransRecvFd(server);
    assert(received_fd >= 0);
    close(received_fd);
    assert(_XSERVTransRecvFd(server) < 0);

    assert(_XSERVTransRead(server, data, 4) == 4);
    assert(memcmp(data, "Q!!!", 4) == 0);
    assert(_XSERVTransGetLastReadFdCount(server) == 0);
    assert(_XSERVTransRecvFd(server) < 0);

    close(first_pipe[0]);
    close(first_pipe[1]);
    close(second_pipe[0]);
    close(second_pipe[1]);
    close(third_pipe[0]);
    close(third_pipe[1]);
    close(client);
    assert(_XSERVTransClose(server) == 0);
}

/* The fixed receive queue fails closed instead of growing dynamically. */
static void xtrans_recv_fd_queue_overflow_test(void)
{
    enum { QUEUE_CAPACITY = 128, OVERFLOW_COUNT = QUEUE_CAPACITY + 1 };
    XtransConnInfo server;
    int client, pipes[OVERFLOW_COUNT][2], received_fds[OVERFLOW_COUNT];
    int i, sent_fd;
    char byte;

    open_xtrans_local_connection(&server, &client);
    for (i = 0; i < OVERFLOW_COUNT; i++)
        assert(pipe(pipes[i]) == 0);

    for (i = 0; i < QUEUE_CAPACITY; i++) {
        sent_fd = pipes[i][0];
        send_fds(client, &sent_fd, 1);
        received_fds[i] = first_free_fd();
        assert(_XSERVTransRead(server, &byte, 1) == 1);
        assert(byte == 'F');
        assert(_XSERVTransGetLastReadFdCount(server) == 1);
    }

    sent_fd = pipes[QUEUE_CAPACITY][0];
    send_fds(client, &sent_fd, 1);
    received_fds[QUEUE_CAPACITY] = first_free_fd();
    errno = 0;
    assert(_XSERVTransRead(server, &byte, 1) == -1);
    assert(errno == ENOBUFS);

    assert(_XSERVTransGetLastReadFdCount(server) == 0);
    assert(_XSERVTransRecvFd(server) == -1);
    for (i = 0; i < OVERFLOW_COUNT; i++) {
        errno = 0;
        assert(fcntl(received_fds[i], F_GETFD) == -1 && errno == EBADF);
    }

    for (i = 0; i < OVERFLOW_COUNT; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    close(client);
    assert(_XSERVTransClose(server) == 0);
}


/* Closing a local Xtrans connection closes descriptors still in its queue. */
static void xtrans_recv_fd_close_cleanup_test(void)
{
    XtransConnInfo server;
    int client, pipe_fds[2], received_fd;
    char byte;
    int sent_fd;

    open_xtrans_local_connection(&server, &client);
    assert(pipe(pipe_fds) == 0);
    received_fd = first_free_fd();
    sent_fd = pipe_fds[0];
    send_fds(client, &sent_fd, 1);
    assert(_XSERVTransRead(server, &byte, 1) == 1);
    assert(byte == 'F');
    assert(_XSERVTransGetLastReadFdCount(server) == 1);

    close(client);
    assert(_XSERVTransClose(server) == 0);
    errno = 0;
    assert(fcntl(received_fd, F_GETFD) == -1 && errno == EBADF);

    close(pipe_fds[0]);
    close(pipe_fds[1]);
}

#endif /* __linux__ && XTRANS_SEND_FDS */

const testfunc_t *xtrans_fd_alloc_test(void)
{
    static const testfunc_t testfuncs[] = {
#if defined(__linux__) && defined(XTRANS_SEND_FDS)
        xtrans_read_fd_count_test,
        xtrans_recv_fd_queue_overflow_test,
        xtrans_recv_fd_close_cleanup_test,
#endif
        NULL,
    };
    return testfuncs;
}
