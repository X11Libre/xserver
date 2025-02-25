/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_MI_PRIV_H
#define _XSERVER_MI_PRIV_H

#include <X11/Xdefs.h>
#include <X11/Xproto.h>
#include <X11/Xprotostr.h>

#include "dix/screenint_priv.h"
#include "include/callback.h"
#include "include/events.h"
#include "include/gc.h"
#include "include/pixmap.h"
#include "include/regionstr.h"
#include "include/screenint.h"
#include "include/scrnintstr.h"
#include "include/validate.h"
#include "include/window.h"
#include "mi/mi.h"
#include "mi/micmap.h"

static inline void SetInstalledmiColormap(ScreenPtr s, ColormapPtr c) {
    dixSetPrivate(&(s)->devPrivates, micmapScrPrivateKey, c);
}

static inline ColormapPtr GetInstalledmiColormap(ScreenPtr s) {
    return (ColormapPtr)dixLookupPrivate(&(s)->devPrivates, &micmapScrPrivateKeyRec);
}

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
void miWindowExposures(WindowPtr pWin, RegionPtr prgn);

void miPaintWindow(WindowPtr pWin, RegionPtr prgn, int what);
void miSourceValidate(DrawablePtr pDrawable, int x, int y, int w, int h,
                      unsigned int subWindowMode);

/* only exported for modesetting, not for external drivers (yet) */
_X_EXPORT Bool miCreateScreenResources(ScreenPtr pScreen);

int miShapedWindowIn(RegionPtr universe, RegionPtr bounding, BoxPtr rect,
                     int x, int y);
int miValidateTree(WindowPtr pParent, WindowPtr pChild, VTKind kind);

void miClearToBackground(WindowPtr pWin, int x, int y, int w, int h,
                         Bool generateExposures);
void miMarkWindow(WindowPtr pWin);
Bool miMarkOverlappedWindows(WindowPtr pWin, WindowPtr pFirst,
                             WindowPtr *ppLayerWin);
void miHandleValidateExposures(WindowPtr pWin);
void miMoveWindow(WindowPtr pWin, int x, int y, WindowPtr pNextSib, VTKind kind);
void miResizeWindow(WindowPtr pWin, int x, int y, unsigned int w,
                    unsigned int h, WindowPtr pSib);
WindowPtr miGetLayerWindow(WindowPtr pWin);
void miSetShape(WindowPtr pWin, int kind);
void miChangeBorderWidth(WindowPtr pWin, unsigned int width);
void miMarkUnrealizedWindow(WindowPtr pChild, WindowPtr pWin, Bool fromConfigure);
WindowPtr miSpriteTrace(SpritePtr pSprite, int x, int y);
WindowPtr miXYToWindow(ScreenPtr pScreen, SpritePtr pSprite, int x, int y);

int miExpandDirectColors(ColormapPtr, int, xColorItem *, xColorItem *);

#endif /* _XSERVER_MI_PRIV_H */
