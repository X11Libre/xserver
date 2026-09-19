/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111github@gmail.com>
 */

#ifndef _KGLAMOR_H_
#define _KGLAMOR_H_

#include "kdrive.h"

typedef struct {
    /* Passed to glamor_egl */
    const char* glvnd; /* glvnd vendor library or driver name */
    const char* dri_path; /* /dev/dri/[cardxx|renderDxxx] */
    Bool use_gbm; /* If glamor should try to use libgbm */
    Bool direct_dri3; /* If glamor should only use the direct DRI3 implementation */
    Bool force_gl; /* If glamor should only create desktop gl contexts */
    Bool force_es; /* If glamor should only create gles contexts */

    /* Handled by KdGlamorInit */
    Bool want_dri_fd; /* If dri_fd is already populated */
    int fake_rate; /* If a fake refresh rate should be used by present */
    Bool drop_master; /* If KdGlamorInit should call drmDropMaster on the dri fd */
    Bool no_xv; /* Disable X-Video support */
    Bool no_render_accel; /* Disable render acceleration */
    Bool force_render_accel; /* Enable render acceleration on top of sw renderers */

    /* Populated and closed by KdGlamorInit / KdGlamorFini */
    int dri_fd;
} KdGlamorInfo;

#ifdef GLAMOR
Bool KdGlamorInit(ScreenPtr pScreen, KdGlamorInfo *info, int *caps);
void KdGlamorEnable(ScreenPtr pScreen);
void KdGlamorDisable(ScreenPtr pScreen);
void KdGlamorFini(ScreenPtr pScreen, KdGlamorInfo *info);
#endif

int KdGlamorParse(KdGlamorInfo *info, int argc, char **argv, int i);
void KdGlamorLogScreenInfo(const KdGlamorInfo *info, int screen_num);
void KdGlamorUseMsg(void);

/* Put each screen on a different card */
void KdEnsureCard(int argc, char **argv, int i, Bool force);

#endif /* _KGLAMOR_H_ */
