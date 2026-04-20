/*
 * Copyright © 2010 Intel Corporation.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including
 * the next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Zhigang Gong <zhigang.gong@linux.intel.com>
 *
 */
#include <dix-config.h>

#define GLAMOR_FOR_XORG /* for -Wmissing-prototypes */
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef WITH_LIBDRM
#include <xf86drm.h>
#include <drm_fourcc.h>
#endif

#define EGL_DISPLAY_NO_X_MESA

#ifdef GLAMOR_HAS_GBM
#include <gbm.h>
#endif

#include "dix/screen_hooks_priv.h"
#include "glamor/glamor_priv.h"
#include "os/bug_priv.h"

#include "glamor.h"
#include "glamor_egl.h"
#include "glamor_egl_ext.h"
#include "glamor_glx_provider.h"
#include "dri3.h"

/**
 * Hack to not break xf86 drivers.
 *
 * We actually want this to be a regular dixprivate,
 * just like the regular glamor private is.
 *
 * However, this risks breaking drivers.
 *
 * See: https://gitlab.freedesktop.org/xorg/xserver/-/merge_requests/309
 */

static glamor_egl_priv_t*
(*glamor_egl_get_screen_private)(ScreenPtr screen) = NULL;

static void
glamor_egl_make_current(struct glamor_context *glamor_ctx)
{
    /* There's only a single global dispatch table in Mesa.  EGL, GLX,
     * and AIGLX's direct dispatch table manipulation don't talk to
     * each other.  We need to set the context to NULL first to avoid
     * EGL's no-op context change fast path when switching back to
     * EGL.
     */
    eglMakeCurrent(glamor_ctx->display, EGL_NO_SURFACE,
                   EGL_NO_SURFACE, EGL_NO_CONTEXT);

    if (!eglMakeCurrent(glamor_ctx->display,
                        glamor_ctx->surface, glamor_ctx->surface,
                        glamor_ctx->ctx)) {
        FatalError("Failed to make EGL context current\n");
    }
}

#if defined(GLAMOR_HAS_GBM) && defined (WITH_LIBDRM)
static int
glamor_get_flink_name(int fd, int handle, int *name)
{
    struct drm_gem_flink flink;

    flink.handle = handle;
    if (ioctl(fd, DRM_IOCTL_GEM_FLINK, &flink) < 0) {

	/*
	 * Assume non-GEM kernels have names identical to the handle
	 */
	if (errno == ENODEV) {
	    *name = handle;
	    return TRUE;
	} else {
	    return FALSE;
	}
    }
    *name = flink.name;
    return TRUE;
}
#endif

#ifdef GLAMOR_HAS_GBM
static Bool
glamor_create_texture_from_image(ScreenPtr screen,
                                 EGLImageKHR image, GLuint * texture)
{
    struct glamor_screen_private *glamor_priv =
        glamor_get_screen_private(screen);

    glamor_make_current(glamor_priv);

    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);
    glBindTexture(GL_TEXTURE_2D, 0);

    return TRUE;
}
#endif

struct gbm_device *
glamor_egl_get_gbm_device(ScreenPtr screen)
{
#ifdef GLAMOR_HAS_GBM
    return glamor_egl_get_screen_private(screen)->gbm;
#else
    return NULL;
#endif
}

#ifdef GLAMOR_HAS_GBM
static void
glamor_egl_set_pixmap_image(PixmapPtr pixmap, EGLImageKHR image,
                            Bool used_modifiers)
{
    struct glamor_pixmap_private *pixmap_priv =
        glamor_get_pixmap_private(pixmap);
    EGLImageKHR old;

    BUG_RETURN(!pixmap_priv);

    old = pixmap_priv->image;
    if (old) {
        ScreenPtr                               screen = pixmap->drawable.pScreen;
        glamor_egl_priv_t        *glamor_egl = glamor_egl_get_screen_private(screen);

        eglDestroyImageKHR(glamor_egl->display, old);
    }
    pixmap_priv->image = image;
    pixmap_priv->used_modifiers = used_modifiers;
}
#endif

Bool
glamor_egl_create_textured_pixmap(PixmapPtr pixmap, int handle, int stride)
{
#ifdef WITH_LIBDRM
    ScreenPtr screen = pixmap->drawable.pScreen;
    glamor_egl_priv_t *glamor_egl =
        glamor_egl_get_screen_private(screen);
    int ret, fd;

    /* GBM doesn't have an import path from handles, so we make a
     * dma-buf fd from it and then go through that.
     */
    ret = drmPrimeHandleToFD(glamor_egl->fd, handle, O_CLOEXEC, &fd);
    if (ret) {
        LogMessage(X_ERROR,
                   "Failed to make prime FD for handle: %d\n", errno);
        return FALSE;
    }

    if (!glamor_back_pixmap_from_fd(pixmap, fd,
                                    pixmap->drawable.width,
                                    pixmap->drawable.height,
                                    stride,
                                    pixmap->drawable.depth,
                                    pixmap->drawable.bitsPerPixel)) {
        LogMessage(X_ERROR,
                   "Failed to make import prime FD as pixmap: %d\n", errno);
        close(fd);
        return FALSE;
    }

    close(fd);
    return TRUE;
#else
    return FALSE;
#endif
}

Bool
glamor_egl_create_textured_pixmap_from_gbm_bo(PixmapPtr pixmap,
                                              struct gbm_bo *bo,
                                              Bool used_modifiers)
{
#ifdef GLAMOR_HAS_GBM
    ScreenPtr screen = pixmap->drawable.pScreen;
    struct glamor_screen_private *glamor_priv =
        glamor_get_screen_private(screen);
    glamor_egl_priv_t *glamor_egl;
    EGLImageKHR image;
    GLuint texture;
    Bool ret = FALSE;

    glamor_egl = glamor_egl_get_screen_private(screen);
#ifdef GBM_BO_FD_FOR_PLANE
    uint64_t modifier = gbm_bo_get_modifier(bo);
    const int num_planes = gbm_bo_get_plane_count(bo);
    int fds[GBM_MAX_PLANES];
    int plane;
    int attr_num = 0;
    EGLint img_attrs[64] = {0};
    enum PlaneAttrs {
        PLANE_FD,
        PLANE_OFFSET,
        PLANE_PITCH,
        PLANE_MODIFIER_LO,
        PLANE_MODIFIER_HI,
        NUM_PLANE_ATTRS
    };
    static const EGLint planeAttrs[][NUM_PLANE_ATTRS] = {
        {
            EGL_DMA_BUF_PLANE0_FD_EXT,
            EGL_DMA_BUF_PLANE0_OFFSET_EXT,
            EGL_DMA_BUF_PLANE0_PITCH_EXT,
            EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT,
            EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT,
        },
        {
            EGL_DMA_BUF_PLANE1_FD_EXT,
            EGL_DMA_BUF_PLANE1_OFFSET_EXT,
            EGL_DMA_BUF_PLANE1_PITCH_EXT,
            EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT,
            EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT,
        },
        {
            EGL_DMA_BUF_PLANE2_FD_EXT,
            EGL_DMA_BUF_PLANE2_OFFSET_EXT,
            EGL_DMA_BUF_PLANE2_PITCH_EXT,
            EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT,
            EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT,
        },
        {
            EGL_DMA_BUF_PLANE3_FD_EXT,
            EGL_DMA_BUF_PLANE3_OFFSET_EXT,
            EGL_DMA_BUF_PLANE3_PITCH_EXT,
            EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT,
            EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT,
        },
    };

    for (plane = 0; plane < num_planes; plane++) fds[plane] = -1;
#endif

    glamor_make_current(glamor_priv);

#ifdef GBM_BO_FD_FOR_PLANE
    if (glamor_egl->dmabuf_capable) {
#define ADD_ATTR(attrs, num, attr)                                      \
        do {                                                            \
            assert(((num) + 1) < (sizeof(attrs) / sizeof((attrs)[0]))); \
            (attrs)[(num)++] = (attr);                                  \
        } while (0)
        ADD_ATTR(img_attrs, attr_num, EGL_WIDTH);
        ADD_ATTR(img_attrs, attr_num, gbm_bo_get_width(bo));
        ADD_ATTR(img_attrs, attr_num, EGL_HEIGHT);
        ADD_ATTR(img_attrs, attr_num, gbm_bo_get_height(bo));
        ADD_ATTR(img_attrs, attr_num, EGL_LINUX_DRM_FOURCC_EXT);
        ADD_ATTR(img_attrs, attr_num, gbm_bo_get_format(bo));

        for (plane = 0; plane < num_planes; plane++) {
            fds[plane] = gbm_bo_get_fd_for_plane(bo, plane);
            ADD_ATTR(img_attrs, attr_num, planeAttrs[plane][PLANE_FD]);
            ADD_ATTR(img_attrs, attr_num, fds[plane]);
            ADD_ATTR(img_attrs, attr_num, planeAttrs[plane][PLANE_OFFSET]);
            ADD_ATTR(img_attrs, attr_num, gbm_bo_get_offset(bo, plane));
            ADD_ATTR(img_attrs, attr_num, planeAttrs[plane][PLANE_PITCH]);
            ADD_ATTR(img_attrs, attr_num, gbm_bo_get_stride_for_plane(bo, plane));
            ADD_ATTR(img_attrs, attr_num, planeAttrs[plane][PLANE_MODIFIER_LO]);
            ADD_ATTR(img_attrs, attr_num, (uint32_t)(modifier & 0xFFFFFFFFULL));
            ADD_ATTR(img_attrs, attr_num, planeAttrs[plane][PLANE_MODIFIER_HI]);
            ADD_ATTR(img_attrs, attr_num, (uint32_t)(modifier >> 32ULL));
        }
        ADD_ATTR(img_attrs, attr_num, EGL_NONE);
#undef ADD_ATTR

        image = eglCreateImageKHR(glamor_egl->display,
                                  EGL_NO_CONTEXT,
                                  EGL_LINUX_DMA_BUF_EXT,
                                  NULL,
                                  img_attrs);

        for (plane = 0; plane < num_planes; plane++) {
            close(fds[plane]);
            fds[plane] = -1;
        }
    }
    else
#endif
    {
        image = eglCreateImageKHR(glamor_egl->display,
                                  EGL_NO_CONTEXT,
                                  EGL_NATIVE_PIXMAP_KHR, bo, NULL);
    }

    if (image == EGL_NO_IMAGE_KHR) {
        glamor_set_pixmap_type(pixmap, GLAMOR_DRM_ONLY);
        goto done;
    }
    glamor_create_texture_from_image(screen, image, &texture);
    glamor_set_pixmap_type(pixmap, GLAMOR_TEXTURE_DRM);
    glamor_set_pixmap_texture(pixmap, texture);
    glamor_egl_set_pixmap_image(pixmap, image, used_modifiers);
    ret = TRUE;

 done:
    return ret;
#else
    return FALSE;
#endif
}

#if defined(GLAMOR_HAS_GBM) && defined (WITH_LIBDRM)
static void
glamor_get_name_from_bo(int gbm_fd, struct gbm_bo *bo, int *name)
{
    union gbm_bo_handle handle;

    handle = gbm_bo_get_handle(bo);
    if (!glamor_get_flink_name(gbm_fd, handle.u32, name))
        *name = -1;
}
#endif

#ifdef GLAMOR_HAS_GBM
static Bool
glamor_make_pixmap_exportable(PixmapPtr pixmap, Bool modifiers_ok)
{
    ScreenPtr screen = pixmap->drawable.pScreen;
    glamor_egl_priv_t *glamor_egl =
        glamor_egl_get_screen_private(screen);
    struct glamor_pixmap_private *pixmap_priv =
        glamor_get_pixmap_private(pixmap);
    unsigned width = pixmap->drawable.width;
    unsigned height = pixmap->drawable.height;
    uint32_t format;
    struct gbm_bo *bo = NULL;
    Bool used_modifiers = FALSE;
    PixmapPtr exported;
    GCPtr scratch_gc;

    BUG_RETURN_VAL(!pixmap_priv, FALSE);

    if (pixmap_priv->image &&
        (modifiers_ok || !pixmap_priv->used_modifiers))
        return TRUE;

    switch (pixmap->drawable.depth) {
    case 30:
        format = GBM_FORMAT_ARGB2101010;
        break;
    case 32:
    case 24:
        format = GBM_FORMAT_ARGB8888;
        break;
    case 16:
        format = GBM_FORMAT_RGB565;
        break;
    case 15:
        format = GBM_FORMAT_ARGB1555;
        break;
    case 8:
        format = GBM_FORMAT_R8;
        break;
    default:
        LogMessage(X_ERROR,
                   "Failed to make %d depth, %dbpp pixmap exportable\n",
                   pixmap->drawable.depth, pixmap->drawable.bitsPerPixel);
        return FALSE;
    }

#ifdef GBM_BO_WITH_MODIFIERS
    if (modifiers_ok && glamor_egl->dmabuf_capable) {
        uint32_t num_modifiers;
        uint64_t *modifiers = NULL;

        if (!glamor_get_modifiers(screen, format, &num_modifiers, &modifiers)) {
            return FALSE;
        }

        if (num_modifiers > 0) {
#ifdef GBM_BO_WITH_MODIFIERS2
            /* TODO: Is scanout ever used? If so, where? */
            bo = gbm_bo_create_with_modifiers2(glamor_egl->gbm, width, height,
                                               format, modifiers, num_modifiers,
                                               GBM_BO_USE_RENDERING | GBM_BO_USE_SCANOUT);
            if (!bo) {
                /* something failed, try again without GBM_BO_USE_SCANOUT */
                /* maybe scanout does work, but modifiers aren't supported */
                /* we handle this case on the fallback path */
                bo = gbm_bo_create_with_modifiers2(glamor_egl->gbm, width, height,
                                                   format, modifiers, num_modifiers,
                                                   GBM_BO_USE_RENDERING);
#if 0
                if (bo) {
                    /* TODO: scanout failed, but regular buffer succeeded, maybe log something? */
                }
#endif
            }
#else
            bo = gbm_bo_create_with_modifiers(glamor_egl->gbm, width, height,
                                              format, modifiers, num_modifiers);
#endif
        }
        if (bo)
            used_modifiers = TRUE;
        free(modifiers);
    }
#endif

    if (!bo)
    {
        /* TODO: Is scanout ever used? If so, where? */
        bo = gbm_bo_create(glamor_egl->gbm, width, height, format,
#ifdef GBM_BO_USE_LINEAR
                (pixmap->usage_hint == CREATE_PIXMAP_USAGE_SHARED ?
                 GBM_BO_USE_LINEAR : 0) |
#endif
                GBM_BO_USE_RENDERING | GBM_BO_USE_SCANOUT);
        if (!bo) {
            /* something failed, try again without GBM_BO_USE_SCANOUT */
            bo = gbm_bo_create(glamor_egl->gbm, width, height, format,
#ifdef GBM_BO_USE_LINEAR
                    (pixmap->usage_hint == CREATE_PIXMAP_USAGE_SHARED ?
                     GBM_BO_USE_LINEAR : 0) |
#endif
                     GBM_BO_USE_RENDERING);
#if 0
            if (bo) {
                /* TODO: scanout failed, but regular buffer succeeded, maybe log something? */
            }
#endif
        }
    }

    if (!bo) {
        LogMessage(X_ERROR,
                   "Failed to make %dx%dx%dbpp GBM bo\n",
                   width, height, pixmap->drawable.bitsPerPixel);
        return FALSE;
    }

    exported = screen->CreatePixmap(screen, 0, 0, pixmap->drawable.depth, 0);
    screen->ModifyPixmapHeader(exported, width, height, 0, 0,
                               gbm_bo_get_stride(bo), NULL);
    if (!glamor_egl_create_textured_pixmap_from_gbm_bo(exported, bo,
                                                       used_modifiers)) {
        LogMessage(X_ERROR,
                   "Failed to make %dx%dx%dbpp pixmap from GBM bo\n",
                   width, height, pixmap->drawable.bitsPerPixel);
        dixDestroyPixmap(exported, 0);
        gbm_bo_destroy(bo);
        return FALSE;
    }
    gbm_bo_destroy(bo);

    scratch_gc = GetScratchGC(pixmap->drawable.depth, screen);
    ValidateGC(&pixmap->drawable, scratch_gc);
    (void) scratch_gc->ops->CopyArea(&pixmap->drawable, &exported->drawable,
                              scratch_gc,
                              0, 0, width, height, 0, 0);
    FreeScratchGC(scratch_gc);

    /* Now, swap the tex/gbm/EGLImage/etc. of the exported pixmap into
     * the original pixmap struct.
     */
    glamor_egl_exchange_buffers(pixmap, exported);

    /* Swap the devKind into the original pixmap, reflecting the bo's stride */
    screen->ModifyPixmapHeader(pixmap, 0, 0, 0, 0, exported->devKind, NULL);

    dixDestroyPixmap(exported, 0);

    return TRUE;
}
#endif

#ifdef GLAMOR_HAS_GBM
static struct gbm_bo *
glamor_gbm_bo_from_pixmap_internal(ScreenPtr screen, PixmapPtr pixmap)
{
    glamor_egl_priv_t *glamor_egl =
        glamor_egl_get_screen_private(screen);
    struct glamor_pixmap_private *pixmap_priv =
        glamor_get_pixmap_private(pixmap);

    BUG_RETURN_VAL(!pixmap_priv, NULL);

    if (!pixmap_priv->image)
        return NULL;

    return gbm_bo_import(glamor_egl->gbm, GBM_BO_IMPORT_EGL_IMAGE,
                         pixmap_priv->image, GBM_BO_USE_RENDERING);
}
#endif

struct gbm_bo *
glamor_gbm_bo_from_pixmap(ScreenPtr screen, PixmapPtr pixmap)
{
#ifdef GLAMOR_HAS_GBM
    if (!glamor_make_pixmap_exportable(pixmap, TRUE))
        return NULL;

    return glamor_gbm_bo_from_pixmap_internal(screen, pixmap);
#else
    return NULL;
#endif
}

int
glamor_egl_fds_from_pixmap(ScreenPtr screen, PixmapPtr pixmap, int *fds,
                           uint32_t *strides, uint32_t *offsets,
                           uint64_t *modifier)
{
#if defined(GLAMOR_HAS_GBM) && defined (WITH_LIBDRM)
    struct gbm_bo *bo;
    int num_fds;
#ifdef GBM_BO_WITH_MODIFIERS
#ifndef GBM_BO_FD_FOR_PLANE
    int32_t first_handle;
#endif
    int i;
#endif

    if (!glamor_make_pixmap_exportable(pixmap, TRUE))
        return 0;

    bo = glamor_gbm_bo_from_pixmap_internal(screen, pixmap);
    if (!bo)
        return 0;

#ifdef GBM_BO_WITH_MODIFIERS
    num_fds = gbm_bo_get_plane_count(bo);
    for (i = 0; i < num_fds; i++) {
#ifdef GBM_BO_FD_FOR_PLANE
        fds[i] = gbm_bo_get_fd_for_plane(bo, i);
#else
        union gbm_bo_handle plane_handle = gbm_bo_get_handle_for_plane(bo, i);

        if (i == 0)
            first_handle = plane_handle.s32;

        /* If all planes point to the same object as the first plane, i.e. they
         * all have the same handle, we can fall back to the non-planar
         * gbm_bo_get_fd without losing information. If they point to different
         * objects we are out of luck and need to give up.
         */
	if (first_handle == plane_handle.s32)
            fds[i] = gbm_bo_get_fd(bo);
        else
            fds[i] = -1;
#endif
        if (fds[i] == -1) {
            while (--i >= 0)
                close(fds[i]);
            return 0;
        }
        strides[i] = gbm_bo_get_stride_for_plane(bo, i);
        offsets[i] = gbm_bo_get_offset(bo, i);
    }
    *modifier = gbm_bo_get_modifier(bo);
#else
    num_fds = 1;
    fds[0] = gbm_bo_get_fd(bo);
    if (fds[0] == -1)
        return 0;
    strides[0] = gbm_bo_get_stride(bo);
    offsets[0] = 0;
    *modifier = DRM_FORMAT_MOD_INVALID;
#endif

    gbm_bo_destroy(bo);
    return num_fds;
#else
    return 0;
#endif
}

int
glamor_egl_fd_from_pixmap(ScreenPtr screen, PixmapPtr pixmap,
                          CARD16 *stride, CARD32 *size)
{
#ifdef GLAMOR_HAS_GBM
    struct gbm_bo *bo;
    int fd;

    if (!glamor_make_pixmap_exportable(pixmap, FALSE))
        return -1;

    bo = glamor_gbm_bo_from_pixmap_internal(screen, pixmap);
    if (!bo)
        return -1;

    fd = gbm_bo_get_fd(bo);
    *stride = gbm_bo_get_stride(bo);
    *size = *stride * gbm_bo_get_height(bo);
    gbm_bo_destroy(bo);

    return fd;
#else
    return -1;
#endif
}

int
glamor_egl_fd_name_from_pixmap(ScreenPtr screen,
                               PixmapPtr pixmap,
                               CARD16 *stride, CARD32 *size)
{
#if defined(GLAMOR_HAS_GBM) && defined(WITH_LIBDRM)
    glamor_egl_priv_t *glamor_egl;
    struct gbm_bo *bo;
    int fd = -1;

    glamor_egl = glamor_egl_get_screen_private(screen);

    if (!glamor_make_pixmap_exportable(pixmap, FALSE))
        goto failure;

    bo = glamor_gbm_bo_from_pixmap_internal(screen, pixmap);
    if (!bo)
        goto failure;

    pixmap->devKind = gbm_bo_get_stride(bo);

    glamor_get_name_from_bo(glamor_egl->fd, bo, &fd);
    *stride = pixmap->devKind;
    *size = pixmap->devKind * gbm_bo_get_height(bo);

    gbm_bo_destroy(bo);
 failure:
    return fd;
#else
    return -1;
#endif
}

#ifdef GLAMOR_HAS_GBM
static bool
gbm_format_for_depth(CARD8 depth, uint32_t *format)
{
    switch (depth) {
    case 15:
        *format = GBM_FORMAT_ARGB1555;
        return true;
    case 16:
        *format = GBM_FORMAT_RGB565;
        return true;
    case 24:
        *format = GBM_FORMAT_XRGB8888;
        return true;
    case 30:
        *format = GBM_FORMAT_ARGB2101010;
        return true;
    case 32:
        *format = GBM_FORMAT_ARGB8888;
        return true;
    default:
        ErrorF("unexpected depth: %d\n", depth);
        return false;
    }
}
#endif

Bool
glamor_back_pixmap_from_fd(PixmapPtr pixmap,
                           int fd,
                           CARD16 width,
                           CARD16 height,
                           CARD16 stride, CARD8 depth, CARD8 bpp)
{
#ifdef GLAMOR_HAS_GBM
    ScreenPtr screen = pixmap->drawable.pScreen;
    glamor_egl_priv_t *glamor_egl;
    struct gbm_bo *bo;
    struct gbm_import_fd_data import_data = { 0 };
    Bool ret;

    glamor_egl = glamor_egl_get_screen_private(screen);

    if (!gbm_format_for_depth(depth, &import_data.format) ||
        width == 0 || height == 0)
        return FALSE;

    import_data.fd = fd;
    import_data.width = width;
    import_data.height = height;
    import_data.stride = stride;
    bo = gbm_bo_import(glamor_egl->gbm, GBM_BO_IMPORT_FD, &import_data,
                       GBM_BO_USE_RENDERING);
    if (!bo)
        return FALSE;

    screen->ModifyPixmapHeader(pixmap, width, height, 0, 0, stride, NULL);

    ret = glamor_egl_create_textured_pixmap_from_gbm_bo(pixmap, bo, FALSE);
    gbm_bo_destroy(bo);
    return ret;
#else
    return FALSE;
#endif
}

PixmapPtr
glamor_pixmap_from_fds(ScreenPtr screen,
                       CARD8 num_fds, const int *fds,
                       CARD16 width, CARD16 height,
                       const CARD32 *strides, const CARD32 *offsets,
                       CARD8 depth, CARD8 bpp,
                       uint64_t modifier)
{
#if defined(GLAMOR_HAS_GBM) && defined(WITH_LIBDRM)
    PixmapPtr pixmap;
    glamor_egl_priv_t *glamor_egl;
    Bool ret = FALSE;
    int i;

    glamor_egl = glamor_egl_get_screen_private(screen);

    pixmap = screen->CreatePixmap(screen, 0, 0, depth, 0);

#ifdef GBM_BO_WITH_MODIFIERS
    if (glamor_egl->dmabuf_capable && modifier != DRM_FORMAT_MOD_INVALID) {
        struct gbm_import_fd_modifier_data import_data = { 0 };
        struct gbm_bo *bo;

        if (!gbm_format_for_depth(depth, &import_data.format) ||
            width == 0 || height == 0)
            goto error;

        import_data.width = width;
        import_data.height = height;
        import_data.num_fds = num_fds;
        import_data.modifier = modifier;
        for (i = 0; i < num_fds; i++) {
            import_data.fds[i] = fds[i];
            import_data.strides[i] = strides[i];
            import_data.offsets[i] = offsets[i];
        }
        bo = gbm_bo_import(glamor_egl->gbm, GBM_BO_IMPORT_FD_MODIFIER, &import_data,
                           GBM_BO_USE_RENDERING);
        if (bo) {
            screen->ModifyPixmapHeader(pixmap, width, height, 0, 0, strides[0], NULL);
            ret = glamor_egl_create_textured_pixmap_from_gbm_bo(pixmap, bo, TRUE);
            gbm_bo_destroy(bo);
        }
    } else
#endif
    {
        if (num_fds == 1) {
            ret = glamor_back_pixmap_from_fd(pixmap, fds[0], width, height,
                                             strides[0], depth, bpp);
        }
    }

error:
    if (ret == FALSE) {
        dixDestroyPixmap(pixmap, 0);
        return NULL;
    }
    return pixmap;
#else
    return NULL;
#endif
}

PixmapPtr
glamor_pixmap_from_fd(ScreenPtr screen,
                      int fd,
                      CARD16 width,
                      CARD16 height,
                      CARD16 stride, CARD8 depth, CARD8 bpp)
{
#ifdef GLAMOR_HAS_GBM
    PixmapPtr pixmap;
    Bool ret;

    pixmap = screen->CreatePixmap(screen, 0, 0, depth, 0);

    ret = glamor_back_pixmap_from_fd(pixmap, fd, width, height,
                                     stride, depth, bpp);

    if (ret == FALSE) {
        dixDestroyPixmap(pixmap, 0);
        return NULL;
    }
    return pixmap;
#else
    return NULL;
#endif
}

static Bool
glamor_get_formats_internal(glamor_egl_priv_t *glamor_egl,
                            CARD32 *num_formats, CARD32 **formats)
{
#ifdef GLAMOR_HAS_EGL_QUERY_DMABUF
    EGLint num;
#else
    (void)glamor_egl;
#endif

    /* Explicitly zero the count and formats as the caller may ignore the return value */
    *num_formats = 0;
    *formats = NULL;
#ifdef GLAMOR_HAS_EGL_QUERY_DMABUF
    if (!glamor_egl->dmabuf_capable)
        return TRUE;

    if (!eglQueryDmaBufFormatsEXT(glamor_egl->display, 0, NULL, &num))
        return FALSE;

    if (num == 0)
        return TRUE;

    *formats = calloc(num, sizeof(CARD32));
    if (*formats == NULL)
        return FALSE;

    if (!eglQueryDmaBufFormatsEXT(glamor_egl->display, num,
                                  (EGLint *) *formats, &num)) {
        free(*formats);
        *formats = NULL;
        return FALSE;
    }

    *num_formats = num;
#endif
    return TRUE;
}

Bool
glamor_get_formats(ScreenPtr screen,
                   CARD32 *num_formats, CARD32 **formats)
{
    glamor_egl_priv_t *glamor_egl;
    glamor_egl = glamor_egl_get_screen_private(screen);
    return glamor_get_formats_internal(glamor_egl, num_formats, formats);
}

static void
glamor_filter_modifiers(uint32_t *num_modifiers, uint64_t **modifiers,
                        EGLBoolean *external_only, int linear_only)
{
    uint32_t write_pos = 0;
    for (uint32_t i = 0; i < *num_modifiers; i++) {
        if (external_only[i]) {
            continue;
        }

        if (linear_only &&
#ifdef WITH_LIBDRM
            ((*modifiers)[i] != DRM_FORMAT_MOD_LINEAR) &&
            ((*modifiers)[i] != DRM_FORMAT_MOD_INVALID))
#else
            (*modifiers)[i] != 0) /* DRM_FORMAT_MOD_LINEAR */
#endif
        {
            continue;
        }

        (*modifiers)[write_pos++] = (*modifiers)[i];
    }

    if (write_pos == 0) {
        *num_modifiers = 0;
        free(*modifiers);
        *modifiers = NULL;
    } else if (write_pos != *num_modifiers) {
        *num_modifiers = write_pos;
        uint64_t *filtered_modifiers = realloc(*modifiers, write_pos * sizeof(*modifiers));
        if (filtered_modifiers != NULL) {
            *modifiers = filtered_modifiers;
        }
    }
}

static Bool
glamor_get_modifiers_internal(glamor_egl_priv_t *glamor_egl, uint32_t format,
                              uint32_t *num_modifiers, uint64_t **modifiers)
{
#ifdef GLAMOR_HAS_EGL_QUERY_DMABUF
    EGLBoolean *external_only;
    EGLint num;
#else
    (void)glamor_egl;
#endif

    /* Explicitly zero the count and modifiers as the caller may ignore the return value */
    *num_modifiers = 0;
    *modifiers = NULL;
#ifdef GLAMOR_HAS_EGL_QUERY_DMABUF
    if (!glamor_egl->dmabuf_capable)
        return FALSE;

    if (!eglQueryDmaBufModifiersEXT(glamor_egl->display, format, 0, NULL,
                                    NULL, &num))
        return FALSE;

    if (num == 0)
        return TRUE;

    *modifiers = calloc(num, sizeof(uint64_t));
    if (*modifiers == NULL)
        return FALSE;

    external_only = calloc(num, sizeof(EGLBoolean));
    if (!external_only) {
        free(*modifiers);
        *modifiers = NULL;
        return FALSE;
    }

    if (!eglQueryDmaBufModifiersEXT(glamor_egl->display, format, num,
                                    (EGLuint64KHR *) *modifiers, external_only, &num)) {
        free(external_only);
        free(*modifiers);
        *modifiers = NULL;
        return FALSE;
    }

    *num_modifiers = num;
    glamor_filter_modifiers(num_modifiers, modifiers, external_only, glamor_egl->linear_only);
    free(external_only);


    if (num && *num_modifiers == 0) {
        /**
         * The api explicitly told us what the supported modifiers are,
         * but we can't use any of them for our purposes
         */
        return FALSE;
    }
#endif
    return TRUE;
}

Bool
glamor_get_modifiers(ScreenPtr screen, uint32_t format,
                     uint32_t *num_modifiers, uint64_t **modifiers)
{
    glamor_egl_priv_t *glamor_egl;
    glamor_egl = glamor_egl_get_screen_private(screen);
    return glamor_get_modifiers_internal(glamor_egl, format, num_modifiers, modifiers);
}

const char *
glamor_egl_get_driver_name(ScreenPtr screen)
{
#ifdef GLAMOR_HAS_EGL_QUERY_DRIVER
    glamor_egl_priv_t *glamor_egl;

    glamor_egl = glamor_egl_get_screen_private(screen);

    if (epoxy_has_egl_extension(glamor_egl->display, "EGL_MESA_query_driver"))
        return eglGetDisplayDriverName(glamor_egl->display);
#endif

    return NULL;
}

#ifdef GLAMOR_HAS_GBM
static void glamor_egl_pixmap_destroy(CallbackListPtr *pcbl, ScreenPtr pScreen, PixmapPtr pixmap)
{
    ScreenPtr screen = pixmap->drawable.pScreen;
    glamor_egl_priv_t *glamor_egl =
        glamor_egl_get_screen_private(screen);

    struct glamor_pixmap_private *pixmap_priv =
        glamor_get_pixmap_private(pixmap);

    BUG_RETURN(!pixmap_priv);

    if (pixmap_priv->image)
        eglDestroyImageKHR(glamor_egl->display, pixmap_priv->image);
}
#endif

void
glamor_egl_exchange_buffers(PixmapPtr front, PixmapPtr back)
{
#ifdef GLAMOR_HAS_GBM
    EGLImageKHR temp_img;
    Bool temp_mod;
    struct glamor_pixmap_private *front_priv =
        glamor_get_pixmap_private(front);
    struct glamor_pixmap_private *back_priv =
        glamor_get_pixmap_private(back);
#endif

    glamor_pixmap_exchange_fbos(front, back);

#ifdef GLAMOR_HAS_GBM
    temp_img = back_priv->image;
    temp_mod = back_priv->used_modifiers;
    BUG_RETURN(!back_priv);
    back_priv->image = front_priv->image;
    back_priv->used_modifiers = front_priv->used_modifiers;
    BUG_RETURN(!front_priv);
    front_priv->image = temp_img;
    front_priv->used_modifiers = temp_mod;
#endif

    glamor_set_pixmap_type(front, GLAMOR_TEXTURE_DRM);
    glamor_set_pixmap_type(back, GLAMOR_TEXTURE_DRM);
}

static void glamor_egl_pre_close_screen_cleanup(glamor_egl_priv_t *glamor_egl);

static void glamor_egl_close_screen(CallbackListPtr *pcbl, ScreenPtr screen, void *unused)
{
    glamor_egl_priv_t *glamor_egl;
#ifdef GLAMOR_HAS_GBM
    struct glamor_pixmap_private *pixmap_priv;
    PixmapPtr screen_pixmap;
#endif

    glamor_egl = glamor_egl_get_screen_private(screen);
#ifdef GLAMOR_HAS_GBM
    screen_pixmap = screen->GetScreenPixmap(screen);

    pixmap_priv = glamor_get_pixmap_private(screen_pixmap);
    BUG_RETURN(!pixmap_priv);

    eglDestroyImageKHR(glamor_egl->display, pixmap_priv->image);
    pixmap_priv->image = NULL;
#endif

    glamor_egl_pre_close_screen_cleanup(glamor_egl);

    dixScreenUnhookClose(screen, glamor_egl_close_screen);
#ifdef GLAMOR_HAS_GBM
    dixScreenUnhookPixmapDestroy(screen, glamor_egl_pixmap_destroy);
#endif
}

static void
glamor_egl_post_close_screen(CallbackListPtr *pcbl, ScreenPtr screen, void *unused)
{
#ifdef GLAMOR_HAS_GBM
    glamor_egl_priv_t *glamor_egl = glamor_egl_get_screen_private(screen);

    if (glamor_egl->gbm)
        gbm_device_destroy(glamor_egl->gbm);
#endif

    dixScreenUnhookPostClose(screen, glamor_egl_post_close_screen);
}

#ifdef DRI3
static int
glamor_dri3_open_client(ClientPtr client,
                        ScreenPtr screen,
                        RRProviderPtr provider,
                        int *fdp)
{
    glamor_egl_priv_t *glamor_egl =
        glamor_egl_get_screen_private(screen);
    int fd;
    drm_magic_t magic;

    fd = open(glamor_egl->device_path, O_RDWR|O_CLOEXEC);
    if (fd < 0)
        return BadAlloc;

    /* Before FD passing in the X protocol with DRI3 (and increased
     * security of rendering with per-process address spaces on the
     * GPU), the kernel had to come up with a way to have the server
     * decide which clients got to access the GPU, which was done by
     * each client getting a unique (magic) number from the kernel,
     * passing it to the server, and the server then telling the
     * kernel which clients were authenticated for using the device.
     *
     * Now that we have FD passing, the server can just set up the
     * authentication on its own and hand the prepared FD off to the
     * client.
     */
    if (drmGetMagic(fd, &magic) < 0) {
        if (errno == EACCES) {
            /* Assume that we're on a render node, and the fd is
             * already as authenticated as it should be.
             */
            *fdp = fd;
            return Success;
        } else {
            close(fd);
            return BadMatch;
        }
    }

    if (drmAuthMagic(glamor_egl->fd, magic) < 0) {
        close(fd);
        return BadMatch;
    }

    *fdp = fd;
    return Success;
}

static const dri3_screen_info_rec glamor_dri3_info = {
    .version = 2,
    .open_client = glamor_dri3_open_client,
    .pixmap_from_fds = glamor_pixmap_from_fds,
    .fd_from_pixmap = glamor_egl_fd_from_pixmap,
    .fds_from_pixmap = glamor_egl_fds_from_pixmap,
    .get_formats = glamor_get_formats,
    .get_modifiers = glamor_get_modifiers,
    .get_drawable_modifiers = glamor_get_drawable_modifiers,
};
#endif /* DRI3 */

static inline void
glamor_egl_set_glvnd_vendor(ScreenPtr screen)
{
    glamor_egl_priv_t *glamor_egl =
        glamor_egl_get_screen_private(screen);

    /* Should we make sure the vendor is valid? (nvidia, mesa, ???) */
    if (glamor_egl->exact_glvnd_vendor) {
        glamor_set_glvnd_vendor(screen, glamor_egl->glvnd_vendor);
        return;
    }

#ifdef GLAMOR_HAS_GBM
    if (glamor_egl->fd >= 0) {
        const char *gbm_backend_name;
        gbm_backend_name = gbm_device_get_backend_name(glamor_egl->gbm);
        if (gbm_backend_name) {
            if (!strncmp(gbm_backend_name, "nvidia", sizeof("nvidia") - 1)) {
                 glamor_set_glvnd_vendor(screen, "nvidia");
            } else {
                 glamor_set_glvnd_vendor(screen, "mesa");
            }
            return;
        }
    }
#endif

    const char *vendor = (const char*)glGetString(GL_VENDOR);
    const char *renderer = (const char*)glGetString(GL_RENDERER);

    if (!glamor_egl->glvnd_vendor) {
        if (renderer && strstr(renderer, "NVIDIA")) {
            glamor_set_glvnd_vendor(screen, "nvidia");
        } else if (vendor && strstr(vendor, "NVIDIA")) {
            glamor_set_glvnd_vendor(screen, "nvidia");
        } else {
            glamor_set_glvnd_vendor(screen, "mesa");
        }
    } else {
        if (strstr(glamor_egl->glvnd_vendor, "nvidia")) {
            glamor_set_glvnd_vendor(screen, "nvidia");
        } else {
            glamor_set_glvnd_vendor(screen, "mesa");
        }
    }
}

void
glamor_egl_screen_init(ScreenPtr screen, struct glamor_context *glamor_ctx)
{
    glamor_egl_priv_t *glamor_egl =
        glamor_egl_get_screen_private(screen);
#ifdef DRI3
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
#endif
#ifdef GLXEXT
    static Bool vendor_initialized = FALSE;
#endif

    dixScreenHookClose(screen, glamor_egl_close_screen);
    dixScreenHookPostClose(screen, glamor_egl_post_close_screen);
#ifdef GLAMOR_HAS_GBM
    dixScreenHookPixmapDestroy(screen, glamor_egl_pixmap_destroy);
#endif

    glamor_ctx->ctx = glamor_egl->context;
    glamor_ctx->display = glamor_egl->display;

    glamor_ctx->make_current = glamor_egl_make_current;

    glamor_egl_set_glvnd_vendor(screen);
#ifdef DRI3
    if (glamor_egl->fd >= 0) {
        /* Tell the core that we have the interfaces for import/export
         * of pixmaps.
         */
        glamor_enable_dri3(screen);

        /* If the driver wants to do its own auth dance (e.g. Xwayland
         * on pre-3.15 kernels that don't have render nodes and thus
         * has the wayland compositor as a master), then it needs us
         * to stay out of the way and let it init DRI3 on its own.
         */
        if (!(glamor_priv->flags & GLAMOR_NO_DRI3)) {
            /* To do DRI3 device FD generation, we need to open a new fd
             * to the same device we were handed in originally.
             */
            glamor_egl->device_path = drmGetRenderDeviceNameFromFd(glamor_egl->fd);
            if (!glamor_egl->device_path)
                glamor_egl->device_path = drmGetDeviceNameFromFd2(glamor_egl->fd);

            if (!dri3_screen_init(screen, &glamor_dri3_info)) {
                LogMessage(X_ERROR,
                           "Failed to initialize DRI3.\n");
            }
        }
    }
#endif
#ifdef GLXEXT
    if (!vendor_initialized) {
        GlxPushProvider(&glamor_provider);
        xorgGlxCreateVendor();
        vendor_initialized = TRUE;
    }
#endif
}

static Bool
glamor_query_devices_ext(EGLDeviceEXT **devices, EGLint *num_devices)
{
    EGLint max_devices = 0;

    *devices = NULL;
    *num_devices = 0;

    if (!epoxy_has_egl_extension(NULL, "EGL_EXT_device_base") &&
        !(epoxy_has_egl_extension(NULL, "EGL_EXT_device_query") &&
          epoxy_has_egl_extension(NULL, "EGL_EXT_device_enumeration"))) {
        return FALSE;
    }

    if (!eglQueryDevicesEXT(0, NULL, &max_devices) || max_devices < 1) {
         return FALSE;
    }

    *devices = calloc(max_devices, sizeof(**devices));
    if (*devices == NULL) {
         return FALSE;
    }

    if (!eglQueryDevicesEXT(max_devices, *devices, num_devices) || *num_devices < 1) {
         free(*devices);
         *devices = NULL;
         *num_devices = 0;
         return FALSE;
    }

    if (*num_devices < max_devices) {
         /* Shouldn't happen */
         void *tmp = realloc(*devices, *num_devices * sizeof(**devices));
         if (tmp) {
             *devices = tmp;
         }
    }

    return TRUE;
}

/* Perhaps we should move this function to os/ ? */
static inline Bool
glamor_egl_device_matches_fd(EGLDeviceEXT device, int fd)
{
    const char *dev_file = eglQueryDeviceStringEXT(device, EGL_DRM_DEVICE_FILE_EXT);
    if (!dev_file) {
        return FALSE;
    }

    int dev_fd = open(dev_file, O_RDWR);
    if (dev_fd < 0) {
        return FALSE;
    }

    /**
     * From https://pubs.opengroup.org/onlinepubs/009696699/basedefs/sys/stat.h.html
     *
     * The st_ino and st_dev fields taken together uniquely identify the file within the system.
     */
    struct stat stat1, stat2;
    if(fstat(dev_fd, &stat2) < 0) {
        close(dev_fd);
        return FALSE;
    }

    close(dev_fd);

    if(fstat(fd, &stat1) < 0) {
        return FALSE;
    }

    return (stat1.st_dev == stat2.st_dev) && (stat1.st_ino == stat2.st_ino);
}

static inline const char*
glamor_egl_device_get_name(EGLDeviceEXT device)
{
/**
 * For some reason, this isn't part of the epoxy headers.
 * It is part of EGL/eglext.h, but we can't include that
 * alongside the epoxy headers.
 *
 * See: https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_device_persistent_id.txt
 * for the spec where this is defined
 */
#ifndef EGL_DRIVER_NAME_EXT
#define EGL_DRIVER_NAME_EXT 0x335E
#endif

/**
 * Same for this one
 *
 * See: https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_device_query_name.txt
 * for the spec where this is defined
 */
#ifndef EGL_RENDERER_EXT
#define EGL_RENDERER_EXT 0x335F
#endif

    const char *dev_ext = eglQueryDeviceStringEXT(device, EGL_EXTENSIONS);

    const char *driver_name = epoxy_extension_in_string(dev_ext, "EGL_EXT_device_persistent_id") ?
                              eglQueryDeviceStringEXT(device, EGL_DRIVER_NAME_EXT) : NULL;

    if (driver_name) {
        return driver_name;
    }

    /* This might seem like overkill, but it's actually needed for the nvidia 470 driver */
    if (epoxy_extension_in_string(dev_ext, "EGL_EXT_device_query_name")) {
        const char *egl_renderer = eglQueryDeviceStringEXT(device, EGL_RENDERER_EXT);
        if (egl_renderer) {
            return strstr(egl_renderer, "NVIDIA") ? "nvidia" : "mesa";
        }
        const char *egl_vendor = eglQueryDeviceStringEXT(device, EGL_VENDOR);
        if (egl_vendor) {
            return strstr(egl_vendor, "NVIDIA") ? "nvidia" : "mesa";
        }
    }

    return NULL;
}

/**
 * Find the desired EGLDevice for our config.
 *
 * If strict == 2, we are looking for EGLDevices with names and,
 * if a glvnd vendor was passed, an exact match between the
 * device's name, and the desired vendor.
 *
 * If strict == 1, we are looking for EGLDevices with names and,
 * if a glvnd vendor was passed, a match between the gl vendor library
 * provider and the desired vendor's library.
 *
 * If strict == 0, we accept all devices, even those with no names.
 *
 * Regardless of success/failure, and regardless of strictness level,
 * we save the statically allocated string with the EGLDevice's name
 * in *driver_name, even if that name is NULL.
 */
static inline Bool
glamor_egl_device_matches_config(EGLDeviceEXT device,
                                 glamor_egl_priv_t *glamor_egl,
                                 int strict,
                                 const char** driver_name)
{
    *driver_name = glamor_egl_device_get_name(device);

    /**
     * If we're trying to do direct rendering,
     * we can't have a mismatch between the gpu and the device we pick
     *
     * If not, we don't have any strict requirements for out device
     */
    if (glamor_egl->fd >= 0 &&
        !glamor_egl_device_matches_fd(device, glamor_egl->fd)) {
        return FALSE;
    }

    /* We have no further requirements, mark this as valid */
    if (strict <= 0) {
        return TRUE;
    }

    /* From here on, strict >= 1, we want the device to have a name */
    if (*driver_name == NULL) {
        return FALSE;
    }

    /* No glvnd vendor was requested, we have no further requirements */
    if (!glamor_egl->glvnd_vendor) {
        return TRUE;
    }

    /**
     * A glvnd vendor was requested.
     * Check for an exact match between the driver name and the requested
     * vendor.
     *
     * We're looking for _driver_ names, not library names here.
     * If we find an exact match, that's the most we ask.
     */
    if (!strcmp(*driver_name, glamor_egl->glvnd_vendor)) {
        return TRUE;
    }

    /* We don't have an exact driver name match, reject this device is strict == 2 */
    if (strict >= 2) {
        return FALSE;
    }

    /**
     * Here, strict == 1
     * We're looking for a glvnd library name match.
     *
     * This is not specific to nvidia,
     * but I don't know of any gl library vendors
     * other than mesa and nvidia
     */
    Bool device_is_nvidia = !!strstr(*driver_name, "nvidia");
    Bool config_is_nvidia = !!strstr(glamor_egl->glvnd_vendor, "nvidia");

    return device_is_nvidia == config_is_nvidia;
}

static void
glamor_egl_pre_close_screen_cleanup(glamor_egl_priv_t *glamor_egl)
{
    if (!glamor_egl) {
        return;
    }

    if (glamor_egl->display != EGL_NO_DISPLAY) {
        if (glamor_egl->context != EGL_NO_CONTEXT) {
            eglDestroyContext(glamor_egl->display, glamor_egl->context);
            glamor_egl->context = EGL_NO_CONTEXT;
        }

        eglMakeCurrent(glamor_egl->display,
                       EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        /*
         * Force the next glamor_make_current call to update the context
         * (on hot unplug another GPU may still be using glamor)
         */
        lastGLContext = NULL;
        eglTerminate(glamor_egl->display);
        glamor_egl->display = EGL_NO_DISPLAY;
    }

    free(glamor_egl->device_path);
    free(glamor_egl->glvnd_vendor);
}

void glamor_egl_cleanup(glamor_egl_priv_t *glamor_egl)
{
    glamor_egl_pre_close_screen_cleanup(glamor_egl);

#ifdef GLAMOR_HAS_GBM
    if (glamor_egl->gbm)
        gbm_device_destroy(glamor_egl->gbm);
#endif
}

static Bool
glamor_egl_try_big_gl_api(glamor_egl_priv_t *glamor_egl)
{
    if (eglBindAPI(EGL_OPENGL_API)) {
        static const EGLint config_attribs_core[] = {
            EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,
            EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
            EGL_CONTEXT_MAJOR_VERSION_KHR,
            GLAMOR_GL_CORE_VER_MAJOR,
            EGL_CONTEXT_MINOR_VERSION_KHR,
            GLAMOR_GL_CORE_VER_MINOR,
            EGL_NONE
        };
        static const EGLint config_attribs[] = {
            EGL_NONE
        };

        glamor_egl->context = eglCreateContext(glamor_egl->display,
                                               EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT,
                                               config_attribs_core);

        if (glamor_egl->context == EGL_NO_CONTEXT)
            glamor_egl->context = eglCreateContext(glamor_egl->display,
                                                   EGL_NO_CONFIG_KHR,
                                                   EGL_NO_CONTEXT,
                                                   config_attribs);
    }

    if (glamor_egl->context != EGL_NO_CONTEXT) {
        if (!eglMakeCurrent(glamor_egl->display,
                            EGL_NO_SURFACE, EGL_NO_SURFACE, glamor_egl->context)) {
            LogMessage(X_ERROR,
                       "Failed to make GL context current\n");
            return FALSE;
        }

        if (epoxy_gl_version() < 21) {
            LogMessage(X_INFO,
                       "glamor: Ignoring GL < 2.1, falling back to GLES.\n");
            eglDestroyContext(glamor_egl->display, glamor_egl->context);
            glamor_egl->context = EGL_NO_CONTEXT;
        }
        LogMessage(X_INFO,
            "glamor: Using OpenGL %d.%d context.\n",
            epoxy_gl_version() / 10,
            epoxy_gl_version() % 10);
    }
    return TRUE;
}

static Bool
glamor_egl_try_gles_api(glamor_egl_priv_t *glamor_egl)
{
    static const EGLint config_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        LogMessage(X_ERROR,
                    "glamor: Failed to bind GLES API.\n");
        return FALSE;
    }

    glamor_egl->context = eglCreateContext(glamor_egl->display,
                                            EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT,
                                            config_attribs);

    if (glamor_egl->context != EGL_NO_CONTEXT) {
        if (!eglMakeCurrent(glamor_egl->display,
                            EGL_NO_SURFACE, EGL_NO_SURFACE, glamor_egl->context)) {
            LogMessage(X_ERROR,
                       "Failed to make GLES context current\n");
            return FALSE;
        }
        LogMessage(X_INFO,
                "glamor: Using OpenGL ES %d.%d context.\n",
                epoxy_gl_version() / 10,
                epoxy_gl_version() % 10);
    }
    return TRUE;
}

#ifdef GLAMOR_HAS_GBM
static inline struct gbm_device*
gbm_create_device_by_name(int fd, const char* name)
{
    struct gbm_device* ret = NULL;
    const char* old_backend = getenv("GBM_BACKEND");
    setenv("GBM_BACKEND", name, 1);
    ret = gbm_create_device(fd);
    unsetenv("GBM_BACKEND");
    if (old_backend) {
        setenv("GBM_BACKEND", old_backend, 1);
    }
    return ret;
}
#endif

static Bool
glamor_egl_init_display(struct glamor_egl_screen_private *glamor_egl)
{
    EGLDeviceEXT *devices = NULL;
    EGLint num_devices = 0;
    const char *driver_name = NULL;
    /**
     * If the user didn't give us a GL driver/library name,
     * we populate it with what we queried
     */
#define GLAMOR_EGL_TRY_PLATFORM(platform, native, platform_fallback) \
    glamor_egl->display = glamor_egl_get_display2(platform, native, platform_fallback); \
    if (glamor_egl->display == EGL_NO_DISPLAY) { \
        LogMessage(X_ERROR, "eglGetDisplay(" #platform ", " #native ") failed\n"); \
    } else { \
        if (eglInitialize(glamor_egl->display, NULL, NULL)) { \
            if (!glamor_egl->glvnd_vendor && driver_name) { \
                glamor_egl->glvnd_vendor = strdup(driver_name); \
            } \
            LogMessage(X_INFO, "eglInitialize() succeeded on " #platform "\n"); \
            free(devices); \
            return TRUE; \
        } \
        LogMessage(X_ERROR, "eglInitialize() failed on " #platform "\n"); \
        eglTerminate(glamor_egl->display); \
        glamor_egl->display = EGL_NO_DISPLAY; \
    }

#ifdef GLAMOR_HAS_GBM
    if (glamor_egl->fd >= 0) {
        GLAMOR_EGL_TRY_PLATFORM(EGL_PLATFORM_GBM_KHR, glamor_egl->gbm, FALSE);
        GLAMOR_EGL_TRY_PLATFORM(EGL_PLATFORM_GBM_MESA, glamor_egl->gbm, TRUE);
    }
#endif

    if (glamor_query_devices_ext(&devices, &num_devices)) {
#define GLAMOR_EGL_TRY_PLATFORM_DEVICE(strict) \
        for (uint32_t i = 0; i < num_devices; i++) { \
            if (glamor_egl_device_matches_config(devices[i], glamor_egl, strict, &driver_name)) { \
                GLAMOR_EGL_TRY_PLATFORM(EGL_PLATFORM_DEVICE_EXT, devices[i], TRUE); \
            } \
        }

        GLAMOR_EGL_TRY_PLATFORM_DEVICE(2);
        GLAMOR_EGL_TRY_PLATFORM_DEVICE(1);
        GLAMOR_EGL_TRY_PLATFORM_DEVICE(0);

#undef GLAMOR_EGL_TRY_PLATFORM_DEVICE
    }
    driver_name = NULL;

    /**
     * We only try these falbacks if we don't have an fd passed, since we
     * have to do some guessing anyway to find the desired gpu.
     *
     * Trying these in multi-card setups risks a screen driven by one card
     * being mapped a, EGLDisplay backed by a different card, which can break.
     *
     * We actualy can specify the device using EGL_EXT_explicit_device:
     * https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_explicit_device.txt
     *
     * However, it doesn't seem worth it to implement this fallback, given
     * we're already trying the device platform, and the extension is
     * relatively new (2022), which means that it will be missing on a lot of cards.
     */
    if (glamor_egl->fd < 0) {
        GLAMOR_EGL_TRY_PLATFORM(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, FALSE);

        /**
         * From https://registry.khronos.org/EGL/extensions/KHR/EGL_KHR_platform_gbm.txt
         *
         * If <native_display> is EGL_DEFAULT_DISPLAY,
         * then the resultant EGLDisplay will be backed by some
         * implementation-chosen GBM device.
         */
        GLAMOR_EGL_TRY_PLATFORM(EGL_PLATFORM_GBM_KHR, EGL_DEFAULT_DISPLAY, FALSE);
        GLAMOR_EGL_TRY_PLATFORM(EGL_PLATFORM_GBM_MESA, EGL_DEFAULT_DISPLAY, FALSE);

        /**
         * According to https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_platform_device.txt :
         *
         * When <platform> is EGL_PLATFORM_DEVICE_EXT, <native_display> must
         * be an EGLDeviceEXT object.  Platform-specific extensions may
         * define other valid values for <platform>.
         *
         * As far as I know, this is the relevant standard, and it has not been superceeded in this regard.
         * However, some vendors do allow passing EGL_DEFAULT_DISPLAY as the <native_display> argument.
         * So, while this is incorrect according to the standard, it doesn't hurt, and it actually does
         * something with some vendors (notably intel from my testing).
         */
        GLAMOR_EGL_TRY_PLATFORM(EGL_PLATFORM_DEVICE_EXT, EGL_DEFAULT_DISPLAY, TRUE);
    }

#undef GLAMOR_EGL_TRY_PLATFORM

    free(devices);
    return FALSE;
}

Bool
glamor_egl_init_internal(glamor_egl_priv_t* glamor_egl, Bool *compat_ret)
{
    const GLubyte *renderer;

    if (compat_ret) {
        *compat_ret = TRUE;
    }

    glamor_egl_get_screen_private = glamor_egl->GLAMOR_EGL_PRIV_PROC;

#ifdef GLAMOR_HAS_GBM
    if (glamor_egl->fd >= 0) {
        glamor_egl->gbm = gbm_create_device(glamor_egl->fd);
        if (!glamor_egl->gbm) {
            glamor_egl->gbm = gbm_create_device_by_name(glamor_egl->fd, "dumb");
        }

        if (glamor_egl->gbm == NULL) {
            ErrorF("couldn't create gbm device\n");
            glamor_egl->fd = -1;
            if (compat_ret) {
                *compat_ret = FALSE;
            }
        }

        const char* gbm_backend = glamor_egl->gbm ?
                                  gbm_device_get_backend_name(glamor_egl->gbm) : NULL;
        if (gbm_backend && !strcmp(gbm_backend, "dumb")) {
            glamor_egl->linear_only = TRUE;
        }
    }
#endif

    if (!glamor_egl_init_display(glamor_egl)) {
        goto error;
    }

#define GLAMOR_CHECK_EGL_EXTENSION(EXT)  \
	if (!epoxy_has_egl_extension(glamor_egl->display, "EGL_" #EXT)) {  \
		ErrorF("EGL_" #EXT " required.\n");  \
		goto error;  \
	}

#define GLAMOR_CHECK_EGL_EXTENSIONS(EXT1, EXT2)	 \
	if (!epoxy_has_egl_extension(glamor_egl->display, "EGL_" #EXT1) &&  \
	    !epoxy_has_egl_extension(glamor_egl->display, "EGL_" #EXT2)) {  \
		ErrorF("EGL_" #EXT1 " or EGL_" #EXT2 " required.\n");  \
		goto error;  \
	}

    GLAMOR_CHECK_EGL_EXTENSION(KHR_surfaceless_context);
    GLAMOR_CHECK_EGL_EXTENSION(KHR_no_config_context);

    if (!glamor_egl->force_es) {
        if(!glamor_egl_try_big_gl_api(glamor_egl))
            goto error;
    }

    if (glamor_egl->context == EGL_NO_CONTEXT && !glamor_egl->es_disallowed) {
        if(!glamor_egl_try_gles_api(glamor_egl))
            goto error;
    }

    if (glamor_egl->context == EGL_NO_CONTEXT) {
        LogMessage(X_ERROR,
                    "glamor: Failed to create GL or GLES2 contexts\n");
        goto error;
    }

    renderer = glGetString(GL_RENDERER);
    if (!renderer) {
        LogMessage(X_ERROR,
                   "glGetString() returned NULL, your GL is broken\n");
        goto error;
    }
    if (strstr((const char *)renderer, "softpipe")) {
        LogMessage(X_INFO,
                   "Refusing to try glamor on softpipe\n");
        goto error;
    }
    if (!strncmp("llvmpipe", (const char *)renderer, strlen("llvmpipe"))) {
        if (glamor_egl->llvmpipe_allowed)
            LogMessage(X_INFO,
                       "Allowing glamor on llvmpipe for PRIME\n");
        else {
            LogMessage(X_INFO,
                       "Refusing to try glamor on llvmpipe\n");
            goto error;
        }
    }

    /*
     * Force the next glamor_make_current call to set the right context
     * (in case of multiple GPUs using glamor)
     */
    lastGLContext = NULL;

    if (!epoxy_has_gl_extension("GL_OES_EGL_image")) {
        LogMessage(X_ERROR,
                   "glamor acceleration requires GL_OES_EGL_image\n");
        goto error;
    }

    LogMessage(X_INFO, "glamor X acceleration enabled on %s\n",
               renderer);

#ifdef GBM_BO_WITH_MODIFIERS
    if (epoxy_has_egl_extension(glamor_egl->display,
                                "EGL_EXT_image_dma_buf_import") &&
        epoxy_has_egl_extension(glamor_egl->display,
                                "EGL_EXT_image_dma_buf_import_modifiers")) {

        if (strstr((const char *)renderer, "Intel"))
            glamor_egl->dmabuf_capable = TRUE;
        else if (strstr((const char *)renderer, "zink"))
            glamor_egl->dmabuf_capable = TRUE;
        else if (strstr((const char *)renderer, "NVIDIA"))
            glamor_egl->dmabuf_capable = TRUE;
        else if (strstr((const char *)renderer, "radeonsi"))
            glamor_egl->dmabuf_capable = TRUE;
        else
            glamor_egl->dmabuf_capable = (glamor_egl->dmabuf_capable == TRUE);
    }
#endif

    /* Check if at least one combination of format + modifier is supported */
    CARD32 *formats = NULL;
    CARD32 num_formats = 0;
    Bool found = FALSE;
    if (!glamor_get_formats_internal(glamor_egl, &num_formats, &formats)) {
        goto error;
    }

    if (num_formats == 0) {
        return TRUE;
    }

    for (uint32_t i = 0; i < num_formats; i++) {
        uint64_t *modifiers = NULL;
        uint32_t num_modifiers = 0;
        if (glamor_get_modifiers_internal(glamor_egl, formats[i],
                                          &num_modifiers, &modifiers)) {
            found = TRUE;
            free(modifiers);
            break;
        }
    }
    free(formats);

    if (!found) {
        LogMessage(X_ERROR,
                   "glamor: No combination of format + modifier is supported\n");
        goto error;
    }

    return TRUE;

error:
    glamor_egl_cleanup(glamor_egl);
    return FALSE;
}
