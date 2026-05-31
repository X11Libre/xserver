/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111@shitposting.expert>
 */

#include <kdrive-config.h>

#include <X11/Xfuncproto.h>

#include "scrnintstr.h"

#include "glamor.h"
#include "glamor_egl.h"

#include "fbdev.h"

#ifdef XV
#include "kxv.h"
#endif

#ifdef WITH_LIBDRM
#include <xf86drm.h>
#endif

#include <errno.h>

Bool
fbdevInitAccel(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    FbdevScrPriv *scrpriv = screen->driver;
    FbScreenConf *config = screen->card->closure;

    if (config->fbNoAccel) {
        /* Only disable accel for this screen */
        return TRUE;
    }

    if (config->fbdev_dri_path) {
        scrpriv->dri_fd = open(config->fbdev_dri_path, O_RDWR);
        if (scrpriv->dri_fd >= 0) {
#ifdef WITH_LIBDRM
            if (config->fbdev_drm_master) {
                drmSetMaster(scrpriv->dri_fd);
            } else {
                drmDropMaster(scrpriv->dri_fd);
            }
#endif
        } else {
            LogMessage(X_WARNING, "Xfbdev(%d): Could not open %s: %s\n", pScreen->myNum, config->fbdev_dri_path, strerror(errno));
        }
    } else {
        scrpriv->dri_fd = -1;
    }

    if (scrpriv->dri_fd >= 0) {
        config->fbdev_auto_dri3 = FALSE;
    }

    glamor_egl_conf_t glamor_egl_conf = {
                                         .screen = pScreen,
                                         .glvnd_vendor = config->fbdev_glvnd_provider,
                                         .fd = scrpriv->dri_fd,
                                         .auto_dri = config->fbdev_auto_dri3,
                                         .llvmpipe_allowed = TRUE,
                                         .force_glamor = TRUE,
                                         .es_disallowed = !config->es_allowed,
                                         .force_es = config->force_es,
                                        };

    if (!glamor_egl_init_internal(&glamor_egl_conf, NULL)) {
        return FALSE;
    }

    if (config->fbdev_auto_dri3) {
        scrpriv->dri_fd = glamor_egl_get_fd(pScreen);
    }

    const char *renderer = (const char*)glGetString(GL_RENDERER);

    int flags = GLAMOR_USE_EGL_SCREEN;
    if (!config->fbGlamorAllowed) {
        flags |= GLAMOR_NO_RENDER_ACCEL;
    } else if (!config->fbForceGlamor){
        if (!renderer ||
            strstr(renderer, "softpipe") ||
            strstr(renderer, "llvmpipe")) {
            flags |= GLAMOR_NO_RENDER_ACCEL;
        }
    }

    if (scrpriv->dri_fd < 0 ||
        flags & GLAMOR_NO_RENDER_ACCEL) {
        flags |= GLAMOR_NO_DRI3;
    }

    if (!glamor_init(pScreen, flags)) {
        return FALSE;
    }

#ifdef XV
    /* X-Video needs glamor render accel */
    if (config->fbXVAllowed && !(flags & GLAMOR_NO_RENDER_ACCEL)) {
        kd_glamor_xv_init(pScreen);
    }
#endif

    return TRUE;
}

void
fbdevEnableAccel(ScreenPtr pScreen)
{
#ifdef WITH_LIBDRM
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    FbdevScrPriv *scrpriv = screen->driver;
    FbScreenConf *config = screen->card->closure;

    if (config->fbNoAccel) {
        return;
    }

    if (config->fbdev_drm_master && scrpriv->dri_fd >= 0) {
        drmSetMaster(scrpriv->dri_fd);
    }
#endif
}

void
fbdevDisableAccel(ScreenPtr pScreen)
{
#ifdef WITH_LIBDRM
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    FbdevScrPriv *scrpriv = screen->driver;
    FbScreenConf *config = screen->card->closure;

    if (config->fbNoAccel) {
        return;
    }

    if (config->fbdev_drm_master && scrpriv->dri_fd >= 0) {
        drmDropMaster(scrpriv->dri_fd);
    }
#endif
}

void
fbdevFiniAccel(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    FbdevScrPriv *scrpriv = screen->driver;
    FbScreenConf *config = screen->card->closure;

    if (config->fbNoAccel) {
        return;
    }

    if (scrpriv->dri_fd >= 0) {
        close(scrpriv->dri_fd);
    }
}
