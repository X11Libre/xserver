/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111@shitposting.expert>
 */

#include <kdrive-config.h>

#include "present.h"
#include "Xext/present/present_priv.h" /* extern uint32_t FakeScreenFps; */

#include "fake.h"
#include "kglamor.h"

#include "glamor.h"

#ifdef WITH_LIBDRM
#include <xf86drm.h>
#endif

#include <errno.h>

Bool
fakeInitAccel(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    FakeScrPriv *scrpriv = screen->driver;
    FakeScreenConf *config = screen->card->closure;
    int caps = GLAMOR_EGL_CAP_NONE;
    int has_dri3;

    if (config->dri_path) {
        scrpriv->dri_fd = open(config->dri_path, O_RDWR);
        if (scrpriv->dri_fd >= 0) {
#ifdef WITH_LIBDRM
            drmDropMaster(scrpriv->dri_fd);
#endif
        } else {
            LogMessage(X_WARNING, "Xfake(%d): Could not open %s: %s\n", pScreen->myNum, config->dri_path, strerror(errno));
        }
    } else {
        scrpriv->dri_fd = -1;
    }

    config->glamor_info.dri_fd = scrpriv->dri_fd;

    if (!KdGlamorInit(pScreen, &config->glamor_info, &caps)) {
        if (scrpriv->dri_fd >= 0) {
            close(scrpriv->dri_fd);
            scrpriv->dri_fd = -1;
            config->glamor_info.dri_fd = -1;
        }
        return FALSE;
    }

#define GLAMOR_EGL_CAP_DRI3_IMPORT_EXPORT (GLAMOR_EGL_CAP_DRI3_IMPORT | GLAMOR_EGL_CAP_DRI3_EXPORT)
    has_dri3 = (caps & GLAMOR_EGL_CAP_DRI3_IMPORT_EXPORT) == GLAMOR_EGL_CAP_DRI3_IMPORT_EXPORT;
    LogMessage(X_INFO, "Xfake(%d): DRI3 %s initialized\n", pScreen->myNum, has_dri3 ? "" : "not");

#if 0 /* Not yet implemented */
    LogMessage(X_INFO, "Xfake(%d): DRI3 explicit sync %s\n", pScreen->myNum,
               (caps & GLAMOR_EGL_CAP_DRI3_SYNCOBJ) ?
               "available" : "unavailable");
#endif

    if (scrpriv->dri_fd >= 0) {
        /*
         * X clients use present to try to synchronize with the screen
         * If no global fake rate was requested, use the highest value dix accepts (600)
         */
        if (!FakeScreenFps) {
            FakeScreenFps = 600;
            present_screen_init(pScreen, NULL);
            FakeScreenFps = 0;
        }
    }

    return TRUE;
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
