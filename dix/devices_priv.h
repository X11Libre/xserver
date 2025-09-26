/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_DEVICES_PRIV_H
#define _XSERVER_DIX_DEVICES_PRIV_H

#include "include/callback.h"
#include "include/dix.h"

/*
 * called when a client tries to access devices
 */
extern CallbackListPtr DeviceAccessCallback;

typedef struct {
    ClientPtr client;
    DeviceIntPtr dev;
    Mask access_mode;
    int status;
} DeviceAccessCallbackParam;

static inline int dixCallDeviceAccessCallback(ClientPtr client, DeviceIntPtr dev, Mask access_mode)
{
    DeviceAccessCallbackParam rec = { client, dev, access_mode, Success };
    CallCallbacks(&DeviceAccessCallback, &rec);
    return rec.status;
}

#endif /* _XSERVER_DIX_DEVICES_PRIV_H */
