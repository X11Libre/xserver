/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 1993 Silicon Graphics Computer Systems, Inc.
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_XKBSRV_PRIV_H_
#define _XSERVER_XKBSRV_PRIV_H_

#include <X11/Xdefs.h>
#include <X11/Xmd.h>

#include "dix.h"
#include "input.h"
#include "xkbsrv.h"
#include "xkbstr.h"

void xkbUnwrapProc(DeviceIntPtr, DeviceHandleProc, void *);

void XkbForceUpdateDeviceLEDs(DeviceIntPtr keybd);

void XkbPushLockedStateToSlaves(DeviceIntPtr master, int evtype, int key);

Bool XkbCopyKeymap(XkbDescPtr dst, XkbDescPtr src);

void XkbFilterEvents(ClientPtr pClient, int nEvents, xEvent *xE);

int XkbGetEffectiveGroup(XkbSrvInfoPtr xkbi, XkbStatePtr xkbstate, CARD8 keycode);

void XkbMergeLockedPtrBtns(DeviceIntPtr master);

void XkbFakeDeviceButton(DeviceIntPtr dev, int press, int button);

#endif /* _XSERVER_XKBSRV_PRIV_H_ */
