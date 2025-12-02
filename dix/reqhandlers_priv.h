/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_REQHANDLERS_H
#define _XSERVER_DIX_REQHANDLERS_H

#include "include/dix.h"
#include "include/os.h"


/*
 * prototypes for various X11 request handlers
 *
 * those should only be called by the dispatcher
 */

/* events.c */
XRetCode ProcAllowEvents(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);
XRetCode ProcChangeActivePointerGrab(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);
XRetCode ProcGrabButton(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);
XRetCode ProcGetInputFocus(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);
XRetCode ProcGrabKey(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);
XRetCode ProcGrabKeyboard(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);
XRetCode ProcGrabPointer(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);
XRetCode ProcQueryPointer(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);
XRetCode ProcRecolorCursor(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);
XRetCode ProcSendEvent(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);
XRetCode ProcSetInputFocus(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);
XRetCode ProcUngrabButton(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);
XRetCode ProcUngrabKey(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);
XRetCode ProcUngrabKeyboard(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);
XRetCode ProcUngrabPointer(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);
XRetCode ProcWarpPointer(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);

XRetCode SProcChangeActivePointerGrab(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);
XRetCode SProcGrabButton(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);
XRetCode SProcGrabKey(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);
XRetCode SProcGrabKeyboard(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);
XRetCode SProcRecolorCursor(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);
XRetCode SProcSetInputFocus(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);
XRetCode SProcSendEvent(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);
XRetCode SProcUngrabButton(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);
XRetCode SProcUngrabKey(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);
XRetCode SProcUngrabKeyboard(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);
XRetCode SProcWarpPointer(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);

#endif /* _XSERVER_DIX_REQHANDLERS_H */
