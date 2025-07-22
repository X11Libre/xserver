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

void gu1_enable_compression(void);      /* private routine definition */
void gu1_disable_compression(void);     /* private routine definition */
int gu1_set_specified_mode(DISPLAYMODE * pMode, int bpp);

/* VIDEO BUFFER SIZE */

unsigned long vid_buf_size = 0;
int vid_enabled = 0;

/*----------------------------------------------------------------------------
 * GU1_DELAY_APPROXIMATE (PRIVATE ROUTINE - NOT PART OF DURANGO API)
 *
 * Delay the requested number of milliseconds by reading a register.  This 
 * function generally takes longer than the requested time.
 *----------------------------------------------------------------------------
 */

#define READS_PER_MILLISECOND 60000L

void
gu1_delay_approximate(unsigned long milliseconds)
{
    /* ASSUME 300 MHz, 5 CLOCKS PER READ */

    unsigned long loop;

    loop = milliseconds * READS_PER_MILLISECOND;
    while (loop-- > 0) {
        READ_REG32(DC_UNLOCK);
    }
}

/*----------------------------------------------------------------------------
 * GU1_DELAY_PRECISE (PRIVATE ROUTINE - NOT PART OF DURANGO API)
 *
 * Delay the number of milliseconds on a more precise level, varying only by 
 * 1/10 of a ms.  This function should only be called if an SC1200 is present.
 *----------------------------------------------------------------------------
 */
void
gu1_delay_precise(unsigned long milliseconds)
{
#if GFX_VIDEO_SC1200

#define LOOP 1000
    unsigned long i, timer_start, timer_end, total_ticks, previous_ticks,
        temp_ticks;

    /* Get current time */
    timer_start = IND(SC1200_CB_BASE_ADDR + SC1200_CB_TMVALUE);

    /* Calculate expected end time */
    if (INB(SC1200_CB_BASE_ADDR + SC1200_CB_TMCNFG) & SC1200_TMCLKSEL_27MHZ)
        total_ticks = 27000 * milliseconds;     /* timer resolution is 27 MHz */
    else
        total_ticks = 1000 * milliseconds;      /* timer resolution is 1 MHz */

    if (total_ticks > ((unsigned long) 0xffffffff - timer_start))
        /* wrap-around */
        timer_end = total_ticks - ((unsigned long) 0xffffffff - timer_start);
    else
        timer_end = timer_start + total_ticks;

    /* in case of wrap around */
    if (timer_end < timer_start) {
        previous_ticks = timer_start;
        while (1) {
            temp_ticks = IND(SC1200_CB_BASE_ADDR + SC1200_CB_TMVALUE);
            if (temp_ticks < previous_ticks)
                break;
            else
                previous_ticks = temp_ticks;
            for (i = 0; i < LOOP; i++)
                READ_REG32(DC_UNLOCK);
        }
    }

    /* now the non-wrap around part */
    while (1) {
        for (i = 0; i < LOOP; i++)
            READ_REG32(DC_UNLOCK);
        if (IND(SC1200_CB_BASE_ADDR + SC1200_CB_TMVALUE) > timer_end)
            break;
    }
#endif                          /* GFX_VIDEO_SC1200 */
}

/*----------------------------------------------------------------------------
 * WARNING!!!! INACCURATE DELAY MECHANISM
 *
 * In an effort to keep the code self contained and operating system 
 * independent, the delay loop just performs reads of a display controller
 * register.  This time will vary for faster processors.  The delay can always
 * be longer than intended, only effecting the time of the mode switch 
 * (obviously want it to still be under a second).  Problems with the hardware
 * only arise if the delay is not long enough.
 *
 * For the SC1200, the high resolution timer can be used as an accurate
 * mechanism for keeping time. However, in order to avoid a busy loop of IO
 * reads, the timer is polled in-between busy loops, and therefore the actual
 * delay might be longer than the requested delay by the time of one busy loop
 * (which on a 200 MHz system took 95 us)
 *
 * There are thus two delay functions which are called from the main API
 * routine.
 * One is meant to be more precise and should only called if an SC1200 is 
 * present.
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu1_delay_milliseconds(unsigned long milliseconds)
#else
void
gfx_delay_milliseconds(unsigned long milliseconds)
#endif
{
#if GFX_VIDEO_SC1200
#if GFX_VIDEO_DYNAMIC
    if (gfx_video_type == GFX_VIDEO_TYPE_SC1200) {
#endif
        gu1_delay_precise(milliseconds);
        return;
#if GFX_VIDEO_DYNAMIC
    }
#endif
#endif                          /* GFX_VIDEO_SC1200 */
    gu1_delay_approximate(milliseconds);
}

#if GFX_DISPLAY_DYNAMIC
void
gu1_delay_microseconds(unsigned long microseconds)
#else
void
gfx_delay_microseconds(unsigned long microseconds)
#endif
{
    /* ASSUME 300 MHz, 2 CLOCKS PER INCREMENT */
    unsigned long loop_count = microseconds * 150;

    while (loop_count-- > 0) {
        ;
    }
}

/*----------------------------------------------------------------------------
 * GFX_VIDEO_SHUTDOWN
 *
 * This routine disables the display controller output.
 *----------------------------------------------------------------------------
 */
void
gu1_video_shutdown(void)
{
    unsigned long unlock;
    unsigned long gcfg, tcfg;

    /* DISABLE COMPRESSION */
    gu1_disable_compression();

    /* ALSO DISABLE VIDEO */
    /* Use private "reset video" routine to do all that is needed. */
    /* SC1200, for example, also disables the alpha blending regions. */
    gfx_reset_video();

    /* UNLOCK THE DISPLAY CONTROLLER REGISTERS */
    unlock = READ_REG32(DC_UNLOCK);
    WRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);

    /* READ THE CURRENT GX VALUES */
    gcfg = READ_REG32(DC_GENERAL_CFG);
    tcfg = READ_REG32(DC_TIMING_CFG);

    /* BLANK THE GX DISPLAY AND DISABLE THE TIMING GENERATOR */
    tcfg &= ~((unsigned long) DC_TCFG_BLKE | (unsigned long) DC_TCFG_TGEN);
    WRITE_REG32(DC_TIMING_CFG, tcfg);

    /* DELAY: WAIT FOR PENDING MEMORY REQUESTS */
    /* This delay is used to make sure that all pending requests to the */
    /* memory controller have completed before disabling the FIFO load. */
    gfx_delay_milliseconds(1);

    /* DISABLE DISPLAY FIFO LOAD AND DISABLE COMPRESSION */
    gcfg &= ~(unsigned long) (DC_GCFG_DFLE | DC_GCFG_CMPE | DC_GCFG_DECE);
    WRITE_REG32(DC_GENERAL_CFG, gcfg);
    WRITE_REG32(DC_UNLOCK, unlock);
    return;
}

/*----------------------------------------------------------------------------
 * GFX_SET_DISPLAY_BPP
 *
 * This routine programs the bpp in the display controller.
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu1_set_display_bpp(unsigned short bpp)
#else
int
gfx_set_display_bpp(unsigned short bpp)
#endif
{
    unsigned long ocfg, lock;

    lock = READ_REG32(DC_UNLOCK);
    ocfg = READ_REG32(DC_OUTPUT_CFG) & ~(DC_OCFG_8BPP | DC_OCFG_555);

    /* SET DC PIXEL FORMAT */
    if (bpp == 8)
        ocfg |= DC_OCFG_8BPP;
    else if (bpp == 15)
        ocfg |= DC_OCFG_555;
    else if (bpp != 16)
        return GFX_STATUS_BAD_PARAMETER;

    WRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);
    WRITE_REG32(DC_OUTPUT_CFG, ocfg);
    WRITE_REG32(DC_UNLOCK, lock);

    /* SET BPP IN GRAPHICS PIPELINE */
    gfx_set_bpp(bpp);

    return 0;
}

/*----------------------------------------------------------------------------
 * GFX_SET_SPECIFIED_MODE
 * This routine uses the parameters in the specified display mode structure
 * to program the display controller hardware.  
 *----------------------------------------------------------------------------
 */
int
gu1_set_specified_mode(DISPLAYMODE *pMode, int bpp)
{
    unsigned long unlock, value;
    unsigned long gcfg, tcfg, ocfg;
    unsigned long size, pitch;
    unsigned long hactive, vactive;

    gbpp = bpp;

    /* CHECK WHETHER TIMING CHANGE IS ALLOWED */
    /* Flag used for locking also overrides timing change restriction */
    if (gfx_timing_lock && !(pMode->flags & GFX_MODE_LOCK_TIMING))
        return GFX_STATUS_ERROR;

    /* SET GLOBAL FLAG */
    if (pMode->flags & GFX_MODE_LOCK_TIMING)
        gfx_timing_lock = 1;

    /* DISABLE COMPRESSION */
    gu1_disable_compression();

    /* ALSO DISABLE VIDEO */
    /* Use private "reset video" routine to do all that is needed. */
    /* SC1200, for example, also disables the alpha blending regions. */
    gfx_reset_video();

    /* UNLOCK THE DISPLAY CONTROLLER REGISTERS */
    unlock = READ_REG32(DC_UNLOCK);
    WRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);

    /* READ THE CURRENT GX VALUES */
    gcfg = READ_REG32(DC_GENERAL_CFG);
    tcfg = READ_REG32(DC_TIMING_CFG);

    /* BLANK THE GX DISPLAY AND DISABLE THE TIMING GENERATOR */
    tcfg &= ~((unsigned long) DC_TCFG_BLKE | (unsigned long) DC_TCFG_TGEN);
    WRITE_REG32(DC_TIMING_CFG, tcfg);

    /* DELAY: WAIT FOR PENDING MEMORY REQUESTS 
     * This delay is used to make sure that all pending requests to the 
     * memory controller have completed before disabling the FIFO load.
     */
    gfx_delay_milliseconds(1);

    /* DISABLE DISPLAY FIFO LOAD AND DISABLE COMPRESSION */
    gcfg &= ~(unsigned long) (DC_GCFG_DFLE | DC_GCFG_CMPE | DC_GCFG_DECE);
    WRITE_REG32(DC_GENERAL_CFG, gcfg);

    /* CLEAR THE "DCLK_MUL" FIELD */
    gcfg &= ~(unsigned long) (DC_GCFG_DDCK | DC_GCFG_DPCK | DC_GCFG_DFCK);
    gcfg &= ~(unsigned long) DC_GCFG_DCLK_MASK;
    WRITE_REG32(DC_GENERAL_CFG, gcfg);

    /* SET THE DOT CLOCK FREQUENCY */
    /* Mask off the divide by two bit (bit 31) */
    gfx_set_clock_frequency(pMode->frequency & 0x7FFFFFFF);

    /* DELAY: WAIT FOR THE PLL TO SETTLE */
    /* This allows the dot clock frequency that was just set to settle. */
    gfx_delay_milliseconds(1);

    /* SET THE "DCLK_MUL" FIELD OF DC_GENERAL_CFG */
    /* The GX hardware divides the dot clock, so 2x really means that the */
    /* internal dot clock equals the external dot clock. */
    if (pMode->frequency & 0x80000000)
        gcfg |= 0x0040;
    else
        gcfg |= 0x0080;

    WRITE_REG32(DC_GENERAL_CFG, gcfg);

    /* DELAY: WAIT FOR THE ADL TO LOCK */
    /* This allows the clock generatation within GX to settle.  This is */
    /* needed since some of the register writes that follow require that */
    /* clock to be present. */
    gfx_delay_milliseconds(1);

    /* SET THE GX DISPLAY CONTROLLER PARAMETERS */
    WRITE_REG32(DC_FB_ST_OFFSET, 0);
    WRITE_REG32(DC_CB_ST_OFFSET, 0);
    WRITE_REG32(DC_CURS_ST_OFFSET, 0);

    /* SET LINE SIZE AND PITCH */
    /* Flat panels use the current flat panel line size to    */
    /* calculate the pitch, but load the true line size       */
    /* for the mode into the "Frame Buffer Line Size" field   */
    /* of DC_BUF_SIZE.                                        */
    if (PanelEnable)
        size = ModeWidth;
    else
        size = pMode->hactive;

    if (bpp > 8)
        size <<= 1;

    /* ONLY PYRAMID SUPPORTS 4K LINE SIZE */
    if (size <= 1024) {
        pitch = 1024;
        /* SPECIAL CASE  */
        /* Graphics acceleration in 16-bit pixel line double modes */
        /* requires a pitch of 2048.                               */
        if ((pMode->flags & GFX_MODE_LINE_DOUBLE) && bpp > 8)
            pitch <<= 1;
    }
    else {
        if (gfx_cpu_version == GFX_CPU_PYRAMID)
            pitch = (size <= 2048) ? 2048 : 4096;
        else
            pitch = 2048;
    }

    WRITE_REG32(DC_LINE_DELTA, pitch >> 2);

    if (PanelEnable) {
        size = pMode->hactive;
        if (bpp > 8)
            size <<= 1;
    }

    /* ADD 2 TO SIZE FOR POSSIBLE START ADDRESS ALIGNMENTS */
    WRITE_REG32(DC_BUF_SIZE, (size >> 3) + 2);

    /* ALWAYS ENABLE "PANEL" DATA FROM MEDIAGX */
    /* That is really just the 18 BPP data bus to the companion chip */
    ocfg = DC_OCFG_PCKE | DC_OCFG_PDEL | DC_OCFG_PDEH;

    /* SET PIXEL FORMAT */
    if (bpp == 8)
        ocfg |= DC_OCFG_8BPP;
    else if (bpp == 15)
        ocfg |= DC_OCFG_555;

    /* ENABLE TIMING GENERATOR, SYNCS, AND FP DATA */
    tcfg = DC_TCFG_FPPE | DC_TCFG_HSYE | DC_TCFG_VSYE | DC_TCFG_BLKE |
        DC_TCFG_TGEN;

    /* SET FIFO PRIORITY, DCLK MULTIPLIER, AND FIFO ENABLE */
    /* Default 6/5 for FIFO, 2x for DCLK multiplier. */
    gcfg = (6 << DC_GCFG_DFHPEL_POS) | (5 << DC_GCFG_DFHPSL_POS) | DC_GCFG_DFLE;

    /* INCREASE FIFO PRIORITY FOR LARGE MODES */
    if (pMode->hactive == 1280 && pMode->vactive == 1024) {
        if ((bpp == 8) && (pMode->flags & GFX_MODE_85HZ))
            gcfg =
                (8l << DC_GCFG_DFHPEL_POS) | (7l << DC_GCFG_DFHPSL_POS) |
                DC_GCFG_DFLE;

        if ((bpp > 8) && (pMode->flags & GFX_MODE_75HZ))
            gcfg =
                (7l << DC_GCFG_DFHPEL_POS) | (6l << DC_GCFG_DFHPSL_POS) |
                DC_GCFG_DFLE;

        if ((bpp > 8) && (pMode->flags & GFX_MODE_85HZ))
            gcfg =
                (9l << DC_GCFG_DFHPEL_POS) | (8l << DC_GCFG_DFHPSL_POS) |
                DC_GCFG_DFLE;
    }

    /* SET DOT CLOCK MULTIPLIER */
    /* Bit 31 of frequency indicates divide frequency by two */
    if (pMode->frequency & 0x80000000)
        gcfg |= (1l << DC_GCFG_DCLK_POS);
    else
        gcfg |= (2l << DC_GCFG_DCLK_POS);

    /* DIVIDE VIDEO CLOCK */
    /* CPU core frequencies above 266 MHz will divide the video */
    /* clock by 4 to ensure that we are running below 150 MHz.  */
    if (gfx_cpu_frequency > 266)
        gcfg |= DC_GCFG_VCLK_DIV;

    /* ALWAYS ENABLE VIDEO IN THE DISPLAY CONTROLLER */
    /* Enabling video at an inopportune momemt can corrupt the DC fetch */
    /* engine and cause screen artifacts or system hang.                */
    gcfg |= (DC_GCFG_VIDE | DC_GCFG_VRDY);

    /* SET THE PIXEL AND LINE DOUBLE BITS IF NECESSARY */
    hactive = pMode->hactive;
    vactive = pMode->vactive;
    gfx_line_double = 0;
    gfx_pixel_double = 0;

    if (pMode->flags & GFX_MODE_LINE_DOUBLE) {
        gcfg |= DC_GCFG_LDBL;
        hactive <<= 1;

        /* SET GLOBAL FLAG */
        gfx_line_double = 1;
    }

    if (pMode->flags & GFX_MODE_PIXEL_DOUBLE) {
        tcfg |= DC_TCFG_PXDB;
        vactive <<= 1;

        /* SET GLOBAL FLAG */
        gfx_pixel_double = 1;
    }

    /* COMBINE AND SET TIMING VALUES */

    value = (unsigned long) (hactive - 1) |
        (((unsigned long) (pMode->htotal - 1)) << 16);
    WRITE_REG32(DC_H_TIMING_1, value);
    value = (unsigned long) (pMode->hblankstart - 1) |
        (((unsigned long) (pMode->hblankend - 1)) << 16);
    WRITE_REG32(DC_H_TIMING_2, value);
    value = (unsigned long) (pMode->hsyncstart - 1) |
        (((unsigned long) (pMode->hsyncend - 1)) << 16);
    WRITE_REG32(DC_H_TIMING_3, value);
    WRITE_REG32(DC_FP_H_TIMING, value);
    value = (unsigned long) (vactive - 1) |
        (((unsigned long) (pMode->vtotal - 1)) << 16);
    WRITE_REG32(DC_V_TIMING_1, value);
    value = (unsigned long) (pMode->vblankstart - 1) |
        (((unsigned long) (pMode->vblankend - 1)) << 16);
    WRITE_REG32(DC_V_TIMING_2, value);
    value = (unsigned long) (pMode->vsyncstart - 1) |
        (((unsigned long) (pMode->vsyncend - 1)) << 16);
    WRITE_REG32(DC_V_TIMING_3, value);
    value = (unsigned long) (pMode->vsyncstart - 2) |
        (((unsigned long) (pMode->vsyncend - 2)) << 16);
    WRITE_REG32(DC_FP_V_TIMING, value);

    WRITE_REG32(DC_OUTPUT_CFG, ocfg);
    WRITE_REG32(DC_TIMING_CFG, tcfg);
    gfx_delay_milliseconds(1);  /* delay after TIMING_CFG */
    WRITE_REG32(DC_GENERAL_CFG, gcfg);

    /* ENABLE FLAT PANEL CENTERING */
    /* For 640x480 modes displayed with the 9211 within a 800x600 */
    /* flat panel display, turn on flat panel centering.          */
    if (PanelEnable) {
        if (ModeWidth < PanelWidth) {
            tcfg = READ_REG32(DC_TIMING_CFG);
            tcfg = tcfg | DC_TCFG_FCEN;
            WRITE_REG32(DC_TIMING_CFG, tcfg);
            gfx_delay_milliseconds(1);  /* delay after TIMING_CFG */
        }
    }

    /* CONFIGURE DISPLAY OUTPUT FROM VIDEO PROCESSOR */
    gfx_set_display_control(((pMode->flags & GFX_MODE_NEG_HSYNC) ? 1 : 0) |
                            ((pMode->flags & GFX_MODE_NEG_VSYNC) ? 2 : 0));

    /* RESTORE VALUE OF DC_UNLOCK */
    WRITE_REG32(DC_UNLOCK, unlock);

    /* ALSO WRITE GP_BLIT_STATUS FOR PITCH AND 8/18 BPP */
    /* Remember, only Pyramid supports 4K line pitch    */
    value = 0;
    if (bpp > 8)
        value |= BC_16BPP;
    if ((gfx_cpu_version == GFX_CPU_PYRAMID) && (pitch > 2048))
        value |= BC_FB_WIDTH_4096;
    else if (pitch > 1024)
        value |= BC_FB_WIDTH_2048;

    WRITE_REG16(GP_BLIT_STATUS, (unsigned short) value);

    return GFX_STATUS_OK;
}                               /* end gfx_set_specified_mode() */

/*----------------------------------------------------------------------------
 * GFX_IS_DISPLAY_MODE_SUPPORTED
 *
 * This routine sets the specified display mode.
 *
 * Returns the index of the mode if successful and mode returned, -1 if the 
 * mode could not be found.
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu1_is_display_mode_supported(int xres, int yres, int bpp, int hz)
#else
int
gfx_is_display_mode_supported(int xres, int yres, int bpp, int hz)
#endif
{
    unsigned int mode = 0;
    unsigned long hz_flag = 0, bpp_flag = 0;

    /* SET FLAGS TO MATCH REFRESH RATE */
    if (hz == 56)
        hz_flag = GFX_MODE_56HZ;
    else if (hz == 60)
        hz_flag = GFX_MODE_60HZ;
    else if (hz == 70)
        hz_flag = GFX_MODE_70HZ;
    else if (hz == 72)
        hz_flag = GFX_MODE_72HZ;
    else if (hz == 75)
        hz_flag = GFX_MODE_75HZ;
    else if (hz == 85)
        hz_flag = GFX_MODE_85HZ;
    else
        return -1;

    /* SET BPP FLAGS TO LIMIT MODE SELECTION */
    if (bpp == 8)
        bpp_flag = GFX_MODE_8BPP;
    else if (bpp == 15)
        bpp_flag = GFX_MODE_15BPP;
    else if (bpp == 16)
        bpp_flag = GFX_MODE_16BPP;
    else
        return -1;

    /* ONLY PYRAMID SUPPORTS 4K PITCH */
    if (gfx_cpu_version != GFX_CPU_PYRAMID && xres > 1024) {
        if (bpp > 8)
            return (-1);        /* return with mode not found */
    }

    /* LOOP THROUGH THE AVAILABLE MODES TO FIND A MATCH */
    for (mode = 0; mode < NUM_GX_DISPLAY_MODES; mode++) {
        if ((DisplayParams[mode].hactive == (unsigned short) xres) &&
            (DisplayParams[mode].vactive == (unsigned short) yres) &&
            (DisplayParams[mode].flags & hz_flag) &&
            (DisplayParams[mode].flags & bpp_flag)) {

            /* SET THE DISPLAY CONTROLLER FOR THE SELECTED MODE */
            return (mode);
        }
    }
    return (-1);
}

/*----------------------------------------------------------------------------
 * GFX_SET_DISPLAY_MODE
 *
 * This routine sets the specified display mode.
 *
 * Returns 1 if successful, 0 if mode could not be set.
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu1_set_display_mode(int xres, int yres, int bpp, int hz)
#else
int
gfx_set_display_mode(int xres, int yres, int bpp, int hz)
#endif
{
    int mode;

    /* DISABLE FLAT PANEL */
    /* Flat Panel settings are enabled by the function gfx_set_fixed_timings
     * and disabled by gfx_set_display_mode. */
    PanelEnable = 0;

    mode = gfx_is_display_mode_supported(xres, yres, bpp, hz);
    if (mode >= 0) {
        if (gu1_set_specified_mode(&DisplayParams[mode], bpp) == GFX_STATUS_OK)
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
 * Returns GFX_STATUS_OK on success, GFX_STATUS_ERROR otherwise.
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu1_set_display_timings(unsigned short bpp, unsigned short flags,
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
    return (gu1_set_specified_mode(&gfx_display_mode, bpp));
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
gu1_set_vtotal(unsigned short vtotal)
#else
int
gfx_set_vtotal(unsigned short vtotal)
#endif
{
    unsigned long unlock, tcfg, timing1, timing2;

    /* UNLOCK THE DISPLAY CONTROLLER REGISTERS */
    unlock = READ_REG32(DC_UNLOCK);
    WRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);

    /* READ THE CURRENT GX VALUES */
    tcfg = READ_REG32(DC_TIMING_CFG);
    timing1 = READ_REG32(DC_V_TIMING_1);
    timing2 = READ_REG32(DC_V_TIMING_2);

    /* DISABLE THE TIMING GENERATOR */
    WRITE_REG32(DC_TIMING_CFG, tcfg & ~(unsigned long) DC_TCFG_TGEN);

    /* WRITE NEW TIMING VALUES */
    WRITE_REG32(DC_V_TIMING_1,
                (timing1 & 0xffff) | (unsigned long) (vtotal - 1) << 16);
    WRITE_REG32(DC_V_TIMING_2,
                (timing2 & 0xffff) | (unsigned long) (vtotal - 1) << 16);

    /* RESTORE GX VALUES */
    WRITE_REG32(DC_TIMING_CFG, tcfg);
    WRITE_REG32(DC_UNLOCK, unlock);

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
gu1_set_display_pitch(unsigned short pitch)
#else
void
gfx_set_display_pitch(unsigned short pitch)
#endif
{
    unsigned long value = 0;
    unsigned long lock = READ_REG32(DC_UNLOCK);

    value = READ_REG32(DC_LINE_DELTA) & 0xFFFFF000;
    value |= (pitch >> 2);
    WRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);
    WRITE_REG32(DC_LINE_DELTA, value);
    WRITE_REG32(DC_UNLOCK, lock);

    /* ALSO UPDATE PITCH IN GRAPHICS ENGINE */
    /* Pyramid alone supports 4K line pitch */
    value = (unsigned long) READ_REG16(GP_BLIT_STATUS);
    value &= ~(BC_FB_WIDTH_2048 | BC_FB_WIDTH_4096);

    if ((gfx_cpu_version == GFX_CPU_PYRAMID) && (pitch > 2048))
        value |= BC_FB_WIDTH_4096;

    else if (pitch > 1024)
        value |= BC_FB_WIDTH_2048;

    WRITE_REG16(GP_BLIT_STATUS, (unsigned short) value);
    return;
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
gu1_set_display_offset(unsigned long offset)
#else
void
gfx_set_display_offset(unsigned long offset)
#endif
{
    /* UPDATE FRAME BUFFER OFFSET */

    unsigned long lock;

    lock = READ_REG32(DC_UNLOCK);
    WRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);

    /* START ADDRESS EFFECTS DISPLAY COMPRESSION */
    /* Disable compression for non-zero start address values.              */
    /* Enable compression if offset is zero and compression is intended to */
    /* be enabled from a previous call to "gfx_set_compression_enable".    */
    /* Compression should be disabled BEFORE the offset is changed         */
    /* and enabled AFTER the offset is changed.                            */
    if (offset == 0) {
        WRITE_REG32(DC_FB_ST_OFFSET, offset);
        if (gfx_compression_enabled) {
            /* WAIT FOR THE OFFSET TO BE LATCHED */
            gfx_wait_vertical_blank();
            gu1_enable_compression();
        }
    }
    else {
        /* ONLY DISABLE COMPRESSION ONCE */
        if (gfx_compression_active)
            gu1_disable_compression();

        WRITE_REG32(DC_FB_ST_OFFSET, offset);
    }

    WRITE_REG32(DC_UNLOCK, lock);
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
gu1_set_display_palette_entry(unsigned long index, unsigned long palette)
#else
int
gfx_set_display_palette_entry(unsigned long index, unsigned long palette)
#endif
{
    unsigned long data;

    if (index > 0xFF)
        return GFX_STATUS_BAD_PARAMETER;

    WRITE_REG32(DC_PAL_ADDRESS, index);
    data = ((palette >> 2) & 0x0003F) |
        ((palette >> 4) & 0x00FC0) | ((palette >> 6) & 0x3F000);
    WRITE_REG32(DC_PAL_DATA, data);

    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_display_palette
 *
 * This routine sets the entire palette in the display controller.
 * A pointer is provided to a 256 entry table of 32-bit X:R:G:B values.
 *
 * Restriction:
 * Due to SC1200 Issue #748 (in Notes DB) this function should be called only
 * when DCLK is active, i.e PLL is already powered up and genlock is not 
 * active.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu1_set_display_palette(unsigned long *palette)
#else
int
gfx_set_display_palette(unsigned long *palette)
#endif
{
    unsigned long data, i;

    WRITE_REG32(DC_PAL_ADDRESS, 0);
    if (palette) {
        for (i = 0; i < 256; i++) {
            /* CONVERT 24 BPP COLOR DATA TO 18 BPP COLOR DATA */
            data = ((palette[i] >> 2) & 0x0003F) |
                ((palette[i] >> 4) & 0x00FC0) | ((palette[i] >> 6) & 0x3F000);
            WRITE_REG32(DC_PAL_DATA, data);
        }
    }
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_cursor_enable
 *
 * This routine enables or disables the hardware cursor.  
 *
 * WARNING: The cusrsor start offset must be set by setting the cursor 
 * position before calling this routine to assure that memory reads do not
 * go past the end of graphics memory (this can hang GXm).
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu1_set_cursor_enable(int enable)
#else
void
gfx_set_cursor_enable(int enable)
#endif
{
    unsigned long unlock, gcfg;

    /* SET OR CLEAR CURSOR ENABLE BIT */
    unlock = READ_REG32(DC_UNLOCK);
    gcfg = READ_REG32(DC_GENERAL_CFG);
    if (enable)
        gcfg |= DC_GCFG_CURE;
    else
        gcfg &= ~(DC_GCFG_CURE);

    /* WRITE NEW REGISTER VALUE */
    WRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);
    WRITE_REG32(DC_GENERAL_CFG, gcfg);
    WRITE_REG32(DC_UNLOCK, unlock);
}

/*---------------------------------------------------------------------------
 * gfx_set_cursor_colors
 *
 * This routine sets the colors of the hardware cursor.
 *
 * Restriction:
 * Due to SC1200 Issue #748 (in Notes DB) this function should be called only
 * when DCLK is active, i.e PLL is already powered up.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu1_set_cursor_colors(unsigned long bkcolor, unsigned long fgcolor)
#else
void
gfx_set_cursor_colors(unsigned long bkcolor, unsigned long fgcolor)
#endif
{
    unsigned long value;

    /* If genlock is enabled DCLK might be disabled in vertical blank. */
    /* Due to SC1200 Issue #748 in Notes DB this would fail the cursor color settings */
    /* So Wait for vertical blank to end */

#if GFX_VIDEO_SC1200
    if (gfx_test_timing_active())
        while ((gfx_get_vline()) > gfx_get_vactive());
#endif
    /* SET CURSOR COLORS */
    WRITE_REG32(DC_PAL_ADDRESS, 0x100);
    value = ((bkcolor & 0x000000FC) >> 2) |
        ((bkcolor & 0x0000FC00) >> (2 + 8 - 6)) |
        ((bkcolor & 0x00FC0000) >> (2 + 16 - 12));
    WRITE_REG32(DC_PAL_DATA, value);
    value = ((fgcolor & 0x000000FC) >> 2) |
        ((fgcolor & 0x0000FC00) >> (2 + 8 - 6)) |
        ((fgcolor & 0x00FC0000) >> (2 + 16 - 12));
    WRITE_REG32(DC_PAL_DATA, value);
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
gu1_set_cursor_position(unsigned long memoffset,
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

    short x, y;
    short xoffset = 0;
    short yoffset = 0;

    /* SUPPORT CURSOR IN EMULATED VGA MODES */
    /* Timings are for twice the resolution */
    if (gfx_pixel_double)
        xpos <<= 1;

    if (gfx_line_double)
        ypos <<= 1;

    x = (short) xpos - (short) xhotspot;
    y = (short) ypos - (short) yhotspot;
    if (x < -31)
        return;

    if (y < -31)
        return;

    if (x < 0) {
        xoffset = -x;
        x = 0;
    }

    if (y < 0) {
        yoffset = -y;
        y = 0;
    }

    memoffset += (unsigned long) yoffset << 3;

    if (PanelEnable) {
        if ((ModeWidth > PanelWidth) || (ModeHeight > PanelHeight)) {
            gfx_enable_panning(xpos, ypos);
            x = x - (short) panelLeft;
            y = y - (short) panelTop;
        }
    }

    /* SET CURSOR POSITION */
    unlock = READ_REG32(DC_UNLOCK);
    WRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);
    WRITE_REG32(DC_CURS_ST_OFFSET, memoffset);
    WRITE_REG32(DC_CURSOR_X, (unsigned long) x |
                (((unsigned long) xoffset) << 11));
    WRITE_REG32(DC_CURSOR_Y, (unsigned long) y |
                (((unsigned long) yoffset) << 11));
    WRITE_REG32(DC_UNLOCK, unlock);
}

/*---------------------------------------------------------------------------
 * gfx_set_cursor_shape32
 *
 * This routine loads 32x32 cursor data into the specified location in 
 * graphics memory.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu1_set_cursor_shape32(unsigned long memoffset,
                       unsigned long *andmask, unsigned long *xormask)
#else
void
gfx_set_cursor_shape32(unsigned long memoffset,
                       unsigned long *andmask, unsigned long *xormask)
#endif
{
    int i;
    unsigned long value;

    for (i = 0; i < 32; i++) {
        /* CONVERT TO 16 BITS AND MASK, 16 BITS XOR MASK PER DWORD */
        value = (andmask[i] & 0xFFFF0000) | (xormask[i] >> 16);
        WRITE_FB32(memoffset, value);
        memoffset += 4;
        value = (andmask[i] << 16) | (xormask[i] & 0x0000FFFF);
        WRITE_FB32(memoffset, value);
        memoffset += 4;
    }
}

/*---------------------------------------------------------------------------
 * gu1_enable_compression
 *
 * This is a private routine to this module (not exposed in the Durango API).
 * It enables display compression.
 *---------------------------------------------------------------------------
 */
void
gu1_enable_compression(void)
{
    int i;
    unsigned long unlock, gcfg, offset;

    /* DO NOT ENABLE IF START ADDRESS IS NOT ZERO */
    offset = READ_REG32(DC_FB_ST_OFFSET) & 0x003FFFFF;
    if (offset != 0)
        return;

    /* DO NOT ENABLE IF WE ARE WITHIN AN EMULATED VGA MODE */
    if (gfx_line_double || gfx_pixel_double)
        return;

    /* SET GLOBAL INDICATOR */
    gfx_compression_active = 1;

    /* CLEAR DIRTY/VALID BITS IN MEMORY CONTROLLER */
    /* Software is required to do this before enabling compression. */
    /* Don't want controller to think that old lines are still valid. */
    for (i = 0; i < 1024; i++) {
        WRITE_REG32(MC_DR_ADD, i);
        WRITE_REG32(MC_DR_ACC, 0);
    }

    /* TURN ON COMPRESSION CONTROL BITS */
    unlock = READ_REG32(DC_UNLOCK);
    gcfg = READ_REG32(DC_GENERAL_CFG);
    gcfg |= DC_GCFG_CMPE | DC_GCFG_DECE;
    WRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);
    WRITE_REG32(DC_GENERAL_CFG, gcfg);
    WRITE_REG32(DC_UNLOCK, unlock);
}

/*---------------------------------------------------------------------------
 * gu1_disable_compression
 *
 * This is a private routine to this module (not exposed in the Durango API).
 * It disables display compression.
 *---------------------------------------------------------------------------
 */
void
gu1_disable_compression(void)
{
    unsigned long unlock, gcfg;

    /* SET GLOBAL INDICATOR */
    gfx_compression_active = 0;

    /* TURN OFF COMPRESSION CONTROL BITS */
    unlock = READ_REG32(DC_UNLOCK);
    gcfg = READ_REG32(DC_GENERAL_CFG);
    gcfg &= ~(DC_GCFG_CMPE | DC_GCFG_DECE);
    WRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);
    WRITE_REG32(DC_GENERAL_CFG, gcfg);
    WRITE_REG32(DC_UNLOCK, unlock);
}

/*---------------------------------------------------------------------------
 * gfx_set_compression_enable
 *
 * This routine enables or disables display compression.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu1_set_compression_enable(int enable)
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
        gu1_enable_compression();
    else
        gu1_disable_compression();

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
gu1_set_compression_offset(unsigned long offset)
#else
int
gfx_set_compression_offset(unsigned long offset)
#endif
{
    unsigned long lock;

    /* MUST BE 16-BYTE ALIGNED FOR GXLV */
    if (offset & 0x0F)
        return (1);

    /* SET REGISTER VALUE */
    lock = READ_REG32(DC_UNLOCK);
    WRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);
    WRITE_REG32(DC_CB_ST_OFFSET, offset);
    WRITE_REG32(DC_UNLOCK, lock);
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
gu1_set_compression_pitch(unsigned short pitch)
#else
int
gfx_set_compression_pitch(unsigned short pitch)
#endif
{
    unsigned long lock, line_delta;

    /* SET REGISTER VALUE */
    lock = READ_REG32(DC_UNLOCK);
    line_delta = READ_REG32(DC_LINE_DELTA) & 0xFF800FFF;
    line_delta |= ((unsigned long) pitch << 10l) & 0x007FF000;
    WRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);
    WRITE_REG32(DC_LINE_DELTA, line_delta);
    WRITE_REG32(DC_UNLOCK, lock);
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
gu1_set_compression_size(unsigned short size)
#else
int
gfx_set_compression_size(unsigned short size)
#endif
{
    unsigned long lock, buf_size;

    /* SUBTRACT 16 FROM SIZE                          */
    /* The display controller will actually write     */
    /* 2 extra QWords.  So, if we assume that "size"  */
    /* refers to the allocated size, we must subtract */
    /* 16 bytes.                                      */
    size -= 16;

    /* SET REGISTER VALUE */
    lock = READ_REG32(DC_UNLOCK);
    buf_size = READ_REG32(DC_BUF_SIZE) & 0xFFFF01FF;
    buf_size |= (((size >> 2) + 1) & 0x7F) << 9;
    WRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);
    WRITE_REG32(DC_BUF_SIZE, buf_size);
    WRITE_REG32(DC_UNLOCK, lock);
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_set_display_video_enable (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine enables/disables video on GX.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu1_set_display_video_enable(int enable)
#else
void
gfx_set_display_video_enable(int enable)
#endif
{
    unsigned long lock, gcfg, buf_size;

    lock = READ_REG32(DC_UNLOCK);
    gcfg = READ_REG32(DC_GENERAL_CFG);
    buf_size = READ_REG32(DC_BUF_SIZE);

    WRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);

    vid_enabled = enable;

    /* SET THE BUFFER SIZE TO A NON-ZERO VALUE ONLY WHEN */
    /* ENABLING VIDEO                                    */
    if (enable) {
        gcfg |= (DC_GCFG_VIDE | DC_GCFG_VRDY);
        WRITE_REG32(DC_GENERAL_CFG, gcfg);

        WRITE_REG32(DC_BUF_SIZE, (buf_size & 0x0000FFFFl) | vid_buf_size);
    }
    /* CLEAR THE VIDEO BUFFER SIZE WHEN DISABLING VIDEO  */
    else {
        gcfg &= ~(DC_GCFG_VIDE);
        WRITE_REG32(DC_GENERAL_CFG, gcfg);

        vid_buf_size = buf_size & 0xFFFF0000l;
        WRITE_REG32(DC_BUF_SIZE, buf_size & 0x0000FFFFl);
    }

    WRITE_REG32(DC_UNLOCK, lock);
    return;
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
gu1_set_display_video_size(unsigned short width, unsigned short height)
#else
void
gfx_set_display_video_size(unsigned short width, unsigned short height)
#endif
{
    unsigned long lock, size, value;

    size = (unsigned long) (width << 1) * (unsigned long) height;

    /* STORE THE VIDEO BUFFER SIZE AS A GLOBAL */
    vid_buf_size = ((size + 63) >> 6) << 16;

    lock = READ_REG32(DC_UNLOCK);
    WRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);
    value = READ_REG32(DC_BUF_SIZE) & 0x0000FFFF;
    value |= vid_buf_size;
    WRITE_REG32(DC_BUF_SIZE, value);
    WRITE_REG32(DC_UNLOCK, lock);
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
gu1_set_display_video_offset(unsigned long offset)
#else
void
gfx_set_display_video_offset(unsigned long offset)
#endif
{
    unsigned long lock;

    lock = READ_REG32(DC_UNLOCK);
    WRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);
    offset &= 0x003FFFFF;
    WRITE_REG32(DC_VID_ST_OFFSET, offset);
    WRITE_REG32(DC_UNLOCK, lock);
}

/*---------------------------------------------------------------------------
 * gfx_set_display_priority_high
 *
 * This routine controls the x-bus round robin arbitration mechanism.
 * When enable is TRUE, graphics pipeline requests and non-critical display
 * controller requests are arbitrated at the same priority as processor
 * requests. When FALSE processor requests are arbitrated at a higher priority
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu1_set_display_priority_high(int enable)
#else
void
gfx_set_display_priority_high(int enable)
#endif
{
    unsigned long lock, control;

    lock = READ_REG32(DC_UNLOCK);
    control = READ_REG32(MC_MEM_CNTRL1);
    WRITE_REG32(DC_UNLOCK, DC_UNLOCK_VALUE);
    if (enable)
        control |= MC_XBUSARB;
    else
        control &= ~(MC_XBUSARB);
    WRITE_REG32(MC_MEM_CNTRL1, control);
    WRITE_REG32(DC_UNLOCK, lock);
    return;
}

/*---------------------------------------------------------------------------
 * gfx_test_timing_active
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu1_test_timing_active(void)
#else
int
gfx_test_timing_active(void)
#endif
{
    if (READ_REG32(DC_TIMING_CFG) & DC_TCFG_TGEN)
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
gu1_test_vertical_active(void)
#else
int
gfx_test_vertical_active(void)
#endif
{
    if (READ_REG32(DC_TIMING_CFG) & DC_TCFG_VNA)
        return (0);
    else
        return (1);
}

/*---------------------------------------------------------------------------
 * gfx_wait_vertical_blank
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu1_wait_vertical_blank(void)
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
gu1_enable_panning(int x, int y)
#else
void
gfx_enable_panning(int x, int y)
#endif
{
    unsigned long modeBytesPerPixel;
    unsigned long modeBytesPerScanline = 0;
    unsigned long startAddress = 0;

    modeBytesPerPixel = (gbpp + 7) / 8;
    modeBytesPerScanline =
        (((ModeWidth + 1023) / 1024) * 1024) * modeBytesPerPixel;

    /* TEST FOR NO-WORK */
    if (x >= DeltaX && (unsigned short) x < (PanelWidth + DeltaX) &&
        y >= DeltaY && (unsigned short) y < (PanelHeight + DeltaY))
        return;

    /* ADJUST PANNING VARIABLES WHEN CURSOR EXCEEDS BOUNDARY       */
    /* Test the boundary conditions for each coordinate and update */
    /* all variables and the starting offset accordingly.          */
    if (x < DeltaX)
        DeltaX = x;
    else if ((unsigned short) x >= (DeltaX + PanelWidth))
        DeltaX = x - PanelWidth + 1;

    if (y < DeltaY)
        DeltaY = y;
    else if ((unsigned short) y >= (DeltaY + PanelHeight))
        DeltaY = y - PanelHeight + 1;

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
 * gfx_set_fixed_timings
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu1_set_fixed_timings(int panelResX, int panelResY, unsigned short width,
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
gu1_set_panel_present(int panelResX, int panelResY, unsigned short width,
                      unsigned short height, unsigned short bpp)
#else
int
gfx_set_panel_present(int panelResX, int panelResY, unsigned short width,
                      unsigned short height, unsigned short bpp)
#endif
{
    /* SET VALID BPP         */
    /* 16BPP is the default. */
    if (bpp != 8 && bpp != 15 && bpp != 16)
        bpp = 16;

    /* RECORD PANEL PARAMETERS */
    /* This routine does not touch any panel timings.  It is used when custom 
     * panel settings are set up in advance by the BIOS or an application, but 
     * the application still requires access to other panel functionality 
     * provided by Durango (i.e. panning)
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

/*-----------------------------------------------------------------------*
 * THE FOLLOWING READ ROUTINES ARE ALWAYS INCLUDED:                      *    
 * gfx_get_hsync_end, gfx_get_htotal, gfx_get_vsync_end, gfx_get_vtotal  *
 * are used by the video overlay routines.                               *
 *                                                                       *
 * gfx_get_vline and gfx_vactive are used to prevent an issue for the    *
 * SC1200.                                                               *
 *                                                                       *
 * The others are part of the Durango API.                               *
 *-----------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
 * gfx_get_display_pitch
 *
 * This routine returns the current pitch of the frame buffer, in bytes.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu1_get_display_pitch(void)
#else
unsigned short
gfx_get_display_pitch(void)
#endif
{
    unsigned long value;

    if (gfx_cpu_version == GFX_CPU_PYRAMID) {
        /* Pyramid update for 4KB line pitch */
        value = (READ_REG32(DC_LINE_DELTA) & 0x07FF) << 2;
    }
    else {
        value = (READ_REG32(DC_LINE_DELTA) & 0x03FF) << 2;
    }

    return ((unsigned short) value);
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
gu1_get_display_details(unsigned int mode, int *xres, int *yres, int *hz)
#else
int
gfx_get_display_details(unsigned int mode, int *xres, int *yres, int *hz)
#endif
{
    if (mode < NUM_GX_DISPLAY_MODES) {
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

        *xres = DisplayParams[mode].hactive;
        *yres = DisplayParams[mode].vactive;

        return (1);
    }
    return (0);
}

/*----------------------------------------------------------------------------
 * GFX_GET_DISPLAY_MODE_COUNT
 *
 * Returns number of modes supported.
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu1_get_display_mode_count(void)
#else
int
gfx_get_display_mode_count(void)
#endif
{
    return (NUM_GX_DISPLAY_MODES);
}

/*----------------------------------------------------------------------------
 * gfx_get_frame_buffer_line_size
 *
 * Returns the current frame buffer line size, in bytes
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long
gu1_get_frame_buffer_line_size(void)
#else
unsigned long
gfx_get_frame_buffer_line_size(void)
#endif
{
    return ((READ_REG32(DC_BUF_SIZE) & 0x1FF) << 3);
}

/*----------------------------------------------------------------------------
 * gfx_mode_frequency_supported
 *
 * This routine examines if the requested mode with pixel frequency is supported.
 *
 * Returns >0 if successful , <0 if freq. could not be found and matched.
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu1_mode_frequency_supported(int xres, int yres, int bpp,
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

    bpp_flag = GFX_MODE_8BPP;
    if (bpp > 8)
        bpp_flag = GFX_MODE_16BPP;

    for (index = 0; index < NUM_GX_DISPLAY_MODES; index++) {
        if ((DisplayParams[index].hactive == (unsigned short) xres) &&
            (DisplayParams[index].vactive == (unsigned short) yres) &&
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
gu1_get_refreshrate_from_frequency(int xres, int yres, int bpp, int *hz,
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

    bpp_flag = GFX_MODE_8BPP;
    if (bpp > 8)
        bpp_flag = GFX_MODE_16BPP;

    /* FIND THE REGISTER VALUES FOR THE DESIRED FREQUENCY */
    /* Search the table for the closest frequency (16.16 format). */
    min = 0x7fffffff;
    for (index = 0; index < NUM_GX_DISPLAY_MODES; index++) {
        if ((DisplayParams[index].htotal == (unsigned short) xres) &&
            (DisplayParams[index].vtotal == (unsigned short) yres) &&
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
gu1_get_refreshrate_from_mode(int xres, int yres, int bpp, int *hz,
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

    bpp_flag = GFX_MODE_8BPP;
    if (bpp > 8)
        bpp_flag = GFX_MODE_16BPP;

    /* FIND THE REGISTER VALUES FOR THE DESIRED FREQUENCY */
    /* Search the table for the closest frequency (16.16 format). */
    min = 0x7fffffff;
    for (index = 0; index < NUM_GX_DISPLAY_MODES; index++) {
        if ((DisplayParams[index].hactive == (unsigned short) xres) &&
            (DisplayParams[index].vactive == (unsigned short) yres) &&
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
gu1_get_frequency_from_refreshrate(int xres, int yres, int bpp, int hz,
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

    if (hz == 56)
        hz_flag = GFX_MODE_56HZ;
    else if (hz == 60)
        hz_flag = GFX_MODE_60HZ;
    else if (hz == 70)
        hz_flag = GFX_MODE_70HZ;
    else if (hz == 72)
        hz_flag = GFX_MODE_72HZ;
    else if (hz == 75)
        hz_flag = GFX_MODE_75HZ;
    else if (hz == 85)
        hz_flag = GFX_MODE_85HZ;

    bpp_flag = GFX_MODE_8BPP;
    if (bpp > 8)
        bpp_flag = GFX_MODE_16BPP;

    /* FIND THE REGISTER VALUES FOR THE DESIRED FREQUENCY */

    for (index = 0; index < NUM_GX_DISPLAY_MODES; index++) {
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
 * This routine returns the maximum recommended speed for the pixel clock.  The 
 * return value is an integer of the format xxxyyy, where xxx.yyy is the maximum
 * floating point pixel clock speed.
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long
gu1_get_max_supported_pixel_clock(void)
#else
unsigned long
gfx_get_max_supported_pixel_clock(void)
#endif
{
    /* ALL CHIPS CAN HANDLE 1280X1024@85HZ - 157.5 MHz */
    return 157500;
}

/*----------------------------------------------------------------------------
 * gfx_get_display_mode
 *
 * This routine gets the specified display mode.
 *
 * Returns: >0 if successful and mode returned, <0 if mode could not be found.
 *----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu1_get_display_mode(int *xres, int *yres, int *bpp, int *hz)
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

    /* SUPPORT EMULATED VGA MODES */
    if (gfx_pixel_double)
        *xres >>= 1;

    if (gfx_line_double)
        *yres >>= 1;

    /* SET BPP FLAGS TO LIMIT MODE SELECTION */
    bpp_flag = GFX_MODE_8BPP;
    if (*bpp > 8)
        bpp_flag = GFX_MODE_16BPP;

    for (mode = 0; mode < NUM_GX_DISPLAY_MODES; mode++) {
        if ((DisplayParams[mode].hactive == (unsigned short) *xres) &&
            (DisplayParams[mode].vactive == (unsigned short) *yres) &&
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

            return (1);
        }
    }
    return (-1);
}

/*---------------------------------------------------------------------------
 * gfx_get_hactive
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu1_get_hactive(void)
#else
unsigned short
gfx_get_hactive(void)
#endif
{
    return ((unsigned short) ((READ_REG32(DC_H_TIMING_1) & 0x07F8) + 8));
}

/*---------------------------------------------------------------------------
 * gfx_get_hsync_start
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu1_get_hsync_start(void)
#else
unsigned short
gfx_get_hsync_start(void)
#endif
{
    return ((unsigned short) ((READ_REG32(DC_H_TIMING_3) & 0x07F8) + 8));
}

/*---------------------------------------------------------------------------
 * gfx_get_hsync_end
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu1_get_hsync_end(void)
#else
unsigned short
gfx_get_hsync_end(void)
#endif
{
    return ((unsigned short) (((READ_REG32(DC_H_TIMING_3) >> 16) & 0x07F8) +
                              8));
}

/*---------------------------------------------------------------------------
 * gfx_get_htotal
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu1_get_htotal(void)
#else
unsigned short
gfx_get_htotal(void)
#endif
{
    return ((unsigned short) (((READ_REG32(DC_H_TIMING_1) >> 16) & 0x07F8) +
                              8));
}

/*---------------------------------------------------------------------------
 * gfx_get_vactive
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu1_get_vactive(void)
#else
unsigned short
gfx_get_vactive(void)
#endif
{
    return ((unsigned short) ((READ_REG32(DC_V_TIMING_1) & 0x07FF) + 1));
}

/*---------------------------------------------------------------------------
 * gfx_get_vsync_end
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu1_get_vsync_end(void)
#else
unsigned short
gfx_get_vsync_end(void)
#endif
{
    return ((unsigned short) (((READ_REG32(DC_V_TIMING_3) >> 16) & 0x07FF) +
                              1));
}

/*---------------------------------------------------------------------------
 * gfx_get_vtotal
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu1_get_vtotal(void)
#else
unsigned short
gfx_get_vtotal(void)
#endif
{
    return ((unsigned short) (((READ_REG32(DC_V_TIMING_1) >> 16) & 0x07FF) +
                              1));
}

/*-----------------------------------------------------------------------------
 * gfx_get_display_bpp
 *
 * This routine returns the current color depth of the active display.
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu1_get_display_bpp(void)
#else
unsigned short
gfx_get_display_bpp(void)
#endif
{
    switch (READ_REG32(DC_OUTPUT_CFG) & 3) {
    case 0:
        return (16);
    case 2:
        return (15);
    }
    return (8);
}

/*---------------------------------------------------------------------------
 * gfx_get_vline
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu1_get_vline(void)
#else
unsigned short
gfx_get_vline(void)
#endif
{
    unsigned short current_scan_line;

    /* Read similar value twice to ensure that the value is not transitioning */
    do {
        current_scan_line = (unsigned short) READ_REG32(DC_V_LINE_CNT) & 0x07FF;
    } while (current_scan_line !=
             (unsigned short) (READ_REG32(DC_V_LINE_CNT) & 0x07FF));

    return (current_scan_line);
}

/*-----------------------------------------------------------------------------
 * gfx_get_display_offset
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long
gu1_get_display_offset(void)
#else
unsigned long
gfx_get_display_offset(void)
#endif
{
    return (READ_REG32(DC_FB_ST_OFFSET) & 0x003FFFFF);
}

/*-----------------------------------------------------------------------------
 * gfx_get_cursor_offset
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long
gu1_get_cursor_offset(void)
#else
unsigned long
gfx_get_cursor_offset(void)
#endif
{
    return (READ_REG32(DC_CURS_ST_OFFSET) & 0x003FFFFF);
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
gu1_get_hblank_start(void)
#else
unsigned short
gfx_get_hblank_start(void)
#endif
{
    return ((unsigned short) ((READ_REG32(DC_H_TIMING_2) & 0x07F8) + 8));
}

/*---------------------------------------------------------------------------
 * gfx_get_hblank_end
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu1_get_hblank_end(void)
#else
unsigned short
gfx_get_hblank_end(void)
#endif
{
    return ((unsigned short) (((READ_REG32(DC_H_TIMING_2) >> 16) & 0x07F8) +
                              8));
}

/*---------------------------------------------------------------------------
 * gfx_get_vblank_start
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu1_get_vblank_start(void)
#else
unsigned short
gfx_get_vblank_start(void)
#endif
{
    return ((unsigned short) ((READ_REG32(DC_V_TIMING_2) & 0x07FF) + 1));
}

/*---------------------------------------------------------------------------
 * gfx_get_vsync_start
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu1_get_vsync_start(void)
#else
unsigned short
gfx_get_vsync_start(void)
#endif
{
    return ((unsigned short) ((READ_REG32(DC_V_TIMING_3) & 0x07FF) + 1));
}

/*---------------------------------------------------------------------------
 * gfx_get_vblank_end
 *---------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu1_get_vblank_end(void)
#else
unsigned short
gfx_get_vblank_end(void)
#endif
{
    return ((unsigned short) (((READ_REG32(DC_V_TIMING_2) >> 16) & 0x07FF) +
                              1));
}

/*-----------------------------------------------------------------------------
 * gfx_get_display_palette_entry
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu1_get_display_palette_entry(unsigned long index, unsigned long *palette)
#else
int
gfx_get_display_palette_entry(unsigned long index, unsigned long *palette)
#endif
{
    unsigned long data;

    if (index > 0xFF)
        return GFX_STATUS_BAD_PARAMETER;

    WRITE_REG32(DC_PAL_ADDRESS, index);
    data = READ_REG32(DC_PAL_DATA);
    data = ((data << 2) & 0x000000FC) |
        ((data << 4) & 0x0000FC00) | ((data << 6) & 0x00FC0000);

    *palette = data;
    return 0;
}

/*-----------------------------------------------------------------------------
 * gfx_get_display_palette
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
void
gu1_get_display_palette(unsigned long *palette)
#else
void
gfx_get_display_palette(unsigned long *palette)
#endif
{
    unsigned long i, data;

    WRITE_REG32(DC_PAL_ADDRESS, 0);
    for (i = 0; i < 256; i++) {
        data = READ_REG32(DC_PAL_DATA);
        data = ((data << 2) & 0x000000FC) |
            ((data << 4) & 0x0000FC00) | ((data << 6) & 0x00FC0000);
        palette[i] = data;
    }
}

/*-----------------------------------------------------------------------------
 * gfx_get_cursor_enable
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long
gu1_get_cursor_enable(void)
#else
unsigned long
gfx_get_cursor_enable(void)
#endif
{
    return (READ_REG32(DC_GENERAL_CFG) & DC_GCFG_CURE);
}

/*-----------------------------------------------------------------------------
 * gfx_get_cursor_position
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long
gu1_get_cursor_position(void)
#else
unsigned long
gfx_get_cursor_position(void)
#endif
{
    return ((READ_REG32(DC_CURSOR_X) & 0x07FF) |
            ((READ_REG32(DC_CURSOR_Y) << 16) & 0x03FF0000));
}

/*-----------------------------------------------------------------------------
 * gfx_get_cursor_clip
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long
gu1_get_cursor_clip(void)
#else
unsigned long
gfx_get_cursor_clip(void)
#endif
{
    return (((READ_REG32(DC_CURSOR_X) >> 11) & 0x01F) |
            ((READ_REG32(DC_CURSOR_Y) << 5) & 0x1F0000));
}

/*-----------------------------------------------------------------------------
 * gfx_get_cursor_color
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long
gu1_get_cursor_color(int color)
#else
unsigned long
gfx_get_cursor_color(int color)
#endif
{
    unsigned long data;

    if (color) {
        WRITE_REG32(DC_PAL_ADDRESS, 0x101);
    }
    else {
        WRITE_REG32(DC_PAL_ADDRESS, 0x100);
    }
    data = READ_REG32(DC_PAL_DATA);
    data = ((data << 6) & 0x00FC0000) |
        ((data << 4) & 0x0000FC00) | ((data << 2) & 0x000000FC);
    return (data);
}

/*-----------------------------------------------------------------------------
 * gfx_get_compression_enable
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu1_get_compression_enable(void)
#else
int
gfx_get_compression_enable(void)
#endif
{
    unsigned long gcfg;

    gcfg = READ_REG32(DC_GENERAL_CFG);
    if (gcfg & DC_GCFG_CMPE)
        return (1);
    else
        return (0);
}

/*-----------------------------------------------------------------------------
 * gfx_get_compression_offset
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned long
gu1_get_compression_offset(void)
#else
unsigned long
gfx_get_compression_offset(void)
#endif
{
    unsigned long offset;

    offset = READ_REG32(DC_CB_ST_OFFSET) & 0x003FFFFF;
    return (offset);
}

/*-----------------------------------------------------------------------------
 * gfx_get_compression_pitch
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu1_get_compression_pitch(void)
#else
unsigned short
gfx_get_compression_pitch(void)
#endif
{
    unsigned short pitch;

    pitch = (unsigned short) (READ_REG32(DC_LINE_DELTA) >> 12) & 0x03FF;
    return (pitch << 2);
}

/*-----------------------------------------------------------------------------
 * gfx_get_compression_size
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
unsigned short
gu1_get_compression_size(void)
#else
unsigned short
gfx_get_compression_size(void)
#endif
{
    unsigned short size;

    size = (unsigned short) ((READ_REG32(DC_BUF_SIZE) >> 9) & 0x7F) - 1;
    return ((size << 2) + 16);
}

/*-----------------------------------------------------------------------------
 * gfx_get_valid_bit
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu1_get_valid_bit(int line)
#else
int
gfx_get_valid_bit(int line)
#endif
{
    int valid;

    WRITE_REG32(MC_DR_ADD, line);
    valid = (int) READ_REG32(MC_DR_ACC) & 1;
    return (valid);
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
gu1_get_display_video_offset(void)
#else
unsigned long
gfx_get_display_video_offset(void)
#endif
{
    return (READ_REG32(DC_VID_ST_OFFSET) & 0x003FFFFF);
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
gu1_get_display_video_size(void)
#else
unsigned long
gfx_get_display_video_size(void)
#endif
{
    /* RETURN TOTAL SIZE, IN BYTES */
    return ((READ_REG32(DC_BUF_SIZE) >> 10) & 0x000FFFC0);
}

/*-----------------------------------------------------------------------------
 * gfx_get_display_priority_high
 *-----------------------------------------------------------------------------
 */
#if GFX_DISPLAY_DYNAMIC
int
gu1_get_display_priority_high(void)
#else
int
gfx_get_display_priority_high(void)
#endif
{
    if (READ_REG32(MC_MEM_CNTRL1) & MC_XBUSARB)
        return (1);
    else
        return (0);
}

#endif                          /* GFX_READ_ROUTINES */

/* END OF FILE */
