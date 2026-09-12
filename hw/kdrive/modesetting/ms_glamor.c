/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111github@gmail.com>
 */

#include <kdrive-config.h>

#include "present.h"
#include "Xext/present/present_priv.h" /* extern uint32_t FakeScreenFps; */

#include "modesetting.h"
#include "kglamor.h"

#include "glamor.h"

#include <xf86drm.h>

#include <errno.h>

Bool
msInitAccel(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *scrpriv = screen->driver;
    msPriv *priv = screen->card->driver;
    MsScreenConf *config = screen->card->closure;
    int caps = GLAMOR_EGL_CAP_NONE;
    int has_dri3;

    if (config->dri_path) {
        scrpriv->dri_fd = open(config->dri_path, O_RDWR);
        if (scrpriv->dri_fd >= 0) {
#ifdef WITH_LIBDRM
            drmDropMaster(scrpriv->dri_fd);
#endif
        } else {
            LogMessage(X_WARNING, "Xmodesetting(%d): Could not open %s: %s\n", pScreen->myNum, config->dri_path, strerror(errno));
        }
    } else {
        scrpriv->dri_fd = dup(gbm_device_get_fd(priv->gbm));
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
    LogMessage(X_INFO, "Xmodesetting(%d): DRI3 %s initialized\n", pScreen->myNum, has_dri3 ? "" : "not");

#if 0 /* Not yet implemented */
    LogMessage(X_INFO, "Xmodesetting(%d): DRI3 explicit sync %s\n", pScreen->myNum,
               (caps & GLAMOR_EGL_CAP_DRI3_SYNCOBJ) ?
               "available" : "unavailable");
#endif

    if (scrpriv->dri_fd >= 0) {
        /*
         * X clients use present to try to synchronize with the screen
         * If no global fake rate was requested and the screen's rate is sane,
         * use that instead of the 60 fps default
         */
        if (!FakeScreenFps && (screen->rate >= 60) && (screen->rate <= 600)) {
            FakeScreenFps = screen->rate;
            present_screen_init(pScreen, NULL);
            FakeScreenFps = 0;
        }
    }

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
