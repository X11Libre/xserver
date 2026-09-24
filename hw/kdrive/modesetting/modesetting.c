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
modesetting_open(msPriv *priv, KdScreenInfo *screen, Bool need_map)
{
#ifdef GLAMOR
    MsScreenConf *config = screen->closure;
#endif
    struct gbm_bo *ret = NULL;
    uint32_t format, format_swap;

    /*
     *  XXX This all simplifies with modifier support
     *
     * Just query the renderable modifiers on the render card,
     * and allocate a gbm bo on the scanout card using those modifiers.
     *
     * If that fails, fall back to dumb buffers.
     */

#ifdef GLAMOR
    if (screen->dumb) {
        need_map = TRUE;
    } else if (config->glamor_info.dri_path) {
        /*
         * Until modifier support, we want to be conservative here.
         *
         * If the render card is different fron the scanout card, assume the modifier sets are disjoint.
         * If the cards are the same, and it is from nvidia, and the gbm backend is mesa or dumb,
         * assume that buffers are not renderable.
         */
        int dri_fd = open(config->glamor_info.dri_path, O_RDWR);
        if (!msFdMatch(gbm_device_get_fd(priv->gbm), dri_fd)) {
            need_map = TRUE;
        }

        if (dri_fd >= 0) {
            close(dri_fd);
        }
    } else {
        drmVersionPtr version;
        Bool is_nvidia = TRUE;
        Bool backend_is_mesa = FALSE;
        Bool linear_only = FALSE;
        const char *backend_name;

        version = drmGetVersion(gbm_device_get_fd(priv->gbm));
        if (version) {
            is_nvidia = !version->name || !strcmp(version->name, "nvidia-drm");
            drmFreeVersion(version);
        }
        backend_name = gbm_device_get_backend_name(priv->gbm);
        if (!backend_name) {
            linear_only = TRUE;
        } else if (!strcmp(backend_name, "dumb")) {
            linear_only = TRUE;
        } else if (!strcmp(backend_name, "drm")) {
            backend_is_mesa = TRUE;
        }

        /**
         * Nvidia's egl libraries do not allow creating GL_TEXTURE_2D textures from linear buffers.
         *
         * See: https://gitlab.freedesktop.org/xorg/xserver/-/work_items/1444
         */
        if (is_nvidia) {
            if (linear_only || backend_is_mesa) {
                need_map = TRUE;
            }
        }
    }
#endif

    while (!ret) {
        format = gbm_front_format_for_depth(screen->fb.depth, screen->fb.bitsPerPixel, FALSE /* rb_swap */);
        format_swap = gbm_front_format_for_depth(screen->fb.depth, screen->fb.bitsPerPixel, TRUE /* rb_swap */);

        if (config->format_swap) {
            uint32_t tmp = format;
            format = format_swap;
            format_swap = tmp;
        }

#ifdef GLAMOR
        if (!need_map) {
            if (!ret) {
                ret = gbm_create_front_bo(priv->gbm, FALSE /* do map */, screen->width, screen->height, format);
            }
            if (!ret) {
                ret = gbm_create_front_bo(priv->gbm, FALSE /* do map */, screen->width, screen->height, format_swap);
            }
        }
#endif

        if (!ret) {
            ret = gbm_create_front_bo(priv->gbm, TRUE /* do map */, screen->width, screen->height, format);
        }
        if (!ret) {
            ret = gbm_create_front_bo(priv->gbm, TRUE /* do map */, screen->width, screen->height, format_swap);
        }

        if (!ret) {
            int old_depth = screen->fb.depth;
            int old_bpp = screen->fb.bitsPerPixel;
            if (screen->fb.depth > 30) {
                screen->fb.depth = 30;
                screen->fb.bitsPerPixel = 24;
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
#ifdef GLAMOR
    MsScreenConf *config = screen->closure;
#endif
    msPriv *priv = screen->card->driver;
    int fd = gbm_device_get_fd(priv->gbm);
    uint32_t format;
    Bool rb_swap = FALSE;

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

    scrpriv->front = modesetting_open(priv, screen, FALSE /* need_map */);
    if (!scrpriv->front) {
        LogMessage(X_ERROR, "Xmodesetting(card %d, screen %d): Could not create a front buffer\n",
                   screen->card->mynum, screen->mynum);
        goto fail;
    }

    screen->width = gbm_bo_get_width(scrpriv->front);
    screen->height = gbm_bo_get_height(scrpriv->front);
    screen->rate = scrpriv->mode ? scrpriv->mode->vrefresh : 0; /* XXX 0 means accept any rate on msEnable */

    format = gbm_bo_get_format(scrpriv->front);

    switch (format) {
    case GBM_FORMAT_C8:
    case GBM_FORMAT_R8:
        screen->fb.depth = 8;
        screen->fb.bitsPerPixel = 8;
        screen->fb.visuals = (1 << GrayScale);
        screen->fb.redMask = 0xff;
        screen->fb.greenMask = 0x00;
        screen->fb.blueMask = 0x00;
        break;
    case GBM_FORMAT_XBGR1555:
        rb_swap = TRUE;
    case GBM_FORMAT_XRGB1555:
        screen->fb.depth = 15;
        screen->fb.bitsPerPixel = 16;
        screen->fb.visuals = (1 << TrueColor);
        screen->fb.redMask = 0x1f << 10;
        screen->fb.greenMask = 0x1f << 5;
        screen->fb.blueMask = 0x1f;
        break;
    case GBM_FORMAT_BGR565:
        rb_swap = TRUE;
    case GBM_FORMAT_RGB565:
        screen->fb.depth = 16;
        screen->fb.bitsPerPixel = 16;
        screen->fb.visuals = (1 << TrueColor);
        screen->fb.redMask = 0x1f << 11;
        screen->fb.greenMask = 0x3f << 5;
        screen->fb.blueMask = 0x1f;
        break;
    case GBM_FORMAT_BGR888:
        rb_swap = TRUE;
    case GBM_FORMAT_RGB888:
        screen->fb.depth = 24;
        screen->fb.bitsPerPixel = 24;
        screen->fb.visuals = (1 << TrueColor);
        screen->fb.redMask = 0xff << 16;
        screen->fb.greenMask = 0xff << 8;
        screen->fb.blueMask = 0xff;
        break;
    case GBM_FORMAT_XBGR8888:
        rb_swap = TRUE;
    case GBM_FORMAT_XRGB8888:
        screen->fb.depth = 24;
        screen->fb.bitsPerPixel = 32;
        screen->fb.visuals = (1 << TrueColor);
        screen->fb.redMask = 0xff << 16;
        screen->fb.greenMask = 0xff << 8;
        screen->fb.blueMask = 0xff;
        break;
    case GBM_FORMAT_XBGR2101010:
        rb_swap = TRUE;
    case GBM_FORMAT_XRGB2101010:
        screen->fb.depth = 30;
        screen->fb.bitsPerPixel = 32;
        screen->fb.visuals = (1 << TrueColor);
        screen->fb.redMask = 0x3ff << 20;
        screen->fb.greenMask = 0x3ff <<10;
        screen->fb.blueMask = 0x3ff;
        break;
    }

    /* XXX Tiled buffers don't need r-b swap, unless it's depth 30 on gles */
    if (!gbm_bo_get_map(scrpriv->front)) {
        if (screen->fb.depth != 30 ||
            !config->glamor_info.force_es) {
            rb_swap = FALSE;
        }
    }

    if (rb_swap) {
        int tmp = screen->fb.blueMask;
        screen->fb.blueMask = screen->fb.redMask;
        screen->fb.redMask = tmp;
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
        scrpriv->shadow = FALSE;
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

    if (screen->fb.bitsPerPixel == 24)
        update = shadowUpdate32to24;
    else if (scrpriv->randr)
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

/* Heavily inspired from the Xorg modesetting driver */
static void
msBlockHandler(void *blockData, void *timeout)
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
msWakeupHandler(void *blockData, int result)
{
}

Bool
msCreateResources(ScreenPtr pScreen)
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

    if (!msSetShadow(pScreen)) {
        return FALSE;
    }

#ifdef GLAMOR
    if (!msGlamorCreateRes(pScreen)) {
        return FALSE;
    }
#endif

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

    if (!RegisterBlockAndWakeupHandlers(msBlockHandler, msWakeupHandler, pScreen)) {
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

void
msCloseScreen(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *priv = screen->driver;

    if (!priv->blockHandler) {
        return;
    }

    RemoveBlockAndWakeupHandlers(msBlockHandler, msWakeupHandler, pScreen);
    if (priv->damage) {
        DamageUnregister(priv->damage);
        DamageDestroy(priv->damage);
    }
}
