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
 * This header file contains the graphics register definitions.
 * */

/*----------------------------------*/
/*  FIRST GENERATION GRAPHICS UNIT  */
/*----------------------------------*/

#define GP_DST_XCOOR			0x8100  /* x destination origin         */
#define GP_DST_YCOOR			0x8102  /* y destination origin                 */
#define GP_WIDTH				0x8104  /* pixel width                  */
#define GP_HEIGHT				0x8106  /* pixel height                 */
#define GP_SRC_XCOOR			0x8108  /* x source origin              */
#define GP_SRC_YCOOR			0x810A  /* y source origin              */

#define GP_VECTOR_LENGTH		0x8104  /* vector length                */
#define GP_INIT_ERROR			0x8106  /* vector initial error         */
#define GP_AXIAL_ERROR			0x8108  /* axial error increment        */
#define GP_DIAG_ERROR			0x810A  /* diagonal error increment     */

#define GP_SRC_COLOR_0			0x810C  /* source color 0               */
#define GP_SRC_COLOR_1			0x810E  /* source color 1               */
#define GP_PAT_COLOR_0			0x8110  /* pattern color 0              */
#define GP_PAT_COLOR_1			0x8112  /* pattern color 1              */
#define GP_PAT_COLOR_2			0x8114  /* pattern color 2              */
#define GP_PAT_COLOR_3			0x8116  /* pattern color 3              */
#define GP_PAT_DATA_0			0x8120  /* bits 31:0 of pattern         */
#define GP_PAT_DATA_1			0x8124  /* bits 63:32 of pattern        */
#define GP_PAT_DATA_2			0x8128  /* bits 95:64 of pattern        */
#define GP_PAT_DATA_3			0x812C  /* bits 127:96 of pattern       */

#define GP_VGA_WRITE			0x8140  /* VGA write path control       */
#define GP_VGA_READ				0x8144  /* VGA read path control        */

#define GP_RASTER_MODE			0x8200  /* raster operation             */
#define GP_VECTOR_MODE			0x8204  /* vector mode register         */
#define GP_BLIT_MODE			0x8208  /* blit mode register           */
#define GP_BLIT_STATUS			0x820C  /* blit status register         */

#define GP_VGA_BASE				0x8210  /* VGA memory offset (x64K)     */
#define GP_VGA_LATCH			0x8214  /* VGA display latch            */

/* "GP_VECTOR_MODE" BIT DEFINITIONS */

#define VM_X_MAJOR				0x0000  /* X major vector               */
#define VM_Y_MAJOR				0x0001  /* Y major vector               */
#define VM_MAJOR_INC			0x0002  /* positive major axis step     */
#define VM_MINOR_INC			0x0004  /* positive minor axis step     */
#define VM_READ_DST_FB			0x0008  /* read destination data        */

/* "GP_RASTER_MODE" BIT DEFINITIONS */

#define RM_PAT_DISABLE			0x0000  /* pattern is disabled          */
#define RM_PAT_MONO				0x0100  /* 1BPP pattern expansion       */
#define RM_PAT_DITHER			0x0200  /* 2BPP pattern expansion       */
#define RM_PAT_COLOR			0x0300  /* 8BPP or 16BPP pattern        */
#define RM_PAT_MASK				0x0300  /* mask for pattern mode        */
#define RM_PAT_TRANSPARENT		0x0400  /* transparent 1BPP pattern     */
#define RM_SRC_TRANSPARENT		0x0800  /* transparent 1BPP source      */

/* "GP_BLIT_STATUS" BIT DEFINITIONS */

#define BS_BLIT_BUSY			0x0001  /* blit engine is busy          */
#define BS_PIPELINE_BUSY		0x0002  /* graphics pipeline is busy    */
#define BS_BLIT_PENDING			0x0004  /* blit pending                 */
#define BC_FLUSH				0x0080  /* flush pipeline requests      */
#define BC_8BPP					0x0000  /* 8BPP mode                    */
#define BC_16BPP				0x0100  /* 16BPP mode                   */
#define BC_FB_WIDTH_1024		0x0000  /* framebuffer width = 1024     */
#define BC_FB_WIDTH_2048		0x0200  /* framebuffer width = 2048     */
#define BC_FB_WIDTH_4096		0x0400  /* framebuffer width = 4096     */

/* "GP_BLIT_MODE" BIT DEFINITIONS */

#define	BM_READ_SRC_NONE		0x0000  /* source foreground color      */
#define BM_READ_SRC_FB			0x0001  /* read source from FB          */
#define BM_READ_SRC_BB0			0x0002  /* read source from BB0         */
#define BM_READ_SRC_BB1			0x0003  /* read source from BB1         */
#define BM_READ_SRC_MASK		0x0003  /* read source mask             */

#define	BM_READ_DST_NONE		0x0000  /* no destination data          */
#define BM_READ_DST_BB0			0x0008  /* destination from BB0         */
#define BM_READ_DST_BB1			0x000C  /* destination from BB1         */
#define BM_READ_DST_FB0			0x0010  /* dest from FB (store BB0)     */
#define BM_READ_DST_FB1			0x0014  /* dest from FB (store BB1)     */
#define BM_READ_DST_MASK		0x001C  /* read destination mask        */

#define BM_WRITE_FB				0x0000  /* write to framebuffer         */
#define	BM_WRITE_MEM			0x0020  /* write to memory              */
#define BM_WRITE_MASK			0x0020  /* write mask                   */

#define	BM_SOURCE_COLOR			0x0000  /* source is 8BPP or 16BPP      */
#define BM_SOURCE_EXPAND		0x0040  /* source is 1BPP               */
#define BM_SOURCE_TEXT			0x00C0  /* source is 1BPP text          */
#define BM_SOURCE_MASK			0x00C0  /* source mask                  */

#define BM_REVERSE_Y			0x0100  /* reverse Y direction          */

/*---------------------------------------*/
/*  FIRST GENERATION DISPLAY CONTROLLER  */
/*---------------------------------------*/

#define DC_UNLOCK				0x8300  /* lock register                */
#define DC_GENERAL_CFG			0x8304  /* config registers...          */
#define DC_TIMING_CFG			0x8308
#define DC_OUTPUT_CFG			0x830C

#define DC_FB_ST_OFFSET			0x8310  /* framebuffer start offset     */
#define DC_CB_ST_OFFSET			0x8314  /* compression start offset     */
#define DC_CURS_ST_OFFSET		0x8318  /* cursor start offset          */
#define DC_ICON_ST_OFFSET		0x831C  /* icon start offset            */
#define DC_VID_ST_OFFSET		0x8320  /* video start offset           */
#define DC_LINE_DELTA			0x8324  /* fb and cb skip counts        */
#define DC_BUF_SIZE				0x8328  /* fb and cb line size          */

#define DC_H_TIMING_1			0x8330  /* horizontal timing...         */
#define DC_H_TIMING_2			0x8334
#define DC_H_TIMING_3			0x8338
#define DC_FP_H_TIMING			0x833C

#define DC_V_TIMING_1			0x8340  /* vertical timing...           */
#define DC_V_TIMING_2			0x8344
#define DC_V_TIMING_3			0x8348
#define DC_FP_V_TIMING			0x834C

#define DC_CURSOR_X				0x8350  /* cursor x position            */
#define DC_ICON_X				0x8354  /* HACK - 1.3 definition        */
#define DC_V_LINE_CNT			0x8354  /* vertical line counter        */
#define DC_CURSOR_Y				0x8358  /* cursor y position            */
#define DC_ICON_Y				0x835C  /* HACK - 1.3 definition        */
#define DC_SS_LINE_CMP			0x835C  /* line compare value           */
#define DC_CURSOR_COLOR			0x8360  /* cursor colors                */
#define DC_ICON_COLOR			0x8364  /* icon colors                  */
#define DC_BORDER_COLOR			0x8368  /* border color                 */
#define DC_PAL_ADDRESS			0x8370  /* palette address              */
#define DC_PAL_DATA				0x8374  /* palette data                 */
#define DC_DFIFO_DIAG			0x8378  /* display FIFO diagnostic      */
#define DC_CFIFO_DIAG			0x837C  /* compression FIF0 diagnostic  */

/* PALETTE LOCATIONS */

#define PAL_CURSOR_COLOR_0		0x100
#define PAL_CURSOR_COLOR_1		0x101
#define PAL_ICON_COLOR_0		0x102
#define PAL_ICON_COLOR_1		0x103
#define PAL_OVERSCAN_COLOR		0x104

/* UNLOCK VALUE */

#define DC_UNLOCK_VALUE		0x00004758      /* used to unlock DC regs   */

/* "DC_GENERAL_CFG" BIT DEFINITIONS */

#define DC_GCFG_DFLE		0x00000001      /* display FIFO load enable */
#define DC_GCFG_CURE		0x00000002      /* cursor enable            */
#define DC_GCFG_VCLK_DIV	0x00000004      /* vid clock divisor        */
#define DC_GCFG_PLNO		0x00000004      /* planar offset LSB        */
#define DC_GCFG_PPC			0x00000008      /* pixel pan compatibility  */
#define DC_GCFG_CMPE		0x00000010      /* compression enable       */
#define DC_GCFG_DECE		0x00000020      /* decompression enable     */
#define DC_GCFG_DCLK_MASK	0x000000C0      /* dotclock multiplier      */
#define DC_GCFG_DCLK_POS	6       /* dotclock multiplier      */
#define DC_GCFG_DFHPSL_MASK	0x00000F00      /* FIFO high-priority start */
#define DC_GCFG_DFHPSL_POS	8       /* FIFO high-priority start */
#define DC_GCFG_DFHPEL_MASK	0x0000F000      /* FIFO high-priority end   */
#define DC_GCFG_DFHPEL_POS	12      /* FIFO high-priority end   */
#define DC_GCFG_CIM_MASK	0x00030000      /* compressor insert mode   */
#define DC_GCFG_CIM_POS		16      /* compressor insert mode   */
#define DC_GCFG_FDTY		0x00040000      /* frame dirty mode         */
#define DC_GCFG_RTPM		0x00080000      /* real-time perf. monitor  */
#define DC_GCFG_DAC_RS_MASK	0x00700000      /* DAC register selects     */
#define DC_GCFG_DAC_RS_POS	20      /* DAC register selects     */
#define DC_GCFG_CKWR		0x00800000      /* clock write              */
#define DC_GCFG_LDBL		0x01000000      /* line double              */
#define DC_GCFG_DIAG		0x02000000      /* FIFO diagnostic mode     */
#define DC_GCFG_CH4S		0x04000000      /* sparse refresh mode      */
#define DC_GCFG_SSLC		0x08000000      /* enable line compare      */
#define DC_GCFG_VIDE		0x10000000      /* video enable             */
#define DC_GCFG_DFCK		0x20000000      /* divide flat-panel clock      */
                                                                                        /*   - rev 2.3 down             */
#define DC_GCFG_VRDY		0x20000000      /* video port speed             */
                                                                                        /*   - rev 2.4 up                       */
#define DC_GCFG_DPCK		0x40000000      /* divide pixel clock       */
#define DC_GCFG_DDCK		0x80000000      /* divide dot clock         */

/* "DC_TIMING_CFG" BIT DEFINITIONS */

#define DC_TCFG_FPPE		0x00000001      /* flat-panel power enable  */
#define DC_TCFG_HSYE		0x00000002      /* horizontal sync enable   */
#define DC_TCFG_VSYE		0x00000004      /* vertical sync enable     */
#define DC_TCFG_BLKE		0x00000008      /* blank enable             */
#define DC_TCFG_DDCK		0x00000010      /* DDC clock                */
#define DC_TCFG_TGEN		0x00000020      /* timing generator enable  */
#define DC_TCFG_VIEN		0x00000040      /* vertical interrupt enable */
#define DC_TCFG_BLNK		0x00000080      /* blink enable             */
#define DC_TCFG_CHSP		0x00000100      /* horizontal sync polarity */
#define DC_TCFG_CVSP		0x00000200      /* vertical sync polarity   */
#define DC_TCFG_FHSP		0x00000400      /* panel horz sync polarity */
#define DC_TCFG_FVSP		0x00000800      /* panel vert sync polarity */
#define DC_TCFG_FCEN		0x00001000      /* flat-panel centering     */
#define DC_TCFG_CDCE		0x00002000      /* HACK - 1.3 definition    */
#define DC_TCFG_PLNR		0x00002000      /* planar mode enable       */
#define DC_TCFG_INTL		0x00004000      /* interlace scan           */
#define DC_TCFG_PXDB		0x00008000      /* pixel double             */
#define DC_TCFG_BKRT		0x00010000      /* blink rate               */
#define DC_TCFG_PSD_MASK	0x000E0000      /* power sequence delay     */
#define DC_TCFG_PSD_POS		17      /* power sequence delay     */
#define DC_TCFG_DDCI		0x08000000      /* DDC input (RO)           */
#define DC_TCFG_SENS		0x10000000      /* monitor sense (RO)       */
#define DC_TCFG_DNA			0x20000000      /* display not active (RO)  */
#define DC_TCFG_VNA			0x40000000      /* vertical not active (RO) */
#define DC_TCFG_VINT		0x80000000      /* vertical interrupt (RO)  */

/* "DC_OUTPUT_CFG" BIT DEFINITIONS */

#define DC_OCFG_8BPP		0x00000001      /* 8/16 bpp select          */
#define DC_OCFG_555			0x00000002      /* 16 bpp format            */
#define DC_OCFG_PCKE		0x00000004      /* PCLK enable              */
#define DC_OCFG_FRME		0x00000008      /* frame rate mod enable    */
#define DC_OCFG_DITE		0x00000010      /* dither enable            */
#define DC_OCFG_2PXE		0x00000020      /* 2 pixel enable           */
#define DC_OCFG_2XCK		0x00000040      /* 2 x pixel clock          */
#define DC_OCFG_2IND		0x00000080      /* 2 index enable           */
#define DC_OCFG_34ADD		0x00000100      /* 3- or 4-bit add          */
#define DC_OCFG_FRMS		0x00000200      /* frame rate mod select    */
#define DC_OCFG_CKSL		0x00000400      /* clock select             */
#define DC_OCFG_PRMP		0x00000800      /* palette re-map           */
#define DC_OCFG_PDEL		0x00001000      /* panel data enable low    */
#define DC_OCFG_PDEH		0x00002000      /* panel data enable high   */
#define DC_OCFG_CFRW		0x00004000      /* comp line buffer r/w sel */
#define DC_OCFG_DIAG		0x00008000      /* comp line buffer diag    */

#define MC_MEM_CNTRL1       0x00008400
#define MC_DR_ADD			0x00008418
#define MC_DR_ACC			0x0000841C

/* MC_MEM_CNTRL1 BIT DEFINITIONS */

#define MC_XBUSARB          0x00000008  /* 0 = GP priority < CPU priority */
                                                                                        /* 1 = GP priority = CPU priority */
                                                                                        /* GXm databook V2.0 is wrong ! */
/*----------*/
/*  CS5530  */
/*----------*/

/* CS5530 REGISTER DEFINITIONS */

#define CS5530_VIDEO_CONFIG 		0x0000
#define CS5530_DISPLAY_CONFIG       0x0004
#define CS5530_VIDEO_X_POS          0x0008
#define CS5530_VIDEO_Y_POS          0x000C
#define CS5530_VIDEO_SCALE          0x0010
#define CS5530_VIDEO_COLOR_KEY		0x0014
#define CS5530_VIDEO_COLOR_MASK		0x0018
#define CS5530_PALETTE_ADDRESS 		0x001C
#define CS5530_PALETTE_DATA	 		0x0020
#define CS5530_DOT_CLK_CONFIG       0x0024
#define CS5530_CRCSIG_TFT_TV        0x0028

/* "CS5530_VIDEO_CONFIG" BIT DEFINITIONS */

#define CS5530_VCFG_VID_EN					0x00000001
#define CS5530_VCFG_VID_REG_UPDATE			0x00000002
#define CS5530_VCFG_VID_INP_FORMAT			0x0000000C
#define CS5530_VCFG_8_BIT_4_2_0				0x00000004
#define CS5530_VCFG_16_BIT_4_2_0			0x00000008
#define CS5530_VCFG_GV_SEL					0x00000010
#define CS5530_VCFG_CSC_BYPASS				0x00000020
#define CS5530_VCFG_X_FILTER_EN				0x00000040
#define CS5530_VCFG_Y_FILTER_EN				0x00000080
#define CS5530_VCFG_LINE_SIZE_LOWER_MASK	0x0000FF00
#define CS5530_VCFG_INIT_READ_MASK			0x01FF0000
#define CS5530_VCFG_EARLY_VID_RDY  			0x02000000
#define CS5530_VCFG_LINE_SIZE_UPPER			0x08000000
#define CS5530_VCFG_4_2_0_MODE				0x10000000
#define CS5530_VCFG_16_BIT_EN				0x20000000
#define CS5530_VCFG_HIGH_SPD_INT			0x40000000

/* "CS5530_DISPLAY_CONFIG" BIT DEFINITIONS */

#define CS5530_DCFG_DIS_EN					0x00000001
#define CS5530_DCFG_HSYNC_EN				0x00000002
#define CS5530_DCFG_VSYNC_EN				0x00000004
#define CS5530_DCFG_DAC_BL_EN				0x00000008
#define CS5530_DCFG_DAC_PWDNX				0x00000020
#define CS5530_DCFG_FP_PWR_EN				0x00000040
#define CS5530_DCFG_FP_DATA_EN				0x00000080
#define CS5530_DCFG_CRT_HSYNC_POL 			0x00000100
#define CS5530_DCFG_CRT_VSYNC_POL 			0x00000200
#define CS5530_DCFG_FP_HSYNC_POL  			0x00000400
#define CS5530_DCFG_FP_VSYNC_POL  			0x00000800
#define CS5530_DCFG_XGA_FP		  			0x00001000
#define CS5530_DCFG_FP_DITH_EN				0x00002000
#define CS5530_DCFG_CRT_SYNC_SKW_MASK		0x0001C000
#define CS5530_DCFG_CRT_SYNC_SKW_INIT		0x00010000
#define CS5530_DCFG_PWR_SEQ_DLY_MASK		0x000E0000
#define CS5530_DCFG_PWR_SEQ_DLY_INIT		0x00080000
#define CS5530_DCFG_VG_CK					0x00100000
#define CS5530_DCFG_GV_PAL_BYP				0x00200000
#define CS5530_DCFG_DDC_SCL					0x00400000
#define CS5530_DCFG_DDC_SDA					0x00800000
#define CS5530_DCFG_DDC_OE					0x01000000
#define CS5530_DCFG_16_BIT_EN				0x02000000

/*----------*/
/*  SC1200  */
/*----------*/

/* SC1200 VIDEO REGISTER DEFINITIONS */

#define SC1200_VIDEO_CONFIG 				0x000
#define SC1200_DISPLAY_CONFIG				0x004
#define SC1200_VIDEO_X_POS					0x008
#define SC1200_VIDEO_Y_POS					0x00C
#define SC1200_VIDEO_UPSCALE				0x010
#define SC1200_VIDEO_COLOR_KEY				0x014
#define SC1200_VIDEO_COLOR_MASK				0x018
#define SC1200_PALETTE_ADDRESS 				0x01C
#define SC1200_PALETTE_DATA	 				0x020
#define SC1200_VID_MISC						0x028
#define SC1200_VID_CLOCK_SELECT				0x02C
#define SC1200_VIDEO_DOWNSCALER_CONTROL     0x03C
#define SC1200_VIDEO_DOWNSCALER_COEFFICIENTS 0x40
#define SC1200_VID_CRC						0x044
#define SC1200_DEVICE_ID					0x048
#define SC1200_VID_ALPHA_CONTROL			0x04C
#define SC1200_CURSOR_COLOR_KEY				0x050
#define SC1200_CURSOR_COLOR_MASK			0x054
#define SC1200_CURSOR_COLOR_1				0x058
#define SC1200_CURSOR_COLOR_2				0x05C
#define SC1200_ALPHA_XPOS_1					0x060
#define SC1200_ALPHA_YPOS_1					0x064
#define SC1200_ALPHA_COLOR_1				0x068
#define SC1200_ALPHA_CONTROL_1				0x06C
#define SC1200_ALPHA_XPOS_2					0x070
#define SC1200_ALPHA_YPOS_2					0x074
#define SC1200_ALPHA_COLOR_2				0x078
#define SC1200_ALPHA_CONTROL_2				0x07C
#define SC1200_ALPHA_XPOS_3					0x080
#define SC1200_ALPHA_YPOS_3					0x084
#define SC1200_ALPHA_COLOR_3				0x088
#define SC1200_ALPHA_CONTROL_3				0x08C
#define SC1200_VIDEO_REQUEST                0x090
#define SC1200_ALPHA_WATCH 					0x094
#define SC1200_VIDEO_DISPLAY_MODE           0x400
#define SC1200_VIDEO_ODD_VBI_LINE_ENABLE    0x40C
#define SC1200_VIDEO_EVEN_VBI_LINE_ENABLE   0x410
#define SC1200_VIDEO_VBI_HORIZ_CONTROL      0x414
#define SC1200_VIDEO_ODD_VBI_TOTAL_COUNT    0x418
#define SC1200_VIDEO_EVEN_VBI_TOTAL_COUNT   0x41C
#define SC1200_GENLOCK                      0x420
#define SC1200_GENLOCK_DELAY                0x424
#define SC1200_TVOUT_HORZ_TIM				0x800
#define SC1200_TVOUT_HORZ_SYNC				0x804
#define SC1200_TVOUT_VERT_SYNC				0x808
#define SC1200_TVOUT_LINE_END				0x80C
#define SC1200_TVOUT_VERT_DOWNSCALE			0x810   /* REV. A & B */
#define SC1200_TVOUT_HORZ_PRE_ENCODER_SCALE 0x810       /* REV. C */
#define SC1200_TVOUT_HORZ_SCALING			0x814
#define SC1200_TVOUT_DEBUG                  0x818
#define SC1200_TVENC_TIM_CTRL_1				0xC00
#define SC1200_TVENC_TIM_CTRL_2				0xC04
#define SC1200_TVENC_TIM_CTRL_3				0xC08
#define SC1200_TVENC_SUB_FREQ				0xC0C
#define SC1200_TVENC_DISP_POS				0xC10
#define SC1200_TVENC_DISP_SIZE				0xC14
#define SC1200_TVENC_CC_DATA				0xC18
#define SC1200_TVENC_EDS_DATA				0xC1C
#define SC1200_TVENC_CGMS_DATA				0xC20
#define SC1200_TVENC_WSS_DATA				0xC24
#define SC1200_TVENC_CC_CONTROL				0xC28
#define SC1200_TVENC_DAC_CONTROL			0xC2C
#define SC1200_TVENC_MV_CONTROL             0xC30

/* "SC1200_VIDEO_CONFIG" BIT DEFINITIONS */

#define SC1200_VCFG_VID_EN					0x00000001
#define SC1200_VCFG_VID_INP_FORMAT			0x0000000C
#define SC1200_VCFG_UYVY_FORMAT				0x00000000
#define SC1200_VCFG_Y2YU_FORMAT				0x00000004
#define SC1200_VCFG_YUYV_FORMAT				0x00000008
#define SC1200_VCFG_YVYU_FORMAT				0x0000000C
#define SC1200_VCFG_X_FILTER_EN				0x00000040
#define SC1200_VCFG_Y_FILTER_EN				0x00000080
#define SC1200_VCFG_LINE_SIZE_LOWER_MASK	0x0000FF00
#define SC1200_VCFG_INIT_READ_MASK			0x01FF0000
#define SC1200_VCFG_LINE_SIZE_UPPER			0x08000000
#define SC1200_VCFG_4_2_0_MODE				0x10000000

/* "SC1200_DISPLAY_CONFIG" BIT DEFINITIONS */

#define SC1200_DCFG_DIS_EN					0x00000001
#define SC1200_DCFG_HSYNC_EN				0x00000002
#define SC1200_DCFG_VSYNC_EN				0x00000004
#define SC1200_DCFG_DAC_BL_EN				0x00000008
#define SC1200_DCFG_FP_PWR_EN				0x00000040
#define SC1200_DCFG_FP_DATA_EN				0x00000080
#define SC1200_DCFG_CRT_HSYNC_POL 			0x00000100
#define SC1200_DCFG_CRT_VSYNC_POL 			0x00000200
#define SC1200_DCFG_CRT_SYNC_SKW_MASK		0x0001C000
#define SC1200_DCFG_CRT_SYNC_SKW_INIT		0x00010000
#define SC1200_DCFG_PWR_SEQ_DLY_MASK		0x000E0000
#define SC1200_DCFG_PWR_SEQ_DLY_INIT		0x00080000
#define SC1200_DCFG_VG_CK					0x00100000
#define SC1200_DCFG_GV_PAL_BYP				0x00200000
#define SC1200_DCFG_DDC_SCL					0x00400000
#define SC1200_DCFG_DDC_SDA					0x00800000
#define SC1200_DCFG_DDC_OE					0x01000000

/* "SC1200_VID_MISC" BIT DEFINITIONS */

#define SC1200_GAMMA_BYPASS_BOTH            0x00000001
#define SC1200_DAC_POWER_DOWN               0x00000400
#define SC1200_ANALOG_POWER_DOWN            0x00000800
#define SC1200_PLL_POWER_NORMAL             0x00001000

/* "SC1200_VIDEO_DOWNSCALER_CONTROL" BIT DEFINITIONS */

#define SC1200_VIDEO_DOWNSCALE_ENABLE       0x00000001
#define SC1200_VIDEO_DOWNSCALE_FACTOR_POS   1
#define SC1200_VIDEO_DOWNSCALE_FACTOR_MASK  0x0000001E
#define SC1200_VIDEO_DOWNSCALE_TYPE_A       0x00000000
#define SC1200_VIDEO_DOWNSCALE_TYPE_B       0x00000040
#define SC1200_VIDEO_DOWNSCALE_TYPE_MASK    0x00000040

/* "SC1200_VIDEO_DOWNSCALER_COEFFICIENTS" BIT DEFINITIONS */

#define SC1200_VIDEO_DOWNSCALER_COEF1_POS   0
#define SC1200_VIDEO_DOWNSCALER_COEF2_POS   8
#define SC1200_VIDEO_DOWNSCALER_COEF3_POS   16
#define SC1200_VIDEO_DOWNSCALER_COEF4_POS   24
#define SC1200_VIDEO_DOWNSCALER_COEF_MASK   0xF

/* VIDEO DE-INTERLACING AND ALPHA CONTROL (REGISTER 0x4C) */

#define SC1200_VERTICAL_SCALER_SHIFT_MASK   0x00000007
#define SC1200_VERTICAL_SCALER_SHIFT_INIT   0x00000004
#define SC1200_VERTICAL_SCALER_SHIFT_EN     0x00000010
#define SC1200_TOP_LINE_IN_ODD              0x00000040
#define SC1200_NO_CK_OUTSIDE_ALPHA          0x00000100
#define SC1200_VIDEO_IS_INTERLACED          0x00000200
#define SC1200_CSC_VIDEO_YUV_TO_RGB         0x00000400
#define SC1200_CSC_GFX_RGB_TO_YUV           0x00000800
#define SC1200_VIDEO_INPUT_IS_RGB           0x00002000
#define SC1200_VIDEO_LINE_OFFSET_ODD        0x00001000
#define SC1200_ALPHA1_PRIORITY_POS			16
#define SC1200_ALPHA1_PRIORITY_MASK			0x00030000
#define SC1200_ALPHA2_PRIORITY_POS			18
#define SC1200_ALPHA2_PRIORITY_MASK			0x000C0000
#define SC1200_ALPHA3_PRIORITY_POS			20
#define SC1200_ALPHA3_PRIORITY_MASK			0x00300000

/* VIDEO CURSOR COLOR KEY DEFINITIONS (REGISTER 0x50) */

#define SC1200_CURSOR_COLOR_KEY_OFFSET_POS  24
#define SC1200_CURSOR_COLOR_BITS            23
#define SC1200_COLOR_MASK                   0x00FFFFFF  /* 24 significant 
                                                         * bits */

/* ALPHA COLOR BIT DEFINITION (REGISTERS 0x68, 0x78, AND 0x88) */

#define SC1200_ALPHA_COLOR_ENABLE           0x01000000

/* ALPHA CONTROL BIT DEFINITIONS (REGISTERS 0x6C, 0x7C, AND 0x8C) */

#define SC1200_ACTRL_WIN_ENABLE				0x00010000
#define SC1200_ACTRL_LOAD_ALPHA				0x00020000

/* VIDEO REQUEST DEFINITIONS (REGISTER 0x90) */

#define SC1200_VIDEO_Y_REQUEST_POS          0
#define SC1200_VIDEO_X_REQUEST_POS          16
#define SC1200_VIDEO_REQUEST_MASK           0x00000FFF

/* VIDEO DISPLAY MODE (REGISTER 0x400) */

#define SC1200_VIDEO_SOURCE_MASK            0x00000003
#define SC1200_VIDEO_SOURCE_GX1             0x00000000
#define SC1200_VIDEO_SOURCE_DVIP            0x00000002
#define SC1200_VBI_SOURCE_MASK              0x00000004
#define SC1200_VBI_SOURCE_DVIP              0x00000000
#define SC1200_VBI_SOURCE_GX1               0x00000004

/* ODD/EVEN VBI LINE ENABLE (REGISTERS 0x40C, 0x410) */

#define SC1200_VIDEO_VBI_LINE_ENABLE_MASK   0x00FFFFFC
#define SC1200_VIDEO_ALL_ACTIVE_IS_VBI      0x01000000
#define SC1200_VIDEO_VBI_LINE_OFFSET_POS    25
#define SC1200_VIDEO_VBI_LINE_OFFSET_MASK   0x3E000000

/* ODD/EVEN VBI TOTAL COUNT (REGISTERS 0x418, 0x41C) */

#define SC1200_VIDEO_VBI_TOTAL_COUNT_MASK   0x000FFFFF

/* GENLOCK BIT DEFINITIONS */

#define SC1200_GENLOCK_SINGLE_ENABLE              0x00000001
#define SC1200_GENLOCK_FIELD_SYNC_ENABLE          0x00000001
#define SC1200_GENLOCK_CONTINUOUS_ENABLE          0x00000002
#define SC1200_GENLOCK_GX_VSYNC_FALLING_EDGE      0x00000004
#define SC1200_GENLOCK_VIP_VSYNC_FALLING_EDGE     0x00000008
#define SC1200_GENLOCK_TIMEOUT_ENABLE             0x00000010
#define SC1200_GENLOCK_TVENC_RESET_EVEN_FIELD     0x00000020
#define SC1200_GENLOCK_TVENC_RESET_BEFORE_DELAY   0x00000040
#define SC1200_GENLOCK_TVENC_RESET_ENABLE         0x00000080
#define SC1200_GENLOCK_SYNC_TO_TVENC              0x00000100
#define SC1200_GENLOCK_DELAY_MASK                 0x001FFFFF

/* TVOUT HORIZONTAL PRE ENCODER SCALE BIT DEFINITIONS */

#define SC1200_TVOUT_YC_DELAY_MASK                0x00C00000
#define SC1200_TVOUT_YC_DELAY_NONE                0x00000000
#define SC1200_TVOUT_Y_DELAY_ONE_PIXEL            0x00400000
#define SC1200_TVOUT_C_DELAY_ONE_PIXEL            0x00800000
#define SC1200_TVOUT_C_DELAY_TWO_PIXELS           0x00C00000

/* TVOUT HORIZONTAL SCALING/CONTROL BIT DEFINITIONS */

#define SC1200_TVOUT_FLICKER_FILTER_MASK               	0x60000000
#define SC1200_TVOUT_FLICKER_FILTER_FOURTH_HALF_FOURTH 	0x00000000
#define SC1200_TVOUT_FLICKER_FILTER_HALF_ONE_HALF      	0x20000000
#define SC1200_TVOUT_FLICKER_FILTER_DISABLED           	0x40000000
#define SC1200_TVENC_EXTERNAL_RESET_INTERVAL_MASK      	0x0F000000
#define SC1200_TVENC_EXTERNAL_RESET_EVERY_ODD_FIELD    	0x00000000
#define SC1200_TVENC_EXTERNAL_RESET_EVERY_EVEN_FIELD   	0x02000000
#define SC1200_TVENC_EXTERNAL_RESET_NEXT_ODD_FIELD     	0x05000000
#define SC1200_TVENC_EXTERNAL_RESET_NEXT_EVEN_FIELD    	0x07000000
#define SC1200_TVENC_EXTERNAL_RESET_EVERY_FIELD        	0x0E000000
#define SC1200_TVENC_EXTERNAL_RESET_EVERY_X_ODD_FIELDS  0x08000000
#define SC1200_TVENC_EXTERNAL_RESET_EVERY_X_EVEN_FIELDS 0x0A000000

/* TVOUT DEBUG BIT DEFINITIONS */

#define SC1200_TVOUT_FIELD_STATUS_EVEN         	0x00000040
#define SC1200_TVOUT_FIELD_STATUS_TV           	0x00000080
#define SC1200_TVOUT_CRT_VSYNC_STATUS_TRAILING 	0x00000100
#define SC1200_TVOUT_FIELD_STATUS_INVERT      	0x00000200
#define SC1200_TVOUT_CONVERTER_INTERPOLATION   	0x00000400

/* TVENC TIMING/CONTROL 1 BIT DEFINITIONS (REGISTER 0xC00) */

#define SC1200_TVENC_VPHASE_MASK                          0x001FF800
#define SC1200_TVENC_VPHASE_POS                           11
#define SC1200_TVENC_SUB_CARRIER_RESET_MASK               0x30000000
#define SC1200_TVENC_SUB_CARRIER_RESET_NEVER              0x00000000
#define SC1200_TVENC_SUB_CARRIER_RESET_EVERY_TWO_LINES    0x10000000
#define SC1200_TVENC_SUB_CARRIER_RESET_EVERY_TWO_FRAMES   0x20000000
#define SC1200_TVENC_SUB_CARRIER_RESET_EVERY_FOUR_FRAMES  0x30000000
#define SC1200_TVENC_VIDEO_TIMING_ENABLE                  0x80000000

/* TVENC TIMING/CONTROL 2 BIT DEFINITIONS (REGISTER 0xC04) */

#define SC1200_TVENC_OUTPUT_YCBCR                         0x40000000
#define SC1200_TVENC_CFS_MASK                             0x00030000
#define SC1200_TVENC_CFS_BYPASS                           0x00000000
#define SC1200_TVENC_CFS_CVBS                             0x00020000
#define SC1200_TVENC_CFS_SVIDEO                           0x00030000

/* TVENC TIMING/CONTROL 3 BIT DEFINITIONS (REGISTER 0xC08) */

#define SC1200_TVENC_CS                                   0x00000001
#define SC1200_TVENC_SYNCMODE_MASK                        0x00000006
#define SC1200_TVENC_SYNC_ON_GREEN                        0x00000002
#define SC1200_TVENC_SYNC_ON_CVBS                         0x00000004
#define SC1200_TVENC_CM                                   0x00000008

/* TVENC DAC CONTROL BIT DEFINITIONS (REGISTER 0xC2C) */
#define SC1200_TVENC_TRIM_MASK	               0x00000007
#define SC1200_TVENC_POWER_DOWN	               0x00000020

/* TVENC MV CONTROL BIT DEFINITIONS (REGISTER 0xC30) */
#define SC1200_TVENC_MV_ENABLE                 0xBE

/* SC1200 VIP REGISTER DEFINITIONS */

#define SC1200_VIP_CONFIG					0x00000000
#define SC1200_VIP_CONTROL					0x00000004
#define SC1200_VIP_STATUS					0x00000008
#define SC1200_VIP_CURRENT_LINE				0x00000010
#define SC1200_VIP_LINE_TARGET				0x00000014
#define SC1200_ODD_DIRECT_VBI_LINE_ENABLE   0x00000018
#define SC1200_EVEN_DIRECT_VBI_LINE_ENABLE  0x0000001C
#define SC1200_VIP_ODD_BASE					0x00000020
#define SC1200_VIP_EVEN_BASE				0x00000024
#define SC1200_VIP_PITCH					0x00000028
#define SC1200_VBI_ODD_BASE					0x00000040
#define SC1200_VBI_EVEN_BASE				0x00000044
#define SC1200_VBI_PITCH					0x00000048

/* "SC1200_VIP_CONFIG" BIT DEFINITIONS */

#define SC1200_VIP_MODE_MASK                0x00000003
#define	SC1200_VIP_MODE_C       			0x00000002
#define SC1200_VBI_ANCILLARY_TO_MEMORY      0x000C0000
#define SC1200_VBI_TASK_A_TO_MEMORY         0x00140000
#define SC1200_VBI_TASK_B_TO_MEMORY         0x00240000
#define SC1200_VIP_BUS_REQUEST_THRESHOLD    0x00400000

/* "SC1200_VIP_CONTROL" BIT DEFINITIONS */

#define SC1200_CAPTURE_RUN_MODE_MASK        0x00000003
#define SC1200_CAPTURE_RUN_MODE_STOP_LINE   0x00000000
#define SC1200_CAPTURE_RUN_MODE_STOP_FIELD  0x00000001
#define SC1200_CAPTURE_RUN_MODE_START       0x00000003
#define	SC1200_VIP_DATA_CAPTURE_EN			0x00000100
#define	SC1200_VIP_VBI_CAPTURE_EN			0x00000200
#define	SC1200_VIP_VBI_FIELD_INTERRUPT_EN	0x00010000

/* "SC1200_VIP_STATUS" BIT DEFINITIONS */

#define	SC1200_VIP_CURRENT_FIELD_ODD		0x01000000
#define SC1200_VIP_BASE_NOT_UPDATED         0x00200000
#define	SC1200_VIP_FIFO_OVERFLOW			0x00100000
#define	SC1200_VIP_CLEAR_LINE_INT			0x00020000
#define	SC1200_VIP_CLEAR_FIELD_INT			0x00010000
#define	SC1200_VBI_DATA_CAPTURE_ACTIVE		0x00000200
#define	SC1200_VIDEO_DATA_CAPTURE_ACTIVE	0x00000100

/* "SC1200_VIP_CURRENT_LINE" BIT DEFINITIONS */

#define SC1200_VIP_CURRENT_LINE_MASK	    0x000003FF

/* "SC1200_VIP_LINE_TARGET" BIT DEFINITIONS */

#define SC1200_VIP_LAST_LINE_MASK	        0x03FF0000

/* "SC1200_VIP_PITCH" BIT DEFINITION */

#define SC1200_VIP_PITCH_MASK               0x0000FFFC

/* "SC1200_VBI_PITCH" BIT DEFINITION */

#define SC1200_VBI_PITCH_MASK               0x0000FFFC

/* SC1200 DIRECT VBI LINE ENABLE BIT DEFINITION */

#define SC1200_DIRECT_VBI_LINE_ENABLE_MASK  0x00FFFFFF

/* SC1200 CONFIGURATION BLOCK */

#define SC1200_CB_BASE_ADDR                 0x9000
#define SC1200_CB_WDTO                      0x0000
#define SC1200_CB_WDCNFG                    0x0002
#define SC1200_CB_WDSTS                     0x0004
#define SC1200_CB_TMVALUE                   0x0008
#define SC1200_CB_TMCNFG                    0x000D
#define SC1200_CB_CCFC                      0x001E
#define SC1200_CB_PMR                       0x0030
#define SC1200_CB_MCR                       0x0034
#define SC1200_CB_INTSEL                    0x0038
#define SC1200_CB_PID                       0x003C
#define SC1200_CB_REV                       0x003D

/* SC1200 HIGH RESOLUTION TIMER CONFIGURATION REGISTER BITS */

#define SC1200_TMCLKSEL_27MHZ               0x2

/*---------------------------------*/
/*  PHILIPS SAA7114 VIDEO DECODER  */
/*---------------------------------*/

#define SAA7114_CHIPADDR					0x42

/* VIDEO DECODER REGISTER DEFINITIONS */

#define SAA7114_ANALOG_INPUT_CTRL1			0x02
#define SAA7114_LUMINANCE_CONTROL           0x09
#define SAA7114_BRIGHTNESS					0x0A
#define SAA7114_CONTRAST					0x0B
#define SAA7114_SATURATION					0x0C
#define SAA7114_HUE							0x0D
#define SAA7114_STATUS						0x1F
#define SAA7114_IPORT_CONTROL				0x86

/* TASK A REGISTER DEFINITIONS */

#define SAA7114_TASK_A_HORZ_OUTPUT_LO		0x9C
#define SAA7114_TASK_A_HORZ_OUTPUT_HI		0x9D
#define SAA7114_TASK_A_HSCALE_LUMA_LO		0xA8
#define SAA7114_TASK_A_HSCALE_LUMA_HI		0xA9
#define SAA7114_TASK_A_HSCALE_CHROMA_LO		0xAC
#define SAA7114_TASK_A_HSCALE_CHROMA_HI		0xAD

/* TASK B REGISTER DEFINITIONS */

#define SAA7114_HORZ_OFFSET_LO				0xC4
#define SAA7114_HORZ_OFFSET_HI				0xC5
#define SAA7114_HORZ_INPUT_LO				0xC6
#define SAA7114_HORZ_INPUT_HI				0xC7
#define SAA7114_VERT_OFFSET_LO				0xC8
#define SAA7114_VERT_OFFSET_HI				0xC9
#define SAA7114_VERT_INPUT_LO				0xCA
#define SAA7114_VERT_INPUT_HI				0xCB
#define SAA7114_HORZ_OUTPUT_LO				0xCC
#define SAA7114_HORZ_OUTPUT_HI				0xCD
#define SAA7114_VERT_OUTPUT_LO				0xCE
#define SAA7114_VERT_OUTPUT_HI				0xCF
#define SAA7114_HORZ_PRESCALER				0xD0
#define SAA7114_HORZ_ACL					0xD1
#define SAA7114_HORZ_FIR_PREFILTER			0xD2
#define SAA7114_FILTER_CONTRAST				0xD5
#define SAA7114_FILTER_SATURATION			0xD6
#define SAA7114_HSCALE_LUMA_LO				0xD8
#define SAA7114_HSCALE_LUMA_HI				0xD9
#define SAA7114_HSCALE_CHROMA_LO			0xDC
#define SAA7114_HSCALE_CHROMA_HI			0xDD
#define SAA7114_VSCALE_LUMA_LO				0xE0
#define SAA7114_VSCALE_LUMA_HI				0xE1
#define SAA7114_VSCALE_CHROMA_LO			0xE2
#define SAA7114_VSCALE_CHROMA_HI			0xE3
#define SAA7114_VSCALE_CONTROL				0xE4
#define SAA7114_VSCALE_CHROMA_OFFS0			0xE8
#define SAA7114_VSCALE_CHROMA_OFFS1			0xE9
#define SAA7114_VSCALE_CHROMA_OFFS2			0xEA
#define SAA7114_VSCALE_CHROMA_OFFS3			0xEB
#define SAA7114_VSCALE_LUMINA_OFFS0			0xEC
#define SAA7114_VSCALE_LUMINA_OFFS1			0xED
#define SAA7114_VSCALE_LUMINA_OFFS2			0xEE
#define SAA7114_VSCALE_LUMINA_OFFS3			0xEF

/* Still need to determine PHO value (common phase offset) */
#define SAA7114_VSCALE_PHO					0x00

/*----------------------------------------------*/
/*  SECOND GENERATION GRAPHICS UNIT (REDCLOUD)  */
/*----------------------------------------------*/

#define MGP_DST_OFFSET			0x0000  /* dst address                  */
#define MGP_SRC_OFFSET			0x0004  /* src address                  */
#define MGP_VEC_ERR				0x0004  /* vector diag/axial errors     */
#define MGP_STRIDE				0x0008  /* src and dst strides          */
#define MGP_WID_HEIGHT			0x000C  /* width and height of BLT      */
#define MGP_VEC_LEN				0x000C  /* vector length/init error             */
#define MGP_SRC_COLOR_FG		0x0010  /* src mono data fgcolor        */
#define MGP_SRC_COLOR_BG		0x0014  /* src mono data bkcolor        */
#define MGP_PAT_COLOR_0			0x0018  /* pattern color 0              */
#define MGP_PAT_COLOR_1			0x001C  /* pattern color 1              */
#define MGP_PAT_COLOR_2			0x0020  /* pattern color 2              */
#define MGP_PAT_COLOR_3			0x0024  /* pattern color 3              */
#define MGP_PAT_COLOR_4			0x0028  /* pattern color 4              */
#define MGP_PAT_COLOR_5			0x002C  /* pattern color 5              */
#define MGP_PAT_DATA_0			0x0030  /* pattern data 0               */
#define MGP_PAT_DATA_1			0x0034  /* pattern data 1               */
#define MGP_RASTER_MODE			0x0038  /* raster operation             */
#define MGP_VECTOR_MODE			0x003C  /* render vector                */
#define MGP_BLT_MODE			0x0040  /* render BLT                   */
#define MGP_BLT_STATUS			0x0044  /* BLT status register          */
#define MGP_RESET				0x0044  /* reset register (write)       */
#define MGP_HST_SOURCE			0x0048  /* host src data (bitmap)       */
#define MGP_BASE_OFFSET			0x004C  /* base render offset           */

/* MGP_RASTER_MODE DEFINITIONS */

#define MGP_RM_BPPFMT_332		    0x00000000  /* 8 BPP, 3:3:2                 */
#define MGP_RM_BPPFMT_4444		    0x40000000  /* 16 BPP, 4:4:4:4              */
#define MGP_RM_BPPFMT_1555		    0x50000000  /* 16 BPP, 1:5:5:5              */
#define MGP_RM_BPPFMT_565		    0x60000000  /* 16 BPP, 5:6:5                */
#define MGP_RM_BPPFMT_8888		    0x80000000  /* 32 BPP, 8:8:8:8              */
#define MGP_RM_ALPHA_EN_MASK        0x00C00000  /* Alpha enable             */
#define MGP_RM_ALPHA_TO_RGB         0x00400000  /* Alpha applies to RGB     */
#define MGP_RM_ALPHA_TO_ALPHA       0x00800000  /* Alpha applies to alpha   */
#define MGP_RM_ALPHA_OP_MASK        0x00300000  /* Alpha operation          */
#define MGP_RM_ALPHA_TIMES_A        0x00000000  /* Alpha * A                */
#define MGP_RM_BETA_TIMES_B         0x00100000  /* (1-alpha) * B            */
#define MGP_RM_A_PLUS_BETA_B        0x00200000  /* A + (1-alpha) * B        */
#define MGP_RM_ALPHA_A_PLUS_BETA_B  0x00300000  /* alpha * A + (1 - alpha)B */
#define MGP_RM_ALPHA_SELECT         0x000E0000  /* Alpha Select             */
#define MGP_RM_SELECT_ALPHA_A       0x00000000  /* Alpha from channel A     */
#define MGP_RM_SELECT_ALPHA_B       0x00020000  /* Alpha from channel B     */
#define MGP_RM_SELECT_ALPHA_R       0x00040000  /* Registered alpha         */
#define MGP_RM_SELECT_ALPHA_1       0x00060000  /* Constant 1               */
#define MGP_RM_SELECT_ALPHA_CHAN_A  0x00080000  /* RGB Values from A        */
#define MGP_RM_SELECT_ALPHA_CHAN_B  0x000A0000  /* RGB Values from B        */
#define MGP_RM_DEST_FROM_CHAN_A     0x00010000  /* Alpha channel select     */
#define MGP_RM_PAT_FLAGS		    0x00000700  /* pattern related bits     */
#define MGP_RM_PAT_MONO			    0x00000100  /* monochrome pattern       */
#define MGP_RM_PAT_COLOR		    0x00000200  /* color pattern            */
#define MGP_RM_PAT_TRANS		    0x00000400  /* pattern transparency     */
#define MGP_RM_SRC_TRANS		    0x00000800  /* source transparency      */

/* MGP_VECTOR_MODE DEFINITIONS */

#define MGP_VM_DST_REQ			0x00000008      /* dst data required        */
#define MGP_VM_THROTTLE			0x00000010      /* sync to VBLANK           */

/* MGP_BLT_MODE DEFINITIONS */

#define MGP_BM_SRC_FB			0x00000001      /* src = frame buffer       */
#define MGP_BM_SRC_HOST			0x00000002      /* src = host register      */
#define MGP_BM_DST_REQ			0x00000004      /* dst data required        */
#define MGP_BM_SRC_MONO			0x00000040      /* monochrome source data   */
#define MGP_BM_SRC_BP_MONO      0x00000080      /* Byte-packed monochrome   */
#define MGP_BM_SRC_TYPE_MASK    0x000000C0      /* Mask for all source fmts */
#define MGP_BM_NEG_YDIR			0x00000100      /* negative Y direction     */
#define MGP_BM_NEG_XDIR			0x00000200      /* negative X direction     */
#define MGP_BM_THROTTLE			0x00000400      /* sync to VBLANK           */

/* MGP_BLT_STATUS DEFINITIONS */

#define MGP_BS_BLT_BUSY			0x00000001      /* GP is not idle           */
#define MGP_BS_BLT_PENDING		0x00000004      /* second BLT is pending    */
#define MGP_BS_HALF_EMPTY		0x00000008      /* src FIFO half empty      */

/* ALPHA BLENDING MODES       */

#define ALPHA_MODE_BLEND        0x00000000

/*---------------------------------------------------*/
/*  SECOND GENERATION DISPLAY CONTROLLER (REDCLOUD)  */
/*---------------------------------------------------*/

#define MDC_UNLOCK              0x00000000      /* Unlock register          */
#define MDC_GENERAL_CFG         0x00000004      /* Config registers         */
#define MDC_DISPLAY_CFG         0x00000008
#define MDC_GFX_SCL             0x0000000C      /* Graphics scaling         */

#define MDC_FB_ST_OFFSET        0x00000010      /* Frame buffer start offset */
#define MDC_CB_ST_OFFSET        0x00000014      /* Compression start offset */
#define MDC_CURS_ST_OFFSET      0x00000018      /* Cursor buffer start          */
                                                                                                /*  offset                              */
#define MDC_ICON_ST_OFFSET      0x0000001C      /* Icon buffer start offset */
#define MDC_VID_Y_ST_OFFSET     0x00000020      /* Video Y Buffer start         */
                                                                                                /*  offset                              */
#define MDC_VID_U_ST_OFFSET     0x00000024      /* Video U Buffer start         */
                                                                                                /*      offset                                  */
#define MDC_VID_V_ST_OFFSET     0x00000028      /* Video V Buffer start         */
                                                                                                /*      offset                                  */
#define MDC_LINE_SIZE           0x00000030      /* Video, CB, and FB line       */
                                                                                                /*      sizes                                   */
#define MDC_GFX_PITCH           0x00000034      /* FB and DB skip counts    */
#define MDC_VID_YUV_PITCH       0x00000038      /* Y, U and V buffer skip       */
                                                                                                /*      counts                                  */

#define MDC_H_ACTIVE_TIMING     0x00000040      /* Horizontal timings       */
#define MDC_H_BLANK_TIMING      0x00000044
#define MDC_H_SYNC_TIMING       0x00000048
#define MDC_V_ACTIVE_TIMING     0x00000050      /* Vertical Timings         */
#define MDC_V_BLANK_TIMING      0x00000054
#define MDC_V_SYNC_TIMING       0x00000058

#define MDC_CURSOR_X            0x00000060      /* Cursor X position        */
#define MDC_CURSOR_Y            0x00000064      /* Cursor Y Position        */
#define MDC_ICON_X              0x00000068      /* Icon X Position          */
#define MDC_LINE_CNT_STATUS     0x0000006C      /* Icon Y Position          */

#define MDC_PAL_ADDRESS         0x00000070      /* Palette Address          */
#define MDC_PAL_DATA            0x00000074      /* Palette Data             */
#define MDC_DFIFO_DIAG          0x00000078      /* Display FIFO diagnostic  */
#define MDC_CFIFO_DIAG          0x0000007C      /* Compression FIFO             */
                                                                                                /*      diagnostic                      */

#define MDC_VID_DS_DELTA        0x00000080      /* Vertical Downscaling         */
                                                                                                /*      fraction                                */

#define MDC_PHY_MEM_OFFSET      0x00000084      /* VG Base Address Register */
#define MDC_DV_CTL              0x00000088      /* Dirty-Valid Control          */
                                                                                                /*      Register                                */
#define MDC_DV_ACC              0x0000008C      /* Dirty-Valid RAM Access   */

/* UNLOCK VALUE */

#define MDC_UNLOCK_VALUE		0x00004758      /* used to unlock DC regs   */

/* VG MBUS DEVICE SMI MSR FIELDS */

#define MDC_VG_BL_MASK            0x00000001
#define MDC_MISC_MASK             0x00000002
#define MDC_ISR0_MASK             0x00000004
#define MDC_VGA_BL_MASK           0x00000008
#define MDC_CRTCIO_MSK            0x00000010
#define MDC_VG_BLANK_SMI          0x00000001
#define MDC_MISC_SMI              0x00000002
#define MDC_ISR0_SMI              0x00000004
#define MDC_VGA_BLANK_SMI         0x00000008
#define MDC_CRTCIO_SMI            0x00000010

/* MDC_GENERAL_CFG BIT FIELDS */

#define MDC_GCFG_DBUG             0x80000000
#define MDC_GCFG_DBSL             0x40000000
#define MDC_GCFG_CFRW             0x20000000
#define MDC_GCFG_DIAG             0x10000000
#define MDC_GCFG_GXRFS4           0x08000000
#define MDC_GCFG_SGFR             0x04000000
#define MDC_GCFG_SGRE             0x02000000
#define MDC_GCFG_SIGE             0x01000000
#define MDC_GCFG_YUVM             0x00100000
#define MDC_GCFG_VDSE             0x00080000
#define MDC_GCFG_VGAFT            0x00040000
#define MDC_GCFG_FDTY             0x00020000
#define MDC_GCFG_STFM             0x00010000
#define MDC_GCFG_DFHPEL_MASK      0x0000F000
#define MDC_GCFG_DFHPSL_MASK      0x00000F00
#define MDC_GCFG_VGAE             0x00000080
#define MDC_GCFG_DECE             0x00000040
#define MDC_GCFG_CMPE             0x00000020
#define MDC_GCFG_VIDE             0x00000008
#define MDC_GCFG_ICNE             0x00000004
#define MDC_GCFG_CURE             0x00000002
#define MDC_GCFG_DFLE             0x00000001

/* MDC_DISPLAY_CFG BIT FIELDS */

#define MDC_DCFG_A20M             0x80000000
#define MDC_DCFG_A18M             0x40000000
#define MDC_DCFG_VISL             0x08000000
#define MDC_DCFG_FRLK             0x04000000
#define MDC_DCFG_PALB             0x02000000
#define MDC_DCFG_PIX_PAN_MASK     0x00F00000
#define MDC_DCFG_DCEN             0x00080000
#define MDC_DCFG_16BPP_MODE_MASK  0x00000C00
#define MDC_DCFG_16BPP            0x00000000
#define MDC_DCFG_15BPP            0x00000400
#define MDC_DCFG_12BPP            0x00000800
#define MDC_DCFG_DISP_MODE_MASK   0x00000300
#define MDC_DCFG_DISP_MODE_8BPP   0x00000000
#define MDC_DCFG_DISP_MODE_16BPP  0x00000100
#define MDC_DCFG_DISP_MODE_24BPP  0x00000200
#define MDC_DCFG_SCLE             0x00000080
#define MDC_DCFG_TRUP             0x00000040
#define MDC_DCFG_VIEN             0x00000020
#define MDC_DCFG_VDEN             0x00000010
#define MDC_DCFG_GDEN             0x00000008
#define MDC_DCFG_VCKE             0x00000004
#define MDC_DCFG_PCKE             0x00000002
#define MDC_DCFG_TGEN             0x00000001

/* MDC_LINE_CNT BIT FIELDS     */

#define MDC_LNCNT_DNA             0x80000000
#define MDC_LNCNT_VNA             0x40000000
#define MDC_LNCNT_VSA             0x20000000
#define MDC_LNCNT_VINT            0x10000000
#define MDC_LNCNT_FLIP            0x08000000
#define MDC_LNCNT_V_LINE_CNT      0x07FF0000
#define MDC_LNCNT_VFLIP           0x00008000
#define MDC_LNCNT_SIGC            0x00004000
#define MDC_LNCNT_SS_LINE_CMP     0x000007FF

/* MDC_FB_ST_OFFSET BIT FIELDS */

#define MDC_FB_ST_OFFSET_MASK     0x0FFFFFFF

/* MDC_CB_ST_OFFSET BIT FIELDS */

#define MDC_CB_ST_OFFSET_MASK     0x0FFFFFFF

/* MDC_CURS_ST_OFFSET BIT FIELDS */

#define MDC_CURS_ST_OFFSET_MASK   0x0FFFFFFF

/* MDC_ICON_ST_OFFSET BIT FIELDS */

#define MDC_ICON_ST_OFFSET_MASK   0x0FFFFFFF

/* MDC_VID_Y_ST_OFFSET BIT FIELDS */

#define MDC_VID_Y_ST_OFFSET_MASK  0x0FFFFFFF

/* MDC_VID_U_ST_OFFSET BIT FIELDS */

#define MDC_VID_U_ST_OFFSET_MASK  0x0FFFFFFF

/* MDC_VID_V_ST_OFFSET BIT FIELDS */

#define MDC_VID_V_ST_OFFSET_MASK  0x0FFFFFFF

/* MDC_LINE_SIZE BIT FIELDS */

#define MDC_LINE_SIZE_VLS_MASK    0xFF000000
#define MDC_LINE_SIZE_CBLS_MASK   0x007F0000
#define MDC_LINE_SIZE_FBLS_MASK   0x000007FF

/* MDC_GFX_PITCH BIT FIELDS */

#define MDC_GFX_PITCH_CBP_MASK    0xFFFF0000
#define MDC_GFX_PITCH_FBP_MASK    0x0000FFFF

/* MDC_VID_YUV_PITCH BIT FIELDS */

#define MDC_YUV_PITCH_UVP_MASK    0xFFFF0000
#define MDC_YUV_PITCH_YBP_MASK    0x0000FFFF

/* MDC_H_ACTIVE_TIMING BIT FIELDS */

#define MDC_HAT_HT_MASK           0x0FF80000
#define MDC_HAT_HA_MASK           0x00000FF8

/* MDC_H_BLANK_TIMING BIT FIELDS */

#define MDC_HBT_HBE_MASK          0x0FF80000
#define MDC_HBT_HBS_MASK          0x00000FF8

/* MDC_H_SYNC_TIMING BIT FIELDS */

#define MDC_HST_HSE_MASK          0x0FF80000
#define MDC_HST_HSS_MASK          0x00000FF8

/* MDC_V_ACTIVE_TIMING BIT FIELDS */

#define MDC_VAT_VT_MASK           0x07FF0000
#define MDC_VAT_VA_MASK           0x000007FF

/* MDC_V_BLANK_TIMING BIT FIELDS */

#define MDC_VBT_VBE_MASK          0x07FF0000
#define MDC_VBT_VBS_MASK          0x000007FF

/* MDC_V_SYNC_TIMING BIT FIELDS */

#define MDC_VST_VSE_MASK          0x07FF0000
#define MDC_VST_VSS_MASK          0x000007FF

/* MDC_DV_CTL BIT DEFINITIONS */

#define MDC_DV_LINE_SIZE_MASK     0x00000C00
#define MDC_DV_LINE_SIZE_1024     0x00000000
#define MDC_DV_LINE_SIZE_2048     0x00000400
#define MDC_DV_LINE_SIZE_4096     0x00000800
#define MDC_DV_LINE_SIZE_8192     0x00000C00

/* VGA DEFINITIONS */

#define MDC_SEQUENCER_INDEX       0x03C4
#define MDC_SEQUENCER_DATA        0x03C5
#define MDC_SEQUENCER_RESET       0x00
#define MDC_SEQUENCER_CLK_MODE    0x01

#define MDC_RESET_VGA_DISP_ENABLE 0x03
#define MDC_CLK_MODE_SCREEN_OFF   0x20

/*---------------------------------------------------*/
/*  REDCLOUD DISPLAY FILTER                          */
/*---------------------------------------------------*/

/* RCDF VIDEO REGISTER DEFINITIONS */

#define RCDF_VIDEO_CONFIG 				    0x000
#define RCDF_DISPLAY_CONFIG				    0x008
#define RCDF_VIDEO_X_POS					0x010
#define RCDF_VIDEO_Y_POS					0x018
#define RCDF_VIDEO_SCALE					0x020
#define RCDF_VIDEO_COLOR_KEY				0x028
#define RCDF_VIDEO_COLOR_MASK				0x030
#define RCDF_PALETTE_ADDRESS 				0x038
#define RCDF_PALETTE_DATA	 				0x040
#define RCDF_VID_MISC						0x050
#define RCDF_VID_CLOCK_SELECT				0x058
#define RCDF_VIDEO_DOWNSCALER_CONTROL       0x078
#define RCDF_VIDEO_DOWNSCALER_COEFFICIENTS  0x080
#define RCDF_VID_CRC						0x088
#define RCDF_VID_CRC32						0x090
#define RCDF_VID_ALPHA_CONTROL			    0x098
#define RCDF_CURSOR_COLOR_KEY				0x0A0
#define RCDF_CURSOR_COLOR_MASK			    0x0A8
#define RCDF_CURSOR_COLOR_1				    0x0B0
#define RCDF_CURSOR_COLOR_2				    0x0B8
#define RCDF_ALPHA_XPOS_1					0x0C0
#define RCDF_ALPHA_YPOS_1					0x0C8
#define RCDF_ALPHA_COLOR_1				    0x0D0
#define RCDF_ALPHA_CONTROL_1				0x0D8
#define RCDF_ALPHA_XPOS_2					0x0E0
#define RCDF_ALPHA_YPOS_2					0x0E8
#define RCDF_ALPHA_COLOR_2				    0x0F0
#define RCDF_ALPHA_CONTROL_2				0x0F8
#define RCDF_ALPHA_XPOS_3					0x100
#define RCDF_ALPHA_YPOS_3					0x108
#define RCDF_ALPHA_COLOR_3				    0x110
#define RCDF_ALPHA_CONTROL_3				0x118
#define RCDF_VIDEO_REQUEST                  0x120
#define RCDF_ALPHA_WATCH                    0x128
#define RCDF_VIDEO_TEST_MODE                0x210
#define RCDF_POWER_MANAGEMENT               0x410

/* DISPLAY FILTER POWER MANAGEMENT DEFINITIONS */

#define RCDF_PM_PANEL_POWER_ON              0x01000000

/* DISPLAY FILTER MSRS */

#define RCDF_MBD_MSR_DIAG_DF				0x2010
#define RCDF_DIAG_32BIT_CRC					0x80000000

/* "RCDF_VIDEO_CONFIG" BIT DEFINITIONS */

#define RCDF_VCFG_VID_EN					0x00000001
#define RCDF_VCFG_VID_INP_FORMAT			0x0000000C
#define RCDF_VCFG_X_FILTER_EN				0x00000040
#define RCDF_VCFG_Y_FILTER_EN				0x00000080
#define RCDF_VCFG_LINE_SIZE_LOWER_MASK	    0x0000FF00
#define RCDF_VCFG_INIT_READ_MASK			0x01FF0000
#define RCDF_VCFG_LINE_SIZE_UPPER			0x08000000
#define RCDF_VCFG_4_2_0_MODE				0x10000000
#define RCDF_VCFG_UYVY_FORMAT				0x00000000
#define RCDF_VCFG_Y2YU_FORMAT				0x00000004
#define RCDF_VCFG_YUYV_FORMAT				0x00000008
#define RCDF_VCFG_YVYU_FORMAT				0x0000000C

/* "RCDF_DISPLAY_CONFIG" BIT DEFINITIONS */

#define RCDF_DCFG_DIS_EN				    0x00000001
#define RCDF_DCFG_HSYNC_EN				    0x00000002
#define RCDF_DCFG_VSYNC_EN				    0x00000004
#define RCDF_DCFG_DAC_BL_EN				    0x00000008
#define RCDF_DCFG_FP_PWR_EN				    0x00000040
#define RCDF_DCFG_FP_DATA_EN				0x00000080
#define RCDF_DCFG_CRT_HSYNC_POL 			0x00000100
#define RCDF_DCFG_CRT_VSYNC_POL 			0x00000200
#define RCDF_DCFG_CRT_SYNC_SKW_MASK		    0x0001C000
#define RCDF_DCFG_CRT_SYNC_SKW_INIT		    0x00010000
#define RCDF_DCFG_PWR_SEQ_DLY_MASK		    0x000E0000
#define RCDF_DCFG_PWR_SEQ_DLY_INIT		    0x00080000
#define RCDF_DCFG_VG_CK					    0x00100000
#define RCDF_DCFG_GV_PAL_BYP				0x00200000
#define RCDF_DAC_VREF                       0x04000000
#define RCDF_FP_ON_STATUS                   0x08000000

/* "RCDF_VID_MISC" BIT DEFINITIONS */

#define RCDF_GAMMA_BYPASS_BOTH              0x00000001
#define RCDF_DAC_POWER_DOWN                 0x00000400
#define RCDF_ANALOG_POWER_DOWN              0x00000800

/* "RCDF_VIDEO_DOWNSCALER_CONTROL" BIT DEFINITIONS */

#define RCDF_VIDEO_DOWNSCALE_ENABLE         0x00000001
#define RCDF_VIDEO_DOWNSCALE_FACTOR_POS     1
#define RCDF_VIDEO_DOWNSCALE_FACTOR_MASK    0x0000001E
#define RCDF_VIDEO_DOWNSCALE_TYPE_A         0x00000000
#define RCDF_VIDEO_DOWNSCALE_TYPE_B         0x00000040
#define RCDF_VIDEO_DOWNSCALE_TYPE_MASK      0x00000040

/* "RCDF_VIDEO_DOWNSCALER_COEFFICIENTS" BIT DEFINITIONS */

#define RCDF_VIDEO_DOWNSCALER_COEF1_POS     0
#define RCDF_VIDEO_DOWNSCALER_COEF2_POS     8
#define RCDF_VIDEO_DOWNSCALER_COEF3_POS     16
#define RCDF_VIDEO_DOWNSCALER_COEF4_POS     24
#define RCDF_VIDEO_DOWNSCALER_COEF_MASK     0xF

/* VIDEO DE-INTERLACING AND ALPHA CONTROL */

#define RCDF_NO_CK_OUTSIDE_ALPHA            0x00000100
#define RCDF_CSC_VIDEO_YUV_TO_RGB           0x00000400
#define RCDF_VIDEO_INPUT_IS_RGB             0x00002000
#define RCDF_ALPHA1_PRIORITY_POS			16
#define RCDF_ALPHA1_PRIORITY_MASK			0x00030000
#define RCDF_ALPHA2_PRIORITY_POS			18
#define RCDF_ALPHA2_PRIORITY_MASK			0x000C0000
#define RCDF_ALPHA3_PRIORITY_POS			20
#define RCDF_ALPHA3_PRIORITY_MASK			0x00300000

/* VIDEO CURSOR COLOR KEY DEFINITIONS */

#define RCDF_CURSOR_COLOR_KEY_ENABLE      0x20000000
#define RCDF_CURSOR_COLOR_KEY_OFFSET_POS  24
#define RCDF_CURSOR_COLOR_BITS            23
#define RCDF_COLOR_MASK                   0x00FFFFFF
                                                                                /* 24 significant bits */

/* ALPHA COLOR BIT DEFINITION (REGISTERS 0x68, 0x78, AND 0x88) */

#define RCDF_ALPHA_COLOR_ENABLE           0x01000000

/* ALPHA CONTROL BIT DEFINITIONS (REGISTERS 0x6C, 0x7C, AND 0x8C) */

#define RCDF_ACTRL_WIN_ENABLE				0x00010000
#define RCDF_ACTRL_LOAD_ALPHA				0x00020000

/* VIDEO REQUEST DEFINITIONS (REGISTER 0x90) */

#define RCDF_VIDEO_Y_REQUEST_POS          0
#define RCDF_VIDEO_X_REQUEST_POS          16
#define RCDF_VIDEO_REQUEST_MASK           0x000007FF

/* GEODELINK DEVICE MSR REGISTER SUMMARY */

#define MBD_MSR_CAP         0x2000      /* Device Capabilities              */
#define MBD_MSR_CONFIG      0x2001      /* Device Master Configuration          */
                                                                                /*      Register                                                */
#define MBD_MSR_SMI         0x2002      /* MBus Device SMI Register         */
#define MBD_MSR_ERROR       0x2003      /* MBus Device Error                */
#define MBD_MSR_PM          0x2004      /* MBus Device Power Management         */
                                                                                /*      Register                                                */
#define MBD_MSR_DIAG        0x2005      /* Mbus Device Diagnostic Register  */

/* DISPLAY FILTER MBD_MSR_DIAG DEFINITIONS */

#define RCDF_MBD_DIAG_SEL0        0x00007FFF    /* Lower 32-bits of Diag Bus 
                                                 * Select     */
#define RCDF_MBD_DIAG_EN0         0x00008000    /* Enable for lower 32-bits of
                                                 * diag bus */
#define RCDF_MBD_DIAG_SEL1        0x7FFF0000    /* Upper 32-bits of Diag Bus 
                                                 * Select     */
#define RCDF_MBD_DIAG_EN1         0x80000000    /* Enable for upper 32-bits of
                                                 * diag bus */

/* DISPLAY FILTER MBD_MSR_CONFIG DEFINITIONS */

#define RCDF_CONFIG_FMT_MASK      0x00000038    /* Output Format */
#define RCDF_CONFIG_FMT_CRT       0x00000000
#define RCDF_CONFIG_FMT_FP        0x00000008

/* MCP MSR DEFINITIONS */

#define MCP_CLKOFF                0x0010
#define MCP_CLKACTIVE             0x0011
#define MCP_CLKDISABLE            0x0012
#define MCP_CLK4ACK               0x0013
#define MCP_SYS_RSTPLL            0x0014
#define MCP_DOTPLL                0x0015
#define MCP_DBGCLKCTL             0x0016
#define MCP_RC_REVID              0x0017
#define MCP_SETM0CTL              0x0040
#define MCP_SETN0CTL              0x0048
#define MCP_CMPVAL0               0x0050
#define MCP_CMPMASK0              0x0051
#define MCP_REGA                  0x0058
#define MCP_REGB                  0x0059
#define MCP_REGAMASK              0x005A
#define MCP_REGAVAL               0x005B
#define MCP_REGBMASK              0x005C
#define MCP_REGBVAL               0x005D
#define MCP_FIFOCTL               0x005E
#define MCP_DIAGCTL               0x005F
#define MCP_H0CTL                 0x0060
#define MCP_XSTATE                0x0066
#define MCP_YSTATE                0x0067
#define MCP_ACTION0               0x0068

/* MCP_SYS_RSTPLL DEFINITIONS */

#define MCP_DOTPOSTDIV3           0x00000008
#define MCP_DOTPREMULT2           0x00000004
#define MCP_DOTPREDIV2            0x00000002
#define MCP_DOTPLL_HALFPIX        0x01000000

/* MCP MBD_MSR_DIAG DEFINITIONS */

#define MCP_MBD_DIAG_SEL0         0x00000007
#define MCP_MBD_DIAG_EN0          0x00008000
#define MCP_MBD_DIAG_SEL1         0x00070000
#define MCP_MBD_DIAG_EN1          0x80000000

/* MCP_DOTPLL DEFINITIONS */

#define MCP_DOTPLL_P              0x00000003
#define MCP_DOTPLL_N              0x000001FC
#define MCP_DOTPLL_M              0x00001E00
#define MCP_DOTPLL_LOCK           0x02000000
#define MCP_DOTPLL_BYPASS         0x00008000
