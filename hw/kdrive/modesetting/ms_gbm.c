/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111@shitposting.expert>
 */

#include <kdrive-config.h>

#include "modesetting.h"

#include <drm_fourcc.h>

/* TODO: Stuff that should be in msutil */

typedef struct {
    void *map_data;
    void *map_addr;

    Bool used_modifiers;

    uint32_t fb_id;
} gbm_user_data_t;

void*
gbm_bo_get_map(struct gbm_bo *bo)
{
    gbm_user_data_t *data = gbm_bo_get_user_data(bo);
    return data ? data->map_addr : NULL;
}

uint32_t
gbm_bo_get_fb(struct gbm_bo *bo)
{
    gbm_user_data_t *data = gbm_bo_get_user_data(bo);
    return data ? data->fb_id : 0;
}

Bool
gbm_bo_get_used_modifiers(struct gbm_bo *bo)
{
    gbm_user_data_t *data = gbm_bo_get_user_data(bo);
    return data ? data->used_modifiers : FALSE;
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
gbm_bo_create_and_map(struct gbm_device *gbm, gbm_user_data_t *data, uint32_t width, uint32_t height, uint32_t format,
                      uint64_t *modifiers, int num_modifiers)
{
#if 0
    uint32_t flags = GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING | GBM_BO_USE_FRONT_RENDERING;
    uint32_t flags2 = GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING;
#endif
    uint32_t flags_dumb = GBM_BO_USE_SCANOUT | GBM_BO_USE_WRITE;

    struct gbm_bo *bo = NULL;

    /* Implicit modifiers are ok */
    Bool modifiers_ok = !num_modifiers;

    for (int i = 0; !modifiers_ok && i < num_modifiers; i++) {
        switch (modifiers[i]) {
        case DRM_FORMAT_MOD_LINEAR:
        case DRM_FORMAT_MOD_INVALID:
            modifiers_ok = TRUE;
        }
    }

    if (!modifiers_ok) {
        return NULL;
    }

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

static struct gbm_bo*
gbm_bo_create_tiled(struct gbm_device *gbm, uint32_t width, uint32_t height, uint32_t format,
                    uint64_t *modifiers, int num_modifiers)
{
    struct gbm_bo *bo = NULL;

    /* Used by mesa */
    uint32_t flags = GBM_BO_USE_SCANOUT | GBM_BO_USE_FRONT_RENDERING;

    /* Used by nvidia */
    uint32_t flags2 = GBM_BO_USE_SCANOUT;

    /* We can't use a tiled buffer for these formats */
    switch (format) {
    case GBM_FORMAT_RGB888:
    case GBM_FORMAT_BGR888:
        return NULL;
    }

    if (!bo) {
        bo = gbm_bo_create_with_modifiers2(gbm, width, height, format,
                                           modifiers, num_modifiers, flags);
    }

    if (!bo) {
        bo = gbm_bo_create_with_modifiers2(gbm, width, height, format,
                                           modifiers, num_modifiers, flags2);
    }

    if (num_modifiers &&
        !(num_modifiers == 1 && modifiers[0] == DRM_FORMAT_MOD_INVALID)) {
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
    uint32_t fb_id = 0;
    Bool need_check = FALSE;

    uint32_t format = gbm_bo_get_format(bo);
    int depth = gbm_format_get_depth(format);
    int bpp = gbm_bo_get_bpp(bo);
    int num_planes = gbm_bo_get_plane_count(bo);
    int ret;

    uint32_t handles[4] = {0};
    uint32_t pitches[4] = {0};
    uint32_t offsets[4] = {0};
    uint64_t modifiers[4] = {0};
    uint64_t modifier = gbm_bo_get_modifier(bo);

    for (int i = 0; i < num_planes; i++) {
        handles[i] = gbm_bo_get_handle_for_plane(bo, i).u32;
        pitches[i] = gbm_bo_get_stride_for_plane(bo, i);
        offsets[i] = gbm_bo_get_offset(bo, i);
        modifiers[i] = modifier;
    }

    ret = drmModeAddFB2WithModifiers(fd, width, height, format, handles, pitches, offsets,
                                     modifiers, &fb_id, DRM_MODE_FB_MODIFIERS);
    if (ret) {
        need_check = TRUE;
        ret = drmModeAddFB2(fd, width, height, format, handles, pitches, offsets, &fb_id, 0);
    }
    if (ret && num_planes == 1) {
        ret = drmModeAddFB(fd, width, height, depth, bpp, pitches[0], handles[0], &fb_id);
    }

    if (!ret && need_check) {
        /* Check that we didn't lose format + modifier information */
        drmModeFB2Ptr fb_ptr;

        fb_ptr = drmModeGetFB2(fd, fb_id);
        if (fb_ptr) {
            if (fb_ptr->pixel_format != format ||
                fb_ptr->modifier != modifier) {
                drmModeFreeFB2(fb_ptr);
                drmModeRmFB(fd, fb_id);
                return 0;
            }
            drmModeFreeFB2(fb_ptr);
        }
    }

    return ret ? 0 : fb_id;
}

static uint32_t
gbm_front_format_for_depth_swap(int depth, int bpp)
{
    switch (depth) {
    case 8:
        return GBM_FORMAT_C8;
    case 15:
        return GBM_FORMAT_XBGR1555;
    case 16:
        return GBM_FORMAT_BGR565;
    case 30:
        return GBM_FORMAT_XBGR2101010;
    case 24:
    default:
        return (bpp == 24) ? GBM_FORMAT_BGR888 : GBM_FORMAT_XBGR8888;
    }

}

uint32_t
gbm_front_format_for_depth(int depth, int bpp, Bool rb_swap)
{
    if (rb_swap) {
        return gbm_front_format_for_depth_swap(depth, bpp);
    }

    switch (depth) {
    case 8:
        return GBM_FORMAT_R8;
    case 15:
        return GBM_FORMAT_XRGB1555;
    case 16:
        return GBM_FORMAT_RGB565;
    case 30:
        return GBM_FORMAT_XRGB2101010;
    case 24:
    default:
        return (bpp == 24) ? GBM_FORMAT_RGB888 : GBM_FORMAT_XRGB8888;
    }
}

int
gbm_format_get_depth(uint32_t format)
{
    switch (format) {
    case GBM_FORMAT_R8:
    case GBM_FORMAT_C8:
        return 8;
    case GBM_FORMAT_XRGB1555:
    case GBM_FORMAT_XBGR1555:
        return 15;
    case GBM_FORMAT_RGB565:
    case GBM_FORMAT_BGR565:
        return 16;
    case GBM_FORMAT_RGB888:
    case GBM_FORMAT_BGR888:
    case GBM_FORMAT_XRGB8888:
    case GBM_FORMAT_XBGR8888:
    default:
        return 24;
    case GBM_FORMAT_XRGB2101010:
    case GBM_FORMAT_XBGR2101010:
        return 30;
    }
}

struct gbm_bo*
gbm_create_front_bo(struct gbm_device *gbm, Bool do_map, uint32_t width, uint32_t height, uint32_t format,
                    uint64_t *modifiers, int num_modifiers)
{
    struct gbm_bo *ret = NULL;
    gbm_user_data_t *data = NULL;

    data = calloc(1, sizeof(*data));
    if (!data) {
        goto fail;
    }

    ret = do_map ? gbm_bo_create_and_map(gbm, data, width, height, format, modifiers, num_modifiers) :
                   gbm_bo_create_tiled(gbm, width, height, format, modifiers, num_modifiers);
    if (!ret) {
        goto fail;
    }

    gbm_bo_set_user_data(ret, data, destroy_user_data);

    data->fb_id = gbm_bo_create_fb(ret);
    if (!data->fb_id) {
        goto fail;
    }

    if (!do_map && num_modifiers &&
        !(num_modifiers == 1 && modifiers[0] == DRM_FORMAT_MOD_INVALID)) {
        data->used_modifiers = TRUE;
    }

    return ret;

fail:
    if (ret) {
        gbm_bo_destroy(ret);
        /* destroy_user_data takes care of the rest */
        return NULL;
    }

    if (data) {
        free(data);
    }

    return NULL;
}

struct gbm_bo*
gbm_create_front_for_screen(KdScreenInfo *screen, Bool do_map, Bool do_swap)
{
    msPriv *priv = screen->card->driver;
    msScrPriv *scrpriv = screen->driver;
    uint32_t format = gbm_front_format_for_depth(screen->fb.depth, screen->fb.bitsPerPixel, do_swap /* rb_swap */);
    uint32_t format_swap = gbm_front_format_for_depth(screen->fb.depth, screen->fb.bitsPerPixel, !do_swap /* rb_swap */);
    struct gbm_bo *ret = NULL;

    /* TODO: Query scanout modifiers in CardInit,
     * intersect with render modifiers queried in msGlamorInit
     */
    uint64_t *modifiers = scrpriv ? scrpriv->render_modifiers : NULL;
    int num_modifiers = scrpriv ? scrpriv->num_render_modifiers : 0;

    if (!ret) {
        ret = gbm_create_front_bo(priv->gbm, do_map, screen->width, screen->height, format, modifiers, num_modifiers);
    }
    if (!ret) {
        ret = gbm_create_front_bo(priv->gbm, do_map, screen->width, screen->height, format_swap, modifiers, num_modifiers);
    }

    return ret;
}

void
gbm_bo_set_screen_fb_info(struct gbm_bo *bo, KdScreenInfo *screen, Bool is_gles)
{
    uint32_t format = gbm_bo_get_format(bo);
    Bool rb_swap = FALSE;

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

    /* XXX Tiled buffers don't need r-b swap, unless it's depth 30 on gles
     *
     * This is because glamor_setup_formats tells glamor to expect pixels
     * in rgb format for all depth, except depth 30 on gles
     */
    if (!gbm_bo_get_map(bo)) {
        if (screen->fb.depth != 30 || !is_gles) {
            rb_swap = FALSE;
        }
    }

    if (rb_swap) {
        int tmp = screen->fb.blueMask;
        screen->fb.blueMask = screen->fb.redMask;
        screen->fb.redMask = tmp;
    }
}
