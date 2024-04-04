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

#define XkbChangeTypesOfKey		SrvXkbChangeTypesOfKey
#define XkbCopyKeyTypes			SrvXkbCopyKeyTypes
#define	XkbKeyTypesForCoreSymbols	SrvXkbKeyTypesForCoreSymbols
#define	XkbApplyCompatMapToKey		SrvXkbApplyCompatMapToKey
#define XkbResizeKeyActions		SrvXkbResizeKeyActions
#define XkbResizeKeySyms		SrvXkbResizeKeySyms
#define XkbResizeKeyType		SrvXkbResizeKeyType
#define XkbAllocCompatMap		SrvXkbAllocCompatMap
#define XkbAllocControls		SrvXkbAllocControls
#define XkbAllocIndicatorMaps		SrvXkbAllocIndicatorMaps
#define XkbAllocKeyboard		SrvXkbAllocKeyboard
#define XkbAllocNames			SrvXkbAllocNames
#define XkbFreeCompatMap		SrvXkbFreeCompatMap
#define XkbFreeKeyboard			SrvXkbFreeKeyboard
#define XkbFreeNames			SrvXkbFreeNames
#define XkbVirtualModsToReal		SrvXkbVirtualModsToReal
#define	XkbChangeKeycodeRange		SrvXkbChangeKeycodeRange
#define	XkbApplyVirtualModChanges	SrvXkbApplyVirtualModChanges

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

extern _X_EXPORT void XkbUseMsg(void
    );

extern _X_EXPORT int XkbProcessArguments(int /* argc */ ,
                                         char ** /* argv */ ,
                                         int    /* i */
    );

extern _X_EXPORT Bool XkbInitPrivates(void);

extern _X_EXPORT void XkbSetExtension(DeviceIntPtr device,
                                      ProcessInputProc proc);

extern _X_EXPORT void XkbFreeCompatMap(XkbDescPtr /* xkb */ ,
                                       unsigned int /* which */ ,
                                       Bool     /* freeMap */
    );

extern _X_EXPORT void XkbFreeNames(XkbDescPtr /* xkb */ ,
                                   unsigned int /* which */ ,
                                   Bool /* freeMap */
    );

extern _X_EXPORT XkbDescPtr XkbAllocKeyboard(void
    );

extern _X_EXPORT Status XkbAllocIndicatorMaps(XkbDescPtr        /* xkb */
    );

extern _X_EXPORT Status XkbAllocCompatMap(XkbDescPtr /* xkb */ ,
                                          unsigned int /* which */ ,
                                          unsigned int  /* nInterpret */
    );

extern _X_EXPORT Status XkbAllocNames(XkbDescPtr /* xkb */ ,
                                      unsigned int /* which */ ,
                                      int /* nTotalRG */ ,
                                      int       /* nTotalAliases */
    );

extern _X_EXPORT Status XkbAllocControls(XkbDescPtr /* xkb */ ,
                                         unsigned int   /* which */
    );

extern _X_EXPORT Status XkbCopyKeyTypes(XkbKeyTypePtr /* from */ ,
                                        XkbKeyTypePtr /* into */ ,
                                        int     /* num_types */
    );

extern _X_EXPORT Status XkbResizeKeyType(XkbDescPtr /* xkb */ ,
                                         int /* type_ndx */ ,
                                         int /* map_count */ ,
                                         Bool /* want_preserve */ ,
                                         int    /* new_num_lvls */
    );

extern _X_EXPORT void XkbFreeKeyboard(XkbDescPtr /* xkb */ ,
                                      unsigned int /* which */ ,
                                      Bool      /* freeDesc */
    );

extern _X_EXPORT void XkbFreeComponentNames(XkbComponentNamesPtr /* names */ ,
                                            Bool        /* freeNames */
    );

extern _X_EXPORT void XkbSetActionKeyMods(XkbDescPtr /* xkb */ ,
                                          XkbAction * /* act */ ,
                                          unsigned int  /* mods */
    );

extern _X_EXPORT unsigned int XkbMaskForVMask(XkbDescPtr /* xkb */ ,
                                              unsigned int      /* vmask */
    );

extern _X_EXPORT Bool XkbVirtualModsToReal(XkbDescPtr /* xkb */ ,
                                           unsigned int /* virtua_mask */ ,
                                           unsigned int *       /* mask_rtrn */
    );

extern _X_EXPORT unsigned int XkbAdjustGroup(int /* group */ ,
                                             XkbControlsPtr     /* ctrls */
    );

extern _X_EXPORT KeySym *XkbResizeKeySyms(XkbDescPtr /* xkb */ ,
                                          int /* key */ ,
                                          int   /* needed */
    );

extern _X_EXPORT XkbAction *XkbResizeKeyActions(XkbDescPtr /* xkb */ ,
                                                int /* key */ ,
                                                int     /* needed */
    );

extern _X_EXPORT void XkbUpdateDescActions(XkbDescPtr /* xkb */ ,
                                           KeyCode /* first */ ,
                                           CARD8 /* num */ ,
                                           XkbChangesPtr        /* changes */
    );

extern _X_EXPORT void XkbUpdateActions(DeviceIntPtr /* pXDev */ ,
                                       KeyCode /* first */ ,
                                       CARD8 /* num */ ,
                                       XkbChangesPtr /* pChanges */ ,
                                       unsigned int * /* needChecksRtrn */ ,
                                       XkbEventCausePtr /* cause */
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

extern _X_EXPORT void XkbSetIndicators(DeviceIntPtr /* pXDev */ ,
                                       CARD32 /* affect */ ,
                                       CARD32 /* values */ ,
                                       XkbEventCausePtr /* cause */
    );

extern _X_EXPORT void XkbUpdateIndicators(DeviceIntPtr /* keybd */ ,
                                          CARD32 /* changed */ ,
                                          Bool /* check_edevs */ ,
                                          XkbChangesPtr /* pChanges */ ,
                                          XkbEventCausePtr      /* cause */
    );

extern _X_EXPORT void XkbUpdateAllDeviceIndicators(XkbChangesPtr /* changes */,
                                                   XkbEventCausePtr /* cause */
    );

extern _X_EXPORT unsigned int XkbIndicatorsToUpdate(DeviceIntPtr /* dev */ ,
                                                    unsigned long
                                                    /* state_changes */ ,
                                                    Bool        /* enabled_ctrl_changes */
    );

extern _X_EXPORT void XkbComputeDerivedState(XkbSrvInfoPtr      /* xkbi */
    );

extern _X_EXPORT void XkbCheckSecondaryEffects(XkbSrvInfoPtr /* xkbi */ ,
                                               unsigned int /* which */ ,
                                               XkbChangesPtr /* changes */ ,
                                               XkbEventCausePtr /* cause */
    );

extern _X_EXPORT void XkbCheckIndicatorMaps(DeviceIntPtr /* dev */ ,
                                            XkbSrvLedInfoPtr /* sli */ ,
                                            unsigned int        /* which */
    );

extern _X_EXPORT unsigned int XkbStateChangedFlags(XkbStatePtr /* old */ ,
                                                   XkbStatePtr  /* new */
    );

extern _X_EXPORT void XkbHandleBell(BOOL force,
                                    BOOL eventOnly,
                                    DeviceIntPtr kbd,
                                    CARD8 percent,
                                    void *ctrl,
                                    CARD8 class,
                                    Atom name,
                                    WindowPtr pWin,
                                    ClientPtr pClient
    );

extern _X_EXPORT void XkbProcessKeyboardEvent(DeviceEvent * /* event */ ,
                                              DeviceIntPtr      /* keybd */
    );

extern _X_EXPORT void XkbHandleActions(DeviceIntPtr /* dev */ ,
                                       DeviceIntPtr /* kbd */ ,
                                       DeviceEvent *    /* event */
    );

extern _X_EXPORT Bool XkbEnableDisableControls(XkbSrvInfoPtr /* xkbi */ ,
                                               unsigned long /* change */ ,
                                               unsigned long /* newValues */ ,
                                               XkbChangesPtr /* changes */ ,
                                               XkbEventCausePtr /* cause */
    );

extern _X_EXPORT int XkbDDXUsesSoftRepeat(DeviceIntPtr  /* dev */
    );

extern _X_EXPORT void XkbDDXKeybdCtrlProc(DeviceIntPtr /* dev */ ,
                                          KeybdCtrl *   /* ctrl */
    );

extern _X_EXPORT void XkbDDXChangeControls(DeviceIntPtr /* dev */ ,
                                           XkbControlsPtr /* old */ ,
                                           XkbControlsPtr       /* new */
    );

extern _X_EXPORT void XkbDDXUpdateDeviceIndicators(DeviceIntPtr /* dev */ ,
                                                   XkbSrvLedInfoPtr /* sli */ ,
                                                   CARD32       /* newState */
    );

extern _X_EXPORT void XkbDisableComputedAutoRepeats(DeviceIntPtr /* pXDev */ ,
                                                    unsigned int        /* key */
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

extern _X_EXPORT XkbGeometryPtr XkbLookupNamedGeometry(DeviceIntPtr /* dev */ ,
                                                       Atom /* name */ ,
                                                       Bool *   /* shouldFree */
    );

extern _X_EXPORT void XkbConvertCase(KeySym /* sym */ ,
                                     KeySym * /* lower */ ,
                                     KeySym *   /* upper */
    );

extern _X_EXPORT Status XkbChangeKeycodeRange(XkbDescPtr /* xkb */ ,
                                              int /* minKC */ ,
                                              int /* maxKC */ ,
                                              XkbChangesPtr     /* changes */
    );

extern _X_EXPORT void XkbFreeInfo(XkbSrvInfoPtr /* xkbi */
    );

extern _X_EXPORT Status XkbChangeTypesOfKey(XkbDescPtr /* xkb */ ,
                                            int /* key */ ,
                                            int /* nGroups */ ,
                                            unsigned int /* groups */ ,
                                            int * /* newTypesIn */ ,
                                            XkbMapChangesPtr    /* changes */
    );

extern _X_EXPORT int XkbKeyTypesForCoreSymbols(XkbDescPtr /* xkb */ ,
                                               int /* map_width */ ,
                                               KeySym * /* core_syms */ ,
                                               unsigned int /* protected */ ,
                                               int * /* types_inout */ ,
                                               KeySym * /* xkb_syms_rtrn */
    );

extern _X_EXPORT Bool XkbApplyCompatMapToKey(XkbDescPtr /* xkb */ ,
                                             KeyCode /* key */ ,
                                             XkbChangesPtr      /* changes */
    );

extern _X_EXPORT Bool XkbApplyVirtualModChanges(XkbDescPtr /* xkb */ ,
                                                unsigned int /* changed */ ,
                                                XkbChangesPtr   /* changes */
    );

extern _X_EXPORT void XkbSendNewKeyboardNotify(DeviceIntPtr /* kbd */ ,
                                               xkbNewKeyboardNotify *   /* pNKN */
    );

extern _X_EXPORT Bool XkbCopyDeviceKeymap(DeviceIntPtr /* dst */,
					  DeviceIntPtr /* src */);

extern _X_EXPORT Bool XkbDeviceApplyKeymap(DeviceIntPtr /* dst */ ,
                                           XkbDescPtr /* src */ );

extern _X_EXPORT void XkbCopyControls(XkbDescPtr /* dst */ ,
                                      XkbDescPtr /* src */ );

#include "xkbstr.h"
#include "xkbrules.h"

extern _X_EXPORT unsigned int XkbDDXLoadKeymapByNames(DeviceIntPtr /* keybd */ ,
                                                      XkbComponentNamesPtr
                                                      /* names */ ,
                                                      unsigned int /* want */ ,
                                                      unsigned int /* need */ ,
                                                      XkbDescPtr *
                                                      /* finfoRtrn */ ,
                                                      char *
                                                      /* keymapNameRtrn */ ,
                                                      int       /* keymapNameRtrnLen */
    );

extern _X_EXPORT Bool XkbDDXNamesFromRules(DeviceIntPtr /* keybd */ ,
                                           const char * /* rules */ ,
                                           XkbRF_VarDefsPtr /* defs */ ,
                                           XkbComponentNamesPtr /* names */
    );

#endif                          /* _XKBSRV_H_ */
