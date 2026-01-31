#ifndef _XSERV_GLOBAL_H_
#define _XSERV_GLOBAL_H_

#include <X11/Xdefs.h>
#include <X11/Xfuncproto.h>

/* Global X server variables that are visible to mi, dix, os, and ddx */

extern _X_EXPORT const char *defaultFontPath;
extern _X_EXPORT int monitorResolution;

extern _X_EXPORT char *SeatId;

#endif                          /* !_XSERV_GLOBAL_H_ */
