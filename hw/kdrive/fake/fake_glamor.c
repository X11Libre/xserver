/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111@shitposting.expert>
 */

#include <kdrive-config.h>

#include "fake.h"
#include "kglamor.h"

Bool
fakeInitAccel(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    FakeScreenConf *config = screen->card->closure;

    return KdGlamorInit(pScreen, &config->glamor_info, NULL);
}

void
fakeEnableAccel(ScreenPtr pScreen)
{
    KdGlamorEnable(pScreen);
}

void
fakeDisableAccel(ScreenPtr pScreen)
{
    KdGlamorDisable(pScreen);
}

void
fakeFiniAccel(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    FakeScreenConf *config = screen->card->closure;

    KdGlamorFini(pScreen, &config->glamor_info);
}
