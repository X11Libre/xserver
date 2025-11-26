/*
 * Copyright © 2007 Red Hat, Inc.
 * Copyright © 2019 NVIDIA CORPORATION
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Dave Airlie <airlied@redhat.com>
 *    Aaron Plattner <aplattner@nvidia.com>
 *
 */

#include "dix-config.h"

#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "dix/dix_priv.h"
#include "os/fmt.h"
#include "os/mathx_priv.h"
#include "present/present_priv.h"

#include "dumb_bo.h"
#include "inputstr.h"
#include "xf86str.h"
#include "X11/Xatom.h"
#include "mi.h"
#include "micmap.h"
#include "xf86cmap.h"
#include "xf86DDC_priv.h"
#include <drm_fourcc.h>
#include <drm_mode.h>

#include <xf86drm.h>
#include "xf86Crtc.h"
#include "drmmode_display.h"

#include <cursorstr.h>

#include <X11/extensions/dpmsconst.h>

#include "driver.h"

#ifndef GBM_BO_USE_FRONT_RENDERING
#define GBM_BO_USE_FRONT_RENDERING 0
#endif

static Bool drmmode_xf86crtc_resize(ScrnInfoPtr scrn, int width, int height);
static PixmapPtr drmmode_create_pixmap_header(ScreenPtr pScreen, int width, int height,
                                              int depth, int bitsPerPixel, int devKind,
                                              void *pPixData);

static const struct drm_color_ctm ctm_identity = { {
    1ULL << 32, 0, 0,
    0, 1ULL << 32, 0,
    0, 0, 1ULL << 32
} };

static Bool ctm_is_identity(const struct drm_color_ctm *ctm)
{
    const size_t matrix_len = sizeof(ctm->matrix) / sizeof(ctm->matrix[0]);
    const uint64_t one = 1ULL << 32;
    const uint64_t neg_zero = 1ULL << 63;
    int i;

    for (i = 0; i < matrix_len; i++) {
        const Bool diagonal = i / 3 == i % 3;
        const uint64_t val = ctm->matrix[i];

        if ((diagonal && val != one) ||
            (!diagonal && val != 0 && val != neg_zero)) {
            return FALSE;
        }
    }

    return TRUE;
}

static inline uint32_t *
formats_ptr(struct drm_format_modifier_blob *blob)
{
    return (uint32_t *)(((char *)blob) + blob->formats_offset);
}

static inline struct drm_format_modifier *
modifiers_ptr(struct drm_format_modifier_blob *blob)
{
    return (struct drm_format_modifier *)(((char *)blob) + blob->modifiers_offset);
}

static uint32_t
get_opaque_format(uint32_t format)
{
    switch (format) {
    case DRM_FORMAT_ARGB8888:
        return DRM_FORMAT_XRGB8888;
    case DRM_FORMAT_ARGB2101010:
        return DRM_FORMAT_XRGB2101010;
    default:
        return format;
    }
}

static drmmode_format_ptr
drmmode_crtc_get_format(drmmode_crtc_private_ptr drmmode_crtc,
                        Bool async_flip, int i)
{
    if (async_flip && drmmode_crtc->formats_async)
        return &drmmode_crtc->formats_async[i];
    else
        return &drmmode_crtc->formats[i];
}

Bool
drmmode_is_format_supported(ScrnInfoPtr scrn, uint32_t format,
                            uint64_t modifier, Bool async_flip)
{
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    int c, i, j;

    /* BO are imported as opaque surface, so let's pretend there is no alpha */
    format = get_opaque_format(format);

    for (c = 0; c < xf86_config->num_crtc; c++) {
        xf86CrtcPtr crtc = xf86_config->crtc[c];
        drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
        Bool found = FALSE;

        if (!crtc->enabled)
            continue;

        if (drmmode_crtc->num_formats == 0)
            continue;

        for (i = 0; i < drmmode_crtc->num_formats; i++) {
            drmmode_format_ptr iter = drmmode_crtc_get_format(drmmode_crtc, async_flip, i);

            if (iter->format != format)
                continue;

            if (modifier == DRM_FORMAT_MOD_INVALID ||
                iter->num_modifiers == 0) {
                found = TRUE;
                break;
            }

            for (j = 0; j < iter->num_modifiers; j++) {
                if (iter->modifiers[j] == modifier) {
                    found = TRUE;
                    break;
                }
            }

            break;
        }

        if (!found)
            return FALSE;
    }

    return TRUE;
}

#ifdef GBM_BO_WITH_MODIFIERS
static uint32_t
get_modifiers_set(ScrnInfoPtr scrn, uint32_t format, uint64_t **modifiers,
                  Bool enabled_crtc_only, Bool exclude_multiplane, Bool async_flip)
{
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    modesettingPtr ms = modesettingPTR(scrn);
    drmmode_ptr drmmode = &ms->drmmode;
    int c, i, j, k, count_modifiers = 0;
    uint64_t *tmp, *ret = NULL;

    /* BOs are imported as opaque surfaces, so pretend the same thing here */
    format = get_opaque_format(format);

    *modifiers = NULL;
    for (c = 0; c < xf86_config->num_crtc; c++) {
        xf86CrtcPtr crtc = xf86_config->crtc[c];
        drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

        if (enabled_crtc_only && !crtc->enabled)
            continue;

        for (i = 0; i < drmmode_crtc->num_formats; i++) {
            drmmode_format_ptr iter = drmmode_crtc_get_format(drmmode_crtc, async_flip, i);

            if (iter->format != format)
                continue;

            for (j = 0; j < iter->num_modifiers; j++) {
                Bool found = FALSE;

                /* Don't choose multi-plane formats for our screen pixmap.
                 * These will get used with frontbuffer rendering, which will
                 * lead to worse-than-tearing with multi-plane formats, as the
                 * primary and auxiliary planes go out of sync. */
                if (exclude_multiplane &&
                    gbm_device_get_format_modifier_plane_count(drmmode->gbm,
                                                               format,
                                                               iter->modifiers[j]) > 1) {
                    continue;
                }

                for (k = 0; k < count_modifiers; k++) {
                    if (iter->modifiers[j] == ret[k])
                        found = TRUE;
                }
                if (!found) {
                    count_modifiers++;
                    tmp = realloc(ret, count_modifiers * sizeof(uint64_t));
                    if (!tmp) {
                        free(ret);
                        return 0;
                    }
                    ret = tmp;
                    ret[count_modifiers - 1] = iter->modifiers[j];
                }
            }
        }
    }

    *modifiers = ret;
    return count_modifiers;
}

static Bool
get_drawable_modifiers(DrawablePtr draw, uint32_t format,
                       uint32_t *num_modifiers, uint64_t **modifiers)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(draw->pScreen);
    modesettingPtr ms = modesettingPTR(scrn);
    Bool async_flip;

    if (!present_can_window_flip((WindowPtr) draw) ||
        !ms->drmmode.pageflip || ms->drmmode.dri2_flipping || !scrn->vtSema) {
        *num_modifiers = 0;
        *modifiers = NULL;
        return TRUE;
    }

    async_flip = ms_window_has_async_flip((WindowPtr)draw);
    ms_window_update_async_flip_modifiers((WindowPtr)draw, async_flip);

    *num_modifiers = get_modifiers_set(scrn, format, modifiers,
                                       TRUE, FALSE, async_flip);
    return TRUE;
}
#endif

static Bool
drmmode_zaphod_string_matches(ScrnInfoPtr scrn, const char *s, char *output_name)
{
    char **token = xstrtokenize(s, ", \t\n\r");
    Bool ret = FALSE;

    if (!token)
        return FALSE;

    for (int i = 0; token[i]; i++) {
        if (strcmp(token[i], output_name) == 0)
            ret = TRUE;

        free(token[i]);
    }

    free(token);

    return ret;
}

static uint64_t
drmmode_prop_get_value(drmmode_prop_info_ptr info,
                       drmModeObjectPropertiesPtr props,
                       uint64_t def)
{
    unsigned int i;

    if (info->prop_id == 0)
        return def;

    for (i = 0; i < props->count_props; i++) {
        unsigned int j;

        if (props->props[i] != info->prop_id)
            continue;

        /* Simple (non-enum) types can return the value directly */
        if (info->num_enum_values == 0)
            return props->prop_values[i];

        /* Map from raw value to enum value */
        for (j = 0; j < info->num_enum_values; j++) {
            if (!info->enum_values[j].valid)
                continue;
            if (info->enum_values[j].value != props->prop_values[i])
                continue;

            return j;
        }
    }

    return def;
}

static uint32_t
drmmode_prop_info_update(drmmode_ptr drmmode,
                         drmmode_prop_info_ptr info,
                         unsigned int num_infos,
                         drmModeObjectProperties *props)
{
    drmModePropertyRes *prop;
    uint32_t valid_mask = 0;
    unsigned i, j;

    assert(num_infos <= 32 && "update return type");

    for (i = 0; i < props->count_props; i++) {
        Bool props_incomplete = FALSE;
        unsigned int k;

        for (j = 0; j < num_infos; j++) {
            if (info[j].prop_id == props->props[i])
                break;
            if (!info[j].prop_id)
                props_incomplete = TRUE;
        }

        /* We've already discovered this property. */
        if (j != num_infos)
            continue;

        /* We haven't found this property ID, but as we've already
         * found all known properties, we don't need to look any
         * further. */
        if (!props_incomplete)
            break;

        prop = drmModeGetProperty(drmmode->fd, props->props[i]);
        if (!prop)
            continue;

        for (j = 0; j < num_infos; j++) {
            if (!strcmp(prop->name, info[j].name))
                break;
        }

        /* We don't know/care about this property. */
        if (j == num_infos) {
            drmModeFreeProperty(prop);
            continue;
        }

        info[j].prop_id = props->props[i];
        info[j].value = props->prop_values[i];
        valid_mask |= 1U << j;

        if (info[j].num_enum_values == 0) {
            drmModeFreeProperty(prop);
            continue;
        }

        if (!(prop->flags & DRM_MODE_PROP_ENUM)) {
            xf86DrvMsg(drmmode->scrn->scrnIndex, X_WARNING,
                       "expected property %s to be an enum,"
                       " but it is not; ignoring\n", prop->name);
            drmModeFreeProperty(prop);
            continue;
        }

        for (k = 0; k < info[j].num_enum_values; k++) {
            int l;

            if (info[j].enum_values[k].valid)
                continue;

            for (l = 0; l < prop->count_enums; l++) {
                if (!strcmp(prop->enums[l].name,
                            info[j].enum_values[k].name))
                    break;
            }

            if (l == prop->count_enums)
                continue;

            info[j].enum_values[k].valid = TRUE;
            info[j].enum_values[k].value = prop->enums[l].value;
        }

        drmModeFreeProperty(prop);
    }

    return valid_mask;
}

static Bool
drmmode_prop_info_copy(drmmode_prop_info_ptr dst,
                       const drmmode_prop_info_rec *src,
                       unsigned int num_props,
                       Bool copy_prop_id)
{
    unsigned int i;

    memcpy(dst, src, num_props * sizeof(*dst));

    for (i = 0; i < num_props; i++) {
        unsigned int j;

        if (copy_prop_id)
            dst[i].prop_id = src[i].prop_id;
        else
            dst[i].prop_id = 0;

        if (src[i].num_enum_values == 0)
            continue;

        dst[i].enum_values =
            calloc(src[i].num_enum_values,
                    sizeof(*dst[i].enum_values));
        if (!dst[i].enum_values)
            goto err;

        memcpy(dst[i].enum_values, src[i].enum_values,
                src[i].num_enum_values * sizeof(*dst[i].enum_values));

        for (j = 0; j < dst[i].num_enum_values; j++)
            dst[i].enum_values[j].valid = FALSE;
    }

    return TRUE;

err:
    while (i--)
        free(dst[i].enum_values);
    return FALSE;
}

static void
drmmode_prop_info_free(drmmode_prop_info_ptr info, int num_props)
{
    int i;

    for (i = 0; i < num_props; i++)
        free(info[i].enum_values);
}

static void
drmmode_ConvertToKMode(ScrnInfoPtr scrn,
                       drmModeModeInfo * kmode, DisplayModePtr mode);


static int
plane_add_prop(drmModeAtomicReq *req, drmmode_crtc_private_ptr drmmode_crtc,
               enum drmmode_plane_property prop, uint64_t val)
{
    drmmode_prop_info_ptr info = &drmmode_crtc->props_plane[prop];
    int ret;

    if (!info)
        return -1;

    ret = drmModeAtomicAddProperty(req, drmmode_crtc->plane_id,
                                   info->prop_id, val);
    return (ret <= 0) ? -1 : 0;
}

static int
plane_add_props(drmModeAtomicReq *req, xf86CrtcPtr crtc,
                uint32_t fb_id, int x, int y)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    int ret = 0;

    ret |= plane_add_prop(req, drmmode_crtc, DRMMODE_PLANE_FB_ID,
                          fb_id);
    ret |= plane_add_prop(req, drmmode_crtc, DRMMODE_PLANE_CRTC_ID,
                          fb_id ? drmmode_crtc->mode_crtc->crtc_id : 0);
    ret |= plane_add_prop(req, drmmode_crtc, DRMMODE_PLANE_SRC_X, x << 16);
    ret |= plane_add_prop(req, drmmode_crtc, DRMMODE_PLANE_SRC_Y, y << 16);
    ret |= plane_add_prop(req, drmmode_crtc, DRMMODE_PLANE_SRC_W,
                          crtc->mode.HDisplay << 16);
    ret |= plane_add_prop(req, drmmode_crtc, DRMMODE_PLANE_SRC_H,
                          crtc->mode.VDisplay << 16);
    ret |= plane_add_prop(req, drmmode_crtc, DRMMODE_PLANE_CRTC_X, 0);
    ret |= plane_add_prop(req, drmmode_crtc, DRMMODE_PLANE_CRTC_Y, 0);
    ret |= plane_add_prop(req, drmmode_crtc, DRMMODE_PLANE_CRTC_W,
                          crtc->mode.HDisplay);
    ret |= plane_add_prop(req, drmmode_crtc, DRMMODE_PLANE_CRTC_H,
                          crtc->mode.VDisplay);

    return ret;
}

static int
crtc_add_prop(drmModeAtomicReq *req, drmmode_crtc_private_ptr drmmode_crtc,
              enum drmmode_crtc_property prop, uint64_t val)
{
    drmmode_prop_info_ptr info = &drmmode_crtc->props[prop];
    int ret;

    if (!info)
        return -1;

    ret = drmModeAtomicAddProperty(req, drmmode_crtc->mode_crtc->crtc_id,
                                   info->prop_id, val);
    return (ret <= 0) ? -1 : 0;
}

static int
connector_add_prop(drmModeAtomicReq *req, drmmode_output_private_ptr drmmode_output,
                   enum drmmode_connector_property prop, uint64_t val)
{
    drmmode_prop_info_ptr info = &drmmode_output->props_connector[prop];
    int ret;

    if (!info)
        return -1;

    ret = drmModeAtomicAddProperty(req, drmmode_output->output_id,
                                   info->prop_id, val);
    return (ret <= 0) ? -1 : 0;
}

static int
drmmode_CompareKModes(const drmModeModeInfo * kmode, const drmModeModeInfo * other)
{
    return memcmp(kmode, other, sizeof(*kmode));
}

static int
drm_mode_ensure_blob(xf86CrtcPtr crtc, const drmModeModeInfo* mode_info)
{
    modesettingPtr ms = modesettingPTR(crtc->scrn);
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_mode_ptr mode;
    int ret;

    if (drmmode_crtc->current_mode &&
        drmmode_CompareKModes(&drmmode_crtc->current_mode->mode_info, mode_info) == 0)
        return 0;

    mode = calloc(1, sizeof(drmmode_mode_rec));
    if (!mode)
        return -1;

    mode->mode_info = *mode_info;
    ret = drmModeCreatePropertyBlob(ms->fd,
                                    &mode->mode_info,
                                    sizeof(mode->mode_info),
                                    &mode->blob_id);
    drmmode_crtc->current_mode = mode;
    xorg_list_add(&mode->entry, &drmmode_crtc->mode_list);

    return ret;
}

static int
crtc_add_dpms_props(drmModeAtomicReq *req, xf86CrtcPtr crtc,
                    int new_dpms, Bool *active)
{
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(crtc->scrn);
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    Bool crtc_active = FALSE;
    int i;
    int ret = 0;

    for (i = 0; i < xf86_config->num_output; i++) {
        xf86OutputPtr output = xf86_config->output[i];
        drmmode_output_private_ptr drmmode_output = output->driver_private;

        if (output->crtc != crtc) {
            if (drmmode_output->current_crtc == crtc) {
                ret |= connector_add_prop(req, drmmode_output,
                                          DRMMODE_CONNECTOR_CRTC_ID, 0);
            }
            continue;
        }

        if (drmmode_output->output_id == -1)
            continue;

        if (new_dpms == DPMSModeOn)
            crtc_active = TRUE;

        ret |= connector_add_prop(req, drmmode_output,
                                  DRMMODE_CONNECTOR_CRTC_ID,
                                  crtc_active ?
                                      drmmode_crtc->mode_crtc->crtc_id : 0);
    }

    if (crtc_active) {
        drmModeModeInfo kmode;

        drmmode_ConvertToKMode(crtc->scrn, &kmode, &crtc->mode);
        ret |= drm_mode_ensure_blob(crtc, &kmode);

        ret |= crtc_add_prop(req, drmmode_crtc,
                             DRMMODE_CRTC_ACTIVE, 1);
        ret |= crtc_add_prop(req, drmmode_crtc,
                             DRMMODE_CRTC_MODE_ID,
                             drmmode_crtc->current_mode->blob_id);
    } else {
        ret |= crtc_add_prop(req, drmmode_crtc,
                             DRMMODE_CRTC_ACTIVE, 0);
        ret |= crtc_add_prop(req, drmmode_crtc,
                             DRMMODE_CRTC_MODE_ID, 0);
    }

    if (active)
        *active = crtc_active;

    return ret;
}

static void
drm_mode_destroy(xf86CrtcPtr crtc, drmmode_mode_ptr mode)
{
    modesettingPtr ms = modesettingPTR(crtc->scrn);
    if (mode->blob_id)
        drmModeDestroyPropertyBlob(ms->fd, mode->blob_id);
    xorg_list_del(&mode->entry);
    free(mode);
}

static int
drmmode_crtc_can_test_mode(xf86CrtcPtr crtc)
{
    modesettingPtr ms = modesettingPTR(crtc->scrn);

    return ms->atomic_modeset;
}

Bool
drmmode_crtc_get_fb_id(xf86CrtcPtr crtc, uint32_t *fb_id, int *x, int *y)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_ptr drmmode = drmmode_crtc->drmmode;
    drmmode_tearfree_ptr trf = &drmmode_crtc->tearfree;
    int ret;

    *fb_id = 0;

    if (drmmode_crtc->prime_pixmap) {
        if (!drmmode->reverse_prime_offload_mode) {
            msPixmapPrivPtr ppriv =
                msGetPixmapPriv(drmmode, drmmode_crtc->prime_pixmap);
            *fb_id = ppriv->fb_id;
            *x = 0;
        } else
            *x = drmmode_crtc->prime_pixmap_x;
        *y = 0;
    }
    else if (trf->buf[trf->back_idx ^ 1].px) {
        *fb_id = trf->buf[trf->back_idx ^ 1].fb_id;
        *x = *y = 0;
    }
    else if (drmmode_crtc->rotate_fb_id) {
        *fb_id = drmmode_crtc->rotate_fb_id;
        *x = *y = 0;
    }
    else {
        *fb_id = drmmode->fb_id;
        *x = crtc->x;
        *y = crtc->y;
    }

    if (*fb_id == 0) {
        ret = drmmode_bo_import(drmmode, &drmmode->front_bo,
                                &drmmode->fb_id);
        if (ret < 0) {
            ErrorF("failed to add fb %d\n", ret);
            return FALSE;
        }
        *fb_id = drmmode->fb_id;
    }

    return TRUE;
}

void
drmmode_set_dpms(ScrnInfoPtr scrn, int dpms, int flags)
{
    modesettingPtr ms = modesettingPTR(scrn);
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    drmModeAtomicReq *req = drmModeAtomicAlloc();
    uint32_t mode_flags = DRM_MODE_ATOMIC_ALLOW_MODESET;
    int ret = 0;
    int i;

    assert(ms->atomic_modeset);

    if (!req)
        return;

    for (i = 0; i < xf86_config->num_output; i++) {
        xf86OutputPtr output = xf86_config->output[i];
        drmmode_output_private_ptr drmmode_output = output->driver_private;

        if (output->crtc != NULL)
            continue;

        ret = connector_add_prop(req, drmmode_output,
                                 DRMMODE_CONNECTOR_CRTC_ID, 0);
    }

    for (i = 0; i < xf86_config->num_crtc; i++) {
        xf86CrtcPtr crtc = xf86_config->crtc[i];
        drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
        Bool active = FALSE;

        ret |= crtc_add_dpms_props(req, crtc, dpms, &active);

        if (dpms == DPMSModeOn && active && drmmode_crtc->need_modeset) {
            uint32_t fb_id;
            int x, y;

            if (!drmmode_crtc_get_fb_id(crtc, &fb_id, &x, &y))
                continue;
            ret |= plane_add_props(req, crtc, fb_id, x, y);
            drmmode_crtc->need_modeset = FALSE;
        }
    }

    if (ret == 0)
        drmModeAtomicCommit(ms->fd, req, mode_flags, NULL);
    drmModeAtomicFree(req);

    ms->pending_modeset = TRUE;
    xf86DPMSSet(scrn, dpms, flags);
    ms->pending_modeset = FALSE;
}

static int
drmmode_output_disable(xf86OutputPtr output)
{
    modesettingPtr ms = modesettingPTR(output->scrn);
    drmmode_output_private_ptr drmmode_output = output->driver_private;
    xf86CrtcPtr crtc = drmmode_output->current_crtc;
    drmModeAtomicReq *req = drmModeAtomicAlloc();
    uint32_t flags = DRM_MODE_ATOMIC_ALLOW_MODESET;
    int ret = 0;

    assert(ms->atomic_modeset);

    if (!req)
        return 1;

    ret |= connector_add_prop(req, drmmode_output,
                              DRMMODE_CONNECTOR_CRTC_ID, 0);
    if (crtc)
        ret |= crtc_add_dpms_props(req, crtc, DPMSModeOff, NULL);

    if (ret == 0)
        ret = drmModeAtomicCommit(ms->fd, req, flags, NULL);

    if (ret == 0)
        drmmode_output->current_crtc = NULL;

    drmModeAtomicFree(req);
    return ret;
}

static int
drmmode_crtc_disable(xf86CrtcPtr crtc)
{
    modesettingPtr ms = modesettingPTR(crtc->scrn);
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmModeAtomicReq *req = drmModeAtomicAlloc();
    uint32_t flags = DRM_MODE_ATOMIC_ALLOW_MODESET;
    int ret = 0;

    assert(ms->atomic_modeset);

    if (!req)
        return 1;

    ret |= crtc_add_prop(req, drmmode_crtc,
                         DRMMODE_CRTC_ACTIVE, 0);
    ret |= crtc_add_prop(req, drmmode_crtc,
                         DRMMODE_CRTC_MODE_ID, 0);

    if (ret == 0)
        ret = drmModeAtomicCommit(ms->fd, req, flags, NULL);

    drmModeAtomicFree(req);
    return ret;
}

static void
drmmode_set_ctm(xf86CrtcPtr crtc, const struct drm_color_ctm *ctm)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_ptr drmmode = drmmode_crtc->drmmode;
    drmmode_prop_info_ptr ctm_info =
        &drmmode_crtc->props[DRMMODE_CRTC_CTM];
    int ret;
    uint32_t blob_id = 0;

    if (ctm_info->prop_id == 0)
        return;

    if (ctm && drmmode_crtc->use_gamma_lut && !ctm_is_identity(ctm)) {
        ret = drmModeCreatePropertyBlob(drmmode->fd, ctm, sizeof(*ctm), &blob_id);
        if (ret != 0) {
            xf86DrvMsg(crtc->scrn->scrnIndex, X_ERROR,
                       "Failed to create CTM property blob: %d\n", ret);
            blob_id = 0;
        }
    }

    ret = drmModeObjectSetProperty(drmmode->fd,
                                   drmmode_crtc->mode_crtc->crtc_id,
                                   DRM_MODE_OBJECT_CRTC, ctm_info->prop_id,
                                   blob_id);
    if (ret != 0)
        xf86DrvMsg(crtc->scrn->scrnIndex, X_ERROR,
                   "Failed to set CTM property: %d\n", ret);

    drmModeDestroyPropertyBlob(drmmode->fd, blob_id);
}

static int
drmmode_crtc_set_mode(xf86CrtcPtr crtc, Bool test_only)
{
    modesettingPtr ms = modesettingPTR(crtc->scrn);
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(crtc->scrn);
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_ptr drmmode = drmmode_crtc->drmmode;
    drmModeModeInfo kmode;
    int output_count = 0;
    uint32_t *output_ids = NULL;
    uint32_t fb_id;
    int x, y;
    int i, ret = 0;
    const struct drm_color_ctm *ctm = NULL;

    if (!drmmode_crtc_get_fb_id(crtc, &fb_id, &x, &y))
        return 1;

#ifdef GLAMOR_HAS_GBM
    /* Make sure any pending drawing will be visible in a new scanout buffer */
    if (drmmode->glamor)
        glamor_finish(crtc->scrn->pScreen);
#endif

    if (ms->atomic_modeset) {
        drmModeAtomicReq *req = drmModeAtomicAlloc();
        Bool active;
        uint32_t flags = DRM_MODE_ATOMIC_ALLOW_MODESET;

        if (!req)
            return 1;

        ret |= crtc_add_dpms_props(req, crtc, DPMSModeOn, &active);
        ret |= plane_add_props(req, crtc, active ? fb_id : 0, x, y);

        /* Orphaned CRTCs need to be disabled right now in atomic mode */
        for (i = 0; i < xf86_config->num_crtc; i++) {
            xf86CrtcPtr other_crtc = xf86_config->crtc[i];
            drmmode_crtc_private_ptr other_drmmode_crtc = other_crtc->driver_private;
            int lost_outputs = 0;
            int remaining_outputs = 0;
            int j;

            if (other_crtc == crtc)
                continue;

            for (j = 0; j < xf86_config->num_output; j++) {
                xf86OutputPtr output = xf86_config->output[j];
                drmmode_output_private_ptr drmmode_output = output->driver_private;

                if (drmmode_output->current_crtc == other_crtc) {
                    if (output->crtc == crtc)
                        lost_outputs++;
                    else
                        remaining_outputs++;
                }
            }

            if (lost_outputs > 0 && remaining_outputs == 0) {
                ret |= crtc_add_prop(req, other_drmmode_crtc,
                                     DRMMODE_CRTC_ACTIVE, 0);
                ret |= crtc_add_prop(req, other_drmmode_crtc,
                                     DRMMODE_CRTC_MODE_ID, 0);
            }
        }

        if (test_only)
            flags |= DRM_MODE_ATOMIC_TEST_ONLY;

        if (ret == 0)
            ret = drmModeAtomicCommit(ms->fd, req, flags, NULL);

        if (ret == 0 && !test_only) {
            for (i = 0; i < xf86_config->num_output; i++) {
                xf86OutputPtr output = xf86_config->output[i];
                drmmode_output_private_ptr drmmode_output = output->driver_private;

                if (output->crtc == crtc)
                    drmmode_output->current_crtc = crtc;
                else if (drmmode_output->current_crtc == crtc)
                    drmmode_output->current_crtc = NULL;
            }
        }

        drmModeAtomicFree(req);
        return ret;
    }

    output_ids = calloc(xf86_config->num_output, sizeof(uint32_t));
    if (!output_ids)
        return -1;

    for (i = 0; i < xf86_config->num_output; i++) {
        xf86OutputPtr output = xf86_config->output[i];
        drmmode_output_private_ptr drmmode_output;

        if (output->crtc != crtc)
            continue;

        drmmode_output = output->driver_private;
        if (drmmode_output->output_id == -1)
            continue;
        output_ids[output_count] = drmmode_output->output_id;
        output_count++;

        ctm = &drmmode_output->ctm;
    }

    drmmode_ConvertToKMode(crtc->scrn, &kmode, &crtc->mode);
    ret = drmModeSetCrtc(drmmode->fd, drmmode_crtc->mode_crtc->crtc_id,
                         fb_id, x, y, output_ids, output_count, &kmode);
    if (!ret && !ms->atomic_modeset) {
        drmmode_crtc->src_x = x;
        drmmode_crtc->src_y = y;
    }

    drmmode_set_ctm(crtc, ctm);

    free(output_ids);
    return ret;
}

int
drmmode_crtc_flip(xf86CrtcPtr crtc, uint32_t fb_id, int x, int y,
                  uint32_t flags, void *data)
{
    modesettingPtr ms = modesettingPTR(crtc->scrn);
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    int ret;

    if (ms->atomic_modeset) {
        drmModeAtomicReq *req = drmModeAtomicAlloc();

        if (!req)
            return 1;

        ret = plane_add_props(req, crtc, fb_id, x, y);
        flags |= DRM_MODE_ATOMIC_NONBLOCK;
        if (ret == 0)
            ret = drmModeAtomicCommit(ms->fd, req, flags, data);
        drmModeAtomicFree(req);
        return ret;
    }

    /* The frame buffer source coordinates may change when switching between the
     * primary frame buffer and a per-CRTC frame buffer. Set the correct source
     * coordinates if they differ for this flip.
     */
    if (drmmode_crtc->src_x != x || drmmode_crtc->src_y != y) {
        ret = drmModeSetPlane(ms->fd, drmmode_crtc->plane_id,
                              drmmode_crtc->mode_crtc->crtc_id, fb_id, 0,
                              0, 0, crtc->mode.HDisplay, crtc->mode.VDisplay,
                              x << 16, y << 16, crtc->mode.HDisplay << 16,
                              crtc->mode.VDisplay << 16);
        if (ret) {
            xf86DrvMsg(crtc->scrn->scrnIndex, X_WARNING,
                       "error changing fb src coordinates for flip: %d\n", ret);
            return ret;
        }

        drmmode_crtc->src_x = x;
        drmmode_crtc->src_y = y;
    }

    return drmModePageFlip(ms->fd, drmmode_crtc->mode_crtc->crtc_id,
                           fb_id, flags, data);
}

void
drmmode_bo_destroy(drmmode_ptr drmmode, drmmode_bo *bo)
{
#ifdef GLAMOR_HAS_GBM
    if (bo->gbm) {
        gbm_bo_destroy(bo->gbm);
        bo->gbm = NULL;
    }
#endif

    if (bo->dumb) {
        int ret = dumb_bo_destroy(drmmode->fd, bo->dumb);
        if (ret == 0) {
            bo->dumb = NULL;
        }
    }
}

uint32_t
drmmode_bo_get_pitch(drmmode_bo *bo)
{
#ifdef GLAMOR_HAS_GBM
    if (bo->gbm) {
        return gbm_bo_get_stride(bo->gbm);
    }
#endif

    if (bo->dumb) {
        return bo->dumb->pitch;
    }

    return 0;
}

static void*
drmmode_bo_get_bo(drmmode_bo *bo)
{
#ifdef GLAMOR_HAS_GBM
    if (bo->gbm) {
        return bo->gbm;
    }
#endif

    return bo->dumb;
}

static uint32_t
drmmode_bo_get_handle(drmmode_bo *bo)
{
#ifdef GLAMOR_HAS_GBM
    if (bo->gbm) {
        return gbm_bo_get_handle(bo->gbm).u32;
    }
#endif

    if (bo->dumb) {
        return bo->dumb->handle;
    }

    return (uint32_t)-1;
}

static void*
drmmode_bo_map(drmmode_ptr drmmode, drmmode_bo *bo)
{
    if (bo->map) {
        return bo->map;
    }

#ifdef GLAMOR_HAS_GBM
    if (bo->gbm) {
        /* We shouldn't read from gpu memory */
        uint32_t stride;
        void* unused;
        void* map = gbm_bo_map(bo->gbm, 0, 0, bo->width, bo->height, GBM_BO_TRANSFER_WRITE, &stride, &unused);
        if (map) {
            bo->map = map;
            return bo->map;
        }
    }
#endif

    if (bo->dumb) {
        int ret = dumb_bo_map(drmmode->fd, bo->dumb);
        if (ret) {
            return NULL;
        }

        bo->map = bo->dumb->ptr;
        return bo->map;
    }

    return NULL;
}

int
drmmode_bo_import(drmmode_ptr drmmode, drmmode_bo *bo,
                  uint32_t *fb_id)
{
#ifdef GBM_BO_WITH_MODIFIERS
    modesettingPtr ms = modesettingPTR(drmmode->scrn);
    if (bo->gbm && ms->kms_has_modifiers &&
        gbm_bo_get_modifier(bo->gbm) != DRM_FORMAT_MOD_INVALID) {
        int num_fds;

        num_fds = gbm_bo_get_plane_count(bo->gbm);
        if (num_fds > 0) {
            int i;
            uint32_t format;
            uint32_t handles[4];
            uint32_t strides[4];
            uint32_t offsets[4];
            uint64_t modifiers[4];

            memset(handles, 0, sizeof(handles));
            memset(strides, 0, sizeof(strides));
            memset(offsets, 0, sizeof(offsets));
            memset(modifiers, 0, sizeof(modifiers));

            format = gbm_bo_get_format(bo->gbm);
            format = get_opaque_format(format);
            for (i = 0; i < num_fds; i++) {
                handles[i] = gbm_bo_get_handle_for_plane(bo->gbm, i).u32;
                strides[i] = gbm_bo_get_stride_for_plane(bo->gbm, i);
                offsets[i] = gbm_bo_get_offset(bo->gbm, i);
                modifiers[i] = gbm_bo_get_modifier(bo->gbm);
            }

            return drmModeAddFB2WithModifiers(drmmode->fd, bo->width, bo->height,
                                              format, handles, strides,
                                              offsets, modifiers, fb_id,
                                              DRM_MODE_FB_MODIFIERS);
        }
    }
#endif
    return drmModeAddFB(drmmode->fd, bo->width, bo->height,
                        drmmode->scrn->depth, drmmode->kbpp,
                        drmmode_bo_get_pitch(bo),
                        drmmode_bo_get_handle(bo), fb_id);
}

#ifdef GLAMOR_HAS_GBM
/* formats taken from glamor/glamor_egl.c */
static inline uint32_t
drmmode_gbm_format_for_depth(int depth)
{
    switch (depth) {
    case 8:
        return GBM_FORMAT_R8;
    case 15:
        return GBM_FORMAT_ARGB1555;
    case 16:
        return GBM_FORMAT_RGB565;
    case 30:
        /* XXX Is this format right? https://github.com/X11Libre/xserver/pull/1396/files#r2523698616 XXX */
        return GBM_FORMAT_ARGB2101010;
    default:
        return GBM_FORMAT_ARGB8888;
    }
}
#endif

static Bool
drmmode_create_front_bo(drmmode_ptr drmmode, drmmode_bo *bo,
                        unsigned width, unsigned height, unsigned bpp)
{
    bo->width = width;
    bo->height = height;

#ifdef GLAMOR_HAS_GBM
    if (drmmode->glamor) {
        uint32_t format = drmmode_gbm_format_for_depth(drmmode->scrn->depth);

#ifdef GBM_BO_WITH_MODIFIERS
        uint32_t num_modifiers;
        uint64_t *modifiers = NULL;
        num_modifiers = get_modifiers_set(drmmode->scrn, format, &modifiers,
                                          FALSE, TRUE, TRUE);
        if (num_modifiers) {
#ifdef GBM_BO_WITH_MODIFIERS2
            bo->gbm = gbm_bo_create_with_modifiers2(drmmode->gbm, width, height,
                                                    format, modifiers, num_modifiers,
                                                    GBM_BO_USE_RENDERING | GBM_BO_USE_SCANOUT |
                                                    GBM_BO_USE_FRONT_RENDERING);
#else
            bo->gbm = gbm_bo_create_with_modifiers(drmmode->gbm, width, height,
                                                   format, modifiers,
                                                   num_modifiers);
#endif
            free(modifiers);
            if (bo->gbm) {
                bo->used_modifiers = TRUE;
                return TRUE;
            }
        }
#endif

        bo->gbm = gbm_bo_create(drmmode->gbm, width, height, format,
                                GBM_BO_USE_RENDERING | GBM_BO_USE_SCANOUT |
                                GBM_BO_USE_FRONT_RENDERING);
        bo->used_modifiers = FALSE;
        return bo->gbm != NULL;
    }
#endif

    bo->dumb = dumb_bo_create(drmmode->fd, width, height, bpp);
    return bo->dumb != NULL;
}

Bool
drmmode_SetSlaveBO(PixmapPtr ppix,
                   drmmode_ptr drmmode, int fd_handle, int pitch, int size)
{
    msPixmapPrivPtr ppriv = msGetPixmapPriv(drmmode, ppix);

    if (fd_handle == -1) {
        dumb_bo_destroy(drmmode->fd, ppriv->backing_bo);
        ppriv->backing_bo = NULL;
        return TRUE;
    }

    ppriv->backing_bo =
        dumb_get_bo_from_fd(drmmode->fd, fd_handle, pitch, size);
    if (!ppriv->backing_bo)
        return FALSE;

    close(fd_handle);
    return TRUE;
}

static Bool
drmmode_SharedPixmapPresent(PixmapPtr ppix, xf86CrtcPtr crtc,
                            drmmode_ptr drmmode)
{
    ScreenPtr primary = crtc->randr_crtc->pScreen->current_primary;

    if (primary->PresentSharedPixmap(ppix)) {
        /* Success, queue flip to back target */
        if (drmmode_SharedPixmapFlip(ppix, crtc, drmmode))
            return TRUE;

        xf86DrvMsg(drmmode->scrn->scrnIndex, X_WARNING,
                   "drmmode_SharedPixmapFlip() failed, trying again next vblank\n");

        return drmmode_SharedPixmapPresentOnVBlank(ppix, crtc, drmmode);
    }

    /* Failed to present, try again on next vblank after damage */
    if (primary->RequestSharedPixmapNotifyDamage) {
        msPixmapPrivPtr ppriv = msGetPixmapPriv(drmmode, ppix);

        /* Set flag first in case we are immediately notified */
        ppriv->wait_for_damage = TRUE;

        if (primary->RequestSharedPixmapNotifyDamage(ppix))
            return TRUE;
        else
            ppriv->wait_for_damage = FALSE;
    }

    /* Damage notification not available, just try again on vblank */
    return drmmode_SharedPixmapPresentOnVBlank(ppix, crtc, drmmode);
}

struct vblank_event_args {
    PixmapPtr frontTarget;
    PixmapPtr backTarget;
    xf86CrtcPtr crtc;
    drmmode_ptr drmmode;
    Bool flip;
};
static void
drmmode_SharedPixmapVBlankEventHandler(uint64_t frame, uint64_t usec,
                                       void *data)
{
    struct vblank_event_args *args = data;

    drmmode_crtc_private_ptr drmmode_crtc = args->crtc->driver_private;

    if (args->flip) {
        /* frontTarget is being displayed, update crtc to reflect */
        drmmode_crtc->prime_pixmap = args->frontTarget;
        drmmode_crtc->prime_pixmap_back = args->backTarget;

        /* Safe to present on backTarget, no longer displayed */
        drmmode_SharedPixmapPresent(args->backTarget, args->crtc, args->drmmode);
    } else {
        /* backTarget is still being displayed, present on frontTarget */
        drmmode_SharedPixmapPresent(args->frontTarget, args->crtc, args->drmmode);
    }

    free(args);
}

static void
drmmode_SharedPixmapVBlankEventAbort(void *data)
{
    struct vblank_event_args *args = data;

    msGetPixmapPriv(args->drmmode, args->frontTarget)->flip_seq = 0;

    free(args);
}

Bool
drmmode_SharedPixmapPresentOnVBlank(PixmapPtr ppix, xf86CrtcPtr crtc,
                                    drmmode_ptr drmmode)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    msPixmapPrivPtr ppriv = msGetPixmapPriv(drmmode, ppix);
    struct vblank_event_args *event_args;

    if (ppix == drmmode_crtc->prime_pixmap)
        return FALSE; /* Already flipped to this pixmap */
    if (ppix != drmmode_crtc->prime_pixmap_back)
        return FALSE; /* Pixmap is not a scanout pixmap for CRTC */

    event_args = calloc(1, sizeof(*event_args));
    if (!event_args)
        return FALSE;

    event_args->frontTarget = ppix;
    event_args->backTarget = drmmode_crtc->prime_pixmap;
    event_args->crtc = crtc;
    event_args->drmmode = drmmode;
    event_args->flip = FALSE;

    ppriv->flip_seq =
        ms_drm_queue_alloc(crtc, event_args,
                           drmmode_SharedPixmapVBlankEventHandler,
                           drmmode_SharedPixmapVBlankEventAbort);

    return ms_queue_vblank(crtc, MS_QUEUE_RELATIVE, 1, NULL, ppriv->flip_seq);
}

Bool
drmmode_SharedPixmapFlip(PixmapPtr frontTarget, xf86CrtcPtr crtc,
                         drmmode_ptr drmmode)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    msPixmapPrivPtr ppriv_front = msGetPixmapPriv(drmmode, frontTarget);

    struct vblank_event_args *event_args;

    event_args = calloc(1, sizeof(*event_args));
    if (!event_args)
        return FALSE;

    event_args->frontTarget = frontTarget;
    event_args->backTarget = drmmode_crtc->prime_pixmap;
    event_args->crtc = crtc;
    event_args->drmmode = drmmode;
    event_args->flip = TRUE;

    ppriv_front->flip_seq =
        ms_drm_queue_alloc(crtc, event_args,
                           drmmode_SharedPixmapVBlankEventHandler,
                           drmmode_SharedPixmapVBlankEventAbort);

    if (drmModePageFlip(drmmode->fd, drmmode_crtc->mode_crtc->crtc_id,
                        ppriv_front->fb_id, DRM_MODE_PAGE_FLIP_EVENT,
                        (void *)(intptr_t) ppriv_front->flip_seq) < 0) {
        ms_drm_abort_seq(crtc->scrn, ppriv_front->flip_seq);
        return FALSE;
    }

    return TRUE;
}

static Bool
drmmode_InitSharedPixmapFlipping(xf86CrtcPtr crtc, drmmode_ptr drmmode)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

    if (!drmmode_crtc->enable_flipping)
        return FALSE;

    if (drmmode_crtc->flipping_active)
        return TRUE;

    drmmode_crtc->flipping_active =
        drmmode_SharedPixmapPresent(drmmode_crtc->prime_pixmap_back,
                                    crtc, drmmode);

    return drmmode_crtc->flipping_active;
}

static void
drmmode_FiniSharedPixmapFlipping(xf86CrtcPtr crtc, drmmode_ptr drmmode)
{
    uint32_t seq;
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

    if (!drmmode_crtc->flipping_active)
        return;

    drmmode_crtc->flipping_active = FALSE;

    /* Abort page flip event handler on prime_pixmap */
    seq = msGetPixmapPriv(drmmode, drmmode_crtc->prime_pixmap)->flip_seq;
    if (seq)
        ms_drm_abort_seq(crtc->scrn, seq);

    /* Abort page flip event handler on prime_pixmap_back */
    seq = msGetPixmapPriv(drmmode,
                          drmmode_crtc->prime_pixmap_back)->flip_seq;
    if (seq)
        ms_drm_abort_seq(crtc->scrn, seq);
}

static Bool drmmode_set_target_scanout_pixmap(xf86CrtcPtr crtc, PixmapPtr ppix,
                                              PixmapPtr *target);

Bool
drmmode_EnableSharedPixmapFlipping(xf86CrtcPtr crtc, drmmode_ptr drmmode,
                                   PixmapPtr front, PixmapPtr back)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

    drmmode_crtc->enable_flipping = TRUE;

    /* Set front scanout pixmap */
    drmmode_crtc->enable_flipping &=
        drmmode_set_target_scanout_pixmap(crtc, front,
                                          &drmmode_crtc->prime_pixmap);
    if (!drmmode_crtc->enable_flipping)
        return FALSE;

    /* Set back scanout pixmap */
    drmmode_crtc->enable_flipping &=
        drmmode_set_target_scanout_pixmap(crtc, back,
                                          &drmmode_crtc->prime_pixmap_back);
    if (!drmmode_crtc->enable_flipping) {
        drmmode_set_target_scanout_pixmap(crtc, NULL,
                                          &drmmode_crtc->prime_pixmap);
        return FALSE;
    }

    return TRUE;
}

void
drmmode_DisableSharedPixmapFlipping(xf86CrtcPtr crtc, drmmode_ptr drmmode)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

    drmmode_crtc->enable_flipping = FALSE;

    drmmode_FiniSharedPixmapFlipping(crtc, drmmode);

    drmmode_set_target_scanout_pixmap(crtc, NULL, &drmmode_crtc->prime_pixmap);

    drmmode_set_target_scanout_pixmap(crtc, NULL,
                                      &drmmode_crtc->prime_pixmap_back);
}

static void
drmmode_ConvertFromKMode(ScrnInfoPtr scrn,
                         drmModeModeInfo * kmode, DisplayModePtr mode)
{
    memset(mode, 0, sizeof(DisplayModeRec));
    mode->status = MODE_OK;

    mode->Clock = kmode->clock;

    mode->HDisplay = kmode->hdisplay;
    mode->HSyncStart = kmode->hsync_start;
    mode->HSyncEnd = kmode->hsync_end;
    mode->HTotal = kmode->htotal;
    mode->HSkew = kmode->hskew;

    mode->VDisplay = kmode->vdisplay;
    mode->VSyncStart = kmode->vsync_start;
    mode->VSyncEnd = kmode->vsync_end;
    mode->VTotal = kmode->vtotal;
    mode->VScan = kmode->vscan;

    mode->Flags = kmode->flags; //& FLAG_BITS;
    mode->name = strdup(kmode->name);

    if (kmode->type & DRM_MODE_TYPE_DRIVER)
        mode->type = M_T_DRIVER;
    if (kmode->type & DRM_MODE_TYPE_PREFERRED)
        mode->type |= M_T_PREFERRED;
    xf86SetModeCrtc(mode, scrn->adjustFlags);
}

static void
drmmode_ConvertToKMode(ScrnInfoPtr scrn,
                       drmModeModeInfo * kmode, DisplayModePtr mode)
{
    memset(kmode, 0, sizeof(*kmode));

    kmode->clock = mode->Clock;
    kmode->hdisplay = mode->HDisplay;
    kmode->hsync_start = mode->HSyncStart;
    kmode->hsync_end = mode->HSyncEnd;
    kmode->htotal = mode->HTotal;
    kmode->hskew = mode->HSkew;

    kmode->vdisplay = mode->VDisplay;
    kmode->vsync_start = mode->VSyncStart;
    kmode->vsync_end = mode->VSyncEnd;
    kmode->vtotal = mode->VTotal;
    kmode->vscan = mode->VScan;

    kmode->flags = mode->Flags; //& FLAG_BITS;
    if (mode->name)
        strncpy(kmode->name, mode->name, DRM_DISPLAY_MODE_LEN);
    kmode->name[DRM_DISPLAY_MODE_LEN - 1] = 0;

}

static void
drmmode_crtc_dpms(xf86CrtcPtr crtc, int mode)
{
    modesettingPtr ms = modesettingPTR(crtc->scrn);
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_ptr drmmode = drmmode_crtc->drmmode;

    /* XXX Check if DPMS mode is already the right one */

    drmmode_crtc->dpms_mode = mode;

    if (ms->atomic_modeset) {
        if (mode != DPMSModeOn && !ms->pending_modeset)
            drmmode_crtc_disable(crtc);
    } else if (crtc->enabled == FALSE) {
        drmModeSetCrtc(drmmode->fd, drmmode_crtc->mode_crtc->crtc_id,
                       0, 0, 0, NULL, 0, NULL);
    }
}

#ifdef GLAMOR_HAS_GBM
static PixmapPtr
create_pixmap_for_fbcon(drmmode_ptr drmmode, ScrnInfoPtr pScrn, int fbcon_id)
{
    PixmapPtr pixmap = drmmode->fbcon_pixmap;
    drmModeFBPtr fbcon;
    ScreenPtr pScreen = xf86ScrnToScreen(pScrn);
    modesettingPtr ms = modesettingPTR(pScrn);
    Bool ret;

    if (pixmap)
        return pixmap;

    fbcon = drmModeGetFB(drmmode->fd, fbcon_id);
    if (fbcon == NULL)
        return NULL;

    if (fbcon->depth != pScrn->depth ||
        fbcon->width != pScrn->virtualX ||
        fbcon->height != pScrn->virtualY)
        goto out_free_fb;

    pixmap = drmmode_create_pixmap_header(pScreen, fbcon->width,
                                          fbcon->height, fbcon->depth,
                                          fbcon->bpp, fbcon->pitch, NULL);
    if (!pixmap)
        goto out_free_fb;

    ret = ms->glamor.egl_create_textured_pixmap(pixmap, fbcon->handle,
                                                fbcon->pitch);
    if (!ret) {
      FreePixmap(pixmap);
      pixmap = NULL;
    }

    drmmode->fbcon_pixmap = pixmap;
out_free_fb:
    drmModeFreeFB(fbcon);
    return pixmap;
}
#endif

void
drmmode_copy_fb(ScrnInfoPtr pScrn, drmmode_ptr drmmode)
{
#ifdef GLAMOR_HAS_GBM
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
    ScreenPtr pScreen = xf86ScrnToScreen(pScrn);
    PixmapPtr src, dst;
    int fbcon_id = 0;
    GCPtr gc;
    int i;

    for (i = 0; i < xf86_config->num_crtc; i++) {
        drmmode_crtc_private_ptr drmmode_crtc = xf86_config->crtc[i]->driver_private;
        if (drmmode_crtc->mode_crtc->buffer_id)
            fbcon_id = drmmode_crtc->mode_crtc->buffer_id;
    }

    if (!fbcon_id)
        return;

    if (fbcon_id == drmmode->fb_id) {
        /* in some rare case there might be no fbcon and we might already
         * be the one with the current fb to avoid a false deadlck in
         * kernel ttm code just do nothing as anyway there is nothing
         * to do
         */
        return;
    }

    src = create_pixmap_for_fbcon(drmmode, pScrn, fbcon_id);
    if (!src)
        return;

    dst = pScreen->GetScreenPixmap(pScreen);

    gc = GetScratchGC(pScrn->depth, pScreen);
    ValidateGC(&dst->drawable, gc);

    (void) (*gc->ops->CopyArea)(&src->drawable, &dst->drawable, gc, 0, 0,
                         pScrn->virtualX, pScrn->virtualY, 0, 0);

    FreeScratchGC(gc);

    pScreen->canDoBGNoneRoot = TRUE;

    dixDestroyPixmap(drmmode->fbcon_pixmap, 0);
    drmmode->fbcon_pixmap = NULL;
#endif
}

void
drmmode_copy_damage(xf86CrtcPtr crtc, PixmapPtr dst, RegionPtr dmg, Bool empty)
{
#ifdef GLAMOR_HAS_GBM
    ScreenPtr pScreen = xf86ScrnToScreen(crtc->scrn);
    DrawableRec *src;

    /* Copy the screen's pixmap into the destination pixmap */
    if (crtc->rotatedPixmap) {
        src = &crtc->rotatedPixmap->drawable;
        xf86RotateCrtcRedisplay(crtc, dst, src, dmg, FALSE);
    } else {
        src = &pScreen->GetScreenPixmap(pScreen)->drawable;
        PixmapDirtyCopyArea(dst, src, 0, 0, -crtc->x, -crtc->y, dmg);
    }

    /* Reset the damages if requested */
    if (empty)
        RegionEmpty(dmg);

    /* Wait until the GC operations finish */
    modesettingPTR(crtc->scrn)->glamor.finish(pScreen);
#endif
}

static void
drmmode_shadow_fb_destroy(xf86CrtcPtr crtc, PixmapPtr pixmap,
                          void *data, drmmode_bo *bo, uint32_t *fb_id);
static void
drmmode_destroy_tearfree_shadow(xf86CrtcPtr crtc)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_tearfree_ptr trf = &drmmode_crtc->tearfree;
    int i;

    if (trf->flip_seq)
        ms_drm_abort_seq(crtc->scrn, trf->flip_seq);

    for (i = 0; i < ARRAY_SIZE(trf->buf); i++) {
        if (trf->buf[i].px) {
            drmmode_shadow_fb_destroy(crtc, trf->buf[i].px, (void *)(long)1,
                                      &trf->buf[i].bo, &trf->buf[i].fb_id);
            trf->buf[i].px = NULL;
            RegionUninit(&trf->buf[i].dmg);
        }
    }
}

static PixmapPtr
drmmode_shadow_fb_create(xf86CrtcPtr crtc, void *data, int width, int height,
                         drmmode_bo *bo, uint32_t *fb_id);
static Bool
drmmode_create_tearfree_shadow(xf86CrtcPtr crtc)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_ptr drmmode = drmmode_crtc->drmmode;
    drmmode_tearfree_ptr trf = &drmmode_crtc->tearfree;
    uint32_t w = crtc->mode.HDisplay, h = crtc->mode.VDisplay;
    int i;

    if (!drmmode->tearfree_enable)
        return TRUE;

    /* Destroy the old mode's buffers and make new ones */
    drmmode_destroy_tearfree_shadow(crtc);
    for (i = 0; i < ARRAY_SIZE(trf->buf); i++) {
        trf->buf[i].px = drmmode_shadow_fb_create(crtc, NULL, w, h,
                                                  &trf->buf[i].bo,
                                                  &trf->buf[i].fb_id);
        if (!trf->buf[i].px) {
            drmmode_destroy_tearfree_shadow(crtc);
            xf86DrvMsg(crtc->scrn->scrnIndex, X_ERROR,
                       "shadow creation failed for TearFree buf%d\n", i);
            return FALSE;
        }
        RegionInit(&trf->buf[i].dmg, &crtc->bounds, 0);
    }

    /* Initialize the front buffer with the current scanout */
    drmmode_copy_damage(crtc, trf->buf[trf->back_idx ^ 1].px,
                        &trf->buf[trf->back_idx ^ 1].dmg, TRUE);
    return TRUE;
}

static void drmmmode_prepare_modeset(ScrnInfoPtr scrn)
{
    ScreenPtr pScreen = scrn->pScreen;
    modesettingPtr ms = modesettingPTR(scrn);

    if (ms->drmmode.pending_modeset)
        return;

    /*
     * Force present to unflip everything before we might
     * try lighting up new displays. This makes sure fancy
     * modifiers can't cause the modeset to fail.
     */
    ms->drmmode.pending_modeset = TRUE;
    present_check_flips(pScreen->root);
    ms->drmmode.pending_modeset = FALSE;

    ms_drain_drm_events(pScreen);
}

static Bool
drmmode_set_mode_major(xf86CrtcPtr crtc, DisplayModePtr mode,
                       Rotation rotation, int x, int y)
{
    modesettingPtr ms = modesettingPTR(crtc->scrn);
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(crtc->scrn);
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_ptr drmmode = drmmode_crtc->drmmode;
    int saved_x, saved_y;
    Rotation saved_rotation;
    DisplayModeRec saved_mode;
    Bool ret = TRUE;
    Bool can_test;
    int i;

    if (mode)
        drmmmode_prepare_modeset(crtc->scrn);

    saved_mode = crtc->mode;
    saved_x = crtc->x;
    saved_y = crtc->y;
    saved_rotation = crtc->rotation;

    if (mode) {
        crtc->mode = *mode;
        crtc->x = x;
        crtc->y = y;
        crtc->rotation = rotation;

        if (!xf86CrtcRotate(crtc)) {
            goto done;
        }

        crtc->funcs->gamma_set(crtc, crtc->gamma_red, crtc->gamma_green,
                               crtc->gamma_blue, crtc->gamma_size);

        ret = drmmode_create_tearfree_shadow(crtc);
        if (!ret)
            goto done;

        can_test = drmmode_crtc_can_test_mode(crtc);
        if (drmmode_crtc_set_mode(crtc, can_test)) {
            xf86DrvMsg(crtc->scrn->scrnIndex, X_ERROR,
                       "failed to set mode: %s\n", strerror(errno));
            ret = FALSE;
            goto done;
        } else
            ret = TRUE;

        if (crtc->scrn->pScreen)
            xf86CrtcSetScreenSubpixelOrder(crtc->scrn->pScreen);

        ms->pending_modeset = TRUE;
        drmmode_crtc->need_modeset = FALSE;
        crtc->funcs->dpms(crtc, DPMSModeOn);

        if (drmmode_crtc->prime_pixmap_back)
            drmmode_InitSharedPixmapFlipping(crtc, drmmode);

        /* go through all the outputs and force DPMS them back on? */
        for (i = 0; i < xf86_config->num_output; i++) {
            xf86OutputPtr output = xf86_config->output[i];
            drmmode_output_private_ptr drmmode_output;

            if (output->crtc != crtc)
                continue;

            drmmode_output = output->driver_private;
            if (drmmode_output->output_id == -1)
                continue;
            output->funcs->dpms(output, DPMSModeOn);
        }

        /* if we only tested the mode previously, really set it now */
        if (can_test)
            drmmode_crtc_set_mode(crtc, FALSE);
        ms->pending_modeset = FALSE;
    }

 done:
    if (!ret) {
        crtc->x = saved_x;
        crtc->y = saved_y;
        crtc->rotation = saved_rotation;
        crtc->mode = saved_mode;
        drmmode_create_tearfree_shadow(crtc);
    } else
        crtc->active = TRUE;

    return ret;
}

static void
drmmode_set_cursor_colors(xf86CrtcPtr crtc, int bg, int fg)
{

}

static void
drmmode_set_cursor_position(xf86CrtcPtr crtc, int x, int y)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_ptr drmmode = drmmode_crtc->drmmode;

    drmModeMoveCursor(drmmode->fd, drmmode_crtc->mode_crtc->crtc_id, x, y);
}

static Bool
drmmode_set_cursor(xf86CrtcPtr crtc, int width, int height)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_ptr drmmode = drmmode_crtc->drmmode;
    uint32_t handle = drmmode_crtc->cursor.bo->handle;
    CursorPtr cursor = xf86CurrentCursor(crtc->scrn->pScreen);
    int ret = -EINVAL;

    if (cursor == NullCursor)
        return TRUE;

    ret = drmModeSetCursor2(drmmode->fd, drmmode_crtc->mode_crtc->crtc_id,
                            handle, width, height,
                            cursor->bits->xhot, cursor->bits->yhot);

    /* -EINVAL can mean that an old kernel supports drmModeSetCursor but
     * not drmModeSetCursor2, though it can mean other things too. */
    if (ret == -EINVAL)
        ret = drmModeSetCursor(drmmode->fd, drmmode_crtc->mode_crtc->crtc_id,
                               handle, width, height);

    /* -ENXIO normally means that the current drm driver supports neither
     * cursor_set nor cursor_set2.  Disable hardware cursor support for
     * the rest of the session in that case. */
    if (ret == -ENXIO) {
        xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(crtc->scrn);
        xf86CursorInfoPtr cursor_info = xf86_config->cursor_info;

        cursor_info->MaxWidth = cursor_info->MaxHeight = 0;
        drmmode_crtc->drmmode->sw_cursor = TRUE;
        drmmode_crtc->drmmode->set_cursor_failed = TRUE;
    }

    if (ret) {
        /* fallback to swcursor */
        return FALSE;
    }

    return TRUE;
}

static int
drmmode_cursor_get_pitch(drmmode_crtc_private_ptr drmmode_crtc, int idx)
{
    drmmode_ptr drmmode = drmmode_crtc->drmmode;
    drmmode_cursor_ptr drmmode_cursor = &drmmode_crtc->cursor;

    int width  = drmmode_cursor->dimensions[idx].width;
    int height = drmmode_cursor->dimensions[idx].height;

    int num_pitches = drmmode_cursor->num_dimensions;

    if (!drmmode_crtc->cursor_pitches) {
        drmmode_crtc->cursor_pitches = calloc(num_pitches, sizeof(int));
        if (!drmmode_crtc->cursor_pitches) {
            /* we couldn't allocate memory for the cache, so we don't cache the result */
            int ret;
            struct dumb_bo *bo = dumb_bo_create(drmmode->fd, width, height, drmmode->kbpp);
            if (!bo) {
                /* We couldn't allocate a bo, so we try to guess the pitch */
                return MAX(width, 64);
            }

            ret = bo->pitch / drmmode->cpp;

            dumb_bo_destroy(drmmode->fd, bo);
            return ret;
        }
    }

    if (drmmode_crtc->cursor_pitches[idx]) {
        /* return the cached pitch */
        return drmmode_crtc->cursor_pitches[idx];
    }

    struct dumb_bo *bo = dumb_bo_create(drmmode->fd, width, height, drmmode->kbpp);
    if (!bo) {
        /* We couldn't allocate a bo, so we try to guess the pitch */
        return MAX(width, 64);
    }

    drmmode_crtc->cursor_pitches[idx] = bo->pitch / drmmode->cpp;

    dumb_bo_destroy(drmmode->fd, bo);
    return drmmode_crtc->cursor_pitches[idx];
}

static void
drmmode_paint_cursor(struct dumb_bo *cursor_bo, int cursor_pitch, int cursor_width, int cursor_height,
                     const CARD32 * restrict image, int image_width, int image_height,
                     drmmode_crtc_private_ptr restrict drmmode_crtc, int glyph_width, int glyph_height)
{
    int width_todo;
    int height_todo;

    CARD32 *cursor = cursor_bo->ptr;

    /*
     * The image buffer can be smaller than the cursor buffer.
     * This means that we can't clear the cursor by copying '\0' bytes
     * from the image buffer, because we might read out of bounds.
     */
    if (
        /* If the buffer is uninitialized, assume it is dirty */
        (drmmode_crtc->cursor_glyph_width == 0 &&
         drmmode_crtc->cursor_glyph_height == 0) ||

        /* Sanity check so we don't read from the image out of bounds */
        (drmmode_crtc->cursor_glyph_width > image_width ||
         drmmode_crtc->cursor_glyph_height > image_height) ||

        /* If the pitch changed, the memory layout of the cursor data changed, so the buffer is dirty */
        /* See: https://github.com/X11Libre/xserver/pull/1234 */
        (drmmode_crtc->old_pitch != cursor_pitch)
       ) {
        memset(cursor, 0, cursor_bo->size);

        /* Since we already cleared the buffer, no need to clear it again bellow */
        drmmode_crtc->cursor_glyph_width = 0;
        drmmode_crtc->cursor_glyph_height = 0;
    }

    drmmode_crtc->old_pitch = cursor_pitch;

    /* Paint only what we need to */
    width_todo = MAX(drmmode_crtc->cursor_glyph_width, glyph_width);
    height_todo = MAX(drmmode_crtc->cursor_glyph_height, glyph_height);

    /* remember the size of the current cursor glyph */
    drmmode_crtc->cursor_glyph_width = glyph_width;
    drmmode_crtc->cursor_glyph_height = glyph_height;

    for (int i = 0; i < height_todo; i++) {
        memcpy(cursor + i * cursor_pitch, image + i * image_width, width_todo * sizeof(*cursor));    /* cpu_to_le32(image[i]); */
    }
}

static void drmmode_hide_cursor(xf86CrtcPtr crtc);
static void drmmode_probe_cursor_size(xf86CrtcPtr crtc);

/*
 * The load_cursor_argb_check driver hook.
 *
 * Sets the hardware cursor by calling the drmModeSetCursor2 ioctl.
 * On failure, returns FALSE indicating that the X server should fall
 * back to software cursors.
 */
static Bool
drmmode_load_cursor_argb_check(xf86CrtcPtr crtc, CARD32 *image)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    modesettingPtr ms = modesettingPTR(crtc->scrn);
    CursorPtr cursor = xf86CurrentCursor(crtc->scrn->pScreen);
    int i;

    if (drmmode_crtc->cursor_up) {
        /* we probe the cursor so late, because we want to make sure that
           the screen is fully initialized and something is already drawn on it.
           Otherwise, we can't get reliable results with the probe. */
        drmmode_probe_cursor_size(crtc);
    }

    drmmode_cursor_rec drmmode_cursor = drmmode_crtc->cursor;

    /* Find the most compatiable size. */
    for (i = 0; i < drmmode_cursor.num_dimensions; i++)
    {
        drmmode_cursor_dim_rec dimensions = drmmode_cursor.dimensions[i];

        if (dimensions.width >= cursor->bits->width &&
            dimensions.height >= cursor->bits->height) {
                break;
        }
    }

    const int cursor_pitch = drmmode_cursor_get_pitch(drmmode_crtc, i);

    /* Get the resolution of the cursor. */
    int cursor_width  = drmmode_cursor.dimensions[i].width;
    int cursor_height = drmmode_cursor.dimensions[i].height;

    /* Get the size of the cursor image buffer */
    int image_width  = ms->cursor_image_width;
    int image_height = ms->cursor_image_height;

    /* cursor should be mapped already */
    drmmode_paint_cursor(drmmode_cursor.bo, cursor_pitch, cursor_width, cursor_height,
                         image, image_width, image_height,
                         drmmode_crtc, cursor->bits->width, cursor->bits->height);

    /* set cursor width and height here for drmmode_show_cursor */
    drmmode_crtc->cursor_width  = cursor_width;
    drmmode_crtc->cursor_height = cursor_height;

    return drmmode_crtc->cursor_up ? drmmode_set_cursor(crtc, cursor_width, cursor_height) : TRUE;
}

static void
drmmode_hide_cursor(xf86CrtcPtr crtc)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_ptr drmmode = drmmode_crtc->drmmode;

    drmmode_crtc->cursor_up = FALSE;
    drmModeSetCursor(drmmode->fd, drmmode_crtc->mode_crtc->crtc_id, 0,
                     drmmode_crtc->cursor_width, drmmode_crtc->cursor_height);
}

static Bool
drmmode_show_cursor(xf86CrtcPtr crtc)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_crtc->cursor_up = TRUE;
    return drmmode_set_cursor(crtc, drmmode_crtc->cursor_width, drmmode_crtc->cursor_height);
}

static void
drmmode_set_gamma_lut(drmmode_crtc_private_ptr drmmode_crtc,
                      uint16_t * red, uint16_t * green, uint16_t * blue,
                      int size)
{
    drmmode_ptr drmmode = drmmode_crtc->drmmode;
    drmmode_prop_info_ptr gamma_lut_info =
        &drmmode_crtc->props[DRMMODE_CRTC_GAMMA_LUT];
    const uint32_t crtc_id = drmmode_crtc->mode_crtc->crtc_id;
    struct drm_color_lut *lut = calloc(size, sizeof(struct drm_color_lut));
    if (!lut)
        return;

    assert(gamma_lut_info->prop_id != 0);

    for (int i = 0; i < size; i++) {
        lut[i].red = red[i];
        lut[i].green = green[i];
        lut[i].blue = blue[i];
        lut[i].reserved = 0;
    }

    uint32_t blob_id;
    if (drmModeCreatePropertyBlob(drmmode->fd, lut, size * sizeof(struct drm_color_lut), &blob_id)) {
        free(lut);
        return;
    }

    drmModeObjectSetProperty(drmmode->fd, crtc_id, DRM_MODE_OBJECT_CRTC,
                             gamma_lut_info->prop_id, blob_id);

    drmModeDestroyPropertyBlob(drmmode->fd, blob_id);
    free(lut);
}

static void
drmmode_crtc_gamma_set(xf86CrtcPtr crtc, uint16_t * red, uint16_t * green,
                       uint16_t * blue, int size)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_ptr drmmode = drmmode_crtc->drmmode;

    if (drmmode_crtc->use_gamma_lut) {
        drmmode_set_gamma_lut(drmmode_crtc, red, green, blue, size);
    } else {
        drmModeCrtcSetGamma(drmmode->fd, drmmode_crtc->mode_crtc->crtc_id,
                            size, red, green, blue);
    }
}

static Bool
drmmode_set_target_scanout_pixmap_gpu(xf86CrtcPtr crtc, PixmapPtr ppix,
                                      PixmapPtr *target)
{
    ScreenPtr screen = xf86ScrnToScreen(crtc->scrn);
    PixmapPtr screenpix = screen->GetScreenPixmap(screen);
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(crtc->scrn);
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_ptr drmmode = drmmode_crtc->drmmode;
    int c, total_width = 0, max_height = 0, this_x = 0;

    if (*target) {
        PixmapStopDirtyTracking(&(*target)->drawable, screenpix);
        if (drmmode->fb_id) {
            drmModeRmFB(drmmode->fd, drmmode->fb_id);
            drmmode->fb_id = 0;
        }
        drmmode_crtc->prime_pixmap_x = 0;
        *target = NULL;
    }

    if (!ppix)
        return TRUE;

    /* iterate over all the attached crtcs to work out the bounding box */
    for (c = 0; c < xf86_config->num_crtc; c++) {
        xf86CrtcPtr iter = xf86_config->crtc[c];
        if (!iter->enabled && iter != crtc)
            continue;
        if (iter == crtc) {
            this_x = total_width;
            total_width += ppix->drawable.width;
            if (max_height < ppix->drawable.height)
                max_height = ppix->drawable.height;
        } else {
            total_width += iter->mode.HDisplay;
            if (max_height < iter->mode.VDisplay)
                max_height = iter->mode.VDisplay;
        }
    }

    if (total_width != screenpix->drawable.width ||
        max_height != screenpix->drawable.height) {

        if (!drmmode_xf86crtc_resize(crtc->scrn, total_width, max_height))
            return FALSE;

        screenpix = screen->GetScreenPixmap(screen);
        screen->width = screenpix->drawable.width = total_width;
        screen->height = screenpix->drawable.height = max_height;
    }
    drmmode_crtc->prime_pixmap_x = this_x;
    PixmapStartDirtyTracking(&ppix->drawable, screenpix, 0, 0, this_x, 0,
                             RR_Rotate_0);
    *target = ppix;
    return TRUE;
}

static Bool
drmmode_set_target_scanout_pixmap_cpu(xf86CrtcPtr crtc, PixmapPtr ppix,
                                      PixmapPtr *target)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_ptr drmmode = drmmode_crtc->drmmode;
    msPixmapPrivPtr ppriv;
    void *ptr;

    if (*target) {
        ppriv = msGetPixmapPriv(drmmode, *target);
        drmModeRmFB(drmmode->fd, ppriv->fb_id);
        ppriv->fb_id = 0;
        if (ppriv->secondary_damage) {
            DamageUnregister(ppriv->secondary_damage);
            ppriv->secondary_damage = NULL;
        }
        *target = NULL;
    }

    if (!ppix)
        return TRUE;

    ppriv = msGetPixmapPriv(drmmode, ppix);
    if (!ppriv->secondary_damage) {
        ppriv->secondary_damage = DamageCreate(NULL, NULL,
                                           DamageReportNone,
                                           TRUE,
                                           crtc->randr_crtc->pScreen,
                                           NULL);
    }
    ptr = drmmode_map_secondary_bo(drmmode, ppriv);
    ppix->devPrivate.ptr = ptr;
    DamageRegister(&ppix->drawable, ppriv->secondary_damage);

    if (ppriv->fb_id == 0) {
        drmModeAddFB(drmmode->fd, ppix->drawable.width,
                     ppix->drawable.height,
                     ppix->drawable.depth,
                     ppix->drawable.bitsPerPixel,
                     ppix->devKind, ppriv->backing_bo->handle, &ppriv->fb_id);
    }
    *target = ppix;
    return TRUE;
}

static Bool
drmmode_set_target_scanout_pixmap(xf86CrtcPtr crtc, PixmapPtr ppix,
                                  PixmapPtr *target)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_ptr drmmode = drmmode_crtc->drmmode;

    if (drmmode->reverse_prime_offload_mode)
        return drmmode_set_target_scanout_pixmap_gpu(crtc, ppix, target);
    else
        return drmmode_set_target_scanout_pixmap_cpu(crtc, ppix, target);
}

static Bool
drmmode_set_scanout_pixmap(xf86CrtcPtr crtc, PixmapPtr ppix)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

    /* Use DisableSharedPixmapFlipping before switching to single buf */
    if (drmmode_crtc->enable_flipping)
        return FALSE;

    return drmmode_set_target_scanout_pixmap(crtc, ppix,
                                             &drmmode_crtc->prime_pixmap);
}

static void
drmmode_clear_pixmap(PixmapPtr pixmap)
{
    ScreenPtr screen = pixmap->drawable.pScreen;
    GCPtr gc;
#ifdef GLAMOR_HAS_GBM
    modesettingPtr ms = modesettingPTR(xf86ScreenToScrn(screen));

    if (ms->drmmode.glamor) {
        ms->glamor.clear_pixmap(pixmap);
        return;
    }
#endif

    gc = GetScratchGC(pixmap->drawable.depth, screen);
    if (gc) {
        miClearDrawable(&pixmap->drawable, gc);
        FreeScratchGC(gc);
    }
}

static void *
drmmode_shadow_fb_allocate(xf86CrtcPtr crtc, int width, int height,
                           drmmode_bo *bo, uint32_t *fb_id)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_ptr drmmode = drmmode_crtc->drmmode;
    int ret;

    /* If we're using shadow fb, we're not using glamor, so no gbm bo's */
    if (!drmmode_create_front_bo(drmmode, bo, width, height, drmmode->kbpp)) {
        xf86DrvMsg(crtc->scrn->scrnIndex, X_ERROR,
               "Couldn't allocate shadow memory for rotated CRTC\n");
        return NULL;
    }

    ret = drmmode_bo_import(drmmode, bo, fb_id);

    if (ret) {
        ErrorF("failed to add rotate fb\n");
        drmmode_bo_destroy(drmmode, bo);
        return NULL;
    }

    return drmmode_bo_get_bo(bo);
}

static void *
drmmode_shadow_allocate(xf86CrtcPtr crtc, int width, int height)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

    return drmmode_shadow_fb_allocate(crtc, width, height,
                                      &drmmode_crtc->rotate_bo,
                                      &drmmode_crtc->rotate_fb_id);
}

static PixmapPtr
drmmode_create_pixmap_header(ScreenPtr pScreen, int width, int height,
                             int depth, int bitsPerPixel, int devKind,
                             void *pPixData)
{
    PixmapPtr pixmap;

    /* width and height of 0 means don't allocate any pixmap data */
    pixmap = (*pScreen->CreatePixmap)(pScreen, 0, 0, depth, 0);

    if (pixmap) {
        if ((*pScreen->ModifyPixmapHeader)(pixmap, width, height, depth,
                                           bitsPerPixel, devKind, pPixData))
            return pixmap;
        dixDestroyPixmap(pixmap, 0);
    }
    return NullPixmap;
}

static Bool
drmmode_set_pixmap_bo(drmmode_ptr drmmode, PixmapPtr pixmap, drmmode_bo *bo);

static PixmapPtr
drmmode_shadow_fb_create(xf86CrtcPtr crtc, void *data, int width, int height,
                         drmmode_bo *bo, uint32_t *fb_id)
{
    ScrnInfoPtr scrn = crtc->scrn;
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_ptr drmmode = drmmode_crtc->drmmode;
    uint32_t pitch;
    PixmapPtr pixmap;
    void *pPixData = NULL;

    if (!data) {
        data = drmmode_shadow_fb_allocate(crtc, width, height, bo, fb_id);
        if (!data) {
            xf86DrvMsg(scrn->scrnIndex, X_ERROR,
                       "Couldn't allocate shadow pixmap for CRTC\n");
            return NULL;
        }
    }

    if (!drmmode_bo_get_bo(bo)) {
        xf86DrvMsg(scrn->scrnIndex, X_ERROR,
                   "Couldn't allocate shadow pixmap for CRTC\n");
        return NULL;
    }

    pPixData = drmmode_bo_map(drmmode, bo);
    pitch = drmmode_bo_get_pitch(bo);

    pixmap = drmmode_create_pixmap_header(scrn->pScreen,
                                          width, height,
                                          scrn->depth,
                                          drmmode->kbpp,
                                          pitch,
                                          pPixData);

    if (pixmap == NULL) {
        xf86DrvMsg(scrn->scrnIndex, X_ERROR,
                   "Couldn't allocate shadow pixmap for CRTC\n");
        return NULL;
    }

    drmmode_set_pixmap_bo(drmmode, pixmap, bo);

    return pixmap;
}

static PixmapPtr
drmmode_shadow_create(xf86CrtcPtr crtc, void *data, int width, int height)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

    return drmmode_shadow_fb_create(crtc, data, width, height,
                                    &drmmode_crtc->rotate_bo,
                                    &drmmode_crtc->rotate_fb_id);
}

static void
drmmode_shadow_fb_destroy(xf86CrtcPtr crtc, PixmapPtr pixmap,
                          void *data, drmmode_bo *bo, uint32_t *fb_id)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_ptr drmmode = drmmode_crtc->drmmode;

    dixDestroyPixmap(pixmap, 0);

    if (data) {
        drmModeRmFB(drmmode->fd, *fb_id);
        *fb_id = 0;

        drmmode_bo_destroy(drmmode, bo);
        memset(bo, 0, sizeof(*bo));
    }
}

static void
drmmode_shadow_destroy(xf86CrtcPtr crtc, PixmapPtr pixmap, void *data)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

    drmmode_shadow_fb_destroy(crtc, pixmap, data, &drmmode_crtc->rotate_bo,
                              &drmmode_crtc->rotate_fb_id);
}

static void
drmmode_crtc_destroy(xf86CrtcPtr crtc)
{
    drmmode_mode_ptr iterator, next;
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    modesettingPtr ms = modesettingPTR(crtc->scrn);

    /* Used even without atomic modesetting */
    free(drmmode_crtc->cursor.dimensions);
    free(drmmode_crtc->cursor_pitches);

    if (!ms->atomic_modeset)
        return;

    drmmode_prop_info_free(drmmode_crtc->props_plane, DRMMODE_PLANE__COUNT);
    xorg_list_for_each_entry_safe(iterator, next, &drmmode_crtc->mode_list, entry) {
        drm_mode_destroy(crtc, iterator);
    }
}

static const xf86CrtcFuncsRec drmmode_crtc_funcs = {
    .dpms = drmmode_crtc_dpms,
    .set_mode_major = drmmode_set_mode_major,
    .set_cursor_colors = drmmode_set_cursor_colors,
    .set_cursor_position = drmmode_set_cursor_position,
    .show_cursor_check = drmmode_show_cursor,
    .hide_cursor = drmmode_hide_cursor,
    .load_cursor_argb_check = drmmode_load_cursor_argb_check,

    .gamma_set = drmmode_crtc_gamma_set,
    .destroy = drmmode_crtc_destroy,
    .set_scanout_pixmap = drmmode_set_scanout_pixmap,
    .shadow_allocate = drmmode_shadow_allocate,
    .shadow_create = drmmode_shadow_create,
    .shadow_destroy = drmmode_shadow_destroy,
};

static uint32_t
drmmode_crtc_vblank_pipe(int crtc_id)
{
    if (crtc_id > 1)
        return crtc_id << DRM_VBLANK_HIGH_CRTC_SHIFT;
    else if (crtc_id > 0)
        return DRM_VBLANK_SECONDARY;
    else
        return 0;
}

static Bool
is_plane_assigned(ScrnInfoPtr scrn, int plane_id)
{
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    int c;

    for (c = 0; c < xf86_config->num_crtc; c++) {
        xf86CrtcPtr iter = xf86_config->crtc[c];
        drmmode_crtc_private_ptr drmmode_crtc = iter->driver_private;
        if (drmmode_crtc->plane_id == plane_id)
            return TRUE;
    }

    return FALSE;
}

/**
 * Populates the formats array, and the modifiers of each format for a drm_plane.
 */
static Bool
populate_format_modifiers(xf86CrtcPtr crtc, const drmModePlane *kplane,
                          drmmode_format_rec *formats, uint32_t blob_id)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_ptr drmmode = drmmode_crtc->drmmode;
    unsigned i, j;
    drmModePropertyBlobRes *blob;
    struct drm_format_modifier_blob *fmt_mod_blob;
    uint32_t *blob_formats;
    struct drm_format_modifier *blob_modifiers;

    if (!blob_id)
        return FALSE;

    blob = drmModeGetPropertyBlob(drmmode->fd, blob_id);
    if (!blob)
        return FALSE;

    fmt_mod_blob = blob->data;
    blob_formats = formats_ptr(fmt_mod_blob);
    blob_modifiers = modifiers_ptr(fmt_mod_blob);

    assert(drmmode_crtc->num_formats == fmt_mod_blob->count_formats);

    for (i = 0; i < fmt_mod_blob->count_formats; i++) {
        uint32_t num_modifiers = 0;
        uint64_t *modifiers = NULL;
        uint64_t *tmp;
        for (j = 0; j < fmt_mod_blob->count_modifiers; j++) {
            struct drm_format_modifier *mod = &blob_modifiers[j];

            if ((i < mod->offset) || (i > mod->offset + 63))
                continue;

            if (!(mod->formats & (1 << (i - mod->offset))))
                continue;

            if (mod->modifier == DRM_FORMAT_MOD_INVALID)
                continue;

            num_modifiers++;
            tmp = realloc(modifiers, num_modifiers * sizeof(modifiers[0]));
            if (!tmp) {
                free(modifiers);
                drmModeFreePropertyBlob(blob);
                return FALSE;
            }
            modifiers = tmp;
            modifiers[num_modifiers - 1] = mod->modifier;
        }

        formats[i].format = blob_formats[i];
        formats[i].modifiers = modifiers;
        formats[i].num_modifiers = num_modifiers;
    }

    drmModeFreePropertyBlob(blob);

    return TRUE;
}

#ifdef LIBDRM_HAS_PLANE_SIZE_HINTS
static void
drmmode_populate_cursor_size_hints(drmmode_ptr drmmode, drmmode_crtc_private_ptr drmmode_crtc, int size_hints_blob)
{
    drmModePropertyBlobRes *blob;

    if (!drmmode_crtc)
        return;

    if (drmmode_crtc->cursor_probed)
        return;

    if (!size_hints_blob)
        return;

    blob = drmModeGetPropertyBlob(drmmode->fd, size_hints_blob);

    if (!blob)
        return;

    if (!blob->length)
        goto fail;

    const struct drm_plane_size_hint *size_hints = blob->data;
    size_t size_hints_len = blob->length / sizeof(size_hints[0]);

    if (!size_hints_len)
        goto fail;

    void *tmp = realloc(drmmode_crtc->cursor.dimensions, size_hints_len * sizeof(drmmode_cursor_dim_rec));
    if (!tmp)
        goto fail;

    drmmode_crtc->cursor.dimensions = tmp;
    drmmode_crtc->cursor.num_dimensions = size_hints_len;

    for (int idx = 0; idx < size_hints_len; idx++)
    {
        struct drm_plane_size_hint size_hint = size_hints[idx];

        drmmode_crtc->cursor.dimensions[idx].width = size_hint.width;
        drmmode_crtc->cursor.dimensions[idx].height = size_hint.height;
    }

    drmmode_crtc->cursor_probed = TRUE;
fail:
    drmModeFreePropertyBlob(blob);
}
#endif

static void
drmmode_crtc_create_planes(xf86CrtcPtr crtc, int num)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_ptr drmmode = drmmode_crtc->drmmode;
    drmModePlaneRes *kplane_res;
    drmModePlane *kplane, *best_kplane = NULL;
    drmModeObjectProperties *props;
    uint32_t blob_id, async_blob_id;
    int best_plane = 0;

    static drmmode_prop_enum_info_rec plane_type_enums[] = {
        [DRMMODE_PLANE_TYPE_PRIMARY] = {
            .name = "Primary",
        },
        [DRMMODE_PLANE_TYPE_OVERLAY] = {
            .name = "Overlay",
        },
        [DRMMODE_PLANE_TYPE_CURSOR] = {
            .name = "Cursor",
        },
    };
    static const drmmode_prop_info_rec plane_props[] = {
        [DRMMODE_PLANE_TYPE] = {
            .name = "type",
            .enum_values = plane_type_enums,
            .num_enum_values = DRMMODE_PLANE_TYPE__COUNT,
        },
        [DRMMODE_PLANE_FB_ID] = { .name = "FB_ID", },
        [DRMMODE_PLANE_CRTC_ID] = { .name = "CRTC_ID", },
        [DRMMODE_PLANE_IN_FORMATS] = { .name = "IN_FORMATS", },
        [DRMMODE_PLANE_IN_FORMATS_ASYNC] = { .name = "IN_FORMATS_ASYNC", },
        [DRMMODE_PLANE_SRC_X] = { .name = "SRC_X", },
        [DRMMODE_PLANE_SRC_Y] = { .name = "SRC_Y", },
        [DRMMODE_PLANE_SRC_W] = { .name = "SRC_W", },
        [DRMMODE_PLANE_SRC_H] = { .name = "SRC_H", },
        [DRMMODE_PLANE_CRTC_X] = { .name = "CRTC_X", },
        [DRMMODE_PLANE_CRTC_Y] = { .name = "CRTC_Y", },
        [DRMMODE_PLANE_CRTC_W] = { .name = "CRTC_W", },
        [DRMMODE_PLANE_CRTC_H] = { .name = "CRTC_H", },
        [DRMMODE_PLANE_SIZE_HINTS] = { .name = "SIZE_HINTS" }
    };
    drmmode_prop_info_rec tmp_props[DRMMODE_PLANE__COUNT];

    if (!drmmode_prop_info_copy(tmp_props, plane_props, DRMMODE_PLANE__COUNT, 0)) {
        xf86DrvMsg(drmmode->scrn->scrnIndex, X_ERROR,
                   "failed to copy plane property info\n");
        drmmode_prop_info_free(tmp_props, DRMMODE_PLANE__COUNT);
        return;
    }

    drmSetClientCap(drmmode->fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
    kplane_res = drmModeGetPlaneResources(drmmode->fd);
    if (!kplane_res) {
        xf86DrvMsg(drmmode->scrn->scrnIndex, X_ERROR,
                   "failed to get plane resources: %s\n", strerror(errno));
        drmmode_prop_info_free(tmp_props, DRMMODE_PLANE__COUNT);
        return;
    }

    for (int i = 0; i < kplane_res->count_planes; i++) {
        int plane_id;

        kplane = drmModeGetPlane(drmmode->fd, kplane_res->planes[i]);
        if (!kplane)
            continue;

        /* If this plane cannot be used on the current crtc, skip it */
        if (!(kplane->possible_crtcs & (1 << num)) ||
            is_plane_assigned(drmmode->scrn, kplane->plane_id)) {
            drmModeFreePlane(kplane);
            continue;
        }

        plane_id = kplane->plane_id;

        props = drmModeObjectGetProperties(drmmode->fd, plane_id,
                                           DRM_MODE_OBJECT_PLANE);
        if (!props) {
            xf86DrvMsg(drmmode->scrn->scrnIndex, X_ERROR,
                    "couldn't get plane properties\n");
            drmModeFreePlane(kplane);
            continue;
        }

        drmmode_prop_info_update(drmmode, tmp_props, DRMMODE_PLANE__COUNT, props);

        int plane_crtc = drmmode_prop_get_value(&tmp_props[DRMMODE_PLANE_CRTC_ID],
                                                props, 0);

        uint32_t type = drmmode_prop_get_value(&tmp_props[DRMMODE_PLANE_TYPE],
                                               props, DRMMODE_PLANE_TYPE__COUNT);

        switch (type) {
        case DRMMODE_PLANE_TYPE_CURSOR:
        {
            /* For some reason, cursor planes may not have prop_crtc_id set, so we don't check it */
#ifdef LIBDRM_HAS_PLANE_SIZE_HINTS
            /* Get the SIZE_HINT dimensions, if supported. */
            int size_hint = drmmode_prop_get_value(&tmp_props[DRMMODE_PLANE_SIZE_HINTS], props, 0);
            drmmode_populate_cursor_size_hints(drmmode, drmmode_crtc, size_hint);
            drmModeFreePlane(kplane);
            drmModeFreeObjectProperties(props);
#endif
            continue;
        }
        case DRMMODE_PLANE_TYPE_PRIMARY:
        {
            /* If this plane is not on this crtc, skip it */
            if (plane_crtc != drmmode_crtc->mode_crtc->crtc_id) {
                drmModeFreePlane(kplane);
                drmModeFreeObjectProperties(props);
                continue;
            }

            /* Only primary planes are important for atomic page-flipping */
            if (best_plane) { /* Can we have more that one primary plane on a crtc? */
                drmModeFreePlane(best_kplane);
                drmmode_prop_info_free(drmmode_crtc->props_plane, DRMMODE_PLANE__COUNT);
            }
            best_plane = plane_id;
            best_kplane = kplane;
            blob_id = drmmode_prop_get_value(&tmp_props[DRMMODE_PLANE_IN_FORMATS], props, 0);
            async_blob_id = drmmode_prop_get_value(&tmp_props[DRMMODE_PLANE_IN_FORMATS_ASYNC], props, 0);
            drmmode_prop_info_copy(drmmode_crtc->props_plane, tmp_props,
                                   DRMMODE_PLANE__COUNT, 1);
            drmModeFreeObjectProperties(props);
            continue;
        }
        case DRMMODE_PLANE_TYPE_OVERLAY:
        {
            drmModeFreePlane(kplane);
            drmModeFreeObjectProperties(props);
            continue;
        }
        default:
        {
            xf86DrvMsg(drmmode->scrn->scrnIndex, X_WARNING, "Plane with id: %d has unknown plane type: %d\n", plane_id, type);
            drmModeFreePlane(kplane);
            drmModeFreeObjectProperties(props);
            continue;
        }
        }
    }

    drmmode_crtc->plane_id = best_plane;
    if (best_kplane) {
        drmmode_crtc->num_formats = best_kplane->count_formats;
        drmmode_crtc->formats = calloc(best_kplane->count_formats,
                                       sizeof(drmmode_format_rec));
        if (!populate_format_modifiers(crtc, best_kplane,
                                       drmmode_crtc->formats, blob_id)) {
            for (int i = 0; i < best_kplane->count_formats; i++)
                drmmode_crtc->formats[i].format = best_kplane->formats[i];
        } else {
            drmmode_crtc->formats_async = calloc(best_kplane->count_formats,
                                                 sizeof(drmmode_format_rec));
            if (!populate_format_modifiers(crtc, best_kplane,
                                           drmmode_crtc->formats_async, async_blob_id)) {
                free(drmmode_crtc->formats_async);
                drmmode_crtc->formats_async = NULL;
            }
        }
        drmModeFreePlane(best_kplane);
    }

    drmmode_prop_info_free(tmp_props, DRMMODE_PLANE__COUNT);
    drmModeFreePlaneResources(kplane_res);
}

static uint32_t
drmmode_crtc_get_prop_id(uint32_t drm_fd,
                         drmModeObjectPropertiesPtr props,
                         char const* name)
{
    uint32_t i, prop_id = 0;

    for (i = 0; !prop_id && i < props->count_props; ++i) {
        drmModePropertyPtr drm_prop =
                     drmModeGetProperty(drm_fd, props->props[i]);

        if (!drm_prop)
            continue;

        if (strcmp(drm_prop->name, name) == 0)
            prop_id = drm_prop->prop_id;

        drmModeFreeProperty(drm_prop);
    }

    return prop_id;
}

static void
drmmode_crtc_vrr_init(int drm_fd, xf86CrtcPtr crtc)
{
    drmModeObjectPropertiesPtr drm_props;
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_ptr drmmode = drmmode_crtc->drmmode;

    if (drmmode->vrr_prop_id)
        return;

    drm_props = drmModeObjectGetProperties(drm_fd,
                                           drmmode_crtc->mode_crtc->crtc_id,
                                           DRM_MODE_OBJECT_CRTC);

    if (!drm_props)
        return;

    drmmode->vrr_prop_id = drmmode_crtc_get_prop_id(drm_fd,
                                                    drm_props,
                                                    "VRR_ENABLED");

    drmModeFreeObjectProperties(drm_props);
}

static inline drmmode_cursor_dim_rec
drmmode_get_kms_default(drmmode_ptr drmmode)
{
    uint64_t value = 0;
    drmmode_cursor_dim_rec fallback;

    /* We begin by using the largest supported cursor, and change it later,
       when we can reliably probe for the smallest suppored cursor size */
    int ret1 = drmGetCap(drmmode->fd, DRM_CAP_CURSOR_WIDTH, &value);
    fallback.width = value;

    int ret2 = drmGetCap(drmmode->fd, DRM_CAP_CURSOR_HEIGHT, &value);
    fallback.height = value;

    /* 64x64 is the safest fallback value to use when we can't probe in any other way,
     * as it is the default value that KMS uses.  */
    if (ret1 || ret2) {
        fallback.width  = 64;
        fallback.height = 64;
    }

    return fallback;
}

static drmmode_cursor_dim_rec
drmmode_cursor_get_fallback(drmmode_crtc_private_ptr drmmode_crtc)
{
    drmmode_ptr drmmode = drmmode_crtc->drmmode;
    drmmode_cursor_dim_rec fallback;

    const char *cursor_size_str = xf86GetOptValString(drmmode->Options,
                                                      OPTION_CURSOR_SIZE);

    char *height;

    if (!cursor_size_str) {
        return drmmode_get_kms_default(drmmode);
    }

    errno = 0;
    fallback.width = strtol(cursor_size_str, &height, 10);
    if (errno || fallback.width == 0) {
        return drmmode_get_kms_default(drmmode);
    }

    if (*height == '\0') {
        /* we have a width, but don't have a height */
        fallback.height = fallback.width;
        drmmode_crtc->cursor_probed = TRUE;
        return fallback;
    }

    fallback.height = strtol(height + 1, NULL, 10);
    if (errno || fallback.height == 0) {
        return drmmode_get_kms_default(drmmode);
    }

    drmmode_crtc->cursor_probed = TRUE;
    return fallback;
}

static unsigned int
drmmode_crtc_init(ScrnInfoPtr pScrn, drmmode_ptr drmmode, drmModeResPtr mode_res, int num)
{
    xf86CrtcPtr crtc;
    drmmode_crtc_private_ptr drmmode_crtc;
    modesettingEntPtr ms_ent = ms_ent_priv(pScrn);
    drmModeObjectPropertiesPtr props;
    static const drmmode_prop_info_rec crtc_props[] = {
        [DRMMODE_CRTC_ACTIVE] = { .name = "ACTIVE" },
        [DRMMODE_CRTC_MODE_ID] = { .name = "MODE_ID" },
        [DRMMODE_CRTC_GAMMA_LUT] = { .name = "GAMMA_LUT" },
        [DRMMODE_CRTC_GAMMA_LUT_SIZE] = { .name = "GAMMA_LUT_SIZE" },
        [DRMMODE_CRTC_CTM] = { .name = "CTM" },
    };

    crtc = xf86CrtcCreate(pScrn, &drmmode_crtc_funcs);
    if (crtc == NULL)
        return 0;
    drmmode_crtc = XNFcallocarray(1, sizeof(drmmode_crtc_private_rec));
    crtc->driver_private = drmmode_crtc;
    drmmode_crtc->mode_crtc =
        drmModeGetCrtc(drmmode->fd, mode_res->crtcs[num]);
    drmmode_crtc->drmmode = drmmode;
    drmmode_crtc->vblank_pipe = drmmode_crtc_vblank_pipe(num);
    xorg_list_init(&drmmode_crtc->mode_list);
    xorg_list_init(&drmmode_crtc->tearfree.dri_flip_list);
    drmmode_crtc->next_msc = UINT64_MAX;

    /* Setup the fallback cursor immediately. */
    drmmode_crtc->cursor.dimensions = malloc(sizeof(drmmode_cursor_dim_rec));
    if (drmmode_crtc->cursor.dimensions == NULL)
        return 0;

    drmmode_crtc->cursor.num_dimensions = 1;

    drmmode_crtc->cursor.dimensions[0] = drmmode_cursor_get_fallback(drmmode_crtc);

    props = drmModeObjectGetProperties(drmmode->fd, mode_res->crtcs[num],
                                       DRM_MODE_OBJECT_CRTC);
    if (!props || !drmmode_prop_info_copy(drmmode_crtc->props, crtc_props,
                                          DRMMODE_CRTC__COUNT, 0)) {
        xf86CrtcDestroy(crtc);
        return 0;
    }

    drmmode_prop_info_update(drmmode, drmmode_crtc->props,
                             DRMMODE_CRTC__COUNT, props);
    drmModeFreeObjectProperties(props);
    drmmode_crtc_create_planes(crtc, num);

    /* Hide any cursors which may be active from previous users */
    drmModeSetCursor(drmmode->fd, drmmode_crtc->mode_crtc->crtc_id, 0, 0, 0);

    drmmode_crtc_vrr_init(drmmode->fd, crtc);

    /* Mark num'th crtc as in use on this device. */
    ms_ent->assigned_crtcs |= (1 << num);
    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, MS_LOGLEVEL_DEBUG,
                   "Allocated crtc nr. %d to this screen.\n", num);

    if (drmmode_crtc->props[DRMMODE_CRTC_GAMMA_LUT_SIZE].prop_id &&
        drmmode_crtc->props[DRMMODE_CRTC_GAMMA_LUT_SIZE].value) {
        /*
         * GAMMA_LUT property supported, and so far tested to be safe to use by
         * default for lut sizes up to 4096 slots. Intel Tigerlake+ has some
         * issues, and a large GAMMA_LUT with 262145 slots, so keep GAMMA_LUT
         * off for large lut sizes by default for now.
         */
        drmmode_crtc->use_gamma_lut = drmmode_crtc->props[DRMMODE_CRTC_GAMMA_LUT_SIZE].value <= 4096;

        /* Allow config override. */
        drmmode_crtc->use_gamma_lut = xf86ReturnOptValBool(drmmode->Options,
                                                           OPTION_USE_GAMMA_LUT,
                                                           drmmode_crtc->use_gamma_lut);
    } else {
        drmmode_crtc->use_gamma_lut = FALSE;
    }

    if (drmmode_crtc->use_gamma_lut &&
        drmmode_crtc->props[DRMMODE_CRTC_CTM].prop_id) {
        drmmode->use_ctm = TRUE;
    }

    return 1;
}

/*
 * Update all of the property values for an output
 */
static void
drmmode_output_update_properties(xf86OutputPtr output)
{
    drmmode_output_private_ptr drmmode_output = output->driver_private;
    int i, j, k;
    int err;
    drmModeConnectorPtr koutput;

    /* Use the most recently fetched values from the kernel */
    koutput = drmmode_output->mode_output;

    if (!koutput)
        return;

    for (i = 0; i < drmmode_output->num_props; i++) {
        drmmode_prop_ptr p = &drmmode_output->props[i];

        for (j = 0; koutput && j < koutput->count_props; j++) {
            if (koutput->props[j] == p->mode_prop->prop_id) {

                /* Check to see if the property value has changed */
                if (koutput->prop_values[j] != p->value) {

                    p->value = koutput->prop_values[j];

                    if (p->mode_prop->flags & DRM_MODE_PROP_RANGE) {
                        INT32 value = p->value;

                        err = RRChangeOutputProperty(output->randr_output, p->atoms[0],
                                                     XA_INTEGER, 32, PropModeReplace, 1,
                                                     &value, FALSE, TRUE);

                        if (err != 0) {
                            xf86DrvMsg(output->scrn->scrnIndex, X_ERROR,
                                       "RRChangeOutputProperty error, %d\n", err);
                        }
                    }
                    else if (p->mode_prop->flags & DRM_MODE_PROP_ENUM) {
                        for (k = 0; k < p->mode_prop->count_enums; k++)
                            if (p->mode_prop->enums[k].value == p->value)
                                break;
                        if (k < p->mode_prop->count_enums) {
                            err = RRChangeOutputProperty(output->randr_output, p->atoms[0],
                                                         XA_ATOM, 32, PropModeReplace, 1,
                                                         &p->atoms[k + 1], FALSE, TRUE);
                            if (err != 0) {
                                xf86DrvMsg(output->scrn->scrnIndex, X_ERROR,
                                           "RRChangeOutputProperty error, %d\n", err);
                            }
                        }
                    }
                }
                break;
            }
        }
    }

    /* Update the CTM property */
    if (drmmode_output->ctm_atom) {
        err = RRChangeOutputProperty(output->randr_output,
                                     drmmode_output->ctm_atom,
                                     XA_INTEGER, 32, PropModeReplace, 18,
                                     &drmmode_output->ctm,
                                     FALSE, TRUE);
        if (err != 0) {
            xf86DrvMsg(output->scrn->scrnIndex, X_ERROR,
                       "RRChangeOutputProperty error, %d\n", err);
        }
    }

}

static xf86OutputStatus
drmmode_output_detect(xf86OutputPtr output)
{
    /* go to the hw and retrieve a new output struct */
    drmmode_output_private_ptr drmmode_output = output->driver_private;
    drmmode_ptr drmmode = drmmode_output->drmmode;
    xf86OutputStatus status;

    if (drmmode_output->output_id == -1)
        return XF86OutputStatusDisconnected;

    drmModeFreeConnector(drmmode_output->mode_output);

    drmmode_output->mode_output =
        drmModeGetConnector(drmmode->fd, drmmode_output->output_id);

    if (!drmmode_output->mode_output) {
        drmmode_output->output_id = -1;
        return XF86OutputStatusDisconnected;
    }

    drmmode_output_update_properties(output);

    switch (drmmode_output->mode_output->connection) {
    case DRM_MODE_CONNECTED:
        status = XF86OutputStatusConnected;
        break;
    case DRM_MODE_DISCONNECTED:
        status = XF86OutputStatusDisconnected;
        break;
    default:
    case DRM_MODE_UNKNOWNCONNECTION:
        status = XF86OutputStatusUnknown;
        break;
    }
    return status;
}

static Bool
drmmode_output_mode_valid(xf86OutputPtr output, DisplayModePtr pModes)
{
    return MODE_OK;
}

static int
koutput_get_prop_idx(int fd, drmModeConnectorPtr koutput,
        int type, const char *name)
{
    int idx = -1;

    for (int i = 0; i < koutput->count_props; i++) {
        drmModePropertyPtr prop = drmModeGetProperty(fd, koutput->props[i]);

        if (!prop)
            continue;

        if (drm_property_type_is(prop, type) && !strcmp(prop->name, name))
            idx = i;

        drmModeFreeProperty(prop);

        if (idx > -1)
            break;
    }

    return idx;
}

static int
koutput_get_prop_id(int fd, drmModeConnectorPtr koutput,
        int type, const char *name)
{
    int idx = koutput_get_prop_idx(fd, koutput, type, name);

    return (idx > -1) ? koutput->props[idx] : -1;
}

static drmModePropertyBlobPtr
koutput_get_prop_blob(int fd, drmModeConnectorPtr koutput, const char *name)
{
    drmModePropertyBlobPtr blob = NULL;
    int idx = koutput_get_prop_idx(fd, koutput, DRM_MODE_PROP_BLOB, name);

    if (idx > -1)
        blob = drmModeGetPropertyBlob(fd, koutput->prop_values[idx]);

    return blob;
}

static void
drmmode_output_attach_tile(xf86OutputPtr output)
{
    drmmode_output_private_ptr drmmode_output = output->driver_private;
    drmModeConnectorPtr koutput = drmmode_output->mode_output;
    drmmode_ptr drmmode = drmmode_output->drmmode;
    struct xf86CrtcTileInfo tile_info, *set = NULL;

    if (!koutput) {
        xf86OutputSetTile(output, NULL);
        return;
    }

    drmModeFreePropertyBlob(drmmode_output->tile_blob);

    /* look for a TILE property */
    drmmode_output->tile_blob =
        koutput_get_prop_blob(drmmode->fd, koutput, "TILE");

    if (drmmode_output->tile_blob) {
        if (xf86OutputParseKMSTile(drmmode_output->tile_blob->data, drmmode_output->tile_blob->length, &tile_info) == TRUE)
            set = &tile_info;
    }
    xf86OutputSetTile(output, set);
}

static Bool
has_panel_fitter(xf86OutputPtr output)
{
    drmmode_output_private_ptr drmmode_output = output->driver_private;
    drmModeConnectorPtr koutput = drmmode_output->mode_output;
    drmmode_ptr drmmode = drmmode_output->drmmode;
    int idx;

    /* Presume that if the output supports scaling, then we have a
     * panel fitter capable of adjust any mode to suit.
     */
    idx = koutput_get_prop_idx(drmmode->fd, koutput,
            DRM_MODE_PROP_ENUM, "scaling mode");

    return (idx > -1);
}

static DisplayModePtr
drmmode_output_add_gtf_modes(xf86OutputPtr output, DisplayModePtr Modes)
{
    xf86MonPtr mon = output->MonInfo;
    DisplayModePtr i, j, m, preferred = NULL;
    int max_x = 0, max_y = 0;
    float max_vrefresh = 0.0;

    if (mon && gtf_supported(mon))
        return Modes;

    if (!has_panel_fitter(output))
        return Modes;

    for (m = Modes; m; m = m->next) {
        if (m->type & M_T_PREFERRED)
            preferred = m;
        max_x = MAX(max_x, m->HDisplay);
        max_y = MAX(max_y, m->VDisplay);
        max_vrefresh = MAX(max_vrefresh, xf86ModeVRefresh(m));
    }

    max_vrefresh = MAX(max_vrefresh, 60.0);
    max_vrefresh *= (1 + SYNC_TOLERANCE);

    m = xf86GetDefaultModes();
    xf86ValidateModesSize(output->scrn, m, max_x, max_y, 0);

    for (i = m; i; i = i->next) {
        if (xf86ModeVRefresh(i) > max_vrefresh)
            i->status = MODE_VSYNC;
        if (preferred &&
            i->HDisplay >= preferred->HDisplay &&
            i->VDisplay >= preferred->VDisplay &&
            xf86ModeVRefresh(i) >= xf86ModeVRefresh(preferred))
            i->status = MODE_VSYNC;
        if (preferred && xf86ModeVRefresh(i) > 0.0) {
            i->Clock = i->Clock * xf86ModeVRefresh(preferred) / xf86ModeVRefresh(i);
            i->VRefresh = xf86ModeVRefresh(preferred);
        }
        for (j = m; j != i; j = j->next) {
            if (!strcmp(i->name, j->name) &&
                xf86ModeVRefresh(i) * (1 + SYNC_TOLERANCE) >= xf86ModeVRefresh(j) &&
                xf86ModeVRefresh(i) * (1 - SYNC_TOLERANCE) <= xf86ModeVRefresh(j)) {
                i->status = MODE_DUPLICATE;
            }
        }
    }

    xf86PruneInvalidModes(output->scrn, &m, FALSE);

    return xf86ModesAdd(Modes, m);
}

static DisplayModePtr
drmmode_output_get_modes(xf86OutputPtr output)
{
    drmmode_output_private_ptr drmmode_output = output->driver_private;
    drmModeConnectorPtr koutput = drmmode_output->mode_output;
    drmmode_ptr drmmode = drmmode_output->drmmode;
    int i;
    DisplayModePtr Modes = NULL, Mode;
    xf86MonPtr mon = NULL;

    if (!koutput)
        return NULL;

    drmModeFreePropertyBlob(drmmode_output->edid_blob);

    /* look for an EDID property */
    drmmode_output->edid_blob =
        koutput_get_prop_blob(drmmode->fd, koutput, "EDID");

    if (drmmode_output->edid_blob) {
        mon = xf86InterpretEDID(output->scrn->scrnIndex,
                                drmmode_output->edid_blob->data);
        if (mon && drmmode_output->edid_blob->length > 128)
            mon->flags |= MONITOR_EDID_COMPLETE_RAWDATA;
    }
    xf86OutputSetEDID(output, mon);

    drmmode_output_attach_tile(output);

    /* modes should already be available */
    for (i = 0; i < koutput->count_modes; i++) {
        Mode = XNFalloc(sizeof(DisplayModeRec));

        drmmode_ConvertFromKMode(output->scrn, &koutput->modes[i], Mode);
        Modes = xf86ModesAdd(Modes, Mode);

    }

    return drmmode_output_add_gtf_modes(output, Modes);
}

static void
drmmode_output_destroy(xf86OutputPtr output)
{
    drmmode_output_private_ptr drmmode_output = output->driver_private;
    int i;

    drmModeFreePropertyBlob(drmmode_output->edid_blob);
    drmModeFreePropertyBlob(drmmode_output->tile_blob);

    for (i = 0; i < drmmode_output->num_props; i++) {
        drmModeFreeProperty(drmmode_output->props[i].mode_prop);
        free(drmmode_output->props[i].atoms);
    }
    free(drmmode_output->props);
    if (drmmode_output->mode_output) {
        for (i = 0; i < drmmode_output->mode_output->count_encoders; i++) {
            drmModeFreeEncoder(drmmode_output->mode_encoders[i]);
        }
        drmModeFreeConnector(drmmode_output->mode_output);
    }
    free(drmmode_output->mode_encoders);
    free(drmmode_output);
    output->driver_private = NULL;
}

static void
drmmode_output_dpms(xf86OutputPtr output, int mode)
{
    modesettingPtr ms = modesettingPTR(output->scrn);
    drmmode_output_private_ptr drmmode_output = output->driver_private;
    drmmode_ptr drmmode = drmmode_output->drmmode;
    xf86CrtcPtr crtc = output->crtc;
    drmModeConnectorPtr koutput = drmmode_output->mode_output;

    if (!koutput)
        return;

    /* XXX Check if DPMS mode is already the right one */

    drmmode_output->dpms = mode;

    if (ms->atomic_modeset) {
        if (mode != DPMSModeOn && !ms->pending_modeset)
            drmmode_output_disable(output);
    } else {
        drmModeConnectorSetProperty(drmmode->fd, koutput->connector_id,
                                    drmmode_output->dpms_enum_id, mode);
    }

    if (crtc) {
        drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

        if (mode == DPMSModeOn) {
            if (drmmode_crtc->need_modeset)
                drmmode_set_mode_major(crtc, &crtc->mode, crtc->rotation,
                                       crtc->x, crtc->y);

            if (drmmode_crtc->enable_flipping)
                drmmode_InitSharedPixmapFlipping(crtc, drmmode_crtc->drmmode);
        } else {
            if (drmmode_crtc->enable_flipping)
                drmmode_FiniSharedPixmapFlipping(crtc, drmmode_crtc->drmmode);
        }
    }

    return;
}

static Bool
drmmode_property_ignore(drmModePropertyPtr prop)
{
    if (!prop)
        return TRUE;
    /* ignore blob prop */
    if (prop->flags & DRM_MODE_PROP_BLOB)
        return TRUE;
    /* ignore standard property */
    if (!strcmp(prop->name, "EDID") || !strcmp(prop->name, "DPMS") ||
        !strcmp(prop->name, "CRTC_ID"))
        return TRUE;

    return FALSE;
}

static void
drmmode_output_create_resources(xf86OutputPtr output)
{
    drmmode_output_private_ptr drmmode_output = output->driver_private;
    drmModeConnectorPtr mode_output = drmmode_output->mode_output;
    drmmode_ptr drmmode = drmmode_output->drmmode;
    drmModePropertyPtr drmmode_prop;
    int i, j, err;

    drmmode_output->props =
        calloc(mode_output->count_props, sizeof(drmmode_prop_rec));
    if (!drmmode_output->props)
        return;

    drmmode_output->num_props = 0;
    for (i = 0, j = 0; i < mode_output->count_props; i++) {
        drmmode_prop = drmModeGetProperty(drmmode->fd, mode_output->props[i]);
        if (drmmode_property_ignore(drmmode_prop)) {
            drmModeFreeProperty(drmmode_prop);
            continue;
        }
        drmmode_output->props[j].mode_prop = drmmode_prop;
        drmmode_output->props[j].value = mode_output->prop_values[i];
        drmmode_output->num_props++;
        j++;
    }

    /* Create CONNECTOR_ID property */
    {
        Atom    name = dixAddAtom("CONNECTOR_ID");
        INT32   value = mode_output->connector_id;

        if (name != BAD_RESOURCE) {
            err = RRConfigureOutputProperty(output->randr_output, name,
                                            FALSE, FALSE, TRUE,
                                            1, &value);
            if (err != 0) {
                xf86DrvMsg(output->scrn->scrnIndex, X_ERROR,
                           "RRConfigureOutputProperty error, %d\n", err);
            }
            err = RRChangeOutputProperty(output->randr_output, name,
                                         XA_INTEGER, 32, PropModeReplace, 1,
                                         &value, FALSE, FALSE);
            if (err != 0) {
                xf86DrvMsg(output->scrn->scrnIndex, X_ERROR,
                           "RRChangeOutputProperty error, %d\n", err);
            }
        }
    }

    if (drmmode->use_ctm) {
        Atom name = dixAddAtom("CTM");

        if (name != BAD_RESOURCE) {
            drmmode_output->ctm_atom = name;

            err = RRConfigureOutputProperty(output->randr_output, name,
                                            FALSE, FALSE, TRUE, 0, NULL);
            if (err != 0) {
                xf86DrvMsg(output->scrn->scrnIndex, X_ERROR,
                           "RRConfigureOutputProperty error, %d\n", err);
            }

            err = RRChangeOutputProperty(output->randr_output, name,
                                         XA_INTEGER, 32, PropModeReplace, 18,
                                         &ctm_identity, FALSE, FALSE);
            if (err != 0) {
                xf86DrvMsg(output->scrn->scrnIndex, X_ERROR,
                           "RRChangeOutputProperty error, %d\n", err);
            }

            drmmode_output->ctm = ctm_identity;
        }
    }

    for (i = 0; i < drmmode_output->num_props; i++) {
        drmmode_prop_ptr p = &drmmode_output->props[i];

        drmmode_prop = p->mode_prop;

        if (drmmode_prop->flags & DRM_MODE_PROP_RANGE) {
            INT32 prop_range[2];
            INT32 value = p->value;

            p->num_atoms = 1;
            p->atoms = calloc(p->num_atoms, sizeof(Atom));
            if (!p->atoms)
                continue;
            p->atoms[0] = dixAddAtom(drmmode_prop->name);
            prop_range[0] = drmmode_prop->values[0];
            prop_range[1] = drmmode_prop->values[1];
            err = RRConfigureOutputProperty(output->randr_output, p->atoms[0],
                                            FALSE, TRUE,
                                            drmmode_prop->
                                            flags & DRM_MODE_PROP_IMMUTABLE ?
                                            TRUE : FALSE, 2, prop_range);
            if (err != 0) {
                xf86DrvMsg(output->scrn->scrnIndex, X_ERROR,
                           "RRConfigureOutputProperty error, %d\n", err);
            }
            err = RRChangeOutputProperty(output->randr_output, p->atoms[0],
                                         XA_INTEGER, 32, PropModeReplace, 1,
                                         &value, FALSE, TRUE);
            if (err != 0) {
                xf86DrvMsg(output->scrn->scrnIndex, X_ERROR,
                           "RRChangeOutputProperty error, %d\n", err);
            }
        }
        else if (drmmode_prop->flags & DRM_MODE_PROP_ENUM) {
            p->num_atoms = drmmode_prop->count_enums + 1;
            p->atoms = calloc(p->num_atoms, sizeof(Atom));
            if (!p->atoms)
                continue;
            p->atoms[0] = dixAddAtom(drmmode_prop->name);
            for (j = 1; j <= drmmode_prop->count_enums; j++) {
                struct drm_mode_property_enum *e = &drmmode_prop->enums[j - 1];
                p->atoms[j] = dixAddAtom(e->name);
            }
            err = RRConfigureOutputProperty(output->randr_output, p->atoms[0],
                                            FALSE, FALSE,
                                            drmmode_prop->
                                            flags & DRM_MODE_PROP_IMMUTABLE ?
                                            TRUE : FALSE, p->num_atoms - 1,
                                            (INT32 *) &p->atoms[1]);
            if (err != 0) {
                xf86DrvMsg(output->scrn->scrnIndex, X_ERROR,
                           "RRConfigureOutputProperty error, %d\n", err);
            }
            for (j = 0; j < drmmode_prop->count_enums; j++)
                if (drmmode_prop->enums[j].value == p->value)
                    break;
            /* there's always a matching value */
            err = RRChangeOutputProperty(output->randr_output, p->atoms[0],
                                         XA_ATOM, 32, PropModeReplace, 1,
                                         &p->atoms[j + 1], FALSE, TRUE);
            if (err != 0) {
                xf86DrvMsg(output->scrn->scrnIndex, X_ERROR,
                           "RRChangeOutputProperty error, %d\n", err);
            }
        }
    }
}

static Bool
drmmode_output_set_property(xf86OutputPtr output, Atom property,
                            RRPropertyValuePtr value)
{
    drmmode_output_private_ptr drmmode_output = output->driver_private;
    drmmode_ptr drmmode = drmmode_output->drmmode;
    int i;

    for (i = 0; i < drmmode_output->num_props; i++) {
        drmmode_prop_ptr p = &drmmode_output->props[i];

        if ((!p->atoms) || (p->atoms[0] != property))
            continue;

        if (p->mode_prop->flags & DRM_MODE_PROP_RANGE) {
            uint32_t val;

            if (value->type != XA_INTEGER || value->format != 32 ||
                value->size != 1)
                return FALSE;
            val = *(uint32_t *) value->data;

            drmModeConnectorSetProperty(drmmode->fd, drmmode_output->output_id,
                                        p->mode_prop->prop_id, (uint64_t) val);
            return TRUE;
        }
        else if (p->mode_prop->flags & DRM_MODE_PROP_ENUM) {
            Atom atom;
            const char *name;
            int j;

            if (value->type != XA_ATOM || value->format != 32 ||
                value->size != 1)
                return FALSE;
            memcpy(&atom, value->data, 4);
            if (!(name = NameForAtom(atom)))
                return FALSE;

            /* search for matching name string, then set its value down */
            for (j = 0; j < p->mode_prop->count_enums; j++) {
                if (!strcmp(p->mode_prop->enums[j].name, name)) {
                    drmModeConnectorSetProperty(drmmode->fd,
                                                drmmode_output->output_id,
                                                p->mode_prop->prop_id,
                                                p->mode_prop->enums[j].value);
                    return TRUE;
                }
            }
        }
    }

    if (property == drmmode_output->ctm_atom) {
        const size_t matrix_size = sizeof(drmmode_output->ctm);

        if (value->type != XA_INTEGER || value->format != 32 ||
            value->size * 4 != matrix_size)
            return FALSE;

        memcpy(&drmmode_output->ctm, value->data, matrix_size);

        // Update the CRTC if there is one bound to this output.
        if (output->crtc) {
            drmmode_set_ctm(output->crtc, &drmmode_output->ctm);
        }
    }

    return TRUE;
}

static Bool
drmmode_output_get_property(xf86OutputPtr output, Atom property)
{
    return TRUE;
}

static const xf86OutputFuncsRec drmmode_output_funcs = {
    .dpms = drmmode_output_dpms,
    .create_resources = drmmode_output_create_resources,
    .set_property = drmmode_output_set_property,
    .get_property = drmmode_output_get_property,
    .detect = drmmode_output_detect,
    .mode_valid = drmmode_output_mode_valid,

    .get_modes = drmmode_output_get_modes,
    .destroy = drmmode_output_destroy
};

static int subpixel_conv_table[7] = {
    0,
    SubPixelUnknown,
    SubPixelHorizontalRGB,
    SubPixelHorizontalBGR,
    SubPixelVerticalRGB,
    SubPixelVerticalBGR,
    SubPixelNone
};

static const char *const output_names[] = {
    "None",
    "VGA",
    "DVI-I",
    "DVI-D",
    "DVI-A",
    "Composite",
    "SVIDEO",
    "LVDS",
    "Component",
    "DIN",
    "DP",
    "HDMI",
    "HDMI-B",
    "TV",
    "eDP",
    "Virtual",
    "DSI",
    "DPI",
};

static xf86OutputPtr find_output(ScrnInfoPtr pScrn, int id)
{
    xf86CrtcConfigPtr   xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
    int i;
    for (i = 0; i < xf86_config->num_output; i++) {
        xf86OutputPtr output = xf86_config->output[i];
        drmmode_output_private_ptr drmmode_output;

        drmmode_output = output->driver_private;
        if (drmmode_output->output_id == id)
            return output;
    }
    return NULL;
}

static int parse_path_blob(drmModePropertyBlobPtr path_blob, int *conn_base_id, char **path)
{
    char *conn;
    char conn_id[5];
    int id, len;
    char *blob_data;

    if (!path_blob)
        return -1;

    blob_data = path_blob->data;
    /* we only handle MST paths for now */
    if (strncmp(blob_data, "mst:", 4))
        return -1;

    conn = strchr(blob_data + 4, '-');
    if (!conn)
        return -1;
    len = conn - (blob_data + 4);
    if (len + 1> 5)
        return -1;
    memcpy(conn_id, blob_data + 4, len);
    conn_id[len] = '\0';
    id = strtoul(conn_id, NULL, 10);

    *conn_base_id = id;

    *path = conn + 1;
    return 0;
}

static void
drmmode_create_name(ScrnInfoPtr pScrn, drmModeConnectorPtr koutput, char *name,
                    drmModePropertyBlobPtr path_blob)
{
    int ret;
    char *extra_path;
    int conn_id;
    xf86OutputPtr output;

    ret = parse_path_blob(path_blob, &conn_id, &extra_path);
    if (ret == -1)
        goto fallback;

    output = find_output(pScrn, conn_id);
    if (!output)
        goto fallback;

    snprintf(name, 32, "%s-%s", output->name, extra_path);
    return;

 fallback:
    if (koutput->connector_type >= ARRAY_SIZE(output_names))
        snprintf(name, 32, "Unknown%d-%d", koutput->connector_type, koutput->connector_type_id);
    else if (pScrn->is_gpu)
        snprintf(name, 32, "%s-%d-%d", output_names[koutput->connector_type], pScrn->scrnIndex - GPU_SCREEN_OFFSET + 1, koutput->connector_type_id);
    else
        snprintf(name, 32, "%s-%d", output_names[koutput->connector_type], koutput->connector_type_id);
}

static Bool
drmmode_connector_check_vrr_capable(uint32_t drm_fd, int connector_id)
{
    uint32_t i;
    Bool found = FALSE;
    uint64_t prop_value = 0;
    drmModeObjectPropertiesPtr props;
    const char* prop_name = "VRR_CAPABLE";

    props = drmModeObjectGetProperties(drm_fd, connector_id,
                                    DRM_MODE_OBJECT_CONNECTOR);

    for (i = 0; !found && i < props->count_props; ++i) {
        drmModePropertyPtr drm_prop = drmModeGetProperty(drm_fd, props->props[i]);

        if (!drm_prop)
            continue;

        if (strcasecmp(drm_prop->name, prop_name) == 0) {
            prop_value = props->prop_values[i];
            found = TRUE;
        }

        drmModeFreeProperty(drm_prop);
    }

    drmModeFreeObjectProperties(props);

    if(found)
        return prop_value ? TRUE : FALSE;

    return FALSE;
}

static unsigned int
drmmode_output_init(ScrnInfoPtr pScrn, drmmode_ptr drmmode, drmModeResPtr mode_res, int num, Bool dynamic, int crtcshift)
{
    xf86OutputPtr output;
    xf86CrtcConfigPtr   xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
    modesettingPtr ms = modesettingPTR(pScrn);
    drmModeConnectorPtr koutput;
    drmModeEncoderPtr *kencoders = NULL;
    drmmode_output_private_ptr drmmode_output;
    char name[32];
    int i;
    Bool nonDesktop = FALSE;
    drmModePropertyBlobPtr path_blob = NULL;
    const char *s;
    drmModeObjectPropertiesPtr props;
    static const drmmode_prop_info_rec connector_props[] = {
        [DRMMODE_CONNECTOR_CRTC_ID] = { .name = "CRTC_ID", },
    };

    koutput =
        drmModeGetConnector(drmmode->fd, mode_res->connectors[num]);
    if (!koutput)
        return 0;

    path_blob = koutput_get_prop_blob(drmmode->fd, koutput, "PATH");
    i = koutput_get_prop_idx(drmmode->fd, koutput, DRM_MODE_PROP_RANGE, RR_PROPERTY_NON_DESKTOP);
    if (i >= 0)
        nonDesktop = koutput->prop_values[i] != 0;

    drmmode_create_name(pScrn, koutput, name, path_blob);

    if (path_blob)
        drmModeFreePropertyBlob(path_blob);

    if (path_blob && dynamic) {
        /* see if we have an output with this name already
           and hook stuff up */
        for (i = 0; i < xf86_config->num_output; i++) {
            output = xf86_config->output[i];

            if (strncmp(output->name, name, 32))
                continue;

            drmmode_output = output->driver_private;
            drmmode_output->output_id = mode_res->connectors[num];
            drmmode_output->mode_output = koutput;
            output->non_desktop = nonDesktop;
            return 1;
        }
    }

    kencoders = calloc(koutput->count_encoders, sizeof(drmModeEncoderPtr));
    if (!kencoders) {
        goto out_free_encoders;
    }

    for (i = 0; i < koutput->count_encoders; i++) {
        kencoders[i] = drmModeGetEncoder(drmmode->fd, koutput->encoders[i]);
        if (!kencoders[i]) {
            goto out_free_encoders;
        }
    }

    if (xf86IsEntityShared(pScrn->entityList[0])) {
        if ((s = xf86GetOptValString(drmmode->Options, OPTION_ZAPHOD_HEADS))) {
            if (!drmmode_zaphod_string_matches(pScrn, s, name))
                goto out_free_encoders;
        } else {
            if (!drmmode->is_secondary && (num != 0))
                goto out_free_encoders;
            else if (drmmode->is_secondary && (num != 1))
                goto out_free_encoders;
        }
    }

    output = xf86OutputCreate(pScrn, &drmmode_output_funcs, name);
    if (!output) {
        goto out_free_encoders;
    }

    drmmode_output = calloc(1, sizeof(drmmode_output_private_rec));
    if (!drmmode_output) {
        xf86OutputDestroy(output);
        goto out_free_encoders;
    }

    drmmode_output->output_id = mode_res->connectors[num];
    drmmode_output->mode_output = koutput;
    drmmode_output->mode_encoders = kencoders;
    drmmode_output->drmmode = drmmode;
    output->mm_width = koutput->mmWidth;
    output->mm_height = koutput->mmHeight;

    output->subpixel_order = subpixel_conv_table[koutput->subpixel];
    output->interlaceAllowed = TRUE;
    output->doubleScanAllowed = TRUE;
    output->driver_private = drmmode_output;
    output->non_desktop = nonDesktop;

    output->possible_crtcs = 0;
    for (i = 0; i < koutput->count_encoders; i++) {
        output->possible_crtcs |= (kencoders[i]->possible_crtcs >> crtcshift) & 0x7f;
    }
    /* work out the possible clones later */
    output->possible_clones = 0;

    if (ms->atomic_modeset) {
        if (!drmmode_prop_info_copy(drmmode_output->props_connector,
                                    connector_props, DRMMODE_CONNECTOR__COUNT,
                                    0)) {
            goto out_free_encoders;
        }
        props = drmModeObjectGetProperties(drmmode->fd,
                                           drmmode_output->output_id,
                                           DRM_MODE_OBJECT_CONNECTOR);
        drmmode_prop_info_update(drmmode, drmmode_output->props_connector,
                                 DRMMODE_CONNECTOR__COUNT, props);
    } else {
        drmmode_output->dpms_enum_id =
            koutput_get_prop_id(drmmode->fd, koutput, DRM_MODE_PROP_ENUM,
                                "DPMS");
    }

    if (dynamic) {
        output->randr_output = RROutputCreate(xf86ScrnToScreen(pScrn), output->name, strlen(output->name), output);
        if (output->randr_output) {
            drmmode_output_create_resources(output);
            RRPostPendingProperties(output->randr_output);
        }
    }

    ms->is_connector_vrr_capable |=
              drmmode_connector_check_vrr_capable(drmmode->fd,
                                                  drmmode_output->output_id);
    return 1;

 out_free_encoders:
    if (kencoders) {
        for (i = 0; i < koutput->count_encoders; i++)
            drmModeFreeEncoder(kencoders[i]);
        free(kencoders);
    }
    drmModeFreeConnector(koutput);

    return 0;
}

static uint32_t
find_clones(ScrnInfoPtr scrn, xf86OutputPtr output)
{
    drmmode_output_private_ptr drmmode_output =
        output->driver_private, clone_drmout;
    int i;
    xf86OutputPtr clone_output;
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    int index_mask = 0;

    if (drmmode_output->enc_clone_mask == 0)
        return index_mask;

    for (i = 0; i < xf86_config->num_output; i++) {
        clone_output = xf86_config->output[i];
        clone_drmout = clone_output->driver_private;
        if (output == clone_output)
            continue;

        if (clone_drmout->enc_mask == 0)
            continue;
        if (drmmode_output->enc_clone_mask == clone_drmout->enc_mask)
            index_mask |= (1 << i);
    }
    return index_mask;
}

static void
drmmode_clones_init(ScrnInfoPtr scrn, drmmode_ptr drmmode, drmModeResPtr mode_res)
{
    int i, j;
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);

    for (i = 0; i < xf86_config->num_output; i++) {
        xf86OutputPtr output = xf86_config->output[i];
        drmmode_output_private_ptr drmmode_output;

        drmmode_output = output->driver_private;
        drmmode_output->enc_clone_mask = 0xff;
        /* and all the possible encoder clones for this output together */
        for (j = 0; j < drmmode_output->mode_output->count_encoders; j++) {
            int k;

            for (k = 0; k < mode_res->count_encoders; k++) {
                if (mode_res->encoders[k] ==
                    drmmode_output->mode_encoders[j]->encoder_id)
                    drmmode_output->enc_mask |= (1 << k);
            }

            drmmode_output->enc_clone_mask &=
                drmmode_output->mode_encoders[j]->possible_clones;
        }
    }

    for (i = 0; i < xf86_config->num_output; i++) {
        xf86OutputPtr output = xf86_config->output[i];

        output->possible_clones = find_clones(scrn, output);
    }
}

static Bool
drmmode_set_pixmap_bo(drmmode_ptr drmmode, PixmapPtr pixmap, drmmode_bo *bo)
{
#ifdef GLAMOR_HAS_GBM
    ScrnInfoPtr scrn = drmmode->scrn;
    modesettingPtr ms = modesettingPTR(scrn);

    if (!drmmode->glamor)
        return TRUE;

    if (!ms->glamor.egl_create_textured_pixmap_from_gbm_bo(pixmap, bo->gbm,
                                                           bo->used_modifiers)) {
        xf86DrvMsg(scrn->scrnIndex, X_ERROR, "Failed to create pixmap\n");
        return FALSE;
    }
#endif

    return TRUE;
}

Bool
drmmode_glamor_handle_new_screen_pixmap(drmmode_ptr drmmode)
{
    ScreenPtr screen = xf86ScrnToScreen(drmmode->scrn);
    PixmapPtr screen_pixmap = screen->GetScreenPixmap(screen);

    if (!drmmode_set_pixmap_bo(drmmode, screen_pixmap, &drmmode->front_bo))
        return FALSE;

    return TRUE;
}

static Bool
drmmode_xf86crtc_resize(ScrnInfoPtr scrn, int width, int height)
{
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    modesettingPtr ms = modesettingPTR(scrn);
    drmmode_ptr drmmode = &ms->drmmode;
    drmmode_bo old_front;
    ScreenPtr screen = xf86ScrnToScreen(scrn);
    uint32_t old_fb_id;
    int i, pitch, old_width, old_height, old_pitch;
    int cpp = (scrn->bitsPerPixel + 7) / 8;
    int kcpp = (drmmode->kbpp + 7) / 8;
    PixmapPtr ppix = screen->GetScreenPixmap(screen);
    void *new_pixels = NULL;

    if (scrn->virtualX == width && scrn->virtualY == height)
        return TRUE;

    xf86DrvMsg(scrn->scrnIndex, X_INFO,
               "Allocate new frame buffer %dx%d stride\n", width, height);

    old_width = scrn->virtualX;
    old_height = scrn->virtualY;
    old_pitch = drmmode_bo_get_pitch(&drmmode->front_bo);
    old_front = drmmode->front_bo;
    old_fb_id = drmmode->fb_id;
    drmmode->fb_id = 0;

    if (!drmmode_create_front_bo(drmmode, &drmmode->front_bo,
                                 width, height, drmmode->kbpp))
        goto fail;

    pitch = drmmode_bo_get_pitch(&drmmode->front_bo);

    scrn->virtualX = width;
    scrn->virtualY = height;
    scrn->displayWidth = pitch / kcpp;

    if (!drmmode->gbm) {
        new_pixels = drmmode_map_front_bo(drmmode);
        if (!new_pixels)
            goto fail;
    }

    if (drmmode->shadow_enable) {
        uint32_t size = scrn->displayWidth * scrn->virtualY * cpp;
        new_pixels = calloc(1, size);
        if (new_pixels == NULL)
            goto fail;
        free(drmmode->shadow_fb);
        drmmode->shadow_fb = new_pixels;
    }

    if (drmmode->shadow_enable2) {
        uint32_t size = scrn->displayWidth * scrn->virtualY * cpp;
        void *fb2 = calloc(1, size);
        free(drmmode->shadow_fb2);
        drmmode->shadow_fb2 = fb2;
    }

    screen->ModifyPixmapHeader(ppix, width, height, -1, -1,
                               scrn->displayWidth * cpp, new_pixels);

    if (!drmmode_glamor_handle_new_screen_pixmap(drmmode))
        goto fail;

    drmmode_clear_pixmap(ppix);

    for (i = 0; i < xf86_config->num_crtc; i++) {
        xf86CrtcPtr crtc = xf86_config->crtc[i];

        if (!crtc->enabled)
            continue;

        drmmode_set_mode_major(crtc, &crtc->mode,
                               crtc->rotation, crtc->x, crtc->y);
    }

    if (old_fb_id)
        drmModeRmFB(drmmode->fd, old_fb_id);

    drmmode_bo_destroy(drmmode, &old_front);

    return TRUE;

 fail:
    drmmode_bo_destroy(drmmode, &drmmode->front_bo);
    drmmode->front_bo = old_front;
    scrn->virtualX = old_width;
    scrn->virtualY = old_height;
    scrn->displayWidth = old_pitch / kcpp;
    drmmode->fb_id = old_fb_id;

    return FALSE;
}

static void
drmmode_validate_leases(ScrnInfoPtr scrn)
{
    ScreenPtr screen = scrn->pScreen;
    rrScrPrivPtr scr_priv;
    modesettingPtr ms = modesettingPTR(scrn);
    drmmode_ptr drmmode = &ms->drmmode;
    drmModeLesseeListPtr lessees;
    RRLeasePtr lease, next;
    int l;

    /* Bail out if RandR wasn't initialized. */
    if (!dixPrivateKeyRegistered(rrPrivKey))
        return;

    scr_priv = rrGetScrPriv(screen);

    /* We can't talk to the kernel about leases when VT switched */
    if (!scrn->vtSema)
        return;

    lessees = drmModeListLessees(drmmode->fd);
    if (!lessees)
        return;

    xorg_list_for_each_entry_safe(lease, next, &scr_priv->leases, list) {
        drmmode_lease_private_ptr lease_private = lease->devPrivate;

        for (l = 0; l < lessees->count; l++) {
            if (lessees->lessees[l] == lease_private->lessee_id)
                break;
        }

        /* check to see if the lease has gone away */
        if (l == lessees->count) {
            free(lease_private);
            lease->devPrivate = NULL;
            xf86CrtcLeaseTerminated(lease);
        }
    }

    free(lessees);
}

static int
drmmode_create_lease(RRLeasePtr lease, int *fd)
{
    ScreenPtr screen = lease->screen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);
    drmmode_ptr drmmode = &ms->drmmode;
    int ncrtc = lease->numCrtcs;
    int noutput = lease->numOutputs;
    int nobjects;
    int c, o;
    int i;
    int lease_fd;
    uint32_t *objects;
    drmmode_lease_private_ptr   lease_private;

    nobjects = ncrtc + noutput;

    if (ms->atomic_modeset)
        nobjects += ncrtc; /* account for planes as well */

    if (nobjects == 0)
        return BadValue;

    lease_private = calloc(1, sizeof (drmmode_lease_private_rec));
    if (!lease_private)
        return BadAlloc;

    objects = calloc(nobjects, sizeof(uint32_t));

    if (!objects) {
        free(lease_private);
        return BadAlloc;
    }

    i = 0;

    /* Add CRTC and plane ids */
    for (c = 0; c < ncrtc; c++) {
        xf86CrtcPtr crtc = lease->crtcs[c]->devPrivate;
        drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

        objects[i++] = drmmode_crtc->mode_crtc->crtc_id;
        if (ms->atomic_modeset)
            objects[i++] = drmmode_crtc->plane_id;
    }

    /* Add connector ids */

    for (o = 0; o < noutput; o++) {
        xf86OutputPtr   output = lease->outputs[o]->devPrivate;
        drmmode_output_private_ptr drmmode_output = output->driver_private;

        objects[i++] = drmmode_output->mode_output->connector_id;
    }

    /* call kernel to create lease */
    assert (i == nobjects);

    lease_fd = drmModeCreateLease(drmmode->fd, objects, nobjects, 0, &lease_private->lessee_id);

    free(objects);

    if (lease_fd < 0) {
        free(lease_private);
        return BadMatch;
    }

    lease->devPrivate = lease_private;

    xf86CrtcLeaseStarted(lease);

    *fd = lease_fd;
    return Success;
}

static void
drmmode_terminate_lease(RRLeasePtr lease)
{
    ScreenPtr screen = lease->screen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);
    drmmode_ptr drmmode = &ms->drmmode;
    drmmode_lease_private_ptr lease_private = lease->devPrivate;

    if (drmModeRevokeLease(drmmode->fd, lease_private->lessee_id) == 0) {
        free(lease_private);
        lease->devPrivate = NULL;
        xf86CrtcLeaseTerminated(lease);
    }
}

static const xf86CrtcConfigFuncsRec drmmode_xf86crtc_config_funcs = {
    .resize = drmmode_xf86crtc_resize,
    .create_lease = drmmode_create_lease,
    .terminate_lease = drmmode_terminate_lease
};

Bool
drmmode_pre_init(ScrnInfoPtr pScrn, drmmode_ptr drmmode, int cpp)
{
    modesettingEntPtr ms_ent = ms_ent_priv(pScrn);
    int i;
    int ret;
    uint64_t value = 0;
    unsigned int crtcs_needed = 0;
    drmModeResPtr mode_res;
    int crtcshift;

    /* check for dumb capability */
    ret = drmGetCap(drmmode->fd, DRM_CAP_DUMB_BUFFER, &value);
    if (ret > 0 || value != 1) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                   "KMS doesn't support dumb interface\n");
        return FALSE;
    }

    xf86CrtcConfigInit(pScrn, &drmmode_xf86crtc_config_funcs);

    drmmode->scrn = pScrn;
    drmmode->cpp = cpp;
    mode_res = drmModeGetResources(drmmode->fd);
    if (!mode_res)
        return FALSE;

    crtcshift = ffs(ms_ent->assigned_crtcs ^ 0xffffffff) - 1;
    for (i = 0; i < mode_res->count_connectors; i++)
        crtcs_needed += drmmode_output_init(pScrn, drmmode, mode_res, i, FALSE,
                                            crtcshift);

    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, MS_LOGLEVEL_DEBUG,
                   "Up to %d crtcs needed for screen.\n", crtcs_needed);

    xf86CrtcSetSizeRange(pScrn, 320, 200, mode_res->max_width,
                         mode_res->max_height);
    for (i = 0; i < mode_res->count_crtcs; i++)
        if (!xf86IsEntityShared(pScrn->entityList[0]) ||
            (crtcs_needed && !(ms_ent->assigned_crtcs & (1 << i))))
            crtcs_needed -= drmmode_crtc_init(pScrn, drmmode, mode_res, i);

    /* All ZaphodHeads outputs provided with matching crtcs? */
    if (xf86IsEntityShared(pScrn->entityList[0]) && (crtcs_needed > 0))
        xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
                   "%d ZaphodHeads crtcs unavailable. Some outputs will stay off.\n",
                   crtcs_needed);

    /* workout clones */
    drmmode_clones_init(pScrn, drmmode, mode_res);

    drmModeFreeResources(mode_res);
    xf86ProviderSetup(pScrn, NULL, "modesetting");

    xf86InitialConfiguration(pScrn, TRUE);

    return TRUE;
}

Bool
drmmode_init(ScrnInfoPtr pScrn, drmmode_ptr drmmode)
{
#ifdef GLAMOR_HAS_GBM
    ScreenPtr pScreen = xf86ScrnToScreen(pScrn);
    modesettingPtr ms = modesettingPTR(pScrn);

    if (drmmode->glamor) {
        if (!ms->glamor.init(pScreen, GLAMOR_USE_EGL_SCREEN)) {
            return FALSE;
        }
#ifdef GBM_BO_WITH_MODIFIERS
        ms->glamor.set_drawable_modifiers_func(pScreen, get_drawable_modifiers);
#endif
    }
#endif

    return TRUE;
}

void
drmmode_adjust_frame(ScrnInfoPtr pScrn, drmmode_ptr drmmode, int x, int y)
{
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
    xf86OutputPtr output = config->output[config->compat_output];
    xf86CrtcPtr crtc = output->crtc;

    if (crtc && crtc->enabled) {
        drmmode_set_mode_major(crtc, &crtc->mode, crtc->rotation, x, y);
    }
}

Bool
drmmode_set_desired_modes(ScrnInfoPtr pScrn, drmmode_ptr drmmode, Bool set_hw,
                          Bool ign_err)
{
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
    Bool success = TRUE;
    int c;

    drmmmode_prepare_modeset(pScrn);

    for (c = 0; c < config->num_crtc; c++) {
        xf86CrtcPtr crtc = config->crtc[c];
        drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
        xf86OutputPtr output = NULL;
        int o;

        /* Skip disabled CRTCs */
        if (!crtc->enabled) {
            if (set_hw) {
                drmModeSetCrtc(drmmode->fd, drmmode_crtc->mode_crtc->crtc_id,
                               0, 0, 0, NULL, 0, NULL);
            }
            continue;
        }

        if (config->output[config->compat_output]->crtc == crtc)
            output = config->output[config->compat_output];
        else {
            for (o = 0; o < config->num_output; o++)
                if (config->output[o]->crtc == crtc) {
                    output = config->output[o];
                    break;
                }
        }
        /* paranoia */
        if (!output)
            continue;

        /* Mark that we'll need to re-set the mode for sure */
        memset(&crtc->mode, 0, sizeof(crtc->mode));
        if (!crtc->desiredMode.CrtcHDisplay) {
            DisplayModePtr mode =
                xf86OutputFindClosestMode(output, pScrn->currentMode);

            if (!mode)
                return FALSE;
            crtc->desiredMode = *mode;
            crtc->desiredRotation = RR_Rotate_0;
            crtc->desiredX = 0;
            crtc->desiredY = 0;
        }

        if (set_hw) {
            if (!crtc->funcs->
                set_mode_major(crtc, &crtc->desiredMode, crtc->desiredRotation,
                               crtc->desiredX, crtc->desiredY)) {
                if (!ign_err)
                    return FALSE;
                else {
                    success = FALSE;
                    crtc->enabled = FALSE;
                    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
                               "Failed to set the desired mode on connector %s\n",
                               output->name);
                }
            }
        } else {
            crtc->mode = crtc->desiredMode;
            crtc->rotation = crtc->desiredRotation;
            crtc->x = crtc->desiredX;
            crtc->y = crtc->desiredY;
            if (!xf86CrtcRotate(crtc))
                return FALSE;
        }
    }

    /* Validate leases on VT re-entry */
    drmmode_validate_leases(pScrn);

    return success;
}

static void
drmmode_load_palette(ScrnInfoPtr pScrn, int numColors,
                     int *indices, LOCO * colors, VisualPtr pVisual)
{
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
    uint16_t lut_r[256], lut_g[256], lut_b[256];
    int index, j, i;
    int c;

    for (c = 0; c < xf86_config->num_crtc; c++) {
        xf86CrtcPtr crtc = xf86_config->crtc[c];
        drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

        for (i = 0; i < 256; i++) {
            lut_r[i] = drmmode_crtc->lut_r[i] << 6;
            lut_g[i] = drmmode_crtc->lut_g[i] << 6;
            lut_b[i] = drmmode_crtc->lut_b[i] << 6;
        }

        switch (pScrn->depth) {
        case 15:
            for (i = 0; i < numColors; i++) {
                index = indices[i];
                for (j = 0; j < 8; j++) {
                    lut_r[index * 8 + j] = colors[index].red << 6;
                    lut_g[index * 8 + j] = colors[index].green << 6;
                    lut_b[index * 8 + j] = colors[index].blue << 6;
                }
            }
            break;
        case 16:
            for (i = 0; i < numColors; i++) {
                index = indices[i];

                if (i <= 31) {
                    for (j = 0; j < 8; j++) {
                        lut_r[index * 8 + j] = colors[index].red << 6;
                        lut_b[index * 8 + j] = colors[index].blue << 6;
                    }
                }

                for (j = 0; j < 4; j++) {
                    lut_g[index * 4 + j] = colors[index].green << 6;
                }
            }
            break;
        default:
            for (i = 0; i < numColors; i++) {
                index = indices[i];
                lut_r[index] = colors[index].red << 6;
                lut_g[index] = colors[index].green << 6;
                lut_b[index] = colors[index].blue << 6;
            }
            break;
        }

        /* Make the change through RandR */
        if (crtc->randr_crtc)
            RRCrtcGammaSet(crtc->randr_crtc, lut_r, lut_g, lut_b);
        else
            crtc->funcs->gamma_set(crtc, lut_r, lut_g, lut_b, 256);
    }
}

static Bool
drmmode_crtc_upgrade_lut(xf86CrtcPtr crtc, int num)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    uint64_t size;

    if (!drmmode_crtc->use_gamma_lut)
        return TRUE;

    assert(drmmode_crtc->props[DRMMODE_CRTC_GAMMA_LUT_SIZE].prop_id);

    size = drmmode_crtc->props[DRMMODE_CRTC_GAMMA_LUT_SIZE].value;

    if (size != crtc->gamma_size) {
        ScrnInfoPtr pScrn = crtc->scrn;
        uint16_t *gamma = calloc(3 * size, sizeof(uint16_t));

        if (gamma) {
            free(crtc->gamma_red);

            crtc->gamma_size = size;
            crtc->gamma_red = gamma;
            crtc->gamma_green = gamma + size;
            crtc->gamma_blue = gamma + size * 2;

            xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, MS_LOGLEVEL_DEBUG,
                           "Gamma ramp set to %lld entries on CRTC %d\n",
                           (long long)size, num);
        } else {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                       "Failed to allocate memory for %lld gamma ramp entries "
                       "on CRTC %d.\n",
                       (long long)size, num);
            return FALSE;
        }
    }

    return TRUE;
}

Bool
drmmode_setup_colormap(ScreenPtr pScreen, ScrnInfoPtr pScrn)
{
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
    int i;

    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
              "Initializing kms color map for depth %d, %d bpc.\n",
              pScrn->depth, pScrn->rgbBits);
    if (!miCreateDefColormap(pScreen))
        return FALSE;

    /* If the GAMMA_LUT property is available, replace the server's default
     * gamma ramps with ones of the appropriate size. */
    for (i = 0; i < xf86_config->num_crtc; i++)
        if (!drmmode_crtc_upgrade_lut(xf86_config->crtc[i], i))
            return FALSE;

    /* Adapt color map size and depth to color depth of screen. */
    if (!xf86HandleColormaps(pScreen, 1 << pScrn->rgbBits, 10,
                             drmmode_load_palette, NULL,
                             CMAP_PALETTED_TRUECOLOR |
                             CMAP_RELOAD_ON_MODE_SWITCH))
        return FALSE;
    return TRUE;
}

#define DRM_MODE_LINK_STATUS_GOOD       0
#define DRM_MODE_LINK_STATUS_BAD        1

void
drmmode_update_kms_state(drmmode_ptr drmmode)
{
    ScrnInfoPtr scrn = drmmode->scrn;
    drmModeResPtr mode_res;
    xf86CrtcConfigPtr  config = XF86_CRTC_CONFIG_PTR(scrn);
    int i, j;
    Bool found = FALSE;
    Bool changed = FALSE;

    /* Try to re-set the mode on all the connectors with a BAD link-state:
     * This may happen if a link degrades and a new modeset is necessary, using
     * different link-training parameters. If the kernel found that the current
     * mode is not achievable anymore, it should have pruned the mode before
     * sending the hotplug event. Try to re-set the currently-set mode to keep
     * the display alive, this will fail if the mode has been pruned.
     * In any case, we will send randr events for the Desktop Environment to
     * deal with it, if it wants to.
     */
    for (i = 0; i < config->num_output; i++) {
        xf86OutputPtr output = config->output[i];
        drmmode_output_private_ptr drmmode_output = output->driver_private;

        drmmode_output_detect(output);

        /* Get an updated view of the properties for the current connector and
         * look for the link-status property
         */
        for (j = 0; j < drmmode_output->num_props; j++) {
            drmmode_prop_ptr p = &drmmode_output->props[j];

            if (!strcmp(p->mode_prop->name, "link-status")) {
                if (p->value == DRM_MODE_LINK_STATUS_BAD) {
                    xf86CrtcPtr crtc = output->crtc;
                    if (!crtc)
                        continue;

                    /* the connector got a link failure, re-set the current mode */
                    drmmode_set_mode_major(crtc, &crtc->mode, crtc->rotation,
                                           crtc->x, crtc->y);

                    drmModeConnectorPtr mode_output = drmmode_output->mode_output;
                    if (mode_output) {
                        xf86DrvMsg(scrn->scrnIndex, X_WARNING,
                                   "hotplug event: connector %u's link-state is BAD, "
                                   "tried resetting the current mode. You may be left "
                                   "with a black screen if this fails...\n",
                                   mode_output->connector_id);
                    } else {
                        xf86DrvMsg(scrn->scrnIndex, X_WARNING,
                                   "hotplug event: NULL connector's link-state is BAD, "
                                   "tried resetting the current mode. You may be left "
                                   "with a black screen if this fails...\n");
                    }
                }
                break;
            }
        }
    }

    mode_res = drmModeGetResources(drmmode->fd);
    if (!mode_res)
        goto out;

    if (mode_res->count_crtcs != config->num_crtc) {
        /* this triggers with Zaphod mode where we don't currently support connector hotplug or MST. */
        goto out_free_res;
    }

    /* figure out if we have gotten rid of any connectors
       traverse old output list looking for outputs */
    for (i = 0; i < config->num_output; i++) {
        xf86OutputPtr output = config->output[i];
        drmmode_output_private_ptr drmmode_output;

        drmmode_output = output->driver_private;
        found = FALSE;
        for (j = 0; j < mode_res->count_connectors; j++) {
            if (mode_res->connectors[j] == drmmode_output->output_id) {
                found = TRUE;
                break;
            }
        }
        if (found)
            continue;

        drmModeFreeConnector(drmmode_output->mode_output);
        drmmode_output->mode_output = NULL;
        drmmode_output->output_id = -1;

        changed = TRUE;
    }

    /* find new output ids we don't have outputs for */
    for (i = 0; i < mode_res->count_connectors; i++) {
        found = FALSE;

        for (j = 0; j < config->num_output; j++) {
            xf86OutputPtr output = config->output[j];
            drmmode_output_private_ptr drmmode_output;

            drmmode_output = output->driver_private;
            if (mode_res->connectors[i] == drmmode_output->output_id) {
                found = TRUE;
                break;
            }
        }
        if (found)
            continue;

        changed = TRUE;
        drmmode_output_init(scrn, drmmode, mode_res, i, TRUE, 0);
    }

    if (changed) {
        RRSetChanged(xf86ScrnToScreen(scrn));
        RRTellChanged(xf86ScrnToScreen(scrn));
    }

out_free_res:

    /* Check to see if a lessee has disappeared */
    drmmode_validate_leases(scrn);

    drmModeFreeResources(mode_res);
out:
    RRGetInfo(xf86ScrnToScreen(scrn), TRUE);
}

#undef DRM_MODE_LINK_STATUS_BAD
#undef DRM_MODE_LINK_STATUS_GOOD

#ifdef CONFIG_UDEV_KMS

static void
drmmode_handle_uevents(int fd, void *closure)
{
    drmmode_ptr drmmode = closure;
    struct udev_device *dev;
    Bool found = FALSE;

    while ((dev = udev_monitor_receive_device(drmmode->uevent_monitor))) {
        udev_device_unref(dev);
        found = TRUE;
    }
    if (!found)
        return;

    drmmode_update_kms_state(drmmode);
}

#endif

void
drmmode_uevent_init(ScrnInfoPtr scrn, drmmode_ptr drmmode)
{
#ifdef CONFIG_UDEV_KMS
    struct udev *u;
    struct udev_monitor *mon;

    u = udev_new();
    if (!u)
        return;
    mon = udev_monitor_new_from_netlink(u, "udev");
    if (!mon) {
        udev_unref(u);
        return;
    }

    if (udev_monitor_filter_add_match_subsystem_devtype(mon,
                                                        "drm",
                                                        "drm_minor") < 0 ||
        udev_monitor_enable_receiving(mon) < 0) {
        udev_monitor_unref(mon);
        udev_unref(u);
        return;
    }

    drmmode->uevent_handler =
        xf86AddGeneralHandler(udev_monitor_get_fd(mon),
                              drmmode_handle_uevents, drmmode);

    drmmode->uevent_monitor = mon;
#endif
}

void
drmmode_uevent_fini(ScrnInfoPtr scrn, drmmode_ptr drmmode)
{
#ifdef CONFIG_UDEV_KMS
    if (drmmode->uevent_handler) {
        struct udev *u = udev_monitor_get_udev(drmmode->uevent_monitor);

        xf86RemoveGeneralHandler(drmmode->uevent_handler);

        udev_monitor_unref(drmmode->uevent_monitor);
        udev_unref(u);
    }
#endif
}

static inline void
drmmode_reset_cursor(drmmode_crtc_private_ptr drmmode_crtc)
{
    /* Mark the entire cursor buffer as dirty */
    drmmode_crtc->cursor_glyph_width = 0;
    drmmode_crtc->cursor_glyph_height = 0;
    drmmode_crtc->old_pitch = 0;

    /* If we had any cursor pitches for the old cursor, they are no longer valid now */
    free(drmmode_crtc->cursor_pitches);
    drmmode_crtc->cursor_pitches = NULL;
}

/*
 * This is the old probe method for the minimum cursor size.
 * This is only used if the SIZE_HINTS probe fails.
 */
static void drmmode_probe_cursor_size(xf86CrtcPtr crtc)
{
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    uint32_t handle = drmmode_crtc->cursor.bo->handle;
    drmmode_ptr drmmode = drmmode_crtc->drmmode;
    drmmode_cursor_ptr drmmode_cursor = &drmmode_crtc->cursor;
    int width, height, size;
    int max_width, max_height;
    int min_width, min_height;

    if (drmmode_crtc->cursor_probed) {
        return;
    }

    drmmode_crtc->cursor_probed = TRUE;

    xf86DrvMsg(crtc->scrn->scrnIndex, X_WARNING,
               "Probing the cursor size using the old method\n");

    /* If we're here, we only have one size, the fallback size */
    max_width = drmmode_cursor->dimensions[0].width;
    max_height = drmmode_cursor->dimensions[0].height;

    min_width = max_width;
    min_height = max_height;

    /* probe square min first */
    for (size = 1; size <= max_width &&
             size <= max_height; size *= 2) {
        int ret;

        ret = drmModeSetCursor2(drmmode->fd, drmmode_crtc->mode_crtc->crtc_id,
                                handle, size, size, 0, 0);
        if (ret == 0) {
            min_width = size;
            min_height = size;
            break;
        }
    }

    /* check if smaller width works with non-square */
    for (width = 1; width <= size; width *= 2) {
        int ret;

        ret = drmModeSetCursor2(drmmode->fd, drmmode_crtc->mode_crtc->crtc_id,
                                handle, width, size, 0, 0);
        if (ret == 0) {
            min_width = width;
            break;
        }
    }

    /* check if smaller height works with non-square */
    for (height = 1; height <= size; height *= 2) {
        int ret;

        ret = drmModeSetCursor2(drmmode->fd, drmmode_crtc->mode_crtc->crtc_id,
                                handle, size, height, 0, 0);
        if (ret == 0) {
            min_height = height;
            break;
        }
    }

    drmModeSetCursor2(drmmode->fd, drmmode_crtc->mode_crtc->crtc_id, 0, 0, 0, 0, 0);

    if (min_width == max_width && min_height == max_height) {
        xf86DrvMsgVerb(crtc->scrn->scrnIndex, X_INFO, MS_LOGLEVEL_DEBUG,
                       "Cursor size: %dx%d\n",
                       min_width, min_height);

        return;
    }

    drmmode_reset_cursor(drmmode_crtc);

    /*
     * We could add as many sizes as we want here.
     * We want the minimum size to be here, and we need the maximum size to be here,
     * because that's what we initialize the cursor image with, and we could theoretically
     * get cursor glyph sizes that big.
     *
     * There is no problem with multiple sizes being equal here.
     * We want dimensions[i] <= dimensions[i + 1] for all i, but even if
     * this doesn't happen, there shouldn't be any issues.
     */

    int num_dimensions = 1;
    if (min_width > min_height) {
        for(int j = min_height; j <= min_width; j *= 2) {
            num_dimensions++;
        }
    } else {
        for(int j = min_width; j <= min_height; j *= 2) {
            num_dimensions++;
        }

    }

    void *tmp = realloc(drmmode_cursor->dimensions, num_dimensions * sizeof(drmmode_cursor_dim_rec));
    if (!tmp) {
        xf86DrvMsgVerb(crtc->scrn->scrnIndex, X_INFO, MS_LOGLEVEL_DEBUG,
                       "Cursor size: %dx%d\n",
                       max_width, max_height);
        return;
    }

    drmmode_cursor->dimensions = tmp;
    drmmode_cursor->num_dimensions = num_dimensions;

    if (min_width > min_height) {
        int idx = 0;
        for(int j = min_height; j <= min_width; j *= 2) {
            drmmode_cursor->dimensions[idx].width = min_width;
            drmmode_cursor->dimensions[idx].height = j;
            idx++;
        }
    } else {
        int idx = 0;
        for(int j = min_width; j <= min_height; j *= 2) {
            drmmode_cursor->dimensions[idx].width = j;
            drmmode_cursor->dimensions[idx].height = min_height;
            idx++;
        }
    }

    /* maximum size */
    drmmode_cursor->dimensions[num_dimensions - 1].width = max_width;
    drmmode_cursor->dimensions[num_dimensions - 1].height = max_height;

    xf86DrvMsgVerb(crtc->scrn->scrnIndex, X_INFO, MS_LOGLEVEL_DEBUG,
                   "Minimum cursor size: %dx%d\n",
                   min_width, min_height);
}

/* create front and cursor BOs */
Bool
drmmode_create_initial_bos(ScrnInfoPtr pScrn, drmmode_ptr drmmode)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
    int bpp = ms->drmmode.kbpp;
    int cpp = (bpp + 7) / 8;

    int width, height;
    uint32_t min_width = 1 << 30, min_height = 1 << 30;

    width = pScrn->virtualX;
    height = pScrn->virtualY;

    if (!drmmode_create_front_bo(drmmode, &drmmode->front_bo, width, height, bpp))
        return FALSE;
    pScrn->displayWidth = drmmode_bo_get_pitch(&drmmode->front_bo) / cpp;

    for (int i = 0; i < xf86_config->num_crtc; i++) {
        xf86CrtcPtr crtc = xf86_config->crtc[i];
        drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
        drmmode_cursor_rec cursor = drmmode_crtc->cursor;

        /* If we don't have any dimensions then
         * something has gone terribly wrong. */
        assert(cursor.num_dimensions);

        /* Use the maximum available size. */
        width  = cursor.dimensions[cursor.num_dimensions - 1].width;
        height = cursor.dimensions[cursor.num_dimensions - 1].height;

        /* We take the minimum of the sizes here
         * so that we don't get a cursor glyph larger
         * that a crtc's cursor buffer */
        min_width  = MIN(width, min_width);
        min_height = MIN(height, min_height);

        drmmode_crtc->cursor.bo = dumb_bo_create(drmmode->fd, width, height, bpp);
        if (!drmmode_crtc->cursor.bo) {
            drmmode_bo_destroy(drmmode, &drmmode->front_bo);
            for (int j = 0; j < i; j++) {
                xf86CrtcPtr free_crtc = xf86_config->crtc[j];
                drmmode_crtc_private_ptr free_drmmode_crtc = free_crtc->driver_private;
                dumb_bo_destroy(drmmode->fd, free_drmmode_crtc->cursor.bo);
                free_drmmode_crtc->cursor.bo = NULL;
            }
            return FALSE;
        }
    }

    ms->cursor_image_width  = min_width;
    ms->cursor_image_height = min_height;

    return TRUE;
}

void *
drmmode_map_front_bo(drmmode_ptr drmmode)
{
    return drmmode_bo_map(drmmode, &drmmode->front_bo);
}

void *
drmmode_map_secondary_bo(drmmode_ptr drmmode, msPixmapPrivPtr ppriv)
{
    int ret;

    if (ppriv->backing_bo->ptr)
        return ppriv->backing_bo->ptr;

    ret = dumb_bo_map(drmmode->fd, ppriv->backing_bo);
    if (ret)
        return NULL;

    return ppriv->backing_bo->ptr;
}

Bool
drmmode_map_cursor_bos(ScrnInfoPtr pScrn, drmmode_ptr drmmode)
{
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
    int i, ret;

    for (i = 0; i < xf86_config->num_crtc; i++) {
        xf86CrtcPtr crtc = xf86_config->crtc[i];
        drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

        ret = dumb_bo_map(drmmode->fd, drmmode_crtc->cursor.bo);
        if (ret)
            return FALSE;
    }
    return TRUE;
}

void
drmmode_free_bos(ScrnInfoPtr pScrn, drmmode_ptr drmmode)
{
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
    int i;

    if (drmmode->fb_id) {
        drmModeRmFB(drmmode->fd, drmmode->fb_id);
        drmmode->fb_id = 0;
    }

    drmmode_bo_destroy(drmmode, &drmmode->front_bo);

    for (i = 0; i < xf86_config->num_crtc; i++) {
        xf86CrtcPtr crtc = xf86_config->crtc[i];
        drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

        dumb_bo_destroy(drmmode->fd, drmmode_crtc->cursor.bo);
        drmmode_destroy_tearfree_shadow(crtc);
    }
}

/* ugly workaround to see if we can create 32bpp */
void
drmmode_get_default_bpp(ScrnInfoPtr pScrn, drmmode_ptr drmmode, int *depth,
                        int *bpp)
{
    drmModeResPtr mode_res;
    uint64_t value;
    struct dumb_bo *bo;
    uint32_t fb_id;
    int ret;

    /* 16 is fine */
    ret = drmGetCap(drmmode->fd, DRM_CAP_DUMB_PREFERRED_DEPTH, &value);
    if (!ret && (value == 16 || value == 8)) {
        *depth = value;
        *bpp = value;
        return;
    }

    *depth = 24;
    mode_res = drmModeGetResources(drmmode->fd);
    if (!mode_res)
        return;

    if (mode_res->min_width == 0)
        mode_res->min_width = 1;
    if (mode_res->min_height == 0)
        mode_res->min_height = 1;
    /*create a bo */
    bo = dumb_bo_create(drmmode->fd, mode_res->min_width, mode_res->min_height,
                        32);
    if (!bo) {
        *bpp = 24;
        goto out;
    }

    ret = drmModeAddFB(drmmode->fd, mode_res->min_width, mode_res->min_height,
                       24, 32, bo->pitch, bo->handle, &fb_id);

    if (ret) {
        *bpp = 24;
        dumb_bo_destroy(drmmode->fd, bo);
        goto out;
    }

    drmModeRmFB(drmmode->fd, fb_id);
    *bpp = 32;

    dumb_bo_destroy(drmmode->fd, bo);
 out:
    drmModeFreeResources(mode_res);
    return;
}

void
drmmode_crtc_set_vrr(xf86CrtcPtr crtc, Bool enabled)
{
    ScrnInfoPtr pScrn = crtc->scrn;
    modesettingPtr ms = modesettingPTR(pScrn);
    drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;
    drmmode_ptr drmmode = drmmode_crtc->drmmode;

    if (drmmode->vrr_prop_id && drmmode_crtc->vrr_enabled != enabled &&
        drmModeObjectSetProperty(ms->fd,
                                 drmmode_crtc->mode_crtc->crtc_id,
                                 DRM_MODE_OBJECT_CRTC,
                                 drmmode->vrr_prop_id,
                                 enabled) == 0)
        drmmode_crtc->vrr_enabled = enabled;
}

/*
 * We hook the screen's cursor-sprite (swcursor) functions to see if a swcursor
 * is active. When a swcursor is active we disable page-flipping.
 */

static void drmmode_sprite_do_set_cursor(msSpritePrivPtr sprite_priv,
                                         ScrnInfoPtr scrn, int x, int y)
{
    modesettingPtr ms = modesettingPTR(scrn);
    CursorPtr cursor = sprite_priv->cursor;
    Bool sprite_visible = sprite_priv->sprite_visible;

    if (cursor) {
        x -= cursor->bits->xhot;
        y -= cursor->bits->yhot;

        sprite_priv->sprite_visible =
            x < scrn->virtualX && y < scrn->virtualY &&
            (x + cursor->bits->width > 0) &&
            (y + cursor->bits->height > 0);
    } else {
        sprite_priv->sprite_visible = FALSE;
    }

    ms->drmmode.sprites_visible += sprite_priv->sprite_visible - sprite_visible;
}

static void drmmode_sprite_set_cursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                                      CursorPtr pCursor, int x, int y)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(pScreen);
    modesettingPtr ms = modesettingPTR(scrn);
    msSpritePrivPtr sprite_priv = msGetSpritePriv(pDev, ms, pScreen);

    sprite_priv->cursor = pCursor;
    drmmode_sprite_do_set_cursor(sprite_priv, scrn, x, y);

    ms->SpriteFuncs->SetCursor(pDev, pScreen, pCursor, x, y);
}

static void drmmode_sprite_move_cursor(DeviceIntPtr pDev, ScreenPtr pScreen,
                                       int x, int y)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(pScreen);
    modesettingPtr ms = modesettingPTR(scrn);
    msSpritePrivPtr sprite_priv = msGetSpritePriv(pDev, ms, pScreen);

    drmmode_sprite_do_set_cursor(sprite_priv, scrn, x, y);

    ms->SpriteFuncs->MoveCursor(pDev, pScreen, x, y);
}

static Bool drmmode_sprite_realize_realize_cursor(DeviceIntPtr pDev,
                                                  ScreenPtr pScreen,
                                                  CursorPtr pCursor)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(pScreen);
    modesettingPtr ms = modesettingPTR(scrn);

    return ms->SpriteFuncs->RealizeCursor(pDev, pScreen, pCursor);
}

static Bool drmmode_sprite_realize_unrealize_cursor(DeviceIntPtr pDev,
                                                    ScreenPtr pScreen,
                                                    CursorPtr pCursor)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(pScreen);
    modesettingPtr ms = modesettingPTR(scrn);

    return ms->SpriteFuncs->UnrealizeCursor(pDev, pScreen, pCursor);
}

static Bool drmmode_sprite_device_cursor_initialize(DeviceIntPtr pDev,
                                                    ScreenPtr pScreen)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(pScreen);
    modesettingPtr ms = modesettingPTR(scrn);

    return ms->SpriteFuncs->DeviceCursorInitialize(pDev, pScreen);
}

static void drmmode_sprite_device_cursor_cleanup(DeviceIntPtr pDev,
                                                 ScreenPtr pScreen)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(pScreen);
    modesettingPtr ms = modesettingPTR(scrn);

    ms->SpriteFuncs->DeviceCursorCleanup(pDev, pScreen);
}

miPointerSpriteFuncRec drmmode_sprite_funcs = {
    .RealizeCursor = drmmode_sprite_realize_realize_cursor,
    .UnrealizeCursor = drmmode_sprite_realize_unrealize_cursor,
    .SetCursor = drmmode_sprite_set_cursor,
    .MoveCursor = drmmode_sprite_move_cursor,
    .DeviceCursorInitialize = drmmode_sprite_device_cursor_initialize,
    .DeviceCursorCleanup = drmmode_sprite_device_cursor_cleanup,
};
