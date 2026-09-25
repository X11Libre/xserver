/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111github@gmail.com>
 */

#include <kdrive-config.h>

#include "modesetting.h"

#ifdef RANDR

static Bool
msSetMode(ScreenPtr pScreen, int width, int height, int rate)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msPriv *priv = screen->card->driver;
    msScrPriv *scrpriv = screen->driver;
    struct gbm_bo *new_front = NULL;
    drmModeModeInfo *old_mode;
    int old_width, old_height, old_rate;

    /* TODO: Query scanout modifiers in CardInit,
     * intersect with render modifiers queried in msGlamorInit
     */
    uint64_t *modifiers = scrpriv->render_modifiers;
    int num_modifiers = scrpriv->num_render_modifiers;

    old_mode = scrpriv->mode;

    old_width = screen->width;
    old_height = screen->height;
    old_rate = screen->rate;

    /* Find the mode */
    scrpriv->mode = modesetting_find_mode(scrpriv->connector, width, height, rate);
    if (!scrpriv->mode) {
        goto bail;
    }

    screen->width = width;
    screen->height = height;
    screen->rate = rate;

    /* Create a new front with the new sizes */
    if (width != old_width ||
        height != old_height) {
        uint32_t format = gbm_bo_get_format(scrpriv->front);
        Bool do_map = !!gbm_bo_get_map(scrpriv->front);
        uint64_t modifier = gbm_bo_get_modifier(scrpriv->front);

        /* Try the current modifier first */
        new_front = gbm_create_front_bo(priv->gbm, do_map, width, height, format,
                                        &modifier, 1);
        if (!new_front) {
            new_front = gbm_create_front_bo(priv->gbm, do_map, width, height, format,
                                            modifiers, num_modifiers);
        }
        if (!new_front ||
            !msSetScreenBo(pScreen, new_front, FALSE /* flip */)) {
            goto bail;
        }
    } else {
        if (!msSetScreenBo(pScreen, scrpriv->front, TRUE /* flip */)) {
            goto bail;
        }
    }

    return TRUE;

bail:
    if (new_front) {
        gbm_bo_destroy(new_front);
    }

    screen->width = old_width;
    screen->height = old_height;
    screen->rate = old_rate;

    scrpriv->mode = old_mode;

    return FALSE;
}

static Bool
msRandRGetInfo(ScreenPtr pScreen, Rotation * rotations)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *scrpriv = screen->driver;
    Rotation randr;
    int n;

    *rotations = RR_Rotate_All | RR_Reflect_All;

    for (n = 0; n < pScreen->numDepths; n++) {
        if (pScreen->allowedDepths[n].numVids) {
            break;
        }
    }

    if (n == pScreen->numDepths) {
        return FALSE;
    }

    randr = KdSubRotation(scrpriv->randr, screen->randr);

    for (int i = 0; i < scrpriv->connector->count_modes; i++) {
        drmModeModeInfo *mode = &scrpriv->connector->modes[i];
        RRScreenSizePtr pSize;
        pSize = RRRegisterSize(pScreen,
                               mode->hdisplay,
                               mode->vdisplay,
                               screen->width_mm, screen->height_mm);

        RRRegisterRate(pScreen, pSize, mode->vrefresh);

        if (mode->hdisplay == screen->width &&
            mode->vdisplay == screen->height &&
            mode->vrefresh == screen->rate) {
            RRSetCurrentConfig(pScreen, randr, mode->vrefresh, pSize);
        }
    }

    return TRUE;
}

static Bool
msRandRSetConfig(ScreenPtr pScreen,
                    Rotation randr, int rate, RRScreenSizePtr pSize)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *scrpriv = screen->driver;

    int oldmmwidth;
    int oldmmheight;
    int newmmwidth;
    int newmmheight;

    if (screen->randr & (RR_Rotate_0 | RR_Rotate_180)) {
        newmmwidth = pSize->mmWidth;
        newmmheight = pSize->mmHeight;
    } else {
        newmmwidth = pSize->mmHeight;
        newmmheight = pSize->mmWidth;
    }

    oldmmwidth = pScreen->mmWidth;
    oldmmheight = pScreen->mmHeight;

    /*
     * Set new configuration
     */

    scrpriv->randr = KdAddRotation(screen->randr, randr);

    pScreen->mmWidth = newmmwidth;
    pScreen->mmHeight = newmmheight;

    if (!msSetMode(pScreen, pSize->width, pSize->height, rate)) {
        goto bail;
    }

    return TRUE;

bail:
    pScreen->mmWidth = oldmmwidth;
    pScreen->mmHeight = oldmmheight;
    return FALSE;
}

static Bool
msGetPhysicalScreenSizes(ScreenPtr pScreen, int *mmWidth, int *mmHeight)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *scrpriv = screen->driver;

    *mmWidth = screen->width_mm;
    *mmHeight = screen->height_mm;

    if (screen->requested_mm) {
        return TRUE;
    }

    if (((int)scrpriv->connector->mmWidth > 0) && ((int)scrpriv->connector->mmHeight > 0)) {
        *mmWidth = scrpriv->connector->mmWidth;
        *mmWidth = scrpriv->connector->mmHeight;
        return TRUE;
    }

    return FALSE;
}

static Bool
msRandRSetPhysicalScreenSizes(ScreenPtr pScreen)
{
    int mmWidth, mmHeight;

    if (msGetPhysicalScreenSizes(pScreen, &mmWidth, &mmHeight)) {
        RROutputPtr pOutput;
        pOutput = RRFirstOutput(pScreen);

        if (!pOutput) {
            return FALSE;
        }

        RROutputSetPhysicalSize(pOutput,
                                mmWidth,
                                mmHeight);
    }

    return TRUE;
}

#if RANDR_12_INTERFACE
static Bool
msRandRCrtcSetGamma(ScreenPtr pScreen, RRCrtcPtr crtc)
{
    return msSetGamma(pScreen, crtc->gammaSize, crtc->gammaRed, crtc->gammaGreen, crtc->gammaBlue);
}

static Bool
msRandRGammaInit(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *priv = screen->driver;
    RROutputPtr pOutput;
    RRCrtcPtr crtc;

    uint16_t *gamma;
    int gamma_size;

    pOutput = RRFirstOutput(pScreen);
    if (!pOutput) {
        return FALSE;
    }

    if (!pOutput->numCrtcs || !pOutput->crtcs) {
        return FALSE;
    }

    crtc = pOutput->crtc ? pOutput->crtc : pOutput->crtcs[0];
    if (!crtc) {
        return FALSE;
    }

    if (priv->crtc && priv->crtc->gamma_size) {
        gamma_size = priv->crtc->gamma_size;
    } else {
        switch (screen->fb.depth) {
        case 8:
            gamma_size = 1 << 8;
        case 15:
            gamma_size = 1 << 5;
        case 16:
            gamma_size = 1 << 6;
        case 24:
            gamma_size = 1 << 8;
        case 30:
        default:
            gamma_size = 1 << 10;
        }
    }

    gamma = calloc(3 * gamma_size, sizeof(*gamma));
    if (!gamma) {
        return FALSE;
    }

    if (!RRCrtcGammaSetSize(crtc, gamma_size)) {
        return FALSE;
    }

    msGetGamma(pScreen, gamma_size, gamma, gamma + gamma_size, gamma + 2 * gamma_size);
    RRCrtcGammaSet(crtc, gamma, gamma + gamma_size, gamma + 2 * gamma_size);

    free(gamma);
    return TRUE;
}
#endif /* RANDR_12_INTERFACE */

Bool
msRandRInit(ScreenPtr pScreen)
{
    rrScrPrivPtr pScrPriv;

    if (!RRScreenInit(pScreen)) {
        return FALSE;
    }

    pScrPriv = rrGetScrPriv(pScreen);
    pScrPriv->rrGetInfo = msRandRGetInfo;
    pScrPriv->rrSetConfig = msRandRSetConfig;

    /* Create the output */
    RRGetInfo(pScreen, TRUE);

    msRandRSetPhysicalScreenSizes(pScreen);

#if RANDR_12_INTERFACE
    if (msRandRGammaInit(pScreen)) {
        pScrPriv->rrCrtcSetGamma = msRandRCrtcSetGamma;
    }
#endif
    return TRUE;
}
#endif /* RANDR */
