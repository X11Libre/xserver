/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111@shitposting.expert>
 */

#include <kdrive-config.h>

#include "modesetting.h"
#include "kglamor.h"

#include "glamor.h"

#include <drm_fourcc.h>

static Bool
msGlamorTileFront(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;

    KdFrameBuffer saved_framebuffer = screen->fb;

    /* TODO: query glamor */
    MsScreenConf *config = screen->closure;
    Bool is_gles = config->glamor_info.force_es;

    struct gbm_bo *new_front = NULL;

    /* By design, kdrive depth and bpp can't change between initAccel and finiAccel
     * https://www.x.org/Development/Documentation/KdriveDrivers/
     */
    new_front = modesetting_open(screen, FALSE /* need_map */, TRUE /* keep_depth */);
    if (!new_front) {
        return FALSE;
    }

    /* Maybe we could switch to this bo, but why? */
    if (gbm_bo_get_map(new_front)) {
        gbm_bo_destroy(new_front);
        return FALSE;
    }

    if (!msSetScreenBo(pScreen, new_front, FALSE /* flip */)) {
        gbm_bo_destroy(new_front);
        LogMessage(X_ERROR, "Xmodesetting(%d): Could not swap to the new front bo\n", pScreen->myNum);
        return FALSE;
    }

    /* Update visual masks if needed */
    gbm_bo_set_screen_fb_info(new_front, screen, is_gles);
    if (memcmp(&saved_framebuffer, &screen->fb, sizeof(screen->fb))) {
        /* TODO: Make this generic
         * For now, the only thing that can happen is r-b masks need swapping
         */

        for (int i = 0; i < pScreen->numVisuals; i++) {
            VisualPtr pVisual = &pScreen->visuals[i];
            if (pVisual->redMask == screen->fb.blueMask &&
                pVisual->blueMask == screen->fb.redMask) {
                int off = pVisual->offsetRed;
                pVisual->offsetRed = pVisual->offsetBlue;
                pVisual->offsetBlue = off;
                pVisual->redMask = screen->fb.redMask;
                pVisual->blueMask = screen->fb.blueMask;
            }
        }
    }
    return TRUE;
}

Bool
msGlamorCreateRes(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *scrpriv = screen->driver;

    struct gbm_format_name_desc desc = {0};
    uint32_t format;
    uint64_t modifier;
    const char *format_name;

    if (!screen->dumb) {
        if (!msGlamorTileFront(pScreen)) {
            LogMessage(X_INFO, "Xmodesetting(%d): Could not create a usable tiled front buffer\n", pScreen->myNum);
        }
    }

    if (gbm_bo_get_map(scrpriv->front)) {
        LogMessage(X_INFO, "Xmodesetting(%d): Using a cpu mapped front buffer\n", pScreen->myNum);
    } else {
        LogMessage(X_INFO, "Xmodesetting(%d): Using a tiled front buffer\n", pScreen->myNum);
    }

    format = gbm_bo_get_format(scrpriv->front);
    modifier = gbm_bo_get_modifier(scrpriv->front);
    format_name = gbm_format_get_name(format, &desc);
    LogMessage(X_INFO, "Xmodesetting(%d): Front buffer depth: %d, bpp: %d, format: %s, modifier: 0x%lx\n",
               pScreen->myNum, screen->fb.depth, screen->fb.bitsPerPixel, format_name, modifier);
    return TRUE;
}

Bool
msGlamorInit(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    MsScreenConf *config = screen->closure;
    msPriv *priv = screen->card->driver;
    msScrPriv *scrpriv = screen->driver;
    uint32_t format;
    int write_pos = 0;

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

    /*
     * TODO: Don't assume all formats support the same modifiers
     *
     * Not that we can do much if we are forced to chose a different format
     */
    format = gbm_bo_get_format(scrpriv->front);
    if (!glamor_get_modifiers(pScreen, format,
                              &scrpriv->num_render_modifiers,
                              &scrpriv->render_modifiers)) {
        scrpriv->num_render_modifiers = 0;
        scrpriv->render_modifiers = NULL;
    }

    /* TODO: Query scanout modifiers in CardInit,
     * intersect with the render modifiers, and cache them
     */

    /* Don't choose multi-plane formats for our screen pixmap.
     * These will get used with frontbuffer rendering, which will
     * lead to worse-than-tearing with multi-plane formats, as the
     * primary and auxiliary planes go out of sync. */
    if (!config->planar) {
        for (int i = 0; i < scrpriv->num_render_modifiers; i++) {
            if (gbm_device_get_format_modifier_plane_count(priv->gbm, format, scrpriv->render_modifiers[i]) > 1) {
                continue;
            }
            scrpriv->render_modifiers[write_pos++] = scrpriv->render_modifiers[i];
        }
    }

    if (write_pos == 0 ||
        (scrpriv->num_render_modifiers == 1 &&
         scrpriv->render_modifiers[0] == DRM_FORMAT_MOD_INVALID)) {
        free(scrpriv->render_modifiers);
        scrpriv->render_modifiers = NULL;
        scrpriv->num_render_modifiers = 0;
    } else if (write_pos < scrpriv->num_render_modifiers) {
        void* tmp = realloc(scrpriv->render_modifiers,
                            write_pos * sizeof(scrpriv->render_modifiers));
        if (tmp) {
            scrpriv->render_modifiers = tmp;
        }
    }

    return TRUE;
}

void
msGlamorEnable(ScreenPtr pScreen)
{
    KdGlamorEnable(pScreen);
}

void
msGlamorDisable(ScreenPtr pScreen)
{
    KdGlamorDisable(pScreen);
}

void
msGlamorFini(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    MsScreenConf *config = screen->closure;
    msScrPriv *scrpriv = screen->driver;

    free(scrpriv->render_modifiers);
    scrpriv->render_modifiers = NULL;
    scrpriv->num_render_modifiers = 0;
    KdGlamorFini(pScreen, &config->glamor_info);
}
