/* Copyright (c) 2008-2012 Apple Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT
 * HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above
 * copyright holders shall not be used in advertising or otherwise to
 * promote the sale, use or other dealings in this Software without
 * prior written authorization.
 */

#include <dix-config.h>

#include <stdbool.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "xpr.h"

#include "eventstr.h"
#include "inputstr.h"
#include "mi.h"
#include "mipointer.h"
#include "misc.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include <X11/X.h>
#include <X11/Xmd.h>
#include <X11/Xproto.h>

#include "darwinEvents.h"
#include "quartz.h"
#include "quartzKeyboard.h"
#include "rootlessWindow.h"
#include "xprEvent.h"

#if XPLUGIN_VERSION >= 6
#include <dispatch/dispatch.h>

static void bringAllToFront(void *unused) {
  (void)unused; /* to silence the compiler warning */
  xp_window_bring_all_to_front();
}
#endif

bool QuartzModeEventHandler(int screenNum, XQuartzEvent *e, DeviceIntPtr dev) {
  switch (e->subtype) {
  case kXquartzWindowState:
    DEBUG_LOG("kXquartzWindowState\n");
    RootlessNativeWindowStateChanged(xprGetXWindow(e->data[0]), e->data[1]);
    return TRUE;

  case kXquartzWindowMoved:
    DEBUG_LOG("kXquartzWindowMoved\n");
    RootlessNativeWindowMoved(xprGetXWindow(e->data[0]));
    return TRUE;

  case kXquartzBringAllToFront:
    DEBUG_LOG("kXquartzBringAllToFront\n");
#if XPLUGIN_VERSION >= 6
    dispatch_async_f(
        dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), NULL,
        bringAllToFront);
#else
        RootlessOrderAllWindows(e->data[0]);
#endif

    return TRUE;

  default:
    return FALSE;
  }
}
