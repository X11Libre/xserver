/* SPDX-License-Identifier: MIT OR X11
 *
 + Copyright © 1987, 1998  The Open Group
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_INPUT_PRIV_H
#define _XSERVER_INPUT_PRIV_H

#include "input.h"

typedef struct _InputOption InputOption;
typedef struct _XI2Mask XI2Mask;

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
                          Bool calcChild);
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

#endif /* _XSERVER_INPUT_PRIV_H */
