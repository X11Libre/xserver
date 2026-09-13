/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111github@gmail.com>
 */

#include <kdrive-config.h>

#include "modesetting.h"
#include "kglamor.h"

#include "glamor.h"

Bool
msInitAccel(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *scrpriv = screen->driver;
    MsScreenConf *config = screen->card->closure;
    int caps = GLAMOR_EGL_CAP_NONE;

    if (screen->rate > 60) {
        config->glamor_info.fake_rate = screen->rate;
    }

    if (!KdGlamorInit(pScreen, &config->glamor_info, &caps, &scrpriv->dri_fd)) {
        return FALSE;
    }

#if 0
    if (caps & GLAMOR_EGL_CAP_TEXTURE_GBM_BO) {
    }
#endif

    return TRUE;
}

void
msEnableAccel(ScreenPtr pScreen)
{
    KdGlamorEnable(pScreen);
}

void
msDisableAccel(ScreenPtr pScreen)
{
    KdGlamorDisable(pScreen);
}

void
msFiniAccel(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *scrpriv = screen->driver;

    KdGlamorFini(pScreen);

    if (scrpriv->dri_fd >= 0) {
        close(scrpriv->dri_fd);
    }
}
