
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Pci.h"
#include "ct_driver.h"
#include "dgaproc.h"


static Bool CHIPS_OpenFramebuffer(ScrnInfoPtr, char **, unsigned char **, 
					int *, int *, int *);
static Bool CHIPS_SetMode(ScrnInfoPtr, DGAModePtr);
static int  CHIPS_GetViewport(ScrnInfoPtr);
static void CHIPS_SetViewport(ScrnInfoPtr, int, int, int);

static
DGAFunctionRec CHIPS_DGAFuncs = {
   CHIPS_OpenFramebuffer,
   NULL,
   CHIPS_SetMode,
   CHIPS_SetViewport,
   CHIPS_GetViewport,
   NULL, NULL, NULL, NULL
};

static
DGAFunctionRec CHIPS_MMIODGAFuncs = {
   CHIPS_OpenFramebuffer,
   NULL,
   CHIPS_SetMode,
   CHIPS_SetViewport,
   CHIPS_GetViewport,
   NULL, NULL, NULL, NULL
};

static
DGAFunctionRec CHIPS_HiQVDGAFuncs = {
   CHIPS_OpenFramebuffer,
   NULL,
   CHIPS_SetMode,
   CHIPS_SetViewport,
   CHIPS_GetViewport,
   NULL, NULL, NULL, NULL
};


Bool
CHIPSDGAInit(ScreenPtr pScreen)
{   
   ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
   CHIPSPtr cPtr = CHIPSPTR(pScrn);
   DGAModePtr modes = NULL, newmodes = NULL, currentMode;
   DisplayModePtr pMode, firstMode;
   int Bpp = pScrn->bitsPerPixel >> 3;
   int num = 0;
   Bool oneMore;
   int imlines =  (pScrn->videoRam * 1024) /
      (pScrn->displayWidth * (pScrn->bitsPerPixel >> 3));

   pMode = firstMode = pScrn->modes;

   while(pMode) {

	if(0 /*pScrn->displayWidth != pMode->HDisplay*/) {
	    newmodes = realloc(modes, (num + 2) * sizeof(DGAModeRec));
	    oneMore = TRUE;
	} else {
	    newmodes = realloc(modes, (num + 1) * sizeof(DGAModeRec));
	    oneMore = FALSE;
	}

	if(!newmodes) {
	   free(modes);
	   return FALSE;
	}
	modes = newmodes;

SECOND_PASS:

	currentMode = modes + num;
	num++;

	currentMode->mode = pMode;
	currentMode->flags = DGA_CONCURRENT_ACCESS | DGA_PIXMAP_AVAILABLE;
	if(pMode->Flags & V_DBLSCAN)
	   currentMode->flags |= DGA_DOUBLESCAN;
	if(pMode->Flags & V_INTERLACE)
	   currentMode->flags |= DGA_INTERLACED;
	currentMode->byteOrder = pScrn->imageByteOrder;
	currentMode->depth = pScrn->depth;
	currentMode->bitsPerPixel = pScrn->bitsPerPixel;
	currentMode->red_mask = pScrn->mask.red;
	currentMode->green_mask = pScrn->mask.green;
	currentMode->blue_mask = pScrn->mask.blue;
	currentMode->visualClass = (Bpp == 1) ? PseudoColor : TrueColor;
	currentMode->viewportWidth = pMode->HDisplay;
	currentMode->viewportHeight = pMode->VDisplay;
	currentMode->xViewportStep = 1;
	currentMode->yViewportStep = 1;
 	currentMode->viewportFlags = DGA_FLIP_RETRACE | DGA_FLIP_IMMEDIATE;
	currentMode->offset = 0;
	currentMode->address = cPtr->FbBase;

	if(oneMore) { /* first one is narrow width */
	    currentMode->bytesPerScanline = ((pMode->HDisplay * Bpp) + 3) & ~3L;
	    currentMode->imageWidth = pMode->HDisplay;
	    currentMode->imageHeight =  imlines;
	    currentMode->pixmapWidth = currentMode->imageWidth;
	    currentMode->pixmapHeight = currentMode->imageHeight;
	    currentMode->maxViewportX = currentMode->imageWidth - 
					currentMode->viewportWidth;
	    /* this might need to get clamped to some maximum */
	    currentMode->maxViewportY = currentMode->imageHeight -
					currentMode->viewportHeight;
	    oneMore = FALSE;
	    goto SECOND_PASS;
	} else {
	    currentMode->bytesPerScanline = 
			((pScrn->displayWidth * Bpp) + 3) & ~3L;
	    currentMode->imageWidth = pScrn->displayWidth;
	    currentMode->imageHeight =  imlines;
	    currentMode->pixmapWidth = currentMode->imageWidth;
	    currentMode->pixmapHeight = currentMode->imageHeight;
	    currentMode->maxViewportX = currentMode->imageWidth - 
					currentMode->viewportWidth;
	    /* this might need to get clamped to some maximum */
	    currentMode->maxViewportY = currentMode->imageHeight -
					currentMode->viewportHeight;
	}	    

	pMode = pMode->next;
	if(pMode == firstMode)
	   break;
   }

   cPtr->numDGAModes = num;
   cPtr->DGAModes = modes;

   if (IS_HiQV(cPtr)) {
	return DGAInit(pScreen, &CHIPS_HiQVDGAFuncs, modes, num);  
   } else {
	if(!cPtr->UseMMIO) {
	    return DGAInit(pScreen, &CHIPS_DGAFuncs, modes, num);  
	} else {
	    return DGAInit(pScreen, &CHIPS_MMIODGAFuncs, modes, num);  
	}
   }
}


static Bool
CHIPS_SetMode(
   ScrnInfoPtr pScrn,
   DGAModePtr pMode
){
   static int OldDisplayWidth[MAXSCREENS];
   int index = pScrn->pScreen->myNum;

   CHIPSPtr cPtr = CHIPSPTR(pScrn);

   if (!pMode) { /* restore the original mode */
	/* put the ScreenParameters back */
       if (cPtr->DGAactive) {
           pScrn->displayWidth = OldDisplayWidth[index];
	   pScrn->EnterVT(VT_FUNC_ARGS);

	   cPtr->DGAactive = FALSE;
       }
   } else {
	if(!cPtr->DGAactive) {  /* save the old parameters */
	    OldDisplayWidth[index] = pScrn->displayWidth;
	    pScrn->LeaveVT(VT_FUNC_ARGS);
	    cPtr->DGAactive = TRUE;
	}

	pScrn->displayWidth = pMode->bytesPerScanline / 
			      (pMode->bitsPerPixel >> 3);

        CHIPSSwitchMode(SWITCH_MODE_ARGS(pScrn, pMode->mode));
   }
   
   return TRUE;
}



static int  
CHIPS_GetViewport(
  ScrnInfoPtr pScrn
){
    CHIPSPtr cPtr = CHIPSPTR(pScrn);

    return cPtr->DGAViewportStatus;
}

static void 
CHIPS_SetViewport(
   ScrnInfoPtr pScrn, 
   int x, int y, 
   int flags
   ){
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
  
    if (flags & DGA_FLIP_RETRACE) {
	while ((hwp->readST01(hwp)) & 0x08){};
	while (!((hwp->readST01(hwp)) & 0x08)){};
    }

    CHIPSAdjustFrame(ADJUST_FRAME_ARGS(pScrn, x, y));
    cPtr->DGAViewportStatus = 0;  /* CHIPSAdjustFrame loops until finished */
}

static Bool 
CHIPS_OpenFramebuffer(
   ScrnInfoPtr pScrn, 
   char **name,
   unsigned char **mem,
   int *size,
   int *offset,
   int *flags
){
    CHIPSPtr cPtr = CHIPSPTR(pScrn);

    *name = NULL; 		/* no special device */
    *mem = (unsigned char*)cPtr->FbAddress;
    *size = cPtr->FbMapSize;
    *offset = 0;
    *flags = DGA_NEED_ROOT;

    return TRUE;
}
