/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include "dix/callback_priv.h"
#include "dix/dix_priv.h"
#include "dix/gc_priv.h"
#include "dix/screensaver_priv.h"
#include "include/screenint.h"
#include "include/scrnintstr.h"

CallbackListPtr ScreenSaverAccessCallback = NULL;
CallbackListPtr ScreenAccessCallback = NULL;

static void dixFreeScreen(ScreenPtr pScreen)
{
    if (!pScreen)
        return;

    FreeGCperDepth(pScreen);
    dixDestroyPixmap(pScreen->defaultStipple, 0);
    dixFreeScreenSpecificPrivates(pScreen);
    dixScreenRaiseClose(pScreen);
    dixFreePrivates(pScreen->devPrivates, PRIVATE_SCREEN);
    DeleteCallbackList(&pScreen->hookWindowDestroy);
    DeleteCallbackList(&pScreen->hookWindowPosition);
    DeleteCallbackList(&pScreen->hookClose);
    DeleteCallbackList(&pScreen->hookPostClose);
    DeleteCallbackList(&pScreen->hookPixmapDestroy);
    DeleteCallbackList(&pScreen->hookPostCreateResources);
    free(pScreen);
}

void dixFreeAllScreens(void)
{
    if (screenInfo.numGPUScreens > 0) {
        for (int walkScreenIdx = screenInfo.numGPUScreens - 1; walkScreenIdx >= 0; walkScreenIdx--) {
            ScreenPtr walkScreen = screenInfo.gpuscreens[walkScreenIdx];
            dixFreeScreen(walkScreen);
            screenInfo.numGPUScreens = walkScreenIdx;
        }
    }
    memset(&screenInfo.gpuscreens, 0, sizeof(screenInfo.gpuscreens));

    if (screenInfo.numScreens > 0) {
        for (int walkScreenIdx = screenInfo.numScreens - 1; walkScreenIdx >= 0; walkScreenIdx--) {
            ScreenPtr walkScreen = screenInfo.screens[walkScreenIdx];
            dixFreeScreen(walkScreen);
            screenInfo.numScreens = walkScreenIdx;
        }
    }
    memset(&screenInfo.screens, 0, sizeof(screenInfo.screens));
}
