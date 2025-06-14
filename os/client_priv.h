/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 * Copyright © 2010 Nokia Corporation and/or its subsidiary(-ies).
 */
#ifndef _XSERVER_DIX_CLIENT_PRIV_H
#define _XSERVER_DIX_CLIENT_PRIV_H

#include <sys/types.h>
#include <X11/Xdefs.h>
#include <X11/Xfuncproto.h>

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

/* Query cached client IDs. Exported on purpose for drivers. */
pid_t GetClientPid(struct _Client *client);
const char *GetClientCmdName(struct _Client *client);
const char *GetClientCmdArgs(struct _Client *client);

Bool ClientIsLocal(struct _Client *client);
XID AuthorizationIDOfClient(struct _Client *client);
const char *ClientAuthorized(struct _Client *client,
                             unsigned int proto_n,
                             char *auth_proto,
                             unsigned int string_n,
                             char *auth_string);
Bool AddClientOnOpenFD(int fd);
void ListenOnOpenFD(int fd, int noxauth);
int ReadRequestFromClient(struct _Client *client);
int WriteFdToClient(struct _Client *client, int fd, Bool do_close);
Bool InsertFakeRequest(struct _Client *client, char *data, int count);
void FlushAllOutput(void);
void FlushIfCriticalOutputPending(void);
void ResetOsBuffers(void);
void NotifyParentProcess(void);
void CreateWellKnownSockets(void);
void ResetWellKnownSockets(void);
void CloseWellKnownConnections(void);

// exported for nvidia driver
_X_EXPORT void SetCriticalOutputPending(void);

/* exported only for DRI module, but should not be used by external drivers */
_X_EXPORT void ResetCurrentRequest(struct _Client *client);

#endif /* _XSERVER_DIX_CLIENT_PRIV_H */
