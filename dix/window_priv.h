/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2025 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_WINDOW_PRIV_H
#define _XSERVER_DIX_WINDOW_PRIV_H

#include <X11/X.h>

#include "include/dix.h"
#include "include/window.h"
#include "include/windowstr.h"

#define wTrackParent(w,field)   ((w)->optional ? \
                                    (w)->optional->field \
                                 : FindWindowWithOptional(w)->optional->field)
#define wUseDefault(w,field,def)        ((w)->optional ? \
                                    (w)->optional->field \
                                 : def)

#define wVisual(w)              wTrackParent(w, visual)
#define wCursor(w)              ((w)->cursorIsNone ? None : wTrackParent(w, cursor))
#define wColormap(w)            ((w)->drawable.class == InputOnly ? None : wTrackParent(w, colormap))
#define wDontPropagateMask(w)   wUseDefault(w, dontPropagateMask, DontPropagateMasks[(w)->dontPropagate])
#define wOtherEventMasks(w)     wUseDefault(w, otherEventMasks, 0)
#define wOtherClients(w)        wUseDefault(w, otherClients, NULL)
#define wOtherInputMasks(w)     wUseDefault(w, inputMasks, NULL)
#define wPassiveGrabs(w)        wUseDefault(w, passiveGrabs, NULL)
#define wBackingBitPlanes(w)    wUseDefault(w, backingBitPlanes, ~0L)
#define wBackingPixel(w)        wUseDefault(w, backingPixel, 0)
#define wBoundingShape(w)       wUseDefault(w, boundingShape, NULL)
#define wClipShape(w)           wUseDefault(w, clipShape, NULL)
#define wInputShape(w)          wUseDefault(w, inputShape, NULL)

#define SameBackground(as, a, bs, b)				\
    ((as) == (bs) && ((as) == None ||				\
                      (as) == ParentRelative ||			\
                      SamePixUnion(a,b,as == BackgroundPixel)))

#define SameBorder(as, a, bs, b) EqualPixUnion(as, a, bs, b)

extern Mask DontPropagateMasks[];

#define wDontPropagateMask(w)   wUseDefault(w, dontPropagateMask, DontPropagateMasks[(w)->dontPropagate])

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
/*
 * @brief Make sure the window->optional structure exists.
 *
 * allocate if window->optional == NULL, otherwise do nothing.
 *
 * @param pWin the window to operate on
 * @return FALSE if allocation failed, otherwise TRUE
 */
Bool MakeWindowOptional(WindowPtr pWin);

/*
 * @brief check whether a window (ID) is a screen root window
 *
 * The underlying resource query is explicitly done on behalf of serverClient,
 * so XACE resource hooks don't recognize this as a client action.
 * It's explicitly designed for use in hooks that don't wanna cause unncessary
 * traffic in other XACE resource hooks: things done by the serverClient usually
 * considered safe enough for not needing any additional security checks.
 * (we don't have any way for completely skipping the XACE hook yet)
 */
Bool dixWindowIsRoot(Window window);

/*
 * @brief lower part of X_CreateWindow request handler.
 * Called by ProcCreateWindow() as well as PanoramiXCreateWindow()
 */
int DoCreateWindowReq(ClientPtr client, xCreateWindowReq *stuff, XID *xids);

void PrintPassiveGrabs(void);
void PrintWindowTree(void);

#endif /* _XSERVER_DIX_WINDOW_PRIV_H */
