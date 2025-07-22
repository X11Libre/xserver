/* Copyright (c) 2008 Advanced Micro Devices, Inc.
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
#include <xorg-config.h>

#include "xf86.h"
#include "X11/Xatom.h"
#include "geode.h"
#include "xf86Modes.h"
#include "xf86Crtc.h"
#include "cim/cim_defs.h"
#include "cim/cim_regs.h"

#define MAKE_ATOM(a) MakeAtom(a, sizeof(a) - 1, TRUE)

typedef struct _LXOutputPrivateRec {
    I2CBusPtr pDDCBus;
} LXOutputPrivateRec, *LXOutputPrivatePtr;

static Atom scale_atom;

static void
lx_create_resources(xf86OutputPtr output)
{
    int ret;
    const char *s;
    ScrnInfoPtr pScrni = output->scrn;
    GeodeRec *pGeode = GEODEPTR(pScrni);

    /* Scaling is only used for panels */

    if (!(pGeode->Output & OUTPUT_PANEL))
        return;

    scale_atom = MAKE_ATOM("scale");
    ret = RRConfigureOutputProperty(output->randr_output,
                                    scale_atom, FALSE, FALSE, FALSE, 0, NULL);

    if (ret) {
        xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                   "RRConfigureOutputProperty error %d\n", ret);
    }

    s = "on";
    ret = RRChangeOutputProperty(output->randr_output, scale_atom,
                                 XA_STRING, 8, PropModeReplace, strlen(s),
                                 (pointer) s, FALSE, FALSE);

    if (ret) {
        xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                   "RRCharOutputProperty error %d\n", ret);
    }
}

static Bool
lx_output_set_property(xf86OutputPtr output, Atom property,
                       RRPropertyValuePtr value)
{
    ScrnInfoPtr pScrni = output->scrn;
    GeodeRec *pGeode = GEODEPTR(pScrni);
    Bool scale = pGeode->Scale;
    char *s;
    int ret;

    if (property != scale_atom)
        return FALSE;

    if (value->type != XA_STRING || value->format != 8)
        return FALSE;

    s = (char *) value->data;

    if (value->size == 2 && !strncmp("on", s, 2))
        pGeode->Scale = TRUE;
    else if (value->size == 3 && !strncmp("off", s, 3))
        pGeode->Scale = FALSE;

    if (pGeode->Scale != scale && output->crtc) {
        xf86CrtcPtr crtc = output->crtc;

        if (crtc->enabled) {
            ret = xf86CrtcSetMode(crtc, &crtc->desiredMode,
                                  crtc->desiredRotation, crtc->desiredX,
                                  crtc->desiredY);

            if (!ret) {
                xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                           "Failed to set mode after property change!\n");

                pGeode->Scale = scale;
                return FALSE;
            }
        }
    }

    return TRUE;
}

static void
lx_output_dpms(xf86OutputPtr output, int mode)
{
    /* DPMS is handled by the CRTC */
}

static void
lx_output_prepare(xf86OutputPtr output)
{
    /* Nothing to do */
}

static void
lx_output_commit(xf86OutputPtr output)
{
    /* Nothing to do */
}

static void
lx_output_save(xf86OutputPtr output)
{
    /* Nothing to do */
}

static void
lx_output_restore(xf86OutputPtr output)
{
    /* Nothing to do */
}

static int
lx_output_mode_valid(xf86OutputPtr output, DisplayModePtr pMode)
{
    ScrnInfoPtr pScrni = output->scrn;
    GeodeRec *pGeode = GEODEPTR(pScrni);

    /* DCON Panel specific resolution - OLPC's one */
    if (pGeode->Output & OUTPUT_DCON) {
        if (pGeode->panelMode->HDisplay == 1200 &&
            pGeode->panelMode->VDisplay == 900)
            return MODE_OK;
    }

    if ((pGeode->Output & OUTPUT_PANEL) &&
        gfx_is_panel_mode_supported(pGeode->panelMode->HDisplay,
                                    pGeode->panelMode->VDisplay,
                                    pMode->HDisplay,
                                    pMode->VDisplay,
                                    pScrni->bitsPerPixel) != -1) {

        return MODE_OK;
    }

    if (gfx_is_display_mode_supported(pMode->HDisplay,
                                      pMode->VDisplay,
                                      pScrni->bitsPerPixel,
                                      GeodeGetRefreshRate(pMode)) != -1) {
        return MODE_OK;
    }

    if (pMode->type & (M_T_DRIVER | M_T_PREFERRED))
        return MODE_OK;

    return MODE_OK;
}

static Bool
lx_output_mode_fixup(xf86OutputPtr output, DisplayModePtr mode,
                     DisplayModePtr adjusted_mode)
{
    return TRUE;
}

static void
lx_output_mode_set(xf86OutputPtr output, DisplayModePtr mode,
                   DisplayModePtr adjusted_mode)
{
    ScrnInfoPtr pScrni = output->scrn;
    GeodeRec *pGeode = GEODEPTR(pScrni);

    /* Configure the output path */

    if (pGeode->Output & OUTPUT_PANEL)
        df_set_output_path((pGeode->Output & OUTPUT_CRT) ?
                           DF_DISPLAY_CRT_FP : DF_DISPLAY_FP);
    else
        df_set_output_path(DF_DISPLAY_CRT);
}

static xf86OutputStatus
lx_output_detect(xf86OutputPtr output)
{
    /* We assume that there is always something
     * out there */

    return XF86OutputStatusConnected;
}

static DisplayModePtr
lx_output_get_modes(xf86OutputPtr output)
{
    ScrnInfoPtr pScrni = output->scrn;
    GeodeRec *pGeode = GEODEPTR(pScrni);
    LXOutputPrivatePtr lx_output = output->driver_private;

    xf86MonPtr mon;
    DisplayModePtr modes;

    if (!(pGeode->Output & OUTPUT_PANEL)) {
        mon = xf86OutputGetEDID(output, lx_output->pDDCBus);
        xf86OutputSetEDID(output, mon);
        modes = xf86OutputGetEDIDModes(output);
    }
    else {
        modes = xf86DuplicateMode(pGeode->panelMode);
    }

    return modes;
}

static void
lx_output_destroy(xf86OutputPtr output)
{
    if (output->driver_private)
        free(output->driver_private);

    output->driver_private = NULL;
}

#ifdef RANDR_GET_CRTC_INTERFACE
static xf86CrtcPtr
lx_output_get_crtc(xf86OutputPtr output)
{
    return output->crtc;
}
#endif

static const xf86OutputFuncsRec lx_output_funcs = {
    .create_resources = lx_create_resources,
    .dpms = lx_output_dpms,
    .save = lx_output_save,
    .restore = lx_output_restore,
    .mode_valid = lx_output_mode_valid,
    .mode_fixup = lx_output_mode_fixup,
    .prepare = lx_output_prepare,
    .mode_set = lx_output_mode_set,
    .commit = lx_output_commit,
    .detect = lx_output_detect,
    .get_modes = lx_output_get_modes,
#ifdef RANDR_GET_CRTC_INTERFACE
    .get_crtc = lx_output_get_crtc,
#endif
    .set_property = lx_output_set_property,
    .destroy = lx_output_destroy,
};

void
LXSetupOutput(ScrnInfoPtr pScrni)
{
    xf86OutputPtr output;
    LXOutputPrivatePtr lxpriv;
    GeodePtr pGeode = GEODEPTR(pScrni);

    output = xf86OutputCreate(pScrni, &lx_output_funcs, "default");

    lxpriv = XNFcallocarray(1, sizeof(LXOutputPrivateRec));

    if (!lxpriv) {
        xf86OutputDestroy(output);
        return;
    }

    output->driver_private = lxpriv;
    output->interlaceAllowed = TRUE;
    output->doubleScanAllowed = TRUE;

    /* Set up the DDC bus */

    GeodeI2CInit(pScrni, &lxpriv->pDDCBus, "CS5536 DDC");

    if (pScrni->monitor->widthmm && pScrni->monitor->heightmm) {
        /* prioritize the admin's screen size */
        output->mm_width = pScrni->monitor->widthmm;
        output->mm_height = pScrni->monitor->heightmm;
    }
    else if (pGeode->mm_width && pGeode->mm_height) {
        /* if we have a panel that we're certain of the size of, set it */
        output->mm_width = pScrni->monitor->widthmm = pGeode->mm_width;
        output->mm_height = pScrni->monitor->heightmm = pGeode->mm_height;
    }

    /* We only have one CRTC, and this output is tied to it */
    output->possible_crtcs = 1;
}
