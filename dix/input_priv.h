/* SPDX-License-Identifier: MIT OR X11
 *
 + Copyright © 1987, 1998  The Open Group
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
/************************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

********************************************************/
#ifndef _XSERVER_INPUT_PRIV_H
#define _XSERVER_INPUT_PRIV_H

#include "include/cursor.h"
#include "include/input.h"
#include "include/inputstr.h"

typedef struct _DDXTouchPointInfo {
    uint32_t client_id;         /* touch ID as seen in client events */
    Bool active;                /* whether or not the touch is active */
    uint32_t ddx_id;            /* touch ID given by the DDX */
    Bool emulate_pointer;

    ValuatorMask *valuators;    /* last axis values as posted, pre-transform */
} DDXTouchPointInfoRec;

void InitCoreDevices(void);
void InitXTestDevices(void);

void DisableAllDevices(void);
int InitAndStartDevices(void);

void CloseDownDevices(void);
void AbortDevices(void);

void UndisplayDevices(void);

ValuatorClassPtr AllocValuatorClass(ValuatorClassPtr src, int numAxes);
void FreeDeviceClass(int type, void **class);

int ApplyPointerMapping(DeviceIntPtr pDev,
                        CARD8 *map,
                        int len,
                        ClientPtr client);

Bool BadDeviceMap(BYTE *buff,
                  int length,
                  unsigned low,
                  unsigned high,
                  XID *errval);

void NoteLedState(DeviceIntPtr keybd, int led, Bool on);

void MaybeStopHint(DeviceIntPtr device, ClientPtr client );

void ProcessPointerEvent(InternalEvent *ev, DeviceIntPtr mouse);

void ProcessKeyboardEvent(InternalEvent *ev, DeviceIntPtr keybd);

void CreateClassesChangedEvent(InternalEvent *event,
                               DeviceIntPtr master,
                               DeviceIntPtr slave,
                               int flags);

InternalEvent *UpdateFromMaster(InternalEvent *events,
                                DeviceIntPtr pDev,
                                int type,
                                int *num_events);

void PostSyntheticMotion(DeviceIntPtr pDev,
                         int x,
                         int y,
                         int screen,
                         unsigned long time);

void ReleaseButtonsAndKeys(DeviceIntPtr dev);

int AttachDevice(ClientPtr client, DeviceIntPtr slave, DeviceIntPtr master);

void DeepCopyDeviceClasses(DeviceIntPtr from,
                           DeviceIntPtr to,
                           DeviceChangedEvent *dce);

int change_modmap(ClientPtr client,
                  DeviceIntPtr dev,
                  KeyCode *map,
                  int max_keys_per_mod);

int AllocXTestDevice(ClientPtr client,
                     const char *name,
                     DeviceIntPtr *ptr,
                     DeviceIntPtr *keybd,
                     DeviceIntPtr master_ptr,
                     DeviceIntPtr master_keybd);
BOOL IsXTestDevice(DeviceIntPtr dev, DeviceIntPtr master);
DeviceIntPtr GetXTestDevice(DeviceIntPtr master);

void SendDevicePresenceEvent(int deviceid, int type);
void DeliverDeviceClassesChangedEvent(int sourceid, Time time);

/* touch support */
int GetTouchEvents(InternalEvent *events,
                   DeviceIntPtr pDev,
                   uint32_t ddx_touchid,
                   uint16_t type,
                   uint32_t flags,
                   const ValuatorMask *mask);
void QueueTouchEvents(DeviceIntPtr device,
                      int type,
                      uint32_t ddx_touchid,
                      int flags, const ValuatorMask *mask);
int GetTouchOwnershipEvents(InternalEvent *events,
                            DeviceIntPtr pDev,
                            TouchPointInfoPtr ti,
                            uint8_t mode,
                            XID resource,
                            uint32_t flags);
void GetDixTouchEnd(InternalEvent *ievent,
                    DeviceIntPtr dev,
                    TouchPointInfoPtr ti,
                    uint32_t flags);
void TouchInitDDXTouchPoint(DeviceIntPtr dev, DDXTouchPointInfoPtr ddxtouch);
DDXTouchPointInfoPtr TouchBeginDDXTouch(DeviceIntPtr dev, uint32_t ddx_id);
void TouchEndDDXTouch(DeviceIntPtr dev, DDXTouchPointInfoPtr ti);
DDXTouchPointInfoPtr TouchFindByDDXID(DeviceIntPtr dev,
                                      uint32_t ddx_id,
                                      Bool create);
Bool TouchInitTouchPoint(TouchClassPtr touch, ValuatorClassPtr v, int index);
void TouchFreeTouchPoint(DeviceIntPtr dev, int index);
TouchPointInfoPtr TouchBeginTouch(DeviceIntPtr dev,
                                  int sourceid,
                                  uint32_t touchid,
                                  Bool emulate_pointer);
TouchPointInfoPtr TouchFindByClientID(DeviceIntPtr dev, uint32_t client_id);
void TouchEndTouch(DeviceIntPtr dev, TouchPointInfoPtr ti);
Bool TouchEventHistoryAllocate(TouchPointInfoPtr ti);
void TouchEventHistoryFree(TouchPointInfoPtr ti);
void TouchEventHistoryPush(TouchPointInfoPtr ti, const DeviceEvent *ev);
void TouchEventHistoryReplay(TouchPointInfoPtr ti, DeviceIntPtr dev, XID resource);
Bool TouchResourceIsOwner(TouchPointInfoPtr ti, XID resource);
void TouchAddListener(TouchPointInfoPtr ti,
                      XID resource,
                      int resource_type,
                      enum InputLevel level,
                      enum TouchListenerType type,
                      enum TouchListenerState state,
                      WindowPtr window,
                      GrabPtr grab);
Bool TouchRemoveListener(TouchPointInfoPtr ti, XID resource);
void TouchSetupListeners(DeviceIntPtr dev,
                         TouchPointInfoPtr ti,
                         InternalEvent *ev);
Bool TouchBuildSprite(DeviceIntPtr sourcedev,
                      TouchPointInfoPtr ti,
                      InternalEvent *ev);
Bool TouchBuildDependentSpriteTrace(DeviceIntPtr dev, SpritePtr sprite);
int TouchConvertToPointerEvent(const InternalEvent *ev,
                               InternalEvent *motion,
                               InternalEvent *button);
int TouchGetPointerEventType(const InternalEvent *ev);
void TouchRemovePointerGrab(DeviceIntPtr dev);
void TouchListenerGone(XID resource);
int TouchListenerAcceptReject(DeviceIntPtr dev,
                              TouchPointInfoPtr ti,
                              int listener,
                              int mode);
int TouchAcceptReject(ClientPtr client,
                      DeviceIntPtr dev,
                      int mode,
                      uint32_t touchid,
                      Window grab_window,
                      XID *error);
void TouchEndPhysicallyActiveTouches(DeviceIntPtr dev);
void TouchEmitTouchEnd(DeviceIntPtr dev,
                       TouchPointInfoPtr ti,
                       int flags,
                       XID resource);
void TouchAcceptAndEnd(DeviceIntPtr dev, int touchid);

/* Gesture support */
void InitGestureEvent(InternalEvent *ievent,
                      DeviceIntPtr dev,
                      CARD32 ms,
                      int type,
                      uint16_t num_touches,
                      uint32_t flags,
                      double delta_x,
                      double delta_y,
                      double delta_unaccel_x,
                      double delta_unaccel_y,
                      double scale,
                      double delta_angle);
int GetGestureEvents(InternalEvent *events,
                     DeviceIntPtr dev,
                     uint16_t type,
                     uint16_t num_touches,
                     uint32_t flags,
                     double delta_x,
                     double delta_y,
                     double delta_unaccel_x,
                     double delta_unaccel_y,
                     double scale,
                     double delta_angle);
void QueueGesturePinchEvents(DeviceIntPtr dev,
                             uint16_t type,
                             uint16_t num_touches,
                             uint32_t flags,
                             double delta_x,
                             double delta_y,
                             double delta_unaccel_x,
                             double delta_unaccel_y,
                             double scale,
                             double delta_angle);
void QueueGestureSwipeEvents(DeviceIntPtr dev,
                             uint16_t type,
                             uint16_t num_touches,
                             uint32_t flags,
                             double delta_x,
                             double delta_y,
                             double delta_unaccel_x,
                             double delta_unaccel_y);
Bool GestureInitGestureInfo(GestureInfoPtr gesture);
void GestureFreeGestureInfo(GestureInfoPtr gesture);
GestureInfoPtr GestureBeginGesture(DeviceIntPtr dev, InternalEvent *ev);
GestureInfoPtr GestureFindActiveByEventType(DeviceIntPtr dev, int type);
void GestureEndGesture(GestureInfoPtr gi);
Bool GestureResourceIsOwner(GestureInfoPtr gi, XID resource);
void GestureAddListener(GestureInfoPtr gi,
                        XID resource,
                        int resource_type,
                        enum GestureListenerType type,
                        WindowPtr window,
                        GrabPtr grab);
void GestureSetupListener(DeviceIntPtr dev, GestureInfoPtr gi, InternalEvent *ev);
Bool GestureBuildSprite(DeviceIntPtr sourcedev, GestureInfoPtr gi);
void GestureListenerGone(XID resource);
void GestureEndActiveGestures(DeviceIntPtr dev);
void GestureEmitGestureEndToOwner(DeviceIntPtr dev, GestureInfoPtr gi);
void ProcessGestureEvent(InternalEvent *ev, DeviceIntPtr dev);

/* misc event helpers */
void CopyPartialInternalEvent(InternalEvent* dst_event,
                              const InternalEvent* src_event);
Mask GetEventMask(DeviceIntPtr dev, xEvent *ev, InputClientsPtr clients);
Mask GetEventFilter(DeviceIntPtr dev, xEvent *event);
Bool WindowXI2MaskIsset(DeviceIntPtr dev, WindowPtr win, xEvent *ev);
int GetXI2MaskByte(XI2Mask *mask, DeviceIntPtr dev, int event_type);
void FixUpEventFromWindow(SpritePtr pSprite,
                          xEvent *xE,
                          WindowPtr pWin,
                          Window child,
                          Bool calcChild,
                          enum InputLevel XILevel);
Bool PointInBorderSize(WindowPtr pWin, int x, int y);
WindowPtr XYToWindow(SpritePtr pSprite, int x, int y);
int EventIsDeliverable(DeviceIntPtr dev, int evtype, WindowPtr win);
Bool ActivatePassiveGrab(DeviceIntPtr dev,
                         GrabPtr grab,
                         InternalEvent *ev,
                         InternalEvent *real_event);
void ActivateGrabNoDelivery(DeviceIntPtr dev,
                            GrabPtr grab,
                            InternalEvent *event,
                            InternalEvent *real_event);

/* states for device grabs */

#define GRAB_STATE_NOT_GRABBED             0
#define GRAB_STATE_THAWED                  1
#define GRAB_STATE_THAWED_BOTH             2       /* not a real state */
#define GRAB_STATE_FREEZE_NEXT_EVENT       3
#define GRAB_STATE_FREEZE_BOTH_NEXT_EVENT  4
#define GRAB_STATE_FROZEN                  5       /* any state >= has device frozen */
#define GRAB_STATE_FROZEN_NO_EVENT         5
#define GRAB_STATE_FROZEN_WITH_EVENT       6
#define GRAB_STATE_THAW_OTHERS             7

/**
 * Masks specifying the type of event to deliver for an InternalEvent; used
 * by EventIsDeliverable.
 * @defgroup EventIsDeliverable return flags
 * @{
 */
#define EVENT_XI1_MASK                (1 << 0) /**< XI1.x event */
#define EVENT_CORE_MASK               (1 << 1) /**< Core event */
#define EVENT_DONT_PROPAGATE_MASK     (1 << 2) /**< DontPropagate mask set */
#define EVENT_XI2_MASK                (1 << 3) /**< XI2 mask set on window */
/* @} */

enum EventDeliveryState {
    EVENT_DELIVERED,     /**< Event has been delivered to a client  */
    EVENT_NOT_DELIVERED, /**< Event was not delivered to any client */
    EVENT_SKIP,          /**< Event can be discarded by the caller  */
    EVENT_REJECTED,      /**< Event was rejected for delivery to the client */
};

#define VALUATOR_MODE_ALL_AXES -1
int valuator_get_mode(DeviceIntPtr dev, int axis);
void valuator_set_mode(DeviceIntPtr dev, int axis, int mode);

/* Set to TRUE by default - os/utils.c sets it to FALSE on user request,
   xfixes/cursor.c uses it to determine if the cursor is enabled */
extern Bool EnableCursor;

/* Set to FALSE by default - ChangeWindowAttributes sets it to TRUE on
 * CWCursor, xfixes/cursor.c uses it to determine if the cursor is enabled
 */
extern Bool CursorVisible;

void valuator_mask_drop_unaccelerated(ValuatorMask *mask);

Bool point_on_screen(ScreenPtr pScreen, int x, int y);
void update_desktop_dimensions(void);

void input_constrain_cursor(DeviceIntPtr pDev,
                            ScreenPtr screen,
                            int current_x,
                            int current_y,
                            int dest_x,
                            int dest_y,
                            int *out_x,
                            int *out_y,
                            int *nevents,
                            InternalEvent* events);

void InputThreadPreInit(void);
void InputThreadInit(void);
void InputThreadFini(void);

int InputThreadRegisterDev(int fd,
                           NotifyFdProcPtr readInputProc,
                           void *readInputArgs);

int InputThreadUnregisterDev(int fd);

/*
 * @brief get current sprite cursor for input device
 *
 * @param pDev pointer to device structure
 * @return pointer to device cursor
 */
CursorPtr InputDevGetSpriteCursor(DeviceIntPtr pDev)
    _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * @brief confine cursor position to specific region
 *
 * this is used eg. when a cursor position should be moved, but the cursor
 * is constrained to specific region. it moves the position so it fits
 * into the region.
 *
 * @param region pointer to the constraining region
 * @param px     in/out buffer for X position
 * @param py     in/out buffer for Y position
 */
void ConfineToShape(RegionPtr region, int *px, int *py)
    _X_ATTRIBUTE_NONNULL_ARG(1,2,3);

/*
 * @brief get root window the input device is currently on
 *
 * @param pDev  pointer to input device structure
 * @return pointer to current root window
 */
WindowPtr InputDevCurrentRootWindow(DeviceIntPtr pDev)
    _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * @brief return Window underneath the input device's cursor sprite
 *
 * @param pDev  pointer to input device structure
 * @return pointer to window the cursor is currently above
 */
WindowPtr InputDevSpriteWindow(DeviceIntPtr pDev)
    _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * @brief deliver a raw input device event
 *
 * @param event     pointer to raw input device event structure
 * @param device    pointer to input device structure
 */
void DeliverRawEvent(RawDeviceEvent *event, DeviceIntPtr device)
    _X_ATTRIBUTE_NONNULL_ARG(1,2);

/*
 * @brief callback on input device events
 */
extern CallbackListPtr DeviceEventCallback;

/*
 * @brief pick an appropriate pointer for the given client.
 *
 * An "appropriate device" is (in order of priority):
 *  1) A device the given client has a core grab on.
 *  2) A device set as ClientPointer for the given client.
 *  3) The first master device.
 */
DeviceIntPtr PickPointer(ClientPtr pClient)
    _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * @brief pick an appropriate keyboard for the given client
 *
 * searching the list of devices for the keyboard device that is
 * paired with the client's pointer.
 */
DeviceIntPtr PickKeyboard(ClientPtr client)
    _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * @brief check whether input device is a pointer device
 *
 * @param dev   pointer to device structure
 * @return TRUE if dev is a pointer device
 */
Bool IsPointerDevice(DeviceIntPtr dev)
    _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * @brief check whether input device is a keyboard
 *
 * @param dev   pointer to device structure
 * @return TRUE if dev is a keyboard device
 */
Bool IsKeyboardDevice(DeviceIntPtr dev)
    _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * @brief check whether input device is a master
 *
 * @param dev   device to be checked
 * @return TRUE if the device is a master
 */
Bool InputDevIsMaster(DeviceIntPtr dev)
    _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * @brief check whether input device is floating
 *
 * when a slave device is grabbed directly (but not it's master), it's
 * temporarily detached from the master (for as long as the grab is held)
 * we call this state `floating`
 *
 * @param dev   device to check
 * @return TRUE if the device is in `floating` state
 */
Bool InputDevIsFloating(DeviceIntPtr dev)
    _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * @brief store timestamp as the device's last event time
 *
 * @param dev       device the timestamp is stored for
 * @param time      the timestamp to store
 */
void NoticeTime(const DeviceIntPtr dev, TimeStamp time)
    _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * @brief store event's timestamp as the device's last event time
 *
 * @param event     source event
 * @param dev       device the timestamp is stored for
 */
void NoticeEventTime(InternalEvent *ev, DeviceIntPtr dev)
    _X_ATTRIBUTE_NONNULL_ARG(1,2);

/*
 * @brief retrieve last event's timestamp for given device ID
 *
 * @param deviceid  ID of device get fetch timestamp for
 * @return timestamp of last event
 */
TimeStamp LastEventTime(int deviceid);

Bool LastEventTimeWasReset(int deviceid);

void LastEventTimeToggleResetFlag(int deviceid, Bool state);

void LastEventTimeToggleResetAll(Bool state);

/*
 * @brief count the bits set in the given bitmask
 *
 * @param mask pointer to bitmask
 * @param len size of bitmask in bits (may span multiple bytes)
 * @return number of bits set in the given bitmask
 */
static inline int CountBits(const uint8_t * mask, int len)
{
    int ret = 0;
    for (int i = 0; i < len; i++)
        if (BitIsOn(mask, i))
            ret++;
    return ret;
}

void AssignTypeAndName(DeviceIntPtr dev, Atom type, const char *name);

#endif /* _XSERVER_INPUT_PRIV_H */
