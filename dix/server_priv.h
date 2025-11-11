/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_SERVER_PRIV_H
#define _XSERVER_DIX_SERVER_PRIV_H

#include "include/callback.h"
#include "include/dix.h"

typedef struct {
    ClientPtr client;
    Mask access_mode;
    int status;
} ServerAccessCallbackParam;

extern CallbackListPtr ServerAccessCallback;

static inline int dixCallServerAccessCallback(ClientPtr client, Mask access_mode)
{
    ServerAccessCallbackParam rec = { client, access_mode, Success };
    CallCallbacks(&ServerAccessCallback, &rec);
    return rec.status;
}

extern char *ConnectionInfo;

#endif /* _XSERVER_DIX_SERVER_PRIV_H */
