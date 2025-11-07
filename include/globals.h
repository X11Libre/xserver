
#ifndef _XSERV_GLOBAL_H_
#define _XSERV_GLOBAL_H_

#include <X11/Xdefs.h>
#include <X11/Xfuncproto.h>

#include "window.h"             /* for WindowPtr */
#include "extinit.h"
#ifdef DPMSExtension
/* sigh, too many drivers assume this */
#include <X11/extensions/dpmsconst.h>
#endif

/* Global X server variables that are visible to mi, dix, os, and ddx */

extern _X_EXPORT int monitorResolution;
extern _X_EXPORT int defaultColorVisualClass;

extern _X_EXPORT char *SeatId;

#endif                          /* !_XSERV_GLOBAL_H_ */
