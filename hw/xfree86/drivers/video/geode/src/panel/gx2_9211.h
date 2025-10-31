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
 * File Contents:   This file contains the panel library files to the 
 *                  GX2 platforms with 9211 support.
 * 
 * SubModule:       Geode FlatPanel library
 * */

/* -----------------------------------------------------------
 *  GX2 FLAT PANEL CONTROLLER REGISTER DEFINITIONS           
 *-----------------------------------------------------------
 */

#define	GX2_VP_MSR_PAD_SELECT		0x2011
#define	GX2_VP_PAD_SELECT_MASK		0x3FFFFFFF
#define	GX2_VP_PAD_SELECT_TFT		0x1FFFFFFF
#define	GX2_VP_PAD_SELECT_DSTN		0x00000000

/* This is useful for generating addresses incrementally 
 * (ie, vidtest register display code).
 */

#define GX2_FP_LCD_OFFSET       0x00000400
#define CS9211_REDCLOUD         0x0400  /* Moved 9211 Rev C3 up to next major
                                         * no.                                                          */
#define GX2_FP_PAN_TIMING1      0x0400  /* FP timings 1                                         */
#define GX2_FP_PAN_TIMING2      0x0408  /* FP timings 2                                         */
#define GX2_FP_PWR_MAN          0x0410  /* FP power management                          */
#define GX2_FP_DITH_FR_CNTRL    0x0418  /* FP dither and frame rate             */
#define GX2_FP_BLFSR            0x0420  /* Blue LFSR seed                                       */
#define GX2_FP_RLFSR            0x0428  /* Red and Green LFSR seed                      */
#define GX2_FP_FMI              0x0430  /* FRM Memory Index                             */
#define GX2_FP_FMD              0x0438  /* FRM Memory Data                                      */
#define GX2_FP_DCA              0x0448  /* Dither ram control and address       */
#define GX2_FP_DMD              0x0450  /* Dither memory data                           */
#define GX2_FP_PAN_CRC_SIG      0x0458  /* FP CRC signature                             */
#define GX2_FP_FBB              0x0460  /* Frame Buffer Base Address            */

/* GX2_FP_PAN_TIMING2 bits */

#define GX2_FP_TFT_PASS_THRU		0x40000000      /* TFT pass through enable      */
#define GX2_FP_PT2_PIX_OUT_MASK		0xFFF8FFFF      /* panel output bit formats */
#define GX2_FP_PT2_PIX_OUT_TFT		0x00000000      /* 8 BIT DSTN or TFT panel  */
#define GX2_FP_PT2_COLOR_MONO		0x00080000      /* color or monochrome              */
#define GX2_FP_PT2_DSTN_TFT_MASK	0xFFCFFFFF      /* panel type bits                  */
#define GX2_FP_PT2_DSTN_TFT_TFT		0x00100000      /* TFT panel                                */
#define GX2_FP_PT2_PSH_CLK_CTL		0x08000000      /* shift clock retrace 
                                                         * activity control                 */

/*  GX2_FP_PWR_MAN bits */

#define GX2_FP_PM_SHFCLK_INVERT		0x00002000      /* Invert shfclk to panel       */
#define GX2_FP_PM_VSYNC_DELAY		0x0000C000      /* Vert Sync delay                  */
#define GX2_FP_PM_HSYNC_DELAY		0x00030000      /* Horiz Sync delay                 */
#define GX2_FP_PM_PWRDN_PHASE_BIT0	0x00040000      /* panel power down phase bit
                                                         * 0                                                */
#define GX2_FP_PM_PWRDN_PHASE_BIT1	0x00080000      /* panel power down phase bit
                                                         * 1                                                */
#define GX2_FP_PM_PWRDN_PHASE_BIT2	0x00100000      /* panel power down phase bit
                                                         * 2                                                */
#define GX2_FP_PM_PWRUP_PHASE_BIT0	0x00200000      /* panel power up phase bit 
                                                         * 0                                                */
#define GX2_FP_PM_PWRUP_PHASE_BIT1	0x00400000      /* panel power up phase bit 
                                                         * 1                                                */
#define GX2_FP_PM_PWRUP_PHASE_BIT2	0x00800000      /* panel power up phase bit 
                                                         * 2                                                */
#define GX2_FP_PM_PWR_ON			0x01000000      /* panel power ON */
#define GX2_FP_PM_DIS_OFF_CTL		0x02000000      /* disable the panel back 
                                                         * light                                    */
#define GX2_FP_PM_EXT_PWR_SEQ		0x08000000      /* external power sequence  */

/*  GX2_FP_PAN_CRC_SIG bits */

#define GX2_FP_PAN_CRC_SIGE         0x00000001  /* CRC Sig Enable                       */
#define GX2_FP_PAN_CRC_SFR          0x00000002  /* CRC Sig Free Run             */

/* This define is used by the hardware CRC mechanism */
#define GX2_FP_CRC_PASS_THRU_MASK	0x00000070

#define GX2_READ 0
#define GX2_WRITE 1

void SetFPBaseAddr(unsigned long);
void Redcloud_9211init(Pnl_PanelStat *);
void protected_mode_access(unsigned long mode,
                           unsigned long width, unsigned long addr,
                           char *pdata);
void write_video_reg64_low(unsigned long offset, unsigned long value);
unsigned long read_video_reg64_low(unsigned long offset);
void Redcloud_fp_reg(int mode, unsigned long address, unsigned long *data);
void set_Redcloud_92xx_mode_params(int mode);
unsigned char set_Redcloud_92xx_mode(Pnl_PanelStat * pstat);
