





/* Definitions for the Chips and Technology BitBLT engine communication. */
/* These are done using Memory Mapped IO, of the registers */
/* BitBLT modes for register 93D0. */

#define ctPATCOPY               0xF0
#define ctLEFT2RIGHT            0x000
#define ctRIGHT2LEFT            0x100
#define ctTOP2BOTTOM            0x000
#define ctBOTTOM2TOP            0x200
#define ctSRCSYSTEM             0x400
#define ctDSTSYSTEM             0x800
#define ctSRCMONO               0x1000
#define ctBGTRANSPARENT         0x22000
#define ctCOLORTRANSENABLE      0x4000
#define ctCOLORTRANSDISABLE     0x0
#define ctCOLORTRANSDST         0x8000
#define ctCOLORTRANSROP         0x0
#define ctCOLORTRANSEQUAL       0x10000L
#define ctCOLORTRANSNEQUAL      0x0
#define ctPATMONO               0x40000L
#define ctPATSOLID              0x80000L
#define ctPATSTART0             0x000000L
#define ctPATSTART1             0x100000L
#define ctPATSTART2             0x200000L
#define ctPATSTART3             0x300000L
#define ctPATSTART4             0x400000L
#define ctPATSTART5             0x500000L
#define ctPATSTART6             0x600000L
#define ctPATSTART7             0x700000L
#define ctSRCFG                 0x000000L	/* Where is this for the 65550?? */

/* The Monochrome expansion register setup */
#define ctCLIPLEFT(clip)        ((clip)&0x3F)
#define ctCLIPRIGHT(clip)       (((clip)&0x3F) << 8)
#define ctSRCDISCARD(clip)      (((clip)&0x3F) << 16)
#define ctBITALIGN              0x1000000L
#define ctBYTEALIGN             0x2000000L
#define ctWORDALIGN             0x3000000L
#define ctDWORDALIGN            0x4000000L
#define ctQWORDALIGN            0x5000000L
/* This shouldn't be used because not all chip rev's
 * have BR09 and BR0A, and I haven't even defined
 * macros to write to these registers 
 */
#define ctEXPCOLSEL             0x8000000L

/* Macros to do useful things with the C&T BitBLT engine */

/* For some odd reason the blitter busy bit occasionly "locks up" when 
 * it gets polled to fast. However I have observed this behavior only 
 * when doing ScreenToScreenColorExpandFill on a 65550. This operation
 * was broken anyway (the source offset register is not observed) therefore
 * no action was taken.
 *
 * This function uses indirect access to XR20 to test whether the blitter
 * is busy. If the cost of doing this is too high then other options will
 * need to be considered.
 *
 * Note that BR04[31] can't be used as some C&T chipsets lockup when reading
 * the BRxx registers.
 */
#define ctBLTWAIT \
                     {int timeout; \
                     timeout = 0; \
		     for (;;) { \
                         if (cPtr->Chipset >= CHIPS_CT69000 ) { \
                            if (!(MMIO_IN32(cPtr->MMIOBase,BR(0x4))&(1<<31)))\
                                    break; \
                         } else { \
                            if (!(cPtr->readXR(cPtr,0x20) & 0x1)) break; \
                         } \
                         timeout++; \
                         if ((cPtr->Chipset < CHIPS_CT69000 && \
			     (timeout > 100000)) || timeout > 300000) { \
			    unsigned char tmp; \
                            ErrorF("timeout\n"); \
			    tmp = cPtr->readXR(cPtr, 0x20); \
			    cPtr->writeXR(cPtr, 0x20, ((tmp & 0xFD) | 0x2)); \
                            usleep(10000); \
                            cPtr->writeXR(cPtr, 0x20, (tmp & 0xFD)); \
			    break; \
                         } \
		      } \
		    }

#if X_BYTE_ORDER == X_BIG_ENDIAN
# define TWEAK_24_BE(c) \
    c = ((c & 0xFF0000) >> 16) | (c & 0xFF00) | (( c & 0xFF) << 16)
#else
# define TWEAK_24_BE(c)
#endif

#define ctSETROP(op) \
  MMIO_OUT32(cPtr->MMIOBase, BR(0x4), op)

#define ctSETMONOCTL(op) \
  MMIO_OUT32(cPtr->MMIOBase, BR(0x3), op)

#define ctSETSRCADDR(srcAddr) \
  MMIO_OUT32(cPtr->MMIOBase, BR(0x6), (srcAddr)&0x7FFFFFL)

#define ctSETDSTADDR(dstAddr) \
  MMIO_OUT32(cPtr->MMIOBase, BR(0x7), (dstAddr)&0x7FFFFFL)

#define ctSETPITCH(srcPitch,dstPitch) \
  MMIO_OUT32(cPtr->MMIOBase, BR(0x0), (((dstPitch)&0xFFFF)<<16)| \
      ((srcPitch)&0xFFFF))

#define ctSETHEIGHTWIDTHGO(Height,Width)\
  MMIO_OUT32(cPtr->MMIOBase, BR(0x8), (((Height)&0xFFFF)<<16)| \
      ((Width)&0xFFFF))

#define ctSETPATSRCADDR(srcAddr)\
  MMIO_OUT32(cPtr->MMIOBase, BR(0x5), (srcAddr)&0x7FFFFFL)

#define ctSETBGCOLOR8(c) {\
    if ((cAcl->bgColor != (c)) || (cAcl->bgColor == -1)) { \
	cAcl->bgColor = (c); \
        MMIO_OUT32(cPtr->MMIOBase, BR(0x1), ((c)&0xFF)); \
    } \
}

#define ctSETBGCOLOR16(c) {\
    if ((cAcl->bgColor != (c)) || (cAcl->bgColor == -1)) { \
	cAcl->bgColor = (c); \
        MMIO_OUT32(cPtr->MMIOBase, BR(0x1), ((c)&0xFFFF)); \
    } \
}

#define ctSETBGCOLOR24(c) {\
    TWEAK_24_BE(c); \
    if ((cAcl->bgColor != (c)) || (cAcl->bgColor == -1)) { \
	cAcl->bgColor = (c); \
        MMIO_OUT32(cPtr->MMIOBase, BR(0x1), ((c)&0xFFFFFF)); \
    } \
}

#define ctSETFGCOLOR8(c) {\
    if ((cAcl->fgColor != (c)) || (cAcl->fgColor == -1)) { \
	cAcl->fgColor = (c); \
        MMIO_OUT32(cPtr->MMIOBase, BR(0x2), ((c)&0xFF)); \
    } \
}

#define ctSETFGCOLOR16(c) {\
    if ((cAcl->fgColor != (c)) || (cAcl->fgColor == -1)) { \
	cAcl->fgColor = (c); \
        MMIO_OUT32(cPtr->MMIOBase, BR(0x2), ((c)&0xFFFF)); \
    } \
}

#define ctSETFGCOLOR24(c) {\
    TWEAK_24_BE(c); \
    if ((cAcl->fgColor != (c)) || (cAcl->fgColor == -1)) { \
	cAcl->fgColor = (c); \
        MMIO_OUT32(cPtr->MMIOBase, BR(0x2), ((c)&0xFFFFFF)); \
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
