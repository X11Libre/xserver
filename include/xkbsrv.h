// exports scan complete

/************************************************************
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of Silicon Graphics not be
used in advertising or publicity pertaining to distribution
of the software without specific prior written permission.
Silicon Graphics makes no representation about the suitability
of this software for any purpose. It is provided "as is"
without any express or implied warranty.

SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

#ifndef _XKBSRV_H_
#define	_XKBSRV_H_

#define XkbFreeKeyboard			SrvXkbFreeKeyboard

#include <X11/Xdefs.h>
#include <X11/extensions/XKBproto.h>

#include "xkbstr.h"
#include "xkbrules.h"
#include "inputstr.h"
#include "events.h"

typedef struct _XkbInterest {
    DeviceIntPtr dev;
    ClientPtr client;
    XID resource;
    struct _XkbInterest *next;
    CARD16 extDevNotifyMask;
    CARD16 stateNotifyMask;
    CARD16 namesNotifyMask;
    CARD32 ctrlsNotifyMask;
    CARD8 compatNotifyMask;
    BOOL bellNotifyMask;
    BOOL actionMessageMask;
    CARD16 accessXNotifyMask;
    CARD32 iStateNotifyMask;
    CARD32 iMapNotifyMask;
    CARD16 altSymsNotifyMask;
    CARD32 autoCtrls;
    CARD32 autoCtrlValues;
} XkbInterestRec, *XkbInterestPtr;

typedef struct _XkbRadioGroup {
    CARD8 flags;
    CARD8 nMembers;
    CARD8 dfltDown;
    CARD8 currentDown;
    CARD8 members[XkbRGMaxMembers];
} XkbRadioGroupRec, *XkbRadioGroupPtr;

typedef struct _XkbEventCause {
    CARD8 kc;
    CARD8 event;
    CARD8 mjr;
    CARD8 mnr;
    ClientPtr client;
} XkbEventCauseRec, *XkbEventCausePtr;

typedef struct _XkbFilter {
    CARD16 keycode;
    CARD8 what;
    CARD8 active;
    CARD8 filterOthers;
    CARD32 priv;
    XkbAction upAction;
    int (*filter) (struct _XkbSrvInfo * /* xkbi */ ,
                   struct _XkbFilter * /* filter */ ,
                   unsigned /* keycode */ ,
                   XkbAction *  /* action */
        );
    struct _XkbFilter *next;
} XkbFilterRec, *XkbFilterPtr;

typedef Bool (*XkbSrvCheckRepeatPtr) (DeviceIntPtr dev,
                                      struct _XkbSrvInfo * /* xkbi */ ,
                                      unsigned /* keycode */);

typedef struct _XkbSrvInfo {
    XkbStateRec prev_state;
    XkbStateRec state;
    XkbDescPtr desc;

    DeviceIntPtr device;
    KbdCtrlProcPtr kbdProc;

    XkbRadioGroupPtr radioGroups;
    CARD8 nRadioGroups;
    CARD8 clearMods;
    CARD8 setMods;
    INT16 groupChange;

    CARD16 dfltPtrDelta;

    double mouseKeysCurve;
    double mouseKeysCurveFactor;
    INT16 mouseKeysDX;
    INT16 mouseKeysDY;
    CARD8 mouseKeysFlags;
    Bool mouseKeysAccel;
    CARD8 mouseKeysCounter;

    CARD8 lockedPtrButtons;
    CARD8 shiftKeyCount;
    KeyCode mouseKey;
    KeyCode inactiveKey;
    KeyCode slowKey;
    KeyCode slowKeyEnableKey;
    KeyCode repeatKey;
    CARD8 krgTimerActive;
    CARD8 beepType;
    CARD8 beepCount;

    CARD32 flags;
    CARD32 lastPtrEventTime;
    CARD32 lastShiftEventTime;
    OsTimerPtr beepTimer;
    OsTimerPtr mouseKeyTimer;
    OsTimerPtr slowKeysTimer;
    OsTimerPtr bounceKeysTimer;
    OsTimerPtr repeatKeyTimer;
    OsTimerPtr krgTimer;

    int szFilters;
    XkbFilterPtr filters;

    XkbSrvCheckRepeatPtr checkRepeat;

    char overlay_perkey_state[256/8]; /* bitfield */
} XkbSrvInfoRec, *XkbSrvInfoPtr;

typedef struct _XkbSrvLedInfo {
    CARD16 flags;
    CARD16 class;
    CARD16 id;
    union {
        KbdFeedbackPtr kf;
        LedFeedbackPtr lf;
    } fb;

    CARD32 physIndicators;
    CARD32 autoState;
    CARD32 explicitState;
    CARD32 effectiveState;

    CARD32 mapsPresent;
    CARD32 namesPresent;
    XkbIndicatorMapPtr maps;
    Atom *names;

    CARD32 usesBase;
    CARD32 usesLatched;
    CARD32 usesLocked;
    CARD32 usesEffective;
    CARD32 usesCompat;
    CARD32 usesControls;

    CARD32 usedComponents;
} XkbSrvLedInfoRec, *XkbSrvLedInfoPtr;

typedef struct {
    ProcessInputProc processInputProc;
    /* If processInputProc is set to something different than realInputProc,
     * UNWRAP and COND_WRAP will not touch processInputProc and update only
     * realInputProc.  This ensures that
     *   processInputProc == (frozen ? EnqueueEvent : realInputProc)
     *
     * WRAP_PROCESS_INPUT_PROC should only be called during initialization,
     * since it may destroy this invariant.
     */
    ProcessInputProc realInputProc;
    DeviceUnwrapProc unwrapProc;
} xkbDeviceInfoRec, *xkbDeviceInfoPtr;

/***====================================================================***/

#define	Status		int

extern _X_EXPORT void XkbFreeKeyboard(XkbDescPtr /* xkb */ ,
                                      unsigned int /* which */ ,
                                      Bool      /* freeDesc */
    );

extern _X_EXPORT KeySymsPtr XkbGetCoreMap(DeviceIntPtr  /* keybd */
    );

extern _X_EXPORT void XkbApplyMappingChange(DeviceIntPtr /* pXDev */ ,
                                            KeySymsPtr /* map */ ,
                                            KeyCode /* firstKey */ ,
                                            CARD8 /* num */ ,
                                            CARD8 * /* modmap */ ,
                                            ClientPtr   /* client */
    );

extern _X_EXPORT void XkbDDXChangeControls(DeviceIntPtr /* dev */ ,
                                           XkbControlsPtr /* old */ ,
                                           XkbControlsPtr       /* new */
    );

extern _X_EXPORT void XkbSetRepeatKeys(DeviceIntPtr /* pXDev */ ,
                                       int /* key */ ,
                                       int      /* onoff */
    );

extern _X_EXPORT void XkbGetRulesDflts(XkbRMLVOSet *    /* rmlvo */
    );

extern _X_EXPORT void XkbFreeRMLVOSet(XkbRMLVOSet * /* rmlvo */ ,
                                      Bool      /* freeRMLVO */
    );

extern _X_EXPORT Bool XkbCopyDeviceKeymap(DeviceIntPtr /* dst */,
					  DeviceIntPtr /* src */);

#include "xkbstr.h"
#include "xkbrules.h"

#endif                          /* _XKBSRV_H_ */
