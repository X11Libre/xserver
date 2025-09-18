#include <dix-config.h>

#include <X11/Xfuncproto.h>

#include "os/osdep.h"

#include "xf86_compat.h"

/*
 * needed for NVidia proprietary driver 570.x and 575.x version
 * force the server to see if any timer callbacks should be called
 *
 * this function had been obsolete and removed long ago, but NVidia folks
 * still didn't do basic maintenance and fixed their driver
 */

_X_EXPORT void TimerCheck(void);

void TimerCheck(void) {
    xf86NVidiaBugObsoleteFunc("TimerCheck()");

    DoTimers(GetTimeInMillis());
}
