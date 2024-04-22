/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_MI_PRIV_H
#define _XSERVER_MI_PRIV_H

#include <X11/Xdefs.h>
#include <X11/Xprotostr.h>

#include "dix/screenint_priv.h"
#include "include/callback.h"
#include "include/events.h"
#include "include/gc.h"
#include "include/pixmap.h"
#include "include/regionstr.h"
#include "include/screenint.h"
#include "include/window.h"
#include "mi/mi.h"

void miScreenClose(ScreenPtr pScreen);

void miWideArc(DrawablePtr pDraw, GCPtr pGC, int narcs, xArc * parcs);
void miStepDash(int dist, int * pDashIndex, unsigned char * pDash,
                int numInDashList, int *pDashOffset);

Bool mieqInit(void);
void mieqFini(void);
void mieqEnqueue(DeviceIntPtr pDev, InternalEvent *e);
void mieqSwitchScreen(DeviceIntPtr pDev, ScreenPtr pScreen, Bool set_dequeue_screen);
void mieqProcessDeviceEvent(DeviceIntPtr dev, InternalEvent *event, ScreenPtr screen);
void mieqProcessInputEvents(void);
void mieqAddCallbackOnDrained(CallbackProcPtr callback, void *param);
void mieqRemoveCallbackOnDrained(CallbackProcPtr callback, void *param);

/**
 * Custom input event handler. If you need to process input events in some
 * other way than the default path, register an input event handler for the
 * given internal event type.
 */
typedef void (*mieqHandler) (int screen, InternalEvent *event,
                             DeviceIntPtr dev);
void mieqSetHandler(int event, mieqHandler handler);

void miSendExposures(WindowPtr pWin, RegionPtr pRgn, int dx, int dy);

#endif /* _XSERVER_MI_PRIV_H */
