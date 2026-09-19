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


static void
msSetScreenSizes(ScreenPtr pScreen);

static Bool
msFdMatch(int fd1, int fd2);

static int
modesetting_grade_mode(drmModeModeInfo *mode, uint32_t req_w, uint32_t req_h, uint32_t req_rate)
{
    int score = 1;

    if (req_w && (req_w == mode->hdisplay)) {
        score += 10;
    }

    if (req_h && (req_h == mode->vdisplay)) {
        score += 10;
    }

    if (req_rate && (req_rate == mode->vrefresh)) {
        score += 5;
    }

    if (mode->type & DRM_MODE_TYPE_PREFERRED) {
        score++;
    }

    return score;
}

static drmModeModeInfo*
modesetting_find_mode(drmModeConnector *conn, uint32_t req_w, uint32_t req_h, uint32_t req_rate)
{
    drmModeModeInfo *best_mode = NULL;
    int best_score = 0;

    for (int i = 0; i < conn->count_modes; i++) {
        drmModeModeInfo *mode = &conn->modes[i];
        int score;

        score = modesetting_grade_mode(mode, req_w, req_h, req_rate);
        if (score <= best_score) {
            continue;
        }

        best_mode = mode;
        best_score = score;
    }

    return best_mode;
}

static int
modesetting_grade_connector(msPriv *priv, drmModeConnector *conn, uint32_t conn_id)
{
    int score = 1;
    Bool in_use = FALSE;

    if (conn->modes && conn->count_modes) {
        score += 5;
    }

    switch(conn->connection) {
    case DRM_MODE_CONNECTED:
        score++;
    case DRM_MODE_UNKNOWNCONNECTION:
        score++;
    case DRM_MODE_DISCONNECTED:
        score++;
    }

    for(int i = 0; i < priv->num_used_connectors; i++) {
        if (priv->used_connectors[i] == conn_id) {
            in_use = TRUE;
            break;
        }
    }

    if (!in_use) {
        score += 10;
    }

    return score;
}

static drmModeConnector*
modesetting_find_connector(msPriv *priv, int fd, uint32_t *conn_id)
{
    drmModeConnector *best_connector = NULL;
    int best_score = 0;

    drmModeRes *res = priv->resources;

    for (int i = 0; i < res->count_connectors; i++) {
        drmModeConnector *conn;
        int id;
        int score;
        id = res->connectors[i];
        conn = drmModeGetConnector(fd, id);
        if (!conn) {
            continue;
        }

        score = modesetting_grade_connector(priv, conn, id);
        if (score <= best_score) {
            drmModeFreeConnector(conn);
            continue;
        }

        if (best_connector) {
            drmModeFreeConnector(best_connector);
        }

        best_connector = conn;
        *conn_id = id;
        best_score = score;
    }

    return best_connector;
}

static Bool
modesetting_crtc_is_used(msPriv *priv, int crtc)
{
    for (int i = 0; i < priv->num_used_crtcs; i++) {
        if (priv->used_crtcs[i] == crtc) {
            return TRUE;
        }
    }

    return FALSE;
}

/* Slightly modified rom man drm-kms */
static int
modeseting_find_crtc(msPriv *priv, int fd, drmModeConnector *conn)
{
    drmModeRes *res = priv->resources;
    drmModeEncoder *enc;
    unsigned int i, j;
    int crtc = -ENOENT;

    /* iterate all encoders of this connector */
    for (i = 0; i < conn->count_encoders; ++i) {
        enc = drmModeGetEncoder(fd, conn->encoders[i]);
        if (!enc) {
            /* cannot retrieve encoder, ignoring... */
            continue;
        }

        /* iterate all global CRTCs */
        for (j = 0; j < res->count_crtcs; ++j) {
            /* check whether this CRTC works with the encoder */
            if (!(enc->possible_crtcs & (1 << j)))
                continue;

            /* Here you need to check that no other connector
             * currently uses the CRTC with id "crtc". If you intend
             * to drive one connector only, then you can skip this
             * step. Otherwise, simply scan your list of configured
             * connectors and CRTCs whether this CRTC is already
             * used. If it is, then simply continue the search here. */
            if (!modesetting_crtc_is_used(priv, res->crtcs[j])) {
                drmModeFreeEncoder(enc);
                return res->crtcs[j];
            }

            /* Allow reusing crtcs for testing */
            if (crtc < 0) {
                crtc = res->crtcs[j];
            }
        }

        drmModeFreeEncoder(enc);
    }

    /* cannot find a suitable CRTC */
    return crtc;
}


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

Bool
msSetScreenBo(ScreenPtr pScreen, struct gbm_bo *bo, Bool flip)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *scrpriv = screen->driver;
    struct gbm_bo *old_front;
    Bool wasEnabled = pScreenPriv->enabled;
    msScrPriv oldscr;

    if (wasEnabled) {
        KdDisableScreen(pScreen);
    }

    oldscr = *scrpriv;

    old_front = scrpriv->front;

    msUnmapFramebuffer(screen);

    scrpriv->front = bo;

    if (!msMapFramebuffer(screen)) {
        goto bail;
    }

    KdShadowUnset(screen->pScreen);

    if (!msSetShadow(screen->pScreen)) {
        goto bail;
    }

    msSetScreenSizes(screen->pScreen);

    /*
     * Set frame buffer mapping
     */
    (*pScreen->ModifyPixmapHeader) ((*pScreen->GetScreenPixmap)(pScreen),
                                    pScreen->width,
                                    pScreen->height,
                                    screen->fb.depth,
                                    screen->fb.bitsPerPixel,
                                    screen->fb.byteStride,
                                    screen->fb.frameBuffer);

    /* set the subpixel order */

    KdSetSubpixelOrder(pScreen, scrpriv->randr);

    /* Scan out the new bo on the screen's crtc */
    if (wasEnabled) {
        KdEnableScreen(pScreen);
    }

    if (!flip) {
        gbm_bo_destroy(old_front);
    }
    return TRUE;

bail:
    msUnmapFramebuffer(screen);
    old_front = scrpriv->front;
    *scrpriv = oldscr;
    msMapFramebuffer(screen);
    msSetScreenSizes(screen->pScreen);

    if (wasEnabled) {
        KdEnableScreen(pScreen);
    }
    return FALSE;
}

static Bool
msSetMode(ScreenPtr pScreen, int width, int height, int rate)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msPriv *priv = screen->card->driver;
    msScrPriv *scrpriv = screen->driver;
    struct gbm_bo *new_front = NULL;
    drmModeModeInfo *old_mode;
    int old_width, old_height, old_rate;

    old_mode = scrpriv->mode;

    old_width = screen->width;
    old_height = screen->height;
    old_rate = screen->rate;

    /* Find the mode */
    scrpriv->mode = modesetting_find_mode(scrpriv->connector, width, height, rate);
    if (!scrpriv->mode) {
        goto bail;
    }

    screen->width = width;
    screen->height = height;
    screen->rate = rate;

    /* Create a new front with the new sizes */
    if (width != screen->width ||
        height != screen->height) {
        uint32_t format = gbm_bo_get_format(scrpriv->front);
        Bool do_map = !!gbm_bo_get_map(scrpriv->front);
        new_front = gbm_create_front_bo(priv->gbm, do_map, width, height, format);
        if (!new_front ||
            !msSetScreenBo(pScreen, new_front, FALSE /* flip */)) {
            goto bail;
        }
    }

    return TRUE;

bail:
    if (new_front) {
        gbm_bo_destroy(new_front);
    }

    screen->width = old_width;
    screen->height = old_height;
    screen->rate = old_rate;

    scrpriv->mode = old_mode;

    return FALSE;
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

/* XXX This really belongs in os/, like the version from glamor_egl */
static Bool
msFdMatch(int fd1, int fd2)
{
    struct stat stat1, stat2;

    if (fd1 == fd2) {
        return TRUE;
    }

    if (fd1 < 0 || fd2 < 0) {
        return FALSE;
    }

    if (fstat(fd1, &stat1) < 0 ||
        fstat(fd2, &stat2) < 0) {
        return FALSE;
    }

    /**
     * From https://pubs.opengroup.org/onlinepubs/009696699/basedefs/sys/stat.h.html
     *
     * The st_ino and st_dev fields taken together uniquely identify the file within the system.
     */
    return (stat1.st_dev == stat2.st_dev) && (stat1.st_ino == stat2.st_ino);
}

static KdCardInfo*
msFindCardForFd(int fd)
{
    for (KdCardInfo *card = kdCardInfo; card; card = card->next) {
        const char *card_path = card->closure;
        int cardFd = card_path ? open(card_path, O_RDWR) : -1;
        Bool ret = msFdMatch(cardFd, fd);

        if (cardFd >= 0) {
            close(cardFd);
        }
        if (ret) {
            return card;
        }
    }

    return NULL;
}

KdCardInfo*
msFindMatchingCard(const char *card_path)
{
    KdCardInfo *card;
    int fd = card_path ? open(card_path, O_RDWR) : -1;

    card = msFindCardForFd(fd);

    if (fd >= 0) {
        close(fd);
    }
    return card;
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

    unsigned long stride = gbm_bo_get_stride(scrpriv->front);

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
        screen->fb.byteStride = stride;
        screen->fb.pixelStride = stride / 4;
        screen->fb.frameBuffer = gbm_bo_get_map(scrpriv->front);
    }

    return TRUE;
}

static void
msSetScreenSizes(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *scrpriv = screen->driver;

    if (scrpriv->randr & (RR_Rotate_0 | RR_Rotate_180)) {
        pScreen->width = gbm_bo_get_width(scrpriv->front);
        pScreen->height = gbm_bo_get_height(scrpriv->front);
        pScreen->mmWidth = screen->width_mm;
        pScreen->mmHeight = screen->height_mm;
    } else {
        pScreen->width = gbm_bo_get_height(scrpriv->front);
        pScreen->height = gbm_bo_get_width(scrpriv->front);
        pScreen->mmWidth = screen->height_mm;
        pScreen->mmHeight = screen->width_mm;
    }
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

static Bool
msRandRGetInfo(ScreenPtr pScreen, Rotation * rotations)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *scrpriv = screen->driver;
    Rotation randr;
    int n;

    *rotations = RR_Rotate_All | RR_Reflect_All;

    for (n = 0; n < pScreen->numDepths; n++) {
        if (pScreen->allowedDepths[n].numVids) {
            break;
        }
    }

    if (n == pScreen->numDepths) {
        return FALSE;
    }

    randr = KdSubRotation(scrpriv->randr, screen->randr);

    for (int i = 0; i < scrpriv->connector->count_modes; i++) {
        drmModeModeInfo *mode = &scrpriv->connector->modes[i];
        RRScreenSizePtr pSize;
        pSize = RRRegisterSize(pScreen,
                               mode->hdisplay,
                               mode->vdisplay,
                               screen->width_mm, screen->height_mm);

        RRRegisterRate(pScreen, pSize, mode->vrefresh);

        if (mode->hdisplay == screen->width &&
            mode->vdisplay == screen->height &&
            mode->vrefresh == screen->rate) {
            RRSetCurrentConfig(pScreen, randr, mode->vrefresh, pSize);
        }
    }

    return TRUE;
}

static Bool
msRandRSetConfig(ScreenPtr pScreen,
		    Rotation randr, int rate, RRScreenSizePtr pSize)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *scrpriv = screen->driver;

    int oldmmwidth;
    int oldmmheight;
    int newmmwidth;
    int newmmheight;

    if (screen->randr & (RR_Rotate_0 | RR_Rotate_180)) {
        newmmwidth = pSize->mmWidth;
        newmmheight = pSize->mmHeight;
    } else {
        newmmwidth = pSize->mmHeight;
        newmmheight = pSize->mmWidth;
    }

    oldmmwidth = pScreen->mmWidth;
    oldmmheight = pScreen->mmHeight;

    /*
     * Set new configuration
     */

    scrpriv->randr = KdAddRotation(screen->randr, randr);

    pScreen->mmWidth = newmmwidth;
    pScreen->mmHeight = newmmheight;

    if (!msSetMode(pScreen, pSize->width, pSize->height, rate)) {
        goto bail;
    }

    return TRUE;

bail:
    pScreen->mmWidth = oldmmwidth;
    pScreen->mmHeight = oldmmheight;
    return FALSE;
}

#ifdef RANDR
static Bool
msGetPhysicalScreenSizes(ScreenPtr pScreen, int *mmWidth, int *mmHeight)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *scrpriv = screen->driver;

    *mmWidth = screen->width_mm;
    *mmHeight = screen->height_mm;

    if (screen->requested_mm) {
        return TRUE;
    }

    if (((int)scrpriv->connector->mmWidth > 0) && ((int)scrpriv->connector->mmHeight > 0)) {
        *mmWidth = scrpriv->connector->mmWidth;
        *mmWidth = scrpriv->connector->mmHeight;
        return TRUE;
    }

    return FALSE;
}

static Bool
msRandRSetPhysicalScreenSizes(ScreenPtr pScreen)
{
    int mmWidth, mmHeight;

    if (msGetPhysicalScreenSizes(pScreen, &mmWidth, &mmHeight)) {
        RROutputPtr pOutput;
        pOutput = RRFirstOutput(pScreen);

        if (!pOutput) {
            return FALSE;
        }

        RROutputSetPhysicalSize(pOutput,
                                mmWidth,
                                mmHeight);
    }

    return TRUE;
}

static Bool
msRandRInit(ScreenPtr pScreen)
{
    rrScrPrivPtr pScrPriv;

    if (!RRScreenInit(pScreen)) {
        return FALSE;
    }

    pScrPriv = rrGetScrPriv(pScreen);
    pScrPriv->rrGetInfo = msRandRGetInfo;
    pScrPriv->rrSetConfig = msRandRSetConfig;

    /* Create the output */
    RRGetInfo(pScreen, TRUE);

    msRandRSetPhysicalScreenSizes(pScreen);

#if RANDR_12_INTERFACE
    if (msRandRGammaInit(pScreen)) {
        pScrPriv->rrCrtcSetGamma = msRandRCrtcSetGamma;
    }
#endif
    return TRUE;
}
#endif /* RANDR */

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
