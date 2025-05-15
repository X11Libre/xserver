/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 1996 Thomas E. Dickey <dickey@clark.net>
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_EXEVENTS_PRIV_H
#define _XSERVER_EXEVENTS_PRIV_H

#include <X11/extensions/XIproto.h>
#include "exevents.h"

/**
 * Attached to the devPrivates of each client. Specifies the version number as
 * supported by the client.
 */
typedef struct _XIClientRec {
    int major_version;
    int minor_version;
} XIClientRec, *XIClientPtr;

typedef struct _GrabParameters {
    int grabtype;               /* CORE, etc. */
    unsigned int ownerEvents;
    unsigned int this_device_mode;
    unsigned int other_devices_mode;
    Window grabWindow;
    Window confineTo;
    Cursor cursor;
    unsigned int modifiers;
} GrabParameters;

int UpdateDeviceState(DeviceIntPtr device, DeviceEvent *xE);

void ProcessOtherEvent(InternalEvent *ev, DeviceIntPtr other);

int CheckGrabValues(ClientPtr client, GrabParameters *param);

int GrabButton(ClientPtr client,
               DeviceIntPtr dev,
               DeviceIntPtr modifier_device,
               int button,
               GrabParameters *param,
               enum InputLevel grabtype,
               GrabMask *eventMask);

int GrabKey(ClientPtr client,
            DeviceIntPtr dev,
            DeviceIntPtr modifier_device,
            int key,
            GrabParameters *param,
            enum InputLevel grabtype,
            GrabMask *eventMask);

int GrabWindow(ClientPtr client,
               DeviceIntPtr dev,
               int type,
               GrabParameters *param,
               GrabMask *eventMask);

int GrabTouchOrGesture(ClientPtr client,
                       DeviceIntPtr dev,
                       DeviceIntPtr mod_dev,
                       int type,
                       GrabParameters *param,
                       GrabMask *eventMask);

int SelectForWindow(DeviceIntPtr dev,
                    WindowPtr pWin,
                    ClientPtr client,
                    Mask mask,
                    Mask exclusivemasks);

int AddExtensionClient(WindowPtr pWin,
                       ClientPtr client,
                       Mask mask,
                       int mskidx);

void RecalculateDeviceDeliverableEvents(WindowPtr pWin);

int InputClientGone(WindowPtr pWin, XID id);

void WindowGone(WindowPtr win);

int SendEvent(ClientPtr client,
              DeviceIntPtr d,
              Window dest,
              Bool propagate,
              xEvent *ev,
              Mask mask ,
              int count);

int SetButtonMapping(ClientPtr client,
                     DeviceIntPtr dev,
                     int nElts,
                     BYTE *map);

int ChangeKeyMapping(ClientPtr client,
                     DeviceIntPtr dev,
                     unsigned len,
                     int type,
                     KeyCode firstKeyCode,
                     CARD8 keyCodes,
                     CARD8 keySymsPerKeyCode,
                     KeySym *map);

void DeleteWindowFromAnyExtEvents(WindowPtr pWin, Bool freeResources);

int MaybeSendDeviceMotionNotifyHint(deviceKeyButtonPointer *pEvents, Mask mask);

void CheckDeviceGrabAndHintWindow(WindowPtr pWin,
                                  int type,
                                  deviceKeyButtonPointer *xE,
                                  GrabPtr grab,
                                  ClientPtr client,
                                  Mask deliveryMask);

void MaybeStopDeviceHint(DeviceIntPtr dev, ClientPtr client);

int DeviceEventSuppressForWindow(WindowPtr pWin,
                                 ClientPtr client,
                                 Mask mask,
                                 int maskndx);

void SendEventToAllWindows(DeviceIntPtr dev, Mask mask, xEvent *ev, int count);

void TouchRejected(DeviceIntPtr sourcedev,
                   TouchPointInfoPtr ti,
                   XID resource,
                   TouchOwnershipEvent *ev);

_X_HIDDEN void XI2EventSwap(xGenericEvent *from, xGenericEvent *to);

/* For an event such as MappingNotify which affects client interpretation
 * of input events sent by device dev, should we notify the client, or
 * would it merely be irrelevant and confusing? */
int XIShouldNotify(ClientPtr client,
                   DeviceIntPtr dev);

void XISendDeviceChangedEvent(DeviceIntPtr device,
                              DeviceChangedEvent *dce);

int XISetEventMask(DeviceIntPtr dev,
                   WindowPtr win,
                   ClientPtr client,
                   unsigned int len,
                   unsigned char *mask);

int  XICheckInvalidMaskBits(ClientPtr client,
                            unsigned char *mask,
                            int len);

void XTestDeviceSendEvents(DeviceIntPtr dev,
                           int type,
                           int detail,
                           int flags,
                           const ValuatorMask *mask);

int XIPropToInt(XIPropertyValuePtr val, int *nelem_return, int **buf_return);

int XIPropToFloat(XIPropertyValuePtr val, int *nelem_return, float **buf_return);

#endif /* _XSERVER_EXEVENTS_PRIV_H */
