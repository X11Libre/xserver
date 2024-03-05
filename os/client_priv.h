/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 * Copyright © 2010 Nokia Corporation and/or its subsidiary(-ies).
 */
#ifndef _XSERVER_DIX_CLIENT_PRIV_H
#define _XSERVER_DIX_CLIENT_PRIV_H

#include <sys/types.h>

#include "client.h"

/* Client IDs. Use GetClientPid, GetClientCmdName and GetClientCmdArgs
 * instead of accessing the fields directly. */
struct _ClientId {
    pid_t pid;                  /* process ID, -1 if not available */
    const char *cmdname;        /* process name, NULL if not available */
    const char *cmdargs;        /* process arguments, NULL if not available */
};

struct _Client;

/* Initialize and clean up. */
void ReserveClientIds(struct _Client *client);
void ReleaseClientIds(struct _Client *client);

/* Determine client IDs for caching. Exported on purpose for
 * extensions such as SELinux. */
pid_t DetermineClientPid(struct _Client *client);
void DetermineClientCmd(pid_t, const char **cmdname, const char **cmdargs);

#endif /* _XSERVER_DIX_CLIENT_PRIV_H */
