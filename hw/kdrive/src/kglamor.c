/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111github@gmail.com>
 */

#include <kdrive-config.h>

#include <X11/Xfuncproto.h>

#include "scrnintstr.h"

#ifdef GLAMOR
#include "glamor.h"
#include "glamor_egl.h"
#endif

#include "kglamor.h"

#ifdef XV
#include "kxv.h"
#endif

#include <errno.h>

const KdGlamorInfo kdGlamorDefault = {
                                      .glvnd = NULL,
                                      .dri_fd = -1,
                                      .use_gbm = FALSE,
                                      .direct_dri3 = FALSE,
                                      .force_gl = FALSE,
                                      .force_es = FALSE,
                                      .use_xv = TRUE,
                                      .no_render_accel = FALSE,
                                      .force_render_accel = FALSE,
                                     };

#ifdef GLAMOR
Bool
KdGlamorInit(ScreenPtr pScreen, const KdGlamorInfo *info, int *caps)
{
    int flags = GLAMOR_USE_EGL_SCREEN;

    glamor_egl_conf_t glamor_egl_conf = {
                                         .server_private = NULL, /* only for xf86 */
                                         .screen = pScreen,
                                         .glamor_egl_priv = NULL, /* only for xf86 */
                                         .GLAMOR_EGL_PRIV_PROC = NULL, /* only for xf86 */
                                         .glvnd_vendor = info->glvnd,
                                         .fd = info->dri_fd,
                                         .gbm_forbidden = !info->use_gbm,
                                         .direct_dri3_only = info->direct_dri3,
                                         .auto_dri = FALSE, /* deprecated */
                                         .partial_dri_allowed = FALSE, /* deprecated */
                                         .dmabuf_forced = FALSE, /* only for xf86 */
                                         .dmabuf_capable = TRUE, /* only for xf86 */
                                         .llvmpipe_allowed = TRUE, /* only for xf86 */
                                         .force_glamor = TRUE, /* only for xf86 */
                                         .es_disallowed = info->force_gl,
                                         .force_es = info->force_es,
                                        };

    if (caps) {
        *caps = GLAMOR_EGL_CAP_NONE;
    }

    if (!glamor_egl_init_internal(&glamor_egl_conf, caps)) {
        return FALSE;
    }

    if (info->no_render_accel) {
        flags |= GLAMOR_NO_RENDER_ACCEL;
    } else if (!info->force_render_accel) {
        const char *renderer = (const char*)glGetString(GL_RENDERER);
        if (!renderer ||
            strstr(renderer, "softpipe") ||
            strstr(renderer, "llvmpipe")) {
            flags |= GLAMOR_NO_RENDER_ACCEL;
        }
    }

    if (info->dri_fd < 0) {
        flags |= GLAMOR_NO_DRI3;
    }

    if (!glamor_init(pScreen, flags)) {
        return FALSE;
    }

#ifdef XV
    if (info->use_xv) {
        kd_glamor_xv_init(pScreen);
    }
#endif

    return TRUE;
}

void
KdGlamorEnable(ScreenPtr pScreen)
{
}

void
KdGlamorDisable(ScreenPtr pScreen)
{
}

void
KdGlamorFini(ScreenPtr pScreen)
{
    glamor_fini(pScreen);
}
#endif
