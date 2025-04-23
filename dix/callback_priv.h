/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_CALLBACK_PRIV_H
#define _XSERVER_CALLBACK_PRIV_H

#include "callback.h"

void InitCallbackManager(void);
void DeleteCallbackManager(void);

/*
 * @brief delete a callback list
 *
 * Calling this is necessary if a CallbackListPtr is used inside a dynamically
 * allocated structure, before it is freed. If it's not done, memory corruption
 * or segfault can happen at a much later point (eg. next server incarnation)
 *
 * @param pcbl pointer to the list head (CallbackListPtr)
 */
void DeleteCallbackList(CallbackListPtr *pcbl);

#endif /* _XSERVER_CALLBACK_PRIV_H */
