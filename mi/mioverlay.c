#include <dix-config.h>

#include "windowstr.h"

/* this all is just left here for compat with proprietary Nvidia drivers */

typedef void (*miOverlayTransFunc) (ScreenPtr, int, BoxPtr);
typedef Bool (*miOverlayInOverlayFunc) (WindowPtr);

_X_EXPORT Bool miInitOverlay(ScreenPtr pScreen,
                             miOverlayInOverlayFunc inOverlay,
                             miOverlayTransFunc trans);

_X_EXPORT Bool miOverlayGetPrivateClips(WindowPtr pWin,
                                        RegionPtr *borderClip,
                                        RegionPtr *clipList);

_X_EXPORT Bool miOverlayCollectUnderlayRegions(WindowPtr, RegionPtr *);
_X_EXPORT void miOverlayComputeCompositeClip(GCPtr, WindowPtr);
_X_EXPORT Bool miOverlayCopyUnderlay(ScreenPtr);
_X_EXPORT void miOverlaySetRootClip(ScreenPtr, Bool);

Bool
miInitOverlay(ScreenPtr pScreen,
              miOverlayInOverlayFunc inOverlayFunc,
              miOverlayTransFunc transFunc)
{
    LogMessage(X_WARNING, "miInitOverlay() shouldn't be called anymore (dummy)\n");
    return FALSE;
}

void
miOverlaySetRootClip(ScreenPtr pScreen, Bool enable)
{
    LogMessage(X_WARNING, "miOverlaySetRootClip() shouldn't be called anymore (dummy)\n");
}

/* not used */
Bool
miOverlayGetPrivateClips(WindowPtr pWin,
                         RegionPtr *borderClip, RegionPtr *clipList)
{
    LogMessage(X_WARNING, "miOverlayGetPrivateClips() shouldn't be called anymore (dummy)\n");
    *borderClip = *clipList = NULL;
    return FALSE;
}

Bool
miOverlayCopyUnderlay(ScreenPtr pScreen)
{
    LogMessage(X_WARNING, "miOverlayCopyUnderlay() shouldn't be called anymore (dummy)\n");
    return FALSE;
}

void
miOverlayComputeCompositeClip(GCPtr pGC, WindowPtr pWin)
{
    LogMessage(X_WARNING, "miOverlayComputeCompositeClip() shouldn't be called anymore (dummy)\n");
}

Bool
miOverlayCollectUnderlayRegions(WindowPtr pWin, RegionPtr *region)
{
    LogMessage(X_WARNING, "miOverlayCollectUnderlayRegions() shouldn't be called anymore (dummy)\n");
    return FALSE;
}
