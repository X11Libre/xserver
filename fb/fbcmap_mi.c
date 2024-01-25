/*
 * Copyright (c) 1987, Oracle and/or its affiliates.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * This version of fbcmap.c is implemented in terms of mi functions.
 * These functions used to be in fbcmap.c and depended upon the symbol
 * XFree86Server being defined.
 */

#include <dix-config.h>

#include <X11/X.h>

#include "dix/colormap_priv.h"

#include "fb.h"
#include "micmap.h"

int
fbListInstalledColormaps(ScreenPtr pScreen, Colormap * pmaps)
{
    return miListInstalledColormaps(pScreen, pmaps);
}

void
fbInstallColormap(ColormapPtr pmap)
{
    miInstallColormap(pmap);
}

void
fbUninstallColormap(ColormapPtr pmap)
{
    miUninstallColormap(pmap);
}

void
fbResolveColor(unsigned short *pred,
               unsigned short *pgreen, unsigned short *pblue, VisualPtr pVisual)
{
    miResolveColor(pred, pgreen, pblue, pVisual);
}

Bool
fbInitializeColormap(ColormapPtr pmap)
{
    return miInitializeColormap(pmap);
}

Bool
mfbCreateColormap(ColormapPtr pmap)
{
    ScreenPtr	pScreen;
    unsigned short  red0, green0, blue0;
    unsigned short  red1, green1, blue1;
    Pixel pix;

    pScreen = pmap->pScreen;
    if (pScreen->whitePixel == 0)
    {
	red0 = green0 = blue0 = ~0;
	red1 = green1 = blue1 = 0;
    }
    else
    {
	red0 = green0 = blue0 = 0;
	red1 = green1 = blue1 = ~0;
    }

    /* this is a monochrome colormap, it only has two entries, just fill
     * them in by hand.  If it were a more complex static map, it would be
     * worth writing a for loop or three to initialize it */

    /* this will be pixel 0 */
    pix = 0;
    if (AllocColor(pmap, &red0, &green0, &blue0, &pix, 0) != Success)
	return FALSE;

    /* this will be pixel 1 */
    if (AllocColor(pmap, &red1, &green1, &blue1, &pix, 0) != Success)
	return FALSE;
    return TRUE;
}

int
fbExpandDirectColors(ColormapPtr pmap,
                     int ndef, xColorItem * indefs, xColorItem * outdefs)
{
    return miExpandDirectColors(pmap, ndef, indefs, outdefs);
}

Bool
fbCreateDefColormap(ScreenPtr pScreen)
{
    return miCreateDefColormap(pScreen);
}

void
fbClearVisualTypes(void)
{
    miClearVisualTypes();
}

Bool
fbSetVisualTypes(int depth, int visuals, int bitsPerRGB)
{
    return miSetVisualTypes(depth, visuals, bitsPerRGB, -1);
}

Bool
fbSetVisualTypesAndMasks(int depth, int visuals, int bitsPerRGB,
                         Pixel redMask, Pixel greenMask, Pixel blueMask)
{
    return miSetVisualTypesAndMasks(depth, visuals, bitsPerRGB, -1,
                                    redMask, greenMask, blueMask);
}

/*
 * Given a list of formats for a screen, create a list
 * of visuals and depths for the screen which correspond to
 * the set which can be used with this version of fb.
 */
Bool
fbInitVisuals(VisualPtr * visualp,
              DepthPtr * depthp,
              int *nvisualp,
              int *ndepthp,
              int *rootDepthp,
              VisualID * defaultVisp, unsigned long sizes, int bitsPerRGB)
{
    return miInitVisuals(visualp, depthp, nvisualp, ndepthp, rootDepthp,
                         defaultVisp, sizes, bitsPerRGB, -1);
}
