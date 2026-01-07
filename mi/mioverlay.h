#ifndef __MIOVERLAY_H
#define __MIOVERLAY_H

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

typedef void (*miOverlayTransFunc) (ScreenPtr, int, BoxPtr);
typedef Bool (*miOverlayInOverlayFunc) (WindowPtr);

extern _X_EXPORT Bool

miInitOverlay(ScreenPtr pScreen,
              miOverlayInOverlayFunc inOverlay, miOverlayTransFunc trans);

extern _X_EXPORT Bool

miOverlayGetPrivateClips(WindowPtr pWin,
                         RegionPtr *borderClip, RegionPtr *clipList);

#endif                          /* __MIOVERLAY_H */
