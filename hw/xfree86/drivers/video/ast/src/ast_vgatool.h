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

/* VRAM Size Definition */
#define VIDEOMEM_SIZE_08M	0x00800000
#define VIDEOMEM_SIZE_16M	0x01000000
#define VIDEOMEM_SIZE_32M	0x02000000
#define VIDEOMEM_SIZE_64M	0x04000000
#define VIDEOMEM_SIZE_128M	0x08000000

#define DRAM_SIZE_016M		0x01000000
#define DRAM_SIZE_032M		0x02000000
#define DRAM_SIZE_064M		0x04000000
#define DRAM_SIZE_128M		0x08000000
#define DRAM_SIZE_256M		0x10000000

#define DRAMTYPE_512Mx16	0
#define DRAMTYPE_1Gx16		1
#define DRAMTYPE_512Mx32	2
#define DRAMTYPE_1Gx32		3
#define DRAMTYPE_2Gx16		6
#define DRAMTYPE_4Gx16		7

#define AR_PORT_WRITE		(pAST->MMIOVirtualAddr + 0x3c0)
#define MISC_PORT_WRITE		(pAST->MMIOVirtualAddr + 0x3c2)
#define VGA_ENABLE_PORT		(pAST->MMIOVirtualAddr + 0x3c3)
#define SEQ_PORT		(pAST->MMIOVirtualAddr + 0x3c4)
#define DAC_INDEX_READ		(pAST->MMIOVirtualAddr + 0x3c7)
#define DAC_INDEX_WRITE		(pAST->MMIOVirtualAddr + 0x3c8)
#define DAC_DATA		(pAST->MMIOVirtualAddr + 0x3c9)
#define GR_PORT			(pAST->MMIOVirtualAddr + 0x3cE)
#define CRTC_PORT		(pAST->MMIOVirtualAddr + 0x3d4)
#define INPUT_STATUS1_READ	(pAST->MMIOVirtualAddr + 0x3dA)
#define MISC_PORT_READ		(pAST->MMIOVirtualAddr + 0x3cc)

#define GetReg(base)				MMIO_IN8(base, 0)
#define SetReg(base,val)			MMIO_OUT8(base, 0, val)
#define GetIndexReg(base,index,val)		{ \
						MMIO_OUT8(base, 0, index);	\
						val = MMIO_IN8(base, 1);	\
						}
#define SetIndexReg(base,index, val)		{ \
						MMIO_OUT8(base, 0, index);	\
						MMIO_OUT8(base, 1, val);	\
						}
#define GetIndexRegMask(base,index, and, val)	{ \
						MMIO_OUT8(base, 0, index);	\
						val = MMIO_IN8(base, 1) & and;	\
						}
#define SetIndexRegMask(base,index, and, val)	{ \
						UCHAR __Temp;	\
						MMIO_OUT8(base, 0, index);	\
						__Temp = (MMIO_IN8(base, 1)&(and))|(val);	\
						SetIndexReg(base,index,__Temp);			\
						}

#define VGA_GET_PALETTE_INDEX(index, red, green, blue) \
{ \
   UCHAR __junk;				\
   SetReg(DAC_INDEX_READ,(UCHAR)(index));	\
   __junk = GetReg(SEQ_PORT);			\
   red = GetReg(DAC_DATA);		\
   __junk = GetReg(SEQ_PORT);			\
   green = GetReg(DAC_DATA);		\
   __junk = GetReg(SEQ_PORT);			\
   blue = GetReg(DAC_DATA);		\
   __junk = GetReg(SEQ_PORT);			\
   (void)__junk;				\
}

#define VGA_LOAD_PALETTE_INDEX(index, red, green, blue) \
{ \
   UCHAR __junk;				\
   SetReg(DAC_INDEX_WRITE,(UCHAR)(index));	\
   __junk = GetReg(SEQ_PORT);			\
   SetReg(DAC_DATA,(UCHAR)(red));		\
   __junk = GetReg(SEQ_PORT);			\
   SetReg(DAC_DATA,(UCHAR)(green));		\
   __junk = GetReg(SEQ_PORT);			\
   SetReg(DAC_DATA,(UCHAR)(blue));		\
   __junk = GetReg(SEQ_PORT);			\
   (void)__junk;				\
}

/* Reg. Definition */
#define AST1180_MEM_BASE		0x40000000
#define AST1180_MMC_BASE		0x80FC8000
#define AST1180_SCU_BASE		0x80FC8200
#define AST1180_GFX_BASE		0x80FC9000
#define AST1180_VIDEO_BASE		0x80FCD000

/* AST1180 GFX */
#define AST1180_VGA1_CTRL 	        0x60
#define AST1180_VGA1_CTRL2	        0x64
#define AST1180_VGA1_STATUS 	        0x68
#define AST1180_VGA1_PLL 	        0x6C
#define AST1180_VGA1_HTREG              0x70
#define AST1180_VGA1_HRREG              0x74
#define AST1180_VGA1_VTREG              0x78
#define AST1180_VGA1_VRREG              0x7C
#define AST1180_VGA1_STARTADDR          0x80
#define AST1180_VGA1_OFFSET 	        0x84
#define AST1180_VGA1_THRESHOLD		0x88

#define AST1180_HWC1_OFFSET		0x90
#define AST1180_HWC1_POSITION		0x94
#define AST1180_HWC1_PATTERNADDR	0x98

#define CRT_LOW_THRESHOLD_VALUE         0x40
#define CRT_HIGH_THRESHOLD_VALUE        0x7E

/* GFX Ctrl Reg */
#define AST1180_ENABLECRT		0x00000001
#define AST1180_ENABLEHWC		0x00000002
#define AST1180_MONOHWC			0x00000000
#define AST1180_ALPHAHWC		0x00000400
#define AST1180_HSYNCOFF		0x00040000
#define AST1180_VSYNCOFF		0x00080000
#define AST1180_VGAOFF			0x00100000

#define ReadAST1180SOC(addr, data)	\
{	\
        *(ULONG *) (pAST->MMIOVirtualAddr + 0xF004) = (addr) & 0xFFFF0000;	\
        *(ULONG *) (pAST->MMIOVirtualAddr + 0xF000) = 0x1;	\
        data = (*(ULONG *) (pAST->MMIOVirtualAddr + 0x10000 + ((addr) & 0x0000FFFF)));	\
}

#define WriteAST1180SOC(addr, data)	\
{	\
        ULONG temp _X_UNUSED;	\
        *(ULONG *) (pAST->MMIOVirtualAddr + 0xF004) = (addr) & 0xFFFF0000;	\
        *(ULONG *) (pAST->MMIOVirtualAddr + 0xF000) = 0x1;	\
        *(ULONG *) (pAST->MMIOVirtualAddr + 0x10000 + ((addr) & 0x0000FFFF)) = (data);	\
        temp = *(ULONG *) (pAST->MMIOVirtualAddr + 0x10000 + ((addr) & 0x0000FFFF));	\
}

#define ReadAST1180MEM(addr, data)	\
{	\
        *(ULONG *) (pAST->MMIOVirtualAddr + 0xF004) = (addr) & 0xFFFF0000;	\
        *(ULONG *) (pAST->MMIOVirtualAddr + 0xF000) = 0x1;	\
        data = (*(ULONG *) (pAST->MMIOVirtualAddr + 0x10000 + ((addr) & 0x0000FFFF)));	\
}

#define WriteAST1180MEM(addr, data)	\
{	\
        *(ULONG *) (pAST->MMIOVirtualAddr + 0xF004) = (addr) & 0xFFFF0000;	\
        *(ULONG *) (pAST->MMIOVirtualAddr + 0xF000) = 0x1;	\
        *(ULONG *) (pAST->MMIOVirtualAddr + 0x10000 + ((addr) & 0x0000FFFF)) = (data);	\
}

/* Delay */
#define DelayUS(x)		usleep(x)
#define DelayMS(x)		DelayUS(1000*x)
