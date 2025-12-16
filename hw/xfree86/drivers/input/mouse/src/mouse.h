/*
 * Copyright (c) 1999-2003 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

/* Public interface to OS-specific mouse support. */

#ifndef _XF86OSMOUSE_H_
#define _XF86OSMOUSE_H_

#include <X11/Xfuncproto.h>

#ifndef _X_FALLTHROUGH /* xproto < 7.0.34 */
# if  __has_attribute(fallthrough)
#  define _X_FALLTHROUGH __attribute__((fallthrough))
# else
#  define _X_FALLTHROUGH (void)0
# endif
#endif

#include "xf86Xinput.h"

/* Mouse interface classes */
#define MSE_NONE        0x00
#define MSE_SERIAL      0x01            /* serial port */
#define MSE_BUS         0x02            /* old bus mouse */
#define MSE_PS2         0x04            /* standard read-only PS/2 */
#define MSE_XPS2        0x08            /* extended PS/2 */
#define MSE_AUTO        0x10            /* auto-detect (PnP) */
#define MSE_MISC        0x20            /* The OS layer will identify the
                                         * specific protocol names that are
                                         * supported for this class. */

/* Mouse Protocol IDs. */
typedef enum {
    PROT_UNKNOWN = -2,
    PROT_UNSUP = -1,            /* protocol is not supported */
    PROT_MS = 0,
    PROT_MSC,
    PROT_MM,
    PROT_LOGI,
    PROT_LOGIMAN,
    PROT_MMHIT,
    PROT_GLIDE,
    PROT_IMSERIAL,
    PROT_THINKING,
    PROT_ACECAD,
    PROT_VALUMOUSESCROLL,
    PROT_PS2,
    PROT_GENPS2,
    PROT_IMPS2,
    PROT_EXPPS2,
    PROT_THINKPS2,
    PROT_MMPS2,
    PROT_GLIDEPS2,
    PROT_NETPS2,
    PROT_NETSCPS2,
    PROT_BM,
    PROT_AUTO,
    PROT_SYSMOUSE,
    PROT_WSMOUSE,
    PROT_VUID,
    PROT_NUMPROTOS      /* This must always be last. */
} MouseProtocolID;

struct _MouseDevRec;

typedef int (*GetInterfaceTypesProc)(void);
typedef const char **(*BuiltinNamesProc)(void);
typedef Bool (*CheckProtocolProc)(const char *protocol);
typedef Bool (*BuiltinPreInitProc)(InputInfoPtr pInfo, const char *protocol,
                                   int flags);
typedef const char *(*DefaultProtocolProc)(void);
typedef const char *(*SetupAutoProc)(InputInfoPtr pInfo, int *protoPara);
typedef void (*SetResProc)(InputInfoPtr pInfo, const char* protocol, int rate,
                           int res);
typedef const char *(*FindDeviceProc)(InputInfoPtr pInfo, const char *protocol,
                                      int flags);
typedef const char *(*GuessProtocolProc)(InputInfoPtr pInfo, int flags);

/*
 * OSMouseInfoRec is used to pass information from the OSMouse layer to the
 * OS-independent mouse driver.
 */
typedef struct {
        GetInterfaceTypesProc   SupportedInterfaces;
        BuiltinNamesProc        BuiltinNames;
        CheckProtocolProc       CheckProtocol;
        BuiltinPreInitProc      PreInit;
        DefaultProtocolProc     DefaultProtocol;
        SetupAutoProc           SetupAuto;
        SetResProc              SetPS2Res;
        SetResProc              SetBMRes;
        SetResProc              SetMiscRes;
        FindDeviceProc          FindDevice;
        GuessProtocolProc       GuessProtocol;
} OSMouseInfoRec, *OSMouseInfoPtr;

/*
 * SupportedInterfaces: Returns the mouse interface types that the OS support.
 *              If MSE_MISC is returned, then the BuiltinNames and
 *              CheckProtocol should be set.
 *
 * BuiltinNames: Returns the names of the protocols that are fully handled
 *              in the OS-specific code.  These are names that don't appear
 *              directly in the main "mouse" driver.
 *
 * CheckProtocol: Checks if the protocol name given is supported by the
 *              OS.  It should return TRUE for both "builtin" protocols and
 *              protocols of type MSE_MISC that are supported by the OS.
 *
 * PreInit:     The PreInit function for protocols that are builtin.  This
 *              function is passed the protocol name.
 *
 * DefaultProtocol: Returns the name of a default protocol that should be used
 *              for the OS when none has been supplied in the config file.
 *              This should only be set when there is a reasonable default.
 *
 * SetupAuto:   This function can be used to do OS-specific protocol
 *              auto-detection.  It returns the name of the detected protocol,
 *              or NULL when detection fails.  It may also adjust one or more
 *              of the "protoPara" values for the detected protocol by setting
 *              then to something other than -1.  SetupAuto gets called in two
 *              ways.  The first is before any devices have been opened.  This
 *              can be used when the protocol "Auto" always maps to a single
 *              protocol type.  The second is with the device open, allowing
 *              OS-specific probing to be done.
 *
 * SetPS2Res:   Set the resolution and sample rate for MSE_PS2 and MSE_XPS2
 *              protocol types.
 *
 * SetBMRes:    Set the resolution and sample rate for MSE_BM protocol types.
 *
 * SetMiscRes:  Set the resolution and sample rate for MSE_MISC protocol types.
 *
 * FindDevice:  This function gets called when no Device has been specified
 *              in the config file.  OS-specific methods may be used to guess
 *              which input device to use.  This function is called after the
 *              pre-open attempts at protocol discovery are done, but before
 *              the device is open.  I.e., after the first SetupAuto() call,
 *              after the DefaultProtocol() call, but before the PreInit()
 *              call.  Available protocol information may be used in locating
 *              the default input device.
 *
 * GuessProtocol: A last resort attempt at guessing the mouse protocol by
 *              whatever OS-specific means might be available.  OS-independent
 *              things should be in the mouse driver.  This function gets
 *              called after the mouse driver's OS-independent methods have
 *              failed.
 */

extern OSMouseInfoPtr OSMouseInit(int flags);

/* Z axis mapping */
#define MSE_NOZMAP      0
#define MSE_MAPTOX      -1
#define MSE_MAPTOY      -2
#define MSE_MAPTOZ      -3
#define MSE_MAPTOW      -4

/* Generalize for other axes. */
#define MSE_NOAXISMAP   MSE_NOZMAP

#define MSE_MAXBUTTONS  24
#define MSE_DFLTBUTTONS  3

/*
 * Mouse device record.  This is shared by the mouse driver and the OSMouse
 * layer.
 */

typedef void (*checkMovementsProc)(InputInfoPtr,int, int);
typedef void (*autoProbeProc)(InputInfoPtr, Bool, Bool);
typedef Bool (*collectDataProc)(struct _MouseDevRec *, unsigned char);
typedef Bool (*dataGoodProc)(struct _MouseDevRec *);

typedef void (*PostMseEventProc)(InputInfoPtr pInfo, int buttons,
                              int dx, int dy, int dz, int dw);
typedef void (*MouseCommonOptProc)(InputInfoPtr pInfo);

typedef struct _MouseDevRec {
    PtrCtrlProcPtr      Ctrl;
    PostMseEventProc    PostEvent;
    MouseCommonOptProc  CommonOptions;
    DeviceIntPtr        device;
    const char *        protocol;
    MouseProtocolID     protocolID;
    MouseProtocolID     oldProtocolID; /* hack */
    int                 class;
    int                 mseModel;
    int                 baudRate;
    int                 oldBaudRate;
    int                 sampleRate;
    int                 lastButtons;
    int                 buttons;        /* # of buttons */
    int                 emulateState;   /* automata state for 2 button mode */
    Bool                emulate3Buttons;
    Bool                emulate3ButtonsSoft;
    int                 emulate3Timeout;/* Timeout for 3 button emulation */
    Bool                chordMiddle;
    Bool                flipXY;
    int                 invX;
    int                 invY;
    int                 resolution;
    int                 negativeZ;      /* button mask */
    int                 positiveZ;      /* button mask */
    int                 negativeW;      /* button mask */
    int                 positiveW;      /* button mask */
    void               *buffer;         /* usually an XISBuffer* */
    int                 protoBufTail;
    unsigned char       protoBuf[8];
    unsigned char       protoPara[8];
    unsigned char       inSync;         /* driver in sync with datastream */
    void               *mousePriv;      /* private area */
    InputInfoPtr        pInfo;
    Bool                emulate3Pending;/* timer waiting */
    CARD32              emulate3Expires;/* time to fire emulation code */
    Bool                emulateWheel;
    int                 wheelInertia;
    int                 wheelButton;
    int                 negativeX;      /* Button values.  Unlike the Z and */
    int                 positiveX;      /* W equivalents, these are button  */
    int                 negativeY;      /* values rather than button masks. */
    int                 positiveY;
    int                 wheelYDistance;
    int                 wheelXDistance;
    Bool                autoProbe;
    checkMovementsProc  checkMovements;
    autoProbeProc       autoProbeMouse;
    collectDataProc     collectData;
    dataGoodProc        dataGood;
    int                 angleOffset;
    void               *pDragLock;      /* drag lock area */
    int                 xisbscale;      /* buffer size for 1 event */
    int                 wheelButtonTimeout;/* Timeout for the wheel button emulation */
    CARD32              wheelButtonExpires;
    int                 doubleClickSourceButtonMask;
    int                 doubleClickTargetButton;
    int                 doubleClickTargetButtonMask;
    int                 doubleClickOldSourceState;
    int                 lastMappedButtons;
    int                 buttonMap[MSE_MAXBUTTONS];
} MouseDevRec, *MouseDevPtr;

#endif /* _XF86OSMOUSE_H_ */
