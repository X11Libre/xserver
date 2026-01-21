/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111@shitposting.expert>
 */

#include <kdrive-config.h>

#include <X11/Xfuncproto.h>

#include <epoxy/egl.h>
#include "scrnintstr.h"
#include "glamor_priv.h"
#include "glamor_egl.h"
#include "glamor_glx_provider.h"
#include "glx_extinit.h"

#include "fbdev.h"

const char *fbdev_glvnd_provider = "mesa";

Bool es_allowed = TRUE;
Bool force_es = FALSE;

static void
fbdev_glamor_egl_cleanup(FbdevScrPriv *scrpriv)
{
    if (scrpriv->display != EGL_NO_DISPLAY) {
        if (scrpriv->ctx != EGL_NO_CONTEXT) {
            eglDestroyContext(scrpriv->display, scrpriv->ctx);
        }

        eglMakeCurrent(scrpriv->display,
                       EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        /*
         * Force the next glamor_make_current call to update the context
         * (on hot unplug another GPU may still be using glamor)
         */
        lastGLContext = NULL;
        eglTerminate(scrpriv->display);
        scrpriv->display = EGL_NO_DISPLAY;
    }
}

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

static Bool fbdev_glamor_egl_init(ScreenPtr screen);

Bool
fbdevInitAccel(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    FbdevScrPriv *scrpriv = screen->driver;
#ifdef GLXEXT
    static Bool vendor_initialized = FALSE;
#endif

    if (!fbdev_glamor_egl_init(pScreen)) {
        screen->dumb = TRUE;
        return FALSE;
    }

    if (!glamor_init(pScreen, GLAMOR_USE_EGL_SCREEN | GLAMOR_NO_DRI3)) {
        fbdev_glamor_egl_cleanup(scrpriv);
        screen->dumb = TRUE;
        return FALSE;
    }

#ifdef GLXEXT
    if (!vendor_initialized) {
        GlxPushProvider(&glamor_provider);
        vendor_initialized = TRUE;
    }
#endif

    return TRUE;
}

void
fbdevEnableAccel(ScreenPtr screen)
{
    (void)screen;
}

void
fbdevDisableAccel(ScreenPtr screen)
{
    (void)screen;
}

void
fbdevFiniAccel(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    FbdevScrPriv *scrpriv = screen->driver;

    fbdev_glamor_egl_cleanup(scrpriv);
}

static Bool
glamor_query_devices_ext(EGLDeviceEXT **devices, EGLint *num_devices)
{
    EGLint max_devices = 0;

    *devices = NULL;
    *num_devices = 0;

    if (!epoxy_has_egl_extension(NULL, "EGL_EXT_device_query") ||
        !epoxy_has_egl_extension(NULL, "EGL_EXT_device_enumeration")) {
        return FALSE;
    }

    if (!eglQueryDevicesEXT(0, NULL, &max_devices)) {
         return FALSE;
    }

    *devices = calloc(sizeof(EGLDeviceEXT), max_devices);
    if (*devices == NULL) {
         return FALSE;
    }

    if (!eglQueryDevicesEXT(max_devices, *devices, num_devices)) {
         free(*devices);
         *devices = NULL;
         *num_devices = 0;
         return FALSE;
    }

    if (*num_devices < max_devices) {
         /* Shouldn't happen */
         void *tmp = realloc(*devices, *num_devices * sizeof(EGLDeviceEXT));
         if (tmp) {
             *devices = tmp;
         }
    }

    return TRUE;
}

static inline Bool
glamor_egl_device_matches_config(EGLDeviceEXT device, Bool strict)
{
    if (!strict) {
        return TRUE;
    }
/**
 * For some reason, this isn't part of the epoxy headers.
 * It is part of EGL/eglext.h, but we can't include that
 * alongside the expoxy headers.
 *
 * See: https://registry.khronos.org/EGL/extensions/EXT/EGL_EXT_device_persistent_id.txt
 * for the spec where this is defined
 */
#ifndef EGL_DRIVER_NAME_EXT
#define EGL_DRIVER_NAME_EXT 0x335E
#endif

    const char *driver_name = eglQueryDeviceStringEXT(device, EGL_DRIVER_NAME_EXT);
    if (!driver_name) {
        return FALSE;
    }

    if (!strcmp(driver_name, fbdev_glvnd_provider)) {
        return TRUE;
    }

    /**
     * This is not specific to nvidia,
     * but I don't know of any gl library vendors
     * other than mesa and nvidia
     */
    Bool device_is_nvidia = !!strstr(driver_name, "nvidia");
    Bool config_is_nvidia = !!strstr(fbdev_glvnd_provider, "nvidia");

    return device_is_nvidia == config_is_nvidia;
}

static Bool
fbdev_glamor_egl_init_display(FbdevScrPriv *scrpriv)
{
    EGLDeviceEXT *devices = NULL;
    EGLint num_devices = 0;

#define GLAMOR_EGL_TRY_PLATFORM(platform, native, platform_fallback) \
    scrpriv->display = glamor_egl_get_display2(platform, native, platform_fallback); \
    if (scrpriv->display != EGL_NO_DISPLAY) { \
        if (eglInitialize(scrpriv->display, NULL, NULL)) { \
            free(devices); \
            return TRUE; \
        } \
        eglTerminate(scrpriv->display); \
        scrpriv->display = EGL_NO_DISPLAY; \
    }

    if (glamor_query_devices_ext(&devices, &num_devices)) {
        /* Try a strict match first */
        for (uint32_t i = 0; i < num_devices; i++) {
            if (glamor_egl_device_matches_config(devices[i], TRUE)) {
                GLAMOR_EGL_TRY_PLATFORM(EGL_PLATFORM_DEVICE_EXT, devices[i], TRUE);
            }
        }

        /* Try a try a less strict match now */
        for (uint32_t i = 0; i < num_devices; i++) {
            if (glamor_egl_device_matches_config(devices[i], FALSE)) {
                GLAMOR_EGL_TRY_PLATFORM(EGL_PLATFORM_DEVICE_EXT, devices[i], TRUE);
            }
        }
    }

    GLAMOR_EGL_TRY_PLATFORM(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, FALSE);

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

#undef GLAMOR_EGL_TRY_PLATFORM

    return FALSE;
}

static Bool
fbdev_glamor_egl_try_big_gl_api(FbdevScrPriv *scrpriv)
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

        scrpriv->ctx = eglCreateContext(scrpriv->display,
                                        EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT,
                                        config_attribs_core);

        if (scrpriv->ctx == EGL_NO_CONTEXT)
            scrpriv->ctx = eglCreateContext(scrpriv->display,
                                            EGL_NO_CONFIG_KHR,
                                            EGL_NO_CONTEXT,
                                            config_attribs);
    }

    if (scrpriv->ctx != EGL_NO_CONTEXT) {
        if (!eglMakeCurrent(scrpriv->display,
                            EGL_NO_SURFACE, EGL_NO_SURFACE, scrpriv->ctx)) {
            return FALSE;
        }

        if (epoxy_gl_version() < 21) {
            /* Ignoring GL < 2.1, falling back to GLES */
            eglDestroyContext(scrpriv->display, scrpriv->ctx);
            scrpriv->ctx = EGL_NO_CONTEXT;
        }
    }
    return TRUE;
}

static Bool
fbdev_glamor_egl_try_gles_api(FbdevScrPriv *scrpriv)
{
    static const EGLint config_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        return FALSE;
    }

    scrpriv->ctx = eglCreateContext(scrpriv->display,
                                    EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT,
                                    config_attribs);

    if (scrpriv->ctx != EGL_NO_CONTEXT) {
        if (!eglMakeCurrent(scrpriv->display,
                            EGL_NO_SURFACE, EGL_NO_SURFACE, scrpriv->ctx)) {
            return FALSE;
        }
    }
    return TRUE;
}

static Bool
fbdev_glamor_bind_gl_api(FbdevScrPriv *scrpriv)
{
    if (!force_es && fbdev_glamor_egl_try_big_gl_api(scrpriv)) {
        return TRUE;
    }

    return es_allowed && fbdev_glamor_egl_try_gles_api(scrpriv);
}

static Bool
fbdev_glamor_egl_init(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    FbdevScrPriv *scrpriv = screen->driver;

    if (!fbdev_glamor_egl_init_display(scrpriv)) {
        screen->dumb = TRUE;
        return FALSE;
    }

    if (!fbdev_glamor_bind_gl_api(scrpriv)) {
        fbdev_glamor_egl_cleanup(scrpriv);
        screen->dumb = TRUE;
        return FALSE;
    }

    return TRUE;
}

/* Actual glamor functionality */
void
glamor_egl_screen_init(ScreenPtr pScreen, struct glamor_context *glamor_ctx)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    FbdevScrPriv *scrpriv = screen->driver;

    /* No dri3 */

    glamor_ctx->display = scrpriv->display;
    glamor_ctx->ctx = scrpriv->ctx;
    glamor_ctx->surface = EGL_NO_SURFACE;
    glamor_ctx->make_current = glamor_egl_make_current;
}

/* Stubs for glamor */
#define SET(ptr, val) if(ptr) { *ptr = val; }
int
glamor_egl_fd_name_from_pixmap(ScreenPtr screen,
                               PixmapPtr pixmap,
                               CARD16 *stride, CARD32 *size)
{
    (void)screen;
    (void)pixmap;
    SET(stride, 0);
    SET(size, 0);
    return -1;
}


int
glamor_egl_fds_from_pixmap(ScreenPtr screen, PixmapPtr pixmap, int *fds,
                           uint32_t *offsets, uint32_t *strides,
                           uint64_t *modifier)
{
    (void)screen;
    (void)pixmap;
    (void)fds;
    (void)offsets;
    (void)strides;
    (void)modifier;
    return 0;
}

int
glamor_egl_fd_from_pixmap(ScreenPtr screen, PixmapPtr pixmap,
                          CARD16 *stride, CARD32 *size)
{
    (void)screen;
    (void)pixmap;
    SET(stride, 0);
    SET(size, 0);
    return -1;
}
