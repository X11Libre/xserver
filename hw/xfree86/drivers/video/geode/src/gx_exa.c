/* Copyright (c) 2003-2005 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Neither the name of the Advanced Micro Devices, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 * */

/*
 * File Contents:   This file is consists of main Xfree acceleration supported
 *                  routines like solid fill used here.
 *
 * Project:         Geode Xfree Frame buffer device driver.
 * */

/* #undef OPT_ACCEL */

#include <xorg-config.h>

#include "vgaHW.h"
#include "xf86.h"
#include "xf86fbman.h"
#include "miline.h"
#include "xaarop.h"
#include "servermd.h"
#include "picture.h"
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Pci.h"
#include "geode.h"
#include "gfx_defs.h"
#include "gfx_regs.h"

/* Common macros for blend operations are here */

#include "geode_blend.h"

#undef ulong
typedef unsigned long ulong;

#undef uint
typedef unsigned int uint;

#undef ushort
typedef unsigned short ushort;

#undef uchar
typedef unsigned char uchar;

#define CALC_FBOFFSET(x, y) \
	        (((ulong)(y) * gu2_pitch + ((ulong)(x) << gu2_xshift)))

#define FBADDR(x,y)				\
		((unsigned char *)pGeode->FBBase + CALC_FBOFFSET(x, y))

#define OS_UDELAY 0
#if OS_UDELAY > 0
#define OS_USLEEP(usec) usleep(usec);
#else
#define OS_USLEEP(usec)
#endif

#ifdef OPT_ACCEL
static unsigned int BPP;
static unsigned int BLT_MODE;
static unsigned int ACCEL_STRIDE;

#define GU2_WAIT_PENDING while(READ_GP32(MGP_BLT_STATUS) & MGP_BS_BLT_PENDING)
#define GU2_WAIT_BUSY    while(READ_GP32(MGP_BLT_STATUS) & MGP_BS_BLT_BUSY)
#endif

#define HOOK(fn) localRecPtr->fn = GX##fn

#define DLOG(l, fmt, args...) ErrorF(fmt, ##args)

/* static storage declarations */

typedef struct sGBltBox {
    ulong x, y;
    ulong w, h;
    ulong color;
    int bpp, transparent;
} GBltBox;

static unsigned int gu2_xshift, gu2_yshift;
static unsigned int gu2_pitch;

/* pat  0xF0 */
/* src  0xCC */
/* dst  0xAA */

/* (src FUNC dst) */

static const int SDfn[16] = {
    0x00, 0x88, 0x44, 0xCC, 0x22, 0xAA, 0x66, 0xEE,
    0x11, 0x99, 0x55, 0xDD, 0x33, 0xBB, 0x77, 0xFF
};

/* ((src FUNC dst) AND pat-mask) OR (dst AND (NOT pat-mask)) */

static const int SDfn_PM[16] = {
    0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,
    0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA
};

#ifdef OPT_ACCEL
static inline CARD32
amd_gx_BppToRasterMode(int bpp)
{
    switch (bpp) {
    case 16:
        return MGP_RM_BPPFMT_565;
    case 32:
        return MGP_RM_BPPFMT_8888;
    case 8:
        return MGP_RM_BPPFMT_332;
    default:
        return 0;
    }
}
#endif                          /* OPT_ACCEL */

/*----------------------------------------------------------------------------
 * GXAccelSync.
 *
 * Description  :This function is called to synchronize with the graphics
 *               engine and it waits the graphic engine is idle.  This is
 *               required before allowing direct access to the framebuffer.
 *
 *    Arg        Type     Comment
 *  pScrni   ScrnInfoPtr  pointer to Screeen info
 *
 * Returns              :none
 *---------------------------------------------------------------------------*/
void
GXAccelSync(ScrnInfoPtr pScrni)
{
    //ErrorF("GXAccelSync()\n");
#ifndef OPT_ACCEL
    gfx_wait_until_idle();
#else
    GU2_WAIT_BUSY;
#endif
}

#define VM_MAJOR_DEC 0
#define VM_MINOR_DEC 0

#if XF86EXA

static void
amd_gx_exa_WaitMarker(ScreenPtr pScreen, int Marker)
{
    GU2_WAIT_BUSY;
}

static void
amd_gx_exa_Done(PixmapPtr p)
{
}

static Bool
amd_gx_exa_UploadToScreen(PixmapPtr pDst, int x, int y, int w, int h,
                          char *src, int src_pitch)
{
    GeodeRec *pGeode = GEODEPTR_FROM_PIXMAP(pDst);
    unsigned char *dst = pGeode->pExa->memoryBase + exaGetPixmapOffset(pDst);
    int dst_pitch = exaGetPixmapPitch(pDst);
    int bpp = pDst->drawable.bitsPerPixel;

    dst += y * dst_pitch + x * (bpp >> 3);
    GU2_WAIT_BUSY;
    geode_memory_to_screen_blt((unsigned long) src, (unsigned long) dst,
                               src_pitch, dst_pitch, w, h, bpp);
    return TRUE;
}

static Bool
amd_gx_exa_DownloadFromScreen(PixmapPtr pSrc, int x, int y, int w, int h,
                              char *dst, int dst_pitch)
{
    GeodeRec *pGeode = GEODEPTR_FROM_PIXMAP(pSrc);
    unsigned char *src = pGeode->pExa->memoryBase + exaGetPixmapOffset(pSrc);
    int src_pitch = exaGetPixmapPitch(pSrc);
    int bpp = pSrc->drawable.bitsPerPixel;

    src += (y * src_pitch) + (x * (bpp >> 3));
    GU2_WAIT_BUSY;
    geode_memory_to_screen_blt((unsigned long) src, (unsigned long) dst,
                               src_pitch, dst_pitch, w, h, bpp);
    return TRUE;
}

/* Solid */

static Bool
amd_gx_exa_PrepareSolid(PixmapPtr pxMap, int alu, Pixel planemask, Pixel fg)
{
    int dstPitch = exaGetPixmapPitch(pxMap);
    unsigned int ROP = amd_gx_BppToRasterMode(pxMap->drawable.bitsPerPixel)
        | (planemask == ~0U ? SDfn[alu] : SDfn_PM[alu]);

    //  FIXME: this should go away -- workaround for the blockparty icon corruption
    //if (pxMap->drawable.bitsPerPixel == 32)
    //  return FALSE;

    BLT_MODE = ((ROP ^ (ROP >> 2)) & 0x33) == 0 ? MGP_BM_SRC_MONO : 0;
    if (((ROP ^ (ROP >> 1)) & 0x55) != 0)
        BLT_MODE |= MGP_BM_DST_REQ;
    //ErrorF("amd_gx_exa_PrepareSolid(%#x,%#x,%#x - ROP=%x,BLT_MODE=%x)\n", alu, planemask, fg, ROP, BLT_MODE);
    GU2_WAIT_PENDING;
    WRITE_GP32(MGP_RASTER_MODE, ROP);
    WRITE_GP32(MGP_PAT_COLOR_0, planemask);
    WRITE_GP32(MGP_SRC_COLOR_FG, fg);
    WRITE_GP32(MGP_STRIDE, dstPitch);
    return TRUE;
}

static void
amd_gx_exa_Solid(PixmapPtr pxMap, int x1, int y1, int x2, int y2)
{
    int bpp = (pxMap->drawable.bitsPerPixel + 7) / 8;
    int pitch = exaGetPixmapPitch(pxMap);
    unsigned int offset = exaGetPixmapOffset(pxMap) + pitch * y1 + bpp * x1;
    unsigned int size = ((x2 - x1) << 16) | (y2 - y1);

    //ErrorF("amd_gx_exa_Solid() at %d,%d %d,%d - offset=%d, bpp=%d\n", x1, y1, x2, y2, offset, bpp);

    GU2_WAIT_PENDING;
    WRITE_GP32(MGP_DST_OFFSET, offset);
    WRITE_GP32(MGP_WID_HEIGHT, size);
    WRITE_GP32(MGP_BLT_MODE, BLT_MODE);
}

/* Copy */

static Bool
amd_gx_exa_PrepareCopy(PixmapPtr pxSrc, PixmapPtr pxDst, int dx, int dy,
                       int alu, Pixel planemask)
{
    GeodeRec *pGeode = GEODEPTR_FROM_PIXMAP(pxDst);
    int dstPitch = exaGetPixmapPitch(pxDst);
    unsigned int ROP;

    /* Punt if the color formats aren't the same */

    if (pxSrc->drawable.bitsPerPixel != pxDst->drawable.bitsPerPixel)
        return FALSE;

    //ErrorF("amd_gx_exa_PrepareCopy() dx%d dy%d alu %#x %#x\n",
    //  dx, dy, alu, planemask);

    pGeode->cpySrcOffset = exaGetPixmapOffset(pxSrc);
    pGeode->cpySrcPitch = exaGetPixmapPitch(pxSrc);
    pGeode->cpySrcBpp = (pxSrc->drawable.bitsPerPixel + 7) / 8;
    pGeode->cpyDx = dx;
    pGeode->cpyDy = dy;
    ROP = amd_gx_BppToRasterMode(pxSrc->drawable.bitsPerPixel) |
        (planemask == ~0U ? SDfn[alu] : SDfn_PM[alu]);

    BLT_MODE = ((ROP ^ (ROP >> 1)) & 0x55) != 0 ?
        MGP_BM_SRC_FB | MGP_BM_DST_REQ : MGP_BM_SRC_FB;
    GU2_WAIT_PENDING;
    WRITE_GP32(MGP_RASTER_MODE, ROP);
    WRITE_GP32(MGP_PAT_COLOR_0, planemask);
    WRITE_GP32(MGP_SRC_COLOR_FG, ~0);
    WRITE_GP32(MGP_SRC_COLOR_BG, ~0);
    WRITE_GP32(MGP_STRIDE, (pGeode->cpySrcPitch << 16) | dstPitch);
    return TRUE;
}

static void
amd_gx_exa_Copy(PixmapPtr pxDst, int srcX, int srcY, int dstX, int dstY,
                int w, int h)
{
    GeodeRec *pGeode = GEODEPTR_FROM_PIXMAP(pxDst);
    int dstBpp = (pxDst->drawable.bitsPerPixel + 7) / 8;
    int dstPitch = exaGetPixmapPitch(pxDst);
    unsigned int srcOffset =
        pGeode->cpySrcOffset + (pGeode->cpySrcPitch * srcY) +
        (pGeode->cpySrcBpp * srcX);
    unsigned int dstOffset =
        exaGetPixmapOffset(pxDst) + (dstPitch * dstY) + (dstBpp * dstX);
    unsigned int size = (w << 16) | h;
    unsigned int blt_mode = BLT_MODE;

    //ErrorF("amd_gx_exa_Copy() from %d,%d to %d,%d %dx%d\n", srcX, srcY,
    //   dstX, dstY, w, h);

    if (pGeode->cpyDx < 0) {
        srcOffset += w * pGeode->cpySrcBpp - 1;
        dstOffset += w * dstBpp - 1;
        blt_mode |= MGP_BM_NEG_XDIR;
    }
    if (pGeode->cpyDy < 0) {
        srcOffset += (h - 1) * pGeode->cpySrcPitch;
        dstOffset += (h - 1) * dstPitch;
        blt_mode |= MGP_BM_NEG_YDIR;
    }
    GU2_WAIT_PENDING;
    WRITE_GP32(MGP_SRC_OFFSET, srcOffset);
    WRITE_GP32(MGP_DST_OFFSET, dstOffset);
    WRITE_GP32(MGP_WID_HEIGHT, size);
    WRITE_GP16(MGP_BLT_MODE, blt_mode);
}

/* A=SRC, B=DST */
#define SRC_DST 0
/* B=SRC, A=DST */
#define DST_SRC MGP_RM_DEST_FROM_CHAN_A
/* A*alpha + B*0         */
#define Aa_B0   MGP_RM_ALPHA_TIMES_A
/* A*0     + B*(1-alpha) */
#define A0_B1a  MGP_RM_BETA_TIMES_B
/* A*1     + B*(1-alpha) */
#define A1_B1a  MGP_RM_A_PLUS_BETA_B
/* A*alpha + B*(1-alpha) */
#define Aa_B1a  MGP_RM_ALPHA_A_PLUS_BETA_B
/* alpha from A */
#define a_A MGP_RM_SELECT_ALPHA_A
/* alpha from B */
#define a_B MGP_RM_SELECT_ALPHA_B
/* alpha from const */
#define a_C MGP_RM_SELECT_ALPHA_R
/* alpha = 1 */
#define a_1 MGP_RM_SELECT_ALPHA_1

#define MGP_RM_ALPHA_TO_ARGB (MGP_RM_ALPHA_TO_ALPHA | MGP_RM_ALPHA_TO_RGB)
#define gxPictOpMAX PictOpAdd   /* highest accelerated op */

unsigned int amd_gx_exa_alpha_ops[] =
/*    A   B      OP     AS           const = 0 */
{
    (SRC_DST | Aa_B0 | a_C), 0, /* clear    (src*0) */
    (SRC_DST | Aa_B0 | a_1), 0, /* src      (src*1) */
    (DST_SRC | Aa_B0 | a_1), 0, /* dst      (dst*1) */
    (SRC_DST | A1_B1a | a_A), 0,        /* src-over (src*1 + dst(1-A)) */
    (DST_SRC | A1_B1a | a_A), 0,        /* dst-over (dst*1 + src(1-B)) */
    (SRC_DST | Aa_B0 | a_B), 0, /* src-in   (src*B) */
    (DST_SRC | Aa_B0 | a_B), 0, /* dst-in   (dst*A) */
    (DST_SRC | A0_B1a | a_A), 0,        /* src-out  (src*(1-B)) */
    (SRC_DST | A0_B1a | a_A), 0,        /* dst-out  (dst*(1-A)) */
/* pass1 (SRC=dst DST=scr=src), pass2 (SRC=src, DST=dst) */
    (DST_SRC | Aa_B0 | a_B),    /* srcatop  (src*B) */
    (SRC_DST | A0_B1a | a_A),   /*                  + (dst(1-A)) */
    (SRC_DST | Aa_B0 | a_B),    /* dstatop  (dst*A) */
    (DST_SRC | A0_B1a | a_A),   /*                  + (src(1-B) */
    (SRC_DST | A0_B1a | a_A),   /* xor      (src*(1-B) */
    (SRC_DST | A0_B1a | a_A),   /*                  + (dst(1-A) */
    (SRC_DST | A1_B1a | a_C), 0,        /* add      (src*1 + dst*1) */
};

typedef struct {
    int exa_fmt;
    int bpp;
    int gx_fmt;
    int alpha_bits;
} amd_gx_exa_fmt_t;

amd_gx_exa_fmt_t amd_gx_exa_fmts[] = {
    {PICT_a8r8g8b8, 32, MGP_RM_BPPFMT_8888, 8},
    {PICT_x8r8g8b8, 32, MGP_RM_BPPFMT_8888, 0},
    {PICT_a4r4g4b4, 16, MGP_RM_BPPFMT_4444, 4},
    {PICT_a1r5g5b5, 16, MGP_RM_BPPFMT_1555, 1},
    {PICT_r5g6b5, 16, MGP_RM_BPPFMT_565, 0},
    {PICT_r3g3b2, 8, MGP_RM_BPPFMT_332, 0},
};

static amd_gx_exa_fmt_t *
amd_gx_exa_check_format(PicturePtr p)
{
    int i;
    int bpp = p->pDrawable ? p->pDrawable->bitsPerPixel : 0;
    amd_gx_exa_fmt_t *fp = &amd_gx_exa_fmts[0];

    for (i = sizeof(amd_gx_exa_fmts) / sizeof(amd_gx_exa_fmts[0]); --i >= 0;
         ++fp) {
        if (fp->bpp < bpp)
            return NULL;
        if (fp->bpp != bpp)
            continue;
        if (fp->exa_fmt == p->format)
            break;
    }
    return i < 0 ? NULL : fp;
}

/* Composite */

static Bool
amd_gx_exa_CheckComposite(int op, PicturePtr pSrc, PicturePtr pMsk,
                          PicturePtr pDst)
{
    GeodeRec *pGeode = GEODEPTR_FROM_PICTURE(pDst);

    if (op > gxPictOpMAX)
        return FALSE;
    if (pMsk)
        return FALSE;
    if (usesPasses(op) && pGeode->exaBfrSz == 0)
        return FALSE;
    if (pSrc->filter != PictFilterNearest &&
        pSrc->filter != PictFilterFast &&
        pSrc->filter != PictFilterGood && pSrc->filter != PictFilterBest)
        return FALSE;
    if (pSrc->repeat)
        return FALSE;
    if (pSrc->transform)
        return FALSE;
    return TRUE;
}

static Bool
amd_gx_exa_PrepareComposite(int op, PicturePtr pSrc, PicturePtr pMsk,
                            PicturePtr pDst, PixmapPtr pxSrc, PixmapPtr pxMsk,
                            PixmapPtr pxDst)
{
    int srcPitch;
    amd_gx_exa_fmt_t *sfp, *dfp;
    GeodeRec *pGeode = GEODEPTR_FROM_PIXMAP(pxDst);

    if (!pxSrc || !pSrc->pDrawable)
        return FALSE;

    //ErrorF("amd_gx_exa_PrepareComposite()\n");

    if ((sfp = amd_gx_exa_check_format(pSrc)) == NULL)
        return FALSE;
    if (sfp->alpha_bits == 0 && usesSrcAlpha(op))
        return FALSE;
    if ((dfp = amd_gx_exa_check_format(pDst)) == NULL)
        return FALSE;
    if (dfp->alpha_bits == 0 && usesDstAlpha(op))
        return FALSE;
    if (sfp->gx_fmt != dfp->gx_fmt)
        return FALSE;
    srcPitch = exaGetPixmapPitch(pxSrc);
    if (usesPasses(op) && srcPitch > pGeode->exaBfrSz)
        return FALSE;
    pGeode->cmpSrcPitch = srcPitch;
    pGeode->cmpOp = op;
    pGeode->cmpSrcOffset = exaGetPixmapOffset(pxSrc);
    pGeode->cmpSrcBpp = (pxSrc->drawable.bitsPerPixel + 7) / 8;
    pGeode->cmpSrcFmt = sfp->gx_fmt;
    pGeode->cmpDstFmt = dfp->gx_fmt | (dfp->alpha_bits == 0 ?
                                       MGP_RM_ALPHA_TO_RGB :
                                       MGP_RM_ALPHA_TO_ARGB);
    return TRUE;
}

static void
amd_gx_exa_Composite(PixmapPtr pxDst, int srcX, int srcY, int maskX,
                     int maskY, int dstX, int dstY, int width, int height)
{
    int op, current_line, max_lines, lines, pass, scratchPitch;
    unsigned int srcOffset, srcOfs = 0, srcPitch, srcPch = 0, srcBpp;
    unsigned int dstOffset, dstOfs = 0, dstPitch, dstPch = 0, dstBpp;
    unsigned int sizes, strides, blt_mode = 0, rop = 0;
    GeodeRec *pGeode = GEODEPTR_FROM_PIXMAP(pxDst);

    //ErrorF("amd_gx_exa_Composite() from %d,%d to %d,%d %dx%d\n",
    //    srcX, srcY, dstX, dstY, width, height);

    op = pGeode->cmpOp;
    if (usesPasses(op)) {
        int cacheLineSz = 32;
        int cachelines =
            (width * pGeode->cmpSrcBpp + cacheLineSz - 1) / cacheLineSz;
        scratchPitch = cachelines * cacheLineSz;
        if (scratchPitch > pGeode->cmpSrcPitch)
            scratchPitch = pGeode->cmpSrcPitch;
        max_lines = pGeode->exaBfrSz / scratchPitch;
    }
    else {
        scratchPitch = 0;
        max_lines = height;
    }

    dstBpp = (pxDst->drawable.bitsPerPixel + 7) / 8;
    dstPitch = exaGetPixmapPitch(pxDst);
    dstOffset = exaGetPixmapOffset(pxDst) + dstPitch * dstY + dstBpp * dstX;
    srcBpp = pGeode->cmpSrcBpp;
    srcPitch = pGeode->cmpSrcPitch;
    srcOffset = pGeode->cmpSrcOffset + srcPitch * srcY + srcBpp * srcX;

    current_line = pass = 0;
    while (current_line < height) {
        if (usesPasses(op)) {
            lines = height - current_line;
            if (lines > max_lines)
                lines = max_lines;
            switch (pass) {
            case 0:            /* copy src to scratch */
                srcPch = srcPitch;
                srcOfs = srcOffset + current_line * srcPch;
                dstPch = scratchPitch;
                dstOfs = pGeode->exaBfrOffset;
                rop = pGeode->cmpSrcFmt | MGP_RM_ALPHA_TO_ARGB;
                rop |= amd_gx_exa_alpha_ops[PictOpSrc * 2];
                blt_mode = usesChanB0(PictOpSrc) ?
                    MGP_BM_SRC_FB | MGP_BM_DST_REQ : MGP_BM_SRC_FB;
                ++pass;
                break;
            case 1:            /* pass1 */
                srcPch = dstPitch;
                srcOfs = dstOffset + current_line * srcPch;
                dstPch = scratchPitch;
                dstOfs = pGeode->exaBfrOffset;
                rop = pGeode->cmpSrcFmt | MGP_RM_ALPHA_TO_ARGB;
                rop |= amd_gx_exa_alpha_ops[op * 2];
                blt_mode = usesChanB1(op) ?
                    MGP_BM_SRC_FB | MGP_BM_DST_REQ : MGP_BM_SRC_FB;
                ++pass;
                break;
            case 2:            /* pass2 */
                srcPch = srcPitch;
                srcOfs = srcOffset + current_line * srcPch;
                dstPch = dstPitch;
                dstOfs = dstOffset + current_line * dstPch;
                rop = pGeode->cmpSrcFmt | MGP_RM_ALPHA_TO_ARGB;
                rop |= amd_gx_exa_alpha_ops[op * 2 + 1];
                blt_mode = usesChanB2(op) ?
                    MGP_BM_SRC_FB | MGP_BM_DST_REQ : MGP_BM_SRC_FB;
                ++pass;
                break;
            case 3:            /* add */
                srcPch = scratchPitch;
                srcOfs = pGeode->exaBfrOffset;
                dstPch = dstPitch;
                dstOfs = dstOffset + current_line * dstPch;
                rop = pGeode->cmpDstFmt;
                rop |= amd_gx_exa_alpha_ops[PictOpAdd * 2];
                blt_mode = usesChanB0(PictOpAdd) ?
                    MGP_BM_SRC_FB | MGP_BM_DST_REQ : MGP_BM_SRC_FB;
                current_line += lines;
                pass = 0;
                break;
            }
            strides = (srcPch << 16) | dstPch;
        }
        else {                  /* not multi pass */
            srcOfs = srcOffset;
            dstOfs = dstOffset;
            current_line = lines = height;
            strides = (srcPitch << 16) | dstPitch;
            rop = pGeode->cmpDstFmt | amd_gx_exa_alpha_ops[op * 2];
            blt_mode = usesChanB0(op) ?
                MGP_BM_SRC_FB | MGP_BM_DST_REQ : MGP_BM_SRC_FB;
        }
        sizes = (width << 16) | lines;
        if (srcOfs < dstOfs) {
            srcOfs += (lines - 1) * srcPitch + width * srcBpp - 1;
            dstOfs += (lines - 1) * dstPitch + width * dstBpp - 1;
            blt_mode |= MGP_BM_NEG_XDIR | MGP_BM_NEG_YDIR;
        }
        GU2_WAIT_PENDING;
        WRITE_GP32(MGP_RASTER_MODE, rop);
        WRITE_GP32(MGP_SRC_OFFSET, srcOfs);
        WRITE_GP32(MGP_DST_OFFSET, dstOfs);
        WRITE_GP32(MGP_WID_HEIGHT, sizes);
        WRITE_GP32(MGP_STRIDE, strides);
        WRITE_GP16(MGP_BLT_MODE, blt_mode);
    }
}
#endif                          /* #if XF86EXA */

/*----------------------------------------------------------------------------
 * GXAccelInit.
 *
 * Description:	This function sets up the supported acceleration routines and
 *              appropriate flags.
 *
 * Parameters:
 *      pScrn:	Screeen pointer structure.
 *
 * Returns:		TRUE on success and FALSE on Failure
 *
 * Comments:	This function is called in GXScreenInit in
 *              geode_driver.c to set  * the acceleration.
 *----------------------------------------------------------------------------
 */
Bool
GXAccelInit(ScreenPtr pScrn)
{
    ScrnInfoPtr pScrni = xf86ScreenToScrn(pScrn);
    GeodeRec *pGeode = GEODEPTR(pScrni);

#if XF86EXA
    ExaDriverPtr pExa = pGeode->pExa;
#endif

    gu2_xshift = pScrni->bitsPerPixel >> 4;

    /* XXX - fixme - this will change - we'll need to update it */

    gu2_pitch = pGeode->Pitch;

    switch (pGeode->Pitch) {
    case 1024:
        gu2_yshift = 10;
        break;
    case 2048:
        gu2_yshift = 11;
        break;
    case 4096:
        gu2_yshift = 12;
        break;
    default:
        gu2_yshift = 13;
        break;
    }

#ifdef OPT_ACCEL
    ACCEL_STRIDE = (pGeode->Pitch << 16) | pGeode->Pitch;
    BPP = amd_gx_BppToRasterMode(pScrni->bitsPerPixel);
#endif

#if XF86EXA
    if (pExa && pGeode->useEXA) {
        pExa->exa_major = EXA_VERSION_MAJOR;
        pExa->exa_minor = EXA_VERSION_MINOR;

        /* Sync */
        pExa->WaitMarker = amd_gx_exa_WaitMarker;
        /* UploadToScreen */
        pExa->UploadToScreen = amd_gx_exa_UploadToScreen;
        pExa->DownloadFromScreen = amd_gx_exa_DownloadFromScreen;

        /* Solid fill */
        pExa->PrepareSolid = amd_gx_exa_PrepareSolid;
        pExa->Solid = amd_gx_exa_Solid;
        pExa->DoneSolid = amd_gx_exa_Done;

        /* Copy */
        pExa->PrepareCopy = amd_gx_exa_PrepareCopy;
        pExa->Copy = amd_gx_exa_Copy;
        pExa->DoneCopy = amd_gx_exa_Done;

        /* Composite */
        pExa->CheckComposite = amd_gx_exa_CheckComposite;
        pExa->PrepareComposite = amd_gx_exa_PrepareComposite;
        pExa->Composite = amd_gx_exa_Composite;
        pExa->DoneComposite = amd_gx_exa_Done;

        return exaDriverInit(pScrn, pGeode->pExa);
    }
#endif

    return FALSE;
}
