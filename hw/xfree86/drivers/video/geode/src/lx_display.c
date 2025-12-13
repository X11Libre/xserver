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
#include "geode.h"
#include "xf86Crtc.h"
#include "cim/cim_defs.h"
#include "cim/cim_regs.h"

typedef struct _LXOutputPrivateRec {
    int video_enable;
    unsigned long video_flags;
    GeodeMemPtr rotate_mem;
} LXCrtcPrivateRec, *LXCrtcPrivatePtr;

void
lx_enable_dac_power(ScrnInfoPtr pScrni, int option)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    df_set_crt_enable(DF_CRT_ENABLE);

    /* Turn off the DAC if we don't need the CRT */

    if (option && (!(pGeode->Output & OUTPUT_CRT))) {
        unsigned int misc = READ_VID32(DF_VID_MISC);

        misc |= DF_DAC_POWER_DOWN;
        WRITE_VID32(DF_VID_MISC, misc);
    }

    if (pGeode->Output & OUTPUT_PANEL)
        df_set_panel_enable(1);
}

void
lx_disable_dac_power(ScrnInfoPtr pScrni, int option)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    if (pGeode->Output & OUTPUT_PANEL)
        df_set_panel_enable(0);

    if (pGeode->Output & OUTPUT_CRT) {

        /* Wait for the panel to finish its procedure */

        if (pGeode->Output & OUTPUT_PANEL)
            while ((READ_VID32(DF_POWER_MANAGEMENT) & 2) == 0);
        df_set_crt_enable(option);
    }
}

static void
lx_set_panel_mode(VG_DISPLAY_MODE *mode, DisplayModePtr pMode)
{
    int hsync, vsync;

    mode->mode_width = mode->panel_width = pMode->HDisplay;
    mode->mode_height = mode->panel_height = pMode->VDisplay;

    mode->hactive = pMode->HDisplay;
    mode->hblankstart = pMode->HDisplay;
    mode->hsyncstart = pMode->HSyncStart;
    mode->hsyncend = pMode->HSyncEnd;
    mode->hblankend = pMode->HTotal;
    mode->htotal = pMode->HTotal;

    mode->vactive = pMode->VDisplay;
    mode->vblankstart = pMode->VDisplay;
    mode->vsyncstart = pMode->VSyncStart;
    mode->vsyncend = pMode->VSyncEnd;
    mode->vblankend = pMode->VTotal;
    mode->vtotal = pMode->VTotal;

    mode->vactive_even = pMode->VDisplay;
    mode->vblankstart_even = pMode->VDisplay;
    mode->vsyncstart_even = pMode->VSyncStart;
    mode->vsyncend_even = pMode->VSyncEnd;
    mode->vblankend_even = pMode->VTotal;
    mode->vtotal_even = pMode->VTotal;

    mode->frequency = (int) ((pMode->Clock / 1000.0) * 0x10000);

    /* In panel mode, Cimarron purposely swizzles these,
     * so we swizzle them first  */

    hsync = (pMode->Flags & V_NHSYNC) ? 0 : 1;
    vsync = (pMode->Flags & V_NVSYNC) ? 0 : 1;

    mode->flags |= (hsync) ? VG_MODEFLAG_NEG_HSYNC : 0;
    mode->flags |= (vsync) ? VG_MODEFLAG_NEG_VSYNC : 0;
}

static void
lx_set_crt_mode(VG_DISPLAY_MODE *mode, DisplayModePtr pMode)
{
    int hsync, vsync;

    mode->mode_width = mode->panel_width = pMode->HDisplay;
    mode->mode_height = mode->panel_height = pMode->VDisplay;

    mode->hactive = pMode->CrtcHDisplay;
    mode->hblankstart = pMode->CrtcHBlankStart;
    mode->hsyncstart = pMode->CrtcHSyncStart;
    mode->hsyncend = pMode->CrtcHSyncEnd;
    mode->hblankend = pMode->CrtcHBlankEnd;
    mode->htotal = pMode->CrtcHTotal;

    mode->vactive = pMode->CrtcVDisplay;
    mode->vblankstart = pMode->CrtcVBlankStart;
    mode->vsyncstart = pMode->CrtcVSyncStart;
    mode->vsyncend = pMode->CrtcVSyncEnd;
    mode->vblankend = pMode->CrtcVBlankEnd;
    mode->vtotal = pMode->CrtcVTotal;

    mode->vactive_even = pMode->CrtcVDisplay;
    mode->vblankstart_even = pMode->CrtcVBlankStart;
    mode->vsyncstart_even = pMode->CrtcVSyncStart;
    mode->vsyncend_even = pMode->CrtcVSyncEnd;
    mode->vblankend_even = pMode->CrtcVBlankEnd;
    mode->vtotal_even = pMode->CrtcVTotal;

    mode->frequency = (int) ((pMode->Clock / 1000.0) * 0x10000);

    hsync = (pMode->Flags & V_NHSYNC) ? 1 : 0;
    vsync = (pMode->Flags & V_NVSYNC) ? 1 : 0;

    mode->flags |= (hsync) ? VG_MODEFLAG_NEG_HSYNC : 0;
    mode->flags |= (vsync) ? VG_MODEFLAG_NEG_VSYNC : 0;
}

static int
lx_set_mode(ScrnInfoPtr pScrni, DisplayModePtr pMode, int bpp)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    VG_DISPLAY_MODE mode;
    int ret;

    memset(&mode, 0, sizeof(mode));

    mode.flags |= pGeode->Output & OUTPUT_CRT ? VG_MODEFLAG_CRT_AND_FP : 0;

    if (pGeode->Output & OUTPUT_PANEL) {
        mode.flags |= VG_MODEFLAG_PANELOUT;
        if (pGeode->Output & OUTPUT_CRT)
            mode.flags |= VG_MODEFLAG_CRT_AND_FP;
    }

    if (pGeode->Output & OUTPUT_PANEL && pGeode->Scale)
        lx_set_panel_mode(&mode, pGeode->panelMode);
    else
        lx_set_crt_mode(&mode, pMode);

    mode.src_width = pMode->HDisplay;
    mode.src_height = pMode->VDisplay;

    /* Set the filter coefficients to the default values */
    vg_set_scaler_filter_coefficients(NULL, NULL);

    ret = vg_set_custom_mode(&mode, bpp);
    return (ret == CIM_STATUS_OK) ? 0 : -1;
}

static void
lx_crtc_dpms(xf86CrtcPtr crtc, int mode)
{
    ScrnInfoPtr pScrni = crtc->scrn;
    GeodeRec *pGeode = GEODEPTR(pScrni);

    if (pGeode->Output & OUTPUT_DCON)
        DCONDPMSSet(pScrni, mode);

    switch (mode) {
    case DPMSModeOn:
        lx_enable_dac_power(pScrni, 1);
        break;

    case DPMSModeStandby:
        lx_disable_dac_power(pScrni, DF_CRT_STANDBY);
        break;

    case DPMSModeSuspend:
        lx_disable_dac_power(pScrni, DF_CRT_SUSPEND);
        break;

    case DPMSModeOff:
        lx_disable_dac_power(pScrni, DF_CRT_DISABLE);
        break;
    }
}

static Bool
lx_crtc_lock(xf86CrtcPtr crtc)
{
    /* Wait until the GPU is idle */
    gp_wait_until_idle();
    return TRUE;
}

static void
lx_crtc_unlock(xf86CrtcPtr crtc)
{
    /* Nothing to do here */
}

static void
lx_crtc_prepare(xf86CrtcPtr crtc)
{
    LXCrtcPrivatePtr lx_crtc = crtc->driver_private;

    /* Disable the video */
    df_get_video_enable(&lx_crtc->video_enable, &lx_crtc->video_flags);

    if (lx_crtc->video_enable)
        df_set_video_enable(0, 0);

    /* Turn off compression */
    vg_set_compression_enable(0);

    /* Hide the cursor */
    crtc->funcs->hide_cursor(crtc);

    /* Turn off the display */
    crtc->funcs->dpms(crtc, DPMSModeOff);
}

static Bool
lx_crtc_mode_fixup(xf86CrtcPtr crtc, DisplayModePtr mode,
                   DisplayModePtr adjusted_mode)
{
    return TRUE;
}

static void
lx_crtc_mode_set(xf86CrtcPtr crtc, DisplayModePtr mode,
                 DisplayModePtr adjusted_mode, int x, int y)
{
    ScrnInfoPtr pScrni = crtc->scrn;
    GeodeRec *pGeode = GEODEPTR(pScrni);
    DF_VIDEO_SOURCE_PARAMS vs_odd, vs_even;
    unsigned int rpitch;

    df_get_video_source_configuration(&vs_odd, &vs_even);

    /* Note - the memory gets adjusted when virtualX/virtualY
     * gets changed - so we don't need to worry about it here
     */

    if (lx_set_mode(pScrni, adjusted_mode, pScrni->bitsPerPixel))
        ErrorF("ERROR!  Unable to set the mode!\n");

    /* The output gets turned in in the output code as
     * per convention */

    /* For rotation, any write to the frame buffer region marks
     * the retire frame as dirty.
     */
    if (crtc->rotatedData != NULL) {
        rpitch = pScrni->displayWidth * (pScrni->bitsPerPixel / 8);
        vg_set_display_pitch(rpitch);
    }
    else
        vg_set_display_pitch(pGeode->Pitch);
    gp_set_bpp(pScrni->bitsPerPixel);

    /* Set the acceleration offset if we are drawing to a shadow */
    if (crtc->rotatedData != NULL)
        vg_set_display_offset((unsigned int) ((char *) crtc->rotatedData -
                                              (char *) pGeode->FBBase));
    else
        vg_set_display_offset(0);

    /* FIXME: What's up with X and Y?  Does that come into play
     * here? */

    df_configure_video_source(&vs_odd, &vs_even);

    vg_wait_vertical_blank();
}

static void
lx_crtc_commit(xf86CrtcPtr crtc)
{
    LXCrtcPrivatePtr lx_crtc = crtc->driver_private;
    ScrnInfoPtr pScrni = crtc->scrn;
    GeodeRec *pGeode = GEODEPTR(pScrni);

    /* Turn back on the sreen */
    crtc->funcs->dpms(crtc, DPMSModeOn);

    /* Turn on compression */

    if (pGeode->Compression) {
        vg_configure_compression(&(pGeode->CBData));
        vg_set_compression_enable(1);
    }

    /* Load the cursor */
    if (crtc->scrn->pScreen != NULL) {
        crtc->funcs->hide_cursor(crtc);
        crtc->cursor_shown = FALSE;
    }

    /* Renable the video */

    if (lx_crtc->video_enable)
        df_set_video_enable(lx_crtc->video_enable, lx_crtc->video_flags);

    lx_crtc->video_enable = 0;
    lx_crtc->video_flags = 0;
}

static void
lx_crtc_gamma_set(xf86CrtcPtr crtc, CARD16 *red, CARD16 *green,
                  CARD16 *blue, int size)
{
    unsigned int dcfg;
    int i;

    assert(size == 256);

    /* We need the Gamma Correction for video - fading operation,
     * the values address should be plused for every cycle.
     * Special for Screensaver Operation.
     */

    for (i = 0; i < 256; i++) {
        unsigned int val;

        (*red) &= 0xff00;
        (*green) &= 0xff00;
        (*blue) &= 0xff00;
        val = (*(red++) << 8) | *(green++) | (*(blue++) >> 8);

        df_set_video_palette_entry(i, val);
    }

    /* df_set_video_palette_entry automatically turns on
     * gamma for video - if this gets called, we assume that
     * RandR wants it set for graphics, so reverse cimarron
     */

    dcfg = READ_VID32(DF_DISPLAY_CONFIG);
    dcfg &= ~DF_DCFG_GV_PAL_BYP;
    WRITE_VID32(DF_DISPLAY_CONFIG, dcfg);
}

    /* The Xserver has a scratch pixmap allocation routine that will
     * try to use the existing scratch pixmap if possible. When the driver
     * or any other user stop using it, it need to clear out any pixmap
     * state (private data etc) otherwise the next user may get stale data.
     */

    /* Use our own wrapper to allocate a pixmap for wrapping a buffer object
     * It removes using scratch pixmaps for rotate.
     */
static PixmapPtr
lx_create_bo_pixmap(ScreenPtr pScreen,
                    int width, int height,
                    int depth, int bpp, int pitch, void *pPixData)
{
    PixmapPtr pixmap;

    pixmap = (*pScreen->CreatePixmap) (pScreen, 0, 0, depth, 0);

    if (!pixmap)
        return NULL;
    if (!(*pScreen->ModifyPixmapHeader) (pixmap, width, height,
                                         depth, bpp, pitch, pPixData)) {
        /* ModifyPixmapHeader failed, so we can't use it as scratch pixmap
         */
        dixDestroyPixmap(pixmap, 0);
        return NULL;
    }

    return pixmap;
}

    /* Allocates shadow memory, and allocating a new space for Rotation.
     * The size is measured in bytes, and the offset from the beginning
     * of card space is returned.
     */

static Bool
LXAllocShadow(ScrnInfoPtr pScrni, int size)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    if (pGeode->shadowArea) {
        if (pGeode->shadowArea->size != size) {
            exaOffscreenFree(pScrni->pScreen, pGeode->shadowArea);
            pGeode->shadowArea = NULL;
        }
    }

    if (pGeode->shadowArea == NULL) {
        pGeode->shadowArea =
            exaOffscreenAlloc(pScrni->pScreen, size, 4, TRUE, NULL, NULL);

        if (pGeode->shadowArea == NULL)
            return FALSE;
    }

    pScrni->fbOffset = pGeode->shadowArea->offset;
    return TRUE;
}

static void *
lx_crtc_shadow_allocate(xf86CrtcPtr crtc, int width, int height)
{
    ScrnInfoPtr pScrni = crtc->scrn;
    GeodePtr pGeode = GEODEPTR(pScrni);
    unsigned int rpitch, size;

    rpitch = pScrni->displayWidth * (pScrni->bitsPerPixel / 8);
    size = rpitch * height;

    /* Allocate shadow memory */
    if (LXAllocShadow(pScrni, size) == FALSE) {
        xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                   "Couldn't allocate the shadow memory for rotation\n");
        xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                   " You need 0x%x bytes, but only 0x%x bytes are available\n",
                   size, GeodeOffscreenFreeSize(pGeode));

        return NULL;
    }

    memset(pGeode->FBBase + pGeode->shadowArea->offset, 0, size);
    return pGeode->FBBase + pGeode->shadowArea->offset;
}

static PixmapPtr
lx_crtc_shadow_create(xf86CrtcPtr crtc, void *data, int width, int height)
{
    ScrnInfoPtr pScrni = crtc->scrn;
    PixmapPtr rpixmap;
    unsigned int rpitch;

    rpitch = pScrni->displayWidth * (pScrni->bitsPerPixel / 8);
    if (!data)
        data = lx_crtc_shadow_allocate(crtc, width, height);

    rpixmap = lx_create_bo_pixmap(pScrni->pScreen,
                                  width, height, pScrni->depth,
                                  pScrni->bitsPerPixel, rpitch, data);

    if (rpixmap == NULL) {
        xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                   "Couldn't allocate shadow pixmap for rotated CRTC\n");
    }

    return rpixmap;
}

static void
lx_crtc_shadow_destroy(xf86CrtcPtr crtc, PixmapPtr rpixmap, void *data)
{
    ScrnInfoPtr pScrni = crtc->scrn;
    GeodeRec *pGeode = GEODEPTR(pScrni);

    if (rpixmap)
        dixDestroyPixmap(rpixmap, 0);

    /* Free shadow memory */
    if (data) {
        gp_wait_until_idle();
        if (pGeode->shadowArea != NULL) {
            exaOffscreenFree(pScrni->pScreen, pGeode->shadowArea);
            pGeode->shadowArea = NULL;
        }
    }
}

static void
lx_crtc_set_cursor_colors(xf86CrtcPtr crtc, int bg, int fg)
{
    vg_set_mono_cursor_colors(bg, fg);
}

static void
lx_crtc_set_cursor_position(xf86CrtcPtr crtc, int x, int y)
{
    VG_PANNING_COORDINATES panning;

    vg_set_cursor_position(x, y, &panning);
}

static void
lx_crtc_show_cursor(xf86CrtcPtr crtc)
{
    vg_set_cursor_enable(1);
}

static void
lx_crtc_hide_cursor(xf86CrtcPtr crtc)
{
    vg_set_cursor_enable(0);
}

static void
lx_crtc_load_cursor_argb(xf86CrtcPtr crtc, CARD32 *image)
{
    LXLoadARGBCursorImage(crtc->scrn, (unsigned char *) image);
}

static const xf86CrtcFuncsRec lx_crtc_funcs = {
    .dpms = lx_crtc_dpms,
    .lock = lx_crtc_lock,
    .unlock = lx_crtc_unlock,
    .mode_fixup = lx_crtc_mode_fixup,
    .prepare = lx_crtc_prepare,
    .mode_set = lx_crtc_mode_set,
    .commit = lx_crtc_commit,
    .gamma_set = lx_crtc_gamma_set,
    .shadow_create = lx_crtc_shadow_create,
    .shadow_allocate = lx_crtc_shadow_allocate,
    .shadow_destroy = lx_crtc_shadow_destroy,
    .set_cursor_colors = lx_crtc_set_cursor_colors,
    .set_cursor_position = lx_crtc_set_cursor_position,
    .show_cursor = lx_crtc_show_cursor,
    .hide_cursor = lx_crtc_hide_cursor,
    .load_cursor_argb = lx_crtc_load_cursor_argb,
};

void
LXSetupCrtc(ScrnInfoPtr pScrni)
{
    xf86CrtcPtr crtc;
    LXCrtcPrivatePtr lxpriv;

    crtc = xf86CrtcCreate(pScrni, &lx_crtc_funcs);

    if (crtc == NULL) {
        ErrorF("ERROR - failed to create a CRTC\n");
        return;
    }

    lxpriv = XNFcallocarray(1, sizeof(LXCrtcPrivateRec));

    if (!lxpriv) {
        xf86CrtcDestroy(crtc);
        ErrorF("unable to allocate memory for lxpriv\n");
        return;
    }

    crtc->driver_private = lxpriv;
}
