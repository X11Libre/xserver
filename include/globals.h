
#ifndef _XSERV_GLOBAL_H_
#define _XSERV_GLOBAL_H_

#include "window.h"             /* for WindowPtr */
#include "extinit.h"
#ifdef DPMSExtension
/* sigh, too many drivers assume this */
#include <X11/extensions/dpmsconst.h>
#endif

/* Global X server variables that are visible to mi, dix, os, and ddx */

extern _X_EXPORT CARD32 defaultScreenSaverTime;
extern _X_EXPORT CARD32 defaultScreenSaverInterval;
extern _X_EXPORT CARD32 ScreenSaverTime;
extern _X_EXPORT CARD32 ScreenSaverInterval;

#ifdef SCREENSAVER
extern _X_EXPORT Bool screenSaverSuspended;
#endif

extern _X_EXPORT const char *defaultFontPath;
extern _X_EXPORT int monitorResolution;
extern _X_EXPORT int defaultColorVisualClass;

extern _X_EXPORT int GrabInProgress;
extern _X_EXPORT char *SeatId;
extern _X_EXPORT char *ConnectionInfo;
extern _X_EXPORT sig_atomic_t inSignalContext;

#ifdef PANORAMIX
extern _X_EXPORT Bool PanoramiXExtensionDisabledHack;
#endif

#endif                          /* !_XSERV_GLOBAL_H_ */
