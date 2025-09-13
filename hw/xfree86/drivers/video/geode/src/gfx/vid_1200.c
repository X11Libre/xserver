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
 * This file contains routines to control the SC1200 video overlay hardware.
 * */

/*----------------------------------------------------------------------------
 * SC1200 GAMMA RAM TABLE
 *----------------------------------------------------------------------------
 */

unsigned long gfx_gamma_ram_1200[] = {
    0x00000000, 0x00040003, 0x00050104, 0x00060205, 0x00070306, 0x00080407,
    0x00090508, 0x000A0609, 0x000B070A, 0x000C080B, 0x000D090C, 0x000E0A0D,
    0x000F0B0E, 0x00100C0F, 0x00110D10, 0x00110E11, 0x00120F12, 0x00141013,
    0x00151114, 0x00161215, 0x00171316, 0x00181417, 0x00191518, 0x001A1619,
    0x001B171A, 0x001C181B, 0x001D191C, 0x001D1A1D, 0x001E1B1E, 0x00201C1F,
    0x00201D20, 0x00221E21, 0x00231F22, 0x00242023, 0x00252124, 0x00262225,
    0x00262326, 0x00282427, 0x00292528, 0x00292629, 0x002B272A, 0x002C282B,
    0x002D292C, 0x002E2A2D, 0x002F2B2E, 0x00302C2F, 0x00312D30, 0x00322E31,
    0x00332F32, 0x00343033, 0x00353134, 0x00363235, 0x00373336, 0x00383437,
    0x00393538, 0x003A3639, 0x003B373A, 0x003C383B, 0x003D393C, 0x003E3A3D,
    0x003F3B3E, 0x00403C3F, 0x00413D40, 0x00423E41, 0x00433F42, 0x00444043,
    0x00444144, 0x00454245, 0x00474346, 0x00484447, 0x00494548, 0x004A4649,
    0x004B474A, 0x004C484B, 0x004D494C, 0x004E4A4D, 0x004F4B4E, 0x00504C4F,
    0x00514D50, 0x00524E51, 0x00534F52, 0x00545053, 0x00555154, 0x00565255,
    0x00575356, 0x00585457, 0x00595558, 0x005A5659, 0x005B575A, 0x005C585B,
    0x005D595C, 0x005E5A5D, 0x005F5B5E, 0x00605C5F, 0x00615D60, 0x00625E61,
    0x00635F62, 0x00646063, 0x00656164, 0x00666265, 0x00676366, 0x00686467,
    0x00696568, 0x006A6669, 0x006B676A, 0x006C686B, 0x006D696C, 0x006E6A6D,
    0x006F6B6E, 0x00706C6F, 0x00716D70, 0x00726E71, 0x00736F72, 0x00747073,
    0x00757174, 0x00767275, 0x00777376, 0x00787477, 0x00797578, 0x007A7679,
    0x007B777A, 0x007C787B, 0x007D797C, 0x007E7A7D, 0x007F7B7E, 0x00807C7F,
    0x00817D80, 0x00827E81, 0x00837F82, 0x00848083, 0x00858184, 0x00868285,
    0x00878386, 0x00888487, 0x00898588, 0x008A8689, 0x008B878A, 0x008C888B,
    0x008D898C, 0x008E8A8D, 0x008F8B8E, 0x00908C8F, 0x00918D90, 0x00928E91,
    0x00938F92, 0x00949093, 0x00959194, 0x00969295, 0x00979396, 0x00989497,
    0x00999598, 0x009A9699, 0x009B979A, 0x009C989B, 0x009D999C, 0x009E9A9D,
    0x009F9B9E, 0x00A09C9F, 0x00A19DA0, 0x00A29EA1, 0x00A39FA2, 0x00A4A0A3,
    0x00A5A1A4, 0x00A6A2A5, 0x00A7A3A6, 0x00A8A4A7, 0x00A9A5A8, 0x00AAA6A9,
    0x00ABA7AA, 0x00ACA8AB, 0x00ADA9AC, 0x00AEAAAD, 0x00AFABAE, 0x00B0ACAF,
    0x00B1ADB0, 0x00B2AEB1, 0x00B3AFB2, 0x00B4B0B3, 0x00B5B1B4, 0x00B6B2B5,
    0x00B7B3B6, 0x00B8B4B7, 0x00B9B5B8, 0x00BAB6B9, 0x00BBB7BA, 0x00BCB8BB,
    0x00BDB9BC, 0x00BEBABD, 0x00BFBBBE, 0x00C0BCBF, 0x00C1BDC0, 0x00C2BEC1,
    0x00C3BFC2, 0x00C4C0C3, 0x00C5C1C4, 0x00C6C2C5, 0x00C7C3C6, 0x00C8C4C7,
    0x00C9C5C8, 0x00CAC6C9, 0x00CBC7CA, 0x00CCC8CB, 0x00CDC9CC, 0x00CECACD,
    0x00CFCBCE, 0x00D0CCCF, 0x00D1CDD0, 0x00D2CED1, 0x00D3CFD2, 0x00D4D0D3,
    0x00D5D1D4, 0x00D6D2D5, 0x00D7D3D6, 0x00D8D4D7, 0x00D9D5D8, 0x00DAD6D9,
    0x00DBD7DA, 0x00DCD8DB, 0x00DDD9DC, 0x00DEDADD, 0x00DFDBDE, 0x00E0DCDF,
    0x00E1DDE0, 0x00E2DEE1, 0x00E3DFE2, 0x00E4E0E3, 0x00E5E1E4, 0x00E6E2E5,
    0x00E7E3E6, 0x00E8E4E7, 0x00E9E5E8, 0x00EAE6E9, 0x00EBE7EA, 0x00ECE8EB,
    0x00EDE9EC, 0x00EEEAED, 0x00EFEBEE, 0x00F0ECEF, 0x00F1EDF0, 0x00F2EEF1,
    0x00F3EFF2, 0x00F4F0F3, 0x00F5F1F4, 0x00F6F2F5, 0x00F7F3F6, 0x00F8F4F7,
    0x00F9F5F8, 0x00FAF6F9, 0x00FBF7FA, 0x00FCF8FB, 0x00FDF9FC, 0x00FEFAFD,
    0x00FFFBFE, 0x00FFFCFE, 0x00FFFDFE, 0x00FFFFFF
};

/*----------------------------------------------------------------------------
 * SC1200 PLL TABLE
 *----------------------------------------------------------------------------
 */

typedef struct tagSC1200PLL {
    long frequency;             /* 16.16 fixed point frequency */
    unsigned long clock_select; /* clock select register (0x2C) */
} SC1200PLL;

SC1200PLL gfx_sc1200_clock_table[] = {
    {(25L << 16) | ((1750L * 65536L) / 10000L), 0x0070E00C},    /* 25.1750 
                                                                 * (sc=24.9231) */
    {(27L << 16) | ((0000L * 65536L) / 10000L), 0x00300100},    /* 27.0000 */
    {(28L << 16) | ((3220L * 65536L) / 10000L), 0x0070EC0C},    /* 28.3220 
                                                                 * (SC=27.000) */
    {(31L << 16) | ((5000L * 65536L) / 10000L), 0x00500D02},    /* 31.5000 */
    {(36L << 16) | ((0000L * 65536L) / 10000L), 0x00500F02},    /* 36.0000 */
    {(37L << 16) | ((5000L * 65536L) / 10000L), 0x0050B108},    /* 37.5000 */
    {(40L << 16) | ((0000L * 65536L) / 10000L), 0x0050D20D},    /* 40.0000 */
    {(44L << 16) | ((9000L * 65536L) / 10000L), 0x0050DC0D},    /* 44.9000 */
    {(49L << 16) | ((5000L * 65536L) / 10000L), 0x00501502},    /* 49.5000 */
    {(50L << 16) | ((0000L * 65536L) / 10000L), 0x0050A404},    /* 50.0000 */
    {(50L << 16) | ((3500L * 65536L) / 10000L), 0x0050E00C},    /* 50.3500 */
    {(54L << 16) | ((0000L * 65536L) / 10000L), 0x00300300},    /* 54.0000 */
    {(56L << 16) | ((3916L * 65536L) / 10000L), 0x0050F40D},    /* 56.3916 */
    {(56L << 16) | ((6440L * 65536L) / 10000L), 0x0050EC0C},    /* 56.6440 */
    {(59L << 16) | ((0000L * 65536L) / 10000L), 0x0030A207},    /* 59.0000 */
    {(63L << 16) | ((0000L * 65536L) / 10000L), 0x00300D02},    /* 63.0000 */
    {(65L << 16) | ((0000L * 65536L) / 10000L), 0x0030CC0F},    /* 65.0000 */
    {(67L << 16) | ((5000L * 65536L) / 10000L), 0x00300400},    /* 67.5000 */
    {(70L << 16) | ((8000L * 65536L) / 10000L), 0x00301403},    /* 70.8000 */
    {(72L << 16) | ((0000L * 65536L) / 10000L), 0x00300F02},    /* 72.0000 */
    {(75L << 16) | ((0000L * 65536L) / 10000L), 0x0030B108},    /* 75.0000 */
    {(78L << 16) | ((7500L * 65536L) / 10000L), 0x0030A205},    /* 78.7500 */
    {(80L << 16) | ((0000L * 65536L) / 10000L), 0x0030D20D},    /* 80.0000 */
    {(87L << 16) | ((2728L * 65536L) / 10000L), 0x0030E00E},    /* 87.2728 */
    {(89L << 16) | ((8000L * 65536L) / 10000L), 0x0030DC0D},    /* 89.8000 */
    {(94L << 16) | ((5000L * 65536L) / 10000L), 0x00300600},    /* 99.0000 */
    {(99L << 16) | ((0000L * 65536L) / 10000L), 0x00301502},    /* 99.0000 */
    {(100L << 16) | ((0000L * 65536L) / 10000L), 0x0030A404},   /* 100.00  */
    {(108L << 16) | ((0000L * 65536L) / 10000L), 0x00100300},   /* 108.00  */
    {(112L << 16) | ((5000L * 65536L) / 10000L), 0x00301802},   /* 108.00  */
    {(130L << 16) | ((0000L * 65536L) / 10000L), 0x0010CC0F},   /* 130.00  */
    {(135L << 16) | ((0000L * 65536L) / 10000L), 0x00100400},   /* 135.00  */
    {(157L << 16) | ((5000L * 65536L) / 10000L), 0x0010A205},   /* 157.50  */
    {(162L << 16) | ((0000L * 65536L) / 10000L), 0x00100500},   /* 162.00  */
    {(175L << 16) | ((0000L * 65536L) / 10000L), 0x0010E00E},   /* 175.50  */
    {(189L << 16) | ((0000L * 65536L) / 10000L), 0x00100600},   /* 189.00  */
    {(202L << 16) | ((0000L * 65536L) / 10000L), 0x0010EF0E},   /* 202.50  */
    {(232L << 16) | ((0000L * 65536L) / 10000L), 0x0010AA04},   /* 232.50  */

    /* Precomputed inidces in the hardware */
    {0x0018EC4D, 0x000F0000},   /*  24.923052 */
    {0x00192CCC, 0x00000000},   /*  25.1750  */
    {0x001B0000, 0x00300100},   /*  27.0000  */
    {0x001F8000, 0x00010000},   /*  31.5000  */
    {0x00240000, 0x00020000},   /*  36.0000  */
    {0x00280000, 0x00030000},   /*  40.0000  */
    {0x00318000, 0x00050000},   /*  49.5000  */
    {0x00320000, 0x00040000},   /*  50.0000  */
    {0x00384000, 0x00060000},   /*  56.2500  */
    {0x00410000, 0x00080000},   /*  65.0000  */
    {0x004E8000, 0x000A0000},   /*  78.5000  */
    {0x005E8000, 0x000B0000},   /*  94.5000  */
    {0x006C0000, 0x000C0000},   /* 108.0000  */
    {0x00870000, 0x000D0000},   /* 135.0000  */
};

#define NUM_SC1200_FREQUENCIES	\
		sizeof(gfx_sc1200_clock_table)/sizeof(SC1200PLL)

/*---------------------------------------------------------------------------
 * gfx_reset_video (PRIVATE ROUTINE: NOT PART OF DURANGO API)
 *
 * This routine is used to disable all components of video overlay before
 * performing a mode switch.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void
sc1200_reset_video(void)
#else
void
gfx_reset_video(void)
#endif
{
    int i;

    gfx_set_video_enable(0);

    /* SET WINDOW 0 AFTER RESET */

    for (i = 2; i >= 0; i--) {
        gfx_select_alpha_region(i);
        gfx_set_alpha_enable(0);
        gfx_set_alpha_color_enable(0);
    }

    /* CLEAR THE DISPLAY BUFFER SIZE TO ZERO                               */
    /* During a modeset,if FIFO load is enabled with a large buffer size,  */
    /* the FIFO can hang.  To prevent this, we set the buffer size to zero */
    /* and wait for this new size to be latched                            */

    gfx_set_display_video_size(0, 0);

    if (gfx_test_timing_active()) {
        while (!gfx_test_vertical_active());
        while (gfx_test_vertical_active());
        while (!gfx_test_vertical_active());
        while (gfx_test_vertical_active());
    }
}

/*-----------------------------------------------------------------------------
 * gfx_set_display_control (PRIVATE ROUTINE: NOT PART OF DURANGO API)
 *
 * This routine configures the display output.
 *
 * "sync_polarities" is used to set the polarities of the sync pulses 
 * according to the following mask:
 *
 *     Bit 0: If set to 1, negative horizontal polarity is programmed,
 *            otherwise positive horizontal polarity is programmed.
 *     Bit 1: If set to 1, negative vertical polarity is programmed,
 *            otherwise positive vertical polarity is programmed.
 *
 *-----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_display_control(int sync_polarities)
#else
int
gfx_set_display_control(int sync_polarities)
#endif
{
    unsigned long dcfg;

    /* CONFIGURE DISPLAY OUTPUT FROM VIDEO PROCESSOR */

    dcfg = READ_VID32(SC1200_DISPLAY_CONFIG);
    dcfg &= ~(SC1200_DCFG_CRT_SYNC_SKW_MASK | SC1200_DCFG_PWR_SEQ_DLY_MASK |
              SC1200_DCFG_CRT_HSYNC_POL | SC1200_DCFG_CRT_VSYNC_POL |
              SC1200_DCFG_FP_PWR_EN | SC1200_DCFG_FP_DATA_EN);

    dcfg |= (SC1200_DCFG_CRT_SYNC_SKW_INIT |
             SC1200_DCFG_PWR_SEQ_DLY_INIT | SC1200_DCFG_GV_PAL_BYP);

    if (PanelEnable)
        dcfg |= SC1200_DCFG_FP_PWR_EN;

    /* SET APPROPRIATE SYNC POLARITIES */

    if (sync_polarities & 0x1)
        dcfg |= SC1200_DCFG_CRT_HSYNC_POL;
    if (sync_polarities & 0x2)
        dcfg |= SC1200_DCFG_CRT_VSYNC_POL;

    WRITE_VID32(SC1200_DISPLAY_CONFIG, dcfg);

    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_clock_frequency
 *
 * This routine sets the clock frequency, specified as a 16.16 fixed point
 * value (0x00318000 = 49.5 MHz).  It will set the closest frequency found
 * in the lookup table.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void
sc1200_set_clock_frequency(unsigned long frequency)
#else
void
gfx_set_clock_frequency(unsigned long frequency)
#endif
{
    unsigned int index;
    unsigned long value, pll;
    long min, diff;

    /* FIND THE REGISTER VALUES FOR THE DESIRED FREQUENCY */
    /* Search the table for the closest frequency (16.16 format). */

    value = gfx_sc1200_clock_table[0].clock_select;
    min = (long) gfx_sc1200_clock_table[0].frequency - frequency;
    if (min < 0L)
        min = -min;
    for (index = 1; index < NUM_SC1200_FREQUENCIES; index++) {
        diff = (long) gfx_sc1200_clock_table[index].frequency - frequency;
        if (diff < 0L)
            diff = -diff;
        if (diff < min) {
            min = diff;
            value = gfx_sc1200_clock_table[index].clock_select;
        }
    }

    /* SET THE DOT CLOCK REGISTER */

    pll = READ_VID32(SC1200_VID_MISC);
    WRITE_VID32(SC1200_VID_MISC, pll | SC1200_PLL_POWER_NORMAL);
    WRITE_VID32(SC1200_VID_CLOCK_SELECT, value);
    return;
}

/*---------------------------------------------------------------------------
 * gfx_set_screen_enable (PRIVATE ROUTINE - NOT PART OF API)
 * 
 * This routine enables or disables the graphics display logic of the video 
 * processor.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_screen_enable(int enable)
#else
int
gfx_set_screen_enable(int enable)
#endif
{
    unsigned long config;

    config = READ_VID32(SC1200_DISPLAY_CONFIG);
    if (enable)
        WRITE_VID32(SC1200_DISPLAY_CONFIG, config | SC1200_DCFG_DIS_EN);
    else
        WRITE_VID32(SC1200_DISPLAY_CONFIG, config & ~SC1200_DCFG_DIS_EN);
    return (GFX_STATUS_OK);
}

/*---------------------------------------------------------------------------
 * gfx_set_crt_enable
 * 
 * This routine enables or disables the CRT output from the video processor.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_crt_enable(int enable)
#else
int
gfx_set_crt_enable(int enable)
#endif
{
    unsigned long config, misc;

    config = READ_VID32(SC1200_DISPLAY_CONFIG);
    misc = READ_VID32(SC1200_VID_MISC);

    /*
     *  IMPORTANT: For all modes do NOT disable the graphics display logic
     *  because it might be needed for TV
     */

    switch (enable) {
    case CRT_DISABLE:          /* HSync:Off VSync:Off */
        WRITE_VID32(SC1200_DISPLAY_CONFIG, config & ~(SC1200_DCFG_HSYNC_EN
                                                      | SC1200_DCFG_VSYNC_EN |
                                                      SC1200_DCFG_DAC_BL_EN));
        WRITE_VID32(SC1200_VID_MISC, misc | SC1200_DAC_POWER_DOWN);
        break;
    case CRT_ENABLE:           /* Enable CRT display, including 
                                 * display logic */
        WRITE_VID32(SC1200_DISPLAY_CONFIG, config | SC1200_DCFG_HSYNC_EN
                    | SC1200_DCFG_VSYNC_EN | SC1200_DCFG_DAC_BL_EN);
        WRITE_VID32(SC1200_VID_MISC, misc & ~SC1200_DAC_POWER_DOWN);

        /* ENABLE GRAPHICS DISPLAY LOGIC */
        gfx_set_screen_enable(1);
        break;
    case CRT_STANDBY:          /* HSync:Off VSync:On */
        WRITE_VID32(SC1200_DISPLAY_CONFIG, (config & ~(SC1200_DCFG_HSYNC_EN
                                                       | SC1200_DCFG_DAC_BL_EN))
                    | SC1200_DCFG_VSYNC_EN);
        WRITE_VID32(SC1200_VID_MISC, misc | SC1200_DAC_POWER_DOWN);
        break;
    case CRT_SUSPEND:          /* HSync:On VSync:Off */
        WRITE_VID32(SC1200_DISPLAY_CONFIG, (config & ~(SC1200_DCFG_VSYNC_EN
                                                       | SC1200_DCFG_DAC_BL_EN))
                    | SC1200_DCFG_HSYNC_EN);
        WRITE_VID32(SC1200_VID_MISC, misc | SC1200_DAC_POWER_DOWN);
        break;
    default:
        return GFX_STATUS_BAD_PARAMETER;
    }
    return (GFX_STATUS_OK);
}

/*----------------------------------------------------------------------------
 * gfx_set_video_enable
 *
 * This routine enables or disables the video overlay functionality.
 *----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_video_enable(int enable)
#else
int
gfx_set_video_enable(int enable)
#endif
{
    unsigned long vcfg;

    /* WAIT FOR VERTICAL BLANK TO START */
    /* Otherwise a glitch can be observed. */

    if (gfx_test_timing_active()) {
        if (!gfx_test_vertical_active()) {
            while (!gfx_test_vertical_active());
        }
        while (gfx_test_vertical_active());
    }

    vcfg = READ_VID32(SC1200_VIDEO_CONFIG);
    if (enable) {
        /* ENABLE SC1200 VIDEO OVERLAY */

        vcfg |= SC1200_VCFG_VID_EN;
        WRITE_VID32(SC1200_VIDEO_CONFIG, vcfg);
    }
    else {
        /* DISABLE SC1200 VIDEO OVERLAY */

        vcfg &= ~SC1200_VCFG_VID_EN;
        WRITE_VID32(SC1200_VIDEO_CONFIG, vcfg);
    }
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_video_format
 *
 * Sets input video format type, to one of the YUV formats or to RGB.
 *----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_video_format(unsigned long format)
#else
int
gfx_set_video_format(unsigned long format)
#endif
{
    unsigned long ctrl, vcfg = 0;

    /* SET THE SC1200 VIDEO INPUT FORMAT */

    vcfg = READ_VID32(SC1200_VIDEO_CONFIG);
    ctrl = READ_VID32(SC1200_VID_ALPHA_CONTROL);
    ctrl &= ~(SC1200_VIDEO_INPUT_IS_RGB);
    vcfg &= ~(SC1200_VCFG_VID_INP_FORMAT | SC1200_VCFG_4_2_0_MODE);
    switch (format) {
    case VIDEO_FORMAT_UYVY:
        vcfg |= SC1200_VCFG_UYVY_FORMAT;
        break;
    case VIDEO_FORMAT_YUYV:
        vcfg |= SC1200_VCFG_YUYV_FORMAT;
        break;
    case VIDEO_FORMAT_Y2YU:
        vcfg |= SC1200_VCFG_Y2YU_FORMAT;
        break;
    case VIDEO_FORMAT_YVYU:
        vcfg |= SC1200_VCFG_YVYU_FORMAT;
        break;
    case VIDEO_FORMAT_Y0Y1Y2Y3:
        vcfg |= SC1200_VCFG_UYVY_FORMAT;
        vcfg |= SC1200_VCFG_4_2_0_MODE;
        break;
    case VIDEO_FORMAT_Y3Y2Y1Y0:
        vcfg |= SC1200_VCFG_Y2YU_FORMAT;
        vcfg |= SC1200_VCFG_4_2_0_MODE;
        break;
    case VIDEO_FORMAT_Y1Y0Y3Y2:
        vcfg |= SC1200_VCFG_YUYV_FORMAT;
        vcfg |= SC1200_VCFG_4_2_0_MODE;
        break;
    case VIDEO_FORMAT_Y1Y2Y3Y0:
        vcfg |= SC1200_VCFG_YVYU_FORMAT;
        vcfg |= SC1200_VCFG_4_2_0_MODE;
        break;
    case VIDEO_FORMAT_RGB:
        ctrl |= SC1200_VIDEO_INPUT_IS_RGB;
        vcfg |= SC1200_VCFG_UYVY_FORMAT;
        break;
    case VIDEO_FORMAT_P2M_P2L_P1M_P1L:
        ctrl |= SC1200_VIDEO_INPUT_IS_RGB;
        vcfg |= SC1200_VCFG_Y2YU_FORMAT;
        break;
    case VIDEO_FORMAT_P1M_P1L_P2M_P2L:
        ctrl |= SC1200_VIDEO_INPUT_IS_RGB;
        vcfg |= SC1200_VCFG_YUYV_FORMAT;
        break;
    case VIDEO_FORMAT_P1M_P2L_P2M_P1L:
        ctrl |= SC1200_VIDEO_INPUT_IS_RGB;
        vcfg |= SC1200_VCFG_YVYU_FORMAT;
        break;
    default:
        return GFX_STATUS_BAD_PARAMETER;
    }

    /* ALWAYS DISABLE GRAPHICS CSC */
    /* This is enabled in the function gfx_set_color_space_YUV for */
    /* YUV blending on TV.                                         */

    ctrl &= ~SC1200_CSC_GFX_RGB_TO_YUV;

    if (ctrl & SC1200_VIDEO_INPUT_IS_RGB)
        ctrl &= ~SC1200_CSC_VIDEO_YUV_TO_RGB;
    else
        ctrl |= SC1200_CSC_VIDEO_YUV_TO_RGB;

    WRITE_VID32(SC1200_VIDEO_CONFIG, vcfg);
    WRITE_VID32(SC1200_VID_ALPHA_CONTROL, ctrl);

    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_video_size
 *
 * This routine specifies the size of the source data.  It is used only 
 * to determine how much data to transfer per frame, and is not used to 
 * calculate the scaling value (that is handled by a separate routine).
 *----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_video_size(unsigned short width, unsigned short height)
#else
int
gfx_set_video_size(unsigned short width, unsigned short height)
#endif
{
    unsigned long size, vcfg;

    /* SET THE SC1200 VIDEO LINE SIZE */

    vcfg = READ_VID32(SC1200_VIDEO_CONFIG);
    vcfg &= ~(SC1200_VCFG_LINE_SIZE_LOWER_MASK | SC1200_VCFG_LINE_SIZE_UPPER);
    size = (width >> 1);
    vcfg |= (size & 0x00FF) << 8;
    if (size & 0x0100)
        vcfg |= SC1200_VCFG_LINE_SIZE_UPPER;
    WRITE_VID32(SC1200_VIDEO_CONFIG, vcfg);

    /* SET TOTAL VIDEO BUFFER SIZE IN DISPLAY CONTROLLER */
    /* Use private routine to abstract the display controller. */

    /* Add 1 line to bypass issue #803 */
    gfx_set_display_video_size(width, (unsigned short) (height + 2));
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_video_offset
 *
 * This routine sets the starting offset for the video buffer when only 
 * one offset needs to be specified.
 *----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_video_offset(unsigned long offset)
#else
int
gfx_set_video_offset(unsigned long offset)
#endif
{
    /* SAVE VALUE FOR FUTURE CLIPPING OF THE TOP OF THE VIDEO WINDOW */

    gfx_vid_offset = offset;

    /* SET VIDEO BUFFER OFFSET IN DISPLAY CONTROLLER */
    /* Use private routine to abstract the display controller. */

    gfx_set_display_video_offset(offset);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_upscale
 * 
 * This routine sets the scale factor for the video overlay window.  The 
 * size of the source and destination regions are specified in pixels.  
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_video_upscale(unsigned short srcw, unsigned short srch,
                         unsigned short dstw, unsigned short dsth)
#else
int
gfx_set_video_upscale(unsigned short srcw, unsigned short srch,
                      unsigned short dstw, unsigned short dsth)
#endif
{
    unsigned long xscale, yscale;

    /* SAVE PARAMETERS (unless don't-care zero destination arguments are used)
     * */
    /* These are needed for clipping the video window later. */

    if (dstw != 0) {
        gfx_vid_srcw = srcw;
        gfx_vid_dstw = dstw;
    }
    if (dsth != 0) {
        gfx_vid_srch = srch;
        gfx_vid_dsth = dsth;
    }

    /* CALCULATE SC1200 SCALE FACTORS */

    if (dstw == 0)
        xscale = READ_VID32(SC1200_VIDEO_UPSCALE) & 0xffff;
    /* keep previous if don't-care argument */
    else if (dstw <= srcw)
        xscale = 0x2000l;
    /* horizontal downscaling is currently done in a separate function */
    else if ((srcw == 1) || (dstw == 1))
        return GFX_STATUS_BAD_PARAMETER;
    else
        xscale = (0x2000l * (srcw - 1l)) / (dstw - 1l);

    if (dsth == 0)
        yscale = (READ_VID32(SC1200_VIDEO_UPSCALE) & 0xffff0000) >> 16;
    /* keep previous if don't-care argument */
    else if (dsth <= srch)
        yscale = 0x2000l;
    /* No vertical downscaling in SC1200 so force to 1x if attempted */
    else if ((srch == 1) || (dsth == 1))
        return GFX_STATUS_BAD_PARAMETER;
    else
        yscale = (0x2000l * (srch - 1l)) / (dsth - 1l);

    WRITE_VID32(SC1200_VIDEO_UPSCALE, (yscale << 16) | xscale);

    /* CALL ROUTINE TO UPDATE WINDOW POSITION */
    /* This is required because the scale values effect the number of */
    /* source data pixels that need to be clipped, as well as the */
    /* amount of data that needs to be transferred. */

    gfx_set_video_window(gfx_vid_xpos, gfx_vid_ypos, gfx_vid_width,
                         gfx_vid_height);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_scale
 * 
 * This routine sets the scale factor for the video overlay window.  The 
 * size of the source and destination regions are specified in pixels.  
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_video_scale(unsigned short srcw, unsigned short srch,
                       unsigned short dstw, unsigned short dsth)
#else
int
gfx_set_video_scale(unsigned short srcw, unsigned short srch,
                    unsigned short dstw, unsigned short dsth)
#endif
{
    return gfx_set_video_upscale(srcw, srch, dstw, dsth);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_downscale_config
 * 
 * This routine sets the downscale type and factor for the video overlay 
 * window.
 * Note: No downscaling support for RGB565 and YUV420 video formats.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_video_downscale_config(unsigned short type, unsigned short m)
#else
int
gfx_set_video_downscale_config(unsigned short type, unsigned short m)
#endif
{
    unsigned long downscale;

    if ((m < 1) || (m > 16))
        return GFX_STATUS_BAD_PARAMETER;

    downscale = READ_VID32(SC1200_VIDEO_DOWNSCALER_CONTROL);
    downscale &=
        ~(SC1200_VIDEO_DOWNSCALE_FACTOR_MASK |
          SC1200_VIDEO_DOWNSCALE_TYPE_MASK);
    downscale |= ((m - 1l) << SC1200_VIDEO_DOWNSCALE_FACTOR_POS);
    switch (type) {
    case VIDEO_DOWNSCALE_KEEP_1_OF:
        downscale |= SC1200_VIDEO_DOWNSCALE_TYPE_A;
        break;
    case VIDEO_DOWNSCALE_DROP_1_OF:
        downscale |= SC1200_VIDEO_DOWNSCALE_TYPE_B;
        break;
    default:
        return GFX_STATUS_BAD_PARAMETER;
    }
    WRITE_VID32(SC1200_VIDEO_DOWNSCALER_CONTROL, downscale);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_downscale_coefficients
 * 
 * This routine sets the downscale filter coefficients.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_video_downscale_coefficients(unsigned short coef1,
                                        unsigned short coef2,
                                        unsigned short coef3,
                                        unsigned short coef4)
#else
int
gfx_set_video_downscale_coefficients(unsigned short coef1,
                                     unsigned short coef2, unsigned short coef3,
                                     unsigned short coef4)
#endif
{
    if ((coef1 + coef2 + coef3 + coef4) != 16)
        return GFX_STATUS_BAD_PARAMETER;

    WRITE_VID32(SC1200_VIDEO_DOWNSCALER_COEFFICIENTS,
                ((unsigned long) coef1 << SC1200_VIDEO_DOWNSCALER_COEF1_POS) |
                ((unsigned long) coef2 << SC1200_VIDEO_DOWNSCALER_COEF2_POS) |
                ((unsigned long) coef3 << SC1200_VIDEO_DOWNSCALER_COEF3_POS) |
                ((unsigned long) coef4 << SC1200_VIDEO_DOWNSCALER_COEF4_POS));
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_downscale_enable
 * 
 * This routine enables or disables downscaling for the video overlay window.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_video_downscale_enable(int enable)
#else
int
gfx_set_video_downscale_enable(int enable)
#endif
{
    unsigned long downscale;

    downscale = READ_VID32(SC1200_VIDEO_DOWNSCALER_CONTROL);
    downscale &= ~SC1200_VIDEO_DOWNSCALE_ENABLE;
    if (enable)
        downscale |= SC1200_VIDEO_DOWNSCALE_ENABLE;
    WRITE_VID32(SC1200_VIDEO_DOWNSCALER_CONTROL, downscale);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_window
 * 
 * This routine sets the position and size of the video overlay window.  The 
 * y position is specified in screen relative coordinates, and may be negative.  
 * The size of destination region is specified in pixels.  The line size
 * indicates the number of bytes of source data per scanline.
 * For the effect of negative x values, call the function
 * gfx_set_video_left_crop(). 
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_video_window(short x, short y, unsigned short w, unsigned short h)
#else
int
gfx_set_video_window(short x, short y, unsigned short w, unsigned short h)
#endif
{
    unsigned long control;
    unsigned long hadjust, vadjust;
    unsigned long xstart, ystart, xend, yend;

    /* For left cropping call the function gfx_set_video_left_crop() */

    if (x < 0)
        return GFX_STATUS_BAD_PARAMETER;

    /* SAVE PARAMETERS */
    /* These are needed to call this routine if the scale value changes. */
    /* In the case of SC1200 they are also needed for restoring when video
     * is re-enabled */

    gfx_vid_xpos = x;
    gfx_vid_ypos = y;
    gfx_vid_width = w;
    gfx_vid_height = h;

    /* GET ADJUSTMENT VALUES */
    /* Use routines to abstract version of display controller. */

    hadjust = gfx_get_htotal() - gfx_get_hsync_end() - 14l;
    vadjust = gfx_get_vtotal() - gfx_get_vsync_end() + 1l;

    /* HORIZONTAL START */

    xstart = (unsigned long) x + hadjust;

    /* HORIZONTAL END */
    /* End positions in register are non-inclusive (one more than the actual 
     * end) */

    if ((x + w) < gfx_get_hactive())
        xend = (unsigned long) x + (unsigned long) w + hadjust;
    else                        /* right clipping needed */
        xend = (unsigned long) gfx_get_hactive() + hadjust;

    /* VERTICAL START */

    ystart = (unsigned long) y + vadjust;

    /* VERTICAL END */

    if ((y + h) < gfx_get_vactive())
        yend = (unsigned long) y + (unsigned long) h + vadjust;
    else                        /* bottom clipping needed */
        yend = (unsigned long) gfx_get_vactive() + vadjust;

    /* SET VIDEO LINE INVERT BIT */

    control = READ_VID32(SC1200_VID_ALPHA_CONTROL);
    if (y & 0x1)
        WRITE_VID32(SC1200_VID_ALPHA_CONTROL,
                    control | SC1200_VIDEO_LINE_OFFSET_ODD);
    else
        WRITE_VID32(SC1200_VID_ALPHA_CONTROL,
                    control & ~SC1200_VIDEO_LINE_OFFSET_ODD);

    /* SET VIDEO POSITION */

    WRITE_VID32(SC1200_VIDEO_X_POS, (xend << 16) | xstart);
    WRITE_VID32(SC1200_VIDEO_Y_POS, (yend << 16) | ystart);

    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_left_crop
 * 
 * This routine sets the number of pixels which will be cropped from the
 * beginning of each video line. The video window will begin to display only
 * from the pixel following the cropped pixels, and the cropped pixels
 * will be ignored.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_video_left_crop(unsigned short x)
#else
int
gfx_set_video_left_crop(unsigned short x)
#endif
{
    unsigned long vcfg, initread;

    /* CLIPPING ON LEFT */
    /* Adjust initial read for scale, checking for divide by zero */

    if (gfx_vid_dstw)
        initread = (unsigned long) x *gfx_vid_srcw / gfx_vid_dstw;

    else
        initread = 0l;

    /* SET INITIAL READ ADDRESS */

    vcfg = READ_VID32(SC1200_VIDEO_CONFIG);
    vcfg &= ~SC1200_VCFG_INIT_READ_MASK;
    vcfg |= (initread << 15) & SC1200_VCFG_INIT_READ_MASK;
    WRITE_VID32(SC1200_VIDEO_CONFIG, vcfg);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_color_key
 * 
 * This routine specifies the color key value and mask for the video overlay
 * hardware. To disable color key, the color and mask should both be set to 
 * zero. The hardware uses the color key in the following equation:
 *
 * ((source data) & (color key mask)) == ((color key) & (color key mask))
 *
 * If "graphics" is set to TRUE, the source data is graphics, and color key
 * is an RGB value. If "graphics" is set to FALSE, the source data is the 
 * video, and color key is a YUV value.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_video_color_key(unsigned long key, unsigned long mask, int graphics)
#else
int
gfx_set_video_color_key(unsigned long key, unsigned long mask, int graphics)
#endif
{
    unsigned long dcfg = 0;

    /* SET SC1200 COLOR KEY VALUE */

    WRITE_VID32(SC1200_VIDEO_COLOR_KEY, key);
    WRITE_VID32(SC1200_VIDEO_COLOR_MASK, mask);

    /* SELECT GRAPHICS OR VIDEO DATA TO COMPARE TO THE COLOR KEY */

    dcfg = READ_VID32(SC1200_DISPLAY_CONFIG);
    if (graphics & 0x01)
        dcfg &= ~SC1200_DCFG_VG_CK;
    else
        dcfg |= SC1200_DCFG_VG_CK;
    WRITE_VID32(SC1200_DISPLAY_CONFIG, dcfg);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_filter
 * 
 * This routine enables or disables the video overlay filters.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_video_filter(int xfilter, int yfilter)
#else
int
gfx_set_video_filter(int xfilter, int yfilter)
#endif
{
    unsigned long vcfg = 0;

    /* ENABLE OR DISABLE SC1200 VIDEO OVERLAY FILTERS */

    vcfg = READ_VID32(SC1200_VIDEO_CONFIG);
    vcfg &= ~(SC1200_VCFG_X_FILTER_EN | SC1200_VCFG_Y_FILTER_EN);
    if (xfilter)
        vcfg |= SC1200_VCFG_X_FILTER_EN;
    if (yfilter)
        vcfg |= SC1200_VCFG_Y_FILTER_EN;
    WRITE_VID32(SC1200_VIDEO_CONFIG, vcfg);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_palette
 * 
 * This routine loads the video hardware palette.  If a NULL pointer is 
 * specified, the palette is bypassed (for SC1200, this means loading the 
 * palette with identity values). 
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_video_palette(unsigned long *palette)
#else
int
gfx_set_video_palette(unsigned long *palette)
#endif
{
    unsigned long i, entry;
    unsigned long dcfg, misc;

    /* WAIT FOR VERTICAL BLANK TO END */
    /* Otherwise palette will not be written properly. */

    if (gfx_test_timing_active()) {
        if (gfx_test_vertical_active()) {
            while (gfx_test_vertical_active());
        }
        while (!gfx_test_vertical_active());
    }

    /* LOAD SC1200 VIDEO PALETTE */

    WRITE_VID32(SC1200_PALETTE_ADDRESS, 0);
    for (i = 0; i < 256; i++) {
        if (palette)
            entry = palette[i];
        else
            entry = gfx_gamma_ram_1200[i] << 8;
        WRITE_VID32(SC1200_PALETTE_DATA, entry);
    }

    /* ENABLE THE VIDEO PALETTE */
    /* Ensure that the video palette has an effect by routing video data */
    /* through the palette RAM and clearing the 'Bypass Both' bit.       */

    dcfg = READ_VID32(SC1200_DISPLAY_CONFIG);
    misc = READ_VID32(SC1200_VID_MISC);

    dcfg |= SC1200_DCFG_GV_PAL_BYP;
    misc &= ~SC1200_GAMMA_BYPASS_BOTH;

    WRITE_VID32(SC1200_DISPLAY_CONFIG, dcfg);
    WRITE_VID32(SC1200_VID_MISC, misc);

    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_palette_entry
 * 
 * This routine loads a single entry of the video hardware palette.  
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_video_palette_entry(unsigned long index, unsigned long palette)
#else
int
gfx_set_video_palette_entry(unsigned long index, unsigned long palette)
#endif
{
    unsigned long dcfg, misc;

    if (index > 0xFF)
        return GFX_STATUS_BAD_PARAMETER;

    /* WAIT FOR VERTICAL BLANK TO END */
    /* Otherwise palette will not be written properly. */

    if (gfx_test_timing_active()) {
        if (gfx_test_vertical_active()) {
            while (gfx_test_vertical_active());
        }
        while (!gfx_test_vertical_active());
    }

    /* SET A SINGLE ENTRY */

    WRITE_VID32(SC1200_PALETTE_ADDRESS, index);
    WRITE_VID32(SC1200_PALETTE_DATA, palette);

    /* ENABLE THE VIDEO PALETTE */
    /* Ensure that the video palette has an effect by routing video data */
    /* through the palette RAM and clearing the 'Bypass Both' bit.       */

    dcfg = READ_VID32(SC1200_DISPLAY_CONFIG);
    misc = READ_VID32(SC1200_VID_MISC);

    dcfg |= SC1200_DCFG_GV_PAL_BYP;
    misc &= ~SC1200_GAMMA_BYPASS_BOTH;

    WRITE_VID32(SC1200_DISPLAY_CONFIG, dcfg);
    WRITE_VID32(SC1200_VID_MISC, misc);

    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_palette_bypass
 * 
 * This routine enables/disables the palette RAM bypass.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_video_palette_bypass(int enable)
#else
int
gfx_set_video_palette_bypass(int enable)
#endif
{
    unsigned long misc;

    misc = READ_VID32(SC1200_VID_MISC);

    if (enable)
        misc |= SC1200_GAMMA_BYPASS_BOTH;
    else
        misc &= ~SC1200_GAMMA_BYPASS_BOTH;

    WRITE_VID32(SC1200_VID_MISC, misc);

    return 0;
}

/*---------------------------------------------------------------------------
 * gfx_set_video_request()
 * 
 * This routine sets the horizontal (pixel) and vertical (line) video request
 * values.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_video_request(short x, short y)
#else
int
gfx_set_video_request(short x, short y)
#endif
{
    /* SET SC1200 VIDEO REQUEST */

    x += gfx_get_htotal() - gfx_get_hsync_end() - 2;
    y += gfx_get_vtotal() - gfx_get_vsync_end() + 1;

    if ((x < 0) || (x > SC1200_VIDEO_REQUEST_MASK) ||
        (y < 0) || (y > SC1200_VIDEO_REQUEST_MASK))
        return GFX_STATUS_BAD_PARAMETER;

    WRITE_VID32(SC1200_VIDEO_REQUEST,
                ((unsigned long) x << SC1200_VIDEO_X_REQUEST_POS) |
                ((unsigned long) y << SC1200_VIDEO_Y_REQUEST_POS));
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_source()
 * 
 * This routine sets the video source to either memory or Direct VIP.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_video_source(VideoSourceType source)
#else
int
gfx_set_video_source(VideoSourceType source)
#endif
{
    unsigned long display_mode;

    display_mode = READ_VID32(SC1200_VIDEO_DISPLAY_MODE);

    /* SET SC1200 VIDEO SOURCE */
    switch (source) {
    case VIDEO_SOURCE_MEMORY:
        WRITE_VID32(SC1200_VIDEO_DISPLAY_MODE,
                    (display_mode & ~SC1200_VIDEO_SOURCE_MASK) |
                    SC1200_VIDEO_SOURCE_GX1);
        break;
    case VIDEO_SOURCE_DVIP:
        WRITE_VID32(SC1200_VIDEO_DISPLAY_MODE,
                    (display_mode & ~SC1200_VIDEO_SOURCE_MASK) |
                    SC1200_VIDEO_SOURCE_DVIP);
        break;
    default:
        return GFX_STATUS_BAD_PARAMETER;
    }
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_vbi_source()
 * 
 * This routine sets the vbi source to either memory or Direct VIP.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_vbi_source(VbiSourceType source)
#else
int
gfx_set_vbi_source(VbiSourceType source)
#endif
{
    unsigned long display_mode;

    display_mode = READ_VID32(SC1200_VIDEO_DISPLAY_MODE);

    /* SET SC1200 VBI SOURCE */
    switch (source) {
    case VBI_SOURCE_MEMORY:
        WRITE_VID32(SC1200_VIDEO_DISPLAY_MODE,
                    (display_mode & ~SC1200_VBI_SOURCE_MASK) |
                    SC1200_VBI_SOURCE_GX1);
        break;
    case VBI_SOURCE_DVIP:
        WRITE_VID32(SC1200_VIDEO_DISPLAY_MODE,
                    (display_mode & ~SC1200_VBI_SOURCE_MASK) |
                    SC1200_VBI_SOURCE_DVIP);
        break;
    default:
        return GFX_STATUS_BAD_PARAMETER;
    }
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_vbi_lines()
 * 
 * This routine sets the VBI lines to pass to the TV encoder.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_vbi_lines(unsigned long even, unsigned long odd)
#else
int
gfx_set_vbi_lines(unsigned long even, unsigned long odd)
#endif
{
    /* SET SC1200 VBI LINES */
    WRITE_VID32(SC1200_VIDEO_EVEN_VBI_LINE_ENABLE,
                even & SC1200_VIDEO_VBI_LINE_ENABLE_MASK);
    WRITE_VID32(SC1200_VIDEO_ODD_VBI_LINE_ENABLE,
                odd & SC1200_VIDEO_VBI_LINE_ENABLE_MASK);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_vbi_total()
 * 
 * This routine sets the total number of VBI bytes for each field.
 * The total is needed when both VBI and active video are received from memory
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_vbi_total(unsigned long even, unsigned long odd)
#else
int
gfx_set_vbi_total(unsigned long even, unsigned long odd)
#endif
{
    /* SET SC1200 VBI TOTAL */
    WRITE_VID32(SC1200_VIDEO_EVEN_VBI_TOTAL_COUNT,
                even & SC1200_VIDEO_VBI_TOTAL_COUNT_MASK);
    WRITE_VID32(SC1200_VIDEO_ODD_VBI_TOTAL_COUNT,
                odd & SC1200_VIDEO_VBI_TOTAL_COUNT_MASK);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_interlaced()
 * 
 * This routine configures the video processor video overlay mode to be
 * interlaced YUV.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_video_interlaced(int enable)
#else
int
gfx_set_video_interlaced(int enable)
#endif
{
    unsigned long control;

    control = READ_VID32(SC1200_VID_ALPHA_CONTROL);
    /* SET INTERLACED VIDEO */
    if (enable)
        WRITE_VID32(SC1200_VID_ALPHA_CONTROL,
                    control | SC1200_VIDEO_IS_INTERLACED);
    else
        WRITE_VID32(SC1200_VID_ALPHA_CONTROL,
                    control & ~SC1200_VIDEO_IS_INTERLACED);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_color_space_YUV()
 * 
 * This routine configures the video processor to process graphics and video
 * in either YUV or RGB color space. The mode should be set to tune image
 * quality.
 * Setting "enable" to TRUE improves image quality on TV,
 * but in this mode colors on CRT will not be correct.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_color_space_YUV(int enable)
#else
int
gfx_set_color_space_YUV(int enable)
#endif
{
    unsigned long control;

    control = READ_VID32(SC1200_VID_ALPHA_CONTROL);

    /* SET SC1200 VIDEO COLOR SPACE TO YUV OR RGB */

    if (enable) {
        /* ENABLE YUV BLENDING */
        /* YUV blending cannot be enabled in RGB video formats */

        control |= SC1200_CSC_GFX_RGB_TO_YUV;   /* Convert graphics to YUV */
        control &= ~SC1200_CSC_VIDEO_YUV_TO_RGB;        /* Leave video in YUV      */

        if (control & SC1200_VIDEO_INPUT_IS_RGB)
            return (GFX_STATUS_UNSUPPORTED);    /* Can't convert video from 
                                                 * RGB to YUV */
    }
    else {
        /* RGB BLENDING */

        control &= ~SC1200_CSC_GFX_RGB_TO_YUV;  /* Leave graphics in RGB */
        if (control & SC1200_VIDEO_INPUT_IS_RGB)
            control &= ~SC1200_CSC_VIDEO_YUV_TO_RGB;    /* Leave video in RGB */
        else
            control |= SC1200_CSC_VIDEO_YUV_TO_RGB;     /* Convert video to RGB */
    }
    WRITE_VID32(SC1200_VID_ALPHA_CONTROL, control);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_vertical_scaler_offset()
 * 
 * This routine sets the value by which the odd frame is shifted with respect
 * to the even frame. This is useful for de-interlacing in Bob method, by
 * setting the shift value to be one line.
 * If offset is 0, no shifting occurs.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_vertical_scaler_offset(char offset)
#else
int
gfx_set_vertical_scaler_offset(char offset)
#endif
{
    unsigned long control;

    control = READ_VID32(SC1200_VID_ALPHA_CONTROL);
    if (offset == 1) {
        control &= ~SC1200_VERTICAL_SCALER_SHIFT_MASK;
        /* Clear shifting value                                 */
        control |= SC1200_VERTICAL_SCALER_SHIFT_INIT;
        /* Set shifting value                                   */
        control |= SC1200_VERTICAL_SCALER_SHIFT_EN;
        /* Enable odd frame shifting                   */
    }
    else if (offset == 0) {
        control &= ~SC1200_VERTICAL_SCALER_SHIFT_EN;
        /* No shifting occurs                                   */
        control &= ~SC1200_VERTICAL_SCALER_SHIFT_MASK;
        /* Clear shifting value                                 */
    }
    else
        return (GFX_STATUS_BAD_PARAMETER);
    /* TODO: how to program other values ?  */
    WRITE_VID32(SC1200_VID_ALPHA_CONTROL, control);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_top_line_in_odd()
 * 
 * This routine sets the field in which the top line of input video resides.
 * If enable is "0", this is the even field (default). [not to be confused
 * with the odd field being the top field on TV].
 * If enable is "1", this is the odd field.
 * Use enable "1" for input devices whose field indication is reversed from
 * normal, i.e. an indication of "odd" field is given for even field data,
 * and vice versa.
 * This setting affects the video processor only when it is in either 
 * interlaced or Bob (scaler offset active) modes.
  *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_top_line_in_odd(int enable)
#else
int
gfx_set_top_line_in_odd(int enable)
#endif
{
    unsigned long control;

    control = READ_VID32(SC1200_VID_ALPHA_CONTROL);
    if (enable)
        control |= SC1200_TOP_LINE_IN_ODD;      /* Set shifting value */
    else
        control &= ~SC1200_TOP_LINE_IN_ODD;     /* No shifting occurs */
    WRITE_VID32(SC1200_VID_ALPHA_CONTROL, control);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_genlock_delay()
 * 
 * This routine sets the delay between VIP VSYNC and display controller VSYNC.
 * The delay is in 27 MHz clocks.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_genlock_delay(unsigned long delay)
#else
int
gfx_set_genlock_delay(unsigned long delay)
#endif
{
    /* SET SC1200 GENLOCK DELAY */
    WRITE_VID32(SC1200_GENLOCK_DELAY, delay & SC1200_GENLOCK_DELAY_MASK);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_genlock_enable()
 * 
 * This routine sets and configures the genlock according to the flags 
 * parameter.
 * Flags value of 0 disables genlock and resets its configuration.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_genlock_enable(int flags)
#else
int
gfx_set_genlock_enable(int flags)
#endif
{
    unsigned long genlock = 0;

    if (flags) {
        /* SET SC1200 GENLOCK CONFIGURATION */
        if (flags & GENLOCK_SINGLE)
            genlock |= SC1200_GENLOCK_SINGLE_ENABLE;
        if (flags & GENLOCK_FIELD_SYNC)
            genlock |= SC1200_GENLOCK_FIELD_SYNC_ENABLE;
        if (flags & GENLOCK_CONTINUOUS)
            genlock |= SC1200_GENLOCK_CONTINUOUS_ENABLE;
        if (flags & GENLOCK_SYNCED_EDGE_FALLING)
            genlock |= SC1200_GENLOCK_GX_VSYNC_FALLING_EDGE;
        if (flags & GENLOCK_SYNCING_EDGE_FALLING)
            genlock |= SC1200_GENLOCK_VIP_VSYNC_FALLING_EDGE;
        if (flags & GENLOCK_TIMEOUT)
            genlock |= SC1200_GENLOCK_TIMEOUT_ENABLE;
        if (flags & GENLOCK_TVENC_RESET_EVEN_FIELD)
            genlock |= SC1200_GENLOCK_TVENC_RESET_EVEN_FIELD;
        if (flags & GENLOCK_TVENC_RESET_BEFORE_DELAY)
            genlock |= SC1200_GENLOCK_TVENC_RESET_BEFORE_DELAY;
        if (flags & GENLOCK_TVENC_RESET)
            genlock |= SC1200_GENLOCK_TVENC_RESET_ENABLE;
        if (flags & GENLOCK_SYNC_TO_TVENC)
            genlock |= SC1200_GENLOCK_SYNC_TO_TVENC;
    }
    WRITE_VID32(SC1200_GENLOCK, genlock);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_video_cursor()
 * 
 * This routine configures the video hardware cursor.
 * If the "mask"ed bits in the graphics pixel match "key", then either 
 * "color1" or "color2" will be used for this pixel, according to the value of
 * bit number "select_color2" of the graphics pixel.
 *
 * key - 24 bit RGB value
 * mask - 24 bit mask
 * color1, color2 - RGB or YUV, depending on the current color space 
 * conversion select_color2 - value between 0 to 23
 *
 * To disable match, a "mask" and "key" value of 0xffffff should be set,
 * because the graphics pixels incoming to the video processor have maximum 16
 * bits set (0xF8FCF8).
 *
 * This feature is useful for disabling alpha blending of the cursor.
 * Otherwise cursor image would be blurred (or completely invisible if video
 * alpha is maximum value).
 * Note: the cursor pixel replacements take place both inside and outside the
 * video overlay window.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_video_cursor(unsigned long key, unsigned long mask,
                        unsigned short select_color2, unsigned long color1,
                        unsigned long color2)
#else
int
gfx_set_video_cursor(unsigned long key, unsigned long mask,
                     unsigned short select_color2, unsigned long color1,
                     unsigned long color2)
#endif
{
    if (select_color2 > SC1200_CURSOR_COLOR_BITS)
        return GFX_STATUS_BAD_PARAMETER;
    key =
        (key & SC1200_COLOR_MASK) | ((unsigned long) select_color2 <<
                                     SC1200_CURSOR_COLOR_KEY_OFFSET_POS);
    WRITE_VID32(SC1200_CURSOR_COLOR_KEY, key);
    WRITE_VID32(SC1200_CURSOR_COLOR_MASK, mask);
    WRITE_VID32(SC1200_CURSOR_COLOR_1, color1);
    WRITE_VID32(SC1200_CURSOR_COLOR_2, color2);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_enable
 * 
 * This routine enables or disables the currently selected alpha region.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_alpha_enable(int enable)
#else
int
gfx_set_alpha_enable(int enable)
#endif
{
    unsigned long address = 0, value = 0;

    if (gfx_alpha_select > 2)
        return (GFX_STATUS_UNSUPPORTED);
    address = SC1200_ALPHA_CONTROL_1 + ((unsigned long) gfx_alpha_select << 4);
    value = READ_VID32(address);
    if (enable)
        value |= (SC1200_ACTRL_WIN_ENABLE | SC1200_ACTRL_LOAD_ALPHA);
    else
        value &= ~(SC1200_ACTRL_WIN_ENABLE);
    WRITE_VID32(address, value);
    return (GFX_STATUS_OK);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_window
 * 
 * This routine sets the size of the currently selected alpha region.
 * Note: "x" and "y" are signed to enable using negative values needed for
 * implementing workarounds of hardware issues.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_alpha_window(short x, short y,
                        unsigned short width, unsigned short height)
#else
int
gfx_set_alpha_window(short x, short y,
                     unsigned short width, unsigned short height)
#endif
{
    unsigned long address = 0;

    /* CHECK FOR CLIPPING */

    if ((x + width) > gfx_get_hactive())
        width = gfx_get_hactive() - x;
    if ((y + height) > gfx_get_vactive())
        height = gfx_get_vactive() - y;

    /* ADJUST POSITIONS */

    x += gfx_get_htotal() - gfx_get_hsync_end() - 2;
    y += gfx_get_vtotal() - gfx_get_vsync_end() + 1;

    if (gfx_alpha_select > 2)
        return (GFX_STATUS_UNSUPPORTED);
    address = SC1200_ALPHA_XPOS_1 + ((unsigned long) gfx_alpha_select << 4);

    /* End positions in register are non-inclusive (one more than the actual 
     * end) */

    WRITE_VID32(address, (unsigned long) x |
                ((unsigned long) (x + width) << 16));
    WRITE_VID32(address + 4l, (unsigned long) y |
                ((unsigned long) (y + height) << 16));
    return (GFX_STATUS_OK);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_value
 * 
 * This routine sets the alpha value for the currently selected alpha
 * region.  It also specifies an increment/decrement value for fading.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_alpha_value(unsigned char alpha, char delta)
#else
int
gfx_set_alpha_value(unsigned char alpha, char delta)
#endif
{
    unsigned long address = 0, value = 0;
    unsigned char new_value = 0;
    int loop = 1;

    if (gfx_alpha_select > 2)
        return (GFX_STATUS_UNSUPPORTED);
    address = SC1200_ALPHA_CONTROL_1 + ((unsigned long) gfx_alpha_select << 4);
    value = READ_VID32(address);
    value &= SC1200_ACTRL_WIN_ENABLE;   /* keep only enable bit */
    value |= (unsigned long) alpha;
    value |= (((unsigned long) delta) & 0xff) << 8;
    value |= SC1200_ACTRL_LOAD_ALPHA;
    WRITE_VID32(address, value);

    /* WORKAROUND FOR ISSUE #1187 */
    /* Need to verify that the alpha operation succeeded */

    while (1) {
        /* WAIT FOR VERTICAL BLANK TO END */
        if (gfx_test_timing_active()) {
            if (gfx_test_vertical_active())
                while (gfx_test_vertical_active());
            while (!gfx_test_vertical_active());
        }
        new_value =
            (unsigned char) ((READ_VID32(SC1200_ALPHA_WATCH) >>
                              (gfx_alpha_select << 3)) & 0xff);
        if (new_value == alpha)
            return GFX_STATUS_OK;
        if (++loop > 10)
            return GFX_STATUS_ERROR;
        WRITE_VID32(address, value);
    }
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_priority
 * 
 * This routine sets the priority of the currently selected alpha region.
 * A higher value indicates a higher priority.
 * Note: Priority of enabled alpha windows must be different.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_alpha_priority(int priority)
#else
int
gfx_set_alpha_priority(int priority)
#endif
{
    unsigned long pos = 0, value = 0;

    if (priority > 3)
        return (GFX_STATUS_BAD_PARAMETER);
    if (gfx_alpha_select > 2)
        return (GFX_STATUS_UNSUPPORTED);
    value = READ_VID32(SC1200_VID_ALPHA_CONTROL);
    pos = 16 + (gfx_alpha_select << 1);
    value &= ~(0x03l << pos);
    value |= (unsigned long) priority << pos;
    WRITE_VID32(SC1200_VID_ALPHA_CONTROL, value);
    return (GFX_STATUS_OK);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_color
 * 
 * This routine sets the color to be displayed inside the currently selected
 * alpha window when there is a color key match (when the alpha color
 * mechanism is enabled).
 * "color" is a 24 bit RGB value (for RGB blending) or YUV value (for YUV 
 * blending).
 * In Interlaced YUV blending mode, Y/2 value should be used.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_alpha_color(unsigned long color)
#else
int
gfx_set_alpha_color(unsigned long color)
#endif
{
    unsigned long address = 0;

    if (gfx_alpha_select > 2)
        return (GFX_STATUS_UNSUPPORTED);
    address = SC1200_ALPHA_COLOR_1 + ((unsigned long) gfx_alpha_select << 4);

    /* ONLY 24 VALID BITS */
    color &= 0xffffffl;

    /* KEEP UPPER BYTE UNCHANGED */
    WRITE_VID32(address, (color | (READ_VID32(address) & ~0xffffffl)));
    return (GFX_STATUS_OK);
}

/*---------------------------------------------------------------------------
 * gfx_set_alpha_color_enable
 * 
 * Enable or disable the color mechanism in the alpha window.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_alpha_color_enable(int enable)
#else
int
gfx_set_alpha_color_enable(int enable)
#endif
{
    unsigned long color;
    unsigned long address = 0;

    if (gfx_alpha_select > 2)
        return (GFX_STATUS_UNSUPPORTED);
    address = SC1200_ALPHA_COLOR_1 + ((unsigned long) gfx_alpha_select << 4);
    color = READ_VID32(address);
    if (enable)
        color |= SC1200_ALPHA_COLOR_ENABLE;
    else
        color &= ~SC1200_ALPHA_COLOR_ENABLE;
    WRITE_VID32(address, color);
    return (GFX_STATUS_OK);
}

/*---------------------------------------------------------------------------
 * gfx_set_no_ck_outside_alpha
 * 
 * This function affects where inside the video window color key or chroma
 * key comparison is done:
 * If enable is TRUE, color/chroma key comparison is performed only inside
 * the enabled alpha windows. Outside the (enabled) alpha windows, only video
 * is displayed if color key is used, and only graphics is displayed if chroma
 * key is used.
 * If enable is FALSE, color/chroma key comparison is performed in all the
 * video window area.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_no_ck_outside_alpha(int enable)
#else
int
gfx_set_no_ck_outside_alpha(int enable)
#endif
{
    unsigned long value;

    value = READ_VID32(SC1200_VID_ALPHA_CONTROL);
    if (enable)
        WRITE_VID32(SC1200_VID_ALPHA_CONTROL,
                    value | SC1200_NO_CK_OUTSIDE_ALPHA);
    else
        WRITE_VID32(SC1200_VID_ALPHA_CONTROL,
                    value & ~SC1200_NO_CK_OUTSIDE_ALPHA);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_macrovision_enable
 * 
 * This routine enables or disables macrovision on the tv encoder output.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_set_macrovision_enable(int enable)
#else
int
gfx_set_macrovision_enable(int enable)
#endif
{
    if (enable)
        WRITE_VID32(SC1200_TVENC_MV_CONTROL, SC1200_TVENC_MV_ENABLE);
    else
        WRITE_VID32(SC1200_TVENC_MV_CONTROL, 0);
    return (GFX_STATUS_OK);
}

#define SC1200_VIDEO_PCI_44 0x80009444

/*---------------------------------------------------------------------------
 * gfx_disable_softvga
 *
 * Disables SoftVga. This function is only valid with VSA2, Returns 1 if
 * SoftVga can be disabled; 0 if not.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_disable_softvga(void)
#else
int
gfx_disable_softvga(void)
#endif
{
    unsigned long reg_val;

    /* get the current value */
    reg_val = gfx_pci_config_read(SC1200_VIDEO_PCI_44);
    /* setting video PCI register 44 bit 0 to 1 disables SoftVga */
    reg_val |= 0x1;
    gfx_pci_config_write(SC1200_VIDEO_PCI_44, reg_val);

    /* see if we set the bit and return the appropriate value */
    reg_val = gfx_pci_config_read(SC1200_VIDEO_PCI_44);
    if ((reg_val & 0x1) == 0x1)
        return (1);
    else
        return (0);
}

/*---------------------------------------------------------------------------
 * gfx_enable_softvga
 *
 * Enables SoftVga. This function is only valid with VSA2, Returns 1 if
 * SoftVga can be enbled; 0 if not.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_enable_softvga(void)
#else
int
gfx_enable_softvga(void)
#endif
{
    unsigned long reg_val;

    /* get the current value */
    reg_val = gfx_pci_config_read(SC1200_VIDEO_PCI_44);
    /* clearing video PCI register 44 bit 0 enables SoftVga */
    gfx_pci_config_write(SC1200_VIDEO_PCI_44, reg_val & 0xfffffffel);

    /* see if we cleared the bit and return the appropriate value */
    reg_val = gfx_pci_config_read(SC1200_VIDEO_PCI_44);
    if ((reg_val & 0x1) == 0)
        return (1);
    else
        return (0);
}

/*---------------------------------------------------------------------------
 * gfx_get_clock_frequency
 *
 * This routine returns the current clock frequency in 16.16 format.
 * It reads the current register value and finds the match in the table.
 * If no match is found, this routine returns 0.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
sc1200_get_clock_frequency(void)
#else
unsigned long
gfx_get_clock_frequency(void)
#endif
{
    unsigned int index;
    unsigned long value, mask;

    mask = 0x007FFF0F;
    value = READ_VID32(SC1200_VID_CLOCK_SELECT) & mask;
    for (index = 0; index < NUM_SC1200_FREQUENCIES; index++) {
        if ((gfx_sc1200_clock_table[index].clock_select & mask) == value)
            return (gfx_sc1200_clock_table[index].frequency);
    }
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_get_max_video_width
 *
 * This routine returns the maximum theoretical video width for the current
 * display mode.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
sc1200_get_max_video_width(void)
#else
unsigned long
gfx_get_max_video_width(void)
#endif
{
    /* RETURN MPEG2 MAXIMUM */

    return 720;
}

/*************************************************************/
/*  READ ROUTINES  |  INCLUDED FOR DIAGNOSTIC PURPOSES ONLY  */
/*************************************************************/

#if GFX_READ_ROUTINES

/*---------------------------------------------------------------------------
 * gfx_get_vsa2_softvga_enable
 * 
 * This function returns the enable status of SoftVGA.  It is valid
 * only if VSAII is present.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_get_vsa2_softvga_enable(void)
#else
int
gfx_get_vsa2_softvga_enable(void)
#endif
{
    unsigned long reg_val;

    reg_val = gfx_pci_config_read(SC1200_VIDEO_PCI_44);
    if ((reg_val & 0x1) == 0)
        return (1);
    else
        return (0);

}

/*---------------------------------------------------------------------------
 * gfx_get_sync_polarities
 *
 * This routine returns the polarities of the sync pulses:
 *     Bit 0: Set if negative horizontal polarity.
 *     Bit 1: Set if negative vertical polarity.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_get_sync_polarities(void)
#else
int
gfx_get_sync_polarities(void)
#endif
{
    int polarities = 0;

    if (READ_VID32(SC1200_DISPLAY_CONFIG) & SC1200_DCFG_CRT_HSYNC_POL)
        polarities |= 1;
    if (READ_VID32(SC1200_DISPLAY_CONFIG) & SC1200_DCFG_CRT_VSYNC_POL)
        polarities |= 2;
    return (polarities);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_palette_entry
 *
 * This routine returns a single palette entry.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_get_video_palette_entry(unsigned long index, unsigned long *palette)
#else
int
gfx_get_video_palette_entry(unsigned long index, unsigned long *palette)
#endif
{
    if (index > 0xFF)
        return GFX_STATUS_BAD_PARAMETER;

    /* READ A SINGLE ENTRY */

    WRITE_VID32(SC1200_PALETTE_ADDRESS, index);
    *palette = READ_VID32(SC1200_PALETTE_DATA);

    return (GFX_STATUS_OK);
}

/*-----------------------------------------------------------------------------
 * gfx_get_video_enable
 *
 * This routine returns the value "one" if video overlay is currently enabled,
 * otherwise it returns the value "zero".
 *-----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_get_video_enable(void)
#else
int
gfx_get_video_enable(void)
#endif
{
    if (READ_VID32(SC1200_VIDEO_CONFIG) & SC1200_VCFG_VID_EN)
        return (1);
    return (0);
}

/*-----------------------------------------------------------------------------
 * gfx_get_video_format
 *
 * This routine returns the current video overlay format.
 *-----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_get_video_format(void)
#else
int
gfx_get_video_format(void)
#endif
{
    unsigned long ctrl, vcfg;

    ctrl = READ_VID32(SC1200_VID_ALPHA_CONTROL);
    vcfg = READ_VID32(SC1200_VIDEO_CONFIG);

    if (ctrl & SC1200_VIDEO_INPUT_IS_RGB) {
        switch (vcfg & SC1200_VCFG_VID_INP_FORMAT) {
        case SC1200_VCFG_UYVY_FORMAT:
            return VIDEO_FORMAT_RGB;
        case SC1200_VCFG_Y2YU_FORMAT:
            return VIDEO_FORMAT_P2M_P2L_P1M_P1L;
        case SC1200_VCFG_YUYV_FORMAT:
            return VIDEO_FORMAT_P1M_P1L_P2M_P2L;
        case SC1200_VCFG_YVYU_FORMAT:
            return VIDEO_FORMAT_P1M_P2L_P2M_P1L;
        }
    }

    if (vcfg & SC1200_VCFG_4_2_0_MODE) {
        switch (vcfg & SC1200_VCFG_VID_INP_FORMAT) {
        case SC1200_VCFG_UYVY_FORMAT:
            return VIDEO_FORMAT_Y0Y1Y2Y3;
        case SC1200_VCFG_Y2YU_FORMAT:
            return VIDEO_FORMAT_Y3Y2Y1Y0;
        case SC1200_VCFG_YUYV_FORMAT:
            return VIDEO_FORMAT_Y1Y0Y3Y2;
        case SC1200_VCFG_YVYU_FORMAT:
            return VIDEO_FORMAT_Y1Y2Y3Y0;
        }
    }
    else {
        switch (vcfg & SC1200_VCFG_VID_INP_FORMAT) {
        case SC1200_VCFG_UYVY_FORMAT:
            return VIDEO_FORMAT_UYVY;
        case SC1200_VCFG_Y2YU_FORMAT:
            return VIDEO_FORMAT_Y2YU;
        case SC1200_VCFG_YUYV_FORMAT:
            return VIDEO_FORMAT_YUYV;
        case SC1200_VCFG_YVYU_FORMAT:
            return VIDEO_FORMAT_YVYU;
        }
    }
    return (GFX_STATUS_ERROR);
}

/*----------------------------------------------------------------------------
 * gfx_get_video_src_size
 *
 * This routine returns the size of the source video overlay buffer.  The 
 * return value is (height << 16) | width.
 *----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
sc1200_get_video_src_size(void)
#else
unsigned long
gfx_get_video_src_size(void)
#endif
{
    unsigned long width = 0, height = 0;

    /* DETERMINE SOURCE WIDTH FROM THE SC1200 VIDEO LINE SIZE */

    width = (READ_VID32(SC1200_VIDEO_CONFIG) >> 7) & 0x000001FE;
    if (READ_VID32(SC1200_VIDEO_CONFIG) & SC1200_VCFG_LINE_SIZE_UPPER)
        width += 512l;

    if (width) {
        /* DETERMINE HEIGHT BY DIVIDING TOTAL SIZE BY WIDTH */
        /* Get total size from display controller - abstracted. */

        height = gfx_get_display_video_size() / (width << 1);
    }
    return ((height << 16) | width);
}

/*----------------------------------------------------------------------------
 * gfx_get_video_line_size
 *
 * This routine returns the line size of the source video overlay buffer, in
 * pixels.
 *----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
sc1200_get_video_line_size(void)
#else
unsigned long
gfx_get_video_line_size(void)
#endif
{
    unsigned long width = 0;

    /* DETERMINE SOURCE WIDTH FROM THE SC1200 VIDEO LINE SIZE */

    width = (READ_VID32(SC1200_VIDEO_CONFIG) >> 7) & 0x000001FE;
    if (READ_VID32(SC1200_VIDEO_CONFIG) & SC1200_VCFG_LINE_SIZE_UPPER)
        width += 512l;
    return (width);
}

/*----------------------------------------------------------------------------
 * gfx_get_video_xclip
 *
 * This routine returns the number of bytes clipped on the left side of a
 * video overlay line (skipped at beginning).
 *----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
sc1200_get_video_xclip(void)
#else
unsigned long
gfx_get_video_xclip(void)
#endif
{
    unsigned long clip = 0;

    /* DETERMINE SOURCE WIDTH FROM THE SC1200 VIDEO LINE SIZE */

    clip = (READ_VID32(SC1200_VIDEO_CONFIG) >> 14) & 0x000007FC;
    return (clip);
}

/*----------------------------------------------------------------------------
 * gfx_get_video_offset
 *
 * This routine returns the current offset for the video overlay buffer.
 *----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
sc1200_get_video_offset(void)
#else
unsigned long
gfx_get_video_offset(void)
#endif
{
    return (gfx_get_display_video_offset());
}

/*---------------------------------------------------------------------------
 * gfx_get_video_upscale
 * 
 * This routine returns the scale factor for the video overlay window.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
sc1200_get_video_upscale(void)
#else
unsigned long
gfx_get_video_upscale(void)
#endif
{
    return (READ_VID32(SC1200_VIDEO_UPSCALE));
}

/*---------------------------------------------------------------------------
 * gfx_get_video_scale
 * 
 * This routine returns the scale factor for the video overlay window.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
sc1200_get_video_scale(void)
#else
unsigned long
gfx_get_video_scale(void)
#endif
{
    return gfx_get_video_upscale();
}

/*---------------------------------------------------------------------------
 * gfx_get_video_downscale_config
 * 
 * This routine returns the current type and value of video downscaling.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_get_video_downscale_config(unsigned short *type, unsigned short *m)
#else
int
gfx_get_video_downscale_config(unsigned short *type, unsigned short *m)
#endif
{
    unsigned long downscale;

    downscale = READ_VID32(SC1200_VIDEO_DOWNSCALER_CONTROL);
    *m = (unsigned short) ((downscale & SC1200_VIDEO_DOWNSCALE_FACTOR_MASK) >>
                           SC1200_VIDEO_DOWNSCALE_FACTOR_POS) + 1;

    switch (downscale & SC1200_VIDEO_DOWNSCALE_TYPE_MASK) {
    case SC1200_VIDEO_DOWNSCALE_TYPE_A:
        *type = VIDEO_DOWNSCALE_KEEP_1_OF;
        break;
    case SC1200_VIDEO_DOWNSCALE_TYPE_B:
        *type = VIDEO_DOWNSCALE_DROP_1_OF;
        break;
    default:
        return GFX_STATUS_ERROR;
        break;
    }
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_downscale_coefficients
 * 
 * This routine returns the current video downscaling coefficients.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void
sc1200_get_video_downscale_coefficients(unsigned short *coef1,
                                        unsigned short *coef2,
                                        unsigned short *coef3,
                                        unsigned short *coef4)
#else
void
gfx_get_video_downscale_coefficients(unsigned short *coef1,
                                     unsigned short *coef2,
                                     unsigned short *coef3,
                                     unsigned short *coef4)
#endif
{
    unsigned long coef;

    coef = READ_VID32(SC1200_VIDEO_DOWNSCALER_COEFFICIENTS);
    *coef1 =
        (unsigned short) ((coef >> SC1200_VIDEO_DOWNSCALER_COEF1_POS) &
                          SC1200_VIDEO_DOWNSCALER_COEF_MASK);
    *coef2 =
        (unsigned short) ((coef >> SC1200_VIDEO_DOWNSCALER_COEF2_POS) &
                          SC1200_VIDEO_DOWNSCALER_COEF_MASK);
    *coef3 =
        (unsigned short) ((coef >> SC1200_VIDEO_DOWNSCALER_COEF3_POS) &
                          SC1200_VIDEO_DOWNSCALER_COEF_MASK);
    *coef4 =
        (unsigned short) ((coef >> SC1200_VIDEO_DOWNSCALER_COEF4_POS) &
                          SC1200_VIDEO_DOWNSCALER_COEF_MASK);
    return;
}

/*---------------------------------------------------------------------------
 * gfx_get_video_downscale_enable
 * 
 * This routine returns 1 if video downscaling is currently enabled,
 * or 0 if it is currently disabled.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void
sc1200_get_video_downscale_enable(int *enable)
#else
void
gfx_get_video_downscale_enable(int *enable)
#endif
{
    if (READ_VID32(SC1200_VIDEO_DOWNSCALER_CONTROL) &
        SC1200_VIDEO_DOWNSCALE_ENABLE)
        *enable = 1;
    else
        *enable = 0;
    return;
}

/*---------------------------------------------------------------------------
 * gfx_get_video_dst_size
 * 
 * This routine returns the size of the displayed video overlay window.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
sc1200_get_video_dst_size(void)
#else
unsigned long
gfx_get_video_dst_size(void)
#endif
{
    unsigned long xsize, ysize;

    xsize = READ_VID32(SC1200_VIDEO_X_POS);
    xsize = ((xsize >> 16) & 0x7FF) - (xsize & 0x7FF);
    ysize = READ_VID32(SC1200_VIDEO_Y_POS);
    ysize = ((ysize >> 16) & 0x7FF) - (ysize & 0x7FF);
    return ((ysize << 16) | xsize);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_position
 * 
 * This routine returns the position of the video overlay window.  The
 * return value is (ypos << 16) | xpos.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
sc1200_get_video_position(void)
#else
unsigned long
gfx_get_video_position(void)
#endif
{
    unsigned long hadjust, vadjust;
    unsigned long xpos, ypos;

    /* READ HARDWARE POSITION */

    xpos = READ_VID32(SC1200_VIDEO_X_POS) & 0x000007FF;
    ypos = READ_VID32(SC1200_VIDEO_Y_POS) & 0x000007FF;

    /* GET ADJUSTMENT VALUES */
    /* Use routines to abstract version of display controller. */

    hadjust =
        (unsigned long) gfx_get_htotal() - (unsigned long) gfx_get_hsync_end() -
        14l;
    vadjust =
        (unsigned long) gfx_get_vtotal() - (unsigned long) gfx_get_vsync_end() +
        1l;
    xpos -= hadjust;
    ypos -= vadjust;
    return ((ypos << 16) | (xpos & 0x0000FFFF));
}

/*---------------------------------------------------------------------------
 * gfx_get_video_color_key
 * 
 * This routine returns the current video color key value.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
sc1200_get_video_color_key(void)
#else
unsigned long
gfx_get_video_color_key(void)
#endif
{
    return (READ_VID32(SC1200_VIDEO_COLOR_KEY));
}

/*---------------------------------------------------------------------------
 * gfx_get_video_color_key_mask
 * 
 * This routine returns the current video color mask value.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
sc1200_get_video_color_key_mask(void)
#else
unsigned long
gfx_get_video_color_key_mask(void)
#endif
{
    return (READ_VID32(SC1200_VIDEO_COLOR_MASK));
}

/*---------------------------------------------------------------------------
 * gfx_get_video_color_key_src
 * 
 * This routine returns 0 for video data compare, 1 for graphics data.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_get_video_color_key_src(void)
#else
int
gfx_get_video_color_key_src(void)
#endif
{
    if (READ_VID32(SC1200_DISPLAY_CONFIG) & SC1200_DCFG_VG_CK)
        return (0);
    return (1);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_filter
 * 
 * This routine returns if the filters are currently enabled.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_get_video_filter(void)
#else
int
gfx_get_video_filter(void)
#endif
{
    int retval = 0;

    if (READ_VID32(SC1200_VIDEO_CONFIG) & SC1200_VCFG_X_FILTER_EN)
        retval |= 1;
    if (READ_VID32(SC1200_VIDEO_CONFIG) & SC1200_VCFG_Y_FILTER_EN)
        retval |= 2;
    return (retval);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_request
 * 
 * This routine returns the horizontal (pixel) and vertical (lines) video
 * request values.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_get_video_request(short *x, short *y)
#else
int
gfx_get_video_request(short *x, short *y)
#endif
{
    int request = 0;

    request = (int) (READ_VID32(SC1200_VIDEO_REQUEST));
    *x = (request >> SC1200_VIDEO_X_REQUEST_POS) & SC1200_VIDEO_REQUEST_MASK;
    *y = (request >> SC1200_VIDEO_Y_REQUEST_POS) & SC1200_VIDEO_REQUEST_MASK;

    *x -= gfx_get_htotal() - gfx_get_hsync_end() - 2;
    *y -= gfx_get_vtotal() - gfx_get_vsync_end() + 1;

    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_source
 * 
 * This routine returns the current video source.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_get_video_source(VideoSourceType *source)
#else
int
gfx_get_video_source(VideoSourceType *source)
#endif
{
    switch (READ_VID32(SC1200_VIDEO_DISPLAY_MODE) & SC1200_VIDEO_SOURCE_MASK) {
    case SC1200_VIDEO_SOURCE_GX1:
        *source = VIDEO_SOURCE_MEMORY;
        break;
    case SC1200_VIDEO_SOURCE_DVIP:
        *source = VIDEO_SOURCE_DVIP;
        break;
    default:
        return GFX_STATUS_ERROR;
    }
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_get_vbi_source
 * 
 * This routine returns the current vbi source.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_get_vbi_source(VbiSourceType *source)
#else
int
gfx_get_vbi_source(VbiSourceType *source)
#endif
{
    switch (READ_VID32(SC1200_VIDEO_DISPLAY_MODE) & SC1200_VBI_SOURCE_MASK) {
    case SC1200_VBI_SOURCE_GX1:
        *source = VBI_SOURCE_MEMORY;
        break;
    case SC1200_VBI_SOURCE_DVIP:
        *source = VBI_SOURCE_DVIP;
        break;
    default:
        return GFX_STATUS_ERROR;
    }
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_get_vbi_lines
 * 
 * This routine returns the VBI lines which are sent to the TV encoder.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
sc1200_get_vbi_lines(int odd)
#else
unsigned long
gfx_get_vbi_lines(int odd)
#endif
{
    if (odd)
        return (READ_VID32(SC1200_VIDEO_ODD_VBI_LINE_ENABLE) &
                SC1200_VIDEO_VBI_LINE_ENABLE_MASK);
    return (READ_VID32(SC1200_VIDEO_EVEN_VBI_LINE_ENABLE) &
            SC1200_VIDEO_VBI_LINE_ENABLE_MASK);
}

/*---------------------------------------------------------------------------
 * gfx_get_vbi_total
 * 
 * This routine returns the total number of VBI bytes in the field.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
sc1200_get_vbi_total(int odd)
#else
unsigned long
gfx_get_vbi_total(int odd)
#endif
{
    if (odd)
        return (READ_VID32(SC1200_VIDEO_ODD_VBI_TOTAL_COUNT) &
                SC1200_VIDEO_VBI_TOTAL_COUNT_MASK);
    return (READ_VID32(SC1200_VIDEO_EVEN_VBI_TOTAL_COUNT) &
            SC1200_VIDEO_VBI_TOTAL_COUNT_MASK);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_interlaced()
 * 
 * This routine returns "1" if input video is currently in interlaced mode.
 * "0" otherwise.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_get_video_interlaced(void)
#else
int
gfx_get_video_interlaced(void)
#endif
{
    if (READ_VID32(SC1200_VID_ALPHA_CONTROL) & SC1200_VIDEO_IS_INTERLACED)
        return (1);
    else
        return (0);
}

/*---------------------------------------------------------------------------
 * gfx_get_color_space_YUV()
 * 
 * This routine returns "1" if video processor color space mode is currently
 * YUV. "0" otherwise.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_get_color_space_YUV(void)
#else
int
gfx_get_color_space_YUV(void)
#endif
{
    unsigned long control;

    control = READ_VID32(SC1200_VID_ALPHA_CONTROL);

    /* IS SC1200 VIDEO COLOR SPACE RGB OR CONVERTED TO RGB */
    if ((control & SC1200_VIDEO_INPUT_IS_RGB)
        || (control & SC1200_CSC_VIDEO_YUV_TO_RGB))
        return (0);
    else
        return (1);
}

/*---------------------------------------------------------------------------
 * gfx_get_vertical_scaler_offset()
 * 
 * This routine sets "offset" to the value by which odd frames are shifted,
 * if insert is enabled, and to 0 if no shifting occurs.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_get_vertical_scaler_offset(char *offset)
#else
int
gfx_get_vertical_scaler_offset(char *offset)
#endif
{
    unsigned long control;

    control = READ_VID32(SC1200_VID_ALPHA_CONTROL);
    if (control & SC1200_VERTICAL_SCALER_SHIFT_EN) {
        if ((control & SC1200_VERTICAL_SCALER_SHIFT_MASK) ==
            SC1200_VERTICAL_SCALER_SHIFT_INIT)
            *offset = 1;
        else
            return GFX_STATUS_ERROR;
        /* TODO: find the interpretation of other values */
    }
    else
        *offset = 0;
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_get_genlock_delay
 * 
 * This routine returns the genlock delay in 27 MHz clocks.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
sc1200_get_genlock_delay(void)
#else
unsigned long
gfx_get_genlock_delay(void)
#endif
{
    return (READ_VID32(SC1200_GENLOCK_DELAY) & SC1200_GENLOCK_DELAY_MASK);
}

/*---------------------------------------------------------------------------
 * gfx_get_genlock_enable
 * 
 * This routine returns "1" if genlock is currently enabled, "0" otherwise.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_get_genlock_enable(void)
#else
int
gfx_get_genlock_enable(void)
#endif
{
    if (READ_VID32(SC1200_GENLOCK) &
        (SC1200_GENLOCK_SINGLE_ENABLE | SC1200_GENLOCK_CONTINUOUS_ENABLE))
        return (1);
    else
        return (0);
}

/*---------------------------------------------------------------------------
 * gfx_get_video_cursor()
 * 
 * This routine configures the video hardware cursor.
 * If the "mask"ed bits in the graphics pixel match "key", then either 
 * "color1" or "color2" will be used for this pixel, according to the value of
 * the bit in offset "select_color2".
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_get_video_cursor(unsigned long *key, unsigned long *mask,
                        unsigned short *select_color2, unsigned long *color1,
                        unsigned short *color2)
#else
int
gfx_get_video_cursor(unsigned long *key, unsigned long *mask,
                     unsigned short *select_color2, unsigned long *color1,
                     unsigned short *color2)
#endif
{
    *select_color2 =
        (unsigned short) (READ_VID32(SC1200_CURSOR_COLOR_KEY) >>
                          SC1200_CURSOR_COLOR_KEY_OFFSET_POS);
    *key = READ_VID32(SC1200_CURSOR_COLOR_KEY) & SC1200_COLOR_MASK;
    *mask = READ_VID32(SC1200_CURSOR_COLOR_MASK) & SC1200_COLOR_MASK;
    *color1 = READ_VID32(SC1200_CURSOR_COLOR_1) & SC1200_COLOR_MASK;
    *color2 =
        (unsigned short) (READ_VID32(SC1200_CURSOR_COLOR_2) &
                          SC1200_COLOR_MASK);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_read_crc
 *
 * This routine returns the hardware CRC value, which is used for automated 
 * testing.  The value is like a checksum, but will change if pixels move
 * locations.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
unsigned long
sc1200_read_crc(void)
#else
unsigned long
gfx_read_crc(void)
#endif
{
    unsigned long crc = 0xFFFFFFFF;

    if (gfx_test_timing_active()) {
        /* WAIT UNTIL ACTIVE DISPLAY */

        while (!gfx_test_vertical_active());

        /* RESET CRC DURING ACTIVE DISPLAY */

        WRITE_VID32(SC1200_VID_CRC, 0);
        WRITE_VID32(SC1200_VID_CRC, 1);

        /* WAIT UNTIL NOT ACTIVE, THEN ACTIVE, NOT ACTIVE, THEN ACTIVE */

        while (gfx_test_vertical_active());
        while (!gfx_test_vertical_active());
        while (gfx_test_vertical_active());
        while (!gfx_test_vertical_active());
        crc = READ_VID32(SC1200_VID_CRC) >> 8;
    }
    return (crc);
}

/*----------------------------------------------------------------------------
 * gfx_get_macrovision_enable
 *
 * This routine returns the value "one" if macrovision currently enabled in 
 * the TV encoder, otherwise it returns the value "zero".
 *----------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
int
sc1200_get_macrovision_enable(void)
#else
int
gfx_get_macrovision_enable(void)
#endif
{
    if (READ_VID32(SC1200_TVENC_MV_CONTROL) == SC1200_TVENC_MV_ENABLE)
        return (1);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_get_alpha_enable
 * 
 * This routine returns 1 if the selected alpha window is currently 
 * enabled, or 0 if it is currently disabled.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void
sc1200_get_alpha_enable(int *enable)
#else
void
gfx_get_alpha_enable(int *enable)
#endif
{
    unsigned long value = 0;

    *enable = 0;
    if (gfx_alpha_select <= 2) {
        value =
            READ_VID32(SC1200_ALPHA_CONTROL_1 +
                       ((unsigned long) gfx_alpha_select << 4));
        if (value & SC1200_ACTRL_WIN_ENABLE)
            *enable = 1;
    }
    return;
}

/*---------------------------------------------------------------------------
 * gfx_get_alpha_size
 * 
 * This routine returns the size of the currently selected alpha region.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void
sc1200_get_alpha_size(unsigned short *x, unsigned short *y,
                      unsigned short *width, unsigned short *height)
#else
void
gfx_get_alpha_size(unsigned short *x, unsigned short *y,
                   unsigned short *width, unsigned short *height)
#endif
{
    unsigned long value = 0;

    *x = 0;
    *y = 0;
    *width = 0;
    *height = 0;
    if (gfx_alpha_select <= 2) {
        value =
            READ_VID32(SC1200_ALPHA_XPOS_1 +
                       ((unsigned long) gfx_alpha_select << 4));
        *x = (unsigned short) (value & 0x000007FF);
        *width = (unsigned short) ((value >> 16) & 0x000007FF) - *x;
        value =
            READ_VID32(SC1200_ALPHA_YPOS_1 +
                       ((unsigned long) gfx_alpha_select << 4));
        *y = (unsigned short) (value & 0x000007FF);
        *height = (unsigned short) ((value >> 16) & 0x000007FF) - *y;
    }
    *x -= gfx_get_htotal() - gfx_get_hsync_end() - 2;
    *y -= gfx_get_vtotal() - gfx_get_vsync_end() + 1;
    return;
}

/*---------------------------------------------------------------------------
 * gfx_get_alpha_value
 * 
 * This routine returns the alpha value and increment/decrement value of 
 * the currently selected alpha region.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void
sc1200_get_alpha_value(unsigned char *alpha, char *delta)
#else
void
gfx_get_alpha_value(unsigned char *alpha, char *delta)
#endif
{
    unsigned long value = 0;

    *alpha = 0;
    *delta = 0;
    if (gfx_alpha_select <= 2) {
        value =
            READ_VID32(SC1200_ALPHA_CONTROL_1 +
                       ((unsigned long) gfx_alpha_select << 4));
        *alpha = (unsigned char) (value & 0x00FF);
        *delta = (char) ((value >> 8) & 0x00FF);
    }
    return;
}

/*---------------------------------------------------------------------------
 * gfx_get_alpha_priority
 * 
 * This routine returns the priority of the currently selected alpha region.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void
sc1200_get_alpha_priority(int *priority)
#else
void
gfx_get_alpha_priority(int *priority)
#endif
{
    unsigned long pos = 0, value = 0;

    *priority = 0;
    if (gfx_alpha_select <= 2) {
        value = READ_VID32(SC1200_VID_ALPHA_CONTROL);
        pos = 16 + (gfx_alpha_select << 1);
        *priority = (int) ((value >> pos) & 3);
    }
    return;
}

/*---------------------------------------------------------------------------
 * gfx_get_alpha_color
 * 
 * This routine returns the color register value for the currently selected 
 * alpha region.  Bit 24 is set if the color register is enabled.
 *---------------------------------------------------------------------------
 */
#if GFX_VIDEO_DYNAMIC
void
sc1200_get_alpha_color(unsigned long *color)
#else
void
gfx_get_alpha_color(unsigned long *color)
#endif
{
    *color = 0;
    if (gfx_alpha_select <= 2) {
        *color =
            READ_VID32(SC1200_ALPHA_COLOR_1 +
                       ((unsigned long) gfx_alpha_select << 4));
    }
    return;
}

#endif                          /* GFX_READ_ROUTINES */

/* END OF FILE */
