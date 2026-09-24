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

Bool msMapFramebuffer(KdScreenInfo * screen);

Bool msUnmapFramebuffer(KdScreenInfo * screen);

Bool msSetShadow(ScreenPtr pScreen);

struct gbm_bo*
modesetting_open(msPriv *priv, KdScreenInfo *screen, Bool need_map);

/* ms_damage.c */

Bool msDamageCreateRes(ScreenPtr pScreen);

void msDamageCloseScreen(ScreenPtr pScreen);

/* ms_gamma.c */

Bool
msGetGamma(ScreenPtr pScreen, int size, uint16_t *r, uint16_t *g, uint16_t *b);

Bool
msSetGamma(ScreenPtr pScreen, int size, uint16_t *r, uint16_t *g, uint16_t *b);

void msGetColors(ScreenPtr pScreen, int n, xColorItem * pdefs);

void msPutColors(ScreenPtr pScreen, int n, xColorItem * pdefs);

#ifdef GLAMOR
/* ms_glamor.c */

Bool msGlamorCreateRes(ScreenPtr pScreen);

Bool msInitAccel(ScreenPtr screen);

void msEnableAccel(ScreenPtr screen);

void msDisableAccel(ScreenPtr screen);

void msFiniAccel(ScreenPtr screen);
#endif /* GLAMOR */

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

void
gbm_bo_set_screen_fb_info(struct gbm_bo *bo, KdScreenInfo *screen, Bool is_gles);

/* ms_query.c */

drmModeModeInfo*
modesetting_find_mode(drmModeConnector *conn, uint32_t req_w, uint32_t req_h, uint32_t req_rate);

drmModeConnector*
modesetting_find_connector(msPriv *priv, int fd, uint32_t *conn_id);

int
modeseting_find_crtc(msPriv *priv, int fd, drmModeConnector *conn);

#ifdef RANDR
/* ms_randr.c */

Bool msRandRInit(ScreenPtr pScreen);
#endif /* RANDR */

/* ms_util.c */

Bool msSetScreenBo(ScreenPtr pScreen, struct gbm_bo *bo, Bool flip);

Bool msFdMatch(int fd1, int fd2);

KdCardInfo* msFindMatchingCard(const char *card_path);
#endif				/* _MODESETTING_H_ */
