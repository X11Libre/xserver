/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_CLIENT_PRIV_H
#define _XSERVER_DIX_CLIENT_PRIV_H

#include "include/callback.h"
#include "include/dix.h"

typedef struct {
    ClientPtr client;
    ClientPtr target;
    Mask access_mode;
    int status;
} ClientAccessCallbackParam;

extern CallbackListPtr ClientAccessCallback;

static inline int dixCallClientAccessCallback(ClientPtr client, ClientPtr target, Mask access_mode)
{
    ClientAccessCallbackParam rec = { client, target, access_mode, Success };
    CallCallbacks(&ClientAccessCallback, &rec);
    return rec.status;
}

#endif /* _XSERVER_DIX_CLIENT_PRIV_H */
