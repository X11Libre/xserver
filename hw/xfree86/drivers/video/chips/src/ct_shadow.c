
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Pci.h"
#include "shadowfb.h"
#include "servermd.h"
#include "ct_driver.h"

void
chipsRefreshArea(ScrnInfoPtr pScrn, int num, BoxPtr pbox)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    int width, height, Bpp, FBPitch;
    unsigned char *src, *dst;
   
    Bpp = pScrn->bitsPerPixel >> 3;
    FBPitch = BitmapBytePad(pScrn->displayWidth * pScrn->bitsPerPixel);

    while(num--) {
	width = (pbox->x2 - pbox->x1) * Bpp;
	height = pbox->y2 - pbox->y1;
	src = cPtr->ShadowPtr + (pbox->y1 * cPtr->ShadowPitch) + 
						(pbox->x1 * Bpp);
	dst = cPtr->FbBase + (pbox->y1 * FBPitch) + (pbox->x1 * Bpp);

	while(height--) {
	    memcpy(dst, src, width);
	    dst += FBPitch;
	    src += cPtr->ShadowPitch;
	}
	
	pbox++;
    }
} 

void
chipsPointerMoved(SCRN_ARG_TYPE arg, int x, int y)
{
    SCRN_INFO_PTR(arg);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    int newX, newY;
    
    if(cPtr->Rotate == 1) {
	newX = pScrn->pScreen->height - y - 1;
	newY = x;
    } else {
	newX = y;
	newY = pScrn->pScreen->width - x - 1;
    }

    (*cPtr->PointerMoved)(arg, newX, newY);
}

void
chipsRefreshArea8(ScrnInfoPtr pScrn, int num, BoxPtr pbox)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    int count, width, height, y1, y2, dstPitch, srcPitch;
    CARD8 *dstPtr, *srcPtr, *src;
    CARD32 *dst;

    dstPitch = pScrn->displayWidth;
    srcPitch = -cPtr->Rotate * cPtr->ShadowPitch;

    while(num--) {
	width = pbox->x2 - pbox->x1;
	y1 = pbox->y1 & ~3;
	y2 = (pbox->y2 + 3) & ~3;
	height = (y2 - y1) >> 2;  /* in dwords */

	if(cPtr->Rotate == 1) {
	    dstPtr = cPtr->FbBase + 
			(pbox->x1 * dstPitch) + pScrn->virtualX - y2;
	    srcPtr = cPtr->ShadowPtr + ((1 - y2) * srcPitch) + pbox->x1;
	} else {
	    dstPtr = cPtr->FbBase + 
			((pScrn->virtualY - pbox->x2) * dstPitch) + y1;
	    srcPtr = cPtr->ShadowPtr + (y1 * srcPitch) + pbox->x2 - 1;
	}

	while(width--) {
	    src = srcPtr;
	    dst = (CARD32*)dstPtr;
	    count = height;
	    while(count--) {
		*(dst++) = src[0] | (src[srcPitch] << 8) | 
					(src[srcPitch * 2] << 16) | 
					(src[srcPitch * 3] << 24);
		src += srcPitch * 4;
	    }
	    srcPtr += cPtr->Rotate;
	    dstPtr += dstPitch;
	}

	pbox++;
    }
} 


void
chipsRefreshArea16(ScrnInfoPtr pScrn, int num, BoxPtr pbox)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    int count, width, height, y1, y2, dstPitch, srcPitch;
    CARD16 *dstPtr, *srcPtr, *src;
    CARD32 *dst;

    dstPitch = pScrn->displayWidth;
    srcPitch = -cPtr->Rotate * cPtr->ShadowPitch >> 1;

    while(num--) {
	width = pbox->x2 - pbox->x1;
	y1 = pbox->y1 & ~1;
	y2 = (pbox->y2 + 1) & ~1;
	height = (y2 - y1) >> 1;  /* in dwords */
	
	if(cPtr->Rotate == 1) {
	    dstPtr = (CARD16*)cPtr->FbBase + 
			(pbox->x1 * dstPitch) + pScrn->virtualX - y2;
	    srcPtr = (CARD16*)cPtr->ShadowPtr + 
			((1 - y2) * srcPitch) + pbox->x1;
	} else {
	    dstPtr = (CARD16*)cPtr->FbBase + 
			((pScrn->virtualY - pbox->x2) * dstPitch) + y1;
	    srcPtr = (CARD16*)cPtr->ShadowPtr + 
			(y1 * srcPitch) + pbox->x2 - 1;
/*	    ErrorF("dst: %x base: %x\n",dstPtr,cPtr->FbBase);*/
	}

	while(width--) {
	    src = srcPtr;
	    dst = (CARD32*)dstPtr;
	    count = height;
	    while(count--) {
		*(dst++) = src[0] | (src[srcPitch] << 16);
		src += srcPitch * 2;
	    }
	    srcPtr += cPtr->Rotate;
	    dstPtr += dstPitch;
	}

	pbox++;
    }
}


/* this one could be faster */
void
chipsRefreshArea24(ScrnInfoPtr pScrn, int num, BoxPtr pbox)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    int count, width, height, y1, y2, dstPitch, srcPitch;
    CARD8 *dstPtr, *srcPtr, *src;
    CARD32 *dst;

    dstPitch = BitmapBytePad(pScrn->displayWidth * 24);
    srcPitch = -cPtr->Rotate * cPtr->ShadowPitch;

    while(num--) {
        width = pbox->x2 - pbox->x1;
        y1 = pbox->y1 & ~3;
        y2 = (pbox->y2 + 3) & ~3;
        height = (y2 - y1) >> 2;  /* blocks of 3 dwords */

	if(cPtr->Rotate == 1) {
	    dstPtr = cPtr->FbBase + 
			(pbox->x1 * dstPitch) + ((pScrn->virtualX - y2) * 3);
	    srcPtr = cPtr->ShadowPtr + ((1 - y2) * srcPitch) + (pbox->x1 * 3);
	} else {
	    dstPtr = cPtr->FbBase + 
			((pScrn->virtualY - pbox->x2) * dstPitch) + (y1 * 3);
	    srcPtr = cPtr->ShadowPtr + (y1 * srcPitch) + (pbox->x2 * 3) - 3;
	}

	while(width--) {
	    src = srcPtr;
	    dst = (CARD32*)dstPtr;
	    count = height;
	    while(count--) {
		dst[0] = src[0] | (src[1] << 8) | (src[2] << 16) |
				(src[srcPitch] << 24);		
		dst[1] = src[srcPitch + 1] | (src[srcPitch + 2] << 8) |
				(src[srcPitch * 2] << 16) |
				(src[(srcPitch * 2) + 1] << 24);		
		dst[2] = src[(srcPitch * 2) + 2] | (src[srcPitch * 3] << 8) |
				(src[(srcPitch * 3) + 1] << 16) |
				(src[(srcPitch * 3) + 2] << 24);	
		dst += 3;
		src += srcPitch * 4;
	    }
	    srcPtr += cPtr->Rotate * 3;
	    dstPtr += dstPitch; 
	}

	pbox++;
    }
}

void
chipsRefreshArea32(ScrnInfoPtr pScrn, int num, BoxPtr pbox)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    int count, width, height, dstPitch, srcPitch;
    CARD32 *dstPtr, *srcPtr, *src, *dst;

    dstPitch = pScrn->displayWidth;
    srcPitch = -cPtr->Rotate * cPtr->ShadowPitch >> 2;

    while(num--) {
	width = pbox->x2 - pbox->x1;
	height = pbox->y2 - pbox->y1;

	if(cPtr->Rotate == 1) {
	    dstPtr = (CARD32*)cPtr->FbBase + 
			(pbox->x1 * dstPitch) + pScrn->virtualX - pbox->y2;
	    srcPtr = (CARD32*)cPtr->ShadowPtr + 
			((1 - pbox->y2) * srcPitch) + pbox->x1;
	} else {
	    dstPtr = (CARD32*)cPtr->FbBase + 
			((pScrn->virtualY - pbox->x2) * dstPitch) + pbox->y1;
	    srcPtr = (CARD32*)cPtr->ShadowPtr + 
			(pbox->y1 * srcPitch) + pbox->x2 - 1;
	}

	while(width--) {
	    src = srcPtr;
	    dst = dstPtr;
	    count = height;
	    while(count--) {
		*(dst++) = *src;
		src += srcPitch;
	    }
	    srcPtr += cPtr->Rotate;
	    dstPtr += dstPitch;
	}

	pbox++;
    }
}
