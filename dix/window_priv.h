/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2025 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_WINDOW_H
#define _XSERVER_DIX_WINDOW_H

#include <X11/X.h>

#include "include/dix.h"
#include "include/window.h"

/*
 * @brief create a window
 *
 * Creates a window with given XID, geometry, etc
 *
 * @return pointer to new Window or NULL on error (see error pointer)
 */
WindowPtr dixCreateWindow(Window wid,
                          WindowPtr pParent,
                          int x,
                          int y,
                          unsigned int w,
                          unsigned int h,
                          unsigned int bw,
                          unsigned int windowclass,
                          Mask vmask,
                          XID * vlist,
                          int depth,
                          ClientPtr client,
                          VisualID visual,
                          int * error);

#endif /* _XSERVER_DIX_WINDOW_H */
