/* Originally derived from the Intel example
 * Copyright (C) 2002 Keith Packard, member of The XFree86 Project, Inc.

 * Copyright (c) 2006 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Neither the name of the Advanced Micro Devices, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"
#include "xf86Modes.h"
#include "os.h"
#include "globals.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86DDC.h"
#include "xf86Module.h"
#include "mipointer.h"
#include "windowstr.h"
#include "inputstr.h"
#include <X11/extensions/randr.h>
#include <randrstr.h>

#include "geode.h"

static int GXRandRGeneration;

typedef struct _GXRandRInfo {
    int virtualX;
    int virtualY;
    int mmWidth;
    int mmHeight;
    int maxX;
    int maxY;
    Rotation rotation;          /* current mode */
    Rotation supported_rotations;       /* driver supported */
} XF86RandRInfoRec, *XF86RandRInfoPtr;

static DevPrivateKeyRec GXRandRIndex;

#define XF86RANDRINFO(p) ((XF86RandRInfoPtr)						\
			  dixLookupPrivate(&(p)->devPrivates, &GXRandRIndex))
#define XF86RANDRSET(p, v) dixSetPrivate(&(p)->devPrivates, &GXRandRIndex, v)

static int
GXRandRModeRefresh(DisplayModePtr mode)
{
    if (mode->VRefresh)
        return (int) (mode->VRefresh + 0.5);
    else
        return (int) (mode->Clock * 1000.0 / mode->HTotal / mode->VTotal + 0.5);
}

static Bool
GXRandRGetInfo(ScreenPtr pScreen, Rotation *rotations)
{
    RRScreenSizePtr pSize;
    ScrnInfoPtr pScrni = XF86SCRNINFO(pScreen);
    XF86RandRInfoPtr pRandr = XF86RANDRINFO(pScreen);
    DisplayModePtr mode;
    int refresh0 = 60;
    int maxX = 0, maxY = 0;

    *rotations = pRandr->supported_rotations;

    if (pRandr->virtualX == -1 || pRandr->virtualY == -1) {
        pRandr->virtualX = pScrni->virtualX;
        pRandr->virtualY = pScrni->virtualY;
    }

    for (mode = pScrni->modes;; mode = mode->next) {
        int refresh = GXRandRModeRefresh(mode);

        if (pRandr->maxX == 0 || pRandr->maxY == 0) {
            if (maxX < mode->HDisplay)
                maxX = mode->HDisplay;
            if (maxY < mode->VDisplay)
                maxY = mode->VDisplay;
        }

        if (mode == pScrni->modes)
            refresh0 = refresh;

        pSize = RRRegisterSize(pScreen,
                               mode->HDisplay, mode->VDisplay,
                               pRandr->mmWidth, pRandr->mmHeight);
        if (!pSize)
            return FALSE;

        RRRegisterRate(pScreen, pSize, refresh);

        if (mode == pScrni->currentMode &&
            mode->HDisplay == pScrni->virtualX
            && mode->VDisplay == pScrni->virtualY)
            RRSetCurrentConfig(pScreen, pRandr->rotation, refresh, pSize);
        if (mode->next == pScrni->modes)
            break;
    }

    if (pRandr->maxX == 0 || pRandr->maxY == 0) {
        pRandr->maxX = maxX;
        pRandr->maxY = maxY;
    }

    if (pScrni->currentMode->HDisplay != pScrni->virtualX ||
        pScrni->currentMode->VDisplay != pScrni->virtualY) {

        mode = pScrni->modes;
        pSize = RRRegisterSize(pScreen,
                               pRandr->virtualX, pRandr->virtualY,
                               pRandr->mmWidth, pRandr->mmHeight);
        if (!pSize)
            return FALSE;

        RRRegisterRate(pScreen, pSize, refresh0);
        if (pScrni->virtualX == pRandr->virtualX &&
            pScrni->virtualY == pRandr->virtualY) {
            RRSetCurrentConfig(pScreen, pRandr->rotation, refresh0, pSize);
        }
    }

    return TRUE;
}

static Bool
GXRandRSetMode(ScreenPtr pScreen,
               DisplayModePtr mode, Bool useVirtual, int mmWidth, int mmHeight)
{
    ScrnInfoPtr pScrni = XF86SCRNINFO(pScreen);
    XF86RandRInfoPtr pRandr = XF86RANDRINFO(pScreen);

    int oldWidth = pScreen->width;
    int oldHeight = pScreen->height;
    int oldmmWidth = pScreen->mmWidth;
    int oldmmHeight = pScreen->mmHeight;

    WindowPtr pRoot = pScreen->root;
    DisplayModePtr currentMode = NULL;
    Bool ret = TRUE;

#if XORG_VERSION_CURRENT < XORG_VERSION_NUMERIC(1,9,99,1,0)
    PixmapPtr pspix = NULL;
#endif

    if (pRoot)
        (*pScrni->EnableDisableFBAccess) (pScrni, FALSE);

    if (useVirtual) {
        pScrni->virtualX = pRandr->virtualX;
        pScrni->virtualY = pRandr->virtualY;
    }
    else {
        pScrni->virtualX = mode->HDisplay;
        pScrni->virtualY = mode->VDisplay;
    }

    if (pRandr->rotation & (RR_Rotate_90 | RR_Rotate_270)) {
        pScreen->width = pScrni->virtualY;
        pScreen->height = pScrni->virtualX;
        pScreen->mmWidth = mmHeight;
        pScreen->mmHeight = mmWidth;
    }
    else {
        pScreen->width = pScrni->virtualX;
        pScreen->height = pScrni->virtualY;
        pScreen->mmWidth = mmWidth;
        pScreen->mmHeight = mmHeight;
    }

    if (pScrni->currentMode == mode) {
        currentMode = pScrni->currentMode;
        pScrni->currentMode = NULL;
    }

    if (!xf86SwitchMode(pScreen, mode)) {
        ret = FALSE;
        pScrni->virtualX = pScreen->width = oldWidth;
        pScrni->virtualY = pScreen->height = oldHeight;
        pScreen->mmWidth = oldmmWidth;
        pScreen->mmHeight = oldmmHeight;
        pScrni->currentMode = currentMode;
    }

#if XORG_VERSION_CURRENT < XORG_VERSION_NUMERIC(1,9,99,1,0)
    /*
     * Get the new Screen pixmap ptr as SwitchMode might have called
     * ModifyPixmapHeader and xf86EnableDisableFBAccess will put it back...
     * Unfortunately.
     */
    pspix = (*pScreen->GetScreenPixmap) (pScreen);
    if (pspix->devPrivate.ptr)
        pScrni->pixmapPrivate = pspix->devPrivate;
#endif

    xf86ReconfigureLayout();

    xf86SetViewport(pScreen, pScreen->width, pScreen->height);
    xf86SetViewport(pScreen, 0, 0);

    if (pRoot)
        (*pScrni->EnableDisableFBAccess) (pScrni, TRUE);

    return ret;
}

Bool
GXRandRSetConfig(ScreenPtr pScreen, Rotation rotation,
                 int rate, RRScreenSizePtr pSize)
{
    ScrnInfoPtr pScrni = XF86SCRNINFO(pScreen);
    XF86RandRInfoPtr pRandr = XF86RANDRINFO(pScreen);

    DisplayModePtr mode;
    int px, py;
    Bool useVirtual = FALSE;
    int maxX = 0, maxY = 0;
    Rotation oldRotation = pRandr->rotation;

    pRandr->rotation = rotation;

    if (pRandr->virtualX == -1 || pRandr->virtualY == -1) {
        pRandr->virtualX = pScrni->virtualX;
        pRandr->virtualY = pScrni->virtualY;
    }

/* FIXME: we don't have a new video ABI yet */
    miPointerGetPosition(inputInfo.pointer, &px, &py);

    for (mode = pScrni->modes;; mode = mode->next) {
        if (pRandr->maxX == 0 || pRandr->maxY == 0) {
            if (maxX < mode->HDisplay)
                maxX = mode->HDisplay;
            if (maxY < mode->VDisplay)
                maxY = mode->VDisplay;
        }
        if (mode->HDisplay == pSize->width &&
            mode->VDisplay == pSize->height &&
            (rate == 0 || GXRandRModeRefresh(mode) == rate))
            break;
        if (mode->next == pScrni->modes) {
            if (pSize->width == pRandr->virtualX &&
                pSize->height == pRandr->virtualY) {
                mode = pScrni->modes;
                useVirtual = TRUE;
                break;
            }
            if (pRandr->maxX == 0 || pRandr->maxY == 0) {
                pRandr->maxX = maxX;
                pRandr->maxY = maxY;
            }
            return FALSE;
        }
    }

    if (pRandr->maxX == 0 || pRandr->maxY == 0) {
        pRandr->maxX = maxX;
        pRandr->maxY = maxY;
    }

    if (!GXRandRSetMode(pScreen, mode, useVirtual, pSize->mmWidth,
                        pSize->mmHeight)) {
        pRandr->rotation = oldRotation;
        return FALSE;
    }

/* FIXME: we don't have a new video ABI yet */
    if (pScreen == miPointerGetScreen(inputInfo.pointer)) {
        px = (px >= pScreen->width ? (pScreen->width - 1) : px);
        py = (py >= pScreen->height ? (pScreen->height - 1) : py);

        xf86SetViewport(pScreen, px, py);

/* FIXME: we don't have a new video ABI yet */
        (*pScreen->SetCursorPosition) (inputInfo.pointer, pScreen, px, py,
                                       FALSE);
    }

    return TRUE;
}

Rotation
GXGetRotation(ScreenPtr pScreen)
{
    XF86RandRInfoPtr pRandr = XF86RANDRINFO(pScreen);

    return pRandr->rotation;
}

Bool
GXRandRInit(ScreenPtr pScreen, int rotation)
{
    XF86RandRInfoPtr pRandr;
    rrScrPrivPtr rp;

    if (GXRandRGeneration != serverGeneration) {
        GXRandRGeneration = serverGeneration;
    }
    if (!dixRegisterPrivateKey(&GXRandRIndex, PRIVATE_SCREEN, 0))
        return FALSE;

    pRandr = calloc(1, sizeof(XF86RandRInfoRec));
    if (pRandr == NULL)
        return FALSE;

    if (!RRScreenInit(pScreen)) {
        free(pRandr);
        return FALSE;
    }

    rp = rrGetScrPriv(pScreen);
    rp->rrGetInfo = GXRandRGetInfo;
    rp->rrSetConfig = GXRandRSetConfig;

    pRandr->virtualX = -1;
    pRandr->virtualY = -1;

    pRandr->mmWidth = pScreen->mmWidth;
    pRandr->mmHeight = pScreen->mmHeight;

    pRandr->rotation = RR_Rotate_0;
    pRandr->supported_rotations = rotation;
    pRandr->maxX = pRandr->maxY = 0;

    XF86RANDRSET(pScreen, pRandr);

    return TRUE;
}
