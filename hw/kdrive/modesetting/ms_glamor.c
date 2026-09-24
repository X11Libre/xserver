/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111github@gmail.com>
 */

#include <kdrive-config.h>

#include "modesetting.h"
#include "kglamor.h"

#include "glamor.h"

#include <drm_fourcc.h>

static Bool
msGlamorTryNewFront(ScreenPtr pScreen, Bool strip_modifiers, Bool need_map)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msPriv *priv = screen->card->driver;
    msScrPriv *scrpriv = screen->driver;
    KdFrameBuffer saved_framebuffer = screen->fb;

    /* TODO: query glamor */
    MsScreenConf *config = screen->closure;
    Bool is_gles = config->glamor_info.force_es;

    struct gbm_bo *old_front;
    struct gbm_bo *new_front = NULL;

    if (strip_modifiers) {
        if (!need_map && !scrpriv->allow_modifier_strip) {
            return FALSE;
        }
        screen->driver = NULL;
    }

    old_front = scrpriv->front;
    new_front = modesetting_open(priv, screen, need_map, TRUE /* keep_depth */);
    screen->driver = scrpriv;
    if (!new_front) {
        return FALSE;
    }

    gbm_bo_set_screen_fb_info(new_front, screen, is_gles);
    if (memcmp(&saved_framebuffer, &screen->fb, sizeof(screen->fb))) {
        /* Visual masks changed, the bo is unusable */
        /* XXX We could still use this if we had a way to change visual masks this late */
        screen->fb = saved_framebuffer;
        gbm_bo_destroy(new_front);
        LogMessage(X_ERROR, "Xmodesetting(%d): Cannot use a new front with different visual masks\n", pScreen->myNum);
        return FALSE;
    }

    if (!msSetScreenBo(pScreen, new_front, TRUE /* flip */)) {
        gbm_bo_destroy(new_front);
        LogMessage(X_ERROR, "Xmodesetting(%d): Could not flip to the new front bo\n", pScreen->myNum);
        return FALSE;
    }

    if (!gbm_bo_get_map(new_front)) {
        PixmapPtr rootPixmap = (*pScreen->GetScreenPixmap)(pScreen);
        Bool used_modifiers = gbm_bo_get_used_modifiers(new_front);

        if (!glamor_egl_create_textured_pixmap_from_gbm_bo(rootPixmap, new_front, used_modifiers)) {
            /* Put the old front back, destroy the new front */
            msSetScreenBo(pScreen, old_front, FALSE /* flip */);
            LogMessage(X_ERROR, "Xmodesetting(%d): Could not texture the front bo\n", pScreen->myNum);
            return FALSE;
        }
    }

    gbm_bo_destroy(old_front);
    return TRUE;
}

static Bool
msGlamorTileFront(ScreenPtr pScreen, Bool map_fallback)
{
    if (msGlamorTryNewFront(pScreen, FALSE /* strip_modifiers */, FALSE /* need_map */) ||
        msGlamorTryNewFront(pScreen, TRUE /* strip_modifiers */, FALSE /* need_map */)) {
        return TRUE;
    }

    LogMessage(X_ERROR, "Xmodesetting(%d): Cannot use a textured gbm front, using a cpu-mapped front buffer\n", pScreen->myNum);

    if (map_fallback) {
        if (!msGlamorTryNewFront(pScreen, FALSE /* strip_modifiers */, TRUE /* need_map */)) {
            msGlamorTryNewFront(pScreen, TRUE /* strip_modifiers */, TRUE /* need_map */);
        }
    }
    return FALSE;
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
        Bool map_fallback = !gbm_bo_get_map(scrpriv->front);
        if (!msGlamorTileFront(pScreen, map_fallback) &&
            !gbm_bo_get_map(scrpriv->front)) {
            LogMessage(X_ERROR, "Xmodesetting(%d): Could not create a usable front buffer\n", pScreen->myNum);
            return FALSE;
        }
    }

    if (gbm_bo_get_map(scrpriv->front)) {
        LogMessage(X_INFO, "Xmodesetting(%d): Using a cpu mapped front buffer\n", pScreen->myNum);
    } else {
        LogMessage(X_INFO, "Xmodesetting(%d): Using a textured front buffer\n", pScreen->myNum);
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
    int caps = GLAMOR_EGL_CAP_NONE;
    int write_pos = 0;

    if (!config->glamor_info.dri_path) {
        config->glamor_info.dri_fd = dup(gbm_device_get_fd(priv->gbm));
        if (config->glamor_info.dri_fd >= 0) {
            config->glamor_info.want_dri_fd = TRUE;
        } else {
            config->glamor_info.dri_path = screen->card->closure;
        }
    }

    if (!KdGlamorInit(pScreen, &config->glamor_info, &caps)) {
        return FALSE;
    }

    /* Workaround for https://gitlab.freedesktop.org/mesa/mesa/-/work_items/14475#note_3659774 */
    scrpriv->allow_modifier_strip = !!(caps & GLAMOR_EGL_CAP_TEXTURE_GBM_BO);

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
    for (int i = 0; i < scrpriv->num_render_modifiers; i++) {
        if (gbm_device_get_format_modifier_plane_count(priv->gbm, format, scrpriv->render_modifiers[i]) > 1) {
            continue;
        }
        scrpriv->render_modifiers[write_pos++] = scrpriv->render_modifiers[i];
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
