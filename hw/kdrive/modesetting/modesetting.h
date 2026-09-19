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

#ifdef RANDR
#include "randrstr.h"
#endif

typedef struct _msPriv {
    struct gbm_device *gbm;
    drmModeRes *resources;

    int *used_crtcs;
    int num_used_crtcs;

    int num_used_connectors;
    uint32_t *used_connectors;
} msPriv;

typedef struct _msScrPriv {
    struct gbm_bo *front;
    drmModeConnector *connector;
    drmModeModeInfo *mode;
    drmModeCrtcPtr crtc;
    uint32_t conn_id;
    uint32_t crtc_id;

    DamagePtr damage;
    Rotation randr;
    Bool blockHandler;
    Bool shadow;

    Bool error;
    Bool setPixmapBits;
} msScrPriv;

typedef struct _msScreenConf {
    Bool shadow;
    Bool format_swap;
    KdGlamorInfo glamor_info;
} MsScreenConf;

extern KdCardFuncs msFuncs;

KdCardInfo* msFindMatchingCard(const char *card_path);

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

Bool msSetScreenBo(ScreenPtr pScreen, struct gbm_bo *bo, Bool flip);

/* ms_gamma.c */

void msGetColors(ScreenPtr pScreen, int n, xColorItem * pdefs);

void msPutColors(ScreenPtr pScreen, int n, xColorItem * pdefs);

#ifdef RANDR
#if RANDR_12_INTERFACE
Bool msRandRCrtcSetGamma(ScreenPtr pScreen, RRCrtcPtr crtc);
#endif
Bool msRandRGammaInit(ScreenPtr pScreen);
#endif

#ifdef GLAMOR
/* ms_glamor.c */

Bool msGlamorCreateRes(ScreenPtr pScreen);

Bool msInitAccel(ScreenPtr screen);

void msEnableAccel(ScreenPtr screen);

void msDisableAccel(ScreenPtr screen);

void msFiniAccel(ScreenPtr screen);
#endif

void msCloseScreen(ScreenPtr pScreen);

Bool msMapFramebuffer(KdScreenInfo * screen);

Bool msUnmapFramebuffer(KdScreenInfo * screen);

Bool msSetShadow(ScreenPtr pScreen);

struct gbm_bo*
modesetting_open(msPriv *priv, KdScreenInfo *screen, Bool need_map);

/* ms_gbm.c */

void*
gbm_bo_get_map(struct gbm_bo *bo);

uint32_t
gbm_bo_get_fb(struct gbm_bo *bo);

Bool
gbm_bo_get_used_modifiers(struct gbm_bo *bo);

uint32_t
gbm_front_format_for_depth(int depth, int bpp, Bool rb_swap);

int
gbm_format_get_depth(uint32_t format);

struct gbm_bo*
gbm_create_front_bo(struct gbm_device *gbm, Bool do_map, uint32_t width, uint32_t height, uint32_t format);

#endif				/* _MODESETTING_H_ */
