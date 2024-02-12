/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */

/* prototypes for the os-support layer of xfree86 DDX */

#ifndef _XSERVER_XF86_OS_SUPPORT
#define _XSERVER_XF86_OS_SUPPORT

#include <X11/Xdefs.h>

#include "os.h"
#include "dix/dix_priv.h"

/*
 * This is to prevent re-entrancy to FatalError() when aborting.
 * Anything that can be called as a result of ddxGiveUp() should use this
 * instead of FatalError().
 */

#define xf86FatalError(a, b) \
	if (dispatchException & DE_TERMINATE) { \
		ErrorF(a, b); \
		ErrorF("\n"); \
		return; \
	} else FatalError(a, b)

typedef void (*PMClose) (void);

void xf86OpenConsole(void);
void xf86CloseConsole(void);
Bool xf86VTActivate(int vtno);
Bool xf86VTSwitchPending(void);
Bool xf86VTSwitchAway(void);
Bool xf86VTSwitchTo(void);
void xf86VTRequest(int sig);
int xf86ProcessArgument(int argc, char **argv, int i);
void xf86UseMsg(void);
PMClose xf86OSPMOpen(void);
void xf86InitVidMem(void);

void xf86OSRingBell(int volume, int pitch, int duration);
void xf86OSInputThreadInit(void);
Bool xf86DeallocateGARTMemory(int screenNum, int key);
int xf86RemoveSIGIOHandler(int fd);

typedef struct {
    Bool initialised;
} VidMemInfo, *VidMemInfoPtr;

void xf86OSInitVidMem(VidMemInfoPtr);

#ifdef XSERVER_PLATFORM_BUS
#include "hotplug.h"

struct OdevAttributes;

void
xf86PlatformDeviceProbe(struct OdevAttributes *attribs);

void
xf86PlatformReprobeDevice(int index, struct OdevAttributes *attribs);
#endif

#if defined(__sun)
extern char xf86SolarisFbDev[PATH_MAX];

/* these are only used inside sun-specific os-support */
void xf86VTAcquire(int);
void xf86VTRelease(int);
#endif

#endif /* _XSERVER_XF86_OS_SUPPORT */
