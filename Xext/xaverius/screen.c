/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * Xaverius per-screen private implementation.
 * Xaverius allocates the structure, DDX fills in ops.
 * Uses screen close hook for automatic cleanup.
 */
#include <dix-config.h>

#include <stdbool.h>

#include "dix/screen_hooks_priv.h"
#include "include/privates.h"
#include "include/scrnintstr.h"

#include "xaverius_intern.h"
#include "xaverius_priv.h"

DevPrivateKeyRec XaveriusScreenPrivateKeyRec = { 0 };

static void XaveriusScreenCloseHook(CallbackListPtr *pcbl, ScreenPtr pScreen, void *unused)
{
    (void)pcbl;
    (void)unused;

    XaveriusScreenPtr xav_screen = dixLookupPrivate(&pScreen->devPrivates,
                                                     &XaveriusScreenPrivateKeyRec);
    if (xav_screen) {
        if (xav_screen->ops && xav_screen->ops->ScreenFini)
            xav_screen->ops->ScreenFini(xav_screen);

        dixSetPrivate(&pScreen->devPrivates, &XaveriusScreenPrivateKeyRec, NULL);
        free(xav_screen);
    }
}

bool XaveriusScreenInit(ScreenPtr pScreen, const XaveriusDDXOps *ops)
{
    if (pScreen == NULL || ops == NULL)
        return false;

    XaveriusScreenPtr xav_screen = calloc(1, sizeof(XaveriusScreenRec));
    if (!xav_screen)
        return false;

    xav_screen->pScreen = pScreen;
    xav_screen->ops = ops;
    xav_screen->initialized = true;

    dixSetPrivate(&pScreen->devPrivates,
                  &XaveriusScreenPrivateKeyRec, xav_screen);

    dixScreenHookClose(pScreen, XaveriusScreenCloseHook);

    return true;
}

XaveriusScreenPtr XaveriusGetScreen(ScreenPtr pScreen)
{
    if (pScreen == NULL)
        return NULL;
    return dixLookupPrivate(&pScreen->devPrivates, &XaveriusScreenPrivateKeyRec);
}
