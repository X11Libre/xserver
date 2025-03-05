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
#include "include/os.h"
#include "include/resource.h"
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

int dixLookupResourceOwner(ClientPtr *result,
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

/*
 * @brief mark event ID as critical
 * @param event the event to add to the critical events bitmap
 */
void SetCriticalEvent(int event);

/**
 * @brief try to deliver (single) event to interested parties.
 *
 * @param pWindow       target window
 * @param pEvent        event to be delivered
 * @param filter        filter mask based on event type
 * @param skipClient    don't deliver to this client (if not NULL)
 * @return TRUE when event was delivered
 */
Bool MaybeDeliverEventToClient(WindowPtr pWindow,
                               xEvent *pEvent,
                               Mask filter,
                               ClientPtr skipClient)
    _X_ATTRIBUTE_NONNULL_ARG(1,2);

/*
 * @brief select window events to listen on
 *
 * @param pWindow   window to listen on
 * @param pClient   the client that's listening on the events
 * @param mask      mask of events to listen on
 * @return X error code
 */
XRetCode EventSelectForWindow(WindowPtr pWindow, ClientPtr pClient, Mask mask)
    _X_ATTRIBUTE_NONNULL_ARG(1,2);

/*
 * @brief set block propagation of specific events on window
 *
 * @param pWindow       window to act on
 * @param pClient       client to act on
 * @param mask          mask of events to not propagate
 * @param checkOptional set to w/ TRUE when window's optional structure changed
 * @return X error code
 */
int EventSuppressForWindow(WindowPtr pWindow,
                           ClientPtr pClient,
                           Mask mask,
                           Bool *checkOptional)
    _X_ATTRIBUTE_NONNULL_ARG(1,2,4);

/*
 * @brief allocate new ClientRec and initialize it
 *
 * Returns NULL on allocation failure or when client limit reached.
 *
 * @param ospriv pointer to OS layer's internal data
 * @return pointer to new ClientRec or NULL on failure
 */
ClientPtr NextAvailableClient(void *ospriv);

/*
 * @brief mark exception on client - will be closed down later
 *
 * @param pClient pointer to client that has exception
 */
void MarkClientException(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);

typedef Bool (*ClientSleepProcPtr) (ClientPtr client, void *closure);

/*
 * @brief put a client to sleep
 *
 * @param pClient   the client to put into sleep
 * @param func  called when client wakes up
 * @param closure   data passed to the callback function
 */
Bool ClientSleep(ClientPtr pClient, ClientSleepProcPtr func, void *closure)
    _X_ATTRIBUTE_NONNULL_ARG(1,2);

/*
 * @brief signal to sleeping client there's work to do
 *
 * @param pClient   the client to signal to
 * @return TRUE on success
 */
Bool dixClientSignal(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);

#define CLIENT_SIGNAL_ANY ((void *)-1)
/*
 * @brief signal to all sleeping clients matching client, func, closure
 *
 * If any of the client, func and closure parameters may be CLIENT_SIGNAL_ANY,
 * so those will be matching any value
 *
 * @param pClient   match for client
 * @param func      match for callback function
 * @param closure   match for callback closure
 * @return number of matched / queued clients
 */
int ClientSignalAll(ClientPtr pClient, ClientSleepProcPtr func, void *closure)
    _X_ATTRIBUTE_NONNULL_ARG(1,2);

/*
 * @brief wake up a client and restart request processing of this client
 *
 * @param pClient pointer to client structure
 */
void ClientWakeup(ClientPtr pclient)
    _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * @brief check whether client is asleep
 *
 * @param pClient pointer to client structure
 * @return TRUE if client is sleeping and has no work to do
 */
Bool ClientIsAsleep(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * @brief send error packet (xError) to client
 *
 * @param pClient     pointer to client structure
 * @param majorCode   major opcode of failed request
 * @param minorCode   minor opcode of failed request
 * @param resId       ID of resource the failure occured on
 * @param errorCode   error code value
 */
void SendErrorToClient(ClientPtr pClient,
                       CARD8 majorCode,
                       CARD16 minorCode,
                       XID resId,
                       BYTE errorCode)
    _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * @brief send GraphicsExposure events for given region to client

 * Sends a series of GraphicsExposure events spanning the given region to the
 * client. If region is empty or NULL, sending NoExpose event instead.
 *
 + @param pClient     the client to send events to
 * @param pRegion     the region the events are based on
 * @param drawable    ID of the drawable the events are related to
 * @param majorOpcode major opcode of request which caused the expose event
 * @param minorOpcode minor opcode of request which caused the expose event
 */
void SendGraphicsExpose(ClientPtr pClient, RegionPtr pRegion, XID drawable,
                        BYTE majorOpcode, CARD16 minorOpcode)
    _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * @brief alter the SaveSet of a window (see automatic reparenting when client gone)
 *
 * @param pClient pointer to client structure
 * @param pWindow pointer to window structure whose SaveSet is changed
 * @param mode    what to do when client goes away
 * @param toRoot  TRUE to reparent to root when parent's client gone
 * @param map     TRUE if window shall remain mapped
 * @return X return code
 */
XRetCode AlterSaveSetForClient(ClientPtr pClient,
                               WindowPtr pWin,
                               unsigned mode,
                               Bool toRoot,
                               Bool map);

void dixFreeScreen(ScreenPtr pScreen);

/*
 * @brief call screen's window destructors
 * @see dixScreenHookWindowDestroy
 * @param pWin the window thats being destroyed
 * @result the ScreenRec's DestroyWindow() return value
 *
 * Call the pluggable window destructors that extensions might have registered on
 * the screen, and finally call ScreenRec's DestroyWindow proc.
 *
 * Should only be called by DIX itself.
 */
int dixScreenRaiseWindowDestroy(WindowPtr pWin);

/*
 * @brief call screen's window position notification hooks
 * @see dixScreenHookWindowPosition
 * @param pWin the window to notify on
 *
 * Call the pluggable window position hooks that extensions might have registered on
 * the screen, and finally call ScreenRec's PositionWindow proc.
 *
 * Should only be called by DIX itself.
 */
void dixScreenRaiseWindowPosition(WindowPtr pWin, uint32_t x, uint32_t y);

/*
 * @brief call screen's close hooks
 * @see dixScreenHookClose
 * @param pScreen the screen being closed
 *
 * Call the pluggable screen close hooks that extensions might have registered on
 * the screen, and finally call ScreenRec's CloseScreen proc.
 *
 * Should only be called by DIX itself.
 */
void dixScreenRaiseClose(ScreenPtr pScreen);

/*
 * @brief call screen's PixmapDestroy hook
 * @see dixScreenHookPixmapDestroy
 * @param pPixmap the pixmap being destroyed
 *
 * Call the pluggable pixmap destroy pixmap hooks that extensions might have
 * registered on the screen.
 * Note that it's *only* called, when the pixmap is really being destroyed
 * (instead of just unref'ed)
 *
 * Should only be called by DIX itself, by dixDestroyPixmap()
 * It must be called *before* the ScreenRec->DestroyPixmap() is called, any
 * only if the reference counter reaches 1.
 */
void dixScreenRaisePixmapDestroy(PixmapPtr pPixmap);

/*
 * @brief call screen's CreateScreenResources chain
 * @see dixScreenHookPostCreateResources
 * @param pScreen the screen to run on
 *
 * Call the screen's CreateScreenResources() proc, and if it returns TRUE,
 * run the installed hooks afterwards.
 *
 * Should only be called by DIX itself.
 */
Bool dixScreenRaiseCreateResources(ScreenPtr pScreen);

#endif /* _XSERVER_DIX_PRIV_H */
