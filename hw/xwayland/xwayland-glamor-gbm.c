/*
 * Copyright © 2011-2014 Intel Corporation
 * Copyright © 2017 Red Hat Inc.
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
 *    Lyude Paul <lyude@redhat.com>
 *
 */

#include <xwayland-config.h>

#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#ifdef DRI3
#include <sys/eventfd.h>
#endif /* DRI3 */
#include <sys/stat.h>
#include <xf86drm.h>
#include <drm_fourcc.h>
#if defined(__linux__)
#include <linux/dma-buf.h>
#include <linux/sync_file.h>
#include <sys/ioctl.h>
#endif

#define MESA_EGL_NO_X11_HEADERS
#define EGL_NO_X11
#include <gbm.h>
#include <glamor_egl.h>

#include <glamor.h>
#include <glamor_context.h>
#include <dri3.h>
#include "drm-client-protocol.h"

#include "xwayland-glamor.h"
#include "xwayland-glamor-gbm.h"
#include "xwayland-pixmap.h"
#include "xwayland-screen.h"
#include "xwayland-window-buffers.h"

#include "linux-dmabuf-unstable-v1-client-protocol.h"
#include "linux-drm-syncobj-v1-client-protocol.h"

struct xwl_gbm_private {
    drmDevice *device;
    char *device_name;
    struct gbm_device *gbm;
    int drm_fd;
    Bool fd_render_node;
    Bool drm_authenticated;
    Bool dmabuf_capable;
    Bool glamor_gles;
    Bool implicit_sync;
    Bool supports_syncobjs;

    /* Set if wl_drm is available */
    struct wl_drm *drm;
    uint32_t capabilities;
};

struct xwl_pixmap {
    struct wl_buffer *buffer;
    EGLImage image;
    unsigned int texture;
    struct gbm_bo *bo;
    Bool implicit_modifier;
#ifdef DRI3
    struct dri3_syncobj *syncobj;
    uint64_t timeline_point;
    int efd;
    struct xwl_window_buffer *xwl_window_buffer;
#endif /* DRI3 */
};

static DevPrivateKeyRec xwl_gbm_private_key;
static DevPrivateKeyRec xwl_auth_state_private_key;

static inline struct xwl_gbm_private *
xwl_gbm_get(struct xwl_screen *xwl_screen)
{
    return dixLookupPrivate(&xwl_screen->screen->devPrivates,
                            &xwl_gbm_private_key);
}

Bool
xwl_glamor_has_wl_drm(struct xwl_screen *xwl_screen)
{
    struct xwl_gbm_private *xwl_gbm = xwl_gbm_get(xwl_screen);

    return !!(xwl_gbm->drm != NULL);
}

/* There is a workaround for Mesa behaviour, which will cause black windows
 * when RGBX formats is using. Why exactly? There is an explanation:
 * 1. We create GL_RGBA texture with GL_UNSIGNED_BYTE type, all allowed by ES.
 * 2 .We export these texture to GBM bo with GBM_FORMAT_XRGB8888, and Mesa sets internal
 * format of these textures as GL_RGB8 (mesa/mesa!5034 (merged))
 * 3. We import these BO at some point, and use glTexSubImage on it with GL_RGBA format
 * and with GL_UNSIGNED_BYTE type, as we creates. Mesa checks its internalformat
 * in glTexSubImage2D and fails due to GLES internal format limitation
 * (see https://registry.khronos.org/OpenGL/specs/es/2.0/es_full_spec_2.0.pdf, section 3.7.1).
 */
static uint32_t
gbm_format_for_depth(int depth, int gles)
{
    switch (depth) {
    case 15:
        return GBM_FORMAT_ARGB1555;
    case 16:
        return GBM_FORMAT_RGB565;
    case 24:
        if (gles)
            return GBM_FORMAT_ARGB8888;
        return GBM_FORMAT_XRGB8888;
    case 30:
        return GBM_FORMAT_ARGB2101010;
    default:
        ErrorF("unexpected depth: %d\n", depth);
    case 32:
        return GBM_FORMAT_ARGB8888;
    }
}

static char
is_device_path_render_node (const char *device_path)
{
    char is_render_node;
    int fd;

    fd = open(device_path, O_RDWR | O_CLOEXEC);
    if (fd < 0)
        return 0;

    is_render_node = (drmGetNodeTypeFromFd(fd) == DRM_NODE_RENDER);
    close(fd);

    return is_render_node;
}

static PixmapPtr
xwl_glamor_gbm_create_pixmap_for_bo(ScreenPtr screen, struct gbm_bo *bo,
                                    int depth,
                                    Bool implicit_modifier)
{
    PixmapPtr pixmap;
    struct xwl_screen *xwl_screen = xwl_screen_get(screen);
#ifdef GBM_BO_FD_FOR_PLANE
    struct xwl_gbm_private *xwl_gbm = xwl_gbm_get(xwl_screen);
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

    struct xwl_pixmap *xwl_pixmap = calloc(1, sizeof(struct xwl_pixmap));
    if (xwl_pixmap == NULL)
        return NULL;

    pixmap = glamor_create_pixmap(screen,
                                  gbm_bo_get_width(bo),
                                  gbm_bo_get_height(bo),
                                  depth,
                                  GLAMOR_CREATE_PIXMAP_NO_TEXTURE);
    if (!pixmap) {
        free(xwl_pixmap);
        return NULL;
    }

    xwl_glamor_egl_make_current(xwl_screen);
    xwl_pixmap->bo = bo;
    xwl_pixmap->buffer = NULL;
    xwl_pixmap->implicit_modifier = implicit_modifier;
#if defined(XWL_HAS_GLAMOR) && defined(DRI3)
    xwl_pixmap->efd = -1;
#endif /* defined(XWL_HAS_GLAMOR) && defined(DRI3) */

#ifdef GBM_BO_FD_FOR_PLANE
    if (xwl_gbm->dmabuf_capable) {
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

        xwl_pixmap->image = eglCreateImageKHR(xwl_screen->egl_display,
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
        xwl_pixmap->image = eglCreateImageKHR(xwl_screen->egl_display,
                                              EGL_NO_CONTEXT,
                                              EGL_NATIVE_PIXMAP_KHR,
                                              xwl_pixmap->bo, NULL);
    }

    if (xwl_pixmap->image == EGL_NO_IMAGE_KHR)
      goto error;

    glGenTextures(1, &xwl_pixmap->texture);
    glBindTexture(GL_TEXTURE_2D, xwl_pixmap->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, xwl_pixmap->image);
    if (eglGetError() != EGL_SUCCESS)
      goto error;

    glBindTexture(GL_TEXTURE_2D, 0);

    if (!glamor_set_pixmap_texture(pixmap, xwl_pixmap->texture))
      goto error;

    glamor_set_pixmap_type(pixmap, GLAMOR_TEXTURE_DRM);
    xwl_pixmap_set_private(pixmap, xwl_pixmap);

    return pixmap;

error:
    if (xwl_pixmap->image != EGL_NO_IMAGE_KHR)
      eglDestroyImageKHR(xwl_screen->egl_display, xwl_pixmap->image);
    if (pixmap)
      glamor_destroy_pixmap(pixmap);
    free(xwl_pixmap);

    return NULL;
}

static PixmapPtr
xwl_glamor_gbm_create_pixmap_internal(struct xwl_screen *xwl_screen,
                                      DrawablePtr drawable,
                                      int width, int height, int depth,
                                      unsigned int hint,
                                      Bool implicit_scanout)
{
    struct xwl_gbm_private *xwl_gbm = xwl_gbm_get(xwl_screen);
    struct gbm_bo *bo = NULL;
    PixmapPtr pixmap = NULL;
    uint32_t num_modifiers = 0;
    uint64_t *modifiers = NULL;

    if (width > 0 && height > 0 && depth >= 15 &&
        (hint == CREATE_PIXMAP_USAGE_BACKING_PIXMAP ||
         hint == CREATE_PIXMAP_USAGE_SHARED ||
         (xwl_screen->rootless && hint == 0))) {
        uint32_t format = gbm_format_for_depth(depth, xwl_gbm->glamor_gles);
        Bool implicit = FALSE;

#ifdef GBM_BO_WITH_MODIFIERS
        if (xwl_gbm->dmabuf_capable) {
            Bool supports_scanout = FALSE;

            if (drawable) {
                xwl_glamor_get_drawable_modifiers_and_scanout(drawable,
                                                              format,
                                                              &num_modifiers,
                                                              &modifiers,
                                                              &supports_scanout);
            }

            if (num_modifiers == 0) {
                xwl_glamor_get_modifiers(xwl_screen->screen, format,
                                         &num_modifiers, &modifiers);
            }

            if (num_modifiers > 0) {
#ifdef GBM_BO_WITH_MODIFIERS2
                uint32_t usage = GBM_BO_USE_RENDERING;
                if (supports_scanout)
                    usage |= GBM_BO_USE_SCANOUT;
                bo = gbm_bo_create_with_modifiers2(xwl_gbm->gbm, width, height,
                                                   format, modifiers, num_modifiers,
                                                   usage);
#else
                bo = gbm_bo_create_with_modifiers(xwl_gbm->gbm, width, height,
                                                  format, modifiers, num_modifiers);
#endif
            }
        }
#endif
        if (bo == NULL) {
            uint32_t usage = GBM_BO_USE_RENDERING;
            implicit = TRUE;
            if (implicit_scanout)
                usage |= GBM_BO_USE_SCANOUT;

            if (num_modifiers > 0) {
                Bool has_mod_invalid = FALSE, has_mod_linear = FALSE;
                int i;

                for (i = 0; i < num_modifiers; i++) {
                    if (modifiers[i] == DRM_FORMAT_MOD_INVALID)
                        has_mod_invalid = TRUE;
                    else if (modifiers[i] == DRM_FORMAT_MOD_LINEAR)
                        has_mod_linear = TRUE;
                }

                if (!has_mod_invalid && has_mod_linear)
                    usage |= GBM_BO_USE_LINEAR;
            }

            bo = gbm_bo_create(xwl_gbm->gbm, width, height, format, usage);
        }

        if (bo) {
            pixmap = xwl_glamor_gbm_create_pixmap_for_bo(xwl_screen->screen, bo, depth, implicit);

            if (!pixmap) {
                gbm_bo_destroy(bo);
            }
            else if (xwl_screen->rootless && hint == CREATE_PIXMAP_USAGE_BACKING_PIXMAP) {
                glamor_clear_pixmap(pixmap);
            }
        }
    }

    if (!pixmap)
        pixmap = glamor_create_pixmap(xwl_screen->screen, width, height, depth, hint);

    free(modifiers);
    return pixmap;
}

static PixmapPtr
xwl_glamor_gbm_create_pixmap(ScreenPtr screen,
                             int width, int height, int depth,
                             unsigned int hint)
{
    return xwl_glamor_gbm_create_pixmap_internal(xwl_screen_get(screen), NULL,
                                                 width, height, depth, hint, FALSE);
}

PixmapPtr
xwl_glamor_create_pixmap_for_window(struct xwl_window *xwl_window)
{
    struct xwl_screen *xwl_screen = xwl_window->xwl_screen;
    WindowPtr window = xwl_window->surface_window;
    unsigned border_width = 2 * window->borderWidth;

    if (!xwl_screen->glamor)
        return NullPixmap;

    return xwl_glamor_gbm_create_pixmap_internal(xwl_screen,
                                                 &window->drawable,
                                                 window->drawable.width + border_width,
                                                 window->drawable.height + border_width,
                                                 window->drawable.depth,
                                                 CREATE_PIXMAP_USAGE_BACKING_PIXMAP,
                                                 xwl_window->has_implicit_scanout_support);
}

static Bool
xwl_glamor_gbm_destroy_pixmap(PixmapPtr pixmap)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(pixmap->drawable.pScreen);
    struct xwl_pixmap *xwl_pixmap = xwl_pixmap_get(pixmap);

    if (xwl_pixmap && pixmap->refcnt == 1) {
        xwl_pixmap_del_buffer_release_cb(pixmap);
        if (xwl_pixmap->buffer)
            wl_buffer_destroy(xwl_pixmap->buffer);

        eglDestroyImageKHR(xwl_screen->egl_display, xwl_pixmap->image);
        if (xwl_pixmap->bo)
           gbm_bo_destroy(xwl_pixmap->bo);
        xwl_glamor_gbm_dispose_syncpts(pixmap);
        free(xwl_pixmap);
    }

    return glamor_destroy_pixmap(pixmap);
}

static const struct wl_buffer_listener xwl_glamor_gbm_buffer_listener = {
    xwl_pixmap_buffer_release_cb,
};

#ifdef GBM_BO_WITH_MODIFIERS
static Bool
init_buffer_params_with_modifiers(struct xwl_pixmap *xwl_pixmap,
                                  uint64_t          *modifier,
                                  int               *num_planes,
                                  int               *prime_fds,
                                  uint32_t          *strides,
                                  uint32_t          *offsets)
{
#ifndef GBM_BO_FD_FOR_PLANE
    int32_t first_handle;
#endif
    int i;

    *num_planes = gbm_bo_get_plane_count(xwl_pixmap->bo);
    *modifier = gbm_bo_get_modifier(xwl_pixmap->bo);

    for (i = 0; i < *num_planes; i++) {
#ifdef GBM_BO_FD_FOR_PLANE
        prime_fds[i] = gbm_bo_get_fd_for_plane(xwl_pixmap->bo, i);
#else
        union gbm_bo_handle plane_handle;

        plane_handle = gbm_bo_get_handle_for_plane(xwl_pixmap->bo, i);
        if (i == 0)
            first_handle = plane_handle.s32;

        /* If all planes point to the same object as the first plane, i.e. they
         * all have the same handle, we can fall back to the non-planar
         * gbm_bo_get_fd without losing information. If they point to different
         * objects we are out of luck and need to give up.
         */
        if (first_handle == plane_handle.s32)
            prime_fds[i] = gbm_bo_get_fd(xwl_pixmap->bo);
        else
            prime_fds[i] = -1;
#endif
        if (prime_fds[i] == -1) {
            while (--i >= 0)
                close(prime_fds[i]);
            return FALSE;
        }
        strides[i] = gbm_bo_get_stride_for_plane(xwl_pixmap->bo, i);
        offsets[i] = gbm_bo_get_offset(xwl_pixmap->bo, i);
    }

    return TRUE;
}
#endif

static Bool
init_buffer_params_fallback(struct xwl_pixmap *xwl_pixmap,
                            uint64_t          *modifier,
                            int               *num_planes,
                            int               *prime_fds,
                            uint32_t          *strides,
                            uint32_t          *offsets)
{
    *num_planes = 1;
    *modifier = DRM_FORMAT_MOD_INVALID;
    prime_fds[0] = gbm_bo_get_fd(xwl_pixmap->bo);
    if (prime_fds[0] == -1)
        return FALSE;

    strides[0] = gbm_bo_get_stride(xwl_pixmap->bo);
    offsets[0] = 0;

    return TRUE;
}

struct wl_buffer *
xwl_glamor_pixmap_get_wl_buffer(PixmapPtr pixmap)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(pixmap->drawable.pScreen);
    struct xwl_pixmap *xwl_pixmap = xwl_pixmap_get(pixmap);
    struct xwl_gbm_private *xwl_gbm = xwl_gbm_get(xwl_screen);
    unsigned short width = pixmap->drawable.width;
    unsigned short height = pixmap->drawable.height;
    uint32_t format;
    int num_planes;
    int prime_fds[4];
    uint32_t strides[4];
    uint32_t offsets[4];
    uint64_t modifier;
    int i;

    if (xwl_pixmap == NULL)
       return NULL;

    if (xwl_pixmap->buffer) {
        /* Buffer already exists. */
        return xwl_pixmap->buffer;
    }

    if (!xwl_pixmap->bo)
       return NULL;

    format = wl_drm_format_for_depth(pixmap->drawable.depth);

#ifdef GBM_BO_WITH_MODIFIERS
    if (!xwl_pixmap->implicit_modifier) {
        if (!init_buffer_params_with_modifiers(xwl_pixmap,
                                               &modifier,
                                               &num_planes,
                                               prime_fds,
                                               strides,
                                               offsets))
            return NULL;
    } else
#endif
    {
        if (!init_buffer_params_fallback(xwl_pixmap,
                                         &modifier,
                                         &num_planes,
                                         prime_fds,
                                         strides,
                                         offsets))
            return NULL;
    }

    if (xwl_screen->dmabuf &&
        xwl_glamor_is_modifier_supported(xwl_screen, format, modifier)) {
        struct zwp_linux_buffer_params_v1 *params;

        params = zwp_linux_dmabuf_v1_create_params(xwl_screen->dmabuf);
        for (i = 0; i < num_planes; i++) {
            zwp_linux_buffer_params_v1_add(params, prime_fds[i], i,
                                           offsets[i], strides[i],
                                           modifier >> 32, modifier & 0xffffffff);
        }

        xwl_pixmap->buffer =
           zwp_linux_buffer_params_v1_create_immed(params, width, height,
                                                   format, 0);
        zwp_linux_buffer_params_v1_destroy(params);
    } else if (num_planes == 1 && modifier == DRM_FORMAT_MOD_INVALID &&
               xwl_gbm->drm) {
        xwl_pixmap->buffer =
            wl_drm_create_prime_buffer(xwl_gbm->drm, prime_fds[0], width, height,
                                       format,
                                       0, gbm_bo_get_stride(xwl_pixmap->bo),
                                       0, 0,
                                       0, 0);
    }

    for (i = 0; i < num_planes; i++)
        close(prime_fds[i]);

    /* Add our listener now */
    if (xwl_pixmap->buffer)
        wl_buffer_add_listener(xwl_pixmap->buffer,
                               &xwl_glamor_gbm_buffer_listener, pixmap);

    return xwl_pixmap->buffer;
}

static void
xwl_glamor_gbm_cleanup(struct xwl_screen *xwl_screen)
{
    struct xwl_gbm_private *xwl_gbm = xwl_gbm_get(xwl_screen);

    if (!xwl_gbm)
        return;

    /* Cannot use GBM after clean-up, disable GLAMOR support from now on */
    ErrorF("XWAYLAND: Disabling GLAMOR support\n");
    xwl_screen->glamor = XWL_GLAMOR_NONE;

    if (xwl_gbm->device_name)
        free(xwl_gbm->device_name);
    drmFreeDevice(&xwl_gbm->device);
    if (xwl_gbm->drm_fd)
        close(xwl_gbm->drm_fd);
    if (xwl_gbm->drm)
        wl_drm_destroy(xwl_gbm->drm);
    if (xwl_gbm->gbm)
        gbm_device_destroy(xwl_gbm->gbm);
    if (xwl_screen->explicit_sync)
        wp_linux_drm_syncobj_manager_v1_destroy(xwl_screen->explicit_sync);

    dixSetPrivate(&xwl_screen->screen->devPrivates, &xwl_gbm_private_key,
                  NULL);
    free(xwl_gbm);
}

struct xwl_auth_state {
    int fd;
    ClientPtr client;
    struct wl_callback *callback;
};

static void
free_xwl_auth_state(ClientPtr pClient, struct xwl_auth_state *state)
{
    dixSetPrivate(&pClient->devPrivates, &xwl_auth_state_private_key, NULL);
    if (state) {
        wl_callback_destroy(state->callback);
        free(state);
    }
}

static void
xwl_auth_state_client_callback(CallbackListPtr *pcbl, void *unused, void *data)
{
    NewClientInfoRec *clientinfo = (NewClientInfoRec *) data;
    ClientPtr pClient = clientinfo->client;
    struct xwl_auth_state *state;

    switch (pClient->clientState) {
    case ClientStateGone:
    case ClientStateRetained:
        state = dixLookupPrivate(&pClient->devPrivates,
                                 &xwl_auth_state_private_key);
        free_xwl_auth_state(pClient, state);
        break;
    default:
        break;
    }
}

#ifdef DRI3
static void
sync_callback(void *data, struct wl_callback *callback, uint32_t serial)
{
    struct xwl_auth_state *state = data;
    ClientPtr client = state->client;

    /* if the client is gone, the callback is cancelled so it's safe to
     * assume the client is still in ClientStateRunning at this point...
     */
    dri3_send_open_reply(client, state->fd);
    AttendClient(client);
    free_xwl_auth_state(client, state);
}

static const struct wl_callback_listener sync_listener = {
   sync_callback
};

static int
xwl_dri3_open_client(ClientPtr client,
                     ScreenPtr screen,
                     RRProviderPtr provider,
                     int *pfd)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(screen);
    struct xwl_gbm_private *xwl_gbm = xwl_gbm_get(xwl_screen);
    drm_magic_t magic;
    int fd;

    fd = open(xwl_gbm->device_name, O_RDWR | O_CLOEXEC);
    if (fd < 0)
        return BadAlloc;
    if (xwl_gbm->fd_render_node) {
        *pfd = fd;
        return Success;
    }

    struct xwl_auth_state *state = calloc(1, sizeof(struct xwl_auth_state));
    if (state == NULL) {
        close(fd);
        return BadAlloc;
    }

    state->client = client;
    state->fd = fd;

    if (drmGetMagic(state->fd, &magic) < 0) {
        close(state->fd);
        free(state);
        return BadMatch;
    }

    wl_drm_authenticate(xwl_gbm->drm, magic);
    state->callback = wl_display_sync(xwl_screen->display);
    wl_callback_add_listener(state->callback, &sync_listener, state);
    dixSetPrivate(&client->devPrivates, &xwl_auth_state_private_key, state);

    IgnoreClient(client);

    return Success;
}
#endif /* DRI3 */

PixmapPtr
glamor_pixmap_from_fds(ScreenPtr screen, CARD8 num_fds, const int *fds,
                       CARD16 width, CARD16 height,
                       const CARD32 *strides, const CARD32 *offsets,
                       CARD8 depth, CARD8 bpp, uint64_t modifier)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(screen);
    struct xwl_gbm_private *xwl_gbm = xwl_gbm_get(xwl_screen);
    struct gbm_bo *bo = NULL;
    PixmapPtr pixmap;
    int i;
    Bool implicit = FALSE;

    if (width == 0 || height == 0 || num_fds == 0 ||
        depth < 15 || bpp != BitsPerPixel(depth) ||
        strides[0] < width * bpp / 8)
       goto error;

    if (xwl_gbm->dmabuf_capable && modifier != DRM_FORMAT_MOD_INVALID) {
#ifdef GBM_BO_WITH_MODIFIERS
       struct gbm_import_fd_modifier_data data;

       data.width = width;
       data.height = height;
       data.num_fds = num_fds;
       data.format = gbm_format_for_depth(depth, xwl_gbm->glamor_gles);
       data.modifier = modifier;
       for (i = 0; i < num_fds; i++) {
          data.fds[i] = fds[i];
          data.strides[i] = strides[i];
          data.offsets[i] = offsets[i];
       }
       bo = gbm_bo_import(xwl_gbm->gbm, GBM_BO_IMPORT_FD_MODIFIER, &data,
                          GBM_BO_USE_RENDERING);
#endif
    } else if (num_fds == 1) {
       struct gbm_import_fd_data data;

       data.fd = fds[0];
       data.width = width;
       data.height = height;
       data.stride = strides[0];
       data.format = gbm_format_for_depth(depth, xwl_gbm->glamor_gles);
       bo = gbm_bo_import(xwl_gbm->gbm, GBM_BO_IMPORT_FD, &data,
                          GBM_BO_USE_RENDERING);
       implicit = TRUE;
    } else {
       goto error;
    }

    if (bo == NULL)
       goto error;

    pixmap = xwl_glamor_gbm_create_pixmap_for_bo(screen, bo, depth, implicit);
    if (pixmap == NULL) {
       gbm_bo_destroy(bo);
       goto error;
    }

    return pixmap;

error:
    return NULL;
}

int
glamor_egl_fds_from_pixmap(ScreenPtr screen, PixmapPtr pixmap, int *fds,
                           uint32_t *strides, uint32_t *offsets,
                           uint64_t *modifier)
{
    struct xwl_pixmap *xwl_pixmap;
#ifdef GBM_BO_WITH_MODIFIERS
#ifndef GBM_BO_FD_FOR_PLANE
    int32_t first_handle;
#endif
    uint32_t num_fds;
    int i;
#endif

    xwl_pixmap = xwl_pixmap_get(pixmap);

    if (xwl_pixmap == NULL)
       return 0;

    if (!xwl_pixmap->bo)
       return 0;

#ifdef GBM_BO_WITH_MODIFIERS
    num_fds = gbm_bo_get_plane_count(xwl_pixmap->bo);
    *modifier = gbm_bo_get_modifier(xwl_pixmap->bo);

    for (i = 0; i < num_fds; i++) {
#ifdef GBM_BO_FD_FOR_PLANE
        fds[i] = gbm_bo_get_fd_for_plane(xwl_pixmap->bo, i);
#else
        union gbm_bo_handle plane_handle;

        plane_handle = gbm_bo_get_handle_for_plane(xwl_pixmap->bo, i);
        if (i == 0)
            first_handle = plane_handle.s32;

        /* If all planes point to the same object as the first plane, i.e. they
         * all have the same handle, we can fall back to the non-planar
         * gbm_bo_get_fd without losing information. If they point to different
         * objects we are out of luck and need to give up.
         */
        if (first_handle == plane_handle.s32)
            fds[i] = gbm_bo_get_fd(xwl_pixmap->bo);
        else
            fds[i] = -1;
#endif
        if (fds[i] == -1) {
            while (--i >= 0)
                close(fds[i]);
            return 0;
        }
        strides[i] = gbm_bo_get_stride_for_plane(xwl_pixmap->bo, i);
        offsets[i] = gbm_bo_get_offset(xwl_pixmap->bo, i);
    }

    return num_fds;
#else
    *modifier = DRM_FORMAT_MOD_INVALID;
    fds[0] = gbm_bo_get_fd(xwl_pixmap->bo);
    if (fds[0] == -1)
        return 0;
    strides[0] = gbm_bo_get_stride(xwl_pixmap->bo);
    offsets[0] = 0;
    return 1;
#endif
}

/* Not actually used, just defined here so there's something for
 * _glamor_egl_fds_from_pixmap() to link against
 */
int
glamor_egl_fd_from_pixmap(ScreenPtr screen, PixmapPtr pixmap,
                          CARD16 *stride, CARD32 *size)
{
    return -1;
}

int
xwl_glamor_dmabuf_export_sync_file(PixmapPtr pixmap)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(pixmap->drawable.pScreen);
    struct xwl_pixmap *xwl_pixmap;
    int num_planes;
    int sync_file = -1;
    int p;
#ifndef GBM_BO_FD_FOR_PLANE
    int32_t first_handle;
#endif

    if (!xwl_screen->glamor)
        return -1;

#ifdef DMA_BUF_IOCTL_EXPORT_SYNC_FILE
    xwl_pixmap = xwl_pixmap_get(pixmap);
    num_planes = gbm_bo_get_plane_count(xwl_pixmap->bo);

    for (p = 0; p < num_planes; ++p) {
        int plane_fd;
#ifdef GBM_BO_FD_FOR_PLANE
        plane_fd = gbm_bo_get_fd_for_plane(xwl_pixmap->bo, p);
#else
        union gbm_bo_handle plane_handle =
            gbm_bo_get_handle_for_plane(xwl_pixmap->bo, p);
        if (p == 0)
            first_handle = plane_handle.s32;

        if (plane_handle.s32 == first_handle)
            plane_fd = gbm_bo_get_fd(xwl_pixmap->bo);
        else
            continue;
#endif /* GBM_BO_FD_FOR_PLANE */
        struct dma_buf_export_sync_file export_args = { 0 };
        export_args.fd = -1;
        export_args.flags = DMA_BUF_SYNC_READ;
        drmIoctl(plane_fd, DMA_BUF_IOCTL_EXPORT_SYNC_FILE, &export_args);
        close(plane_fd);
        if (sync_file == -1) {
            sync_file = export_args.fd;
        } else {
            struct sync_merge_data merge_args = { 0 };
            merge_args.fd2 = export_args.fd;
            ioctl(sync_file, SYNC_IOC_MERGE, &merge_args);
            close(export_args.fd);
            close(sync_file);
            sync_file = merge_args.fence;
        }
    }
#endif /* DMA_BUF_IOCTL_EXPORT_SYNC_FILE */
    return sync_file;
}

void
xwl_glamor_dmabuf_import_sync_file(PixmapPtr pixmap, int sync_file)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(pixmap->drawable.pScreen);
    struct xwl_pixmap *xwl_pixmap;
    int num_planes;
    int p;
#ifndef GBM_BO_FD_FOR_PLANE
    int32_t first_handle;
#endif

    if (!xwl_screen->glamor)
        return;

#ifdef DMA_BUF_IOCTL_IMPORT_SYNC_FILE
    xwl_pixmap = xwl_pixmap_get(pixmap);
    num_planes = gbm_bo_get_plane_count(xwl_pixmap->bo);

    for (p = 0; p < num_planes; ++p) {
        int plane_fd;
#ifdef GBM_BO_FD_FOR_PLANE
        plane_fd = gbm_bo_get_fd_for_plane(xwl_pixmap->bo, p);
#else
        union gbm_bo_handle plane_handle =
            gbm_bo_get_handle_for_plane(xwl_pixmap->bo, p);
        if (p == 0)
            first_handle = plane_handle.s32;

        if (plane_handle.s32 == first_handle)
            plane_fd = gbm_bo_get_fd(xwl_pixmap->bo);
        else
            continue;
#endif /* GBM_BO_FD_FOR_PLANE */
        struct dma_buf_import_sync_file import_args = { 0 };
        import_args.fd = sync_file;
        import_args.flags = DMA_BUF_SYNC_WRITE;
        drmIoctl(plane_fd, DMA_BUF_IOCTL_IMPORT_SYNC_FILE, &import_args);
        close(plane_fd);
    }
#endif /* DMA_BUF_IOCTL_IMPORT_SYNC_FILE */
    close(sync_file);
}

#ifdef DRI3
struct xwl_dri3_syncobj
{
    struct dri3_syncobj base;
    uint32_t handle;
    struct wp_linux_drm_syncobj_timeline_v1 *timeline;
};

void
xwl_glamor_dri3_syncobj_passthrough(struct xwl_window *xwl_window,
                                    struct dri3_syncobj *acquire_syncobj,
                                    struct dri3_syncobj *release_syncobj,
                                    uint64_t acquire_point,
                                    uint64_t release_point)
{
    struct xwl_screen *xwl_screen = xwl_window->xwl_screen;
    struct xwl_dri3_syncobj *xwl_acquire_syncobj = (struct xwl_dri3_syncobj *)acquire_syncobj;
    struct xwl_dri3_syncobj *xwl_release_syncobj = (struct xwl_dri3_syncobj *)release_syncobj;
    uint32_t acquire_hi = acquire_point >> 32;
    uint32_t acquire_lo = acquire_point & 0xffffffff;
    uint32_t release_hi = release_point >> 32;
    uint32_t release_lo = release_point & 0xffffffff;

    if (!xwl_window->surface_sync)
        xwl_window->surface_sync =
            wp_linux_drm_syncobj_manager_v1_get_surface(xwl_screen->explicit_sync,
                                                        xwl_window->surface);

    wp_linux_drm_syncobj_surface_v1_set_acquire_point(xwl_window->surface_sync,
                                                      xwl_acquire_syncobj->timeline,
                                                      acquire_hi, acquire_lo);
    wp_linux_drm_syncobj_surface_v1_set_release_point(xwl_window->surface_sync,
                                                      xwl_release_syncobj->timeline,
                                                      release_hi, release_lo);
}

static Bool
xwl_dri3_check_syncobj(struct dri3_syncobj *syncobj, uint64_t point, Bool check_avail)
{
    struct xwl_dri3_syncobj *xwl_syncobj = (struct xwl_dri3_syncobj *)syncobj;
    struct xwl_screen *xwl_screen = xwl_screen_get(syncobj->screen);
    struct xwl_gbm_private *xwl_gbm = xwl_gbm_get(xwl_screen);

    return !drmSyncobjTimelineWait(xwl_gbm->drm_fd,
                                   &xwl_syncobj->handle, &point, 1,
                                   0 /* timeout */,
                                   check_avail ?
                                   DRM_SYNCOBJ_WAIT_FLAGS_WAIT_AVAILABLE :
                                   DRM_SYNCOBJ_WAIT_FLAGS_WAIT_FOR_SUBMIT,
                                   NULL /* first_signaled */);
}

static Bool
xwl_dri3_syncobj_has_fence(struct dri3_syncobj *syncobj, uint64_t point)
{
    return xwl_dri3_check_syncobj(syncobj, point, TRUE /* check_avail */);
}

static Bool
xwl_dri3_syncobj_is_signaled(struct dri3_syncobj *syncobj, uint64_t point)
{
    return xwl_dri3_check_syncobj(syncobj, point, FALSE /* check_avail */);
}

static int
xwl_dri3_syncobj_export_fence(struct dri3_syncobj *syncobj, uint64_t point)
{
    struct xwl_dri3_syncobj *xwl_syncobj = (struct xwl_dri3_syncobj *)syncobj;
    struct xwl_screen *xwl_screen = xwl_screen_get(syncobj->screen);
    struct xwl_gbm_private *xwl_gbm = xwl_gbm_get(xwl_screen);
    uint32_t temp_syncobj;
    int fd = -1;

    drmSyncobjCreate(xwl_gbm->drm_fd, 0, &temp_syncobj);
    drmSyncobjTransfer(xwl_gbm->drm_fd, temp_syncobj, 0,
                       xwl_syncobj->handle, point, 0);
    drmSyncobjExportSyncFile(xwl_gbm->drm_fd, temp_syncobj, &fd);
    drmSyncobjDestroy(xwl_gbm->drm_fd, temp_syncobj);
    return fd;
}

static void
xwl_dri3_syncobj_import_fence(struct dri3_syncobj *syncobj,
                              uint64_t point, int fd)
{
    struct xwl_dri3_syncobj *xwl_syncobj = (struct xwl_dri3_syncobj *)syncobj;
    struct xwl_screen *xwl_screen = xwl_screen_get(syncobj->screen);
    struct xwl_gbm_private *xwl_gbm = xwl_gbm_get(xwl_screen);
    uint32_t temp_syncobj;

    drmSyncobjCreate(xwl_gbm->drm_fd, 0, &temp_syncobj);
    drmSyncobjImportSyncFile(xwl_gbm->drm_fd, temp_syncobj, fd);
    drmSyncobjTransfer(xwl_gbm->drm_fd, xwl_syncobj->handle, point,
                       temp_syncobj, 0, 0);
    drmSyncobjDestroy(xwl_gbm->drm_fd, temp_syncobj);
    close(fd);
}

static void
xwl_dri3_signal_syncobj(struct dri3_syncobj *syncobj, uint64_t point)
{
    struct xwl_dri3_syncobj *xwl_syncobj = (struct xwl_dri3_syncobj *)syncobj;
    struct xwl_screen *xwl_screen = xwl_screen_get(syncobj->screen);
    struct xwl_gbm_private *xwl_gbm = xwl_gbm_get(xwl_screen);

    drmSyncobjTimelineSignal(xwl_gbm->drm_fd, &xwl_syncobj->handle, &point, 1);
}

static void
xwl_dri3_free_syncobj(struct dri3_syncobj *syncobj)
{
    struct xwl_dri3_syncobj *xwl_syncobj = (struct xwl_dri3_syncobj *)syncobj;
    struct xwl_screen *xwl_screen = xwl_screen_get(syncobj->screen);
    struct xwl_gbm_private *xwl_gbm = xwl_gbm_get(xwl_screen);

    if (xwl_syncobj->timeline)
        wp_linux_drm_syncobj_timeline_v1_destroy(xwl_syncobj->timeline);

    if (xwl_syncobj->handle)
        drmSyncobjDestroy(xwl_gbm->drm_fd, xwl_syncobj->handle);

    free(xwl_syncobj);
}

static void
xwl_dri3_syncobj_eventfd(struct dri3_syncobj *syncobj, uint64_t point,
                         int efd, Bool wait_avail)
{
    struct xwl_dri3_syncobj *xwl_syncobj = (struct xwl_dri3_syncobj *)syncobj;
    struct xwl_screen *xwl_screen = xwl_screen_get(syncobj->screen);
    struct xwl_gbm_private *xwl_gbm = xwl_gbm_get(xwl_screen);

    drmSyncobjEventfd(xwl_gbm->drm_fd, xwl_syncobj->handle, point, efd,
                      wait_avail ? DRM_SYNCOBJ_WAIT_FLAGS_WAIT_AVAILABLE : 0);
}

static void
xwl_dri3_syncobj_submitted_eventfd(struct dri3_syncobj *syncobj,
                                   uint64_t point, int efd)
{
    xwl_dri3_syncobj_eventfd(syncobj, point, efd, TRUE /* wait_avail */);
}

static void
xwl_dri3_syncobj_signaled_eventfd(struct dri3_syncobj *syncobj,
                                  uint64_t point, int efd)
{
    xwl_dri3_syncobj_eventfd(syncobj, point, efd, FALSE /* wait_avail */);
}

static struct dri3_syncobj *
xwl_dri3_create_syncobj(struct xwl_screen *xwl_screen, uint32_t handle)
{
    struct xwl_dri3_syncobj *syncobj = calloc(1, sizeof (*syncobj));
    struct xwl_gbm_private *xwl_gbm = xwl_gbm_get(xwl_screen);
    Bool create = !handle;

    if (!syncobj)
        return NULL;

    if (create && drmSyncobjCreate(xwl_gbm->drm_fd, 0, &handle))
        goto fail;

    if (xwl_screen->explicit_sync) {
        int syncobj_fd = -1;
        if (drmSyncobjHandleToFD(xwl_gbm->drm_fd, handle, &syncobj_fd))
            goto fail;

        syncobj->timeline =
            wp_linux_drm_syncobj_manager_v1_import_timeline(xwl_screen->explicit_sync,
                                                            syncobj_fd);
        close(syncobj_fd);
        if (!syncobj->timeline)
            goto fail;
    }

    syncobj->handle = handle;
    syncobj->base.screen = xwl_screen->screen;
    syncobj->base.refcount = 1;

    syncobj->base.free = xwl_dri3_free_syncobj;
    syncobj->base.has_fence = xwl_dri3_syncobj_has_fence;
    syncobj->base.is_signaled = xwl_dri3_syncobj_is_signaled;
    syncobj->base.export_fence = xwl_dri3_syncobj_export_fence;
    syncobj->base.import_fence = xwl_dri3_syncobj_import_fence;
    syncobj->base.signal = xwl_dri3_signal_syncobj;
    syncobj->base.signaled_eventfd = xwl_dri3_syncobj_signaled_eventfd;
    syncobj->base.submitted_eventfd = xwl_dri3_syncobj_submitted_eventfd;
    return &syncobj->base;

fail:
    if (create && handle)
        drmSyncobjDestroy(xwl_gbm->drm_fd, handle);
    free(syncobj);
    return NULL;
}

struct dri3_syncobj *
xwl_glamor_dri3_syncobj_create(struct xwl_screen *xwl_screen)
{
    return xwl_dri3_create_syncobj(xwl_screen, 0 /* allocate new handle */);
}

static struct dri3_syncobj *
xwl_dri3_import_syncobj(ClientPtr client, ScreenPtr screen, XID id, int fd)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(screen);
    struct xwl_gbm_private *xwl_gbm = xwl_gbm_get(xwl_screen);
    struct xwl_dri3_syncobj *syncobj = NULL;
    uint32_t handle;

    if (drmSyncobjFDToHandle(xwl_gbm->drm_fd, fd, &handle))
        return NULL;

    syncobj = (struct xwl_dri3_syncobj *)xwl_dri3_create_syncobj(xwl_screen, handle);
    if (!syncobj) {
        drmSyncobjDestroy(xwl_gbm->drm_fd, handle);
        return NULL;
    }

    syncobj->base.id = id;

    return &syncobj->base;
}

static Bool
xwl_gbm_supports_syncobjs(struct xwl_screen *xwl_screen)
{
    struct xwl_gbm_private *xwl_gbm = xwl_gbm_get(xwl_screen);
    uint64_t syncobj_cap = 0;

    if (drmGetCap(xwl_gbm->drm_fd, DRM_CAP_SYNCOBJ_TIMELINE,
                  &syncobj_cap) || !syncobj_cap)
        return FALSE;

    /* Check if syncobj eventfd is supported. */
    drmSyncobjEventfd(xwl_gbm->drm_fd, 0, 0, -1, 0);
    if (errno != ENOENT)
        return FALSE;

#if !defined(DMA_BUF_IOCTL_EXPORT_SYNC_FILE) || \
    !defined(DMA_BUF_IOCTL_IMPORT_SYNC_FILE)
    return FALSE;
#else
    return TRUE;
#endif
}

static dri3_screen_info_rec xwl_dri3_info = {
    .version = 2,
    .open = NULL,
    .pixmap_from_fds = glamor_pixmap_from_fds,
    .fds_from_pixmap = glamor_fds_from_pixmap,
    .open_client = xwl_dri3_open_client,
    .get_formats = xwl_glamor_get_formats,
    .get_modifiers = xwl_glamor_get_modifiers,
    .get_drawable_modifiers = xwl_glamor_get_drawable_modifiers,
    .import_syncobj = NULL, /* need to check for kernel support */
};
#endif /* DRI3 */

static const char *
get_render_node_path_for_device(const drmDevicePtr drm_device,
                                const char *device_path)
{
    const char *render_node_path;
    int i;

    if (!(drm_device->available_nodes & (1 << DRM_NODE_RENDER)))
        return NULL;
    render_node_path = drm_device->nodes[DRM_NODE_RENDER];

    for (i = 0; i < DRM_NODE_MAX; i++) {
        if (!(drm_device->available_nodes & (1 << i)))
            continue;
        if (strcmp(device_path, drm_device->nodes[i]) == 0)
            return render_node_path;
    }

    return NULL;
}

static char *
get_render_node_path(const char *device_path)
{
    drmDevicePtr *devices = NULL;
    char *render_node_path = NULL;
    int i, n_devices, max_devices;

    max_devices = drmGetDevices2(0, NULL, 0);
    if (max_devices <= 0)
        goto out;

    devices = calloc(max_devices, sizeof(drmDevicePtr));
    if (!devices)
        goto out;

    n_devices = drmGetDevices2(0, devices, max_devices);
    if (n_devices < 0)
        goto out;

    for (i = 0; i < n_devices; i++) {
       const char *node_path = get_render_node_path_for_device(devices[i],
                                                               device_path);
       if (node_path) {
           render_node_path = strdup(node_path);
           break;
       }
    }

out:
    free(devices);
    return render_node_path;
}

static void
xwl_drm_handle_device(void *data, struct wl_drm *drm, const char *device)
{
   struct xwl_screen *xwl_screen = data;
   struct xwl_gbm_private *xwl_gbm = xwl_gbm_get(xwl_screen);
   drm_magic_t magic;
   char *render_node_path = NULL;

   if (!is_device_path_render_node(device))
       render_node_path = get_render_node_path(device);

   if (render_node_path)
       xwl_gbm->device_name = render_node_path;
   else
       xwl_gbm->device_name = strdup(device);

   if (!xwl_gbm->device_name) {
       xwl_glamor_gbm_cleanup(xwl_screen);
       xwl_screen->expecting_event--;
       return;
   }

   xwl_gbm->drm_fd = open(xwl_gbm->device_name, O_RDWR | O_CLOEXEC);
   if (xwl_gbm->drm_fd == -1) {
       ErrorF("wayland-egl: could not open %s (%s)\n",
              xwl_gbm->device_name, strerror(errno));
       xwl_glamor_gbm_cleanup(xwl_screen);
       xwl_screen->expecting_event--;
       return;
   }

   if (drmGetDevice2(xwl_gbm->drm_fd, 0, &xwl_gbm->device) != 0) {
       ErrorF("wayland-egl: Could not fetch DRM device %s\n",
              xwl_gbm->device_name);
       xwl_screen->expecting_event--;
       return;
   }

   if (drmGetNodeTypeFromFd(xwl_gbm->drm_fd) == DRM_NODE_RENDER) {
       xwl_gbm->fd_render_node = TRUE;
       xwl_screen->expecting_event--;
   } else {
       drmGetMagic(xwl_gbm->drm_fd, &magic);
       wl_drm_authenticate(xwl_gbm->drm, magic);
   }
}

static void
xwl_drm_handle_format(void *data, struct wl_drm *drm, uint32_t format)
{
}

static void
xwl_drm_handle_authenticated(void *data, struct wl_drm *drm)
{
    struct xwl_screen *xwl_screen = data;
    struct xwl_gbm_private *xwl_gbm = xwl_gbm_get(xwl_screen);

    xwl_gbm->drm_authenticated = TRUE;
    xwl_screen->expecting_event--;
}

static void
xwl_drm_handle_capabilities(void *data, struct wl_drm *drm, uint32_t value)
{
    xwl_gbm_get(data)->capabilities = value;
}

static const struct wl_drm_listener xwl_drm_listener = {
    xwl_drm_handle_device,
    xwl_drm_handle_format,
    xwl_drm_handle_authenticated,
    xwl_drm_handle_capabilities
};

Bool
xwl_screen_set_drm_interface(struct xwl_screen *xwl_screen,
                             uint32_t id, uint32_t version)
{
    struct xwl_gbm_private *xwl_gbm = xwl_gbm_get(xwl_screen);

    if (version < 2)
        return FALSE;

    xwl_gbm->drm =
        wl_registry_bind(xwl_screen->registry, id, &wl_drm_interface, 2);
    wl_drm_add_listener(xwl_gbm->drm, &xwl_drm_listener, xwl_screen);
    xwl_screen->expecting_event++;

    return TRUE;
}

Bool
xwl_screen_set_syncobj_interface(struct xwl_screen *xwl_screen,
                                 uint32_t id, uint32_t version)
{
    xwl_screen->explicit_sync =
        wl_registry_bind(xwl_screen->registry, id,
                         &wp_linux_drm_syncobj_manager_v1_interface,
                         version);
    return TRUE;
}

static Bool
xwl_glamor_gbm_has_egl_extension(void)
{
    return (epoxy_has_egl_extension(NULL, "EGL_MESA_platform_gbm") ||
            epoxy_has_egl_extension(NULL, "EGL_KHR_platform_gbm"));
}

#ifdef DRI3
static void
xwl_glamor_gbm_release_fence_avail(int fd, int xevents, void *data)
{
    struct xwl_pixmap *xwl_pixmap = data;
    struct xwl_window_buffer *xwl_window_buffer = xwl_pixmap->xwl_window_buffer;

    SetNotifyFd(fd, NULL, 0, NULL);
    close(fd);
    xwl_pixmap->efd = -1;

    xwl_window_buffer_release(xwl_window_buffer);
}
#endif /* DRI3 */

Bool
xwl_glamor_supports_implicit_sync(struct xwl_screen *xwl_screen)
{
    /* absent glamor, implicit sync is irrelevant so just return TRUE */
    return !xwl_screen->glamor ||
        xwl_gbm_get(xwl_screen)->implicit_sync;
}

Bool
xwl_glamor_supports_syncobjs(struct xwl_screen *xwl_screen)
{
    return xwl_screen->glamor &&
        xwl_gbm_get(xwl_screen)->supports_syncobjs;
}

Bool
xwl_glamor_gbm_set_syncpts(struct xwl_window *xwl_window, PixmapPtr pixmap)
{
#ifdef DRI3
    struct xwl_screen *xwl_screen = xwl_window->xwl_screen;
    struct xwl_pixmap *xwl_pixmap = xwl_pixmap_get(pixmap);
    uint64_t acquire_point;
    uint64_t release_point;
    int fence_fd;

    if (!xwl_screen->glamor)
        return FALSE;

    if (!xwl_pixmap) {
        ErrorF("XWAYLAND: Failed to set synchronization point, no backing xwl_pixmap!\n");
        return FALSE;
    }

    acquire_point = ++xwl_pixmap->timeline_point;
    release_point = ++xwl_pixmap->timeline_point;

    if (!xwl_pixmap->syncobj) {
        struct dri3_syncobj *syncobj = xwl_glamor_dri3_syncobj_create(xwl_screen);
        if (!syncobj)
            goto fail;
        xwl_pixmap->syncobj = syncobj;
    }

    fence_fd = xwl_glamor_get_fence(xwl_screen);
    if (fence_fd >= 0)
        xwl_pixmap->syncobj->import_fence(xwl_pixmap->syncobj, acquire_point, fence_fd);
    else
        goto fail;

    xwl_glamor_dri3_syncobj_passthrough(xwl_window,
                                        xwl_pixmap->syncobj,
                                        xwl_pixmap->syncobj,
                                        acquire_point,
                                        release_point);
    return TRUE;

fail:
    /* can't use explicit sync, we will do a glFinish() before presenting */
    if (xwl_pixmap->syncobj) {
        xwl_pixmap->syncobj->free(xwl_pixmap->syncobj);
        xwl_pixmap->syncobj = NULL;
    }
#endif /* DRI3 */
    return FALSE;
}

void
xwl_glamor_gbm_dispose_syncpts(PixmapPtr pixmap)
{
#ifdef DRI3
    struct xwl_pixmap *xwl_pixmap = xwl_pixmap_get(pixmap);
    struct xwl_screen *xwl_screen = xwl_screen_get(pixmap->drawable.pScreen);

    if (!xwl_screen->glamor || !xwl_pixmap)
        return;

    if (xwl_pixmap->syncobj) {
        xwl_pixmap->syncobj->free(xwl_pixmap->syncobj);
        xwl_pixmap->syncobj = NULL;
    }

    if (xwl_pixmap->efd >= 0) {
        SetNotifyFd(xwl_pixmap->efd, NULL, 0, NULL);
        close(xwl_pixmap->efd);
    }
#endif /* DRI3 */
}

void
xwl_glamor_gbm_wait_syncpts(PixmapPtr pixmap)
{
#ifdef DRI3
    struct xwl_screen *xwl_screen = xwl_screen_get(pixmap->drawable.pScreen);
    struct xwl_pixmap *xwl_pixmap = xwl_pixmap_get(pixmap);

    if (!xwl_screen->glamor || !xwl_pixmap)
        return;

    if (xwl_pixmap->syncobj) {
        int fence_fd = xwl_pixmap->syncobj->export_fence(xwl_pixmap->syncobj,
                                                         xwl_pixmap->timeline_point);

        xwl_glamor_wait_fence(xwl_screen, fence_fd);
    }
#endif /* DRI3 */
}

void
xwl_glamor_gbm_wait_release_fence(struct xwl_window *xwl_window,
                                  PixmapPtr pixmap,
                                  struct xwl_window_buffer *xwl_window_buffer)
{
#ifdef DRI3
    struct xwl_pixmap *xwl_pixmap = xwl_pixmap_get(pixmap);
    struct xwl_screen *xwl_screen = xwl_screen_get(pixmap->drawable.pScreen);

    if (!xwl_screen->glamor || !xwl_pixmap || !xwl_window_buffer)
        return;

    xwl_pixmap->xwl_window_buffer = xwl_window_buffer;
    /* wait until the release fence is available before re-using this buffer */
    xwl_pixmap->efd = eventfd(0, EFD_CLOEXEC);
    SetNotifyFd(xwl_pixmap->efd, xwl_glamor_gbm_release_fence_avail, X_NOTIFY_READ,
                xwl_pixmap);
    xwl_pixmap->syncobj->submitted_eventfd(xwl_pixmap->syncobj,
                                           xwl_pixmap->timeline_point,
                                           xwl_pixmap->efd);
#endif /* DRI3 */
}

static Bool
xwl_glamor_try_to_make_context_current(struct xwl_screen *xwl_screen)
{
    if (xwl_screen->egl_context == EGL_NO_CONTEXT)
        return FALSE;

    return eglMakeCurrent(xwl_screen->egl_display, EGL_NO_SURFACE,
                          EGL_NO_SURFACE, xwl_screen->egl_context);
}

static void
xwl_glamor_maybe_destroy_context(struct xwl_screen *xwl_screen)
{
    if (xwl_screen->egl_context == EGL_NO_CONTEXT)
        return;

   eglMakeCurrent(xwl_screen->egl_display, EGL_NO_SURFACE,
                  EGL_NO_SURFACE, EGL_NO_CONTEXT);
   eglDestroyContext(xwl_screen->egl_display, xwl_screen->egl_context);
   xwl_screen->egl_context = EGL_NO_CONTEXT;
}

static Bool
xwl_glamor_try_big_gl_api(struct xwl_screen *xwl_screen)
{
    static const EGLint config_attribs_core[] = {
        EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,
        EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
        EGL_CONTEXT_MAJOR_VERSION_KHR,
        GLAMOR_GL_CORE_VER_MAJOR,
        EGL_CONTEXT_MINOR_VERSION_KHR,
        GLAMOR_GL_CORE_VER_MINOR,
        EGL_NONE
    };
    int gl_version;

    if (!(xwl_screen->glamor & XWL_GLAMOR_GL))
        return FALSE;

    eglBindAPI(EGL_OPENGL_API);

    xwl_screen->egl_context =
        eglCreateContext(xwl_screen->egl_display, EGL_NO_CONFIG_KHR,
                         EGL_NO_CONTEXT, config_attribs_core);

    if (xwl_screen->egl_context == EGL_NO_CONTEXT)
        xwl_screen->egl_context =
            eglCreateContext(xwl_screen->egl_display, EGL_NO_CONFIG_KHR,
                             EGL_NO_CONTEXT, NULL);

    if (!xwl_glamor_try_to_make_context_current(xwl_screen)) {
        ErrorF("Failed to make EGL context current with GL\n");
        xwl_glamor_maybe_destroy_context(xwl_screen);
        return FALSE;
    }

    /* glamor needs at least GL 2.1, if the GL version is less than 2.1,
     * drop the context we created, it's useless.
     */
    gl_version = epoxy_gl_version();
    if (gl_version < 21) {
        ErrorF("Supported GL version is not sufficient (required 21, found %i)\n",
               gl_version);
        xwl_glamor_maybe_destroy_context(xwl_screen);
        return FALSE;
    }

    return TRUE;
}

static Bool
xwl_glamor_try_gles_api(struct xwl_screen *xwl_screen)
{
    const EGLint gles_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION,
        2,
        EGL_NONE,
    };

    if (!(xwl_screen->glamor & XWL_GLAMOR_GLES))
        return FALSE;

    eglBindAPI(EGL_OPENGL_ES_API);

    xwl_screen->egl_context = eglCreateContext(xwl_screen->egl_display,
                                               EGL_NO_CONFIG_KHR,
                                               EGL_NO_CONTEXT, gles_attribs);

    if (!xwl_glamor_try_to_make_context_current(xwl_screen)) {
        ErrorF("Failed to make EGL context current with GLES2\n");
        xwl_glamor_maybe_destroy_context(xwl_screen);
        return FALSE;
    }

    return TRUE;
}

static Bool
xwl_glamor_gbm_init_main_dev(struct xwl_screen *xwl_screen)
{
    struct xwl_gbm_private *xwl_gbm = xwl_gbm_get(xwl_screen);
    drmDevice *main_dev;

    while (!xwl_screen->default_feedback.feedback_done) {
        if (wl_display_dispatch(xwl_screen->display) < 0) {
            ErrorF("Failed to dispatch Wayland display\n");
            return FALSE;
        }
    }

    main_dev = xwl_screen->default_feedback.main_dev;
    if (!main_dev) {
        ErrorF("No main linux-dmabuf device advertised by compositor\n");
        return FALSE;
    }

    if (!(main_dev->available_nodes & (1 << DRM_NODE_RENDER))) {
        ErrorF("Main linux-dmabuf device has no render node\n");
        return FALSE;
    }

    xwl_gbm->device_name = strdup(main_dev->nodes[DRM_NODE_RENDER]);
    if (!xwl_gbm->device_name) {
        return FALSE;
    }

    xwl_gbm->drm_fd = open(xwl_gbm->device_name, O_RDWR | O_CLOEXEC);
    if (xwl_gbm->drm_fd < 0) {
        ErrorF("wayland-egl: could not open %s (%s)\n",
               xwl_gbm->device_name, strerror(errno));
        return FALSE;
    }

    if (drmGetDevice2(xwl_gbm->drm_fd, 0, &xwl_gbm->device) != 0) {
        ErrorF("wayland-egl: Could not fetch DRM device %s\n",
               xwl_gbm->device_name);
        return FALSE;
    }

    xwl_gbm->fd_render_node = TRUE;
    return TRUE;
}

Bool
xwl_glamor_gbm_init_egl(struct xwl_screen *xwl_screen)
{
    struct xwl_gbm_private *xwl_gbm = xwl_gbm_get(xwl_screen);
    EGLint major, minor;
    const GLubyte *renderer;
    const char *gbm_backend_name, *egl_vendor;

    if (!xwl_gbm->drm && !xwl_glamor_gbm_init_main_dev(xwl_screen))
        return FALSE;

    if (!xwl_gbm->fd_render_node && !xwl_gbm->drm_authenticated) {
        ErrorF("Failed to get wl_drm, disabling Glamor and DRI3\n");
	return FALSE;
    }

    xwl_gbm->gbm = gbm_create_device(xwl_gbm->drm_fd);
    if (!xwl_gbm->gbm) {
        ErrorF("couldn't create gbm device\n");
        goto error;
    }

    xwl_screen->egl_display = glamor_egl_get_display(EGL_PLATFORM_GBM_MESA,
                                                     xwl_gbm->gbm);
    if (xwl_screen->egl_display == EGL_NO_DISPLAY) {
        ErrorF("glamor_egl_get_display() failed\n");
        goto error;
    }

    if (!eglInitialize(xwl_screen->egl_display, &major, &minor)) {
        ErrorF("eglInitialize() failed\n");
        goto error;
    }

    if (!xwl_glamor_try_big_gl_api(xwl_screen) &&
        !xwl_glamor_try_gles_api(xwl_screen)) {
        ErrorF("Cannot use neither GL nor GLES2\n");
        goto error;
    }

    renderer = glGetString(GL_RENDERER);
    if (!renderer) {
        ErrorF("glGetString() returned NULL, your GL is broken\n");
        goto error;
    }
    if (strstr((const char *)renderer, "softpipe")) {
        ErrorF("Refusing to try glamor on softpipe\n");
        goto error;
    }
    if (!strncmp("llvmpipe", (const char *)renderer, strlen("llvmpipe"))) {
        ErrorF("Refusing to try glamor on llvmpipe\n");
        goto error;
    }

    if (!epoxy_has_gl_extension("GL_OES_EGL_image")) {
        ErrorF("GL_OES_EGL_image not available\n");
        goto error;
    }

    if (epoxy_has_egl_extension(xwl_screen->egl_display,
                                "EXT_image_dma_buf_import") &&
        epoxy_has_egl_extension(xwl_screen->egl_display,
                                "EXT_image_dma_buf_import_modifiers"))
       xwl_gbm->dmabuf_capable = TRUE;

    gbm_backend_name = gbm_device_get_backend_name(xwl_gbm->gbm);
    /* Mesa uses "drm" as backend name, in that case, just do nothing */
    if (gbm_backend_name && strcmp(gbm_backend_name, "drm") != 0)
        xwl_screen->glvnd_vendor = gbm_backend_name;
    xwl_gbm->glamor_gles = !epoxy_is_desktop_gl();

    egl_vendor = eglQueryString(xwl_screen->egl_display, EGL_VENDOR);
    if (!egl_vendor) {
        ErrorF("Could not determine EGL vendor\n");
        goto error;
    }
    /* NVIDIA driver does not support implicit sync */
    xwl_gbm->implicit_sync = !strstr(egl_vendor, "NVIDIA");
#ifdef DRI3
    if (xwl_gbm_supports_syncobjs(xwl_screen) &&
        epoxy_has_egl_extension(xwl_screen->egl_display,
                                "ANDROID_native_fence_sync"))
        xwl_gbm->supports_syncobjs = TRUE;

    if (!xwl_gbm->supports_syncobjs && xwl_screen->explicit_sync) {
        /* explicit sync requires syncobj support */
        wp_linux_drm_syncobj_manager_v1_destroy(xwl_screen->explicit_sync);
        xwl_screen->explicit_sync = NULL;
    }
#endif /* DRI3 */
    return TRUE;
error:
    if (xwl_screen->egl_display != EGL_NO_DISPLAY) {
        xwl_glamor_maybe_destroy_context(xwl_screen);
        eglTerminate(xwl_screen->egl_display);
        xwl_screen->egl_display = EGL_NO_DISPLAY;
    }

    xwl_glamor_gbm_cleanup(xwl_screen);
    return FALSE;
}

Bool
xwl_glamor_gbm_init_screen(struct xwl_screen *xwl_screen)
{
    struct xwl_gbm_private *xwl_gbm = xwl_gbm_get(xwl_screen);
#ifdef DRI3
    if (xwl_gbm->supports_syncobjs) {
        xwl_dri3_info.version = 4;
        xwl_dri3_info.import_syncobj = xwl_dri3_import_syncobj;
    }

    if (!dri3_screen_init(xwl_screen->screen, &xwl_dri3_info)) {
        ErrorF("Failed to initialize dri3\n");
        goto error;
    }
#endif /* DRI3 */
    if (xwl_gbm->fd_render_node)
        goto skip_drm_auth;

    if (!dixRegisterPrivateKey(&xwl_auth_state_private_key, PRIVATE_CLIENT,
                               0)) {
        ErrorF("Failed to register private key\n");
        goto error;
    }

    if (!AddCallback(&ClientStateCallback, xwl_auth_state_client_callback,
                     NULL)) {
        ErrorF("Failed to add client state callback\n");
        goto error;
    }

skip_drm_auth:
    xwl_screen->screen->CreatePixmap = xwl_glamor_gbm_create_pixmap;
    xwl_screen->screen->DestroyPixmap = xwl_glamor_gbm_destroy_pixmap;

    return TRUE;
error:
    xwl_glamor_gbm_cleanup(xwl_screen);
    return FALSE;
}

drmDevice *xwl_gbm_get_main_device(struct xwl_screen *xwl_screen)
{
    struct xwl_gbm_private *xwl_gbm = xwl_gbm_get(xwl_screen);

    return xwl_gbm->device;
}

Bool
xwl_glamor_init_gbm(struct xwl_screen *xwl_screen)
{
    struct xwl_gbm_private *xwl_gbm;

    if (!xwl_glamor_gbm_has_egl_extension())
        return FALSE;

    if (!dixRegisterPrivateKey(&xwl_gbm_private_key, PRIVATE_SCREEN, 0))
        return FALSE;

    xwl_gbm = calloc(1, sizeof(*xwl_gbm));
    if (!xwl_gbm) {
        ErrorF("glamor: Not enough memory to setup GBM, disabling\n");
        return FALSE;
    }

    dixSetPrivate(&xwl_screen->screen->devPrivates, &xwl_gbm_private_key,
                  xwl_gbm);

    return TRUE;
}
