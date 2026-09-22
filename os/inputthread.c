/* inputthread.c -- Threaded generation of input events.
 *
 * Copyright © 2007-2008 Tiago Vignatti <vignatti at freedesktop org>
 * Copyright © 2010 Nokia
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Fernando Carrijo <fcarrijo at freedesktop org>
 *          Tiago Vignatti <vignatti at freedesktop org>
 */

#include <dix-config.h>

#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "dix/input_priv.h"
#include "os/ddx_priv.h"
#include "os/log_priv.h"
#include "os/ossock.h"

#include "inputstr.h"
#include "opaque.h"
#include "osdep.h"

#if defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#include <io.h>
#define PIPE_FD SOCKET
#define PIPE_CLOSE closesocket
#define PIPE_INVALID INVALID_SOCKET
#define PIPE_NONBLOCK(fd) do { u_long nonblock = 1; ioctlsocket((SOCKET)(fd), FIONBIO, &nonblock); } while(0)
#define PIPE_MAKE_INHERIT(fd) SetHandleInformation((HANDLE)(fd), HANDLE_FLAG_INHERIT, 0)
#else
#include <fcntl.h>
#include <signal.h>
#define PIPE_FD int
#define PIPE_CLOSE close
#define PIPE_INVALID -1
#define PIPE_NONBLOCK(fd) do { fcntl((fd), F_SETFL, O_NONBLOCK); } while(0)
#define PIPE_MAKE_INHERIT(fd) do { int flags = fcntl((fd), F_GETFD); if (flags != -1) { flags |= FD_CLOEXEC; fcntl((fd), F_SETFD, flags); } } while(0)
#endif

#if INPUTTHREAD

Bool InputThreadEnable = TRUE;

/**
 * An input device as seen by the threaded input facility
 */

typedef enum _InputDeviceState {
    device_state_added,
    device_state_running,
    device_state_removed
} InputDeviceState;

typedef struct _InputThreadDevice {
    struct xorg_list node;
    NotifyFdProcPtr readInputProc;
    void *readInputArgs;
    int fd;
    InputDeviceState state;
} InputThreadDevice;

/**
 * The threaded input facility.
 *
 * For now, we have one instance for all input devices.
 */
typedef struct {
    pthread_t thread;
    struct xorg_list devs;
    struct ospoll *fds;
    PIPE_FD readPipe;
    PIPE_FD writePipe;
    bool changed;
    bool running;
} InputThreadInfo;

static InputThreadInfo *inputThreadInfo;

static PIPE_FD hotplugPipeRead = PIPE_INVALID;
static PIPE_FD hotplugPipeWrite = PIPE_INVALID;

static int input_mutex_count;

#ifdef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
static pthread_mutex_t input_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#else
static pthread_mutex_t input_mutex;
static Bool input_mutex_initialized;
#endif

int
in_input_thread(void)
{
    return inputThreadInfo &&
           pthread_equal(pthread_self(), inputThreadInfo->thread);
}

void
input_lock(void)
{
#ifndef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
    if (!input_mutex_initialized) {
        pthread_mutexattr_t mutex_attr;

        input_mutex_initialized = TRUE;
        pthread_mutexattr_init(&mutex_attr);
        pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&input_mutex, &mutex_attr);
    }
#endif
    pthread_mutex_lock(&input_mutex);
    ++input_mutex_count;
}

void
input_unlock(void)
{
    --input_mutex_count;
    pthread_mutex_unlock(&input_mutex);
}

void
input_force_unlock(void)
{
    if (pthread_mutex_trylock(&input_mutex) == 0) {
        input_mutex_count++;
        /* unlock +1 times for the trylock */
        while (input_mutex_count > 0)
            input_unlock();
    }
}

/**
 * Notify a thread about the availability of new asynchronously enqueued input
 * events.
 *
 * @see WaitForSomething()
 */
static void
InputThreadFillPipe(PIPE_FD writeHead)
{
    int ret;
    char byte = 0;

#if defined(_WIN32)
    do {
        ret = send(writeHead, &byte, 1, 0);
    } while (ret == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK);
#else
    do {
        ret = write(writeHead, &byte, 1);
    } while (ret < 0 && ossock_wouldblock(errno));
#endif
}

/**
 * Consume eventual notifications left by a thread.
 *
 * @see WaitForSomething()
 * @see InputThreadFillPipe()
 */
static int
InputThreadReadPipe(PIPE_FD readHead)
{
    int ret;
    char array[10];

#if defined(_WIN32)
    ret = recv(readHead, array, sizeof(array), 0);
    if (ret >= 0)
        return ret;

    if (WSAGetLastError() != WSAEWOULDBLOCK)
        FatalError("input-thread: draining socket (%d)", WSAGetLastError());

    return 1;
#else
    ret = read(readHead, &array, sizeof(array));
    if (ret >= 0)
        return ret;

    if (errno != EAGAIN)
        FatalError("input-thread: draining pipe (%d)", errno);

    return 1;
#endif
}

static void
InputReady(int fd, int xevents, void *data)
{
    InputThreadDevice *dev = data;

    input_lock();
    if (dev->state == device_state_running)
        dev->readInputProc(fd, xevents, dev->readInputArgs);
    input_unlock();
}

/**
 * Register an input device in the threaded input facility
 *
 * @param fd File descriptor which identifies the input device
 * @param readInputProc Procedure used to read input from the device
 * @param readInputArgs Arguments to be consumed by the above procedure
 *
 * return 1 if success; 0 otherwise.
 */
int
InputThreadRegisterDev(int fd,
                       NotifyFdProcPtr readInputProc,
                       void *readInputArgs)
{
    InputThreadDevice *dev, *old;

    if (!inputThreadInfo)
        return SetNotifyFd(fd, readInputProc, X_NOTIFY_READ, readInputArgs);

    input_lock();

    dev = NULL;
    xorg_list_for_each_entry(old, &inputThreadInfo->devs, node) {
        if (old->fd == fd && old->state != device_state_removed) {
            dev = old;
            break;
        }
    }

    if (dev) {
        dev->readInputProc = readInputProc;
        dev->readInputArgs = readInputArgs;
    } else {
        dev = calloc(1, sizeof(InputThreadDevice));
        if (dev == NULL) {
            DebugF("input-thread: could not register device\n");
            input_unlock();
            return 0;
        }

        dev->fd = fd;
        dev->readInputProc = readInputProc;
        dev->readInputArgs = readInputArgs;
        dev->state = device_state_added;

        /* Do not prepend, so that any dev->state == device_state_removed
         * with the same dev->fd get processed first. */
        xorg_list_append(&dev->node, &inputThreadInfo->devs);
    }

    inputThreadInfo->changed = TRUE;

    input_unlock();

    DebugF("input-thread: registered device %d\n", fd);
    InputThreadFillPipe(hotplugPipeWrite);

    return 1;
}

/**
 * Unregister a device in the threaded input facility
 *
 * @param fd File descriptor which identifies the input device
 *
 * @return 1 if success; 0 otherwise.
 */
int
InputThreadUnregisterDev(int fd)
{
    InputThreadDevice *dev;
    bool found_device = FALSE;

    /* return silently if input thread is already finished (e.g., at
     * DisableDevice time, evdev tries to call this function again through
     * xf86RemoveEnabledDevice) */
    if (!inputThreadInfo) {
        RemoveNotifyFd(fd);
        return 1;
    }

    input_lock();
    xorg_list_for_each_entry(dev, &inputThreadInfo->devs, node)
        if (dev->fd == fd) {
            found_device = TRUE;
            break;
        }

    /* fd didn't match any registered device. */
    if (!found_device) {
        input_unlock();
        return 0;
    }

    dev->state = device_state_removed;
    inputThreadInfo->changed = TRUE;

    input_unlock();

    InputThreadFillPipe(hotplugPipeWrite);
    DebugF("input-thread: unregistered device: %d\n", fd);

    return 1;
}

static void
InputThreadPipeNotify(int fd, int revents, void *data)
{
    /* Empty pending input, shut down if the pipe has been closed */
    if (InputThreadReadPipe(hotplugPipeRead) == 0) {
        inputThreadInfo->running = FALSE;
    }
}

/**
 * The workhorse of threaded input event generation.
 *
 * Or if you prefer: The WaitForSomething for input devices. :)
 *
 * Runs in parallel with the server main thread, listening to input devices in
 * an endless loop. Whenever new input data is made available, calls the
 * proper device driver's routines which are ultimately responsible for the
 * generation of input events.
 *
 * @see InputThreadPreInit()
 * @see InputThreadInit()
 */

static void*
InputThreadDoWork(void *arg)
{
#if !defined(_WIN32)
    sigset_t set;

    /* Don't handle any signals on this thread */
    sigfillset(&set);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
#endif

    ddxInputThreadInit();

    inputThreadInfo->running = TRUE;

#if defined(HAVE_PTHREAD_SETNAME_NP_WITH_TID)
    pthread_setname_np (pthread_self(), "InputThread");
#elif defined(HAVE_PTHREAD_SETNAME_NP_WITHOUT_TID)
    pthread_setname_np ("InputThread");
#endif

    ospoll_add(inputThreadInfo->fds, hotplugPipeRead,
               ospoll_trigger_level,
               InputThreadPipeNotify,
               NULL);
    ospoll_listen(inputThreadInfo->fds, hotplugPipeRead, X_NOTIFY_READ);

    while (inputThreadInfo->running)
    {
        DebugF("input-thread: %s waiting for devices\n", __func__);

        /* Check for hotplug changes and modify the ospoll structure to suit */
        if (inputThreadInfo->changed) {
            InputThreadDevice *dev, *tmp;

            input_lock();
            inputThreadInfo->changed = FALSE;
            xorg_list_for_each_entry_safe(dev, tmp, &inputThreadInfo->devs, node) {
                switch (dev->state) {
                case device_state_added:
                    ospoll_add(inputThreadInfo->fds, dev->fd,
                               ospoll_trigger_level,
                               InputReady,
                               dev);
                    ospoll_listen(inputThreadInfo->fds, dev->fd, X_NOTIFY_READ);
                    dev->state = device_state_running;
                    break;
                case device_state_running:
                    break;
                case device_state_removed:
                    ospoll_remove(inputThreadInfo->fds, dev->fd);
                    xorg_list_del(&dev->node);
                    free(dev);
                    break;
                }
            }
            input_unlock();
        }

        if (ospoll_wait(inputThreadInfo->fds, -1) < 0) {
#if defined(_WIN32)
            int err = WSAGetLastError();
            if (err == WSAEINVAL)
                FatalError("input-thread: %s (%d)", __func__, err);
            else if (err != WSAEINTR)
                ErrorF("input-thread: %s (%d)\n", __func__, err);
#else
            if (errno == EINVAL)
                FatalError("input-thread: %s (%s)", __func__, strerror(errno));
            else if (errno != EINTR)
                ErrorF("input-thread: %s (%s)\n", __func__, strerror(errno));
#endif
        }

        /* Kick main thread to process the generated input events and drain
         * events from hotplug pipe */
        InputThreadFillPipe(inputThreadInfo->writePipe);
    }

    ospoll_remove(inputThreadInfo->fds, hotplugPipeRead);

    return NULL;
}

static void
InputThreadNotifyPipe(int fd, int mask, void *data)
{
    InputThreadReadPipe(fd);
}

/**
 * Pre-initialize the facility used for threaded generation of input events
 *
 */
void
InputThreadPreInit(void)
{
    PIPE_FD fds[2], hotplugPipe[2];

    if (!InputThreadEnable)
        return;

#if defined(_WIN32)
    /* Windows doesn't have socketpair(), emulate with loopback connect */
    {
        struct sockaddr_in addr;
        int addr_len = sizeof(addr);
        SOCKET listen_sock = PIPE_INVALID;

        fds[0] = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        fds[1] = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        hotplugPipe[0] = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        hotplugPipe[1] = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

        if (fds[0] == INVALID_SOCKET || fds[1] == INVALID_SOCKET ||
            hotplugPipe[0] == INVALID_SOCKET || hotplugPipe[1] == INVALID_SOCKET) {
            FatalError("input-thread: could not create socketpair");
        }

        /* Make all sockets non-blocking and non-inheritable */
        PIPE_NONBLOCK(fds[0]);
        PIPE_NONBLOCK(fds[1]);
        PIPE_NONBLOCK(hotplugPipe[0]);
        PIPE_NONBLOCK(hotplugPipe[1]);
        PIPE_MAKE_INHERIT(fds[0]);
        PIPE_MAKE_INHERIT(fds[1]);
        PIPE_MAKE_INHERIT(hotplugPipe[0]);
        PIPE_MAKE_INHERIT(hotplugPipe[1]);

        /* Create connected pairs via loopback */
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;

        listen_sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        if (listen_sock == INVALID_SOCKET ||
            bind(listen_sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR ||
            listen(listen_sock, 1) == SOCKET_ERROR ||
            getsockname(listen_sock, (struct sockaddr*)&addr, &addr_len) == SOCKET_ERROR) {
            FatalError("input-thread: socketpair emulation setup failed");
        }

        /* Connect first pair (fds) */
        if (connect(fds[0], (struct sockaddr*)&addr, addr_len) == SOCKET_ERROR ||
            (fds[1] = accept(listen_sock, NULL, NULL)) == INVALID_SOCKET) {
            FatalError("input-thread: socketpair emulation connect failed");
        }

        /* Connect second pair (hotplugPipe) */
        if (getsockname(listen_sock, (struct sockaddr*)&addr, &addr_len) == SOCKET_ERROR) {
            FatalError("input-thread: socketpair emulation getsockname failed");
        }
        if (connect(hotplugPipe[0], (struct sockaddr*)&addr, addr_len) == SOCKET_ERROR ||
            (hotplugPipe[1] = accept(listen_sock, NULL, NULL)) == INVALID_SOCKET) {
            FatalError("input-thread: socketpair emulation connect 2 failed");
        }

        PIPE_CLOSE(listen_sock);
    }
#else
    if (pipe(fds) < 0)
        FatalError("input-thread: could not create pipe");

    if (pipe(hotplugPipe) < 0)
        FatalError("input-thread: could not create pipe");

    /* Make pipes non-blocking and close-on-exec */
    PIPE_NONBLOCK(fds[0]);
    PIPE_MAKE_INHERIT(fds[0]);
    PIPE_NONBLOCK(fds[1]);
    PIPE_MAKE_INHERIT(fds[1]);
    PIPE_NONBLOCK(hotplugPipe[0]);
    PIPE_MAKE_INHERIT(hotplugPipe[0]);
    PIPE_NONBLOCK(hotplugPipe[1]);
    PIPE_MAKE_INHERIT(hotplugPipe[1]);
#endif

    inputThreadInfo = calloc(1, sizeof(InputThreadInfo));
    if (!inputThreadInfo)
        FatalError("input-thread: could not allocate memory");

    inputThreadInfo->changed = FALSE;

    inputThreadInfo->thread = 0;
    xorg_list_init(&inputThreadInfo->devs);
    inputThreadInfo->fds = ospoll_create();

    /* By making read head non-blocking, we ensure that while the main thread
     * is busy servicing client requests, the dedicated input thread can work
     * in parallel.
     */
    inputThreadInfo->readPipe = fds[0];
    SetNotifyFd(inputThreadInfo->readPipe, InputThreadNotifyPipe, X_NOTIFY_READ, NULL);

    inputThreadInfo->writePipe = fds[1];

    hotplugPipeRead = hotplugPipe[0];
    hotplugPipeWrite = hotplugPipe[1];

#ifndef __linux__ /* Linux does not deal well with renaming the main thread */
#if defined(HAVE_PTHREAD_SETNAME_NP_WITH_TID)
    pthread_setname_np (pthread_self(), "MainThread");
#elif defined(HAVE_PTHREAD_SETNAME_NP_WITHOUT_TID)
    pthread_setname_np ("MainThread");
#endif
#endif

}

/**
 * Start the threaded generation of input events. This routine complements what
 * was previously done by InputThreadPreInit(), being only responsible for
 * creating the dedicated input thread.
 *
 */
void
InputThreadInit(void)
{
    pthread_attr_t attr;

    /* If the driver hasn't asked for input thread support by calling
     * InputThreadPreInit, then do nothing here
     */
    if (!inputThreadInfo)
        return;

    pthread_attr_init(&attr);

    /* For OSes that differentiate between processes and threads, the following
     * lines have sense. Linux uses the 1:1 thread model. The scheduler handles
     * every thread as a normal process. Therefore this probably has no meaning
     * if we are under Linux.
     */
    if (pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM) != 0)
        ErrorF("input-thread: error setting thread scope\n");

    DebugF("input-thread: creating thread\n");
    pthread_create(&inputThreadInfo->thread, &attr,
                   &InputThreadDoWork, NULL);

    pthread_attr_destroy (&attr);
}

/**
 * Stop the threaded generation of input events
 *
 * This function is supposed to be called at server shutdown time only.
 */
void
InputThreadFinish(void)
{
    InputThreadDevice *dev, *next;

    if (!inputThreadInfo)
        return;

    /* Close the pipe to get the input thread to shut down */
    PIPE_CLOSE(hotplugPipeWrite);
    input_force_unlock();
    pthread_join(inputThreadInfo->thread, NULL);

    xorg_list_for_each_entry_safe(dev, next, &inputThreadInfo->devs, node) {
        ospoll_remove(inputThreadInfo->fds, dev->fd);
        free(dev);
    }
    xorg_list_init(&inputThreadInfo->devs);
    ospoll_destroy(inputThreadInfo->fds);

    RemoveNotifyFd(inputThreadInfo->readPipe);
    PIPE_CLOSE(inputThreadInfo->readPipe);
    PIPE_CLOSE(inputThreadInfo->writePipe);
    inputThreadInfo->readPipe = PIPE_INVALID;
    inputThreadInfo->writePipe = PIPE_INVALID;

    PIPE_CLOSE(hotplugPipeRead);
    hotplugPipeRead = PIPE_INVALID;
    hotplugPipeWrite = PIPE_INVALID;

    free(inputThreadInfo);
    inputThreadInfo = NULL;
}

int xthread_sigmask(int how, const sigset_t *set, sigset_t *oldset)
{
    return pthread_sigmask(how, set, oldset);
}

#else /* INPUTTHREAD */

Bool InputThreadEnable = FALSE;

void input_lock(void) {}
void input_unlock(void) {}
void input_force_unlock(void) {}

void InputThreadPreInit(void) {}
void InputThreadInit(void) {}
void InputThreadFinish(void) {}
int in_input_thread(void) { return 0; }

int InputThreadRegisterDev(int fd,
                           NotifyFdProcPtr readInputProc,
                           void *readInputArgs)
{
    return SetNotifyFd(fd, readInputProc, X_NOTIFY_READ, readInputArgs);
}

extern int InputThreadUnregisterDev(int fd)
{
    RemoveNotifyFd(fd);
    return 1;
}

int xthread_sigmask(int how, const sigset_t *set, sigset_t *oldset)
{
#ifdef HAVE_SIGPROCMASK
    return sigprocmask(how, set, oldset);
#else
    return 0;
#endif
}

#endif