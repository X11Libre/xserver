/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_CLIENT_PRIV_H
#define _XSERVER_DIX_CLIENT_PRIV_H

#include "include/callback.h"

/*
 * called right before ClientRec is about to be destroyed,
 * after resources have been freed. argument is ClientPtr
 */
extern CallbackListPtr ClientDestroyCallback;

#endif /* _XSERVER_DIX_CLIENT_PRIV_H */
