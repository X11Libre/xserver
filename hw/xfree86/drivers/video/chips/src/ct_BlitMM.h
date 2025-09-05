





/* Definitions for the Chips and Technology BitBLT engine communication. */
/* These are done using Memory Mapped IO, of the registers */
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
  {HW_DEBUG(0x4); \
   while(MMIO_IN32(cPtr->MMIOBase, MR(0x4)) & 0x00100000){};}

#define ctSETROP(op) \
  {HW_DEBUG(0x4);   MMIO_OUT32(cPtr->MMIOBase, MR(0x4), op);}

#define ctSETSRCADDR(srcAddr) \
  {HW_DEBUG(0x5); \
  MMIO_OUT32(cPtr->MMIOBase, MR(0x5),(srcAddr)&0x7FFFFFL);}

#define ctSETDSTADDR(dstAddr) \
{HW_DEBUG(0x6); \
  MMIO_OUT32(cPtr->MMIOBase, MR(0x6), (dstAddr)&0x7FFFFFL);}

#define ctSETPITCH(srcPitch,dstPitch) \
{HW_DEBUG(0x0); \
  MMIO_OUT32(cPtr->MMIOBase, MR(0x0),(((dstPitch)&0xFFFF)<<16)| \
      ((srcPitch)&0xFFFF));}

#define ctSETHEIGHTWIDTHGO(Height,Width)\
{HW_DEBUG(0x7); \
  MMIO_OUT32(cPtr->MMIOBase, MR(0x7), (((Height)&0xFFFF)<<16)| \
      ((Width)&0xFFFF));}

#define ctSETPATSRCADDR(srcAddr)\
{HW_DEBUG(0x1); \
  MMIO_OUT32(cPtr->MMIOBase, MR(0x1),(srcAddr)&0x1FFFFFL);}

#define ctSETBGCOLOR8(c) {\
    HW_DEBUG(0x2); \
    if ((cAcl->bgColor != (c)) || (cAcl->bgColor == -1)) { \
	cAcl->bgColor = (c); \
        MMIO_OUT32(cPtr->MMIOBase, MR(0x2),\
           ((((((c)&0xFF)<<8)|((c)&0xFF))<<16) | \
	   ((((c)&0xFF)<<8)|((c)&0xFF)))); \
    } \
}

#define ctSETBGCOLOR16(c) {\
    HW_DEBUG(0x2); \
    if ((cAcl->bgColor != (c)) || (cAcl->bgColor == -1)) { \
	cAcl->bgColor = (c); \
        MMIO_OUT32(cPtr->MMIOBase, MR(0x2), \
             ((((c)&0xFFFF)<<16)|((c)&0xFFFF))); \
    } \
}

/* As the 6554x doesn't support 24bpp colour expansion this doesn't work,
 * It is here only for later use with the 65550 */
#define ctSETBGCOLOR24(c) {\
    HW_DEBUG(0x2); \
    if ((cAcl->bgColor != (c)) || (cAcl->bgColor == -1)) { \
	cAcl->bgColor = (c); \
        MMIO_OUT32(cPtr->MMIOBase, MR(0x2),((c)&0xFFFFFF)); \
    } \
}

#define ctSETFGCOLOR8(c) {\
    HW_DEBUG(0x3); \
    if ((cAcl->fgColor != (c)) || (cAcl->fgColor == -1)) { \
	cAcl->fgColor = (c); \
        MMIO_OUT32(cPtr->MMIOBase, MR(0x3), \
           ((((((c)&0xFF)<<8)|((c)&0xFF))<<16) | \
	   ((((c)&0xFF)<<8)|((c)&0xFF)))); \
    } \
}

#define ctSETFGCOLOR16(c) {\
    HW_DEBUG(0x3); \
    if ((cAcl->fgColor != (c)) || (cAcl->fgColor == -1)) { \
	cAcl->fgColor = (c); \
        MMIO_OUT32(cPtr->MMIOBase, MR(0x3), \
           ((((c)&0xFFFF)<<16)|((c)&0xFFFF))); \
    } \
}

/* As the 6554x doesn't support 24bpp colour expansion this doesn't work,
 * It is here only for later use with the 65550 */
#define ctSETFGCOLOR24(c) {\
    HW_DEBUG(0x3); \
    if ((cAcl->fgColor != (c)) || (cAcl->fgColor == -1)) { \
	cAcl->fgColor = (c); \
        MMIO_OUT32(cPtr->MMIOBase, MR(0x3),((c)&0xFFFFFF)); \
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
