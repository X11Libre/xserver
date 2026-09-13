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
    FakeScrPriv *scrpriv = screen->driver;
    FakeScreenConf *config = screen->card->closure;

    return KdGlamorInit(pScreen, &config->glamor_info, NULL, &scrpriv->dri_fd);
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
    FakeScrPriv *scrpriv = screen->driver;

    KdGlamorFini(pScreen);

    if (scrpriv->dri_fd >= 0) {
        close(scrpriv->dri_fd);
    }
}
