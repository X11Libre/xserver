/*
 * Copyright (c) 2005 ASPEED Technology Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the authors not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The authors makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THE AUTHORS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* Eng Capability Definition */
#define	ENG_CAP_Sync				0x0001
#define	ENG_CAP_ScreenToScreenCopy		0x0002
#define	ENG_CAP_SolidFill			0x0004
#define	ENG_CAP_SolidLine			0x0008
#define	ENG_CAP_DashedLine			0x0010
#define	ENG_CAP_Mono8x8PatternFill		0x0020
#define	ENG_CAP_Color8x8PatternFill		0x0040
#define	ENG_CAP_CPUToScreenColorExpand		0x0080
#define	ENG_CAP_ScreenToScreenColorExpand	0x0100
#define	ENG_CAP_Clipping			0x0200
#define ENG_CAP_ALL	(ENG_CAP_Sync | ENG_CAP_ScreenToScreenCopy | ENG_CAP_SolidFill	|	\
                         ENG_CAP_SolidLine | ENG_CAP_DashedLine |				\
                         ENG_CAP_Mono8x8PatternFill | ENG_CAP_Color8x8PatternFill |		\
                         ENG_CAP_Clipping);

/* CMDQ Definition */
#define    AGP_CMD_QUEUE                   	1
#define    VM_CMD_QUEUE                    	0
#define    VM_CMD_MMIO                     	2

#define    CMD_QUEUE_SIZE_256K             	0x00040000
#define    CMD_QUEUE_SIZE_512K             	0x00080000
#define    CMD_QUEUE_SIZE_1M               	0x00100000
#define    CMD_QUEUE_SIZE_2M               	0x00200000
#define    CMD_QUEUE_SIZE_4M               	0x00400000

#define PIXEL_FMT_YV12 				FOURCC_YV12  		/* 0x32315659 */
#define PIXEL_FMT_UYVY 				FOURCC_UYVY  		/* 0x59565955 */
#define PIXEL_FMT_YUY2 				FOURCC_YUY2  		/* 0x32595559 */
#define PIXEL_FMT_RGB5 				0x35315652
#define PIXEL_FMT_RGB6 				0x36315652
#define PIXEL_FMT_YVYU 				0x55595659
#define PIXEL_FMT_NV12 				0x3231564e
#define PIXEL_FMT_NV21 				0x3132564e

/* CMD Type Info */
#define    PKT_NULL_CMD             		0x00009561
#define    PKT_BURST_CMD_HEADER0    	    	0x00009564

#define    PKT_SINGLE_LENGTH        		8
#define    PKT_SINGLE_CMD_HEADER    		0x00009562

typedef struct  _PKT_SC
{
    ULONG    PKT_SC_dwHeader;
    ULONG    PKT_SC_dwData[1];

} PKT_SC, *PPKT_SC;

/* Packet CMD Scale */
#define    PKT_TYPESCALE_LENGTH        		56
#define    PKT_TYPESCALE_DATALENGTH    		(0xC<<16)
#define    PKT_TYPESCALE_ADDRSTART     		0x00000000

typedef struct _BURSTSCALECMD
{
    ULONG    dwHeader0;
    ULONG    dwSrcBaseAddr;         		/* 8000 */
    union
    {
        struct
        {
            USHORT    wSrcDummy;      		/* 8004 */
            USHORT    wSrcPitch;      		/* 8006 */
        };
        ULONG    dwSrcPitch;         		/* 8004 */
    };
    ULONG    dwDstBaseAddr;         		/* 8008 */
    union
    {
        struct
        {
            USHORT   wDstHeight;      		/* 800C */
            USHORT   wDstPitch;       		/* 800E */
        };
        ULONG    dwDstHeightPitch;   		/* 800C */
    };
    union
    {
        struct
        {
            short    wDstY;           		/* 8010 */
            short    wDstX;           		/* 8012 */
        };
        ULONG    dwDstXY;            		/* 8010 */
    };
    union
    {
        struct
        {
            short    wSrcY;           		/* 8014 */
            short    wSrcX;           		/* 8016 */
        };
        ULONG    dwSrcXY;            		/* 8014 */
    };
    union
    {
        struct
        {
            USHORT   wRecHeight;      		/* 8018 */
            USHORT   wRecWidth;       		/* 801A */
        };
        ULONG    dwRecHeightWidth;   		/* 8018 */
    };
    ULONG    dwInitScaleFactorH;        	/* 801C */
    ULONG    dwInitScaleFactorV;        	/* 8020 */
    ULONG    dwScaleFactorH;            	/* 8024 */
    ULONG    dwScaleFactorV;            	/* 8028 */

    ULONG    dwCmd;             	    	/* 823C */
    ULONG    NullData[1];
} BURSTSCALECMD, *PBURSTSCALECMD;

/* Eng Reg. Limitation */
#define	MAX_SRC_X				0x7FF
#define	MAX_SRC_Y				0x7FF
#define	MAX_DST_X				0x7FF
#define	MAX_DST_Y				0x7FF

#define	MASK_SRC_PITCH				0x1FFF
#define	MASK_DST_PITCH				0x1FFF
#define	MASK_DST_HEIGHT				0x7FF
#define	MASK_SRC_X				0xFFF
#define	MASK_SRC_Y				0xFFF
#define	MASK_DST_X				0xFFF
#define	MASK_DST_Y				0xFFF
#define	MASK_RECT_WIDTH				0x7FF
#define	MASK_RECT_HEIGHT			0x7FF
#define MASK_CLIP				0xFFF

#define MASK_LINE_X        			0xFFF
#define MASK_LINE_Y           			0xFFF
#define MASK_LINE_ERR   			0x3FFFFF
#define MASK_LINE_WIDTH   			0x7FF
#define MASK_LINE_K1				0x3FFFFF
#define MASK_LINE_K2				0x3FFFFF
#define MASK_AIPLINE_X        			0xFFF
#define MASK_AIPLINE_Y         			0xFFF

#define MAX_PATReg_Size				256

/* Eng Reg. Definition */
/* MMIO Reg */
#define MMIOREG_SRC_BASE	(pAST->MMIOVirtualAddr + 0x8000)
#define MMIOREG_SRC_PITCH	(pAST->MMIOVirtualAddr + 0x8004)
#define MMIOREG_DST_BASE	(pAST->MMIOVirtualAddr + 0x8008)
#define MMIOREG_DST_PITCH	(pAST->MMIOVirtualAddr + 0x800C)
#define MMIOREG_DST_XY		(pAST->MMIOVirtualAddr + 0x8010)
#define MMIOREG_SRC_XY		(pAST->MMIOVirtualAddr + 0x8014)
#define MMIOREG_RECT_XY		(pAST->MMIOVirtualAddr + 0x8018)
#define MMIOREG_FG		(pAST->MMIOVirtualAddr + 0x801C)
#define MMIOREG_BG		(pAST->MMIOVirtualAddr + 0x8020)
#define MMIOREG_FG_SRC		(pAST->MMIOVirtualAddr + 0x8024)
#define MMIOREG_BG_SRC		(pAST->MMIOVirtualAddr + 0x8028)
#define MMIOREG_MONO1		(pAST->MMIOVirtualAddr + 0x802C)
#define MMIOREG_MONO2		(pAST->MMIOVirtualAddr + 0x8030)
#define MMIOREG_CLIP1		(pAST->MMIOVirtualAddr + 0x8034)
#define MMIOREG_CLIP2		(pAST->MMIOVirtualAddr + 0x8038)
#define MMIOREG_CMD		(pAST->MMIOVirtualAddr + 0x803C)
#define MMIOREG_PAT		(pAST->MMIOVirtualAddr + 0x8100)

#define MMIOREG_LINE_XY         (pAST->MMIOVirtualAddr + 0x8010)
#define MMIOREG_LINE_Err        (pAST->MMIOVirtualAddr + 0x8014)
#define MMIOREG_LINE_WIDTH      (pAST->MMIOVirtualAddr + 0x8018)
#define MMIOREG_LINE_K1         (pAST->MMIOVirtualAddr + 0x8024)
#define MMIOREG_LINE_K2         (pAST->MMIOVirtualAddr + 0x8028)
#define MMIOREG_LINE_STYLE1     (pAST->MMIOVirtualAddr + 0x802C)
#define MMIOREG_LINE_STYLE2     (pAST->MMIOVirtualAddr + 0x8030)
#define MMIOREG_LINE_XY2        (pAST->MMIOVirtualAddr + 0x8014)
#define MMIOREG_LINE_NUMBER     (pAST->MMIOVirtualAddr + 0x8018)

/* CMDQ Reg */
#define CMDQREG_SRC_BASE	(0x00 << 24)
#define CMDQREG_SRC_PITCH	(0x01 << 24)
#define CMDQREG_DST_BASE	(0x02 << 24)
#define CMDQREG_DST_PITCH	(0x03 << 24)
#define CMDQREG_DST_XY		(0x04 << 24)
#define CMDQREG_SRC_XY		(0x05 << 24)
#define CMDQREG_RECT_XY		(0x06 << 24)
#define CMDQREG_FG		(0x07 << 24)
#define CMDQREG_BG		(0x08 << 24)
#define CMDQREG_FG_SRC		(0x09 << 24)
#define CMDQREG_BG_SRC		(0x0A << 24)
#define CMDQREG_MONO1		(0x0B << 24)
#define CMDQREG_MONO2		(0x0C << 24)
#define CMDQREG_CLIP1		(0x0D << 24)
#define CMDQREG_CLIP2		(0x0E << 24)
#define CMDQREG_CMD		(0x0F << 24)
#define CMDQREG_PAT		(0x40 << 24)

#define CMDQREG_LINE_XY         (0x04 << 24)
#define CMDQREG_LINE_Err        (0x05 << 24)
#define CMDQREG_LINE_WIDTH      (0x06 << 24)
#define CMDQREG_LINE_K1         (0x09 << 24)
#define CMDQREG_LINE_K2         (0x0A << 24)
#define CMDQREG_LINE_STYLE1     (0x0B << 24)
#define CMDQREG_LINE_STYLE2     (0x0C << 24)
#define CMDQREG_LINE_XY2        (0x05 << 24)
#define CMDQREG_LINE_NUMBER     (0x06 << 24)

/* CMD Reg. Definition */
#define   CMD_BITBLT                 		0x00000000
#define   CMD_LINEDRAW               		0x00000001
#define   CMD_COLOREXP               		0x00000002
#define   CMD_ENHCOLOREXP            		0x00000003
#define   CMD_TRANSPARENTBLT           		0x00000004
#define   CMD_TYPE_SCALE                    	0x00000005
#define   CMD_MASK            	        	0x00000007

#define   CMD_DISABLE_CLIP           		0x00000000
#define   CMD_ENABLE_CLIP            		0x00000008

#define   CMD_COLOR_08               		0x00000000
#define   CMD_COLOR_16               		0x00000010
#define   CMD_COLOR_32               		0x00000020

#define   CMD_SRC_SIQ                		0x00000040

#define   CMD_TRANSPARENT               	0x00000080

#define   CMD_PAT_FGCOLOR            		0x00000000
#define   CMD_PAT_MONOMASK           		0x00010000
#define   CMD_PAT_PATREG             		0x00020000

#define   CMD_OPAQUE                 		0x00000000
#define   CMD_FONT_TRANSPARENT          	0x00040000

#define   CMD_X_INC				0x00000000
#define   CMD_X_DEC				0x00200000

#define   CMD_Y_INC				0x00000000
#define   CMD_Y_DEC				0x00100000

#define   CMD_NT_LINE				0x00000000
#define	  CMD_NORMAL_LINE			0x00400000

#define   CMD_DRAW_LAST_PIXEL           	0x00000000
#define   CMD_NOT_DRAW_LAST_PIXEL       	0x00800000

#define   CMD_DISABLE_LINE_STYLE     		0x00000000
#define   CMD_ENABLE_LINE_STYLE      		0x40000000

#define   CMD_RESET_STYLE_COUNTER       	0x80000000
#define   CMD_NOT_RESET_STYLE_COUNTER   	0x00000000

#define   BURST_FORCE_CMD            		0x80000000

#define YUV_FORMAT_YUYV     			(0UL<<12)
#define YUV_FORMAT_YVYU     			(1UL<<12)
#define YUV_FORMAT_UYVY     			(2UL<<12)
#define YUV_FORMAT_VYUY     			(3UL<<12)

#define SCALE_FORMAT_RGB2RGB        		(0UL<<14)
#define SCALE_FORMAT_YUV2RGB        		(1UL<<14)
#define SCALE_FORMAT_RGB2RGB_DOWN   		(2UL<<14)	/* RGB32 to RGB16 */
#define SCALE_FORMAT_RGB2RGB_UP     		(3UL<<14) 	/* RGB16 to RGB32 */
#define SCALE_SEG_NUM_1         		(0x3FUL<<24) 	/* DstWi >= SrcWi */
#define SCALE_SEG_NUM_2         		(0x1FUL<<24) 	/* DstWi < SrcWi */
#define	SCALE_EQUAL_VER				(0x1UL<<23)

/* Line */
#define	LINEPARAM_XM				0x00000001
#define	LINEPARAM_X_DEC				0x00000002
#define	LINEPARAM_Y_DEC				0x00000004

typedef struct _LINEPARAM {
    USHORT	dsLineX;
    USHORT	dsLineY;
    USHORT	dsLineWidth;
    ULONG	dwErrorTerm;
    ULONG	dwK1Term;
    ULONG	dwK2Term;
    ULONG	dwLineAttributes;
} LINEPARAM, *PLINEPARAM;

typedef struct {

    LONG X1;
    LONG Y1;
    LONG X2;
    LONG Y2;

} _LINEInfo;

/* Macro */
/* MMIO 2D Macro */
#define ASTSetupSRCBase_MMIO(base) \
      { \
        do { \
           *(ULONG *)(MMIOREG_SRC_BASE) = (ULONG) (base); \
        } while (*(volatile ULONG *)(MMIOREG_SRC_BASE) != (ULONG) (base)); \
      }
#define ASTSetupSRCPitch_MMIO(pitch) \
      { \
        do { \
           *(ULONG *)(MMIOREG_SRC_PITCH) = (ULONG)(pitch << 16); \
        } while (*(volatile ULONG *)(MMIOREG_SRC_PITCH) != (ULONG)(pitch << 16)); \
      }
#define ASTSetupDSTBase_MMIO(base) \
      { \
        do { \
           *(ULONG *)(MMIOREG_DST_BASE) = (ULONG)(base); \
        } while (*(volatile ULONG *)(MMIOREG_DST_BASE) != (ULONG)(base)); \
      }
#define ASTSetupDSTPitchHeight_MMIO(pitch, height) \
      { \
        ULONG dstpitch; \
        dstpitch = (ULONG)((pitch << 16) + ((height) & MASK_DST_HEIGHT)); \
        do { \
           *(ULONG *)(MMIOREG_DST_PITCH) = dstpitch; \
        } while (*(volatile ULONG *)(MMIOREG_DST_PITCH) != dstpitch); \
      }
#define ASTSetupDSTXY_MMIO(x, y) \
      { \
        ULONG dstxy; \
        dstxy = (ULONG)(((x & MASK_DST_X) << 16) + (y & MASK_DST_Y)); \
        do { \
           *(ULONG *)(MMIOREG_DST_XY) = dstxy; \
        } while (*(volatile ULONG *)(MMIOREG_DST_XY) != dstxy); \
      }
#define ASTSetupSRCXY_MMIO(x, y) \
      { \
        ULONG srcxy; \
        srcxy = (ULONG)(((x & MASK_SRC_X) << 16) + (y & MASK_SRC_Y)); \
        do { \
           *(ULONG *)(MMIOREG_SRC_XY) = srcxy; \
        } while (*(volatile ULONG *)(MMIOREG_SRC_XY) != srcxy); \
      }
#define ASTSetupRECTXY_MMIO(x, y) \
      { \
        ULONG rectxy; \
        rectxy = (ULONG)(((x & MASK_RECT_WIDTH) << 16) + (y & MASK_RECT_WIDTH)); \
        do { \
           *(ULONG *)(MMIOREG_RECT_XY) = rectxy; \
        } while (*(volatile ULONG *)(MMIOREG_RECT_XY) != rectxy); \
      }
#define ASTSetupFG_MMIO(color) \
      { \
        do { \
           *(ULONG *)(MMIOREG_FG) = (ULONG)(color); \
        } while (*(volatile ULONG *)(MMIOREG_FG) != (ULONG)(color)); \
      }
#define ASTSetupBG_MMIO(color) \
      { \
        do { \
           *(ULONG *)(MMIOREG_BG) = (ULONG)(color); \
        } while (*(volatile ULONG *)(MMIOREG_BG) != (ULONG)(color)); \
      }
#define ASTSetupMONO1_MMIO(pat) \
      { \
        do { \
          *(ULONG *)(MMIOREG_MONO1) = (ULONG)(pat); \
        } while (*(volatile ULONG *)(MMIOREG_MONO1) != (ULONG)(pat)); \
      }
#define ASTSetupMONO2_MMIO(pat) \
      { \
        do { \
          *(ULONG *)(MMIOREG_MONO2) = (ULONG)(pat); \
        } while (*(volatile ULONG *)(MMIOREG_MONO2) != (ULONG)(pat)); \
      }
#define ASTSetupCLIP1_MMIO(left, top) \
      { \
       ULONG clip1; \
       clip1 = (ULONG)(((left & MASK_CLIP) << 16) + (top & MASK_CLIP)); \
       do { \
          *(ULONG *)(MMIOREG_CLIP1) = clip1; \
       } while (*(volatile ULONG *)(MMIOREG_CLIP1) != clip1); \
      }
#define ASTSetupCLIP2_MMIO(right, bottom) \
      { \
       ULONG clip2; \
       clip2 = (ULONG)(((right & MASK_CLIP) << 16) + (bottom & MASK_CLIP)); \
       do { \
          *(ULONG *)(MMIOREG_CLIP2) = clip2; \
       } while (*(volatile ULONG *)(MMIOREG_CLIP2) != clip2); \
      }
#define ASTSetupCMDReg_MMIO(reg) \
      { \
        *(ULONG *)(MMIOREG_CMD) = (ULONG)(reg);	\
      }
#define ASTSetupPatReg_MMIO(patreg, pat) \
      { \
       do { \
          *(ULONG *)(MMIOREG_PAT + patreg*4) = (ULONG)(pat); \
       } while (*(volatile ULONG *)(MMIOREG_PAT + patreg*4) != (ULONG)(pat)); \
      }

/* Line CMD */
#define ASTSetupLineXY_MMIO(x, y) \
      { \
        ULONG linexy; \
        linexy = (ULONG)(((x & MASK_LINE_X) << 16) + (y & MASK_LINE_Y)); \
        do { \
           *(ULONG *)(MMIOREG_LINE_XY) = linexy; \
        } while (*(volatile ULONG *)(MMIOREG_LINE_XY) != linexy); \
      }
#define ASTSetupLineXMErrTerm_MMIO(xm, err) \
      { \
        ULONG lineerr; \
        lineerr = (ULONG)((xm << 24) + (err & MASK_LINE_ERR)); \
        do { \
           *(ULONG *)(MMIOREG_LINE_Err) = lineerr; \
        } while (*(volatile ULONG *)(MMIOREG_LINE_Err) != lineerr); \
      }
#define ASTSetupLineWidth_MMIO(width) \
      { \
        ULONG linewidth; \
        linewidth = (ULONG)((width & MASK_LINE_WIDTH) << 16); \
        do { \
          *(ULONG *)(MMIOREG_LINE_WIDTH) = linewidth; \
        } while (*(volatile ULONG *)(MMIOREG_LINE_WIDTH) != linewidth); \
      }
#define ASTSetupLineK1Term_MMIO(err) \
      { \
        do { \
          *(ULONG *)(MMIOREG_LINE_K1) = (ULONG)(err & MASK_LINE_K1); \
        } while (*(volatile ULONG *)(MMIOREG_LINE_K1) != (ULONG)(err & MASK_LINE_K1)); \
      }
#define ASTSetupLineK2Term_MMIO(err) \
      { \
        do { \
           *(ULONG *)(MMIOREG_LINE_K2) = (ULONG)(err & MASK_LINE_K2); \
        } while (*(volatile ULONG *)(MMIOREG_LINE_K2) != (ULONG)(err & MASK_LINE_K2)); \
      }
#define ASTSetupLineStyle1_MMIO(pat) \
      { \
        do { \
           *(ULONG *)(MMIOREG_LINE_STYLE1) = (ULONG)(pat); \
        } while (*(volatile ULONG *)(MMIOREG_LINE_STYLE1) != (ULONG)(pat)); \
      }
#define ASTSetupLineStyle2_MMIO(pat) \
      { \
        do { \
          *(ULONG *)(MMIOREG_LINE_STYLE2) = (ULONG)(pat); \
        } while (*(volatile ULONG *)(MMIOREG_LINE_STYLE2) != (ULONG)(pat)); \
      }

/* AIP Line CMD */
#define AIPSetupLineXY_MMIO(x, y) \
      { \
        ULONG linexy; \
        linexy = (ULONG)(((x & MASK_AIPLINE_X) << 16) + (y & MASK_AIPLINE_Y)); \
        do { \
           *(ULONG *)(MMIOREG_LINE_XY) = linexy; \
        } while (*(volatile ULONG *)(MMIOREG_LINE_XY) != linexy); \
      }
#define AIPSetupLineXY2_MMIO(x, y) \
      { \
        ULONG linexy; \
        linexy = (ULONG)(((x & MASK_AIPLINE_X) << 16) + (y & MASK_AIPLINE_Y)); \
        do { \
           *(ULONG *)(MMIOREG_LINE_XY2) = linexy; \
        } while (*(volatile ULONG *)(MMIOREG_LINE_XY2) != linexy); \
      }
#define AIPSetupLineNumber_MMIO(no) \
      { \
        do { \
           *(ULONG *)(MMIOREG_LINE_NUMBER) = (ULONG) no; \
        } while (*(volatile ULONG *)(MMIOREG_LINE_NUMBER) != (ULONG) no); \
      }

/* CMDQ Mode Macro */
#define mUpdateWritePointer *(ULONG *) (pAST->CMDQInfo.pjWritePort) = (pAST->CMDQInfo.ulWritePointer >>3)

/* General CMD */
#define ASTSetupSRCBase(addr, base) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG)(PKT_SINGLE_CMD_HEADER + CMDQREG_SRC_BASE); 	\
        addr->PKT_SC_dwData[0] = (ULONG)(base);					\
      }
#define ASTSetupSRCPitch(addr, pitch) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG)(PKT_SINGLE_CMD_HEADER + CMDQREG_SRC_PITCH); 	\
        addr->PKT_SC_dwData[0] = (ULONG)(pitch << 16);					\
      }
#define ASTSetupDSTBase(addr, base) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG)(PKT_SINGLE_CMD_HEADER + CMDQREG_DST_BASE); 	\
        addr->PKT_SC_dwData[0] = (ULONG)(base);					\
      }
#define ASTSetupDSTPitchHeight(addr, pitch, height) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG)(PKT_SINGLE_CMD_HEADER + CMDQREG_DST_PITCH); 	\
        addr->PKT_SC_dwData[0] = (ULONG)((pitch << 16) + ((height) & MASK_DST_HEIGHT));					\
      }
#define ASTSetupDSTXY(addr, x, y) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG)(PKT_SINGLE_CMD_HEADER + CMDQREG_DST_XY); 	\
        addr->PKT_SC_dwData[0] = (ULONG)(((x & MASK_DST_X) << 16) + (y & MASK_DST_Y));					\
      }
#define ASTSetupSRCXY(addr, x, y) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG)(PKT_SINGLE_CMD_HEADER + CMDQREG_SRC_XY); 	\
        addr->PKT_SC_dwData[0] = (ULONG)(((x & MASK_SRC_X) << 16) + (y & MASK_SRC_Y));					\
      }
#define ASTSetupRECTXY(addr, x, y) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG)(PKT_SINGLE_CMD_HEADER + CMDQREG_RECT_XY); 	\
        addr->PKT_SC_dwData[0] = (ULONG)(((x & MASK_RECT_WIDTH) << 16) + (y & MASK_RECT_WIDTH));					\
      }
#define ASTSetupFG(addr, color) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG)(PKT_SINGLE_CMD_HEADER + CMDQREG_FG); 	\
        addr->PKT_SC_dwData[0] = (ULONG)(color);					\
      }
#define ASTSetupBG(addr, color) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG)(PKT_SINGLE_CMD_HEADER + CMDQREG_BG); 	\
        addr->PKT_SC_dwData[0] = (ULONG)(color);					\
      }
#define ASTSetupMONO1(addr, pat) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG)(PKT_SINGLE_CMD_HEADER + CMDQREG_MONO1); 	\
        addr->PKT_SC_dwData[0] = (ULONG)(pat);				\
      }
#define ASTSetupMONO2(addr, pat) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG)(PKT_SINGLE_CMD_HEADER + CMDQREG_MONO2); 	\
        addr->PKT_SC_dwData[0] = (ULONG)(pat);				\
      }
#define ASTSetupCLIP1(addr, left, top) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG)(PKT_SINGLE_CMD_HEADER + CMDQREG_CLIP1); 	\
        addr->PKT_SC_dwData[0] = (ULONG)(((left & MASK_CLIP) << 16) + (top & MASK_CLIP));	\
      }
#define ASTSetupCLIP2(addr, right, bottom) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG)(PKT_SINGLE_CMD_HEADER + CMDQREG_CLIP2); 	\
        addr->PKT_SC_dwData[0] = (ULONG)(((right & MASK_CLIP) << 16) + (bottom & MASK_CLIP));	\
      }
#define ASTSetupCMDReg(addr, reg) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG)(PKT_SINGLE_CMD_HEADER + CMDQREG_CMD); 	\
        addr->PKT_SC_dwData[0] = (ULONG)(reg);					\
      }
#define ASTSetupPatReg(addr, patreg, pat) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG)(PKT_SINGLE_CMD_HEADER + (CMDQREG_PAT + (patreg << 24))); 	\
        addr->PKT_SC_dwData[0] = (ULONG)(pat);				\
      }

/* Line CMD */
#define ASTSetupLineXY(addr, x, y) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG)(PKT_SINGLE_CMD_HEADER + CMDQREG_LINE_XY); 	\
        addr->PKT_SC_dwData[0] = (ULONG)(((x & MASK_LINE_X) << 16) + (y & MASK_LINE_Y));					\
      }
#define ASTSetupLineXMErrTerm(addr, xm, err) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG)(PKT_SINGLE_CMD_HEADER + CMDQREG_LINE_Err); 	\
        addr->PKT_SC_dwData[0] = (ULONG)((xm << 24) + (err & MASK_LINE_ERR));					\
      }
#define ASTSetupLineWidth(addr, width) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG)(PKT_SINGLE_CMD_HEADER + CMDQREG_LINE_WIDTH); 	\
        addr->PKT_SC_dwData[0] = (ULONG)((width & MASK_LINE_WIDTH) << 16);				\
      }
#define ASTSetupLineK1Term(addr, err) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG)(PKT_SINGLE_CMD_HEADER + CMDQREG_LINE_K1); 	\
        addr->PKT_SC_dwData[0] = (ULONG)(err & MASK_LINE_K1);				\
      }
#define ASTSetupLineK2Term(addr, err) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG)(PKT_SINGLE_CMD_HEADER + CMDQREG_LINE_K2); 	\
        addr->PKT_SC_dwData[0] = (ULONG)(err & MASK_LINE_K2);				\
      }
#define ASTSetupLineStyle1(addr, pat) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG)(PKT_SINGLE_CMD_HEADER + CMDQREG_LINE_STYLE1); 	\
        addr->PKT_SC_dwData[0] = (ULONG)(pat);				\
      }
#define ASTSetupLineStyle2(addr, pat) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG)(PKT_SINGLE_CMD_HEADER + CMDQREG_LINE_STYLE2); 	\
        addr->PKT_SC_dwData[0] = (ULONG)(pat);				\
      }

#define ASTSetupNULLCMD(addr) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG) (PKT_NULL_CMD); 			\
        addr->PKT_SC_dwData[0] = (ULONG) 0;					\
      }

/* AIP Line CMD */
#define AIPSetupLineXY(addr, x, y) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG)(PKT_SINGLE_CMD_HEADER + CMDQREG_LINE_XY); 	\
        addr->PKT_SC_dwData[0] = (ULONG)(((x & MASK_AIPLINE_X) << 16) + (y & MASK_AIPLINE_Y));					\
      }
#define AIPSetupLineXY2(addr, x, y) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG)(PKT_SINGLE_CMD_HEADER + CMDQREG_LINE_XY2); 	\
        addr->PKT_SC_dwData[0] = (ULONG)(((x & MASK_AIPLINE_X) << 16) + (y & MASK_AIPLINE_Y));					\
      }
#define AIPSetupLineNumber(addr, no) \
      { \
        addr->PKT_SC_dwHeader  = (ULONG)(PKT_SINGLE_CMD_HEADER + CMDQREG_LINE_NUMBER); 	\
        addr->PKT_SC_dwData[0] = (ULONG)(no);					\
      }

Bool bASTGetLineTerm(_LINEInfo *LineInfo, LINEPARAM *dsLineParam);
