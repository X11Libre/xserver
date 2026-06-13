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

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef HAVE_SYS_SYSMACROS_H
#include <sys/sysmacros.h> /* for major() & minor() */
#endif
#ifdef HAVE_SYS_MKDEV_H
#include <sys/mkdev.h>          /* for major() & minor() on Solaris */
#endif

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
#include "glamor_egl_priv.h"
#include "glamor_glx_provider.h"
#include "dri3.h"

#ifndef GBM_MAX_PLANES
#define GBM_MAX_PLANES 4
#endif

/**
 * EGLDeviceEXT's are internally stored as globals.
 * As such, when multiple screens query the same device,
 * they end up with the same exact pointer value for the device.
 *
 * Then, per the spec, eglGetPlatformDisplayEXT returns the
 * same EGLDisplay handle.
 *
 * This is a problem, because on teardown, each screen
 * destroys its EGLDisplay, and since it can be shared by
 * multiple screens, we risk destroying the display from under it.
 *
 * See: https://github.com/X11Libre/xserver/pull/2721
 */

typedef struct _usedDisplayList{
    EGLDisplay dpy;
    struct _usedDisplayList *next;
} UsedDisplayList;

static UsedDisplayList *usedDisplayList = NULL;

static void
glamor_egl_add_display_to_list(EGLDisplay dpy)
{
    UsedDisplayList *new;
    if (dpy == EGL_NO_DISPLAY) {
        return;
    }

    new = XNFalloc(sizeof(*new));
    new->dpy = dpy;
    new->next = usedDisplayList;
    usedDisplayList = new;
}

static void
glamor_egl_destroy_display(EGLDisplay dpy)
{
    UsedDisplayList **ptr = &usedDisplayList;
    void *free_me;

    if (dpy == EGL_NO_DISPLAY) {
        return;
    }

    for (; *ptr && ((*ptr)->dpy != dpy); ptr = &(*ptr)->next) {}
    if (*ptr == NULL) {
        LogMessage(X_ERROR, "glamor: EGLDisplay: %p not in usedlist\n", dpy);
        LogMessage(X_ERROR, "glamor: This is an X server bug, please report it\n");
        return;
    }

    /* Remove the display from the list */
    free_me = *ptr;
    *ptr = (*ptr)->next;
    free(free_me);

    /* Check if the display is still in use */
    for (; *ptr && ((*ptr)->dpy != dpy); ptr = &(*ptr)->next) {}
    if (*ptr == NULL) {
        eglTerminate(dpy);
    }
}

static DevPrivateKeyRec glamor_egl_screen_private_key;

static inline Bool
glamor_egl_init_screen_private(ScreenPtr screen)
{
    if (!dixRegisterPrivateKey(&glamor_egl_screen_private_key, PRIVATE_SCREEN, sizeof(glamor_egl_priv_t))) {
        LogMessage(X_ERROR,
                   "glamor%d: Failed to allocate screen private\n",
                   screen->myNum);
        return FALSE;
    }

    return TRUE;
}

static glamor_egl_priv_t*
_glamor_egl_get_screen_private(ScreenPtr screen)
{
    return dixLookupPrivate(&screen->devPrivates, &glamor_egl_screen_private_key);
}

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
(*glamor_egl_get_screen_private)(ScreenPtr screen) = _glamor_egl_get_screen_private;

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

static Bool
glamor_create_texture_from_image(ScreenPtr screen,
                                 EGLImageKHR image, GLuint * texture)
{
    struct glamor_screen_private *glamor_priv =
        glamor_get_screen_private(screen);

    glamor_egl_priv_t *glamor_egl =
        glamor_egl_get_screen_private(screen);

    glamor_make_current(glamor_priv);

    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    if (glamor_egl->has_OES_EGL_image) {
        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);
    } else if (glamor_egl->has_EXT_EGL_image_storage) {
        glEGLImageTargetTexStorageEXT(GL_TEXTURE_2D, image, NULL);
    } else {
        glDeleteTextures(1, texture);
        glBindTexture(GL_TEXTURE_2D, 0);
        return FALSE;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    return TRUE;
}

struct gbm_device *
glamor_egl_get_gbm_device(ScreenPtr screen)
{
#ifdef GLAMOR_HAS_GBM
    return glamor_egl_get_screen_private(screen)->gbm;
#else
    return NULL;
#endif
}

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
        ScreenPtr screen = pixmap->drawable.pScreen;
        glamor_egl_priv_t *glamor_egl = glamor_egl_get_screen_private(screen);

        eglDestroyImageKHR(glamor_egl->display, old);
    }
    pixmap_priv->image = image;
    pixmap_priv->used_modifiers = used_modifiers;
}

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

static EGLImageKHR
glamor_egl_image_from_dma_bufs(ScreenPtr screen,
                               uint32_t num_fds, const int *fds,
                               int width, int height,
                               const int *strides, const int *offsets,
                               uint32_t format, uint64_t modifier)
{
    struct glamor_screen_private *glamor_priv =
        glamor_get_screen_private(screen);
    glamor_egl_priv_t *glamor_egl =
        glamor_egl_get_screen_private(screen);

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

    glamor_make_current(glamor_priv);

#define ADD_ATTR(attrs, num, attr)                                      \
    do {                                                            \
        assert(((num) + 1) < (sizeof(attrs) / sizeof((attrs)[0]))); \
        (attrs)[(num)++] = (attr);                                  \
    } while (0)
    ADD_ATTR(img_attrs, attr_num, EGL_WIDTH);
    ADD_ATTR(img_attrs, attr_num, width);
    ADD_ATTR(img_attrs, attr_num, EGL_HEIGHT);
    ADD_ATTR(img_attrs, attr_num, height);
    ADD_ATTR(img_attrs, attr_num, EGL_LINUX_DRM_FOURCC_EXT);
    ADD_ATTR(img_attrs, attr_num, format);

    for (plane = 0; plane < num_fds; plane++) {
        ADD_ATTR(img_attrs, attr_num, planeAttrs[plane][PLANE_FD]);
        ADD_ATTR(img_attrs, attr_num, fds[plane]);
        ADD_ATTR(img_attrs, attr_num, planeAttrs[plane][PLANE_OFFSET]);
        ADD_ATTR(img_attrs, attr_num, offsets[plane]);
        ADD_ATTR(img_attrs, attr_num, planeAttrs[plane][PLANE_PITCH]);
        ADD_ATTR(img_attrs, attr_num, strides[plane]);
        ADD_ATTR(img_attrs, attr_num, planeAttrs[plane][PLANE_MODIFIER_LO]);
        ADD_ATTR(img_attrs, attr_num, (uint32_t)(modifier & 0xFFFFFFFFULL));
        ADD_ATTR(img_attrs, attr_num, planeAttrs[plane][PLANE_MODIFIER_HI]);
        ADD_ATTR(img_attrs, attr_num, (uint32_t)(modifier >> 32ULL));
    }
    ADD_ATTR(img_attrs, attr_num, EGL_NONE);
#undef ADD_ATTR

    return eglCreateImageKHR(glamor_egl->display,
                             EGL_NO_CONTEXT,
                             EGL_LINUX_DMA_BUF_EXT,
                             NULL,
                             img_attrs);
}

#ifdef GLAMOR_HAS_EXPORT_DMABUF_MESA
static EGLImageKHR
glamor_egl_image_from_pixmap(PixmapPtr pixmap)
{
    struct glamor_pixmap_private *pixmap_priv =
        glamor_get_pixmap_private(pixmap);
    ScreenPtr screen = pixmap->drawable.pScreen;
    struct glamor_screen_private *glamor_priv =
        glamor_get_screen_private(screen);
    glamor_egl_priv_t *glamor_egl =
        glamor_egl_get_screen_private(screen);

    if (!glamor_pixmap_ensure_fbo(pixmap, 0)) {
        return EGL_NO_IMAGE_KHR;
    }

    GLuint texture = pixmap_priv->fbo->tex;

    glamor_make_current(glamor_priv);

    return eglCreateImageKHR(glamor_egl->display,
                             glamor_egl->context,
                             EGL_GL_TEXTURE_2D,
                             (EGLClientBuffer)(uintptr_t)texture,
                             NULL);
}

static Bool
glamor_egl_make_pixmap_exportable2(PixmapPtr pixmap, Bool used_modifiers)
{
    struct glamor_pixmap_private *pixmap_priv =
        glamor_get_pixmap_private(pixmap);

    BUG_RETURN_VAL(!pixmap_priv, FALSE);

    if (pixmap_priv->image != EGL_NO_IMAGE_KHR) {
        return TRUE;
    }

    EGLImageKHR image = glamor_egl_image_from_pixmap(pixmap);

    if (image == EGL_NO_IMAGE_KHR) {
        glamor_set_pixmap_type(pixmap, GLAMOR_DRM_ONLY);
        return FALSE;
    }

    glamor_set_pixmap_type(pixmap, GLAMOR_TEXTURE_DRM);
    glamor_egl_set_pixmap_image(pixmap, image, used_modifiers);
    return TRUE;
}
#endif

#ifdef GLAMOR_HAS_GBM
static EGLImageKHR
glamor_egl_image_from_gbm_bo(ScreenPtr screen, struct gbm_bo *bo)
{
    glamor_egl_priv_t *glamor_egl =
        glamor_egl_get_screen_private(screen);

    Bool tried_fast_import = FALSE;

    if (glamor_egl->fast_gbm_import) {
        EGLImageKHR image;
        image = eglCreateImageKHR(glamor_egl->display,
                                  EGL_NO_CONTEXT,
                                  EGL_NATIVE_PIXMAP_KHR, bo, NULL);
        if (image != EGL_NO_IMAGE_KHR) {
            return image;
        }
        tried_fast_import = TRUE;
    }

    if (glamor_egl->dmabuf_capable) {
        EGLImageKHR image;
        uint64_t modifier = gbm_bo_get_modifier(bo);
        uint32_t num_planes = gbm_bo_get_plane_count(bo);
        uint32_t format = gbm_bo_get_format(bo);
        int fds[GBM_MAX_PLANES];
        int strides[GBM_MAX_PLANES];
        int offsets[GBM_MAX_PLANES];

        uint32_t width = gbm_bo_get_width(bo);
        uint32_t height = gbm_bo_get_height(bo);

#ifdef GBM_BO_FD_FOR_PLANE
        if (num_planes > GBM_MAX_PLANES) {
            goto fallback;
        }
        for (int plane = 0; plane < num_planes; plane++) {
            fds[plane] = gbm_bo_get_fd_for_plane(bo, plane);
            offsets[plane] = gbm_bo_get_offset(bo, plane);
            strides[plane] = gbm_bo_get_stride_for_plane(bo, plane);
        }
#else
#ifdef GBM_BO_FD
        num_planes = 1;
        fds[0] = gbm_bo_get_fd(bo);
        offsets[0] = 0;
        strides[0] = gbm_bo_get_stride(bo);
#else
        goto fallback;
#endif
#endif

        /* -Werror=maybe-uninitialized */
        memset(fds + num_planes, -1, (GBM_MAX_PLANES - num_planes) * sizeof(*fds));
        memset(offsets + num_planes, 0, (GBM_MAX_PLANES - num_planes) * sizeof(*offsets));
        memset(strides + num_planes, 0, (GBM_MAX_PLANES - num_planes) * sizeof(*strides));

        image = glamor_egl_image_from_dma_bufs(screen,
                                               num_planes, fds,
                                               width, height,
                                               strides, offsets,
                                               format, modifier);

        for (int plane = 0; plane < num_planes; plane++) {
            close(fds[plane]);
            fds[plane] = -1;
        }

        if (image != EGL_NO_IMAGE_KHR) {
            glamor_egl->fast_gbm_import = FALSE;
            return image;
        }
    }

#if defined(GBM_BO_FD_FOR_PLANE) || !defined(GBM_BO_FD) /* -Werror=unused-label */
fallback:
#endif
    if (tried_fast_import) {
        return EGL_NO_IMAGE_KHR;
    }

    return eglCreateImageKHR(glamor_egl->display,
                             EGL_NO_CONTEXT,
                             EGL_NATIVE_PIXMAP_KHR, bo, NULL);
}
#endif

static Bool
glamor_egl_create_textured_pixmap_from_egl_image(PixmapPtr pixmap,
                                                 EGLImageKHR image,
                                                 Bool used_modifiers)
{
    ScreenPtr screen = pixmap->drawable.pScreen;
    GLuint texture;

    if (image == EGL_NO_IMAGE_KHR ||
        !glamor_create_texture_from_image(screen, image, &texture)) {
        glamor_set_pixmap_type(pixmap, GLAMOR_DRM_ONLY);
        return FALSE;
    }

    glamor_set_pixmap_type(pixmap, GLAMOR_TEXTURE_DRM);
    glamor_set_pixmap_texture(pixmap, texture);
    glamor_egl_set_pixmap_image(pixmap, image, used_modifiers);
    return TRUE;
}

#ifdef WITH_LIBDRM
/**
 * XXX We only need libdrm for fourcc's here XXX
 *
 * Perhaps we should have some compatibility defines somewhere?
 */
static Bool
glamor_egl_create_textured_pixmap_from_dma_bufs(PixmapPtr pixmap,
                                                uint32_t num_fds, const int *fds,
                                                int width, int height,
                                                const int *strides, const int *offsets,
                                                uint32_t format, uint64_t modifier)
{
    ScreenPtr screen = pixmap->drawable.pScreen;
    Bool used_modifiers = modifier != DRM_FORMAT_MOD_INVALID;

    EGLImageKHR image = glamor_egl_image_from_dma_bufs(screen,
                                                       num_fds, fds,
                                                       width, height,
                                                       strides, offsets,
                                                       format, modifier);

    return glamor_egl_create_textured_pixmap_from_egl_image(pixmap, image,
                                                            used_modifiers);
}
#endif

Bool
glamor_egl_create_textured_pixmap_from_gbm_bo(PixmapPtr pixmap,
                                              struct gbm_bo *bo,
                                              Bool used_modifiers)
{
#ifdef GLAMOR_HAS_GBM
    ScreenPtr screen = pixmap->drawable.pScreen;
    EGLImageKHR image = glamor_egl_image_from_gbm_bo(screen, bo);

    return glamor_egl_create_textured_pixmap_from_egl_image(pixmap, image,
                                                            used_modifiers);
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

    if (!glamor_egl->gbm || !glamor_egl->can_texture_gbm_bo) {
        return FALSE;
    }

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
    struct gbm_bo* ret = NULL;

    glamor_egl_priv_t *glamor_egl =
        glamor_egl_get_screen_private(screen);
    struct glamor_pixmap_private *pixmap_priv =
        glamor_get_pixmap_private(pixmap);

    BUG_RETURN_VAL(!pixmap_priv, NULL);

    if (!pixmap_priv->image)
        return NULL;

    if (!glamor_egl->gbm) {
        return NULL;
    }

    ret = gbm_bo_import(glamor_egl->gbm, GBM_BO_IMPORT_EGL_IMAGE,
                        pixmap_priv->image, GBM_BO_USE_RENDERING);

#ifdef GLAMOR_HAS_EXPORT_DMABUF_MESA
    if (ret || !glamor_egl->has_image_dma_buf_export) {
        return ret;
    }

    int fourcc = 0;
    int num_planes = 0;
    EGLuint64KHR modifiers[GBM_MAX_PLANES] = {0};
    if (!eglExportDMABUFImageQueryMESA(glamor_egl->display, pixmap_priv->image, &fourcc, &num_planes, modifiers)) {
        return NULL;
    }
    assert(num_planes <= GBM_MAX_PLANES);
    struct gbm_import_fd_modifier_data fd_modifier_data =
    {
        .width = pixmap->drawable.width,
        .height = pixmap->drawable.height,
        .format = fourcc, /* GBM and DRM formats are the same */
        .num_fds = num_planes,
        .modifier = modifiers[0],
        .fds = {-1, -1, -1, -1},
        .strides = {0},
        .offsets = {0},
    };
/* If the spec somehow changes in the future */
#if GBM_MAX_PLANES != 4
    memset(fd_modifier_data.fds, -1, sizeof(fd_modifier_data));
#endif
    if (eglExportDMABUFImageMESA(glamor_egl->display, pixmap_priv->image,
                                 fd_modifier_data.fds,
                                 fd_modifier_data.strides,
                                 fd_modifier_data.offsets)) {
        ret = gbm_bo_import(glamor_egl->gbm, GBM_BO_IMPORT_FD_MODIFIER,
                            &fd_modifier_data, GBM_BO_USE_RENDERING);
    }
    for (int i = 0; i < num_planes; i++) {
        if (fd_modifier_data.fds[i] != -1) {
            close(fd_modifier_data.fds[i]);
        }
    }
#endif
    return ret;
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

/* Used for untextured pixmaps */
/**
 * XXX Do we even need this? XXX
 *
 * During normal operation, all pixmaps created by glamor
 * are textured. Unless we've run out of video memory,
 * the only untextured pixmaps we can have are the root
 * pixmaps for each screen.
 * (The root pixmaps can be textured, but they don't have to be)
 *
 * I don't see how an X client could request us to export
 * the root pixmap through DRI3. If this ever gets called,
 * it really feels like a bug to me.
 *
 * Same for the gbm fallback single-fd export.
 */
static int
glamor_egl_fds_from_pixmap_slow(ScreenPtr screen, PixmapPtr pixmap, int *fds,
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
#ifdef GBM_BO_FD
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
#endif
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
#ifdef GBM_BO_FD
    fds[0] = gbm_bo_get_fd(bo);
#else
    fds[0] = -1;
#endif
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

/* for textured pixmaps */
static int
glamor_egl_fds_from_pixmap_fast(ScreenPtr screen, PixmapPtr pixmap, int *fds,
                                uint32_t *strides, uint32_t *offsets,
                                uint64_t *modifier)
{
#ifdef GLAMOR_HAS_EXPORT_DMABUF_MESA
    glamor_egl_priv_t *glamor_egl =
        glamor_egl_get_screen_private(screen);
    struct glamor_pixmap_private *pixmap_priv =
        glamor_get_pixmap_private(pixmap);

    if (!glamor_egl_make_pixmap_exportable2(pixmap, TRUE)) {
        return 0;
    }

    int num_planes = 0;
    EGLuint64KHR modifiers[GBM_MAX_PLANES] = {0};
    if (!eglExportDMABUFImageQueryMESA(glamor_egl->display, pixmap_priv->image, NULL, &num_planes, modifiers)) {
        return 0;
    }

    /* typedef int32_t EGLint; */
    if (!eglExportDMABUFImageMESA(glamor_egl->display, pixmap_priv->image,
                                  fds,
                                  (EGLint*)strides,
                                  (EGLint*)offsets)) {
        return 0;
    }

    *modifier = modifiers[0];

    pixmap_priv->used_modifiers = (*modifier != DRM_FORMAT_MOD_INVALID);

    return num_planes;
#else
    return 0;
#endif
}

int
glamor_egl_fds_from_pixmap(ScreenPtr screen, PixmapPtr pixmap, int *fds,
                           uint32_t *strides, uint32_t *offsets,
                           uint64_t *modifier)
{
    static int warned = FALSE;
    int ret = glamor_egl_fds_from_pixmap_fast(screen, pixmap, fds,
                                              strides, offsets, modifier);
    if (ret) {
        return ret;
    }

    ret = glamor_egl_fds_from_pixmap_slow(screen, pixmap, fds,
                                          strides, offsets, modifier);

    if (!warned && ret) {
        LogMessage(X_WARNING, "glamor(%d): glamor_egl_fds_from_pixmap_fast failed\n", screen->myNum);
        warned = TRUE;
    }
    return ret;
}

static int
glamor_egl_fd_from_pixmap_slow(ScreenPtr screen, PixmapPtr pixmap,
                               CARD16 *stride, CARD32 *size)
{
#ifdef GBM_BO_FD
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

static int
glamor_egl_fd_from_pixmap_fast(ScreenPtr screen, PixmapPtr pixmap,
                               CARD16 *stride, CARD32 *size)
{
    uint32_t strides[GBM_MAX_PLANES] = {0};
    uint32_t offsets[GBM_MAX_PLANES] = {0};
    int fds[GBM_MAX_PLANES] = {-1, -1, -1, -1};
    uint64_t modifier = 0;

    int ret = glamor_egl_fds_from_pixmap_fast(screen, pixmap, fds,
                                              strides, offsets, &modifier);

    if (ret == 1) {
        *stride = strides[0];
        *size = *stride * pixmap->drawable.height;
        return fds[0];
    }

    for (int i = 0; i < ret; i++) {
        close(fds[i]);
    }
    return -1;
}

int
glamor_egl_fd_from_pixmap(ScreenPtr screen, PixmapPtr pixmap,
                          CARD16 *stride, CARD32 *size)
{
    static int warned = FALSE;
    int fd = glamor_egl_fd_from_pixmap_fast(screen, pixmap, stride, size);
    if (fd >= 0) {
        return fd;
    }

    fd = glamor_egl_fd_from_pixmap_slow(screen, pixmap, stride, size);

    if (!warned && (fd >= 0)) {
        LogMessage(X_WARNING, "glamor(%d): glamor_egl_fd_from_pixmap_fast failed\n", screen->myNum);
        warned = TRUE;
    }

    return fd;
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

#ifdef WITH_LIBDRM
static bool
glamor_drm_format_for_depth(CARD8 depth, uint32_t *format)
{
    switch (depth) {
    case 15:
        *format = DRM_FORMAT_ARGB1555;
        return true;
    case 16:
        *format = DRM_FORMAT_RGB565;
        return true;
    case 24:
        *format = DRM_FORMAT_XRGB8888;
        return true;
    case 30:
        *format = DRM_FORMAT_ARGB2101010;
        return true;
    case 32:
        *format = DRM_FORMAT_ARGB8888;
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
                           CARD16 _stride, CARD8 depth, CARD8 bpp)
{
#ifdef WITH_LIBDRM
    ScreenPtr screen = pixmap->drawable.pScreen;
    uint32_t format;
    const int stride = _stride;
    const int offset = 0;

    if (width == 0 || height == 0 ||
        !glamor_drm_format_for_depth(depth, &format)) {
        return FALSE;
    }

    screen->ModifyPixmapHeader(pixmap, width, height, 0, 0, stride, NULL);

    return glamor_egl_create_textured_pixmap_from_dma_bufs(pixmap,
                                                           1, &fd,
                                                           width, height,
                                                           &stride, &offset,
                                                           format, DRM_FORMAT_MOD_INVALID);
#else
    return FALSE;
#endif
}

static PixmapPtr
glamor_pixmap_from_fds_noop(ScreenPtr screen,
                            CARD8 num_fds, const int *fds,
                            CARD16 width, CARD16 height,
                            const CARD32 *_strides, const CARD32 *_offsets,
                            CARD8 depth, CARD8 bpp,
                            uint64_t modifier)
{
    return NULL;
}

PixmapPtr
glamor_pixmap_from_fds(ScreenPtr screen,
                       CARD8 num_fds, const int *fds,
                       CARD16 width, CARD16 height,
                       const CARD32 *_strides, const CARD32 *_offsets,
                       CARD8 depth, CARD8 bpp,
                       uint64_t modifier)
{
#ifdef WITH_LIBDRM
    PixmapPtr pixmap;
    glamor_egl_priv_t *glamor_egl;
    Bool ret = FALSE;

    glamor_egl = glamor_egl_get_screen_private(screen);

    pixmap = screen->CreatePixmap(screen, 0, 0, depth, 0);

    if ((glamor_egl->dmabuf_capable && modifier != DRM_FORMAT_MOD_INVALID)) {
        uint32_t format;
        if (width == 0 || height == 0 ||
            !glamor_drm_format_for_depth(depth, &format)) {
            goto error;
        }


        int strides_mem[GBM_MAX_PLANES];
        int offsets_mem[GBM_MAX_PLANES];

        int *strides;
        int *offsets;

        /* XXX Could we do this at compile-time? XXX */
        if (sizeof(int) == sizeof(CARD32)) {
            strides = (int*)_strides;
            offsets = (int*)_offsets;
        } else {
            strides = strides_mem;
            offsets = offsets_mem;
            if (num_fds > GBM_MAX_PLANES) {
                goto error;
            }
            for (int i = 0; i < num_fds; i++) {
                offsets[i] = _offsets[i];
                strides[i] = _strides[i];
            }
        }

        screen->ModifyPixmapHeader(pixmap, width, height, 0, 0, strides[0], NULL);

        ret = glamor_egl_create_textured_pixmap_from_dma_bufs(pixmap,
                                                              num_fds, fds,
                                                              width, height,
                                                              strides, offsets,
                                                              format, modifier);
    } else {
        if (num_fds == 1) {
            ret = glamor_back_pixmap_from_fd(pixmap, fds[0], width, height,
                                             _strides[0], depth, bpp);
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


/**
 * See: https://gitlab.freedesktop.org/xorg/xserver/-/work_items/1444
 * For an explanation as to why this is needed
 */
static void
glamor_filter_modifiers(uint32_t *num_modifiers, uint64_t **modifiers,
                        EGLBoolean *external_only)
{
    uint32_t write_pos = 0;
    for (uint32_t i = 0; i < *num_modifiers; i++) {
        if (external_only[i]) {
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
        uint64_t *filtered_modifiers = realloc(*modifiers, write_pos * sizeof(**modifiers));
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
    glamor_filter_modifiers(num_modifiers, modifiers, external_only);
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

static void glamor_egl_pixmap_destroy(CallbackListPtr *pcbl, ScreenPtr pScreen, PixmapPtr pixmap)
{
    ScreenPtr screen = pixmap->drawable.pScreen;
    glamor_egl_priv_t *glamor_egl =
        glamor_egl_get_screen_private(screen);

    struct glamor_pixmap_private *pixmap_priv =
        glamor_get_pixmap_private(pixmap);

    BUG_RETURN(!pixmap_priv);

    if (pixmap_priv->image) {
        eglDestroyImageKHR(glamor_egl->display, pixmap_priv->image);
        pixmap_priv->image = NULL;
    }
}

void
glamor_egl_exchange_buffers(PixmapPtr front, PixmapPtr back)
{
    EGLImageKHR temp_img;
    Bool temp_mod;
    struct glamor_pixmap_private *front_priv =
        glamor_get_pixmap_private(front);
    struct glamor_pixmap_private *back_priv =
        glamor_get_pixmap_private(back);

    BUG_RETURN(!back_priv);
    BUG_RETURN(!front_priv);

    glamor_pixmap_exchange_fbos(front, back);

    temp_img = back_priv->image;
    temp_mod = back_priv->used_modifiers;

    back_priv->image = front_priv->image;
    back_priv->used_modifiers = front_priv->used_modifiers;

    front_priv->image = temp_img;
    front_priv->used_modifiers = temp_mod;

    glamor_set_pixmap_type(front, GLAMOR_TEXTURE_DRM);
    glamor_set_pixmap_type(back, GLAMOR_TEXTURE_DRM);
}

static void glamor_egl_pre_close_screen_cleanup(glamor_egl_priv_t *glamor_egl);

static void
glamor_egl_close_screen(CallbackListPtr *pcbl, ScreenPtr screen, void *unused)
{
    glamor_egl_priv_t *glamor_egl;

    struct glamor_pixmap_private *pixmap_priv;
    PixmapPtr screen_pixmap;

    glamor_egl = glamor_egl_get_screen_private(screen);

    screen_pixmap = screen->GetScreenPixmap(screen);

    pixmap_priv = glamor_get_pixmap_private(screen_pixmap);

    if (pixmap_priv && pixmap_priv->image) {
        eglDestroyImageKHR(glamor_egl->display, pixmap_priv->image);
        pixmap_priv->image = NULL;
    }

    glamor_egl_pre_close_screen_cleanup(glamor_egl);

    dixScreenUnhookClose(screen, glamor_egl_close_screen);
    dixScreenUnhookPixmapDestroy(screen, glamor_egl_pixmap_destroy);
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

static dri3_screen_info_rec glamor_dri3_info = {
    .version = 2,

    .fd_from_pixmap = glamor_egl_fd_from_pixmap,

    /* Version 1 */
    .open_client = glamor_dri3_open_client,

    /* Version 2 */
    .pixmap_from_fds = glamor_pixmap_from_fds,
    .fds_from_pixmap = glamor_egl_fds_from_pixmap,
    .get_formats = glamor_get_formats,
    .get_modifiers = glamor_get_modifiers,
    .get_drawable_modifiers = glamor_get_drawable_modifiers,

    /* Version 4 */
    .import_syncobj = NULL, /* TODO: implement */
};
#endif /* DRI3 */

static inline void
glamor_egl_set_glvnd_vendor(ScreenPtr screen)
{
    const char *vendor;
    const char *renderer;

    glamor_egl_priv_t *glamor_egl =
        glamor_egl_get_screen_private(screen);

    /* Should we make sure the vendor is valid? (nvidia, mesa, ???) */
    if (glamor_egl->exact_glvnd_vendor) {
        glamor_set_glvnd_vendor(screen, glamor_egl->glvnd_vendor);
        return;
    }

/**
 * If we're on nvidia and the user didn't request a particular gl vendor, set it to nvidia.
 * See: https://github.com/X11Libre/xserver/pull/2847
 */
#ifdef WITH_LIBDRM
    if (glamor_egl->fd >= 0) {
        drmVersionPtr version = drmGetVersion(glamor_egl->fd);
        if (version) {
            if (version->name && !strcmp(version->name, "nvidia-drm")) {
                drmFreeVersion(version);
                glamor_set_glvnd_vendor(screen, "nvidia");
                return;
            }
            drmFreeVersion(version);
        }
    }
#endif

#ifdef GLAMOR_HAS_GBM
    if (glamor_egl->gbm) {
        const char *gbm_backend_name;
        gbm_backend_name = gbm_device_get_backend_name(glamor_egl->gbm);
        if (gbm_backend_name) {
            if (!strncmp(gbm_backend_name, "nvidia", sizeof("nvidia") - 1)) {
                 glamor_set_glvnd_vendor(screen, "nvidia");
                 return;
            } else if (!strcmp(gbm_backend_name, "drm")) {
                 /* Mesa uses "drm" as the gbm backend name */
                 glamor_set_glvnd_vendor(screen, "mesa");
                 return;
            }
        }
    }
#endif

    vendor = (const char*)glGetString(GL_VENDOR);
    renderer = (const char*)glGetString(GL_RENDERER);

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
    dixScreenHookPixmapDestroy(screen, glamor_egl_pixmap_destroy);

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

static inline Bool
glamor_egl_fd_is_render_node(int fd)
{
    struct stat buf;
    if (fstat(fd, &buf) < 0) {
        return FALSE;
    }

    return (major(buf.st_rdev) != 0) && (minor(buf.st_rdev) >= 128);
}

static inline int
glamor_egl_render_node_from_fd(int fd)
{
#ifdef WITH_LIBDRM
    char* render_name;
    int ret;

    render_name = drmGetRenderDeviceNameFromFd(fd);
    if (!render_name) {
        return -1;
    }

    ret = open(render_name, O_RDWR);
    free(render_name);
    return ret;
#else
    return -1;
#endif
}

static inline int
glamor_egl_device_get_fd(EGLDeviceEXT device)
{
    const char *dev_file = eglQueryDeviceStringEXT(device, EGL_DRM_DEVICE_FILE_EXT);
    return dev_file ? open(dev_file, O_RDWR) : -1;
}

static inline int
glamor_egl_device_get_matching_fd(EGLDeviceEXT device, int fd)
{
    int card_fd = glamor_egl_device_get_fd(device);
    if (glamor_egl_fd_is_render_node(fd)) {
        int render_fd = glamor_egl_render_node_from_fd(card_fd);
        close(card_fd);
        return render_fd;
    }

    return card_fd;
}

static inline Bool
glamor_egl_device_matches_fd(EGLDeviceEXT device, int fd)
{
    int dev_fd = glamor_egl_device_get_matching_fd(device, fd);
    if (dev_fd < 0) {
        return FALSE;
    }

    /**
     * From https://pubs.opengroup.org/onlinepubs/009696699/basedefs/sys/stat.h.html
     *
     * The st_ino and st_dev fields taken together uniquely identify the file within the system.
     */
    struct stat stat1, stat2;
    if (fstat(dev_fd, &stat2) < 0) {
        close(dev_fd);
        return FALSE;
    }

    close(dev_fd);

    if (fstat(fd, &stat1) < 0) {
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
     * If the fd passed to glamor is a render node,
     * it is safe to pick a device that doesn't match it.
     */
    if (strict <= 0 && glamor_egl->fd >= 0 &&
        glamor_egl_fd_is_render_node(glamor_egl->fd)) {
        return TRUE;
    }

    /**
     * If we're trying to do direct rendering,
     * we can't have a mismatch between the gpu and the device we pick
     *
     * If not, we don't have any strict requirements for our device
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

    /* We don't have an exact driver name match, reject this device if strict == 2 */
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
        glamor_egl_destroy_display(glamor_egl->display);
        glamor_egl->display = EGL_NO_DISPLAY;
    }

    free(glamor_egl->device_path);
    free(glamor_egl->glvnd_vendor);
}

void glamor_egl_cleanup(glamor_egl_priv_t *glamor_egl)
{
    if (!glamor_egl) {
        return;
    }

    glamor_egl_pre_close_screen_cleanup(glamor_egl);

#ifdef GLAMOR_HAS_GBM
    if (glamor_egl->gbm)
        gbm_device_destroy(glamor_egl->gbm);
#endif
}

void glamor_egl_cleanup_screen(ScreenPtr screen)
{
    /* Only clean up stuff if we set it up to begin with */
    if (screen && (glamor_egl_screen_init2 == glamor_egl_screen_init)) {
        glamor_egl_cleanup(glamor_egl_get_screen_private(screen));
    }
}

static void
glamor_egl_choose_configs(EGLDisplay display, const EGLint *attrib_list,
                          EGLConfig **configs, EGLint *num_configs)
{
    EGLint max_configs = 0;
    *configs = NULL;
    *num_configs = 0;
    if (!eglChooseConfig(display, attrib_list, NULL, 0, &max_configs) || max_configs == 0) {
        return;
    }
    *configs = calloc(max_configs, sizeof(EGLConfig));
    if (*configs == NULL) {
        return;
    }
    if (!eglChooseConfig(display, attrib_list, *configs, max_configs, num_configs) || *num_configs == 0) {
        free(*configs);
        *configs = NULL;
        *num_configs = 0;
    }
    if (*num_configs < max_configs) {
        /* Shouldn't happen */
        void *tmp = realloc(*configs, *num_configs * sizeof(EGLConfig));
        if (tmp) {
            *configs = tmp;
        }
    }
}
static EGLContext
glamor_egl_create_context(EGLDisplay display,
                          const EGLint *config_attrib_list,
                          const EGLint **ctx_attrib_lists, int num_attr_lists)
{
    EGLConfig *configs = NULL;
    EGLint num_configs = 0;
    EGLContext ctx = EGL_NO_CONTEXT;
    /* Try creating a no-config context, maybe we can skip all the config stuff */
    /* if (epoxy_has_egl_extension(display, "EGL_KHR_no_config_context")) */
    for (int j = 0; j < num_attr_lists; j++) {
        ctx = eglCreateContext(display, EGL_NO_CONFIG_KHR,
                               EGL_NO_CONTEXT, ctx_attrib_lists[j]);
        if (ctx != EGL_NO_CONTEXT) {
            return ctx;
        }
    }
    glamor_egl_choose_configs(display, config_attrib_list,
                              &configs, &num_configs);
    for (int i = 0; i < num_configs; i++) {
        for (int j = 0; j < num_attr_lists; j++) {
            ctx = eglCreateContext(display, configs[i],
                                   EGL_NO_CONTEXT, ctx_attrib_lists[j]);
            if (ctx != EGL_NO_CONTEXT) {
                free(configs);
                return ctx;
            }
        }
    }
    free(configs);
    return EGL_NO_CONTEXT;
}

static Bool
glamor_egl_try_big_gl_api(glamor_egl_priv_t *glamor_egl)
{
    static const EGLint config_attribs_core[] = {
        EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,
        EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
        EGL_CONTEXT_MAJOR_VERSION_KHR,
        GLAMOR_GL_CORE_VER_MAJOR,
        EGL_CONTEXT_MINOR_VERSION_KHR,
        GLAMOR_GL_CORE_VER_MINOR,
     /* EGL_CONTEXT_PRIORITY_LEVEL_IMG, EGL_CONTEXT_PRIORITY_HIGH_IMG, */
        EGL_NONE
    };
    static const EGLint config_attribs[] = {
        EGL_NONE
    };

    static const EGLint* ctx_attrib_lists[] =
        { config_attribs_core, config_attribs };

    static const EGLint config_attrib_list[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_CONFORMANT, EGL_OPENGL_BIT,
        EGL_SURFACE_TYPE, EGL_DONT_CARE, /* EGL_STREAM_BIT_KHR */
        EGL_NONE
    };

    if (!eglBindAPI(EGL_OPENGL_API)) {
        LogMessage(X_ERROR, "glamor: Failed to bind GL API.\n");
        return FALSE;
    }

    glamor_egl->context = glamor_egl_create_context(glamor_egl->display,
                                                    config_attrib_list,
                                                    ctx_attrib_lists,
                                                    ARRAY_SIZE(ctx_attrib_lists));

    if (glamor_egl->context == EGL_NO_CONTEXT) {
        LogMessage(X_ERROR, "Failed to create GL context\n");
        return FALSE;
    }

    if (!eglMakeCurrent(glamor_egl->display,
                        EGL_NO_SURFACE, EGL_NO_SURFACE, glamor_egl->context)) {
        LogMessage(X_ERROR, "Failed to make GL context current\n");

        eglDestroyContext(glamor_egl->display, glamor_egl->context);
        glamor_egl->context = EGL_NO_CONTEXT;
        return FALSE;
    }
    if (epoxy_gl_version() < 21) {
        LogMessage(X_INFO, "glamor: Ignoring GL < 2.1, falling back to GLES.\n");

        eglDestroyContext(glamor_egl->display, glamor_egl->context);
        glamor_egl->context = EGL_NO_CONTEXT;
        return FALSE;
    }

    LogMessage(X_INFO,
        "glamor: Using OpenGL %d.%d context.\n",
        epoxy_gl_version() / 10,
        epoxy_gl_version() % 10);

    return TRUE;
}

static Bool
glamor_egl_try_gles_api(glamor_egl_priv_t *glamor_egl)
{
    static const EGLint config_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
     /* EGL_CONTEXT_PRIORITY_LEVEL_IMG, EGL_CONTEXT_PRIORITY_HIGH_IMG, */
        EGL_NONE
    };

    static const EGLint* ctx_attrib_lists[] =
        { config_attribs };

    static const EGLint config_attrib_list[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_CONFORMANT, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_DONT_CARE, /* EGL_STREAM_BIT_KHR */
        EGL_NONE
    };


    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        LogMessage(X_ERROR, "glamor: Failed to bind GLES API.\n");
        return FALSE;
    }

    glamor_egl->context = glamor_egl_create_context(glamor_egl->display,
                                                    config_attrib_list,
                                                    ctx_attrib_lists,
                                                    ARRAY_SIZE(ctx_attrib_lists));

    if (glamor_egl->context == EGL_NO_CONTEXT) {
        LogMessage(X_ERROR, "Failed to create GLES context\n");
        return FALSE;
    }
    if (!eglMakeCurrent(glamor_egl->display,
                        EGL_NO_SURFACE, EGL_NO_SURFACE, glamor_egl->context)) {
        eglDestroyContext(glamor_egl->display, glamor_egl->context);
        glamor_egl->context = EGL_NO_CONTEXT;
        LogMessage(X_ERROR, "Failed to make GLES context current\n");
        return FALSE;
    }

    LogMessage(X_INFO,
               "glamor: Using OpenGL ES %d.%d context.\n",
               epoxy_gl_version() / 10,
               epoxy_gl_version() % 10);

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
glamor_egl_init_display(glamor_egl_priv_t *glamor_egl, int *dri_fd, int *out_platform)
{
    EGLDeviceEXT *devices = NULL;
    EGLint num_devices = 0;
    const char *driver_name = NULL;
    int try_egl_devices = FALSE;

#ifdef GLAMOR_HAS_GBM
    int gbm_platform_tried = FALSE;
#endif

/**
 * See:
 * https://registry.khronos.org/EGL/extensions/KHR/EGL_KHR_platform_gbm.txt
 * https://registry.khronos.org/EGL/extensions/MESA/EGL_MESA_platform_gbm.txt
 *
 * For where this is defined
 */
#ifndef EGL_PLATFORM_GBM_KHR
#define EGL_PLATFORM_GBM_KHR 0x31D7
#endif

/**
 * If we're on nvidia and the user didn't request a particular gl vendor, set it to nvidia.
 * See: https://github.com/X11Libre/xserver/pull/2847
 */
#ifdef WITH_LIBDRM
    if (!glamor_egl->glvnd_vendor && (glamor_egl->fd >= 0)) {
        drmVersionPtr version = drmGetVersion(glamor_egl->fd);
        if (version) {
            if (version->name && !strcmp(version->name, "nvidia-drm")) {
                glamor_egl->glvnd_vendor = strdup("nvidia");
            }
            drmFreeVersion(version);
        }
    }
#endif

    /**
     * If the user didn't give us a GL driver/library name,
     * we populate it with what we queried
     */
#define GLAMOR_EGL_TRY_PLATFORM(platform, native, platform_fallback) \
    glamor_egl->display = glamor_egl_get_display2(platform, native, platform_fallback); \
    glamor_egl_add_display_to_list(glamor_egl->display); \
    if (glamor_egl->display == EGL_NO_DISPLAY) { \
        LogMessage(X_ERROR, "glamor: eglGetDisplay(" #platform ", " #native ") failed\n"); \
    } else { \
        if (eglInitialize(glamor_egl->display, NULL, NULL)) { \
            if (out_platform) { \
                *out_platform = platform; \
            } \
            if (!glamor_egl->glvnd_vendor && driver_name) { \
                glamor_egl->glvnd_vendor = strdup(driver_name); \
            } \
            LogMessage(X_INFO, "glamor: eglInitialize() succeeded on " #platform "\n"); \
            if (dri_fd && platform == EGL_PLATFORM_DEVICE_EXT) { \
                *dri_fd = glamor_egl_device_get_fd(native); \
            } \
            free(devices); \
            return TRUE; \
        } \
        LogMessage(X_ERROR, "glamor: eglInitialize() failed on " #platform "\n"); \
        glamor_egl_destroy_display(glamor_egl->display); \
        glamor_egl->display = EGL_NO_DISPLAY; \
    }

    /* If no gl vendor is passed, we don't gain anything by first trying the device platform */
#ifdef GLAMOR_HAS_GBM
    if (glamor_egl->gbm && !glamor_egl->glvnd_vendor) {
        GLAMOR_EGL_TRY_PLATFORM(EGL_PLATFORM_GBM_KHR, glamor_egl->gbm, FALSE);
        gbm_platform_tried = TRUE;
    }
#endif

    try_egl_devices = glamor_query_devices_ext(&devices, &num_devices);

    if (try_egl_devices) {
#define GLAMOR_EGL_TRY_PLATFORM_DEVICE(strict) \
        for (uint32_t i = 0; i < num_devices; i++) { \
            if (glamor_egl_device_matches_config(devices[i], glamor_egl, strict, &driver_name)) { \
                GLAMOR_EGL_TRY_PLATFORM(EGL_PLATFORM_DEVICE_EXT, devices[i], TRUE); \
            } \
        }

        /* These 2 queries are exact matches to our fd and gl library */
        GLAMOR_EGL_TRY_PLATFORM_DEVICE(2);
        GLAMOR_EGL_TRY_PLATFORM_DEVICE(1);
    }

#ifdef GLAMOR_HAS_GBM
    if (glamor_egl->gbm && !gbm_platform_tried) {
        GLAMOR_EGL_TRY_PLATFORM(EGL_PLATFORM_GBM_KHR, glamor_egl->gbm, FALSE);
    }
#endif

    if (try_egl_devices) {
        GLAMOR_EGL_TRY_PLATFORM_DEVICE(0);
    }

#undef GLAMOR_EGL_TRY_PLATFORM_DEVICE

    driver_name = NULL;

    /**
     * We only try these fallbacks if we don't have an fd passed, since we
     * have to do some guessing anyway to find the desired gpu.
     *
     * Trying these in multi-card setups risks a screen driven by one card
     * being mapped an EGLDisplay backed by a different card, which can break.
     *
     * We actually can specify the device using EGL_EXT_explicit_device:
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

int
glamor_egl_get_fd(ScreenPtr screen)
{
    return glamor_egl_get_screen_private(screen)->fd;
}

#ifdef GLAMOR_HAS_GBM
static Bool
glamor_egl_can_texture_gbm_bo(glamor_egl_priv_t *glamor_egl, int is_nvidia)
{
    int backend_is_mesa = FALSE;
    int linear_only = FALSE;
    const char *backend_name = gbm_device_get_backend_name(glamor_egl->gbm);
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
            return FALSE;
        }
    }

#if 0 /* If there is another vendor that has similar issues, re-enable this code */
    /* Check if at least one combination of format + modifier is supported */
    CARD32 *formats = NULL;
    CARD32 num_formats = 0;
    Bool found = FALSE;

    int linear_only;

    if (linear_only && !glamor_egl->dmabuf_capable) {
        /* We can't check reliably */
        return FALSE;
    }

    if (!glamor_get_formats_internal(glamor_egl, &num_formats, &formats)) {
        return FALSE;
    }
    if (num_formats == 0) {
        /* Everything is supported (unlikely) */
        return TRUE;
    }
    for (uint32_t i = 0; i < num_formats; i++) {
        uint64_t *modifiers = NULL;
        uint32_t num_modifiers = 0;
        if (glamor_get_modifiers_internal(glamor_egl, formats[i],
                                          &num_modifiers, &modifiers)) {
            if (num_modifiers == 0) {
                /* Modifiers are implicit */
                found = TRUE;
                break;
            }

            if (linear_only) {
                for (uint32_t j = 0; j < num_modifiers; j++) {
                    if (
#ifdef WITH_LIBDRM
                        (modifiers[j] == DRM_FORMAT_MOD_LINEAR) ||
                        (modifiers[j] == DRM_FORMAT_MOD_INVALID))
#else
                        modifiers[j] == 0) /* DRM_FORMAT_MOD_LINEAR */
#endif
                    {
                        found = TRUE;
                        break;
                    }
                }
            } else {
                /* Nothing to filter */
                found = TRUE;
            }

            free(modifiers);
            if (found) {
                break;
            }
        }
    }
    free(formats);
    return found;
#else
    return TRUE;
#endif
}
#endif

Bool
glamor_egl_init_internal(glamor_egl_conf_t* glamor_egl_conf, int *caps)
{
    const char *renderer;
    const char *vendor;
    glamor_egl_priv_t* glamor_egl = NULL;
    int *dri_fd = NULL;
    int platform = 0;
    int _caps;
    int is_nvidia = FALSE;

    if (!caps) {
        caps = &_caps;
    }

    *caps = GLAMOR_EGL_CAP_NONE;

    if (glamor_egl_conf->GLAMOR_EGL_PRIV_PROC) {
        glamor_egl_get_screen_private = glamor_egl_conf->GLAMOR_EGL_PRIV_PROC;
        glamor_egl = glamor_egl_conf->glamor_egl_priv;
    } else {
        if (!glamor_egl_conf->screen ||
            !glamor_egl_init_screen_private(glamor_egl_conf->screen)) {
            goto error;
        }
        glamor_egl = glamor_egl_get_screen_private(glamor_egl_conf->screen);
    }

    memset(glamor_egl, 0, sizeof(*glamor_egl));

    if (glamor_egl_conf->glvnd_vendor) {
        glamor_egl->glvnd_vendor = strdup(glamor_egl_conf->glvnd_vendor);
        glamor_egl->exact_glvnd_vendor = !!glamor_egl->glvnd_vendor;
    }
    glamor_egl->fd = glamor_egl_conf->fd;

#ifdef GLAMOR_HAS_GBM
    if (glamor_egl->fd >= 0 && !glamor_egl_conf->gbm_forbidden) {
        glamor_egl->gbm = gbm_create_device(glamor_egl->fd);
        if (!glamor_egl->gbm) {
            glamor_egl->gbm = gbm_create_device_by_name(glamor_egl->fd, "dumb");
        }

        if (glamor_egl->gbm == NULL) {
            ErrorF("couldn't create gbm device\n");
            glamor_egl->fd = -1;
        }
    }
#endif

    if (glamor_egl_conf->auto_dri && glamor_egl->fd < 0) {
        dri_fd = &glamor_egl->fd;
    }

    if (!glamor_egl_init_display(glamor_egl, dri_fd, &platform)) {
        goto error;
    }

#define GLAMOR_CHECK_EGL_EXTENSION(EXT)  \
	if (!epoxy_has_egl_extension(glamor_egl->display, "EGL_" #EXT)) {  \
		ErrorF("EGL_" #EXT " required.\n");  \
		goto error;  \
	}

    GLAMOR_CHECK_EGL_EXTENSION(KHR_surfaceless_context);

#ifdef GLAMOR_HAS_GBM
    if (!epoxy_has_egl_extension(glamor_egl->display, "EGL_MESA_image_dma_buf_export")) {
        LogMessage(X_WARNING, "glamor: EGL extension EGL_MESA_image_dma_buf_export not available\n");
        LogMessage(X_WARNING, "glamor: DRI3 dmabuf export will be slower\n");
        glamor_dri3_info.fd_from_pixmap = glamor_egl_fd_from_pixmap_slow;
        glamor_dri3_info.fds_from_pixmap = glamor_egl_fds_from_pixmap_slow;
    }
#endif

    if (!glamor_egl_conf->force_es) {
        glamor_egl_try_big_gl_api(glamor_egl);
    }

    if (glamor_egl->context == EGL_NO_CONTEXT && !glamor_egl_conf->es_disallowed) {
        glamor_egl_try_gles_api(glamor_egl);
    }

    if (glamor_egl->context == EGL_NO_CONTEXT) {
        LogMessage(X_ERROR,
                   "glamor: Failed to create GL or GLES2 contexts\n");
        goto error;
    }

    renderer = (const char*)glGetString(GL_RENDERER);
    vendor = (const char*)glGetString(GL_VENDOR);

    if (renderer && strstr(renderer, "NVIDIA")) {
        is_nvidia = TRUE;
    } else if (vendor && strstr(vendor, "NVIDIA")) {
        is_nvidia = TRUE;
    } else {
        is_nvidia = FALSE;
    }

    if (!glamor_egl_conf->force_glamor) {
        if (!renderer) {
            LogMessage(X_ERROR,
                       "glGetString() returned NULL, your GL is broken\n");
            goto error;
        }
        if (strstr(renderer, "softpipe")) {
            LogMessage(X_INFO,
                       "Refusing to try glamor on softpipe\n");
            goto error;
        }
        if (!strncmp("llvmpipe", renderer, sizeof("llvmpipe") - 1)) {
            if (glamor_egl_conf->llvmpipe_allowed)
                LogMessage(X_INFO,
                           "Allowing glamor on llvmpipe for PRIME\n");
            else {
                LogMessage(X_INFO,
                           "Refusing to try glamor on llvmpipe\n");
                 goto error;
            }
        }
    }

    /*
     * Force the next glamor_make_current call to set the right context
     * (in case of multiple GPUs using glamor)
     */
    lastGLContext = NULL;

    /* XXX From here on, glamor initialization should not fail completely XXX */

    if (glamor_egl->fd < 0) {
        goto glamor_no_dri;
    }

    glamor_egl->has_EXT_EGL_image_storage = epoxy_has_gl_extension("GL_EXT_EGL_image_storage");
    glamor_egl->has_OES_EGL_image = epoxy_has_gl_extension("GL_OES_EGL_image");

    if (!glamor_egl->has_EXT_EGL_image_storage && !glamor_egl->has_OES_EGL_image) {
        LogMessage(X_ERROR,
                   "glamor: Extensions GL_EXT_EGL_image_storage and GL_OES_EGL_image are both unavailable\n");
        LogMessage(X_ERROR,
                   "glamor: DRI3 import will not be available\n");
        glamor_dri3_info.pixmap_from_fds = NULL;
    }

#if defined(GLAMOR_HAS_GBM) && defined(GLAMOR_HAS_EXPORT_DMABUF_MESA)
    glamor_egl->has_image_dma_buf_export = epoxy_has_egl_extension(glamor_egl->display, "EGL_MESA_image_dma_buf_export");
#endif

    if (epoxy_has_egl_extension(glamor_egl->display,
                                "EGL_EXT_image_dma_buf_import") &&
        epoxy_has_egl_extension(glamor_egl->display,
                                "EGL_EXT_image_dma_buf_import_modifiers")) {

        if (glamor_egl_conf->dmabuf_forced)
            glamor_egl->dmabuf_capable = glamor_egl_conf->dmabuf_capable;
        else if (!renderer)
            glamor_egl->dmabuf_capable = FALSE;
        else if (strstr(renderer, "Intel"))
            glamor_egl->dmabuf_capable = TRUE;
        else if (strstr(renderer, "zink"))
            glamor_egl->dmabuf_capable = TRUE;
        else if (is_nvidia)
            glamor_egl->dmabuf_capable = TRUE;
        else if (strstr(renderer, "radeonsi"))
            glamor_egl->dmabuf_capable = TRUE;
        else
            glamor_egl->dmabuf_capable = FALSE;
    }

#ifdef GLAMOR_HAS_GBM
    glamor_egl->fast_gbm_import = renderer && vendor && !is_nvidia && (platform == EGL_PLATFORM_GBM_KHR);
    if (glamor_egl->gbm) {
        glamor_egl->can_texture_gbm_bo = glamor_egl_can_texture_gbm_bo(glamor_egl, is_nvidia);
    }
#endif

    *caps |= GLAMOR_EGL_DEFAULT_CAPS;
    if (!glamor_dri3_info.pixmap_from_fds) {
        *caps &= ~GLAMOR_EGL_CAP_DRI3_IMPORT;
        /* Avoid DRI3 returning BadImplementation */
        glamor_dri3_info.pixmap_from_fds = glamor_pixmap_from_fds_noop;
    }

#ifdef GLAMOR_HAS_GBM
    if (glamor_egl->can_texture_gbm_bo) {
        LogMessage(X_INFO, "glamor: Can texture gbm buffers\n");
    }
#endif

#ifdef GLAMOR_HAS_GBM
    if (!glamor_egl->gbm || !glamor_egl->can_texture_gbm_bo)
#endif
    {
        if (!glamor_egl_conf->gbm_forbidden) {
            LogMessage(X_ERROR, "glamor: Cannot texture gbm buffers\n");
        }
        *caps &= ~GLAMOR_EGL_CAP_TEXTURE_GBM_BO;
        if (epoxy_has_egl_extension(glamor_egl->display, "EGL_MESA_image_dma_buf_export")) {
            glamor_dri3_info.fd_from_pixmap = glamor_egl_fd_from_pixmap_fast;
            glamor_dri3_info.fds_from_pixmap = glamor_egl_fds_from_pixmap_fast;
        } else {
            LogMessage(X_WARNING, "glamor: EGL extension EGL_MESA_image_dma_buf_export not available\n");
            LogMessage(X_WARNING, "glamor: DRI3 dmabuf export will be unavailable\n");
            glamor_dri3_info.fd_from_pixmap = NULL;
            glamor_dri3_info.fds_from_pixmap = NULL;
            *caps &= ~GLAMOR_EGL_CAP_DRI3_EXPORT;
        }
    }

#define GLAMOR_EGL_CAP_DRI3_BASE (GLAMOR_EGL_CAP_DRI3_IMPORT | GLAMOR_EGL_CAP_DRI3_EXPORT)

    /* Some clients can handle DRI3 missing, but not partial support */
    if ((*caps & GLAMOR_EGL_CAP_DRI3_BASE) != GLAMOR_EGL_CAP_DRI3_BASE) {
        if (!glamor_egl_conf->partial_dri_allowed) {
            LogMessage(X_INFO, "glamor: Not enabling partial DRI3 support\n");
            goto glamor_no_dri;
        } else {
            LogMessage(X_INFO, "glamor: Using partial DRI3 support\n");
        }
    }

    LogMessage(X_INFO, "glamor dri X acceleration enabled on %s\n",
               renderer);
    return TRUE;

error:
    LogMessage(X_ERROR, "glamor X acceleration failed to initialize\n");
    glamor_egl_cleanup(glamor_egl);
    return FALSE;

glamor_no_dri:
    glamor_egl->fd = -1;

    LogMessage(X_WARNING, "glamor X acceleration enabled without dri support on %s\n",
               renderer);
    return TRUE;
}
