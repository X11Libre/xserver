/*
 * Copyright (c) 2004, 2022, Oracle and/or its affiliates.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
/*
 * Copyright 1999-2001 The XFree86 Project, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 */
#include <xorg-config.h>

#include <unistd.h> /* for ioctl(2) */
#include <sys/stropts.h>
#include <sys/vuid_event.h>
#include <sys/msio.h>
#include <fcntl.h>
#include <errno.h>
#include "xf86.h"
#include "xf86_OSlib.h"
#include "mouse.h"
#include "xisb.h"
#include "mipointer.h"
#include "xf86Crtc.h"

/* Wheel mouse support in VUID drivers in Solaris 9 updates & Solaris 10 */
#ifdef WHEEL_DEVID /* Defined in vuid_event.h if VUID wheel support present */
# define HAVE_VUID_WHEEL
#endif
#ifdef HAVE_VUID_WHEEL
# include <sys/vuid_wheel.h>
#endif

#include <libdevinfo.h>

/* Support for scaling absolute coordinates to screen size in
 * Solaris 10 updates and beyond */
#if !defined(HAVE_ABSOLUTE_MOUSE_SCALING)
# ifdef MSIOSRESOLUTION /* Defined in msio.h if scaling support present */
#  define HAVE_ABSOLUTE_MOUSE_SCALING
# endif
#endif

/* Names of protocols that are handled internally here. */

static const char *internalNames[] = {
        "VUID",
        NULL
};

static const char *solarisMouseDevs[] = {
    /* Device file:     Protocol:                       */
    "/dev/mouse",       "VUID",         /* USB or SPARC */
#if defined(__i386) || defined(__x86)
    "/dev/kdmouse",     "PS/2",         /* PS/2 */
#endif
    NULL
};

typedef struct _VuidMseRec {
    struct _VuidMseRec *next;
    InputInfoPtr        pInfo;
    Firm_event          event;
    unsigned char *     buffer;
    char *              strmod;
    Bool(*wrapped_device_control)(DeviceIntPtr device, int what);
#ifdef HAVE_ABSOLUTE_MOUSE_SCALING
    Ms_screen_resolution         absres;
#endif
    OsTimerPtr          remove_timer;   /* Callback for removal on ENODEV */
    int                 relToAbs;
    int                 absX;
    int                 absY;
} VuidMseRec, *VuidMsePtr;

static VuidMsePtr       vuidMouseList = NULL;

static int  vuidMouseProc(DeviceIntPtr pPointer, int what);
static void vuidReadInput(InputInfoPtr pInfo);

static int CheckRelToAbs(InputInfoPtr pInfo);
static int CheckRelToAbsWalker(di_node_t node, void *arg);

#ifdef HAVE_ABSOLUTE_MOUSE_SCALING

static void vuidMouseSendScreenSize(ScreenPtr pScreen, VuidMsePtr pVuidMse);
static void vuidMouseAdjustFrame(ScrnInfoPtr pScrn, int x, int y);

static unsigned long vuidMouseGeneration = 0;

static DevPrivateKeyRec vuidMouseScreenIndex;

#define vuidMouseGetScreenPrivate(s) ( \
    dixLookupPrivate(&(s)->devPrivates, &vuidMouseScreenIndex))
#define vuidMouseSetScreenPrivate(s,p) \
    dixSetPrivate(&(s)->devPrivates, &vuidMouseScreenIndex, (void *) p)
#endif /* HAVE_ABSOLUTE_MOUSE_SCALING */

static inline
VuidMsePtr getVuidMsePriv(InputInfoPtr pInfo)
{
    VuidMsePtr m = vuidMouseList;

    while ((m != NULL) && (m->pInfo != pInfo)) {
        m = m->next;
    }

    return m;
}

/* Called from OsTimer callback, since removing a device from the device
   list or changing pInfo->fd while xf86Wakeup is looping through the list
   causes server crashes */
static CARD32
vuidRemoveMouse(OsTimerPtr timer, CARD32 now, void *arg)
{
    InputInfoPtr pInfo = (InputInfoPtr) arg;

    xf86DisableDevice(pInfo->dev, TRUE);

    return 0;  /* All done, don't set to run again */
}

/*
 * Initialize and enable the mouse wheel, if present.
 *
 * Returns 1 if mouse wheel was successfully enabled.
 * Returns 0 if an error occurred or if there is no mouse wheel.
 */
static int
vuidMouseWheelInit(InputInfoPtr pInfo)
{
#ifdef HAVE_VUID_WHEEL
    wheel_state wstate;
    int nwheel = -1;
    int i;

    wstate.vers = VUID_WHEEL_STATE_VERS;
    wstate.id = 0;
    wstate.stateflags = (uint32_t) -1;

    SYSCALL(i = ioctl(pInfo->fd, VUIDGWHEELCOUNT, &nwheel));
    if (i != 0)
        return (0);

    SYSCALL(i = ioctl(pInfo->fd, VUIDGWHEELSTATE, &wstate));
    if (i != 0) {
        xf86Msg(X_WARNING, "%s: couldn't get wheel state\n", pInfo->name);
        return (0);
    }

    wstate.stateflags |= VUID_WHEEL_STATE_ENABLED;

    SYSCALL(i = ioctl(pInfo->fd, VUIDSWHEELSTATE, &wstate));
    if (i != 0) {
        xf86Msg(X_WARNING, "%s: couldn't enable wheel\n", pInfo->name);
        return (0);
    }

    return (1);
#else
    return (0);
#endif
}


/* This function is called when the protocol is "VUID". */
static Bool
vuidPreInit(InputInfoPtr pInfo, const char *protocol, int flags)
{
    MouseDevPtr pMse = pInfo->private;
    VuidMsePtr pVuidMse;

    /* Ensure we don't add the same device twice */
    if (getVuidMsePriv(pInfo) != NULL)
        return TRUE;

    pVuidMse = calloc(1, sizeof(VuidMseRec));
    if (pVuidMse == NULL) {
        xf86Msg(X_ERROR, "%s: cannot allocate VuidMouseRec\n", pInfo->name);
        free(pMse);
        return FALSE;
    }

    pVuidMse->buffer = (unsigned char *)&pVuidMse->event;
    pVuidMse->strmod = xf86SetStrOption(pInfo->options, "StreamsModule", NULL);
    pVuidMse->relToAbs = xf86SetIntOption(pInfo->options, "RelToAbs", -1);
    if (pVuidMse->relToAbs == -1)
        pVuidMse->relToAbs = CheckRelToAbs(pInfo);
    pVuidMse->absX = 0;
    pVuidMse->absY = 0;

    /* Setup the local procs. */
    pVuidMse->wrapped_device_control = pInfo->device_control;
    pInfo->device_control = vuidMouseProc;
    pInfo->read_input = vuidReadInput;

    pMse->xisbscale = sizeof(Firm_event);

#ifdef HAVE_ABSOLUTE_MOUSE_SCALING
    pVuidMse->absres.height = pVuidMse->absres.width = 0;
#endif
    pVuidMse->pInfo = pInfo;
    pVuidMse->next = vuidMouseList;
    vuidMouseList = pVuidMse;

    return TRUE;
}

/*
 * It seems that the mouse that is presented by the Emulex ILOM
 * device (USB 0x430, 0xa101 and USB 0x430, 0xa102) sends relative
 * mouse movements.  But relative mouse movements are subject to
 * acceleration.  This causes the position indicated on the ILOM
 * window to not match what the Xorg server actually has.  This
 * makes the mouse in this environment rather unusable.  So, for the
 * Emulex ILOM device, we will change all relative mouse movements
 * into absolute mouse movements, making it appear more like a
 * tablet.  This will not be subject to acceleration, and this
 * should keep the ILOM window and Xorg server with the same values
 * for the coordinates of the mouse.
 */

typedef struct reltoabs_match {
	int matched;
	char const *matchname;
	} reltoabs_match_t;

/* Sun Microsystems, keyboard / mouse */
#define	RELTOABS_MATCH1	"usbif430,a101.config1.1"
/* Sun Microsystems, keyboard / mouse / storage */
#define	RELTOABS_MATCH2	"usbif430,a102.config1.1"

static int
CheckRelToAbsWalker(di_node_t node, void *arg)
{
	di_minor_t minor;
	char *path;
	int numvalues;
	int valueon;
	char const *stringptr;
	int status;
	reltoabs_match_t *const matchptr = (reltoabs_match_t *)arg;
	char *stringvalues;

	for (minor = di_minor_next(node, DI_MINOR_NIL);
	  minor != DI_MINOR_NIL;
	  minor = di_minor_next(node, minor)) {
	    path = di_devfs_minor_path(minor);
	    status = path != NULL && strcmp(path, matchptr -> matchname) == 0;
	    di_devfs_path_free(path);
	    if (status)
		break;
	    }

	if (minor == DI_MINOR_NIL)
	    return (DI_WALK_CONTINUE);

	numvalues = di_prop_lookup_strings(DDI_DEV_T_ANY, node,
	  "compatible", &stringvalues);
	if (numvalues <= 0)
	    return (DI_WALK_CONTINUE);

	for (valueon = 0, stringptr = stringvalues; valueon < numvalues;
	  valueon++, stringptr += strlen(stringptr) + 1) {
	    if (strcmp(stringptr, RELTOABS_MATCH1) == 0) {
		matchptr -> matched = 1;
		return (DI_WALK_TERMINATE);
	    }
	    if (strcmp(stringptr, RELTOABS_MATCH2) == 0) {
		matchptr -> matched = 2;
		return (DI_WALK_TERMINATE);
	    }
	}
	return (DI_WALK_CONTINUE);
}

static int
CheckRelToAbs(InputInfoPtr pInfo)
{
	char const *device;
	char const *matchname;
	ssize_t readstatus;
	di_node_t node;
	struct stat statbuf;
	char linkname[512];
	reltoabs_match_t reltoabs_match;

	device = xf86CheckStrOption(pInfo->options, "Device", NULL);
	if (device == NULL)
	    return (0);

	matchname = device;

	if (lstat(device, &statbuf) == 0 &&
	  (statbuf.st_mode & S_IFMT) == S_IFLNK) {
	    readstatus = readlink(device, linkname, sizeof(linkname));
	    if (readstatus > 0 && readstatus < (ssize_t) sizeof(linkname)) {
		linkname[readstatus] = 0;
		matchname = linkname;
		if (strncmp(matchname, "../..", sizeof("../..") - 1) == 0)
		    matchname += sizeof("../..") - 1;
	    }
	}

	if (strncmp(matchname, "/devices", sizeof("/devices") - 1) == 0)
	    matchname += sizeof("/devices") - 1;

	reltoabs_match.matched = 0;
	reltoabs_match.matchname = matchname;

	node = di_init("/", DINFOCPYALL);
	if (node == DI_NODE_NIL)
	    return (0);

	di_walk_node(node, DI_WALK_CLDFIRST, (void *)&reltoabs_match,
	  CheckRelToAbsWalker);

	di_fini(node);

	return (reltoabs_match.matched != 0);
}

static void
vuidFlushAbsEvents(InputInfoPtr pInfo, int absX, int absY,
                   Bool *absXset, Bool *absYset)
{
#ifdef DEBUG
    ErrorF("vuidFlushAbsEvents: %d,%d (set: %d, %d)\n", absX, absY,
           *absXset, *absYset);
#endif
    if ((*absXset) && (*absYset)) {
        xf86PostMotionEvent(pInfo->dev,
                            /* is_absolute: */    TRUE,
                            /* first_valuator: */ 0,
                            /* num_valuators: */  2,
                            absX, absY);
    } else if (*absXset) {
        xf86PostMotionEvent(pInfo->dev,
                            /* is_absolute: */    TRUE,
                            /* first_valuator: */ 0,
                            /* num_valuators: */  1,
                            absX);
    } else if (*absYset) {
        xf86PostMotionEvent(pInfo->dev,
                            /* is_absolute: */    TRUE,
                            /* first_valuator: */ 1,
                            /* num_valuators: */  1,
                            absY);
    }

    *absXset = FALSE;
    *absYset = FALSE;
}

static void
vuidReadInput(InputInfoPtr pInfo)
{
    MouseDevPtr pMse;
    VuidMsePtr pVuidMse;
    int buttons;
    int dx = 0, dy = 0, dz = 0, dw = 0;
    ssize_t n;
    unsigned char *pBuf;
    int absX = 0, absY = 0;
    int hdisplay = 0, vdisplay = 0;
    Bool absXset = FALSE, absYset = FALSE;
    int relToAbs;

    pMse = pInfo->private;
    buttons = pMse->lastButtons;
    pVuidMse = getVuidMsePriv(pInfo);
    if (pVuidMse == NULL) {
        xf86Msg(X_ERROR, "%s: cannot locate VuidMsePtr\n", pInfo->name);
        return;
    }
    pBuf = pVuidMse->buffer;
    relToAbs = pVuidMse->relToAbs;
    if (relToAbs) {
	ScreenPtr   pScreen = miPointerGetScreen(pInfo->dev);
	ScrnInfoPtr pScr = XF86SCRNINFO(pScreen);

	if (pScr->currentMode) {
	    hdisplay = pScr->currentMode->HDisplay;
	    vdisplay = pScr->currentMode->VDisplay;
	}
	absX = pVuidMse->absX;
	absY = pVuidMse->absY;
    }

    do {
        n = read(pInfo->fd, pBuf, sizeof(Firm_event));

        if (n == 0) {
            break;
        } else if (n == -1) {
            switch (errno) {
                case EAGAIN: /* Nothing to read now */
                    n = 0;   /* End loop, go on to flush events & return */
                    continue;
                case EINTR:  /* Interrupted, try again */
                    continue;
                case ENODEV: /* May happen when USB mouse is unplugged */
                    /* We use X_NONE here because it didn't alloc since we
                       may be called from SIGIO handler. No longer true for
                       sigsafe logging, but matters for older servers  */
                    LogMessageVerbSigSafe(X_NONE, 0,
                                          "%s: Device no longer present - removing.\n",
                                          pInfo->name);
                    xf86RemoveEnabledDevice(pInfo);
                    pVuidMse->remove_timer =
                        TimerSet(pVuidMse->remove_timer, 0, 1,
                                 vuidRemoveMouse, pInfo);
                    return;
                default:     /* All other errors */
                    /* We use X_NONE here because it didn't alloc since we
                       may be called from SIGIO handler. No longer true for
                       sigsafe logging, but matters for older servers  */
                    LogMessageVerbSigSafe(X_NONE, 0, "%s: Read error: %s\n",
                                          pInfo->name, strerror(errno));
                    return;
            }
        } else if (n != sizeof(Firm_event)) {
            xf86Msg(X_WARNING, "%s: incomplete packet, size %zd\n",
                        pInfo->name, n);
        }

#ifdef DEBUG
        LogMessageVerbSigSafe("vuidReadInput: event type: %d value: %d\n",
                              pVuidMse->event.id, pVuidMse->event.value);
#endif

        if (pVuidMse->event.id >= BUT_FIRST && pVuidMse->event.id <= BUT_LAST) {
            /* button */
            int butnum = pVuidMse->event.id - BUT_FIRST;

            if (butnum < 3)
                butnum = 2 - butnum;
            if (!pVuidMse->event.value)
                buttons &= ~(1 << butnum);
            else
                buttons |= (1 << butnum);
        } else if (pVuidMse->event.id >= VLOC_FIRST &&
                   pVuidMse->event.id <= VLOC_LAST) {
            /* axis */
            int delta = pVuidMse->event.value;
            switch(pVuidMse->event.id) {
            case LOC_X_DELTA:
                if (!relToAbs)
                    dx += delta;
                else {
                    if (absXset)
                        vuidFlushAbsEvents(pInfo, absX, absY, &absXset, &absYset);
                    absX += delta;
                    if (absX < 0)
                        absX = 0;
                    else if (absX >= hdisplay && hdisplay > 0)
                        absX = hdisplay - 1;
                    pVuidMse->absX = absX;
                    absXset = TRUE;
                }
                break;
            case LOC_Y_DELTA:
                if (!relToAbs)
                    dy -= delta;
                else {
                    if (absYset)
                        vuidFlushAbsEvents(pInfo, absX, absY, &absXset, &absYset);
                    absY -= delta;
                    if (absY < 0)
                        absY = 0;
                    else if (absY >= vdisplay && vdisplay > 0)
                        absY = vdisplay - 1;
                    pVuidMse->absY = absY;
                    absYset = TRUE;
                }
                break;
            case LOC_X_ABSOLUTE:
                if (absXset) {
                    vuidFlushAbsEvents(pInfo, absX, absY, &absXset, &absYset);
                }
                absX = delta;
                absXset = TRUE;
                break;
            case LOC_Y_ABSOLUTE:
                if (absYset) {
                    vuidFlushAbsEvents(pInfo, absX, absY, &absXset, &absYset);
                }
                absY = delta;
                absYset = TRUE;
                break;
            }
        }
#ifdef HAVE_VUID_WHEEL
        else if (vuid_in_range(VUID_WHEEL, pVuidMse->event.id)) {
            if (vuid_id_offset(pVuidMse->event.id) == 0)
                dz -= VUID_WHEEL_GETDELTA(pVuidMse->event.value);
            else
                dw -= VUID_WHEEL_GETDELTA(pVuidMse->event.value);
        }
#endif
#ifdef HAVE_ABSOLUTE_MOUSE_SCALING
        else if (pVuidMse->event.id == MOUSE_TYPE_ABSOLUTE) {
            ScreenPtr   ptrCurScreen;

            /* force sending absolute resolution scaling ioctl */
            pVuidMse->absres.height = pVuidMse->absres.width = 0;
            ptrCurScreen = miPointerGetScreen(pInfo->dev);
            vuidMouseSendScreenSize(ptrCurScreen, pVuidMse);
        }
#endif

    } while (n != 0);

    if (absXset || absYset) {
        vuidFlushAbsEvents(pInfo, absX, absY, &absXset, &absYset);
    }

    pMse->PostEvent(pInfo, buttons, dx, dy, dz, dw);
    return;
}

#ifdef HAVE_ABSOLUTE_MOUSE_SCALING
static void vuidMouseSendScreenSize(ScreenPtr pScreen, VuidMsePtr pVuidMse)
{
    InputInfoPtr pInfo = pVuidMse->pInfo;
    ScrnInfoPtr pScr = XF86SCRNINFO(pScreen);
    int result;

    if ((pVuidMse->absres.width != pScr->virtualX) ||
        (pVuidMse->absres.height != pScr->virtualY))
    {
        pVuidMse->absres.width = pScr->virtualX;
        pVuidMse->absres.height = pScr->virtualY;

        do {
            result = ioctl(pInfo->fd, MSIOSRESOLUTION, &(pVuidMse->absres));
        } while ( (result != 0) && (errno == EINTR) );

        if (result != 0) {
            LogMessageVerbSigSafe(X_WARNING, -1,
                                  "%s: couldn't set absolute mouse scaling resolution: %s\n",
                                  pInfo->name, strerror(errno));
#ifdef DEBUG
        } else {
            LogMessageVerbSigSafe(X_INFO,
                                  "%s: absolute mouse scaling resolution set to %d x %d\n",
                                  pInfo->name,
                                  pVuidMse->absres.width,
                                  pVuidMse->absres.height);
#endif
        }
    }
}

static void vuidMouseAdjustFrame(ScrnInfoPtr pScrn, int x, int y)
{
      ScreenPtr         pScreen = xf86ScrnToScreen(pScrn);
      xf86AdjustFrameProc *wrappedAdjustFrame
          = (xf86AdjustFrameProc *) vuidMouseGetScreenPrivate(pScreen);
      VuidMsePtr        m;
      ScreenPtr         ptrCurScreen;

      if (wrappedAdjustFrame) {
          pScrn->AdjustFrame = wrappedAdjustFrame;
          (*pScrn->AdjustFrame)(pScrn, x, y);
          pScrn->AdjustFrame = vuidMouseAdjustFrame;
      }

      for (m = vuidMouseList; m != NULL ; m = m->next) {
          ptrCurScreen = miPointerGetScreen(m->pInfo->dev);
          if (ptrCurScreen == pScreen)
          {
              vuidMouseSendScreenSize(pScreen, m);
          }
      }
}

static void vuidMouseCrtcNotify(ScreenPtr pScreen)
{
    xf86_crtc_notify_proc_ptr wrappedCrtcNotify
        = (xf86_crtc_notify_proc_ptr) vuidMouseGetScreenPrivate(pScreen);
    VuidMsePtr       m;
    ScreenPtr        ptrCurScreen;

    if (wrappedCrtcNotify)
        wrappedCrtcNotify(pScreen);

    for (m = vuidMouseList; m != NULL ; m = m->next) {
        ptrCurScreen = miPointerGetScreen(m->pInfo->dev);
        if (ptrCurScreen == pScreen) {
            vuidMouseSendScreenSize(pScreen, m);
        }
    }
}
#endif /* HAVE_ABSOLUTE_MOUSE_SCALING */


static int
vuidMouseProc(DeviceIntPtr pPointer, int what)
{
    InputInfoPtr pInfo;
    MouseDevPtr pMse;
    VuidMsePtr pVuidMse;
    int ret = Success;
    int i;

    pInfo = pPointer->public.devicePrivate;
    pMse = pInfo->private;
    pMse->device = pPointer;

    pVuidMse = getVuidMsePriv(pInfo);
    if (pVuidMse == NULL) {
        return BadImplementation;
    }

    switch (what) {

    case DEVICE_INIT:
#ifdef HAVE_ABSOLUTE_MOUSE_SCALING

        if (!dixRegisterPrivateKey(&vuidMouseScreenIndex, PRIVATE_SCREEN, 0))
                return BadAlloc;

        if (vuidMouseGeneration != serverGeneration) {
                for (i = 0; i < screenInfo.numScreens; i++) {
                    ScreenPtr pScreen = screenInfo.screens[i];
                    ScrnInfoPtr pScrn = XF86SCRNINFO(pScreen);
                    if (xf86CrtcConfigPrivateIndex != -1) {
                        xf86_crtc_notify_proc_ptr pCrtcNotify
                            = xf86_wrap_crtc_notify(pScreen,
                                                    vuidMouseCrtcNotify);
                        vuidMouseSetScreenPrivate(pScreen, pCrtcNotify);
                    } else {
                        vuidMouseSetScreenPrivate(pScreen,
                                                  pScrn->AdjustFrame);
                        pScrn->AdjustFrame = vuidMouseAdjustFrame;
                    }
                }
            vuidMouseGeneration = serverGeneration;
        }
#endif
        ret = pVuidMse->wrapped_device_control(pPointer, what);
        break;

    case DEVICE_ON:
        ret = pVuidMse->wrapped_device_control(pPointer, DEVICE_ON);

        if ((ret == Success) && (pInfo->fd != -1)) {
            int fmt = VUID_FIRM_EVENT;

            if (pVuidMse->strmod) {
                /* Check to see if module is already pushed */
                SYSCALL(i = ioctl(pInfo->fd, I_FIND, pVuidMse->strmod));

                if (i == 0) { /* Not already pushed */
                    SYSCALL(i = ioctl(pInfo->fd, I_PUSH, pVuidMse->strmod));
                    if (i < 0) {
                        xf86Msg(X_WARNING, "%s: cannot push module '%s' "
                                "onto mouse device: %s\n", pInfo->name,
                                pVuidMse->strmod, strerror(errno));
                        free(pVuidMse->strmod);
                        pVuidMse->strmod = NULL;
                    }
                }
            }

            SYSCALL(i = ioctl(pInfo->fd, VUIDSFORMAT, &fmt));
            if (i < 0) {
                xf86Msg(X_WARNING,
                        "%s: cannot set mouse device to VUID mode: %s\n",
                        pInfo->name, strerror(errno));
            }
            vuidMouseWheelInit(pInfo);
#ifdef HAVE_ABSOLUTE_MOUSE_SCALING
            vuidMouseSendScreenSize(screenInfo.screens[0], pVuidMse);
#endif
            xf86FlushInput(pInfo->fd);

            /* Allocate here so we don't alloc in ReadInput which may be called
               from SIGIO handler. */
            if (pVuidMse->remove_timer == NULL) {
                pVuidMse->remove_timer = TimerSet(pVuidMse->remove_timer,
                                                  0, 0, NULL, NULL);
            }
        }
        break;

    case DEVICE_CLOSE:
        if (vuidMouseList == pVuidMse)
            vuidMouseList = vuidMouseList->next;
        else {
            VuidMsePtr m = vuidMouseList;

            while ((m != NULL) && (m->next != pVuidMse)) {
                m = m->next;
            }

            if (m != NULL)
                m->next = pVuidMse->next;
        }
        _X_FALLTHROUGH; /* fallthrough */
    case DEVICE_OFF:
        if (pInfo->fd != -1) {
            if (pVuidMse->strmod) {
                SYSCALL(i = ioctl(pInfo->fd, I_POP, pVuidMse->strmod));
                if (i == -1) {
                    xf86Msg(X_WARNING,
                      "%s: cannot pop module '%s' off mouse device: %s\n",
                      pInfo->name, pVuidMse->strmod, strerror(errno));
                }
            }
        }
        if (pVuidMse->remove_timer) {
            TimerFree(pVuidMse->remove_timer);
            pVuidMse->remove_timer = NULL;
        }
        ret = pVuidMse->wrapped_device_control(pPointer, what);
        break;

    default: /* Should never be called, but just in case */
        ret = pVuidMse->wrapped_device_control(pPointer, what);
        break;
    }
    return ret;
}

static Bool
sunMousePreInit(InputInfoPtr pInfo, const char *protocol, int flags)
{
    /* The protocol is guaranteed to be one of the internalNames[] */
    if (xf86NameCmp(protocol, "VUID") == 0) {
        return vuidPreInit(pInfo, protocol, flags);
    }
    return TRUE;
}

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
    return "Auto";
}

static Bool
solarisMouseAutoProbe(InputInfoPtr pInfo, const char **protocol,
        const char **device)
{
    const char **pdev, **pproto;
    int fd = -1;
    Bool found;
    char *strmod;

    if (*device == NULL) {
        /* Check to see if xorg.conf or HAL specified a device to use */
        *device = xf86CheckStrOption(pInfo->options, "Device", NULL);
    }

    if (*device != NULL) {
        strmod = xf86CheckStrOption(pInfo->options, "StreamsModule", NULL);
        if (strmod) {
            /* if a device name is already known, and a StreamsModule is
               specified to convert events to VUID, then we don't need to
               probe further */
            *protocol = "VUID";
            return TRUE;
        }
    }


    for (pdev = solarisMouseDevs; *pdev; pdev += 2) {
        pproto = pdev + 1;
        if ((*protocol != NULL) && (strcmp(*protocol, "Auto") != 0) &&
          (*pproto != NULL) && (strcmp(*pproto, *protocol) != 0)) {
            continue;
        }
        if ((*device != NULL) && (strcmp(*device, *pdev) != 0)) {
            continue;
        }
        SYSCALL (fd = open(*pdev, O_RDWR | O_NONBLOCK));
        if (fd == -1) {
#ifdef DEBUG
            ErrorF("Cannot open %s (%s)\n", pdev, strerror(errno));
#endif
        } else {
            found = TRUE;
            if ((*pproto != NULL) && (strcmp(*pproto, "VUID") == 0)) {
                int i, r;
                SYSCALL(r = ioctl(fd, VUIDGFORMAT, &i));
                if (r < 0) {
                    found = FALSE;
                }
            }
            close(fd);
            if (found == TRUE) {
                if (*pproto != NULL) {
                    *protocol = *pproto;
                }
                *device = *pdev;
                return TRUE;
            }
        }
    }
    return FALSE;
}

static const char *
SetupAuto(InputInfoPtr pInfo, int *protoPara)
{
    const char *pdev = NULL;
    const char *pproto = NULL;
    MouseDevPtr pMse = pInfo->private;

    if (pInfo->fd == -1) {
        /* probe to find device/protocol to use */
        if (solarisMouseAutoProbe(pInfo, &pproto, &pdev) != FALSE) {
            /* Set the Device option. */
            pInfo->options =
             xf86AddNewOption(pInfo->options, "Device", pdev);
            xf86Msg(X_INFO, "%s: Setting Device option to \"%s\"\n",
              pInfo->name, pdev);
        }
    } else if (pMse->protocolID == PROT_AUTO) {
        pdev = xf86CheckStrOption(pInfo->options,
                "Device", NULL);
        if ((solarisMouseAutoProbe(pInfo, &pproto, &pdev) != FALSE) &&
            (pproto != NULL))
            sunMousePreInit(pInfo, pproto, 0);
    }
    return pproto;
}

static const char *
FindDevice(InputInfoPtr pInfo, const char *protocol, int flags)
{
    const char *pdev = NULL;
    const char *pproto = protocol;

    if (solarisMouseAutoProbe(pInfo, &pproto, &pdev) != FALSE) {
        /* Set the Device option. */
        pInfo->options =
          xf86AddNewOption(pInfo->options, "Device", pdev);
        xf86Msg(X_INFO, "%s: Setting Device option to \"%s\"\n",
          pInfo->name, pdev);
    }
    return pdev;
}

static int
SupportedInterfaces(void)
{
    /* XXX This needs to be checked. */
    return MSE_SERIAL | MSE_BUS | MSE_PS2 | MSE_AUTO | MSE_XPS2 | MSE_MISC;
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
    p->CheckProtocol = CheckProtocol;
    p->PreInit = sunMousePreInit;
    p->DefaultProtocol = DefaultProtocol;
    p->SetupAuto = SetupAuto;
    p->FindDevice = FindDevice;

    return p;
}

