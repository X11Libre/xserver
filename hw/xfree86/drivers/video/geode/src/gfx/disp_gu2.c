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
 */

/* 
 * This file contains routines for the second generation display controller.
 * */

void gu2_enable_compression(void);      /* private routine definition */
void gu2_disable_compression(void);     /* private routine definition */
int gu2_set_specified_mode(DISPLAYMODE * pMode, int bpp);

 /*---------------------------------------------------------------------------
 * WARNING!!!! INACCURATE DELAY MECHANISM
 *
 * In an effort to keep the code self contained and operating system 
 * independent, the delay loop just performs reads of a display controller
 * register.  This time will vary for faster processors.  The delay can always
 * be longer than intended, only effecting the time of the mode switch 
 * (obviously want it to still be under a second).  Problems with the hardware
 * only arise if the delay is not long enough.  
 *----------------------------------------------------------------------------
 */
#define RC_READS_PER_MILLISECOND 15000L

#if GFX_DISPLAY_DYNAMIC
void
gu2_delay_milliseconds(unsigned long milliseconds)
#else
void
gfx_delay_milliseconds(unsigned long milliseconds)
#endif
{
    /* ASSUME 300 MHZ 20 CLOCKS PER READ */
    unsigned long loop;

    loop = milliseconds * RC_READS_PER_MILLISECOND;
    while (loop-- > 0) {
        READ_REG32(MDC_UNLOCK);
    }
}

#if GFX_DISPLAY_DYNAMIC
void
gu2_delay_microseconds(unsigned long microseconds)
#else
void
gfx_delay_microseconds(unsigned long microseconds)
#endif
{
    /* ASSUME 400 MHz, 2 CLOCKS PER INCREMENT */
    unsigned long loop_count = microseconds * 15;

    while (loop_count-- > 0) {
        READ_REG32(MDC_UNLOCK);
    }
}

/*----------------------------------------------------------------------------
 * GFX_SET_DISPLAY_BPP
 *
 * This routine programs the bpp in the display controller.
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_set_display_bpp(unsigned short bpp)
#else
int
gfx_set_display_bpp(unsigned short bpp)
#endif
{
    unsigned long dcfg, lock;

    dcfg =
        READ_REG32(MDC_DISPLAY_CFG) & ~(MDC_DCFG_DISP_MODE_MASK |
                                        MDC_DCFG_16BPP_MODE_MASK);
    lock = READ_REG32(MDC_UNLOCK);

    switch (bpp) {
    case 12:
        dcfg |= (MDC_DCFG_DISP_MODE_16BPP | MDC_DCFG_12BPP);
        break;
    case 15:
        dcfg |= (MDC_DCFG_DISP_MODE_16BPP | MDC_DCFG_15BPP);
        break;
    case 16:
        dcfg |= (MDC_DCFG_DISP_MODE_16BPP | MDC_DCFG_16BPP);
        break;
    case 32:
        dcfg |= (MDC_DCFG_DISP_MODE_24BPP);
        break;
    case 8:
        dcfg |= (MDC_DCFG_DISP_MODE_8BPP);
        break;
    default:
        return GFX_STATUS_BAD_PARAMETER;
    }

    WRITE_REG32(MDC_UNLOCK, MDC_UNLOCK_VALUE);
    WRITE_REG32(MDC_DISPLAY_CFG, dcfg);
    WRITE_REG32(MDC_UNLOCK, lock);

    /* SET BPP IN GRAPHICS PIPELINE */
    gfx_set_bpp(bpp);

    return 0;
}

/*----------------------------------------------------------------------------
 * gu2_set_specified_mode (private routine)
 * This routine uses the parameters in the specified display mode structure
 * to program the display controller hardware.  
 *----------------------------------------------------------------------------
 */
int
gu2_set_specified_mode(DISPLAYMODE *pMode, int bpp)
{
    unsigned long unlock, value;
    unsigned long gcfg, dcfg;
    unsigned long size, pitch;
    unsigned long vid_buf_size;
    unsigned long bpp_mask, temp, dv_size;

    /* CHECK WHETHER TIMING CHANGE IS ALLOWED */
    /* Flag used for locking also overrides timing change restriction */
    if (gfx_timing_lock && !(pMode->flags & GFX_MODE_LOCK_TIMING))
        return GFX_STATUS_ERROR;

    /* CLEAR PANNING OFFSETS */
    DeltaX = 0;
    DeltaY = 0;
    panelLeft = 0;
    panelTop = 0;

    /* SET GLOBAL FLAG */

    if (pMode->flags & GFX_MODE_LOCK_TIMING)
        gfx_timing_lock = 1;

    /* CHECK FOR VALID BPP                          */
    /* As this function can be called directly from */
    /* gfx_set_display_timings, we must correct any */
    /* invalid bpp settings.                        */
    switch (bpp) {
    case 12:
        bpp_mask = 0x00000900;
        break;
    case 15:
        bpp_mask = 0x00000500;
        break;
    case 16:
        bpp_mask = 0x00000100;
        break;
    case 32:
        bpp_mask = 0x00000200;
        break;
    default:
        bpp_mask = 0x00000000;
        bpp = 8;
        break;
    }

    gbpp = bpp;

    /* DISABLE COMPRESSION */
    gu2_disable_compression();

    /* ALSO DISABLE VIDEO */
    /* Use private "reset video" routine to do all that is needed. */
    /* SC1200, for example, also disables the alpha blending regions. */
    gfx_reset_video();

    /* UNLOCK THE DISPLAY CONTROLLER REGISTERS */
    unlock = READ_REG32(MDC_UNLOCK);
    WRITE_REG32(MDC_UNLOCK, MDC_UNLOCK_VALUE);

    /* READ THE CURRENT REGISTER VALUES */
    gcfg = READ_REG32(MDC_GENERAL_CFG);
    dcfg = READ_REG32(MDC_DISPLAY_CFG);

    /* BLANK THE DISPLAY IN THE DISPLAY FILTER */
    gfx_set_crt_enable(0);

    /* DISABLE THE TIMING GENERATOR */
    dcfg &= ~(unsigned long) MDC_DCFG_TGEN;
    WRITE_REG32(MDC_DISPLAY_CFG, dcfg);

    /* DELAY: WAIT FOR PENDING MEMORY REQUESTS                            */
    /* This delay is used to make sure that all pending requests to the   */
    /* memory controller have completed before disabling the FIFO load.   */
    gfx_delay_milliseconds(5);

    /* DISABLE DISPLAY FIFO LOAD */
    gcfg &= ~(unsigned long) MDC_GCFG_DFLE;
    WRITE_REG32(MDC_GENERAL_CFG, gcfg);

    /* PRESERVE VIDEO INFORMATION */
    gcfg &= (unsigned long) (MDC_GCFG_YUVM | MDC_GCFG_VDSE);
    dcfg = 0;

    /* SET THE DOT CLOCK FREQUENCY             */
    /* Mask off the divide by two bit (bit 31) */
    if (!(pMode->flags & GFX_MODE_EXCLUDE_PLL))
        gfx_set_clock_frequency(pMode->frequency & 0x7FFFFFFF);

    /* DELAY: WAIT FOR THE PLL TO SETTLE */
    /* This allows the dot clock frequency that was just set to settle. */
    gfx_delay_milliseconds(10);

    /* SET THE GX DISPLAY CONTROLLER PARAMETERS */
    WRITE_REG32(MDC_FB_ST_OFFSET, 0);
    WRITE_REG32(MDC_CB_ST_OFFSET, 0);
    WRITE_REG32(MDC_CURS_ST_OFFSET, 0);
    WRITE_REG32(MDC_ICON_ST_OFFSET, 0);

    /* SET LINE SIZE AND PITCH */
    /* 1. Flat Panels must use the mode width and not  */
    /*    the timing width to set the pitch.           */
    /* 2. Mode sets will use a pitch that is aligned   */
    /*    on a 1K boundary to preserve legacy.  The    */
    /*    pitch can be overridden by a subsequent call */
    /*    to gfx_set_display_pitch.                    */
    if (PanelEnable)
        size = ModeWidth;
    else
        size = pMode->hactive;

    if (bpp > 8)
        size <<= 1;

    if (bpp > 16)
        size <<= 1;

    pitch = 1024;
    dv_size = MDC_DV_LINE_SIZE_1024;

    if (size > 1024) {
        pitch = 2048;
        dv_size = MDC_DV_LINE_SIZE_2048;
    }

    if (size > 2048) {
        pitch = 4096;
        dv_size = MDC_DV_LINE_SIZE_4096;
    }

    if (size > 4096) {
        pitch = 8192;
        dv_size = MDC_DV_LINE_SIZE_8192;
    }

    WRITE_REG32(MDC_GFX_PITCH, pitch >> 3);

    /* WRITE DIRTY/VALID CONTROL WITH LINE LENGTH */
    temp = READ_REG32(MDC_DV_CTL);
    WRITE_REG32(MDC_DV_CTL, (temp & ~MDC_DV_LINE_SIZE_MASK) | dv_size);

    if (PanelEnable) {
        size = pMode->hactive;
        if (bpp > 8)
            size <<= 1;
        if (bpp > 16)
            size <<= 1;
    }

    /* SAVE PREVIOUSLY STORED VIDEO LINE SIZE */
    vid_buf_size = READ_REG32(MDC_LINE_SIZE) & 0xFF000000;

    /* ADD 2 TO SIZE FOR POSSIBLE START ADDRESS ALIGNMENTS */
    WRITE_REG32(MDC_LINE_SIZE, ((size >> 3) + 2) | vid_buf_size);

    /* ALWAYS ENABLE VIDEO AND GRAPHICS DATA            */
    /* These bits are relics from a previous design and */
    /* should always be enabled.                        */
    dcfg |= (unsigned long) (MDC_DCFG_VDEN | MDC_DCFG_GDEN);

    /* SET PIXEL FORMAT */
    dcfg |= bpp_mask;

    /* ENABLE TIMING GENERATOR, TIM. REG. UPDATES, PALETTE BYPASS */
    /* AND VERT. INT. SELECT                                      */
    dcfg |=
        (unsigned long) (MDC_DCFG_TGEN | MDC_DCFG_TRUP | MDC_DCFG_PALB |
                         MDC_DCFG_VISL);

    /* DISABLE ADDRESS MASKS */
    dcfg |= MDC_DCFG_A20M;
    dcfg |= MDC_DCFG_A18M;

    /* SET FIFO PRIORITIES AND DISPLAY FIFO LOAD ENABLE     */
    /* Set the priorities higher for high resolution modes. */
    if (pMode->hactive > 1024 || bpp == 32)
        gcfg |= 0x000A901;
    else
        gcfg |= 0x0006501;

    /* ENABLE FLAT PANEL CENTERING                          */
    /* For panel modes having a resolution smaller than the */
    /* panel resolution, turn on data centering.            */
    if (PanelEnable && ModeWidth < PanelWidth)
        dcfg |= MDC_DCFG_DCEN;

    /* COMBINE AND SET TIMING VALUES */
    value = (unsigned long) (pMode->hactive - 1) |
        (((unsigned long) (pMode->htotal - 1)) << 16);
    WRITE_REG32(MDC_H_ACTIVE_TIMING, value);
    value = (unsigned long) (pMode->hblankstart - 1) |
        (((unsigned long) (pMode->hblankend - 1)) << 16);
    WRITE_REG32(MDC_H_BLANK_TIMING, value);
    value = (unsigned long) (pMode->hsyncstart - 1) |
        (((unsigned long) (pMode->hsyncend - 1)) << 16);
    WRITE_REG32(MDC_H_SYNC_TIMING, value);
    value = (unsigned long) (pMode->vactive - 1) |
        (((unsigned long) (pMode->vtotal - 1)) << 16);
    WRITE_REG32(MDC_V_ACTIVE_TIMING, value);
    value = (unsigned long) (pMode->vblankstart - 1) |
        (((unsigned long) (pMode->vblankend - 1)) << 16);
    WRITE_REG32(MDC_V_BLANK_TIMING, value);
    value = (unsigned long) (pMode->vsyncstart - 1) |
        (((unsigned long) (pMode->vsyncend - 1)) << 16);
    WRITE_REG32(MDC_V_SYNC_TIMING, value);

    WRITE_REG32(MDC_DISPLAY_CFG, dcfg);
    WRITE_REG32(MDC_GENERAL_CFG, gcfg);

    /* CONFIGURE DISPLAY OUTPUT FROM VIDEO PROCESSOR */
    gfx_set_display_control(((pMode->flags & GFX_MODE_NEG_HSYNC) ? 1 : 0) |
                            ((pMode->flags & GFX_MODE_NEG_VSYNC) ? 2 : 0));

    /* RESTORE VALUE OF MDC_UNLOCK */
    WRITE_REG32(MDC_UNLOCK, unlock);

    /* RESET THE PITCH VALUES IN THE GP */
    gfx_reset_pitch((unsigned short) pitch);

    gfx_set_bpp((unsigned short) bpp);

    return GFX_STATUS_OK;
}

/*----------------------------------------------------------------------------
 * GFX_IS_DISPLAY_MODE_SUPPORTED
 *
 * This routine sets the specified display mode.
 *
 * Returns 1 if successful, 0 if mode could not be set.
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_is_display_mode_supported(int xres, int yres, int bpp, int hz)
#else
int
gfx_is_display_mode_supported(int xres, int yres, int bpp, int hz)
#endif
{
    unsigned int mode;
    unsigned long hz_flag = 0, bpp_flag = 0;

    /* SET FLAGS TO MATCH REFRESH RATE */
    gfx_mode_hz_conversion
        /* SET BPP FLAGS TO LIMIT MODE SELECTION */
        gfx_mode_bpp_conversion
        /* LOOP THROUGH THE AVAILABLE MODES TO FIND A MATCH */
        for (mode = 0; mode < NUM_RC_DISPLAY_MODES; mode++) {
        if ((DisplayParams[mode].hactive == (unsigned short) xres) &&
            (DisplayParams[mode].vactive == (unsigned short) yres) &&
            (DisplayParams[mode].flags & hz_flag) &&
            (DisplayParams[mode].flags & bpp_flag)) {

            /* REDCLOUD DOES NOT SUPPORT EMULATED VGA MODES */
            if ((DisplayParams[mode].flags & GFX_MODE_PIXEL_DOUBLE) ||
                (DisplayParams[mode].flags & GFX_MODE_LINE_DOUBLE))
                continue;

            /* SET THE DISPLAY CONTROLLER FOR THE SELECTED MODE */
            return (mode);
        }
    }
    return (-1);
}

/*----------------------------------------------------------------------------
 * gfx_set_display_mode
 *
 * This routine sets the specified display mode.
 *
 * Returns 1 if successful, 0 if mode could not be set.
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_set_display_mode(int xres, int yres, int bpp, int hz)
#else
int
gfx_set_display_mode(int xres, int yres, int bpp, int hz)
#endif
{
    int mode;

    /* DISABLE FLAT PANEL */
    /* Flat Panel settings are enabled by the function gfx_set_fixed_timings 
     * and disabled by gfx_set_display_mode.
     * */
    PanelEnable = 0;

    mode = gfx_is_display_mode_supported(xres, yres, bpp, hz);
    if (mode >= 0) {
        if (gu2_set_specified_mode(&DisplayParams[mode], bpp) == GFX_STATUS_OK)
            return (1);
    }
    return (0);
}

/*----------------------------------------------------------------------------
 * GFX_SET_DISPLAY_TIMINGS
 *
 * This routine sets the display controller mode using the specified timing
 * values (as opposed to using the tables internal to Durango).
 *
 * Returns GFX_STATUS_OK ON SUCCESS, GFX_STATUS_ERROR otherwise.
 *----------------------------------------------------------------------------  
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_set_display_timings(unsigned short bpp, unsigned short flags,
                        unsigned short hactive, unsigned short hblankstart,
                        unsigned short hsyncstart, unsigned short hsyncend,
                        unsigned short hblankend, unsigned short htotal,
                        unsigned short vactive, unsigned short vblankstart,
                        unsigned short vsyncstart, unsigned short vsyncend,
                        unsigned short vblankend, unsigned short vtotal,
                        unsigned long frequency)
#else
int
gfx_set_display_timings(unsigned short bpp, unsigned short flags,
                        unsigned short hactive, unsigned short hblankstart,
                        unsigned short hsyncstart, unsigned short hsyncend,
                        unsigned short hblankend, unsigned short htotal,
                        unsigned short vactive, unsigned short vblankstart,
                        unsigned short vsyncstart, unsigned short vsyncend,
                        unsigned short vblankend, unsigned short vtotal,
                        unsigned long frequency)
#endif
{
    /* SET MODE STRUCTURE WITH SPECIFIED VALUES */

    gfx_display_mode.flags = 0;
    if (flags & 1)
        gfx_display_mode.flags |= GFX_MODE_NEG_HSYNC;
    if (flags & 2)
        gfx_display_mode.flags |= GFX_MODE_NEG_VSYNC;
    if (flags & 4)
        gfx_display_mode.flags |= GFX_MODE_EXCLUDE_PLL;
    if (flags & 0x1000)
        gfx_display_mode.flags |= GFX_MODE_LOCK_TIMING;
    gfx_display_mode.hactive = hactive;
    gfx_display_mode.hblankstart = hblankstart;
    gfx_display_mode.hsyncstart = hsyncstart;
    gfx_display_mode.hsyncend = hsyncend;
    gfx_display_mode.hblankend = hblankend;
    gfx_display_mode.htotal = htotal;
    gfx_display_mode.vactive = vactive;
    gfx_display_mode.vblankstart = vblankstart;
    gfx_display_mode.vsyncstart = vsyncstart;
    gfx_display_mode.vsyncend = vsyncend;
    gfx_display_mode.vblankend = vblankend;
    gfx_display_mode.vtotal = vtotal;
    gfx_display_mode.frequency = frequency;

    /* CALL ROUTINE TO SET MODE */
    return (gu2_set_specified_mode(&gfx_display_mode, bpp));
}

/*----------------------------------------------------------------------------
 * GFX_SET_VTOTAL
 *
 * This routine sets the display controller vertical total to
 * "vtotal". As a side effect it also sets vertical blank end.
 * It should be used when only this value needs to be changed,
 * due to speed considerations.
 *
 * Note: it is the caller's responsibility to make sure that
 * a legal vtotal is used, i.e. that "vtotal" is greater than or
 * equal to vsync end.
 *
 * Always returns 0.
 *----------------------------------------------------------------------------  
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_set_vtotal(unsigned short vtotal)
#else
int
gfx_set_vtotal(unsigned short vtotal)
#endif
{
    unsigned long unlock, dcfg, vactive, vblank;

    /* UNLOCK THE DISPLAY CONTROLLER REGISTERS */
    unlock = READ_REG32(MDC_UNLOCK);
    WRITE_REG32(MDC_UNLOCK, MDC_UNLOCK_VALUE);

    /* READ THE CURRENT RC VALUES */
    dcfg = READ_REG32(MDC_DISPLAY_CFG);
    vactive = READ_REG32(MDC_V_ACTIVE_TIMING);
    vblank = READ_REG32(MDC_V_BLANK_TIMING);

    /* DISABLE TIMING REGISTER UPDATES */
    WRITE_REG32(MDC_DISPLAY_CFG, dcfg & ~(unsigned long) MDC_DCFG_TRUP);

    /* WRITE NEW TIMING VALUES */
    WRITE_REG32(MDC_V_ACTIVE_TIMING,
                (vactive & MDC_VAT_VA_MASK) | (unsigned long) (vtotal -
                                                               1) << 16);
    WRITE_REG32(MDC_V_BLANK_TIMING,
                (vblank & MDC_VBT_VBS_MASK) | (unsigned long) (vtotal -
                                                               1) << 16);

    /* RESTORE OLD RC VALUES */
    WRITE_REG32(MDC_DISPLAY_CFG, dcfg);
    WRITE_REG32(MDC_UNLOCK, unlock);

    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_display_pitch
 *
 * This routine sets the pitch of the frame buffer to the specified value.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu2_set_display_pitch(unsigned short pitch)
#else
void
gfx_set_display_pitch(unsigned short pitch)
#endif
{
    unsigned long value = 0;
    unsigned long lock = READ_REG32(MDC_UNLOCK);

    value = READ_REG32(MDC_GFX_PITCH) & 0xFFFF0000;
    value |= (pitch >> 3);
    WRITE_REG32(MDC_UNLOCK, MDC_UNLOCK_VALUE);
    WRITE_REG32(MDC_GFX_PITCH, value);

    /* SET RENDERING PITCHES TO MATCH */
    gfx_reset_pitch(pitch);

    /* SET THE FRAME DIRTY MODE                  */
    /* Non-standard pitches, i.e. pitches that   */
    /* are not 1K, 2K or 4K must mark the entire */
    /* frame as dirty when writing to the frame  */
    /* buffer.                                   */
    value = READ_REG32(MDC_GENERAL_CFG);

    if (pitch == 1024 || pitch == 2048 || pitch == 4096 || pitch == 8192)
        value &= ~(unsigned long) (MDC_GCFG_FDTY);
    else
        value |= (unsigned long) (MDC_GCFG_FDTY);

    WRITE_REG32(MDC_GENERAL_CFG, value);
    WRITE_REG32(MDC_UNLOCK, lock);
}

/*---------------------------------------------------------------------------
 * gfx_set_display_offset
 *
 * This routine sets the start address of the frame buffer.  It is 
 * typically used to pan across a virtual desktop (frame buffer larger than 
 * the displayed screen) or to flip the display between multiple buffers.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu2_set_display_offset(unsigned long offset)
#else
void
gfx_set_display_offset(unsigned long offset)
#endif
{
    /* UPDATE FRAME BUFFER OFFSET */
    unsigned long lock;

    lock = READ_REG32(MDC_UNLOCK);
    WRITE_REG32(MDC_UNLOCK, MDC_UNLOCK_VALUE);

    /* START ADDRESS EFFECTS DISPLAY COMPRESSION */
    /* Disable compression for non-zero start address values.              */
    /* Enable compression if offset is zero and compression is intended to */
    /* be enabled from a previous call to "gfx_set_compression_enable".    */
    /* Compression should be disabled BEFORE the offset is changed         */
    /* and enabled AFTER the offset is changed.                            */
    if (offset == 0) {
        WRITE_REG32(MDC_FB_ST_OFFSET, offset);
        if (gfx_compression_enabled) {
            /* WAIT FOR THE OFFSET TO BE LATCHED */
            gfx_wait_vertical_blank();
            gu2_enable_compression();
        }
    }
    else {
        /* ONLY DISABLE COMPRESSION ONCE */
        if (gfx_compression_active)
            gu2_disable_compression();

        WRITE_REG32(MDC_FB_ST_OFFSET, offset);
    }

    WRITE_REG32(MDC_UNLOCK, lock);
}

/*---------------------------------------------------------------------------
 * gfx_set_display_palette_entry
 *
 * This routine sets an palette entry in the display controller.
 * A 32-bit X:R:G:B value.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_set_display_palette_entry(unsigned long index, unsigned long palette)
#else
int
gfx_set_display_palette_entry(unsigned long index, unsigned long palette)
#endif
{
    unsigned long dcfg, unlock;

    if (index > 0xFF)
        return GFX_STATUS_BAD_PARAMETER;

    unlock = READ_REG32(MDC_UNLOCK);
    dcfg = READ_REG32(MDC_DISPLAY_CFG);

    WRITE_REG32(MDC_UNLOCK, MDC_UNLOCK_VALUE);
    WRITE_REG32(MDC_DISPLAY_CFG, dcfg & ~MDC_DCFG_PALB);
    WRITE_REG32(MDC_UNLOCK, unlock);

    WRITE_REG32(MDC_PAL_ADDRESS, index);
    WRITE_REG32(MDC_PAL_DATA, palette);

    return GFX_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * gfx_set_display_palette
 *
 * This routine sets the entire palette in the display controller.
 * A pointer is provided to a 256 entry table of 32-bit X:R:G:B values.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_set_display_palette(unsigned long *palette)
#else
int
gfx_set_display_palette(unsigned long *palette)
#endif
{
    unsigned long unlock, dcfg, i;

    WRITE_REG32(MDC_PAL_ADDRESS, 0);

    if (palette) {
        unlock = READ_REG32(MDC_UNLOCK);
        dcfg = READ_REG32(MDC_DISPLAY_CFG);

        WRITE_REG32(MDC_UNLOCK, MDC_UNLOCK_VALUE);
        WRITE_REG32(MDC_DISPLAY_CFG, dcfg & ~MDC_DCFG_PALB);
        WRITE_REG32(MDC_UNLOCK, unlock);

        for (i = 0; i < 256; i++)
            WRITE_REG32(MDC_PAL_DATA, palette[i]);

        return GFX_STATUS_OK;
    }
    return GFX_STATUS_BAD_PARAMETER;
}

/*---------------------------------------------------------------------------
 * gfx_set_cursor_enable
 *
 * This routine enables or disables the hardware cursor.  
 *
 * WARNING: The cursor start offset must be set by setting the cursor 
 * position before calling this routine to assure that memory reads do not
 * go past the end of graphics memory (this can hang GXm).
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu2_set_cursor_enable(int enable)
#else
void
gfx_set_cursor_enable(int enable)
#endif
{
    unsigned long unlock, gcfg;

    /* SET OR CLEAR CURSOR ENABLE BIT */
    unlock = READ_REG32(MDC_UNLOCK);
    gcfg = READ_REG32(MDC_GENERAL_CFG);
    if (enable)
        gcfg |= MDC_GCFG_CURE;
    else
        gcfg &= ~(MDC_GCFG_CURE);

    /* WRITE NEW REGISTER VALUE */
    WRITE_REG32(MDC_UNLOCK, MDC_UNLOCK_VALUE);
    WRITE_REG32(MDC_GENERAL_CFG, gcfg);
    WRITE_REG32(MDC_UNLOCK, unlock);
}

/*---------------------------------------------------------------------------
 * gfx_set_cursor_colors
 *
 * This routine sets the colors of the hardware cursor.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu2_set_cursor_colors(unsigned long bkcolor, unsigned long fgcolor)
#else
void
gfx_set_cursor_colors(unsigned long bkcolor, unsigned long fgcolor)
#endif
{
    /* SET CURSOR COLORS */
    WRITE_REG32(MDC_PAL_ADDRESS, 0x100);
    WRITE_REG32(MDC_PAL_DATA, bkcolor);
    WRITE_REG32(MDC_PAL_DATA, fgcolor);
}

/*---------------------------------------------------------------------------
 * gfx_set_cursor_position
 *
 * This routine sets the position of the hardware cusror.  The starting
 * offset of the cursor buffer must be specified so that the routine can 
 * properly clip scanlines if the cursor is off the top of the screen.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu2_set_cursor_position(unsigned long memoffset,
                        unsigned short xpos, unsigned short ypos,
                        unsigned short xhotspot, unsigned short yhotspot)
#else
void
gfx_set_cursor_position(unsigned long memoffset,
                        unsigned short xpos, unsigned short ypos,
                        unsigned short xhotspot, unsigned short yhotspot)
#endif
{
    unsigned long unlock;

    short x = (short) xpos - (short) xhotspot;
    short y = (short) ypos - (short) yhotspot;
    short xoffset = 0;
    short yoffset = 0;

    if (x < -63)
        return;
    if (y < -63)
        return;

    if (PanelEnable) {
        if ((ModeWidth > PanelWidth) || (ModeHeight > PanelHeight)) {
            gfx_enable_panning(xpos, ypos);
            x = x - (unsigned short) panelLeft;
            y = y - (unsigned short) panelTop;
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

    if (y < 0) {
        yoffset = -y;
        y = 0;
    }
    memoffset += (unsigned long) yoffset << 4;

    /* SET CURSOR POSITION */
    unlock = READ_REG32(MDC_UNLOCK);
    WRITE_REG32(MDC_UNLOCK, MDC_UNLOCK_VALUE);
    WRITE_REG32(MDC_CURS_ST_OFFSET, memoffset);
    WRITE_REG32(MDC_CURSOR_X, (unsigned long) x |
                (((unsigned long) xoffset) << 11));
    WRITE_REG32(MDC_CURSOR_Y, (unsigned long) y |
                (((unsigned long) yoffset) << 11));
    WRITE_REG32(MDC_UNLOCK, unlock);
}

/*---------------------------------------------------------------------------
 * gfx_set_cursor_shape32
 *
 * This routine loads 32x32 cursor data into the cursor buffer in graphics 
 * memory.
 * As the Redcloud cursor is actually 64x64, we must pad the outside of the 
 * cursor data with transparent pixels.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu2_set_cursor_shape32(unsigned long memoffset,
                       unsigned long *andmask, unsigned long *xormask)
#else
void
gfx_set_cursor_shape32(unsigned long memoffset,
                       unsigned long *andmask, unsigned long *xormask)
#endif
{
    int i;

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
}

/*---------------------------------------------------------------------------
 * gfx_set_cursor_shape64
 *
 * This routine loads 64x64 cursor data into the cursor buffer in graphics 
 * memory.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu2_set_cursor_shape64(unsigned long memoffset,
                       unsigned long *andmask, unsigned long *xormask)
#else
void
gfx_set_cursor_shape64(unsigned long memoffset,
                       unsigned long *andmask, unsigned long *xormask)
#endif
{
    int i;

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
}

/*---------------------------------------------------------------------------
 * gfx_set_icon_enable
 *
 * This routine enables or disables the hardware icon.  The icon position
 * and colors should be programmed prior to calling this routine for the
 * first time.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu2_set_icon_enable(int enable)
#else
void
gfx_set_icon_enable(int enable)
#endif
{
    unsigned long unlock, gcfg;

    /* SET OR CLEAR ICON ENABLE BIT */
    unlock = READ_REG32(MDC_UNLOCK);
    gcfg = READ_REG32(MDC_GENERAL_CFG);
    if (enable)
        gcfg |= MDC_GCFG_ICNE;
    else
        gcfg &= ~(MDC_GCFG_ICNE);

    /* WRITE NEW REGISTER VALUE */
    WRITE_REG32(MDC_UNLOCK, MDC_UNLOCK_VALUE);
    WRITE_REG32(MDC_GENERAL_CFG, gcfg);
    WRITE_REG32(MDC_UNLOCK, unlock);
}

/*---------------------------------------------------------------------------
 * gfx_set_icon_colors
 *
 * This routine sets the three icon colors.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu2_set_icon_colors(unsigned long color0, unsigned long color1,
                    unsigned long color2)
#else
void
gfx_set_icon_colors(unsigned long color0, unsigned long color1,
                    unsigned long color2)
#endif
{
    /* ICON COLORS LOCATED AT PALETTE INDEXES 102-104h */
    WRITE_REG32(MDC_PAL_ADDRESS, 0x102);

    WRITE_REG32(MDC_PAL_DATA, color0);
    WRITE_REG32(MDC_PAL_DATA, color1);
    WRITE_REG32(MDC_PAL_DATA, color2);
}

/*---------------------------------------------------------------------------
 * gfx_set_icon_position
 *
 * This routine sets the starting X coordinate for the hardware icon and the 
 * memory offset for the icon buffer.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu2_set_icon_position(unsigned long memoffset, unsigned short xpos)
#else
void
gfx_set_icon_position(unsigned long memoffset, unsigned short xpos)
#endif
{
    unsigned long lock = READ_REG32(MDC_UNLOCK);

    WRITE_REG32(MDC_UNLOCK, MDC_UNLOCK_VALUE);

    /* PROGRAM THE MEMORY OFFSET */
    WRITE_REG32(MDC_ICON_ST_OFFSET, memoffset & 0x0FFFFFFF);

    /* PROGRAM THE XCOORDINATE */
    WRITE_REG32(MDC_ICON_X, (unsigned long) (xpos & 0x07FF));

    WRITE_REG32(MDC_UNLOCK, lock);
}

/*---------------------------------------------------------------------------
 * gfx_set_icon_shape64
 *
 * This routine initializes the icon buffer according to the current mode.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu2_set_icon_shape64(unsigned long memoffset, unsigned long *andmask,
                     unsigned long *xormask, unsigned int lines)
#else
void
gfx_set_icon_shape64(unsigned long memoffset, unsigned long *andmask,
                     unsigned long *xormask, unsigned int lines)
#endif
{
    unsigned short i, height;

    height = lines << 1;

    for (i = 0; i < height; i += 2) {
        /* EVEN QWORDS CONTAIN THE AND MASK     */
        /* Swap dwords to hide qword constraint */
        WRITE_FB32(memoffset, andmask[i + 1]);
        WRITE_FB32(memoffset + 4, andmask[i]);

        /* ODD QWORDS CONTAIN THE XOR MASK */
        WRITE_FB32(memoffset + 8, xormask[i + 1]);
        WRITE_FB32(memoffset + 12, xormask[i]);

        memoffset += 16;
    }
}

/*---------------------------------------------------------------------------
 * gu2_enable_compression
 *
 * This is a private routine to this module (not exposed in the Durango API).
 * It enables display compression.
 *---------------------------------------------------------------------------
 */
void
gu2_enable_compression(void)
{
    unsigned long unlock, gcfg, temp;

    /* DO NOT ENABLE IF START ADDRESS IS NOT ZERO */
    if (READ_REG32(MDC_FB_ST_OFFSET) & 0x0FFFFFFF)
        return;

    /* SET GLOBAL INDICATOR */
    gfx_compression_active = 1;

    /* CLEAR DIRTY/VALID BITS IN MEMORY CONTROLLER */
    /* Software is required to do this before enabling compression.   */
    /* Don't want controller to think that old lines are still valid. */
    /* Writing a 1 to bit 0 of the DV Control register will force the */
    /* hardware to clear all the valid bits.                          */
    temp = READ_REG32(MDC_DV_CTL);
    WRITE_REG32(MDC_DV_CTL, temp | 0x00000001);

    /* TURN ON COMPRESSION CONTROL BITS */
    unlock = READ_REG32(MDC_UNLOCK);
    gcfg = READ_REG32(MDC_GENERAL_CFG);
    gcfg |= MDC_GCFG_CMPE | MDC_GCFG_DECE;
    WRITE_REG32(MDC_UNLOCK, MDC_UNLOCK_VALUE);
    WRITE_REG32(MDC_GENERAL_CFG, gcfg);
    WRITE_REG32(MDC_UNLOCK, unlock);
}

/*---------------------------------------------------------------------------
 * gu2_disable_compression
 *
 * This is a private routine to this module (not exposed in the Durango API).
 * It disables display compression.
 *---------------------------------------------------------------------------
 */
void
gu2_disable_compression(void)
{
    unsigned long unlock, gcfg;

    /* SET GLOBAL INDICATOR */

    gfx_compression_active = 0;

    /* TURN OFF COMPRESSION CONTROL BITS */

    unlock = READ_REG32(MDC_UNLOCK);
    gcfg = READ_REG32(MDC_GENERAL_CFG);
    gcfg &= ~(MDC_GCFG_CMPE | MDC_GCFG_DECE);
    WRITE_REG32(MDC_UNLOCK, MDC_UNLOCK_VALUE);
    WRITE_REG32(MDC_GENERAL_CFG, gcfg);
    WRITE_REG32(MDC_UNLOCK, unlock);
}

/*---------------------------------------------------------------------------
 * gfx_set_compression_enable
 *
 * This routine enables or disables display compression.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_set_compression_enable(int enable)
#else
int
gfx_set_compression_enable(int enable)
#endif
{
    /* SET GLOBAL VARIABLE FOR INTENDED STATE */
    /* Compression can only be enabled for non-zero start address values. */
    /* Keep state to enable compression on start address changes. */

    gfx_compression_enabled = enable;
    if (enable)
        gu2_enable_compression();
    else
        gu2_disable_compression();
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_compression_offset
 *
 * This routine sets the base offset for the compression buffer.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_set_compression_offset(unsigned long offset)
#else
int
gfx_set_compression_offset(unsigned long offset)
#endif
{
    unsigned long lock;

    /* MUST BE 16-BYTE ALIGNED FOR REDCLOUD */

    if (offset & 0x0F)
        return (1);

    /* SET REGISTER VALUE */

    lock = READ_REG32(MDC_UNLOCK);
    WRITE_REG32(MDC_UNLOCK, MDC_UNLOCK_VALUE);
    WRITE_REG32(MDC_CB_ST_OFFSET, offset & 0x0FFFFFFF);
    WRITE_REG32(MDC_UNLOCK, lock);

    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_compression_pitch
 *
 * This routine sets the pitch, in bytes, of the compression buffer. 
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_set_compression_pitch(unsigned short pitch)
#else
int
gfx_set_compression_pitch(unsigned short pitch)
#endif
{
    unsigned long lock, line_delta;

    lock = READ_REG32(MDC_UNLOCK);

    /* SET REGISTER VALUE */

    line_delta = READ_REG32(MDC_GFX_PITCH) & 0x0000FFFF;
    line_delta |= (((unsigned long) pitch << 13) & 0xFFFF0000);
    WRITE_REG32(MDC_UNLOCK, MDC_UNLOCK_VALUE);
    WRITE_REG32(MDC_GFX_PITCH, line_delta);
    WRITE_REG32(MDC_UNLOCK, lock);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_compression_size
 *
 * This routine sets the line size of the compression buffer, which is the
 * maximum number of bytes allowed to store a compressed line.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_set_compression_size(unsigned short size)
#else
int
gfx_set_compression_size(unsigned short size)
#endif
{
    unsigned long lock, buf_size;

    /* SUBTRACT 32 FROM SIZE                          */
    /* The display controller will actually write     */
    /* 4 extra QWords.  So, if we assume that "size"  */
    /* refers to the allocated size, we must subtract */
    /* 32 bytes.                                      */

    size -= 32;

    /* SET REGISTER VALUE */

    lock = READ_REG32(MDC_UNLOCK);
    buf_size = READ_REG32(MDC_LINE_SIZE) & 0xFF80FFFF;
    buf_size |= ((((unsigned long) size >> 3) + 1) & 0x7F) << 16;
    WRITE_REG32(MDC_UNLOCK, MDC_UNLOCK_VALUE);
    WRITE_REG32(MDC_LINE_SIZE, buf_size);
    WRITE_REG32(MDC_UNLOCK, lock);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_display_video_format (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine is called by "gfx_set_video_format".  It abstracts the 
 * version of the display controller from the video overlay routines.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu2_set_display_video_format(unsigned long format)
#else
void
gfx_set_display_video_format(unsigned long format)
#endif
{
    unsigned long gcfg, lock;

    lock = READ_REG32(MDC_UNLOCK);
    gcfg = READ_REG32(MDC_GENERAL_CFG);

    switch (format) {
    case VIDEO_FORMAT_Y0Y1Y2Y3:
    case VIDEO_FORMAT_Y3Y2Y1Y0:
    case VIDEO_FORMAT_Y1Y0Y3Y2:
    case VIDEO_FORMAT_Y1Y2Y3Y0:
        gcfg |= MDC_GCFG_YUVM;
        break;

    default:
        gcfg &= ~MDC_GCFG_YUVM;
        break;
    }

    WRITE_REG32(MDC_UNLOCK, MDC_UNLOCK_VALUE);
    WRITE_REG32(MDC_GENERAL_CFG, gcfg);
    WRITE_REG32(MDC_UNLOCK, lock);
}

/*---------------------------------------------------------------------------
 * gfx_set_display_video_enable (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine is called by "gfx_set_video_enable".  It abstracts the 
 * version of the display controller from the video overlay routines.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu2_set_display_video_enable(int enable)
#else
void
gfx_set_display_video_enable(int enable)
#endif
{
    unsigned long lock, gcfg;

    /* READ CURRENT VALUES */

    lock = READ_REG32(MDC_UNLOCK);
    gcfg = READ_REG32(MDC_GENERAL_CFG);

    /* SET OR CLEAR VIDEO ENABLE IN GENERAL_CFG */

    if (enable)
        gcfg |= MDC_GCFG_VIDE;
    else
        gcfg &= ~MDC_GCFG_VIDE;

    /* WRITE REGISTER */

    WRITE_REG32(MDC_UNLOCK, MDC_UNLOCK_VALUE);
    WRITE_REG32(MDC_GENERAL_CFG, gcfg);
    WRITE_REG32(MDC_UNLOCK, lock);
}

/*---------------------------------------------------------------------------
 * gfx_set_display_video_size (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine is called by "gfx_set_video_size".  It abstracts the 
 * version of the display controller from the video overlay routines.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu2_set_display_video_size(unsigned short width, unsigned short height)
#else
void
gfx_set_display_video_size(unsigned short width, unsigned short height)
#endif
{
    unsigned long lock, value, yuv_420;

    /* READ CURRENT VALUES */

    lock = READ_REG32(MDC_UNLOCK);
    value = READ_REG32(MDC_LINE_SIZE) & 0x00FFFFFF;
    yuv_420 = READ_REG32(MDC_GENERAL_CFG) & MDC_GCFG_YUVM;

    /* LINE WIDTH IS 1/4 FOR 4:2:0 VIDEO */
    /* All data must be 32-byte aligned. */

    if (yuv_420) {
        width >>= 1;
        width = (width + 7) & 0xFFF8;
    }
    else {
        width <<= 1;
        width = (width + 31) & 0xFFE0;
    }

    /* ONLY THE LINE SIZE IS PROGRAMMED IN THE DISPLAY CONTROLLER */

    value |= ((unsigned long) width << 21);

    /* WRITE THE REGISTER */

    WRITE_REG32(MDC_UNLOCK, MDC_UNLOCK_VALUE);
    WRITE_REG32(MDC_LINE_SIZE, value);
    WRITE_REG32(MDC_UNLOCK, lock);
}

/*---------------------------------------------------------------------------
 * gfx_set_display_video_offset (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine is called by "gfx_set_video_offset".  It abstracts the 
 * version of the display controller from the video overlay routines.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu2_set_display_video_offset(unsigned long offset)
#else
void
gfx_set_display_video_offset(unsigned long offset)
#endif
{
    unsigned long lock;

    lock = READ_REG32(MDC_UNLOCK);
    WRITE_REG32(MDC_UNLOCK, MDC_UNLOCK_VALUE);
    offset &= 0x0FFFFFF0;
    WRITE_REG32(MDC_VID_Y_ST_OFFSET, offset);
    WRITE_REG32(MDC_UNLOCK, lock);
}

/*---------------------------------------------------------------------------
 * gfx_set_display_video_yuv_offsets (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine is called by gfx_set_video_yuv_offsets.  It abstracts the 
 * version of the display controller from the video overlay routines.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu2_set_display_video_yuv_offsets(unsigned long yoffset,
                                  unsigned long uoffset, unsigned long voffset)
#else
void
gfx_set_display_video_yuv_offsets(unsigned long yoffset,
                                  unsigned long uoffset, unsigned long voffset)
#endif
{
    unsigned long lock;

    lock = READ_REG32(MDC_UNLOCK);

    yoffset &= 0x0FFFFFF0;
    uoffset &= 0x0FFFFFF8;
    voffset &= 0x0FFFFFF8;

    WRITE_REG32(MDC_UNLOCK, MDC_UNLOCK_VALUE);
    WRITE_REG32(MDC_VID_Y_ST_OFFSET, yoffset);
    WRITE_REG32(MDC_VID_U_ST_OFFSET, uoffset);
    WRITE_REG32(MDC_VID_V_ST_OFFSET, voffset);
    WRITE_REG32(MDC_UNLOCK, lock);
}

/*---------------------------------------------------------------------------
 * gfx_set_display_video_yuv_pitch (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine is called by gfx_set_video_yuv_pitch.  It abstracts the
 * version of the display controller from the video overlay routines.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu2_set_display_video_yuv_pitch(unsigned long ypitch, unsigned long uvpitch)
#else
void
gfx_set_display_video_yuv_pitch(unsigned long ypitch, unsigned long uvpitch)
#endif
{
    unsigned long lock, pitch;

    lock = READ_REG32(MDC_UNLOCK);

    pitch = ((uvpitch << 13) & 0xFFFF0000) | ((ypitch >> 3) & 0xFFFF);

    WRITE_REG32(MDC_UNLOCK, MDC_UNLOCK_VALUE);
    WRITE_REG32(MDC_VID_YUV_PITCH, pitch);
    WRITE_REG32(MDC_UNLOCK, lock);
}

/*---------------------------------------------------------------------------
 * gfx_set_display_video_downscale (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine is called by gfx_set_video_vertical_downscale.  It abstracts
 * the version of the display controller from the video overlay routines.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu2_set_display_video_downscale(unsigned short srch, unsigned short dsth)
#else
void
gfx_set_display_video_downscale(unsigned short srch, unsigned short dsth)
#endif
{
    unsigned long lock, delta;

    lock = READ_REG32(MDC_UNLOCK);

    /* CLIP SCALING LIMITS */
    /* Upscaling is performed in a separate function. */
    /* Maximum scale ratio is 1/2.                    */

    if (dsth > srch || dsth <= (srch >> 1))
        delta = 0;
    else
        delta = (((unsigned long) srch << 14) / (unsigned long) dsth) << 18;

    WRITE_REG32(MDC_UNLOCK, MDC_UNLOCK_VALUE);
    WRITE_REG32(MDC_VID_DS_DELTA, delta);
    WRITE_REG32(MDC_UNLOCK, lock);
}

/*---------------------------------------------------------------------------
 * gfx_set_display_video_downscale_enable (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine is called by "gfx_set_video_vertical_downscale_enable".  
 * It abstracts the version of the display controller from the video overlay
 * routines.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu2_set_display_video_vertical_downscale_enable(int enable)
#else
void
gfx_set_display_video_vertical_downscale_enable(int enable)
#endif
{
    unsigned long gcfg, unlock;

    unlock = READ_REG32(MDC_UNLOCK);
    gcfg = READ_REG32(MDC_GENERAL_CFG);

    if (enable)
        gcfg |= MDC_GCFG_VDSE;
    else
        gcfg &= ~MDC_GCFG_VDSE;

    WRITE_REG32(MDC_UNLOCK, MDC_UNLOCK_VALUE);
    WRITE_REG32(MDC_GENERAL_CFG, gcfg);
    WRITE_REG32(MDC_UNLOCK, unlock);
}

/*---------------------------------------------------------------------------
 * gfx_test_timing_active
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_test_timing_active(void)
#else
int
gfx_test_timing_active(void)
#endif
{
    if (READ_REG32(MDC_DISPLAY_CFG) & MDC_DCFG_TGEN)
        return (1);
    else
        return (0);
}

/*---------------------------------------------------------------------------
 * gfx_test_vertical_active
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_test_vertical_active(void)
#else
int
gfx_test_vertical_active(void)
#endif
{
    if (READ_REG32(MDC_LINE_CNT_STATUS) & MDC_LNCNT_VNA)
        return (0);

    return (1);
}

/*---------------------------------------------------------------------------
 * gfx_wait_vertical_blank
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_wait_vertical_blank(void)
#else
int
gfx_wait_vertical_blank(void)
#endif
{
    if (gfx_test_timing_active()) {
        while (!gfx_test_vertical_active());
        while (gfx_test_vertical_active());
    }
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_enable_panning 
 *
 * This routine  enables the panning when the Mode is bigger than the panel
 * size.
 *---------------------------------------------------------------------------
 */

#if GFX_DISPLAY_DYNAMIC
void
gu2_enable_panning(int x, int y)
#else
void
gfx_enable_panning(int x, int y)
#endif
{
    unsigned long modeBytesPerPixel;
    unsigned long modeBytesPerScanline = 0;
    unsigned long startAddress = 0;

    modeBytesPerPixel = (gbpp + 7) / 8;
    modeBytesPerScanline = (READ_REG32(MDC_GFX_PITCH) & 0x0000FFFF) << 3;

    /* TEST FOR NO-WORK */

    if (x >= DeltaX && x < ((int) PanelWidth + DeltaX) &&
        y >= DeltaY && y < ((int) PanelHeight + DeltaY))
        return;

    /* ADJUST PANNING VARIABLES WHEN CURSOR EXCEEDS BOUNDARY       */
    /* Test the boundary conditions for each coordinate and update */
    /* all variables and the starting offset accordingly.          */

    if (x < DeltaX)
        DeltaX = x;

    else if (x >= (DeltaX + (int) PanelWidth))
        DeltaX = x - (int) PanelWidth + 1;

    if (y < DeltaY)
        DeltaY = y;

    else if (y >= (DeltaY + (int) PanelHeight))
        DeltaY = y - (int) PanelHeight + 1;

    /* CALCULATE THE START OFFSET */

    startAddress =
        (DeltaX * modeBytesPerPixel) + (DeltaY * modeBytesPerScanline);

    gfx_set_display_offset(startAddress);

    /* SET PANEL COORDINATES                    */
    /* Panel's x position must be DWORD aligned */

    panelTop = DeltaY;
    panelLeft = DeltaX * modeBytesPerPixel;

    if (panelLeft & 3)
        panelLeft = (panelLeft & 0xFFFFFFFC) + 4;

    panelLeft /= modeBytesPerPixel;
}

/*---------------------------------------------------------------------------
 * gfx_is_panel_mode_supported
 *---------------------------------------------------------------------------
 */

#if GFX_DISPLAY_DYNAMIC
int
gu2_is_panel_mode_supported(int panelResX, int panelResY,
                            unsigned short width, unsigned short height,
                            unsigned short bpp)
#else
int
gfx_is_panel_mode_supported(int panelResX, int panelResY,
                            unsigned short width, unsigned short height,
                            unsigned short bpp)
#endif
{
    unsigned int mode;

    /* LOOP THROUGH THE AVAILABLE MODES TO FIND A MATCH */
    for (mode = 0; mode < NUM_FIXED_TIMINGS_MODES; mode++) {
        if ((FixedParams[mode].xres == width) &&
            (FixedParams[mode].yres == height) &&
            (FixedParams[mode].panelresx == panelResX) &&
            (FixedParams[mode].panelresy == panelResY)) {
            return ((int) mode);
        }
    }

    return -1;
}

/*---------------------------------------------------------------------------
 * gfx_set_fixed_timings
 *---------------------------------------------------------------------------
 */

#if GFX_DISPLAY_DYNAMIC
int
gu2_set_fixed_timings(int panelResX, int panelResY, unsigned short width,
                      unsigned short height, unsigned short bpp)
#else
int
gfx_set_fixed_timings(int panelResX, int panelResY, unsigned short width,
                      unsigned short height, unsigned short bpp)
#endif
{
    unsigned int mode;

    ModeWidth = width;
    ModeHeight = height;
    PanelWidth = (unsigned short) panelResX;
    PanelHeight = (unsigned short) panelResY;
    PanelEnable = 1;

    /* LOOP THROUGH THE AVAILABLE MODES TO FIND A MATCH */
    for (mode = 0; mode < NUM_FIXED_TIMINGS_MODES; mode++) {
        if ((FixedParams[mode].xres == width) &&
            (FixedParams[mode].yres == height) &&
            (FixedParams[mode].panelresx == panelResX) &&
            (FixedParams[mode].panelresy == panelResY)) {

            /* SET THE 92xx FOR THE SELECTED MODE */
            FIXEDTIMINGS *fmode = &FixedParams[mode];

            gfx_set_display_timings(bpp, 3, fmode->hactive,
                                    fmode->hblankstart, fmode->hsyncstart,
                                    fmode->hsyncend, fmode->hblankend,
                                    fmode->htotal, fmode->vactive,
                                    fmode->vblankstart, fmode->vsyncstart,
                                    fmode->vsyncend, fmode->vblankend,
                                    fmode->vtotal, fmode->frequency);

            return (1);
        }                       /* end if() */
    }                           /* end for() */

    return (-1);
}

/*---------------------------------------------------------------------------
 * gfx_set_panel_present
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_set_panel_present(int panelResX, int panelResY, unsigned short width,
                      unsigned short height, unsigned short bpp)
#else
int
gfx_set_panel_present(int panelResX, int panelResY, unsigned short width,
                      unsigned short height, unsigned short bpp)
#endif
{
    /* SET VALID BPP         */
    /* 16BPP is the default. */

    if (bpp != 8 && bpp != 12 && bpp != 15 && bpp != 16 && bpp != 32)
        bpp = 16;

    /* RECORD PANEL PARAMETERS */
    /* This routine does not touch any panel timings.  It is used when custom
     * panel settings are set up in advance by the BIOS or an application, but
     * the application still requires access to other panel functionality
     * provided by Durango (i.e. panning).
     * */

    ModeWidth = width;
    ModeHeight = height;
    PanelWidth = (unsigned short) panelResX;
    PanelHeight = (unsigned short) panelResY;
    PanelEnable = 1;
    gbpp = bpp;

    /* PROGRAM THE BPP IN THE DISPLAY CONTROLLER */

    gfx_set_display_bpp(bpp);

    return (GFX_STATUS_OK);
}

/* THE FOLLOWING READ ROUTINES ARE ALWAYS INCLUDED: */

/*---------------------------------------------------------------------------
 * gfx_get_display_pitch
 *
 * This routine returns the current pitch of the frame buffer, in bytes.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu2_get_display_pitch(void)
#else
unsigned short
gfx_get_display_pitch(void)
#endif
{
    return ((unsigned short) (READ_REG32(MDC_GFX_PITCH) & 0x0000FFFF) << 3);
}

/*----------------------------------------------------------------------------
 * gfx_mode_frequency_supported
 *
 * This routine examines if the requested mode with pixel frequency is 
 * supported.
 *
 * Returns >0 if successful , <0 if freq. could not be found and matched.
 *----------------------------------------------------------------------------  
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_mode_frequency_supported(int xres, int yres, int bpp,
                             unsigned long frequency)
#else
int
gfx_mode_frequency_supported(int xres, int yres, int bpp,
                             unsigned long frequency)
#endif
{
    unsigned int index;
    unsigned long value;
    unsigned long bpp_flag = 0;

    gfx_mode_bpp_conversion_def(bpp)
        for (index = 0; index < NUM_RC_DISPLAY_MODES; index++) {
        if ((DisplayParams[index].hactive == (unsigned int) xres) &&
            (DisplayParams[index].vactive == (unsigned int) yres) &&
            (DisplayParams[index].flags & bpp_flag) &&
            (DisplayParams[index].frequency == frequency)) {
            int hz = 0;

            value = DisplayParams[index].flags;

            if (value & GFX_MODE_56HZ)
                hz = 56;
            else if (value & GFX_MODE_60HZ)
                hz = 60;
            else if (value & GFX_MODE_70HZ)
                hz = 70;
            else if (value & GFX_MODE_72HZ)
                hz = 72;
            else if (value & GFX_MODE_75HZ)
                hz = 75;
            else if (value & GFX_MODE_85HZ)
                hz = 85;
            else if (value & GFX_MODE_90HZ)
                hz = 90;
            else if (value & GFX_MODE_100HZ)
                hz = 100;
            return (hz);
        }
    }

    return (-1);
}

/*----------------------------------------------------------------------------
 * gfx_refreshrate_from_frequency
 *
 * This routine maps the frequency to close match refresh rate
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_get_refreshrate_from_frequency(int xres, int yres, int bpp, int *hz,
                                   unsigned long frequency)
#else
int
gfx_get_refreshrate_from_frequency(int xres, int yres, int bpp, int *hz,
                                   unsigned long frequency)
#endif
{
    unsigned int index, closematch = 0;
    unsigned long value;
    unsigned long bpp_flag = 0;
    long min, diff;

    *hz = 60;

    gfx_mode_bpp_conversion_def(bpp)
        /* FIND THE REGISTER VALUES FOR THE DESIRED FREQUENCY */
        /* Search the table for the closest frequency (16.16 format). */
        min = 0x7fffffff;
    for (index = 0; index < NUM_RC_DISPLAY_MODES; index++) {
        if ((DisplayParams[index].htotal == (unsigned int) xres) &&
            (DisplayParams[index].vtotal == (unsigned int) yres) &&
            (DisplayParams[index].flags & bpp_flag)) {
            diff = (long) frequency - (long) DisplayParams[index].frequency;
            if (diff < 0)
                diff = -diff;

            if (diff < min) {
                min = diff;
                closematch = index;
            }
        }
    }

    value = DisplayParams[closematch].flags;

    if (value & GFX_MODE_56HZ)
        *hz = 56;
    else if (value & GFX_MODE_60HZ)
        *hz = 60;
    else if (value & GFX_MODE_70HZ)
        *hz = 70;
    else if (value & GFX_MODE_72HZ)
        *hz = 72;
    else if (value & GFX_MODE_75HZ)
        *hz = 75;
    else if (value & GFX_MODE_85HZ)
        *hz = 85;
    else if (value & GFX_MODE_90HZ)
        *hz = 90;
    else if (value & GFX_MODE_100HZ)
        *hz = 100;

    return (1);
}

/*----------------------------------------------------------------------------
 * gfx_refreshrate_from_mode
 *
 * This routine is identical to the gfx_get_refreshrate_from_frequency,
 * except that the active timing values are compared instead of the total
 * values.  Some modes (such as 70Hz and 72Hz) may be confused in this routine
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_get_refreshrate_from_mode(int xres, int yres, int bpp, int *hz,
                              unsigned long frequency)
#else
int
gfx_get_refreshrate_from_mode(int xres, int yres, int bpp, int *hz,
                              unsigned long frequency)
#endif
{
    unsigned int index, closematch = 0;
    unsigned long value;
    unsigned long bpp_flag = 0;
    long min, diff;

    *hz = 60;

    gfx_mode_bpp_conversion_def(bpp)
        /* FIND THE REGISTER VALUES FOR THE DESIRED FREQUENCY */
        /* Search the table for the closest frequency (16.16 format). */
        min = 0x7fffffff;
    for (index = 0; index < NUM_RC_DISPLAY_MODES; index++) {
        if ((DisplayParams[index].hactive == (unsigned int) xres) &&
            (DisplayParams[index].vactive == (unsigned int) yres) &&
            (DisplayParams[index].flags & bpp_flag)) {
            diff = (long) frequency - (long) DisplayParams[index].frequency;
            if (diff < 0)
                diff = -diff;

            if (diff < min) {
                min = diff;
                closematch = index;
            }
        }
    }

    value = DisplayParams[closematch].flags;

    if (value & GFX_MODE_56HZ)
        *hz = 56;
    else if (value & GFX_MODE_60HZ)
        *hz = 60;
    else if (value & GFX_MODE_70HZ)
        *hz = 70;
    else if (value & GFX_MODE_72HZ)
        *hz = 72;
    else if (value & GFX_MODE_75HZ)
        *hz = 75;
    else if (value & GFX_MODE_85HZ)
        *hz = 85;
    else if (value & GFX_MODE_90HZ)
        *hz = 90;
    else if (value & GFX_MODE_100HZ)
        *hz = 100;

    return (1);
}

/*----------------------------------------------------------------------------
 * gfx_get_frequency_from_refreshrate
 *
 * This routine maps the refresh rate to the closest matching PLL frequency.
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_get_frequency_from_refreshrate(int xres, int yres, int bpp, int hz,
                                   int *frequency)
#else
int
gfx_get_frequency_from_refreshrate(int xres, int yres, int bpp, int hz,
                                   int *frequency)
#endif
{
    unsigned int index;
    int retval = -1;
    unsigned long hz_flag = 0;
    unsigned long bpp_flag = 0;

    *frequency = 0;

    gfx_mode_hz_conversion gfx_mode_bpp_conversion_def(bpp)
        /* FIND THE REGISTER VALUES FOR THE DESIRED FREQUENCY */
        /* Search the table for the closest frequency (16.16 format). */
    for (index = 0; index < NUM_RC_DISPLAY_MODES; index++) {
        if ((DisplayParams[index].hactive == (unsigned short) xres) &&
            (DisplayParams[index].vactive == (unsigned short) yres) &&
            (DisplayParams[index].flags & bpp_flag) &&
            (DisplayParams[index].flags & hz_flag)) {
            *frequency = DisplayParams[index].frequency;
            retval = 1;
        }
    }
    return retval;
}

/*---------------------------------------------------------------------------
 * gfx_get_max_supported_pixel_clock
 *
 * This routine returns the maximum recommended speed for the pixel clock. The
 * return value is an integer of the format xxxyyy, where xxx.yyy is the
 * maximum floating point pixel clock speed.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long
gu2_get_max_supported_pixel_clock(void)
#else
unsigned long
gfx_get_max_supported_pixel_clock(void)
#endif
{
    return 229500;
}

/*----------------------------------------------------------------------------
 * gfx_get_display_mode
 *
 * This routine gets the specified display mode.
 *
 * Returns >0 if successful and mode returned, <0 if mode could not be found.
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_get_display_mode(int *xres, int *yres, int *bpp, int *hz)
#else
int
gfx_get_display_mode(int *xres, int *yres, int *bpp, int *hz)
#endif
{
    unsigned int mode = 0;
    unsigned long pll_freq = 0, bpp_flag = 0;

    *xres = gfx_get_hactive();
    *yres = gfx_get_vactive();
    *bpp = gfx_get_display_bpp();
    pll_freq = gfx_get_clock_frequency();

    /* SET BPP FLAGS TO LIMIT MODE SELECTION */
    gfx_mode_bpp_conversion_def(*bpp)
        for (mode = 0; mode < NUM_RC_DISPLAY_MODES; mode++) {
        if ((DisplayParams[mode].hactive == (unsigned int) *xres) &&
            (DisplayParams[mode].vactive == (unsigned int) *yres) &&
            (DisplayParams[mode].frequency == pll_freq) &&
            (DisplayParams[mode].flags & bpp_flag)) {

            pll_freq = DisplayParams[mode].flags;

            if (pll_freq & GFX_MODE_56HZ)
                *hz = 56;
            else if (pll_freq & GFX_MODE_60HZ)
                *hz = 60;
            else if (pll_freq & GFX_MODE_70HZ)
                *hz = 70;
            else if (pll_freq & GFX_MODE_72HZ)
                *hz = 72;
            else if (pll_freq & GFX_MODE_75HZ)
                *hz = 75;
            else if (pll_freq & GFX_MODE_85HZ)
                *hz = 85;
            else if (pll_freq & GFX_MODE_90HZ)
                *hz = 90;
            else if (pll_freq & GFX_MODE_100HZ)
                *hz = 100;

            return (1);
        }
    }
    return (-1);
}

/*----------------------------------------------------------------------------
 * GFX_GET_DISPLAY_DETAILS
 *
 * This routine gets the specified display mode.
 *
 * Returns 1 if successful, 0 if mode could not be get.
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_get_display_details(unsigned int mode, int *xres, int *yres, int *hz)
#else
int
gfx_get_display_details(unsigned int mode, int *xres, int *yres, int *hz)
#endif
{
    if (mode < NUM_RC_DISPLAY_MODES) {
        if (DisplayParams[mode].flags & GFX_MODE_56HZ)
            *hz = 56;
        else if (DisplayParams[mode].flags & GFX_MODE_60HZ)
            *hz = 60;
        else if (DisplayParams[mode].flags & GFX_MODE_70HZ)
            *hz = 70;
        else if (DisplayParams[mode].flags & GFX_MODE_72HZ)
            *hz = 72;
        else if (DisplayParams[mode].flags & GFX_MODE_75HZ)
            *hz = 75;
        else if (DisplayParams[mode].flags & GFX_MODE_85HZ)
            *hz = 85;
        else if (DisplayParams[mode].flags & GFX_MODE_90HZ)
            *hz = 90;
        else if (DisplayParams[mode].flags & GFX_MODE_100HZ)
            *hz = 100;

        *xres = DisplayParams[mode].hactive;
        *yres = DisplayParams[mode].vactive;

        if (DisplayParams[mode].flags & GFX_MODE_PIXEL_DOUBLE)
            *xres >>= 1;
        if (DisplayParams[mode].flags & GFX_MODE_LINE_DOUBLE)
            *yres >>= 1;

        return (1);
    }
    return (0);
}

/*----------------------------------------------------------------------------
 * GFX_GET_DISPLAY_MODE_COUNT
 *
 * This routine gets the number of available display modes.
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_get_display_mode_count(void)
#else
int
gfx_get_display_mode_count(void)
#endif
{
    return (NUM_RC_DISPLAY_MODES);
}

/*----------------------------------------------------------------------------
 * gfx_get_frame_buffer_line_size
 *
 * Returns the current frame buffer line size, in bytes
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long
gu2_get_frame_buffer_line_size(void)
#else
unsigned long
gfx_get_frame_buffer_line_size(void)
#endif
{
    return ((READ_REG32(MDC_LINE_SIZE) & 0x7FF) << 3);
}

/*---------------------------------------------------------------------------
 * gfx_get_hactive
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu2_get_hactive(void)
#else
unsigned short
gfx_get_hactive(void)
#endif
{
    return ((unsigned short) ((READ_REG32(MDC_H_ACTIVE_TIMING) & 0x0FF8) + 8));
}

/*---------------------------------------------------------------------------
 * gfx_get_hsync_start
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu2_get_hsync_start(void)
#else
unsigned short
gfx_get_hsync_start(void)
#endif
{
    return ((unsigned short) ((READ_REG32(MDC_H_SYNC_TIMING) & 0x0FF8) + 8));
}

/*---------------------------------------------------------------------------
 * gfx_get_hsync_end
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu2_get_hsync_end(void)
#else
unsigned short
gfx_get_hsync_end(void)
#endif
{
    return ((unsigned short) (((READ_REG32(MDC_H_SYNC_TIMING) >> 16) & 0x0FF8)
                              + 8));
}

/*---------------------------------------------------------------------------
 * gfx_get_htotal
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu2_get_htotal(void)
#else
unsigned short
gfx_get_htotal(void)
#endif
{
    return ((unsigned short) (((READ_REG32(MDC_H_ACTIVE_TIMING) >> 16) &
                               0x0FF8) + 8));
}

/*---------------------------------------------------------------------------
 * gfx_get_vactive
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu2_get_vactive(void)
#else
unsigned short
gfx_get_vactive(void)
#endif
{
    return ((unsigned short) ((READ_REG32(MDC_V_ACTIVE_TIMING) & 0x07FF) + 1));
}

/*---------------------------------------------------------------------------
 * gfx_get_vsync_end
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu2_get_vsync_end(void)
#else
unsigned short
gfx_get_vsync_end(void)
#endif
{
    return ((unsigned short) (((READ_REG32(MDC_V_SYNC_TIMING) >> 16) & 0x07FF)
                              + 1));
}

/*---------------------------------------------------------------------------
 * gfx_get_vtotal
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu2_get_vtotal(void)
#else
unsigned short
gfx_get_vtotal(void)
#endif
{
    return ((unsigned short) (((READ_REG32(MDC_V_ACTIVE_TIMING) >> 16) &
                               0x07FF) + 1));
}

/*----------------------------------------------------------------------------
 * gfx_get_display_bpp
 *
 * This routine returns the current color depth of the active display.
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu2_get_display_bpp(void)
#else
unsigned short
gfx_get_display_bpp(void)
#endif
{
    unsigned long dcfg = READ_REG32(MDC_DISPLAY_CFG);

    switch ((dcfg & MDC_DCFG_DISP_MODE_MASK) >> 8) {
    case 0:
        return (8);
    case 2:
        return (32);
    case 1:
        switch ((dcfg & MDC_DCFG_16BPP_MODE_MASK) >> 10) {
        case 0:
            return (16);
        case 1:
            return (15);
        case 2:
            return (12);
        default:
            return (0);
        }
    }

    /* INVALID SETTING */
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_get_vline
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu2_get_vline(void)
#else
unsigned short
gfx_get_vline(void)
#endif
{
    unsigned short current_scan_line;

    /* Read similar value twice to ensure that the value is not 
     * transitioning */
    do
        current_scan_line =
            (unsigned short) (READ_REG32(MDC_LINE_CNT_STATUS) &
                              MDC_LNCNT_V_LINE_CNT);
    while (current_scan_line !=
           (unsigned short) (READ_REG32(MDC_LINE_CNT_STATUS) &
                             MDC_LNCNT_V_LINE_CNT));

    return (current_scan_line >> 16);
}

/*-----------------------------------------------------------------------------
 * gfx_get_display_offset
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long
gu2_get_display_offset(void)
#else
unsigned long
gfx_get_display_offset(void)
#endif
{
    return (READ_REG32(MDC_FB_ST_OFFSET) & 0x0FFFFFFF);
}

/*-----------------------------------------------------------------------------
 * gfx_get_cursor_offset
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long
gu2_get_cursor_offset(void)
#else
unsigned long
gfx_get_cursor_offset(void)
#endif
{
    return (READ_REG32(MDC_CURS_ST_OFFSET) & 0x0FFFFFFF);
}

#if GFX_READ_ROUTINES

/*************************************************************/
/*  READ ROUTINES  |  INCLUDED FOR DIAGNOSTIC PURPOSES ONLY  */
/*************************************************************/

/*---------------------------------------------------------------------------
 * gfx_get_hblank_start
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu2_get_hblank_start(void)
#else
unsigned short
gfx_get_hblank_start(void)
#endif
{
    return ((unsigned short) ((READ_REG32(MDC_H_BLANK_TIMING) & 0x0FF8) + 8));
}

/*---------------------------------------------------------------------------
 * gfx_get_hblank_end
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu2_get_hblank_end(void)
#else
unsigned short
gfx_get_hblank_end(void)
#endif
{
    return ((unsigned short) (((READ_REG32(MDC_H_BLANK_TIMING) >> 16) & 0x0FF8)
                              + 8));
}

/*---------------------------------------------------------------------------
 * gfx_get_vblank_start
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu2_get_vblank_start(void)
#else
unsigned short
gfx_get_vblank_start(void)
#endif
{
    return ((unsigned short) ((READ_REG32(MDC_V_BLANK_TIMING) & 0x07FF) + 1));
}

/*---------------------------------------------------------------------------
 * gfx_get_vsync_start
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu2_get_vsync_start(void)
#else
unsigned short
gfx_get_vsync_start(void)
#endif
{
    return ((unsigned short) ((READ_REG32(MDC_V_SYNC_TIMING) & 0x07FF) + 1));
}

/*---------------------------------------------------------------------------
 * gfx_get_vblank_end
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu2_get_vblank_end(void)
#else
unsigned short
gfx_get_vblank_end(void)
#endif
{
    return ((unsigned short) (((READ_REG32(MDC_V_BLANK_TIMING) >> 16) & 0x07FF)
                              + 1));
}

/*----------------------------------------------------------------------------
 * gfx_get_display_palette_entry
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_get_display_palette_entry(unsigned long index, unsigned long *palette)
#else
int
gfx_get_display_palette_entry(unsigned long index, unsigned long *palette)
#endif
{
    if (index > 0xFF)
        return GFX_STATUS_BAD_PARAMETER;

    WRITE_REG32(MDC_PAL_ADDRESS, index);
    *palette = READ_REG32(MDC_PAL_DATA);

    return 0;
}

/*----------------------------------------------------------------------------
 * gfx_get_display_palette
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu2_get_display_palette(unsigned long *palette)
#else
void
gfx_get_display_palette(unsigned long *palette)
#endif
{
    unsigned long i;

    WRITE_REG32(MDC_PAL_ADDRESS, 0);
    for (i = 0; i < 256; i++) {
        palette[i] = READ_REG32(MDC_PAL_DATA);
    }
}

/*----------------------------------------------------------------------------
 * gfx_get_cursor_enable
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long
gu2_get_cursor_enable(void)
#else
unsigned long
gfx_get_cursor_enable(void)
#endif
{
    return (READ_REG32(MDC_GENERAL_CFG) & MDC_GCFG_CURE);
}

/*----------------------------------------------------------------------------
 * gfx_get_cursor_position
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long
gu2_get_cursor_position(void)
#else
unsigned long
gfx_get_cursor_position(void)
#endif
{
    return ((READ_REG32(MDC_CURSOR_X) & 0x07FF) |
            ((READ_REG32(MDC_CURSOR_Y) << 16) & 0x07FF0000));
}

/*----------------------------------------------------------------------------
 * gfx_get_cursor_offset
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long
gu2_get_cursor_clip(void)
#else
unsigned long
gfx_get_cursor_clip(void)
#endif
{
    return (((READ_REG32(MDC_CURSOR_X) >> 11) & 0x03F) |
            ((READ_REG32(MDC_CURSOR_Y) << 5) & 0x3F0000));
}

/*----------------------------------------------------------------------------
 * gfx_get_cursor_color
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long
gu2_get_cursor_color(int color)
#else
unsigned long
gfx_get_cursor_color(int color)
#endif
{
    if (color) {
        WRITE_REG32(MDC_PAL_ADDRESS, 0x101);
    }
    else {
        WRITE_REG32(MDC_PAL_ADDRESS, 0x100);
    }
    return READ_REG32(MDC_PAL_DATA);
}

/*----------------------------------------------------------------------------
 * gfx_get_icon_enable
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long
gu2_get_icon_enable(void)
#else
unsigned long
gfx_get_icon_enable(void)
#endif
{
    return (READ_REG32(MDC_GENERAL_CFG) & MDC_GCFG_ICNE);
}

/*----------------------------------------------------------------------------
 * gfx_get_icon_offset
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long
gu2_get_icon_offset(void)
#else
unsigned long
gfx_get_icon_offset(void)
#endif
{
    return (READ_REG32(MDC_ICON_ST_OFFSET) & 0x0FFFFFFF);
}

/*----------------------------------------------------------------------------
 * gfx_get_icon_position
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long
gu2_get_icon_position(void)
#else
unsigned long
gfx_get_icon_position(void)
#endif
{
    return (READ_REG32(MDC_ICON_X) & 0x07FF);
}

/*----------------------------------------------------------------------------
 * gfx_get_icon_color
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long
gu2_get_icon_color(int color)
#else
unsigned long
gfx_get_icon_color(int color)
#endif
{
    if (color >= 3)
        return 0;

    WRITE_REG32(MDC_PAL_ADDRESS, 0x102 + color);

    return READ_REG32(MDC_PAL_DATA);
}

/*----------------------------------------------------------------------------
 * gfx_get_compression_enable
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_get_compression_enable(void)
#else
int
gfx_get_compression_enable(void)
#endif
{
    if (READ_REG32(MDC_GENERAL_CFG) & MDC_GCFG_CMPE)
        return (1);

    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_get_compression_offset
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long
gu2_get_compression_offset(void)
#else
unsigned long
gfx_get_compression_offset(void)
#endif
{
    return (READ_REG32(MDC_CB_ST_OFFSET) & 0x007FFFFF);
}

/*----------------------------------------------------------------------------
 * gfx_get_compression_pitch
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu2_get_compression_pitch(void)
#else
unsigned short
gfx_get_compression_pitch(void)
#endif
{
    unsigned short pitch;

    pitch = (unsigned short) (READ_REG32(MDC_GFX_PITCH) >> 16);
    return (pitch << 3);
}

/*----------------------------------------------------------------------------
 * gfx_get_compression_size
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu2_get_compression_size(void)
#else
unsigned short
gfx_get_compression_size(void)
#endif
{
    unsigned short size;

    size = (unsigned short) ((READ_REG32(MDC_LINE_SIZE) >> 16) & 0x7F) - 1;
    return ((size << 3) + 32);
}

/*----------------------------------------------------------------------------
 * gfx_get_valid_bit
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_get_valid_bit(int line)
#else
int
gfx_get_valid_bit(int line)
#endif
{
    unsigned long offset;
    int valid;

    offset = READ_REG32(MDC_PHY_MEM_OFFSET) & 0xFF000000;
    offset |= line;

    WRITE_REG32(MDC_PHY_MEM_OFFSET, offset);
    valid = (int) READ_REG32(MDC_DV_ACC) & 2;

    if (valid)
        return 1;
    return 0;
}

/*---------------------------------------------------------------------------
 * gfx_get_display_video_offset (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine is called by "gfx_get_video_offset".  It abstracts the 
 * version of the display controller from the video overlay routines.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long
gu2_get_display_video_offset(void)
#else
unsigned long
gfx_get_display_video_offset(void)
#endif
{
    return (READ_REG32(MDC_VID_Y_ST_OFFSET) & 0x0FFFFFFF);
}

/*---------------------------------------------------------------------------
 * gfx_get_display_video_yuv_offsets (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine is called by "gfx_get_video_yuv_offsets".  It abstracts the 
 * version of the display controller from the video overlay routines.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu2_get_display_video_yuv_offsets(unsigned long *yoffset,
                                  unsigned long *uoffset,
                                  unsigned long *voffset)
#else
void
gfx_get_display_video_yuv_offsets(unsigned long *yoffset,
                                  unsigned long *uoffset,
                                  unsigned long *voffset)
#endif
{
    *yoffset = (READ_REG32(MDC_VID_Y_ST_OFFSET) & 0x0FFFFFFF);
    *uoffset = (READ_REG32(MDC_VID_U_ST_OFFSET) & 0x0FFFFFFF);
    *voffset = (READ_REG32(MDC_VID_V_ST_OFFSET) & 0x0FFFFFFF);
}

/*---------------------------------------------------------------------------
 * gfx_get_display_video_yuv_pitch (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine is called by "gfx_get_video_yuv_pitch".  It abstracts the 
 * version of the display controller from the video overlay routines.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu2_get_display_video_yuv_pitch(unsigned long *ypitch, unsigned long *uvpitch)
#else
void
gfx_get_display_video_yuv_pitch(unsigned long *ypitch, unsigned long *uvpitch)
#endif
{
    unsigned long pitch = READ_REG32(MDC_VID_YUV_PITCH);

    *ypitch = ((pitch & 0xFFFF) << 3);
    *uvpitch = (pitch >> 13) & 0x7FFF8;
}

/*---------------------------------------------------------------------------
 * gfx_get_display_video_downscale_delta (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine is called by "gfx_get_video_downscale_delta". It abstracts the
 * version of the display controller from the video overlay routines.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long
gu2_get_display_video_downscale_delta(void)
#else
unsigned long
gfx_get_display_video_downscale_delta(void)
#endif
{
    return (READ_REG32(MDC_VID_DS_DELTA) >> 18);
}

/*---------------------------------------------------------------------------
 * gfx_get_display_video_downscale_enable (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine is called by "gfx_get_video_vertical_downscale_enable". 
 * It abstracts the version of the display controller from the video overlay
 * routines.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu2_get_display_video_downscale_enable(void)
#else
int
gfx_get_display_video_downscale_enable(void)
#endif
{
    return ((int) ((READ_REG32(MDC_GENERAL_CFG) >> 19) & 1));
}

/*---------------------------------------------------------------------------
 * gfx_get_display_video_size (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine is called by "gfx_get_video_size".  It abstracts the 
 * version of the display controller from the video overlay routines.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long
gu2_get_display_video_size(void)
#else
unsigned long
gfx_get_display_video_size(void)
#endif
{
    /* RETURN THE LINE SIZE, AS THIS IS ALL THAT IS AVAILABLE */

    return ((READ_REG32(MDC_LINE_SIZE) >> 21) & 0x000007FF);
}

#endif                          /* GFX_READ_ROUTINES */

/* END OF FILE */
