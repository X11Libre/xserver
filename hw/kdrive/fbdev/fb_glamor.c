/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111@shitposting.expert>
 */

#include <kdrive-config.h>

#include "fbdev.h"
#include "kglamor.h"

Bool
fbdevInitAccel(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    FbScreenConf *config = screen->card->closure;

    if (screen->rate > 60) {
        config->glamor_info.fake_rate = screen->rate;
    }

    return KdGlamorInit(pScreen, &config->glamor_info, NULL);
}

void
fbdevEnableAccel(ScreenPtr pScreen)
{
    KdGlamorEnable(pScreen);
}

void
fbdevDisableAccel(ScreenPtr pScreen)
{
    KdGlamorDisable(pScreen);
}

void
fbdevFiniAccel(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    FbScreenConf *config = screen->card->closure;

    KdGlamorFini(pScreen, &config->glamor_info);
}
