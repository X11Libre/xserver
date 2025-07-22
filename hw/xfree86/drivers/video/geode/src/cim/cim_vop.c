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
  * Cimarron VOP configuration routines.
  */

/*---------------------------------------------------------------------------
 * vop_set_vbi_window
 *
 * This routine configures the output position and location in memory of
 * VBI data.
 *--------------------------------------------------------------------------*/

int
vop_set_vbi_window(VOPVBIWINDOWBUFFER *buffer)
{
    unsigned long unlock, temp;
    unsigned long hstart, hstop;
    unsigned long htotal, hsyncstart;

    if (!buffer)
        return CIM_STATUS_INVALIDPARAMS;

    unlock = READ_REG32(DC3_UNLOCK);
    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);

    /* PROGRAM HORIZONTAL POSITION
     * The horizontal position is a little tricky.  The counter for the
     * horizontal timings is reused for the VBI counter.  Consequently, the
     * horizontal start and stop values are based off the beginning of active
     * data.  However, the VG has a quirk.  If the counter start position is
     * before the beginning of HSync, it applies to the previous line.  If
     * the counter is after the beginning of HSync it applies to the current
     * line.  So, for one line the real range can be thought of as
     * HSync_start to (HSync_start + htotal - 1).  However, the counters
     * must be between 0 and htotal - 1.  When placing VBI data before the
     * start of active data, the horizontal end position will thus be *less*
     * than the horizontal start.
     */

    htotal = ((READ_REG32(DC3_H_ACTIVE_TIMING) >> 16) & 0xFFF) + 1;
    hsyncstart = (READ_REG32(DC3_H_SYNC_TIMING) & 0xFFF) + 1;

    if (buffer->horz_from_hsync) {
        /* VERIFY THAT THE INPUT IS VALID */

        if (buffer->horz_start < 0
            || (buffer->horz_start + buffer->vbi_width) > htotal)
            return CIM_STATUS_INVALIDPARAMS;

        hstart = buffer->horz_start + hsyncstart;
    }
    else {
        /* VERIFY THAT THE INPUT IS VALID */

        if (buffer->horz_start < ((long) hsyncstart - (long) htotal) ||
            buffer->horz_start > (long) hsyncstart ||
            buffer->vbi_width > htotal) {
            return CIM_STATUS_INVALIDPARAMS;
        }

        hstart = buffer->horz_start + htotal;
    }

    hstop = hstart + buffer->vbi_width;
    if (hstart > htotal)
        hstart -= htotal;
    if (hstop > htotal)
        hstop -= htotal;
    hstart--;
    hstop--;
    WRITE_REG32(DC3_VBI_HOR, ((hstop << DC3_VBI_HOR_END_SHIFT) &
                              DC3_VBI_HOR_END_MASK) | (hstart &
                                                       DC3_VBI_HOR_START_MASK));

    /* WRITE LINE CAPTURE MASKS */

    WRITE_REG32(DC3_VBI_LN_ODD, ((buffer->odd_line_offset <<
                                  DC3_VBI_ODD_LINE_SHIFT) &
                                 DC3_VBI_ODD_LINE_MASK) |
                (buffer->odd_line_capture_mask & DC3_VBI_ODD_ENABLE_MASK));

    WRITE_REG32(DC3_VBI_LN_EVEN, ((buffer->even_line_offset <<
                                   DC3_VBI_EVEN_LINE_SHIFT) &
                                  DC3_VBI_EVEN_LINE_MASK) |
                (buffer->even_line_capture_mask & DC3_VBI_EVEN_ENABLE_MASK));

    /* PROGRAM SOURCE OFFSETS
     * Start with the even offsets.  Note that we always enable 16-bit VBI,
     * as this is the only way to get VBI data on each VOP clock.
     */

    temp = READ_REG32(DC3_VBI_EVEN_CTL) & ~DC3_VBI_EVEN_CTL_OFFSET_MASK;
    temp |= DC3_VBI_EVEN_CTL_ENABLE_16;
    if (buffer->enable_upscale)
        temp |= DC3_VBI_EVEN_CTL_UPSCALE;
    WRITE_REG32(DC3_VBI_EVEN_CTL, temp |
                (buffer->even_address_offset & DC3_VBI_EVEN_CTL_OFFSET_MASK));

    /* ODD OFFSET */

    temp = READ_REG32(DC3_VBI_ODD_CTL) & ~DC3_VBI_ODD_CTL_OFFSET_MASK;
    WRITE_REG32(DC3_VBI_ODD_CTL, temp |
                (buffer->odd_address_offset & DC3_VBI_ODD_CTL_OFFSET_MASK));

    /* PITCH */

    temp = ((buffer->data_size >> 3) << 16) | ((buffer->data_pitch >> 3) &
                                               0x0000FFFF);
    WRITE_REG32(DC3_VBI_PITCH, temp);

    WRITE_REG32(DC3_UNLOCK, unlock);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vop_enable_vbi_output
 *
 * This routine enables/disables VBI fetching inside the video generator.
 *--------------------------------------------------------------------------*/

int
vop_enable_vbi_output(int enable)
{
    unsigned long unlock, temp;

    unlock = READ_REG32(DC3_UNLOCK);
    temp = READ_REG32(DC3_VBI_EVEN_CTL);

    if (enable)
        temp |= DC3_VBI_ENABLE;
    else
        temp &= ~DC3_VBI_ENABLE;

    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);
    WRITE_REG32(DC3_VBI_EVEN_CTL, temp);
    WRITE_REG32(DC3_UNLOCK, unlock);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vop_set_configuration
 *
 * This routine is passed a VOP_CONFIGURATION structure that contains all
 * the necessary information to configure VOP output.
 *--------------------------------------------------------------------------*/

int
vop_set_configuration(VOPCONFIGURATIONBUFFER *config)
{
    unsigned long vop_config = 0;
    unsigned long alpha, control2;
    unsigned long unlock;
    unsigned long delta;
    Q_WORD msr_value = { 0, 0 };
    int rgb = 0;

    if (!config)
        return CIM_STATUS_INVALIDPARAMS;

    unlock = READ_REG32(DC3_UNLOCK);
    delta = READ_REG32(DC3_VID_DS_DELTA) & DC3_DS_DELTA_MASK;

    /* OVERRIDE THE OUTPUT SETTINGS TO ENABLE VOP OUTPUT */

    if (config->mode != VOP_MODE_DISABLED) {
        msr_read64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_CONFIG, &msr_value);
        msr_value.low &= ~DF_CONFIG_OUTPUT_MASK;
        msr_value.low |= DF_OUTPUT_VOP;
        msr_write64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_CONFIG, &msr_value);
    }

    /* SET THE UNIVERSAL VOP OPTIONS */

    if (config->flags & VOP_FLAG_SWAP_UV)
        vop_config |= VOP_CONFIG_SWAPUV;
    if (config->flags & VOP_FLAG_SWAP_VBI)
        vop_config |= VOP_CONFIG_SWAPVBI;

    /* SET THE MODE SPECIFIC PARAMETERS */

    if (config->mode == VOP_MODE_601) {
        vop_config |= config->vop601.flags;
        vop_config |= config->vop601.vsync_shift;
        vop_config |= VOP_CONFIG_ENABLE_601 | VOP_CONFIG_VIP2_0;

        switch (config->vop601.output_mode) {
        case VOP_601_YUV_16BIT:
            vop_config |= VOP_CONFIG_VIP2_16BIT;
            break;
        case VOP_601_YUV_4_4_4:
            vop_config |= VOP_CONFIG_DISABLE_DECIMATE;
            break;
        case VOP_601_RGB_8_8_8:
            vop_config |= VOP_CONFIG_DISABLE_DECIMATE | VOP_CONFIG_RGBMODE;
            rgb = 1;
            break;
        }

        if (config->vop601.vsync_shift == VOP_VSYNC_LATER_BY_X) {
            delta |= (config->vop601.vsync_shift_count &
                      DC3_601_VSYNC_SHIFT_MASK);
            delta |= DC3_601_VSYNC_SHIFT_ENABLE;
        }
    }
    else {
        if (config->flags & VOP_FLAG_VBI)
            vop_config |= VOP_CONFIG_VBI;
        if (config->flags & VOP_FLAG_TASK)
            vop_config |= VOP_CONFIG_TASK;
        if (config->flags & VOP_FLAG_SINGLECHIPCOMPAT)
            vop_config |= VOP_CONFIG_SC_COMPATIBLE;
        if (config->flags & VOP_FLAG_EXTENDEDSAV)
            vop_config |= VOP_CONFIG_EXTENDED_SAV;

        switch (config->mode) {
        case VOP_MODE_DISABLED:
            vop_config |= VOP_CONFIG_DISABLED;
            break;
        case VOP_MODE_VIP11:
            vop_config |= VOP_CONFIG_VIP1_1;
            break;
        case VOP_MODE_CCIR656:
            vop_config |= VOP_CONFIG_CCIR656;
            break;
        case VOP_MODE_VIP20_8BIT:
            vop_config |= VOP_CONFIG_VIP2_0;
            break;
        case VOP_MODE_VIP20_16BIT:
            vop_config |= VOP_CONFIG_VIP2_0 | VOP_CONFIG_VIP2_16BIT;
            break;
        }
    }

    /* SET THE 4:4:4 TO 4:2:2 DECIMATION ALGORITHM */

    vop_config |= (config->conversion_mode);

    /* SET THE VSYNC OUT OPTIONS */

    control2 = READ_VIP32(VIP_CONTROL2) & ~VIP_CONTROL2_SYNC2PIN_MASK;
    control2 |= config->vsync_out;
    WRITE_VIP32(VIP_CONTROL2, control2);

    /* FORCE THE CORRECT VOP COLOR SPACE */
    /* The output of the mixer will be either RGB or YUV.  We must enable */
    /* or disable the VOP CSC based on the desired output format.         */

    alpha = READ_VID32(DF_VID_ALPHA_CONTROL);
    if (!(alpha & DF_CSC_GRAPHICS_RGB_TO_YUV)) {
        /* RGB OUTPUT FROM THE MIXER */

        if (!rgb)
            alpha |= DF_CSC_VOP_RGB_TO_YUV;
        else
            alpha &= ~DF_CSC_VOP_RGB_TO_YUV;
    }
    else {
        /* YUV OUTPUT FROM THE MIXER */
        /* As there is no YUV->RGB VOP conversion, we simply disable the */
        /* VOP CSC and trust that the user is competent.                 */

        alpha &= ~DF_CSC_VOP_RGB_TO_YUV;
    }

    /* AND WRITE THE CONFIGURATION */

    WRITE_VID32(DF_VID_ALPHA_CONTROL, alpha);
    WRITE_VOP32(VOP_CONFIGURATION, vop_config);
    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);
    WRITE_REG32(DC3_VID_DS_DELTA, delta);
    WRITE_REG32(DC3_UNLOCK, unlock);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vop_save_state
 *
 * This routine saves the necessary register contents in order to restore
 * at a later point to the same state.  Note that the capture state is
 * forced to OFF in this routine.
 *--------------------------------------------------------------------------*/

int
vop_save_state(VOPSTATEBUFFER *save_buffer)
{
    if (!save_buffer)
        return CIM_STATUS_INVALIDPARAMS;

    save_buffer->config = READ_VOP32(VOP_CONFIGURATION);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vop_restore_state
 *
 * This routine restores the state of the vop registers - which were
 * previously saved using vop_save_state.
 *--------------------------------------------------------------------------*/

int
vop_restore_state(VOPSTATEBUFFER *restore_buffer)
{
    if (!restore_buffer)
        return CIM_STATUS_INVALIDPARAMS;

    WRITE_VOP32(VOP_CONFIGURATION, restore_buffer->config);

    return CIM_STATUS_OK;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * CIMARRON VOP READ ROUTINES
 * These routines are included for use in diagnostics or when debugging.  They
 * can be optionally excluded from a project.
 *++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#if CIMARRON_INCLUDE_VOP_READ_ROUTINES

/*---------------------------------------------------------------------------
 * vop_get_current_mode
 *
 * This routine reads the current VIP operating mode and stores it in the
 * passed VOP_CONFIGURATION structure.
 *--------------------------------------------------------------------------*/

int
vop_get_current_mode(VOPCONFIGURATIONBUFFER *config)
{
    unsigned long vop_config = 0;
    unsigned long alpha;

    if (!config)
        return CIM_STATUS_INVALIDPARAMS;

    vop_config = READ_VOP32(VOP_CONFIGURATION);
    alpha = READ_VID32(DF_VID_ALPHA_CONTROL);

    /* READ THE CURRENT MODE */

    switch (vop_config & VOP_CONFIG_MODE_MASK) {
    case VOP_CONFIG_DISABLED:
        config->mode = VOP_MODE_DISABLED;
        break;
    case VOP_CONFIG_VIP1_1:
        config->mode = VOP_MODE_VIP11;
        break;
    case VOP_CONFIG_CCIR656:
        config->mode = VOP_MODE_CCIR656;
        break;
    case VOP_CONFIG_VIP2_0:

        if (vop_config & VOP_CONFIG_ENABLE_601)
            config->mode = VOP_MODE_601;
        else if (vop_config & VOP_CONFIG_VIP2_16BIT)
            config->mode = VOP_MODE_VIP20_16BIT;
        else
            config->mode = VOP_MODE_VIP20_8BIT;
        break;
    }

    /* READ 601 SETTINGS */

    config->vop601.flags = vop_config & (VOP_CONFIG_INVERT_DISPE |
                                         VOP_CONFIG_INVERT_HSYNC |
                                         VOP_CONFIG_INVERT_VSYNC);

    config->vop601.vsync_shift = vop_config & VOP_CONFIG_VSYNC_MASK;
    config->vop601.vsync_shift_count =
        READ_REG32(DC3_VID_DS_DELTA) & DC3_601_VSYNC_SHIFT_MASK;

    if ((alpha & DF_CSC_GRAPHICS_RGB_TO_YUV) || (alpha & DF_CSC_VOP_RGB_TO_YUV)) {
        /* YUV OUTPUT */

        if (vop_config & VOP_CONFIG_DISABLE_DECIMATE)
            config->vop601.output_mode = VOP_601_YUV_4_4_4;
        else if (vop_config & VOP_CONFIG_VIP2_16BIT)
            config->vop601.output_mode = VOP_601_YUV_16BIT;
        else
            config->vop601.output_mode = VOP_601_YUV_8BIT;
    }
    else {
        config->vop601.output_mode = VOP_601_RGB_8_8_8;
    }

    config->flags = 0;

    /* READ THE UNIVERSAL VOP OPTIONS */

    if (vop_config & VOP_CONFIG_SWAPUV)
        config->flags |= VOP_FLAG_SWAP_UV;
    if (vop_config & VOP_CONFIG_SWAPVBI)
        config->flags |= VOP_FLAG_SWAP_VBI;
    if (vop_config & VOP_CONFIG_VBI)
        config->flags |= VOP_FLAG_VBI;
    if (vop_config & VOP_CONFIG_TASK)
        config->flags |= VOP_FLAG_TASK;
    if (vop_config & VOP_CONFIG_SC_COMPATIBLE)
        config->flags |= VOP_FLAG_SINGLECHIPCOMPAT;
    if (vop_config & VOP_CONFIG_EXTENDED_SAV)
        config->flags |= VOP_FLAG_EXTENDEDSAV;

    config->conversion_mode = vop_config & VOP_CONFIG_422_MASK;

    config->vsync_out = READ_VIP32(VIP_CONTROL2) & VIP_CONTROL2_SYNC2PIN_MASK;

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vop_get_vbi_window
 *
 * This routine reads the current VBI configuration for VOP output.
 *--------------------------------------------------------------------------*/

int
vop_get_vbi_configuration(VOPVBIWINDOWBUFFER *buffer)
{
    unsigned long temp;
    unsigned long hstart, hstop;
    unsigned long htotal, hsyncstart;

    if (!buffer)
        return CIM_STATUS_INVALIDPARAMS;

    htotal = ((READ_REG32(DC3_H_ACTIVE_TIMING) >> 16) & 0xFFF) + 1;
    hsyncstart = (READ_REG32(DC3_H_SYNC_TIMING) & 0xFFF) + 1;

    /* DECODE HORIZONTAL POSITION */
    /* This is done according to the requested horizontal origin */

    temp = READ_REG32(DC3_VBI_HOR);
    hstart = (temp & DC3_VBI_HOR_START_MASK) + 1;
    hstop = ((temp & DC3_VBI_HOR_END_MASK) >> DC3_VBI_HOR_END_SHIFT) + 1;
    if (buffer->horz_from_hsync) {
        buffer->horz_start = hstart + htotal - hsyncstart;
        if (buffer->horz_start >= (long) htotal)
            buffer->horz_start -= htotal;
    }
    else {
        if (hstart > hsyncstart)
            buffer->horz_start = (long) hstart - (long) htotal;
        else
            buffer->horz_start = hstart;
    }

    if (hstop > hstart)
        buffer->vbi_width = hstop - hstart;
    else
        buffer->vbi_width = (htotal - hstart) + hstop;

    /* READ LINE MASKS */

    temp = READ_REG32(DC3_VBI_LN_ODD);
    buffer->odd_line_offset = (temp & DC3_VBI_ODD_LINE_MASK) >>
        DC3_VBI_ODD_LINE_SHIFT;
    buffer->odd_line_capture_mask = (temp & DC3_VBI_ODD_ENABLE_MASK);

    temp = READ_REG32(DC3_VBI_LN_EVEN);
    buffer->even_line_offset = (temp & DC3_VBI_EVEN_LINE_MASK) >>
        DC3_VBI_EVEN_LINE_SHIFT;
    buffer->even_line_capture_mask = (temp & DC3_VBI_EVEN_ENABLE_MASK);

    /* READ VBI UPSCALE SETTINGS */

    buffer->enable_upscale = 0;
    temp = READ_REG32(DC3_VBI_EVEN_CTL);
    if (temp & DC3_VBI_EVEN_CTL_UPSCALE)
        buffer->enable_upscale = 1;

    /* READ SOURCE OFFSETS */

    buffer->even_address_offset = temp & DC3_VBI_EVEN_CTL_OFFSET_MASK;
    buffer->odd_address_offset =
        READ_REG32(DC3_VBI_ODD_CTL) & DC3_VBI_ODD_CTL_OFFSET_MASK;

    /* PITCH AND SIZE */

    temp = READ_REG32(DC3_VBI_PITCH);
    buffer->data_size = (temp >> 16) << 3;
    buffer->data_pitch = (temp & 0xFFFF);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vop_get_vbi_enable
 *
 * This routine reads the current enable status of VBI output.
 *--------------------------------------------------------------------------*/

int
vop_get_vbi_enable(void)
{
    if (READ_REG32(DC3_VBI_EVEN_CTL) & DC3_VBI_ENABLE)
        return 1;

    return 0;
}

/*---------------------------------------------------------------------------
 * vop_get_crc
 *
 * This routine returns a CRC of the current VOP data
 --------------------------------------------------------------------------*/

unsigned long
vop_get_crc(void)
{
    unsigned long crc;
    unsigned long config = READ_VOP32(VOP_CONFIGURATION);
    unsigned long timeout = 1000;

    if (!(READ_REG32(DC3_DISPLAY_CFG) & DC3_DCFG_TGEN))
        return 0xFFFFFFFF;

    /* RESET CRC */

    WRITE_VOP32(VOP_CONFIGURATION, config & ~VOP_CONFIG_ENABLE_SIGNATURE);

    /* WAIT FOR THE RESET TO BE LATCHED */

    while ((READ_VOP32(VOP_SIGNATURE) != 0x00000001) && timeout)
        timeout--;

    WRITE_VOP32(VOP_CONFIGURATION, config | VOP_CONFIG_ENABLE_SIGNATURE);

    /* WAIT UNTIL NOT ACTIVE, THEN ACTIVE, NOT ACTIVE, THEN ACTIVE */

    while (!(READ_VOP32(VOP_CONFIGURATION) & VOP_CONFIG_SIGVAL));

    crc = READ_VOP32(VOP_SIGNATURE);

    return crc;
}

/*---------------------------------------------------------------------------
 * vop_read_vbi_crc
 *
 * This routine returns a CRC of the current VBI data
 ---------------------------------------------------------------------------*/

unsigned long
vop_read_vbi_crc(void)
{
    unsigned long gcfg, unlock, vbi_even;
    unsigned long crc;

    if (!(READ_REG32(DC3_DISPLAY_CFG) & DC3_DCFG_TGEN) ||
        !(READ_REG32(DC3_VBI_EVEN_CTL) & DC3_VBI_ENABLE)) {
        return 0xFFFFFFFF;
    }

    unlock = READ_REG32(DC3_UNLOCK);
    gcfg = READ_REG32(DC3_GENERAL_CFG);
    vbi_even = READ_REG32(DC3_VBI_EVEN_CTL);

    gcfg |= DC3_GCFG_SGRE | DC3_GCFG_CRC_MODE;
    gcfg &= ~(DC3_GCFG_SGFR | DC3_GCFG_SIG_SEL);
    vbi_even |= DC3_VBI_EVEN_ENABLE_CRC;

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

#endif
