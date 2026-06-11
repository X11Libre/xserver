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

/**
 * @file
 *
 * This file contains functionality for identifying clients by various
 * means. The primary purpose of identification is to simply aid in
 * finding out which clients are using X server and how they are using
 * it. For example, it's often necessary to monitor what requests
 * clients are executing (to spot bad behaviour) and how they are
 * allocating resources in X server (to spot excessive resource
 * usage).
 *
 * This framework automatically allocates information, that can be
 * used for client identification, when a client connects to the
 * server. The information is freed when the client disconnects. The
 * allocated information is just a collection of various IDs, such as
 * PID and process name for local clients, that are likely to be
 * useful in analyzing X server usage.
 *
 * Users of the framework can query ID information about clients at
 * any time. To avoid repeated polling of IDs the users can also
 * subscribe for notifications about the availability of ID
 * information. IDs have been allocated before ClientStateCallback is
 * called with ClientStateInitial state. Similarly the IDs will be
 * released after ClientStateCallback is called with ClientStateGone
 * state.
 *
 * Author: Rami Ylimäki <rami.ylimaki@vincit.fi>
 */
#include <dix-config.h>

#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "os/client_priv.h"

#include "os.h"
#include "dixstruct.h"

#ifdef __sun
#include "solaris/client_solaris.h"
#endif

#ifdef __OpenBSD__
#include "openbsd/client_openbsd.h"
#endif

#ifdef __APPLE__
#include "apple/client_apple.h"
#endif /* __APPLE__ */

#if defined(__DragonFly__) || defined(__FreeBSD__)
#include "freebsd/client_freebsd.h"
#endif

#include "os/auth.h"
#include "os/log_priv.h"

/**
 * Try to determine a PID for a client from its connection
 * information. This should be called only once when new client has
 * connected, use GetClientPid to determine the PID at other times.
 *
 * @param[in] client Connection linked to some process.
 *
 * @return PID of the client. Error (-1) if PID can't be determined
 *         for the client.
 *
 * @see GetClientPid
 */
pid_t
DetermineClientPid(struct _Client * client)
{
    LocalClientCredRec *lcc = NULL;
    pid_t pid = -1;

    if (client == NULL)
        return pid;

    if (client == serverClient)
        return getpid();

    if (GetLocalClientCreds(client, &lcc) != -1) {
        if (lcc->fieldsSet & LCC_PID_SET)
            pid = lcc->pid;
        FreeLocalClientCreds(lcc);
    }

    return pid;
}



/**
 * Try to determine a command line string for a client based on its
 * PID. Note that mapping PID to a command hasn't been implemented for
 * some operating systems. This should be called only once when a new
 * client has connected, use GetClientCmdName/Args to determine the
 * string at other times.
 *
 * @param[in]  pid     Process ID of a client.

 * @param[out] cmdname Client process name without arguments. You must
 *                     release this by calling free. On error NULL is
 *                     returned. Pass NULL if you aren't interested in
 *                     this value.
 * @param[out] cmdargs Arguments to client process. Useful for
 *                     identifying a client that is executed from a
 *                     launcher program. You must release this by
 *                     calling free. On error NULL is returned. Pass
 *                     NULL if you aren't interested in this value.
 *
 * @see GetClientCmdName/Args
 */
void
DetermineClientCmd(pid_t pid, const char **cmdname, const char **cmdargs)
{
    if (cmdname)
        *cmdname = NULL;
    if (cmdargs)
        *cmdargs = NULL;

    if (pid == -1)
        return;
#if defined(__APPLE__) /* macOS/OSX and any other operating system by Apple where an Xserver can run */
    DetermineClientCmdApple(pid, cmdname, cmdargs);
/* In this case, the FreeBSD code also works for DragonflyBSD, there's no reason to have two versions of the same function when
 * we can reuse the same one. */
#elif defined(__FreeBSD__) || defined(__DragonFly__)
    DetermineClientCmdFreeBSD(pid, cmdname, cmdargs);
#elif defined(__OpenBSD__)
    DetermineClientCmdOpenBSD(pid, cmdname, cmdargs);
#elif defined(__sun) /* Solaris */
    DetermineClientCmdSolaris(pid, cmdname, cmdargs);
#else /* Possibly Linux or another operating system where the /proc filesystem
       * is available. Use it as a fallback. */
    char path[PATH_MAX + 1];
    /* Check if /proc/pid/cmdline exists. It's not supported on all
     * operating systems. */
    if (snprintf(path, sizeof(path), "/proc/%d/cmdline", pid) < 0)
        return;
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return;

    /* Read the contents of /proc/pid/cmdline. It should contain the
     * process name and arguments. */
    ssize_t totsize = read(fd, path, sizeof(path));
    close(fd);
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
#endif
}

/**
 * Called when a new client connects. Allocates client ID information.
 *
 * @param[in] client Recently connected client.
 */
void
ReserveClientIds(struct _Client *client)
{
    if (client == NULL)
        return;

    assert(!client->clientIds);
    client->clientIds = calloc(1, sizeof(struct _ClientId));
    if (!client->clientIds)
        return;

    client->clientIds->pid = DetermineClientPid(client);
    if (client->clientIds->pid != -1)
        DetermineClientCmd(client->clientIds->pid, &client->clientIds->cmdname,
                           &client->clientIds->cmdargs);

    DebugF("client(%lx): Reserved pid(%d).\n",
           (unsigned long) client->clientAsMask, client->clientIds->pid);
    DebugF("client(%lx): Reserved cmdname(%s) and cmdargs(%s).\n",
           (unsigned long) client->clientAsMask,
           client->clientIds->cmdname ? client->clientIds->cmdname : "NULL",
           client->clientIds->cmdargs ? client->clientIds->cmdargs : "NULL");
}

/**
 * Called when an existing client disconnects. Frees client ID
 * information.
 *
 * @param[in] client Recently disconnected client.
 */
void
ReleaseClientIds(struct _Client *client)
{
    if (client == NULL)
        return;

    if (!client->clientIds)
        return;

    DebugF("client(%lx): Released pid(%d).\n",
           (unsigned long) client->clientAsMask, client->clientIds->pid);
    DebugF("client(%lx): Released cmdline(%s) and cmdargs(%s).\n",
           (unsigned long) client->clientAsMask,
           client->clientIds->cmdname ? client->clientIds->cmdname : "NULL",
           client->clientIds->cmdargs ? client->clientIds->cmdargs : "NULL");

    free((void *) client->clientIds->cmdname);  /* const char * */
    free((void *) client->clientIds->cmdargs);  /* const char * */
    free(client->clientIds);
    client->clientIds = NULL;
}

/**
 * Get cached PID of a client.
 *
 * param[in] client Client whose PID has been already cached.
 *
 * @return Cached client PID. Error (-1) if called:
 *         - before ClientStateInitial client state notification
 *         - after ClientStateGone client state notification
 *         - for remote clients
 *
 * @see DetermineClientPid
 */
pid_t
GetClientPid(struct _Client *client)
{
    if (client == NULL)
        return -1;

    if (!client->clientIds)
        return -1;

    return client->clientIds->pid;
}

/**
 * Get cached command name string of a client.
 *
 * param[in] client Client whose command line string has been already
 *                  cached.
 *
 * @return Cached client command name. Error (NULL) if called:
 *         - before ClientStateInitial client state notification
 *         - after ClientStateGone client state notification
 *         - for remote clients
 *         - on OS that doesn't support mapping of PID to command line
 *
 * @see DetermineClientCmd
 */
const char *
GetClientCmdName(struct _Client *client)
{
    if (client == NULL)
        return NULL;

    if (!client->clientIds)
        return NULL;

    return client->clientIds->cmdname;
}

/**
 * Get cached command arguments string of a client.
 *
 * param[in] client Client whose command line string has been already
 *                  cached.
 *
 * @return Cached client command arguments. Error (NULL) if called:
 *         - before ClientStateInitial client state notification
 *         - after ClientStateGone client state notification
 *         - for remote clients
 *         - on OS that doesn't support mapping of PID to command line
 *
 * @see DetermineClientCmd
 */
const char *
GetClientCmdArgs(struct _Client *client)
{
    if (client == NULL)
        return NULL;

    if (!client->clientIds)
        return NULL;

    return client->clientIds->cmdargs;
}
