/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111github@gmail.com>
 */

#include <kdrive-config.h>

#include "modesetting.h"

#ifdef GLAMOR
#include "glamor.h"
#endif

/* XXX This really belongs in os/, like the version from glamor_egl */
Bool
msFdMatch(int fd1, int fd2)
{
    struct stat stat1, stat2;

    if (fd1 == fd2) {
        return TRUE;
    }

    if (fd1 < 0 || fd2 < 0) {
        return FALSE;
    }

    if (fstat(fd1, &stat1) < 0 ||
        fstat(fd2, &stat2) < 0) {
        return FALSE;
    }

    /**
     * From https://pubs.opengroup.org/onlinepubs/009696699/basedefs/sys/stat.h.html
     *
     * The st_ino and st_dev fields taken together uniquely identify the file within the system.
     */
    return (stat1.st_dev == stat2.st_dev) && (stat1.st_ino == stat2.st_ino);
}

static KdCardInfo*
msFindCardForFd(int fd)
{
    for (KdCardInfo *card = kdCardInfo; card; card = card->next) {
        const char *card_path = card->closure;
        int cardFd = card_path ? open(card_path, O_RDWR) : -1;
        Bool ret = msFdMatch(cardFd, fd);

        if (cardFd >= 0) {
            close(cardFd);
        }
        if (ret) {
            return card;
        }
    }

    return NULL;
}

KdCardInfo*
msFindMatchingCard(const char *card_path)
{
    KdCardInfo *card;
    int fd = card_path ? open(card_path, O_RDWR) : -1;

    card = msFindCardForFd(fd);

    if (fd >= 0) {
        close(fd);
    }
    return card;
}

static void
msSetScreenSizes(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *scrpriv = screen->driver;

    if (scrpriv->randr & (RR_Rotate_0 | RR_Rotate_180)) {
        pScreen->width = gbm_bo_get_width(scrpriv->front);
        pScreen->height = gbm_bo_get_height(scrpriv->front);
        pScreen->mmWidth = screen->width_mm;
        pScreen->mmHeight = screen->height_mm;
    } else {
        pScreen->width = gbm_bo_get_height(scrpriv->front);
        pScreen->height = gbm_bo_get_width(scrpriv->front);
        pScreen->mmWidth = screen->height_mm;
        pScreen->mmHeight = screen->width_mm;
    }
}

Bool
msSetScreenBo(ScreenPtr pScreen, struct gbm_bo *bo, Bool flip)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    msScrPriv *scrpriv = screen->driver;
    struct gbm_bo *old_front;
    Bool wasEnabled = pScreenPriv->enabled;
    msScrPriv oldscr;
    PixmapPtr rootPixmap;

    rootPixmap = (*pScreen->GetScreenPixmap)(pScreen);

    if (wasEnabled) {
        KdDisableScreen(pScreen);
    }

    oldscr = *scrpriv;

    old_front = scrpriv->front;

    msUnmapFramebuffer(screen);

    scrpriv->front = bo;

    if (!msMapFramebuffer(screen)) {
        goto bail;
    }

    KdShadowUnset(screen->pScreen);

    if (!msSetShadow(screen->pScreen)) {
        goto bail;
    }

    msSetScreenSizes(screen->pScreen);

    /*
     * Set frame buffer mapping
     */
    (*pScreen->ModifyPixmapHeader) (rootPixmap,
                                    pScreen->width,
                                    pScreen->height,
                                    screen->fb.depth,
                                    screen->fb.bitsPerPixel,
                                    screen->fb.byteStride,
                                    screen->fb.frameBuffer);

    /* set the subpixel order */

    KdSetSubpixelOrder(pScreen, scrpriv->randr);

    /* Texture the front if needed */
    if (!flip && !gbm_bo_get_map(bo)) {
#ifdef GLAMOR
        Bool used_modifiers = gbm_bo_get_used_modifiers(bo);
        if (screen->dumb ||
            !glamor_egl_create_textured_pixmap_from_gbm_bo(rootPixmap, bo, used_modifiers))
#endif
        {
            goto bail;
        }
    }

    /* Scan out the new bo on the screen's crtc */
    if (wasEnabled) {
        KdEnableScreen(pScreen);
    }

    if (!flip) {
        gbm_bo_destroy(old_front);
    }
    return TRUE;

bail:
    msUnmapFramebuffer(screen);
    old_front = scrpriv->front;
    *scrpriv = oldscr;
    msMapFramebuffer(screen);
    msSetScreenSizes(screen->pScreen);

    /*
     * Set frame buffer mapping
     */
    (*pScreen->ModifyPixmapHeader) (rootPixmap,
                                    pScreen->width,
                                    pScreen->height,
                                    screen->fb.depth,
                                    screen->fb.bitsPerPixel,
                                    screen->fb.byteStride,
                                    screen->fb.frameBuffer);
    if (wasEnabled) {
        KdEnableScreen(pScreen);
    }
    return FALSE;
}
