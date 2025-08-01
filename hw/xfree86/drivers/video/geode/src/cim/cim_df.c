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
  * Cimarron display filter routines.  These routines program the video
  * hardware.
  */

/*---------------------------------------------------------------------------
 * df_set_crt_enable
 *
 * This routine enables or disables CRT output.
 *--------------------------------------------------------------------------*/

int
df_set_crt_enable(int crt_output)
{
    unsigned long config, misc;

    config = READ_VID32(DF_DISPLAY_CONFIG);
    misc = READ_VID32(DF_VID_MISC);

    switch (crt_output) {
        /* DISABLE DISPLAY */

    case DF_CRT_DISABLE:

        config &= ~(DF_DCFG_DIS_EN | DF_DCFG_HSYNC_EN |
                    DF_DCFG_VSYNC_EN | DF_DCFG_DAC_BL_EN);
        misc |= DF_DAC_POWER_DOWN;
        break;

        /* ENABLE THE DISPLAY */

    case DF_CRT_ENABLE:

        config |= (DF_DCFG_DIS_EN | DF_DCFG_HSYNC_EN |
                   DF_DCFG_VSYNC_EN | DF_DCFG_DAC_BL_EN);
        misc &= ~(DF_DAC_POWER_DOWN | DF_ANALOG_POWER_DOWN);
        break;

        /* HSYNC:OFF VSYNC:ON */

    case DF_CRT_STANDBY:

        config = (config & ~(DF_DCFG_DIS_EN | DF_DCFG_HSYNC_EN |
                             DF_DCFG_DAC_BL_EN)) | DF_DCFG_VSYNC_EN;
        misc |= DF_DAC_POWER_DOWN;
        break;

        /* HSYNC:ON VSYNC:OFF */

    case DF_CRT_SUSPEND:

        config = (config & ~(DF_DCFG_DIS_EN | DF_DCFG_VSYNC_EN |
                             DF_DCFG_DAC_BL_EN)) | DF_DCFG_HSYNC_EN;
        misc |= DF_DAC_POWER_DOWN;
        break;

    default:
        return CIM_STATUS_INVALIDPARAMS;
    }

    WRITE_VID32(DF_DISPLAY_CONFIG, config);
    WRITE_VID32(DF_VID_MISC, misc);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_set_panel_enable
 *
 * This routine enables or disables panel output.
 *--------------------------------------------------------------------------*/

int
df_set_panel_enable(int enable)
{
    unsigned long pm;

    pm = READ_VID32(DF_POWER_MANAGEMENT);

    if (enable)
        pm |= DF_PM_PANEL_ON;
    else
        pm &= ~DF_PM_PANEL_ON;

    WRITE_VID32(DF_POWER_MANAGEMENT, pm);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_configure_video_source
 *
 * This routine initializes all aspects of the source buffer for a video overlay.
 *--------------------------------------------------------------------------*/

int
df_configure_video_source(DF_VIDEO_SOURCE_PARAMS *video_source_odd,
                          DF_VIDEO_SOURCE_PARAMS *video_source_even)
{
    unsigned long pitch, ctrl, vcfg;
    unsigned long lock, vg_line, gcfg;
    unsigned long width, size, scale;
    unsigned long misc;

    lock = READ_REG32(DC3_UNLOCK);
    vg_line = READ_REG32(DC3_LINE_SIZE);
    gcfg = READ_REG32(DC3_GENERAL_CFG);
    vcfg = READ_VID32(DF_VIDEO_CONFIG);
    ctrl = READ_VID32(DF_VID_ALPHA_CONTROL);
    scale = READ_VID32(DF_VIDEO_SCALER);

    /* STORE THE DESIRED SCALING PROCEDURE */
    /* Cimarron supports two modes when programming the scale and position  */
    /* of the video window.  The first mode is designed to implicitly apply */
    /* the graphics scale to any video operations.  The second applies the  */
    /* video unchanged, allowing complete control by the user.  To allow    */
    /* visibility between modules, the current mode is stored in a spare    */
    /* bit in the DF miscellaneous register.                                */

    misc = READ_VID32(DF_VID_MISC);
    if (video_source_odd->flags & DF_SOURCEFLAG_IMPLICITSCALING)
        misc |= DF_USER_IMPLICIT_SCALING;
    else
        misc &= DF_USER_IMPLICIT_SCALING;
    WRITE_VID32(DF_VID_MISC, misc);

    /* PARAMETER - VIDEO PITCH */

    pitch =
        (video_source_odd->y_pitch >> 3) | ((video_source_odd->uv_pitch >> 3) <<
                                            16);

    /* PARAMETER - VIDEO FORMAT */

    gcfg &= ~DC3_GCFG_YUV_420;
    vcfg &= ~(DF_VCFG_VID_INP_FORMAT | DF_VCFG_4_2_0_MODE);
    ctrl &= ~(DF_VIDEO_INPUT_IS_RGB | DF_CSC_VIDEO_YUV_TO_RGB | DF_HD_VIDEO |
              DF_YUV_CSC_EN);

    /* SELECT PIXEL ORDERING */

    switch (video_source_odd->video_format & 3) {
    case 0:
        vcfg |= DF_VCFG_UYVY_FORMAT;
        break;
    case 1:
        vcfg |= DF_VCFG_Y2YU_FORMAT;
        break;
    case 2:
        vcfg |= DF_VCFG_YUYV_FORMAT;
        break;
    case 3:
        vcfg |= DF_VCFG_YVYU_FORMAT;
        break;
    }

    /* SELECT SOURCE FORMAT (4:2:2, 4:2:0, RGB) */

    switch (video_source_odd->video_format >> 2) {
    case 0:
        ctrl |= DF_CSC_VIDEO_YUV_TO_RGB;
        break;

    case 1:
        ctrl |= DF_CSC_VIDEO_YUV_TO_RGB;
        vcfg |= DF_VCFG_4_2_0_MODE;
        gcfg |= DC3_GCFG_YUV_420;
        break;

    case 2:
        ctrl |= DF_VIDEO_INPUT_IS_RGB;
        break;

    default:
        return CIM_STATUS_INVALIDPARAMS;
    }

    /* ALIGN TO APPROPRIATE OUTPUT COLOR SPACE                             */
    /* We have assumed until this point that the output color space is RGB */
    /* and the input (if YUV) is always SDTV video.                        */

    if (video_source_odd->flags & DF_SOURCEFLAG_HDTVSOURCE)
        ctrl |= DF_HD_VIDEO;

    if (ctrl & DF_CSC_GRAPHICS_RGB_TO_YUV) {
        /* YUV OUTPUT - DISABLE YUV->RGB AND ENABLE YUV->YUV */

        ctrl &= ~DF_CSC_VIDEO_YUV_TO_RGB;

        if ((!(ctrl & DF_HD_VIDEO) && (ctrl & DF_HD_GRAPHICS)) ||
            ((ctrl & DF_HD_VIDEO) && !(ctrl & DF_HD_GRAPHICS))) {
            ctrl |= DF_YUV_CSC_EN;
        }
    }

    /* PARAMETER - DISPLAY FILTER BUFFER SIZE                        */
    /* The line size in the video generator must be 32-byte aligned. */
    /* However, smaller alignments are managed by setting the        */
    /* appropriate pitch and clipping the video window.              */

    vcfg &= ~(DF_VCFG_LINE_SIZE_LOWER_MASK | DF_VCFG_LINE_SIZE_BIT8 |
              DF_VCFG_LINE_SIZE_BIT9);

    size = ((video_source_odd->width >> 1) + 7) & 0xFFF8;

    vcfg |= (size & 0x00FF) << 8;
    if (size & 0x0100)
        vcfg |= DF_VCFG_LINE_SIZE_BIT8;
    if (size & 0x0200)
        vcfg |= DF_VCFG_LINE_SIZE_BIT9;

    scale = (scale & ~0x7FF) | video_source_odd->height;

    /* PARAMETER - VIDEO GENERATOR BUFFER SIZE */

    vg_line &= ~DC3_LINE_SIZE_VLS_MASK;

    if (gcfg & DC3_GCFG_YUV_420)
        width = ((video_source_odd->width >> 1) + 7) & 0xFFF8;
    else
        width = ((video_source_odd->width << 1) + 31) & 0xFFE0;

    vg_line |= (width >> 3) << DC3_LINE_SIZE_VB_SHIFT;

    /* WRITE ALL PARAMETERS AT ONCE */

    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);
    WRITE_VID32(DF_VIDEO_CONFIG, vcfg);
    WRITE_VID32(DF_VID_ALPHA_CONTROL, ctrl);
    WRITE_VID32(DF_VIDEO_SCALER, scale);
    WRITE_REG32(DC3_GENERAL_CFG, gcfg);
    WRITE_REG32(DC3_LINE_SIZE, vg_line);
    WRITE_REG32(DC3_VID_YUV_PITCH, pitch);

    /* WRITE EVEN OR ODD BUFFER OFFSETS                            */
    /* The even buffer is only valid inside an interlaced display. */

    if (READ_REG32(DC3_IRQ_FILT_CTL) & DC3_IRQFILT_INTL_EN) {
        WRITE_REG32(DC3_VID_EVEN_Y_ST_OFFSET, video_source_even->y_offset);
        WRITE_REG32(DC3_VID_EVEN_U_ST_OFFSET, video_source_even->u_offset);
        WRITE_REG32(DC3_VID_EVEN_V_ST_OFFSET, video_source_even->v_offset);
    }

    WRITE_REG32(DC3_VID_Y_ST_OFFSET, video_source_odd->y_offset);
    WRITE_REG32(DC3_VID_U_ST_OFFSET, video_source_odd->u_offset);
    WRITE_REG32(DC3_VID_V_ST_OFFSET, video_source_odd->v_offset);

    WRITE_REG32(DC3_UNLOCK, lock);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_set_video_offsets
 *
 * This routine sets the starting offset for the video buffer(s).  The buffers
 * can also be configured inside df_configure_video_source, but a separate
 * routine is provided here to allow quick buffer flipping.
 *--------------------------------------------------------------------------*/

int
df_set_video_offsets(int even, unsigned long y_offset,
                     unsigned long u_offset, unsigned long v_offset)
{
    unsigned long lock = READ_REG32(DC3_UNLOCK);

    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);

    if (even) {
        WRITE_REG32(DC3_VID_EVEN_Y_ST_OFFSET, y_offset);
        WRITE_REG32(DC3_VID_EVEN_U_ST_OFFSET, u_offset);
        WRITE_REG32(DC3_VID_EVEN_V_ST_OFFSET, v_offset);
    }
    else {
        WRITE_REG32(DC3_VID_Y_ST_OFFSET, y_offset);
        WRITE_REG32(DC3_VID_U_ST_OFFSET, u_offset);
        WRITE_REG32(DC3_VID_V_ST_OFFSET, v_offset);
    }

    WRITE_REG32(DC3_UNLOCK, lock);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_set_video_scale
 *
 * This routine programs the horizontal/vertical scale factors for video.  To
 * disable scaling/filtering, this routine should be called with identical source
 * and destination dimensions.
 *--------------------------------------------------------------------------*/

int
df_set_video_scale(unsigned long src_width, unsigned long src_height,
                   unsigned long dst_width, unsigned long dst_height,
                   unsigned long flags)
{
    unsigned long temp, misc;
    unsigned long scale, gfxscale;
    unsigned long fbactive, src;
    unsigned long size, downscale;
    unsigned long vcfg, gcfg, unlock;

    /* APPLY THE GRAPHICS SCALE */
    /* When requested by the user, we will adjust the video scale by the  */
    /* current graphics scale factor.  This allows video to be programmed */
    /* in terms of the graphics source resolution.                        */

    misc = READ_VID32(DF_VID_MISC);
    if (misc & DF_USER_IMPLICIT_SCALING) {
        gfxscale = READ_REG32(DC3_GFX_SCALE);
        fbactive = READ_REG32(DC3_FB_ACTIVE);

        /* REVERSE ENGINEER THE SCALE FACTOR */
        /* The graphics scale factor is (source / (dst - 1)), so a little */
        /* math is performed to reverse engineer the correct scale for    */
        /* video.                                                         */
        /*                                                                */
        /* F = (0x4000*S)/(D-1)  ->  (D/S) = (((0x4000*S)/F)+1)/S         */

        scale = gfxscale & 0xFFFF;
        src = (fbactive >> 16) + 1;
        if (scale != 0x4000) {
            dst_width = dst_width * (((0x4000 * src) / scale) + 1);
            dst_width /= src;
        }

        scale = gfxscale >> 16;
        src = (fbactive & 0xFFFF) + 1;
        if (scale != 0x4000) {
            dst_height = dst_height * (((0x4000 * src) / scale) + 1);
            dst_height /= src;
        }
    }

    /* CHECK FOR VALID SCALING FACTOR */
    /* The display filter/video generator can support up to 8:1  */
    /* horizontal downscale and up to 4:1 vertical downscale.    */
    /* Scale factors above 4:1 horizontal and 2:1 horizontal     */
    /* will have a quality impact.  However, at such large scale */
    /* factors, it might not matter,                             */

    if (((flags & DF_SCALEFLAG_CHANGEX) && dst_width < (src_width >> 3)) ||
        ((flags & DF_SCALEFLAG_CHANGEY) && dst_height < (src_height >> 2))) {
        return CIM_STATUS_INVALIDSCALE;
    }

    /* ENABLE OR DISABLE ADVANCED SCALING FEATURES          */
    /* Scaling above 2:1 vertical and 4:1 horizontal relies */
    /* on mechanisms beside the line filter.                */

    if (flags & DF_SCALEFLAG_CHANGEX) {
        scale = READ_VID32(DF_VIDEO_SCALER);
        vcfg = READ_VID32(DF_VIDEO_CONFIG);
        vcfg &= ~(DF_VCFG_LINE_SIZE_LOWER_MASK | DF_VCFG_LINE_SIZE_BIT8 |
                  DF_VCFG_LINE_SIZE_BIT9);

        if (dst_width < (src_width >> 2)) {
            src_width >>= 1;
            WRITE_VID32(DF_VIDEO_SCALER, scale | DF_SCALE_DOUBLE_H_DOWNSCALE);
        }
        else {
            WRITE_VID32(DF_VIDEO_SCALER, scale & ~DF_SCALE_DOUBLE_H_DOWNSCALE);
        }

        /* PROGRAM A NEW LINE SIZE */
        /* The line size must be updated when using the Double Horizontal  */
        /* Downscale (DHD) bit.  This is because the amount of VFIFO space */
        /* consumed is effectively half in this mode.                      */

        size = ((src_width >> 1) + 7) & 0xFFF8;
        vcfg |= (size & 0x00FF) << 8;
        if (size & 0x0100)
            vcfg |= DF_VCFG_LINE_SIZE_BIT8;
        if (size & 0x0200)
            vcfg |= DF_VCFG_LINE_SIZE_BIT9;
        WRITE_VID32(DF_VIDEO_CONFIG, vcfg);
        WRITE_VID32(DF_VIDEO_XSCALE, ((0x10000 * src_width) / dst_width));
    }

    if (flags & DF_SCALEFLAG_CHANGEY) {
        unlock = READ_REG32(DC3_UNLOCK);
        gcfg = READ_REG32(DC3_GENERAL_CFG) & ~DC3_GCFG_VDSE;
        WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);
        if (dst_height < (src_height >> 1)) {
            gcfg |= DC3_GCFG_VDSE;
            downscale = READ_REG32(DC3_VID_DS_DELTA) & ~DC3_DS_DELTA_MASK;
            if (dst_height == (src_height >> 2))
                downscale |= (0x3FFF << 18);
            else
                downscale |= (((src_height >> 1) << 14) / dst_height) << 18;

            WRITE_REG32(DC3_VID_DS_DELTA, downscale);
            WRITE_VID32(DF_VIDEO_YSCALE, 0x20000);
        }
        else {
            WRITE_VID32(DF_VIDEO_YSCALE, ((0x10000 * src_height) / dst_height));
        }
        WRITE_REG32(DC3_GENERAL_CFG, gcfg);
        WRITE_REG32(DC3_UNLOCK, unlock);
    }

    /* CHECK IF SCALING IS DISABLED */
    /* If no scaling occurs, we disable the hardware filter. */

    temp = READ_VID32(DF_VIDEO_CONFIG);
    if ((READ_VID32(DF_VIDEO_XSCALE) == 0x10000) &&
        (READ_VID32(DF_VIDEO_YSCALE) == 0x10000)) {
        WRITE_VID32(DF_VIDEO_CONFIG, (temp | DF_VCFG_SC_BYP));
    }
    else
        WRITE_VID32(DF_VIDEO_CONFIG, (temp & ~DF_VCFG_SC_BYP));

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_set_video_position
 *
 * This routine programs the position of the video window on the display.
 * An indent parameter is also passed to this program to prevent artifacts
 * when the video window is moved beyond the left edge of the screen.
 *--------------------------------------------------------------------------*/

int
df_set_video_position(DF_VIDEO_POSITION *video_window)
{
    unsigned long vblankstart_even, vblankend_even, vsyncend_even,
        vtotal_even, vactive_even;
    unsigned long hblankstart, hblankend, hsyncend, htotal, hactive;
    unsigned long vblankstart, vblankend, vsyncend, vtotal, vactive;
    unsigned long width, height, height_even;
    unsigned long adjust, border_x, border_y, border_y_even;
    unsigned long xstart, xend;
    unsigned long ystart, yend;
    unsigned long ckey_x, ckey_y;
    unsigned long x_copy, y_copy;
    unsigned long width_copy, height_copy;
    unsigned long vcfg, initread;
    unsigned long xscale, dst_clip;
    unsigned long ypos, ypos_even;
    unsigned long y, gfxscale;
    unsigned long misc, fbactive;
    unsigned long scale, src;
    unsigned long irq_ctl;
    unsigned long unlock;

    hsyncend = ((READ_REG32(DC3_H_SYNC_TIMING) >> 16) & 0xFFF) + 1;
    vsyncend = ((READ_REG32(DC3_V_SYNC_TIMING) >> 16) & 0xFFF) + 1;
    vblankend = ((READ_REG32(DC3_V_BLANK_TIMING) >> 16) & 0xFFF) + 1;
    hblankend = ((READ_REG32(DC3_H_BLANK_TIMING) >> 16) & 0xFFF) + 1;
    htotal = ((READ_REG32(DC3_H_ACTIVE_TIMING) >> 16) & 0xFFF) + 1;
    vtotal = ((READ_REG32(DC3_V_ACTIVE_TIMING) >> 16) & 0xFFF) + 1;
    vblankstart = (READ_REG32(DC3_V_BLANK_TIMING) & 0xFFF) + 1;
    hblankstart = (READ_REG32(DC3_H_BLANK_TIMING) & 0xFFF) + 1;
    hactive = (READ_REG32(DC3_H_ACTIVE_TIMING) & 0xFFF) + 1;
    vactive = (READ_REG32(DC3_V_ACTIVE_TIMING) & 0xFFF) + 1;
    unlock = READ_REG32(DC3_UNLOCK);

    /* INCLUDE BORDER IF REQUESTED */

    if (video_window->flags & DF_POSFLAG_INCLUDEBORDER) {
        border_x = htotal - hblankend;
        border_y = vtotal - vblankend;
        hactive = hblankstart + htotal - hblankend;
        vactive = vblankstart + vtotal - vblankend;
    }
    else {
        border_x = border_y = 0;
    }

    /* APPLY THE GRAPHICS SCALE     */
    /* Do not alter the input data. */

    width_copy = video_window->width;
    height_copy = video_window->height;
    x_copy = video_window->x;
    y_copy = video_window->y;

    misc = READ_VID32(DF_VID_MISC);
    if (misc & DF_USER_IMPLICIT_SCALING) {
        gfxscale = READ_REG32(DC3_GFX_SCALE);
        fbactive = READ_REG32(DC3_FB_ACTIVE);

        /* REVERSE ENGINEER THE SCALE FACTOR */

        scale = gfxscale & 0xFFFF;
        src = (fbactive >> 16) + 1;
        if (scale != 0x4000) {
            width_copy = width_copy * (((0x4000 * src) / scale) + 1);
            width_copy /= src;
            x_copy = x_copy * (((0x4000 * src) / scale) + 1);
            x_copy /= src;
        }

        scale = gfxscale >> 16;
        src = (fbactive & 0xFFFF) + 1;
        if (scale != 0x4000) {
            height_copy = height_copy * (((0x4000 * src) / scale) + 1);
            height_copy /= src;
            y_copy = y_copy * (((0x4000 * src) / scale) + 1);
            y_copy /= src;
        }
    }

    /* HANDLE INTERLACING */
    /* When the output is interlaced, we must set the position and height */
    /* on the fields and not on the composite image.                      */

    if ((irq_ctl = READ_REG32(DC3_IRQ_FILT_CTL)) & DC3_IRQFILT_INTL_EN) {
        vsyncend_even = ((READ_REG32(DC3_V_SYNC_EVEN) >> 16) & 0xFFF) + 1;
        vtotal_even = ((READ_REG32(DC3_V_ACTIVE_EVEN) >> 16) & 0xFFF) + 1;
        vblankend_even = ((READ_REG32(DC3_V_BLANK_EVEN) >> 16) & 0xFFF) + 1;
        vactive_even = (READ_REG32(DC3_V_ACTIVE_EVEN) & 0xFFF) + 1;
        vblankstart_even = (READ_REG32(DC3_V_BLANK_EVEN) & 0xFFF) + 1;

        if (video_window->flags & DF_POSFLAG_INCLUDEBORDER) {
            border_y_even = vtotal_even - vblankend_even;
            vactive_even = vblankstart_even + vtotal_even - vblankend_even;
        }
        else
            border_y_even = 0;

        /*
         * THE ODD FIELD MUST ALWAYS PRECEDE THE EVEN FIELD
         * This implies that we can never start video on an odd y position
         * in the composite image.  This is required because the only way
         * to accomplish an odd y start would be to switch the buffer
         * which could have serious repercussions for genlocked VIP.
         */

        y = y_copy >> 1;

        /* CALCULATE Y POSITION FOR ODD FIELD */
        /* Clip the video window to the odd field timings. Note that the */
        /* height in the odd field may be greater if the video height is */
        /* odd.                                                          */

        height = (height_copy + 1) >> 1;
        if ((y + height) > vactive)
            height = vactive - y;

        ystart = y + vtotal_even - vsyncend_even + 1;
        if (video_window->flags & DF_POSFLAG_INCLUDEBORDER)
            ystart -= border_y_even;

        yend = ystart + height;
        ypos = (yend << 16) | ystart;

        /* CALCULATE Y POSITION FOR EVEN FIELD */

        height_even = height_copy >> 1;
        if ((y + height_even) > vactive_even)
            height_even = vactive_even - y;

        ystart = y + vtotal - vsyncend + 1;
        if (video_window->flags & DF_POSFLAG_INCLUDEBORDER)
            ystart -= border_y;

        yend = ystart + height_even;
        ypos_even = (yend << 16) | ystart;

        /* CALCULATE ACTUAL FRAME BUFFER HEIGHT */
        /* The y position and height are used to determine the actual    */
        /* placement of the color key region.  The region will either be */
        /* the sum of the even and odd fields (for interlaced addressing */
        /* or flicker filtering) or it will be the union of the two (for */
        /* line doubling).  We must also adjust the region such that the */
        /* origin (0, 0) is centered on the beginning of graphics data.  */
        /* This is only a problem if video is being displayed over the   */
        /* overscan area.                                                */

        if ((READ_REG32(DC3_GENLK_CTL) & DC3_GC_FLICKER_FILTER_ENABLE) ||
            (irq_ctl & DC3_IRQFILT_INTL_ADDR)) {
            y <<= 1;
            height += height_even;
            adjust = border_y + border_y_even;
        }
        else {
            adjust = border_y;
            if (height_even > height)
                height = height_even;
        }
        if (video_window->flags & DF_POSFLAG_INCLUDEBORDER) {
            if (y > adjust) {
                y -= adjust;
                adjust = 0;
            }
            else {
                adjust -= y;
                if (height > adjust)
                    height -= adjust;
                else
                    height = 0;
            }
        }

    }
    else {
        y = y_copy;

        height = height_copy;
        if ((y + height) > vactive)
            height = vactive - y;

        ystart = y + vtotal - vsyncend + 1;
        if (video_window->flags & DF_POSFLAG_INCLUDEBORDER)
            ystart -= border_y;

        yend = ystart + height;
        ypos = (yend << 16) | ystart;
        ypos_even = 0;
    }

    /* HORIZONTAL POSITION */
    /* The horizontal values are identical for the even and odd field. */

    width = width_copy;
    xstart = x_copy + htotal - hsyncend - 14;
    if (video_window->flags & DF_POSFLAG_INCLUDEBORDER)
        xstart -= border_x;

    /* RIGHT CLIPPING */

    if ((x_copy + width) > hactive)
        width = hactive - x_copy;

    xend = xstart + width;

    /*
     * CALCULATE LEFT CLIPPING PARAMETER
     * The value passed in can be interpreted as destination pixels, in
     * which case the video scale is factored in, or as source pixels, in
     * which case the value is written directly.  Also, the display filter's
     * initial read address value is only programmable on 4-pixel increments.
     * However, we can achieve an arbitrary left clip by adjusting the
     * xstart value, as there is a 14-clock delay in which to play.  Also,
     * according to the designers, 4:2:0 and 4:2:2 behave identically when
     * setting the initial read address.  The addition of scaling further
     * complicates the algorithm.  When setting the initial read address, it
     * is in terms of source pixels, while adjusting the xstart value is in
     * destination pixels We may thus not be able to achieve a perfect
     * clipping fit for scaled video.  We compensate by including two
     * clipping parameters in our structure.  This allows us the user
     * additional control and it allows us to accurately convey to the user
     * the state of clipping on the machine.
     */

    initread = video_window->left_clip;
    dst_clip = 0;
    if (!(video_window->flags & DF_POSFLAG_DIRECTCLIP)) {
        xscale = READ_VID32(DF_VIDEO_XSCALE) & 0xFFFFF;
        initread = (initread * xscale) / 0x10000;
        if (xscale)
            dst_clip = ((initread & 3) * 0x10000) / xscale;
    }
    else
        dst_clip = video_window->dst_clip;

    /*
     * LIMIT THE CLIP
     * We technically have a 14 pixel window in which to play.  However,
     * taking the entire 14 pixels makes the video timing a little hairy...
     * Also note that we cannot do this when performing panel centering, as
     * the video would then exceed the mode size.
     */

    if (dst_clip > 4)
        dst_clip = 4;
    if (READ_REG32(DC3_DISPLAY_CFG) & DC3_DCFG_DCEN)
        dst_clip = 0;

    xstart -= dst_clip;

    vcfg = READ_VID32(DF_VIDEO_CONFIG);
    vcfg &= ~DF_VCFG_INIT_READ_MASK;
    vcfg |= (initread >> 2) << 16;

    /* SET COLOR KEY REGION */
    /* We are assuming that color keying will never be desired outside   */
    /* of the video region.  We adjust the color key region for graphics */
    /* scaling.                                                          */

    gfxscale = READ_REG32(DC3_GFX_SCALE);

    ckey_x = ((x_copy * (gfxscale & 0xFFFF)) / 0x4000) |
        ((((x_copy + width) * (gfxscale & 0xFFFF)) / 0x4000) << 16);
    ckey_y = ((y * (gfxscale >> 16)) / 0x4000) |
        ((((y + height) * (gfxscale >> 16)) / 0x4000) << 16);

    /* WRITE ALL PARAMETERS AT ONCE */

    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);
    WRITE_REG32(DC3_CLR_KEY_X, ckey_x);
    WRITE_REG32(DC3_CLR_KEY_Y, ckey_y);
    WRITE_VID32(DF_VIDEO_X_POS, (xend << 16) | xstart);
    WRITE_VID32(DF_VIDEO_Y_POS, ypos);
    WRITE_VID32(DF_VID_YPOS_EVEN, ypos_even);
    WRITE_VID32(DF_VIDEO_CONFIG, vcfg);
    WRITE_REG32(DC3_UNLOCK, unlock);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_set_video_filter_coefficients
 *
 * This routine sets the horizontal and vertical filter coefficients for video
 * scaling.  These coefficients are used for upscaling and downscaling video.
 * If the phase256 parameter is 1, the coefficient arrays are used as single
 * arrays of 256 phases for both vertical and horizontal scaling.  If the
 * phase256 parameter is clear, the coefficient arrays are used as two
 * 128-phase arrays.  The first 128 entries represent the phases for
 * vertical scaling.  The last 128 entries represent the phases for
 * horizontal scaling.
 *--------------------------------------------------------------------------*/

int
df_set_video_filter_coefficients(long taps[][4], int phase256)
{
    unsigned long scale, coeff0, coeff1;
    unsigned long i;
    long (*defaults)[2];

    /* SET PHASE COUNT AND CHOOSE COEFFICIENT ARRAY */

    scale = READ_VID32(DF_VIDEO_SCALER);
    if (phase256) {
        WRITE_VID32(DF_VIDEO_SCALER, (scale & ~DF_SCALE_128_PHASES));
        defaults = CimarronVideoFilter256;
    }
    else {
        WRITE_VID32(DF_VIDEO_SCALER, (scale | DF_SCALE_128_PHASES));
        defaults = CimarronVideoFilter128;
    }

    /* PROGRAM COEFFICIENTS */

    for (i = 0; i < 256; i++) {
        if (!taps) {
            coeff0 = defaults[i][0];
            coeff1 = defaults[i][1];
        }
        else {
            if (taps[i][1] < 0)
                coeff0 = -taps[i][1] | 0x8000;
            else
                coeff0 = taps[i][1];

            coeff0 <<= 16;

            if (taps[i][0] < 0)
                coeff0 |= -taps[i][0] | 0x8000;
            else
                coeff0 |= taps[i][0];

            if (taps[i][3] < 0)
                coeff1 = -taps[i][3] | 0x8000;
            else
                coeff1 = taps[i][3];

            coeff1 <<= 16;

            if (taps[i][2] < 0)
                coeff1 |= -taps[i][2] | 0x8000;
            else
                coeff1 |= taps[i][2];
        }

        WRITE_VID32((DF_COEFFICIENT_BASE + (i << 3)), coeff0);
        WRITE_VID32((DF_COEFFICIENT_BASE + (i << 3) + 4), coeff1);
    }

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_set_video_enable
 *
 * This routine enables or disables the video overlay.
 *--------------------------------------------------------------------------*/

int
df_set_video_enable(int enable, unsigned long flags)
{
    unsigned long vcfg, lock, gcfg;
    unsigned long dcfg, vg_ckey, fifo = 0;

    vcfg = READ_VID32(DF_VIDEO_CONFIG);
    lock = READ_REG32(DC3_UNLOCK);
    gcfg = READ_REG32(DC3_GENERAL_CFG);

    /* SET VIDEO FIFO END WATERMARK */
    /* The video FIFO end watermark is set to 0 when video is disabled  */
    /* to allow low priority transactions in the VG.  Otherwise, the    */
    /* priority will be forced high until the VG fills the video FIFO   */
    /* by not fetching video.  That could take a while...  Note that    */
    /* we set the end priority to be 4 greater than the start.  We      */
    /* assume that the start priority has been configured by a modeset. */

    dcfg = READ_REG32(DC3_DISPLAY_CFG) & ~DC3_DCFG_VFHPEL_MASK;
    if (enable) {
        fifo = ((dcfg >> 12) & 0x0000000F) + 4;
        if (fifo > 0xF)
            fifo = 0xF;
    }
    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);
    WRITE_REG32(DC3_DISPLAY_CFG, dcfg | (fifo << 16));

    /* ENABLE OR DISABLE VIDEO */
    /* The mechanism to fetch video data is enabled first and */
    /* disabled last.                                         */

    if (enable) {
        WRITE_REG32(DC3_GENERAL_CFG, (gcfg | DC3_GCFG_VIDE));
        WRITE_VID32(DF_VIDEO_CONFIG, (vcfg | DF_VCFG_VID_EN));

        /* DISABLE COLOR KEYING IF REQUESTED BY THE USER */

        if (flags & DF_ENABLEFLAG_NOCOLORKEY) {
            /* OVERRIDE THE MODE TO COLOR KEYING */

            dcfg = READ_VID32(DF_DISPLAY_CONFIG);
            WRITE_VID32(DF_DISPLAY_CONFIG, (dcfg & ~DF_DCFG_VG_CK));

            /* DISABLE COLOR KEYING IN THE VG */

            vg_ckey = READ_REG32(DC3_COLOR_KEY);
            WRITE_REG32(DC3_COLOR_KEY, (vg_ckey & ~DC3_CLR_KEY_ENABLE));
        }
        else if (!(READ_VID32(DF_DISPLAY_CONFIG) & DF_DCFG_VG_CK)) {
            /* OTHERWISE RE-ENABLE COLOR KEYING */

            vg_ckey = READ_REG32(DC3_COLOR_KEY);
            WRITE_REG32(DC3_COLOR_KEY, (vg_ckey | DC3_CLR_KEY_ENABLE));
        }
    }
    else {
        WRITE_VID32(DF_VIDEO_CONFIG, (vcfg & ~DF_VCFG_VID_EN));
        WRITE_REG32(DC3_GENERAL_CFG, (gcfg & ~DC3_GCFG_VIDE));

        /* DISABLE COLOR KEY WINDOW WHEN VIDEO IS INACTIVE         */
        /* To mimic legacy functionality, we disable color keying  */
        /* when the video window is not active.  We will restore   */
        /* the enable when video is re-enabled if the appropriate  */
        /* bit is set in display config.                           */

        vg_ckey = READ_REG32(DC3_COLOR_KEY);
        WRITE_REG32(DC3_COLOR_KEY, (vg_ckey & ~DC3_CLR_KEY_ENABLE));
    }
    WRITE_REG32(DC3_UNLOCK, lock);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_set_video_color_key
 *
 * This routine configures the video color/chroma key mechanism.
 *--------------------------------------------------------------------------*/

int
df_set_video_color_key(unsigned long key, unsigned long mask, int graphics)
{
    unsigned long lock, vg_ckey, df_dcfg;

    vg_ckey = READ_REG32(DC3_COLOR_KEY);
    lock = READ_REG32(DC3_UNLOCK);
    df_dcfg = READ_VID32(DF_DISPLAY_CONFIG);

    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);

    if (graphics) {
        /* COLOR KEY - USE VG HARDWARE */
        /* Note that color key is never enabled unless a video window */
        /* is active.  This is to match legacy behavior.              */

        df_dcfg &= ~DF_DCFG_VG_CK;
        vg_ckey = (vg_ckey & 0xFF000000) | (key & 0xFFFFFF);
        if (READ_VID32(DF_VIDEO_CONFIG) & DF_VCFG_VID_EN)
            vg_ckey |= DC3_CLR_KEY_ENABLE;
        else
            vg_ckey &= ~DC3_CLR_KEY_ENABLE;

        WRITE_VID32(DF_DISPLAY_CONFIG, df_dcfg);
        WRITE_REG32(DC3_COLOR_KEY, vg_ckey);
        WRITE_REG32(DC3_COLOR_MASK, (mask & 0xFFFFFF));
    }
    else {
        /* CHROMA KEY - USE DF HARDWARE */

        df_dcfg |= DF_DCFG_VG_CK;
        vg_ckey &= ~DC3_CLR_KEY_ENABLE;

        WRITE_REG32(DC3_COLOR_KEY, vg_ckey);
        WRITE_VID32(DF_DISPLAY_CONFIG, df_dcfg);
        WRITE_VID32(DF_VIDEO_COLOR_KEY, (key & 0xFFFFFF));
        WRITE_VID32(DF_VIDEO_COLOR_MASK, (mask & 0xFFFFFF));
    }

    WRITE_REG32(DC3_UNLOCK, lock);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_set_video_palette
 *
 * This routine loads the video hardware palette.  If a NULL pointer is
 * specified, the palette is bypassed.
 *-------------------------------------------------------------------------*/

int
df_set_video_palette(unsigned long *palette)
{
    unsigned long i, entry;
    unsigned long misc, dcfg;

    /* LOAD GEODE LX VIDEO PALETTE */

    WRITE_VID32(DF_PALETTE_ADDRESS, 0);
    for (i = 0; i < 256; i++) {
        if (palette)
            entry = palette[i];
        else
            entry = i | (i << 8) | (i << 16);
        WRITE_VID32(DF_PALETTE_DATA, entry);
    }

    /* ENABLE THE VIDEO PALETTE */
    /* Ensure that the video palette has an effect by routing video data */
    /* through the palette RAM and clearing the 'Bypass Both' bit.       */

    dcfg = READ_VID32(DF_DISPLAY_CONFIG);
    misc = READ_VID32(DF_VID_MISC);

    dcfg |= DF_DCFG_GV_PAL_BYP;
    misc &= ~DF_GAMMA_BYPASS_BOTH;

    WRITE_VID32(DF_DISPLAY_CONFIG, dcfg);
    WRITE_VID32(DF_VID_MISC, misc);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_set_video_palette_entry
 *
 * This routine loads a single entry of the video hardware palette.
 *--------------------------------------------------------------------------*/

int
df_set_video_palette_entry(unsigned long index, unsigned long palette)
{
    unsigned long misc, dcfg;

    if (index > 0xFF)
        return CIM_STATUS_INVALIDPARAMS;

    /* SET A SINGLE ENTRY */

    WRITE_VID32(DF_PALETTE_ADDRESS, index);
    WRITE_VID32(DF_PALETTE_DATA, palette);

    /* ENABLE THE VIDEO PALETTE */
    /* Ensure that the video palette has an effect by routing video data */
    /* through the palette RAM and clearing the 'Bypass Both' bit.       */

    dcfg = READ_VID32(DF_DISPLAY_CONFIG);
    misc = READ_VID32(DF_VID_MISC);

    /* Ensure that the Graphic data passes through the Gamma
     * Correction RAM
     *
     * XXX is this a bug? perhaps it should be setting the bit so that video
     * data is routed, according to the description above.
     * it also mismatches df_set_video_palette().
     */
    dcfg &= ~DF_DCFG_GV_PAL_BYP;

    /* Unset the "bypass both" bit to make sure the above selection (gfx/video
     * data through gamma correction RAM) takes effect.
     */
    misc &= ~DF_GAMMA_BYPASS_BOTH;

    WRITE_VID32(DF_DISPLAY_CONFIG, dcfg);
    WRITE_VID32(DF_VID_MISC, misc);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_configure_video_cursor_color_key
 *
 * This routine configures the hardware video cursor color key mechanism.
 *--------------------------------------------------------------------------*/

int
df_configure_video_cursor_color_key(DF_VIDEO_CURSOR_PARAMS *cursor_color_key)
{
    unsigned long key;

    if (cursor_color_key->select_color2 >= 24)
        return CIM_STATUS_INVALIDPARAMS;

    key = READ_VID32(DF_CURSOR_COLOR_KEY) & DF_CURSOR_COLOR_KEY_ENABLE;
    key =
        key | (cursor_color_key->
               key & 0xFFFFFF) | (cursor_color_key->select_color2 << 24);

    WRITE_VID32(DF_CURSOR_COLOR_KEY, key);
    WRITE_VID32(DF_CURSOR_COLOR_MASK, (cursor_color_key->mask & 0xFFFFFF));
    WRITE_VID32(DF_CURSOR_COLOR_1, (cursor_color_key->color1 & 0xFFFFFF));
    WRITE_VID32(DF_CURSOR_COLOR_2, (cursor_color_key->color2 & 0xFFFFFF));

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_set_video_cursor_color_key_enable
 *
 * This routine enables or disables the video cursor color key.
 *--------------------------------------------------------------------------*/

int
df_set_video_cursor_color_key_enable(int enable)
{
    unsigned long temp = READ_VID32(DF_CURSOR_COLOR_KEY);

    if (enable)
        temp |= DF_CURSOR_COLOR_KEY_ENABLE;
    else
        temp &= ~DF_CURSOR_COLOR_KEY_ENABLE;

    WRITE_VID32(DF_CURSOR_COLOR_KEY, temp);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_configure_alpha_window
 *
 * This routine configures one of the three hardware alpha regions.
 *--------------------------------------------------------------------------*/

int
df_configure_alpha_window(int window, DF_ALPHA_REGION_PARAMS *alpha_data)
{
    unsigned long vsyncend_even, vtotal_even, vactive_even;
    unsigned long hsyncend, htotal, hactive;
    unsigned long vsyncend, vtotal, vactive;
    unsigned long alpha_ctl, pos;
    unsigned long hadjust, vadjust;
    unsigned long y, height;
    unsigned long xstart, xend;
    unsigned long ystart, yend;
    unsigned long x_copy, width_copy;
    unsigned long y_copy, height_copy;
    unsigned long scale, src, misc;
    unsigned long gfxscale, fbactive;
    unsigned long color;

    if (window > 2)
        return CIM_STATUS_INVALIDPARAMS;

    hsyncend = ((READ_REG32(DC3_H_SYNC_TIMING) >> 16) & 0xFFF) + 1;
    vsyncend = ((READ_REG32(DC3_V_SYNC_TIMING) >> 16) & 0xFFF) + 1;
    htotal = ((READ_REG32(DC3_H_ACTIVE_TIMING) >> 16) & 0xFFF) + 1;
    vtotal = ((READ_REG32(DC3_V_ACTIVE_TIMING) >> 16) & 0xFFF) + 1;
    hactive = (READ_REG32(DC3_H_ACTIVE_TIMING) & 0xFFF) + 1;
    vactive = (READ_REG32(DC3_V_ACTIVE_TIMING) & 0xFFF) + 1;

    /* APPLY THE GRAPHICS SCALE */

    width_copy = alpha_data->width;
    height_copy = alpha_data->height;
    x_copy = alpha_data->x;
    y_copy = alpha_data->y;

    misc = READ_VID32(DF_VID_MISC);
    if (misc & DF_USER_IMPLICIT_SCALING) {
        gfxscale = READ_REG32(DC3_GFX_SCALE);
        fbactive = READ_REG32(DC3_FB_ACTIVE);

        /* REVERSE ENGINEER THE SCALE FACTOR */

        scale = gfxscale & 0xFFFF;
        src = (fbactive >> 16) + 1;
        if (scale != 0x4000) {
            width_copy = width_copy * (((0x4000 * src) / scale) + 1);
            width_copy /= src;
            x_copy = x_copy * (((0x4000 * src) / scale) + 1);
            x_copy /= src;
        }

        scale = gfxscale >> 16;
        src = (fbactive & 0xFFFF) + 1;
        if (scale != 0x4000) {
            height_copy = height_copy * (((0x4000 * src) / scale) + 1);
            height_copy /= src;
            y_copy = y_copy * (((0x4000 * src) / scale) + 1);
            y_copy /= src;
        }
    }

    /* SET PRIORITY */
    /* Priority is the only alpha parameter that is not in a register that */
    /* can be indexed based on the alpha window number.                    */

    pos = 16 + (window << 1);
    alpha_ctl = READ_VID32(DF_VID_ALPHA_CONTROL) & ~(3L << pos);
    alpha_ctl |= (alpha_data->priority & 3) << pos;
    WRITE_VID32(DF_VID_ALPHA_CONTROL, alpha_ctl);

    /* HANDLE INTERLACED MODES */

    if (READ_REG32(DC3_IRQ_FILT_CTL) & DC3_IRQFILT_INTL_EN) {
        vsyncend_even = ((READ_REG32(DC3_V_SYNC_EVEN) >> 16) & 0xFFF) + 1;
        vtotal_even = ((READ_REG32(DC3_V_ACTIVE_EVEN) >> 16) & 0xFFF) + 1;
        vactive_even = (READ_REG32(DC3_V_ACTIVE_EVEN) & 0xFFF) + 1;

        y = y_copy >> 1;

        /* SET Y POSITION FOR ODD FIELD */

        height = (height_copy + 1) >> 1;
        vadjust = vtotal_even - vsyncend_even + 1;

        ystart = y + vadjust;
        yend = y + vadjust + height;

        if (yend > (vactive + vadjust))
            yend = vactive + vadjust;

        WRITE_VID32((DF_ALPHA_YPOS_1 + (window << 5)), (ystart | (yend << 16)));

        /* SET Y POSITION FOR EVEN FIELD */

        height = height_copy >> 1;
        vadjust = vtotal - vsyncend + 1;

        ystart = y + vadjust;
        yend = y + vadjust + height;

        if (yend > (vactive_even + vadjust))
            yend = vactive_even + vadjust;

        WRITE_VID32((DF_VID_ALPHA_Y_EVEN_1 + (window << 3)),
                    (ystart | (yend << 16)));
    }
    else {
        y = y_copy;
        height = height_copy;
        vadjust = vtotal - vsyncend + 1;

        ystart = y + vadjust;
        yend = y + vadjust + height;

        if (yend > (vactive + vadjust))
            yend = vactive + vadjust;

        WRITE_VID32((DF_ALPHA_YPOS_1 + (window << 5)), (ystart | (yend << 16)));
    }

    /* SET ALPHA X POSITION */
    /* The x position is the same for both the odd and even fields. */

    hadjust = htotal - hsyncend - 2;

    xstart = x_copy + hadjust;
    xend = x_copy + hadjust + width_copy;

    if (xend > (hactive + hadjust))
        xend = hactive + hadjust;

    WRITE_VID32((DF_ALPHA_XPOS_1 + (window << 5)), (xstart | (xend << 16)));

    /* SET COLOR REGISTER */

    color = alpha_data->color & 0xFFFFFF;
    if (alpha_data->flags & DF_ALPHAFLAG_COLORENABLED)
        color |= DF_ALPHA_COLOR_ENABLE;

    WRITE_VID32((DF_ALPHA_COLOR_1 + (window << 5)), color);

    /* SET ALPHA VALUE, DELTA AND PER PIXEL */

    alpha_ctl = READ_VID32(DF_ALPHA_CONTROL_1 + (window << 5)) &
        DF_ACTRL_WIN_ENABLE;
    alpha_ctl |= (alpha_data->alpha_value & 0xFF) | DF_ACTRL_LOAD_ALPHA |
        (((unsigned long) alpha_data->delta & 0xFF) << 8);
    if (alpha_data->flags & DF_ALPHAFLAG_PERPIXELENABLED)
        alpha_ctl |= DF_ACTRL_PERPIXEL_EN;

    WRITE_VID32((DF_ALPHA_CONTROL_1 + (window << 5)), alpha_ctl);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_set_alpha_window_enable
 *
 * This routine enables or disables one of the three hardware alpha regions.
 *--------------------------------------------------------------------------*/

int
df_set_alpha_window_enable(int window, int enable)
{
    unsigned long alpha_ctl;

    if (window > 2)
        return CIM_STATUS_INVALIDPARAMS;

    alpha_ctl = READ_VID32(DF_ALPHA_CONTROL_1 + (window << 5));
    if (enable)
        alpha_ctl |= DF_ACTRL_WIN_ENABLE;
    else
        alpha_ctl &= ~DF_ACTRL_WIN_ENABLE;
    WRITE_VID32((DF_ALPHA_CONTROL_1 + (window << 5)), alpha_ctl);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_set_no_ck_outside_alpha
 *
 * This function affects how color/chroma keying is performed inside the video
 * window.
 *
 * If enable is 1, color/chroma key comparison is performed only inside
 * the enabled alpha windows. Outside the enabled alpha windows, video
 * is displayed if color keying is enabled, or graphics is displayed if
 * chroma keying is enabled.
 * If enable is 0, color/chroma key comparison is performed inside the
 * entire video window.
 *--------------------------------------------------------------------------*/

int
df_set_no_ck_outside_alpha(int enable)
{
    unsigned long value;

    value = READ_VID32(DF_VID_ALPHA_CONTROL);
    if (enable)
        value |= DF_NO_CK_OUTSIDE_ALPHA;
    else
        value &= ~DF_NO_CK_OUTSIDE_ALPHA;
    WRITE_VID32(DF_VID_ALPHA_CONTROL, value);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_set_video_request
 *
 * This routine sets the horizontal (pixel) and vertical (line) video request
 * values.
 *--------------------------------------------------------------------------*/

int
df_set_video_request(unsigned long x, unsigned long y)
{
    unsigned long htotal, hsyncend;
    unsigned long vtotal, vsyncend;

    hsyncend = ((READ_REG32(DC3_H_SYNC_TIMING) >> 16) & 0xFFF) + 1;
    vsyncend = ((READ_REG32(DC3_V_SYNC_TIMING) >> 16) & 0xFFF) + 1;
    htotal = ((READ_REG32(DC3_H_ACTIVE_TIMING) >> 16) & 0xFFF) + 1;
    vtotal = ((READ_REG32(DC3_V_ACTIVE_TIMING) >> 16) & 0xFFF) + 1;

    /* SET DISPLAY FILTER VIDEO REQUEST */

    x += htotal - hsyncend - 2;
    y += vtotal - vsyncend + 1;

    if (x >= 0x1000 || y >= 0x800)
        return CIM_STATUS_INVALIDPARAMS;

    WRITE_VID32(DF_VIDEO_REQUEST, (y | (x << 16)));
    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_set_output_color_space
 *
 * This routine sets the color space used when combining graphics and video.
 *--------------------------------------------------------------------------*/

int
df_set_output_color_space(int color_space)
{
    unsigned long alpha_ctl;

    alpha_ctl = READ_VID32(DF_VID_ALPHA_CONTROL);

    alpha_ctl &= ~(DF_CSC_GRAPHICS_RGB_TO_YUV | DF_CSC_VIDEO_YUV_TO_RGB |
                   DF_HD_GRAPHICS | DF_YUV_CSC_EN | DF_ALPHA_DRGB);

    /* OUTPUT IS RGB */
    /* Enable YUV->RGB CSC if necessary and enable alpha output if  */
    /* requested.                                                   */

    if (color_space == DF_OUTPUT_RGB || color_space == DF_OUTPUT_ARGB) {
        if (!(alpha_ctl & DF_VIDEO_INPUT_IS_RGB))
            alpha_ctl |= DF_CSC_VIDEO_YUV_TO_RGB;

        if (color_space == DF_OUTPUT_ARGB)
            alpha_ctl |= DF_ALPHA_DRGB;
    }

    /* OUTPUT IS YUV */
    /* Enable YUV->YUV CSC if necessary and enable RGB->YUV CSC. */

    else if (color_space == DF_OUTPUT_SDTV || color_space == DF_OUTPUT_HDTV) {
        alpha_ctl |= DF_CSC_GRAPHICS_RGB_TO_YUV;

        if (((alpha_ctl & DF_HD_VIDEO) && color_space == DF_OUTPUT_SDTV) ||
            (!(alpha_ctl & DF_HD_VIDEO) && color_space == DF_OUTPUT_HDTV)) {
            alpha_ctl |= DF_YUV_CSC_EN;
        }

        if (color_space == DF_OUTPUT_HDTV)
            alpha_ctl |= DF_HD_GRAPHICS;
    }
    else
        return CIM_STATUS_INVALIDPARAMS;

    WRITE_VID32(DF_VID_ALPHA_CONTROL, alpha_ctl);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_set_output_path
 *
 * This routine changes the current output path in the display filter.
 *--------------------------------------------------------------------------*/

int
df_set_output_path(int format)
{
    unsigned long panel_tim2, panel_pm;
    unsigned long output = 0;
    Q_WORD msr_value = { 0, 0 };

    msr_read64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_CONFIG, &msr_value);
    msr_value.low &= ~(DF_SIMULTANEOUS_CRT_FP | DF_CONFIG_OUTPUT_MASK);
    panel_tim2 = READ_VID32(DF_VIDEO_PANEL_TIM2);
    panel_pm = READ_VID32(DF_POWER_MANAGEMENT);

    if (format == DF_DISPLAY_CRT) {
        /* SiBZ #4188 */
        /* When CRT output is selected, the DF drives the DISP_EN signal   */
        /* with the CRT display enable.  As a consequence, systems that    */
        /* wire the DISP_EN signal to the TFT backlight control will not   */
        /* be able to set CRT-only output without leaving the backlight    */
        /* enabled.  To workaround this issue, we are setting simultaneous */
        /* TFT/CRT and disabling the TFT logic.  The only caveat to this   */
        /* is that some TFT pins are shared with VIP 601 pins.  VIP 601    */
        /* will thus not work when in this pseudo-CRT mode.  To address    */
        /* THAT issue, normal CRT mode sets (in cim_vg.c) will set CRT     */
        /* as the DF output format.  This will allow VIP 601 on CRT-only   */
        /* systems without a TFT attached.                                 */

        panel_pm &= ~DF_PM_PANEL_ON;
        panel_tim2 |= DF_PMTIM2_TFT_PASSHTHROUGH;
        output = DF_OUTPUT_PANEL | DF_SIMULTANEOUS_CRT_FP;
    }
    else if (format == DF_DISPLAY_FP || format == DF_DISPLAY_CRT_FP) {
        panel_pm |= DF_PM_PANEL_ON;
        panel_tim2 &= ~DF_PMTIM2_TFT_PASSHTHROUGH;

        if (format == DF_DISPLAY_FP)
            output = DF_OUTPUT_PANEL;
        else if (format == DF_DISPLAY_CRT_FP)
            output = DF_OUTPUT_PANEL | DF_SIMULTANEOUS_CRT_FP;
    }
    else {
        switch (format) {
        case DF_DISPLAY_VOP:
            output = DF_OUTPUT_VOP;
            break;
        case DF_DISPLAY_DRGB:
            output = DF_OUTPUT_DRGB;
            break;
        case DF_DISPLAY_CRT_DRGB:
            output = DF_OUTPUT_DRGB | DF_SIMULTANEOUS_CRT_FP;
            break;
        default:
            return CIM_STATUS_INVALIDPARAMS;
        }
    }
    msr_value.low |= output;
    msr_write64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_CONFIG, &msr_value);
    WRITE_VID32(DF_VIDEO_PANEL_TIM2, panel_tim2);
    WRITE_VID32(DF_POWER_MANAGEMENT, panel_pm);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_test_video_flip_status
 *
 * This routine tests if a new video offset has been latched.
 *--------------------------------------------------------------------------*/

unsigned long
df_test_video_flip_status(void)
{
    return (READ_REG32(DC3_LINE_CNT_STATUS) & DC3_LNCNT_VFLIP);
}

/*---------------------------------------------------------------------------
 * df_save_state
 *
 * This routine saves all persistent DF state information.
 *--------------------------------------------------------------------------*/

int
df_save_state(DF_SAVE_RESTORE *df_state)
{
    unsigned long i;

    /* READ ALL DF REGISTERS */

    df_state->vcfg = READ_VID32(DF_VIDEO_CONFIG);
    df_state->dcfg = READ_VID32(DF_DISPLAY_CONFIG);
    df_state->video_x = READ_VID32(DF_VIDEO_X_POS);
    df_state->video_y = READ_VID32(DF_VIDEO_Y_POS);
    df_state->video_scaler = READ_VID32(DF_VIDEO_SCALER);
    df_state->video_color_key = READ_VID32(DF_VIDEO_COLOR_KEY);
    df_state->video_color_mask = READ_VID32(DF_VIDEO_COLOR_MASK);
    df_state->sat_limit = READ_VID32(DF_SATURATION_LIMIT);
    df_state->vid_misc = READ_VID32(DF_VID_MISC);
    df_state->video_yscale = READ_VID32(DF_VIDEO_YSCALE);
    df_state->video_xscale = READ_VID32(DF_VIDEO_XSCALE);
    df_state->vid_alpha_control = READ_VID32(DF_VID_ALPHA_CONTROL);
    df_state->cursor_key = READ_VID32(DF_CURSOR_COLOR_KEY);
    df_state->cursor_mask = READ_VID32(DF_CURSOR_COLOR_MASK);
    df_state->cursor_color1 = READ_VID32(DF_CURSOR_COLOR_1);
    df_state->cursor_color2 = READ_VID32(DF_CURSOR_COLOR_2);
    df_state->alpha_xpos1 = READ_VID32(DF_ALPHA_XPOS_1);
    df_state->alpha_ypos1 = READ_VID32(DF_ALPHA_YPOS_1);
    df_state->alpha_color1 = READ_VID32(DF_ALPHA_COLOR_1);
    df_state->alpha_control1 = READ_VID32(DF_ALPHA_CONTROL_1);
    df_state->alpha_xpos2 = READ_VID32(DF_ALPHA_XPOS_2);
    df_state->alpha_ypos2 = READ_VID32(DF_ALPHA_YPOS_2);
    df_state->alpha_color2 = READ_VID32(DF_ALPHA_COLOR_2);
    df_state->alpha_control2 = READ_VID32(DF_ALPHA_CONTROL_2);
    df_state->alpha_xpos3 = READ_VID32(DF_ALPHA_XPOS_3);
    df_state->alpha_ypos3 = READ_VID32(DF_ALPHA_YPOS_3);
    df_state->alpha_color3 = READ_VID32(DF_ALPHA_COLOR_3);
    df_state->alpha_control3 = READ_VID32(DF_ALPHA_CONTROL_3);
    df_state->vid_request = READ_VID32(DF_VIDEO_REQUEST);
    df_state->vid_ypos_even = READ_VID32(DF_VID_YPOS_EVEN);
    df_state->alpha_ypos_even1 = READ_VID32(DF_VID_ALPHA_Y_EVEN_1);
    df_state->alpha_ypos_even2 = READ_VID32(DF_VID_ALPHA_Y_EVEN_2);
    df_state->alpha_ypos_even3 = READ_VID32(DF_VID_ALPHA_Y_EVEN_3);
    df_state->panel_tim1 = READ_VID32(DF_VIDEO_PANEL_TIM1);
    df_state->panel_tim2 = READ_VID32(DF_VIDEO_PANEL_TIM2);
    df_state->panel_pm = READ_VID32(DF_POWER_MANAGEMENT);
    df_state->panel_dither = READ_VID32(DF_DITHER_CONTROL);

    /* READ DF PALETTE */

    WRITE_VID32(DF_PALETTE_ADDRESS, 0);
    for (i = 0; i < 256; i++)
        df_state->palette[i] = READ_VID32(DF_PALETTE_DATA);

    /* READ FILTER COEFFICIENTS */

    for (i = 0; i < 512; i++)
        df_state->coefficients[i] = READ_VID32(DF_COEFFICIENT_BASE + (i << 2));

    /* READ ALL DF MSRS */

    msr_read64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_CAP, &(df_state->msr_cap));
    msr_read64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_CONFIG,
               &(df_state->msr_config));
    msr_read64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_SMI, &(df_state->msr_smi));
    msr_read64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_ERROR,
               &(df_state->msr_error));
    msr_read64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_PM, &(df_state->msr_pm));
    msr_read64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_DIAG,
               &(df_state->msr_diag));
    msr_read64(MSR_DEVICE_GEODELX_DF, DF_MBD_MSR_DIAG_DF,
               &(df_state->msr_df_diag));
    msr_read64(MSR_DEVICE_GEODELX_DF, DF_MSR_PAD_SEL, &(df_state->msr_pad_sel));

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_restore_state
 *
 * This routine restores all persistent DF state information.
 *--------------------------------------------------------------------------*/

int
df_restore_state(DF_SAVE_RESTORE *df_state)
{
    unsigned long i;

    /* CLEAR VCFG AND DCFG */

    WRITE_VID32(DF_VIDEO_CONFIG, 0);
    WRITE_VID32(DF_DISPLAY_CONFIG, 0);

    /* RESTORE DF MSRS */

    msr_write64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_CAP, &(df_state->msr_cap));
    msr_write64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_CONFIG,
                &(df_state->msr_config));
    msr_write64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_SMI, &(df_state->msr_smi));
    msr_write64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_ERROR,
                &(df_state->msr_error));
    msr_write64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_PM, &(df_state->msr_pm));
    msr_write64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_DIAG,
                &(df_state->msr_diag));
    msr_write64(MSR_DEVICE_GEODELX_DF, DF_MBD_MSR_DIAG_DF,
                &(df_state->msr_df_diag));
    msr_write64(MSR_DEVICE_GEODELX_DF, DF_MSR_PAD_SEL,
                &(df_state->msr_pad_sel));

    /* RESTORE ALL DF REGISTERS */

    WRITE_VID32(DF_VIDEO_X_POS, df_state->video_x);
    WRITE_VID32(DF_VIDEO_Y_POS, df_state->video_y);
    WRITE_VID32(DF_VIDEO_SCALER, df_state->video_scaler);
    WRITE_VID32(DF_VIDEO_COLOR_KEY, df_state->video_color_key);
    WRITE_VID32(DF_VIDEO_COLOR_MASK, df_state->video_color_mask);
    WRITE_VID32(DF_SATURATION_LIMIT, df_state->sat_limit);
    WRITE_VID32(DF_VID_MISC, df_state->vid_misc);
    WRITE_VID32(DF_VIDEO_YSCALE, df_state->video_yscale);
    WRITE_VID32(DF_VIDEO_XSCALE, df_state->video_xscale);
    WRITE_VID32(DF_VID_ALPHA_CONTROL, df_state->vid_alpha_control);
    WRITE_VID32(DF_CURSOR_COLOR_KEY, df_state->cursor_key);
    WRITE_VID32(DF_CURSOR_COLOR_MASK, df_state->cursor_mask);
    WRITE_VID32(DF_CURSOR_COLOR_1, df_state->cursor_color1);
    WRITE_VID32(DF_CURSOR_COLOR_2, df_state->cursor_color2);
    WRITE_VID32(DF_ALPHA_XPOS_1, df_state->alpha_xpos1);
    WRITE_VID32(DF_ALPHA_YPOS_1, df_state->alpha_ypos1);
    WRITE_VID32(DF_ALPHA_COLOR_1, df_state->alpha_color1);
    WRITE_VID32(DF_ALPHA_CONTROL_1, df_state->alpha_control1);
    WRITE_VID32(DF_ALPHA_XPOS_2, df_state->alpha_xpos2);
    WRITE_VID32(DF_ALPHA_YPOS_2, df_state->alpha_ypos2);
    WRITE_VID32(DF_ALPHA_COLOR_2, df_state->alpha_color2);
    WRITE_VID32(DF_ALPHA_CONTROL_2, df_state->alpha_control1);
    WRITE_VID32(DF_ALPHA_XPOS_3, df_state->alpha_xpos3);
    WRITE_VID32(DF_ALPHA_YPOS_3, df_state->alpha_ypos3);
    WRITE_VID32(DF_ALPHA_COLOR_3, df_state->alpha_color3);
    WRITE_VID32(DF_ALPHA_CONTROL_3, df_state->alpha_control3);
    WRITE_VID32(DF_VIDEO_REQUEST, df_state->vid_request);
    WRITE_VID32(DF_VID_YPOS_EVEN, df_state->vid_ypos_even);
    WRITE_VID32(DF_VID_ALPHA_Y_EVEN_1, df_state->alpha_ypos_even1);
    WRITE_VID32(DF_VID_ALPHA_Y_EVEN_2, df_state->alpha_ypos_even2);
    WRITE_VID32(DF_VID_ALPHA_Y_EVEN_3, df_state->alpha_ypos_even3);
    WRITE_VID32(DF_VIDEO_PANEL_TIM1, df_state->panel_tim1);
    WRITE_VID32(DF_VIDEO_PANEL_TIM2, df_state->panel_tim2);
    WRITE_VID32(DF_POWER_MANAGEMENT, df_state->panel_pm);
    WRITE_VID32(DF_DITHER_CONTROL, df_state->panel_dither);

    /* RESTORE DF PALETTE */

    WRITE_VID32(DF_PALETTE_ADDRESS, 0);
    for (i = 0; i < 256; i++)
        WRITE_VID32(DF_PALETTE_DATA, df_state->palette[i]);

    /* RESTORE FILTER COEFFICIENTS */

    for (i = 0; i < 512; i++)
        WRITE_VID32(DF_COEFFICIENT_BASE + (i << 2), df_state->coefficients[i]);

    /* RESTORE DCFG AND VCFG */

    WRITE_VID32(DF_DISPLAY_CONFIG, df_state->dcfg);
    WRITE_VID32(DF_VIDEO_CONFIG, df_state->vcfg);

    return CIM_STATUS_OK;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * CIMARRON DF READ ROUTINES
 * These routines are included for use in diagnostics or when debugging.  They
 * can be optionally excluded from a project.
 *++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#if CIMARRON_INCLUDE_DF_READ_ROUTINES

/*---------------------------------------------------------------------------
 * df_read_composite_crc
 *
 * This routine reads the CRC of the combination of graphics/video data.  This
 * CRC checks data immediately before the CRT DACs.
 *--------------------------------------------------------------------------*/

unsigned long
df_read_composite_crc(int crc_source)
{
    Q_WORD msr_value = { 0, 0 };
    unsigned long crc;
    unsigned long interlaced;
    unsigned long line, field;
    unsigned long timeout = 1000;

    if (!(READ_REG32(DC3_DISPLAY_CFG) & DC3_DCFG_TGEN))
        return 0xFFFFFFFF;

    /* ENABLE 32-BIT CRCS */

    msr_read64(MSR_DEVICE_GEODELX_DF, DF_MBD_MSR_DIAG_DF, &msr_value);
    msr_value.low |= DF_DIAG_32BIT_CRC;
    msr_write64(MSR_DEVICE_GEODELX_DF, DF_MBD_MSR_DIAG_DF, &msr_value);

    /* RESET THE CRC */

    WRITE_VID32(DF_VID_CRC, 0);

    /* WAIT FOR THE RESET TO BE LATCHED */

    while ((READ_VID32(DF_VID_CRC32) != 0x00000001) && timeout)
        timeout--;

    /* WAIT FOR THE CORRECT FIELD */
    /* We use the VG line count and field indicator to determine when */
    /* to kick off a CRC.                                             */

    if (crc_source & DF_CRC_SOURCE_EVEN)
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

        if (crc_source & DF_CRC_SOURCE_EVEN)
            return 0xFFFFFFFF;
    }

    /* ENABLE THE CRC */

    WRITE_VID32(DF_VID_CRC, 1);

    /* WAIT FOR THE CRC TO BE COMPLETED */

    while (!(READ_VID32(DF_VID_CRC) & 4));

    crc = READ_VID32(DF_VID_CRC32);

    return crc;
}

/*---------------------------------------------------------------------------
 * df_read_composite_window_crc
 *
 * This routine reads the CRC of a rectangular subsection of the combination
 * of graphics/video data.
 *--------------------------------------------------------------------------*/

unsigned long
df_read_composite_window_crc(unsigned long x, unsigned long y,
                             unsigned long width, unsigned long height,
                             int source)
{
    Q_WORD msr_value = { 0, 0 };
    unsigned long interlaced;
    unsigned long line, field;
    unsigned long crc = 0;
    unsigned long hsyncend, htotal, hsyncstart;
    unsigned long vsyncend, vtotal;
    unsigned long hblankstart, hactive;
    unsigned long vblankstart, vactive;

    hsyncend = ((READ_REG32(DC3_H_SYNC_TIMING) >> 16) & 0xFFF) + 1;
    htotal = ((READ_REG32(DC3_H_ACTIVE_TIMING) >> 16) & 0xFFF) + 1;
    hsyncstart = (READ_REG32(DC3_H_SYNC_TIMING) & 0xFFF) + 1;
    hactive = (READ_REG32(DC3_H_ACTIVE_TIMING) & 0xFFF) + 1;
    hblankstart = (READ_REG32(DC3_H_BLANK_TIMING) & 0xFFF) + 1;
    if ((interlaced = (READ_REG32(DC3_IRQ_FILT_CTL) & DC3_IRQFILT_INTL_EN)) &&
        !(source & DF_CRC_SOURCE_EVEN)) {
        vsyncend = ((READ_REG32(DC3_V_SYNC_EVEN) >> 16) & 0xFFF) + 1;
        vtotal = ((READ_REG32(DC3_V_ACTIVE_EVEN) >> 16) & 0xFFF) + 1;
        vactive = (READ_REG32(DC3_V_ACTIVE_EVEN) & 0xFFF) + 1;
        vblankstart = (READ_REG32(DC3_V_BLANK_EVEN) & 0xFFF) + 1;
    }
    else {
        vsyncend = ((READ_REG32(DC3_V_SYNC_TIMING) >> 16) & 0xFFF) + 1;
        vtotal = ((READ_REG32(DC3_V_ACTIVE_TIMING) >> 16) & 0xFFF) + 1;
        vactive = (READ_REG32(DC3_V_ACTIVE_TIMING) & 0xFFF) + 1;
        vblankstart = (READ_REG32(DC3_V_BLANK_TIMING) & 0xFFF) + 1;
    }

    /* TIMINGS MUST BE ACTIVE */

    if (!(READ_REG32(DC3_DISPLAY_CFG) & DC3_DCFG_TGEN))
        return 0xFFFFFFFF;

    /* DISABLE GLCP ACTIONS */

    msr_value.low = 0;
    msr_value.high = 0;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_DIAGCTL, &msr_value);

    /* ENABLE HW CLOCK GATING AND SET GLCP CLOCK TO DOT CLOCK */

    msr_value.low = 5;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, MSR_GEODELINK_PM, &msr_value);
    msr_value.low = 0;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_DBGCLKCTL, &msr_value);
    msr_value.low = 3;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_DBGCLKCTL, &msr_value);

    /* USE H4 FUNCTION A FOR HSYNC AND H4 FUNCTION B FOR NOT HSYNC */
    /* HSYNC is bit 30 for the DF                                  */

    msr_value.high = 0x00000001;
    msr_value.low = 0xE0000FF0;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_H0CTL + 4, &msr_value);

    /* USE H3 FUNCTION A FOR VSYNC AND H3 FUNCTION B FOR NOT VSYNC */
    /* VSYNC is bit 54 for VG and bit 29 for DF                    */

    msr_value.high = 0x00000000;
    msr_value.low = 0x001D55AA;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_H0CTL + 3, &msr_value);

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

    /* N1 (XSTATE = 10 and HSYNC LOW) */
    /* Increment H. Counter           */
    /* Note: HSync = H4               */

    msr_value.high = 0x00080000;
    msr_value.low = 0x00000120;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_SETN0CTL + 1, &msr_value);

    /* M0 (XSTATE = 10 and H. COUNTER == LIMIT)  */
    /* Clear H. Counter and increment V. Counter */

    msr_value.high = 0x00000000;
    msr_value.low = 0x00000122;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_SETM0CTL, &msr_value);

    /* N4 (XSTATE = 10 && CMP0 <= H. COUNTER <= CMP1 && CMP2 <= V. COUNTER
     * <= CMP3)
     * CRC into REGB
     */

    msr_value.high = 0x00000000;
    msr_value.low = 0x10C20120;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_SETN0CTL + 4, &msr_value);

    /* COMPARATOR 0 VALUE                                   */
    /* Value = xstart + (htotal - hsync_end) - 1            */
    /* The value will be adjusted for a border if necessary */

    msr_value.low = x + htotal - hsyncend - 1;
    if (READ_REG32(DC3_DISPLAY_CFG) & DC3_DCFG_DCEN)
        msr_value.low -= hblankstart - hactive;
    msr_value.low--;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_CMPVAL0, &msr_value);

    /* COMPARATOR 1 VALUE                                    */
    /* Value = xstart + (htotal - hsync_end - 1) - 1 + width */

    msr_value.low += width - 1;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_CMPVAL0 + 2, &msr_value);

    /* COMPARATOR 2 VALUE                 */
    /* Value = ystart + vtotal - vsyncend */

    msr_value.low = (y + vtotal - vsyncend) << 16;
    if (READ_REG32(DC3_DISPLAY_CFG) & DC3_DCFG_DCEN)
        msr_value.low -= (vblankstart - vactive) << 16;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_CMPVAL0 + 4, &msr_value);

    /* COMPARATOR 3 VALUE                              */
    /* Value = ystart + vtotal - vsyncend + height - 1 */

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

    /* SET REGB MASK                                                */
    /* We set the mask such that all only 24 bits of data are CRCed */

    msr_value.low = 0x00FFFFFF;
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_REGBMASK, &msr_value);

    /* SET REGA LIMITS                              */
    /* Lower counter uses htotal - sync_time - 1.   */
    /* Upper counter is 0xFFFF to prevent rollover. */

    msr_value.low = 0xFFFF0000 | (htotal - (hsyncend - hsyncstart) - 1);
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_REGAVAL, &msr_value);

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

    /* REGISTER ACTION 1 */
    /* CRC into RegB if cmp0 <= h.counter <= cmp1 && cmp2 <= v. counter <
     * cmp3 && 7 xstate = 10 8
     * Increment h.counter if xstate = 10 and HSync is low.
     */

    msr_value.low = 0x000A00A0;
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
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_ACTION0 + 17, &msr_value);
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_ACTION0 + 18, &msr_value);
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_ACTION0 + 19, &msr_value);
    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_ACTION0 + 20, &msr_value);

    /* WAIT FOR THE CORRECT FIELD */
    /* We use the VG line count and field indicator to determine when */
    /* to kick off a CRC.                                             */

    if (source & DF_CRC_SOURCE_EVEN)
        field = 0;
    else
        field = DC3_LNCNT_EVEN_FIELD;

    if (interlaced) {
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

        if (source & DF_CRC_SOURCE_EVEN)
            return 0xFFFFFFFF;
    }

    /* CONFIGURE DISPLAY FILTER TO LOAD DATA ONTO LOWER 32-BITS */

    msr_value.high = 0;
    msr_value.low = 0x0000800B;
    msr_write64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_DIAG, &msr_value);

    /* CONFIGURE DIAG CONTROL */
    /* Set RegA action1 to increment lower 16 bits and clear at limit. (5)
     * Set RegA action2 to increment upper 16 bits. (6)
     * Set RegB action1 to CRC32 (1)
     * Set all comparators to REGA override (0,1 lower mbus, 2,3 upper mbus)
     * Enable all actions
     */

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

    /* DISABLE DF DIAG BUS OUTPUTS */

    msr_value.low = 0x00000000;
    msr_value.high = 0x00000000;
    msr_write64(MSR_DEVICE_GEODELX_DF, MSR_GEODELINK_DIAG, &msr_value);

    /* DISABLE GLCP ACTIONS */

    msr_write64(MSR_DEVICE_GEODELX_GLCP, GLCP_DIAGCTL, &msr_value);

    return crc;
}

/*---------------------------------------------------------------------------
 * df_read_panel_crc
 *
 * This routine reads the CRC for a frame of data after the panel dithering
 * logic.
 *--------------------------------------------------------------------------*/

unsigned long
df_read_panel_crc(void)
{
    Q_WORD msr_value = { 0, 0 };
    unsigned long timeout = 1000;

    if (!(READ_REG32(DC3_DISPLAY_CFG) & DC3_DCFG_TGEN))
        return 0xFFFFFFFF;

    /* ENABLE 32-BIT CRCS */

    msr_read64(MSR_DEVICE_GEODELX_DF, DF_MBD_MSR_DIAG_DF, &msr_value);
    msr_value.low |= DF_DIAG_32BIT_CRC;
    msr_write64(MSR_DEVICE_GEODELX_DF, DF_MBD_MSR_DIAG_DF, &msr_value);

    /* RESET CRC */

    WRITE_VID32(DF_PANEL_CRC, 0);

    /* WAIT FOR THE RESET TO BE LATCHED */

    while ((READ_VID32(DF_PANEL_CRC32) != 0x00000001) && timeout)
        timeout--;

    WRITE_VID32(DF_PANEL_CRC, 1);

    /* WAIT FOR THE CRC TO BE COMPLETED */

    while (!(READ_VID32(DF_PANEL_CRC) & 4));

    return READ_VID32(DF_PANEL_CRC32);
}

/*---------------------------------------------------------------------------
 * df_get_video_enable
 *
 * This routine reads the enable status of the video overlay.
 *--------------------------------------------------------------------------*/

int
df_get_video_enable(int *enable, unsigned long *flags)
{
    *enable = 0;
    *flags = 0;
    if (READ_VID32(DF_VIDEO_CONFIG) & DF_VCFG_VID_EN) {
        *enable = 1;

        /* CHECK FOR COLOR KEY DISABLED */
        /* Color keying can be completely disabled when video is enabled to */
        /* allow unhindered per-pixel alpha blending.  As color keying is   */
        /* always disabled when video is disabled, it is only possible to   */
        /* test for this condition when video is enabled.                   */

        if (!(READ_VID32(DF_DISPLAY_CONFIG) & DF_DCFG_VG_CK) &&
            !(READ_REG32(DC3_COLOR_KEY) & DC3_CLR_KEY_ENABLE)) {
            *flags = DF_ENABLEFLAG_NOCOLORKEY;
        }
    }

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_get_video_source_configuration
 *
 * This routine reads the current configuration of the source buffers for the
 * video overlay.
 *--------------------------------------------------------------------------*/

int
df_get_video_source_configuration(DF_VIDEO_SOURCE_PARAMS *video_source_odd,
                                  DF_VIDEO_SOURCE_PARAMS *video_source_even)
{
    unsigned long format, temp;
    unsigned long size;

    /* READ VIDEO FORMAT */

    temp = READ_VID32(DF_VIDEO_CONFIG);

    format = (temp >> 2) & 3;
    if (temp & DF_VCFG_4_2_0_MODE)
        format |= 4;
    else if (READ_VID32(DF_VID_ALPHA_CONTROL) & DF_VIDEO_INPUT_IS_RGB)
        format |= 8;
    video_source_odd->video_format = format;

    /* CHECK IF SOURCE IS HD VIDEO */

    if (READ_VID32(DF_VID_ALPHA_CONTROL) & DF_HD_VIDEO)
        video_source_odd->flags = DF_SOURCEFLAG_HDTVSOURCE;
    else
        video_source_odd->flags = 0;

    /* READ SCALING ALGORITHM */

    if (READ_VID32(DF_VID_MISC) & DF_USER_IMPLICIT_SCALING)
        video_source_odd->flags |= DF_SOURCEFLAG_IMPLICITSCALING;

    /* READ VIDEO PITCH */

    temp = READ_REG32(DC3_VID_YUV_PITCH);
    video_source_odd->y_pitch = (temp & 0xFFFF) << 3;
    video_source_odd->uv_pitch = (temp >> 16) << 3;

    /* READ VIDEO SIZE */

    temp = READ_VID32(DF_VIDEO_CONFIG);
    size = (temp >> 8) & 0xFF;
    if (temp & DF_VCFG_LINE_SIZE_BIT8)
        size |= 0x100;
    if (temp & DF_VCFG_LINE_SIZE_BIT9)
        size |= 0x200;

    video_source_odd->width = size << 1;
    video_source_odd->height = READ_VID32(DF_VIDEO_SCALER) & 0x7FF;

    /* READ VIDEO OFFSETS */

    video_source_odd->y_offset = READ_REG32(DC3_VID_Y_ST_OFFSET) & 0xFFFFFFF;
    video_source_odd->u_offset = READ_REG32(DC3_VID_U_ST_OFFSET) & 0xFFFFFFF;
    video_source_odd->v_offset = READ_REG32(DC3_VID_V_ST_OFFSET) & 0xFFFFFFF;

    if (READ_REG32(DC3_IRQ_FILT_CTL) & DC3_IRQFILT_INTL_EN) {
        video_source_even->y_offset =
            READ_REG32(DC3_VID_EVEN_Y_ST_OFFSET) & 0xFFFFFFF;
        video_source_even->u_offset =
            READ_REG32(DC3_VID_EVEN_U_ST_OFFSET) & 0xFFFFFFF;
        video_source_even->v_offset =
            READ_REG32(DC3_VID_EVEN_V_ST_OFFSET) & 0xFFFFFFF;
    }

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_get_video_position
 *
 * This routine reads the current position of the video overlay.
 *--------------------------------------------------------------------------*/

int
df_get_video_position(DF_VIDEO_POSITION *video_window)
{
    unsigned long xreg, yreg, dst_clip, clip;
    unsigned long height;
    unsigned long xend, yend;
    unsigned long hsyncend, htotal;
    unsigned long vsyncend, vtotal;
    unsigned long hadjust, vadjust;
    unsigned long misc, gfxscale;
    unsigned long temp;
    long xstart, ystart;

    video_window->flags = DF_POSFLAG_DIRECTCLIP;

    hsyncend = ((READ_REG32(DC3_H_SYNC_TIMING) >> 16) & 0xFFF) + 1;
    htotal = ((READ_REG32(DC3_H_ACTIVE_TIMING) >> 16) & 0xFFF) + 1;

    /* ODD FIELD START COUNTS FROM THE EVEN FIELD TIMINGS */
    /* We assume that the even field y position is always programmed */
    /* to be just after the odd field.                               */

    if (READ_REG32(DC3_IRQ_FILT_CTL) & DC3_IRQFILT_INTL_EN) {
        vsyncend = ((READ_REG32(DC3_V_SYNC_EVEN) >> 16) & 0xFFF) + 1;
        vtotal = ((READ_REG32(DC3_V_ACTIVE_EVEN) >> 16) & 0xFFF) + 1;
    }
    else {
        vsyncend = ((READ_REG32(DC3_V_SYNC_TIMING) >> 16) & 0xFFF) + 1;
        vtotal = ((READ_REG32(DC3_V_ACTIVE_TIMING) >> 16) & 0xFFF) + 1;
    }

    hadjust = htotal - hsyncend - 14;
    vadjust = vtotal - vsyncend + 1;

    xreg = READ_VID32(DF_VIDEO_X_POS);
    yreg = READ_VID32(DF_VIDEO_Y_POS);

    xstart = (xreg & 0xFFF) - hadjust;
    ystart = (yreg & 0x7FF) - vadjust;
    xend = ((xreg >> 16) & 0xFFF) - hadjust;
    yend = ((yreg >> 16) & 0x7FF) - vadjust;

    height = yend - ystart;

    if (READ_REG32(DC3_IRQ_FILT_CTL) & DC3_IRQFILT_INTL_EN) {
        /* Y COORDINATE IS ACTUALLY 2X THE ODD FIELD START */

        ystart <<= 1;

        /* CALCULATE THE EXACT VIDEO HEIGHT */
        /* The height of the video window is the sum of the */
        /* odd and even field heights.                      */

        yreg = READ_VID32(DF_VID_YPOS_EVEN);
        height += ((yreg >> 16) & 0x7FF) - (yreg & 0x7FF);
    }

    clip = ((READ_VID32(DF_VIDEO_CONFIG) >> 16) & 0x1FF) << 2;

    /* ADJUST FOR CLIPPING VALUES THAT ARE NOT FOUR-PIXEL ALIGNED */

    dst_clip = 0;
    if (xstart < 0) {
        dst_clip += -xstart;
        xstart = 0;
    }

    /* REVERSE THE GRAPHICS SCALE */

    misc = READ_VID32(DF_VID_MISC);
    if (misc & DF_USER_IMPLICIT_SCALING) {
        gfxscale = READ_REG32(DC3_GFX_SCALE);

        if (gfxscale != 0x40004000) {
            temp = ystart + height;
            temp = (temp * (gfxscale >> 16)) / 0x4000;

            xstart = (xstart * (gfxscale & 0xFFFF)) / 0x4000;
            xend = (xend * (gfxscale & 0xFFFF)) / 0x4000;
            ystart = (ystart * (gfxscale >> 16)) / 0x4000;
            height = temp - ystart;
        }
    }

    video_window->left_clip = clip;
    video_window->dst_clip = dst_clip;
    video_window->x = xstart;
    video_window->y = ystart;
    video_window->width = xend - xstart;
    video_window->height = height;

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_get_video_scale
 *
 * This routine reads the current scale values for video scaling.
 *--------------------------------------------------------------------------*/

int
df_get_video_scale(unsigned long *x_scale, unsigned long *y_scale)
{
    *x_scale = READ_VID32(DF_VIDEO_XSCALE) & 0x000FFFFF;
    *y_scale = READ_VID32(DF_VIDEO_YSCALE) & 0x000FFFFF;
    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_get_video_filter_coefficients
 *
 * This routine reads the coefficients for the video scaler/filter.
 *--------------------------------------------------------------------------*/

int
df_get_video_filter_coefficients(long taps[][4], int *phase256)
{
    unsigned long i, temp;
    long coeff;

    if (READ_VID32(DF_VIDEO_SCALER) & DF_SCALE_128_PHASES)
        *phase256 = 0;
    else
        *phase256 = 1;

    for (i = 0; i < 256; i++) {
        temp = READ_VID32(DF_COEFFICIENT_BASE + (i << 3));

        /* TAP 0 */

        coeff = temp & 0x7FFF;
        if (temp & 0x8000)
            coeff = -coeff;
        taps[i][0] = coeff;

        /* TAP 1 */

        temp >>= 16;
        coeff = temp & 0x7FFF;
        if (temp & 0x8000)
            coeff = -coeff;
        taps[i][1] = coeff;

        temp = READ_VID32(DF_COEFFICIENT_BASE + (i << 3) + 4);

        /* TAP 2 */

        coeff = temp & 0x7FFF;
        if (temp & 0x8000)
            coeff = -coeff;
        taps[i][2] = coeff;

        /* TAP 3 */

        temp >>= 16;
        coeff = temp & 0x7FFF;
        if (temp & 0x8000)
            coeff = -coeff;
        taps[i][3] = coeff;
    }

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_get_video_color_key
 *
 * This routine reads the current settings for hardware color/chroma keying.
 *--------------------------------------------------------------------------*/

int
df_get_video_color_key(unsigned long *key, unsigned long *mask, int *graphics)
{
    unsigned long chroma = READ_VID32(DF_DISPLAY_CONFIG) & DF_DCFG_VG_CK;

    if (chroma) {
        /* CHROMA KEY - READ KEY AND MASK FROM DF */

        *graphics = 0;
        *key = READ_VID32(DF_VIDEO_COLOR_KEY) & 0xFFFFFF;
        *mask = READ_VID32(DF_VIDEO_COLOR_MASK) & 0xFFFFFF;
    }
    else {
        *graphics = 1;

        *key = READ_REG32(DC3_COLOR_KEY) & 0xFFFFFF;
        *mask = READ_REG32(DC3_COLOR_MASK) & 0xFFFFFF;
    }

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_get_video_palette_entry
 *
 * This routine returns a single palette entry.
 *--------------------------------------------------------------------------*/

int
df_get_video_palette_entry(unsigned long index, unsigned long *palette)
{
    if (index > 0xFF)
        return CIM_STATUS_INVALIDPARAMS;

    /* READ A SINGLE ENTRY */

    WRITE_VID32(DF_PALETTE_ADDRESS, index);
    *palette = READ_VID32(DF_PALETTE_DATA);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_get_video_palette
 *
 * This routine returns the entire video palette.
 *--------------------------------------------------------------------------*/

int
df_get_video_palette(unsigned long *palette)
{
    unsigned long i;

    WRITE_VID32(DF_PALETTE_ADDRESS, 0);
    for (i = 0; i < 256; i++)
        palette[i] = READ_VID32(DF_PALETTE_DATA);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_get_video_cursor_color_key
 *
 * This routine returns the current configuration for the hardware video cursor
 * color key.
 *--------------------------------------------------------------------------*/

int
df_get_video_cursor_color_key(DF_VIDEO_CURSOR_PARAMS *cursor_color_key)
{
    unsigned long key;

    cursor_color_key->flags = 0;
    cursor_color_key->color1 = READ_VID32(DF_CURSOR_COLOR_1) & 0xFFFFFF;
    cursor_color_key->color2 = READ_VID32(DF_CURSOR_COLOR_2) & 0xFFFFFF;
    cursor_color_key->mask = READ_VID32(DF_CURSOR_COLOR_MASK) & 0xFFFFFF;

    key = READ_VID32(DF_CURSOR_COLOR_KEY);
    cursor_color_key->key = key & 0xFFFFFF;
    cursor_color_key->select_color2 = (key >> 24) & 0x1F;

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_get_video_cursor_color_key_enable
 *
 * This routine returns the current enable status of the hardware video cursor
 * color key.
 *--------------------------------------------------------------------------*/

int
df_get_video_cursor_color_key_enable(void)
{
    if (READ_VID32(DF_CURSOR_COLOR_KEY) & DF_CURSOR_COLOR_KEY_ENABLE)
        return 1;

    return 0;
}

/*---------------------------------------------------------------------------
 * df_get_alpha_window_configuration
 *
 * This routine reads the current configuration for one of the three hardware
 * alpha regions.
 *--------------------------------------------------------------------------*/

int
df_get_alpha_window_configuration(int window,
                                  DF_ALPHA_REGION_PARAMS *alpha_data)
{
    unsigned long pos, color, alpha_ctl;
    unsigned long hsyncend, htotal;
    unsigned long vsyncend, vtotal;
    unsigned long hadjust, vadjust;
    unsigned long xreg, yreg;
    unsigned long misc, gfxscale;
    unsigned long temp;
    char delta;

    if (window > 2)
        return CIM_STATUS_INVALIDPARAMS;

    hsyncend = ((READ_REG32(DC3_H_SYNC_TIMING) >> 16) & 0xFFF) + 1;
    htotal = ((READ_REG32(DC3_H_ACTIVE_TIMING) >> 16) & 0xFFF) + 1;
    if (READ_REG32(DC3_IRQ_FILT_CTL) & DC3_IRQFILT_INTL_EN) {
        vtotal = ((READ_REG32(DC3_V_ACTIVE_EVEN) >> 16) & 0xFFF) + 1;
        vsyncend = ((READ_REG32(DC3_V_SYNC_EVEN) >> 16) & 0xFFF) + 1;
    }
    else {
        vtotal = ((READ_REG32(DC3_V_ACTIVE_TIMING) >> 16) & 0xFFF) + 1;
        vsyncend = ((READ_REG32(DC3_V_SYNC_TIMING) >> 16) & 0xFFF) + 1;
    }

    /* GET PRIORITY */

    pos = 16 + (window << 1);
    alpha_data->priority = (READ_VID32(DF_VID_ALPHA_CONTROL) >> pos) & 3L;

    /* GET ALPHA WINDOW */

    hadjust = htotal - hsyncend - 2;
    vadjust = vtotal - vsyncend + 1;

    xreg = READ_VID32(DF_ALPHA_XPOS_1 + (window << 5));
    yreg = READ_VID32(DF_ALPHA_YPOS_1 + (window << 5));
    alpha_data->width = ((xreg >> 16) & 0xFFF) - (xreg & 0xFFF);
    alpha_data->height = ((yreg >> 16) & 0x7FF) - (yreg & 0x7FF);
    alpha_data->x = (xreg & 0xFFF) - hadjust;
    alpha_data->y = (yreg & 0x7FF) - vadjust;

    /* REVERSE THE GRAPHICS SCALE */

    misc = READ_VID32(DF_VID_MISC);
    if (misc & DF_USER_IMPLICIT_SCALING) {
        gfxscale = READ_REG32(DC3_GFX_SCALE);
        if (gfxscale != 0x40004000) {
            temp = alpha_data->y + alpha_data->height;
            temp = (temp * (gfxscale >> 16)) / 0x4000;

            alpha_data->x = (alpha_data->x * (gfxscale & 0xFFFF)) / 0x4000;
            alpha_data->width =
                (alpha_data->width * (gfxscale & 0xFFFF)) / 0x4000;
            alpha_data->y = (alpha_data->y * (gfxscale >> 16)) / 0x4000;
            alpha_data->height = temp - alpha_data->y;
        }
    }

    if (READ_REG32(DC3_IRQ_FILT_CTL) & DC3_IRQFILT_INTL_EN) {
        /* Y COORDINATE IS ACTUALLY 2X THE ODD FIELD START */

        alpha_data->y <<= 1;

        /* CALCULATE THE EXACT VIDEO HEIGHT */
        /* The height of the video window is the sum of the */
        /* odd and even field heights.                      */

        yreg = READ_VID32(DF_VID_ALPHA_Y_EVEN_1 + (window << 3));
        alpha_data->height += ((yreg >> 16) & 0x7FF) - (yreg & 0x7FF);
    }

    /* GET COLOR REGISTER */

    color = READ_VID32(DF_ALPHA_COLOR_1 + (window << 5));
    alpha_data->color = color & 0xFFFFFF;
    if (color & DF_ALPHA_COLOR_ENABLE)
        alpha_data->flags = DF_ALPHAFLAG_COLORENABLED;
    else
        alpha_data->flags = 0;

    /* GET ALPHA VALUE, DELTA AND PER PIXEL */

    alpha_ctl = READ_VID32(DF_ALPHA_CONTROL_1 + (window << 5));
    alpha_data->alpha_value = alpha_ctl & 0xFF;
    if (alpha_ctl & DF_ACTRL_PERPIXEL_EN)
        alpha_data->flags |= DF_ALPHAFLAG_PERPIXELENABLED;

    delta = (char) ((alpha_ctl >> 8) & 0xFF);
    alpha_data->delta = (long) delta;
    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_get_alpha_window_enable
 *
 * This routine reads the current enable status of one of the three hardware
 * alpha regions.
 *--------------------------------------------------------------------------*/

int
df_get_alpha_window_enable(int window)
{
    if (window > 2)
        return 0;

    if (READ_VID32(DF_ALPHA_CONTROL_1 + (window << 5)) & DF_ACTRL_WIN_ENABLE)
        return 1;

    return 0;
}

/*---------------------------------------------------------------------------
 * df_get_video_request
 *
 * This routine reads the horizontal (pixel) and vertical (line) video request
 * values.
 *--------------------------------------------------------------------------*/

int
df_get_video_request(unsigned long *x, unsigned long *y)
{
    unsigned long request;
    unsigned long hsyncend, htotal;
    unsigned long vsyncend, vtotal;

    hsyncend = ((READ_REG32(DC3_H_SYNC_TIMING) >> 16) & 0xFFF) + 1;
    vsyncend = ((READ_REG32(DC3_V_SYNC_TIMING) >> 16) & 0xFFF) + 1;
    htotal = ((READ_REG32(DC3_H_ACTIVE_TIMING) >> 16) & 0xFFF) + 1;
    vtotal = ((READ_REG32(DC3_V_ACTIVE_TIMING) >> 16) & 0xFFF) + 1;

    request = READ_VID32(DF_VIDEO_REQUEST);
    *x = ((request >> 16) & 0xFFF) - (htotal - hsyncend - 2);
    *y = (request & 0x7FF) - (vtotal - vsyncend + 1);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * df_get_output_color_space
 *
 * This routine sets the color space used when combining graphics and video.
 *--------------------------------------------------------------------------*/

int
df_get_output_color_space(int *color_space)
{
    unsigned long alpha_ctl;

    alpha_ctl = READ_VID32(DF_VID_ALPHA_CONTROL);

    if ((alpha_ctl & DF_CSC_VIDEO_YUV_TO_RGB) ||
        !(alpha_ctl & DF_CSC_GRAPHICS_RGB_TO_YUV)) {
        if (alpha_ctl & DF_ALPHA_DRGB)
            *color_space = DF_OUTPUT_ARGB;
        else
            *color_space = DF_OUTPUT_RGB;
    }
    else {
        *color_space = DF_OUTPUT_SDTV;

        if (alpha_ctl & DF_HD_GRAPHICS)
            *color_space = DF_OUTPUT_HDTV;
    }

    return CIM_STATUS_OK;
}

#endif
