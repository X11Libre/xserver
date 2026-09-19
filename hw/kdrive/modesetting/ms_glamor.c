/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111@shitposting.expert>
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

    struct gbm_format_name_desc desc = {0};
    uint32_t format;
    uint64_t modifier;
    const char *format_name;

    rootPixmap = (*pScreen->GetScreenPixmap)(pScreen);

    if (!gbm_bo_get_map(scrpriv->front)) {
        Bool used_modifiers = gbm_bo_get_used_modifiers(scrpriv->front);
        if (!screen->dumb &&
            glamor_egl_create_textured_pixmap_from_gbm_bo(rootPixmap, scrpriv->front, used_modifiers)) {
            LogMessage(X_INFO, "Xmodesetting(%d): Using a tiled front buffer\n", pScreen->myNum);
        } else {
            if (!msGlamorMapFront(pScreen)) {
                LogMessage(X_ERROR, "Xmodesetting(%d): Could not map the front buffer\n",
                           pScreen->myNum);
                return FALSE;
            }

            LogMessage(X_INFO, "Xmodesetting(%d): Using a cpu mapped front buffer\n", pScreen->myNum);
        }
    } else {
        LogMessage(X_INFO, "Xmodesetting(%d): Using a cpu mapped front buffer\n", pScreen->myNum);
    }

    format = gbm_bo_get_format(scrpriv->front);
    modifier = gbm_bo_get_modifier(scrpriv->front);
    format_name = gbm_format_get_name(format, &desc);
    LogMessage(X_INFO, "Xmodesetting(%d): Front buffer depth: %d, bpp: %d, format: %s, modifier: 0x%lx\n",
               pScreen->myNum, screen->fb.depth, screen->fb.bitsPerPixel, format_name, modifier);

    return TRUE;
}

static Bool
msGlamorMapFront(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msPriv *priv = screen->card->driver;
    struct gbm_bo *new_front = NULL;

    LogMessage(X_ERROR, "Xmodesetting(%d): Cannot use tiled gbm front, trying to use a mapped front bo\n", pScreen->myNum);

    new_front = modesetting_open(priv, screen, TRUE /* need_map */);
    if (!new_front ||
        !msSetScreenBo(pScreen, new_front, FALSE /* flip */)) {
        goto bail;
    }

    return TRUE;

bail:
    if (new_front) {
        gbm_bo_destroy(new_front);
    }
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
