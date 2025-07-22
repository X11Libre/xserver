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
  * Cimarron register definitions
  */

#ifndef _cim_regs_h
#define _cim_regs_h

/*----------------------------------------------------------------*/
/*                GRAPHICS PROCESSOR DEFINITIONS                  */
/*----------------------------------------------------------------*/

/*----------------------------*/
/* COMMAND BUFFER DEFINITIONS */
/*----------------------------*/

#define GP3_BLT_COMMAND_SIZE			    68  /* 18 DWORDS */
#define GP3_VECTOR_COMMAND_SIZE             56  /* 14 DWORDS */
#define GP3_4BPP_LUT_COMMAND_SIZE           76  /* 16 DWORDS + 3 CMD DWORDS */
#define GP3_8BPP_LUT_COMMAND_SIZE           1036        /* 256 DWORDS +
                                                         * 3 CMD DWORDS */
#define GP3_VECTOR_PATTERN_COMMAND_SIZE     20  /* 2 DWORDS + 3 CMD DWORDS */
#define GP3_MAX_COMMAND_SIZE                9000        /* 8K +
                                                         * WORKAROUND SPACE */
#define GP3_SCRATCH_BUFFER_SIZE             0x100000    /* 1MB SCRATCH
                                                         * BUFFER */
#define GP3_BLT_1PASS_SIZE                  0xC7F8      /* (50K - 8) is largest
                                                         * 1-Pass load size */

/*-------------------------------------*/
/* BLT COMMAND BUFFER REGISTER OFFSETS */
/*-------------------------------------*/

#define GP3_BLT_CMD_HEADER				    0x00000000
#define GP3_BLT_RASTER_MODE                 0x00000004
#define GP3_BLT_DST_OFFSET				    0x00000008
#define GP3_BLT_SRC_OFFSET				    0x0000000C
#define GP3_BLT_STRIDE					    0x00000010
#define GP3_BLT_WID_HEIGHT				    0x00000014
#define GP3_BLT_SRC_COLOR_FG			    0x00000018
#define GP3_BLT_SRC_COLOR_BG			    0x0000001C
#define GP3_BLT_PAT_COLOR_0				    0x00000020
#define GP3_BLT_PAT_COLOR_1				    0x00000024
#define GP3_BLT_PAT_DATA_0				    0x00000028
#define GP3_BLT_PAT_DATA_1				    0x0000002C
#define GP3_BLT_CH3_OFFSET				    0x00000030
#define GP3_BLT_CH3_MODE_STR			    0x00000034
#define GP3_BLT_CH3_WIDHI				    0x00000038
#define GP3_BLT_BASE_OFFSET				    0x0000003C
#define GP3_BLT_MODE					    0x00000040

/*-----------------------------------------------------------------*/
/* VECTOR COMMAND BUFFER REGISTER OFFSETS                          */
/* Some of these are identical to the BLT registers (and we will   */
/* be assumed to be such in the Cimarron code, but they are listed */
/* here for clarity and for future changes.                        */
/*-----------------------------------------------------------------*/

#define GP3_VEC_CMD_HEADER				    0x00000000
#define GP3_VECTOR_RASTER_MODE			    0x00000004
#define GP3_VECTOR_DST_OFFSET			    0x00000008
#define GP3_VECTOR_VEC_ERR				    0x0000000C
#define GP3_VECTOR_STRIDE				    0x00000010
#define GP3_VECTOR_VEC_LEN				    0x00000014
#define GP3_VECTOR_SRC_COLOR_FG			    0x00000018
#define GP3_VECTOR_PAT_COLOR_0			    0x0000001C
#define GP3_VECTOR_PAT_COLOR_1			    0x00000020
#define GP3_VECTOR_PAT_DATA_0			    0x00000024
#define GP3_VECTOR_PAT_DATA_1			    0x00000028
#define GP3_VECTOR_CH3_MODE_STR			    0x0000002C
#define GP3_VECTOR_BASE_OFFSET			    0x00000030
#define GP3_VECTOR_MODE					    0x00000034

/*---------------------------------------------------*/
/* GP REGISTER DEFINITIONS                           */
/* Addresses for writing or reading directly to/from */
/* the graphics processor.                           */
/*---------------------------------------------------*/

#define GP3_DST_OFFSET                      0x00000000
#define GP3_SRC_OFFSET                      0x00000004
#define GP3_VEC_ERR                         0x00000004
#define GP3_STRIDE                          0x00000008
#define GP3_WID_HEIGHT                      0x0000000C
#define GP3_VEC_LEN                         0x0000000C
#define GP3_SRC_COLOR_FG                    0x00000010
#define GP3_SRC_COLOR_BG                    0x00000014
#define GP3_PAT_COLOR_0                     0x00000018
#define GP3_PAT_COLOR_1                     0x0000001C
#define GP3_PAT_COLOR_2                     0x00000020
#define GP3_PAT_COLOR_3                     0x00000024
#define GP3_PAT_COLOR_4                     0x00000028
#define GP3_PAT_COLOR_5                     0x0000002C
#define GP3_PAT_DATA_0                      0x00000030
#define GP3_PAT_DATA_1                      0x00000034
#define GP3_RASTER_MODE                     0x00000038
#define GP3_VEC_MODE                        0x0000003C
#define GP3_BLT_MODE                        0x00000040
#define GP3_BLT_STATUS                      0x00000044
#define GP3_HST_SRC                         0x00000048
#define GP3_BASE_OFFSET                     0x0000004C
#define GP3_CMD_TOP                         0x00000050
#define GP3_CMD_BOT                         0x00000054
#define GP3_CMD_READ					    0x00000058
#define GP3_CMD_WRITE					    0x0000005C
#define GP3_CH3_OFFSET                      0x00000060
#define GP3_CH3_MODE_STR                    0x00000064
#define GP3_CH3_WIDHI                       0x00000068
#define GP3_CH3_HST_SRC                     0x0000006C
#define GP3_LUT_ADDRESS                     0x00000070
#define GP3_LUT_DATA                        0x00000074
#define GP3_INT_CTL                         0x00000078
#define GP3_HST_SRC_RANGE                   0x00000100

/*------------------------*/
/* REGISTER BIT FIELDS    */
/*------------------------*/

/* GP3_BLT_CMD_HEADER BIT DEFINITIONS */

#define GP3_BLT_HDR_WRAP				    0x80000000
#define GP3_BLT_HDR_TYPE				    0x00000000
#define GP3_BLT_HDR_HAZARD_ENABLE           0x10000000
#define GP3_BLT_HDR_RASTER_ENABLE		    0x00000001
#define GP3_BLT_HDR_DST_OFF_ENABLE		    0x00000002
#define GP3_BLT_HDR_SRC_OFF_ENABLE          0x00000004
#define GP3_BLT_HDR_STRIDE_ENABLE		    0x00000008
#define GP3_BLT_HDR_WIDHI_ENABLE		    0x00000010
#define GP3_BLT_HDR_SRC_FG_ENABLE		    0x00000020
#define GP3_BLT_HDR_SRC_BG_ENABLE		    0x00000040
#define GP3_BLT_HDR_PAT_CLR0_ENABLE		    0x00000080
#define GP3_BLT_HDR_PAT_CLR1_ENABLE		    0x00000100
#define GP3_BLT_HDR_PAT_DATA0_ENABLE	    0x00000200
#define GP3_BLT_HDR_PAT_DATA1_ENABLE        0x00000400
#define GP3_BLT_HDR_CH3_OFF_ENABLE          0x00000800
#define GP3_BLT_HDR_CH3_STR_ENABLE          0x00001000
#define GP3_BLT_HDR_CH3_WIDHI_ENABLE        0x00002000
#define GP3_BLT_HDR_BASE_OFFSET_ENABLE      0x00004000
#define GP3_BLT_HDR_BLT_MODE_ENABLE         0x00008000

/* GP3_VEC_CMD_HEADER BIT DEFINITIONS */

#define GP3_VEC_HDR_WRAP				    0x80000000
#define GP3_VEC_HDR_TYPE				    0x20000000
#define GP3_VEC_HDR_HAZARD_ENABLE           0x10000000
#define GP3_VEC_HDR_RASTER_ENABLE		    0x00000001
#define GP3_VEC_HDR_DST_OFF_ENABLE		    0x00000002
#define GP3_VEC_HDR_VEC_ERR_ENABLE          0x00000004
#define GP3_VEC_HDR_STRIDE_ENABLE		    0x00000008
#define GP3_VEC_HDR_VEC_LEN_ENABLE		    0x00000010
#define GP3_VEC_HDR_SRC_FG_ENABLE		    0x00000020
#define GP3_VEC_HDR_PAT_CLR0_ENABLE		    0x00000040
#define GP3_VEC_HDR_PAT_CLR1_ENABLE		    0x00000080
#define GP3_VEC_HDR_PAT_DATA0_ENABLE	    0x00000100
#define GP3_VEC_HDR_PAT_DATA1_ENABLE        0x00000200
#define GP3_VEC_HDR_CH3_STR_ENABLE          0x00000400
#define GP3_VEC_HDR_BASE_OFFSET_ENABLE      0x00000800
#define GP3_VEC_HDR_VEC_MODE_ENABLE         0x00001000

/* GP3_RASTER_MODE BIT DEFINITIONS */

#define GP3_RM_BPPFMT_332			0x00000000      /* 8 BPP, palettized        */
#define GP3_RM_BPPFMT_4444			0x40000000      /* 16 BPP, 4:4:4:4          */
#define GP3_RM_BPPFMT_1555			0x50000000      /* 16 BPP, 1:5:5:5          */
#define GP3_RM_BPPFMT_565			0x60000000      /* 16 BPP, 5:6:5            */
#define GP3_RM_BPPFMT_8888			0x80000000      /* 32 BPP, 8:8:8:8          */
#define GP3_RM_ALPHA_ALL			0x00C00000      /* Alpha enable             */
#define GP3_RM_ALPHA_TO_RGB			0x00400000      /* Alpha applies to RGB     */
#define GP3_RM_ALPHA_TO_ALPHA		0x00800000      /* Alpha applies to alpha   */
#define GP3_RM_ALPHA_OP_MASK		0x00300000      /* Alpha operation          */
#define GP3_RM_ALPHA_TIMES_A		0x00000000      /* Alpha * A                */
#define GP3_RM_BETA_TIMES_B			0x00100000      /* (1-alpha) * B            */
#define GP3_RM_A_PLUS_BETA_B		0x00200000      /* A + (1-alpha) * B        */
#define GP3_RM_ALPHA_A_PLUS_BETA_B	0x00300000      /* alpha * A + (1 - alpha)B */
#define GP3_RM_ALPHA_SELECT			0x000E0000      /* Alpha Select             */
#define GP3_RM_SELECT_ALPHA_A		0x00000000      /* Alpha from channel A     */
#define GP3_RM_SELECT_ALPHA_B		0x00020000      /* Alpha from channel B     */
#define GP3_RM_SELECT_ALPHA_R		0x00040000      /* Registered alpha         */
#define GP3_RM_SELECT_ALPHA_1		0x00060000      /* Constant 1               */
#define GP3_RM_SELECT_ALPHA_CHAN_A	0x00080000      /* RGB Values from A        */
#define GP3_RM_SELECT_ALPHA_CHAN_B	0x000A0000      /* RGB Values from B        */
#define GP3_RM_SELECT_ALPHA_CHAN_3  0x000C0000  /* Alpha from channel 3     */
#define GP3_RM_DEST_FROM_CHAN_A		0x00010000      /* Alpha channel select     */
#define GP3_RM_PATTERN_INVERT       0x00001000  /* Invert monochrome pat    */
#define GP3_RM_SOURCE_INVERT        0x00002000  /* Invert monochrome src    */
#define GP3_RM_PAT_FLAGS			0x00000700      /* pattern related bits     */
#define GP3_RM_PAT_MONO				0x00000100      /* monochrome pattern       */
#define GP3_RM_PAT_COLOR			0x00000200      /* color pattern            */
#define GP3_RM_PAT_TRANS			0x00000400      /* pattern transparency     */
#define GP3_RM_SRC_TRANS			0x00000800      /* source transparency      */

/* GP3_VECTOR_MODE REGISTER DESCRIPTIONS */

#define GP3_VM_DST_REQ			    0x00000008  /* dst data required        */
#define GP3_VM_THROTTLE			    0x00000010  /* sync to VBLANK           */

/* GP3_BLT_MODE REGISTER DEFINITIONS */

#define GP3_BM_SRC_FB			    0x00000001  /* src = frame buffer       */
#define GP3_BM_SRC_HOST			    0x00000002  /* src = host register      */
#define GP3_BM_DST_REQ			    0x00000004  /* dst data required        */
#define GP3_BM_SRC_MONO			    0x00000040  /* monochrome source data   */
#define GP3_BM_SRC_BP_MONO		    0x00000080  /* Byte-packed monochrome   */
#define GP3_BM_NEG_YDIR			    0x00000100  /* negative Y direction     */
#define GP3_BM_NEG_XDIR			    0x00000200  /* negative X direction     */
#define GP3_BM_THROTTLE			    0x00000400  /* sync to VBLANK           */

/* GP3_BLT_STATUS REGISTER DEFINITIONS */

#define GP3_BS_BLT_BUSY			    0x00000001  /* GP is not idle           */
#define GP3_BS_BLT_PENDING		    0x00000004  /* second BLT is pending    */
#define GP3_BS_HALF_EMPTY		    0x00000008  /* src FIFO half empty      */
#define GP3_BS_CB_EMPTY             0x00000010  /* Command buffer empty.    */

/* GP3_CH3_MODE_STR REGISTER DEFINITIONS */

#define GP3_CH3_C3EN                        0x80000000
#define GP3_CH3_REPLACE_SOURCE              0x40000000
#define GP3_CH3_NEG_XDIR                    0x20000000
#define GP3_CH3_NEG_YDIR                    0x10000000
#define GP3_CH3_SRC_FMT_MASK                0x0f000000
#define GP3_CH3_SRC_3_3_2                   0x00000000
#define GP3_CH3_SRC_8BPP_INDEXED            0x01000000
#define GP3_CH3_SRC_8BPP_ALPHA              0x02000000
#define GP3_CH3_SRC_4_4_4_4                 0x04000000
#define GP3_CH3_SRC_1_5_5_5                 0x05000000
#define GP3_CH3_SRC_0_5_6_5                 0x06000000
#define GP3_CH3_SRC_Y_U_V                   0x07000000
#define GP3_CH3_SRC_8_8_8_8                 0x08000000
#define GP3_CH3_SRC_24BPP_PACKED            0x0B000000
#define GP3_CH3_SRC_4BPP_INDEXED            0x0D000000
#define GP3_CH3_SRC_4BPP_ALPHA              0x0E000000
#define GP3_CH3_SRC_MASK                    0x0F000000
#define GP3_CH3_ROTATE_ENABLE               0x00800000
#define GP3_CH3_BGR_ORDER                   0x00400000
#define GP3_CH3_COLOR_PAT_ENABLE            0x00200000
#define GP3_CH3_PRESERVE_LUT                0x00100000
#define GP3_CH3_PREFETCH_ENABLE             0x00080000
#define GP3_CH3_HST_SRC_ENABLE			    0x00040000
#define GP3_CH3_STRIDE_MASK                 0x0000FFFF

/* DATA AND LUT LOAD BIT DEFINITIONS */

#define GP3_LUT_HDR_WRAP                    0x80000000
#define GP3_LUT_HDR_TYPE				    0x40000000
#define GP3_LUT_HDR_DATA_ENABLE             0x00000003
#define GP3_DATA_LOAD_HDR_WRAP              0x80000000
#define GP3_DATA_LOAD_HDR_TYPE              0x60000000
#define GP3_DATA_LOAD_HDR_ENABLE            0x00000001

#define GP3_HOST_SOURCE_TYPE			    0x00000000
#define GP3_CH3_HOST_SOURCE_TYPE		    0x20000000
#define GP3_OLD_PATTERN_COLORS			    0x40000000
#define GP3_LUT_DATA_TYPE				    0x60000000

#define GP3_BASE_OFFSET_DSTMASK             0xFFC00000
#define GP3_BASE_OFFSET_SRCMASK             0x003FF000
#define GP3_BASE_OFFSET_CH3MASK             0x00000FFC

/*----------------------------------------------------------------*/
/*                  VIDEO GENERATOR DEFINITIONS                   */
/*----------------------------------------------------------------*/

#define DC3_UNLOCK              0x00000000      /* Unlock register              */
#define DC3_GENERAL_CFG         0x00000004      /* Config registers             */
#define DC3_DISPLAY_CFG         0x00000008
#define DC3_ARB_CFG             0x0000000C

#define DC3_FB_ST_OFFSET        0x00000010      /* Frame buffer start offset    */
#define DC3_CB_ST_OFFSET        0x00000014      /* Compression start offset     */
#define DC3_CURS_ST_OFFSET      0x00000018      /* Cursor buffer start offset   */
#define DC3_VID_Y_ST_OFFSET     0x00000020      /* Video Y Buffer start offset  */
#define DC3_VID_U_ST_OFFSET     0x00000024      /* Video U Buffer start offset  */
#define DC3_VID_V_ST_OFFSET     0x00000028      /* Video V Buffer start offset  */
#define DC3_DV_TOP              0x0000002C      /* DV Ram Limit Register        */
#define DC3_LINE_SIZE           0x00000030      /* Video, CB, and FB line sizes */
#define DC3_GFX_PITCH           0x00000034      /* FB and DB skip counts        */
#define DC3_VID_YUV_PITCH       0x00000038      /* Y, U and V buffer skip counts */

#define DC3_H_ACTIVE_TIMING     0x00000040      /* Horizontal timings           */
#define DC3_H_BLANK_TIMING      0x00000044
#define DC3_H_SYNC_TIMING       0x00000048
#define DC3_V_ACTIVE_TIMING     0x00000050      /* Vertical Timings             */
#define DC3_V_BLANK_TIMING      0x00000054
#define DC3_V_SYNC_TIMING       0x00000058
#define DC3_FB_ACTIVE           0x0000005C

#define DC3_CURSOR_X            0x00000060      /* Cursor X position            */
#define DC3_CURSOR_Y            0x00000064      /* Cursor Y Position            */
#define DC3_LINE_CNT_STATUS     0x0000006C

#define DC3_PAL_ADDRESS         0x00000070      /* Palette Address              */
#define DC3_PAL_DATA            0x00000074      /* Palette Data                 */
#define DC3_DFIFO_DIAG          0x00000078      /* Display FIFO diagnostic      */
#define DC3_CFIFO_DIAG          0x0000007C      /* Compression FIFO diagnostic  */

#define DC3_VID_DS_DELTA        0x00000080      /* Vertical Downscaling fraction */

#define DC3_PHY_MEM_OFFSET      0x00000084      /* VG Base Address Register     */
#define DC3_DV_CTL              0x00000088      /* Dirty-Valid Control Register */
#define DC3_DV_ACC              0x0000008C      /* Dirty-Valid RAM Access       */

#define DC3_GFX_SCALE           0x00000090      /* Graphics Scaling             */
#define DC3_IRQ_FILT_CTL        0x00000094      /* VBlank interrupt and filters */
#define DC3_FILT_COEFF1         0x00000098
#define DC3_FILT_COEFF2         0x0000009C

#define DC3_VBI_EVEN_CTL        0x000000A0      /* VBI Data Buffer Controls     */
#define DC3_VBI_ODD_CTL         0x000000A4
#define DC3_VBI_HOR             0x000000A8
#define DC3_VBI_LN_ODD          0x000000AC
#define DC3_VBI_LN_EVEN         0x000000B0
#define DC3_VBI_PITCH           0x000000B4

#define DC3_COLOR_KEY           0x000000B8      /* Graphics color key           */
#define DC3_COLOR_MASK          0x000000BC      /* Graphics color key mask      */
#define DC3_CLR_KEY_X           0x000000C0
#define DC3_CLR_KEY_Y           0x000000C4

#define DC3_IRQ                 0x000000C8
#define DC3_GENLK_CTL           0x000000D4

#define DC3_VID_EVEN_Y_ST_OFFSET    0x000000D8  /* Even field video buffers */
#define DC3_VID_EVEN_U_ST_OFFSET    0x000000DC
#define DC3_VID_EVEN_V_ST_OFFSET    0x000000E0

#define DC3_V_ACTIVE_EVEN       0x000000E4      /* Even field timing registers  */
#define DC3_V_BLANK_EVEN        0x000000E8
#define DC3_V_SYNC_EVEN         0x000000EC

/* UNLOCK VALUE */

#define DC3_UNLOCK_VALUE	    0x00004758  /* used to unlock DC regs       */

/* VG GEODELINK DEVICE SMI MSR FIELDS */

#define DC3_VG_BL_MASK                      0x00000001
#define DC3_MISC_MASK                       0x00000002
#define DC3_ISR0_MASK                       0x00000004
#define DC3_VGA_BL_MASK                     0x00000008
#define DC3_CRTCIO_MSK                      0x00000010
#define DC3_VG_BLANK_SMI                    0x00000001
#define DC3_MISC_SMI                        0x00000002
#define DC3_ISR0_SMI                        0x00000004
#define DC3_VGA_BLANK_SMI                   0x00000008
#define DC3_CRTCIO_SMI                      0x00000010

/* DC3_GENERAL_CFG BIT FIELDS */

#define DC3_GCFG_DBUG                       0x80000000
#define DC3_GCFG_DBSL                       0x40000000
#define DC3_GCFG_CFRW                       0x20000000
#define DC3_GCFG_DIAG                       0x10000000
#define DC3_GCFG_CRC_MODE                   0x08000000
#define DC3_GCFG_SGFR                       0x04000000
#define DC3_GCFG_SGRE                       0x02000000
#define DC3_GCFG_SIGE                       0x01000000
#define DC3_GCFG_SIG_SEL                    0x00800000
#define DC3_GCFG_YUV_420                    0x00100000
#define DC3_GCFG_VDSE                       0x00080000
#define DC3_GCFG_VGAFT                      0x00040000
#define DC3_GCFG_FDTY                       0x00020000
#define DC3_GCFG_STFM                       0x00010000
#define DC3_GCFG_DFHPEL_MASK                0x0000F000
#define DC3_GCFG_DFHPSL_MASK                0x00000F00
#define DC3_GCFG_VGAE                       0x00000080
#define DC3_GCFG_DECE                       0x00000040
#define DC3_GCFG_CMPE                       0x00000020
#define DC3_GCFG_FILT_SIG_SEL               0x00000010
#define DC3_GCFG_VIDE                       0x00000008
#define DC3_GCFG_CLR_CUR                    0x00000004
#define DC3_GCFG_CURE                       0x00000002
#define DC3_GCFG_DFLE                       0x00000001

/* DC3_DISPLAY_CFG BIT FIELDS */

#define DC3_DCFG_VISL                       0x08000000
#define DC3_DCFG_FRLK                       0x04000000
#define DC3_DCFG_PALB                       0x02000000
#define DC3_DCFG_DCEN                       0x01000000
#define DC3_DCFG_VFHPEL_MASK                0x000F0000
#define DC3_DCFG_VFHPSL_MASK                0x0000F000
#define DC3_DCFG_16BPP_MODE_MASK            0x00000C00
#define DC3_DCFG_16BPP                      0x00000000
#define DC3_DCFG_15BPP                      0x00000400
#define DC3_DCFG_12BPP                      0x00000800
#define DC3_DCFG_DISP_MODE_MASK             0x00000300
#define DC3_DCFG_DISP_MODE_8BPP             0x00000000
#define DC3_DCFG_DISP_MODE_16BPP            0x00000100
#define DC3_DCFG_DISP_MODE_24BPP            0x00000200
#define DC3_DCFG_DISP_MODE_32BPP            0x00000300
#define DC3_DCFG_TRUP                       0x00000040
#define DC3_DCFG_VDEN                       0x00000010
#define DC3_DCFG_GDEN                       0x00000008
#define DC3_DCFG_TGEN                       0x00000001

/* DC3_ARB_CFG BIT FIELDS */

#define DC3_ACFG_LB_LOAD_WM_EN              0x00100000
#define DC3_ACFG_LB_LOAD_WM_MASK            0x000F0000
#define DC3_ACFG_LPEN_END_COUNT_MASK        0x0000FE00
#define DC3_ACFG_HPEN_SBINV                 0x00000100
#define DC3_ACFG_HPEN_FB_INV_HALFSB         0x00000080
#define DC3_ACFG_HPEN_FB_INV_SBRD           0x00000040
#define DC3_ACFG_HPEN_FB_INV                0x00000020
#define DC3_ACFG_HPEN_1LB_INV               0x00000010
#define DC3_ACFG_HPEN_2LB_INV               0x00000008
#define DC3_ACFG_HPEN_3LB_INV               0x00000004
#define DC3_ACFG_HPEN_LB_FILL               0x00000002
#define DC3_ACFG_LPEN_VSYNC                 0x00000001

/* DC3_FB_ST_OFFSET BIT FIELDS */

#define DC3_FB_ST_OFFSET_MASK               0x0FFFFFFF

/* DC3_CB_ST_OFFSET BIT FIELDS */

#define DC3_CB_ST_OFFSET_MASK               0x0FFFFFFF

/* DC3_CURS_ST_OFFSET BIT FIELDS */

#define DC3_CURS_ST_OFFSET_MASK             0x0FFFFFFF

/* DC3_ICON_ST_OFFSET BIT FIELDS */

#define DC3_ICON_ST_OFFSET_MASK             0x0FFFFFFF

/* DC3_VID_Y_ST_OFFSET BIT FIELDS */

#define DC3_VID_Y_ST_OFFSET_MASK            0x0FFFFFFF

/* DC3_VID_U_ST_OFFSET BIT FIELDS */

#define DC3_VID_U_ST_OFFSET_MASK            0x0FFFFFFF

/* DC3_VID_V_ST_OFFSET BIT FIELDS */

#define DC3_VID_V_ST_OFFSET_MASK            0x0FFFFFFF

/* DC3_DV_TOP BIT FIELDS */

#define DC3_DVTOP_ENABLE                    0x00000001
#define DC3_DVTOP_MAX_MASK                  0x00FFFC00
#define DC3_DVTOP_MAX_SHIFT                 10

/* DC3_LINE_SIZE BIT FIELDS */

#define DC3_LINE_SIZE_VLS_MASK              0x3FF00000
#define DC3_LINE_SIZE_CBLS_MASK             0x0007F000
#define DC3_LINE_SIZE_FBLS_MASK             0x000003FF
#define DC3_LINE_SIZE_CB_SHIFT              12
#define DC3_LINE_SIZE_VB_SHIFT              20

/* DC3_GFX_PITCH BIT FIELDS */

#define DC3_GFX_PITCH_CBP_MASK              0xFFFF0000
#define DC3_GFX_PITCH_FBP_MASK              0x0000FFFF

/* DC3_VID_YUV_PITCH BIT FIELDS */

#define DC3_YUV_PITCH_UVP_MASK              0xFFFF0000
#define DC3_YUV_PITCH_YBP_MASK              0x0000FFFF

/* DC3_H_ACTIVE_TIMING BIT FIELDS */

#define DC3_HAT_HT_MASK                     0x0FF80000
#define DC3_HAT_HA_MASK                     0x00000FF8

/* DC3_H_BLANK_TIMING BIT FIELDS */

#define DC3_HBT_HBE_MASK                    0x0FF80000
#define DC3_HBT_HBS_MASK                    0x00000FF8

/* DC3_H_SYNC_TIMING BIT FIELDS */

#define DC3_HST_HSE_MASK                    0x0FF80000
#define DC3_HST_HSS_MASK                    0x00000FF8

/* DC3_V_ACTIVE_TIMING BIT FIELDS */

#define DC3_VAT_VT_MASK                     0x07FF0000
#define DC3_VAT_VA_MASK                     0x000007FF

/* DC3_V_BLANK_TIMING BIT FIELDS */

#define DC3_VBT_VBE_MASK                    0x07FF0000
#define DC3_VBT_VBS_MASK                    0x000007FF

/* DC3_V_SYNC_TIMING BIT FIELDS */

#define DC3_VST_VSE_MASK                    0x07FF0000
#define DC3_VST_VSS_MASK                    0x000007FF

/* DC3_LINE_CNT_STATUS BIT FIELDS     */

#define DC3_LNCNT_DNA                       0x80000000
#define DC3_LNCNT_VNA                       0x40000000
#define DC3_LNCNT_VSA                       0x20000000
#define DC3_LNCNT_VINT                      0x10000000
#define DC3_LNCNT_FLIP                      0x08000000
#define DC3_LNCNT_V_LINE_CNT                0x07FF0000
#define DC3_LNCNT_VFLIP                     0x00008000
#define DC3_LNCNT_SIGC                      0x00004000
#define DC3_LNCNT_EVEN_FIELD                0x00002000
#define DC3_LNCNT_SS_LINE_CMP               0x000007FF

/* DC3_VID_DS_DELTA BIT FIELDS */

#define DC3_DS_DELTA_MASK                   0xFFFC0000
#define DC3_601_VSYNC_SHIFT_MASK            0x00000FFF
#define DC3_601_VSYNC_SHIFT_ENABLE          0x00008000

/* DC3_DV_CTL BIT DEFINITIONS */

#define DC3_DV_LINE_SIZE_MASK               0x00000C00
#define DC3_DV_LINE_SIZE_1024               0x00000000
#define DC3_DV_LINE_SIZE_2048               0x00000400
#define DC3_DV_LINE_SIZE_4096               0x00000800
#define DC3_DV_LINE_SIZE_8192               0x00000C00

/* DC3_IRQ_FILT_CTL DEFINITIONS */

#define DC3_IRQFILT_LB_MASK                 0x80000200
#define DC3_IRQFILT_LB_COEFF                0x00000000
#define DC3_IRQFILT_SCALER_FILTER           0x00000200
#define DC3_IRQFILT_SYNCHRONIZER            0x80000000
#define DC3_IRQFILT_FLICKER_FILTER          0x80000200
#define DC3_IRQFILT_LB_SEL_MASK             0x60000000
#define DC3_IRQFILT_INTL_ADDR               0x10000000
#define DC3_IRQFILT_LINE_MASK               0x07FF0000
#define DC3_IRQFILT_ALPHA_FILT_EN           0x00004000
#define DC3_IRQFILT_GFX_FILT_EN             0x00001000
#define DC3_IRQFILT_INTL_EN                 0x00000800
#define DC3_IRQFILT_H_FILT_SEL              0x00000400
#define DC3_IRQFILT_LB_ADDR                 0x00000100

/* DC3_VBI_EVEN_CTL DEFINITIONS */

#define DC3_VBI_EVEN_ENABLE_CRC             (1L << 31)
#define DC3_VBI_EVEN_CTL_ENABLE_16          (1L << 30)
#define DC3_VBI_EVEN_CTL_UPSCALE            (1L << 29)
#define DC3_VBI_ENABLE                      (1L << 28)
#define DC3_VBI_EVEN_CTL_OFFSET_MASK        0x0FFFFFFF

/* DC3_VBI_ODD_CTL DEFINITIONS */

#define DC3_VBI_ODD_CTL_OFFSET_MASK         0x0FFFFFFF

/* DC3_VBI_HOR BIT DEFINITIONS */

#define DC3_VBI_HOR_END_SHIFT               16
#define DC3_VBI_HOR_END_MASK				0x0FFF0000
#define DC3_VBI_HOR_START_MASK				0x00000FFF

/* DC3_VBI_LN_ODD BIT DEFINITIONS */

#define DC3_VBI_ODD_ENABLE_SHIFT            2
#define DC3_VBI_ODD_ENABLE_MASK             0x01FFFFFC
#define DC3_VBI_ODD_LINE_SHIFT              25
#define DC3_VBI_ODD_LINE_MASK               0xFE000000

/* DC3_VBI_LN_EVEN BIT DEFINITIONS */

#define DC3_VBI_EVEN_ENABLE_SHIFT           2
#define DC3_VBI_EVEN_ENABLE_MASK			0x01FFFFFC
#define DC3_VBI_EVEN_LINE_SHIFT             25
#define DC3_VBI_EVEN_LINE_MASK				0xFE000000

/* DC3_COLOR_KEY DEFINITIONS */

#define DC3_CLR_KEY_DATA_MASK               0x00FFFFFF
#define DC3_CLR_KEY_ENABLE                  0x01000000

/* DC3_IRQ DEFINITIONS */

#define DC3_IRQ_MASK                        0x00000001
#define DC3_VSYNC_IRQ_MASK                  0x00000002
#define DC3_IRQ_STATUS                      0x00010000
#define DC3_VSYNC_IRQ_STATUS                0x00020000

/* DC3_GENLK_CTL DEFINITIONS */

#define DC3_GC_FLICKER_FILTER_NONE          0x00000000
#define DC3_GC_FLICKER_FILTER_1_16          0x10000000
#define DC3_GC_FLICKER_FILTER_1_8           0x20000000
#define DC3_GC_FLICKER_FILTER_1_4           0x40000000
#define DC3_GC_FLICKER_FILTER_5_16          0x50000000
#define DC3_GC_FLICKER_FILTER_MASK          0xF0000000
#define DC3_GC_ALPHA_FLICK_ENABLE           0x02000000
#define DC3_GC_FLICKER_FILTER_ENABLE        0x01000000
#define DC3_GC_VIP_VID_OK                   0x00800000
#define DC3_GC_GENLK_ACTIVE                 0x00400000
#define DC3_GC_SKEW_WAIT                    0x00200000
#define DC3_GC_VSYNC_WAIT                   0x00100000
#define DC3_GC_GENLOCK_TO_ENABLE            0x00080000
#define DC3_GC_GENLOCK_ENABLE               0x00040000
#define DC3_GC_GENLOCK_SKEW_MASK            0x0003FFFF

/* VGA DEFINITIONS */

#define DC3_SEQUENCER_INDEX                 0x03C4
#define DC3_SEQUENCER_DATA                  0x03C5
#define DC3_SEQUENCER_RESET                 0x00
#define DC3_SEQUENCER_CLK_MODE              0x01

#define DC3_RESET_VGA_DISP_ENABLE           0x03
#define DC3_CLK_MODE_SCREEN_OFF             0x20

/* DOT CLOCK FREQUENCY STRUCTURE */
/* Note that m, n and p refer to the register m, n and p  */
/* and not the m, n and p from the PLL equation.  The PLL */
/* equation adds 1 to each value.                         */

typedef struct tagPLLFrequency {
    unsigned long pll_value;
    unsigned long frequency;

} PLL_FREQUENCY;

/* VG MSRS */

#define DC3_SPARE_MSR                       0x2011
#define DC3_RAM_CTL                         0x2012

/* DC3_SPARE_MSR DEFINITIONS */

#define DC3_SPARE_DISABLE_CFIFO_HGO         0x00000800
#define DC3_SPARE_VFIFO_ARB_SELECT          0x00000400
#define DC3_SPARE_WM_LPEN_OVRD              0x00000200
#define DC3_SPARE_LOAD_WM_LPEN_MASK         0x00000100
#define DC3_SPARE_DISABLE_INIT_VID_PRI      0x00000080
#define DC3_SPARE_DISABLE_VFIFO_WM          0x00000040
#define DC3_SPARE_DISABLE_CWD_CHECK         0x00000020
#define DC3_SPARE_PIX8_PAN_FIX              0x00000010
#define DC3_SPARE_FIRST_REQ_MASK            0x00000002

/* VG DIAG DEFINITIONS */

#define DC3_MBD_DIAG_EN0                    0x00008000
#define DC3_MBD_DIAG_EN1                    0x80000000
#define DC3_DIAG_DOT_CRTC_DP                0x00000082
#define DC3_DIAG_DOT_CRTC_DP_HIGH           0x00820000
#define DC3_DIAG_EVEN_FIELD                 0x00002000

/*----------------------------------------------------------------*/
/*                DISPLAY FILTER DEFINITIONS                      */
/*----------------------------------------------------------------*/

#define DF_VIDEO_CONFIG 		            0x00000000
#define DF_DISPLAY_CONFIG                   0x00000008
#define DF_VIDEO_X_POS                      0x00000010
#define DF_VIDEO_Y_POS                      0x00000018
#define DF_VIDEO_SCALER                     0x00000020
#define DF_VIDEO_COLOR_KEY			        0x00000028
#define DF_VIDEO_COLOR_MASK			        0x00000030
#define DF_PALETTE_ADDRESS 			        0x00000038
#define DF_PALETTE_DATA	 			        0x00000040
#define DF_SATURATION_LIMIT                 0x00000048
#define DF_VID_MISC					        0x00000050
#define DF_VIDEO_YSCALE                     0x00000060
#define DF_VIDEO_XSCALE                     0x00000068
#define DF_VID_CRC                          0x00000088
#define DF_VID_CRC32                        0x00000090
#define DF_VID_ALPHA_CONTROL                0x00000098
#define DF_CURSOR_COLOR_KEY                 0x000000A0
#define DF_CURSOR_COLOR_MASK                0x000000A8
#define DF_CURSOR_COLOR_1                   0x000000B0
#define DF_CURSOR_COLOR_2                   0x000000B8
#define DF_ALPHA_XPOS_1                     0x000000C0
#define DF_ALPHA_YPOS_1                     0x000000C8
#define DF_ALPHA_COLOR_1                    0x000000D0
#define DF_ALPHA_CONTROL_1                  0x000000D8
#define DF_ALPHA_XPOS_2                     0x000000E0
#define DF_ALPHA_YPOS_2                     0x000000E8
#define DF_ALPHA_COLOR_2                    0x000000F0
#define DF_ALPHA_CONTROL_2                  0x000000F8
#define DF_ALPHA_XPOS_3                     0x00000100
#define DF_ALPHA_YPOS_3                     0x00000108
#define DF_ALPHA_COLOR_3                    0x00000110
#define DF_ALPHA_CONTROL_3                  0x00000118
#define DF_VIDEO_REQUEST                    0x00000120
#define DF_ALPHA_WATCH                      0x00000128
#define DF_VIDEO_TEST_MODE                  0x00000130
#define DF_VID_YPOS_EVEN                    0x00000138
#define DF_VID_ALPHA_Y_EVEN_1               0x00000140
#define DF_VID_ALPHA_Y_EVEN_2               0x00000148
#define DF_VID_ALPHA_Y_EVEN_3               0x00000150
#define DF_VIDEO_PANEL_TIM1                 0x00000400
#define DF_VIDEO_PANEL_TIM2                 0x00000408
#define DF_POWER_MANAGEMENT                 0x00000410
#define DF_DITHER_CONTROL                   0x00000418
#define DF_DITHER_ACCESS                    0x00000448
#define DF_DITHER_DATA                      0x00000450
#define DF_PANEL_CRC                        0x00000458
#define DF_PANEL_CRC32                      0x00000468
#define DF_COEFFICIENT_BASE                 0x00001000

/* DF_VIDEO_CONFIG BIT DEFINITIONS */

#define DF_VCFG_VID_EN                      0x00000001
#define DF_VCFG_VID_INP_FORMAT              0x0000000C
#define DF_VCFG_SC_BYP                      0x00000020
#define DF_VCFG_LINE_SIZE_LOWER_MASK        0x0000FF00
#define DF_VCFG_INIT_READ_MASK              0x01FF0000
#define DF_VCFG_LINE_SIZE_BIT8              0x08000000
#define DF_VCFG_LINE_SIZE_BIT9              0x04000000
#define DF_VCFG_4_2_0_MODE                  0x10000000
#define DF_VCFG_UYVY_FORMAT                 0x00000000
#define DF_VCFG_Y2YU_FORMAT                 0x00000004
#define DF_VCFG_YUYV_FORMAT                 0x00000008
#define DF_VCFG_YVYU_FORMAT                 0x0000000C

/* DF_DISPLAY_CONFIG BIT DEFINITIONS */

#define DF_DCFG_DIS_EN                      0x00000001
#define DF_DCFG_HSYNC_EN                    0x00000002
#define DF_DCFG_VSYNC_EN                    0x00000004
#define DF_DCFG_DAC_BL_EN                   0x00000008
#define DF_DCFG_CRT_HSYNC_POL               0x00000100
#define DF_DCFG_CRT_VSYNC_POL               0x00000200
#define DF_DCFG_CRT_SYNC_SKW_MASK           0x0001C000
#define DF_DCFG_CRT_SYNC_SKW_INIT           0x00010000
#define DF_DCFG_PWR_SEQ_DLY_MASK            0x000E0000
#define DF_DCFG_PWR_SEQ_DLY_INIT            0x00080000
#define DF_DCFG_VG_CK                       0x00100000
#define DF_DCFG_GV_PAL_BYP                  0x00200000
#define DF_DAC_VREF                         0x04000000

/* DF_VID_MISC BIT DEFINITIONS */

#define DF_GAMMA_BYPASS_BOTH                0x00000001
#define DF_DAC_POWER_DOWN                   0x00000400
#define DF_ANALOG_POWER_DOWN                0x00000800
#define DF_USER_IMPLICIT_SCALING            0x00001000

/* DF_VID_ALPHA_CONTROL DEFINITIONS */

#define DF_HD_VIDEO                         0x00000040
#define DF_YUV_CSC_EN                       0x00000080
#define DF_NO_CK_OUTSIDE_ALPHA              0x00000100
#define DF_HD_GRAPHICS                      0x00000200
#define DF_CSC_VIDEO_YUV_TO_RGB             0x00000400
#define DF_CSC_GRAPHICS_RGB_TO_YUV          0x00000800
#define DF_CSC_VOP_RGB_TO_YUV               0x00001000
#define DF_VIDEO_INPUT_IS_RGB               0x00002000
#define DF_VID_ALPHA_EN                     0x00004000
#define DF_ALPHA_DRGB                       0x00008000

/* VIDEO CURSOR COLOR KEY DEFINITIONS */

#define DF_CURSOR_COLOR_KEY_ENABLE          0x20000000

/* ALPHA COLOR BIT DEFINITION */

#define DF_ALPHA_COLOR_ENABLE               0x01000000

/* ALPHA CONTROL BIT DEFINITIONS */

#define DF_ACTRL_WIN_ENABLE                 0x00010000
#define DF_ACTRL_LOAD_ALPHA	                0x00020000
#define DF_ACTRL_PERPIXEL_EN                0x00040000

/* DF_VIDEO_SCALER DEFINITIONS */

#define DF_SCALE_128_PHASES                 0x00002000
#define DF_SCALE_DOUBLE_H_DOWNSCALE         0x00004000

/* DEFAULT PANEL TIMINGS DEFINITIONS */

#define DF_DEFAULT_TFT_PMTIM1               0x00000000
#define DF_DEFAULT_XVGA_PMTIM1              0x00000000
#define DF_DEFAULT_TFT_PMTIM2               0x08C00000
#define DF_DEFAULT_XVGA_PMTIM2              0x08C10000
#define DF_DEFAULT_TFT_PAD_SEL_LOW          0xDFFFFFFF
#define DF_DEFAULT_TFT_PAD_SEL_HIGH         0x0000003F
#define DF_DEFAULT_XVGA_PAD_SEL_LOW         0x00000000
#define DF_DEFAULT_XVGA_PAD_SEL_HIGH        0x00000000
#define DF_DEFAULT_DITHCTL                  0x00000070
#define DF_DEFAULT_TV_PAD_SEL_HIGH          0x000000BF
#define DF_DEFAULT_TV_PAD_SEL_LOW           0xDFFFFFFF
#define DF_INVERT_VOP_CLOCK                 0x00000080

/* DF_VIDEO_PANEL_TIM2 DEFINITIONS */

#define DF_PMTIM2_TFT_PASSHTHROUGH          0x40000000

/* DF_POWER_MANAGEMENT DEFINITIONS */

#define DF_PM_PANEL_ON                      0x01000000
#define DF_PM_INVERT_SHFCLK                 0x00002000

/* DISPLAY FILTER MSRS */

#define DF_MBD_MSR_DIAG_DF                  0x2010
#define DF_MSR_PAD_SEL                      0x2011
#define DF_DIAG_32BIT_CRC                   0x80000000

#define DF_OUTPUT_CRT                       0x00000000
#define DF_OUTPUT_PANEL                     0x00000008
#define DF_OUTPUT_VOP                       0x00000030
#define DF_OUTPUT_DRGB                      0x00000038
#define DF_SIMULTANEOUS_CRT_FP              0x00008000
#define DF_CONFIG_OUTPUT_MASK               0x00000038

/*----------------------------------------------------------------*/
/*                       MSR DEFINITIONS                          */
/*----------------------------------------------------------------*/

/*----------------------------*/
/* STATIC GEODELINK ADDRESSES */
/*----------------------------*/

#define MSR_ADDRESS_GLIU0                   0x10000000
#define MSR_ADDRESS_GLIU1                   0x40000000
#define MSR_ADDRESS_GLIU2                   0x51010000
#define MSR_ADDRESS_5535MPCI                0x51000000
#define MSR_ADDRESS_VAIL                    0x00000000

/*----------------------------*/
/* UNIVERSAL DEVICE MSRS      */
/*----------------------------*/

#define MSR_GEODELINK_CAP                   0x2000
#define MSR_GEODELINK_CONFIG                0x2001
#define MSR_GEODELINK_SMI                   0x2002
#define MSR_GEODELINK_ERROR                 0x2003
#define MSR_GEODELINK_PM                    0x2004
#define MSR_GEODELINK_DIAG                  0x2005

/*----------------------------*/
/* DEVICE CLASS CODES         */
/*----------------------------*/

#define MSR_CLASS_CODE_GLIU                 0x01
#define MSR_CLASS_CODE_GLCP                 0x02
#define MSR_CLASS_CODE_MPCI                 0x05
#define MSR_CLASS_CODE_MC                   0x20
#define MSR_CLASS_CODE_GP                   0x3D
#define MSR_CLASS_CODE_VG                   0x3E
#define MSR_CLASS_CODE_DF                   0x3F
#define MSR_CLASS_CODE_FG                   0xF0
#define MSR_CLASS_CODE_VAIL                 0x86
#define MSR_CLASS_CODE_USB                  0x42
#define MSR_CLASS_CODE_USB2                 0x43
#define MSR_CLASS_CODE_ATAC	                0x47
#define MSR_CLASS_CODE_MDD 	                0xDF
#define MSR_CLASS_CODE_ACC 	                0x33
#define MSR_CLASS_CODE_AES                  0x30
#define MSR_CLASS_CODE_VIP                  0x3C
#define MSR_CLASS_CODE_REFLECTIVE           0xFFF
#define MSR_CLASS_CODE_UNPOPULATED          0x7FF

/*----------------------------*/
/*   GLIU MSR DEFINITIONS     */
/*----------------------------*/

#define MSR_GLIU_CAP                        0x0086
#define MSR_GLIU_WHOAMI			            0x008B

#define NUM_PORTS_MASK                      0x00380000
#define NUM_PORTS_SHIFT                     19
#define WHOAMI_MASK			                0x07

/*----------------------------*/
/*   GLCP MSR DEFINITIONS     */
/*----------------------------*/

#define GLCP_CLKOFF                         0x0010
#define GLCP_CLKACTIVE                      0x0011
#define GLCP_CLKDISABLE                     0x0012
#define GLCP_CLK4ACK                        0x0013
#define GLCP_SYS_RSTPLL                     0x0014
#define GLCP_DOTPLL                         0x0015
#define GLCP_DBGCLKCTL                      0x0016
#define GLCP_REVID                          0x0017
#define GLCP_RAW_DIAG                       0x0028
#define GLCP_SETM0CTL                       0x0040
#define GLCP_SETN0CTL                       0x0048
#define GLCP_CMPVAL0                        0x0050
#define GLCP_CMPMASK0                       0x0051
#define GLCP_REGA                           0x0058
#define GLCP_REGB                           0x0059
#define GLCP_REGAMASK                       0x005A
#define GLCP_REGAVAL                        0x005B
#define GLCP_REGBMASK                       0x005C
#define GLCP_REGBVAL                        0x005D
#define GLCP_FIFOCTL                        0x005E
#define GLCP_DIAGCTL                        0x005F
#define GLCP_H0CTL                          0x0060
#define GLCP_XSTATE                         0x0066
#define GLCP_YSTATE                         0x0067
#define GLCP_ACTION0                        0x0068

/* GLCP_DOTPLL DEFINITIONS */

#define GLCP_DOTPLL_RESET                   0x00000001
#define GLCP_DOTPLL_BYPASS                  0x00008000
#define GLCP_DOTPLL_HALFPIX                 0x01000000
#define GLCP_DOTPLL_LOCK                    0x02000000
#define GLCP_DOTPLL_VIPCLK                  0x00008000
#define GLCP_DOTPLL_DIV4                    0x00010000

/* GLCP DIAG DEFINITIONS */

#define GLCP_MBD_DIAG_SEL0                  0x00000007
#define GLCP_MBD_DIAG_EN0                   0x00008000
#define GLCP_MBD_DIAG_SEL1                  0x00070000
#define GLCP_MBD_DIAG_EN1                   0x80000000

/*--------------------------------*/
/* DISPLAY FILTER MSR DEFINITIONS */
/*--------------------------------*/

/* DISPLAY FILTER MBD_MSR_DIAG DEFINITIONS */

#define DF_MBD_DIAG_SEL0                    0x00007FFF
#define DF_MBD_DIAG_EN0                     0x00008000
#define DF_MBD_DIAG_SEL1                    0x7FFF0000
#define DF_MBD_DIAG_EN1                     0x80000000

/* DISPLAY FILTER MBD_MSR_CONFIG DEFINITIONS */

#define DF_CONFIG_FMT_MASK                  0x00000038
#define DF_CONFIG_FMT_CRT                   0x00000000
#define DF_CONFIG_FMT_FP                    0x00000008

/*----------------------------------------------------------------*/
/*                       PCI DEFINITIONS                          */
/*----------------------------------------------------------------*/

#define PCI_VENDOR_DEVICE_GEODEGX           0x0028100B
#define PCI_VENDOR_DEVICE_GEODEGX_VIDEO     0x0030100B
#define PCI_VENDOR_DEVICE_GEODELX           0x20801022
#define PCI_VENDOR_DEVICE_GEODELX_VIDEO     0x20811022
#define PCI_VENDOR_5535                     0x002B100B
#define PCI_VENDOR_5536                     0x20901022

/*----------------------------------------------------------------*/
/*                       VIP DEFINITIONS                          */
/*----------------------------------------------------------------*/

#define VIP_CONTROL1                        0x00000000
#define VIP_CONTROL2                        0x00000004
#define VIP_STATUS                          0x00000008
#define VIP_INTERRUPT                       0x0000000C
#define VIP_CURRENT_TARGET                  0x00000010
#define VIP_MAX_ADDRESS                     0x00000014
#define	VIP_TASKA_VID_EVEN_BASE             0x00000018
#define	VIP_TASKA_VID_ODD_BASE              0x0000001C
#define	VIP_TASKA_VBI_EVEN_BASE             0x00000020
#define	VIP_TASKA_VBI_ODD_BASE              0x00000024
#define VIP_TASKA_VID_PITCH                 0x00000028
#define VIP_CONTROL3                        0x0000002C
#define VIP_TASKA_V_OFFSET                  0x00000030
#define VIP_TASKA_U_OFFSET                  0x00000034
#define	VIP_TASKB_VID_EVEN_BASE             0x00000038
#define VIP_601_HORZ_END                    0x00000038
#define	VIP_TASKB_VID_ODD_BASE              0x0000003C
#define	VIP_601_HORZ_START                  0x0000003C
#define	VIP_TASKB_VBI_EVEN_BASE             0x00000040
#define	VIP_601_VBI_END                     0x00000040
#define	VIP_TASKB_VBI_ODD_BASE              0x00000044
#define	VIP_601_VBI_START                   0x00000044
#define VIP_TASKB_VID_PITCH                 0x00000048
#define VIP_601_EVEN_START_STOP             0x00000048
#define VIP_TASKB_V_OFFSET                  0x00000050
#define VIP_ODD_FIELD_DETECT                0x00000050
#define VIP_TASKB_U_OFFSET                  0x00000054
#define	VIP_ANC_MSG1_BASE                   0x00000058
#define	VIP_ANC_MSG2_BASE                   0x0000005C
#define	VIP_ANC_MSG_SIZE                    0x00000060
#define VIP_PAGE_OFFSET                     0x00000068
#define VIP_VERTICAL_START_STOP             0x0000006C
#define VIP_601_ODD_START_STOP              0x0000006C
#define VIP_FIFO_ADDRESS                    0x00000070
#define VIP_FIFO_DATA                       0x00000074
#define VIP_VSYNC_ERR_COUNT                 0x00000078
#define VIP_TASKA_U_EVEN_OFFSET             0x0000007C
#define VIP_TASKA_V_EVEN_OFFSET             0x00000080

/* INDIVIDUAL REGISTER BIT DEFINITIONS                          */
/* Multibit register subsets are expressed as a mask and shift. */
/* Single bit values are represented as a mask.                 */

/* VIP_CONTROL1 REGISTER DEFINITIONS */

#define VIP_CONTROL1_DEFAULT_ANC_FF         2
#define VIP_CONTROL1_ANC_FF_MASK            0xE0000000
#define VIP_CONTROL1_ANC_FF_SHIFT           29

#define VIP_CONTROL1_DEFAULT_VID_FF         2
#define VIP_CONTROL1_VID_FF_MASK            0x1F000000
#define VIP_CONTROL1_VID_FF_SHIFT           24

#define VIP_CONTROL1_VDE_FF_MASK            0x00F00000
#define VIP_CONTROL1_VDE_FF_SHIFT           20

#define VIP_CONTROL1_NON_INTERLACED         (1L << 19)
#define VIP_CONTROL1_MSG_STRM_CTRL          (1L << 18)
#define VIP_CONTROL1_DISABLE_ZERO_DETECT    (1L << 17)
#define VIP_CONTROL1_DISABLE_DECIMATION     (1L << 16)

#define VIP_CONTROL1_CAPTURE_ENABLE_MASK    0x0000FF00
#define VIP_CONTROL1_CAPTURE_ENABLE_SHIFT   8

#define VIP_CONTROL1_RUNMODE_MASK           0x000000E0
#define VIP_CONTROL1_RUNMODE_SHIFT          5

#define VIP_CONTROL1_PLANAR                 (1L << 4)

#define VIP_CONTROL1_MODE_MASK              0x0000000E
#define VIP_CONTROL1_MODE_SHIFT             1

#define VIP_CONTROL1_RESET                  0x00000001

/* VIP_CONTROL2 REGISTER DEFINITIONS */

#define VIP_CONTROL2_INVERT_POLARITY        (1L << 31)
#define VIP_CONTROL2_ADD_ERROR_ENABLE       (1L << 30)
#define VIP_CONTROL2_REPEAT_ENABLE          (1L << 29)
#define VIP_CONTROL2_SWC_ENABLE             (1L << 28)
#define VIP_CONTROL2_ANC10                  (1L << 27)
#define VIP_CONTROL2_ANCPEN                 (1L << 26)
#define VIP_CONTROL2_LOOPBACK_ENABLE        (1L << 25)
#define VIP_CONTROL2_FIFO_ACCESS            (1L << 24)
#define VIP_CONTROL2_VERTERROR_ENABLE       (1L << 15)

#define VIP_CONTROL2_PAGECNT_MASK           0x00E00000
#define VIP_CONTROL2_PAGECNT_SHIFT          21

#define VIP_CONTROL2_DEFAULT_ANCTH          5
#define VIP_CONTROL2_ANCTH_MASK             0x001F0000
#define VIP_CONTROL2_ANCTH_SHIFT            16

#define VIP_CONTROL2_DEFAULT_VIDTH_420      19
#define VIP_CONTROL2_DEFAULT_VIDTH_422      19
#define VIP_CONTROL2_VIDTH_MASK             0x00007F00
#define VIP_CONTROL2_VIDTH_SHIFT            8

#define VIP_CONTROL2_SYNC2PIN_MASK          0x000000E0
#define VIP_CONTROL2_SYNC2PIN_SHIFT         5

#define VIP_CONTROL2_FIELD2VG_MASK          0x00000018
#define VIP_CONTROL2_FIELD2VG_SHIFT         3

#define VIP_CONTROL2_SYNC2VG_MASK           0x00000007
#define VIP_CONTROL2_SYNC2VG_SHIFT          0

/* VIP_CONTROL3 REGISTER DEFINITIONS */

#define VIP_CONTROL3_PLANAR_DEINT           0x00000400
#define VIP_CONTROL3_BASE_UPDATE            0x00000200
#define VIP_CONTROL3_DISABLE_OVERFLOW       0x00000100
#define VIP_CONTROL3_DECIMATE_EVEN          0x00000080
#define VIP_CONTROL3_TASK_POLARITY          0x00000040
#define VIP_CONTROL3_VSYNC_POLARITY         0x00000020
#define VIP_CONTROL3_HSYNC_POLARITY         0x00000010
#define VIP_CONTROL3_FIFO_RESET             0x00000001

/* VIP_STATUS REGISTER DEFINITIONS */

#define VIP_STATUS_ANC_COUNT_MASK           0xFF000000
#define VIP_STATUS_ANC_COUNT_SHIFT          24

#define VIP_STATUS_FIFO_ERROR               0x00700000
#define VIP_STATUS_ERROR_SHIFT              20
#define VIP_STATUS_DEC_COUNT                (1L << 18)
#define VIP_STATUS_SYNCOUT                  (1L << 17)
#define VIP_STATUS_BASEREG_NOTUPDT          (1L << 16)
#define VIP_STATUS_MSGBUFF_ERR              (1L << 14)
#define VIP_STATUS_MSGBUFF2_FULL            (1L << 13)
#define VIP_STATUS_MSGBUFF1_FULL            (1L << 12)
#define VIP_STATUS_WRITES_COMPLETE          (1L << 9)
#define VIP_STATUS_FIFO_EMPTY               (1L << 8)
#define VIP_STATUS_FIELD                    (1L << 4)
#define VIP_STATUS_VBLANK                   (1L << 3)

#define VIP_STATUS_RUN_MASK                 0x00000007
#define VIP_STATUS_RUN_SHIFT                0

/* VIP_CURRENT_TARGET REGISTER DEFINITIONS */

#define VIP_CTARGET_TLINE_MASK              0xFFFF0000
#define VIP_CTARGET_TLINE_SHIFT             16

#define VIP_CTARGET_CLINE_MASK              0x0000FFFF
#define VIP_CTARGET_CLINE_SHIFT             0

/* VIP_MAX_ADDRESS REGISTER DEFINITIONS */

#define VIP_MAXADDR_MASK                    0xFFFFFFFF
#define VIP_MAXADDR_SHIFT                   0

/* VIP BUFFER PITCH DEFINITIONS */

#define VIP_TASK_PITCH_MASK                 0x0000FFFF
#define VIP_TASK_PITCH_SHIFT                0

/* VERTICAL START/STOP */

#define VIP_VSTART_VERTEND_MASK             0x0FFF0000
#define VIP_VSTART_VERTEND_SHIFT            16

#define VIP_VSTART_VERTSTART_MASK           0x00000FFF
#define VIP_VSTART_VERTSTART_SHIFT          0

/* VIP FIFO ADDRESS DEFINITIONS */

#define VIP_FIFO_ADDRESS_MASK               0x000000FF
#define VIP_FIFO_ADDRESS_SHIFT              0

/* VIP VSYNC ERROR DEFINITIONS */

#define VIP_VSYNC_ERR_WINDOW_MASK           0xFF000000
#define VIP_VSYNC_ERR_WINDOW_SHIFT          24

#define VIP_VSYNC_ERR_COUNT_MASK            0x00FFFFFF
#define VIP_VSYNC_ERR_COUNT_SHIFT           0

/*---------------------*/
/* VIP MSR DEFINITIONS */
/*---------------------*/

/* CAPABILITIES */

#define VIP_MSR_CAP_NSMI_MASK               0xF8000000
#define VIP_MSR_CAP_NSMI_SHIFT              27
#define VIP_MSR_CAP_NCLK_MASK               0x07000000
#define VIP_MSR_CAP_NCLK_SHIFT              24
#define VIP_MSR_CAP_DEVID_MASK              0x00FFFF00
#define VIP_MSR_CAP_DEVID_SHIFT             8
#define VIP_MSR_CAP_REVID_MASK              0x000000FF
#define VIP_MSR_CAP_REVID_SHIFT             0

/* MASTER CONFIG */

#define VIP_MSR_MCR_SECOND_PRIORITY_MASK    0x00000700
#define VIP_MSR_MCR_SECOND_PRIORITY_SHIFT   8
#define VIP_MSR_MCR_PRIMARY_PRIORITY_MASK   0x00000070
#define VIP_MSR_MCR_PRIMARY_PRIORITY_SHIFT  4
#define VIP_MSR_MCR_PID_MASK                0x00000007
#define VIP_MSR_MCR_PID_SHIFT               0

/* VIP SMI */

#define VIP_MSR_SMI_FIFO_OVERFLOW           (1L << 29)
#define VIP_MSR_SMI_FIFO_THRESHOLD          (1L << 28)
#define VIP_MSR_SMI_LONGLINE                (1L << 27)
#define VIP_MSR_SMI_VERTICAL_TIMING         (1L << 26)
#define VIP_MSR_SMI_ACTIVE_PIXELS           (1L << 25)
#define VIP_MSR_SMI_CLOCK_INPUT             (1L << 24)
#define VIP_MSR_SMI_ANC_CHECKSUM_PARITY     (1L << 23)
#define VIP_MSR_SMI_MSG_BUFFER_FULL         (1L << 22)
#define VIP_MSR_SMI_END_VBLANK              (1L << 21)
#define VIP_MSR_SMI_START_VBLANK            (1L << 20)
#define VIP_MSR_SMI_START_EVEN              (1L << 19)
#define VIP_MSR_SMI_START_ODD               (1L << 18)
#define VIP_MSR_SMI_LINE_MATCH_TARGET       (1L << 17)
#define VIP_MSR_SMI_GLINK                   (1L << 16)

/* VIP ERROR */

#define VIP_MSR_ERROR_ADDRESS_MASK          (1L << 17)
#define VIP_MSR_ERROR_ADDRESS_SHIFT         17
#define VIP_MSR_ERROR_ADDRESS_ENABLE        (1L << 1)
#define VIP_MSR_ERROR_ADDRESS_EN_SHIFT      1
#define VIP_MSR_ERROR_TYPE_MASK             (1L << 16)
#define VIP_MSR_ERROR_TYPE_SHIFT            16
#define VIP_MSR_ERROR_TYPE_ENABLE           1
#define VIP_MSR_ERROR_TYPE_EN_SHIFT         0

/* VIP POWER */

#define VIP_MSR_POWER_GLINK                 (1L << 0)
#define VIP_MSR_POWER_CLOCK                 (1L << 2)

/* VIP DIAG */

#define VIP_MSR_DIAG_BIST_WMASK             0x00000003
#define VIP_MSR_DIAG_BIST_RMASK             0x00000007
#define VIP_MSR_DIAG_BIST_SHIFT             0

#define VIP_MSR_DIAG_MSB_ENABLE             (1L << 31)
#define VIP_MSR_DIAG_SEL_UPPER_MASK         0x7FFF0000
#define VIP_MSR_DIAG_SEL_UPPER_SHIFT        16
#define VIP_MSR_DIAG_LSB_ENABLE             (1L << 15)
#define VIP_MSR_DIAG_SEL_LOWER_MASK         0x00007FFF
#define VIP_MSR_DIAG_SEL_LOWER_SHIFT        0

/*----------------------------------------------------------------*/
/*                       VOP DEFINITIONS                          */
/*----------------------------------------------------------------*/

#define VOP_CONFIGURATION                   0x00000800
#define VOP_SIGNATURE                       0x00000808

/* VOP_CONFIGURATION BIT DEFINITIONS */

#define VOP_CONFIG_SWAPVBI                  0x01000000
#define VOP_CONFIG_RGBMODE                  0x00200000
#define VOP_CONFIG_SIGVAL                   0x00100000
#define VOP_CONFIG_INVERT_DISPE             0x00080000
#define VOP_CONFIG_INVERT_VSYNC             0x00040000
#define VOP_CONFIG_INVERT_HSYNC             0x00020000
#define VOP_CONFIG_SWAPUV                   0x00010000
#define VOP_CONFIG_VSYNC_MASK               0x0000C000
#define VOP_CONFIG_DISABLE_DECIMATE         0x00002000
#define VOP_CONFIG_ENABLE_601               0x00001000
#define VOP_CONFIG_VBI                      0x00000800
#define VOP_CONFIG_TASK                     0x00000200
#define VOP_CONFIG_SIG_FREE_RUN             0x00000100
#define VOP_CONFIG_ENABLE_SIGNATURE         0x00000080
#define VOP_CONFIG_SC_COMPATIBLE            0x00000040
#define VOP_CONFIG_422_COSITED              0x00000000
#define VOP_CONFIG_422_INTERSPERSED         0x00000010
#define VOP_CONFIG_422_ALTERNATING          0x00000020
#define VOP_CONFIG_422_MASK                 0x00000030
#define VOP_CONFIG_EXTENDED_SAV             0x00000008
#define VOP_CONFIG_VIP2_16BIT               0x00000004
#define VOP_CONFIG_DISABLED                 0x00000000
#define VOP_CONFIG_VIP1_1                   0x00000001
#define VOP_CONFIG_VIP2_0                   0x00000002
#define VOP_CONFIG_CCIR656                  0x00000003
#define VOP_CONFIG_MODE_MASK                0x00000003

#endif
