/*
 * Common strutures and function for CL-GD546x -- The Laguna family
 *
 * lg.h
 *
 * (c) 1998 Corin Anderson.
 *          corina@the4cs.com
 *          Tukwila, WA
 *
 *  Inspired by cir.h
 */

#ifndef LG_H
#define LG_H
#define LG_DEBUG

extern ScrnInfoPtr LgProbe(int entity);
extern const OptionInfoRec *	LgAvailableOptions(int chipid);

# ifdef _LG_PRIVATE_

/* Saved registers that are not part of the core VGA */
/* CRTC >= 0x19; Sequencer >= 0x05; Graphics >= 0x09; Attribute >= 0x15 */
	/* CR regs */
enum {
	/* CR regs */
	CR1A,
	CR1B,
	CR1D,
	CR1E,
	/* SR regs */
	SR07,
	SR0E,
	SR1E,
	/* Must be last! */
	LG_LAST_REG
};

#undef FORMAT

typedef struct {
	unsigned char ExtVga[LG_LAST_REG];

	/* Laguna regs */
	CARD8 TILE, BCLK;
	CARD16 FORMAT, DTTC, TileCtrl, CONTROL;
	CARD16 RIFCtrl, RACCtrl;
	CARD32 VSC;
} LgRegRec, *LgRegPtr;

typedef struct {
	int tilesPerLine;	/* Number of tiles per line */
	int pitch;			/* Display pitch, in bytes */
	int width;			/* Tile width.  0 = 128 byte  1 = 256 byte */
} LgLineDataRec, *LgLineDataPtr;


/* lg_driver.c */
extern LgLineDataRec LgLineData[];


/* lg_hwcurs.c */
extern Bool LgHWCursorInit(ScreenPtr pScreen);
extern void LgHideCursor(ScrnInfoPtr pScrn);
extern void LgShowCursor(ScrnInfoPtr pScrn);

/* lg_i2c.c */
extern Bool LgI2CInit(ScrnInfoPtr pScrn);

#define memrb(off) MMIO_IN8(pCir->IOBase,off)
#define memrw(off) MMIO_IN16(pCir->IOBase,off)
#define memrl(off) MMIO_IN32(pCir->IOBase,off)
#define memwb(off,val) MMIO_OUT8(pCir->IOBase,off,val)
#define memww(off,val) MMIO_OUT16(pCir->IOBase,off,val)
#define memwl(off,val) MMIO_OUT32(pCir->IOBase,off,val)

/* Card-specific driver information */
#define LGPTR(p) ((LgPtr)((p)->chip.lg))

typedef struct lgRec {
	CARD32		HWCursorAddr;
	int			HWCursorImageX;
	int			HWCursorImageY;
	int			HWCursorTileWidth;
	int			HWCursorTileHeight;

	int			lineDataIndex;

	int			memInterleave;

	LgRegRec	SavedReg;
	LgRegRec	ModeReg;

	CARD32		oldBitmask;
	Bool		blitTransparent;
	int			blitYDir;
} LgRec, *LgPtr;

# endif /* _LG_PRIVATE_ */
#endif /* LG_H */





