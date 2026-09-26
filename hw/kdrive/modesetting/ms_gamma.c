/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111github@gmail.com>
 */

#include <kdrive-config.h>

#include "modesetting.h"

Bool
msGetGamma(ScreenPtr pScreen, int size, uint16_t *r, uint16_t *g, uint16_t *b)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *priv = screen->driver;

    struct gbm_device *gbm;
    int fd;
    int sz = size;

    gbm = gbm_bo_get_device(priv->front);
    fd = gbm_device_get_fd(gbm);

    if (priv->crtc && priv->crtc->gamma_size < size) {
        sz = priv->crtc->gamma_size;
    }

    memset(r, 0, size * sizeof(*r));
    memset(g, 0, size * sizeof(*g));
    memset(b, 0, size * sizeof(*b));

    if (!drmModeCrtcGetGamma(fd, priv->crtc_id, sz, r, g, b)) {
        return TRUE;
    }

    memset(r, 0, size * sizeof(*r));
    memset(g, 0, size * sizeof(*g));
    memset(b, 0, size * sizeof(*b));
    return FALSE;
}

Bool
msSetGamma(ScreenPtr pScreen, int size, uint16_t *r, uint16_t *g, uint16_t *b)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *priv = screen->driver;

    struct gbm_device *gbm;
    int fd;
    int sz = size;

    gbm = gbm_bo_get_device(priv->front);
    fd = gbm_device_get_fd(gbm);

    if (priv->crtc && priv->crtc->gamma_size < size) {
        sz = priv->crtc->gamma_size;
    }

    return !drmModeCrtcSetGamma(fd, priv->crtc_id, sz, r, g, b);
}

void
msGetColors(ScreenPtr pScreen, int n, xColorItem * pdefs)
{
    uint16_t r[256] = {0}, g[256] = {0}, b[256] = {0};
    int max = 0;

    for (int i = 0; i < n; i++) {
        if (pdefs[i].pixel > max)
            max = pdefs[i].pixel;
        if (max >= sizeof(r) / sizeof(*r)) {
            max = sizeof(r) / sizeof(*r);
            break;
        }
    }

    msGetGamma(pScreen, max, r, g, b);

    for (int i = 0; i < n; i++) {
        int p = pdefs[i].pixel;
        if (p > max) {
            p = max;
        }
        if (p < 0) {
            p = 0;
        }
        pdefs[i].red = r[p];
        pdefs[i].green = g[p];
        pdefs[i].blue = b[p];
    }
}

void
msPutColors(ScreenPtr pScreen, int n, xColorItem * pdefs)
{
    uint16_t r[256] = {0}, g[256] = {0}, b[256] = {0};
    int max = 0;

    for (int i = 0; i < n; i++) {
        int p = pdefs[i].pixel;
        if (p > sizeof(r) / sizeof(*r)) {
            p = sizeof(r) / sizeof(*r);
        }
        if (p < 0) {
            p = 0;
        }
        if (p > max) {
            max = p;
        }
        r[p] = pdefs[i].red;
        g[p] = pdefs[i].green;
        b[p] = pdefs[i].blue;
    }

    msSetGamma(pScreen, max, r, g, b);
}
