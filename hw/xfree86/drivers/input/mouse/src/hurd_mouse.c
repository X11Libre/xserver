/*
 * Copyright 1997,1998 by UCHIYAMA Yasushi
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of UCHIYAMA Yasushi not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  UCHIYAMA Yasushi makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * UCHIYAMA YASUSHI DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL UCHIYAMA YASUSHI BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <xorg-server.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include "inputstr.h"
#include "scrnintstr.h"
#include "mipointer.h"

#include "xf86.h"
#include "xf86Xinput.h"
#include "mouse.h"
#include "xf86_OSlib.h"
#include "xisb.h"

#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/file.h>
#include <assert.h>
#include <mach.h>
#include <sys/ioctl.h>

#define DEFAULT_MOUSE_DEV       "/dev/mouse"

typedef unsigned short kev_type;                /* kd event type */
typedef unsigned char Scancode;

struct mouse_motion {
    short mm_deltaX;            /* units? */
    short mm_deltaY;
};

typedef struct {
    kev_type type;                      /* see below */
    struct timeval time;                /* timestamp */
    union {                             /* value associated with event */
        boolean_t up;           /* MOUSE_LEFT .. MOUSE_RIGHT */
        Scancode sc;            /* KEYBD_EVENT */
        struct mouse_motion mmotion;    /* MOUSE_MOTION */
    } value;
} kd_event;

/*
 * kd_event ID's.
 */
#define MOUSE_LEFT      1               /* mouse left button up/down */
#define MOUSE_MIDDLE    2
#define MOUSE_RIGHT     3
#define MOUSE_MOTION    4               /* mouse motion */
#define KEYBD_EVENT     5               /* key up/down */

#define NUMEVENTS       64

/*
 * OsMouseReadInput --
 *      Get some events from our queue.  Process all outstanding events now.
 */
static void
OsMouseReadInput(InputInfoPtr pInfo)
{
    MouseDevPtr pMse;
    static kd_event eventList[NUMEVENTS];
    static int remainder = 0;
    int n, c;
    kd_event *event = eventList;
    unsigned char *pBuf;

    pMse = pInfo->private;

    XisbBlockDuration(pMse->buffer, -1);
    pBuf = (unsigned char *)eventList;
    n = remainder;
    while (n < sizeof(eventList) && (c = XisbRead(pMse->buffer)) >= 0)
        pBuf[n++] = (unsigned char)c;

    if (n == remainder)
        return;

    remainder = n % sizeof(kd_event);
    n /= sizeof(kd_event);
    while( n-- ) {
        int buttons = pMse->lastButtons;
        int dx = 0, dy = 0;
        switch (event->type) {
        case MOUSE_RIGHT:
            buttons  = (buttons & 6) |(event->value.up ? 0 : 1);
            break;
        case MOUSE_MIDDLE:
            buttons  = (buttons & 5) |(event->value.up ? 0 : 2);
            break;
        case MOUSE_LEFT:
            buttons  = (buttons & 3) |(event->value.up ? 0 : 4) ;
            break;
        case MOUSE_MOTION:
            dx = event->value.mmotion.mm_deltaX;
            dy = - event->value.mmotion.mm_deltaY;
            break;
        default:
            LogMessageVerbSigSafe(X_ERROR, -1, "Bad mouse event (%d)\n",event->type);
            continue;
        }
        pMse->PostEvent(pInfo, buttons, dx, dy, 0, 0);
        ++event;
    }
    memcpy(eventList, event, remainder);
    return;
}

static Bool
OsMousePreInit(InputInfoPtr pInfo, const char *protocol, int flags)
{
    MouseDevPtr pMse;

    /* This is called when the protocol is "OSMouse". */

    pMse = pInfo->private;
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
            free(pMse);
            return FALSE;
        }
    }
    xf86CloseSerial(pInfo->fd);
    pInfo->fd = -1;

    /* Process common mouse options (like Emulate3Buttons, etc). */
    pMse->CommonOptions(pInfo);

    /* Setup the local procs. */
    pInfo->read_input = OsMouseReadInput;

    return TRUE;
}

static const char *
FindDevice(InputInfoPtr pInfo, const char *protocol, int flags)
{
    static const char path[] = DEFAULT_MOUSE_DEV;
    int fd;

    SYSCALL (fd = open(path, O_RDWR | O_NONBLOCK | O_EXCL));

    if (fd == -1)
        return NULL;

    close(fd);
    pInfo->options =
        xf86AddNewOption(pInfo->options, "Device", path);
    xf86Msg(X_INFO, "%s: Setting Device option to \"%s\"\n", pInfo->name,
            path);

    return path;
}

static int
SupportedInterfaces(void)
{
    /* XXX Need to check this. */
    return MSE_SERIAL | MSE_BUS | MSE_PS2 | MSE_XPS2 | MSE_AUTO;
}

static const char *internalNames[] = {
        "OSMouse",
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
    return FALSE;
}

static const char *
DefaultProtocol(void)
{
    return "OSMouse";
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
    p->FindDevice = FindDevice;
    p->DefaultProtocol = DefaultProtocol;
    p->CheckProtocol = CheckProtocol;
    p->PreInit = OsMousePreInit;
    return p;
}

