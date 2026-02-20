#ifndef __MIOVERLAY_H
#define __MIOVERLAY_H

typedef void (*miOverlayTransFunc) (ScreenPtr, int, BoxPtr);
typedef Bool (*miOverlayInOverlayFunc) (WindowPtr);

extern _X_EXPORT Bool

miInitOverlay(ScreenPtr pScreen,
              miOverlayInOverlayFunc inOverlay, miOverlayTransFunc trans);

extern _X_EXPORT Bool

miOverlayGetPrivateClips(WindowPtr pWin,
                         RegionPtr *borderClip, RegionPtr *clipList);

extern _X_EXPORT Bool miOverlayCollectUnderlayRegions(WindowPtr, RegionPtr *);
extern _X_EXPORT void miOverlayComputeCompositeClip(GCPtr, WindowPtr);
extern _X_EXPORT Bool miOverlayCopyUnderlay(ScreenPtr);
extern _X_EXPORT void miOverlaySetRootClip(ScreenPtr, Bool);

#endif                          /* __MIOVERLAY_H */
