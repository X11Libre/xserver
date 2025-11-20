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

#ifndef INPUT_H
#define INPUT_H

#include "misc.h"
#include "screenint.h"
#include <X11/Xmd.h>
#include <X11/Xproto.h>
#include <stdint.h>
#include "window.h"             /* for WindowPtr */
#include "xkbrules.h"
#include "events.h"
#include "list.h"
#include "os.h"
#include <X11/extensions/XI2.h>

#define DEFAULT_KEYBOARD_CLICK 	0
#define DEFAULT_BELL		50
#define DEFAULT_BELL_PITCH	400
#define DEFAULT_BELL_DURATION	100
#define DEFAULT_AUTOREPEAT	TRUE
#define DEFAULT_AUTOREPEATS	{\
        0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,\
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,\
        0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

#define DEFAULT_LEDS		0x0     /* all off */
#define DEFAULT_LEDS_MASK	0xffffffff      /* 32 */
#define DEFAULT_INT_RESOLUTION		1000
#define DEFAULT_INT_MIN_VALUE		0
#define DEFAULT_INT_MAX_VALUE		100
#define DEFAULT_INT_DISPLAYED		0

#define DEFAULT_PTR_NUMERATOR	2
#define DEFAULT_PTR_DENOMINATOR	1
#define DEFAULT_PTR_THRESHOLD	4

#define DEVICE_INIT	0
#define DEVICE_ON	1
#define DEVICE_OFF	2
#define DEVICE_CLOSE	3
#define DEVICE_ABORT	4

#define POINTER_RELATIVE	(1 << 1)
#define POINTER_ABSOLUTE	(1 << 2)
#define POINTER_ACCELERATE	(1 << 3)
#define POINTER_SCREEN		(1 << 4)        /* Data in screen coordinates */
#define POINTER_NORAW		(1 << 5)        /* Don't generate RawEvents */
#define POINTER_EMULATED	(1 << 6)        /* Event was emulated from another event */
#define POINTER_DESKTOP		(1 << 7)        /* Data in desktop coordinates */
#define POINTER_RAWONLY         (1 << 8)        /* Only generate RawEvents */

/* GetTouchEvent flags */
#define TOUCH_ACCEPT            (1 << 0)
#define TOUCH_REJECT            (1 << 1)
#define TOUCH_PENDING_END       (1 << 2)
#define TOUCH_CLIENT_ID         (1 << 3)        /* touch ID is the client-visible id */
#define TOUCH_REPLAYING         (1 << 4)        /* event is being replayed */
#define TOUCH_POINTER_EMULATED  (1 << 5)        /* touch event may be pointer emulated */
#define TOUCH_END               (1 << 6)        /* really end this touch now */

/* GetGestureEvent flags */
#define GESTURE_CANCELLED       (1 << 0)

/*int constants for pointer acceleration schemes*/
#define PtrAccelNoOp            0
#define PtrAccelPredictable     1
#define PtrAccelLightweight     2
#define PtrAccelDefault         PtrAccelPredictable

#define MAX_VALUATORS 36
/* Maximum number of valuators, divided by six, rounded up, to get number
 * of events. */
#define MAX_VALUATOR_EVENTS 6
#define MAX_BUTTONS 256         /* completely arbitrarily chosen */

#define NO_AXIS_LIMITS -1

#define MAP_LENGTH	MAX_BUTTONS
#define DOWN_LENGTH	(MAX_BUTTONS/8)      /* 256/8 => number of bytes to hold 256 bits */
#define NullGrab ((GrabPtr)NULL)
#define PointerRootWin ((WindowPtr)PointerRoot)
#define NoneWin ((WindowPtr)None)
#define NullDevice ((DevicePtr)NULL)

#ifndef FollowKeyboard
#define FollowKeyboard 		3
#endif
#ifndef FollowKeyboardWin
#define FollowKeyboardWin  ((WindowPtr) FollowKeyboard)
#endif
#ifndef RevertToFollowKeyboard
#define RevertToFollowKeyboard	3
#endif

enum InputLevel {
    CORE = 1,
    XI = 2,
    XI2 = 3,
};

typedef unsigned long Leds;
typedef struct _OtherClients *OtherClientsPtr;
typedef struct _InputClients *InputClientsPtr;
typedef struct _DeviceIntRec *DeviceIntPtr;
typedef struct _ValuatorClassRec *ValuatorClassPtr;
typedef struct _ClassesRec *ClassesPtr;
typedef struct _SpriteRec *SpritePtr;
typedef struct _TouchClassRec *TouchClassPtr;
typedef struct _GestureClassRec *GestureClassPtr;
typedef struct _TouchPointInfo *TouchPointInfoPtr;
typedef struct _GestureInfo *GestureInfoPtr;
typedef struct _DDXTouchPointInfo *DDXTouchPointInfoPtr;
typedef union _GrabMask GrabMask;

typedef struct _ValuatorMask ValuatorMask;

/* The DIX stores incoming input events in this list */
extern InternalEvent *InputEventList;

typedef int (*DeviceProc) (DeviceIntPtr /*device */ ,
                           int /*what */ );

typedef void (*ProcessInputProc) (InternalEvent * /*event */ ,
                                  DeviceIntPtr /*device */ );

typedef Bool (*DeviceHandleProc) (DeviceIntPtr /*device */ ,
                                  void *        /*data */
    );

typedef void (*DeviceUnwrapProc) (DeviceIntPtr /*device */ ,
                                  DeviceHandleProc /*proc */ ,
                                  void *        /*data */
    );

/* pointer acceleration handling */
typedef void (*PointerAccelSchemeProc) (DeviceIntPtr /*device */ ,
                                        ValuatorMask * /*valuators */ ,
                                        CARD32 /*evtime */ );

typedef void (*DeviceCallbackProc) (DeviceIntPtr /*pDev */ );

struct _ValuatorAccelerationRec;
typedef Bool (*PointerAccelSchemeInitProc) (DeviceIntPtr /*dev */ ,
                                            struct _ValuatorAccelerationRec *
                                            /*protoScheme */ );

typedef void (*DeviceSendEventsProc) (DeviceIntPtr /*dev */ ,
                                      int /* event type */ ,
                                      int /* detail, buttons or keycode */ ,
                                      int /* flags */ ,
                                      const ValuatorMask * /* valuators */ );

typedef struct _DeviceRec {
    void *devicePrivate;
    ProcessInputProc processInputProc;  /* current */
    ProcessInputProc realInputProc;     /* deliver */
    ProcessInputProc enqueueInputProc;  /* enqueue */
    Bool on;                    /* used by DDX to keep state */
} DeviceRec, *DevicePtr;

typedef struct {
    int click, bell, bell_pitch, bell_duration;
    Bool autoRepeat;
    unsigned char autoRepeats[32];
    Leds leds;
    unsigned char id;
} KeybdCtrl;

typedef struct {
    KeySym *map;
    KeyCode minKeyCode, maxKeyCode;
    int mapWidth;
} KeySymsRec, *KeySymsPtr;

typedef struct {
    int num, den, threshold;
    unsigned char id;
} PtrCtrl;

typedef struct {
    int resolution, min_value, max_value;
    int integer_displayed;
    unsigned char id;
} IntegerCtrl;

typedef struct {
    int max_symbols, num_symbols_supported;
    int num_symbols_displayed;
    KeySym *symbols_supported;
    KeySym *symbols_displayed;
    unsigned char id;
} StringCtrl;

typedef struct {
    int percent, pitch, duration;
    unsigned char id;
} BellCtrl;

typedef struct {
    Leds led_values;
    Mask led_mask;
    unsigned char id;
} LedCtrl;

extern _X_EXPORT KeybdCtrl defaultKeyboardControl;
extern _X_EXPORT PtrCtrl defaultPointerControl;

typedef struct _InputOption InputOption;
typedef struct _XI2Mask XI2Mask;

typedef struct _InputAttributes {
    char *product;
    char *vendor;
    char *device;
    char *pnp_id;
    char *usb_id;
    char **tags;                /* null-terminated */
    uint32_t flags;
} InputAttributes;

#define ATTR_KEYBOARD (1<<0)
#define ATTR_POINTER (1<<1)
#define ATTR_JOYSTICK (1<<2)
#define ATTR_TABLET (1<<3)
#define ATTR_TOUCHPAD (1<<4)
#define ATTR_TOUCHSCREEN (1<<5)
#define ATTR_KEY (1<<6)
#define ATTR_TABLET_PAD (1<<7)

/* Key/Button has been run through all input processing and events sent to clients. */
#define KEY_PROCESSED 1
#define BUTTON_PROCESSED 1
/* Key/Button has not been fully processed, no events have been sent. */
#define KEY_POSTED 2
#define BUTTON_POSTED 2

void set_key_down(DeviceIntPtr pDev, int key_code, int type);
void set_key_up(DeviceIntPtr pDev, int key_code, int type);
int key_is_down(DeviceIntPtr pDev, int key_code, int type);
void set_button_down(DeviceIntPtr pDev, int button, int type);
void set_button_up(DeviceIntPtr pDev, int button, int type);
int button_is_down(DeviceIntPtr pDev, int button, int type);

DeviceIntPtr AddInputDevice(ClientPtr /*client */ ,
                                             DeviceProc /*deviceProc */ ,
                                             Bool /*autoStart */ );

Bool EnableDevice(DeviceIntPtr /*device */ ,
                                   BOOL /* sendevent */ );

Bool ActivateDevice(DeviceIntPtr /*device */ ,
                                     BOOL /* sendevent */ );

Bool DisableDevice(DeviceIntPtr /*device */ ,
                                    BOOL /* sendevent */ );

int RemoveDevice(DeviceIntPtr /*dev */ ,
                                  BOOL /* sendevent */ );

int NumMotionEvents(void);

int dixLookupDevice(DeviceIntPtr * /* dev */ ,
                                     int /* id */ ,
                                     ClientPtr /* client */ ,
                                     Mask /* access_mode */ );

void QueryMinMaxKeyCodes(KeyCode * /*minCode */ ,
                                          KeyCode * /*maxCode */ );

Bool InitButtonClassDeviceStruct(DeviceIntPtr /*device */ ,
                                                  int /*numButtons */ ,
                                                  Atom * /* labels */ ,
                                                  CARD8 * /*map */ );

extern _X_EXPORT Bool InitValuatorClassDeviceStruct(DeviceIntPtr /*device */ ,
                                                    int /*numAxes */ ,
                                                    Atom * /* labels */ ,
                                                    int /*numMotionEvents */ ,
                                                    int /*mode */ );

Bool InitPointerAccelerationScheme(DeviceIntPtr /*dev */ ,
                                                    int /*scheme */ );

extern _X_EXPORT Bool InitFocusClassDeviceStruct(DeviceIntPtr /*device */ );

extern _X_EXPORT Bool InitTouchClassDeviceStruct(DeviceIntPtr /*device */ ,
                                                 unsigned int /*max_touches */ ,
                                                 unsigned int /*mode */ ,
                                                 unsigned int /*numAxes */ );

extern _X_EXPORT Bool InitGestureClassDeviceStruct(DeviceIntPtr device,
                                                   unsigned int max_touches);

typedef void (*BellProcPtr) (int percent,
                             DeviceIntPtr device,
                             void *ctrl,
                             int feedbackClass);

typedef void (*KbdCtrlProcPtr) (DeviceIntPtr /*device */ ,
                                KeybdCtrl * /*ctrl */ );

typedef void (*PtrCtrlProcPtr) (DeviceIntPtr /*device */ ,
                                PtrCtrl * /*ctrl */ );

extern _X_EXPORT Bool InitPtrFeedbackClassDeviceStruct(DeviceIntPtr /*device */
                                                       ,
                                                       PtrCtrlProcPtr
                                                       /*controlProc */ );

typedef void (*StringCtrlProcPtr) (DeviceIntPtr /*device */ ,
                                   StringCtrl * /*ctrl */ );

Bool InitStringFeedbackClassDeviceStruct(DeviceIntPtr
                                                          /*device */ ,
                                                          StringCtrlProcPtr
                                                          /*controlProc */ ,
                                                          int /*max_symbols */ ,
                                                          int
                                                          /*num_symbols_supported */
                                                          ,
                                                          KeySym * /*symbols */
                                                          );

typedef void (*BellCtrlProcPtr) (DeviceIntPtr /*device */ ,
                                 BellCtrl * /*ctrl */ );

Bool InitBellFeedbackClassDeviceStruct(DeviceIntPtr /*device */
                                                        ,
                                                        BellProcPtr
                                                        /*bellProc */ ,
                                                        BellCtrlProcPtr
                                                        /*controlProc */ );

typedef void (*LedCtrlProcPtr) (DeviceIntPtr /*device */ ,
                                LedCtrl * /*ctrl */ );

extern _X_EXPORT Bool InitLedFeedbackClassDeviceStruct(DeviceIntPtr /*device */
                                                       ,
                                                       LedCtrlProcPtr
                                                       /*controlProc */ );

typedef void (*IntegerCtrlProcPtr) (DeviceIntPtr /*device */ ,
                                    IntegerCtrl * /*ctrl */ );

Bool InitIntegerFeedbackClassDeviceStruct(DeviceIntPtr
                                                           /*device */ ,
                                                           IntegerCtrlProcPtr
                                                           /*controlProc */ );

extern _X_EXPORT Bool InitPointerDeviceStruct(DevicePtr /*device */ ,
                                              CARD8 * /*map */ ,
                                              int /*numButtons */ ,
                                              Atom * /* btn_labels */ ,
                                              PtrCtrlProcPtr /*controlProc */ ,
                                              int /*numMotionEvents */ ,
                                              int /*numAxes */ ,
                                              Atom * /* axes_labels */ );

extern _X_EXPORT Bool InitKeyboardDeviceStruct(DeviceIntPtr /*device */ ,
                                               XkbRMLVOSet * /*rmlvo */ ,
                                               BellProcPtr /*bellProc */ ,
                                               KbdCtrlProcPtr /*controlProc */
                                               );

Bool InitKeyboardDeviceStructFromString(DeviceIntPtr dev,
							 const char *keymap,
							 int keymap_length,
							 BellProcPtr bell_func,
							 KbdCtrlProcPtr ctrl_func);

void ProcessInputEvents(void);

void InitInput(int /*argc */ ,
                                char ** /*argv */ );
void CloseInput(void);

int GetMaximumEventsNum(void);

InternalEvent *InitEventList(int num_events);
void FreeEventList(InternalEvent *list, int num_events);

int GetPointerEvents(InternalEvent *events,
                                      DeviceIntPtr pDev,
                                      int type,
                                      int buttons,
                                      int flags, const ValuatorMask *mask);

void QueuePointerEvents(DeviceIntPtr pDev,
                                         int type,
                                         int buttons,
                                         int flags, const ValuatorMask *mask);

int GetKeyboardEvents(InternalEvent *events,
                                       DeviceIntPtr pDev,
                                       int type,
                                       int key_code);

void QueueKeyboardEvents(DeviceIntPtr pDev,
                                          int type,
                                          int key_code);

int GetProximityEvents(InternalEvent *events,
                                        DeviceIntPtr pDev,
                                        int type, const ValuatorMask *mask);

extern _X_EXPORT void QueueProximityEvents(DeviceIntPtr pDev,
                                           int type, const ValuatorMask *mask);

extern _X_EXPORT int GetMotionHistorySize(void);

void AllocateMotionHistory(DeviceIntPtr pDev);

extern _X_EXPORT int GetMotionHistory(DeviceIntPtr pDev,
                                      xTimecoord ** buff,
                                      unsigned long start,
                                      unsigned long stop,
                                      ScreenPtr pScreen, BOOL core);

DeviceIntPtr GetPairedDevice(DeviceIntPtr kbd);
DeviceIntPtr GetMaster(DeviceIntPtr dev, int type);

int AllocDevicePair(ClientPtr client,
                                     const char *name,
                                     DeviceIntPtr *ptr,
                                     DeviceIntPtr *keybd,
                                     DeviceProc ptr_proc,
                                     DeviceProc keybd_proc, Bool master);

/* Helper functions. */
int generate_modkeymap(ClientPtr client, DeviceIntPtr dev,
                                        KeyCode **modkeymap,
                                        int *max_keys_per_mod);

enum TouchListenerState {
    TOUCH_LISTENER_AWAITING_BEGIN = 0, /**< Waiting for a TouchBegin event */
    TOUCH_LISTENER_AWAITING_OWNER,     /**< Waiting for a TouchOwnership event */
    TOUCH_LISTENER_EARLY_ACCEPT,       /**< Waiting for ownership, has already
                                            accepted */
    TOUCH_LISTENER_IS_OWNER,           /**< Is the current owner, hasn't
                                            accepted */
    TOUCH_LISTENER_HAS_ACCEPTED,       /**< Is the current owner, has accepted */
    TOUCH_LISTENER_HAS_END,            /**< Has already received the end event */
};

enum TouchListenerType {
    TOUCH_LISTENER_GRAB,
    TOUCH_LISTENER_POINTER_GRAB,
    TOUCH_LISTENER_REGULAR,
    TOUCH_LISTENER_POINTER_REGULAR,
};

enum GestureListenerType {
    GESTURE_LISTENER_GRAB,
    GESTURE_LISTENER_NONGESTURE_GRAB,
    GESTURE_LISTENER_REGULAR
};

extern _X_EXPORT InputAttributes *DuplicateInputAttributes(InputAttributes *
                                                           attrs);

extern _X_EXPORT void FreeInputAttributes(InputAttributes * attrs);

/* Implemented by the DDX. */
extern _X_EXPORT int NewInputDeviceRequest(InputOption *options,
                                           InputAttributes * attrs,
                                           DeviceIntPtr *dev);
void DeleteInputDeviceRequest(DeviceIntPtr dev);
void RemoveInputDeviceTraces(const char *config_info);
void DDXRingBell(int volume, int pitch, int duration);
extern _X_EXPORT ValuatorMask *valuator_mask_new(int num_valuators);
extern _X_EXPORT void valuator_mask_free(ValuatorMask **mask);
void valuator_mask_set_range(ValuatorMask *mask,
                                              int first_valuator,
                                              int num_valuators,
                                              const int *valuators);
extern _X_EXPORT void valuator_mask_set(ValuatorMask *mask, int valuator,
                                        int data);
extern _X_EXPORT void valuator_mask_set_double(ValuatorMask *mask, int valuator,
                                               double data);
extern _X_EXPORT void valuator_mask_zero(ValuatorMask *mask);
extern _X_EXPORT int valuator_mask_size(const ValuatorMask *mask);
extern _X_EXPORT int valuator_mask_isset(const ValuatorMask *mask, int bit);
extern _X_EXPORT void valuator_mask_unset(ValuatorMask *mask, int bit);
extern _X_EXPORT int valuator_mask_num_valuators(const ValuatorMask *mask);
extern _X_EXPORT void valuator_mask_copy(ValuatorMask *dest,
                                         const ValuatorMask *src);
extern _X_EXPORT int valuator_mask_get(const ValuatorMask *mask, int valnum);
extern _X_EXPORT double valuator_mask_get_double(const ValuatorMask *mask,
                                                 int valnum);
extern _X_EXPORT Bool valuator_mask_fetch(const ValuatorMask *mask,
                                          int valnum, int *val);
Bool valuator_mask_fetch_double(const ValuatorMask *mask,
                                                 int valnum, double *val);
Bool valuator_mask_has_unaccelerated(const ValuatorMask *mask);
extern _X_EXPORT void valuator_mask_set_unaccelerated(ValuatorMask *mask,
                                                      int valuator,
                                                      double accel,
                                                      double unaccel);
void valuator_mask_set_absolute_unaccelerated(ValuatorMask *mask,
                                                               int valuator,
                                                               int absolute,
                                                               double unaccel);
double valuator_mask_get_accelerated(const ValuatorMask *mask,
                                                      int valuator);
double valuator_mask_get_unaccelerated(const ValuatorMask *mask,
                                                        int valuator);
Bool valuator_mask_fetch_unaccelerated(const ValuatorMask *mask,
                                                        int valuator,
                                                        double *accel,
                                                        double *unaccel);
/* InputOption handling interface */
extern _X_EXPORT InputOption *input_option_new(InputOption *list,
                                               const char *key,
                                               const char *value);
extern _X_EXPORT void input_option_free_list(InputOption **opt);
InputOption *input_option_free_element(InputOption *opt,
                                                        const char *key);
InputOption *input_option_find(InputOption *list,
                                                const char *key);
const char *input_option_get_key(const InputOption *opt);
const char *input_option_get_value(const InputOption *opt);
void input_option_set_key(InputOption *opt, const char *key);
void input_option_set_value(InputOption *opt,
                                             const char *value);

extern _X_EXPORT void input_lock(void);
extern _X_EXPORT void input_unlock(void);
void input_force_unlock(void);
int in_input_thread(void);

extern Bool InputThreadEnable;

#endif                          /* INPUT_H */
