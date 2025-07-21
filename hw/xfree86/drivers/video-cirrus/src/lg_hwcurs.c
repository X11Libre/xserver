/*
 * Hardware cursor support for CL-GD546x -- The Laugna family
 *
 * lg_hwcurs.c
 *
 * (c) 1998 Corin Anderson.
 *          corina@the4cs.com
 *          Tukwila, WA
 *
 * Much of this code is inspired by the HW cursor code from XFree86
 * 3.3.3.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"
#include "xf86_OSproc.h"
#include "compiler.h"

#include "xf86Pci.h"

#include "vgaHW.h"

#include "cir.h"
#define _LG_PRIVATE_
#include "lg.h"
#include "lg_blit.h" /* For BitBLT engine macros */

/*
#define LG_CURSOR_DEBUG
*/

#define CURSORWIDTH	64
#define CURSORHEIGHT	64
#define CURSORSIZE      (CURSORWIDTH*CURSORHEIGHT/8)

/* Some registers used for the HW cursor. */
enum {
  PALETTE_READ_ADDR  = 0x00A4,
  PALETTE_WRITE_ADDR = 0x00A8,
  PALETTE_DATA       = 0x00AC,

  PALETTE_STATE      = 0x00B0,
  CURSOR_X_POS       = 0x00E0,
  CURSOR_Y_POS       = 0x00E2,
  CURSOR_PRESET      = 0x00E4,
	CURSOR_CONTROL		= 0x00E6,
  CURSOR_ADDR        = 0x00E8
};
  

static void
LgFindCursorTile(ScrnInfoPtr pScrn, int *x, int *y, int *width, int *height,
		 CARD32 *curAddr);


/*
 * Set the FG and BG colors of the HW cursor.
 */
static void LgSetCursorColors(ScrnInfoPtr pScrn, int bg, int fg)
{
    const CirPtr pCir = CIRPTR(pScrn);

#ifdef LG_CURSOR_DEBUG
  ErrorF("LgSetCursorColors\n");
#endif

  /* Enable access to cursor colors in palette */
  memwb(PALETTE_STATE, (memrb(PALETTE_STATE) | (1<<3)));

  /* Slam in the color */

  memwb(PALETTE_WRITE_ADDR, 0x00);
  memwb(PALETTE_DATA, (bg >> 16));
  memwb(PALETTE_DATA, (bg >>  8));
  memwb(PALETTE_DATA, (bg >>  0));
  memwb(PALETTE_WRITE_ADDR, 0x0F);
  memwb(PALETTE_DATA, (fg >> 16));
  memwb(PALETTE_DATA, (fg >>  8));
  memwb(PALETTE_DATA, (fg >>  0));

  /* Disable access to cursor colors */
  memwb(PALETTE_STATE, (memrb(PALETTE_STATE) & ~(1<<3)));
}


/*
 * Set the (x,y) position of the pointer.
 *
 * Note:  (x,y) are /frame/ relative, not /framebuffer/ relative.
 * That is, if the virtual desktop has been panned all the way to 
 * the right, and the pointer is to be in the upper-right hand corner 
 * of the viewable screen, the pointer coords are (0,0) (even though
 * the pointer is on, say (550,0) wrt the frame buffer).  This is, of
 * course, a /good/ thing -- we don't want to have to deal with where
 * the virtual display is, etc, in the cursor code. 
 *
 */
static void LgSetCursorPosition(ScrnInfoPtr pScrn, int x, int y)
{
    const CirPtr pCir = CIRPTR(pScrn);

#if 0
#ifdef LG_CURSOR_DEBUG
  ErrorF("LgSetCursorPosition %d %d\n", x, y);
#endif
#endif

  if (x < 0 || y < 0) {
    CARD16 oldPreset = memrw(CURSOR_PRESET);
    CARD16 newPreset = 0x8080 & oldPreset; /* Reserved bits */

    if (x < 0) {
      newPreset |= ((-x & 0x7F) << 8);
      x = 0;
    }

    if (y < 0) {
      newPreset |= ((-y & 0x7F) << 0);
      y = 0;
    }

    memww(CURSOR_PRESET, newPreset);
    pCir->CursorIsSkewed = TRUE;
	} else if (pCir->CursorIsSkewed) {
    /* Reset the hotspot location. */
    memww(CURSOR_PRESET, memrw(CURSOR_PRESET & 0x8080));
    pCir->CursorIsSkewed = FALSE;
  }

  /* Commit the new position to the card. */
  memww(CURSOR_X_POS, x);
  memww(CURSOR_Y_POS, y);
}


/*
 * Load the cursor image to the card.  The cursor image is given in
 * bits.  The format is:  ???
 */
static void LgLoadCursorImage(ScrnInfoPtr pScrn, unsigned char *bits)
{
    const CirPtr pCir = CIRPTR(pScrn);
    const LgPtr pLg = LGPTR(pCir);

    volatile CARD32 *pXCursorBits = (CARD32 *)bits;

  int l, w;

#ifdef LG_CURSOR_DEBUG
  ErrorF("LgLoadCursorImage\n");
#endif

  /* All ("all") we have to do is a simple CPU-to-screen copy of the
     cursor image to the frame buffer. */

  while (!LgREADY()) {}
    ;

  /* Wait until there's ample room in the chip's queue */
  while (memrb(QFREE) < 10) {}
    ;

  LgSETMODE(HOST2SCR);       /* Host-to-screen blit */
  LgSETROP(0x00CC);          /* Source copy */

  /* First, copy our transparent cursor image to the next 1/2 tile boundary */
  /* Destination */
  LgSETMDSTXY(pLg->HWCursorImageX+pLg->HWCursorTileWidth, pLg->HWCursorImageY);

  /* Set the source pitch.  0 means that, worst case, the source is 
     aligned only on a byte boundary */
  LgSETMPHASE1(0);

  LgSETMEXTENTSNOMONOQW(pLg->HWCursorTileWidth, pLg->HWCursorTileHeight);

  for (l = 0; l < CURSORHEIGHT; l++) {
    /* Plane 0 */
    for (w = 0; w < CURSORWIDTH >> 5; w++) 
      memwl(HOSTDATA, 0x00000000);
    /* Plane 1 */
    for (w = 0; w < CURSORWIDTH >> 5; w++) 
      memwl(HOSTDATA, 0x00000000);
  }

  /* Now, copy the real cursor image */
    
  /* Set the destination */
  LgSETMDSTXY(pLg->HWCursorImageX, pLg->HWCursorImageY);

  /* Set the source pitch.  0 means that, worst case, the source is 
     aligned only on a byte boundary */
  LgSETMPHASE1(0);

  /* Always copy an entire cursor image to the card. */
  LgSETMEXTENTSNOMONOQW(pLg->HWCursorTileWidth, pLg->HWCursorTileHeight);

  for (l = 0; l < CURSORHEIGHT; l++) {
    /* Plane 0 */
    for (w = 0; w < CURSORWIDTH >> 5; w++) 
      memwl(HOSTDATA, *pXCursorBits++);
    /* Plane 1 */
    for (w = 0; w < CURSORWIDTH >> 5; w++) 
      memwl(HOSTDATA, *pXCursorBits++);
  }

  while (!LgREADY())
    ;
}



/*
 * LgFindCursorTile() finds the tile of display memory that will be
 * used to load the pointer image into.  The tile chosen will be the
 * last tile in the last line of the frame buffer. 
 */
static void
LgFindCursorTile(ScrnInfoPtr pScrn, int *x, int *y, int *width, int *height,
					CARD32 *curAddr)
{
    CirPtr pCir = CIRPTR(pScrn);
    LgPtr pLg = LGPTR(pCir);

  int videoRam = pScrn->videoRam; /* in K */
  int tileHeight = LgLineData[pLg->lineDataIndex].width?8:16;
  int tileWidth = LgLineData[pLg->lineDataIndex].width?256:128;
  int tilesPerLine = LgLineData[pLg->lineDataIndex].tilesPerLine;
  int filledOutTileLines, leftoverMem;
  int yTile, xTile;
    
  filledOutTileLines = videoRam / (tilesPerLine * 2); /* tiles are 2K */
  leftoverMem = videoRam - filledOutTileLines*tilesPerLine*2;
    
  if (leftoverMem > 0) {
    yTile = filledOutTileLines;
  } else {
    /* There is no incomplete row of tiles.  Then just use the last
       tile in the last line */
    yTile = filledOutTileLines - 1;
  }
  xTile = 0;   /* Always use the first tile in the determined tile row */

  /* The (x,y) coords of the pointer image. */
  if (x)
    *x = xTile * tileWidth;
  if (y)
    *y = yTile * tileHeight;

  if (width)
    *width = tileWidth;
  if (height)
    *height = tileHeight / 2;

  /* Now, compute the linear address of the cursor image.  This process
     is unpleasant because the memory is tiled, and we essetially have
     to undo the tiling computation. */
  if (curAddr) {
    unsigned int nIL;  /* Interleaving */
    nIL = pLg->memInterleave==0x00? 1 : (pLg->memInterleave==0x40 ? 2 : 4);

		if (PCI_CHIP_GD5465 == pCir->Chipset) {
      /* The Where's The Cursor formula changed for the 5465.  It's really 
	 kinda weird now. */
      unsigned long page, bank;
      unsigned int nX, nY;
      
      nX = xTile * tileWidth;
      nY = yTile * tileHeight;
      
      page = (nY / (tileHeight * nIL)) * tilesPerLine + nX / tileWidth;
      bank = (nX/tileWidth + nY/tileHeight) % nIL + page/(512*nIL);
      page = page & 0x1FF;
      *curAddr = bank*1024*1024L + page*2048 + (nY%tileHeight)*tileWidth;
    } else {
       int tileNumber = (tilesPerLine*nIL) * (yTile/nIL) + yTile % nIL;
       *curAddr = tileNumber * 2048;
    }
  }
}




/*
 * Hide/disable the HW cursor.
 */
void LgHideCursor(ScrnInfoPtr pScrn)
{
    const CirPtr pCir = CIRPTR(pScrn);

  /* To hide the cursor, we kick it off into the corner, and then set the
     cursor image to be a transparent bitmap.  That way, if X continues
     to move the cursor while it is hidden, there is no way that the user
     can move the cursor back on screen!

     We don't just clear the cursor enable bit because doesn't work in some
     cases (like when switching back to text mode).
     */
  
#ifdef LG_CURSOR_DEBUG
  ErrorF("LgHideCursor\n");
#endif

  memww(CURSOR_CONTROL, (memrw(CURSOR_CONTROL) & 0xFFFE));
}

void LgShowCursor(ScrnInfoPtr pScrn)
{
    const CirPtr pCir = CIRPTR(pScrn);
    const LgPtr pLg = LGPTR(pCir);

#ifdef LG_CURSOR_DEBUG
  ErrorF("LgShowCursor\n");
#endif

  memww(CURSOR_CONTROL,(memrw(CURSOR_CONTROL) | (1<<0)));
  memww(CURSOR_ADDR,(pLg->HWCursorAddr & 0x7FFC));
}


/*
 * Can the HW cursor be used?
 */
static Bool LgUseHWCursor(ScreenPtr pScreen, CursorPtr pCurs)
{
  ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);

#ifdef LG_CURSOR_DEBUG
  ErrorF("LgUseHWCursor\n");
#endif

  if(pScrn->bitsPerPixel < 8)
    return FALSE;

  return TRUE;
}


/*
 * Initialize all the fun HW cursor code.
 */
Bool LgHWCursorInit(ScreenPtr pScreen)
{
  ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
  CirPtr pCir = CIRPTR(pScrn);
  xf86CursorInfoPtr infoPtr;

#ifdef LG_CURSOR_DEBUG
  ErrorF("LgHWCursorInit\n");
#endif

  infoPtr = xf86CreateCursorInfoRec();
  if(!infoPtr) return FALSE;
  
	pCir->CursorInfoRec = infoPtr;
	LgFindCursorTile(pScrn, &pCir->chip.lg->HWCursorImageX, &pCir->chip.lg->HWCursorImageY,
		   &pCir->chip.lg->HWCursorTileWidth, &pCir->chip.lg->HWCursorTileHeight,
		   &pCir->chip.lg->HWCursorAddr);
  /* Keep only bits 22:10 of the address. */
  pCir->chip.lg->HWCursorAddr = (pCir->chip.lg->HWCursorAddr >> 8) & 0x7FFC;
  
	pCir->CursorIsSkewed = FALSE;

  infoPtr->MaxWidth = CURSORWIDTH;
  infoPtr->MaxHeight = CURSORHEIGHT;
	infoPtr->Flags = HARDWARE_CURSOR_TRUECOLOR_AT_8BPP
					| HARDWARE_CURSOR_AND_SOURCE_WITH_MASK
					| HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_64;
  infoPtr->SetCursorColors = LgSetCursorColors;
  infoPtr->SetCursorPosition = LgSetCursorPosition;
  infoPtr->LoadCursorImage = LgLoadCursorImage;
  infoPtr->HideCursor = LgHideCursor;
  infoPtr->ShowCursor = LgShowCursor;
  infoPtr->UseHWCursor = LgUseHWCursor;

#ifdef LG_CURSOR_DEBUG
  ErrorF("LgHWCursorInit before xf86InitCursor\n");
#endif

  return(xf86InitCursor(pScreen, infoPtr));
}
