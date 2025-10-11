





/* Definitions for the Chips and Technology BitBLT engine communication. */
/* registers */
/* Do not read 87D0 while BitBLT is active */
/* 83D0:  11-0  source offset, width of 'screen' */
/*        15-12 reserved (0)  */
/*        27-16 destination offset, width of screen */
/*        31-28 reserved (0)  */
/* 87D0:  20-0  pattern (aligned 8 pixel x 8 line)  pointer */
/*        31-21 reserved (0)  */
/* 8BD0:  15-0  background colour */
/*        31-6  duplicate of 15-0  */
/* 8FD0:  15-0  foregroud/solid colour */
/*        31-6  duplicate of 15-0  */
/* 93D0:  7-0   ROP, same as MS-Windows */
/*        8     BitBLT Y direction, if 0 bottom to top, 1 top to bottom */
/*        9     BitBLT X direction, if 0 right to left, 1 left to right */
/*        10    source data, if 0 source is selected bit 14, 1 foregourd colour */
/*        11    source depth, if 0 source is colour, */
/*              1 source is monochrome(Font expansion) */
/*        12    pattern depth, if 0 colour, else monochrome */
/*        13    background, if 0 opaque (8BD0), else transparent */
/*        14    BitBLT source, if 0 screen, else system memory */
/*        15    reserved (0, destination?) */
/*        18-16 starting row of 8x8 pattern */
/*        19    if 1 solid pattern (Brush), else bitmap */
/*        20(R) BitBLT status, if 1 active */
/*        23-21 reserved (0)              */
/*        27-24 vacancy in buffer         */
/*        31-25 reserved (0)              */
/* 97D0:  20-0  source address (byte aligned) */
/*        31-21 reserved (0)              */
/* 9BD0:  20-0  destination address (byte aligned) */
/*        31-21 reserved (0)              */
/* 9FD0:  11-0  number of bytes to be transferred per line */
/*        15-12 reserved (0)              */
/*        27-16 height in lines of the block to be transferred */
/*        31-28 reserved (0)              */

/* BitBLT modes for register 93D0. */

#define ctPATCOPY               0xF0
#define ctTOP2BOTTOM            0x100
#define ctBOTTOM2TOP            0x000
#define ctLEFT2RIGHT            0x200
#define ctRIGHT2LEFT            0x000
#define ctSRCFG                 0x400
#define ctSRCMONO               0x800
#define ctPATMONO               0x1000
#define ctBGTRANSPARENT         0x2000
#define ctSRCSYSTEM             0x4000
#define ctPATSOLID              0x80000L
#define ctPATSTART0             0x00000L
#define ctPATSTART1             0x10000L
#define ctPATSTART2             0x20000L
#define ctPATSTART3             0x30000L
#define ctPATSTART4             0x40000L
#define ctPATSTART5             0x50000L
#define ctPATSTART6             0x60000L
#define ctPATSTART7             0x70000L

/* Macros to do useful things with the C&T BitBLT engine */

#define ctBLTWAIT \
  {HW_DEBUG(0x4+2); while(inw(cPtr->PIOBase+DR(0x4)+2)&0x10){};}

#define ctSETROP(op) \
  {HW_DEBUG(0x4); outl(cPtr->PIOBase+DR(0x4),(op));}

#define ctSETSRCADDR(srcAddr) \
  {HW_DEBUG(0x5); outl(cPtr->PIOBase+DR(0x5),((srcAddr)&0x1FFFFFL));}

#define ctSETDSTADDR(dstAddr) \
  {HW_DEBUG(0x6); outl(cPtr->PIOBase+DR(0x6),((dstAddr)&0x1FFFFFL));}

#define ctSETPITCH(srcPitch,dstPitch) \
  {HW_DEBUG(0x0); outl(cPtr->PIOBase+DR(0x0),(((dstPitch)<<16)|(srcPitch)));}

/* Note that this command signal a blit to commence */
#define ctSETHEIGHTWIDTHGO(Height,Width)\
  {HW_DEBUG(0x7); outl(cPtr->PIOBase+DR(0x7),(((Height)<<16)|(Width)));}

#define ctSETPATSRCADDR(srcAddr)\
  {HW_DEBUG(0x1); outl(cPtr->PIOBase+DR(0x1),((srcAddr)&0x1FFFFFL));}

/* I can't help pointing out at this point that I'm not complaining
 * about the american spelling of Colour!! [DGB] */

#define ctSETBGCOLOR8(c) {\
    HW_DEBUG(0x2); \
    if ((cAcl->bgColor != (c)) || (cAcl->bgColor == -1)) { \
	cAcl->bgColor = (c); \
	outl(cPtr->PIOBase+DR(0x2),((((((c)&0xFF)<<8)|((c)&0xFF))<<16) | \
	       ((((c)&0xFF)<<8)|((c)&0xFF)))); \
    } \
}

#define ctSETBGCOLOR16(c) {\
    HW_DEBUG(0x2); \
    if ((cAcl->bgColor != (c)) || (cAcl->bgColor == -1)) { \
	cAcl->bgColor = (c); \
	outl(cPtr->PIOBase+DR(0x2),((((c)&0xFFFF)<<16)|((c)&0xFFFF))); \
    } \
}

/* As the 6554x doesn't support 24bpp colour expansion this doesn't work */
#define ctSETBGCOLOR24(c) {\
    HW_DEBUG(0x2); \
    if ((cAcl->bgColor != (c)) || (cAcl->bgColor == -1)) { \
	cAcl->bgColor = (c); \
	outl(cPtr->PIOBase+DR(0x2),(c)&0xFFFFFF); \
    } \
}

#define ctSETFGCOLOR8(c) {\
    HW_DEBUG(0x3); \
    if ((cAcl->fgColor != (c)) || (cAcl->fgColor == -1)) { \
	cAcl->fgColor = (c); \
	outl(cPtr->PIOBase+DR(0x3),((((((c)&0xFF)<<8)|((c)&0xFF))<<16) | \
	       ((((c)&0xFF)<<8)|((c)&0xFF)))); \
    } \
}

#define ctSETFGCOLOR16(c) {\
    HW_DEBUG(0x3); \
    if ((cAcl->fgColor != (c)) || (cAcl->fgColor == -1)) { \
	cAcl->fgColor = (c); \
	outl(cPtr->PIOBase+DR(0x3),((((c)&0xFFFF)<<16)|((c)&0xFFFF))); \
    } \
}

/* As the 6554x doesn't support 24bpp colour expansion this doesn't work */
#define ctSETFGCOLOR24(c) {\
    HW_DEBUG(0x3); \
    if ((cAcl->fgColor != (c)) || (cAcl->fgColor == -1)) { \
	cAcl->fgColor = (c); \
	outl(cPtr->PIOBase+DR(0x3),(c)&0xFFFFFF); \
    } \
}

/* Define a Macro to replicate a planemask 64 times and write to address
 * allocated for planemask pattern */
#define ctWRITEPLANEMASK8(mask,addr) { \
    if (cAcl->planemask != (mask&0xFF)) { \
	cAcl->planemask = (mask&0xFF); \
	memset((unsigned char *)cPtr->FbBase + addr, (mask&0xFF), 64); \
    } \
}

#define ctWRITEPLANEMASK16(mask,addr) { \
    if (cAcl->planemask != (mask&0xFFFF)) { \
	cAcl->planemask = (mask&0xFFFF); \
	{   int i; \
	    for (i = 0; i < 64; i++) { \
		memcpy((unsigned char *)cPtr->FbBase + addr \
			+ i * 2, &mask, 2); \
	    } \
	} \
    } \
}
  
