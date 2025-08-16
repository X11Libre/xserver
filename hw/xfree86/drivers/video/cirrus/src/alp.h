/* (c) Itai Nahshon */

#ifndef ALP_H
#define ALP_H

extern ScrnInfoPtr AlpProbe(int entity);
extern const OptionInfoRec *	AlpAvailableOptions(int chipid);

# ifdef _ALP_PRIVATE_
/* Saved registers that are not part of the core VGA */
/* CRTC >= 0x19; Sequencer >= 0x05; Graphics >= 0x09; Attribute >= 0x15 */

enum {
	/* CR regs */
	CR1A,
	CR1B,
	CR1D,
	/* SR regs */
	SR07,
	SR0E,
	SR12,
	SR13,
	SR17,
	SR1E,
	SR21,
	SR2D,
	/* GR regs */
	GR17,
	GR18,
	/* HDR */
	HDR,
	/* Must be last! */
	CIR_NSAVED
};

typedef enum {LCD_NONE, LCD_DUAL_MONO, LCD_UNKNOWN, LCD_DSTN, LCD_TFT} LCDType;

typedef struct {
	unsigned char	ExtVga[CIR_NSAVED];
} AlpRegRec, *AlpRegPtr;

extern Bool AlpHWCursorInit(ScreenPtr pScreen, int size);
extern Bool AlpDGAInit(ScreenPtr pScreen);
extern Bool AlpI2CInit(ScrnInfoPtr pScrn);

/* Card-specific driver information */
#define ALPPTR(p) ((AlpPtr)((p)->chip.alp))

typedef struct alpRec {
	unsigned char * HWCursorBits;
	unsigned char *	CursorBits;

	AlpRegRec		SavedReg;
	AlpRegRec		ModeReg;
        LCDType                 lcdType;
        int                     lcdWidth, lcdHeight;
        int                 CursorWidth;
        int                 CursorHeight;
        int                 waitMsk;
        int                 scanlineDest;
        int                 scanlineCount;
        int                 scanlineWidth;

        int                 SubsequentColorExpandScanlineDest;
        int                 SubsequentColorExpandScanlineByteWidth;
        int                 SubsequentColorExpandScanlineDWordWidth;

        /* Offset into framebuffer of a 8-byte scratch area for fills */
        CARD32 monoPattern8x8;

        Bool                autoStart;

	/* MMIO Base for BitBLT operation. This is
	   IOBase for 5446 and 7548, IOBase+0x100 for 5480 */
	unsigned char *	    BLTBase;
        CARD32              transRop;
/* XXX For XF86Config based mem configuration */
	CARD32			sr0f, sr17;
} AlpRec, *AlpPtr;

# endif /* _ALP_PRIVATE_ */
#endif /* ALP_H */

