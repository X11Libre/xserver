/*
 * EXA acceleration for now ct65550 only, for lack of other hardware 
 *
 * Copyright (C) 2016 Michael Lorenz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * MICHAEL LORENZ BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <xorg-config.h>

#include <sys/types.h>

/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"

/* Everything using inb/outb, etc needs "compiler.h" */
#include "compiler.h"

#include "ct_driver.h"
#include "ct_BltHiQV.h"


#ifdef DEBUG
#define ENTER xf86Msg(X_ERROR, "%s\n", __func__)
#define LEAVE xf86Msg(X_ERROR, "%s done\n", __func__)
int last_op = 0, lx, ly, lw, lh, dx, dy, xdir, ydir, lsp, ldp;
#else
#define ENTER
#define LEAVE
#endif

static void
ctWaitMarker(ScreenPtr pScreen, int Marker)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    ENTER;
    ctBLTWAIT;
}

static int
ctMarkSync(ScreenPtr pScreenInfo)
{
    ENTER;
    return 0;
}

static Bool
ctPrepareCopy
(
    PixmapPtr pSrcPixmap,
    PixmapPtr pDstPixmap,
    int       xdir,
    int       ydir,
    int       alu,
    Pixel     planemask
)
{
    ScrnInfoPtr pScrn = xf86Screens[pDstPixmap->drawable.pScreen->myNum];
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);

    ENTER;
    ctBLTWAIT;
    cAcl->srcpitch = exaGetPixmapPitch(pSrcPixmap);
    cAcl->srcoffset = exaGetPixmapOffset(pSrcPixmap);
    cAcl->xdir = xdir;
    cAcl->ydir = ydir;
    ctSETROP(ChipsAluConv[alu & 0xF] | ((xdir < 0) ? ctRIGHT2LEFT : 0) |
    				       ((ydir < 0) ? ctBOTTOM2TOP : 0) |
    				       ctPATSOLID/* | ctPATMONO*/);
    ctSETMONOCTL(ctDWORDALIGN);
    ctSETPITCH(cAcl->srcpitch, exaGetPixmapPitch(pDstPixmap));
    LEAVE;
    return TRUE;
}

static void
ctCopy
(
    PixmapPtr pDstPixmap,
    int       srcX,
    int       srcY,
    int       dstX,
    int       dstY,
    int       w,
    int       h
)
{
    ScrnInfoPtr pScrn = xf86Screens[pDstPixmap->drawable.pScreen->myNum];
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);
    int src = cAcl->srcoffset;
    int dst = exaGetPixmapOffset(pDstPixmap);
    int dstpitch = exaGetPixmapPitch(pDstPixmap);

    if (cAcl->ydir < 0) {
	srcY += (h - 1);
	dstY += (h - 1);
    }

    if (cAcl->xdir < 0) {
	srcX += (w - 1);
	dstX += (w - 1);
    }

    src += srcX * cAcl->BytesPerPixel + srcY * cAcl->srcpitch;
    dst += dstX * cAcl->BytesPerPixel + dstY * dstpitch;
    ctBLTWAIT;
    ctSETSRCADDR(src);
    ctSETDSTADDR(dst);
    ctSETHEIGHTWIDTHGO(h, w * cAcl->BytesPerPixel);
    LEAVE;
#ifdef DEBUG
    last_op = 1;
    lx = srcX;
    ly = srcY;
    lw = w;
    lh = h;
    dx = dstX;
    dy = dstY;
    xdir = cAcl->xdir;
    ydir = cAcl->ydir;
    lsp = cAcl->srcpitch;
    ldp = dstpitch;
#endif
}

static void
ctDoneCopy(PixmapPtr pDstPixmap)
{
    ENTER;
    ScrnInfoPtr pScrn = xf86Screens[pDstPixmap->drawable.pScreen->myNum];
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    ctBLTWAIT;
    LEAVE;
}

static Bool
ctPrepareSolid(
    PixmapPtr pPixmap,
    int alu,
    Pixel planemask,
    Pixel fg)
{
    ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);
    int pitch = exaGetPixmapPitch(pPixmap);

    ENTER;
    ctBLTWAIT;
    ctSETPITCH(pitch, pitch);
    ctSETROP(ChipsAluConv2[alu & 0xF] | ctPATSOLID | ctPATMONO);
    ctSETMONOCTL(ctDWORDALIGN);
    ctSETBGCOLOR24(fg);
    LEAVE;
    return TRUE;
}

static void
ctSolid(
    PixmapPtr pPixmap,
    int x1,
    int y1,
    int x2,
    int y2)
{
    ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);
    int dst = exaGetPixmapOffset(pPixmap);

    ENTER;
    ctBLTWAIT;
    dst +=  x1 * cAcl->BytesPerPixel + y1 * exaGetPixmapPitch(pPixmap);
    ctSETDSTADDR(dst);
    ctSETSRCADDR(dst);
    ctSETHEIGHTWIDTHGO(y2 - y1, (x2 - x1) * cAcl->BytesPerPixel);
    LEAVE;
#ifdef DEBUG
    last_op = 2;
#endif
}

/*
 * Memcpy-based UTS.
 */
static Bool
ctUploadToScreen(PixmapPtr pDst, int x, int y, int w, int h,
    char *src, int src_pitch)
{
    ScrnInfoPtr pScrn = xf86Screens[pDst->drawable.pScreen->myNum];
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    unsigned char *dst = cPtr->FbBase + exaGetPixmapOffset(pDst);
    int    dst_pitch  = exaGetPixmapPitch(pDst);

    int bpp    = pDst->drawable.bitsPerPixel;
    int cpp    = (bpp + 7) / 8;
    int wBytes = w * cpp;

    ENTER;
    ctBLTWAIT;
    dst += (x * cpp) + (y * dst_pitch);

    while (h--) {
        memcpy(dst, src, wBytes);
        src += src_pitch;
        dst += dst_pitch;
    }
    LEAVE;
    return TRUE;
}

/*
 * Memcpy-based DFS.
 */
static Bool
ctDownloadFromScreen(PixmapPtr pSrc, int x, int y, int w, int h,
    char *dst, int dst_pitch)
{
    ScrnInfoPtr pScrn = xf86Screens[pSrc->drawable.pScreen->myNum];
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    unsigned char *src = cPtr->FbBase + exaGetPixmapOffset(pSrc);
    int    src_pitch  = exaGetPixmapPitch(pSrc);

    int bpp    = pSrc->drawable.bitsPerPixel;
    int cpp    = (bpp + 7) / 8;
    int wBytes = w * cpp;

    ENTER;
    ctBLTWAIT;
    src += (x * cpp) + (y * src_pitch);

    while (h--) {
        memcpy(dst, src, wBytes);
        src += src_pitch;
        dst += dst_pitch;
    }
    LEAVE;
    return TRUE;
}

Bool
CHIPSInitEXA(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);
    ExaDriverPtr pExa;

    pExa = exaDriverAlloc();
    if (!pExa)
        return FALSE;

    cPtr->pExa = pExa;

#if 0
    cPtr->writeXR(cPtr, 0x20, 0);	/* XXX blitter in 8bit mode */
#endif

    cAcl->BytesPerPixel = pScrn->bitsPerPixel >> 3;
    cAcl->BitsPerPixel = pScrn->bitsPerPixel;
    cAcl->planemask = -1;
    cAcl->bgColor = -1;
    cAcl->fgColor = -1;
    cAcl->FbOffset = 0;

    pExa->exa_major = EXA_VERSION_MAJOR;
    pExa->exa_minor = EXA_VERSION_MINOR;

    pExa->memoryBase = cPtr->FbBase;
    pExa->offScreenBase = cAcl->CacheStart;
    pExa->memorySize = cAcl->CacheEnd;

    /*
     * Contrary to the manual, the blitter needs 8 byte pitch alignment or it
     * will lock up. Probably shouldn't be surprised, there are hidden 64bit 
     * alignment requirements all over the place.
     */ 
    pExa->pixmapOffsetAlign = 8;
    pExa->pixmapPitchAlign = 8;

    pExa->flags = EXA_OFFSCREEN_PIXMAPS;

    /* entirely bogus since the chip doesn't use coordinates */
    pExa->maxX = 2048;
    pExa->maxY = 2048;

    pExa->MarkSync = ctMarkSync;
    pExa->WaitMarker = ctWaitMarker;

    pExa->PrepareSolid = ctPrepareSolid;
    pExa->Solid = ctSolid;
    pExa->DoneSolid = ctDoneCopy;

    pExa->PrepareCopy = ctPrepareCopy;
    pExa->Copy = ctCopy;
    pExa->DoneCopy = ctDoneCopy;

    /* EXA hits more optimized paths when it does not have to fallback because
     * of missing UTS/DFS, hook memcpy-based UTS/DFS.
     */
    pExa->UploadToScreen = ctUploadToScreen;
    pExa->DownloadFromScreen = ctDownloadFromScreen;

    return exaDriverInit(pScreen, pExa);
}
