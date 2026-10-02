/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111github@gmail.com>
 */

#include <kdrive-config.h>

#include "modesetting.h"

#include "cursorstr.h"
#include "mi/mipointer_priv.h"

#include <errno.h>

static Bool msShowCursor(ScreenPtr pScreen);

static void
msGetCursorSizes(int fd, int *w, int *h)
{
    uint64_t value = 0;

    int ret1 = drmGetCap(fd, DRM_CAP_CURSOR_WIDTH, &value);
    *w = value;

    int ret2 = drmGetCap(fd, DRM_CAP_CURSOR_HEIGHT, &value);
    *h = value;

    if (ret1 || ret2) {
        *w = 64;
        *h = 64;
    }

    *w &= ~31;

    if (*w > 256) {
        *w = 256;
    }

    if (*h > 256) {
        *h = 256;
    }
}

static void
msQueryBestSize(int class, unsigned short *pwidth, unsigned short *pheight,
                ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *scrpriv = screen->driver;
    msCursPriv *pCurPriv = &scrpriv->cursor;
    uint32_t width = gbm_bo_get_width(pCurPriv->bo);
    uint32_t height = gbm_bo_get_width(pCurPriv->bo);

    switch (class) {
    case CursorShape:
        if (*pwidth > width)
            *pwidth = width;
        if (*pheight > height)
            *pheight = height;
        if (*pwidth > pScreen->width)
            *pwidth = pScreen->width;
        if (*pheight > pScreen->height)
            *pheight = pScreen->height;
        break;
    default:
        (*pCurPriv->QueryBestSize)(class, pwidth, pheight, pScreen);
        break;
    }
}

/* Adapted from kdrive ati_cursor.c RadeonLoadCursor */
static Bool
msLoadCursor(ScreenPtr pScreen, CursorPtr pCursor)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *scrpriv = screen->driver;
    msCursPriv *pCurPriv = &scrpriv->cursor;
    CursorBitsPtr bits = pCursor->bits;
    uint32_t *msk, *mskLine, *src, *srcLine;

    uint32_t width = gbm_bo_get_width(pCurPriv->bo);
    uint32_t height = gbm_bo_get_width(pCurPriv->bo);
    uint32_t stride = gbm_bo_get_stride(pCurPriv->bo) / sizeof(uint32_t);
    uint32_t *ram = gbm_bo_get_map(pCurPriv->bo);

    int x, y;

    int w = bits->width;
    int h = bits->height;

    if (w > width) {
        w = width;
    }
    if (h > height) {
        h = height;
    }

    /* TODO: Deal with rotations */

    if (bits->argb) {
        srcLine = bits->argb;
        for (y = 0; y < h; y++) {
            src = srcLine;
            srcLine += bits->width;
            for (x = 0; x < w; x++) {
                ram[y * stride + x] = *src++;
            }
            memset(ram + y * stride + x, 0, (width - x) * sizeof(*ram));
        }
        for (; y < height; y++) {
            memset(ram + y * stride, 0, width * sizeof(*ram));
        }
    } else {
        uint32_t colors[4];
        colors[0] = 0;
        colors[1] = 0;
        colors[2] = (((pCursor->backRed   >> 8) << 16) |
                     ((pCursor->backGreen >> 8) <<  8) |
                     ((pCursor->backBlue  >> 8) <<  0) |
                     0xff000000);
        colors[3] = (((pCursor->foreRed   >> 8) << 16) |
                     ((pCursor->foreGreen >> 8) <<  8) |
                     ((pCursor->foreBlue  >> 8) <<  0) |
                     0xff000000);
        mskLine = (uint32_t*)bits->mask;
        srcLine = (uint32_t*)bits->source;

        /* words per line */
        int lwsrc = BitmapBytePad(bits->width) / sizeof(uint32_t);

        for (y = 0; y < height; y++) {
            uint32_t m, s;

            msk = mskLine;
            src = srcLine;
            mskLine += lwsrc;
            srcLine += lwsrc;

            for (x = 0; x < width / 32; x++) {
                if (y < h && x < lwsrc) {
                    m = *msk++;
                    s = *src++;
                } else {
                    m = 0;
                    s = 0;
                }

                for (int k = 0; k < 32; k++) {
                    uint32_t val = (s & 1) | ((m & 1) << 1);
                    ram[y * stride + x * 32 + k] = colors[val];
                    s >>= 1;
                    m >>= 1;
                }
            }
        }
    }

    /* Save the cursor glyph hotspot */
    pCurPriv->xhot = bits->xhot;
    pCurPriv->yhot = bits->yhot;

    return msShowCursor(pScreen);
}

static Bool
msShowCursor(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msPriv *priv = screen->card->driver;
    msScrPriv *scrpriv = screen->driver;
    msCursPriv *pCurPriv = &scrpriv->cursor;
    uint32_t width = gbm_bo_get_width(pCurPriv->bo);
    uint32_t height = gbm_bo_get_width(pCurPriv->bo);
    int fd = gbm_device_get_fd(priv->gbm);
    uint32_t handle = gbm_bo_get_handle(pCurPriv->bo).u32;

    return !drmModeSetCursor(fd, scrpriv->crtc_id, handle, width, height) ||
           !drmModeSetCursor2(fd, scrpriv->crtc_id, handle, width, height, 0 /* xhot */, 0 /* yhot */);
}

static Bool
msUnloadCursor(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msPriv *priv = screen->card->driver;
    msScrPriv *scrpriv = screen->driver;
    int fd = gbm_device_get_fd(priv->gbm);

    return !drmModeSetCursor(fd, scrpriv->crtc_id, 0, 0, 0) ||
           !drmModeSetCursor2(fd, scrpriv->crtc_id, 0, 0, 0, 0, 0);
}


static Bool
msRealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCurs)
{
    return TRUE;
}

static Bool
msUnrealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCurs)
{
    return TRUE;
}

static void
msMoveCursor(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msPriv *priv = screen->card->driver;
    msScrPriv *scrpriv = screen->driver;
    msCursPriv *pCurPriv = &scrpriv->cursor;

    int fd = gbm_device_get_fd(priv->gbm);

    /* Offset the cursor position, so that the cursor hotspot looks right */
    x -= pCurPriv->xhot;
    y -= pCurPriv->yhot;

    if (x < 0) {
        x = 0;
    }

    if (y < 0) {
        y = 0;
    }

    drmModeMoveCursor(fd, scrpriv->crtc_id, x, y);
}

static void
msSetCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor, int x, int y)
{
    KdScreenPriv(pScreen);

    if (!pScreenPriv->enabled) {
        return;
    }

    if (pCursor) {
        msLoadCursor(pScreen, pCursor);
        msMoveCursor(pDev, pScreen, x, y);
    } else {
        msUnloadCursor(pScreen);
    }
}

static Bool
msDeviceCursorInitialize(DeviceIntPtr pDev, ScreenPtr pScreen)
{
    return TRUE;
}

static void
msDeviceCursorCleanup(DeviceIntPtr pDev, ScreenPtr pScreen)
{
    return;
}

static miPointerSpriteFuncRec msPointerSpriteFuncs = {
    msRealizeCursor,
    msUnrealizeCursor,
    msSetCursor,
    msMoveCursor,
    msDeviceCursorInitialize,
    msDeviceCursorCleanup
};

Bool
msCursorInit(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msPriv *priv = screen->card->driver;
    msScrPriv *scrpriv = screen->driver;
    msCursPriv *pCurPriv = &scrpriv->cursor;

    int fd = gbm_device_get_fd(priv->gbm);
    int width, height;

    msGetCursorSizes(fd, &width, &height);

    if (width <= 0 || height <= 0) {
        return FALSE;
    }

    /* See if hw cursor is supported */
    if (drmModeSetCursor(fd, scrpriv->crtc_id, 0, 0, 0) && (errno == ENOSYS || errno == ENXIO)) {
        if (drmModeSetCursor2(fd, scrpriv->crtc_id, 0, 0, 0, 0, 0) && (errno == ENOSYS)) {
            return FALSE;
        }
    }

    pCurPriv->bo = gbm_create_cursor_bo(priv->gbm, width, height);
    if (!pCurPriv->bo) {
        return FALSE;
    }

    pCurPriv->QueryBestSize = pScreen->QueryBestSize;
    pScreen->QueryBestSize = msQueryBestSize;

    if (!miPointerInitialize(pScreen,
                             &msPointerSpriteFuncs,
                             &kdPointerScreenFuncs,
                             FALSE /* waitForUpdate */)) {
        gbm_bo_destroy(pCurPriv->bo);
        pCurPriv->bo = NULL;
        return FALSE;
    }

    LogMessage(X_INFO, "Xmodesetting(%d): Using a %dx%d hw cursor\n", pScreen->myNum, width, height);
    return TRUE;
}

void
msCursorEnable(ScreenPtr pScreen)
{
    msShowCursor(pScreen);
}

void
msCursorDisable(ScreenPtr pScreen)
{
    msUnloadCursor(pScreen);
}

void
msRecolorCursor(ScreenPtr pScreen, int ndef, xColorItem *pdef)
{
}

void
msCursorFini(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *scrpriv = screen->driver;
    msCursPriv *pCurPriv = &scrpriv->cursor;

    pScreen->QueryBestSize = pCurPriv->QueryBestSize;

    gbm_bo_destroy(pCurPriv->bo);
}
