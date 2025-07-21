#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* (c) Itai Nahshon */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "compiler.h"

#include "xf86Pci.h"

#include "vgaHW.h"

#include "cir.h"
#define _ALP_PRIVATE_
#include "alp.h"

#define CURSORWIDTH	pAlp->CursorWidth
#define CURSORHEIGHT	pAlp->CursorHeight
#define CURSORSIZE      (CURSORWIDTH*CURSORHEIGHT/8)
#define MAXCURSORSIZE   (64*64>>3)

static void
AlpSetCursorColors(ScrnInfoPtr pScrn, int bg, int fg)
{
    const AlpPtr pAlp = ALPPTR(CIRPTR(pScrn));
	vgaHWPtr hwp = VGAHWPTR(pScrn);
#ifdef ALP_DEBUG
	ErrorF("AlpSetCursorColors\n");
#endif
	hwp->writeSeq(hwp, 0x12, pAlp->ModeReg.ExtVga[SR12]|0x02);
	hwp->writeDacWriteAddr(hwp, 0x00);
	hwp->writeDacData(hwp, 0x3f & (bg >> 18));
	hwp->writeDacData(hwp, 0x3f & (bg >> 10));
	hwp->writeDacData(hwp, 0x3f & (bg >>  2));
	hwp->writeDacWriteAddr(hwp, 0x0F);
	hwp->writeDacData(hwp, 0x3F & (fg >> 18));
	hwp->writeDacData(hwp, 0x3F & (fg >> 10));
	hwp->writeDacData(hwp, 0x3F & (fg >>  2));
	hwp->writeSeq(hwp, 0x12, pAlp->ModeReg.ExtVga[SR12]);
}

static void
AlpLoadSkewedCursor(CirPtr pCir, int x, int y) {
     
    const AlpPtr pAlp = ALPPTR(pCir);

    unsigned char *memx = pAlp->HWCursorBits;
        unsigned char *CursorBits = pAlp->CursorBits;
 
        unsigned char mem[2*MAXCURSORSIZE];
	unsigned char *p1, *p2;
	int i, a, b;
	Bool cur64 = (CURSORWIDTH == 64);
	int shift = (cur64? 1 : 0);

	if (x > 0) x = 0; else x = -x;
	if (y > 0) y = 0; else y = -y;


	a = ((y*CURSORWIDTH<<shift)+x)>>3;
	b = x & 7;

	/* Copy the skewed mask bits */
	p1 = mem;
	p2 = CursorBits + a;
	for (i = 0; i < (CURSORSIZE << shift)-a-1; i++) {
		*p1++ = (p2[0] << b) | (p2[1] >> (8-b));
		p2++;
	}
	/* last mask byte */
	*p1++ = (p2[0] << b);

	/* Clear to end (bottom) of mask. */
	for (i = i+1; i < (CURSORSIZE << shift); i++)
		*p1++ = 0;

	if (!cur64) {
	    /* Now copy the cursor bits */
	    /* p1 is already right */
	    p2 = CursorBits+CURSORSIZE+a;
	    for (i = 0; i < CURSORSIZE-a-1; i++) {
		*p1++ = (p2[0] << b) | (p2[1] >> (8-b));
		p2++;
	    }
	    /* last cursor  byte */
	    *p1++ = (p2[0] << b);
	}
	
	/* Clear to end (bottom) of cursor. */
	for (i = i+1; i < CURSORSIZE; i++)
		*p1++ = 0;

	/* Clear the right unused area of the mask
	and cyrsor bits.  */
	p2 = mem + CURSORWIDTH/8 - (x>>3) - 1;
	for (i = 0; i < 2*CURSORHEIGHT; i++) {
		int m = (-1)<<(x&7);
		p1 = p2;
		p2 += CURSORWIDTH/8;
		for (int j = x>>3; j >= 0; j--) {
			*p1 &= m;
			m = 0;
			p1++;
		}
	}
	memcpy(memx, mem, 2*CURSORSIZE);
}


static void
AlpSetCursorPosition(ScrnInfoPtr pScrn, int x, int y)
{
	const CirPtr pCir = CIRPTR(pScrn);
	const AlpPtr pAlp = ALPPTR(pCir);
	vgaHWPtr hwp = VGAHWPTR(pScrn);

#if 0
#ifdef ALP_DEBUG
	ErrorF("AlpSetCursorPosition %d %d\n", x, y);
#endif
#endif

	if (x < 0 || y < 0) {
		if (x+CURSORWIDTH <= 0 || y+CURSORHEIGHT <= 0) {
			hwp->writeSeq(hwp, 0x12, pAlp->ModeReg.ExtVga[SR12] & ~0x01);
			return;
		}
		AlpLoadSkewedCursor(pCir, x, y);
		pCir->CursorIsSkewed = TRUE;
		if (x < 0) x = 0;
		if (y < 0) y = 0;
	} else if (pCir->CursorIsSkewed) {
		memcpy(pAlp->HWCursorBits, pAlp->CursorBits, 2*CURSORSIZE);
		pCir->CursorIsSkewed = FALSE;
	}
	hwp->writeSeq(hwp, 0x12, pAlp->ModeReg.ExtVga[SR12]);
	hwp->writeSeq(hwp, ((x << 5)|0x10)&0xff, x >> 3);
	hwp->writeSeq(hwp, ((y << 5)|0x11)&0xff, y >> 3);
}

static void
AlpLoadCursorImage(ScrnInfoPtr pScrn, unsigned char *bits)
{
	const AlpPtr pAlp = ALPPTR(CIRPTR(pScrn));
	vgaHWPtr hwp = VGAHWPTR(pScrn);

#ifdef ALP_DEBUG
	ErrorF("AlpLoadCursorImage\n");
#endif

	pAlp->CursorBits = bits;
	memcpy(pAlp->HWCursorBits, bits, 2*CURSORSIZE);
	/* this should work for both 64 and 32 bit cursors */
	pAlp->ModeReg.ExtVga[SR13] = 0x3f;
	hwp->writeSeq(hwp, 0x13, pAlp->ModeReg.ExtVga[SR13]);
}

static void
AlpHideCursor(ScrnInfoPtr pScrn)
{
	AlpPtr pAlp = ALPPTR(CIRPTR(pScrn));
	vgaHWPtr hwp = VGAHWPTR(pScrn);

#ifdef ALP_DEBUG
	ErrorF("AlpHideCursor\n");
#endif
	pAlp->ModeReg.ExtVga[SR12] &= ~0x01;
	hwp->writeSeq(hwp, 0x12, pAlp->ModeReg.ExtVga[SR12]);
}

static void
AlpShowCursor(ScrnInfoPtr pScrn)
{
	AlpPtr pAlp = ALPPTR(CIRPTR(pScrn));
	vgaHWPtr hwp = VGAHWPTR(pScrn);

#ifdef ALP_DEBUG
	ErrorF("AlpShowCursor\n");
#endif
	pAlp->ModeReg.ExtVga[SR12] |= 0x01;
	hwp->writeSeq(hwp, 0x12, pAlp->ModeReg.ExtVga[SR12]);
}

static Bool
AlpUseHWCursor(ScreenPtr pScreen, CursorPtr pCurs)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
#ifdef ALP_DEBUG
	ErrorF("AlpUseHWCursor\n");
#endif
	if (pScrn->bitsPerPixel < 8)
		return FALSE;

	return TRUE;
}

Bool
AlpHWCursorInit(ScreenPtr pScreen, int size)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	const CirPtr pCir = CIRPTR(pScrn);
	const AlpPtr pAlp = ALPPTR(pCir);
	
	xf86CursorInfoPtr infoPtr;

#ifdef ALP_DEBUG
	ErrorF("AlpHWCursorInit\n");
#endif
	if (!size) return FALSE;

	infoPtr = xf86CreateCursorInfoRec();
	if (!infoPtr) return FALSE;

	pCir->CursorInfoRec = infoPtr;
	pCir->CursorIsSkewed = FALSE;
	pAlp->CursorBits = NULL;

	if (size == 64)
	    CURSORWIDTH = CURSORHEIGHT = 64;
	else
	    CURSORWIDTH = CURSORHEIGHT = 32;
	
	pAlp->HWCursorBits = pCir->FbBase + 1024*pScrn->videoRam - 2*CURSORSIZE;

	infoPtr->MaxWidth = CURSORWIDTH;
	infoPtr->MaxHeight = CURSORHEIGHT;
	if (CURSORWIDTH == 64)
	    infoPtr->Flags = 
#if X_BYTE_ORDER == X_LITTLE_ENDIAN
		    HARDWARE_CURSOR_BIT_ORDER_MSBFIRST |
#endif
		    HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_64 |
		    HARDWARE_CURSOR_TRUECOLOR_AT_8BPP;
	else
		infoPtr->Flags = 
#if X_BYTE_ORDER == X_LITTLE_ENDIAN
		    HARDWARE_CURSOR_BIT_ORDER_MSBFIRST |
#endif
		    HARDWARE_CURSOR_TRUECOLOR_AT_8BPP;

	infoPtr->SetCursorColors = AlpSetCursorColors;
	infoPtr->SetCursorPosition = AlpSetCursorPosition;
	infoPtr->LoadCursorImage = AlpLoadCursorImage;
	infoPtr->HideCursor = AlpHideCursor;
	infoPtr->ShowCursor = AlpShowCursor;
	infoPtr->UseHWCursor = AlpUseHWCursor;

#ifdef ALP_DEBUG
	ErrorF("AlpHWCursorInit before xf86InitCursor\n");
#endif
	xf86DrvMsg(pScrn->scrnIndex,X_INFO,"Hardware cursor: %ix%i\n",
		   CURSORWIDTH,CURSORHEIGHT);
	return(xf86InitCursor(pScreen, infoPtr));
}


