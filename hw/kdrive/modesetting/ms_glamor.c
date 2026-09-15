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
    MsScreenConf *config = screen->closure;
    msPriv *priv = screen->card->driver;
    int caps = GLAMOR_EGL_CAP_NONE;

    if (screen->rate > 60) {
        config->glamor_info.fake_rate = screen->rate;
    }

    if (!config->glamor_info.dri_path) {
        config->glamor_info.dri_fd = dup(gbm_device_get_fd(priv->gbm));
        if (config->glamor_info.dri_fd >= 0) {
            config->glamor_info.want_dri_fd = TRUE;
        } else {
            config->glamor_info.dri_path = screen->card->closure;
        }
    }

    if (!KdGlamorInit(pScreen, &config->glamor_info, &caps)) {
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
    MsScreenConf *config = screen->closure;

    KdGlamorFini(pScreen, &config->glamor_info);
}
