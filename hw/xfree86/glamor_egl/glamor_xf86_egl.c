/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111@shitposting.expert>
 */

#include <dix-config.h>

#define GLAMOR_FOR_XORG
#include <xf86.h>
#include <xf86Priv.h>

#include "glamor.h"
#include "glamor_egl.h"
#include "glamor_egl_priv.h"

enum {
    GLAMOREGLOPT_RENDERING_API,
    GLAMOREGLOPT_VENDOR_LIBRARY
};

static const OptionInfoRec GlamorEGLOptions[] = {
    { GLAMOREGLOPT_RENDERING_API, "RenderingAPI", OPTV_STRING, {0}, FALSE },
    { GLAMOREGLOPT_VENDOR_LIBRARY, "GlxVendorLibrary", OPTV_STRING, {0}, FALSE },
    { -1, NULL, OPTV_NONE, {0}, FALSE },
};

static int xf86GlamorEGLPrivateIndex = -1;

static inline glamor_egl_priv_t*
glamor_xf86_egl_get_scrn_private(ScrnInfoPtr scrn)
{
    return (glamor_egl_priv_t *)
        scrn->privates[xf86GlamorEGLPrivateIndex].ptr;
}

static glamor_egl_priv_t*
glamor_xf86_egl_get_screen_private(ScreenPtr screen)
{
    return glamor_xf86_egl_get_scrn_private(xf86ScreenToScrn(screen));
}

static void
glamor_xf86_egl_free_screen(ScrnInfoPtr scrn)
{
    glamor_egl_priv_t *glamor_egl;

    glamor_egl = glamor_xf86_egl_get_scrn_private(scrn);
    if (glamor_egl != NULL) {
        scrn->FreeScreen = glamor_egl->server_private;
        glamor_egl_cleanup(glamor_egl);
        free(glamor_egl);
        scrn->FreeScreen(scrn);
    }
}

Bool
glamor_egl_init(ScrnInfoPtr scrn, int fd)
{
    glamor_egl_priv_t *glamor_egl;
    OptionInfoPtr options;
    const char *api = NULL;
    const char *glvnd_vendor = NULL;
    glamor_egl_conf_t glamor_egl_conf = {.fd = fd};

    glamor_egl = calloc(1, sizeof(*glamor_egl));
    if (glamor_egl == NULL)
        return FALSE;

    glamor_egl_conf.glamor_egl_priv = glamor_egl;

    if (xf86GlamorEGLPrivateIndex == -1)
        xf86GlamorEGLPrivateIndex = xf86AllocateScrnInfoPrivateIndex();

    options = XNFalloc(sizeof(GlamorEGLOptions));
    memcpy(options, GlamorEGLOptions, sizeof(GlamorEGLOptions));
    xf86ProcessOptions(scrn->scrnIndex, scrn->options, options);
    glvnd_vendor = xf86GetOptValString(options, GLAMOREGLOPT_VENDOR_LIBRARY);
    if (glvnd_vendor) {
        glamor_egl_conf.glvnd_vendor = strdup(glvnd_vendor);
        if (!glamor_egl_conf.glvnd_vendor) {
            LogMessage(X_WARNING, "Couldn't set gl vendor to: %s\n", glvnd_vendor);
        }
    }
    api = xf86GetOptValString(options, GLAMOREGLOPT_RENDERING_API);
    if (api && !strncasecmp(api, "es", 2))
        glamor_egl_conf.force_es = TRUE;
    else if (api && !strncasecmp(api, "gl", 2))
        glamor_egl_conf.es_disallowed = TRUE;
    free(options);

    glamor_egl_conf.GLAMOR_EGL_PRIV_PROC = glamor_xf86_egl_get_screen_private;

    scrn->privates[xf86GlamorEGLPrivateIndex].ptr = glamor_egl;

    if (xf86Info.debug != NULL) {
        glamor_egl_conf.dmabuf_forced = TRUE;
        glamor_egl_conf.dmabuf_capable = !!strstr(xf86Info.debug,
                                                   "dmabuf_capable");
    }

    Bool compat_ret = TRUE;

    glamor_egl_conf.llvmpipe_allowed = !!scrn->confScreen->num_gpu_devices;

    glamor_egl_conf.server_private = scrn->FreeScreen;
    if (glamor_egl_init_internal(&glamor_egl_conf, &compat_ret)) {
        scrn->FreeScreen = glamor_xf86_egl_free_screen;
        return compat_ret;
    }

    free(glamor_egl);
    return FALSE;
}

/** Stub to retain compatibility with pre-server-1.16 ABI. */
Bool
glamor_egl_init_textured_pixmap(ScreenPtr screen)
{
    return TRUE;
}
