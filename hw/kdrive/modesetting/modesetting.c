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

typedef struct {
    drmModeConnector *connector;
    drmModeRes *resources;
    drmModeModeInfo *mode;

    void *map_data;
    void *map_addr;

    uint32_t conn_id;
    uint32_t crtc_id;
    uint32_t fb_id;
} gbm_user_data_t;

static Bool msMapFramebuffer(KdScreenInfo * screen);

static inline void*
gbm_bo_get_map(struct gbm_bo *bo)
{
    gbm_user_data_t *data = gbm_bo_get_user_data(bo);
    return data ? data->map_addr : NULL;
}

static void
destroy_user_data(struct gbm_bo *bo, void *_data)
{
    struct gbm_device *gbm = gbm_bo_get_device(bo);
    int fd = gbm_device_get_fd(gbm);
    gbm_user_data_t* data = _data;
    if (!data) {
        return;
    }

    if (data->fb_id) {
        drmModeRmFB(fd, data->fb_id);
    }

    if (data->map_data) {
        gbm_bo_unmap(bo, data->map_data);
    }

    if (data->connector) {
        drmModeFreeConnector(data->connector);
    }

    if (data->resources) {
        drmModeFreeResources(data->resources);
    }

    free(data);
}

static inline int
gbm_bo_map_all(struct gbm_bo *bo, gbm_user_data_t *data)
{
    uint32_t stride = 0;

    if (!bo || !data) {
        return FALSE;
    }

    if (data->map_addr) {
        return TRUE;
    }

    uint32_t width = gbm_bo_get_width(bo);
    uint32_t height = gbm_bo_get_height(bo);

    /* must be NULL before the map call */
    data->map_data = NULL;

    /* While reading from gpu memory is often very slow, we do allow it */
    data->map_addr = gbm_bo_map(bo, 0, 0, width, height,
                                GBM_BO_TRANSFER_READ_WRITE,
                                &stride, &data->map_data);

    return !!data->map_addr;
}

static inline int
gbm_bo_map_or_free(struct gbm_bo *bo, gbm_user_data_t *data)
{
    if (gbm_bo_map_all(bo, data)) {
        return TRUE;
    }

    if (bo) {
        gbm_bo_destroy(bo);
    }
    return FALSE;
}

static inline struct gbm_bo*
gbm_bo_create_and_map_once(struct gbm_device *gbm,
                           gbm_user_data_t *data,
                           uint32_t width, uint32_t height,
                           uint32_t format, uint32_t flags)
{
    struct gbm_bo *ret = NULL;

    if (!data) {
        return NULL;
    }

    ret = gbm_bo_create(gbm, width, height, format, flags);
    if (ret && gbm_bo_map_or_free(ret, data)) {
        return ret;
    }

    return NULL;
}

static struct gbm_bo*
gbm_bo_create_and_map(struct gbm_device *gbm, gbm_user_data_t *data, int w, int h)
{
    uint32_t width = data->mode ? data->mode->hdisplay : w;
    uint32_t height = data->mode ? data->mode->vdisplay : h;

    uint32_t format = GBM_FORMAT_XRGB8888;
#if 0
    uint32_t flags = GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING | GBM_BO_USE_FRONT_RENDERING;
    uint32_t flags2 = GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING;
#endif
    uint32_t flags_dumb = GBM_BO_USE_SCANOUT | GBM_BO_USE_WRITE;

    struct gbm_bo *bo = NULL;

#if 0 /* non-dumb buffers require unmap + map to flush writes, which is far slower than ShadowFB */
    bo = gbm_bo_create_and_map_once(gbm, data, width, height, format, flags);
    if (!bo) {
        bo = gbm_bo_create_and_map_once(gbm, data, width, height, format, flags2);
    }
#endif
    if (!bo) {
        bo = gbm_bo_create_and_map_once(gbm, data, width, height, format, flags_dumb);
    }

    return bo;
}

static int
gbm_bo_create_fb(struct gbm_bo *bo)
{
    struct gbm_device *gbm = gbm_bo_get_device(bo);
    int fd = gbm_device_get_fd(gbm);

    uint32_t width = gbm_bo_get_width(bo);
    uint32_t height = gbm_bo_get_height(bo);
    uint32_t pitch = gbm_bo_get_stride(bo);
    uint32_t handle = gbm_bo_get_handle(bo).u32;
    uint32_t fb_id = 0;

    int ret = drmModeAddFB(fd, width, height, 24, 32, pitch, handle, &fb_id);
    return ret ? 0 : fb_id;
}

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
modesetting_grade_connector(drmModeConnector *conn)
{
    if (!conn->modes || !conn->count_modes) {
        return 1;
    }

    switch(conn->connection) {
    case DRM_MODE_CONNECTED:
        return 5;
    case DRM_MODE_UNKNOWNCONNECTION:
        return 4;
    case DRM_MODE_DISCONNECTED:
        return 3;
    }

    /* unreachable */
    return 2;
}

static drmModeConnector*
modesetting_find_connector(drmModeRes *res, int fd, uint32_t *conn_id)
{
    drmModeConnector *best_connector = NULL;
    int best_score = 0;

    for (int i = 0; i < res->count_connectors; i++) {
        drmModeConnector *conn;
        int id;
        int score;
        id = res->connectors[i];
        conn = drmModeGetConnector(fd, id);
        if (!conn) {
            continue;
        }

        score = modesetting_grade_connector(conn);
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

/* From man drm-kms */
static int
modeseting_find_crtc(int fd, drmModeRes *res, drmModeConnector *conn)
{
    drmModeEncoder *enc;
    unsigned int i, j;

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
            drmModeFreeEncoder(enc);
            return res->crtcs[j];
        }

        drmModeFreeEncoder(enc);
    }

    /* cannot find a suitable CRTC */
    return -ENOENT;
}


static struct gbm_bo*
modesetting_open(struct gbm_device *gbm, KdScreenInfo *screen)
{
    int fd = gbm_device_get_fd(gbm);
    struct gbm_bo *ret = NULL;
    gbm_user_data_t *data = NULL;

    data = calloc(1, sizeof(*data));
    if (!data) {
        goto fail;
    }

    drmSetMaster(fd);

    data->resources = drmModeGetResources(fd);
    if (!data->resources) {
        goto fail;
    }

    data->connector = modesetting_find_connector(data->resources, fd, &data->conn_id);
    if (!data->connector) {
        goto fail;
    }

    data->mode = modesetting_find_mode(data->connector, screen->width, screen->height, screen->rate);
    if (!data->mode) {
        LogMessage(X_WARNING, "Xmodesetting(%d): Could not find a supported mode\n",
                   screen->card->mynum);
        LogMessage(X_WARNING, "Xmodesetting(%d): This likely means the video card has no connected outputs, or the requested mode is not supported\n",
                   screen->card->mynum);
    }

    ret = gbm_bo_create_and_map(gbm, data, screen->width ? screen->width : 1920, screen->height ? screen->height : 1080);
    if (!ret) {
        goto fail;
    }

    gbm_bo_set_user_data(ret, data, destroy_user_data);

    data->fb_id = gbm_bo_create_fb(ret);
    if (!data->fb_id) {
        goto fail;
    }

    data->crtc_id = modeseting_find_crtc(fd, data->resources, data->connector);
    if (data->crtc_id < 0) {
        goto fail;
    }

    return ret;

fail:
    if (ret) {
        gbm_bo_destroy(ret);
        /* destroy_user_data takes care of the rest */
        return NULL;
    }

    if (data) {
        if (data->connector)
            drmModeFreeConnector(data->connector);
        if (data->resources)
            drmModeFreeResources(data->resources);
        free(data);
    }

    return NULL;
}

static Bool
msSetMode(ScreenPtr pScreen, int rate)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msPriv *priv = screen->card->driver;
    msScrPriv *scrpriv = screen->driver;
    struct gbm_bo *new_bo;
    int oldwidth, oldheight, oldrate;

    oldwidth = screen->width;
    oldheight = screen->height;
    oldrate = screen->rate;

    screen->width = pScreen->width;
    screen->height = pScreen->height;
    screen->rate = rate;

    new_bo = modesetting_open(priv->gbm, screen);
    if (!new_bo) {
        screen->width = oldwidth;
        screen->height = oldheight;
        screen->rate = oldrate;
        return FALSE;
    }

    gbm_bo_destroy(scrpriv->front);
    scrpriv->front = new_bo;
    return TRUE;
}

static Bool msInitialize(KdCardInfo * card, msPriv * priv)
{
    MsScreenConf *config = card->closure;
    int fd;

    if (config->dev_path) {
        fd = open(config->dev_path, O_RDWR);
        if (fd < 0) {
            LogMessage(X_ERROR, "Xmodesetting(%d): Error opening KMS device %s: %s\n",
                       card->mynum, config->dev_path, strerror(errno));
            return FALSE;
        }
        LogMessage(X_INFO, "Xmodesetting(%d): Using KMS device: %s\n",
                   card->mynum, config->dev_path);
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
            ErrorF("Error opening kms devices /dev/dri/card[0-63]\n");
            return FALSE;
        }
        LogMessage(X_INFO, "Xmodesetting(%d): Using kms device: %s\n", card->mynum, devbuf);
    }

    priv->gbm = gbm_create_device(fd);
    if (!priv->gbm) {
        ErrorF("Could not create a gbm device\n");
        close(fd);
        return FALSE;
    }

    return TRUE;
}

Bool msCardInit(KdCardInfo * card)
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

static Bool msScreenInitialize(KdScreenInfo * screen, msScrPriv * scrpriv)
{
    msPriv *priv = screen->card->driver;
    gbm_user_data_t *data = NULL;

    scrpriv->front = modesetting_open(priv->gbm, screen);
    if (!scrpriv->front) {
        return FALSE;
    }

    data = gbm_bo_get_user_data(scrpriv->front);

    screen->width = gbm_bo_get_width(scrpriv->front);
    screen->height = gbm_bo_get_height(scrpriv->front);
    screen->rate = data->mode ? data->mode->vrefresh : 0; /* XXX 0 means accept any rate on msEnable */

    /* GBM_FORMAT_XRGB8888 */
    screen->fb.visuals = (1 << TrueColor);
    screen->fb.redMask = 0xff << 16;
    screen->fb.greenMask = 0xff << 8;
    screen->fb.blueMask = 0xff;

    screen->fb.depth = 24;
    screen->fb.bitsPerPixel = 32;

    scrpriv->randr = screen->randr;
    return msMapFramebuffer(screen);
}

Bool msScreenInit(KdScreenInfo * screen)
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

static void *msWindowLinear(ScreenPtr pScreen,
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

static Bool msMapFramebuffer(KdScreenInfo * screen)
{
    msScrPriv *scrpriv = screen->driver;
    KdPointerMatrix m;
    MsScreenConf *config = screen->card->closure;

    unsigned long stride = gbm_bo_get_stride(scrpriv->front);

    if (config->shadow >= 0) {
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

static void msSetScreenSizes(ScreenPtr pScreen)
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

static Bool msUnmapFramebuffer(KdScreenInfo * screen)
{
    KdShadowFbFree(screen);
    return TRUE;
}

static Bool msSetShadow(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *scrpriv = screen->driver;
    ShadowUpdateProc update;
    ShadowWindowProc window;

    window = msWindowLinear;
    update = 0;

    if (scrpriv->randr)
        update = shadowUpdateRotatePacked;
    else
        update = shadowUpdatePacked;
    return KdShadowSet(pScreen, scrpriv->randr, update, window);
}

static Bool msRandRGetInfo(ScreenPtr pScreen, Rotation * rotations)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *scrpriv = screen->driver;
    gbm_user_data_t *data = gbm_bo_get_user_data(scrpriv->front);
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

    for (int i = 0; i < data->connector->count_modes; i++) {
        drmModeModeInfo *mode = &data->connector->modes[i];
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
    Bool wasEnabled = pScreenPriv->enabled;
    msScrPriv oldscr;
    int oldwidth;
    int oldheight;
    int oldmmwidth;
    int oldmmheight;
    int newwidth, newheight, newmmwidth, newmmheight;

    if (screen->randr & (RR_Rotate_0 | RR_Rotate_180)) {
        newwidth = pSize->width;
        newheight = pSize->height;
        newmmwidth = pSize->mmWidth;
        newmmheight = pSize->mmHeight;
    } else {
        newwidth = pSize->height;
        newheight = pSize->width;
        newmmwidth = pSize->mmHeight;
        newmmheight = pSize->mmWidth;
    }

    if (wasEnabled)
        KdDisableScreen(pScreen);

    oldscr = *scrpriv;

    oldwidth = screen->width;
    oldheight = screen->height;
    oldmmwidth = pScreen->mmWidth;
    oldmmheight = pScreen->mmHeight;

    /*
     * Set new configuration
     */

    scrpriv->randr = KdAddRotation(screen->randr, randr);

    pScreen->width = newwidth;
    pScreen->height = newheight;
    pScreen->mmWidth = newmmwidth;
    pScreen->mmHeight = newmmheight;

    msUnmapFramebuffer(screen);

    if (!msSetMode(pScreen, rate))
        goto bail4;

    if (!msMapFramebuffer(screen))
        goto bail4;

    KdShadowUnset(screen->pScreen);

    if (!msSetShadow(screen->pScreen))
        goto bail4;

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
    if (wasEnabled)
        KdEnableScreen(pScreen);

    return TRUE;

bail4:
    msUnmapFramebuffer(screen);
    *scrpriv = oldscr;
    msMapFramebuffer(screen);
    pScreen->width = oldwidth;
    pScreen->height = oldheight;
    pScreen->mmWidth = oldmmwidth;
    pScreen->mmHeight = oldmmheight;

    if (wasEnabled)
        KdEnableScreen(pScreen);
    return FALSE;
}

static Bool
msGetPhysicalScreenSizes(ScreenPtr pScreen, int *mmWidth, int *mmHeight)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *scrpriv = screen->driver;
    gbm_user_data_t *data = gbm_bo_get_user_data(scrpriv->front);

    *mmWidth = screen->width_mm;
    *mmHeight = screen->height_mm;

    if (screen->requested_mm) {
        return TRUE;
    }

    if (((int)data->connector->mmWidth > 0) && ((int)data->connector->mmHeight > 0)) {
        *mmWidth = data->connector->mmWidth;
        *mmWidth = data->connector->mmHeight;
        return TRUE;
    }

    return FALSE;
}

static Bool msRandRInit(ScreenPtr pScreen)
{
    rrScrPrivPtr pScrPriv;
    int mmWidth, mmHeight;

    if (!RRScreenInit(pScreen)) {
        return FALSE;
    }

    pScrPriv = rrGetScrPriv(pScreen);
    pScrPriv->rrGetInfo = msRandRGetInfo;
    pScrPriv->rrSetConfig = msRandRSetConfig;

    if (msGetPhysicalScreenSizes(pScreen, &mmWidth, &mmHeight)) {
        RROutputPtr pOutput;

        /* Create the output */
        RRGetInfo(pScreen, TRUE);

        pOutput = RRFirstOutput(pScreen);
        if (pOutput) {
            RROutputSetPhysicalSize(pOutput,
                                    mmWidth,
                                    mmHeight);
        }
    }

    return TRUE;
}

Bool msInitScreen(ScreenPtr pScreen)
{
    pScreen->CreateColormap = fbInitializeColormap;
    return TRUE;
}

Bool msFinishInitScreen(ScreenPtr pScreen)
{
    if (!shadowSetup(pScreen)) {
        return FALSE;
    }

    if (!msRandRInit(pScreen)) {
        return FALSE;
    }

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
    gbm_user_data_t *data;
    int fd;
    uint32_t width;
    uint32_t height;
    unsigned num_cliprects;

    gbm = gbm_bo_get_device(priv->front);
    data = gbm_bo_get_user_data(priv->front);
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
        ret = drmModeDirtyFB(fd, data->fb_id, clip, num_cliprects);

        /* if we're swamping it with work, try one at a time */
        if (ret) {
            for (int i = 0; i < num_cliprects; i++) {
                ret = drmModeDirtyFB(fd, data->fb_id, &clip[i], 1);
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
    drmModeDirtyFB(fd, data->fb_id, &full_clip, 1);
    if (priv->damage) {
        DamageEmpty(priv->damage);
    }
}

static void
msWakeupHandler(void *blockData, int result)
{
}

Bool msCreateResources(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *priv = screen->driver;
    PixmapPtr rootPixmap = pScreen->GetScreenPixmap(pScreen);
    struct gbm_device *gbm;
    gbm_user_data_t *data;
    int fd;

    gbm = gbm_bo_get_device(priv->front);
    data = gbm_bo_get_user_data(priv->front);
    fd = gbm_device_get_fd(gbm);

    if (!msSetShadow(pScreen)) {
        return FALSE;
    }

    /* Damage tracking not supported/needed */
    if (drmModeDirtyFB(fd, data->fb_id, NULL, 0) &&
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

void msPreserve(KdCardInfo * card)
{
}

Bool msEnable(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *priv = screen->driver;

    struct gbm_device *gbm = gbm_bo_get_device(priv->front);
    gbm_user_data_t *data = gbm_bo_get_user_data(priv->front);
    int fd = gbm_device_get_fd(gbm);

    drmSetMaster(fd);

    if (!data->mode) {
        data->mode = modesetting_find_mode(data->connector, screen->width, screen->height, screen->rate);
    }
    if (data->mode && drmModeSetCrtc(fd, data->crtc_id, data->fb_id, 0, 0, &data->conn_id, 1, data->mode)) {
        return FALSE;
    }

#ifdef XV
    KdXVEnable (pScreen);
#endif

    return TRUE;
}

#if 0
Bool msDPMS(ScreenPtr pScreen, int mode)
{
    KdScreenPriv(pScreen);
    msPriv *priv = pScreenPriv->card->driver;
    static int oldmode = -1;

    if (mode == oldmode)
        return TRUE;

    /* TODO: implement dpms */

    return FALSE;
}
#endif

void msDisable(ScreenPtr pScreen)
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

void msRestore(KdCardInfo * card)
{
}

void msScreenFini(KdScreenInfo * screen)
{
    msScrPriv *priv = screen->driver;

    gbm_bo_destroy(priv->front);
    free(priv);
    screen->driver = NULL;
}

void msCardFini(KdCardInfo * card)
{
    msPriv *priv = card->driver;

    struct gbm_device *gbm = priv->gbm;
    int fd = gbm_device_get_fd(gbm);

    gbm_device_destroy(gbm);
    close(fd);
    free(priv);
    card->driver = NULL;
}

#if 0
void
msGetColors(ScreenPtr pScreen, int n, xColorItem * pdefs)
{
}

void
msPutColors(ScreenPtr pScreen, int n, xColorItem * pdefs)
{
}
#endif

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
