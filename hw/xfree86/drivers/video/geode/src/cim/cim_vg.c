/*
 * Copyright (c) 2006 Advanced Micro Devices, Inc.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Neither the name of the Advanced Micro Devices, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 */

 /*
  * Cimarron display controller routines.  These routines program the display
  * mode and configure the hardware cursor and video buffers.
  */

/*---------------------*/
/* CIMARRON VG GLOBALS */
/*---------------------*/

CIMARRON_STATIC unsigned long vg3_x_hotspot = 0;
CIMARRON_STATIC unsigned long vg3_y_hotspot = 0;
CIMARRON_STATIC unsigned long vg3_cursor_offset = 0;
CIMARRON_STATIC unsigned long vg3_mode_width = 0;
CIMARRON_STATIC unsigned long vg3_mode_height = 0;
CIMARRON_STATIC unsigned long vg3_panel_width = 0;
CIMARRON_STATIC unsigned long vg3_panel_height = 0;
CIMARRON_STATIC unsigned long vg3_delta_x = 0;
CIMARRON_STATIC unsigned long vg3_delta_y = 0;
CIMARRON_STATIC unsigned long vg3_bpp = 0;

CIMARRON_STATIC unsigned long vg3_color_cursor = 0;
CIMARRON_STATIC unsigned long vg3_panel_enable = 0;

/*---------------------------------------------------------------------------
 * vg_delay_milliseconds
 *
 * This routine delays for a number of milliseconds based on a crude
 * delay loop.
 *--------------------------------------------------------------------------*/

int
vg_delay_milliseconds(unsigned long ms)
{
    /* ASSUME 500 MHZ 20 CLOCKS PER READ */

    unsigned long loop = ms * 25000;

    while (loop-- > 0) {
        READ_REG32(DC3_UNLOCK);
    }
    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vg_set_display_mode
 *
 * This routine sets a CRT display mode using predefined Cimarron timings.
 * The source width and height are specified to allow scaling.
 *--------------------------------------------------------------------------*/

int
vg_set_display_mode(unsigned long src_width, unsigned long src_height,
                    unsigned long dst_width, unsigned long dst_height,
                    int bpp, int hz, unsigned long flags)
{
    VG_QUERY_MODE crt_query;
    VG_DISPLAY_MODE crt_mode;
    int mode;

    crt_query.active_width = dst_width;
    crt_query.active_height = dst_height;
    crt_query.bpp = bpp;
    crt_query.hz = hz;
    crt_query.query_flags = VG_QUERYFLAG_ACTIVEWIDTH |
        VG_QUERYFLAG_ACTIVEHEIGHT | VG_QUERYFLAG_BPP | VG_QUERYFLAG_REFRESH;

    mode = vg_get_display_mode_index(&crt_query);
    if (mode >= 0) {
        crt_mode = CimarronDisplayModes[mode];
        crt_mode.src_width = src_width;
        crt_mode.src_height = src_height;

        /* ADD USER-REQUESTED FLAGS */

        crt_mode.flags |= (flags & VG_MODEFLAG_VALIDUSERFLAGS);

        if (flags & VG_MODEFLAG_OVERRIDE_BAND) {
            crt_mode.flags &= ~VG_MODEFLAG_BANDWIDTHMASK;
            crt_mode.flags |= (flags & VG_MODEFLAG_BANDWIDTHMASK);
        }
        if (flags & VG_MODEFLAG_INT_OVERRIDE) {
            crt_mode.flags &= ~VG_MODEFLAG_INT_MASK;
            crt_mode.flags |= (flags & VG_MODEFLAG_INT_MASK);
        }

        return vg_set_custom_mode(&crt_mode, bpp);
    }
    return CIM_STATUS_ERROR;
}

/*---------------------------------------------------------------------------
 * vg_set_panel_mode
 *
 * This routine sets a panel mode using predefined Cimarron fixed timings.
 * The source width and height specify the width and height of the data in
 * the frame buffer.  The destination width and height specify the width and
 * height of the active data to be displayed.  The panel width and height
 * specify the dimensions of the panel.  This interface allows the user to
 * scale or center graphics data or both.  To perform scaling, the src width
 * or height should be different than the destination width or height.  To
 * perform centering or panning, the destination width and height should be
 * different than the panel resolution.
 *--------------------------------------------------------------------------*/

int
vg_set_panel_mode(unsigned long src_width, unsigned long src_height,
                  unsigned long dst_width, unsigned long dst_height,
                  unsigned long panel_width, unsigned long panel_height,
                  int bpp, unsigned long flags)
{
    unsigned long sync_width;
    unsigned long sync_offset;
    VG_QUERY_MODE panel_query;
    VG_DISPLAY_MODE panel_mode;
    int mode;

    /* SEARCH CIMARRON'S TABLE OF PREDEFINED PANEL MODES                   */
    /* If the destination resolution is larger than the panel resolution,  */
    /* panning will be performed.  However, the timings for a panned mode  */
    /* are identical to the timings without panning.  To save space in the */
    /* mode tables, there are no additional table entries for modes with   */
    /* panning.  Instead, we read the timings for a mode without panning   */
    /* and override the structure entries that specify the width and       */
    /* height of the mode.  We perform a similar procedure for centered    */
    /* modes, except that certain timing parameters are dynamically        */
    /* calculated.                                                         */

    panel_query.active_width = panel_width;
    panel_query.active_height = panel_height;
    panel_query.panel_width = panel_width;
    panel_query.panel_height = panel_height;
    panel_query.bpp = bpp;
    panel_query.query_flags = VG_QUERYFLAG_ACTIVEWIDTH |
        VG_QUERYFLAG_ACTIVEHEIGHT |
        VG_QUERYFLAG_PANELWIDTH |
        VG_QUERYFLAG_PANELHEIGHT | VG_QUERYFLAG_PANEL | VG_QUERYFLAG_BPP;

    mode = vg_get_display_mode_index(&panel_query);

    /* COPY THE DATA FROM THE MODE TABLE TO A TEMPORARY STRUCTURE */

    if (mode >= 0) {
        panel_mode = CimarronDisplayModes[mode];
        panel_mode.mode_width = dst_width;
        panel_mode.mode_height = dst_height;
        panel_mode.src_width = src_width;
        panel_mode.src_height = src_height;

        /* ADD USER-REQUESTED FLAGS */

        panel_mode.flags |= (flags & VG_MODEFLAG_VALIDUSERFLAGS);

        if (flags & VG_MODEFLAG_OVERRIDE_BAND) {
            panel_mode.flags &= ~VG_MODEFLAG_BANDWIDTHMASK;
            panel_mode.flags |= (flags & VG_MODEFLAG_BANDWIDTHMASK);
        }
        if (flags & VG_MODEFLAG_INT_OVERRIDE) {
            panel_mode.flags &= ~VG_MODEFLAG_INT_MASK;
            panel_mode.flags |= (flags & VG_MODEFLAG_INT_MASK);
        }

        /* DYNAMICALLY CALCULATE CENTERED TIMINGS */
        /* For centered timings the blank start and blank end are set to  */
        /* half the difference between the mode dimension and the panel   */
        /* dimension.  The sync pulse preserves the width and offset from */
        /* blanking whenever possible.                                    */

        if (dst_width < panel_width) {
            sync_width = panel_mode.hsyncend - panel_mode.hsyncstart;
            sync_offset = panel_mode.hsyncstart - panel_mode.hblankstart;

            panel_mode.hactive = dst_width;
            panel_mode.hblankstart =
                panel_mode.hactive + ((panel_width - dst_width) >> 1);
            panel_mode.hblankend =
                panel_mode.htotal - ((panel_width - dst_width) >> 1);
            panel_mode.hsyncstart = panel_mode.hblankstart + sync_offset;
            panel_mode.hsyncend = panel_mode.hsyncstart + sync_width;

            panel_mode.flags |= VG_MODEFLAG_CENTERED;
        }
        if (dst_height < panel_height) {
            sync_width = panel_mode.vsyncend - panel_mode.vsyncstart;
            sync_offset = panel_mode.vsyncstart - panel_mode.vblankstart;

            panel_mode.vactive = dst_height;
            panel_mode.vblankstart =
                panel_mode.vactive + ((panel_height - dst_height) >> 1);
            panel_mode.vblankend =
                panel_mode.vtotal - ((panel_height - dst_height) >> 1);
            panel_mode.vsyncstart = panel_mode.vblankstart + sync_offset;
            panel_mode.vsyncend = panel_mode.vsyncstart + sync_width;

            panel_mode.flags |= VG_MODEFLAG_CENTERED;
        }
        return vg_set_custom_mode(&panel_mode, bpp);
    }
    return CIM_STATUS_ERROR;
}

/*---------------------------------------------------------------------------
 * vg_set_tv_mode
 *
 * This routine sets a TV display mode using predefined Cimarron timings.  The
 * source width and height are specified to allow scaling.
 *--------------------------------------------------------------------------*/

int
vg_set_tv_mode(unsigned long *src_width, unsigned long *src_height,
               unsigned long encoder, unsigned long tvres, int bpp,
               unsigned long flags, unsigned long h_overscan,
               unsigned long v_overscan)
{
    unsigned long sync_width;
    unsigned long sync_offset;
    VG_QUERY_MODE tv_query;
    VG_DISPLAY_MODE tv_mode;
    int mode;

    if (!src_width || !src_height)
        return CIM_STATUS_INVALIDPARAMS;

    tv_query.bpp = bpp;
    tv_query.encoder = encoder;
    tv_query.tvmode = tvres;
    tv_query.query_flags = VG_QUERYFLAG_BPP | VG_QUERYFLAG_TVOUT |
        VG_QUERYFLAG_ENCODER | VG_QUERYFLAG_TVMODE;

    mode = vg_get_display_mode_index(&tv_query);
    if (mode >= 0) {
        /* RETRIEVE THE UNSCALED RESOLUTION
         * As we are indexing here simply by a mode and encoder, the actual
         * timings may vary.  A 0 value for source or height will thus query
         * the unscaled resolution.
         */

        if (!(*src_width) || !(*src_height)) {
            *src_width = CimarronDisplayModes[mode].hactive - (h_overscan << 1);
            *src_height = CimarronDisplayModes[mode].vactive;

            if (CimarronDisplayModes[mode].flags & VG_MODEFLAG_INTERLACED) {
                if (((flags & VG_MODEFLAG_INT_OVERRIDE) &&
                     (flags & VG_MODEFLAG_INT_MASK) ==
                     VG_MODEFLAG_INT_LINEDOUBLE)
                    || (!(flags & VG_MODEFLAG_INT_OVERRIDE)
                        && (CimarronDisplayModes[mode].flags &
                            VG_MODEFLAG_INT_MASK) ==
                        VG_MODEFLAG_INT_LINEDOUBLE)) {
                    if (CimarronDisplayModes[mode].vactive_even >
                        CimarronDisplayModes[mode].vactive)
                        *src_height = CimarronDisplayModes[mode].vactive_even;

                    /* ONLY 1/2 THE OVERSCAN FOR LINE DOUBLED MODES */

                    *src_height -= v_overscan;
                }
                else {
                    *src_height += CimarronDisplayModes[mode].vactive_even;
                    *src_height -= v_overscan << 1;
                }
            }
            else {
                *src_height -= v_overscan << 1;
            }

            return CIM_STATUS_OK;
        }

        tv_mode = CimarronDisplayModes[mode];
        tv_mode.src_width = *src_width;
        tv_mode.src_height = *src_height;

        /* ADD USER-REQUESTED FLAGS */

        tv_mode.flags |= (flags & VG_MODEFLAG_VALIDUSERFLAGS);

        if (flags & VG_MODEFLAG_OVERRIDE_BAND) {
            tv_mode.flags &= ~VG_MODEFLAG_BANDWIDTHMASK;
            tv_mode.flags |= (flags & VG_MODEFLAG_BANDWIDTHMASK);
        }
        if (flags & VG_MODEFLAG_INT_OVERRIDE) {
            tv_mode.flags &= ~VG_MODEFLAG_INT_MASK;
            tv_mode.flags |= (flags & VG_MODEFLAG_INT_MASK);
        }

        /* ADJUST FOR OVERSCAN */

        if (h_overscan) {
            sync_width = tv_mode.hsyncend - tv_mode.hsyncstart;
            sync_offset = tv_mode.hsyncstart - tv_mode.hblankstart;

            tv_mode.hactive -= h_overscan << 1;
            tv_mode.hblankstart = tv_mode.hactive + h_overscan;
            tv_mode.hblankend = tv_mode.htotal - h_overscan;
            tv_mode.hsyncstart = tv_mode.hblankstart + sync_offset;
            tv_mode.hsyncend = tv_mode.hsyncstart + sync_width;

            tv_mode.flags |= VG_MODEFLAG_CENTERED;
        }
        if (v_overscan) {
            sync_width = tv_mode.vsyncend - tv_mode.vsyncstart;
            sync_offset = tv_mode.vsyncstart - tv_mode.vblankstart;

            if (tv_mode.flags & VG_MODEFLAG_INTERLACED) {
                tv_mode.vactive -= v_overscan;
                tv_mode.vblankstart = tv_mode.vactive + (v_overscan >> 1);
                tv_mode.vblankend = tv_mode.vtotal - (v_overscan >> 1);
                tv_mode.vsyncstart = tv_mode.vblankstart + sync_offset;
                tv_mode.vsyncend = tv_mode.vsyncstart + sync_width;

                sync_width = tv_mode.vsyncend_even - tv_mode.vsyncstart_even;
                sync_offset = tv_mode.vsyncstart_even -
                    tv_mode.vblankstart_even;

                tv_mode.vactive_even -= v_overscan;
                tv_mode.vblankstart_even =
                    tv_mode.vactive_even + (v_overscan >> 1);
                tv_mode.vblankend_even =
                    tv_mode.vtotal_even - (v_overscan >> 1);
                tv_mode.vsyncstart_even =
                    tv_mode.vblankstart_even + sync_offset;
                tv_mode.vsyncend_even = tv_mode.vsyncstart_even + sync_width;
            }
            else {
                tv_mode.vactive -= v_overscan << 1;
                tv_mode.vblankstart = tv_mode.vactive + v_overscan;
                tv_mode.vblankend = tv_mode.vtotal - v_overscan;
                tv_mode.vsyncstart = tv_mode.vblankstart + sync_offset;
                tv_mode.vsyncend = tv_mode.vsyncstart + sync_width;
            }

            tv_mode.flags |= VG_MODEFLAG_CENTERED;
        }

        /* TV MODES WILL NEVER ALLOW PANNING */

        tv_mode.panel_width = tv_mode.hactive;
        tv_mode.panel_height = tv_mode.vactive;
        tv_mode.mode_width = tv_mode.hactive;
        tv_mode.mode_height = tv_mode.vactive;

        return vg_set_custom_mode(&tv_mode, bpp);
    }
    return CIM_STATUS_ERROR;
}

/*---------------------------------------------------------------------------
 * vg_set_custom_mode
 *
 * This routine sets a display mode.  The API is structured such that this
 * routine can be called from four sources:
 *   - vg_set_display_mode
 *   - vg_set_panel_mode
 *   - vg_set_tv_mode
 *   - directly by the user for a custom mode.
 *--------------------------------------------------------------------------*/

int
vg_set_custom_mode(VG_DISPLAY_MODE *mode_params, int bpp)
{
    unsigned long config, misc, temp;
    unsigned long irq_ctl, genlk_ctl;
    unsigned long unlock, flags;
    unsigned long acfg, gcfg, dcfg;
    unsigned long size, line_size, pitch;
    unsigned long bpp_mask, dv_size;
    unsigned long hscale, vscale, starting_width;
    unsigned long starting_height, output_height;
    Q_WORD msr_value = { 0, 0 };

    /* DETERMINE DIMENSIONS FOR SCALING */
    /* Scaling is performed before flicker filtering and interlacing */

    output_height = mode_params->vactive;

    if (mode_params->flags & VG_MODEFLAG_INTERLACED) {
        /* EVEN AND ODD FIELDS ARE SEPARATE
         * The composite image height is the sum of the height of both
         * fields
         */

        if ((mode_params->flags & VG_MODEFLAG_INT_MASK) ==
            VG_MODEFLAG_INT_FLICKER
            || (mode_params->flags & VG_MODEFLAG_INT_MASK) ==
            VG_MODEFLAG_INT_ADDRESS) {
            output_height += mode_params->vactive_even;
        }

        /* LINE DOUBLING
         * The composite image height is the greater of the two field
         * heights.
         */

        else if (mode_params->vactive_even > output_height)
            output_height = mode_params->vactive_even;
    }

    /* CHECK FOR VALID SCALING FACTOR
     * GeodeLX supports only 2:1 vertical downscale (before interlacing) and
     * 2:1 horizontal downscale.  The source width when scaling must be
     * less than or equal to 1024 pixels.  The destination can be any size,
     * except when flicker filtering is enabled.
     */

    irq_ctl = 0;
    if (mode_params->flags & VG_MODEFLAG_PANELOUT) {
        if (mode_params->src_width != mode_params->mode_width) {
            starting_width = (mode_params->hactive * mode_params->src_width) /
                mode_params->mode_width;
            hscale = (mode_params->src_width << 14) /
                (mode_params->mode_width - 1);
            irq_ctl |= (DC3_IRQFILT_ALPHA_FILT_EN | DC3_IRQFILT_GFX_FILT_EN);
        }
        else {
            starting_width = mode_params->hactive;
            hscale = 0x4000;
        }
        if (mode_params->src_height != mode_params->mode_height) {
            starting_height = (output_height * mode_params->src_height) /
                mode_params->mode_height;
            vscale = (mode_params->src_height << 14) /
                (mode_params->mode_height - 1);
            irq_ctl |= (DC3_IRQFILT_ALPHA_FILT_EN | DC3_IRQFILT_GFX_FILT_EN);
        }
        else {
            starting_height = output_height;
            vscale = 0x4000;
        }
    }
    else {
        starting_width = mode_params->src_width;
        starting_height = mode_params->src_height;
        if (mode_params->src_width != mode_params->hactive) {
            hscale = (mode_params->src_width << 14) /
                (mode_params->hactive - 1);
            irq_ctl |= (DC3_IRQFILT_ALPHA_FILT_EN | DC3_IRQFILT_GFX_FILT_EN);
        }
        else {
            hscale = 0x4000;
        }
        if (mode_params->src_height != output_height) {
            vscale = (mode_params->src_height << 14) / (output_height - 1);
            irq_ctl |= (DC3_IRQFILT_ALPHA_FILT_EN | DC3_IRQFILT_GFX_FILT_EN);
        }
        else {
            vscale = 0x4000;
        }
    }

    starting_width = (starting_width + 7) & 0xFFFF8;

    if (mode_params->hactive < (starting_width >> 1) ||
        output_height < (starting_height >> 1) ||
        (irq_ctl && (starting_width > 1024))) {
        return CIM_STATUS_INVALIDSCALE;
    }

    /* VERIFY INTERLACED SCALING */
    /* The output width must be less than or equal to 1024 pixels when the */
    /* flicker filter is enabled.  Also, scaling should be disabled when   */
    /* the interlacing mode is set to interlaced addressing.               */

    if (mode_params->flags & VG_MODEFLAG_INTERLACED) {
        if ((((mode_params->flags & VG_MODEFLAG_INT_MASK) ==
              VG_MODEFLAG_INT_FLICKER) && (mode_params->hactive > 1024))
            || (((mode_params->flags & VG_MODEFLAG_INT_MASK) ==
                 VG_MODEFLAG_INT_ADDRESS) && irq_ctl)) {
            return CIM_STATUS_INVALIDSCALE;
        }
    }

    /* CHECK FOR VALID BPP */

    switch (bpp) {
    case 8:
        bpp_mask = DC3_DCFG_DISP_MODE_8BPP;
        break;
    case 24:
        bpp_mask = DC3_DCFG_DISP_MODE_24BPP;
        break;
    case 32:
        bpp_mask = DC3_DCFG_DISP_MODE_32BPP;
        break;
    case 12:
        bpp_mask = DC3_DCFG_DISP_MODE_16BPP | DC3_DCFG_12BPP;
        break;
    case 15:
        bpp_mask = DC3_DCFG_DISP_MODE_16BPP | DC3_DCFG_15BPP;
        break;
    case 16:
        bpp_mask = DC3_DCFG_DISP_MODE_16BPP | DC3_DCFG_16BPP;
        break;
    default:
        return CIM_STATUS_INVALIDPARAMS;
    }

    vg3_bpp = bpp;

    /* CLEAR PANNING OFFSETS */

    vg3_delta_x = 0;
    vg3_delta_y = 0;

    /* SAVE PANEL PARAMETERS */

    if (mode_params->flags & VG_MODEFLAG_PANELOUT) {
        vg3_panel_enable = 1;
        vg3_panel_width = mode_params->panel_width;
        vg3_panel_height = mode_params->panel_height;
        vg3_mode_width = mode_params->mode_width;
        vg3_mode_height = mode_params->mode_height;

        /* INVERT THE SHIFT CLOCK IF REQUESTED */
        /* Note that we avoid writing the power management register if */
        /* we can help it.                                             */

        temp = READ_VID32(DF_POWER_MANAGEMENT);
        if ((mode_params->flags & VG_MODEFLAG_INVERT_SHFCLK) &&
            !(temp & DF_PM_INVERT_SHFCLK)) {
            WRITE_VID32(DF_POWER_MANAGEMENT, (temp | DF_PM_INVERT_SHFCLK));
        }
        else if (!(mode_params->flags & VG_MODEFLAG_INVERT_SHFCLK) &&
                 (temp & DF_PM_INVERT_SHFCLK)) {
            WRITE_VID32(DF_POWER_MANAGEMENT, (temp & ~DF_PM_INVERT_SHFCLK));
        }

        /* SET PANEL TIMING VALUES */

        if (!(mode_params->flags & VG_MODEFLAG_NOPANELTIMINGS)) {
            unsigned long pmtim1, pmtim2, dith_ctl;

            if (mode_params->flags & VG_MODEFLAG_XVGA_TFT) {
                pmtim1 = DF_DEFAULT_XVGA_PMTIM1;
                pmtim2 = DF_DEFAULT_XVGA_PMTIM2;
                dith_ctl = DF_DEFAULT_DITHCTL;
                msr_value.low = DF_DEFAULT_XVGA_PAD_SEL_LOW;
                msr_value.high = DF_DEFAULT_XVGA_PAD_SEL_HIGH;
            }
            else if (mode_params->flags & VG_MODEFLAG_CUSTOM_PANEL) {
                pmtim1 = mode_params->panel_tim1;
                pmtim2 = mode_params->panel_tim2;
                dith_ctl = mode_params->panel_dither_ctl;
                msr_value.low = mode_params->panel_pad_sel_low;
                msr_value.high = mode_params->panel_pad_sel_high;
            }
            else {
                pmtim1 = DF_DEFAULT_TFT_PMTIM1;
                pmtim2 = DF_DEFAULT_TFT_PMTIM2;
                dith_ctl = DF_DEFAULT_DITHCTL;
                msr_value.low = DF_DEFAULT_TFT_PAD_SEL_LOW;
                msr_value.high = DF_DEFAULT_TFT_PAD_SEL_HIGH;

            }
            WRITE_VID32(DF_VIDEO_PANEL_TIM1, pmtim1);
            WRITE_VID32(DF_VIDEO_PANEL_TIM2, pmtim2);
            WRITE_VID32(DF_DITHER_CONTROL, dith_ctl);
            msr_write64(MSR_DEVICE_GEODELX_DF, DF_MSR_PAD_SEL, &msr_value);
        }

        /* SET APPROPRIATE PANEL OUTPUT MODE */

        msr_read64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_CONFIG, &msr_value);

        msr_value.low &= ~DF_CONFIG_OUTPUT_MASK;
        msr_value.low |= DF_OUTPUT_PANEL;
        if (mode_params->flags & VG_MODEFLAG_CRT_AND_FP)
            msr_value.low |= DF_SIMULTANEOUS_CRT_FP;
        else
            msr_value.low &= ~DF_SIMULTANEOUS_CRT_FP;

        msr_write64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_CONFIG, &msr_value);

    }
    else if (mode_params->flags & VG_MODEFLAG_TVOUT) {
        vg3_panel_enable = 0;

        /* SET APPROPRIATE TV OUTPUT MODE */

        msr_read64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_CONFIG, &msr_value);

        msr_value.low &= ~DF_CONFIG_OUTPUT_MASK;
        msr_value.low |= DF_OUTPUT_PANEL;
        if (mode_params->flags & VG_MODEFLAG_CRT_AND_FP)
            msr_value.low |= DF_SIMULTANEOUS_CRT_FP;
        else
            msr_value.low &= ~DF_SIMULTANEOUS_CRT_FP;

        msr_write64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_CONFIG, &msr_value);

        /* CONFIGURE PADS FOR VOP OUTPUT */
        /* Note that the VOP clock is currently always inverted. */

        msr_value.low = DF_DEFAULT_TV_PAD_SEL_LOW;
        msr_value.high = DF_DEFAULT_TV_PAD_SEL_HIGH;
        msr_write64(MSR_DEVICE_GEODELX_DF, DF_MSR_PAD_SEL, &msr_value);
    }
    else {
        vg3_panel_enable = 0;

        /* SET OUTPUT TO CRT ONLY */

        msr_read64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_CONFIG, &msr_value);
        msr_value.low &= ~DF_CONFIG_OUTPUT_MASK;
        msr_value.low |= DF_OUTPUT_CRT;
        msr_write64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_CONFIG, &msr_value);
    }

    /* SET UNLOCK VALUE */

    unlock = READ_REG32(DC3_UNLOCK);
    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);

    /*-------------------------------------------------------------------*/
    /* MAKE THE SYSTEM "SAFE"                                            */
    /* Before setting a mode, we first ensure that the system is in a    */
    /* benign quiescent state.  This involves disabling compression and  */
    /* all interrupt sources.  It also involves terminating all accesses */
    /* to memory, including video, FIFO load, VIP and the GP.            */
    /*-------------------------------------------------------------------*/

    /* DISABLE VGA
     * VGA *MUST* be turned off before TGEN is enabled.  If not, a condition
     * will result where VGA Enable is waiting for a VSync to be latched but
     * a VSync will not be generated until VGA is disabled.
     */

    temp = READ_REG32(DC3_GENERAL_CFG) & ~DC3_GCFG_VGAE;

    /* DISABLE VIDEO (INCLUDING ALPHA WINDOWS) */

    WRITE_VID32(DF_ALPHA_CONTROL_1, 0);
    WRITE_VID32(DF_ALPHA_CONTROL_1 + 32, 0);
    WRITE_VID32(DF_ALPHA_CONTROL_1 + 64, 0);

    WRITE_REG32(DC3_GENERAL_CFG, (temp & ~DC3_GCFG_VIDE));
    temp = READ_VID32(DF_VIDEO_CONFIG);
    WRITE_VID32(DF_VIDEO_CONFIG, (temp & ~DF_VCFG_VID_EN));

    /* DISABLE VG INTERRUPTS */

    WRITE_REG32(DC3_IRQ, DC3_IRQ_MASK | DC3_VSYNC_IRQ_MASK |
                DC3_IRQ_STATUS | DC3_VSYNC_IRQ_STATUS);

    /* DISABLE GENLOCK */

    genlk_ctl = READ_REG32(DC3_GENLK_CTL);
    WRITE_REG32(DC3_GENLK_CTL, (genlk_ctl & ~DC3_GC_GENLOCK_ENABLE));

    /* DISABLE VIP CAPTURE AND VIP INTERRUPTS */

    WRITE_VIP32(VIP_CONTROL1, 0);
    WRITE_VIP32(VIP_CONTROL2, 0);
    WRITE_VIP32(VIP_INTERRUPT, VIP_ALL_INTERRUPTS | (VIP_ALL_INTERRUPTS >> 16));

    /* DISABLE COLOR KEYING
     * The color key mechanism should be disabled whenever a mode switch
     * occurs.
     */

    temp = READ_REG32(DC3_COLOR_KEY);
    WRITE_REG32(DC3_COLOR_KEY, (temp & ~DC3_CLR_KEY_ENABLE));

    /* BLANK THE DISPLAY
     * Note that we never blank the panel.  Most flat panels have very long
     * latency requirements when setting their power low.  Some panels require
     * upwards of 500ms before VDD goes high again.  Needless to say, we are
     * not planning to take over one half a second inside this routine.
     */

    misc = READ_VID32(DF_VID_MISC);
    config = READ_VID32(DF_DISPLAY_CONFIG);

    WRITE_VID32(DF_VID_MISC, (misc | DF_DAC_POWER_DOWN));
    WRITE_VID32(DF_DISPLAY_CONFIG,
                (config & ~(DF_DCFG_DIS_EN | DF_DCFG_HSYNC_EN |
                            DF_DCFG_VSYNC_EN | DF_DCFG_DAC_BL_EN)));

    /* DISABLE COMPRESSION  */

    gcfg = READ_REG32(DC3_GENERAL_CFG);
    gcfg &= ~(DC3_GCFG_CMPE | DC3_GCFG_DECE);
    WRITE_REG32(DC3_GENERAL_CFG, gcfg);

    /* DISABLE THE TIMING GENERATOR */

    dcfg = READ_REG32(DC3_DISPLAY_CFG);
    dcfg &= ~DC3_DCFG_TGEN;
    WRITE_REG32(DC3_DISPLAY_CFG, dcfg);

    /* WAIT FOR PENDING MEMORY REQUESTS */

    vg_delay_milliseconds(1);

    /* DISABLE DISPLAY FIFO LOAD */

    gcfg &= ~DC3_GCFG_DFLE;
    WRITE_REG32(DC3_GENERAL_CFG, gcfg);
    gcfg = 0;
    dcfg = 0;

    /* WAIT FOR THE GP TO BE IDLE (JUST IN CASE) */

    while (((temp = READ_GP32(GP3_BLT_STATUS)) & GP3_BS_BLT_BUSY) ||
           !(temp & GP3_BS_CB_EMPTY)) {
        ;
    }

    /* SET THE DOT CLOCK FREQUENCY */

    if (!(mode_params->flags & VG_MODEFLAG_EXCLUDEPLL)) {
        if (mode_params->flags & VG_MODEFLAG_HALFCLOCK)
            flags = VG_PLL_DIVIDE_BY_2;
        else if (mode_params->flags & VG_MODEFLAG_QVGA)
            flags = VG_PLL_DIVIDE_BY_4;
        else
            flags = 0;

        /* ALLOW DOTREF TO BE USED AS THE PLL            */
        /* This is useful for some external TV encoders. */

        if (mode_params->flags & VG_MODEFLAG_PLL_BYPASS)
            flags |= VG_PLL_BYPASS;

        /* ALLOW THE USER TO MANUALLY ENTER THE MSR VALUE */

        if (mode_params->flags & VG_MODEFLAG_MANUAL_FREQUENCY)
            flags |= VG_PLL_MANUAL;
        if (mode_params->flags & VG_MODEFLAG_VIP_TO_DOT_CLOCK)
            flags |= VG_PLL_VIP_CLOCK;

        vg_set_clock_frequency(mode_params->frequency, flags);
    }

    /* CLEAR ALL BUFFER OFFSETS */

    WRITE_REG32(DC3_FB_ST_OFFSET, 0);
    WRITE_REG32(DC3_CB_ST_OFFSET, 0);
    WRITE_REG32(DC3_CURS_ST_OFFSET, 0);

    genlk_ctl = READ_REG32(DC3_GENLK_CTL) & ~(DC3_GC_ALPHA_FLICK_ENABLE |
                                              DC3_GC_FLICKER_FILTER_ENABLE |
                                              DC3_GC_FLICKER_FILTER_MASK);

    /* ENABLE INTERLACING */

    if (mode_params->flags & VG_MODEFLAG_INTERLACED) {
        irq_ctl |= DC3_IRQFILT_INTL_EN;

        if ((mode_params->flags & VG_MODEFLAG_INT_MASK) ==
            VG_MODEFLAG_INT_ADDRESS)
            irq_ctl |= DC3_IRQFILT_INTL_ADDR;
        else if ((mode_params->flags & VG_MODEFLAG_INT_MASK) ==
                 VG_MODEFLAG_INT_FLICKER) {
            genlk_ctl |= DC3_GC_FLICKER_FILTER_1_8 |
                DC3_GC_FLICKER_FILTER_ENABLE | DC3_GC_ALPHA_FLICK_ENABLE;
        }
    }

    WRITE_REG32(DC3_GFX_SCALE, (vscale << 16) | (hscale & 0xFFFF));
    WRITE_REG32(DC3_IRQ_FILT_CTL, irq_ctl);
    WRITE_REG32(DC3_GENLK_CTL, genlk_ctl);

    /* SET LINE SIZE AND PITCH
     * The line size and pitch are calculated from the src_width parameter
     * passed in to this routine.  All other parameters are ignored.
     * The pitch is set either to a power of 2 to allow efficient
     * compression or to a linear value to allow efficient memory management.
     */

    switch (bpp) {
    case 8:
        size = mode_params->src_width;
        line_size = starting_width;
        break;

    case 12:
    case 15:
    case 16:

        size = mode_params->src_width << 1;
        line_size = starting_width << 1;
        break;

    case 24:
    case 32:
    default:

        size = mode_params->src_width << 2;
        line_size = starting_width << 2;
        break;
    }

    /* CALCULATE DV RAM SETTINGS AND POWER OF 2 PITCH */

    pitch = 1024;
    dv_size = DC3_DV_LINE_SIZE_1024;

    if (size > 1024) {
        pitch = 2048;
        dv_size = DC3_DV_LINE_SIZE_2048;
    }
    if (size > 2048) {
        pitch = 4096;
        dv_size = DC3_DV_LINE_SIZE_4096;
    }
    if (size > 4096) {
        pitch = 8192;
        dv_size = DC3_DV_LINE_SIZE_8192;
    }

    /* OVERRIDE SETTINGS FOR LINEAR PITCH */

    if (mode_params->flags & VG_MODEFLAG_LINEARPITCH) {
        unsigned long max;

        if (pitch != size) {
            /* CALCULATE MAXIMUM ADDRESS (1K ALIGNED) */

            max = size * output_height;
            max = (max + 0x3FF) & 0xFFFFFC00;
            WRITE_REG32(DC3_DV_TOP, max | DC3_DVTOP_ENABLE);

            gcfg |= DC3_GCFG_FDTY;
            pitch = size;
        }
        else {
            WRITE_REG32(DC3_DV_TOP, 0);
        }
    }

    /* WRITE PITCH AND DV RAM SETTINGS */
    /* The DV RAM line length is programmed at a power of 2 boundary */
    /* in case the user wants to toggle back to a power of 2 pitch   */
    /* later.  It could happen...                                    */

    temp = READ_REG32(DC3_DV_CTL);
    WRITE_REG32(DC3_GFX_PITCH, pitch >> 3);
    WRITE_REG32(DC3_DV_CTL, (temp & ~DC3_DV_LINE_SIZE_MASK) | dv_size);

    /* SET THE LINE SIZE */

    WRITE_REG32(DC3_LINE_SIZE, (line_size + 7) >> 3);

    /* ALWAYS ENABLE VIDEO AND GRAPHICS DATA            */
    /* These bits are relics from a previous design and */
    /* should always be enabled.                        */

    dcfg |= (DC3_DCFG_VDEN | DC3_DCFG_GDEN);

    /* SET PIXEL FORMAT */

    dcfg |= bpp_mask;

    /* ENABLE TIMING GENERATOR, TIM. REG. UPDATES, PALETTE BYPASS */
    /* AND VERT. INT. SELECT                                      */

    dcfg |= (unsigned long) (DC3_DCFG_TGEN | DC3_DCFG_TRUP | DC3_DCFG_PALB |
                             DC3_DCFG_VISL);

    /* SET FIFO PRIORITIES AND DISPLAY FIFO LOAD ENABLE
     * Note that the bandwidth setting gets upgraded when scaling or flicker
     * filtering are enabled, as they require more data throughput.
     */

    msr_read64(MSR_DEVICE_GEODELX_VG, DC3_SPARE_MSR, &msr_value);
    msr_value.low &= ~(DC3_SPARE_DISABLE_CFIFO_HGO |
                       DC3_SPARE_VFIFO_ARB_SELECT |
                       DC3_SPARE_LOAD_WM_LPEN_MASK | DC3_SPARE_WM_LPEN_OVRD |
                       DC3_SPARE_DISABLE_INIT_VID_PRI |
                       DC3_SPARE_DISABLE_VFIFO_WM);

    if ((mode_params->flags & VG_MODEFLAG_BANDWIDTHMASK) ==
        VG_MODEFLAG_HIGH_BAND || ((mode_params->flags & VG_MODEFLAG_INTERLACED)
                                  && (mode_params->flags & VG_MODEFLAG_INT_MASK)
                                  == VG_MODEFLAG_INT_FLICKER) ||
        (irq_ctl & DC3_IRQFILT_GFX_FILT_EN)) {
        /* HIGH BANDWIDTH */
        /* Set aggressive watermarks and disallow forced low priority */

        gcfg |= 0x0000BA01;
        dcfg |= 0x000EA000;
        acfg = 0x001A0201;

        msr_value.low |= DC3_SPARE_DISABLE_CFIFO_HGO |
            DC3_SPARE_VFIFO_ARB_SELECT | DC3_SPARE_WM_LPEN_OVRD;
    }
    else if ((mode_params->flags & VG_MODEFLAG_BANDWIDTHMASK) ==
             VG_MODEFLAG_AVG_BAND) {
        /* AVERAGE BANDWIDTH
         * Set average watermarks and allow small regions of forced low
         * priority.
         */

        gcfg |= 0x0000B601;
        dcfg |= 0x00009000;
        acfg = 0x00160001;

        msr_value.low |= DC3_SPARE_DISABLE_CFIFO_HGO |
            DC3_SPARE_VFIFO_ARB_SELECT | DC3_SPARE_WM_LPEN_OVRD;

        /* SET THE NUMBER OF LOW PRIORITY LINES TO 1/2 THE TOTAL AVAILABLE */

        temp = ((READ_REG32(DC3_V_ACTIVE_TIMING) >> 16) & 0x7FF) + 1;
        temp -= (READ_REG32(DC3_V_SYNC_TIMING) & 0x7FF) + 1;
        temp >>= 1;
        if (temp > 127)
            temp = 127;

        acfg |= temp << 9;
    }
    else if ((mode_params->flags & VG_MODEFLAG_BANDWIDTHMASK) ==
             VG_MODEFLAG_LOW_BAND) {
        /* LOW BANDWIDTH
         * Set low watermarks and allow larger regions of forced low priority
         */

        gcfg |= 0x00009501;
        dcfg |= 0x00008000;
        acfg = 0x00150001;

        msr_value.low |= DC3_SPARE_DISABLE_CFIFO_HGO |
            DC3_SPARE_VFIFO_ARB_SELECT | DC3_SPARE_WM_LPEN_OVRD;

        /* SET THE NUMBER OF LOW PRIORITY LINES TO 3/4 THE TOTAL AVAILABLE */

        temp = ((READ_REG32(DC3_V_ACTIVE_TIMING) >> 16) & 0x7FF) + 1;
        temp -= (READ_REG32(DC3_V_SYNC_TIMING) & 0x7FF) + 1;
        temp = (temp * 3) >> 2;
        if (temp > 127)
            temp = 127;

        acfg |= temp << 9;
    }
    else {
        /* LEGACY CHARACTERISTICS */
        /* Arbitration from a single set of watermarks. */

        gcfg |= 0x0000B601;
        msr_value.low |= DC3_SPARE_DISABLE_VFIFO_WM |
            DC3_SPARE_DISABLE_INIT_VID_PRI;
        acfg = 0;
    }

    msr_write64(MSR_DEVICE_GEODELX_VG, DC3_SPARE_MSR, &msr_value);

    /* ENABLE FLAT PANEL CENTERING                          */
    /* For panel modes having a resolution smaller than the */
    /* panel resolution, turn on data centering.            */

    if (mode_params->flags & VG_MODEFLAG_CENTERED)
        dcfg |= DC3_DCFG_DCEN;

    /* COMBINE AND SET TIMING VALUES */

    temp = (mode_params->hactive - 1) | ((mode_params->htotal - 1) << 16);
    WRITE_REG32(DC3_H_ACTIVE_TIMING, temp);
    temp = (mode_params->hblankstart - 1) |
        ((mode_params->hblankend - 1) << 16);
    WRITE_REG32(DC3_H_BLANK_TIMING, temp);
    temp = (mode_params->hsyncstart - 1) | ((mode_params->hsyncend - 1) << 16);
    WRITE_REG32(DC3_H_SYNC_TIMING, temp);
    temp = (mode_params->vactive - 1) | ((mode_params->vtotal - 1) << 16);
    WRITE_REG32(DC3_V_ACTIVE_TIMING, temp);
    temp = (mode_params->vblankstart - 1) |
        ((mode_params->vblankend - 1) << 16);
    WRITE_REG32(DC3_V_BLANK_TIMING, temp);
    temp = (mode_params->vsyncstart - 1) | ((mode_params->vsyncend - 1) << 16);
    WRITE_REG32(DC3_V_SYNC_TIMING, temp);
    temp = (mode_params->vactive_even - 1) | ((mode_params->vtotal_even -
                                               1) << 16);
    WRITE_REG32(DC3_V_ACTIVE_EVEN, temp);
    temp = (mode_params->vblankstart_even - 1) |
        ((mode_params->vblankend_even - 1) << 16);
    WRITE_REG32(DC3_V_BLANK_EVEN, temp);
    temp = (mode_params->vsyncstart_even - 1) |
        ((mode_params->vsyncend_even - 1) << 16);
    WRITE_REG32(DC3_V_SYNC_EVEN, temp);

    /* SET THE VIDEO REQUEST REGISTER */

    WRITE_VID32(DF_VIDEO_REQUEST, 0);

    /* SET SOURCE DIMENSIONS */

    WRITE_REG32(DC3_FB_ACTIVE, ((starting_width - 1) << 16) |
                (starting_height - 1));

    /* SET SYNC POLARITIES */

    temp = READ_VID32(DF_DISPLAY_CONFIG);

    temp &= ~(DF_DCFG_CRT_SYNC_SKW_MASK | DF_DCFG_PWR_SEQ_DLY_MASK |
              DF_DCFG_CRT_HSYNC_POL | DF_DCFG_CRT_VSYNC_POL);

    temp |= DF_DCFG_CRT_SYNC_SKW_INIT | DF_DCFG_PWR_SEQ_DLY_INIT;

    if (mode_params->flags & VG_MODEFLAG_NEG_HSYNC)
        temp |= DF_DCFG_CRT_HSYNC_POL;
    if (mode_params->flags & VG_MODEFLAG_NEG_VSYNC)
        temp |= DF_DCFG_CRT_VSYNC_POL;

    WRITE_VID32(DF_DISPLAY_CONFIG, temp);

    WRITE_REG32(DC3_DISPLAY_CFG, dcfg);
    WRITE_REG32(DC3_ARB_CFG, acfg);
    WRITE_REG32(DC3_GENERAL_CFG, gcfg);

    /* RESTORE VALUE OF DC3_UNLOCK */

    WRITE_REG32(DC3_UNLOCK, unlock);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vg_set_bpp
 *
 * This routine changes the display BPP on the fly.  It is intended only to 
 * switch between pixel depths of the same pixel size 24<->32 or 15<->16, NOT
 * between pixel depths of differing sizes 16<->32
 *--------------------------------------------------------------------------*/

int
vg_set_display_bpp(int bpp)
{
    unsigned long unlock, dcfg, bpp_mask;

    switch (bpp) {
    case 8:
        bpp_mask = DC3_DCFG_DISP_MODE_8BPP;
        break;
    case 24:
        bpp_mask = DC3_DCFG_DISP_MODE_24BPP;
        break;
    case 32:
        bpp_mask = DC3_DCFG_DISP_MODE_32BPP;
        break;
    case 12:
        bpp_mask = DC3_DCFG_DISP_MODE_16BPP | DC3_DCFG_12BPP;
        break;
    case 15:
        bpp_mask = DC3_DCFG_DISP_MODE_16BPP | DC3_DCFG_15BPP;
        break;
    case 16:
        bpp_mask = DC3_DCFG_DISP_MODE_16BPP | DC3_DCFG_16BPP;
        break;
    default:
        return CIM_STATUS_INVALIDPARAMS;
    }

    unlock = READ_REG32(DC3_UNLOCK);
    dcfg = READ_REG32(DC3_DISPLAY_CFG) & ~(DC3_DCFG_DISP_MODE_MASK |
                                           DC3_DCFG_16BPP_MODE_MASK);
    dcfg |= bpp_mask;

    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);
    WRITE_REG32(DC3_DISPLAY_CFG, dcfg);
    WRITE_REG32(DC3_UNLOCK, unlock);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vg_get_display_mode_index
 *
 * This routine searches the Cimarron mode table for a mode that matches the
 * input parameters.  If a match is found, the return value is the index into
 * the mode table.  If no match is found, the return value is -1.
 *--------------------------------------------------------------------------*/

int
vg_get_display_mode_index(VG_QUERY_MODE *query)
{
    unsigned int mode;
    unsigned long hz_flag = 0xFFFFFFFF;
    unsigned long bpp_flag = 0xFFFFFFFF;
    unsigned long enc_flag = 0xFFFFFFFF;
    unsigned long tv_flag = 0;
    unsigned long interlaced = 0;
    unsigned long halfclock = 0;
    long minimum = 0x7FFFFFFF;
    long diff;
    int match = -1;

    if (!query || !query->query_flags)
        return -1;

    if (query->query_flags & VG_QUERYFLAG_REFRESH) {
        /* SET FLAGS TO MATCH REFRESH RATE */

        if (query->hz == 56)
            hz_flag = VG_SUPPORTFLAG_56HZ;
        else if (query->hz == 60)
            hz_flag = VG_SUPPORTFLAG_60HZ;
        else if (query->hz == 70)
            hz_flag = VG_SUPPORTFLAG_70HZ;
        else if (query->hz == 72)
            hz_flag = VG_SUPPORTFLAG_72HZ;
        else if (query->hz == 75)
            hz_flag = VG_SUPPORTFLAG_75HZ;
        else if (query->hz == 85)
            hz_flag = VG_SUPPORTFLAG_85HZ;
        else if (query->hz == 90)
            hz_flag = VG_SUPPORTFLAG_90HZ;
        else if (query->hz == 100)
            hz_flag = VG_SUPPORTFLAG_100HZ;
        else
            hz_flag = 0;
    }

    if (query->query_flags & VG_QUERYFLAG_BPP) {
        /* SET BPP FLAGS TO LIMIT MODE SELECTION */

        if (query->bpp == 8)
            bpp_flag = VG_SUPPORTFLAG_8BPP;
        else if (query->bpp == 12)
            bpp_flag = VG_SUPPORTFLAG_12BPP;
        else if (query->bpp == 15)
            bpp_flag = VG_SUPPORTFLAG_15BPP;
        else if (query->bpp == 16)
            bpp_flag = VG_SUPPORTFLAG_16BPP;
        else if (query->bpp == 24)
            bpp_flag = VG_SUPPORTFLAG_24BPP;
        else if (query->bpp == 32)
            bpp_flag = VG_SUPPORTFLAG_32BPP;
        else
            bpp_flag = 0;
    }

    if (query->query_flags & VG_QUERYFLAG_ENCODER) {
        /* SET ENCODER FLAGS TO LIMIT MODE SELECTION */

        if (query->encoder == VG_ENCODER_ADV7171)
            enc_flag = VG_SUPPORTFLAG_ADV7171;
        else if (query->encoder == VG_ENCODER_SAA7127)
            enc_flag = VG_SUPPORTFLAG_SAA7127;
        else if (query->encoder == VG_ENCODER_FS454)
            enc_flag = VG_SUPPORTFLAG_FS454;
        else if (query->encoder == VG_ENCODER_ADV7300)
            enc_flag = VG_SUPPORTFLAG_ADV7300;
        else
            enc_flag = 0;
    }

    if (query->query_flags & VG_QUERYFLAG_TVMODE) {
        /* SET ENCODER FLAGS TO LIMIT MODE SELECTION */

        if (query->tvmode == VG_TVMODE_NTSC)
            tv_flag = VG_SUPPORTFLAG_NTSC;
        else if (query->tvmode == VG_TVMODE_PAL)
            tv_flag = VG_SUPPORTFLAG_PAL;
        else if (query->tvmode == VG_TVMODE_480P)
            tv_flag = VG_SUPPORTFLAG_480P;
        else if (query->tvmode == VG_TVMODE_720P)
            tv_flag = VG_SUPPORTFLAG_720P;
        else if (query->tvmode == VG_TVMODE_1080I)
            tv_flag = VG_SUPPORTFLAG_1080I;
        else if (query->tvmode == VG_TVMODE_6X4_NTSC)
            tv_flag = VG_SUPPORTFLAG_6X4_NTSC;
        else if (query->tvmode == VG_TVMODE_8X6_NTSC)
            tv_flag = VG_SUPPORTFLAG_8X6_NTSC;
        else if (query->tvmode == VG_TVMODE_10X7_NTSC)
            tv_flag = VG_SUPPORTFLAG_10X7_NTSC;
        else if (query->tvmode == VG_TVMODE_6X4_PAL)
            tv_flag = VG_SUPPORTFLAG_6X4_PAL;
        else if (query->tvmode == VG_TVMODE_8X6_PAL)
            tv_flag = VG_SUPPORTFLAG_8X6_PAL;
        else if (query->tvmode == VG_TVMODE_10X7_PAL)
            tv_flag = VG_SUPPORTFLAG_10X7_PAL;
        else
            tv_flag = 0xFFFFFFFF;
    }

    /* SET APPROPRIATE TV AND VOP FLAGS */

    if (query->query_flags & VG_QUERYFLAG_INTERLACED)
        interlaced = query->interlaced ? VG_MODEFLAG_INTERLACED : 0;
    if (query->query_flags & VG_QUERYFLAG_HALFCLOCK)
        halfclock = query->halfclock ? VG_MODEFLAG_HALFCLOCK : 0;

    /* CHECK FOR INVALID REQUEST */

    if (!hz_flag || !bpp_flag || !enc_flag || tv_flag == 0xFFFFFFFF)
        return -1;

    /* LOOP THROUGH THE AVAILABLE MODES TO FIND A MATCH */

    for (mode = 0; mode < NUM_CIMARRON_DISPLAY_MODES; mode++) {
        if ((!(query->query_flags & VG_QUERYFLAG_PANEL) ||
             (CimarronDisplayModes[mode].internal_flags & VG_SUPPORTFLAG_PANEL))
            && (!(query->query_flags & VG_QUERYFLAG_TVOUT)
                || (CimarronDisplayModes[mode].internal_flags &
                    VG_SUPPORTFLAG_TVOUT))
            && (!(query->query_flags & VG_QUERYFLAG_INTERLACED)
                || (CimarronDisplayModes[mode].flags & VG_MODEFLAG_INTERLACED)
                == interlaced)
            && (!(query->query_flags & VG_QUERYFLAG_HALFCLOCK)
                || (CimarronDisplayModes[mode].flags & VG_MODEFLAG_HALFCLOCK) ==
                halfclock)
            && (!(query->query_flags & VG_QUERYFLAG_PANELWIDTH)
                || (CimarronDisplayModes[mode].panel_width ==
                    query->panel_width))
            && (!(query->query_flags & VG_QUERYFLAG_PANELHEIGHT)
                || (CimarronDisplayModes[mode].panel_height ==
                    query->panel_height))
            && (!(query->query_flags & VG_QUERYFLAG_ACTIVEWIDTH)
                || (CimarronDisplayModes[mode].hactive == query->active_width))
            && (!(query->query_flags & VG_QUERYFLAG_ACTIVEHEIGHT)
                || (CimarronDisplayModes[mode].vactive == query->active_height))
            && (!(query->query_flags & VG_QUERYFLAG_TOTALWIDTH)
                || (CimarronDisplayModes[mode].htotal == query->total_width))
            && (!(query->query_flags & VG_QUERYFLAG_TOTALHEIGHT)
                || (CimarronDisplayModes[mode].vtotal == query->total_height))
            && (!(query->query_flags & VG_QUERYFLAG_BPP)
                || (CimarronDisplayModes[mode].internal_flags & bpp_flag))
            && (!(query->query_flags & VG_QUERYFLAG_REFRESH)
                || (CimarronDisplayModes[mode].internal_flags & hz_flag))
            && (!(query->query_flags & VG_QUERYFLAG_ENCODER)
                || (CimarronDisplayModes[mode].internal_flags & enc_flag))
            && (!(query->query_flags & VG_QUERYFLAG_TVMODE)
                ||
                ((CimarronDisplayModes[mode].internal_flags &
                  VG_SUPPORTFLAG_TVMODEMASK) == tv_flag))
            && (!(query->query_flags & VG_QUERYFLAG_PIXELCLOCK)
                || (CimarronDisplayModes[mode].frequency == query->frequency))) {
            /* ALLOW SEARCHING BASED ON AN APPROXIMATE PIXEL CLOCK */

            if (query->query_flags & VG_QUERYFLAG_PIXELCLOCK_APPROX) {
                diff = query->frequency - CimarronDisplayModes[mode].frequency;
                if (diff < 0)
                    diff = -diff;

                if (diff < minimum) {
                    minimum = diff;
                    match = mode;
                }
            }
            else {
                match = mode;
                break;
            }
        }
    }

    /* RETURN DISPLAY MODE INDEX */

    return match;
}

/*---------------------------------------------------------------------------
 * vg_get_display_mode_information
 *
 * This routine retrieves all information for a display mode contained
 * within Cimarron's mode tables.
 *--------------------------------------------------------------------------*/

int
vg_get_display_mode_information(unsigned int index, VG_DISPLAY_MODE *vg_mode)
{
    if (index > NUM_CIMARRON_DISPLAY_MODES)
        return CIM_STATUS_INVALIDPARAMS;

    *vg_mode = CimarronDisplayModes[index];
    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vg_get_display_mode_count
 *
 * This routine retrieves the count of all predefined Cimarron modes.
 *--------------------------------------------------------------------------*/

int
vg_get_display_mode_count(void)
{
    return NUM_CIMARRON_DISPLAY_MODES;
}

/*---------------------------------------------------------------------------
 * vg_get_current_display_mode
 *
 * This routine retrieves the settings for the current display.  This includes
 * any panel settings.
 *--------------------------------------------------------------------------*/

int
vg_get_current_display_mode(VG_DISPLAY_MODE *current_display, int *bpp)
{
    Q_WORD msr_value = { 0, 0 };
    unsigned long active, blank, sync;
    unsigned long i, m, n, p;
    unsigned long genlk, irq, temp;
    unsigned long flags = 0;
    unsigned long iflags = 0;

    /* READ THE CURRENT HORIZONTAL DISPLAY TIMINGS */

    active = READ_REG32(DC3_H_ACTIVE_TIMING);
    blank = READ_REG32(DC3_H_BLANK_TIMING);
    sync = READ_REG32(DC3_H_SYNC_TIMING);

    current_display->hactive = (active & 0xFFF) + 1;
    current_display->hblankstart = (blank & 0xFFF) + 1;
    current_display->hsyncstart = (sync & 0xFFF) + 1;

    current_display->htotal = ((active >> 16) & 0xFFF) + 1;
    current_display->hblankend = ((blank >> 16) & 0xFFF) + 1;
    current_display->hsyncend = ((sync >> 16) & 0xFFF) + 1;

    /* READ THE CURRENT VERTICAL DISPLAY TIMINGS */

    active = READ_REG32(DC3_V_ACTIVE_TIMING);
    blank = READ_REG32(DC3_V_BLANK_TIMING);
    sync = READ_REG32(DC3_V_SYNC_TIMING);

    current_display->vactive = (active & 0x7FF) + 1;
    current_display->vblankstart = (blank & 0x7FF) + 1;
    current_display->vsyncstart = (sync & 0x7FF) + 1;

    current_display->vtotal = ((active >> 16) & 0x7FF) + 1;
    current_display->vblankend = ((blank >> 16) & 0x7FF) + 1;
    current_display->vsyncend = ((sync >> 16) & 0x7FF) + 1;

    /* READ THE CURRENT EVEN FIELD VERTICAL DISPLAY TIMINGS */

    active = READ_REG32(DC3_V_ACTIVE_EVEN);
    blank = READ_REG32(DC3_V_BLANK_EVEN);
    sync = READ_REG32(DC3_V_SYNC_EVEN);

    current_display->vactive_even = (active & 0x7FF) + 1;
    current_display->vblankstart_even = (blank & 0x7FF) + 1;
    current_display->vsyncstart_even = (sync & 0x7FF) + 1;

    current_display->vtotal_even = ((active >> 16) & 0x7FF) + 1;
    current_display->vblankend_even = ((blank >> 16) & 0x7FF) + 1;
    current_display->vsyncend_even = ((sync >> 16) & 0x7FF) + 1;

    /* READ THE CURRENT SOURCE DIMENSIONS                      */
    /* The DC3_FB_ACTIVE register is only used when scaling is enabled.   */
    /* As the goal of this routine is to return a structure that can be   */
    /* passed to vg_set_custom_mode to exactly recreate the current mode, */
    /* we must check the status of the scaler/filter.                     */

    genlk = READ_REG32(DC3_GENLK_CTL);
    irq = READ_REG32(DC3_IRQ_FILT_CTL);
    temp = READ_REG32(DC3_FB_ACTIVE);

    current_display->src_height = (temp & 0xFFFF) + 1;
    current_display->src_width = ((temp >> 16) & 0xFFF8) + 8;

    /* READ THE CURRENT PANEL CONFIGURATION */
    /* We can only infer some of the panel settings based on hardware */
    /* (like when panning).  We will instead assume that the current  */
    /* mode was set using Cimarron and use the panel variables inside */
    /* Cimarron when returning the current mode information.          */

    if (vg3_panel_enable) {
        flags |= VG_MODEFLAG_PANELOUT;

        current_display->panel_width = vg3_panel_width;
        current_display->panel_height = vg3_panel_height;
        current_display->mode_width = vg3_mode_width;
        current_display->mode_height = vg3_mode_height;

        if (READ_REG32(DC3_DISPLAY_CFG) & DC3_DCFG_DCEN)
            flags |= VG_MODEFLAG_CENTERED;

        msr_read64(MSR_DEVICE_GEODELX_DF, DF_MSR_PAD_SEL, &msr_value);
        current_display->panel_tim1 = READ_VID32(DF_VIDEO_PANEL_TIM1);
        current_display->panel_tim2 = READ_VID32(DF_VIDEO_PANEL_TIM2);
        current_display->panel_dither_ctl = READ_VID32(DF_DITHER_CONTROL);
        current_display->panel_pad_sel_low = msr_value.low;
        current_display->panel_pad_sel_high = msr_value.high;
    }

    /* SET MISCELLANEOUS MODE FLAGS */

    /* INTERLACED */

    if (irq & DC3_IRQFILT_INTL_EN) {
        flags |= VG_MODEFLAG_INTERLACED;
        if (irq & DC3_IRQFILT_INTL_ADDR)
            flags |= VG_MODEFLAG_INT_ADDRESS;
        else if (genlk & DC3_GC_FLICKER_FILTER_ENABLE)
            flags |= VG_MODEFLAG_INT_FLICKER;
        else
            flags |= VG_MODEFLAG_INT_LINEDOUBLE;
    }

    /* POLARITIES */

    temp = READ_VID32(DF_DISPLAY_CONFIG);
    if (temp & DF_DCFG_CRT_HSYNC_POL)
        flags |= VG_MODEFLAG_NEG_HSYNC;
    if (temp & DF_DCFG_CRT_VSYNC_POL)
        flags |= VG_MODEFLAG_NEG_VSYNC;

    /* BPP */

    temp = READ_REG32(DC3_DISPLAY_CFG) & DC3_DCFG_DISP_MODE_MASK;
    if (temp == DC3_DCFG_DISP_MODE_8BPP) {
        iflags |= VG_SUPPORTFLAG_8BPP;
        *bpp = 8;
    }
    else if (temp == DC3_DCFG_DISP_MODE_24BPP) {
        iflags |= VG_SUPPORTFLAG_24BPP;
        *bpp = 24;
    }
    else if (temp == DC3_DCFG_DISP_MODE_32BPP) {
        iflags |= VG_SUPPORTFLAG_32BPP;
        *bpp = 32;
    }
    else if (temp == DC3_DCFG_DISP_MODE_16BPP) {
        temp = READ_REG32(DC3_DISPLAY_CFG) & DC3_DCFG_16BPP_MODE_MASK;
        if (temp == DC3_DCFG_16BPP) {
            iflags |= VG_SUPPORTFLAG_16BPP;
            *bpp = 16;
        }
        else if (temp == DC3_DCFG_15BPP) {
            iflags |= VG_SUPPORTFLAG_15BPP;
            *bpp = 15;
        }
        else if (temp == DC3_DCFG_12BPP) {
            iflags |= VG_SUPPORTFLAG_12BPP;
            *bpp = 12;
        }
    }

    /* TV RELATED FLAGS */

    msr_read64(MSR_DEVICE_GEODELX_DF, DF_MSR_PAD_SEL, &msr_value);
    if (msr_value.high & DF_INVERT_VOP_CLOCK)
        flags |= VG_MODEFLAG_TVOUT;

    /* LINEAR PITCH */

    temp = (READ_REG32(DC3_GFX_PITCH) & 0x0000FFFF) << 3;
    if (temp != 1024 && temp != 2048 && temp != 4096 && temp != 8192)
        flags |= VG_MODEFLAG_LINEARPITCH;

    /* SIMULTANEOUS CRT/FP */

    msr_read64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_CONFIG, &msr_value);
    if (msr_value.low & DF_SIMULTANEOUS_CRT_FP)
        flags |= VG_MODEFLAG_CRT_AND_FP;

    /* SET PLL-RELATED FLAGS */

    msr_read64(MSR_DEVICE_GEODELX_GLCP, GLCP_DOTPLL, &msr_value);
    if (msr_value.high & GLCP_DOTPLL_DIV4)
        flags |= VG_MODEFLAG_QVGA;
    if (msr_value.low & GLCP_DOTPLL_HALFPIX)
        flags |= VG_MODEFLAG_HALFCLOCK;

    /* SAVE THE FLAGS IN THE MODE STRUCTURE */

    current_display->internal_flags = iflags;
    current_display->flags = flags;

    /* READ PIXEL CLOCK FREQUENCY */
    /* We first search for an exact match.  If none is found, we try */
    /* a fixed point calculation and return CIM_STATUS_INEXACTMATCH. */

    for (i = 0; i < NUM_CIMARRON_PLL_FREQUENCIES; i++) {
        if (CimarronPLLFrequencies[i].pll_value == msr_value.high)
            break;
    }

    if (i == NUM_CIMARRON_PLL_FREQUENCIES) {
        /* ATTEMPT 16.16 CALCULATION */
        /* We assume the input frequency is 48 MHz, which is represented   */
        /* in 16.16 fixed point as 0x300000. The PLL calculation is:       */
        /*                             n + 1                               */
        /*   Fout =  48.000   *    --------------                          */
        /*                         m + 1   *  p + 1                        */

        p = msr_value.high & 0xF;
        n = (msr_value.high >> 4) & 0xFF;
        m = (msr_value.high >> 12) & 0x7;
        current_display->frequency = (0x300000 * (n + 1)) / ((p + 1) * (m + 1));

        return CIM_STATUS_INEXACTMATCH;
    }

    current_display->frequency = CimarronPLLFrequencies[i].frequency;

    /* NOW SEARCH FOR AN IDENTICAL MODE */
    /* This is just to inform the user that an exact match was found.   */
    /* With an exact match, the user can use the refresh rate flag that */
    /* is returned in the VG_DISPLAY_MODE structure.                    */

    for (i = 0; i < NUM_CIMARRON_DISPLAY_MODES; i++) {
        if ((CimarronDisplayModes[i].flags & current_display->flags) &&
            CimarronDisplayModes[i].frequency ==
            current_display->frequency &&
            CimarronDisplayModes[i].hactive == current_display->hactive &&
            CimarronDisplayModes[i].hblankstart ==
            current_display->hblankstart
            && CimarronDisplayModes[i].hsyncstart ==
            current_display->hsyncstart
            && CimarronDisplayModes[i].hsyncend ==
            current_display->hsyncend
            && CimarronDisplayModes[i].hblankend ==
            current_display->hblankend
            && CimarronDisplayModes[i].htotal == current_display->htotal
            && CimarronDisplayModes[i].vactive == current_display->vactive
            && CimarronDisplayModes[i].vblankstart ==
            current_display->vblankstart
            && CimarronDisplayModes[i].vsyncstart ==
            current_display->vsyncstart
            && CimarronDisplayModes[i].vsyncend ==
            current_display->vsyncend
            && CimarronDisplayModes[i].vblankend ==
            current_display->vblankend
            && CimarronDisplayModes[i].vtotal == current_display->vtotal) {
            break;
        }
    }

    if (i == NUM_CIMARRON_DISPLAY_MODES)
        return CIM_STATUS_INEXACTMATCH;

    current_display->internal_flags |=
        (CimarronDisplayModes[i].internal_flags & VG_SUPPORTFLAG_HZMASK);
    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vg_set_scaler_filter_coefficients
 *
 * This routine sets the vertical and horizontal filter coefficients for
 * graphics scaling.  If either of the input arrays is specified as NULL, a
 * set of default coeffecients will be used.
 *--------------------------------------------------------------------------*/

int
vg_set_scaler_filter_coefficients(long h_taps[][5], long v_taps[][3])
{
    unsigned long irqfilt, i;
    unsigned long temp0, temp1;
    unsigned long lock;

    /* ENABLE ACCESS TO THE HORIZONTAL COEFFICIENTS */

    irqfilt = READ_REG32(DC3_IRQ_FILT_CTL);
    irqfilt |= DC3_IRQFILT_H_FILT_SEL;

    /* UNLOCK THE COEFFICIENT REGISTERS */

    lock = READ_REG32(DC3_UNLOCK);
    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);

    /* WRITE COEFFICIENTS */
    /* Coefficient indexes do not auto-increment, so we must */
    /* write the address for every phase                     */

    for (i = 0; i < 256; i++) {
        WRITE_REG32(DC3_IRQ_FILT_CTL, ((irqfilt & 0xFFFFFF00L) | i));

        if (!h_taps) {
            temp0 = CimarronHorizontalGraphicsFilter[i][0];
            temp1 = CimarronHorizontalGraphicsFilter[i][1];
        }
        else {
            temp0 = ((unsigned long) h_taps[i][0] & 0x3FF) |
                (((unsigned long) h_taps[i][1] & 0x3FF) << 10) |
                (((unsigned long) h_taps[i][2] & 0x3FF) << 20);

            temp1 = ((unsigned long) h_taps[i][3] & 0x3FF) |
                (((unsigned long) h_taps[i][4] & 0x3FF) << 10);
        }
        WRITE_REG32(DC3_FILT_COEFF1, temp0);
        WRITE_REG32(DC3_FILT_COEFF2, temp1);
    }

    /* ENABLE ACCESS TO THE VERTICAL COEFFICIENTS */

    irqfilt &= ~DC3_IRQFILT_H_FILT_SEL;

    /* WRITE COEFFICIENTS */

    for (i = 0; i < 256; i++) {
        WRITE_REG32(DC3_IRQ_FILT_CTL, ((irqfilt & 0xFFFFFF00L) | i));

        if (!v_taps) {
            temp0 = CimarronVerticalGraphicsFilter[i];
        }
        else {
            temp0 = ((unsigned long) v_taps[i][0] & 0x3FF) |
                (((unsigned long) v_taps[i][1] & 0x3FF) << 10) |
                (((unsigned long) v_taps[i][2] & 0x3FF) << 20);
        }

        WRITE_REG32(DC3_FILT_COEFF1, temp0);
    }

    WRITE_REG32(DC3_UNLOCK, lock);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vg_configure_flicker_filter
 *
 * This routine updates the VG flicker filter settings when in an interlaced
 * mode.  Note that flicker filtering is enabled inside a mode set.  This routine
 * is provided to change from the default flicker filter setting of
 * 1/4, 1/2, 1/4.
 *--------------------------------------------------------------------------*/

int
vg_configure_flicker_filter(unsigned long flicker_strength, int flicker_alpha)
{
    unsigned long unlock;
    unsigned long genlk_ctl;

    /* CHECK FOR VALID FLICKER SETTING */

    if (flicker_strength != VG_FLICKER_FILTER_NONE &&
        flicker_strength != VG_FLICKER_FILTER_1_16 &&
        flicker_strength != VG_FLICKER_FILTER_1_8 &&
        flicker_strength != VG_FLICKER_FILTER_1_4 &&
        flicker_strength != VG_FLICKER_FILTER_5_16) {
        return CIM_STATUS_INVALIDPARAMS;
    }

    unlock = READ_REG32(DC3_UNLOCK);
    genlk_ctl = READ_REG32(DC3_GENLK_CTL) & ~(DC3_GC_FLICKER_FILTER_MASK |
                                              DC3_GC_ALPHA_FLICK_ENABLE);
    genlk_ctl |= flicker_strength;
    if (flicker_alpha)
        genlk_ctl |= DC3_GC_ALPHA_FLICK_ENABLE;

    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);
    WRITE_REG32(DC3_GENLK_CTL, genlk_ctl);
    WRITE_REG32(DC3_UNLOCK, unlock);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vg_set_clock_frequency
 *
 * This routine sets the frequency of the dot clock.  The input to this
 * routine is a 16.16 fraction.  If an exact match is not found, this
 * routine will program the closest available frequency and return
 * CIM_STATUS_INEXACTMATCH.
 *--------------------------------------------------------------------------*/

int
vg_set_clock_frequency(unsigned long frequency, unsigned long pll_flags)
{
    Q_WORD msr_value = { 0, 0 };
    unsigned long timeout;
    unsigned long index = 0;
    unsigned long unlock, i;
    unsigned long pll_high, pll_low;
    long diff, min = 0;

    /* FIND THE REGISTER VALUES FOR THE DESIRED FREQUENCY         */
    /* Search the table for the closest frequency (16.16 format). */
    /* This search is skipped if the user is manually specifying  */
    /* the MSR value.                                             */

    pll_low = 0;
    if (!(pll_flags & VG_PLL_MANUAL)) {
        min = (long) CimarronPLLFrequencies[0].frequency - (long) frequency;
        if (min < 0L)
            min = -min;

        for (i = 1; i < NUM_CIMARRON_PLL_FREQUENCIES; i++) {
            diff = (long) CimarronPLLFrequencies[i].frequency -
                (long) frequency;
            if (diff < 0L)
                diff = -diff;

            if (diff < min) {
                min = diff;
                index = i;
            }
        }

        pll_high = CimarronPLLFrequencies[index].pll_value & 0x00007FFF;
    }
    else {
        pll_high = frequency;
    }

    if (pll_flags & VG_PLL_DIVIDE_BY_2)
        pll_low |= GLCP_DOTPLL_HALFPIX;
    if (pll_flags & VG_PLL_DIVIDE_BY_4)
        pll_high |= GLCP_DOTPLL_DIV4;
    if (pll_flags & VG_PLL_BYPASS)
        pll_low |= GLCP_DOTPLL_BYPASS;
    if (pll_flags & VG_PLL_VIP_CLOCK)
        pll_high |= GLCP_DOTPLL_VIPCLK;

    /* VERIFY THAT WE ARE NOT WRITING WHAT IS ALREADY IN THE REGISTERS */
    /* The Dot PLL reset bit is tied to VDD for flat panels.  This can */
    /* cause a brief drop in flat panel power, which can cause serious */
    /* glitches on some panels.                                        */

    msr_read64(MSR_DEVICE_GEODELX_GLCP, GLCP_DOTPLL, &msr_value);

    if ((msr_value.low & GLCP_DOTPLL_LOCK) &&
        ((msr_value.low & (GLCP_DOTPLL_HALFPIX | GLCP_DOTPLL_BYPASS)) ==
         pll_low) && (msr_value.high == pll_high)) {
        return CIM_STATUS_OK;
    }

    /* PROGRAM THE SETTINGS WITH THE RESET BIT SET */
    /* Clear the bypass bit to ensure that the programmed */
    /* M, N and P values are being used.                  */

    msr_value.high = pll_high;
    msr_value.low &= ~(GLCP_DOTPLL_BYPASS | GLCP_DOTPLL_HALFPIX);
    msr_value.low |= (pll_low | 0x00000001);
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_DOTPLL, &msr_value);

    /* WAIT FOR THE LOCK BIT */
    /* The PLL spec states that the PLL may take up to 100 us to */
    /* properly lock.  Furthermore, the lock signal is not 100%  */
    /* reliable.  To address this, we add a hefty delay followed */
    /* by a polling loop that times out after a 1000 reads.      */

    unlock = READ_REG32(DC3_UNLOCK);
    for (timeout = 0; timeout < 1280; timeout++)
        WRITE_REG32(DC3_UNLOCK, unlock);

    for (timeout = 0; timeout < 1000; timeout++) {
        msr_read64(MSR_DEVICE_GEODELX_GLCP, GLCP_DOTPLL, &msr_value);
        if (msr_value.low & GLCP_DOTPLL_LOCK)
            break;
    }

    /* CLEAR THE RESET BIT */

    msr_value.low &= 0xFFFFFFFE;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_DOTPLL, &msr_value);

    /* DID THE PLL SUCCESSFULLY LOCK? */

    if (!(msr_value.low & GLCP_DOTPLL_LOCK))
        return CIM_STATUS_NOLOCK;

    /* RETURN THE APPROPRIATE CODE */

    if (min == 0)
        return CIM_STATUS_OK;
    else
        return CIM_STATUS_INEXACTMATCH;
}

/*---------------------------------------------------------------------------
 * vg_set_border_color
 *
 * This routine sets the color used as the border in centered panel modes.
 *--------------------------------------------------------------------------*/

int
vg_set_border_color(unsigned long border_color)
{
    unsigned long lock = READ_REG32(DC3_UNLOCK);

    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);
    WRITE_REG32(DC3_PAL_ADDRESS, 0x104);
    WRITE_REG32(DC3_PAL_DATA, border_color);
    WRITE_REG32(DC3_UNLOCK, lock);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vg_set_cursor_enable
 *
 * This routine enables or disables the hardware cursor.  This routine should
 * only be called after the hardware cursor has been completely configured.
 *--------------------------------------------------------------------------*/

int
vg_set_cursor_enable(int enable)
{
    unsigned long unlock, gcfg;

    /* SET OR CLEAR CURSOR ENABLE BIT */

    unlock = READ_REG32(DC3_UNLOCK);
    gcfg = READ_REG32(DC3_GENERAL_CFG);
    if (enable)
        gcfg |= DC3_GCFG_CURE;
    else
        gcfg &= ~(DC3_GCFG_CURE);

    /* WRITE NEW REGISTER VALUE */

    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);
    WRITE_REG32(DC3_GENERAL_CFG, gcfg);
    WRITE_REG32(DC3_UNLOCK, unlock);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vg_set_mono_cursor_colors
 *
 * This routine sets the colors of the hardware monochrome cursor.
 *--------------------------------------------------------------------------*/

int
vg_set_mono_cursor_colors(unsigned long bkcolor, unsigned long fgcolor)
{
    unsigned long lock = READ_REG32(DC3_UNLOCK);

    /* SET CURSOR COLORS */

    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);
    WRITE_REG32(DC3_PAL_ADDRESS, 0x100);
    WRITE_REG32(DC3_PAL_DATA, bkcolor);
    WRITE_REG32(DC3_PAL_DATA, fgcolor);
    WRITE_REG32(DC3_UNLOCK, lock);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vg_set_cursor_position
 *
 * This routine sets the position of the hardware cursor.  The cursor hotspots
 * and memory offset must have been specified in an earlier call to
 * a vg_set_cursor_shape_XX routine.  The coordinates passed to this routine
 * generally specify the focal point of the cursor, NOT the upper left
 * coordinate of the cursor pattern.  However, for operating systems that do
 * not include a hotspot the input parameters may be negative.
 *--------------------------------------------------------------------------*/

int
vg_set_cursor_position(long xpos, long ypos, VG_PANNING_COORDINATES *panning)
{
    unsigned long unlock, memoffset;
    unsigned long gcfg;
    long x, xoffset;
    long y, yoffset;

    memoffset = vg3_cursor_offset;
    x = xpos - (long) vg3_x_hotspot;
    y = ypos - (long) vg3_y_hotspot;

    /* HANDLE NEGATIVE COORDINATES                                      */
    /* This routine supports operating systems that use negative        */
    /* coordinates, instead of positive coordinates with an appropriate */
    /* hotspot.                                                         */

    if (xpos < 0)
        xpos = 0;
    if (ypos < 0)
        ypos = 0;

    if (x < -63)
        return CIM_STATUS_INVALIDPARAMS;
    if (y < -63)
        return CIM_STATUS_INVALIDPARAMS;

    if (vg3_panel_enable) {
        if ((vg3_mode_width > vg3_panel_width)
            || (vg3_mode_height > vg3_panel_height)) {
            vg_pan_desktop(xpos, ypos, panning);
            x = x - (unsigned short) vg3_delta_x;
            y = y - (unsigned short) vg3_delta_y;
        }
        else {
            panning->start_x = 0;
            panning->start_y = 0;
            panning->start_updated = 0;
        }
    }

    /* ADJUST OFFSETS */
    /* Cursor movement and panning work as follows:  The cursor position   */
    /* refers to where the hotspot of the cursor is located.  However, for */
    /* non-zero hotspots, the cursor buffer actually begins before the     */
    /* specified position.                                                 */

    if (x < 0) {
        xoffset = -x;
        x = 0;
    }
    else {
        xoffset = 0;
    }
    if (y < 0) {
        yoffset = -y;
        y = 0;
    }
    else {
        yoffset = 0;
    }

    if (vg3_color_cursor)
        memoffset += (unsigned long) yoffset *192;

    else
        memoffset += (unsigned long) yoffset << 4;

    /* SET COLOR CURSOR BIT */

    gcfg = READ_REG32(DC3_GENERAL_CFG);
    if (vg3_color_cursor)
        gcfg |= DC3_GCFG_CLR_CUR;
    else
        gcfg &= ~DC3_GCFG_CLR_CUR;

    /* SET CURSOR POSITION */

    unlock = READ_REG32(DC3_UNLOCK);
    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);
    WRITE_REG32(DC3_CURS_ST_OFFSET, memoffset);
    WRITE_REG32(DC3_GENERAL_CFG, gcfg);
    WRITE_REG32(DC3_CURSOR_X, (unsigned long) x |
                (((unsigned long) xoffset) << 11));
    WRITE_REG32(DC3_CURSOR_Y, (unsigned long) y |
                (((unsigned long) yoffset) << 11));
    WRITE_REG32(DC3_UNLOCK, unlock);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vg_set_mono_cursor_shape32
 *
 * This routine loads 32x32 cursor data into the cursor buffer in graphics
 * memory.  The outside of the GeodeLX cursor buffer is padded with
 * transparency.
 *--------------------------------------------------------------------------*/

int
vg_set_mono_cursor_shape32(unsigned long memoffset, unsigned long *andmask,
                           unsigned long *xormask, unsigned long x_hotspot,
                           unsigned long y_hotspot)
{
    int i;

    /* SAVE THE CURSOR OFFSET AND HOTSPOTS                               */
    /* These are reused later when updating the cursor position, panning */
    /* and clipping the cursor pointer.                                  */

    vg3_x_hotspot = x_hotspot;
    vg3_y_hotspot = y_hotspot;
    vg3_cursor_offset = memoffset;
    vg3_color_cursor = 0;

    for (i = 0; i < 32; i++) {
        /* EVEN QWORDS CONTAIN THE AND MASK */

        WRITE_FB32(memoffset, 0xFFFFFFFF);
        WRITE_FB32(memoffset + 4, andmask[i]);

        /* ODD QWORDS CONTAIN THE XOR MASK  */

        WRITE_FB32(memoffset + 8, 0x00000000);
        WRITE_FB32(memoffset + 12, xormask[i]);

        memoffset += 16;
    }

    /* FILL THE LOWER HALF OF THE BUFFER WITH TRANSPARENT PIXELS */

    for (i = 0; i < 32; i++) {
        WRITE_FB32(memoffset, 0xFFFFFFFF);
        WRITE_FB32(memoffset + 4, 0xFFFFFFFF);
        WRITE_FB32(memoffset + 8, 0x00000000);
        WRITE_FB32(memoffset + 12, 0x00000000);

        memoffset += 16;
    }

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vg_set_mono_cursor_shape64
 *
 * This routine loads 64x64 cursor data into the cursor buffer in graphics
 * memory.
 *--------------------------------------------------------------------------*/

int
vg_set_mono_cursor_shape64(unsigned long memoffset, unsigned long *andmask,
                           unsigned long *xormask, unsigned long x_hotspot,
                           unsigned long y_hotspot)
{
    int i;

    /* SAVE THE CURSOR OFFSET AND HOTSPOTS                               */
    /* These are reused later when updating the cursor position, panning */
    /* and clipping the cursor pointer.                                  */

    vg3_x_hotspot = x_hotspot;
    vg3_y_hotspot = y_hotspot;
    vg3_cursor_offset = memoffset;
    vg3_color_cursor = 0;

    for (i = 0; i < 128; i += 2) {
        /* EVEN QWORDS CONTAIN THE AND MASK */
        /* We invert the dwords to prevent the calling            */
        /* application from having to think in terms of Qwords.   */
        /* The hardware data order is actually 63:0, or 31:0 of   */
        /* the second dword followed by 31:0 of the first dword.  */

        WRITE_FB32(memoffset, andmask[i + 1]);
        WRITE_FB32(memoffset + 4, andmask[i]);

        /* ODD QWORDS CONTAIN THE XOR MASK  */

        WRITE_FB32(memoffset + 8, xormask[i + 1]);
        WRITE_FB32(memoffset + 12, xormask[i]);

        memoffset += 16;
    }

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vg_set_color_cursor_shape
 *
 * This routine loads 8:8:8:8 cursor data into the color cursor buffer.
 *--------------------------------------------------------------------------*/

int
vg_set_color_cursor_shape(unsigned long memoffset, unsigned char *data,
                          unsigned long width, unsigned long height, long pitch,
                          unsigned long x_hotspot, unsigned long y_hotspot)
{
    unsigned long y;

    /* SAVE THE CURSOR OFFSET AND HOTSPOTS                               */
    /* These are reused later when updating the cursor position, panning */
    /* and clipping the cursor pointer.                                  */

    vg3_x_hotspot = x_hotspot;
    vg3_y_hotspot = y_hotspot;
    vg3_cursor_offset = memoffset;
    vg3_color_cursor = 1;

    /* WRITE THE CURSOR DATA */
    /* The outside edges of the color cursor are filled with transparency */
    /* The cursor buffer dimensions are 48x64.                            */

    for (y = 0; y < height; y++) {
        /* WRITE THE ACTIVE AND TRANSPARENT DATA */
        /* We implement this as a macro in our dedication to squeaking */
        /* every ounce of performance out of our code...               */

        WRITE_FB_STRING32(memoffset, data, width);
        WRITE_FB_CONSTANT((memoffset + (width << 2)), 0, (48 - width));

        /* INCREMENT PAST THE LINE */

        memoffset += 192;
        data += pitch;
    }

    /* WRITE THE EXTRA TRANSPARENT LINES */
    /* Write the lines in one big bulk setting. */

    WRITE_FB_CONSTANT(memoffset, 0, ((64 - height) * 48));

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vg_pan_desktop
 *
 * This routine sets the correct display offset based on the current cursor
 * position.
 *--------------------------------------------------------------------------*/

int
vg_pan_desktop(unsigned long x, unsigned long y,
               VG_PANNING_COORDINATES *panning)
{
    unsigned long modeShiftPerPixel;
    unsigned long modeBytesPerScanline;
    unsigned long startAddress;

    /* TEST FOR NO-WORK */

    if (x >= vg3_delta_x && x < (vg3_panel_width + vg3_delta_x) &&
        y >= vg3_delta_y && y < (vg3_panel_height + vg3_delta_y)) {
        panning->start_x = vg3_delta_x;
        panning->start_y = vg3_delta_y;
        panning->start_updated = 0;
        return CIM_STATUS_OK;
    }

    if (vg3_bpp == 24)
        modeShiftPerPixel = 2;
    else
        modeShiftPerPixel = (vg3_bpp + 7) >> 4;

    modeBytesPerScanline = (READ_REG32(DC3_GFX_PITCH) & 0x0000FFFF) << 3;

    /* ADJUST PANNING VARIABLES WHEN CURSOR EXCEEDS BOUNDARY       */
    /* Test the boundary conditions for each coordinate and update */
    /* all variables and the starting offset accordingly.          */

    if (x < vg3_delta_x)
        vg3_delta_x = x;
    else if (x >= (vg3_delta_x + vg3_panel_width))
        vg3_delta_x = x - vg3_panel_width + 1;

    if (y < vg3_delta_y)
        vg3_delta_y = y;
    else if (y >= (vg3_delta_y + vg3_panel_height))
        vg3_delta_y = y - vg3_panel_height + 1;

    /* CALCULATE THE START OFFSET */

    startAddress = (vg3_delta_x << modeShiftPerPixel) +
        (vg3_delta_y * modeBytesPerScanline);

    vg_set_display_offset(startAddress);

    panning->start_updated = 1;
    panning->start_x = vg3_delta_x;
    panning->start_y = vg3_delta_y;
    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vg_set_display_offset
 *
 * This routine sets the start address of the frame buffer.  It is
 * typically used to pan across a virtual desktop (frame buffer larger than
 * the displayed screen) or to flip the display between multiple buffers.
 *--------------------------------------------------------------------------*/

int
vg_set_display_offset(unsigned long address)
{
    unsigned long lock, gcfg;

    lock = READ_REG32(DC3_UNLOCK);
    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);

    /* DISABLE COMPRESSION */
    /* When setting a non-zero display offset, we must disable display  */
    /* compression.  We could maintain a variable and re-enable         */
    /* compression when the offset returns to zero.  However, that      */
    /* creates additional complexity for applications that perform      */
    /* graphics animation.  Re-enabling compression each time would     */
    /* be tedious and slow for such applications, implying that they    */
    /* would have to disable compression before starting the animation. */
    /* We will instead disable compression and force the user to        */
    /* re-enable compression when they are ready.                       */

    if (address != 0) {
        if (READ_REG32(DC3_GENERAL_CFG) & DC3_GCFG_CMPE) {
            gcfg = READ_REG32(DC3_GENERAL_CFG);
            WRITE_REG32(DC3_GENERAL_CFG,
                        (gcfg & ~(DC3_GCFG_CMPE | DC3_GCFG_DECE)));
        }
    }

    WRITE_REG32(DC3_FB_ST_OFFSET, address);
    WRITE_REG32(DC3_UNLOCK, lock);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vg_set_display_pitch
 *
 * This routine sets the stride between successive lines of data in the frame
 * buffer.
 *--------------------------------------------------------------------------*/

int
vg_set_display_pitch(unsigned long pitch)
{
    unsigned long temp, dvsize, dvtop, value;
    unsigned long lock = READ_REG32(DC3_UNLOCK);

    value = READ_REG32(DC3_GFX_PITCH) & 0xFFFF0000;
    value |= (pitch >> 3);

    /* PROGRAM THE DISPLAY PITCH */

    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);
    WRITE_REG32(DC3_GFX_PITCH, value);

    /* SET THE COMPRESSION BEHAVIOR BASED ON THE PITCH              */
    /* Strides that are not a power of two will not work with line  */
    /* by line compression.  For these cases, we enable full-screen */
    /* compression.  In this mode, any write to the frame buffer    */
    /* region marks the entire frame as dirty.   Also, the DV line  */
    /* size must be updated when the pitch is programmed outside of */
    /* the power of 2 range specified in a mode set.                */

    if (pitch > 4096) {
        dvsize = DC3_DV_LINE_SIZE_8192;
    }
    else if (pitch > 2048) {
        dvsize = DC3_DV_LINE_SIZE_4096;
    }
    else if (pitch > 1024) {
        dvsize = DC3_DV_LINE_SIZE_2048;
    }
    else {
        dvsize = DC3_DV_LINE_SIZE_1024;
    }

    temp = READ_REG32(DC3_DV_CTL);
    WRITE_REG32(DC3_DV_CTL,
                (temp & ~DC3_DV_LINE_SIZE_MASK) | dvsize | 0x00000001);

    value = READ_REG32(DC3_GENERAL_CFG);

    if (pitch == 1024 || pitch == 2048 || pitch == 4096 || pitch == 8192) {
        value &= ~DC3_GCFG_FDTY;
        dvtop = 0;
    }
    else {
        value |= DC3_GCFG_FDTY;

        dvtop = (READ_REG32(DC3_FB_ACTIVE) & 0xFFF) + 1;
        dvtop = ((dvtop * pitch) + 0x3FF) & 0xFFFFFC00;
        dvtop |= DC3_DVTOP_ENABLE;
    }

    WRITE_REG32(DC3_GENERAL_CFG, value);
    WRITE_REG32(DC3_DV_TOP, dvtop);
    WRITE_REG32(DC3_UNLOCK, lock);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vg_set_display_palette_entry
 *
 * This routine sets a single 8BPP palette entry in the display controller.
 *--------------------------------------------------------------------------*/

int
vg_set_display_palette_entry(unsigned long index, unsigned long palette)
{
    unsigned long dcfg, unlock;

    if (index > 0xFF)
        return CIM_STATUS_INVALIDPARAMS;

    unlock = READ_REG32(DC3_UNLOCK);
    dcfg = READ_REG32(DC3_DISPLAY_CFG);

    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);
    WRITE_REG32(DC3_DISPLAY_CFG, dcfg & ~DC3_DCFG_PALB);
    WRITE_REG32(DC3_UNLOCK, unlock);

    WRITE_REG32(DC3_PAL_ADDRESS, index);
    WRITE_REG32(DC3_PAL_DATA, palette);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vg_set_display_palette
 *
 * This routine sets the entire palette in the display controller.
 * A pointer is provided to a 256 entry table of 32-bit X:R:G:B values.
 *--------------------------------------------------------------------------*/

int
vg_set_display_palette(unsigned long *palette)
{
    unsigned long unlock, dcfg, i;

    WRITE_REG32(DC3_PAL_ADDRESS, 0);

    if (palette) {
        unlock = READ_REG32(DC3_UNLOCK);
        dcfg = READ_REG32(DC3_DISPLAY_CFG);

        WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);
        WRITE_REG32(DC3_DISPLAY_CFG, dcfg & ~DC3_DCFG_PALB);
        WRITE_REG32(DC3_UNLOCK, unlock);

        for (i = 0; i < 256; i++)
            WRITE_REG32(DC3_PAL_DATA, palette[i]);

        return CIM_STATUS_OK;
    }
    return CIM_STATUS_INVALIDPARAMS;
}

/*---------------------------------------------------------------------------
 * vg_set_compression_enable
 *
 * This routine enables or disables display compression.
 *--------------------------------------------------------------------------*/

int
vg_set_compression_enable(int enable)
{
    Q_WORD msr_value = { 0, 0 };
    unsigned long unlock, gcfg;
    unsigned long temp;

    unlock = READ_REG32(DC3_UNLOCK);
    gcfg = READ_REG32(DC3_GENERAL_CFG);
    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);

    if (enable) {
        /* DO NOT ENABLE IF THE DISPLAY OFFSET IS NOT ZERO */

        if (READ_REG32(DC3_FB_ST_OFFSET) & 0x0FFFFFFF)
            return CIM_STATUS_ERROR;

        /* ENABLE BIT 1 IN THE VG SPARE MSR
         * The bus can hang when the VG attempts to merge compression writes.
         * No performance is lost due to the GeodeLink QUACK features in
         * GeodeLX.  We also enable the command word check for a valid
         * compression header.
         */

        msr_read64(MSR_DEVICE_GEODELX_VG, DC3_SPARE_MSR, &msr_value);
        msr_value.low |= DC3_SPARE_FIRST_REQ_MASK;
        msr_value.low &= ~DC3_SPARE_DISABLE_CWD_CHECK;
        msr_write64(MSR_DEVICE_GEODELX_VG, DC3_SPARE_MSR, &msr_value);

        /* CLEAR DIRTY/VALID BITS IN MEMORY CONTROLLER
         * We don't want the controller to think that old lines are still
         * valid.  Writing a 1 to bit 0 of the DV Control register will force
         * the hardware to clear all the valid bits.
         */

        temp = READ_REG32(DC3_DV_CTL);
        WRITE_REG32(DC3_DV_CTL, temp | 0x00000001);

        /* ENABLE COMPRESSION BITS */

        gcfg |= DC3_GCFG_CMPE | DC3_GCFG_DECE;
    }
    else {
        gcfg &= ~(DC3_GCFG_CMPE | DC3_GCFG_DECE);
    }

    WRITE_REG32(DC3_GENERAL_CFG, gcfg);
    WRITE_REG32(DC3_UNLOCK, unlock);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vg_configure_compression
 *
 * This routine configures all aspects of display compression, including
 * pitch, size and the offset of the compression buffer.
 *--------------------------------------------------------------------------*/

int
vg_configure_compression(VG_COMPRESSION_DATA *comp_data)
{
    unsigned long delta, size;
    unsigned long comp_size, unlock;

    /* CHECK FOR VALID PARAMETERS */
    /* The maximum size for the compression buffer is 544 bytes (with    */
    /* the header)  Also, the pitch cannot be less than the line size    */
    /* and the compression buffer offset must be 16-byte aligned.        */

    if (comp_data->size > 544 || comp_data->pitch < comp_data->size ||
        comp_data->compression_offset & 0x0F) {
        return CIM_STATUS_INVALIDPARAMS;
    }

    /* SUBTRACT 32 FROM SIZE                                           */
    /* The display controller will actually write 4 extra QWords.  So, */
    /* if we assume that "size" refers to the allocated size, we must  */
    /* subtract 32 bytes.                                              */

    comp_size = comp_data->size - 32;

    /* CALCULATE REGISTER VALUES */

    unlock = READ_REG32(DC3_UNLOCK);
    size = READ_REG32(DC3_LINE_SIZE) & ~DC3_LINE_SIZE_CBLS_MASK;
    delta = READ_REG32(DC3_GFX_PITCH) & ~DC3_GFX_PITCH_CBP_MASK;

    size |= ((comp_size >> 3) + 1) << DC3_LINE_SIZE_CB_SHIFT;
    delta |= ((comp_data->pitch >> 3) << 16);

    /* WRITE COMPRESSION PARAMETERS */

    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);
    WRITE_REG32(DC3_CB_ST_OFFSET, comp_data->compression_offset);
    WRITE_REG32(DC3_LINE_SIZE, size);
    WRITE_REG32(DC3_GFX_PITCH, delta);
    WRITE_REG32(DC3_UNLOCK, unlock);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vg_test_timing_active
 *
 * This routine checks the status of the display timing generator.
 *--------------------------------------------------------------------------*/

int
vg_test_timing_active(void)
{
    if (READ_REG32(DC3_DISPLAY_CFG) & DC3_DCFG_TGEN)
        return 1;

    return 0;
}

/*---------------------------------------------------------------------------
 * vg_test_vertical_active
 *
 * This routine checks if the display is currently in the middle of a frame
 * (not in the VBlank interval)
 *--------------------------------------------------------------------------*/

int
vg_test_vertical_active(void)
{
    if (READ_REG32(DC3_LINE_CNT_STATUS) & DC3_LNCNT_VNA)
        return 0;

    return 1;
}

/*---------------------------------------------------------------------------
 * vg_wait_vertical_blank
 *
 * This routine waits until the beginning of the vertical blank interval.
 * When the display is already in vertical blank, this routine will wait until
 * the beginning of the next vertical blank.
 *--------------------------------------------------------------------------*/

int
vg_wait_vertical_blank(void)
{
    if (vg_test_timing_active()) {
        while (!vg_test_vertical_active());
        while (vg_test_vertical_active());
    }
    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vg_test_even_field
 *
 * This routine tests the odd/even status of the current VG output field.
 *--------------------------------------------------------------------------*/

int
vg_test_even_field(void)
{
    if (READ_REG32(DC3_LINE_CNT_STATUS) & DC3_LNCNT_EVEN_FIELD)
        return 1;

    return 0;
}

/*---------------------------------------------------------------------------
 * vg_configure_line_interrupt
 *
 * This routine configures the display controller's line count interrupt.
 * This interrupt can be used to interrupt mid-frame or to interrupt at the
 * beginning of vertical blank.
 *--------------------------------------------------------------------------*/

int
vg_configure_line_interrupt(VG_INTERRUPT_PARAMS *interrupt_info)
{
    unsigned long irq_line, irq_enable;
    unsigned long lock;

    irq_line = READ_REG32(DC3_IRQ_FILT_CTL);
    irq_enable = READ_REG32(DC3_IRQ);
    lock = READ_REG32(DC3_UNLOCK);

    irq_line = (irq_line & ~DC3_IRQFILT_LINE_MASK) |
        ((interrupt_info->line << 16) & DC3_IRQFILT_LINE_MASK);

    /* ENABLE OR DISABLE THE INTERRUPT */
    /* The line count is set before enabling and after disabling to  */
    /* minimize spurious interrupts.  The line count is set even     */
    /* when interrupts are disabled to allow polling-based or debug  */
    /* applications.                                                 */

    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);
    if (interrupt_info->enable) {
        WRITE_REG32(DC3_IRQ_FILT_CTL, irq_line);
        WRITE_REG32(DC3_IRQ, ((irq_enable & ~DC3_IRQ_MASK) | DC3_IRQ_STATUS));
    }
    else {
        WRITE_REG32(DC3_IRQ, (irq_enable | DC3_IRQ_MASK));
        WRITE_REG32(DC3_IRQ_FILT_CTL, irq_line);
    }
    WRITE_REG32(DC3_UNLOCK, lock);
    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vg_test_and_clear_interrupt
 *
 * This routine resets any pending interrupt in the video generator.  The
 * return value indicates the interrupt status prior to the reset.
 *--------------------------------------------------------------------------*/

unsigned long
vg_test_and_clear_interrupt(void)
{
    unsigned long irq_enable;
    unsigned long lock;

    irq_enable = READ_REG32(DC3_IRQ);
    lock = READ_REG32(DC3_UNLOCK);

    /* NO ACTION IF INTERRUPTS ARE MASKED */
    /* We are assuming that a driver or application will not want to receive */
    /* the status of the interrupt when it is masked.                       */

    if ((irq_enable & (DC3_IRQ_MASK | DC3_VSYNC_IRQ_MASK)) ==
        (DC3_IRQ_MASK | DC3_VSYNC_IRQ_MASK))
        return 0;

    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);
    WRITE_REG32(DC3_IRQ, irq_enable);
    WRITE_REG32(DC3_UNLOCK, lock);

    return (irq_enable & (DC3_IRQ_STATUS | DC3_VSYNC_IRQ_STATUS));
}

/*---------------------------------------------------------------------------
 * vg_test_flip_status
 *
 * This routine tests if a new display offset has been latched.
 *--------------------------------------------------------------------------*/

unsigned long
vg_test_flip_status(void)
{
    return (READ_REG32(DC3_LINE_CNT_STATUS) & DC3_LNCNT_FLIP);
}

/*---------------------------------------------------------------------------
 * vg_save_state
 *
 * This routine saves all persistent VG state information.
 *--------------------------------------------------------------------------*/

int
vg_save_state(VG_SAVE_RESTORE *vg_state)
{
    Q_WORD msr_value = { 0, 0 };
    unsigned long irqfilt;
    unsigned long offset, i;
    unsigned long lock;

    /* READ ALL CURRENT REGISTER SETTINGS */

    vg_state->unlock = READ_REG32(DC3_UNLOCK);
    vg_state->gcfg = READ_REG32(DC3_GENERAL_CFG);
    vg_state->dcfg = READ_REG32(DC3_DISPLAY_CFG);
    vg_state->arb_cfg = READ_REG32(DC3_ARB_CFG);
    vg_state->fb_offset = READ_REG32(DC3_FB_ST_OFFSET);
    vg_state->cb_offset = READ_REG32(DC3_CB_ST_OFFSET);
    vg_state->cursor_offset = READ_REG32(DC3_CURS_ST_OFFSET);
    vg_state->video_y_offset = READ_REG32(DC3_VID_Y_ST_OFFSET);
    vg_state->video_u_offset = READ_REG32(DC3_VID_U_ST_OFFSET);
    vg_state->video_v_offset = READ_REG32(DC3_VID_V_ST_OFFSET);
    vg_state->dv_top = READ_REG32(DC3_DV_TOP);
    vg_state->line_size = READ_REG32(DC3_LINE_SIZE);
    vg_state->gfx_pitch = READ_REG32(DC3_GFX_PITCH);
    vg_state->video_yuv_pitch = READ_REG32(DC3_VID_YUV_PITCH);
    vg_state->h_active = READ_REG32(DC3_H_ACTIVE_TIMING);
    vg_state->h_blank = READ_REG32(DC3_H_BLANK_TIMING);
    vg_state->h_sync = READ_REG32(DC3_H_SYNC_TIMING);
    vg_state->v_active = READ_REG32(DC3_V_ACTIVE_TIMING);
    vg_state->v_blank = READ_REG32(DC3_V_BLANK_TIMING);
    vg_state->v_sync = READ_REG32(DC3_V_SYNC_TIMING);
    vg_state->fb_active = READ_REG32(DC3_FB_ACTIVE);
    vg_state->cursor_x = READ_REG32(DC3_CURSOR_X);
    vg_state->cursor_y = READ_REG32(DC3_CURSOR_Y);
    vg_state->vid_ds_delta = READ_REG32(DC3_VID_DS_DELTA);
    vg_state->fb_base = READ_REG32(DC3_PHY_MEM_OFFSET);
    vg_state->dv_ctl = READ_REG32(DC3_DV_CTL);
    vg_state->gfx_scale = READ_REG32(DC3_GFX_SCALE);
    vg_state->irq_ctl = READ_REG32(DC3_IRQ_FILT_CTL);
    vg_state->vbi_even_ctl = READ_REG32(DC3_VBI_EVEN_CTL);
    vg_state->vbi_odd_ctl = READ_REG32(DC3_VBI_ODD_CTL);
    vg_state->vbi_hor_ctl = READ_REG32(DC3_VBI_HOR);
    vg_state->vbi_odd_line_enable = READ_REG32(DC3_VBI_LN_ODD);
    vg_state->vbi_even_line_enable = READ_REG32(DC3_VBI_LN_EVEN);
    vg_state->vbi_pitch = READ_REG32(DC3_VBI_PITCH);
    vg_state->color_key = READ_REG32(DC3_COLOR_KEY);
    vg_state->color_key_mask = READ_REG32(DC3_COLOR_MASK);
    vg_state->color_key_x = READ_REG32(DC3_CLR_KEY_X);
    vg_state->color_key_y = READ_REG32(DC3_CLR_KEY_Y);
    vg_state->irq = READ_REG32(DC3_IRQ);
    vg_state->genlk_ctl = READ_REG32(DC3_GENLK_CTL);
    vg_state->vid_y_even_offset = READ_REG32(DC3_VID_EVEN_Y_ST_OFFSET);
    vg_state->vid_u_even_offset = READ_REG32(DC3_VID_EVEN_U_ST_OFFSET);
    vg_state->vid_v_even_offset = READ_REG32(DC3_VID_EVEN_V_ST_OFFSET);
    vg_state->vactive_even = READ_REG32(DC3_V_ACTIVE_EVEN);
    vg_state->vblank_even = READ_REG32(DC3_V_BLANK_EVEN);
    vg_state->vsync_even = READ_REG32(DC3_V_SYNC_EVEN);

    /* READ THE CURRENT PALETTE */

    lock = READ_REG32(DC3_UNLOCK);
    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);
    WRITE_REG32(DC3_PAL_ADDRESS, 0);
    for (i = 0; i < 261; i++)
        vg_state->palette[i] = READ_REG32(DC3_PAL_DATA);

    /* READ THE CURRENT FILTER COEFFICIENTS */

    /* ENABLE ACCESS TO THE HORIZONTAL COEFFICIENTS */

    irqfilt = READ_REG32(DC3_IRQ_FILT_CTL);
    irqfilt |= DC3_IRQFILT_H_FILT_SEL;

    /* READ HORIZONTAL COEFFICIENTS */

    for (i = 0; i < 256; i++) {
        WRITE_REG32(DC3_IRQ_FILT_CTL, ((irqfilt & 0xFFFFFF00L) | i));

        vg_state->h_coeff[(i << 1)] = READ_REG32(DC3_FILT_COEFF1);
        vg_state->h_coeff[(i << 1) + 1] = READ_REG32(DC3_FILT_COEFF2);
    }

    /* ENABLE ACCESS TO THE VERTICAL COEFFICIENTS */

    irqfilt &= ~DC3_IRQFILT_H_FILT_SEL;

    /* READ COEFFICIENTS */

    for (i = 0; i < 256; i++) {
        WRITE_REG32(DC3_IRQ_FILT_CTL, ((irqfilt & 0xFFFFFF00L) | i));

        vg_state->v_coeff[i] = READ_REG32(DC3_FILT_COEFF1);
    }

    /* READ THE CURSOR DATA */

    offset = READ_REG32(DC3_CURS_ST_OFFSET) & 0x0FFFFFFF;
    for (i = 0; i < 3072; i++)
        vg_state->cursor_data[i] = READ_FB32(offset + (i << 2));

    /* READ THE CURRENT PLL */

    msr_read64(MSR_DEVICE_GEODELX_GLCP, GLCP_DOTPLL, &msr_value);

    vg_state->pll_flags = 0;
    for (i = 0; i < NUM_CIMARRON_PLL_FREQUENCIES; i++) {
        if (CimarronPLLFrequencies[i].pll_value == (msr_value.high & 0x7FFF)) {
            vg_state->dot_pll = CimarronPLLFrequencies[i].frequency;
            break;
        }
    }

    if (i == NUM_CIMARRON_PLL_FREQUENCIES) {
        /* NO MATCH */
        /* Enter the frequency as a manual frequency. */

        vg_state->dot_pll = msr_value.high;
        vg_state->pll_flags |= VG_PLL_MANUAL;
    }
    if (msr_value.low & GLCP_DOTPLL_HALFPIX)
        vg_state->pll_flags |= VG_PLL_DIVIDE_BY_2;
    if (msr_value.low & GLCP_DOTPLL_BYPASS)
        vg_state->pll_flags |= VG_PLL_BYPASS;
    if (msr_value.high & GLCP_DOTPLL_DIV4)
        vg_state->pll_flags |= VG_PLL_DIVIDE_BY_4;
    if (msr_value.high & GLCP_DOTPLL_VIPCLK)
        vg_state->pll_flags |= VG_PLL_VIP_CLOCK;

    /* READ ALL VG MSRS */

    msr_read64(MSR_DEVICE_GEODELX_VG, MSR_GEODELINK_CAP, &(vg_state->msr_cap));
    msr_read64(MSR_DEVICE_GEODELX_VG, MSR_GEODELINK_CONFIG,
               &(vg_state->msr_config));
    msr_read64(MSR_DEVICE_GEODELX_VG, MSR_GEODELINK_SMI, &(vg_state->msr_smi));
    msr_read64(MSR_DEVICE_GEODELX_VG, MSR_GEODELINK_ERROR,
               &(vg_state->msr_error));
    msr_read64(MSR_DEVICE_GEODELX_VG, MSR_GEODELINK_PM, &(vg_state->msr_pm));
    msr_read64(MSR_DEVICE_GEODELX_VG, MSR_GEODELINK_DIAG,
               &(vg_state->msr_diag));
    msr_read64(MSR_DEVICE_GEODELX_VG, DC3_SPARE_MSR, &(vg_state->msr_spare));
    msr_read64(MSR_DEVICE_GEODELX_VG, DC3_RAM_CTL, &(vg_state->msr_ram_ctl));

    WRITE_REG32(DC3_UNLOCK, lock);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vg_restore_state
 *
 * This routine restores all persistent VG state information.
 *--------------------------------------------------------------------------*/

int
vg_restore_state(VG_SAVE_RESTORE *vg_state)
{
    unsigned long irqfilt, j;
    unsigned long memoffset;

    /* TEMPORARILY UNLOCK ALL REGISTERS */

    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);

    /* RESTORE THE FRAME BUFFER OFFSET */

    WRITE_REG32(DC3_PHY_MEM_OFFSET, vg_state->fb_base);

    /* BLANK GCFG AND DCFG */

    WRITE_REG32(DC3_GENERAL_CFG, 0);
    WRITE_REG32(DC3_DISPLAY_CFG, 0);

    /* RESTORE ALL REGISTERS */

    WRITE_REG32(DC3_ARB_CFG, vg_state->arb_cfg);
    WRITE_REG32(DC3_FB_ST_OFFSET, vg_state->fb_offset);
    WRITE_REG32(DC3_CB_ST_OFFSET, vg_state->cb_offset);
    WRITE_REG32(DC3_CURS_ST_OFFSET, vg_state->cursor_offset);
    WRITE_REG32(DC3_VID_Y_ST_OFFSET, vg_state->video_y_offset);
    WRITE_REG32(DC3_VID_U_ST_OFFSET, vg_state->video_u_offset);
    WRITE_REG32(DC3_VID_V_ST_OFFSET, vg_state->video_v_offset);
    WRITE_REG32(DC3_DV_TOP, vg_state->dv_top);
    WRITE_REG32(DC3_LINE_SIZE, vg_state->line_size);
    WRITE_REG32(DC3_GFX_PITCH, vg_state->gfx_pitch);
    WRITE_REG32(DC3_VID_YUV_PITCH, vg_state->video_yuv_pitch);
    WRITE_REG32(DC3_H_ACTIVE_TIMING, vg_state->h_active);
    WRITE_REG32(DC3_H_BLANK_TIMING, vg_state->h_blank);
    WRITE_REG32(DC3_H_SYNC_TIMING, vg_state->h_sync);
    WRITE_REG32(DC3_V_ACTIVE_TIMING, vg_state->v_active);
    WRITE_REG32(DC3_V_BLANK_TIMING, vg_state->v_blank);
    WRITE_REG32(DC3_V_SYNC_TIMING, vg_state->v_sync);
    WRITE_REG32(DC3_FB_ACTIVE, vg_state->fb_active);
    WRITE_REG32(DC3_CURSOR_X, vg_state->cursor_x);
    WRITE_REG32(DC3_CURSOR_Y, vg_state->cursor_y);
    WRITE_REG32(DC3_VID_DS_DELTA, vg_state->vid_ds_delta);
    WRITE_REG32(DC3_PHY_MEM_OFFSET, vg_state->fb_base);
    WRITE_REG32(DC3_DV_CTL, vg_state->dv_ctl | 0x00000001);
    WRITE_REG32(DC3_GFX_SCALE, vg_state->gfx_scale);
    WRITE_REG32(DC3_IRQ_FILT_CTL, vg_state->irq_ctl);
    WRITE_REG32(DC3_VBI_EVEN_CTL, vg_state->vbi_even_ctl);
    WRITE_REG32(DC3_VBI_ODD_CTL, vg_state->vbi_odd_ctl);
    WRITE_REG32(DC3_VBI_HOR, vg_state->vbi_hor_ctl);
    WRITE_REG32(DC3_VBI_LN_ODD, vg_state->vbi_odd_line_enable);
    WRITE_REG32(DC3_VBI_LN_EVEN, vg_state->vbi_even_line_enable);
    WRITE_REG32(DC3_VBI_PITCH, vg_state->vbi_pitch);
    WRITE_REG32(DC3_COLOR_KEY, vg_state->color_key);
    WRITE_REG32(DC3_COLOR_MASK, vg_state->color_key_mask);
    WRITE_REG32(DC3_CLR_KEY_X, vg_state->color_key_x);
    WRITE_REG32(DC3_CLR_KEY_Y, vg_state->color_key_y);
    WRITE_REG32(DC3_IRQ, vg_state->irq);
    WRITE_REG32(DC3_GENLK_CTL, vg_state->genlk_ctl);
    WRITE_REG32(DC3_VID_EVEN_Y_ST_OFFSET, vg_state->vid_y_even_offset);
    WRITE_REG32(DC3_VID_EVEN_U_ST_OFFSET, vg_state->vid_u_even_offset);
    WRITE_REG32(DC3_VID_EVEN_V_ST_OFFSET, vg_state->vid_v_even_offset);
    WRITE_REG32(DC3_V_ACTIVE_EVEN, vg_state->vactive_even);
    WRITE_REG32(DC3_V_BLANK_EVEN, vg_state->vblank_even);
    WRITE_REG32(DC3_V_SYNC_EVEN, vg_state->vsync_even);

    /* RESTORE THE PALETTE */

    WRITE_REG32(DC3_PAL_ADDRESS, 0);
    for (j = 0; j < 261; j++)
        WRITE_REG32(DC3_PAL_DATA, vg_state->palette[j]);

    /* RESTORE THE HORIZONTAL FILTER COEFFICIENTS */

    irqfilt = READ_REG32(DC3_IRQ_FILT_CTL);
    irqfilt |= DC3_IRQFILT_H_FILT_SEL;

    for (j = 0; j < 256; j++) {
        WRITE_REG32(DC3_IRQ_FILT_CTL, ((irqfilt & 0xFFFFFF00L) | j));
        WRITE_REG32(DC3_FILT_COEFF1, vg_state->h_coeff[(j << 1)]);
        WRITE_REG32(DC3_FILT_COEFF2, vg_state->h_coeff[(j << 1) + 1]);
    }

    /* RESTORE VERTICAL COEFFICIENTS */

    irqfilt &= ~DC3_IRQFILT_H_FILT_SEL;

    for (j = 0; j < 256; j++) {
        WRITE_REG32(DC3_IRQ_FILT_CTL, ((irqfilt & 0xFFFFFF00L) | j));
        WRITE_REG32(DC3_FILT_COEFF1, vg_state->v_coeff[j]);
    }

    /* RESTORE THE CURSOR DATA */

    memoffset = READ_REG32(DC3_CURS_ST_OFFSET) & 0x0FFFFFFF;
    WRITE_FB_STRING32(memoffset, (unsigned char *) &(vg_state->cursor_data[0]),
                      3072);

    /* RESTORE THE PLL */
    /* Use a common routine to use common code to poll for lock bit */

    vg_set_clock_frequency(vg_state->dot_pll, vg_state->pll_flags);

    /* RESTORE ALL VG MSRS */

    msr_write64(MSR_DEVICE_GEODELX_VG, MSR_GEODELINK_CAP, &(vg_state->msr_cap));
    msr_write64(MSR_DEVICE_GEODELX_VG, MSR_GEODELINK_CONFIG,
                &(vg_state->msr_config));
    msr_write64(MSR_DEVICE_GEODELX_VG, MSR_GEODELINK_SMI, &(vg_state->msr_smi));
    msr_write64(MSR_DEVICE_GEODELX_VG, MSR_GEODELINK_ERROR,
                &(vg_state->msr_error));
    msr_write64(MSR_DEVICE_GEODELX_VG, MSR_GEODELINK_PM, &(vg_state->msr_pm));
    msr_write64(MSR_DEVICE_GEODELX_VG, MSR_GEODELINK_DIAG,
                &(vg_state->msr_diag));
    msr_write64(MSR_DEVICE_GEODELX_VG, DC3_SPARE_MSR, &(vg_state->msr_spare));
    msr_write64(MSR_DEVICE_GEODELX_VG, DC3_RAM_CTL, &(vg_state->msr_ram_ctl));

    /* NOW RESTORE GCFG AND DCFG */

    WRITE_REG32(DC3_DISPLAY_CFG, vg_state->dcfg);
    WRITE_REG32(DC3_GENERAL_CFG, vg_state->gcfg);

    /* FINALLY RESTORE UNLOCK */

    WRITE_REG32(DC3_UNLOCK, vg_state->unlock);

    return CIM_STATUS_OK;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * CIMARRON VG READ ROUTINES
 * These routines are included for use in diagnostics or when debugging.  They
 * can be optionally excluded from a project.
 *++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#if CIMARRON_INCLUDE_VG_READ_ROUTINES

/*---------------------------------------------------------------------------
 * vg_read_graphics_crc
 *
 * This routine reads the Cyclic Redundancy Check (CRC) value for the graphics
 * frame.
 *--------------------------------------------------------------------------*/

unsigned long
vg_read_graphics_crc(int crc_source)
{
    unsigned long gcfg, unlock;
    unsigned long crc, vbi_even;
    unsigned long interlaced;
    unsigned long line, field;

    if (!(READ_REG32(DC3_DISPLAY_CFG) & DC3_DCFG_TGEN))
        return 0xFFFFFFFF;

    unlock = READ_REG32(DC3_UNLOCK);
    gcfg = READ_REG32(DC3_GENERAL_CFG);
    vbi_even = READ_REG32(DC3_VBI_EVEN_CTL);

    vbi_even &= ~DC3_VBI_EVEN_ENABLE_CRC;

    gcfg |= DC3_GCFG_SGRE | DC3_GCFG_CRC_MODE;
    gcfg &= ~(DC3_GCFG_SGFR | DC3_GCFG_SIG_SEL | DC3_GCFG_FILT_SIG_SEL);

    switch (crc_source) {
    case VG_CRC_SOURCE_PREFILTER_EVEN:
    case VG_CRC_SOURCE_PREFILTER:
        gcfg |= DC3_GCFG_SIG_SEL;
        break;
    case VG_CRC_SOURCE_PREFLICKER:
    case VG_CRC_SOURCE_PREFLICKER_EVEN:
        gcfg |= DC3_GCFG_FILT_SIG_SEL;
        break;
    case VG_CRC_SOURCE_POSTFLICKER:
    case VG_CRC_SOURCE_POSTFLICKER_EVEN:       /* NO WORK */
        break;

    default:
        return 0xFFFFFFFF;
    }

    if (crc_source & VG_CRC_SOURCE_EVEN)
        field = 0;
    else
        field = DC3_LNCNT_EVEN_FIELD;

    if ((interlaced = (READ_REG32(DC3_IRQ_FILT_CTL) & DC3_IRQFILT_INTL_EN))) {
        /* WAIT FOR THE BEGINNING OF THE FIELD (LINE 1-5) */
        /* Note that we wait for the field to be odd when CRCing the even */
        /* field and vice versa.  This is because the CRC will not begin  */
        /* until the following field.                                     */

        do {
            line = READ_REG32(DC3_LINE_CNT_STATUS);
        } while ((line & DC3_LNCNT_EVEN_FIELD) != field ||
                 ((line & DC3_LNCNT_V_LINE_CNT) >> 16) < 10 ||
                 ((line & DC3_LNCNT_V_LINE_CNT) >> 16) > 15);
    }
    else {
        /* NON-INTERLACED - EVEN FIELD CRCS ARE INVALID */

        if (crc_source & VG_CRC_SOURCE_EVEN)
            return 0xFFFFFFFF;
    }

    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);
    WRITE_REG32(DC3_VBI_EVEN_CTL, vbi_even);
    WRITE_REG32(DC3_GENERAL_CFG, gcfg & ~DC3_GCFG_SIGE);
    WRITE_REG32(DC3_GENERAL_CFG, gcfg | DC3_GCFG_SIGE);

    /* WAIT FOR THE CRC TO BE COMPLETED */

    while (!(READ_REG32(DC3_LINE_CNT_STATUS) & DC3_LNCNT_SIGC));

    /* READ THE COMPLETED CRC */

    crc = READ_REG32(DC3_PAL_DATA);

    /* RESTORE THE PALETTE SETTINGS */

    gcfg &= ~DC3_GCFG_SGRE;
    WRITE_REG32(DC3_GENERAL_CFG, gcfg);
    WRITE_REG32(DC3_UNLOCK, unlock);

    return crc;
}

/*---------------------------------------------------------------------------
 * vg_read_window_crc
 *
 * This routine reads the Cyclic Redundancy Check (CRC) value for a sub-
 * section of the frame.
 *--------------------------------------------------------------------------*/

unsigned long
vg_read_window_crc(int crc_source, unsigned long x, unsigned long y,
                   unsigned long width, unsigned long height)
{
    Q_WORD msr_value = { 0, 0 };
    unsigned long crc = 0;
    unsigned long hactive, hblankstart;
    unsigned long htotal, hblankend;
    unsigned long line, field;
    unsigned long diag;

    hactive = ((READ_REG32(DC3_H_ACTIVE_TIMING)) & 0xFFF) + 1;
    hblankstart = ((READ_REG32(DC3_H_BLANK_TIMING)) & 0xFFF) + 1;
    htotal = ((READ_REG32(DC3_H_ACTIVE_TIMING) >> 16) & 0xFFF) + 1;
    hblankend = ((READ_REG32(DC3_H_BLANK_TIMING) >> 16) & 0xFFF) + 1;

    /* TIMINGS MUST BE ACTIVE */

    if (!(READ_REG32(DC3_DISPLAY_CFG) & DC3_DCFG_TGEN))
        return 0xFFFFFFFF;

    /* DISABLE GLCP ACTIONS */

    msr_value.low = 0;
    msr_value.high = 0;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_DIAGCTL, &msr_value);

    if ((x == 0 && width == 1) || x == 1) {
        /* SPECIAL CASE FOR X == 0 */
        /* The comparator output is a clock late in the MCP, so we cannot */
        /* easily catch the first pixel.  If the first pixel is desired,  */
        /* we will insert a special state machine to CRC just the first   */
        /* pixel.                                                         */

        /* N2 - DISPE HIGH AND Y == 1 */
        /* Goto state YState = 2      */

        msr_value.high = 0x00000002;
        msr_value.low = 0x00000C00;
        msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_SETN0CTL + 2, &msr_value);

        /* M3 - DISPE HIGH AND Y == 0 */
        /* Goto YState = 1            */

        msr_value.high = 0x00000002;
        msr_value.low = 0x00000A00;
        msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_SETM0CTL + 3, &msr_value);

        /* N3 - DISPE LOW  */
        /* Goto YState = 0 */

        msr_value.high = 0x00080000;
        msr_value.low = 0x00000000;
        msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_SETN0CTL + 3, &msr_value);

        /* Y0 -> Y1 (SET M3) */

        msr_value.high = 0x00000000;
        msr_value.low = 0x0000C000;
        msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_ACTION0 + 18, &msr_value);

        /* Y1 -> Y0 (SET N3) */

        msr_value.low = 0x0000A000;
        msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_ACTION0 + 17, &msr_value);

        /* Y1 -> Y2 (SET N2) */

        msr_value.low = 0x00000A00;
        msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_ACTION0 + 19, &msr_value);

        /* N5 (XSTATE = 10 && CMP2 <= V. COUNTER <= CMP3) &&DISPE&& Y == 0 */
        /* CRC into REGB                                                   */

        msr_value.high = 0x00000002;
        msr_value.low = 0x10800B20;
        msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_SETN0CTL + 5, &msr_value);

        /* N6 (XSTATE = 10 && CMP2 <= V. COUNTER <= CMP3) && DISPE&&Y == 1 */
        /* CRC into REGB                                                   */

        msr_value.high = 0x00000002;
        msr_value.low = 0x10800D20;
        msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_SETN0CTL + 6, &msr_value);
    }

    /* M4 (XSTATE = 00 AND VSYNC HIGH) */
    /* Goto state 01                   */
    /* Note: VSync = H3A               */

    msr_value.high = 0x00000001;
    msr_value.low = 0x000000A0;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_SETM0CTL + 4, &msr_value);

    /* N0 (XSTATE = 01 AND VSYNC LOW) */
    /* Goto state 02                  */
    /* Note: VSync low = H3B          */

    msr_value.high = 0x00040000;
    msr_value.low = 0x000000C0;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_SETN0CTL, &msr_value);

    /* M5 (XSTATE = 10 AND VSYNC HIGH) */
    /* Goto state 11                   */

    msr_value.high = 0x00000001;
    msr_value.low = 0x00000120;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_SETM0CTL + 5, &msr_value);

    /* N1 (XSTATE = 10 and DISPE HIGH) */
    /* Increment H. Counter           */
    /* Note: DispE = H4               */

    msr_value.high = 0x00000002;
    msr_value.low = 0x00000120;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_SETN0CTL + 1, &msr_value);

    /* M0 (XSTATE = 10 and H. COUNTER == LIMIT)  */
    /* Clear H. Counter and increment V. Counter */

    msr_value.high = 0x00000000;
    msr_value.low = 0x00000122;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_SETM0CTL, &msr_value);

    /* N4 (XSTATE = 10 && CMP0 <= H. COUNTER <= CMP1 && CMP2 <= V. COUNTER
     * <= CMP3) && DISPE
     * CRC into REGB
     */

    msr_value.high = 0x00000002;
    msr_value.low = 0x10C20120;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_SETN0CTL + 4, &msr_value);

    /* COMPARATOR 0 VALUE                                         */
    /* We subtract 1 to account for a pipeline delay in the GLCP. */
    /* When the x coordinate is 0, we must play a special game.   */
    /* If the width is exactly 1, we will set up a state machine  */
    /* to only CRC the first pixel.  Otherwise, we will set it    */
    /* as an OR combination of a state that CRCs the first pixel  */
    /* and a state that CRCs 1 clock delayed width (width - 1)    */

    msr_value.high = 0;
    if (x > 1)
        msr_value.low = (x - 1) & 0xFFFF;
    else
        msr_value.low = x;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_CMPVAL0, &msr_value);

    /* COMPARATOR 1 VALUE */

    if ((x == 0 || x == 1) && width > 1)
        msr_value.low += width - 2;
    else
        msr_value.low += width - 1;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_CMPVAL0 + 2, &msr_value);

    /* COMPARATOR 2 VALUE */

    msr_value.low = y << 16;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_CMPVAL0 + 4, &msr_value);

    /* COMPARATOR 3 VALUE */

    msr_value.low += (height - 1) << 16;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_CMPVAL0 + 6, &msr_value);

    /* COMPARATOR MASKS */
    /* Comparators 0 and 1 refer to lower 16 bits of RegB */

    msr_value.low = 0x0000FFFF;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_CMPMASK0, &msr_value);
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_CMPMASK0 + 2, &msr_value);

    /* Comparators 2 and 3 refer to upper 16 bits of RegB */

    msr_value.low = 0xFFFF0000;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_CMPMASK0 + 4, &msr_value);
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_CMPMASK0 + 6, &msr_value);

    /* SET REGB MASK                                               */
    /* We set the mask such that all all 32 bits of data are CRCed */

    msr_value.low = 0xFFFFFFFF;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_REGBMASK, &msr_value);

    /* ACTIONS */

    /* STATE 00->01 (SET 4M) */

    msr_value.low = 0x000C0000;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_ACTION0 + 14, &msr_value);

    /* STATE 01->10 (SET 0N) */

    msr_value.low = 0x0000000A;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_ACTION0 + 15, &msr_value);

    /* STATE 10->11 (SET 5M) */

    msr_value.low = 0x00C00000;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_ACTION0 + 16, &msr_value);

    /* CLEAR REGA WHEN TRANSITIONING TO STATE 10                 */
    /* Do not clear RegB as the initial value must be 0x00000001 */

    msr_value.low = 0x0000000A;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_ACTION0, &msr_value);

    /* REGISTER ACTION 1
     * CRC into RegB if cmp0 <= h.counter <= cmp1 && cmp2 <= v. counter <
     * cmp3 && 7 xstate = 10
     * Increment h.counter if xstate = 10 and HSync is low.
     */

    msr_value.low = 0x000A00A0;
    if (x == 0 && width == 1)
        msr_value.low = 0x00A000A0;
    else if (x == 1 && width == 1)
        msr_value.low = 0x0A0000A0;
    else if (x == 1 && width > 1)
        msr_value.low |= 0x0A000000;

    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_ACTION0 + 1, &msr_value);

    /* REGISTER ACTION 2            */
    /* Increment V. Counter in REGA */

    msr_value.low = 0x0000000C;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_ACTION0 + 2, &msr_value);

    /* SET REGB TO 0x00000001 */

    msr_value.low = 0x00000001;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_REGB, &msr_value);

    /* SET XSTATE TO 0 */

    msr_value.low = 0x00000000;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_XSTATE, &msr_value);

    /* SET YSTATE TO 0 */

    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_YSTATE, &msr_value);

    /* CLEAR ALL OTHER ACTIONS */
    /* This prevents side-effects from previous accesses to the GLCP */
    /* debug logic.                                                  */

    msr_value.low = 0x00000000;
    msr_value.high = 0x00000000;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_ACTION0 + 3, &msr_value);
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_ACTION0 + 4, &msr_value);
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_ACTION0 + 5, &msr_value);
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_ACTION0 + 6, &msr_value);
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_ACTION0 + 7, &msr_value);
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_ACTION0 + 8, &msr_value);
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_ACTION0 + 9, &msr_value);
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_ACTION0 + 10, &msr_value);
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_ACTION0 + 11, &msr_value);
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_ACTION0 + 12, &msr_value);
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_ACTION0 + 13, &msr_value);
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_ACTION0 + 20, &msr_value);

    /* SET DIAG SETTINGS BASED ON DESIRED CRC */

    if (crc_source == VG_CRC_SOURCE_POSTFLICKER
        || crc_source == VG_CRC_SOURCE_POSTFLICKER_EVEN) {
        diag = 0x80808086;

        /* ENABLE HW CLOCK GATING AND SET GLCP CLOCK TO DOT CLOCK */

        msr_value.high = 0;
        msr_value.low = 5;
        msr_write64(MSR_DEVICE_GEODELX_GLCP, MSR_GEODELINK_PM, &msr_value);
        msr_value.low = 0;
        msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_DBGCLKCTL, &msr_value);
        msr_value.low = 3;
        msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_DBGCLKCTL, &msr_value);

        /* SET REGA LIMITS                              */
        /* Lower counter uses pixels/line               */
        /* Upper counter is 0xFFFF to prevent rollover. */

        msr_value.low = 0xFFFF0000 | (hactive - 1);
        if (READ_REG32(DC3_DISPLAY_CFG) & DC3_DCFG_DCEN) {
            msr_value.low += hblankstart - hactive;
            msr_value.low += htotal - hblankend;
        }
        msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_REGAVAL, &msr_value);

        /* USE H4 FUNCTION A FOR DISPE AND H4 FUNCTION B FOR NOT DISPE */
        /* DISPE is bit 34                                             */

        msr_value.high = 0x00000002;
        msr_value.low = 0x20000FF0;
        msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_H0CTL + 4, &msr_value);

        /* USE H3 FUNCTION A FOR VSYNC AND H3 FUNCTION B FOR NOT VSYNC */
        /* VSYNC is bit 32.                                            */

        msr_value.high = 0x00000000;
        msr_value.low = 0x002055AA;
        msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_H0CTL + 3, &msr_value);
    }
    else if (crc_source == VG_CRC_SOURCE_PREFLICKER
             || crc_source == VG_CRC_SOURCE_PREFLICKER_EVEN) {
        diag = 0x801F8032;

        /* ENABLE HW CLOCK GATING AND SET GLCP CLOCK TO GEODELINK CLOCK */

        msr_value.high = 0;
        msr_value.low = 5;
        msr_write64(MSR_DEVICE_GEODELX_GLCP, MSR_GEODELINK_PM, &msr_value);
        msr_value.low = 0;
        msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_DBGCLKCTL, &msr_value);
        msr_value.low = 2;
        msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_DBGCLKCTL, &msr_value);

        /* SET REGA LIMITS                              */
        /* Lower counter uses pixels/line               */
        /* Upper counter is 0xFFFF to prevent rollover. */

        msr_value.low = 0xFFFF0000 | (hactive - 1);
        msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_REGAVAL, &msr_value);

        /* USE H4 FUNCTION A FOR DISPE AND H4 FUNCTION B FOR NOT DISPE */
        /* DISPE is bit 47                                             */

        msr_value.high = 0x00000002;
        msr_value.low = 0xF0000FF0;
        msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_H0CTL + 4, &msr_value);

        /* USE H3 FUNCTION A FOR VSYNC AND H3 FUNCTION B FOR NOT VSYNC */
        /* VSYNC is bit 45.                                            */

        msr_value.high = 0x00000000;
        msr_value.low = 0x002D55AA;
        msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_H0CTL + 3, &msr_value);
    }
    else {
        /* PREFILTER CRC */

        diag = 0x80138048;
        msr_write64(MSR_DEVICE_GEODELX_VG, MSR_GEODELINK_DIAG, &msr_value);

        /* ENABLE HW CLOCK GATING AND SET GLCP CLOCK TO GEODELINK CLOCK */

        msr_value.high = 0;
        msr_value.low = 5;
        msr_write64(MSR_DEVICE_GEODELX_GLCP, MSR_GEODELINK_PM, &msr_value);
        msr_value.low = 0;
        msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_DBGCLKCTL, &msr_value);
        msr_value.low = 2;
        msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_DBGCLKCTL, &msr_value);

        /* SET REGA LIMITS                                      */
        /* Lower counter uses pixels/line                       */
        /* Upper counter is 0xFFFF to prevent rollover.         */
        /* Note that we are assuming that the number of         */
        /* source pixels is specified in the FB_ACTIVE register */

        msr_value.low =
            0xFFFF0000 | ((READ_REG32(DC3_FB_ACTIVE) >> 16) & 0xFFF);
        msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_REGAVAL, &msr_value);

        /* USE H4 FUNCTION A FOR DISPE AND H4 FUNCTION B FOR NOT DISPE */
        /* DISPE is bit 55                                             */

        msr_value.high = 0x00000003;
        msr_value.low = 0x70000FF0;
        msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_H0CTL + 4, &msr_value);

        /* USE H3 FUNCTION A FOR VSYNC AND H3 FUNCTION B FOR NOT VSYNC */
        /* VSYNC is bit 53.                                            */

        msr_value.high = 0x00000000;
        msr_value.low = 0x003555AA;
        msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_H0CTL + 3, &msr_value);
    }

    /* WAIT FOR THE CORRECT FIELD */
    /* We use the VG line count and field indicator to determine when */
    /* to kick off a CRC.                                             */

    if (crc_source & VG_CRC_SOURCE_EVEN)
        field = 0;
    else
        field = DC3_LNCNT_EVEN_FIELD;

    if (READ_REG32(DC3_IRQ_FILT_CTL) & DC3_IRQFILT_INTL_EN) {
        /* WAIT FOR THE BEGINNING OF THE FIELD (LINE 1-5) */
        /* Note that we wait for the field to be odd when CRCing the even */
        /* field and vice versa.  This is because the CRC will not begin  */
        /* until the following field.                                     */

        do {
            line = READ_REG32(DC3_LINE_CNT_STATUS);
        } while ((line & DC3_LNCNT_EVEN_FIELD) != field ||
                 ((line & DC3_LNCNT_V_LINE_CNT) >> 16) < 1 ||
                 ((line & DC3_LNCNT_V_LINE_CNT) >> 16) > 5);
    }
    else {
        /* NON-INTERLACED - EVEN FIELD CRCS ARE INVALID */

        if (crc_source & VG_CRC_SOURCE_EVEN)
            return 0xFFFFFFFF;
    }

    /* UPDATE VG DIAG OUTPUT */

    msr_value.high = 0;
    msr_value.low = diag;
    msr_write64(MSR_DEVICE_GEODELX_VG, MSR_GEODELINK_DIAG, &msr_value);

    /* CONFIGURE DIAG CONTROL */
    /* Set RegA action1 to increment lower 16 bits and clear at limit. (5)  */
    /* Set RegA action2 to increment upper 16 bits. (6)                     */
    /* Set RegB action1 to CRC32 (1)                                        */
    /* Set all comparators to REGA override (0,1 lower mbus, 2,3 upper mbus) */
    /* Enable all actions                                                   */

    msr_value.low = 0x80EA20A0;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_DIAGCTL, &msr_value);

    /* DELAY TWO FRAMES */

    while (READ_REG32(DC3_LINE_CNT_STATUS) & DC3_LNCNT_VNA);
    while (!(READ_REG32(DC3_LINE_CNT_STATUS) & DC3_LNCNT_VNA));
    while (READ_REG32(DC3_LINE_CNT_STATUS) & DC3_LNCNT_VNA);
    while (!(READ_REG32(DC3_LINE_CNT_STATUS) & DC3_LNCNT_VNA));
    while (READ_REG32(DC3_LINE_CNT_STATUS) & DC3_LNCNT_VNA);

    /* VERIFY THAT XSTATE = 11 */

    msr_read64(MSR_DEVICE_GEODELX_GLCP, GLCP_XSTATE, &msr_value);
    if ((msr_value.low & 3) == 3) {
        msr_read64(MSR_DEVICE_GEODELX_GLCP, GLCP_REGB, &msr_value);

        crc = msr_value.low;
    }

    /* DISABLE VG DIAG BUS OUTPUTS */

    msr_value.low = 0x00000000;
    msr_value.high = 0x00000000;
    msr_write64(MSR_DEVICE_GEODELX_VG, MSR_GEODELINK_DIAG, &msr_value);

    /* DISABLE GLCP ACTIONS */

    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_DIAGCTL, &msr_value);

    return crc;
}

/*---------------------------------------------------------------------------
 * vg_get_scaler_filter_coefficients
 *
 * This routine gets the vertical and horizontal filter coefficients for
 * graphics scaling.  The coefficients are sign extended to 32-bit values.
 *--------------------------------------------------------------------------*/

int
vg_get_scaler_filter_coefficients(long h_taps[][5], long v_taps[][3])
{
    unsigned long irqfilt, i;
    unsigned long temp;
    long coeff0, coeff1, coeff2;
    unsigned long lock;

    /* ENABLE ACCESS TO THE HORIZONTAL COEFFICIENTS */

    lock = READ_REG32(DC3_UNLOCK);
    irqfilt = READ_REG32(DC3_IRQ_FILT_CTL);
    irqfilt |= DC3_IRQFILT_H_FILT_SEL;

    /* WRITE COEFFICIENTS */
    /* Coefficient indexes do not auto-increment, so we must */
    /* write the address for every phase                     */

    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);

    for (i = 0; i < 256; i++) {
        WRITE_REG32(DC3_IRQ_FILT_CTL, ((irqfilt & 0xFFFFFF00L) | i));

        temp = READ_REG32(DC3_FILT_COEFF1);
        coeff0 = (temp & 0x3FF);
        coeff1 = (temp >> 10) & 0x3FF;
        coeff2 = (temp >> 20) & 0x3FF;

        h_taps[i][0] = (coeff0 << 22) >> 22;
        h_taps[i][1] = (coeff1 << 22) >> 22;
        h_taps[i][2] = (coeff2 << 22) >> 22;

        temp = READ_REG32(DC3_FILT_COEFF2);
        coeff0 = (temp & 0x3FF);
        coeff1 = (temp >> 10) & 0x3FF;

        h_taps[i][3] = (coeff0 << 22) >> 22;
        h_taps[i][4] = (coeff1 << 22) >> 22;
    }

    /* ENABLE ACCESS TO THE VERTICAL COEFFICIENTS */

    irqfilt &= ~DC3_IRQFILT_H_FILT_SEL;

    /* WRITE COEFFICIENTS */

    for (i = 0; i < 256; i++) {
        WRITE_REG32(DC3_IRQ_FILT_CTL, ((irqfilt & 0xFFFFFF00L) | i));

        temp = READ_REG32(DC3_FILT_COEFF1);
        coeff0 = (temp & 0x3FF);
        coeff1 = (temp >> 10) & 0x3FF;
        coeff2 = (temp >> 20) & 0x3FF;

        v_taps[i][0] = (coeff0 << 22) >> 22;
        v_taps[i][1] = (coeff1 << 22) >> 22;
        v_taps[i][2] = (coeff2 << 22) >> 22;
    }

    WRITE_REG32(DC3_UNLOCK, lock);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vg_get_flicker_filter_configuration
 *
 * This routine returns the current VG flicker filter configuration.
 *--------------------------------------------------------------------------*/

int
vg_get_flicker_filter_configuration(unsigned long *strength, int *flicker_alpha)
{
    unsigned long genlk_ctl;

    if (!strength || !flicker_alpha)
        return CIM_STATUS_INVALIDPARAMS;

    genlk_ctl = READ_REG32(DC3_GENLK_CTL);
    *strength = genlk_ctl & DC3_GC_FLICKER_FILTER_MASK;
    if (genlk_ctl & DC3_GC_ALPHA_FLICK_ENABLE)
        *flicker_alpha = 1;
    else
        *flicker_alpha = 0;

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vg_get_display_pitch
 *
 * This routine returns the current stride between successive lines of frame
 * buffer data.
 *--------------------------------------------------------------------------*/

unsigned long
vg_get_display_pitch(void)
{
    return ((READ_REG32(DC3_GFX_PITCH) & 0x0000FFFF) << 3);
}

/*---------------------------------------------------------------------------
 * vg_get_frame_buffer_line_size
 *
 * This routine returns the current size in bytes of one line of frame buffer
 * data.
 *--------------------------------------------------------------------------*/

unsigned long
vg_get_frame_buffer_line_size(void)
{
    return ((READ_REG32(DC3_LINE_SIZE) & 0x3FF) << 3);
}

/*---------------------------------------------------------------------------
 * vg_get_current_vline
 *
 * This routine returns the number of the current line that is being displayed
 * by the display controller.
 *--------------------------------------------------------------------------*/

unsigned long
vg_get_current_vline(void)
{
    unsigned long current_line;

    /* READ THE REGISTER TWICE TO ENSURE THAT THE VALUE IS NOT TRANSITIONING */

    do {
        current_line = READ_REG32(DC3_LINE_CNT_STATUS) & DC3_LNCNT_V_LINE_CNT;
    }
    while (current_line !=
           (READ_REG32(DC3_LINE_CNT_STATUS) & DC3_LNCNT_V_LINE_CNT));

    return (current_line >> 16);
}

/*---------------------------------------------------------------------------
 * vg_get_display_offset
 *
 * This routine returns the offset into the frame buffer for the first pixel
 * of the display.
 *--------------------------------------------------------------------------*/

unsigned long
vg_get_display_offset(void)
{
    return (READ_REG32(DC3_FB_ST_OFFSET) & 0x0FFFFFFF);
}

/*---------------------------------------------------------------------------
 * vg_get_cursor_info
 *
 * This routine returns the current settings for the hardware cursor.
 *--------------------------------------------------------------------------*/

int
vg_get_cursor_info(VG_CURSOR_DATA *cursor_data)
{
    unsigned long temp;

    /* CURSOR OFFSET */

    cursor_data->cursor_offset = READ_REG32(DC3_CURS_ST_OFFSET) & 0x0FFFFFFF;

    /* CURSOR X POSITION */

    temp = READ_REG32(DC3_CURSOR_X);
    cursor_data->cursor_x = temp & 0x7FF;
    cursor_data->clipx = (temp >> 11) & 0x3F;

    /* CURSOR Y POSITION */

    temp = READ_REG32(DC3_CURSOR_Y);
    cursor_data->cursor_y = temp & 0x7FF;
    cursor_data->clipy = (temp >> 11) & 0x3F;

    /* CURSOR COLORS */

    WRITE_REG32(DC3_PAL_ADDRESS, 0x100);
    cursor_data->mono_color0 = READ_REG32(DC3_PAL_DATA);
    cursor_data->mono_color1 = READ_REG32(DC3_PAL_DATA);

    /* CURSOR ENABLES */

    temp = READ_REG32(DC3_GENERAL_CFG);
    if (temp & DC3_GCFG_CURE)
        cursor_data->enable = 1;
    else
        cursor_data->enable = 0;
    if (temp & DC3_GCFG_CLR_CUR)
        cursor_data->color_cursor = 1;
    else
        cursor_data->color_cursor = 0;

    return CIM_STATUS_OK;
}

/*----------------------------------------------------------------------------
 * vg_get_display_palette_entry
 *
 * This routine reads a single entry in the 8BPP display palette.
 *--------------------------------------------------------------------------*/

int
vg_get_display_palette_entry(unsigned long index, unsigned long *entry)
{
    if (index > 0xFF)
        return CIM_STATUS_INVALIDPARAMS;

    WRITE_REG32(DC3_PAL_ADDRESS, index);
    *entry = READ_REG32(DC3_PAL_DATA);

    return CIM_STATUS_OK;
}

/*----------------------------------------------------------------------------
 * vg_get_border_color
 *
 * This routine reads the current border color for centered displays.
 *--------------------------------------------------------------------------*/

unsigned long
vg_get_border_color(void)
{
    WRITE_REG32(DC3_PAL_ADDRESS, 0x104);
    return READ_REG32(DC3_PAL_DATA);
}

/*----------------------------------------------------------------------------
 * vg_get_display_palette
 *
 * This routines reads the entire contents of the display palette into a
 * buffer.  The display palette consists of 256 X:R:G:B values.
 *--------------------------------------------------------------------------*/

int
vg_get_display_palette(unsigned long *palette)
{
    unsigned long i;

    if (palette) {
        WRITE_REG32(DC3_PAL_ADDRESS, 0);
        for (i = 0; i < 256; i++) {
            palette[i] = READ_REG32(DC3_PAL_DATA);
        }
        return CIM_STATUS_OK;
    }
    return CIM_STATUS_INVALIDPARAMS;
}

/*----------------------------------------------------------------------------
 * vg_get_compression_info
 *
 * This routines reads the current status of the display compression hardware.
 *--------------------------------------------------------------------------*/

int
vg_get_compression_info(VG_COMPRESSION_DATA *comp_data)
{
    comp_data->compression_offset = READ_REG32(DC3_CB_ST_OFFSET) & 0x0FFFFFFF;
    comp_data->pitch = (READ_REG32(DC3_GFX_PITCH) >> 13) & 0x7FFF8;
    comp_data->size = ((READ_REG32(DC3_LINE_SIZE) >> (DC3_LINE_SIZE_CB_SHIFT -
                                                      3)) & 0x3F8) + 24;

    return CIM_STATUS_OK;
}

/*----------------------------------------------------------------------------
 * vg_get_compression_enable
 *
 * This routines reads the current enable status of the display compression
 * hardware.
 *--------------------------------------------------------------------------*/

int
vg_get_compression_enable(void)
{
    if (READ_REG32(DC3_GENERAL_CFG) & DC3_GCFG_CMPE)
        return 1;

    return 0;
}

/*----------------------------------------------------------------------------
 * vg_get_valid_bit
 *--------------------------------------------------------------------------*/

int
vg_get_valid_bit(int line)
{
    unsigned long offset;
    unsigned long valid;
    unsigned long lock;

    lock = READ_REG32(DC3_UNLOCK);
    offset = READ_REG32(DC3_PHY_MEM_OFFSET) & 0xFF000000;
    offset |= line;

    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);
    WRITE_REG32(DC3_PHY_MEM_OFFSET, offset);
    WRITE_REG32(DC3_UNLOCK, lock);
    valid = READ_REG32(DC3_DV_ACC) & 2;

    if (valid)
        return 1;
    return 0;
}

#endif
