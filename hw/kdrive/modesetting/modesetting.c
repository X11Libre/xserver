/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111github@gmail.com>
 */

#include <kdrive-config.h>

#include "modesetting.h"
#include <sys/ioctl.h>

#include <errno.h>

#ifdef XV
#include "kxv.h"
#endif

struct gbm_bo*
modesetting_open(KdScreenInfo *screen, Bool need_map, Bool keep_depth)
{
    struct gbm_bo *ret = NULL;

#ifdef GLAMOR
    msScrPriv *scrpriv = screen->driver;
    MsScreenConf *config = screen->closure;
    Rotation randr = scrpriv ? scrpriv->randr : screen->randr;

    if (screen->dumb) {
        need_map = TRUE;
    } else if (config->no_tile) {
        need_map = TRUE;
    } else if (randr != RR_Rotate_0) {
        need_map = TRUE;
    }
#endif

    while (!ret) {
#ifdef GLAMOR
        if (!need_map) {
            ret = gbm_create_front_for_screen(screen, FALSE /* do_map */, config->format_swap);
        }
#endif
        if (!ret) {
            ret = gbm_create_front_for_screen(screen, TRUE /* do_map */, config->format_swap);
        }

        if (!ret) {
            int old_depth = screen->fb.depth;
            int old_bpp = screen->fb.bitsPerPixel;

            if (keep_depth) {
                break;
            } else if (screen->fb.depth > 30) {
                screen->fb.depth = 30;
                screen->fb.bitsPerPixel = 32;
            } else if (screen->fb.depth > 24) {
                screen->fb.depth = 24;
                screen->fb.bitsPerPixel = 32;
            } else if (screen->fb.depth == 24 && screen->fb.bitsPerPixel == 32) {
                screen->fb.depth = 24;
                screen->fb.bitsPerPixel = 24;
            } else if (screen->fb.depth > 16) {
                screen->fb.depth = 16;
                screen->fb.bitsPerPixel = 16;
            } else if (screen->fb.depth > 8) {
                screen->fb.depth = 8;
                screen->fb.bitsPerPixel = 8;
            } else {
                break;
            }
            LogMessage(X_ERROR, "Xmodesetting(card: %d, screen: %d): Cannot use a depth %d/%d front, trying again with depth %d/%d\n",
                       screen->card->mynum, screen->mynum, old_depth, old_bpp, screen->fb.depth, screen->fb.bitsPerPixel);
        }
    }

    return ret;
}

static Bool
msInitialize(KdCardInfo * card, msPriv * priv)
{
    const char *dev_path = card->closure;
    int fd;

    if (dev_path) {
        fd = open(dev_path, O_RDWR);
        if (fd < 0) {
            LogMessage(X_ERROR, "Xmodesetting(card %d): Error opening KMS device %s: %s\n",
                       card->mynum, dev_path, strerror(errno));
            goto bail;
        }
        LogMessage(X_INFO, "Xmodesetting(card %d): Using KMS device: %s\n",
                   card->mynum, dev_path);
    } else {
        char devbuf[] = "/dev/dri/cardxx";
        fd = -1;
        for (int i = 0; i < 64 && (fd < 0); i++) {
            snprintf(devbuf, sizeof(devbuf),
                     "/dev/dri/card%d", i);
            fd = open(devbuf, O_RDWR);

            if (fd >= 0) {
                uint64_t check_dumb = 0;

                if (drmGetCap(fd, DRM_CAP_DUMB_BUFFER, &check_dumb) >= 0 && check_dumb) {
                    break;
                }

                close(fd);
                fd = -1;
            }
        }
        if (fd < 0) {
            LogMessage(X_ERROR, "Xmodesetting(card %d): Error opening kms devices /dev/dri/card[0-63]\n", card->mynum);
            goto bail;
        }
        LogMessage(X_INFO, "Xmodesetting(card %d): Using kms device: %s\n", card->mynum, devbuf);
    }

    priv->gbm = gbm_create_device(fd);
    if (!priv->gbm) {
        LogMessage(X_ERROR, "Xmodesetting(card %d): Could not create a gbm device\n", card->mynum);
        goto bail;
    }

    drmSetMaster(fd);

    priv->resources = drmModeGetResources(fd);
    if (!priv->resources) {
        LogMessage(X_ERROR, "Xmodesetting(card %d): Could not get drm resources: %s\n", card->mynum, strerror(errno));
        goto bail;
    }

    return TRUE;

bail:
    if (priv->gbm) {
        gbm_device_destroy(priv->gbm);
        priv->gbm = NULL;
    }
    if (fd >= 0) {
        close(fd);
    }
    return FALSE;
}

Bool
msCardInit(KdCardInfo * card)
{
    msPriv *priv;

    priv = calloc(1, sizeof(msPriv));
    if (!priv)
        return FALSE;

    if (!msInitialize(card, priv)) {
        free(priv);
        return FALSE;
    }
    card->driver = priv;

    return TRUE;
}

static void
modesetting_claim_connector_crtc(msPriv *priv, uint32_t conn, int crtc)
{
    Bool need_new_conn = TRUE;
    Bool need_new_crtc = TRUE;

    for (int i = 0; i < priv->num_used_connectors; i++) {
        if (priv->used_connectors[i] == conn) {
            need_new_conn = FALSE;
            break;
        }
    }

    for (int i = 0; i < priv->num_used_crtcs; i++) {
        if (priv->used_crtcs[i] == crtc) {
            need_new_crtc = FALSE;
            break;
        }
    }

    if (need_new_conn) {
        void *tmp = realloc(priv->used_connectors, (priv->num_used_connectors + 1) * sizeof(*priv->used_connectors));
        if (tmp) {
            priv->used_connectors = tmp;
            priv->used_connectors[priv->num_used_connectors] = conn;
            priv->num_used_connectors++;
        }
    }

    if (need_new_crtc) {
        void *tmp = realloc(priv->used_crtcs, (priv->num_used_crtcs + 1) * sizeof(*priv->used_crtcs));
        if (tmp) {
            priv->used_crtcs = tmp;
            priv->used_crtcs[priv->num_used_crtcs] = crtc;
            priv->num_used_crtcs++;
        }
    }
}

static Bool
msScreenInitialize(KdScreenInfo * screen, msScrPriv * scrpriv)
{
    msPriv *priv = screen->card->driver;
    int fd = gbm_device_get_fd(priv->gbm);

    scrpriv->connector = modesetting_find_connector(priv, fd, &scrpriv->conn_id);
    if (!scrpriv->connector) {
        LogMessage(X_ERROR, "Xmodesetting(card %d, screen %d): Could not find a usable connector\n",
                   screen->card->mynum, screen->mynum);
        goto fail;
    }

    scrpriv->crtc_id = modeseting_find_crtc(priv, fd, scrpriv->connector);
    if (scrpriv->crtc_id < 0) {
        LogMessage(X_ERROR, "Xmodesetting(card %d, screen %d): Could not find a suitable crtc\n",
                   screen->card->mynum, screen->mynum);
        goto fail;
    }

    scrpriv->crtc = drmModeGetCrtc(fd, scrpriv->crtc_id);

    scrpriv->mode = modesetting_find_mode(scrpriv->connector, screen->width, screen->height, screen->rate);
    if (!scrpriv->mode) {
        LogMessage(X_WARNING, "Xmodesetting(card %d, screen %d): Could not find a supported mode\n",
                   screen->card->mynum, screen->mynum);
        LogMessage(X_WARNING, "Xmodesetting(card %d, screen %d): This likely means that this output is not connected, or the requested mode is not supported\n",
                   screen->card->mynum, screen->mynum);
    }

    /* modesetting_open allocates the bo based on this */
    if (!screen->width || !screen->height) {
        screen->width = scrpriv->mode ? scrpriv->mode->hdisplay : 1920;
        screen->height = scrpriv->mode ? scrpriv->mode->vdisplay : 1080;
    }

    scrpriv->front = modesetting_open(screen, TRUE /* need_map */, FALSE /* keep_depth */);
    if (!scrpriv->front) {
        LogMessage(X_ERROR, "Xmodesetting(card %d, screen %d): Could not create a front buffer\n",
                   screen->card->mynum, screen->mynum);
        goto fail;
    }

    screen->width = gbm_bo_get_width(scrpriv->front);
    screen->height = gbm_bo_get_height(scrpriv->front);
    screen->rate = scrpriv->mode ? scrpriv->mode->vrefresh : 0; /* XXX 0 means accept any rate on msEnable */

    gbm_bo_set_screen_fb_info(scrpriv->front, screen, FALSE /* is_gles */);

    /* Make fbSetupScreen happy */
    if (screen->fb.bitsPerPixel == 24) {
        screen->fb.bitsPerPixel = 32;
        scrpriv->is_24bpp = TRUE;
    }

    scrpriv->randr = screen->randr;
    if (!msMapFramebuffer(screen)) {
        goto fail;
    }

    /* Mark the connector and crtc used */
    modesetting_claim_connector_crtc(priv, scrpriv->conn_id, scrpriv->crtc_id);

    return TRUE;

fail:
    if (scrpriv->front) {
        gbm_bo_destroy(scrpriv->front);
    }

    if (scrpriv->connector) {
        drmModeFreeConnector(scrpriv->connector);
    }

    return FALSE;
}

Bool
msScreenInit(KdScreenInfo * screen)
{
    msScrPriv *scrpriv;

    scrpriv = calloc(1, sizeof(msScrPriv));
    if (!scrpriv)
        return FALSE;

    screen->driver = scrpriv;
    if (!msScreenInitialize(screen, scrpriv)) {
        screen->driver = 0;
        free(scrpriv);
        return FALSE;
    }
    return TRUE;
}

static void*
msWindowLinear(ScreenPtr pScreen,
               CARD32 row,
               CARD32 offset, int mode, CARD32 * size, void *closure)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *priv = screen->driver;
    CARD8 *mem;

    if (!pScreenPriv->enabled)
        return NULL;

    *size = gbm_bo_get_stride(priv->front);
    mem = gbm_bo_get_map(priv->front);
    return mem + row * (*size) + offset;
}

Bool
msMapFramebuffer(KdScreenInfo * screen)
{
    msScrPriv *scrpriv = screen->driver;
    KdPointerMatrix m;
    MsScreenConf *config = screen->closure;

    if (!gbm_bo_get_map(scrpriv->front)) {
        /* TODO: Do something useful */
        if (scrpriv->randr != RR_Rotate_0) {
            return FALSE;
        }
        scrpriv->shadow = FALSE;
    } else if (scrpriv->is_24bpp) {
        scrpriv->shadow = TRUE;
    } else if (config->shadow >= 0) {
        scrpriv->shadow = config->shadow;
    } else if (scrpriv->randr != RR_Rotate_0) {
        scrpriv->shadow = TRUE;
    } else {
        scrpriv->shadow = FALSE;
    }

    KdComputePointerMatrix(&m, scrpriv->randr, screen->width, screen->height);

    KdSetPointerMatrix(&m);

    screen->width = gbm_bo_get_width(scrpriv->front);
    screen->height = gbm_bo_get_height(scrpriv->front);

    if (scrpriv->shadow) {
        if (!KdShadowFbAlloc(screen,
                             scrpriv->randr & (RR_Rotate_90 | RR_Rotate_270))) {
            return FALSE;
        }
    } else {
        unsigned long stride = gbm_bo_get_stride(scrpriv->front);
        int cpp = (gbm_bo_get_bpp(scrpriv->front) + 7) / 8;
        screen->fb.byteStride = stride;
        screen->fb.pixelStride = stride / cpp;
        screen->fb.frameBuffer = gbm_bo_get_map(scrpriv->front);
    }

    return TRUE;
}

Bool
msUnmapFramebuffer(KdScreenInfo * screen)
{
    KdShadowFbFree(screen);
    return TRUE;
}

Bool
msSetShadow(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *scrpriv = screen->driver;
    ShadowUpdateProc update;
    ShadowWindowProc window;

    window = msWindowLinear;
    update = 0;

    if (scrpriv->is_24bpp == 24)
        update = shadowUpdate32to24;
    else if (scrpriv->randr != RR_Rotate_0)
        update = shadowUpdateRotatePacked;
    else
        update = shadowUpdatePacked;
    return KdShadowSet(pScreen, scrpriv->randr, update, window);
}

Bool
msInitScreen(ScreenPtr pScreen)
{
    pScreen->CreateColormap = fbInitializeColormap;
    return TRUE;
}

Bool
msFinishInitScreen(ScreenPtr pScreen)
{
    if (!shadowSetup(pScreen)) {
        return FALSE;
    }
#ifdef RANDR
    if (!msRandRInit(pScreen)) {
        return FALSE;
    }
#endif
    return TRUE;
}

Bool
msCreateResources(ScreenPtr pScreen)
{
    if (!msSetShadow(pScreen)) {
        return FALSE;
    }

#ifdef GLAMOR
    if (!msGlamorCreateRes(pScreen)) {
        return FALSE;
    }
#endif

    if (!msDamageCreateRes(pScreen)) {
        return FALSE;
    }

    return TRUE;
}

void
msPreserve(KdCardInfo * card)
{
}

Bool
msEnable(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *priv = screen->driver;

    struct gbm_device *gbm = gbm_bo_get_device(priv->front);
    uint32_t fb_id = gbm_bo_get_fb(priv->front);
    int fd = gbm_device_get_fd(gbm);

    drmSetMaster(fd);

    if (!priv->mode) {
        priv->mode = modesetting_find_mode(priv->connector, screen->width, screen->height, screen->rate);
    }
    if (priv->mode && drmModeSetCrtc(fd, priv->crtc_id, fb_id, 0, 0, &priv->conn_id, 1, priv->mode)) {
        return FALSE;
    }

#ifdef XV
    KdXVEnable (pScreen);
#endif

    return TRUE;
}

Bool
msDPMS(ScreenPtr pScreen, int mode)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *priv = screen->driver;

    struct gbm_device *gbm = gbm_bo_get_device(priv->front);
    uint32_t fb_id = gbm_bo_get_fb(priv->front);
    int fd = gbm_device_get_fd(gbm);

    if (mode == KD_DPMS_NORMAL) {
        return !priv->mode || !drmModeSetCrtc(fd, priv->crtc_id, fb_id, 0, 0, &priv->conn_id, 1, priv->mode);
    }

    return !drmModeSetCrtc(fd, priv->crtc_id, 0, 0, 0, NULL, 0, NULL);
}

void
msDisable(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *priv = screen->driver;

    struct gbm_device *gbm = gbm_bo_get_device(priv->front);
    int fd = gbm_device_get_fd(gbm);

#ifdef XV
    KdXVDisable (pScreen);
#endif

    drmDropMaster(fd);
}

void
msRestore(KdCardInfo * card)
{
}

void
msScreenFini(KdScreenInfo * screen)
{
    msScrPriv *priv = screen->driver;

    gbm_bo_destroy(priv->front);
    if (priv->crtc) {
        drmModeFreeCrtc(priv->crtc);
    }
    drmModeFreeConnector(priv->connector);

    free(priv);
    screen->driver = NULL;
}

void
msCardFini(KdCardInfo * card)
{
    msPriv *priv = card->driver;

    struct gbm_device *gbm = priv->gbm;
    int fd = gbm_device_get_fd(gbm);

    free(priv->used_crtcs);
    free(priv->used_connectors);
    drmModeFreeResources(priv->resources);
    gbm_device_destroy(gbm);
    close(fd);
    free(priv);
    card->driver = NULL;
}
