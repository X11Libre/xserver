/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 1993 Silicon Graphics Computer Systems, Inc.
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_XKBSRV_PRIV_H_
#define _XSERVER_XKBSRV_PRIV_H_

#include <X11/Xdefs.h>
#include <X11/Xmd.h>

#include "xkb/xkbrules_priv.h"

#include "dix.h"
#include "input.h"
#include "misc.h"
#include "privates.h"
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

#define XkbAX_KRGMask    (XkbSlowKeysMask|XkbBounceKeysMask)
#define XkbAllFilteredEventsMask \
        (XkbAccessXKeysMask|XkbRepeatKeysMask|XkbMouseKeysAccelMask|XkbAX_KRGMask)

/*
 * Settings for xkbClientFlags field (used by DIX)
 * These flags _must_ not overlap with XkbPCF_*
 */
#define _XkbClientInitialized           (1<<7)
#define _XkbClientIsAncient             (1<<6)

/*
 * Settings for flags field
 */
#define _XkbStateNotifyInProgress       (1<<0)

#define _XkbLibError(c,l,d)     /* Epoch fail */

/* "a" is a "unique" numeric identifier that just defines which error
 * code statement it is. _XkbErrCode2(4, foo) means "this is the 4th error
 * statement in this function". lovely.
 */
#define _XkbErrCode2(a,b) ((XID)(((unsigned int)((unsigned int)(a))<<24)|((b)&0xffffff)))
#define _XkbErrCode3(a,b,c)     _XkbErrCode2(a,((unsigned int)((unsigned int)(b))<<16)|(c))
#define _XkbErrCode4(a,b,c,d) _XkbErrCode3(a,b,(((unsigned int)((unsigned int)(c))<<8)|(d)))

#define WRAP_PROCESS_INPUT_PROC(device, oldprocs, proc, unwrapproc) \
        device->public.processInputProc = proc; \
        oldprocs->processInputProc = \
        oldprocs->realInputProc = device->public.realInputProc; \
        device->public.realInputProc = proc; \
        oldprocs->unwrapProc = device->unwrapProc; \
        device->unwrapProc = unwrapproc;

#define COND_WRAP_PROCESS_INPUT_PROC(device, oldprocs, proc, unwrapproc) \
        if (device->public.processInputProc == device->public.realInputProc)\
            device->public.processInputProc = proc; \
        oldprocs->processInputProc = \
        oldprocs->realInputProc = device->public.realInputProc; \
        device->public.realInputProc = proc; \
        oldprocs->unwrapProc = device->unwrapProc; \
        device->unwrapProc = unwrapproc;

#define UNWRAP_PROCESS_INPUT_PROC(device, oldprocs, backupproc) \
        backupproc = device->public.realInputProc; \
        if (device->public.processInputProc == device->public.realInputProc)\
            device->public.processInputProc = oldprocs->realInputProc; \
        device->public.realInputProc = oldprocs->realInputProc; \
        device->unwrapProc = oldprocs->unwrapProc;

void xkbUnwrapProc(DeviceIntPtr, DeviceHandleProc, void *);

void XkbForceUpdateDeviceLEDs(DeviceIntPtr keybd);

void XkbPushLockedStateToSlaves(DeviceIntPtr master, int evtype, int key);

Bool XkbCopyKeymap(XkbDescPtr dst, XkbDescPtr src);

void XkbFilterEvents(ClientPtr pClient, int nEvents, xEvent *xE);

int XkbGetEffectiveGroup(XkbSrvInfoPtr xkbi, XkbStatePtr xkbstate, CARD8 keycode);

void XkbMergeLockedPtrBtns(DeviceIntPtr master);

void XkbFakeDeviceButton(DeviceIntPtr dev, int press, int button);
void XkbUseMsg(void);
int XkbProcessArguments(int argc, char **argv, int i);
Bool XkbInitPrivates(void);
void XkbSetExtension(DeviceIntPtr device, ProcessInputProc proc);
void XkbFreeCompatMap(XkbDescPtr xkb, unsigned int which, Bool freeMap);
void XkbFreeNames(XkbDescPtr xkb, unsigned int which, Bool freeMap);
XkbDescPtr XkbAllocKeyboard(void);
int XkbAllocIndicatorMaps(XkbDescPtr xkb);
int XkbAllocCompatMap(XkbDescPtr xkb, unsigned int which, unsigned int nInterpret);
int XkbAllocNames(XkbDescPtr xkb, unsigned int which, int nTotalRG,
                  int nTotalAliases);
int XkbAllocControls(XkbDescPtr xkb, unsigned int which);
int XkbCopyKeyTypes(XkbKeyTypePtr from, XkbKeyTypePtr into, int num_types);
int XkbResizeKeyType(XkbDescPtr xkb, int type_ndx, int map_count,
                     Bool want_preserve, int new_num_lvls);
void XkbFreeComponentNames(XkbComponentNamesPtr names, Bool freeNames);
void XkbSetActionKeyMods(XkbDescPtr xkb, XkbAction *act, unsigned int mods);
unsigned int XkbMaskForVMask(XkbDescPtr xkb, unsigned int vmask);
Bool XkbVirtualModsToReal(XkbDescPtr xkb, unsigned int virtua_mask,
                          unsigned int *mask_rtrn);
unsigned int XkbAdjustGroup(int group, XkbControlsPtr ctrls);
KeySym *XkbResizeKeySyms(XkbDescPtr xkb, int key, int needed);
XkbAction *XkbResizeKeyActions(XkbDescPtr xkb, int key, int needed);
void XkbUpdateDescActions(XkbDescPtr xkb, KeyCode first, CARD8 num, XkbChangesPtr changes);
void XkbUpdateActions(DeviceIntPtr pXDev, KeyCode first, CARD8 num,
                      XkbChangesPtr pChanges, unsigned int *needChecksRtrn,
                      XkbEventCausePtr);
void XkbSetIndicators(DeviceIntPtr pXDev, CARD32 affect, CARD32 values,
                      XkbEventCausePtr cause);
void XkbUpdateIndicators(DeviceIntPtr keybd, CARD32 changed, Bool check_edevs,
                         XkbChangesPtr pChanges, XkbEventCausePtr cause);
void XkbUpdateAllDeviceIndicators(XkbChangesPtr changes, XkbEventCausePtr cause);
unsigned int XkbIndicatorsToUpdate(DeviceIntPtr dev, unsigned long state_changes,
                                   Bool enabled_ctrl_changes);
void XkbComputeDerivedState(XkbSrvInfoPtr xkbi);
void XkbCheckSecondaryEffects(XkbSrvInfoPtr xkbi, unsigned int which,
                              XkbChangesPtr changes, XkbEventCausePtr cause);
void XkbCheckIndicatorMaps(DeviceIntPtr dev, XkbSrvLedInfoPtr sli,
                           unsigned int which);
unsigned int XkbStateChangedFlags(XkbStatePtr old, XkbStatePtr new);
void XkbHandleBell(BOOL force, BOOL eventOnly, DeviceIntPtr kbd, CARD8 percent,
                   void *ctrl, CARD8 class, Atom name, WindowPtr pWin,
                   ClientPtr pClient);
void XkbHandleActions(DeviceIntPtr dev, DeviceIntPtr kbd, DeviceEvent *event);
void XkbProcessKeyboardEvent(DeviceEvent *event, DeviceIntPtr keybd);
Bool XkbEnableDisableControls(XkbSrvInfoPtr xkbi, unsigned long change,
                              unsigned long newValues, XkbChangesPtr changes,
                              XkbEventCausePtr cause);
void XkbDisableComputedAutoRepeats(DeviceIntPtr pXDev, unsigned int key);
XkbGeometryPtr XkbLookupNamedGeometry(DeviceIntPtr dev, Atom name, Bool *shouldFree);
void XkbConvertCase(KeySym sym, KeySym *lower, KeySym *upper);
int XkbChangeKeycodeRange(XkbDescPtr xkb, int minKC, int maxKC, XkbChangesPtr changes);
void XkbFreeInfo(XkbSrvInfoPtr xkbi);
int XkbChangeTypesOfKey(XkbDescPtr xkb, int key, int nGroups, unsigned int groups,
                        int *newTypesIn, XkbMapChangesPtr changes);
int XkbKeyTypesForCoreSymbols(XkbDescPtr xkb, int map_width, KeySym *core_syms,
                              unsigned int protected, int *types_inout,
                              KeySym *xkb_syms_rtrn);
Bool XkbApplyCompatMapToKey(XkbDescPtr xkb, KeyCode key, XkbChangesPtr changes);
Bool XkbApplyVirtualModChanges(XkbDescPtr xkb, unsigned int changed,
                               XkbChangesPtr changes);
Bool XkbDeviceApplyKeymap(DeviceIntPtr dst, XkbDescPtr src);
void XkbCopyControls(XkbDescPtr dst, XkbDescPtr src);


extern DevPrivateKeyRec xkbDevicePrivateKeyRec;

#define XKBDEVICEINFO(dev) ((xkbDeviceInfoPtr)dixLookupPrivate(&(dev)->devPrivates, &xkbDevicePrivateKeyRec))

extern int XkbReqCode;
extern int XkbEventBase;
extern int XkbKeyboardErrorCode;
extern const char *XkbBaseDirectory;
extern const char *XkbBinDirectory;
extern CARD32 xkbDebugFlags;

/* AccessX functions */
void XkbSendAccessXNotify(DeviceIntPtr kbd, xkbAccessXNotify *pEv);
void AccessXInit(DeviceIntPtr dev);
Bool AccessXFilterPressEvent(DeviceEvent *event, DeviceIntPtr keybd);
Bool AccessXFilterReleaseEvent(DeviceEvent *event, DeviceIntPtr keybd);
void AccessXCancelRepeatKey(XkbSrvInfoPtr xkbi, KeyCode key);
void AccessXComputeCurveFactor(XkbSrvInfoPtr xkbi, XkbControlsPtr ctrls);
int XkbDDXAccessXBeep(DeviceIntPtr dev, unsigned int what, unsigned int which);

/* DDX entry points - DDX needs to implement these */
int XkbDDXTerminateServer(DeviceIntPtr dev, KeyCode key, XkbAction *act);
int XkbDDXSwitchScreen(DeviceIntPtr dev, KeyCode key, XkbAction *act);
int XkbDDXPrivate(DeviceIntPtr dev, KeyCode key, XkbAction *act);

/* client resources */
XkbInterestPtr XkbFindClientResource(DevicePtr inDev, ClientPtr client);
XkbInterestPtr XkbAddClientResource(DevicePtr inDev, ClientPtr client, XID id);
int XkbRemoveResourceClient(DevicePtr inDev, XID id);

/* key latching */
int XkbLatchModifiers(DeviceIntPtr pXDev, CARD8 mask, CARD8 latches);
int XkbLatchGroup(DeviceIntPtr pXDev, int group);
void XkbClearAllLatchesAndLocks(DeviceIntPtr dev, XkbSrvInfoPtr xkbi,
                                Bool genEv, XkbEventCausePtr cause);

/* xkb rules */
void XkbInitRules(XkbRMLVOSet *rmlvo, const char *rules,
                  const char *model, const char *layout,
                  const char *variant, const char *options);
void XkbSetRulesDflts(XkbRMLVOSet *rmlvo);
void XkbDeleteRulesDflts(void);
void XkbDeleteRulesUsed(void);

/* notification sending */
void XkbSendStateNotify(DeviceIntPtr kbd, xkbStateNotify *pSN);
void XkbSendMapNotify(DeviceIntPtr kbd, xkbMapNotify *ev);
int XkbComputeControlsNotify(DeviceIntPtr kbd, XkbControlsPtr old,
                             XkbControlsPtr new, xkbControlsNotify *pCN,
                             Bool forceCtrlProc);
void XkbSendControlsNotify(DeviceIntPtr kbd, xkbControlsNotify *ev);
void XkbSendCompatMapNotify(DeviceIntPtr kbd, xkbCompatMapNotify *ev);
void XkbSendNamesNotify(DeviceIntPtr kbd, xkbNamesNotify *ev);
void XkbSendActionMessage(DeviceIntPtr kbd, xkbActionMessage *ev);
void XkbSendExtensionDeviceNotify(DeviceIntPtr kbd, ClientPtr client,
                                  xkbExtensionDeviceNotify *ev);
void XkbSendNotification(DeviceIntPtr kbd, XkbChangesPtr pChanges,
                         XkbEventCausePtr cause);
void XkbSendNewKeyboardNotify(DeviceIntPtr kbd, xkbNewKeyboardNotify *pNKN);

/* device lookup */
int _XkbLookupAnyDevice(DeviceIntPtr *pDev, int id, ClientPtr client,
                        Mask access_mode, int *xkb_err);
int _XkbLookupKeyboard(DeviceIntPtr *pDev, int id, ClientPtr client,
                       Mask access_mode, int *xkb_err);
int _XkbLookupBellDevice(DeviceIntPtr *pDev, int id, ClientPtr client,
                         Mask access_mode, int *xkb_err);
int _XkbLookupLedDevice(DeviceIntPtr *pDev, int id, ClientPtr client,
                        Mask access_mode, int *xkb_err);
int _XkbLookupButtonDevice(DeviceIntPtr *pDev, int id, ClientPtr client,
                           Mask access_mode, int *xkb_err);

/* XkbSrvLedInfo functions */
XkbSrvLedInfoPtr XkbAllocSrvLedInfo(DeviceIntPtr dev, KbdFeedbackPtr kf,
                                    LedFeedbackPtr lf, unsigned int needed_parts);
XkbSrvLedInfoPtr XkbCopySrvLedInfo(DeviceIntPtr dev, XkbSrvLedInfoPtr src,
                                   KbdFeedbackPtr kf, LedFeedbackPtr lf);
XkbSrvLedInfoPtr XkbFindSrvLedInfo(DeviceIntPtr dev, unsigned int class,
                                   unsigned int id, unsigned int needed_parts);
void XkbFreeSrvLedInfo(XkbSrvLedInfoPtr sli);

/* keymap compile */
XkbDescPtr XkbCompileKeymap(DeviceIntPtr dev, XkbRMLVOSet *rmlvo);
XkbDescPtr XkbCompileKeymapFromString(DeviceIntPtr dev, const char *keymap,
                                      int keymap_length);

/* client map */
int XkbAllocClientMap(XkbDescPtr xkb, unsigned int which, unsigned int nTypes);
void XkbFreeClientMap(XkbDescPtr xkb, unsigned int what, Bool freeMap);

/* server map */
int XkbAllocServerMap(XkbDescPtr xkb, unsigned int which, unsigned int nNewActions);
void XkbFreeServerMap(XkbDescPtr xkb, unsigned int what, Bool freeMap);

/* led functions */
void XkbApplyLedNameChanges(DeviceIntPtr dev, XkbSrvLedInfoPtr sli,
                            unsigned int changed_names, xkbExtensionDeviceNotify *ed,
                            XkbChangesPtr changes, XkbEventCausePtr cause);
void XkbApplyLedMapChanges(DeviceIntPtr dev, XkbSrvLedInfoPtr sli,
                           unsigned int changed_maps, xkbExtensionDeviceNotify *ed,
                           XkbChangesPtr changes, XkbEventCausePtr cause);
void XkbApplyLedStateChanges(DeviceIntPtr dev, XkbSrvLedInfoPtr sli,
                             unsigned int changed_leds,
                             xkbExtensionDeviceNotify *ed,
                             XkbChangesPtr changes, XkbEventCausePtr cause);
void XkbFlushLedEvents(DeviceIntPtr dev, DeviceIntPtr kbd, XkbSrvLedInfoPtr sli,
                       xkbExtensionDeviceNotify *ed, XkbChangesPtr changes,
                       XkbEventCausePtr cause);

/* XkbDDX* functions */
unsigned int XkbDDXLoadKeymapByNames(DeviceIntPtr keybd,
                                     XkbComponentNamesPtr names,
                                     unsigned int want,
                                     unsigned int need,
                                     XkbDescPtr *finfoRtrn,
                                     char *keymapNameRtrn,
                                     int keymapNameRtrnLen);
Bool XkbDDXNamesFromRules(DeviceIntPtr keybd, const char *rules,
                          XkbRF_VarDefsPtr defs, XkbComponentNamesPtr names);
int XkbDDXUsesSoftRepeat(DeviceIntPtr dev);
void XkbDDXKeybdCtrlProc(DeviceIntPtr dev, KeybdCtrl *ctrl);
void XkbDDXUpdateDeviceIndicators(DeviceIntPtr dev, XkbSrvLedInfoPtr sli,
                                  CARD32 newState);
#endif /* _XSERVER_XKBSRV_PRIV_H_ */
