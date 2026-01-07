#ifndef __MIOVERLAY_H
#define __MIOVERLAY_H

#include <X11/Xdefs.h>
#include <X11/Xfuncproto.h>

typedef void (*miOverlayTransFunc) (ScreenPtr, int, BoxPtr);
typedef Bool (*miOverlayInOverlayFunc) (WindowPtr);

extern _X_EXPORT Bool

miInitOverlay(ScreenPtr pScreen,
              miOverlayInOverlayFunc inOverlay, miOverlayTransFunc trans);

#endif                          /* __MIOVERLAY_H */
