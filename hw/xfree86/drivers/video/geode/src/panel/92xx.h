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
 * File Contents: This header file defines the Durango routines and 
 *               variables used to access the memory mapped regions.
 * 
 * SubModule:     Geode FlatPanel library
 * */

#ifndef _92XX_h
#define _92XX_h

typedef unsigned long ULONG;
typedef unsigned char UCHAR;

#define FALSE	         0
#define TRUE             1
#define NUM_92XX_MODES   13
#define ONE_BYTE         1
#define TWO_BYTES        2
#define FOUR_BYTES       4

/* LCD Registers
 * The LCD memory area is shared by both TV and LCD. 
 * This offset is for LCD access. 
 */

#define CS92xx_LCD_OFFSET     0x00000400

/* LCD CONTROL REGISTERS */

#define CS92xx_LCD_PAN_TIMING1	    CS92xx_LCD_OFFSET +	0x00

/* flat panel(FP) timings */
#define CS92xx_LCD_PAN_TIMING2      CS92xx_LCD_OFFSET +	0x04

/* FP panel timings */
#define CS92xx_LCD_PWR_MAN          CS92xx_LCD_OFFSET +	0x08

/* FP power management */
#define CS92xx_LCD_DITH_FR_CNTRL    CS92xx_LCD_OFFSET +	0x0C

/* FP dither and frame rate 
 * these defines are in revisions prior to C 
 */
#define CS92xx_LCD_BLOCK_SEL1       CS92xx_LCD_OFFSET +	0x10

/* FRM register */
#define CS92xx_LCD_BLOCK_SEL2       CS92xx_LCD_OFFSET +	0x14

/* FRM register */
#define CS92xx_LCD_DISPER1          CS92xx_LCD_OFFSET +	0x18

/* FRM register */
#define CS92xx_LCD_DISPER2          CS92xx_LCD_OFFSET +	0x1C

/* FRM register 
 * these defines are revision C 
 */
#define CS92xx_BLUE_LSFR_SEED       CS92xx_LCD_OFFSET +	0x10

/* FRM register */
#define CS92xx_RED_GREEN_LSFR_SEED  CS92xx_LCD_OFFSET +	0x14

/* FRM register */
#define CS92xx_FRM_MEMORY_INDEX     CS92xx_LCD_OFFSET +	0x18

/* FRM register */
#define CS92xx_FRM_MEMORY_DATA      CS92xx_LCD_OFFSET +	0x1C

/* FRM register */
#define CS92xx_LCD_MEM_CNTRL        CS92xx_LCD_OFFSET +	0x20

/* memory PLL register */
#define CS92xx_LCD_RAM_CNTRL        CS92xx_LCD_OFFSET +	0x24

/* ram control */

#define CS92xx_LCD_RAM_DATA         CS92xx_LCD_OFFSET +	0x28    /* ram data */

#define CS92xx_LCD_PAN_CRC_SIG      CS92xx_LCD_OFFSET +	0x2C

/* FP CRC signature */
#define CS92xx_DEV_REV_ID           CS92xx_LCD_OFFSET +	0x30

/* Device and revision id */
#define CS92xx_LCD_GPIO_DATA        CS92xx_LCD_OFFSET +	0x34    /* GPIO Data */

#define CS92xx_LCD_GPIO_CNTRL       CS92xx_LCD_OFFSET +	0x38

/* GPIO Control */
int Pnl_Rev_ID;

typedef struct {
    /* DISPLAY MODE PARAMETERS */
    int xres;
    int yres;
    int bpp;
    int panel_type;
    int color_type;
    /* VALUES USED TO SET THE FLAT PANEL DISPLAY CONTROLLER */
    unsigned long panel_timing1;
    unsigned long panel_timing2;
    unsigned long power_management;
    /* the following 5 registers are prior to revision C */
    unsigned long pre_C_dither_frc;
    unsigned long block_select1;
    unsigned long block_select2;
    unsigned long dispersion1;
    unsigned long dispersion2;
    /* the following 4 registers are revision C only */
    unsigned long rev_C_dither_frc;
    unsigned long blue_lsfr_seed;
    unsigned long red_green_lsfr_seed;
    unsigned long frm_memory_index;
    unsigned long frm_memory_data;
    unsigned long memory_control;

} CS92xx_MODE;

/* VALUES USED TO SAVE AND RESTORE 9211 REGISTERS. */
typedef struct {
    unsigned long panel_state;
    /* VALUES USED TO SET THE FLAT PANEL DISPLAY CONTROLLER */
    unsigned long panel_timing1;
    unsigned long panel_timing2;
    unsigned long power_management;
    unsigned long dither_frc_ctrl;
    unsigned long blue_lsfr_seed;
    unsigned long red_green_lsfr_seed;
    unsigned long frm_memory_index;
    unsigned long frm_memory_data;
    unsigned long memory_control;
} CS92xx_REGS;

CS92xx_REGS cs9211_regs;

/*
 *------------------------------------------------------------------------
 * PANEL MODE TABLES:
 * GLOBAL ARRAY OF FLAT PANEL MODE STRUCTURES
 *------------------------------------------------------------------------
 */
CS92xx_MODE FPModeParams[] = {

    {640, 480, 8, PNL_SSTN, PNL_COLOR_PANEL,    /* display parameters */
     0x01e00000, 0x00034000,    /* panel timing reg 1, panel timing */
     /* reg 2                                                        */
     0x01000000,                /* power management                             */
     /* The next 5 values are prior to revision C                                        */
     0x00000050,                /* dither and frame rate control        */
     0x25cf3096, 0xad47b81e,    /* block select 1, block select 2       */
     0x21446450, 0x21446450,    /* dispersion 1, dispersion 2           */
     /* The next 5 values are for revision C                                                     */
     0x00000050,                /* dither and frame rate control        */
     0x00000000, 0x00000000,    /* blue LSFR, red and green LSFR        */
     0x00000000, 0x00000000,    /* FRM memory index, FRM memory data */
     0x00000000,                /* memory control                                       */
     },

    {640, 480, 12, PNL_TFT, PNL_COLOR_PANEL,    /* display parameters */
     0x01e00000, 0x0f100000,    /* panel timing reg 1, panel timing */
     /* reg 2                                                        */
     0x01000000,                /* power management                             */
     /* The next 5 values are prior to revision C                                        */
     0x00000050,                /* dither and frame rate control        */
     0x00000000, 0x00000000,    /* block select 1, block select 2       */
     0x00000000, 0x00000000,    /* dispersion 1, dispersion 2           */
     /* The next 5 values are for revision C                                             */
     0x00000050,                /* dither and frame rate control        */
     0x00000000, 0x00000000,    /* blue LSFR, red and green LSFR        */
     0x00000000, 0x00000000,    /* FRM memory index, FRM memory data */
     0x00000000,                /* memory control                                       */
     },

    {640, 480, 18, PNL_TFT, PNL_COLOR_PANEL,    /* display parameters */
     0x01e00000, 0x0f100000,    /* panel timing reg 1, panel timing */
     /* reg 2                                                        */
     0x01000000,                /* power management                             */
     /* The next 5 values are prior to revision C                                        */
     0x00000050,                /* dither and frame rate control        */
     0x00000000, 0x00000000,    /* block select 1, block select 2       */
     0x00000000, 0x00000000,    /* dispersion 1, dispersion 2           */
     /* The next 5 values are for revision C */
     0x00000050,                /* dither and frame rate control        */
     0x00000000, 0x00000000,    /* blue LSFR, red and green LSFR        */
     0x00000000, 0x00000000,    /* FRM memory index, FRM memory data */
     0x00000000,                /* memory control                                       */
     },

    {640, 480, 16, PNL_DSTN, PNL_COLOR_PANEL,   /* display parameters */
     0x01e00000, 0x00014000,    /* panel timing reg 1, panel timing */
     /* reg 2                                                        */
     0x01000000,                /* power management                             */
     /* The next 5 values are prior to revision C */
     0x00000050,                /* dither and frame rate control        */
     0x048c26ae, 0x048c26ae,    /* block select 1, block select 2       */
     0x02468ace, 0x13579bdf,    /* dispersion 1, dispersion 2           */
     /* The next 5 values are for revision C */
     0x0000004b,                /* dither and frame rate control        */
     0x00000000, 0x00000000,    /* blue LSFR, red and green LSFR        */
     0x00000000, 0x00000000,    /* FRM memory index, FRM memory data */
     0x00000007,                /* memory control                                       */
     },

    {640, 480, 8, PNL_DSTN, PNL_MONO_PANEL,     /* display parameters */
     0x01e00000, 0x00084000,    /* panel timing reg 1, panel timing */
     /* reg 2                                                        */
     0x01000000,                /* power management                             */
     /* The next 5 values are prior to revision C */
     0x0000004b,                /* dither and frame rate control        */
     0x25cf3096, 0xad47b81e,    /* block select 1, block select 2       */
     0x21446450, 0x21446450,    /* dispersion 1, dispersion 2           */
     /* The next 5 values are for revision C  */
     0x00000050,                /* dither and frame rate control        */
     0x00000000, 0x00000000,    /* blue LSFR, red and green LSFR        */
     0x00000000, 0x00000000,    /* FRM memory index, FRM memory data */
     0x00000007,                /* memory control                                       */
     },

    {640, 480, 16, PNL_DSTN, PNL_MONO_PANEL,    /* display parameters */
     0x01e00000, 0x00094000,    /* panel timing reg 1, panel timing */
     /* reg 2                                                        */
     0x01000000,                /* power management                             */
     /* The next 5 values are prior to revision C */
     0x00000050,                /* dither and frame rate control        */
     0x25cf3096, 0xad47b81e,    /* block select 1, block select 2       */
     0x81a5d470, 0x29cfb63e,    /* dispersion 1, dispersion 2           */
     /* The next 5 values are for revision C */
     0x00000050,                /* dither and frame rate control        */
     0x00000000, 0x00000000,    /* blue LSFR, red and green LSFR        */
     0x00000000, 0x00000000,    /* FRM memory index, FRM memory data */
     0x00000007,                /* memory control                                       */
     },

    {800, 600, 12, PNL_TFT, PNL_COLOR_PANEL,    /* display parameters */
     0x02580000, 0x0f100000,    /* panel timing reg 1, panel timing */
     /* reg 2                                                        */
     0x01000000,                /* power management                             */
     /* The next 5 values are prior to revision C */
     0x00000050,                /* dither and frame rate control        */
     0x00000000, 0x00000000,    /* block select 1, block select 2       */
     0x00000000, 0x00000000,    /* dispersion 1, dispersion 2           */
     /* The next 5 values are for revision C */
     0x00000050,                /* dither and frame rate control        */
     0x00000000, 0x00000000,    /* blue LSFR, red and green LSFR        */
     0x00000000, 0x00000000,    /* FRM memory index, FRM memory data */
     0x00000000,                /* memory control                                       */
     },

    {800, 600, 18, PNL_TFT, PNL_COLOR_PANEL,    /* display parameters */
     0x02580000, 0x0f100000,    /* panel timing reg 1, panel timing */
     /* reg 2                                                        */
     0x01000000,                /* power management                             */
     /* The next 5 values are prior to revision C */
     0x00000050,                /* dither and frame rate control        */
     0x00000000, 0x00000000,    /* block select 1, block select 2       */
     0x00000000, 0x00000000,    /* dispersion 1, dispersion 2           */
     /* The next 5 values are for revision C */
     0x00000050,                /* dither and frame rate control        */
     0x00000000, 0x00000000,    /* blue LSFR, red and green LSFR        */
     0x00000000, 0x00000000,    /* FRM memory index, FRM memory data */
     0x00000000,                /* memory control                                       */
     },

    {800, 600, 16, PNL_DSTN, PNL_COLOR_PANEL,   /* display parameters */
     0x02580000, 0x00014000,    /* panel timing reg 1, panel timing */
     /* reg 2                                                        */
     0x01000000,                /* power management                             */
     /* The next 5 values are prior to revision C */
     0x00000050,                /* dither and frame rate control        */
     0x048c26ae, 0x048c26ae,    /* block select 1, block select 2       */
     0x02468ace, 0x13579bdf,    /* dispersion 1, dispersion 2           */
     /* The next 5 values are for revision C */
     0x0000004b,                /* dither and frame rate control        */
     0x00000000, 0x00000000,    /* blue LSFR, red and green LSFR        */
     0x00000000, 0x00000000,    /* FRM memory index, FRM memory data */
     0x00000007,                /* memory control                                       */
     },

    {800, 600, 8, PNL_DSTN, PNL_MONO_PANEL,     /* display parameters */
     0x02580000, 0x00084000,    /* panel timing reg 1, panel timing */
     /* reg 2                                                        */
     0x01000000,                /* power management                             */
     /* The next 5 values are prior to revision C */
     0x00000050,                /* dither and frame rate control        */
     0x25cf3096, 0xad47b81e,    /* block select 1, block select 2       */
     0x21446450, 0x21446450,    /* dispersion 1, dispersion 2           */
     /* The next 5 values are for revision C */
     0x0000004b,                /* dither and frame rate control        */
     0x00000000, 0x00000000,    /* blue LSFR, red and green LSFR        */
     0x00000000, 0x00000000,    /* FRM memory index, FRM memory data */
     0x00000007,                /* memory control                                       */
     },

    {800, 600, 16, PNL_DSTN, PNL_MONO_PANEL,    /* display parameters */
     0x02580000, 0x00094000,    /* panel timing reg 1, panel timing */
     /* reg 2                                                        */
     0x01000000,                /* power management                             */
     /* The next 5 values are prior to revision C */
     0x00000050,                /* dither and frame rate control        */
     0x25cf3096, 0xad47b81e,    /* block select 1, block select 2       */
     0x81a5d470, 0x29cfb63e,    /* dispersion 1, dispersion 2           */
     /* The next 5 values are for revision C */
     0x00000050,                /* dither and frame rate control        */
     0x00000000, 0x00000000,    /* blue LSFR, red and green LSFR        */
     0x00000000, 0x00000000,    /* FRM memory index, FRM memory data */
     0x00000007,                /* memory control                                       */
     },

    {1024, 768, 18, PNL_TFT, PNL_COLOR_PANEL,   /* display parameters */
     0x03000000, 0x0f100000,    /* panel timing reg 1, panel timing */
     /* reg 2                                                        */
     0x01000000,                /* power management                             */
     /*The next 5 values are prior to revision C */
     0x00000050,                /* dither and frame rate control        */
     0x00000000, 0x00000000,    /* block select 1, block select 2       */
     0x00000000, 0x00000000,    /* dispersion 1, dispersion 2           */
     /*The next 5 values are for revision C */
     0x00000050,                /* dither and frame rate control        */
     0x00000000, 0x00000000,    /* blue LSFR, red and green LSFR        */
     0x00000000, 0x00000000,    /* FRM memory index, FRM memory data */
     0x00000000,                /* memory control                                       */
     },

    {1024, 768, 24, PNL_DSTN, PNL_COLOR_PANEL,  /* display parameters */
     0x03000000, 0x80024000,    /* panel timing reg 1, panel timing reg 2 */
     0x01000000,                /* power management                             */
     /*The next 5 values are prior to revision C                                         */
     0x00000050,                /* dither and frame rate control        */
     0x048c26ae, 0x048c26ae,    /* block select 1, block select 2       */
     0x02468ace, 0x13579bdf,    /* dispersion 1, dispersion 2           */
     /*The next 5 values are for revision C */
     0x0000004b,                /* dither and frame rate control        */
     0x00000000, 0x00000000,    /* blue LSFR, red and green LSFR        */
     0x00000000, 0x00000000,    /* FRM memory index, FRM memory data */
     0x00000005,                /* memory control                                       */
     }
};

#endif                          /* !_92XX_h */

/* END OF FILE */
