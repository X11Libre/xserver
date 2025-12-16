
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

#include <xorg-server.h>

#include <errno.h>
#include <sys/stat.h>
#include <X11/X.h>

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86Xinput.h"
#include "mouse.h"
#include "xisb.h"
#include "mipointer.h"
#ifdef WSCONS_SUPPORT
#include <dev/wscons/wsconsio.h>
#endif
#ifdef USBMOUSE_SUPPORT
#ifdef HAS_LIB_USB_HID
#include <usbhid.h>
#else
#include "usb.h"
#endif

#ifdef __DragonFly__
# include <bus/u4b/usb.h>
#else
# include <dev/usb/usb.h>
#endif
#ifdef USB_GET_REPORT_ID
#define USB_NEW_HID
#endif

#define HUP_GENERIC_DESKTOP     0x0001
#define HUP_BUTTON              0x0009

#define HUG_X                   0x0030
#define HUG_Y                   0x0031
#define HUG_Z                   0x0032
#define HUG_WHEEL               0x0038

#define HID_USAGE2(p,u) (((p) << 16) | u)

/* The UMS mice have middle button as number 3 */
#define UMS_BUT(i) ((i) == 0 ? 2 : (i) == 1 ? 0 : (i) == 2 ? 1 : (i))
#endif /* USBMOUSE_SUPPORT */

#ifdef USBMOUSE_SUPPORT
static void usbSigioReadInput (int fd, void *closure);
#endif
static const char *FindDevice(InputInfoPtr, const char *, int);

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
/* These are for FreeBSD and DragonFly */
#define DEFAULT_MOUSE_DEV               "/dev/mouse"
#define DEFAULT_SYSMOUSE_DEV            "/dev/sysmouse"
#define DEFAULT_PS2_DEV                 "/dev/psm0"

static const char *mouseDevs[] = {
        DEFAULT_MOUSE_DEV,
        DEFAULT_SYSMOUSE_DEV,
        DEFAULT_PS2_DEV,
        NULL
};
#elif (defined(__OpenBSD__) || defined(__NetBSD__)) && defined(WSCONS_SUPPORT)
/* Only wsmouse mice are autoconfigured for now on OpenBSD */
#define DEFAULT_WSMOUSE_DEV             "/dev/wsmouse"
#define DEFAULT_WSMOUSE0_DEV            "/dev/wsmouse0"

static const char *mouseDevs[] = {
        DEFAULT_WSMOUSE_DEV,
        DEFAULT_WSMOUSE0_DEV,
        NULL
};
#endif

static int
SupportedInterfaces(void)
{
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__) || defined(__NetBSD__)
    return MSE_SERIAL | MSE_BUS | MSE_PS2 | MSE_AUTO | MSE_MISC;
#else
    return MSE_SERIAL | MSE_BUS | MSE_PS2 | MSE_XPS2 | MSE_AUTO | MSE_MISC;
#endif
}

/* Names of protocols that are handled internally here. */
static const char *internalNames[] = {
#if defined(WSCONS_SUPPORT)
        "WSMouse",
#endif
#if defined(USBMOUSE_SUPPORT)
        "usb",
#endif
        NULL
};

/*
 * Names of MSC_MISC protocols that the OS supports.  These are decoded by
 * main "mouse" driver.
 */
static const char *miscNames[] = {
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
        "SysMouse",
#endif
        NULL
};

static const char **
BuiltinNames(void)
{
    return internalNames;
}

static Bool
CheckProtocol(const char *protocol)
{
    int i;

    for (i = 0; internalNames[i]; i++)
        if (xf86NameCmp(protocol, internalNames[i]) == 0)
            return TRUE;
    for (i = 0; miscNames[i]; i++)
        if (xf86NameCmp(protocol, miscNames[i]) == 0)
            return TRUE;
    return FALSE;
}

static const char *
DefaultProtocol(void)
{
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
    return "Auto";
#elif (defined(__OpenBSD__) || defined(__NetBSD__)) && defined(WSCONS_SUPPORT)
    return "WSMouse";
#else
    return NULL;
#endif
}

#if (defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)) && defined(MOUSE_PROTO_SYSMOUSE)
static struct {
        int dproto;
        const char *name;
} devproto[] = {
        { MOUSE_PROTO_MS,               "Microsoft" },
        { MOUSE_PROTO_MSC,              "MouseSystems" },
        { MOUSE_PROTO_LOGI,             "Logitech" },
        { MOUSE_PROTO_MM,               "MMSeries" },
        { MOUSE_PROTO_LOGIMOUSEMAN,     "MouseMan" },
        { MOUSE_PROTO_BUS,              "BusMouse" },
        { MOUSE_PROTO_INPORT,           "BusMouse" },
        { MOUSE_PROTO_PS2,              "PS/2" },
        { MOUSE_PROTO_HITTAB,           "MMHitTab" },
        { MOUSE_PROTO_GLIDEPOINT,       "GlidePoint" },
        { MOUSE_PROTO_INTELLI,          "Intellimouse" },
        { MOUSE_PROTO_THINK,            "ThinkingMouse" },
        { MOUSE_PROTO_SYSMOUSE,         "SysMouse" }
};

static const char *
SetupAuto(InputInfoPtr pInfo, int *protoPara)
{
    int i;
    mousehw_t hw;
    mousemode_t mode;

    if (pInfo->fd == -1)
        return NULL;

    /* set the driver operation level, if applicable */
    i = 1;
    ioctl(pInfo->fd, MOUSE_SETLEVEL, &i);

    /* interrogate the driver and get some intelligence on the device. */
    hw.iftype = MOUSE_IF_UNKNOWN;
    hw.model = MOUSE_MODEL_GENERIC;
    ioctl(pInfo->fd, MOUSE_GETHWINFO, &hw);
    xf86MsgVerb(X_INFO, 3, "%s: SetupAuto: hw.iftype is %d, hw.model is %d\n",
                pInfo->name, hw.iftype, hw.model);
    if (ioctl(pInfo->fd, MOUSE_GETMODE, &mode) == 0) {
        for (i = 0; i < sizeof(devproto)/sizeof(devproto[0]); ++i) {
            if (mode.protocol == devproto[i].dproto) {
                /* override some parameters */
                if (protoPara) {
                    protoPara[4] = mode.packetsize;
                    protoPara[0] = mode.syncmask[0];
                    protoPara[1] = mode.syncmask[1];
                }
                xf86MsgVerb(X_INFO, 3, "%s: SetupAuto: protocol is %s\n",
                            pInfo->name, devproto[i].name);
                return devproto[i].name;
            }
        }
    }
    return NULL;
}

static void
SetSysMouseRes(InputInfoPtr pInfo, const char *protocol, int rate, int res)
{
    mousemode_t mode;
    MouseDevPtr pMse;

    pMse = pInfo->private;

    mode.rate = rate > 0 ? rate : -1;
    mode.resolution = res > 0 ? res : -1;
    mode.accelfactor = -1;
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
    if (pMse->autoProbe ||
        (protocol && xf86NameCmp(protocol, "SysMouse") == 0)) {
        /*
         * As the FreeBSD sysmouse driver defaults to protocol level 0
         * every time it is opened we enforce protocol level 1 again at
         * this point.
         */
        mode.level = 1;
    } else
        mode.level = -1;
#else
    mode.level = -1;
#endif
    ioctl(pInfo->fd, MOUSE_SETMODE, &mode);
}
#endif

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)

#define MOUSED_PID_FILE "/var/run/moused.pid"

/*
 * Try to check if moused is running.  DEFAULT_SYSMOUSE_DEV is useless without
 * it.  There doesn't seem to be a better way of checking.
 */
static Bool
MousedRunning(void)
{
    FILE *f = NULL;
    unsigned int pid;

    if ((f = fopen(MOUSED_PID_FILE, "r")) != NULL) {
        if (fscanf(f, "%u", &pid) == 1 && pid > 0) {
            if (kill(pid, 0) == 0) {
                fclose(f);
                return TRUE;
            }
        }
        fclose(f);
    }
    return FALSE;
}

static const char *
FindDevice(InputInfoPtr pInfo, const char *protocol, int flags)
{
    int fd = -1;
    const char **pdev, *dev = NULL;
    Bool devMouse = FALSE;
    struct stat devMouseStat;
    struct stat sb;

    for (pdev = mouseDevs; *pdev; pdev++) {
        SYSCALL (fd = open(*pdev, O_RDWR | O_NONBLOCK));
        if (fd == -1) {
#ifdef DEBUG
            ErrorF("Cannot open %s (%s)\n", *pdev, strerror(errno));
#endif
        } else {
            /*
             * /dev/mouse is held until checks for matches with other devices
             * are done.  This is so that when it points to /dev/sysmouse,
             * the test for whether /dev/sysmouse is usable can be made.
             */
            if (!strcmp(*pdev, DEFAULT_MOUSE_DEV)) {
                if (fstat(fd, &devMouseStat) == 0)
                    devMouse = TRUE;
                close(fd);
                continue;
            } else if (!strcmp(*pdev, DEFAULT_SYSMOUSE_DEV)) {
                /* Check if /dev/mouse is the same as /dev/sysmouse. */
                if (devMouse && fstat(fd, &sb) == 0 &&
                    devMouseStat.st_dev == sb.st_dev &&
                    devMouseStat.st_ino == sb.st_ino) {
                    /* If the same, use /dev/sysmouse. */
                    devMouse = FALSE;
                }
                close(fd);
                if (MousedRunning())
                    break;
                else {
#ifdef DEBUG
                    ErrorF("moused isn't running\n");
#endif
                }
            } else {
                close(fd);
                break;
            }
        }
    }

    if (*pdev)
        dev = *pdev;
    else if (devMouse)
        dev = DEFAULT_MOUSE_DEV;

    if (dev) {
        /* Set the Device option. */
        pInfo->options =
            xf86AddNewOption(pInfo->options, "Device", dev);
        xf86Msg(X_INFO, "%s: Setting Device option to \"%s\"\n",
                pInfo->name, dev);
    }

    return *pdev;
}
#endif

#if (defined(__OpenBSD__) || defined(__NetBSD__)) && defined(WSCONS_SUPPORT)

/* Only support wsmouse configuration for now */
static const char *
SetupAuto(InputInfoPtr pInfo, int *protoPara)
{

    xf86MsgVerb(X_INFO, 3, "%s: SetupAuto: protocol is %s\n",
                pInfo->name, "wsmouse");
    return "wsmouse";
}

static void
SetMouseRes(InputInfoPtr pInfo, const char *protocol, int rate, int res)
{

    xf86MsgVerb(X_INFO, 3, "%s: SetMouseRes: protocol %s rate %d res %d\n",
            pInfo->name, protocol, rate, res);
}

static const char *
FindDevice(InputInfoPtr pInfo, const char *protocol, int flags)
{
    int fd = -1;
    const char **pdev;

    for (pdev = mouseDevs; *pdev; pdev++) {
        SYSCALL(fd = open(*pdev, O_RDWR | O_NONBLOCK));
        if (fd != -1) {
            /* Set the Device option. */
            pInfo->options =
                xf86AddNewOption(pInfo->options,
                                 "Device", *pdev);
            xf86Msg(X_INFO, "%s: found Device \"%s\"\n",
                    pInfo->name, *pdev);
            close(fd);
            break;
        }
    }
    return *pdev;
}
#endif /* __OpenBSD__ || __NetBSD__ && WSCONS_SUPPORT */

#ifdef WSCONS_SUPPORT
#define NUMEVENTS 64

static void
wsconsReadInput(InputInfoPtr pInfo)
{
    MouseDevPtr pMse;
    static struct wscons_event eventList[NUMEVENTS];
    int n, c;
    struct wscons_event *event = eventList;
    unsigned char *pBuf;

    pMse = pInfo->private;

    XisbBlockDuration(pMse->buffer, -1);
    pBuf = (unsigned char *)eventList;
    n = 0;
    while (n < sizeof(eventList) && (c = XisbRead(pMse->buffer)) >= 0) {
        pBuf[n++] = (unsigned char)c;
    }

    if (n == 0)
        return;

    n /= sizeof(struct wscons_event);
    while( n-- ) {
        int buttons = pMse->lastButtons;
        int dx = 0, dy = 0, dz = 0, dw = 0, x, y;
        switch (event->type) {
        case WSCONS_EVENT_MOUSE_UP:
#define BUTBIT (1 << (event->value <= 2 ? 2 - event->value : event->value))
            buttons &= ~BUTBIT;
            break;
        case WSCONS_EVENT_MOUSE_DOWN:
            buttons |= BUTBIT;
            break;
        case WSCONS_EVENT_MOUSE_DELTA_X:
            dx = event->value;
            break;
        case WSCONS_EVENT_MOUSE_DELTA_Y:
            dy = -event->value;
            break;
#ifdef WSCONS_EVENT_MOUSE_DELTA_Z
        case WSCONS_EVENT_MOUSE_DELTA_Z:
            dz = event->value;
            break;
#endif
#ifdef WSCONS_EVENT_MOUSE_DELTA_W
        case WSCONS_EVENT_MOUSE_DELTA_W:
            dw = event->value;
            break;
#endif
	case WSCONS_EVENT_MOUSE_ABSOLUTE_X:
	    x = event->value;
	    xf86PostMotionEvent(pInfo->dev, TRUE, 0, 1, x);
	    ++event;
	    continue;
	case WSCONS_EVENT_MOUSE_ABSOLUTE_Y:
	    y = event->value;
	    xf86PostMotionEvent(pInfo->dev, TRUE, 1, 1, y);
	    ++event;
	    continue;
#ifdef WSCONS_EVENT_MOUSE_ABSOLUTE_Z
	case WSCONS_EVENT_MOUSE_ABSOLUTE_Z:
	    ++event;
	    continue;
#endif
#ifdef WSCONS_EVENT_MOUSE_ABSOLUTE_W
	case WSCONS_EVENT_MOUSE_ABSOLUTE_W:
	    ++event;
	    continue;
#endif
        default:
            LogMessageVerbSigSafe(X_WARNING, -1,
                                  "%s: bad wsmouse event type=%d\n", pInfo->name,
                                  event->type);
            ++event;
            continue;
        }

        pMse->PostEvent(pInfo, buttons, dx, dy, dz, dw);
        ++event;
    }
    return;
}


/* This function is called when the protocol is "wsmouse". */
static Bool
wsconsPreInit(InputInfoPtr pInfo, const char *protocol, int flags)
{
    MouseDevPtr pMse = pInfo->private;

    /* Setup the local input proc. */
    pInfo->read_input = wsconsReadInput;
    pMse->xisbscale = sizeof(struct wscons_event);

    return TRUE;
}
#endif

#if defined(USBMOUSE_SUPPORT)

typedef struct _UsbMseRec {
    int packetSize;
    int iid;
    hid_item_t loc_x;           /* x locator item */
    hid_item_t loc_y;           /* y locator item */
    hid_item_t loc_z;           /* z (wheel) locator item */
    hid_item_t loc_w;           /* z (wheel) locator item */
    hid_item_t loc_btn[MSE_MAXBUTTONS]; /* buttons locator items */
   unsigned char *buffer;
} UsbMseRec, *UsbMsePtr;

static int
usbMouseProc(DeviceIntPtr pPointer, int what)
{
    InputInfoPtr pInfo;
    MouseDevPtr pMse;
    UsbMsePtr pUsbMse;
    unsigned char map[MSE_MAXBUTTONS + 1];
    int nbuttons;

    pInfo = pPointer->public.devicePrivate;
    pMse = pInfo->private;
    pMse->device = pPointer;
    pUsbMse = pMse->mousePriv;

    switch (what) {
    case DEVICE_INIT:
        pPointer->public.on = FALSE;

        for (nbuttons = 0; nbuttons < MSE_MAXBUTTONS; ++nbuttons)
            map[nbuttons + 1] = nbuttons + 1;

        InitPointerDeviceStruct((DevicePtr)pPointer,
                                map,
                                min(pMse->buttons, MSE_MAXBUTTONS),
                                miPointerGetMotionEvents,
                                pMse->Ctrl,
                                miPointerGetMotionBufferSize());

        /* X valuator */
        xf86InitValuatorAxisStruct(pPointer, 0, 0, -1, 1, 0, 1);
        xf86InitValuatorDefaults(pPointer, 0);
        /* Y valuator */
        xf86InitValuatorAxisStruct(pPointer, 1, 0, -1, 1, 0, 1);
        xf86InitValuatorDefaults(pPointer, 1);
        xf86MotionHistoryAllocate(pInfo);
        break;

    case DEVICE_ON:
        pInfo->fd = xf86OpenSerial(pInfo->options);
        if (pInfo->fd == -1)
            xf86Msg(X_WARNING, "%s: cannot open input device\n", pInfo->name);
        else {
            pMse->buffer = XisbNew(pInfo->fd, pUsbMse->packetSize);
            if (!pMse->buffer) {
                free(pMse);
                xf86CloseSerial(pInfo->fd);
                pInfo->fd = -1;
            } else {
                xf86FlushInput(pInfo->fd);
                if (!xf86InstallSIGIOHandler (pInfo->fd, usbSigioReadInput,
                                              pInfo))
                    AddEnabledDevice(pInfo->fd);
            }
        }
        pMse->lastButtons = 0;
        pMse->lastMappedButtons = 0;
        pMse->emulateState = 0;
        pPointer->public.on = TRUE;
        break;

    case DEVICE_OFF:
    case DEVICE_CLOSE:
        if (pInfo->fd != -1) {
            RemoveEnabledDevice(pInfo->fd);
            if (pUsbMse->packetSize > 8 && pUsbMse->buffer) {
                free(pUsbMse->buffer);
            }
            if (pMse->buffer) {
                XisbFree(pMse->buffer);
                pMse->buffer = NULL;
            }
            xf86CloseSerial(pInfo->fd);
            pInfo->fd = -1;
        }
        pPointer->public.on = FALSE;
        usleep(300000);
        break;

    default:
        return BadValue;
    }
    return Success;
}

static void
usbReadInput(InputInfoPtr pInfo)
{
    MouseDevPtr pMse;
    UsbMsePtr pUsbMse;
    int buttons = pMse->lastButtons;
    int dx = 0, dy = 0, dz = 0, dw = 0;
    int n, c;
    unsigned char *pBuf;

    pMse = pInfo->private;
    pUsbMse = pMse->mousePriv;

    XisbBlockDuration(pMse->buffer, -1);
    pBuf = pUsbMse->buffer;
    n = 0;
    while ((c = XisbRead(pMse->buffer)) >= 0 && n < pUsbMse->packetSize) {
        pBuf[n++] = (unsigned char)c;
    }
    if (n == 0)
        return;
    if (n != pUsbMse->packetSize) {
        LogMessageVerbSigSafe(X_WARNING, -1,
                              "%s: incomplete packet, size %d\n",
                              pInfo->name, n);
    }
    /* discard packets with an id that don't match the mouse */
    /* XXX this is probably not the right thing */
    if (pUsbMse->iid != 0) {
        if (*pBuf++ != pUsbMse->iid)
            return;
    }
    dx = hid_get_data(pBuf, &pUsbMse->loc_x);
    dy = hid_get_data(pBuf, &pUsbMse->loc_y);
    dz = hid_get_data(pBuf, &pUsbMse->loc_z);
    dw = hid_get_data(pBuf, &pUsbMse->loc_w);

    buttons = 0;
    for (n = 0; n < pMse->buttons; n++) {
        if (hid_get_data(pBuf, &pUsbMse->loc_btn[n]))
            buttons |= (1 << UMS_BUT(n));
    }
    pMse->PostEvent(pInfo, buttons, dx, dy, dz, dw);
    return;
}

static void
usbSigioReadInput (int fd, void *closure)
{
    usbReadInput ((InputInfoPtr) closure);
}

/* This function is called when the protocol is "usb". */
static Bool
usbPreInit(InputInfoPtr pInfo, const char *protocol, int flags)
{
    MouseDevPtr pMse = pInfo->private;
    UsbMsePtr pUsbMse;
    report_desc_t reportDesc;
    int i;

    pUsbMse = malloc(sizeof(UsbMseRec));
    if (pUsbMse == NULL) {
        xf86Msg(X_ERROR, "%s: cannot allocate UsbMouseRec\n", pInfo->name);
        free(pMse);
        return FALSE;
    }

    pMse->protocol = protocol;
    xf86Msg(X_CONFIG, "%s: Protocol: %s\n", pInfo->name, protocol);

    /* Collect the options, and process the common options. */
    xf86CollectInputOptions(pInfo, NULL);
    xf86ProcessCommonOptions(pInfo, pInfo->options);

    /* Check if the device can be opened. */
    pInfo->fd = xf86OpenSerial(pInfo->options);
    if (pInfo->fd == -1) {
        if (xf86GetAllowMouseOpenFail())
            xf86Msg(X_WARNING, "%s: cannot open input device\n", pInfo->name);
        else {
            xf86Msg(X_ERROR, "%s: cannot open input device\n", pInfo->name);
            free(pUsbMse);
            free(pMse);
            return FALSE;
        }
    }
    /* Get USB information */
    reportDesc = hid_get_report_desc(pInfo->fd);
    /* Get packet size & iid */
#ifdef USB_NEW_HID
    if (ioctl(pInfo->fd, USB_GET_REPORT_ID, &pUsbMse->iid) == -1) {
            xf86Msg(X_ERROR, "Error ioctl USB_GET_REPORT_ID on %s : %s\n",
                    pInfo->name, strerror(errno));
            return FALSE;
    }
    pUsbMse->packetSize = hid_report_size(reportDesc, hid_input,
                                              pUsbMse->iid);
#else
    pUsbMse->packetSize = hid_report_size(reportDesc, hid_input,
                                              &pUsbMse->iid);
#endif
    /* Allocate buffer */
    if (pUsbMse->packetSize <= 8) {
        pUsbMse->buffer = pMse->protoBuf;
    } else {
        pUsbMse->buffer = malloc(pUsbMse->packetSize);
    }
    if (pUsbMse->buffer == NULL) {
        xf86Msg(X_ERROR, "%s: cannot allocate buffer\n", pInfo->name);
        free(pUsbMse);
        free(pMse);
        xf86CloseSerial(pInfo->fd);
        return FALSE;
    }
#ifdef USB_NEW_HID
    if (hid_locate(reportDesc, HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_X),
                   hid_input, &pUsbMse->loc_x, pUsbMse->iid) < 0) {
        xf86Msg(X_WARNING, "%s: no x locator\n", pInfo->name);
    }
    if (hid_locate(reportDesc, HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_Y),
                   hid_input, &pUsbMse->loc_y, pUsbMse->iid) < 0) {
        xf86Msg(X_WARNING, "%s: no y locator\n", pInfo->name);
    }
    if (hid_locate(reportDesc, HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_WHEEL),
                   hid_input, &pUsbMse->loc_z, pUsbMse->iid) < 0) {
    }
#else
    if (hid_locate(reportDesc, HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_X),
                   hid_input, &pUsbMse->loc_x) < 0) {
        xf86Msg(X_WARNING, "%s: no x locator\n", pInfo->name);
    }
    if (hid_locate(reportDesc, HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_Y),
                   hid_input, &pUsbMse->loc_y) < 0) {
        xf86Msg(X_WARNING, "%s: no y locator\n", pInfo->name);
    }
    if (hid_locate(reportDesc, HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_WHEEL),
                   hid_input, &pUsbMse->loc_z) < 0) {
    }
#endif
    /* Probe for number of buttons */
    for (i = 1; i <= MSE_MAXBUTTONS; i++) {
        if (!hid_locate(reportDesc, HID_USAGE2(HUP_BUTTON, i),
                        hid_input, &pUsbMse->loc_btn[i-1]
#ifdef USB_NEW_HID
                        , pUsbMse->iid
#endif
                        ))
            break;
    }
    pMse->buttons = i-1;

    xf86CloseSerial(pInfo->fd);
    pInfo->fd = -1;

    /* Private structure */
    pMse->mousePriv = pUsbMse;

    /* Process common mouse options (like Emulate3Buttons, etc). */
    pMse->CommonOptions(pInfo);

    /* Setup the local procs. */
    pInfo->device_control = usbMouseProc;
    pInfo->read_input = usbReadInput;

    return TRUE;
}
#endif /* USBMOUSE */

static Bool
bsdMousePreInit(InputInfoPtr pInfo, const char *protocol, int flags)
{
    /* The protocol is guaranteed to be one of the internalNames[] */
#ifdef WSCONS_SUPPORT
    if (xf86NameCmp(protocol, "WSMouse") == 0) {
        return wsconsPreInit(pInfo, protocol, flags);
    }
#endif
#ifdef USBMOUSE_SUPPORT
    if (xf86NameCmp(protocol, "usb") == 0) {
        return usbPreInit(pInfo, protocol, flags);
    }
#endif
    return TRUE;
}

OSMouseInfoPtr
OSMouseInit(int flags)
{
    OSMouseInfoPtr p;

    p = calloc(1, sizeof(OSMouseInfoRec));
    if (!p)
        return NULL;
    p->SupportedInterfaces = SupportedInterfaces;
    p->BuiltinNames = BuiltinNames;
    p->DefaultProtocol = DefaultProtocol;
    p->CheckProtocol = CheckProtocol;
#if (defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)) && defined(MOUSE_PROTO_SYSMOUSE)
    p->SetupAuto = SetupAuto;
    p->SetPS2Res = SetSysMouseRes;
    p->SetBMRes = SetSysMouseRes;
    p->SetMiscRes = SetSysMouseRes;
#endif
#if (defined(__OpenBSD__) || defined(__NetBSD__)) && defined(WSCONS_SUPPORT)
    p->SetupAuto = SetupAuto;
    p->SetMiscRes = SetMouseRes;
#endif
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__) || defined(__DragonFly__) || defined(__NetBSD__)
    p->FindDevice = FindDevice;
#endif
    p->PreInit = bsdMousePreInit;
    return p;
}
