#ifndef __LG_BLIT_H
#define __LG_BLIT_H

/*
 * bitBLT acceleration for CL-GD546x -- The Laguna family
 *
 * lg_xaa.h
 *
 * (c) 1996,1998 Corin Anderson.
 *               corina@the4cs.com
 *               Tukwila, WA
 *
 * This header file draws much from the file cir_blitLG.h in version 3.3.3
 * of XFree86.
 */

/* This header file defines the necessary structures, constants, and
   variables for using the bitBLT engine on a Laguna family graphics
   accelerator.  */


enum {						/* Offsets into MMIO space for bitBLT regs */
	STATUS			= 0x0400,
	OP0_opRDRAM		= 0x0520,
	OP1_opRDRAM		= 0x0540,
	OP2_opRDRAM		= 0x0560,
	OP0_opMRDRAM	= 0x0524,
	OP1_opMRDRAM	= 0x0544,
	OP2_opMRDRAM	= 0x0564,
	OP0_opSRAM		= 0x0528,
	OP1_opSRAM		= 0x0548,
	OP2_opSRAM		= 0x0568,
	OP1_opMSRAM		= 0x054A,
	OP2_opMSRAM		= 0x056A,
	DRAWDEF			= 0x0584,
	BLTDEF			= 0x0586,
	BLTEXT_EX		= 0x0700,
	MBLTEXT_EX		= 0x0720,
	MONOQW			= 0x0588,
	QFREE			= 0x0404,
	PATOFF			= 0x052A,
	HOSTDATA		= 0x0800,
	OP_opBGCOLOR	= 0x05E4,
	OP_opFGCOLOR	= 0x05E0,
	bltCONTROL		= 0x0402,
	BITMASK			= 0x05E8
};

enum { HOSTDATASIZE = 2048 };	/* The HOSTDATA port is 2048 BYTES */

enum {						/* OR these together to form a bitBLT mode */
	HOST2SCR		= 0x1120,	/* CPU/Screen transfer modes */
	SCR2HOST		= 0x2010,
	HOST2PAT		= 0x1102,
	HOST2SRAM2		= 0x6020,	/* CPU to SRAM2 transfer */

	SCR2SCR			= 0x1110,	/* Screen/Screen transfers */
	COLORSRC		= 0x0000,	/* Source is color data */
	MONOSRC			= 0x0040,	/* Source is mono data (color expansion) */
	COLORTRANS		= 0x0001,	/* Transparent screen/screen transfer */
	MONOTRANS		= 0x0005,	/* Transparent screen/screen color expansion */
	COLORFILL		= 0x0070,	/* Solid color fill mode */
	SRAM1SCR2SCR	= 0x1180,	/* Pattern fill, source from SRAM1 */

	PAT2SCR			= 0x1109,	/* Pattern/Screen transfers */
	COLORPAT		= 0x0000,	/* Pattern is color data */
	MONOPAT			= 0x0004,	/* Pattern is mono data (color expansion) */
	SRAM2PAT2SCR	= 0x1108,	/* SRAM2 is pattern source */

  PATeqSRC   = 0x0800,      /* The Pattern and Source operands are the same */
								/* Advice from Corey:  don't ever try to use
								   this option!  8)  There's a documented bug
								   with it on the '62, and, well, I have
								   empirical evidence that either the bug's
								   still around, even in the '64 and the '65.
								   It's a performance option, anyway, so not
								   using it should be okay. */
					

	BLITUP			= 0x8000	/* The blit is proceeding from bottom to top */
};

enum {						/* Select transparency compare */
	TRANSBG			= 0x0100,
	TRANSFG			= 0x0300,
	TRANSEQ			= 0x0100,
	TRANSNE			= 0x0300,
	TRANSNONE		= 0x0000
};

#define LgREADY() ((memrb(STATUS) & 0x07) == 0x00)

#define LgSETROP(rop)  memww(DRAWDEF,rop);

#define LgSETTRANSPARENCY(trans) \
  memww(DRAWDEF,(trans) | (memrw(DRAWDEF) & 0x00FF));

#define LgSETMODE(mode)  memww(BLTDEF,mode);

#define LgSETDSTXY(X, Y)  memwl(OP0_opRDRAM, (((Y) << 16) | (X)));

#define LgSETSRCXY(X, Y)  memwl(OP1_opRDRAM, (((Y) << 16) | (X)));

#define LgSETPATXY(X, Y)  memwl(OP2_opRDRAM, (((Y) << 16) | (X)));

#define LgSETTRANSMASK(X, Y) LgSETPATXY(X, Y)

#define LgSETSRAMDST(offset)  memww(OP0_opSRAM, offset);

/* was this a bug? it read OP2_opSRAM */
#define LgSETSRAM1OFFSET(offset)  memww(OP1_opSRAM, offset);

#define LgSETSRAM2OFFSET(offset)  memww(OP2_opSRAM, offset);

#define LgSETMSRAM1OFFSET(offset) memww(OP1_opMSRAM, offset);

#define LgSETMSRAM2OFFSET(offset) memww(OP2_opMSRAM, offset);

#define LgSETMDSTXY(X, Y)  memwl(OP0_opMRDRAM, (((Y) << 16) | (X)));

#define LgSETMSRCXY(X, Y)  memwl(OP1_opMRDRAM, (((Y) << 16) | (X)));

#define LgSETMPATXY(X, Y)  memwl(OP2_opMRDRAM, (((Y) << 16) | (X)));

#define LgSETMTRANSMASK(X, Y) LgSETMPATXY(X, Y)

#define LgSETPHASE0(phase)  memwl( OP0_opRDRAM, phase);

#define LgSETPHASE1(phase)  memwl(OP1_opRDRAM, phase);

#define LgSETPHASE2(phase)  memwl( OP2_opRDRAM, phase);

#define LgSETMPHASE0(phase)  memwl(OP0_opMRDRAM, phase);

#define LgSETMPHASE1(phase)  memwl(OP1_opMRDRAM, phase);

#define LgSETEXTENTS(width, height)  \
           memwl(BLTEXT_EX, (((height) << 16)|(width)));

#if 0
#define LgSETMEXTENTS(width, height)  \
  memwl(MBLTEXT_EX, (((height) << 16)|(width)));
#else
/* For monochrome (byte) blits, we need to set how many QWORDs of data
   encompass the X extent.  Write this piece of data into MONOQW. */
#define LgSETMEXTENTS(width, height)  \
	{ \
    memww(MONOQW, ((width + 7) >> 3));  \
    memwl(MBLTEXT_EX, (((height) << 16)|(width)));  \
	}

#define LgSETMEXTENTSNOMONOQW(width, height)  \
  memwl(MBLTEXT_EX, (((height) << 16)|(width)));

/*
    memww(MBLTEXT_EX, height);
    memww(MBLTEXT_EX + 2, width);
 */
#endif

#define LgHOSTDATAWRITE(data)  memwl(HOSTDATA, data);

#define LgHOSTDATAREAD()   (memrl(HOSTDATA))

#define LgSETBACKGROUND(color)  memwl(OP_opBGCOLOR, color);

#define LgSETFOREGROUND(color)   memwl(OP_opFGCOLOR, color);

#define LgSETPATOFF(xoff, yoff)  memww(PATOFF, (((yoff) << 8) | (xoff)));

#define LgSETSWIZZLE() memww(bltCONTROL, memrw(bltCONTROL | 0x0400));

#define LgCLEARSWIZZLE() memww(bltCONTROL, memrw(bltCONTROL & ~0x0400));

#define LgSETBITMASK(m) memwl(BITMASK, m);



#endif  /* __LG_BLIT_H */
