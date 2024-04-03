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

#define _BEEP_NONE              0
#define _BEEP_FEATURE_ON        1
#define _BEEP_FEATURE_OFF       2
#define _BEEP_FEATURE_CHANGE    3
#define _BEEP_SLOW_WARN         4
#define _BEEP_SLOW_PRESS        5
#define _BEEP_SLOW_ACCEPT       6
#define _BEEP_SLOW_REJECT       7
#define _BEEP_SLOW_RELEASE      8
#define _BEEP_STICKY_LATCH      9
#define _BEEP_STICKY_LOCK       10
#define _BEEP_STICKY_UNLOCK     11
#define _BEEP_LED_ON            12
#define _BEEP_LED_OFF           13
#define _BEEP_LED_CHANGE        14
#define _BEEP_BOUNCE_REJECT     15

#define XkbSetCauseKey(c,k,e)   { (c)->kc= (k),(c)->event= (e),\
                                  (c)->mjr= (c)->mnr= 0; \
                                  (c)->client= NULL; }
#define XkbSetCauseReq(c,j,n,cl) { (c)->kc= (c)->event= 0,\
                                  (c)->mjr= (j),(c)->mnr= (n);\
                                  (c)->client= (cl); }
#define XkbSetCauseCoreReq(c,e,cl) XkbSetCauseReq(c,e,0,cl)
#define XkbSetCauseXkbReq(c,e,cl)  XkbSetCauseReq(c,XkbReqCode,e,cl)
#define XkbSetCauseUnknown(c)      XkbSetCauseKey(c,0,0)

#define XkbSLI_IsDefault        (1L<<0)
#define XkbSLI_HasOwnState      (1L<<1)

void xkbUnwrapProc(DeviceIntPtr, DeviceHandleProc, void *);

void XkbForceUpdateDeviceLEDs(DeviceIntPtr keybd);

void XkbPushLockedStateToSlaves(DeviceIntPtr master, int evtype, int key);

Bool XkbCopyKeymap(XkbDescPtr dst, XkbDescPtr src);

void XkbFilterEvents(ClientPtr pClient, int nEvents, xEvent *xE);

int XkbGetEffectiveGroup(XkbSrvInfoPtr xkbi, XkbStatePtr xkbstate, CARD8 keycode);

void XkbMergeLockedPtrBtns(DeviceIntPtr master);

void XkbFakeDeviceButton(DeviceIntPtr dev, int press, int button);

#endif /* _XSERVER_XKBSRV_PRIV_H_ */
