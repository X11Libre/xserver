/*
 * file: apm_dga.c
 * ported from s3virge, ported from mga
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Pci.h"
#include "apm.h"
#include "dgaproc.h"


static Bool ApmOpenFramebuffer(ScrnInfoPtr, char **, unsigned char **, 
					int *, int *, int *);
static Bool ApmSetMode(ScrnInfoPtr, DGAModePtr);
static int  ApmGetViewport(ScrnInfoPtr);
static void ApmSetViewport(ScrnInfoPtr, int, int, int);
static void ApmSync(ScrnInfoPtr);

static
DGAFunctionRec ApmDGAFuncs = {
    ApmOpenFramebuffer,
    NULL,
    ApmSetMode,
    ApmSetViewport,
    ApmGetViewport,
    ApmSync,
    NULL, NULL, NULL
};

/*
 * Placeholder
 */
void
ApmSync(ScrnInfoPtr pScrn)
{
}

static __inline__ int FindSmallestPitch(ApmPtr pApm, int Bpp, int width)
{
    if (width <= 640)
	return 640;
    else if (width <= 800)
	return 800;
    else if (width <= 1024)
	return 1024;
    else if (width <= 1152)
	return 1152;
    else if (width <= 1280)
	return 1280;
    else if (width <= 1600)
	return 1600;
    return (width + 7) & ~7;
}

static DGAModePtr
ApmSetupDGAMode(ScrnInfoPtr pScrn, DGAModePtr modes, int *num,
		   int bitsPerPixel, int depth, Bool pixmap, int secondPitch,
		   unsigned long red, unsigned long green, unsigned long blue,
		   short visualClass)
{
   DisplayModePtr firstMode, pMode;
   APMDECL(pScrn);
   DGAModePtr mode, newmodes;
   int size, pitch, Bpp = bitsPerPixel >> 3;
   Bool reduced_pitch = TRUE;

SECOND_PASS:

   firstMode = NULL;

   for (pMode = pScrn->modes; pMode != firstMode; pMode = pMode->next) {

	if (!firstMode)
	    firstMode = pMode;

	if (reduced_pitch)
	    pitch = FindSmallestPitch(pApm, Bpp, pMode->HDisplay);
	else
	    pitch = pMode->HDisplay;
	if (!reduced_pitch && pitch == FindSmallestPitch(pApm, Bpp, pMode->HDisplay))
	    continue;

	size = pitch * Bpp * pMode->VDisplay;

	if((!secondPitch || (pitch != secondPitch)) &&
		(size <= pScrn->videoRam * 1024 - pApm->OffscreenReserved)) {

	    if(secondPitch)
		pitch = secondPitch; 

	    if(!(newmodes = realloc(modes, (*num + 1) * sizeof(DGAModeRec))))
		break;

	    modes = newmodes;
	    mode = modes + *num;

	    mode->mode = pMode;
	    mode->flags = DGA_CONCURRENT_ACCESS;

	    if(pixmap)
		mode->flags |= DGA_PIXMAP_AVAILABLE;
	    if(!pApm->NoAccel) {
		mode->flags |= DGA_FILL_RECT | DGA_BLIT_RECT;
		if (Bpp != 3)
		    mode->flags |= DGA_BLIT_RECT_TRANS;
	    }
	    if(pMode->Flags & V_DBLSCAN)
		mode->flags |= DGA_DOUBLESCAN;
	    if(pMode->Flags & V_INTERLACE)
		mode->flags |= DGA_INTERLACED;
	    mode->byteOrder = pScrn->imageByteOrder;
	    mode->depth = depth;
	    mode->bitsPerPixel = bitsPerPixel;
	    mode->red_mask = red;
	    mode->green_mask = green;
	    mode->blue_mask = blue;
	    mode->visualClass = visualClass;
	    mode->viewportWidth = pMode->HDisplay;
	    mode->viewportHeight = pMode->VDisplay;
	    mode->xViewportStep = (bitsPerPixel == 24) ? 4 : 1;
	    mode->yViewportStep = 1;
	    mode->viewportFlags = DGA_FLIP_RETRACE;
	    mode->offset = 0;
	    mode->address = pApm->FbBase;
	    mode->bytesPerScanline = pitch * Bpp;
	    mode->imageWidth = pitch;
	    mode->imageHeight =  (pScrn->videoRam * 1024 -
			pApm->OffscreenReserved) / mode->bytesPerScanline; 
	    mode->pixmapWidth = mode->imageWidth;
	    mode->pixmapHeight = mode->imageHeight;
	    mode->maxViewportX = mode->imageWidth - mode->viewportWidth;
	   /* this might need to get clamped to some maximum */
	    mode->maxViewportY = mode->imageHeight - mode->viewportHeight;

	    (*num)++;
	}

    }

    if(secondPitch) {
	secondPitch = 0;
	goto SECOND_PASS;
    }

    if (reduced_pitch) {
	reduced_pitch = FALSE;
	goto SECOND_PASS;
    }

    return modes;
}

Bool
ApmDGAInit(ScreenPtr pScreen)
{   
   ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
   APMDECL(pScrn);
   DGAModePtr modes = NULL;
   int num = 0;

   /* 8 */
   modes = ApmSetupDGAMode (pScrn, modes, &num, 8, 8, 
		(pScrn->bitsPerPixel != 24),
		(pScrn->bitsPerPixel != 8) ? 0 : pScrn->displayWidth,
		0, 0, 0, PseudoColor);

   /* 15 */
   modes = ApmSetupDGAMode (pScrn, modes, &num, 16, 15, 
		(pScrn->bitsPerPixel != 24),
		(pScrn->depth != 15) ? 0 : pScrn->displayWidth,
		0x7C00, 0x03E0, 0x001F, TrueColor);

   modes = ApmSetupDGAMode (pScrn, modes, &num, 16, 15, 
		(pScrn->bitsPerPixel != 24),
		(pScrn->depth != 15) ? 0 : pScrn->displayWidth,
		0x7C00, 0x03E0, 0x001F, DirectColor);

   /* 16 */
   modes = ApmSetupDGAMode (pScrn, modes, &num, 16, 16, 
		(pScrn->bitsPerPixel != 24),
		(pScrn->depth != 16) ? 0 : pScrn->displayWidth,
		0xF800, 0x07E0, 0x001F, TrueColor);

   modes = ApmSetupDGAMode (pScrn, modes, &num, 16, 16, 
		(pScrn->bitsPerPixel != 24),
		(pScrn->depth != 16) ? 0 : pScrn->displayWidth,
		0xF800, 0x07E0, 0x001F, DirectColor);

   /* 24 */
   modes = ApmSetupDGAMode (pScrn, modes, &num, 24, 24, 
		(pScrn->bitsPerPixel == 24),
		(pScrn->bitsPerPixel != 24) ? 0 : pScrn->displayWidth,
		0xFF0000, 0x00FF00, 0x0000FF, TrueColor);

   modes = ApmSetupDGAMode (pScrn, modes, &num, 24, 24, 
		(pScrn->bitsPerPixel == 24),
		(pScrn->bitsPerPixel != 24) ? 0 : pScrn->displayWidth,
		0xFF0000, 0x00FF00, 0x0000FF, DirectColor);

   /* 32 */
   modes = ApmSetupDGAMode (pScrn, modes, &num, 32, 24, 
		(pScrn->bitsPerPixel != 24),
		(pScrn->bitsPerPixel != 32) ? 0 : pScrn->displayWidth,
		0xFF0000, 0x00FF00, 0x0000FF, TrueColor);

   modes = ApmSetupDGAMode (pScrn, modes, &num, 32, 24, 
		(pScrn->bitsPerPixel != 24),
		(pScrn->bitsPerPixel != 32) ? 0 : pScrn->displayWidth,
		0xFF0000, 0x00FF00, 0x0000FF, DirectColor);

   pApm->numDGAModes = num;
   pApm->DGAModes = modes;

   return DGAInit(pScreen, &ApmDGAFuncs, modes, num);  
}


static Bool
ApmSetMode(ScrnInfoPtr pScrn, DGAModePtr pMode)
{
    APMDECL(pScrn);

    if (!pMode) { /* restore the original mode */
	if (pApm->DGAactive) {
	    memcpy(&pApm->CurrentLayout, &pApm->SavedLayout,
						sizeof pApm->CurrentLayout);
	    pApm->DGAactive = FALSE;
	}

	pScrn->currentMode = pApm->CurrentLayout.pMode;
	ApmSwitchMode(pScrn, pScrn->currentMode);
	ApmAdjustFrame(pScrn, pScrn->frameX0, pScrn->frameY0);
#if 0
	if (pApm->AccelInfoRec)
	    XAAInit(pScrn->pScreen, pApm->AccelInfoRec);
#endif
    }
    else {
	if (!pApm->DGAactive) {
	    memcpy(&pApm->SavedLayout, &pApm->CurrentLayout,
						sizeof pApm->CurrentLayout);
	    pApm->DGAactive = TRUE;
	}

	pApm->CurrentLayout.displayWidth	= pMode->imageWidth;
	pApm->CurrentLayout.displayHeight	= pMode->imageHeight;
	pApm->CurrentLayout.Scanlines		= pMode->imageHeight + 1;
	pApm->CurrentLayout.depth		= pMode->depth;
	pApm->CurrentLayout.bitsPerPixel	= pMode->bitsPerPixel;
	pApm->CurrentLayout.bytesPerScanline	= pMode->bytesPerScanline;
	pApm->CurrentLayout.pMode		= pMode->mode;
	if (pMode->bitsPerPixel == 24)
	    pApm->CurrentLayout.mask32		= 3;
	else
	    pApm->CurrentLayout.mask32		= 32 / pMode->bitsPerPixel - 1;

        ApmSwitchMode(pScrn, pMode->mode);

#if 0
	if (pApm->DGAXAAInfo)
	    bzero(pApm->DGAXAAInfo, sizeof(*pApm->DGAXAAInfo));
	else
	    pApm->DGAXAAInfo = XAACreateInfoRec();
	ApmSetupXAAInfo(pApm, pApm->DGAXAAInfo);
	/* 
	 * Let's hope this won't fail, that is reinitialize XAA for this
	 * setup...
	 */
	XAAInit(pScrn->pScreen, pApm->DGAXAAInfo);
#endif
    }

    return TRUE;
}



static int  
ApmGetViewport(
  ScrnInfoPtr pScrn
)
{
    return 0;
}

static void 
ApmSetViewport(
    ScrnInfoPtr pScrn, 
    int x, int y, 
    int flags
)
{
    unsigned char tmp;

    APMDECL(pScrn);

    if (pApm->apmLock) {
	/*
	 * This is just an attempt, because Daryll is tampering with MY
	 * registers.
	 */
	tmp = (RDXB(0xDB) & 0xF4) |  0x0A;
	WRXB(0xDB, tmp);
	ApmWriteSeq(0x1B, 0x20);
	ApmWriteSeq(0x1C, 0x2F);
	pApm->apmLock = FALSE;
    }
    pScrn->AdjustFrame(pScrn, x, y);
    if (pApm->VGAMap) {
	/* Wait until vertical retrace is in progress. */
	while (APMVGAB(0x3DA) & 0x08);
	while (!(APMVGAB(0x3DA) & 0x08));
    }
    else {
	/* Wait until vertical retrace is in progress. */
	while (inb(pApm->iobase + 0x3DA) & 0x08);
	while (!(inb(pApm->iobase + 0x3DA) & 0x08));            
    }
}

static Bool 
ApmOpenFramebuffer(
    ScrnInfoPtr pScrn, 
    char **name,
    unsigned char **mem,
    int *size,
    int *offset,
    int *flags
)
{
    APMDECL(pScrn);

    *name = NULL; 		/* no special device */
    *mem = (unsigned char*)(pApm->LinAddress +
			0*((char *)pApm->FbBase - (char *)pApm->LinMap));
    *size = pScrn->videoRam << 10;
    *offset = 0;
    *flags = DGA_NEED_ROOT;

    return TRUE;
}
