
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "input.h"
#include "cursorstr.h"
#include "regionstr.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "windowstr.h"
#include "mipointer.h"

#include "apm.h"

#define CURSORWIDTH	64
#define CURSORHEIGHT	64
#define CURSORSIZE	(CURSORWIDTH * CURSORHEIGHT / 8)
#define CURSORALIGN	((CURSORSIZE + 1023) & ~1023l)

static void	ApmShowCursor(ScrnInfoPtr pScrn);
static void	ApmHideCursor(ScrnInfoPtr pScrn);
static void	ApmSetCursorPosition(ScrnInfoPtr pScrn, int x, int y);
static void	ApmSetCursorColors(ScrnInfoPtr pScrn, int bg, int fg);
static void	ApmLoadCursorImage(ScrnInfoPtr pScrn, u8* data);
static Bool	ApmUseHWCursor(ScreenPtr pScreen, CursorPtr pCurs);

static u8 ConvertTable[256];

/* Inline functions */
static __inline__ void
WaitForFifo(ApmPtr pApm, int slots)
{
  if (!pApm->UsePCIRetry) {
    volatile int i;
#define MAXLOOP 1000000

    for(i = 0; i < MAXLOOP; i++) {
      if ((STATUS() & STATUS_FIFO) >= slots)
	break;
    }
    if (i == MAXLOOP) {
      unsigned int status = STATUS();

      WRXB(0x1FF, 0);
      if (!xf86ServerIsExiting())
	FatalError("Hung in WaitForFifo() (Status = 0x%08X)\n", status);
    }
  }
}

void ApmHWCursorReserveSpace(ApmPtr pApm)
{
  pApm->OffscreenReserved	+= 2 * CURSORALIGN;
  pApm->DisplayedCursorAddress	= pApm->BaseCursorAddress =
  pApm->CursorAddress	= 1024 * xf86ScreenToScrn(pApm->pScreen)->videoRam -
					pApm->OffscreenReserved;
}


int ApmHWCursorInit(ScreenPtr pScreen)
{
  ScrnInfoPtr		pScrn = xf86ScreenToScrn(pScreen);
  APMDECL(pScrn);
  xf86CursorInfoPtr	infoPtr;
  u32			i;

  infoPtr = xf86CreateCursorInfoRec();
  if (!infoPtr)
      return FALSE;

  pApm->CursorInfoRec		= infoPtr;

  infoPtr->MaxWidth	= CURSORWIDTH;
  infoPtr->MaxHeight	= CURSORHEIGHT;

  infoPtr->Flags = HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_1;
  infoPtr->SetCursorColors	= ApmSetCursorColors;
  infoPtr->SetCursorPosition	= ApmSetCursorPosition;
  infoPtr->LoadCursorImage	= ApmLoadCursorImage;
  infoPtr->HideCursor		= ApmHideCursor;
  infoPtr->ShowCursor		= ApmShowCursor;
  infoPtr->UseHWCursor		= ApmUseHWCursor;
  
  /* Set up the convert table for the input cursor data */
  for (i = 0; i < 256; i++)
    ConvertTable[i] = ((~i) & 0xAA) | (i & (i >> 1) & 0x55);

  return xf86InitCursor(pScreen, infoPtr);
}

 
static void 
ApmShowCursor(ScrnInfoPtr pScrn)
{
  APMDECL(pScrn);

  WaitForFifo(pApm, 2);
  WRXW(0x144, pApm->CursorAddress >> 10);
  WRXB(0x140, 1);
  pApm->DisplayedCursorAddress = pApm->CursorAddress;
}


static void
ApmHideCursor(ScrnInfoPtr pScrn)
{
  APMDECL(pScrn);

  WaitForFifo(pApm, 1);
  WRXB(0x140, 0);
}

static Bool ApmUseHWCursor(ScreenPtr pScreen, CursorPtr pCurs)
{
    return APMPTR(xf86ScreenToScrn(pScreen))->CurrentLayout.bitsPerPixel >= 8;
}

static void
ApmSetCursorPosition(ScrnInfoPtr pScrn, int x, int y)
{
  APMDECL(pScrn);
  int	xoff, yoff;

  if (x < -CURSORWIDTH || y < -CURSORHEIGHT) {
      WaitForFifo(pApm, 1);
      WRXB(0x140, 0);
      return;
  }

  if (x < 0) {
      xoff = -x;
      x = 0;
  }
  else
      xoff = 0;
  if (y < 0) {
      yoff = -y;
      y = 0;
  }
  else
      yoff = 0;

  WaitForFifo(pApm, 2);
  WRXW(0x14C, (yoff << 8) | (xoff & 0xFF));
  WRXL(0x148, (y << 16) | (x & 0xFFFF));
}


static void
ApmSetCursorColors(ScrnInfoPtr pScrn, int bg, int fg)
{	 
  APMDECL(pScrn);
  u16 packedcolfg, packedcolbg;

  if (pApm->CurrentLayout.bitsPerPixel == 8)
  {
    WaitForFifo(pApm, 2);
    WRXB(0x141, fg);
    WRXB(0x142, bg);
  }
  else
  {
    packedcolfg = 
      ((fg & 0xE00000) >> 16) |
      ((fg & 0x00E000) >> 11) |
      ((fg & 0x0000C0) >> 6);
    packedcolbg = 
      ((bg & 0xE00000) >> 16) |
      ((bg & 0x00E000) >> 11) |
      ((bg & 0x0000C0) >> 6);
    WaitForFifo(pApm, 2);
    WRXB(0x141, packedcolfg);
    WRXB(0x142, packedcolbg);
  }
}


static void 
ApmLoadCursorImage(ScrnInfoPtr pScrn, u8* data)
{
  APMDECL(pScrn);
  u32 i;
  u8 tmp[2 * CURSORSIZE];

  /* Correct input data */
  for (i = 0; i < sizeof tmp; i++)
    tmp[i] = ConvertTable[data[i]];
  /*
   * To avoid flicker.
   * Note: 2*pApm->BaseCursorAddress + CURSORALIGN (=1024) < 2^31 all the time.
   */
  pApm->CursorAddress = 2*pApm->BaseCursorAddress + CURSORALIGN - pApm->DisplayedCursorAddress;
  memcpy((u8*)pApm->FbBase + pApm->CursorAddress, tmp, sizeof tmp);
}
