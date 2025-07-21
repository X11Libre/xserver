/*
   Copyright (c) 1999,2000  The XFree86 Project Inc. 
   based on code written by Mark Vojkovich <markv@valinux.com>
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Pci.h"
#include "shadowfb.h"
#include "servermd.h"
#include "cir.h"
#include "alp.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

_X_EXPORT void
cirRefreshArea(ScrnInfoPtr pScrn, int num, BoxPtr pbox)
{
    CirPtr pCir = CIRPTR(pScrn);
    unsigned char *src, *dst;
   
    int Bpp = pScrn->bitsPerPixel >> 3;
    int FBPitch = BitmapBytePad(pScrn->displayWidth * pScrn->bitsPerPixel);

    while(num--) {
        int x1 = MAX(pbox->x1, 0);
        int y1 = MAX(pbox->y1, 0);
        int x2 = MIN(pbox->x2, pScrn->virtualX);
        int y2 = MIN(pbox->y2, pScrn->virtualY);

        int width = (x2 - x1) * Bpp;
        int height = y2 - y1;

        if (width <= 0 || height <= 0)
            continue;

        src = pCir->ShadowPtr + (y1 * pCir->ShadowPitch) + (x1 * Bpp);
        dst = pCir->FbBase + (y1 * FBPitch) + (x1 * Bpp);

        while(height--) {
            memcpy(dst, src, width);
            dst += FBPitch;
            src += pCir->ShadowPitch;
        }

        pbox++;
    }
} 

_X_EXPORT void
cirPointerMoved(ScrnInfoPtr pScrn, int x, int y)
{
    CirPtr pCir = CIRPTR(pScrn);
    int newX, newY;

    if(pCir->rotate == 1) {
	newX = pScrn->pScreen->height - y - 1;
	newY = x;
    } else {
	newX = y;
	newY = pScrn->pScreen->width - x - 1;
    }

    (*pCir->PointerMoved)(pScrn, newX, newY);
}

_X_EXPORT void
cirRefreshArea8(ScrnInfoPtr pScrn, int num, BoxPtr pbox)
{
    CirPtr pCir = CIRPTR(pScrn);

    int dstPitch = pScrn->displayWidth;
    int srcPitch = -pCir->rotate * pCir->ShadowPitch;

    while(num--) {
        int x1 = MAX(pbox->x1, 0);
        int y1 = MAX(pbox->y1, 0);
        int x2 = MIN(pbox->x2, pScrn->virtualX);
        int y2 = MIN(pbox->y2, pScrn->virtualY);
        int width, height;
        CARD8 *dstPtr, *srcPtr;

        width = x2 - x1;
        y1 = y1 & ~3;
        y2 = (y2 + 3) & ~3;
        height = (y2 - y1) / 4;  /* in dwords */

        if (width <= 0 || height <= 0)
            continue;

        if(pCir->rotate == 1) {
            dstPtr = pCir->FbBase +
			(x1 * dstPitch) + pScrn->virtualX - y2;
            srcPtr = pCir->ShadowPtr + ((1 - y2) * srcPitch) + x1;
        } else {
            dstPtr = pCir->FbBase +
			((pScrn->virtualY - x2) * dstPitch) + y1;
            srcPtr = pCir->ShadowPtr + (y1 * srcPitch) + x2 - 1;
        }

        while(width--) {
            CARD8 *src = srcPtr;
            CARD32 *dst = (CARD32*)dstPtr;
            int count = height;
            while(count--) {
                *(dst++) = src[0] | (src[srcPitch] << 8) |
				(src[srcPitch * 2] << 16) |
				(src[srcPitch * 3] << 24);
                src += srcPitch * 4;
            }
            srcPtr += pCir->rotate;
            dstPtr += dstPitch;
        }

        pbox++;
    }
} 


_X_EXPORT void
cirRefreshArea16(ScrnInfoPtr pScrn, int num, BoxPtr pbox)
{
    CirPtr pCir = CIRPTR(pScrn);

    int dstPitch = pScrn->displayWidth;
    int srcPitch = -pCir->rotate * pCir->ShadowPitch >> 1;

    while(num--) {
        int x1 = MAX(pbox->x1, 0);
        int y1 = MAX(pbox->y1, 0);
        int x2 = MIN(pbox->x2, pScrn->virtualX);
        int y2 = MIN(pbox->y2, pScrn->virtualY);
        int width, height;
        CARD16 *dstPtr, *srcPtr;

        width = x2 - x1;
        y1 = y1 & ~1;
        y2 = (y2 + 1) & ~1;
        height = (y2 - y1) / 2;  /* in dwords */

        if (width <= 0 || height <= 0)
            continue;

        if(pCir->rotate == 1) {
            dstPtr = (CARD16*)pCir->FbBase +
			(x1 * dstPitch) + pScrn->virtualX - y2;
            srcPtr = (CARD16*)pCir->ShadowPtr +
			((1 - y2) * srcPitch) + x1;
        } else {
            dstPtr = (CARD16*)pCir->FbBase +
			((pScrn->virtualY - x2) * dstPitch) + y1;
            srcPtr = (CARD16*)pCir->ShadowPtr +
			(y1 * srcPitch) + x2 - 1;
        }

        while(width--) {
            CARD16 *src = srcPtr;
            CARD32 *dst = (CARD32*)dstPtr;
            int count = height;
            while(count--) {
                *(dst++) = src[0] | (src[srcPitch] << 16);
                src += srcPitch * 2;
            }
            srcPtr += pCir->rotate;
            dstPtr += dstPitch;
        }

        pbox++;
    }
}


/* this one could be faster */
_X_EXPORT void
cirRefreshArea24(ScrnInfoPtr pScrn, int num, BoxPtr pbox)
{
    CirPtr pCir = CIRPTR(pScrn);

    int dstPitch = BitmapBytePad(pScrn->displayWidth * 24);
    int srcPitch = -pCir->rotate * pCir->ShadowPitch;

    while(num--) {
        int x1 = MAX(pbox->x1, 0);
        int y1 = MAX(pbox->y1, 0);
        int x2 = MIN(pbox->x2, pScrn->virtualX);
        int y2 = MIN(pbox->y2, pScrn->virtualY);
        int width, height;
        CARD8 *dstPtr, *srcPtr;

        width = x2 - x1;
        y1 = y1 & ~3;
        y2 = (y2 + 3) & ~3;
        height = (y2 - y1) / 4;  /* blocks of 3 dwords */

        if (width <= 0 || height <= 0)
            continue;

        if(pCir->rotate == 1) {
            dstPtr = pCir->FbBase +
			(x1 * dstPitch) + ((pScrn->virtualX - y2) * 3);
            srcPtr = pCir->ShadowPtr + ((1 - y2) * srcPitch) + (x1 * 3);
        } else {
            dstPtr = pCir->FbBase +
                ((pScrn->virtualY - x2) * dstPitch) + (y1 * 3);
            srcPtr = pCir->ShadowPtr + (y1 * srcPitch) + (x2 * 3) - 3;
        }

        while(width--) {
            CARD8 *src = srcPtr;
            CARD32 *dst = (CARD32*)dstPtr;
            int count = height;
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
            srcPtr += pCir->rotate * 3;
            dstPtr += dstPitch;
        }

        pbox++;
    }
}

_X_EXPORT void
cirRefreshArea32(ScrnInfoPtr pScrn, int num, BoxPtr pbox)
{
    CirPtr pCir = CIRPTR(pScrn);

    int dstPitch = pScrn->displayWidth;
    int srcPitch = -pCir->rotate * pCir->ShadowPitch >> 2;

    while(num--) {
        int x1 = MAX(pbox->x1, 0);
        int y1 = MAX(pbox->y1, 0);
        int x2 = MIN(pbox->x2, pScrn->virtualX);
        int y2 = MIN(pbox->y2, pScrn->virtualY);

        int width = x2 - x1;
        int height = y2 - y1;

        CARD32 *dstPtr, *srcPtr;

        if (width <= 0 || height <= 0)
            continue;

        if(pCir->rotate == 1) {
            dstPtr = (CARD32*)pCir->FbBase +
			(x1 * dstPitch) + pScrn->virtualX - y2;
            srcPtr = (CARD32*)pCir->ShadowPtr +
			((1 - y2) * srcPitch) + x1;
        } else {
            dstPtr = (CARD32*)pCir->FbBase +
			((pScrn->virtualY - x2) * dstPitch) + y1;
            srcPtr = (CARD32*)pCir->ShadowPtr +
			(y1 * srcPitch) + x2 - 1;
        }

        while(width--) {
            CARD32 *src = srcPtr;
            CARD32 *dst = dstPtr;
            int count = height;
            while(count--) {
                *(dst++) = *src;
                src += srcPitch;
            }
            srcPtr += pCir->rotate;
            dstPtr += dstPitch;
        }

        pbox++;
    }
}



