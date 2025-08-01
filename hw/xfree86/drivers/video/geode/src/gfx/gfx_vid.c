/* <LIC_AMD_STD>
 * Copyright (C) 2005 Advanced Micro Devices, Inc.  All Rights Reserved.
 * </LIC_AMD_STD>  */
/* <CTL_AMD_STD>
 * </CTL_AMD_STD>  */
/* Copyright (c) 2005 Advanced Micro Devices, Inc.
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
 * */

/*
 * This file contains routines to control the video overlay window.
 *
 * Video overlay routines:
 * 
 *    gfx_set_clock_frequency
 *    gfx_set_crt_enable
 *    gfx_set_video_enable
 *    gfx_set_video_format
 *    gfx_set_video_size
 *    gfx_set_video_offset
 *    gfx_set_video_yuv_offsets
 *    gfx_set_video_yuv_pitch
 *    gfx_set_video_scale
 *    gfx_set_video_upscale
 *    gfx_set_video_downscale_config
 *    gfx_set_video_downscale_coefficients
 *    gfx_set_video_downscale_enable
 *    gfx_set_video_vertical_downscale
 *    gfx_set_video_vertical_downscale_enable
 *    gfx_set_video_window
 *    gfx_set_video_left_crop
 *    gfx_set_video_color_key
 *    gfx_set_video_filter
 *    gfx_set_video_palette
 *    gfx_set_video_request
 *    gfx_set_video_source
 *    gfx_set_vbi_source
 *    gfx_set_vbi_lines
 *    gfx_set_vbi_total
 *    gfx_set_video_interlaced
 *    gfx_set_color_space_YUV
 *    gfx_set_vertical_scaler_offset
 *    gfx_set_top_line_in_odd
 *    gfx_set_genlock_delay
 *    gfx_set_genlock_enable
 *    gfx_set_video_cursor
 *    gfx_set_video_cursor_enable
 * 
 * Alpha blending routines (SC1200 ONLY):
 *
 *    gfx_select_alpha_region
 *	  gfx_set_alpha_enable
 *	  gfx_set_alpha_window
 *    gfx_set_alpha_value
 *    gfx_set_alpha_priority
 *    gfx_set_alpha_color
 *    gfx_set_alpha_color_enable
 *    gfx_set_no_ck_outside_alpha
 *    gfx_test_tvout_odd_field
 *
 * And the following routines if GFX_READ_ROUTINES is set:
 *
 *    gfx_get_sync_polarities
 *    gfx_get_video_enable
 *    gfx_get_video_format
 *    gfx_get_video_src_size
 *    gfx_get_video_line_size
 *    gfx_get_video_xclip
 *    gfx_get_video_offset
 *    gfx_get_video_yuv_offsets
 *    gfx_get_video_yuv_pitch
 *    gfx_get_video_scale
 *    gfx_get_video_upscale
 *    gfx_get_video_downscale_config
 *    gfx_get_video_downscale_coefficients
 *    gfx_get_video_downscale_enable
 *    gfx_get_video_downscale_delta
 *    gfx_get_video_vertical_downscale_enable
 *    gfx_get_video_dst_size
 *    gfx_get_video_position
 *    gfx_get_video_color_key
 *    gfx_get_video_color_key_mask
 *    gfx_get_video_color_key_src
 *    gfx_get_video_filter
 *    gfx_get_video_request
 *    gfx_get_video_source
 *    gfx_get_vbi_source
 *    gfx_get_vbi_lines
 *    gfx_get_vbi_total
 *    gfx_get_video_interlaced
 *    gfx_get_color_space_YUV
 *    gfx_get_vertical_scaler_offset
 *    gfx_get_genlock_delay
 *    gfx_get_genlock_enable
 *    gfx_get_video_cursor
 *    gfx_get_clock_frequency
 *    gfx_read_crc
 *
 * Alpha blending read routines (SC1200 ONLY):
 *
 *    gfx_get_alpha_enable
 *    gfx_get_alpha_size
 *    gfx_get_alpha_value
 *    gfx_get_alpha_priority
 *    gfx_get_alpha_color
 * */

/* STATIC VARIABLES FOR VIDEO OVERLAY CONTROL */
/* These are saved to allow these routines to do clipping. */

unsigned long gfx_vid_offset = 0;       /* copy from last gfx_set_video_offset  */
unsigned long gfx_vid_uoffset = 0;      /* copy from last 
                                         * gfx_set_video_yuv_offsets                        */
unsigned long gfx_vid_voffset = 0;      /* copy from last 
                                         * gfx_set_video_yuv_offsets                        */
unsigned long gfx_vid_srcw = 300;       /* copy from last gfx_set_video_scale       */
unsigned long gfx_vid_srch = 300;       /* copy from last gfx_set_video_scale       */
unsigned long gfx_vid_dstw = 300;       /* copy from last gfx_set_video_scale       */
unsigned long gfx_vid_dsth = 300;       /* copy from last gfx_set_video_scale       */
short gfx_vid_xpos = 0;         /* copy from last gfx_set_video_window      */
short gfx_vid_ypos = 0;         /* copy from last gfx_set_video_window      */
unsigned short gfx_vid_width = 0;       /* copy from last gfx_set_video_window      */
unsigned short gfx_vid_height = 0;      /* copy from last gfx_set_video_window      */

int gfx_alpha_select = 0;       /* currently selected alpha region          */

/* INCLUDE SUPPORT FOR CS5530, IF SPECIFIED. */

#if GFX_VIDEO_CS5530
#include "vid_5530.c"
#endif

/* INCLUDE SUPPORT FOR SC1200, IF SPECIFIED. */

#if GFX_VIDEO_SC1200
#include "vid_1200.c"
#endif

/* INCLUDE SUPPORT FOR REDCLOUD, IF SPECIFIED. */

#if GFX_VIDEO_REDCLOUD
#include "vid_rdcl.c"
#endif

/*---------------------------------------------------------------------------
 * gfx_select_alpha_region
 * 
 * This routine selects which alpha region should be used for future 
 * updates.  The SC1200, for example, has 3 alpha windows available,
 * so valid parameter values are 0..2.
 *---------------------------------------------------------------------------
 */
int
gfx_select_alpha_region(int region)
{
    if (region > 2)
        return (GFX_STATUS_BAD_PARAMETER);

    gfx_alpha_select = region;
    return (GFX_STATUS_OK);
}

/* WRAPPERS IF DYNAMIC SELECTION */
/* Extra layer to call either CS5530 or SC1200 routines. */

#if GFX_VIDEO_DYNAMIC

/*---------------------------------------------------------------------------
 * gfx_reset_video (PRIVATE ROUTINE: NOT PART OF DURANGO API)
 *
 * This routine is used to disable all components of video overlay before
 * performing a mode switch.
 *---------------------------------------------------------------------------
 */
void
gfx_reset_video(void)
{
#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        cs5530_reset_video();
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        sc1200_reset_video();
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        redcloud_reset_video();
#endif
}

/*---------------------------------------------------------------------------
 * gfx_set_display_control (PRIVATE ROUTINE: NOT PART OF DURANGO API)
 *
 * This routine is used to configure the display output during a modeset
 *---------------------------------------------------------------------------
 */
int
gfx_set_display_control(int sync_polarities)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        status = cs5530_set_display_control(sync_polarities);
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_display_control(sync_polarities);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_display_control(sync_polarities);
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_set_clock_frequency
 *----------------------------------------------------------------------------
 */
void
gfx_set_clock_frequency(unsigned long frequency)
{
#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        cs5530_set_clock_frequency(frequency);
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        sc1200_set_clock_frequency(frequency);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        redcloud_set_clock_frequency(frequency);
#endif
}

/*----------------------------------------------------------------------------
 * gfx_set_crt_enable
 *----------------------------------------------------------------------------
 */
int
gfx_set_crt_enable(int enable)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_crt_enable(enable);
#endif
#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        status = cs5530_set_crt_enable(enable);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_crt_enable(enable);
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_set_video_enable
 *----------------------------------------------------------------------------
 */
int
gfx_set_video_enable(int enable)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        status = cs5530_set_video_enable(enable);
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_video_enable(enable);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_video_enable(enable);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_screen_enable (PRIVATE ROUTINE: NOT PART OF DURANGO API)
 *
 * This routine enables or disables the graphics display logic of the video 
 * processor.
 *---------------------------------------------------------------------------
 */
int
gfx_set_screen_enable(int enable)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_screen_enable(enable);
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_set_video_format
 *----------------------------------------------------------------------------
 */
int
gfx_set_video_format(unsigned long format)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        status = cs5530_set_video_format(format);
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_video_format(format);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_video_format(format);
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_set_video_size
 *----------------------------------------------------------------------------
 */
int
gfx_set_video_size(unsigned short width, unsigned short height)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        status = cs5530_set_video_size(width, height);
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_video_size(width, height);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_video_size(width, height);
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_set_video_yuv_pitch
 *----------------------------------------------------------------------------
 */
int
gfx_set_video_yuv_pitch(unsigned long ypitch, unsigned long uvpitch)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_video_yuv_pitch(ypitch, uvpitch);
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_set_video_offset
 *----------------------------------------------------------------------------
 */
int
gfx_set_video_offset(unsigned long offset)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        status = cs5530_set_video_offset(offset);
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_video_offset(offset);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_video_offset(offset);
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_set_video_yuv_offsets
 *----------------------------------------------------------------------------
 */
int
gfx_set_video_yuv_offsets(unsigned long yoffset, unsigned long uoffset,
                          unsigned long voffset)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_video_yuv_offsets(yoffset, uoffset, voffset);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_scale
 *---------------------------------------------------------------------------
 */
int
gfx_set_video_scale(unsigned short srcw, unsigned short srch,
                    unsigned short dstw, unsigned short dsth)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        status = cs5530_set_video_scale(srcw, srch, dstw, dsth);
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_video_scale(srcw, srch, dstw, dsth);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_video_scale(srcw, srch, dstw, dsth);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_upscale
 *---------------------------------------------------------------------------
 */
int
gfx_set_video_upscale(unsigned short srcw, unsigned short srch,
                      unsigned short dstw, unsigned short dsth)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_video_upscale(srcw, srch, dstw, dsth);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_vertical_downscale
 *---------------------------------------------------------------------------
 */
int
gfx_set_video_vertical_downscale(unsigned short srch, unsigned short dsth)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_video_vertical_downscale(srch, dsth);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_vertical_downscale_enable
 *---------------------------------------------------------------------------
 */
void
gfx_set_video_vertical_downscale_enable(int enable)
{
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        redcloud_set_video_vertical_downscale_enable(enable);
#endif
}

/*---------------------------------------------------------------------------
 * gfx_set_video_downscale_config
 *---------------------------------------------------------------------------
 */
int
gfx_set_video_downscale_config(unsigned short type, unsigned short m)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_video_downscale_config(type, m);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_video_downscale_config(type, m);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_downscale_coefficients
 *---------------------------------------------------------------------------
 */
int
gfx_set_video_downscale_coefficients(unsigned short coef1,
                                     unsigned short coef2, unsigned short coef3,
                                     unsigned short coef4)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status =
            sc1200_set_video_downscale_coefficients(coef1, coef2, coef3, coef4);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status =
            redcloud_set_video_downscale_coefficients(coef1, coef2, coef3,
                                                      coef4);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_downscale_enable
 *---------------------------------------------------------------------------
 */
int
gfx_set_video_downscale_enable(int enable)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_video_downscale_enable(enable);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_video_downscale_enable(enable);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_window
 *---------------------------------------------------------------------------
 */
int
gfx_set_video_window(short x, short y, unsigned short w, unsigned short h)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        status = cs5530_set_video_window(x, y, w, h);
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_video_window(x, y, w, h);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_video_window(x, y, w, h);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_left_crop
 *---------------------------------------------------------------------------
 */
int
gfx_set_video_left_crop(unsigned short x)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_video_left_crop(x);
#endif
#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        status = cs5530_set_video_left_crop(x);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_video_left_crop(x);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_color_key
 *---------------------------------------------------------------------------
 */
int
gfx_set_video_color_key(unsigned long key, unsigned long mask, int graphics)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        status = cs5530_set_video_color_key(key, mask, graphics);
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_video_color_key(key, mask, graphics);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_video_color_key(key, mask, graphics);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_filter
 *---------------------------------------------------------------------------
 */
int
gfx_set_video_filter(int xfilter, int yfilter)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        status = cs5530_set_video_filter(xfilter, yfilter);
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_video_filter(xfilter, yfilter);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_video_filter(xfilter, yfilter);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_palette
 *---------------------------------------------------------------------------
 */
int
gfx_set_video_palette(unsigned long *palette)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        status = cs5530_set_video_palette(palette);
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_video_palette(palette);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_video_palette(palette);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_graphics_palette
 *---------------------------------------------------------------------------
 */
int
gfx_set_graphics_palette(unsigned long *palette)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_graphics_palette(palette);
#endif

    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_palette_entry
 *---------------------------------------------------------------------------
 */
int
gfx_set_video_palette_entry(unsigned long index, unsigned long palette)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        status = cs5530_set_video_palette_entry(index, palette);
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_video_palette_entry(index, palette);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_video_palette_entry(index, palette);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_graphics_palette_entry
 *---------------------------------------------------------------------------
 */
int
gfx_set_graphics_palette_entry(unsigned long index, unsigned long palette)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_graphics_palette_entry(index, palette);
#endif

    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_palette_bypass
 *---------------------------------------------------------------------------
 */
int
gfx_set_video_palette_bypass(int enable)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_video_palette_bypass(enable);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_video_palette_bypass(enable);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_request
 *---------------------------------------------------------------------------
 */
int
gfx_set_video_request(short x, short y)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_video_request(x, y);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_video_request(x, y);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_source
 *---------------------------------------------------------------------------
 */
int
gfx_set_video_source(VideoSourceType source)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_video_source(source);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_vbi_source
 *---------------------------------------------------------------------------
 */
int
gfx_set_vbi_source(VbiSourceType source)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_vbi_source(source);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_vbi_lines
 *---------------------------------------------------------------------------
 */
int
gfx_set_vbi_lines(unsigned long even, unsigned long odd)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_vbi_lines(even, odd);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_vbi_total
 *---------------------------------------------------------------------------
 */
int
gfx_set_vbi_total(unsigned long even, unsigned long odd)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_vbi_total(even, odd);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_interlaced
 *---------------------------------------------------------------------------
 */
int
gfx_set_video_interlaced(int enable)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_video_interlaced(enable);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_color_space_YUV
 *---------------------------------------------------------------------------
 */
int
gfx_set_color_space_YUV(int enable)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_color_space_YUV(enable);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_vertical_scaler_offset
 *---------------------------------------------------------------------------
 */
int
gfx_set_vertical_scaler_offset(char offset)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_vertical_scaler_offset(offset);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_top_line_in_odd
 *---------------------------------------------------------------------------
 */
int
gfx_set_top_line_in_odd(int enable)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_top_line_in_odd(enable);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_genlock_delay
 *---------------------------------------------------------------------------
 */
int
gfx_set_genlock_delay(unsigned long delay)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_genlock_delay(delay);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_genlock_enable
 *---------------------------------------------------------------------------
 */
int
gfx_set_genlock_enable(int flags)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_genlock_enable(flags);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_cursor
 *---------------------------------------------------------------------------
 */
int
gfx_set_video_cursor(unsigned long key, unsigned long mask,
                     unsigned short select_color2, unsigned long color1,
                     unsigned long color2)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status =
            sc1200_set_video_cursor(key, mask, select_color2, color1, color2);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status =
            redcloud_set_video_cursor(key, mask, select_color2, color1, color2);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_cursor_enable
 *---------------------------------------------------------------------------
 */
int
gfx_set_video_cursor_enable(int enable)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_video_cursor_enable(enable);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_enable
 *---------------------------------------------------------------------------
 */
int
gfx_set_alpha_enable(int enable)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_alpha_enable(enable);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_alpha_enable(enable);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_window
 *---------------------------------------------------------------------------
 */
int
gfx_set_alpha_window(short x, short y,
                     unsigned short width, unsigned short height)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_alpha_window(x, y, width, height);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_alpha_window(x, y, width, height);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_value
 *---------------------------------------------------------------------------
 */
int
gfx_set_alpha_value(unsigned char alpha, char delta)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_alpha_value(alpha, delta);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_alpha_value(alpha, delta);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_priority
 *---------------------------------------------------------------------------
 */
int
gfx_set_alpha_priority(int priority)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_alpha_priority(priority);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_alpha_priority(priority);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_color
 *---------------------------------------------------------------------------
 */
int
gfx_set_alpha_color(unsigned long color)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_alpha_color(color);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_alpha_color(color);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_color_enable
 *---------------------------------------------------------------------------
 */
int
gfx_set_alpha_color_enable(int enable)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_alpha_color_enable(enable);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_alpha_color_enable(enable);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_no_ck_outside_alpha
 *---------------------------------------------------------------------------
 */
int
gfx_set_no_ck_outside_alpha(int enable)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_no_ck_outside_alpha(enable);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_set_no_ck_outside_alpha(enable);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_set_macrovision_enable
 *---------------------------------------------------------------------------
 */
int
gfx_set_macrovision_enable(int enable)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_set_macrovision_enable(enable);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_disable_softvga
 *---------------------------------------------------------------------------
 */
int
gfx_disable_softvga(void)
{
    int status = 0;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        status = cs5530_disable_softvga();
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_disable_softvga();
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_enable_softvga
 *---------------------------------------------------------------------------
 */
int
gfx_enable_softvga(void)
{
    int status = 0;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        status = cs5530_enable_softvga();
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_enable_softvga();
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_get_clock_frequency
 *---------------------------------------------------------------------------
 */
unsigned long
gfx_get_clock_frequency(void)
{
    unsigned long frequency = 0;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        frequency = cs5530_get_clock_frequency();
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        frequency = sc1200_get_clock_frequency();
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        frequency = redcloud_get_clock_frequency();
#endif
    return (frequency);
}

/*---------------------------------------------------------------------------
 * gfx_get_max_video_width
 *---------------------------------------------------------------------------
 */
unsigned long
gfx_get_max_video_width(void)
{
    unsigned long width = 0;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        width = cs5530_get_max_video_width();
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        width = sc1200_get_max_video_width();
#endif
    return (width);
}

/*************************************************************/
/*  READ ROUTINES  |  INCLUDED FOR DIAGNOSTIC PURPOSES ONLY  */
/*************************************************************/

#if GFX_READ_ROUTINES

/*---------------------------------------------------------------------------
 * gfx_get_vsa2_softvga_enable
 *---------------------------------------------------------------------------
 */
int
gfx_get_vsa2_softvga_enable(void)
{
    int enable = 0;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        enable = cs5530_get_vsa2_softvga_enable();
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        enable = sc1200_get_vsa2_softvga_enable();
#endif
    return enable;

}

/*---------------------------------------------------------------------------
 * gfx_get_sync_polarities
 *---------------------------------------------------------------------------
 */
int
gfx_get_sync_polarities(void)
{
    int polarities = 0;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        polarities = cs5530_get_sync_polarities();
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        polarities = sc1200_get_sync_polarities();
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        polarities = redcloud_get_sync_polarities();
#endif
    return (polarities);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_palette_entry
 *---------------------------------------------------------------------------
 */
int
gfx_get_video_palette_entry(unsigned long index, unsigned long *palette)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        status = cs5530_get_video_palette_entry(index, palette);
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_get_video_palette_entry(index, palette);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_get_video_palette_entry(index, palette);
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_get_video_enable
 *----------------------------------------------------------------------------
 */
int
gfx_get_video_enable(void)
{
    int enable = 0;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        enable = cs5530_get_video_enable();
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        enable = sc1200_get_video_enable();
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        enable = redcloud_get_video_enable();
#endif
    return (enable);
}

/*----------------------------------------------------------------------------
 * gfx_get_video_format
 *----------------------------------------------------------------------------
 */
int
gfx_get_video_format(void)
{
    int format = 0;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        format = cs5530_get_video_format();
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        format = sc1200_get_video_format();
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        format = redcloud_get_video_format();
#endif
    return (format);
}

/*----------------------------------------------------------------------------
 * gfx_get_video_src_size
 *----------------------------------------------------------------------------
 */
unsigned long
gfx_get_video_src_size(void)
{
    unsigned long size = 0;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        size = cs5530_get_video_src_size();
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        size = sc1200_get_video_src_size();
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        size = redcloud_get_video_src_size();
#endif
    return (size);
}

/*----------------------------------------------------------------------------
 * gfx_get_video_line_size
 *----------------------------------------------------------------------------
 */
unsigned long
gfx_get_video_line_size(void)
{
    unsigned long size = 0;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        size = cs5530_get_video_line_size();
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        size = sc1200_get_video_line_size();
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        size = redcloud_get_video_line_size();
#endif
    return (size);
}

/*----------------------------------------------------------------------------
 * gfx_get_video_xclip
 *----------------------------------------------------------------------------
 */
unsigned long
gfx_get_video_xclip(void)
{
    unsigned long size = 0;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        size = cs5530_get_video_xclip();
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        size = sc1200_get_video_xclip();
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        size = redcloud_get_video_xclip();
#endif
    return (size);
}

/*----------------------------------------------------------------------------
 * gfx_get_video_offset
 *----------------------------------------------------------------------------
 */
unsigned long
gfx_get_video_offset(void)
{
    unsigned long offset = 0;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        offset = cs5530_get_video_offset();
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        offset = sc1200_get_video_offset();
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        offset = redcloud_get_video_offset();
#endif
    return (offset);
}

/*----------------------------------------------------------------------------
 * gfx_get_video_yuv_offsets
 *----------------------------------------------------------------------------
 */
void
gfx_get_video_yuv_offsets(unsigned long *yoffset, unsigned long *uoffset,
                          unsigned long *voffset)
{
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        redcloud_get_video_yuv_offsets(yoffset, uoffset, voffset);
#endif
}

/*----------------------------------------------------------------------------
 * gfx_get_video_yuv_pitch
 *----------------------------------------------------------------------------
 */
void
gfx_get_video_yuv_pitch(unsigned long *ypitch, unsigned long *uvpitch)
{
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        redcloud_get_video_yuv_pitch(ypitch, uvpitch);
#endif
}

/*---------------------------------------------------------------------------
 * gfx_get_video_upscale
 *---------------------------------------------------------------------------
 */
unsigned long
gfx_get_video_upscale(void)
{
    unsigned long scale = 0;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        scale = sc1200_get_video_upscale();
#endif
    return (scale);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_scale
 *---------------------------------------------------------------------------
 */
unsigned long
gfx_get_video_scale(void)
{
    unsigned long scale = 0;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        scale = cs5530_get_video_scale();
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        scale = sc1200_get_video_scale();
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        scale = redcloud_get_video_scale();
#endif
    return (scale);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_downscale_delta
 *---------------------------------------------------------------------------
 */
unsigned long
gfx_get_video_downscale_delta(void)
{
    unsigned long delta = 0;

#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        delta = redcloud_get_video_downscale_delta();
#endif
    return (delta);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_vertical_downscale_enable
 *---------------------------------------------------------------------------
 */
int
gfx_get_video_vertical_downscale_enable(void)
{
    int enable = 0;

#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        enable = redcloud_get_video_vertical_downscale_enable();
#endif
    return (enable);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_downscale_config
 *---------------------------------------------------------------------------
 */
int
gfx_get_video_downscale_config(unsigned short *type, unsigned short *m)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_get_video_downscale_config(type, m);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_get_video_downscale_config(type, m);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_downscale_coefficients
 *---------------------------------------------------------------------------
 */
void
gfx_get_video_downscale_coefficients(unsigned short *coef1,
                                     unsigned short *coef2,
                                     unsigned short *coef3,
                                     unsigned short *coef4)
{
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        sc1200_get_video_downscale_coefficients(coef1, coef2, coef3, coef4);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        redcloud_get_video_downscale_coefficients(coef1, coef2, coef3, coef4);
#endif
}

/*---------------------------------------------------------------------------
 * gfx_get_video_downscale_enable
 *---------------------------------------------------------------------------
 */
void
gfx_get_video_downscale_enable(int *enable)
{
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        sc1200_get_video_downscale_enable(enable);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        redcloud_get_video_downscale_enable(enable);
#endif
}

/*---------------------------------------------------------------------------
 * gfx_get_video_dst_size
 *---------------------------------------------------------------------------
 */
unsigned long
gfx_get_video_dst_size(void)
{
    unsigned long size = 0;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        size = cs5530_get_video_dst_size();
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        size = sc1200_get_video_dst_size();
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        size = redcloud_get_video_dst_size();
#endif
    return (size);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_position
 *---------------------------------------------------------------------------
 */
unsigned long
gfx_get_video_position(void)
{
    unsigned long position = 0;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        position = cs5530_get_video_position();
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        position = sc1200_get_video_position();
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        position = redcloud_get_video_position();
#endif
    return (position);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_color_key
 *---------------------------------------------------------------------------
 */
unsigned long
gfx_get_video_color_key(void)
{
    unsigned long key = 0;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        key = cs5530_get_video_color_key();
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        key = sc1200_get_video_color_key();
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        key = redcloud_get_video_color_key();
#endif
    return (key);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_color_key_mask
 *---------------------------------------------------------------------------
 */
unsigned long
gfx_get_video_color_key_mask(void)
{
    unsigned long mask = 0;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        mask = cs5530_get_video_color_key_mask();
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        mask = sc1200_get_video_color_key_mask();
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        mask = redcloud_get_video_color_key_mask();
#endif
    return (mask);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_color_key_src
 *---------------------------------------------------------------------------
 */
int
gfx_get_video_color_key_src(void)
{
    int src = 0;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        src = cs5530_get_video_color_key_src();
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        src = sc1200_get_video_color_key_src();
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        src = redcloud_get_video_color_key_src();
#endif
    return (src);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_filter
 *---------------------------------------------------------------------------
 */
int
gfx_get_video_filter(void)
{
    int filter = 0;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        filter = cs5530_get_video_filter();
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        filter = sc1200_get_video_filter();
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        filter = redcloud_get_video_filter();
#endif
    return (filter);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_request
 *---------------------------------------------------------------------------
 */
int
gfx_get_video_request(short *x, short *y)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_get_video_request(x, y);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        status = redcloud_get_video_request(x, y);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_source
 *---------------------------------------------------------------------------
 */
int
gfx_get_video_source(VideoSourceType *source)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_get_video_source(source);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_get_vbi_source
 *---------------------------------------------------------------------------
 */
int
gfx_get_vbi_source(VbiSourceType *source)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_get_vbi_source(source);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_get_vbi_lines
 *---------------------------------------------------------------------------
 */
unsigned long
gfx_get_vbi_lines(int odd)
{
    unsigned long lines = (unsigned long) GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        lines = sc1200_get_vbi_lines(odd);
#endif
    return (lines);
}

/*---------------------------------------------------------------------------
 * gfx_get_vbi_total
 *---------------------------------------------------------------------------
 */
unsigned long
gfx_get_vbi_total(int odd)
{
    unsigned long total = (unsigned long) GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        total = sc1200_get_vbi_total(odd);
#endif
    return (total);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_interlaced
 *---------------------------------------------------------------------------
 */
int
gfx_get_video_interlaced(void)
{
    int interlaced = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        interlaced = sc1200_get_video_interlaced();
#endif
    return (interlaced);
}

/*---------------------------------------------------------------------------
 * gfx_get_color_space_YUV
 *---------------------------------------------------------------------------
 */
int
gfx_get_color_space_YUV(void)
{
    int color_space = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        color_space = sc1200_get_color_space_YUV();
#endif
    return (color_space);
}

/*---------------------------------------------------------------------------
 * gfx_get_vertical_scaler_offset
 *---------------------------------------------------------------------------
 */
int
gfx_get_vertical_scaler_offset(char *offset)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        status = sc1200_get_vertical_scaler_offset(offset);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_get_genlock_delay
 *---------------------------------------------------------------------------
 */
unsigned long
gfx_get_genlock_delay(void)
{
    unsigned long delay = (unsigned long) GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        delay = sc1200_get_genlock_delay();
#endif
    return (delay);
}

/*---------------------------------------------------------------------------
 * gfx_get_genlock_enable
 *---------------------------------------------------------------------------
 */
int
gfx_get_genlock_enable(void)
{
    int enable = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        enable = sc1200_get_genlock_enable();
#endif
    return (enable);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_cursor
 *---------------------------------------------------------------------------
 */
int
gfx_get_video_cursor(unsigned long *key, unsigned long *mask,
                     unsigned short *select_color2, unsigned long *color1,
                     unsigned short *color2)
{
    int enable = GFX_STATUS_UNSUPPORTED;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        enable =
            sc1200_get_video_cursor(key, mask, select_color2, color1, color2);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        enable =
            redcloud_get_video_cursor(key, mask, select_color2, color1, color2);
#endif
    return (enable);
}

/*---------------------------------------------------------------------------
 * gfx_read_crc
 *---------------------------------------------------------------------------
 */
unsigned long
gfx_read_crc(void)
{
    unsigned long crc = 0;

#if GFX_VIDEO_CS5530
    if (gfx_video_type == GFX_VIDEO_TYPE_CS5530)
        crc = cs5530_read_crc();
#endif
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        crc = sc1200_read_crc();
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        crc = redcloud_read_crc();
#endif
    return (crc);
}

/*---------------------------------------------------------------------------
 * gfx_read_crc32
 *---------------------------------------------------------------------------
 */
unsigned long
gfx_read_crc32(void)
{
    unsigned long crc = 0;

#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        crc = redcloud_read_crc32();
#endif
    return (crc);
}

/*---------------------------------------------------------------------------
 * gfx_read_window_crc
 *---------------------------------------------------------------------------
 */
unsigned long
gfx_read_window_crc(int source, unsigned short x, unsigned short y,
                    unsigned short width, unsigned short height, int crc32)
{
    unsigned long crc = 0;

#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        crc = redcloud_read_window_crc(source, x, y, width, height, crc32);
#endif
    return (crc);
}

/*-----------------------------------------------------------------------------
 * gfx_get_macrovision_enable
 *-----------------------------------------------------------------------------
 */
int
gfx_get_macrovision_enable(void)
{
    int enable = 0;

#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        enable = sc1200_get_video_enable();
#endif
    return (enable);
}

/*---------------------------------------------------------------------------
 * gfx_get_alpha_enable
 *---------------------------------------------------------------------------
 */
void
gfx_get_alpha_enable(int *enable)
{
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        sc1200_get_alpha_enable(enable);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        redcloud_get_alpha_enable(enable);
#endif
    return;
}

/*---------------------------------------------------------------------------
 * gfx_get_alpha_size
 *---------------------------------------------------------------------------
 */
void
gfx_get_alpha_size(unsigned short *x, unsigned short *y,
                   unsigned short *width, unsigned short *height)
{
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        sc1200_get_alpha_size(x, y, width, height);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        redcloud_get_alpha_size(x, y, width, height);
#endif
    return;
}

/*---------------------------------------------------------------------------
 * gfx_get_alpha_value
 *---------------------------------------------------------------------------
 */
void
gfx_get_alpha_value(unsigned char *alpha, char *delta)
{
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        sc1200_get_alpha_value(alpha, delta);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        redcloud_get_alpha_value(alpha, delta);
#endif
    return;
}

/*---------------------------------------------------------------------------
 * gfx_get_alpha_priority
 *---------------------------------------------------------------------------
 */
void
gfx_get_alpha_priority(int *priority)
{
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        sc1200_get_alpha_priority(priority);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        redcloud_get_alpha_priority(priority);
#endif
    return;
}

/*---------------------------------------------------------------------------
 * gfx_get_alpha_color
 *---------------------------------------------------------------------------
 */
void
gfx_get_alpha_color(unsigned long *color)
{
#if GFX_VIDEO_SC1200
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200)
        sc1200_get_alpha_color(color);
#endif
#if GFX_VIDEO_REDCLOUD
    if (gfx_video_type == GFX_VIDEO_TYPE_REDCLOUD)
        redcloud_get_alpha_color(color);
#endif
    return;
}

#endif                          /* GFX_READ_ROUTINES */

#endif                          /* GFX_VIDEO_DYNAMIC */

/* END OF FILE */
