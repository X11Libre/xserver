/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111@shitposting.expert>
 */

/**
 * Private definitions to be used by glamor and the xf86 server
 */

#ifndef GLAMOR_EGL_PRIV_H
#define GLAMOR_EGL_PRIV_H

#define MESA_EGL_NO_X11_HEADERS
#define EGL_NO_X11
#include <epoxy/gl.h>
#include <epoxy/egl.h>

#include "scrnintstr.h"
#include "glamor_egl_ext.h"

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
    int fast_gbm_import;
    int can_texture_gbm_bo;
#ifdef EGL_MESA_image_dma_buf_export
    int has_image_dma_buf_export;
#endif
#endif

    int has_EXT_EGL_image_storage;
    int has_OES_EGL_image;

    int fd;
    int dmabuf_capable;
} glamor_egl_priv_t;

/**
 * Deinitialize an egl context created by glamor egl
 * and free associated resources.
 */
void glamor_egl_cleanup(glamor_egl_priv_t *glamor_egl);

/**
 * Deinitialize an egl context created by glamor egl
 * and free associated resources.
 */
void glamor_egl_cleanup_screen(ScreenPtr screen);


#endif /* GLAMOR_EGL_PRIV_H */
