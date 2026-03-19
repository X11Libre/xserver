/*
 * Copyright © 2016 Red Hat, Inc.
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *	Adam Jackson <ajax@redhat.com>
 */

#ifndef GLAMOR_EGL_H
#define GLAMOR_EGL_H

#include <stdbool.h>

#define MESA_EGL_NO_X11_HEADERS
#define EGL_NO_X11
#include <epoxy/gl.h>
#include <epoxy/egl.h>

#include "scrnintstr.h"

#ifdef GLAMOR_HAS_GBM
#include <gbm.h>
#endif

typedef struct glamor_egl_screen_private {
    EGLDisplay display;
    EGLContext context;
    char *device_path;
    char *glvnd_vendor; /* glvnd vendor library name or driver name */
    int exact_glvnd_vendor; /* If the glvnd vendor should be assumed valid with no checks */
    void* server_private;

#ifdef GLAMOR_HAS_GBM
    struct gbm_device *gbm;
#endif
    int fd;
    int dmabuf_capable;
    int linear_only; /* When using gbm, this means that only linear buffers can be created */
} glamor_egl_priv_t;

typedef struct {
    void* server_private; /* Data the X server might want to map to a screen */

    /* Pointer to a server-allocated glamor_egl_priv_t, that the server maps to a screen */
    glamor_egl_priv_t *glamor_egl_priv;

    /* Function that maps a glamor_egl_priv_t to each screen*/
    glamor_egl_priv_t* (*GLAMOR_EGL_PRIV_PROC)(ScreenPtr screen);

    char *glvnd_vendor; /* glvnd vendor library or driver name */
    int fd; /* /dev/dri/cardxx */
    int dmabuf_forced; /* If glamor should not use dynamic logic and only listen to the config below */
    int dmabuf_capable; /* If glamor should use dmabufs when using direct rendering (dri) */

    int llvmpipe_allowed; /* If glamor render accel should initialize on llvmpipe */
    int force_glamor; /* If glamor should initialize even on softpipe/llvmpipe */

    int es_disallowed; /* If using GLES contexts is forbidden */
    int force_es; /* If glamor should only use GLES contexts */
} glamor_egl_conf_t;

/**
 * Initialize an egl context suitable to be used by glamor.
 *
 * glamor_egl_conf is a pointer to caller-allocated storage.
 *
 * If compat_ret is not NULL, it will be set to a return value
 * for compatibility with xf86 drivers.
 */
Bool glamor_egl_init_internal(glamor_egl_conf_t* glamor_egl_conf, Bool *compat_ret);

/**
 * Deinitialize an egl context created by glamor egl
 * and free associated resources.
 *
 * glamor_egl is the pointer passed to glamor_egl_init2
 * in glamor_egl_conf->glamor_egl_priv;
 */
void glamor_egl_cleanup(glamor_egl_priv_t *glamor_egl);

/*
 * Create an EGLDisplay from a native display type. This is a little quirky
 * for a few reasons.
 *
 * 1: GetPlatformDisplayEXT and GetPlatformDisplay are the API you want to
 * use, but have different function signatures in the third argument; this
 * happens not to matter for us, at the moment, but it means epoxy won't alias
 * them together.
 *
 * 2: epoxy 1.3 and earlier don't understand EGL client extensions, which
 * means you can't call "eglGetPlatformDisplayEXT" directly, as the resolver
 * will crash.
 *
 * 3: You can't tell whether you have EGL 1.5 at this point, because
 * eglQueryString(EGL_VERSION) is a property of the display, which we don't
 * have yet. So you have to query for extensions no matter what. Fortunately
 * epoxy_has_egl_extension _does_ let you query for client extensions, so
 * we don't have to write our own extension string parsing.
 *
 * 4. There is no EGL_KHR_platform_base to complement the EXT one, thus one
 * needs to know EGL 1.5 is supported in order to use the eglGetPlatformDisplay
 * function pointer.
 * We can workaround this (circular dependency) by probing for the EGL 1.5
 * platform extensions (EGL_KHR_platform_gbm and friends) yet it doesn't seem
 * like mesa will be able to adverise these (even though it can do EGL 1.5).
 */
static inline EGLDisplay
glamor_egl_get_display2(EGLint type, void *native, bool platform_fallback)
{
    /* In practise any EGL 1.5 implementation would support the EXT extension */
    if (epoxy_has_egl_extension(NULL, "EGL_EXT_platform_base")) {
        PFNEGLGETPLATFORMDISPLAYEXTPROC getPlatformDisplayEXT =
            (void *) eglGetProcAddress("eglGetPlatformDisplayEXT");
        if (getPlatformDisplayEXT)
            return getPlatformDisplayEXT(type, native, NULL);
    }

    /* Welp, everything is awful. */
    return platform_fallback ? eglGetDisplay(native) : NULL;
}

/* Used by Xephyr */
static inline EGLDisplay
glamor_egl_get_display(EGLint type, void *native)
{
    return glamor_egl_get_display2(type, native, true);
}

#endif
