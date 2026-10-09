/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111github@gmail.com>
 */

#include <kdrive-config.h>

#include "modesetting.h"

#include "cursorstr.h"
#include "mi/mipointer_priv.h"

#include <errno.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

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
    uint32_t width = pCurPriv->max_w;
    uint32_t height = pCurPriv->max_h;

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

static void
msResetCursor(msCursPriv *pCurPriv)
{
    gbm_bo_destroy(pCurPriv->bo);
    free(pCurPriv->shadow);

    pCurPriv->bo = NULL;
    pCurPriv->shadow = NULL;
    pCurPriv->pCursor = NULL;
    pCurPriv->x = 0;
    pCurPriv->y = 0;
    pCurPriv->old_width = 0;
    pCurPriv->old_height = 0;
    pCurPriv->xhot = 0;
    pCurPriv->yhot = 0;
}

static Bool
msLoadCursor(ScreenPtr pScreen, CursorPtr pCursor)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msPriv *priv = screen->card->driver;
    msScrPriv *scrpriv = screen->driver;
    msCursPriv *pCurPriv = &scrpriv->cursor;

    uint32_t width = gbm_bo_get_width(pCurPriv->bo);
    uint32_t height = gbm_bo_get_height(pCurPriv->bo);
    uint32_t stride = gbm_bo_get_stride(pCurPriv->bo) / sizeof(uint32_t);
    uint32_t *ram = gbm_bo_get_map(pCurPriv->bo);

    if (pCursor->bits->width > width ||
        pCursor->bits->height > height) {
        if (pCurPriv->max_w > width || pCurPriv->max_h > height) {
            struct gbm_bo *bo;
again:
            bo = gbm_create_cursor_bo(priv->gbm, pCurPriv->max_w, pCurPriv->max_h);
            if (bo) {
                msResetCursor(pCurPriv);
                pCurPriv->bo = bo;
                width = pCurPriv->max_w;
                height = pCurPriv->max_h;
                stride = gbm_bo_get_stride(pCurPriv->bo) / sizeof(uint32_t);
                ram = gbm_bo_get_map(pCurPriv->bo);
                LogMessage(X_INFO, "Xmodesetting(%d): Using a larger %dx%d hw cursor\n", pScreen->myNum, width, height);
            }
        }
    } else if (scrpriv->randr == RR_Rotate_0 &&
        pCurPriv->randr == RR_Rotate_0 &&
        pCurPriv->old_width && pCurPriv->old_height) {
        width = MAX(pCursor->bits->width, pCurPriv->old_width);
        width = ((width + 31) / 32) * 32;
        height = MAX(pCursor->bits->height, pCurPriv->old_height);
    }

    KdLoadCursorRandR(ram, pCursor,
                      width, height, stride,
                      scrpriv->randr, &pCurPriv->shadow,
                      &pCurPriv->xhot, &pCurPriv->yhot);

    pCurPriv->pCursor = pCursor;
    pCurPriv->randr = scrpriv->randr;

    pCurPriv->old_width = pCursor->bits->width;
    pCurPriv->old_height = pCursor->bits->height;

    if (!msShowCursor(pScreen)) {
        if (pCurPriv->max_w > width || pCurPriv->max_h> height) {
            /*
             * There is no guarantee that the 64x64 cursor is supported.
             * Try again with the largest cursor.
             */
            goto again;
        }
        return FALSE;
    }
    return TRUE;
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
    uint32_t height = gbm_bo_get_height(pCurPriv->bo);
    int fd = gbm_device_get_fd(priv->gbm);
    uint32_t handle = gbm_bo_get_handle(pCurPriv->bo).u32;

    return !drmModeSetCursor(fd, scrpriv->crtc_id, handle, width, height) ||
           !drmModeSetCursor2(fd, scrpriv->crtc_id, handle, width, height, 0 /* xhot */, 0 /* yhot */);
}

static Bool
msUnloadCursor(ScreenPtr pScreen, Bool clear)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msPriv *priv = screen->card->driver;
    msScrPriv *scrpriv = screen->driver;
    msCursPriv *pCurPriv = &scrpriv->cursor;
    int fd = gbm_device_get_fd(priv->gbm);

    if (clear) {
        pCurPriv->pCursor = NULL;
    }

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

    pCurPriv->x = x;
    pCurPriv->y = y;

    KdGetCursorPosition(pScreen, scrpriv->randr,
                        pCurPriv->xhot, pCurPriv->yhot,
                        &x, &y);

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
        msUnloadCursor(pScreen, TRUE /* clear */);
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

    msGetCursorSizes(fd, &pCurPriv->max_w, &pCurPriv->max_h);

    /* Don't use such small hw cursors */
    if (pCurPriv->max_w < 64 || pCurPriv->max_h < 64) {
        return FALSE;
    }

    /* See if hw cursor is supported */
    if (drmModeSetCursor(fd, scrpriv->crtc_id, 0, 0, 0) && (errno == ENOSYS || errno == ENXIO)) {
        if (drmModeSetCursor2(fd, scrpriv->crtc_id, 0, 0, 0, 0, 0) && (errno == ENOSYS)) {
            return FALSE;
        }
    }

    if ((pCurPriv->bo = gbm_create_cursor_bo(priv->gbm, 64, 64))) {
        width = 64;
        height = 64;
    } else if ((pCurPriv->bo = gbm_create_cursor_bo(priv->gbm, pCurPriv->max_w, pCurPriv->max_h))) {
        width = pCurPriv->max_w;
        height = pCurPriv->max_h;
    } else {
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

    pCurPriv->randr = scrpriv->randr;

    LogMessage(X_INFO, "Xmodesetting(%d): Using a %dx%d hw cursor\n", pScreen->myNum, width, height);
    return TRUE;
}

void
msCursorEnable(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *scrpriv = screen->driver;
    msCursPriv *pCurPriv = &scrpriv->cursor;

    if (pCurPriv->pCursor &&
        scrpriv->randr == pCurPriv->randr) {
        msShowCursor(pScreen);
    } else {
        /* Repaint the cursor glyph */
        msSetCursor(NULL /* pDev */, pScreen, pCurPriv->pCursor, pCurPriv->x, pCurPriv->y);
    }
}

void
msCursorDisable(ScreenPtr pScreen)
{
    msUnloadCursor(pScreen, FALSE /* clear */);
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

    free(pCurPriv->shadow);

    pScreen->QueryBestSize = pCurPriv->QueryBestSize;

    gbm_bo_destroy(pCurPriv->bo);
}
