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
#include "include/screenint.h"
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

#endif /* _XSERVER_MI_PRIV_H */
