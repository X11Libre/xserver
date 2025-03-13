/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include "dix/dix_priv.h"
#include "dix/gc_priv.h"
#include "include/screenint.h"
#include "include/scrnintstr.h"

void dixFreeScreen(ScreenPtr pScreen)
{
    if (!pScreen)
        return;

    FreeGCperDepth(pScreen);
    dixDestroyPixmap(pScreen->defaultStipple, 0);
    dixFreeScreenSpecificPrivates(pScreen);
    pScreen->CloseScreen(pScreen);
    dixFreePrivates(pScreen->devPrivates, PRIVATE_SCREEN);
    free(pScreen);
}
