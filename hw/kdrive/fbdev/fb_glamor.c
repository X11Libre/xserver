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

#ifdef XV
#include "kxv.h"
#endif

char *fbdev_glvnd_provider = NULL;

Bool es_allowed = TRUE;
Bool force_es = FALSE;
Bool fbGlamorAllowed = TRUE;
Bool fbForceGlamor = FALSE;
Bool fbXVAllowed = TRUE;

#define ARR_SIZE(x) (sizeof(x) / sizeof(*(x)))

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

    free(fbdev_glvnd_provider);
    fbdev_glvnd_provider = NULL;
}

static void
fbdev_glamor_egl_make_current(struct glamor_context *glamor_ctx)
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

static inline void
fbdev_glamor_set_glvnd_vendor(ScreenPtr screen, const char* renderer, const char* vendor)
{
    if (!fbdev_glvnd_provider) {
        if (renderer && strstr(renderer, "NVIDIA")) {
            glamor_set_glvnd_vendor(screen, "nvidia");
        } else if (vendor && strstr(vendor, "NVIDIA")) {
            glamor_set_glvnd_vendor(screen, "nvidia");
        } else {
            glamor_set_glvnd_vendor(screen, "mesa");
        }
    } else {
        if (strstr(fbdev_glvnd_provider, "nvidia")) {
            glamor_set_glvnd_vendor(screen, "nvidia");
        } else {
            glamor_set_glvnd_vendor(screen, "mesa");
        }
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
        free(fbdev_glvnd_provider);
        fbdev_glvnd_provider = NULL;
        return FALSE;
    }

    const char *vendor = (const char*)glGetString(GL_VENDOR);
    const char *renderer = (const char*)glGetString(GL_RENDERER);

    int flags = GLAMOR_USE_EGL_SCREEN | GLAMOR_NO_DRI3;
    if (!fbGlamorAllowed) {
        flags |= GLAMOR_NO_RENDER_ACCEL;
    } else if (!fbForceGlamor){
        if (!renderer ||
            strstr(renderer, "softpipe") ||
            strstr(renderer, "llvmpipe")) {
            flags |= GLAMOR_NO_RENDER_ACCEL;
        }
    }

    if (!glamor_init(pScreen, flags)) {
        fbdev_glamor_egl_cleanup(scrpriv);
        return FALSE;
    }

    fbdev_glamor_set_glvnd_vendor(pScreen, renderer, vendor);

#ifdef GLXEXT
    if (!vendor_initialized) {
        GlxPushProvider(&glamor_provider);
        vendor_initialized = TRUE;
    }
#endif

#ifdef XV
    /* X-Video needs glamor render accel */
    if (fbXVAllowed && !(flags & GLAMOR_NO_RENDER_ACCEL)) {
        kd_glamor_xv_init(pScreen);
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
fbdev_glamor_query_devices_ext(EGLDeviceEXT **devices, EGLint *num_devices)
{
    EGLint max_devices = 0;

    *devices = NULL;
    *num_devices = 0;

    if (!epoxy_has_egl_extension(NULL, "EGL_EXT_device_base") &&
        !(epoxy_has_egl_extension(NULL, "EGL_EXT_device_query") &&
          epoxy_has_egl_extension(NULL, "EGL_EXT_device_enumeration"))) {
        return FALSE;
    }

    if (!eglQueryDevicesEXT(0, NULL, &max_devices)) {
         return FALSE;
    }

    *devices = calloc(sizeof(EGLDeviceEXT), max_devices);
    if (*devices == NULL) {
         return FALSE;
    }

    if (!eglQueryDevicesEXT(max_devices, *devices, num_devices) || *num_devices == 0) {
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
fbdev_egl_device_has_extension(const char *dev_ext, const char *ext)
{
    if (!dev_ext) {
        return FALSE;
    }

    int len = strlen(ext);

    for (;;) {
        dev_ext = strstr(dev_ext, ext);
        if (!dev_ext) {
            return FALSE;
        }
        if (dev_ext[len] == '\0' ||
            dev_ext[len] == ' ') {
            return TRUE;
        }
    }

    /* Unreachable */
    return FALSE;
}

static inline const char*
fbdev_egl_device_get_name(EGLDeviceEXT device)
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

    const char *driver_name = fbdev_egl_device_has_extension(dev_ext, "EGL_EXT_device_persistent_id") ?
                              eglQueryDeviceStringEXT(device, EGL_DRIVER_NAME_EXT) : NULL;

    if (driver_name) {
        return driver_name;
    }

    /* This might seem like overkill, but it's actually needed for the nvidia 470 driver */
    if (fbdev_egl_device_has_extension(dev_ext, "EGL_EXT_device_query_name")) {
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
fbdev_glamor_egl_device_matches_config(EGLDeviceEXT device, int strict, const char** driver_name)
{
    *driver_name = fbdev_egl_device_get_name(device);

    if (strict <= 0) {
        return TRUE;
    }

    if (*driver_name == NULL) {
        return FALSE;
    }

    if (!fbdev_glvnd_provider) {
        return TRUE;
    }

    if (!strcmp(*driver_name, fbdev_glvnd_provider)) {
        return TRUE;
    }

    if (strict >= 2) {
        return FALSE;
    }

    /**
     * This is not specific to nvidia,
     * but I don't know of any gl library vendors
     * other than mesa and nvidia
     */
    Bool device_is_nvidia = !!strstr(*driver_name, "nvidia");
    Bool config_is_nvidia = !!strstr(fbdev_glvnd_provider, "nvidia");

    return device_is_nvidia == config_is_nvidia;
}

static Bool
fbdev_glamor_egl_init_display(FbdevScrPriv *scrpriv)
{
    EGLDeviceEXT *devices = NULL;
    EGLint num_devices = 0;
    const char *driver_name = NULL;

    /**
     * If the user didn't give us a GL driver/library name,
     * we populate it with what we queried
     */
#define GLAMOR_EGL_TRY_PLATFORM(platform, native, platform_fallback) \
    scrpriv->display = glamor_egl_get_display2(platform, native, platform_fallback); \
    if (scrpriv->display != EGL_NO_DISPLAY) { \
        if (eglInitialize(scrpriv->display, NULL, NULL)) { \
            if (!fbdev_glvnd_provider && driver_name) { \
                fbdev_glvnd_provider = strdup(driver_name); \
            } \
            free(devices); \
            return TRUE; \
        } \
        eglTerminate(scrpriv->display); \
        scrpriv->display = EGL_NO_DISPLAY; \
    }

    if (fbdev_glamor_query_devices_ext(&devices, &num_devices)) {
#define GLAMOR_EGL_TRY_PLATFORM_DEVICE(strict) \
        for (uint32_t i = 0; i < num_devices; i++) { \
            if (fbdev_glamor_egl_device_matches_config(devices[i], strict, &driver_name)) { \
                GLAMOR_EGL_TRY_PLATFORM(EGL_PLATFORM_DEVICE_EXT, devices[i], TRUE); \
            } \
        }

        GLAMOR_EGL_TRY_PLATFORM_DEVICE(2);
        GLAMOR_EGL_TRY_PLATFORM_DEVICE(1);
        GLAMOR_EGL_TRY_PLATFORM_DEVICE(0);

#undef GLAMOR_EGL_TRY_PLATFORM_DEVICE
    }
    driver_name = NULL;

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

    free(devices);
    return FALSE;
}

static void
fbdev_glamor_egl_chose_configs(EGLDisplay display, const EGLint *attrib_list,
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
fbdev_glamor_egl_create_context(EGLDisplay display,
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

    fbdev_glamor_egl_chose_configs(display, config_attrib_list,
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
fbdev_glamor_egl_try_big_gl_api(FbdevScrPriv *scrpriv)
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
    static const EGLint config_attribs[] = {
        EGL_NONE
    };

    static const EGLint* ctx_attrib_lists[] =
        { config_attribs_core, config_attribs };

    static const EGLint config_attrib_list[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_CONFORMANT, EGL_OPENGL_BIT,
        EGL_SURFACE_TYPE, EGL_DONT_CARE,
        EGL_NONE
    };

    if (!eglBindAPI(EGL_OPENGL_API)) {
        return FALSE;
    }

    scrpriv->ctx = fbdev_glamor_egl_create_context(scrpriv->display,
                                                   config_attrib_list,
                                                   ctx_attrib_lists,
                                                   ARR_SIZE(ctx_attrib_lists));

    if (scrpriv->ctx == EGL_NO_CONTEXT) {
        return FALSE;
    }

    if (!eglMakeCurrent(scrpriv->display,
                        EGL_NO_SURFACE, EGL_NO_SURFACE, scrpriv->ctx)) {
        eglDestroyContext(scrpriv->display, scrpriv->ctx);
        scrpriv->ctx = EGL_NO_CONTEXT;
        return FALSE;
    }

    if (epoxy_gl_version() < 21) {
        /* Ignoring GL < 2.1, falling back to GLES */
        eglDestroyContext(scrpriv->display, scrpriv->ctx);
        scrpriv->ctx = EGL_NO_CONTEXT;
        return FALSE;
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

    static const EGLint* ctx_attrib_lists[] =
        { config_attribs };

    static const EGLint config_attrib_list[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_CONFORMANT, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_DONT_CARE,
        EGL_NONE
    };

    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        return FALSE;
    }

    scrpriv->ctx = fbdev_glamor_egl_create_context(scrpriv->display,
                                                   config_attrib_list,
                                                   ctx_attrib_lists,
                                                   ARR_SIZE(ctx_attrib_lists));

    if (scrpriv->ctx == EGL_NO_CONTEXT) {
        return FALSE;
    }

    if (!eglMakeCurrent(scrpriv->display,
                        EGL_NO_SURFACE, EGL_NO_SURFACE, scrpriv->ctx)) {
        eglDestroyContext(scrpriv->display, scrpriv->ctx);
        scrpriv->ctx = EGL_NO_CONTEXT;
        return FALSE;
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
        return FALSE;
    }

    if (!fbdev_glamor_bind_gl_api(scrpriv)) {
        fbdev_glamor_egl_cleanup(scrpriv);
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
    glamor_ctx->make_current = fbdev_glamor_egl_make_current;
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
