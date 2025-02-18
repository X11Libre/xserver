/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_PRIV_H
#define _XSERVER_DIX_PRIV_H

#include <X11/Xdefs.h>

/* This file holds global DIX settings to be used inside the Xserver,
 *  but NOT supposed to be accessed directly by external server modules like
 *  drivers or extension modules. Thus the definitions here are not part of the
 *  Xserver's module API/ABI.
 */

#include <X11/Xdefs.h>
#include <X11/Xfuncproto.h>
#include <X11/extensions/XI.h>

#include "include/callback.h"
#include "include/cursor.h"
#include "include/dix.h"
#include "include/events.h"
#include "include/gc.h"
#include "include/input.h"
#include "include/window.h"

/* server setting: maximum size for big requests */
#define MAX_BIG_REQUEST_SIZE 4194303
extern long maxBigRequestSize;

extern char dispatchExceptionAtReset;
extern int terminateDelay;
extern Bool touchEmulatePointer;

extern HWEventQueuePtr checkForInput[2];

 /* -retro mode */
extern Bool party_like_its_1989;

static inline _X_NOTSAN Bool
InputCheckPending(void)
{
    return (*checkForInput[0] != *checkForInput[1]);
}

void ClearWorkQueue(void);
void ProcessWorkQueue(void);
void ProcessWorkQueueZombies(void);

void CloseDownClient(ClientPtr client);
ClientPtr GetCurrentClient(void);
void InitClient(ClientPtr client, int i, void *ospriv);

/* lookup builtin color by name */
Bool dixLookupBuiltinColor(int screen,
                           char *name,
                           unsigned len,
                           unsigned short *pred,
                           unsigned short *pgreen,
                           unsigned short *pblue);

void DeleteWindowFromAnySaveSet(WindowPtr pWin);

#define VALIDATE_DRAWABLE_AND_GC(drawID, pDraw, mode)                   \
    do {                                                                \
        int tmprc = dixLookupDrawable(&(pDraw), drawID, client, M_ANY, mode); \
        if (tmprc != Success)                                           \
            return tmprc;                                               \
        tmprc = dixLookupGC(&(pGC), stuff->gc, client, DixUseAccess);   \
        if (tmprc != Success)                                           \
            return tmprc;                                               \
        if ((pGC->depth != pDraw->depth) || (pGC->pScreen != pDraw->pScreen)) \
            return BadMatch;                                            \
        if (pGC->serialNumber != pDraw->serialNumber)                   \
            ValidateGC(pDraw, pGC);                                     \
    } while (0)

int dixLookupGC(GCPtr *result,
                XID id,
                ClientPtr client,
                Mask access_mode);

int dixLookupClient(ClientPtr *result,
                    XID id,
                    ClientPtr client,
                    Mask access_mode);

Bool CreateConnectionBlock(void);

void EnableLimitedSchedulingLatency(void);

void DisableLimitedSchedulingLatency(void);

int dix_main(int argc, char *argv[], char *envp[]);

void SetMaskForEvent(int deviceid, Mask mask, int event);

void EnqueueEvent(InternalEvent *ev, DeviceIntPtr device);

void PlayReleasedEvents(void);

void ActivatePointerGrab(DeviceIntPtr mouse,
                         GrabPtr grab,
                         TimeStamp time,
                         Bool autoGrab);

void DeactivatePointerGrab(DeviceIntPtr mouse);

void ActivateKeyboardGrab(DeviceIntPtr keybd,
                          GrabPtr grab,
                          TimeStamp time,
                          Bool passive);

void DeactivateKeyboardGrab(DeviceIntPtr keybd);

BOOL ActivateFocusInGrab(DeviceIntPtr dev, WindowPtr old, WindowPtr win);

void AllowSome(ClientPtr client,
               TimeStamp time,
               DeviceIntPtr thisDev,
               int newState);

void ReleaseActiveGrabs(ClientPtr client);

GrabPtr CheckPassiveGrabsOnWindow(WindowPtr pWin,
                                  DeviceIntPtr device,
                                  InternalEvent *event,
                                  BOOL checkCore,
                                  BOOL activate);

int DeliverDeviceEvents(WindowPtr pWin,
                        InternalEvent *event,
                        GrabPtr grab,
                        WindowPtr stopAt,
                        DeviceIntPtr dev);

int DeliverOneGrabbedEvent(InternalEvent *event,
                           DeviceIntPtr dev,
                           enum InputLevel level);

void DeliverTouchEvents(DeviceIntPtr dev,
                        TouchPointInfoPtr ti,
                        InternalEvent *ev,
                        XID resource);

Bool DeliverGestureEventToOwner(DeviceIntPtr dev,
                                GestureInfoPtr gi,
                                InternalEvent *ev);

void InitializeSprite(DeviceIntPtr pDev, WindowPtr pWin);
void FreeSprite(DeviceIntPtr pDev);
void UpdateSpriteForScreen(DeviceIntPtr pDev, ScreenPtr pScreen);

Bool CheckDeviceGrabs(DeviceIntPtr device,
                      InternalEvent *event,
                      WindowPtr ancestor);

void DeliverFocusedEvent(DeviceIntPtr keybd,
                         InternalEvent *event,
                         WindowPtr window);

int DeliverGrabbedEvent(InternalEvent *event,
                        DeviceIntPtr thisDev,
                        Bool deactivateGrab);

void FreezeThisEventIfNeededForSyncGrab(DeviceIntPtr thisDev,
                                        InternalEvent *event);

void FixKeyState(DeviceEvent *event, DeviceIntPtr keybd);

void RecalculateDeliverableEvents(WindowPtr pWin);

void DoFocusEvents(DeviceIntPtr dev,
                   WindowPtr fromWin,
                   WindowPtr toWin,
                   int mode);

int SetInputFocus(ClientPtr client,
                  DeviceIntPtr dev,
                  Window focusID,
                  CARD8 revertTo,
                  Time ctime,
                  Bool followOK);

int GrabDevice(ClientPtr client,
               DeviceIntPtr dev,
               unsigned this_mode,
               unsigned other_mode,
               Window grabWindow,
               unsigned ownerEvents,
               Time ctime,
               GrabMask *mask,
               int grabtype,
               Cursor curs,
               Window confineToWin,
               CARD8 *status);

void InitEvents(void);

void CloseDownEvents(void);

void DeleteWindowFromAnyEvents(WindowPtr pWin, Bool freeResources);

Mask EventMaskForClient(WindowPtr pWin, ClientPtr client);

Bool CheckMotion(DeviceEvent *ev, DeviceIntPtr pDev);

int SetClientPointer(ClientPtr client, DeviceIntPtr device);

Bool IsInterferingGrab(ClientPtr client, DeviceIntPtr dev, xEvent *events);

int XItoCoreType(int xi_type);

Bool DevHasCursor(DeviceIntPtr pDev);

Bool IsPointerEvent(InternalEvent *event);

Bool IsTouchEvent(InternalEvent *event);

Bool IsGestureEvent(InternalEvent *event);

Bool IsGestureBeginEvent(InternalEvent *event);

Bool IsGestureEndEvent(InternalEvent *event);

void CopyKeyClass(DeviceIntPtr device, DeviceIntPtr master);

int CorePointerProc(DeviceIntPtr dev, int what);

int CoreKeyboardProc(DeviceIntPtr dev, int what);

extern Bool whiteRoot;

extern volatile char isItTimeToYield;

/* bit values for dispatchException */
#define DE_RESET     1
#define DE_TERMINATE 2
#define DE_PRIORITYCHANGE 4     /* set when a client's priority changes */

extern volatile char dispatchException;

extern int ScreenSaverBlanking;
extern int ScreenSaverAllowExposures;
extern int defaultScreenSaverBlanking;
extern int defaultScreenSaverAllowExposures;
extern const char *display;
extern int displayfd;
extern Bool explicit_display;

extern Bool disableBackingStore;
extern Bool enableBackingStore;

/* in generated BuiltInAtoms.c */
void MakePredeclaredAtoms(void);

#endif /* _XSERVER_DIX_PRIV_H */
