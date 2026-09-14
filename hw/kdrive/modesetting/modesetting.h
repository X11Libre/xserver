/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111github@gmail.com>
 */

#ifndef _MODESETTING_H_
#define _MODESETTING_H_
#include <stdio.h>
#include <unistd.h>

#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "kglamor.h"

#include "randrstr.h"

typedef struct _msPriv {
    struct gbm_device *gbm;
    drmModeRes *resources;

    __u16 red[256];
    __u16 green[256];
    __u16 blue[256];
} msPriv;

typedef struct _msScrPriv {
    struct gbm_bo *front;
    drmModeConnector *connector;
    drmModeModeInfo *mode;
    uint32_t conn_id;
    uint32_t crtc_id;

    DamagePtr damage;
    Rotation randr;
    Bool blockHandler;
    Bool shadow;
} msScrPriv;

typedef struct _msScreenConf {
    const char *dev_path;
    Bool shadow;
    KdGlamorInfo glamor_info;
    const char *dri_path;
} MsScreenConf;

extern KdCardFuncs msFuncs;

Bool msCardInit(KdCardInfo * card);

Bool msScreenInit(KdScreenInfo * screen);

Bool msInitScreen(ScreenPtr pScreen);

Bool msFinishInitScreen(ScreenPtr pScreen);

Bool msCreateResources(ScreenPtr pScreen);

void msPreserve(KdCardInfo * card);

Bool msEnable(ScreenPtr pScreen);

Bool msDPMS(ScreenPtr pScreen, int mode);

void msDisable(ScreenPtr pScreen);

void msRestore(KdCardInfo * card);

void msScreenFini(KdScreenInfo * screen);

void msCardFini(KdCardInfo * card);

#ifdef GLAMOR
Bool msInitAccel(ScreenPtr screen);

void msEnableAccel(ScreenPtr screen);

void msDisableAccel(ScreenPtr screen);

void msFiniAccel(ScreenPtr screen);
#endif

void msCloseScreen(ScreenPtr pScreen);

#endif				/* _MODESETTING_H_ */
