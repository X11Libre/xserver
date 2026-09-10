/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111github@gmail.com>
 */

#include <kdrive-config.h>

#include <X11/Xfuncproto.h>

#include "scrnintstr.h"

#include "os/cmdline.h" /* UseMsg() */

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

int
KdGlamorParse(KdGlamorInfo *info, const char **dri_path, int argc, char **argv, int i)
{
    if (!strcmp(argv[i], "-glamor")) {
        info->force_render_accel = TRUE;
        return 1;
    }

    if (!strcmp(argv[i], "-noglamor")) {
        info->no_render_accel = TRUE;
        return 1;
    }

    if (!strcmp(argv[i], "-gbm")) {
        info->use_gbm = TRUE;
        return 1;
    }

    if (!strcmp(argv[i], "-direct-dri3")) {
        info->direct_dri3 = TRUE;
        return 1;
    }

    if (!strcmp(argv[i], "-glvendor")) {
        if ((i + 1 < argc) && (argv[i + 1][0] != '-')) {
            info->glvnd = argv[i + 1];
            return 2;
        }
        UseMsg();
        exit(1);
    }

    if (!strcmp(argv[i], "-dri")) {
        if ((i + 1 < argc) && (argv[i + 1][0] != '-')) {
            if (dri_path) {
                *dri_path = argv[i + 1];
            }
            return 2;
        }
        UseMsg();
        exit(1);
    }

    if (!strcmp(argv[i], "-force-gl")) {
        info->force_gl = TRUE;
        return 1;
    }

    if (!strcmp(argv[i], "-force-es")) {
        info->force_es = TRUE;
        return 1;
    }

    if (!strcmp(argv[i], "-noxv")) {
        info->use_xv = FALSE;
        return 1;
    }

    return 0;
}

void
KdGlamorUseMsg(void)
{
    KdUseMsg();
    ErrorF("\nTinyX Glamor Usage:\n");
    ErrorF
        ("-dri <path>          Optional drm device path to use\n");
    ErrorF
        ("-glamor              Force enable glamor render acceleration if possible\n");
    ErrorF
        ("-noglamor            Force disable glamor render acceleration\n");
    ErrorF
        ("-gbm                 Allow glamor to use libgbm\n");
    ErrorF
        ("-direct-dri3         Force glamor to use the direct DRI3 implementation\n");
    ErrorF
        ("-glvendor <string>   Suggest what glvnd vendor library should be used\n");
    ErrorF
        ("-force-gl            Force glamor to only use GL contexts\n");
    ErrorF
        ("-force-es            Force glamor to only use GLES contexts\n");
    ErrorF
        ("-noxv                Disable X-Video support\n");
}

/*
 * I don't think this deserves its own .c file
 *
 * Put each screen on a different card
 */
void
KdEnsureCard(int argc, char **argv, int i, Bool force)
{
    if (force /* We need at least one card */
        || ((i >= 1) && !strcmp(argv[i - 1], "-screen")) /* Last screen had no explicit geometry */
        || ((i >= 2) && ('0' <= argv[i - 1][0]) && (argv[i - 1][0] <= '9') && !strcmp(argv[i - 2], "-screen")) /* Last screen had explicit geometry */
        ) {
        /* Put each screen on a separate card */
        Bool need_new_card = force;

        /**
         * If this is either the first argument, or the
         * first argument after the last -screen argument.
         *
         * If this is the first argument, we need to create a new card.
         *
         * If this is the first argument after a -screen argument
         * we need to determine if this argument, and all those that follow
         * represent a new screen, or if they are arguments for the screen we just parsed.
         *
         * We do this by checking if any of the remaining arguments, *including this one* are -screen arguments.
         */
        for (int j = i; j < argc && !need_new_card; j++) {
            if (!strcmp(argv[j], "-screen")) {
                need_new_card = TRUE;
                break;
            }
        }
        if (need_new_card) {
            InitCard(NULL);
        }
    }
}
