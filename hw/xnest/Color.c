/*

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/
#include <dix-config.h>

#include <X11/X.h>
#include <X11/Xdefs.h>
#include <X11/Xproto.h>

#include "dix/colormap_priv.h"

#include "scrnintstr.h"
#include "window.h"
#include "windowstr.h"
#include "resource.h"

#include "Xnest.h"
#include "xnest-xcb.h"

#include "Display.h"
#include "Screen.h"
#include "Color.h"
#include "Visual.h"
#include "XNWindow.h"
#include "Args.h"

#include <xcb/xcb_icccm.h>

DevPrivateKeyRec xnestColormapPrivateKeyRec;

static DevPrivateKeyRec cmapScrPrivateKeyRec;

#define cmapScrPrivateKey (&cmapScrPrivateKeyRec)

#define GetInstalledColormap(s) ((ColormapPtr) dixLookupPrivate(&(s)->devPrivates, cmapScrPrivateKey))
#define SetInstalledColormap(s,c) (dixSetPrivate(&(s)->devPrivates, cmapScrPrivateKey, c))

static Bool load_colormap(ColormapPtr pCmap, int ncolors, uint32_t *colors)
{
    xcb_generic_error_t *err = NULL;
    xcb_query_colors_reply_t *reply = xcb_query_colors_reply(
        xnestUpstreamInfo.conn,
        xcb_query_colors(
            xnestUpstreamInfo.conn,
            xnestColormap(pCmap),
            ncolors,
            colors),
        &err);

    if (!reply) {
        LogMessage(X_WARNING, "load_colormap(): missing reply for QueryColors request\n");
        free(colors);
        return FALSE;
    }

    if (xcb_query_colors_colors_length(reply) != ncolors) {
        LogMessage(X_WARNING, "load_colormap(): received wrong number of entries: %d - expected %d\n",
            xcb_query_colors_colors_length(reply), ncolors);
        free(reply);
        free(colors);
        return FALSE;
    }

    xcb_rgb_t *rgb = xcb_query_colors_colors(reply);
    for (int i = 0; i < ncolors; i++) {
        pCmap->red[i].co.local.red = rgb[i].red;
        pCmap->green[i].co.local.green = rgb[i].green;
        pCmap->blue[i].co.local.blue = rgb[i].blue;
    }

    free(colors);
    free(reply);
    return TRUE;
}

Bool
xnestCreateColormap(ColormapPtr pCmap)
{
    VisualPtr pVisual = pCmap->pVisual;
    int ncolors = pVisual->ColormapEntries;

    uint32_t const cmap = xcb_generate_id(xnestUpstreamInfo.conn);
    xnestColormapPriv(pCmap)->colormap = cmap;

    xcb_create_colormap(xnestUpstreamInfo.conn,
                        (pVisual->class & DynamicClass) ? XCB_COLORMAP_ALLOC_ALL : XCB_COLORMAP_ALLOC_NONE,
                        cmap,
                        xnestDefaultWindows[pCmap->pScreen->myNum],
                        xnestVisual(pVisual)->visualid);

    switch (pVisual->class) {
    case StaticGray:           /* read only */
    case StaticColor:          /* read only */
    {
        uint32_t *colors = malloc(ncolors * sizeof(uint32_t));
        for (int i = 0; i < ncolors; i++)
            colors[i] = i;
        return load_colormap(pCmap, ncolors, colors);
    }
    break;

    case TrueColor:            /* read only */
    {
        uint32_t *colors = malloc(ncolors * sizeof(uint32_t));
        Pixel red = 0, redInc = lowbit(pVisual->redMask);
        Pixel green = 0, greenInc = lowbit(pVisual->greenMask);
        Pixel blue = 0, blueInc = lowbit(pVisual->blueMask);

        for (int i = 0; i < ncolors; i++) {
            colors[i] = red | green | blue;
            red += redInc;
            if (red > pVisual->redMask)
                red = 0L;
            green += greenInc;
            if (green > pVisual->greenMask)
                green = 0L;
            blue += blueInc;
            if (blue > pVisual->blueMask)
                blue = 0L;
        }
        return load_colormap(pCmap, ncolors, colors);
    }
    break;

    case GrayScale:            /* read and write */
        break;

    case PseudoColor:          /* read and write */
        break;

    case DirectColor:          /* read and write */
        break;
    }

    return TRUE;
}

void
xnestDestroyColormap(ColormapPtr pCmap)
{
    xcb_free_colormap(xnestUpstreamInfo.conn, xnestColormap(pCmap));
}

#define SEARCH_PREDICATE \
  (xnestWindow(pWin) != XCB_WINDOW_NONE && wColormap(pWin) == icws->cmapIDs[i])

static int
xnestCountInstalledColormapWindows(WindowPtr pWin, void *ptr)
{
    xnestInstalledColormapWindows *icws = (xnestInstalledColormapWindows *) ptr;
    int i;

    for (i = 0; i < icws->numCmapIDs; i++)
        if (SEARCH_PREDICATE) {
            icws->numWindows++;
            return WT_DONTWALKCHILDREN;
        }

    return WT_WALKCHILDREN;
}

static int
xnestGetInstalledColormapWindows(WindowPtr pWin, void *ptr)
{
    xnestInstalledColormapWindows *icws = (xnestInstalledColormapWindows *) ptr;
    int i;

    for (i = 0; i < icws->numCmapIDs; i++)
        if (SEARCH_PREDICATE) {
            icws->windows[icws->index++] = xnestWindow(pWin);
            return WT_DONTWALKCHILDREN;
        }

    return WT_WALKCHILDREN;
}

static Window *xnestOldInstalledColormapWindows = NULL;
static int xnestNumOldInstalledColormapWindows = 0;

static Bool
xnestSameInstalledColormapWindows(Window *windows, int numWindows)
{
    if (xnestNumOldInstalledColormapWindows != numWindows)
        return FALSE;

    if (xnestOldInstalledColormapWindows == windows)
        return TRUE;

    if (xnestOldInstalledColormapWindows == NULL || windows == NULL)
        return FALSE;

    if (memcmp(xnestOldInstalledColormapWindows, windows,
               numWindows * sizeof(Window)))
        return FALSE;

    return TRUE;
}

void
xnestSetInstalledColormapWindows(ScreenPtr pScreen)
{
    xnestInstalledColormapWindows icws;
    int numWindows;

    if (!(icws.cmapIDs = calloc(pScreen->maxInstalledCmaps, sizeof(Colormap))))
        return;
    icws.numCmapIDs = xnestListInstalledColormaps(pScreen, icws.cmapIDs);
    icws.numWindows = 0;
    WalkTree(pScreen, xnestCountInstalledColormapWindows, (void *) &icws);
    if (icws.numWindows) {
        if (!(icws.windows = calloc(icws.numWindows + 1, sizeof(Window)))) {
            free(icws.cmapIDs);
            return;
        }
        icws.index = 0;
        WalkTree(pScreen, xnestGetInstalledColormapWindows, (void *) &icws);
        icws.windows[icws.numWindows] = xnestDefaultWindows[pScreen->myNum];
        numWindows = icws.numWindows + 1;
    }
    else {
        icws.windows = NULL;
        numWindows = 0;
    }

    free(icws.cmapIDs);

    if (!xnestSameInstalledColormapWindows(icws.windows, icws.numWindows)) {
        free(xnestOldInstalledColormapWindows);

        xnest_wm_colormap_windows(xnestUpstreamInfo.conn,
                                  xnestDefaultWindows[pScreen->myNum],
                                  icws.windows,
                                  numWindows);

        xnestOldInstalledColormapWindows = icws.windows;
        xnestNumOldInstalledColormapWindows = icws.numWindows;

#ifdef DUMB_WINDOW_MANAGERS
        /*
           This code is for dumb window managers.
           This will only work with default local visual colormaps.
         */
        if (icws.numWindows) {
            WindowPtr pWin;
            Visual *visual;
            ColormapPtr pCmap;

            pWin = xnestWindowPtr(icws.windows[0]);
            visual = xnestVisualFromID(pScreen, wVisual(pWin));

            if (visual == xnestDefaultVisual(pScreen))
                dixLookupResourceByType((void **) &pCmap, wColormap(pWin),
                                        X11_RESTYPE_COLORMAP, serverClient,
                                        DixUseAccess);
            else
                dixLookupResourceByType((void **) &pCmap,
                                        pScreen->defColormap, X11_RESTYPE_COLORMAP,
                                        serverClient, DixUseAccess);

            uint32_t cmap = xnestColormap(pCmap);
            xcb_change_window_attributes(xnestUpstreamInfo.conn,
                                         xnestDefaultWindows[pScreen->myNum],
                                         XCB_CW_COLORMAP,
                                         &cmap);
        }
#endif                          /* DUMB_WINDOW_MANAGERS */
    }
    else
        free(icws.windows);
}

void
xnestSetScreenSaverColormapWindow(ScreenPtr pScreen)
{
    free(xnestOldInstalledColormapWindows);

    xnest_wm_colormap_windows(xnestUpstreamInfo.conn,
                              xnestDefaultWindows[pScreen->myNum],
                              &xnestScreenSaverWindows[pScreen->myNum],
                              1);

    xnestOldInstalledColormapWindows = NULL;
    xnestNumOldInstalledColormapWindows = 0;

    xnestDirectUninstallColormaps(pScreen);
}

void
xnestDirectInstallColormaps(ScreenPtr pScreen)
{
    int i, n;
    Colormap pCmapIDs[MAXCMAPS];

    if (!xnestDoDirectColormaps)
        return;

    n = (*pScreen->ListInstalledColormaps) (pScreen, pCmapIDs);

    for (i = 0; i < n; i++) {
        ColormapPtr pCmap;

        dixLookupResourceByType((void **) &pCmap, pCmapIDs[i], X11_RESTYPE_COLORMAP,
                                serverClient, DixInstallAccess);
        if (pCmap)
            XInstallColormap(xnestDisplay, xnestColormap(pCmap));
    }
}

void
xnestDirectUninstallColormaps(ScreenPtr pScreen)
{
    int i, n;
    Colormap pCmapIDs[MAXCMAPS];

    if (!xnestDoDirectColormaps)
        return;

    n = (*pScreen->ListInstalledColormaps) (pScreen, pCmapIDs);

    for (i = 0; i < n; i++) {
        ColormapPtr pCmap;

        dixLookupResourceByType((void **) &pCmap, pCmapIDs[i], X11_RESTYPE_COLORMAP,
                                serverClient, DixUninstallAccess);
        if (pCmap)
            XUninstallColormap(xnestDisplay, xnestColormap(pCmap));
    }
}

void
xnestInstallColormap(ColormapPtr pCmap)
{
    ColormapPtr pOldCmap = GetInstalledColormap(pCmap->pScreen);

    if (pCmap != pOldCmap) {
        xnestDirectUninstallColormaps(pCmap->pScreen);

        /* Uninstall pInstalledMap. Notify all interested parties. */
        if (pOldCmap != (ColormapPtr) XCB_COLORMAP_NONE)
            WalkTree(pCmap->pScreen, TellLostMap, (void *) &pOldCmap->mid);

        SetInstalledColormap(pCmap->pScreen, pCmap);
        WalkTree(pCmap->pScreen, TellGainedMap, (void *) &pCmap->mid);

        xnestSetInstalledColormapWindows(pCmap->pScreen);
        xnestDirectInstallColormaps(pCmap->pScreen);
    }
}

void
xnestUninstallColormap(ColormapPtr pCmap)
{
    ColormapPtr pCurCmap = GetInstalledColormap(pCmap->pScreen);

    if (pCmap == pCurCmap) {
        if (pCmap->mid != pCmap->pScreen->defColormap) {
            dixLookupResourceByType((void **) &pCurCmap,
                                    pCmap->pScreen->defColormap,
                                    X11_RESTYPE_COLORMAP,
                                    serverClient, DixInstallAccess);
            (*pCmap->pScreen->InstallColormap) (pCurCmap);
        }
    }
}

static Bool xnestInstalledDefaultColormap = FALSE;

int
xnestListInstalledColormaps(ScreenPtr pScreen, Colormap * pCmapIDs)
{
    if (xnestInstalledDefaultColormap) {
        *pCmapIDs = GetInstalledColormap(pScreen)->mid;
        return 1;
    }
    else
        return 0;
}

void
xnestStoreColors(ColormapPtr pCmap, int nColors, xColorItem * pColors)
{
    if (pCmap->pVisual->class & DynamicClass)
        xcb_store_colors(xnestUpstreamInfo.conn,
                         xnestColormap(pCmap),
                         nColors,
                         (xcb_coloritem_t*) pColors);
}

void
xnestResolveColor(unsigned short *pRed, unsigned short *pGreen,
                  unsigned short *pBlue, VisualPtr pVisual)
{
    int shift;
    unsigned int lim;

    shift = 16 - pVisual->bitsPerRGBValue;
    lim = (1 << pVisual->bitsPerRGBValue) - 1;

    if ((pVisual->class == PseudoColor) || (pVisual->class == DirectColor)) {
        /* rescale to rgb bits */
        *pRed = ((*pRed >> shift) * 65535) / lim;
        *pGreen = ((*pGreen >> shift) * 65535) / lim;
        *pBlue = ((*pBlue >> shift) * 65535) / lim;
    }
    else if (pVisual->class == GrayScale) {
        /* rescale to gray then rgb bits */
        *pRed = (30L * *pRed + 59L * *pGreen + 11L * *pBlue) / 100;
        *pBlue = *pGreen = *pRed = ((*pRed >> shift) * 65535) / lim;
    }
    else if (pVisual->class == StaticGray) {
        unsigned int limg;

        limg = pVisual->ColormapEntries - 1;
        /* rescale to gray then [0..limg] then [0..65535] then rgb bits */
        *pRed = (30L * *pRed + 59L * *pGreen + 11L * *pBlue) / 100;
        *pRed = ((((*pRed * (limg + 1))) >> 16) * 65535) / limg;
        *pBlue = *pGreen = *pRed = ((*pRed >> shift) * 65535) / lim;
    }
    else {
        unsigned limr, limg, limb;

        limr = pVisual->redMask >> pVisual->offsetRed;
        limg = pVisual->greenMask >> pVisual->offsetGreen;
        limb = pVisual->blueMask >> pVisual->offsetBlue;
        /* rescale to [0..limN] then [0..65535] then rgb bits */
        *pRed = ((((((*pRed * (limr + 1)) >> 16) *
                    65535) / limr) >> shift) * 65535) / lim;
        *pGreen = ((((((*pGreen * (limg + 1)) >> 16) *
                      65535) / limg) >> shift) * 65535) / lim;
        *pBlue = ((((((*pBlue * (limb + 1)) >> 16) *
                     65535) / limb) >> shift) * 65535) / lim;
    }
}

Bool
xnestCreateDefaultColormap(ScreenPtr pScreen)
{
    VisualPtr pVisual;
    ColormapPtr pCmap;
    unsigned short zero = 0, ones = 0xFFFF;
    Pixel wp, bp;

    if (!dixRegisterPrivateKey(&cmapScrPrivateKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    for (pVisual = pScreen->visuals;
         pVisual->vid != pScreen->rootVisual; pVisual++);

    if (dixCreateColormap(pScreen->defColormap, pScreen, pVisual, &pCmap,
                       (pVisual->class & DynamicClass) ? AllocNone : AllocAll,
                       serverClient)
        != Success)
        return FALSE;

    wp = pScreen->whitePixel;
    bp = pScreen->blackPixel;
    if ((AllocColor(pCmap, &ones, &ones, &ones, &wp, 0) !=
         Success) ||
        (AllocColor(pCmap, &zero, &zero, &zero, &bp, 0) != Success))
        return FALSE;
    pScreen->whitePixel = wp;
    pScreen->blackPixel = bp;
    (*pScreen->InstallColormap) (pCmap);

    xnestInstalledDefaultColormap = TRUE;

    return TRUE;
}
