/*
 * Copyright © 2011-2014 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of the
 * copyright holders not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include <xwayland-config.h>

#define MESA_EGL_NO_X11_HEADERS
#define EGL_NO_X11
#include <glamor_egl.h>

#include <glamor.h>
#include <glamor_context.h>
#include <glamor_glx_provider.h>
#ifdef GLXEXT
#include "glx_extinit.h"
#endif

#include "drm-client-protocol.h"
#include "linux-dmabuf-unstable-v1-client-protocol.h"
#include "linux-drm-syncobj-v1-client-protocol.h"

#include "mi/mi_priv.h"

#include "xwayland-dmabuf.h"
#include "xwayland-glamor.h"
#include "xwayland-glamor-gbm.h"
#include "xwayland-present.h"
#include "xwayland-screen.h"
#include "xwayland-window.h"
#include "xwayland-window-buffers.h"

#include <sys/mman.h>

static void
glamor_egl_make_current(struct glamor_context *glamor_ctx)
{
    eglMakeCurrent(glamor_ctx->display, EGL_NO_SURFACE,
                   EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (!eglMakeCurrent(glamor_ctx->display,
                        EGL_NO_SURFACE, EGL_NO_SURFACE,
                        glamor_ctx->ctx))
        FatalError("Failed to make EGL context current\n");
}

void
xwl_glamor_egl_make_current(struct xwl_screen *xwl_screen)
{
    EGLContext ctx = xwl_screen->glamor_ctx->ctx;
    
    if (lastGLContext == ctx)
        return;

    lastGLContext = ctx;
    xwl_screen->glamor_ctx->make_current(xwl_screen->glamor_ctx);
}

void
glamor_egl_screen_init(ScreenPtr screen, struct glamor_context *glamor_ctx)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(screen);

    glamor_set_glvnd_vendor(screen, xwl_screen->glvnd_vendor);
    glamor_enable_dri3(screen);
    glamor_ctx->ctx = xwl_screen->egl_context;
    glamor_ctx->display = xwl_screen->egl_display;

    glamor_ctx->make_current = glamor_egl_make_current;

    xwl_screen->glamor_ctx = glamor_ctx;
}

Bool
xwl_glamor_check_flip(WindowPtr present_window, PixmapPtr pixmap)
{
    ScreenPtr screen = pixmap->drawable.pScreen;
    PixmapPtr backing_pixmap = screen->GetWindowPixmap(present_window);
    struct xwl_window *xwl_window = xwl_window_from_window(present_window);
    WindowPtr surface_window = xwl_window->surface_window;

    if (pixmap->drawable.depth != backing_pixmap->drawable.depth) {
        if (pixmap->drawable.depth == 32)
            return FALSE;

        return xwl_present_maybe_redirect_window(present_window);
    }

    if (surface_window->redirectDraw == RedirectDrawAutomatic &&
        surface_window->drawable.depth != 32 &&
        surface_window->parent->drawable.depth == 32)
        xwl_present_maybe_redirect_window(surface_window);

    return TRUE;
}

void
xwl_glamor_init_wl_registry(struct xwl_screen *xwl_screen,
                            struct wl_registry *registry,
                            uint32_t id, const char *interface,
                            uint32_t version)
{
    if (strcmp(interface, wl_drm_interface.name) == 0)
        xwl_screen_set_drm_interface(xwl_screen, id, version);
    else if (strcmp(interface, zwp_linux_dmabuf_v1_interface.name) == 0)
        xwl_screen_set_dmabuf_interface(xwl_screen, id, version);
    else if (strcmp(interface, wp_linux_drm_syncobj_manager_v1_interface.name) == 0)
        xwl_screen_set_syncobj_interface(xwl_screen, id, version);
}

static Bool
xwl_glamor_has_wl_interfaces(struct xwl_screen *xwl_screen)
{
    if (!xwl_glamor_has_wl_drm(xwl_screen) &&
        xwl_screen->dmabuf_protocol_version < 4) {
        LogMessageVerb(X_INFO, 3, "glamor: 'wl_drm' not supported and linux-dmabuf v4 not supported\n");
        return FALSE;
    }

    return TRUE;
}

static Bool
xwl_glamor_create_screen_resources(ScreenPtr screen)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(screen);

    if (!miCreateScreenResources(screen))
        return FALSE;

    if (xwl_screen->rootless) {
        screen->devPrivate =
            fbCreatePixmap(screen, 0, 0, screen->rootDepth, 0);
    }
    else {
        screen->devPrivate = screen->CreatePixmap(
            screen, screen->width, screen->height, screen->rootDepth,
            CREATE_PIXMAP_USAGE_BACKING_PIXMAP);
    }

    SetRootClip(screen, xwl_screen->root_clip_mode);

    return screen->devPrivate != NULL;
}

int
glamor_egl_fd_name_from_pixmap(ScreenPtr screen,
                               PixmapPtr pixmap,
                               CARD16 *stride, CARD32 *size)
{
    return 0;
}

int
xwl_glamor_get_fence(struct xwl_screen *xwl_screen)
{
    EGLint attribs[3];
    EGLSyncKHR sync;
    int fence_fd = -1;

    if (!xwl_screen->glamor)
        return -1;

    xwl_glamor_egl_make_current(xwl_screen);

    attribs[0] = EGL_SYNC_NATIVE_FENCE_FD_ANDROID;
    attribs[1] = EGL_NO_NATIVE_FENCE_FD_ANDROID;
    attribs[2] = EGL_NONE;
    sync = eglCreateSyncKHR(xwl_screen->egl_display, EGL_SYNC_NATIVE_FENCE_ANDROID, attribs);
    if (sync != EGL_NO_SYNC_KHR) {
        fence_fd = eglDupNativeFenceFDANDROID(xwl_screen->egl_display, sync);
        eglDestroySyncKHR(xwl_screen->egl_display, sync);
    }

    return fence_fd;
}

/* Takes ownership of fence_fd, specifically eglCreateSyncKHR does */
void
xwl_glamor_wait_fence(struct xwl_screen *xwl_screen, int fence_fd)
{
    EGLint attribs[3];
    EGLSyncKHR sync;

    if (!xwl_screen->glamor) {
        close(fence_fd);
        return;
    }

    xwl_glamor_egl_make_current(xwl_screen);

    attribs[0] = EGL_SYNC_NATIVE_FENCE_FD_ANDROID;
    attribs[1] = fence_fd;
    attribs[2] = EGL_NONE;
    sync = eglCreateSyncKHR(xwl_screen->egl_display, EGL_SYNC_NATIVE_FENCE_ANDROID, attribs);
    if (sync != EGL_NO_SYNC_KHR) {
        eglWaitSyncKHR(xwl_screen->egl_display, sync, 0);
        eglDestroySyncKHR(xwl_screen->egl_display, sync);
    }
}

Bool
xwl_glamor_init(struct xwl_screen *xwl_screen)
{
    ScreenPtr screen = xwl_screen->screen;
    const char *no_glamor_env;

    no_glamor_env = getenv("XWAYLAND_NO_GLAMOR");
    if (no_glamor_env && *no_glamor_env != '0') {
        ErrorF("Disabling glamor and dri3 support, XWAYLAND_NO_GLAMOR is set\n");
        return FALSE;
    }

    if (!xwl_glamor_has_wl_interfaces(xwl_screen)) {
        ErrorF("Xwayland glamor: GBM Wayland interfaces not available\n");
        return FALSE;
    }

    if (!xwl_glamor_gbm_init_egl(xwl_screen)) {
        ErrorF("EGL setup failed, disabling glamor\n");
        return FALSE;
    }

    if (!glamor_init(xwl_screen->screen, GLAMOR_USE_EGL_SCREEN)) {
        ErrorF("Failed to initialize glamor\n");
        return FALSE;
    }

    if (!xwl_glamor_gbm_init_screen(xwl_screen)) {
        ErrorF("EGL backend init_screen() failed, disabling glamor\n");
        return FALSE;
    }

    screen->CreateScreenResources = xwl_glamor_create_screen_resources;

#ifdef XV
    if (!xwl_glamor_xv_init(screen))
        ErrorF("Failed to initialize glamor Xv extension\n");
#endif

#ifdef GLXEXT
    GlxPushProvider(&glamor_provider);
#endif

    return TRUE;
}
