/*
 * Copyright 2000 by Egbert Eich
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * EGBERT EICH DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Adapted from: Alan Hourihane, <alanh@fairlite.demon.co.uk>
 *           by: Egbert Eich
 */
#include <dix-config.h>

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Pci.h"
#include "vgaHW.h"
#include "cir.h"
#include "dgaproc.h"

static Bool Cir_OpenFramebuffer(ScrnInfoPtr, char **, unsigned char **, 
				  int *, int *, int *);
static Bool Cir_SetMode(ScrnInfoPtr, DGAModePtr);
static int  Cir_GetViewport(ScrnInfoPtr);
static void Cir_SetViewport(ScrnInfoPtr, int, int, int);

static
DGAFunctionRec CirDGAFuncs = {
   Cir_OpenFramebuffer,
   NULL, /* Cir_CloseFramebuffer */
   Cir_SetMode,
   Cir_SetViewport,
   Cir_GetViewport,
   NULL, NULL, NULL,
   NULL  /* Cir_BlitTransRect */
};




_X_EXPORT Bool
CirDGAInit(ScreenPtr pScreen)
{
  ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
  CirPtr pCir = CIRPTR(pScrn);
  DGAModePtr modes = NULL, newmodes = NULL, currentMode;
  DisplayModePtr pMode, firstMode;
  int Bpp = pScrn->bitsPerPixel >> 3;
  int num = 0;
  int imlines =  (pScrn->videoRam * 1024) /
       (pScrn->displayWidth * (pScrn->bitsPerPixel >> 3));

  
  if (!pCir->DGAnumModes) {
    pMode = firstMode = pScrn->modes;
    while (pMode) {
      newmodes = realloc(modes, (num + 1) * sizeof (DGAModeRec));
      if (!newmodes) {
	free(modes);
	return FALSE;
      }
      modes = newmodes;
      currentMode = modes + num;
      num++;
      (void)memset(currentMode, 1, sizeof(DGAModeRec));
      currentMode->mode = pMode;
      currentMode->flags = DGA_PIXMAP_AVAILABLE
	  | ((!pCir->NoAccel) ? (DGA_FILL_RECT | DGA_BLIT_RECT) : 0);
      if (pMode->Flags & V_DBLSCAN)
	currentMode->flags |= DGA_DOUBLESCAN;
      if(pMode->Flags & V_INTERLACE)
	currentMode->flags |= DGA_INTERLACED;
      currentMode->byteOrder        = pScrn->imageByteOrder;
      currentMode->depth            = pScrn->depth;
      currentMode->bitsPerPixel     = pScrn->bitsPerPixel;
      currentMode->red_mask         = pScrn->mask.red;
      currentMode->green_mask       = pScrn->mask.green;
      currentMode->blue_mask        = pScrn->mask.blue;
      currentMode->visualClass      = (Bpp == 1) ? PseudoColor : TrueColor;
      currentMode->viewportWidth    = pMode->HDisplay;
      currentMode->viewportHeight   = pMode->VDisplay;
      currentMode->xViewportStep    = 1; /*  The granularity of x and y pos. */
      currentMode->yViewportStep    = 1;
      currentMode->viewportFlags    = 0 /*DGA_FLIP_RETRACE*/;
      currentMode->offset           = 0;
      currentMode->address          = pCir->FbBase;
      currentMode->bytesPerScanline = ((pScrn->displayWidth * Bpp) + 3) & ~3L;
      currentMode->pixmapWidth      = currentMode->imageWidth 
	                               = pScrn->displayWidth;
      currentMode->pixmapHeight     = currentMode->imageHeight = imlines;
      currentMode->maxViewportX     = currentMode->imageWidth - 
                                      currentMode->viewportWidth;
      /* this might need to get clamped to some maximum */
      currentMode->maxViewportY     = currentMode->imageHeight -
                                      currentMode->viewportHeight;
      
      pMode = pMode->next;
      if(pMode == firstMode)
	break;
    }
    pCir->DGAnumModes = num;
    pCir->DGAModes = modes;
  }
  return DGAInit(pScreen, &CirDGAFuncs, pCir->DGAModes, pCir->DGAnumModes);
}

static Bool 
Cir_OpenFramebuffer(
   ScrnInfoPtr pScrn, 
   char **name,
   unsigned char **mem,
   int *size,
   int *offset,
   int *flags
){
    CirPtr pCir = CIRPTR(pScrn);

    *name = NULL; 		/* no special device */
    *mem = (unsigned char*)(long)pCir->FbAddress;
    *size = pCir->FbMapSize;
    *offset = 0;                /* Always */
    *flags = 0;                 /* Root permissions OS-dependent */

    return TRUE;
}


static Bool
Cir_SetMode(
   ScrnInfoPtr pScrn,
   DGAModePtr pMode
){
   CirPtr pCir = CIRPTR(pScrn);
   static int OldDisplayWidth[MAXSCREENS];
   int index = pScrn->pScreen->myNum;
   Bool ret = FALSE;

   if(!pMode) { /* restore the original mode */
	/* put the ScreenParameters back */
	pScrn->displayWidth = OldDisplayWidth[index];
	ret = pCir->DGAModeInit(xf86Screens[index], pScrn->currentMode);
	pCir->DGAactive = FALSE;
   } else {
	if(!pCir->DGAactive) {  /* save the old parameters */
	    OldDisplayWidth[index] = pScrn->displayWidth;

	    pCir->DGAactive = TRUE;
	}
	pScrn->displayWidth = pMode->bytesPerScanline / 
			      (pMode->bitsPerPixel >> 3);

	ret = pCir->DGAModeInit(xf86Screens[index], pMode->mode);
   }
   return ret;
}

static void 
Cir_SetViewport(
   ScrnInfoPtr pScrn, 
   int x, int y, 
   int flags
){
   CirPtr pCir = CIRPTR(pScrn);
   vgaHWPtr hwp = VGAHWPTR(pScrn);

   pScrn->AdjustFrame(pScrn, x, y);

   while((hwp->readST01(hwp) & 0x08));
   while(!(hwp->readST01(hwp) & 0x08));

   pCir->DGAViewportStatus = 0;  /* AdjustFrame loops until finished */
}

static int  
Cir_GetViewport(
  ScrnInfoPtr pScrn
){
    CirPtr pCir = CIRPTR(pScrn);

    return pCir->DGAViewportStatus;
}

