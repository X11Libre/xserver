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
 * This file contains routines to program TVOUT and TV encoder. 
 *
 * Routines:
 * 
 *    gfx_set_tv_format
 *    gfx_set_tv_output
 *    gfx_set_tv_enable
 *    gfx_set_tv_flicker_filter
 *    gfx_set_tv_sub_carrier_reset
 *    gfx_set_tv_vphase
 *    gfx_set_tv_YC_delay
 *    gfx_set_tvenc_reset_interval
 *    gfx_set_tv_cc_enable
 *    gfx_set_tv_cc_data
 *    gfx_test_tvout_odd_field
 *    gfx_test_tvenc_odd_field
 *    gfx_set_tv_field_status_invert
 *    gfx_get_tv_vphase
 * */

/* TV TIMINGS */

DISPLAYMODE TVTimings[] = {

/* NTSC resolution */

    {0x3 |                      /* negative syncs       */
     GFX_MODE_TV_NTSC,          /* NTSC format          */
     640, 640, 656, 744, 792, 792,      /* horizontal timings   */
     480, 480, 490, 492, 517, 525,      /* vertical timings     */
     0x0018EC4D,                /* freq = 24.923052 MHz */
     }
    ,

/* PAL resolution */

    {0x3 |                      /* negative syncs       */
     GFX_MODE_TV_PAL,           /* PAL format           */
     768, 768, 800, 848, 864, 864,      /* horizontal timings   */
     576, 576, 586, 588, 625, 625,      /* vertical timings     */
     0x001B0000,                /* freq = 27.00 MHz     */
     }
    ,

/* NTSC resolution non-square pixels */

    {0x3 |                      /* negative syncs       */
     GFX_MODE_TV_NTSC,          /* NTSC format          */
     720, 720, 736, 752, 792, 792,      /* horizontal timings   */
     480, 480, 490, 492, 517, 525,      /* vertical timings     */
     0x0018EC4D,                /* freq = 24.923052 MHz */
     }
    ,

/* PAL resolution non-square pixels */

    {0x3 |                      /* negative syncs       */
     GFX_MODE_TV_PAL,           /* PAL format           */
     720, 720, 752, 816, 864, 864,      /* horizontal timings   */
     576, 576, 586, 588, 625, 625,      /* vertical timings     */
     0x001B0000,                /* freq = 27.00 MHz     */
     }
};

#define NUM_TV_MODES sizeof(TVTimings)/sizeof(DISPLAYMODE)

/* INCLUDE SUPPORT FOR SC1200 TV ENCODER, IF SPECIFIED */

#if GFX_TV_SC1200
#include "tv_1200.c"
#endif

/* INCLUDE SUPPORT FOR FS450 TV ENCODER, IF SPECIFIED */

#if GFX_TV_FS451
#include "tv_fs450.c"
#endif

/* WRAPPERS IF DYNAMIC SELECTION */
/* Extra layer to call either SC1200 or FS450 TV encoder routines. */

#if GFX_TV_DYNAMIC

/*----------------------------------------------------------------------------
 * gfx_set_tv_format
 *----------------------------------------------------------------------------
 */
int
gfx_set_tv_format(TVStandardType format, GfxOnTVType resolution)
{
    int retval = GFX_STATUS_UNSUPPORTED;

#if GFX_TV_SC1200
    if (gfx_tv_type & GFX_TV_TYPE_SC1200)
        retval = sc1200_set_tv_format(format, resolution);
#endif
#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_set_tv_format(format, resolution);
#endif
    return (retval);
}

/*----------------------------------------------------------------------------
 * gfx_set_tv_output
 *----------------------------------------------------------------------------
 */
int
gfx_set_tv_output(int output)
{
    int retval = GFX_STATUS_UNSUPPORTED;

#if GFX_TV_SC1200
    if (gfx_tv_type & GFX_TV_TYPE_SC1200)
        retval = sc1200_set_tv_output(output);
#endif
#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_set_tv_output(output);
#endif
    return (retval);
}

/*----------------------------------------------------------------------------
 * gfx_set_tv_enable
 *----------------------------------------------------------------------------
 */
int
gfx_set_tv_enable(int enable)
{
    int retval = GFX_STATUS_UNSUPPORTED;

#if GFX_TV_SC1200
    if (gfx_tv_type & GFX_TV_TYPE_SC1200)
        retval = sc1200_set_tv_enable(enable);
#endif
#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_set_tv_enable(enable);
#endif
    return (retval);
}

/*----------------------------------------------------------------------------
 * gfx_set_tv_flicker_filter
 *----------------------------------------------------------------------------
 */
int
gfx_set_tv_flicker_filter(int ff)
{
    int retval = GFX_STATUS_UNSUPPORTED;

#if GFX_TV_SC1200
    if (gfx_tv_type & GFX_TV_TYPE_SC1200)
        retval = sc1200_set_tv_flicker_filter(ff);
#endif
    return (retval);
}

/*----------------------------------------------------------------------------
 * gfx_set_tv_sub_carrier_reset
 *----------------------------------------------------------------------------
 */
int
gfx_set_tv_sub_carrier_reset(int screset)
{
    int retval = GFX_STATUS_UNSUPPORTED;

#if GFX_TV_SC1200
    if (gfx_tv_type & GFX_TV_TYPE_SC1200)
        retval = sc1200_set_tv_sub_carrier_reset(screset);
#endif
    return (retval);
}

/*----------------------------------------------------------------------------
 * gfx_set_tv_vphase
 *----------------------------------------------------------------------------
 */
int
gfx_set_tv_vphase(int vphase)
{
    int retval = GFX_STATUS_UNSUPPORTED;

#if GFX_TV_SC1200
    if (gfx_tv_type & GFX_TV_TYPE_SC1200)
        retval = sc1200_set_tv_vphase(vphase);
#endif
    return (retval);
}

/*----------------------------------------------------------------------------
 * gfx_set_tv_YC_delay
 *----------------------------------------------------------------------------
 */
int
gfx_set_tv_YC_delay(int delay)
{
    int retval = GFX_STATUS_UNSUPPORTED;

#if GFX_TV_SC1200
    if (gfx_tv_type & GFX_TV_TYPE_SC1200)
        retval = sc1200_set_tv_YC_delay(delay);
#endif
    return (retval);
}

/*----------------------------------------------------------------------------
 * gfx_set_tvenc_reset_interval
 *----------------------------------------------------------------------------
 */
int
gfx_set_tvenc_reset_interval(int interval)
{
    int retval = GFX_STATUS_UNSUPPORTED;

#if GFX_TV_SC1200
    if (gfx_tv_type & GFX_TV_TYPE_SC1200)
        retval = sc1200_set_tvenc_reset_interval(interval);
#endif
    return (retval);
}

/*----------------------------------------------------------------------------
 * gfx_set_tv_cc_enable
 *----------------------------------------------------------------------------
 */
int
gfx_set_tv_cc_enable(int enable)
{
    int retval = GFX_STATUS_UNSUPPORTED;

#if GFX_TV_SC1200
    if (gfx_tv_type & GFX_TV_TYPE_SC1200)
        retval = sc1200_set_tv_cc_enable(enable);
#endif
    return (retval);
}

/*----------------------------------------------------------------------------
 * gfx_set_tv_cc_data
 *
 * This routine writes the two specified characters to the CC data register 
 * of the TV encoder.
 *----------------------------------------------------------------------------
 */
int
gfx_set_tv_cc_data(unsigned char data1, unsigned char data2)
{
    int retval = GFX_STATUS_UNSUPPORTED;

#if GFX_TV_SC1200
    if (gfx_tv_type & GFX_TV_TYPE_SC1200)
        retval = sc1200_set_tv_cc_data(data1, data2);
#endif
    return (retval);
}

/*---------------------------------------------------------------------------
 * gfx_set_tv_display
 *
 * Set the timings in the display controller to support a TV resolution.
 *---------------------------------------------------------------------------
 */
int
gfx_set_tv_display(int width, int height)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_TV_SC1200
    if (gfx_tv_type & GFX_TV_TYPE_SC1200)
        status = sc1200_set_tv_display(width, height);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_test_tvout_odd_field
 *---------------------------------------------------------------------------
 */
int
gfx_test_tvout_odd_field(void)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_TV_SC1200
    if (gfx_tv_type & GFX_TV_TYPE_SC1200)
        status = sc1200_test_tvout_odd_field();
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_test_tvenc_odd_field
 *---------------------------------------------------------------------------
 */
int
gfx_test_tvenc_odd_field(void)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_TV_SC1200
    if (gfx_tv_type & GFX_TV_TYPE_SC1200)
        status = sc1200_test_tvenc_odd_field();
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_set_tv_field_status_invert
 *----------------------------------------------------------------------------
 */
int
gfx_set_tv_field_status_invert(int enable)
{
    int retval = GFX_STATUS_UNSUPPORTED;

#if GFX_TV_SC1200
    if (gfx_tv_type & GFX_TV_TYPE_SC1200)
        retval = sc1200_set_tv_field_status_invert(enable);
#endif
    return (retval);
}

/*---------------------------------------------------------------------------
 * gfx_get_tv_vphase
 *---------------------------------------------------------------------------
 */
int
gfx_get_tv_vphase(void)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_TV_SC1200
    if (gfx_tv_type & GFX_TV_TYPE_SC1200)
        status = sc1200_get_tv_vphase();
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_get_tv_enable
 *---------------------------------------------------------------------------
 */
int
gfx_get_tv_enable(unsigned int *p_on)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_get_tv_enable(p_on);
#endif
#if GFX_TV_SC1200
    if (gfx_tv_type & GFX_TV_TYPE_SC1200)
        retval = sc1200_get_tv_enable(p_on);
#endif
    return (retval);
}

/*---------------------------------------------------------------------------
 * gfx_get_tv_output
 *---------------------------------------------------------------------------
 */
int
gfx_get_tv_output()
{
    int retval = -1;

#if GFX_TV_SC1200
    if (gfx_tv_type & GFX_TV_TYPE_SC1200)
        retval = sc1200_get_tv_output();
#endif
    return (retval);
}

/*---------------------------------------------------------------------------
 * gfx_get_tv_mode_count 
 *---------------------------------------------------------------------------
 */
int
gfx_get_tv_mode_count(TVStandardType format)
{
    int retval = -1;

#if GFX_TV_SC1200
    if (gfx_tv_type & GFX_TV_TYPE_SC1200)
        retval = sc1200_get_tv_mode_count(format);
#endif
    return (retval);
}

/*---------------------------------------------------------------------------
 * gfx_get_tv_display_mode 
 *---------------------------------------------------------------------------
 */
int
gfx_get_tv_display_mode(int *width, int *height, int *bpp, int *hz)
{
    int retval = -1;

#if GFX_TV_SC1200
    if (gfx_tv_type & GFX_TV_TYPE_SC1200)
        retval = sc1200_get_tv_display_mode(width, height, bpp, hz);
#endif
    return (retval);
}

/*---------------------------------------------------------------------------
 * gfx_get_tv_display_mode_frequency 
 *---------------------------------------------------------------------------
 */
int
gfx_get_tv_display_mode_frequency(unsigned short width, unsigned short height,
                                  TVStandardType format, int *frequency)
{
    int retval = -1;

#if GFX_TV_SC1200
    if (gfx_tv_type & GFX_TV_TYPE_SC1200)
        retval =
            sc1200_get_tv_display_mode_frequency(width, height, format,
                                                 frequency);
#endif
    return (retval);
}

/*---------------------------------------------------------------------------
 * gfx_is_tv_display_mode_supported
 *---------------------------------------------------------------------------
 */
int
gfx_is_tv_display_mode_supported(unsigned short width, unsigned short height,
                                 TVStandardType format)
{
    int retval = -1;

#if GFX_TV_SC1200
    if (gfx_tv_type & GFX_TV_TYPE_SC1200)
        retval = sc1200_is_tv_display_mode_supported(width, height, format);
#endif
    return (retval);
}

/*------------------------------------------
 * The following functions were added to support
 * the FS450 and will eventually be removed.  There 
 * is no equivalent support in the SC1200.
 *----------------------------------------------*/

/*==========================================================================
 *	TV standard
 *==========================================================================
 */
int
gfx_get_tv_standard(unsigned long *p_standard)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_get_tv_standard(p_standard);
#endif
    return (retval);
}

int
gfx_get_available_tv_standards(unsigned long *p_standards)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_get_available_tv_standards(p_standards);
#endif
    return (retval);
}

int
gfx_set_tv_standard(unsigned long standard)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_set_tv_standard(standard);
#endif
    return (retval);
}

/*
 *==========================================================================
 *	vga mode as known by the driver
 *==========================================================================
 */
int
gfx_get_tv_vga_mode(unsigned long *p_vga_mode)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_get_tv_vga_mode(p_vga_mode);
#endif
    return (retval);
}

int
gfx_get_available_tv_vga_modes(unsigned long *p_vga_modes)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_get_available_tv_vga_modes(p_vga_modes);
#endif
    return (retval);
}

int
gfx_set_tv_vga_mode(unsigned long vga_mode)
{
    int re tval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_set_tv_vga_mode(vga_mode);
#endif
    return (retval);
}

/*
 *==========================================================================
 *
 *	tvout mode
 */

int
gfx_get_tvout_mode(unsigned long *p_tvout_mode)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_get_tvout_mode(p_tvout_mode);
#endif
    return (retval);
}

int
gfx_set_tvout_mode(unsigned long tvout_mode)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_set_tvout_mode(tvout_mode);
#endif
    return (retval);
}

/*
 *==========================================================================
 *
 *	Sharpness
 */
int
gfx_get_sharpness(int *p_sharpness)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_get_sharpness(p_sharpness);
#endif
    return (retval);
}

int
gfx_set_sharpness(int sharpness)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_set_sharpness(sharpness);
#endif
    return (retval);
}

/*
 *==========================================================================
 *
 * flicker filter control.
 */

int
gfx_get_flicker_filter(int *p_flicker)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_get_flicker_filter(p_flicker);
#endif
    return (retval);
}

int
gfx_set_flicker_filter(int flicker)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_set_flicker_filter(flicker);
#endif
    return (retval);
}

/*
 *==========================================================================
 *
 *	Overscan and Position
 */

int
gfx_get_overscan(int *p_x, int *p_y)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_get_overscan(p_x, p_y);
#endif
    return (retval);

}

int
gfx_set_overscan(int x, int y)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_set_overscan(x, y);
#endif
    return (retval);
}

int
gfx_get_position(int *p_x, int *p_y)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_get_position(p_x, p_y);
#endif
    return (retval);
}

int
gfx_set_position(int x, int y)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_set_position(x, y);
#endif
    return (retval);
}

/*
 *==========================================================================
 *
 *	Color, Brightness, and Contrast
 */

int
gfx_get_color(int *p_color)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_get_color(p_color);
#endif
    return (retval);
}

int
gfx_set_color(int color)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_set_color(color);
#endif
    return (retval);
}

int
gfx_get_brightness(int *p_brightness)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_get_brightness(p_brightness);
#endif
    return (retval);
}

int
gfx_set_brightness(int brightness)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_set_brightness(brightness);
#endif
    return (retval);
}

int
gfx_get_contrast(int *p_contrast)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_get_contrast(p_contrast);
#endif
    return (retval);
}

int
gfx_set_contrast(int contrast)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_set_contrast(contrast);
#endif
    return (retval);
}

/*
 *==========================================================================
 *
 *	YC filters
 */

int
gfx_get_yc_filter(unsigned int *p_yc_filter)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_get_yc_filter(p_yc_filter);
#endif
    return (retval);
}

int
gfx_set_yc_filter(unsigned int yc_filter)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_set_yc_filter(yc_filter);
#endif
    return (retval);
}

int
gfx_get_aps_trigger_bits(unsigned int *p_trigger_bits)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_get_aps_trigger_bits(p_trigger_bits);
#endif
    return (retval);
}

int
gfx_set_aps_trigger_bits(unsigned int trigger_bits)
{
    int retval = -1;

#if GFX_TV_FS451
    if (gfx_tv_type & GFX_TV_TYPE_FS451)
        retval = fs450_set_aps_trigger_bits(trigger_bits);
#endif
    return (retval);
}

#endif                          /* GFX_TV_DYNAMIC */

/* END OF FILE */
