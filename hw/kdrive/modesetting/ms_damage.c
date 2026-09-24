/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111github@gmail.com>
 */

#include <kdrive-config.h>

#include "modesetting.h"

#include <errno.h>

/* Track and report front buffer damage to the kernel */

/* Heavily inspired from the Xorg modesetting driver */

static void
msFixupDamageRegion(msScrPriv *scrpriv, int w, int h, drmModeClip *clip, BoxPtr rect)
{
    Rotation randr = scrpriv->randr & (RR_Rotate_0 | RR_Rotate_90 | RR_Rotate_180 | RR_Rotate_270);

    switch (randr) {
    case RR_Rotate_0:
        clip->x1 = rect->x1;
        clip->x2 = rect->x2;
        clip->y1 = rect->y1;
        clip->y2 = rect->y2;
        break;
    case RR_Rotate_90:
        clip->x1 = rect->y1;
        clip->y1 = w - rect->x2;
        clip->x2 = rect->y2;
        clip->y2 = w - rect->x1;
        break;
    case RR_Rotate_180:
        clip->x1 = w - rect->x2;
        clip->y1 = h - rect->y2;
        clip->x2 = w - rect->x1;
        clip->y2 = h - rect->y1;
        break;
    case RR_Rotate_270:
        clip->x1 = h - rect->y2;
        clip->y1 = rect->x1;
        clip->x2 = h - rect->y1;
        clip->y2 = rect->x2;
        break;
    default:
        clip->x1 = 0;
        clip->y1 = 0;
        clip->x2 = w;
        clip->y2 = h;
        break;
    }
}

static void
msDamageBlockHandler(void *blockData, void *timeout)
{
    ScreenPtr pScreen = blockData;
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *priv = screen->driver;
    RegionPtr dirty;
    drmModeClip full_clip;
    struct gbm_device *gbm;
    uint32_t fb_id;
    int fd;
    uint32_t width;
    uint32_t height;
    unsigned num_cliprects;

    gbm = gbm_bo_get_device(priv->front);
    fb_id = gbm_bo_get_fb(priv->front);
    fd = gbm_device_get_fd(gbm);

    width = gbm_bo_get_width(priv->front);
    height = gbm_bo_get_height(priv->front);

    full_clip = (drmModeClip){.x1 = 0, .y1 = 0, .x2 = width, .y2 = height,};

    if (!priv->damage) {
        goto bail;
    }

    dirty = DamageRegion(priv->damage);

    num_cliprects = REGION_NUM_RECTS(dirty);

    if (num_cliprects) {
        drmModeClip *clip = calloc(num_cliprects, sizeof(drmModeClip));
        BoxPtr rect = REGION_RECTS(dirty);
        int ret;

        if (!clip) {
            goto bail;
        }

        for (int i = 0; i < num_cliprects; i++) {
            msFixupDamageRegion(priv, width, height, &clip[i], &rect[i]);
        }

        /* TODO query connector property to see if this is needed */
        ret = drmModeDirtyFB(fd, fb_id, clip, num_cliprects);

        /* if we're swamping it with work, try one at a time */
        if (ret) {
            for (int i = 0; i < num_cliprects; i++) {
                ret = drmModeDirtyFB(fd, fb_id, &clip[i], 1);
                if (ret) {
                    break;
                }
            }
        }

        free(clip);

        if (ret) {
            goto bail;
        }
    }

    DamageEmpty(priv->damage);
    return;

bail:
    drmModeDirtyFB(fd, fb_id, &full_clip, 1);
    if (priv->damage) {
        DamageEmpty(priv->damage);
    }
}

static void
msDamageWakeupHandler(void *blockData, int result)
{
}

Bool
msDamageCreateRes(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *priv = screen->driver;
    PixmapPtr rootPixmap = pScreen->GetScreenPixmap(pScreen);
    struct gbm_device *gbm;
    uint32_t fb_id;
    int fd;

    gbm = gbm_bo_get_device(priv->front);
    fb_id = gbm_bo_get_fb(priv->front);
    fd = gbm_device_get_fd(gbm);

    /* Damage tracking not supported/needed */
    if (drmModeDirtyFB(fd, fb_id, NULL, 0) &&
        ((errno == EINVAL) || (errno == ENOSYS))) {
        return TRUE;
    }

    priv->damage = DamageCreate(NULL, NULL, DamageReportNone, TRUE,
                                pScreen, rootPixmap);

    if (priv->damage) {
        DamageRegister(&rootPixmap->drawable, priv->damage);
    }

    if (!RegisterBlockAndWakeupHandlers(msDamageBlockHandler, msDamageWakeupHandler, pScreen)) {
        if (priv->damage) {
            DamageUnregister(priv->damage);
            DamageDestroy(priv->damage);
            priv->damage = NULL;
        }
        return FALSE;
    }

    priv->blockHandler = TRUE;

    return TRUE;
}

void
msDamageCloseScreen(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *priv = screen->driver;

    if (!priv->blockHandler) {
        return;
    }

    RemoveBlockAndWakeupHandlers(msDamageBlockHandler, msDamageWakeupHandler, pScreen);
    if (priv->damage) {
        DamageUnregister(priv->damage);
        DamageDestroy(priv->damage);
    }
}
