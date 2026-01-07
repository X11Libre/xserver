
#include <dix-config.h>

#include "windowstr.h"
#include "mioverlay.h"

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
