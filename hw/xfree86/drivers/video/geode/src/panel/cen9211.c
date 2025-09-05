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
 * File Contents:   This file contains panel functions to interface with
 *                  the centaraus platform.
 * 
 * SubModule:       Geode FlatPanel library
 * */

#include "cen9211.h"

static unsigned char sioc2_orig_val;
static unsigned char must_restore_97317 = FALSE;

/********************************************************************
 *
 * PASS_FAIL init_GPIO(void);
 * Initializes the GPIO pins in the Cx5530 or the National PC97317 
 * for use with a 9211 on a Marmot or Centaurus board. Uses 
 * the global variables RdPCIVal and sioc2_orig_val.
 *
 *********************************************************************/

unsigned char
init_Centaurus_GPIO(void)
{
    unsigned char reg_val;
    static unsigned char first_time = TRUE;

    /* The Centaurus board uses ports 1 and 2 of the 97317 for GPIO. 
     * These ports require bank 0 to be active. The first thing we will
     * do is verify that bank 0 is active and set it if it is not.
     */

    /* set the index for access to the configuration register */
    gfx_outb(CENT_CONFIG_INDEX, CENT_SIOC2);
    reg_val = gfx_inb(CENT_CONFIG_DATA);

    /* set to bank 0 */
    if (reg_val & CENT_GPIO_BANK_SELECT) {
        gfx_outb(CENT_CONFIG_DATA,
                 (unsigned char) (reg_val & ~CENT_GPIO_BANK_SELECT));
    }

    /* If this is the first time we have modified sioc2, we must 
     * save the current value (set by the BIOS) for restoration by 
     * the calling program, set the global flag must_restore_97317, and set 
     * first_time to FALSE. 
     */

    if (first_time == TRUE) {
        sioc2_orig_val = reg_val;
        must_restore_97317 = TRUE;
        first_time = FALSE;
    }

    /* set port 1 direction */
    reg_val = gfx_inb(CENT_PORT1_DIRECTION);

    /* make GPIO 14 and 17 outputs  */
    reg_val |= CENT_97317_CLOCK_MASK | CENT_97317_DATA_OUT_MASK;
    gfx_outb(CENT_PORT1_DIRECTION, reg_val);

    /* set port 2 direction */
    reg_val = gfx_inb(CENT_PORT2_DIRECTION);

    /* make GPIO 20 an output */

    reg_val |= CENT_97317_CHIP_SEL_MASK;

    /* make GPIO 21 an input */

    reg_val &= ~CENT_97317_DATA_IN_MASK;
    gfx_outb(CENT_PORT2_DIRECTION, reg_val);

    /* make GPIO 14 and 17 push-pull */

    reg_val = gfx_inb(CENT_PORT1_OUTPUT_TYPE);
    reg_val |= CENT_97317_CLOCK_MASK | CENT_97317_DATA_OUT_MASK;
    gfx_outb(CENT_PORT1_OUTPUT_TYPE, reg_val);

    /* make GPIO 20 and 21 push-pull */
    reg_val = gfx_inb(CENT_PORT2_OUTPUT_TYPE);
    reg_val |= CENT_97317_CHIP_SEL_MASK | CENT_97317_DATA_IN_MASK;
    gfx_outb(CENT_PORT2_OUTPUT_TYPE, reg_val);
    return CENT_PASS;

}                               /* end init_GPIO() */

/*********************************************************************
 *
 * PASS_FAIL init_9211(void);
 * Initializes (sets to 0) the clock, chip select, and data pins
 * of the Cx9211 on a Marmot or Centaurus board.
 *
 **********************************************************************/

unsigned char
init_Centaurus_9211(void)
{
    unsigned char ReadData;

    /* Uses the 97317 for GPIO. 
     * we will use the clock port define for port 1 
     */
    ReadData = gfx_inb(CENT_97317_CLOCK_PORT);
    ReadData &= ~CENT_97317_CLOCK_MASK & ~CENT_97317_DATA_OUT_MASK;
    gfx_outb(CENT_97317_CLOCK_PORT, ReadData);
    /* we will use the chip select port define for port 2 */
    ReadData = gfx_inb(CENT_97317_CHIP_SELECT);
    ReadData &= ~CENT_97317_CHIP_SEL_MASK & ~CENT_97317_DATA_IN_MASK;
    gfx_outb(CENT_97317_CHIP_SELECT, ReadData);
    return (CENT_PASS);

}                               /*end init_9211() */

/******************************************************************
 *
 * PASS_FAIL restore_97317_SIOC2(void);
 * Restores the original value to the 97317 SIOC2 register using
 * the global variable sioc2_orig_val. Returns PASS if the value 
 * was written, FAIL if not.
 *
 *******************************************************************/

unsigned char
restore_Centaurus_97317_SIOC2(void)
{
    /* set the global flag */
    if (must_restore_97317 == TRUE) {
        unsigned char cfg;

        /* set the index for access to the configuration register */
        gfx_outb(CENT_CONFIG_INDEX, CENT_SIOC2);

        /* restore the value */
        gfx_outb(CENT_CONFIG_DATA, sioc2_orig_val);

        /* now read and verify */
        cfg = gfx_inb(CENT_CONFIG_DATA);
        if (cfg == sioc2_orig_val)
            return (CENT_PASS);
        else
            return (CENT_FAIL);

    }                           /* end if() */
    return (CENT_FAIL);

}                               /* end restore_97317_SIOC2bank() */

/* -----------------------------------------------------------------------
 *
 * SET_FLAT_PANEL_MODE
 *
 * This routine sets the specified flat panel moden parameters in 
 * the 9211.
 * Returns PASS if successful, FAIL if the mode parameters could 
 * not be set.
 *
 *------------------------------------------------------------------------*/

unsigned char
set_Centaurus_92xx_mode(Pnl_PanelStat *pstat)
{
    int mode;

    /* LOOP THROUGH THE AVAILABLE MODES TO FIND A MATCH */

    for (mode = 0; mode < NUM_92XX_MODES; mode++) {
        if ((FPModeParams[mode].xres == pstat->XRes) &&
            (FPModeParams[mode].yres == pstat->YRes) &&
            (FPModeParams[mode].bpp == pstat->Depth) &&
            (FPModeParams[mode].panel_type == pstat->Type) &&
            (FPModeParams[mode].color_type == pstat->MonoColor)) {

            /* SET THE 92xx FOR THE SELECTED MODE */
            set_Centaurus_92xx_mode_params(mode);
            return (CENT_PASS);
        }                       /* end if() */
    }                           /* end for() */
    return (CENT_FAIL);

}                               /* end set_Centaurus_92xx_mode() */

/*-------------------------------------------------------------------
 *
 * SET_92XX_MODE_PARAMS
 * This routine sets the 9211 mode parameters.  
 *
 *-------------------------------------------------------------------*/

void
set_Centaurus_92xx_mode_params(int mode)
{
    CS92xx_MODE *pMode = &FPModeParams[mode];
    unsigned long off_data = 0;

    /* Turn the 92xx power off before setting any new parameters. 
     * Since we are going to reset all the power bit positions, we will
     * force the power register to 0. 
     */

    Centaurus_write_gpio(FOUR_BYTES, CS92xx_LCD_PWR_MAN, off_data);

    /* set 9211 registers using the desired panel settings */

    Centaurus_write_gpio(FOUR_BYTES,
                         CS92xx_LCD_PAN_TIMING1, pMode->panel_timing1);

    Centaurus_write_gpio(FOUR_BYTES,
                         CS92xx_LCD_PAN_TIMING2, pMode->panel_timing2);

    if (Pnl_Rev_ID == PNL_9211_C) {

        /* load the LSFR seeds */
        Centaurus_write_gpio(FOUR_BYTES,
                             CS92xx_LCD_DITH_FR_CNTRL, pMode->rev_C_dither_frc);

        Centaurus_write_gpio(FOUR_BYTES,
                             CS92xx_BLUE_LSFR_SEED, pMode->blue_lsfr_seed);

        Centaurus_write_gpio(FOUR_BYTES,
                             CS92xx_RED_GREEN_LSFR_SEED,
                             pMode->red_green_lsfr_seed);
    }
    else {

        Centaurus_write_gpio(FOUR_BYTES,
                             CS92xx_LCD_DITH_FR_CNTRL, pMode->pre_C_dither_frc);

        Centaurus_write_gpio(FOUR_BYTES,
                             CS92xx_LCD_BLOCK_SEL1, pMode->block_select1);

        Centaurus_write_gpio(FOUR_BYTES,
                             CS92xx_LCD_BLOCK_SEL2, pMode->block_select2);

        Centaurus_write_gpio(FOUR_BYTES,
                             CS92xx_LCD_DISPER1, pMode->dispersion1);

        Centaurus_write_gpio(FOUR_BYTES,
                             CS92xx_LCD_DISPER2, pMode->dispersion2);

        CentaurusProgramFRMload();
    }

    Centaurus_write_gpio(FOUR_BYTES, CS92xx_LCD_MEM_CNTRL,
                         pMode->memory_control);

    /* Set the power register last. This will turn the panel on at the 9211. */

    Centaurus_write_gpio(FOUR_BYTES,
                         CS92xx_LCD_PWR_MAN, pMode->power_management);

}                               /* end set_Centaurus_92xx_mode_params() */

void
Centaurus_write_gpio(int width, ULONG address, unsigned long data)
{
    int num_clock_toggles;
    int count;
    unsigned long Addr = address;

    enable_Centaurus_9211_chip_select();

    /* Write 1 Clock period of no valid transfer */
    write_Centaurus_CX9211_GPIO(CENT_NO_DATA);

    /* Write 1 control bit (the data book calls this the control bar write) */
    write_Centaurus_CX9211_GPIO(0x1);

    /* Write the 12-bit address */
    for (count = 0; count < 12; count++) {
        write_Centaurus_CX9211_GPIO((unsigned char) (Addr & 0x01));
        /*the 9211 expects data LSB->MSB */
        Addr = Addr >> 1;
    }

    /* write */
    write_Centaurus_CX9211_DWdata(data);

    /* a write will require four toggles after disabling CS */
    num_clock_toggles = CENT_NUM_WRITE_CLOCK_TOGGLES;
    disable_Centaurus_9211_chip_select();

    /* now toggle the clock */
    for (count = 0; count < num_clock_toggles; count++) {
        toggle_Centaurus_9211_clock();
    }
    return;

}                               /* end Centaurus_write_gpio() */

unsigned long
Centaurus_read_gpio(int width, unsigned long address)
{
    int num_clock_toggles;
    int count;
    unsigned long Addr = address;
    unsigned long data;

    enable_Centaurus_9211_chip_select();

    /* Write 1 Clock period of no valid transfer */
    write_Centaurus_CX9211_GPIO(CENT_NO_DATA);

    /* Write 1 control bit (the data book calls this the control bar write) */
    write_Centaurus_CX9211_GPIO(0x1);

    /* Write the 12-bit address */
    for (count = 0; count < 12; count++) {
        write_Centaurus_CX9211_GPIO((unsigned char) (Addr & 0x01));

        /*the 9211 expects data LSB->MSB */
        Addr = Addr >> 1;
    }

    data = read_Centaurus_CX9211_DWdata();

    /* a read will require one toggle after disabling CS */
    num_clock_toggles = CENT_NUM_READ_CLOCK_TOGGLES;
    disable_Centaurus_9211_chip_select();

    /* now toggle the clock */
    for (count = 0; count < num_clock_toggles; count++) {
        toggle_Centaurus_9211_clock();
    }
    return data;

}                               /* end Centaurus_read_gpio() */

/*******************************************************************
 *
 * void enable_Centaurus_9211_chip_select(void);
 * Enables the chip select of the CX9211 using the National 97317 
 * on a Centaurus board.
 *
 *******************************************************************/

void
enable_Centaurus_9211_chip_select(void)
{
    unsigned char cs_port_val;

    /* Set the chip select (GPIO20) high */
    cs_port_val = gfx_inb(CENT_97317_CHIP_SELECT);
    gfx_outb(CENT_97317_CHIP_SELECT,
             (unsigned char) (cs_port_val | CENT_97317_CHIP_SEL_MASK));
    return;
}                               /* end enable_Centaurus_9211_chip_select() */

/********************************************************************
 *
 * void disable_Centaurus_9211_chip_select(void);
 * Disables the chip select of the CX9211 using the National 97317 
 * on a Centaurus board.
 *
 *******************************************************************/

void
disable_Centaurus_9211_chip_select(void)
{
    unsigned char cs_port_val;

    /* Set the chip select (GPIO20) low */
    cs_port_val = gfx_inb(CENT_97317_CHIP_SELECT);
    gfx_outb(CENT_97317_CHIP_SELECT,
             (unsigned char) (cs_port_val & ~CENT_97317_CHIP_SEL_MASK));
    return;

}                               /* end disable_Centaurus_9211_chip_select() */

/**********************************************************************
 *
 * void toggle_Centaurus_9211_clock(void);
 * Toggles the clock bit of the CX9211 using the National 97317 on a 
 * Centaurus board. Assumes the 9211 clock bit has previously been 
 * initialized to 0 (this way we do not have to waste GPIO cycles 
 * windowing the clock pulse).
 *
 **********************************************************************/

void
toggle_Centaurus_9211_clock(void)
{
    unsigned char port_val;

    /* get the 97317 GPIO port contents for the 9211 clock */

    port_val = gfx_inb(CENT_97317_CLOCK_PORT);
    /* set the clock bit high */
    gfx_outb(CENT_97317_CLOCK_PORT,
             (unsigned char) (port_val | CENT_97317_CLOCK_MASK));

    /* set the clock bit low */
    gfx_outb(CENT_97317_CLOCK_PORT,
             (unsigned char) (port_val & ~CENT_97317_CLOCK_MASK));

}                               /* end toggle_Centaurus_9211_clock() */

/********************************************************************
 *
 * void write_Centaurus_CX9211_GPIO(unsigned char databit);
 * Writes the value in bit 0 of the value passed in databit to 
 * the 9211 through the GPIO interface of the National 97317 on a 
 * Centaurus board.
 * NOTE: This function does not set or reset the chip select line!
 *
 *******************************************************************/

void
write_Centaurus_CX9211_GPIO(unsigned char databit)
{
    unsigned char data_port_val;

    /* Set the data bit for (GPIO17) */
    databit <<= 7;

    /* read the value of the other bits in the 97317 data port */
    data_port_val = gfx_inb(CENT_97317_DATA_OUTPORT);

    /* set the bit accordingly */
    data_port_val &= ~CENT_97317_DATA_OUT_MASK;
    data_port_val |= databit;
    gfx_outb(CENT_97317_DATA_OUTPORT, data_port_val);

    /* clock the data */
    toggle_Centaurus_9211_clock();
    return;

}                               /* end write_Centaurus_CX9211_GPIO() */

/*****************************************************************
 *
 * void write_Centaurus_CX9211_DWdata(unsigned long data);
 * Writes the doubleword value passed in data to the CX9211 
 * using GPIO Pins of the National 97317 on a Centaurus board.
 * This function assumes the Direction register of the 97317 
 * and the address register of the CX9211 have been previously set.
 * Uses the global variable count.
 * NOTE: This function does not set or reset the chip select line!
 *
 ******************************************************************/

void
write_Centaurus_CX9211_DWdata(unsigned long data)
{
    int count;

    /* Send the read/write command to the 9211 first. */

    write_Centaurus_CX9211_GPIO(CENT_WRITE);

    /* Now write the 32-bit Data */
    for (count = 0; count < 32; count++) {
        write_Centaurus_CX9211_GPIO((unsigned char) (data & 0x01));

        /* the 9211 expects the data LSB->MSB */
        data >>= 1;
    }
    return;

}                               /* end write_Centaurus_CX9211_DWdata() */

/*********************************************************************
 *
 * unsigned char read_Centaurus_CX9211_GPIO(void);
 * Returns the current value of the databit of the 9211 in bit 0 
 * using the GPIO interface of the National 97317 on a Centaurus board.
 * NOTE: This function does not set or reset the chip select line!
 *
 *********************************************************************/

unsigned char
read_Centaurus_CX9211_GPIO(void)
{
    unsigned char data_port_val;

    toggle_Centaurus_9211_clock();

    /* read the data */
    data_port_val = gfx_inb(CENT_97317_DATA_INPORT);

    /* Save the data from (GPIO21) as bit 0 */
    data_port_val >>= 1;
    return (data_port_val & 0x1);

}                               /* end read_Centaurus_CX9211_GPIO() */

/**********************************************************************
 *
 * void read_Centaurus_CX9211_DWdata(unsigned long *data);
 * Reads a doubleword value from the CX9211 using GPIO Pins of 
 * the National 97317 on a Centaurus board.
 * This function assumes the Direction register of the 97317 and 
 * the address register of the CX9211 have been previously set.
 * NOTE: This function does not set or reset the chip select line!
 *
 ***********************************************************************/

unsigned long
read_Centaurus_CX9211_DWdata(void)
{
    unsigned char ReadData;
    int count;
    unsigned long Data;

    /* Send read/write command word to the 9211 first. */
    write_Centaurus_CX9211_GPIO(CENT_READ);

    /* The data book (revision 0.1) states 8 clock periods of no valid data. 
     * However, the data becomes valid on the eighth clock, making the eighth
     * clock valid. Since read_Centaurus_GPIO() toggles the clock before 
     * reading, we will only toggle the clock 7 times here. 
     */
    for (count = 0; count < 7; count++) /* works */
        toggle_Centaurus_9211_clock();

    /* Now read the 32-bit Data, bit by bit in a single loop. */
    Data = 0;
    for (count = 0; count < 32; count++) {
        ReadData = read_Centaurus_CX9211_GPIO();
        /* 9211 sends data  LSB->MSB */
        Data = Data | (((unsigned long) ReadData) << count);
    }                           /* end for() */

    return Data;

}                               /* end read_Centaurus_CX9211_DWdata() */

void
Centaurus_Get_9211_Details(unsigned long flags, Pnl_PanelParams *pParam)
{
    unsigned long PanelType;
    int i;

    for (i = 0; i < 0x7fff; i++) {
    }

    init_Centaurus_GPIO();

    for (i = 0; i < 5; i++)
        toggle_Centaurus_9211_clock();

    if (flags & PNL_PANELCHIP) {

        PanelType = Centaurus_read_gpio(FOUR_BYTES, 0x430);
        PanelType = Centaurus_read_gpio(FOUR_BYTES, 0x430);
        if ((PanelType & 0xFFFF0000) == 0x92110000) {

            /* found 9211 */
            /* check the values for revision ID */
            if (PanelType >= 0x92110301)
                pParam->PanelChip = PNL_9211_C;
            else if ((PanelType >= 0x92110101) && (PanelType < 0x92110301))
                pParam->PanelChip = PNL_9211_A;
            else
                pParam->PanelChip = PNL_UNKNOWN_CHIP;
        }
        else {                  /* no 9211 present */
            pParam->PanelChip = PNL_UNKNOWN_CHIP;
        }
        Pnl_Rev_ID = pParam->PanelChip;
    }
    /* if end */
    if ((pParam->PanelChip != PNL_UNKNOWN_CHIP) && (flags & PNL_PANELSTAT)) {
        PanelType = Centaurus_read_gpio(FOUR_BYTES, 0x438);
        PanelType &= 0x00f8f8f8;
        PanelType |= 0x00070000;
        Centaurus_write_gpio(FOUR_BYTES, 0x438, PanelType);
        PanelType = 0;
        PanelType = Centaurus_read_gpio(FOUR_BYTES, 0x434);
        PanelType = (PanelType >> 8);
        PanelType &= 0x7;

        switch (PanelType) {
        case 0:
            pParam->PanelStat.XRes = 800;
            pParam->PanelStat.YRes = 600;
            pParam->PanelStat.Depth = 18;
            pParam->PanelStat.MonoColor = PNL_COLOR_PANEL;
            pParam->PanelStat.Type = PNL_TFT;
            break;
        case 1:
            pParam->PanelStat.XRes = 640;
            pParam->PanelStat.YRes = 480;
            pParam->PanelStat.Depth = 8;
            pParam->PanelStat.MonoColor = PNL_COLOR_PANEL;
            pParam->PanelStat.Type = PNL_SSTN;
            break;
        case 2:
            pParam->PanelStat.XRes = 1024;
            pParam->PanelStat.YRes = 768;
            pParam->PanelStat.Depth = 18;
            pParam->PanelStat.MonoColor = PNL_COLOR_PANEL;
            pParam->PanelStat.Type = PNL_TFT;
            break;
        case 3:
            pParam->PanelStat.XRes = 640;
            pParam->PanelStat.YRes = 480;
            pParam->PanelStat.Depth = 16;
            pParam->PanelStat.MonoColor = PNL_COLOR_PANEL;
            pParam->PanelStat.Type = PNL_DSTN;
            break;
        case 4:
            pParam->PanelStat.XRes = 640;
            pParam->PanelStat.YRes = 480;
            pParam->PanelStat.Depth = 18;
            pParam->PanelStat.MonoColor = PNL_COLOR_PANEL;
            pParam->PanelStat.Type = PNL_TFT;
            break;
        case 5:
            pParam->PanelStat.XRes = 1024;
            pParam->PanelStat.YRes = 768;
            pParam->PanelStat.Depth = 24;
            pParam->PanelStat.MonoColor = PNL_COLOR_PANEL;
            pParam->PanelStat.Type = PNL_DSTN;
            break;
        case 6:
            pParam->PanelStat.XRes = 640;
            pParam->PanelStat.YRes = 480;
            pParam->PanelStat.Depth = 8;
            pParam->PanelStat.MonoColor = PNL_MONO_PANEL;
            pParam->PanelStat.Type = PNL_DSTN;
            break;
        case 7:
            pParam->PanelStat.XRes = 800;
            pParam->PanelStat.YRes = 600;
            pParam->PanelStat.Depth = 16;
            pParam->PanelStat.MonoColor = PNL_COLOR_PANEL;
            pParam->PanelStat.Type = PNL_DSTN;
            break;
        default:
            break;
        }
    }

}

void
CentaurusProgramFRMload(void)
{
    unsigned long CentaurusFRMtable[] = {
        0x00000000,
        0x00000000,
        0x01000100,
        0x01000100,
        0x01010101,
        0x01010101,
        0x02081041,
        0x02081041,
        0x10111111,
        0x11111101,
        0x49249241,
        0x12412492,
        0x92244891,
        0x92244891,
        0x22252525,
        0x22252525,
        0x528294a5,
        0x2528494a,
        0x294a5295,
        0x294a5295,
        0x54a54a95,
        0x2952a52a,
        0x2a552a55,
        0x2a552a55,
        0x554aa955,
        0x2a9552aa,
        0x2aaa5555,
        0x2aaa5555,
        0x55555555,
        0x2aaaaaaa,
        0x55555555,
        0x55555555,
        0xaaaaaaab,
        0x55555555,
        0x5555aaab,
        0x5555aaab,
        0xaab556ab,
        0x556aad55,
        0x55ab55ab,
        0x55ab55ab,
        0xab5ab56b,
        0x56ad5ad5,
        0x56b5ad6b,
        0x56b5ad6b,
        0xad6d6b5b,
        0x5ad6b6b6,
        0x5b5b5b5b,
        0x5b5b5b5b,
        0x5F6db6db,
        0x5F6db6db,
        0xF776F776,
        0xF776F776,
        0xFBDEFBDE,
        0xFBDEFBDE,
        0x7eFFBFF7,
        0x7eFFBFF7,
        0xFF7FF7F7,
        0xFF7FF7F7,
        0xFF7FFF7F,
        0xFF7FFF7F,
        0xFFF7FFFF,
        0xFFF7FFFF,
        0xFFFFFFFF,
        0xFFFFFFFF,
    };

    unsigned char i;
    unsigned long data;

    Centaurus_write_gpio(FOUR_BYTES, CS92xx_FRM_MEMORY_INDEX, 0);
    for (i = 0; i < 64; i += 2) {
        data = CentaurusFRMtable[i];
        Centaurus_write_gpio(FOUR_BYTES, CS92xx_FRM_MEMORY_DATA, data);
        data = CentaurusFRMtable[i + 1];
        Centaurus_write_gpio(FOUR_BYTES, CS92xx_FRM_MEMORY_DATA, data);
    }

    /*
     * The first FRM location (64 bits) does not program correctly.
     * This location always reads back with the last value programmed.
     * ie. If 32 64-bit values are programmed, location 0 reads 
     * back as the 32nd If 30 locations are programmed, location 0 
     * reads back as the 30th, etc.
     * Fix this by re-writing location 0 after programming all 64 in 
     * the writeFRM loop in RevCFrmload() in CS9211.
     */

    Centaurus_write_gpio(FOUR_BYTES, CS92xx_FRM_MEMORY_INDEX, 0);
    Centaurus_write_gpio(FOUR_BYTES, CS92xx_FRM_MEMORY_DATA, 0);
    Centaurus_write_gpio(FOUR_BYTES, CS92xx_FRM_MEMORY_DATA, 0);
}

/********************************************************************
 *
 * void Centaurus_Enable_Power((void);
 * Enables the power of the CX9211 using the National 97317 on 
 * a Centaurus board.
 *
 ********************************************************************/

void
Centaurus_Power_Up(void)
{
    unsigned long off_data = 0x01000000;

    Centaurus_write_gpio(FOUR_BYTES, CS92xx_LCD_PWR_MAN, off_data);
    return;

}                               /* Centaurus_Disable_Power */

/***********************************************************************
 *
 * void Centaurus_Disable_Power((void);
 * Disables the power of the CX9211 using the National 97317 
 * on a Centaurus board.
 *
 **********************************************************************/

void
Centaurus_Power_Down(void)
{
    unsigned long off_data = 0;

    Centaurus_write_gpio(FOUR_BYTES, CS92xx_LCD_PWR_MAN, off_data);
    return;

}                               /* Centaurus_Disable_Power */

void
Centaurus_9211init(Pnl_PanelStat *pstat)
{
    init_Centaurus_GPIO();
    init_Centaurus_9211();
    set_Centaurus_92xx_mode(pstat);
    restore_Centaurus_97317_SIOC2();
}

void
Centaurus_Save_Panel_State(void)
{
    /* set 9211 registers using the desired panel settings */

    cs9211_regs.panel_timing1 =
        Centaurus_read_gpio(FOUR_BYTES, CS92xx_LCD_PAN_TIMING1);
    cs9211_regs.panel_timing2 =
        Centaurus_read_gpio(FOUR_BYTES, CS92xx_LCD_PAN_TIMING2);
    cs9211_regs.dither_frc_ctrl =
        Centaurus_read_gpio(FOUR_BYTES, CS92xx_LCD_DITH_FR_CNTRL);
    cs9211_regs.blue_lsfr_seed =
        Centaurus_read_gpio(FOUR_BYTES, CS92xx_BLUE_LSFR_SEED);

    cs9211_regs.red_green_lsfr_seed =
        Centaurus_read_gpio(FOUR_BYTES, CS92xx_RED_GREEN_LSFR_SEED);
    /* CentaurusProgramFRMload(); */

    cs9211_regs.memory_control =
        Centaurus_read_gpio(FOUR_BYTES, CS92xx_LCD_MEM_CNTRL);

    /* Set the power register last. 
     * This will turn the panel on at the 9211.
     */
    cs9211_regs.power_management =
        Centaurus_read_gpio(FOUR_BYTES, CS92xx_LCD_PWR_MAN);
}

void
Centaurus_Restore_Panel_State(void)
{

    unsigned long off_data = 0;

    /* Before restoring the 9211 registers, power off the 9211. */
    Centaurus_write_gpio(FOUR_BYTES, CS92xx_LCD_PWR_MAN, off_data);

    /* set 9211 registers using the desired panel settings */

    Centaurus_write_gpio(FOUR_BYTES, CS92xx_LCD_PAN_TIMING1,
                         cs9211_regs.panel_timing1);

    Centaurus_write_gpio(FOUR_BYTES, CS92xx_LCD_PAN_TIMING2,
                         cs9211_regs.panel_timing2);

    /* load the LSFR seeds */

    Centaurus_write_gpio(FOUR_BYTES, CS92xx_LCD_DITH_FR_CNTRL,
                         cs9211_regs.dither_frc_ctrl);

    Centaurus_write_gpio(FOUR_BYTES, CS92xx_BLUE_LSFR_SEED,
                         cs9211_regs.blue_lsfr_seed);

    Centaurus_write_gpio(FOUR_BYTES, CS92xx_RED_GREEN_LSFR_SEED,
                         cs9211_regs.red_green_lsfr_seed);

    Centaurus_write_gpio(FOUR_BYTES, CS92xx_LCD_MEM_CNTRL,
                         cs9211_regs.memory_control);

    /* Set the power register last. This will turn the panel on at the 9211 */

    Centaurus_write_gpio(FOUR_BYTES, CS92xx_LCD_PWR_MAN,
                         cs9211_regs.power_management);

}
