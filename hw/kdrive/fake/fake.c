/*
 * Copyright © 2004 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <kdrive-config.h>

#include "fake.h"

Bool
fakeInitialize(KdCardInfo * card, FakePriv * priv)
{
    priv->base = 0;
    priv->bytes_per_line = 0;
    return TRUE;
}

Bool
fakeCardInit(KdCardInfo * card)
{
    FakePriv *priv;

    priv = (FakePriv *) malloc(sizeof(FakePriv));
    if (!priv)
        return FALSE;

    if (!fakeInitialize(card, priv)) {
        free(priv);
        return FALSE;
    }
    card->driver = priv;

    return TRUE;
}

static Bool
fakeModeSupported(KdScreenInfo * screen, const KdMonitorTiming * t)
{
    return TRUE;
}

static Bool
fakeRandRModeSupported(ScreenPtr pScreen, const KdMonitorTiming * t)
{
    return TRUE;
}

Bool
fakeScreenInitialize(KdScreenInfo * screen, FakeScrPriv * scrpriv)
{
    if (!screen->width || !screen->height) {
        screen->width = 1024;
        screen->height = 768;
        screen->rate = 72;
    }

    if (screen->width <= 0)
        screen->width = 1;
    if (screen->height <= 0)
        screen->height = 1;

    if (!screen->fb.depth)
        screen->fb.depth = 16;

    if (screen->fb.depth <= 8) {
        screen->fb.visuals = ((1 << StaticGray) |
                              (1 << GrayScale) |
                              (1 << StaticColor) |
                              (1 << PseudoColor) |
                              (1 << TrueColor) | (1 << DirectColor));
    }
    else {
        screen->fb.visuals = (1 << TrueColor);
#define Mask(o,l)   (((1 << l) - 1) << o)
        if (screen->fb.depth <= 15) {
            screen->fb.depth = 15;
            screen->fb.bitsPerPixel = 16;
            screen->fb.redMask = Mask (10, 5);
            screen->fb.greenMask = Mask (5, 5);
            screen->fb.blueMask = Mask (0, 5);
        }
        else if (screen->fb.depth <= 16) {
            screen->fb.depth = 16;
            screen->fb.bitsPerPixel = 16;
            screen->fb.redMask = Mask (11, 5);
            screen->fb.greenMask = Mask (5, 6);
            screen->fb.blueMask = Mask (0, 5);
        }
        else {
            screen->fb.depth = 24;
            screen->fb.bitsPerPixel = 32;
            screen->fb.redMask = Mask (16, 8);
            screen->fb.greenMask = Mask (8, 8);
            screen->fb.blueMask = Mask (0, 8);
        }
    }

    scrpriv->randr = screen->randr;

    if (!KdFindRate(screen, fakeModeSupported)) {
        KdAddModeCVT(screen->width, screen->height, screen->rate);
    }

    return fakeMapFramebuffer(screen);
}

Bool
fakeScreenInit(KdScreenInfo * screen)
{
    FakeScrPriv *scrpriv;

    scrpriv = calloc(1, sizeof(FakeScrPriv));
    if (!scrpriv)
        return FALSE;
    screen->driver = scrpriv;
    if (!fakeScreenInitialize(screen, scrpriv)) {
        screen->driver = 0;
        free(scrpriv);
        return FALSE;
    }
    return TRUE;
}

void *
fakeWindowLinear(ScreenPtr pScreen,
                 CARD32 row,
                 CARD32 offset, int mode, CARD32 *size, void *closure)
{
    KdScreenPriv(pScreen);
    FakePriv *priv = pScreenPriv->card->driver;

    if (!pScreenPriv->enabled)
        return 0;
    *size = priv->bytes_per_line;
    return priv->base + row * priv->bytes_per_line + offset;
}

Bool
fakeMapFramebuffer(KdScreenInfo * screen)
{
    FakeScrPriv *scrpriv = screen->driver;
    KdPointerMatrix m;
    FakePriv *priv = screen->card->driver;
    FakeScreenConf *config = screen->card->closure;

    if (config->shadow >= 0) {
        scrpriv->shadow = config->shadow;
    } else if (scrpriv->randr != RR_Rotate_0) {
        scrpriv->shadow = TRUE;
    } else {
        scrpriv->shadow = FALSE;
    }

    KdComputePointerMatrix(&m, scrpriv->randr, screen->width, screen->height);

    KdSetPointerMatrix(&m);

    priv->bytes_per_line =
        ((screen->width * screen->fb.bitsPerPixel + 31) >> 5) << 2;
    free(priv->base);
    priv->base = calloc(priv->bytes_per_line, screen->height);
    if (!priv->base) {
        return FALSE;
    }

    if (scrpriv->shadow) {
        if (!KdShadowFbAlloc
            (screen, scrpriv->randr & (RR_Rotate_90 | RR_Rotate_270)))
            return FALSE;
    }
    else {
        screen->fb.byteStride = priv->bytes_per_line;
        screen->fb.pixelStride = (priv->bytes_per_line * 8 /
                                  screen->fb.bitsPerPixel);
        screen->fb.frameBuffer = (CARD8 *) (priv->base);
    }

    return TRUE;
}

void
fakeSetScreenSizes(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    FakeScrPriv *scrpriv = screen->driver;

    if (scrpriv->randr & (RR_Rotate_0 | RR_Rotate_180)) {
        pScreen->width = screen->width;
        pScreen->height = screen->height;
        pScreen->mmWidth = screen->width_mm;
        pScreen->mmHeight = screen->height_mm;
    }
    else {
        pScreen->width = screen->height;
        pScreen->height = screen->width;
        pScreen->mmWidth = screen->height_mm;
        pScreen->mmHeight = screen->width_mm;
    }
}

Bool
fakeUnmapFramebuffer(KdScreenInfo * screen)
{
    FakePriv *priv = screen->card->driver;

    KdShadowFbFree(screen);
    free(priv->base);
    priv->base = NULL;
    return TRUE;
}

Bool
fakeSetShadow(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    FakeScrPriv *scrpriv = screen->driver;
    ShadowUpdateProc update;
    ShadowWindowProc window;

    window = fakeWindowLinear;
    update = 0;
    if (scrpriv->randr)
        update = shadowUpdateRotatePacked;
    else
        update = shadowUpdatePacked;
    return KdShadowSet(pScreen, scrpriv->randr, update, window);
}

#ifdef RANDR
Bool
fakeRandRGetInfo(ScreenPtr pScreen, Rotation * rotations)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    FakeScrPriv *scrpriv = screen->driver;
    Rotation randr;
    int n;

    *rotations = RR_Rotate_All | RR_Reflect_All;

    for (n = 0; n < pScreen->numDepths; n++)
        if (pScreen->allowedDepths[n].numVids)
            break;
    if (n == pScreen->numDepths)
        return FALSE;

    randr = KdSubRotation(scrpriv->randr, screen->randr);

    return KdRandRGetInfo(pScreen, randr, fakeRandRModeSupported);
}

Bool
fakeRandRSetConfig(ScreenPtr pScreen,
                   Rotation randr, int rate, RRScreenSizePtr pSize)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    FakeScrPriv *scrpriv = screen->driver;
    Bool wasEnabled = pScreenPriv->enabled;
    FakeScrPriv oldscr;
    int oldwidth;
    int oldheight;
    int oldrate;
    int oldmmwidth;
    int oldmmheight;
    int newwidth, newheight, newmmwidth, newmmheight;

    if (screen->randr & (RR_Rotate_0 | RR_Rotate_180)) {
        newwidth = pSize->width;
        newheight = pSize->height;
        newmmwidth = pSize->mmWidth;
        newmmheight = pSize->mmHeight;
    }
    else {
        newwidth = pSize->height;
        newheight = pSize->width;
        newmmwidth = pSize->mmHeight;
        newmmheight = pSize->mmWidth;
    }

    if (wasEnabled)
        KdDisableScreen(pScreen);

    oldscr = *scrpriv;

    oldwidth = screen->width;
    oldheight = screen->height;
    oldrate = screen->rate;
    oldmmwidth = pScreen->mmWidth;
    oldmmheight = pScreen->mmHeight;

    /*
     * Set new configuration
     */

    scrpriv->randr = KdAddRotation(screen->randr, randr);

    /* With real hw, these would be set by the mode setting function to the real hw mode sizes */
    (void)newwidth;
    (void)newheight;
    screen->width = pSize->width;
    screen->height = pSize->height;
    screen->rate = rate;

    pScreen->mmWidth = newmmwidth;
    pScreen->mmHeight = newmmheight;

    fakeUnmapFramebuffer(screen);

    if (!fakeMapFramebuffer(screen))
        goto bail4;

    KdShadowUnset(screen->pScreen);

    if (!fakeSetShadow(screen->pScreen))
        goto bail4;

    fakeSetScreenSizes(screen->pScreen);

    /*
     * Set frame buffer mapping
     */
    (*pScreen->ModifyPixmapHeader) ((*pScreen->GetScreenPixmap)(pScreen),
                                    pScreen->width,
                                    pScreen->height,
                                    screen->fb.depth,
                                    screen->fb.bitsPerPixel,
                                    screen->fb.byteStride,
                                    screen->fb.frameBuffer);

    /* set the subpixel order */

    KdSetSubpixelOrder(pScreen, scrpriv->randr);
    if (wasEnabled)
        KdEnableScreen(pScreen);

    return TRUE;

 bail4:
    fakeUnmapFramebuffer(screen);
    *scrpriv = oldscr;
    (void) fakeMapFramebuffer(screen);
    pScreen->width = oldwidth;
    pScreen->height = oldheight;
    screen->rate = oldrate;
    pScreen->mmWidth = oldmmwidth;
    pScreen->mmHeight = oldmmheight;

    if (wasEnabled)
        KdEnableScreen(pScreen);
    return FALSE;
}

Bool
fakeRandRInit(ScreenPtr pScreen)
{
    rrScrPrivPtr pScrPriv;

    if (!RRScreenInit(pScreen))
        return FALSE;

    pScrPriv = rrGetScrPriv(pScreen);
    pScrPriv->rrGetInfo = fakeRandRGetInfo;
    pScrPriv->rrSetConfig = fakeRandRSetConfig;
    return TRUE;
}
#endif

Bool
fakeCreateColormap(ColormapPtr pmap)
{
    return fbInitializeColormap(pmap);
}

Bool
fakeInitScreen(ScreenPtr pScreen)
{
    pScreen->CreateColormap = fakeCreateColormap;
    return TRUE;
}

Bool
fakeFinishInitScreen(ScreenPtr pScreen)
{
    if (!shadowSetup(pScreen))
        return FALSE;

#ifdef RANDR
    if (!fakeRandRInit(pScreen))
        return FALSE;
#endif

    return TRUE;
}

Bool
fakeCreateResources(ScreenPtr pScreen)
{
    return fakeSetShadow(pScreen);
}

void
fakePreserve(KdCardInfo * card)
{
}

Bool
fakeEnable(ScreenPtr pScreen)
{
    return TRUE;
}

Bool
fakeDPMS(ScreenPtr pScreen, int mode)
{
    return TRUE;
}

void
fakeDisable(ScreenPtr pScreen)
{
}

void
fakeRestore(KdCardInfo * card)
{
}

void
fakeScreenFini(KdScreenInfo * screen)
{
}

void
fakeCardFini(KdCardInfo * card)
{
    FakePriv *priv = card->driver;

    free(priv->base);
    free(priv);

    free(card->closure);
    card->closure = NULL;
}

void
fakeGetColors(ScreenPtr pScreen, int n, xColorItem * pdefs)
{
    while (n--) {
        pdefs->red = 0;
        pdefs->green = 0;
        pdefs->blue = 0;
        pdefs++;
    }
}

void
fakePutColors(ScreenPtr pScreen, int n, xColorItem * pdefs)
{
}
