/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111github@gmail.com>
 */

#include <kdrive-config.h>

#include "modesetting.h"
#include "kglamor.h"

#include "glamor.h"

static Bool
msGlamorMapFront(ScreenPtr pScreen);

Bool
msGlamorCreateRes(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *scrpriv = screen->driver;
    PixmapPtr rootPixmap;
    Bool ret = TRUE;

    rootPixmap = (*pScreen->GetScreenPixmap)(pScreen);

    if (!gbm_bo_get_map(scrpriv->front)) {
        Bool used_modifiers = gbm_bo_get_used_modifiers(scrpriv->front);
        ret = !screen->dumb && glamor_egl_create_textured_pixmap_from_gbm_bo(rootPixmap, scrpriv->front,
                                                                             used_modifiers);
        if (ret) {
            LogMessage(X_INFO, "Xmodesetting(%d): Using a tiled front buffer\n", pScreen->myNum);
        } else {
            if (!msGlamorMapFront(pScreen)) {
                return FALSE;
            }
            ret = (*pScreen->ModifyPixmapHeader) (rootPixmap,
                                                  pScreen->width,
                                                  pScreen->height,
                                                  screen->fb.depth,
                                                  screen->fb.bitsPerPixel,
                                                  screen->fb.byteStride,
                                                  screen->fb.frameBuffer);

            if (ret) {
                LogMessage(X_INFO, "Xmodesetting(%d): Using a cpu mapped front buffer\n", pScreen->myNum);
            }
        }
    } else {
        LogMessage(X_INFO, "Xmodesetting(%d): Using a cpu mapped front buffer\n", pScreen->myNum);
    }

    if (ret) {
        LogMessage(X_INFO, "Xmodesetting(%d): Front buffer depth: %d, bpp: %d\n",
                   pScreen->myNum, screen->fb.depth, screen->fb.bitsPerPixel);
    }

    return ret;
}

static Bool
msGlamorMapFront(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msPriv *priv = screen->card->driver;
    msScrPriv *scrpriv = screen->driver;
    msScrPriv oldscr;

    struct gbm_bo *old_front = scrpriv->front;

    LogMessage(X_ERROR, "Xmodesetting(%d): Cannot use tiled gbm front, trying to use a mapped front bo\n", pScreen->myNum);

    oldscr = *scrpriv;

    msUnmapFramebuffer(screen);

    scrpriv->front = modesetting_open(priv, screen, TRUE /* need_map */);
    if (!scrpriv->front) {
        goto bail;
    }

    if (!msMapFramebuffer(screen)) {
        goto bail;
    }

    gbm_bo_destroy(old_front);
    scrpriv->setPixmapBits = TRUE;

    return TRUE;

bail:
    msUnmapFramebuffer(screen);
    *scrpriv = oldscr;
    msMapFramebuffer(screen);
    scrpriv->error = TRUE;
    return FALSE;
}

Bool
msInitAccel(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    MsScreenConf *config = screen->closure;
    msPriv *priv = screen->card->driver;

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

    if (!KdGlamorInit(pScreen, &config->glamor_info, NULL)) {
        return FALSE;
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
    MsScreenConf *config = screen->closure;

    KdGlamorFini(pScreen, &config->glamor_info);
}
