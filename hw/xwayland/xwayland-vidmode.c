/*
 * Copyright (c) 1999-2003 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#include <dix-config.h>

#include <X11/X.h>

#include "dix/dix_priv.h"

#include "misc.h"
#include "os.h"

#ifdef XF86VIDMODE

#include "randrstr.h"
#include "vidmodestr.h"

#include "xwayland-screen.h"
#include "xwayland-vidmode.h"

static DevPrivateKeyRec xwlVidModePrivateKeyRec;
#define xwlVidModePrivateKey (&xwlVidModePrivateKeyRec)

/* Taken from xrandr, h sync frequency in KHz */
static double
mode_hsync(const xRRModeInfo *mode_info)
{
    double rate;

    if (mode_info->hTotal)
        rate = (double) mode_info->dotClock / (double) mode_info->hTotal;
    else
        rate = 0.0;

    return rate / 1000.0;
}

/* Taken from xrandr, v refresh frequency in Hz */
static double
mode_refresh(const xRRModeInfo *mode_info)
{
    double rate;
    double vTotal = mode_info->vTotal;

    if (mode_info->modeFlags & RR_DoubleScan)
	vTotal *= 2.0;

    if (mode_info->modeFlags & RR_Interlace)
	vTotal /= 2.0;

    if (mode_info->hTotal > 0.0 && vTotal > 0.0)
	rate = ((double) mode_info->dotClock /
		((double) mode_info->hTotal * (double) vTotal));
    else
        rate = 0.0;

    return rate;
}

static void
xwlRRModeToDisplayMode(RRModePtr rrmode, DisplayModePtr mode)
{
    const xRRModeInfo *mode_info = &rrmode->mode;

    mode->next = mode;
    mode->prev = mode;
    mode->name = "";
    mode->VScan = 1;
    mode->Private = NULL;
    mode->HDisplay = mode_info->width;
    mode->HSyncStart = mode_info->hSyncStart;
    mode->HSyncEnd = mode_info->hSyncEnd;
    mode->HTotal = mode_info->hTotal;
    mode->HSkew = mode_info->hSkew;
    mode->VDisplay = mode_info->height;
    mode->VSyncStart = mode_info->vSyncStart;
    mode->VSyncEnd = mode_info->vSyncEnd;
    mode->VTotal = mode_info->vTotal;
    mode->Flags = mode_info->modeFlags;
    mode->Clock = mode_info->dotClock / 1000.0;
    mode->VRefresh = mode_refresh(mode_info); /* Or RRVerticalRefresh() */
    mode->HSync = mode_hsync(mode_info);
}

static RRModePtr
xwlVidModeGetRRMode(ScreenPtr pScreen, int32_t width, int32_t height)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(pScreen);
    struct xwl_output *xwl_output;

    xwl_output = xwl_screen_get_fixed_or_first_output(xwl_screen);
    if (!xwl_output)
        return NULL;

    return xwl_output_find_mode(xwl_output, width, height);
}

static RRModePtr
xwlVidModeGetCurrentRRMode(ScreenPtr pScreen)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(pScreen);
    struct xwl_emulated_mode *emulated_mode;
    struct xwl_output *xwl_output;

    xwl_output = xwl_screen_get_fixed_or_first_output(xwl_screen);
    if (!xwl_output)
        return NULL;

    emulated_mode =
        xwl_output_get_emulated_mode_for_client(xwl_output, GetCurrentClient());

    if (emulated_mode) {
        return xwl_output_find_mode(xwl_output,
                                    emulated_mode->width,
                                    emulated_mode->height);
    } else {
        return xwl_output_find_mode(xwl_output, -1, -1);
    }
}

static Bool
xwlVidModeGetCurrentModeline(ScreenPtr pScreen, DisplayModePtr *mode, int *dotClock)
{
    DisplayModePtr pMod;
    RRModePtr rrmode;

    pMod = dixLookupPrivate(&pScreen->devPrivates, xwlVidModePrivateKey);
    if (pMod == NULL)
        return FALSE;

    rrmode = xwlVidModeGetCurrentRRMode(pScreen);
    if (rrmode == NULL)
        return FALSE;

    xwlRRModeToDisplayMode(rrmode, pMod);

    *mode = pMod;
    if (dotClock != NULL)
        *dotClock = pMod->Clock;

    return TRUE;
}

static vidMonitorValue
xwlVidModeGetMonitorValue(ScreenPtr pScreen, int valtyp, int indx)
{
    vidMonitorValue ret = { NULL, };
    RRModePtr rrmode;

    rrmode = xwlVidModeGetCurrentRRMode(pScreen);
    if (rrmode == NULL)
        return ret;

    switch (valtyp) {
    case VIDMODE_MON_VENDOR:
        ret.ptr = XVENDORNAME;
        break;
    case VIDMODE_MON_MODEL:
        ret.ptr = "XWAYLAND";
        break;
    case VIDMODE_MON_NHSYNC:
        ret.i = 1;
        break;
    case VIDMODE_MON_NVREFRESH:
        ret.i = 1;
        break;
    case VIDMODE_MON_HSYNC_LO:
    case VIDMODE_MON_HSYNC_HI:
        ret.f = mode_hsync(&rrmode->mode) * 100.0;
        break;
    case VIDMODE_MON_VREFRESH_LO:
    case VIDMODE_MON_VREFRESH_HI:
        ret.f = mode_refresh(&rrmode->mode) * 100.0;
        break;
    }
    return ret;
}

static int
xwlVidModeGetDotClock(ScreenPtr pScreen, int Clock)
{
    return Clock;
}

static int
xwlVidModeGetNumOfClocks(ScreenPtr pScreen, Bool *progClock)
{
    /* We emulate a programmable clock, rather then a fixed set of clocks */
    *progClock = TRUE;
    return 0;
}

static Bool
xwlVidModeGetClocks(ScreenPtr pScreen, int *Clocks)
{
    return FALSE; /* Programmable clock, no clock list */
}

/* GetFirstModeline and GetNextModeline are used from Xext/vidmode.c like this:
 *  if (pVidMode->GetFirstModeline(pScreen, &mode, &dotClock)) {
 *      do {
 *          ...
 *          if (...)
 *              break;
 *      } while (pVidMode->GetNextModeline(pScreen, &mode, &dotClock));
 *  }
 * IOW our caller basically always loops over all the modes. There never is a
 * return to the mainloop between GetFirstModeline and NextModeline calls where
 * other parts of the server may change our state so we do not need to worry
 * about xwl_output->randr_output->modes changing underneath us.
 * Thus we can simply implement these two callbacks by storing the enumeration
 * index in pVidMode->Next.
 */

static Bool
xwlVidModeGetNextModeline(ScreenPtr pScreen, DisplayModePtr *mode, int *dotClock)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(pScreen);
    struct xwl_output *xwl_output;
    VidModePtr pVidMode;
    DisplayModePtr pMod;
    intptr_t index;

    xwl_output = xwl_screen_get_fixed_or_first_output(xwl_screen);
    if (xwl_output == NULL)
        return FALSE;

    pMod = dixLookupPrivate(&pScreen->devPrivates, xwlVidModePrivateKey);
    pVidMode = VidModeGetPtr(pScreen);
    if (pMod == NULL || pVidMode == NULL)
        return FALSE;

    index = (intptr_t)pVidMode->Next;
    if (index >= xwl_output->randr_output->numModes)
        return FALSE;
    xwlRRModeToDisplayMode(xwl_output->randr_output->modes[index], pMod);
    index++;
    pVidMode->Next = (void *)index;

    *mode = pMod;
    if (dotClock != NULL)
        *dotClock = pMod->Clock;

    return TRUE;
}

static Bool
xwlVidModeGetFirstModeline(ScreenPtr pScreen, DisplayModePtr *mode, int *dotClock)
{
    VidModePtr pVidMode;
    intptr_t index = 0;

    pVidMode = VidModeGetPtr(pScreen);
    if (pVidMode == NULL)
        return FALSE;

    pVidMode->Next = (void *)index; /* 0 */
    return xwlVidModeGetNextModeline(pScreen, mode, dotClock);
}

static Bool
xwlVidModeDeleteModeline(ScreenPtr pScreen, DisplayModePtr mode)
{
    /* Unsupported */
    return FALSE;
}

static Bool
xwlVidModeZoomViewport(ScreenPtr pScreen, int zoom)
{
    /* Support only no zoom */
    return (zoom == 1);
}

static Bool
xwlVidModeSetViewPort(ScreenPtr pScreen, int x, int y)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(pScreen);
    struct xwl_output *xwl_output;

    xwl_output = xwl_screen_get_fixed_or_first_output(xwl_screen);
    if (xwl_output == NULL)
        return FALSE;

    /* Support only default viewport */
    return (x == xwl_output->x && y == xwl_output->y);
}

static Bool
xwlVidModeGetViewPort(ScreenPtr pScreen, int *x, int *y)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(pScreen);
    struct xwl_output *xwl_output;

    xwl_output = xwl_screen_get_fixed_or_first_output(xwl_screen);
    if (xwl_output == NULL)
        return FALSE;

    *x = xwl_output->x;
    *y = xwl_output->y;

    return TRUE;
}

static Bool
xwlVidModeSwitchMode(ScreenPtr pScreen, DisplayModePtr mode)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(pScreen);
    struct xwl_output *xwl_output;
    RRModePtr rrmode;

    xwl_output = xwl_screen_get_fixed_or_first_output(xwl_screen);
    if (xwl_output == NULL)
        return FALSE;

    rrmode = xwl_output_find_mode(xwl_output, mode->HDisplay, mode->VDisplay);
    if (rrmode == NULL)
        return FALSE;

    if (xwl_screen->rootless)
        xwl_output_set_emulated_mode(xwl_output, GetCurrentClient(), rrmode, TRUE);
    else if (xwl_screen->fixed_output)
        xwl_output_set_mode_fixed(xwl_screen->fixed_output, rrmode);

    return TRUE;
}

static Bool
xwlVidModeLockZoom(ScreenPtr pScreen, Bool lock)
{
    /* Unsupported for now, but pretend it works */
    return TRUE;
}

static ModeStatus
xwlVidModeCheckModeForMonitor(ScreenPtr pScreen, DisplayModePtr mode)
{
    RRModePtr rrmode;

    rrmode = xwlVidModeGetRRMode(pScreen, mode->HDisplay, mode->VDisplay);
    if (rrmode == NULL)
        return MODE_ERROR;

    /* Only support mode with the same HSync/VRefresh as we advertise */
    if (mode->HSync == mode_hsync(&rrmode->mode) &&
        mode->VRefresh == mode_refresh(&rrmode->mode))
        return MODE_OK;

    /* All the rest is unsupported - If we want to succeed, return MODE_OK instead */
    return MODE_ONE_SIZE;
}

static ModeStatus
xwlVidModeCheckModeForDriver(ScreenPtr pScreen, DisplayModePtr mode)
{
    RRModePtr rrmode;

    rrmode = xwlVidModeGetRRMode(pScreen, mode->HDisplay, mode->VDisplay);
    return rrmode ? MODE_OK : MODE_ERROR;
}

static void
xwlVidModeSetCrtcForMode(ScreenPtr pScreen, DisplayModePtr mode)
{
    /* Unsupported */
    return;
}

static Bool
xwlVidModeAddModeline(ScreenPtr pScreen, DisplayModePtr mode)
{
    /* Unsupported */
    return FALSE;
}

static int
xwlVidModeGetNumOfModes(ScreenPtr pScreen)
{
    struct xwl_screen *xwl_screen = xwl_screen_get(pScreen);
    struct xwl_output *xwl_output;

    xwl_output = xwl_screen_get_fixed_or_first_output(xwl_screen);

    return xwl_output ? xwl_output->randr_output->numModes : 0;
}

static Bool
xwlVidModeSetGamma(ScreenPtr pScreen, float red, float green, float blue)
{
    /* Unsupported for now, but pretend it works */
    return TRUE;
}

static Bool
xwlVidModeGetGamma(ScreenPtr pScreen, float *red, float *green, float *blue)
{
    /* Unsupported for now, but pretend it works */
    *red = *green = *blue = 1.0f;
    return TRUE;
}

static Bool
xwlVidModeSetGammaRamp(ScreenPtr pScreen, int size, CARD16 *r, CARD16 *g, CARD16 *b)
{
    /* Unsupported for now */
    return FALSE;
}

static Bool
xwlVidModeGetGammaRamp(ScreenPtr pScreen, int size, CARD16 *r, CARD16 *g, CARD16 *b)
{
    /* Unsupported for now */
    return FALSE;
}

static int
xwlVidModeGetGammaRampSize(ScreenPtr pScreen)
{
    /* Unsupported for now */
    return 0;
}

static Bool
xwlVidModeInit(ScreenPtr pScreen)
{
    VidModePtr pVidMode = NULL;

    pVidMode = VidModeInit(pScreen);
    if (!pVidMode)
        return FALSE;

    pVidMode->Flags = 0;
    pVidMode->Next = NULL;

    pVidMode->GetMonitorValue = xwlVidModeGetMonitorValue;
    pVidMode->GetCurrentModeline = xwlVidModeGetCurrentModeline;
    pVidMode->GetFirstModeline = xwlVidModeGetFirstModeline;
    pVidMode->GetNextModeline = xwlVidModeGetNextModeline;
    pVidMode->DeleteModeline = xwlVidModeDeleteModeline;
    pVidMode->ZoomViewport = xwlVidModeZoomViewport;
    pVidMode->GetViewPort = xwlVidModeGetViewPort;
    pVidMode->SetViewPort = xwlVidModeSetViewPort;
    pVidMode->SwitchMode = xwlVidModeSwitchMode;
    pVidMode->LockZoom = xwlVidModeLockZoom;
    pVidMode->GetNumOfClocks = xwlVidModeGetNumOfClocks;
    pVidMode->GetClocks = xwlVidModeGetClocks;
    pVidMode->CheckModeForMonitor = xwlVidModeCheckModeForMonitor;
    pVidMode->CheckModeForDriver = xwlVidModeCheckModeForDriver;
    pVidMode->SetCrtcForMode = xwlVidModeSetCrtcForMode;
    pVidMode->AddModeline = xwlVidModeAddModeline;
    pVidMode->GetDotClock = xwlVidModeGetDotClock;
    pVidMode->GetNumOfModes = xwlVidModeGetNumOfModes;
    pVidMode->SetGamma = xwlVidModeSetGamma;
    pVidMode->GetGamma = xwlVidModeGetGamma;
    pVidMode->SetGammaRamp = xwlVidModeSetGammaRamp;
    pVidMode->GetGammaRamp = xwlVidModeGetGammaRamp;
    pVidMode->GetGammaRampSize = xwlVidModeGetGammaRampSize;

    return TRUE;
}

void
xwlVidModeExtensionInit(void)
{
    int i;
    Bool enabled = FALSE;

    for (i = 0; i < screenInfo.numScreens; i++) {
        if (xwlVidModeInit (screenInfo.screens[i]))
            enabled = TRUE;
    }
    /* This means that the DDX doesn't want the vidmode extension enabled */
    if (!enabled)
        return;

    if (!dixRegisterPrivateKey(xwlVidModePrivateKey, PRIVATE_SCREEN,
                               sizeof(DisplayModeRec)))
        return;

    VidModeAddExtension(FALSE);
}

#endif                          /* XF86VIDMODE */
