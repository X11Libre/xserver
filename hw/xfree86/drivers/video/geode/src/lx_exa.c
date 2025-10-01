/*
 * Copyright (c) 2007-2008 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Neither the name of the Advanced Micro Devices, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 */

/* TODO:
   Support a8 as a source or destination?
   convert !a8 or !a4 masks?
   support multiple pass operations?
*/

/* To support PictOptAdd with a mask */
#include <xorg-config.h>

#include "xf86.h"
#include "exa.h"

#include "geode.h"
#include "cim_defs.h"
#include "cim_regs.h"

#include "geode_blend.h"

#define F(x)    IntToxFixed(x)
#define I(x)    xFixedToInt(x)

#define GEODE_TRACE_FALL 0

#if GEODE_TRACE_FALL
#define GEODE_FALLBACK(x)               \
do {                                    \
	ErrorF("%s: ", __FUNCTION__);   \
	ErrorF x;                       \
	return FALSE;                   \
} while (0)
#else
#define GEODE_FALLBACK(x) return FALSE
#endif

static const struct exa_format_t {
    int exa;
    int bpp;
    int fmt;
    int alphabits;
} lx_exa_formats[] = {
    {
     PICT_a8r8g8b8, 32, CIMGP_SOURCE_FMT_8_8_8_8, 8}, {
                                                       PICT_x8r8g8b8, 32,
                                                       CIMGP_SOURCE_FMT_8_8_8_8,
                                                       0}, {
                                                            PICT_x8b8g8r8, 32,
                                                            CIMGP_SOURCE_FMT_32BPP_BGR,
                                                            0}, {
                                                                 PICT_a4r4g4b4,
                                                                 16,
                                                                 CIMGP_SOURCE_FMT_4_4_4_4,
                                                                 4}, {
                                                                      PICT_a1r5g5b5,
                                                                      16,
                                                                      CIMGP_SOURCE_FMT_1_5_5_5,
                                                                      1}, {
                                                                           PICT_r5g6b5,
                                                                           16,
                                                                           CIMGP_SOURCE_FMT_0_5_6_5,
                                                                           0}, {
                                                                                PICT_b5g6r5,
                                                                                16,
                                                                                CIMGP_SOURCE_FMT_16BPP_BGR,
                                                                                0},
    {
     PICT_x1r5g5b5, 16, CIMGP_SOURCE_FMT_1_5_5_5, 0}, {
                                                       PICT_x1b5g5r5, 16,
                                                       CIMGP_SOURCE_FMT_15BPP_BGR,
                                                       0}, {
                                                            PICT_r3g3b2, 8,
                                                            CIMGP_SOURCE_FMT_3_3_2,
                                                            0}, {
                                                                 PICT_a8, 32,
                                                                 CIMGP_SOURCE_FMT_8_8_8_8,
                                                                 8}
};

/* This is a chunk of memory we use for scratch space */

#define COMP_TYPE_MASK 0
#define COMP_TYPE_ONEPASS 1
#define COMP_TYPE_TWOPASS 3
#define COMP_TYPE_ROTATE  5

static struct {
    int type;

    unsigned int srcOffset;
    unsigned int srcPitch;
    unsigned int srcBpp;
    unsigned int srcWidth, srcHeight;

    unsigned int srcColor;
    int op;
    int repeat;
    int maskrepeat;
    unsigned int fourBpp;
    unsigned int bufferOffset;
    struct exa_format_t *srcFormat;
    struct exa_format_t *dstFormat;

    int rotate;
    PictTransform *transform;

} exaScratch;

static const int SDfn[16] = {
    0x00, 0x88, 0x44, 0xCC, 0x22, 0xAA, 0x66, 0xEE,
    0x11, 0x99, 0x55, 0xDD, 0x33, 0xBB, 0x77, 0xFF
};

static const int SDfn_PM[16] = {
    0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,
    0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA
};

/* These functions check to see if we can safely prefetch the memory
 * for the blt, or if we have to wait the previous blt to complete.
 * One function is for the fill, and the other is for the copy because
 * they have different requirements based on ROP
 */

static int lx0 = -1, ly0 = -1, lx1 = -1, ly1 = -1;

static int
lx_fill_flags(int x0, int y0, int w, int h, int rop)
{
    int x1 = x0 + w, y1 = y0 + h;
    int n = ((rop ^ (rop >> 1)) & 0x55) == 0 || /* no dst */
        x0 >= lx1 || y0 >= ly1 ||       /* rght/below */
        x1 <= lx0 || y1 <= ly0 ?        /* left/above */
        0 : CIMGP_BLTFLAGS_HAZARD;

    lx0 = x0;
    ly0 = y0;
    lx1 = x1;
    ly1 = y1;

    return n;
}

static int
lx_copy_flags(int x0, int y0, int x1, int y1, int w, int h, int rop)
{
    int x2 = x1 + w, y2 = y1 + h;

    /* dst not hazzard and src not hazzard */
    int n = (((rop ^ (rop >> 1)) & 0x55) == 0 ||
             x1 >= lx1 || y1 >= ly1 ||
             x2 <= lx0 || y2 <= ly0) &&
        (((rop ^ (rop >> 2)) & 0x33) == 0 ||
         x0 >= lx1 || y0 >= ly1 ||
         x0 + w <= lx0 || y0 + h <= ly0) ? 0 : CIMGP_BLTFLAGS_HAZARD;

    lx0 = x1;
    ly0 = y1;
    lx1 = x2;
    ly1 = y2;

    return n;
}

/* These are borrowed from the exa engine - they should be made global
   and available to drivers, but until then....
*/

/* exaGetPixelFromRGBA (exa_render.c) */

static Bool
_GetPixelFromRGBA(CARD32 *pixel,
                  CARD16 red, CARD16 green, CARD16 blue, CARD16 alpha,
                  CARD32 format)
{
    int rbits, bbits, gbits, abits;
    int rshift, bshift, gshift, ashift;

    *pixel = 0;

    if (!PICT_FORMAT_COLOR(format))
        return FALSE;

    rbits = PICT_FORMAT_R(format);
    gbits = PICT_FORMAT_G(format);
    bbits = PICT_FORMAT_B(format);
    abits = PICT_FORMAT_A(format);

    if (PICT_FORMAT_TYPE(format) == PICT_TYPE_ARGB) {
        bshift = 0;
        gshift = bbits;
        rshift = gshift + gbits;
        ashift = rshift + rbits;
    }
    else {                      /* PICT_TYPE_ABGR */
        rshift = 0;
        gshift = rbits;
        bshift = gshift + gbits;
        ashift = bshift + bbits;
    }

    *pixel |= (blue >> (16 - bbits)) << bshift;
    *pixel |= (red >> (16 - rbits)) << rshift;
    *pixel |= (green >> (16 - gbits)) << gshift;
    *pixel |= (alpha >> (16 - abits)) << ashift;

    return TRUE;
}

/* exaGetRGBAFromPixel (exa_render.c) */

static Bool
_GetRGBAFromPixel(CARD32 pixel,
                  CARD16 *red,
                  CARD16 *green, CARD16 *blue, CARD16 *alpha, CARD32 format)
{
    int rbits, bbits, gbits, abits;
    int rshift, bshift, gshift, ashift;

    if (!PICT_FORMAT_COLOR(format))
        return FALSE;

    rbits = PICT_FORMAT_R(format);
    gbits = PICT_FORMAT_G(format);
    bbits = PICT_FORMAT_B(format);
    abits = PICT_FORMAT_A(format);

    if (PICT_FORMAT_TYPE(format) == PICT_TYPE_ARGB) {
        bshift = 0;
        gshift = bbits;
        rshift = gshift + gbits;
        ashift = rshift + rbits;
    }
    else {                      /* PICT_TYPE_ABGR */
        rshift = 0;
        gshift = rbits;
        bshift = gshift + gbits;
        ashift = bshift + bbits;
    }

    *red = ((pixel >> rshift) & ((1 << rbits) - 1)) << (16 - rbits);
    while (rbits < 16) {
        *red |= *red >> rbits;
        rbits <<= 1;
    }

    *green = ((pixel >> gshift) & ((1 << gbits) - 1)) << (16 - gbits);
    while (gbits < 16) {
        *green |= *green >> gbits;
        gbits <<= 1;
    }

    *blue = ((pixel >> bshift) & ((1 << bbits) - 1)) << (16 - bbits);
    while (bbits < 16) {
        *blue |= *blue >> bbits;
        bbits <<= 1;
    }

    if (abits) {
        *alpha = ((pixel >> ashift) & ((1 << abits) - 1)) << (16 - abits);
        while (abits < 16) {
            *alpha |= *alpha >> abits;
            abits <<= 1;
        }
    }
    else
        *alpha = 0xffff;

    return TRUE;
}

static unsigned int
lx_get_source_color(PixmapPtr pSrc, int srcFormat, int dstFormat)
{
    CARD32 in, out;
    CARD16 red = 0, green = 0, blue = 0, alpha = 0;

    /* Stall to avoid a race with the upload function */
    /* for 1.4 and newer, the problem will be resolved within
     * exaGetPixmapFirstPixel, so this should be adjusted so
     * the stall isn't run needlessly
     */
    /* FIXME: xserver-1.4 with a supposed fix for this is really old, so kill the stall? */

    gp_wait_until_idle();
    in = exaGetPixmapFirstPixel(pSrc);

    _GetRGBAFromPixel(in, &red, &blue, &green, &alpha, srcFormat);
    _GetPixelFromRGBA(&out, red, blue, green, alpha, dstFormat);

    return out;
}

static Bool
lx_prepare_solid(PixmapPtr pxMap, int alu, Pixel planemask, Pixel fg)
{
    int pitch = exaGetPixmapPitch(pxMap);
    int op = (planemask == ~0U) ? SDfn[alu] : SDfn_PM[alu];

    gp_declare_blt(0);
    gp_set_bpp(pxMap->drawable.bitsPerPixel);

    gp_set_raster_operation(op);

    if (planemask != ~0U)
        gp_set_solid_pattern(planemask);

    exaScratch.op = op;

    gp_set_solid_source(fg);

    gp_set_strides(pitch, pitch);
    gp_write_parameters();
    return TRUE;
}

static void
lx_do_solid(PixmapPtr pxMap, int x1, int y1, int x2, int y2)
{
    int bpp = (pxMap->drawable.bitsPerPixel + 7) / 8;
    int pitch = exaGetPixmapPitch(pxMap);
    unsigned int offset = exaGetPixmapOffset(pxMap) + (pitch * y1) + (bpp * x1);

    gp_declare_blt(lx_fill_flags(x1, y1, x2 - x1, y2 - y1, exaScratch.op));
    gp_pattern_fill(offset, x2 - x1, y2 - y1);
}

static Bool
lx_prepare_copy(PixmapPtr pxSrc, PixmapPtr pxDst, int dx, int dy,
                int alu, Pixel planemask)
{
    int dpitch = exaGetPixmapPitch(pxDst);
    int op = (planemask == ~0U) ? SDfn[alu] : SDfn_PM[alu];

    gp_declare_blt(0);
    gp_set_bpp(pxDst->drawable.bitsPerPixel);

    gp_set_raster_operation(op);

    if (planemask != ~0U)
        gp_set_solid_pattern(planemask);

    exaScratch.srcOffset = exaGetPixmapOffset(pxSrc);
    exaScratch.srcPitch = exaGetPixmapPitch(pxSrc);
    exaScratch.srcBpp = (pxSrc->drawable.bitsPerPixel + 7) / 8;

    exaScratch.op = op;

    gp_set_strides(dpitch, exaScratch.srcPitch);
    gp_write_parameters();
    return TRUE;
}

static void
lx_do_copy(PixmapPtr pxDst, int srcX, int srcY,
           int dstX, int dstY, int w, int h)
{
    int dstBpp = (pxDst->drawable.bitsPerPixel + 7) / 8;
    int dstPitch = exaGetPixmapPitch(pxDst);
    unsigned int srcOffset, dstOffset;
    int flags = 0;

    gp_declare_blt(lx_copy_flags(srcX, srcY, dstX, dstY, w, h, exaScratch.op));

    srcOffset = exaScratch.srcOffset + (exaScratch.srcPitch * srcY) +
        (exaScratch.srcBpp) * srcX;

    dstOffset = exaGetPixmapOffset(pxDst) + (dstPitch * dstY) + (dstBpp * dstX);

    if (dstX > srcX)
        flags |= CIMGP_NEGXDIR;

    if (dstY > srcY)
        flags |= CIMGP_NEGYDIR;

    gp_screen_to_screen_blt(dstOffset, srcOffset, w, h, flags);
}

/* Composite operations

These are the simplest - one pass operations - if there is no format or
mask, the we can make these happen pretty fast

                       Operation  Type  Channel   Alpha
PictOpClear            0          2     0         3
PictOpSrc              0          3     0         3
PictOpDst              0          3     1         3
PictOpOver             2          0     0         3
PictOpOverReverse      2          0     1         3
PictOpIn               0          1     0         3
PictOpInReverse        0          1     1         3
PictOpOut              1          0     0         3
PictOpOutReverse       1          0     1         3
PictOpAdd              2          2     0         3

The following require multiple passes
PictOpAtop
PictOpXor
*/

struct blend_ops_t {
    int operation;
    int type;
    int channel;
} lx_alpha_ops[] = {
    /* PictOpClear */
    {
     CIMGP_ALPHA_TIMES_A, CIMGP_CONSTANT_ALPHA, CIMGP_CHANNEL_A_SOURCE}, {
                                                                          },
    /* PictOpSrc */
    {
     CIMGP_ALPHA_TIMES_A, CIMGP_ALPHA_EQUALS_ONE, CIMGP_CHANNEL_A_SOURCE}, {
                                                                            },
    /* PictOpDst */
    {
     CIMGP_ALPHA_TIMES_A, CIMGP_ALPHA_EQUALS_ONE, CIMGP_CHANNEL_A_DEST}, {
                                                                          },
    /* PictOpOver */
    {
     CIMGP_A_PLUS_BETA_B, CIMGP_CHANNEL_A_ALPHA, CIMGP_CHANNEL_A_SOURCE}, {
                                                                           CIMGP_ALPHA_TIMES_A,
                                                                           CIMGP_CONVERTED_ALPHA,
                                                                           CIMGP_CHANNEL_A_SOURCE},
    /* PictOpOverReverse */
    {
     CIMGP_A_PLUS_BETA_B, CIMGP_CHANNEL_A_ALPHA, CIMGP_CHANNEL_A_DEST}, {
                                                                         CIMGP_ALPHA_TIMES_A,
                                                                         CIMGP_CONVERTED_ALPHA,
                                                                         CIMGP_CHANNEL_A_SOURCE},
    /* PictOpIn */
    {
     CIMGP_ALPHA_TIMES_A, CIMGP_CHANNEL_B_ALPHA, CIMGP_CHANNEL_A_SOURCE}, {
                                                                           CIMGP_ALPHA_TIMES_A,
                                                                           CIMGP_CONVERTED_ALPHA,
                                                                           CIMGP_CHANNEL_A_SOURCE},
    /* PictOpInReverse */
    {
     CIMGP_ALPHA_TIMES_A, CIMGP_CHANNEL_B_ALPHA, CIMGP_CHANNEL_A_DEST}, {
                                                                         CIMGP_ALPHA_TIMES_A,
                                                                         CIMGP_CONVERTED_ALPHA,
                                                                         CIMGP_CHANNEL_A_SOURCE},
    /* PictOpOut */
    {
     CIMGP_BETA_TIMES_B, CIMGP_CHANNEL_A_ALPHA, CIMGP_CHANNEL_A_DEST}, {
                                                                        CIMGP_ALPHA_TIMES_A,
                                                                        CIMGP_CONVERTED_ALPHA,
                                                                        CIMGP_CHANNEL_A_SOURCE},
    /* PictOpOutReverse */
    {
     CIMGP_BETA_TIMES_B, CIMGP_CHANNEL_A_ALPHA, CIMGP_CHANNEL_A_SOURCE}, {
                                                                          CIMGP_ALPHA_TIMES_A,
                                                                          CIMGP_CONVERTED_ALPHA,
                                                                          CIMGP_CHANNEL_A_SOURCE},
    /* SrcAtop */
    {
     CIMGP_ALPHA_TIMES_A, CIMGP_CHANNEL_B_ALPHA, CIMGP_CHANNEL_A_DEST}, {
                                                                         CIMGP_BETA_TIMES_B,
                                                                         CIMGP_CHANNEL_A_ALPHA,
                                                                         CIMGP_CHANNEL_A_SOURCE},
    /* SrcAtopReverse */
    {
     CIMGP_ALPHA_TIMES_A, CIMGP_CHANNEL_B_ALPHA, CIMGP_CHANNEL_A_SOURCE}, {
                                                                           CIMGP_BETA_TIMES_B,
                                                                           CIMGP_CHANNEL_A_ALPHA,
                                                                           CIMGP_CHANNEL_A_DEST},
    /* Xor */
    {
     CIMGP_BETA_TIMES_B, CIMGP_CHANNEL_A_ALPHA, CIMGP_CHANNEL_A_SOURCE}, {
                                                                          CIMGP_BETA_TIMES_B,
                                                                          CIMGP_CHANNEL_A_ALPHA,
                                                                          CIMGP_CHANNEL_A_SOURCE},
    /* PictOpAdd */
    {
     CIMGP_A_PLUS_BETA_B, CIMGP_CONSTANT_ALPHA, CIMGP_CHANNEL_A_SOURCE}, {
                                                                          }
};

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof((a)) / (sizeof(*(a))))
#endif

static const struct exa_format_t *
lx_get_format(PicturePtr p)
{
    int i;
    unsigned int format = p->format;

    for (i = 0; i < ARRAY_SIZE(lx_exa_formats); i++)
        if (lx_exa_formats[i].exa == format)
            return (&lx_exa_formats[i]);

    return NULL;
}

static Bool
lx_process_transform(PicturePtr pSrc)
{
    PictTransformPtr t = pSrc->transform;
    xFixed c0 = t->matrix[0][0];
    xFixed s0 = t->matrix[0][1];
    xFixed s1 = t->matrix[1][0];
    xFixed c1 = t->matrix[1][1];

    /* If the transform doesn't have any rotation
     * or scaling components, then just grab the
     * translate coordinates */

    if (t->matrix[0][0] == 0 &&
        t->matrix[0][1] == 0 && t->matrix[1][0] == 0 && t->matrix[1][1] == 0) {
        exaScratch.transform = pSrc->transform;
        return TRUE;
    }

    /* Otherwise, see if this is a simple
     * rotate transform - if it isn't, then
     * we have to punt back to software */

    if (t->matrix[2][2] != F(1))
        return FALSE;

    /* The rotate matrix looks like this:
     * [ cos X   -sin x 
     * sin X   cos X ]
     * 
     * Where X is the angle.  We do a simple
     * check first - if [0,0] != [1,1], then 
     * scaling was specified too, and we can 
     * bail, and if [0,1] != -[1,1] then this
     * isn't scaling that we can handle.
     */

    if ((c0 != c1) || (s0 != -s1))
        return FALSE;

    /* Now, figure out what angle we want - we
     * can only accelerate right angle rotations,
     * so this turns into an easy set of if statements */

    if (c0 == F(1) && s1 == F(0))
        exaScratch.rotate = RR_Rotate_0;
    else if (c0 == F(0) && s1 == F(1))
        exaScratch.rotate = RR_Rotate_90;
    else if (c0 == F(-1) && s1 == F(0))
        exaScratch.rotate = RR_Rotate_180;
    else if (c0 == F(0) && s1 == F(-1))
        exaScratch.rotate = RR_Rotate_270;
    else
        return FALSE;

    exaScratch.transform = pSrc->transform;

    return TRUE;
}

static Bool
lx_check_composite(int op, PicturePtr pSrc, PicturePtr pMsk, PicturePtr pDst)
{
    GeodeRec *pGeode = GEODEPTR_FROM_PICTURE(pDst);
    const struct exa_format_t *srcFmt, *dstFmt;

    if (op > PictOpAdd)
        GEODE_FALLBACK(("Operation %d is not supported\n", op));

    /* XXX - don't know if we can do any hwaccel on solid fills or gradient types in generic cases */
    if (pMsk && pMsk->pSourcePict)
        GEODE_FALLBACK(("%s are not supported as a mask\n",
                        pMsk->pSourcePict->type ==
                        SourcePictTypeSolidFill ? "Solid pictures" :
                        "Gradients"));

    if (pSrc->pSourcePict && pSrc->pSourcePict->type != SourcePictTypeSolidFill)
        GEODE_FALLBACK(("Gradients are not supported as the source\n"));

    if (pMsk && op == PictOpAdd)
        GEODE_FALLBACK(("PictOpAdd with mask is not supported\n"));

    /* FIXME: Meet this conditions from the debug for PictOpAdd.
     * Any Other possibilities? Add a judge for the future supplement */
    if (op == PictOpAdd && pSrc->format == PICT_a8r8g8b8 &&
        pDst->format == PICT_a8)
        return TRUE;

    if (op == PictOpAdd && pSrc->format == PICT_x8r8g8b8 &&
        pDst->format == PICT_a8)
        return TRUE;

    if (op == PictOpAdd && pSrc->format == PICT_r5g6b5 &&
        pDst->format == PICT_a8)
        return TRUE;

    if (usesPasses(op)) {
        /* FIXME: Slightly misleading fallback msg when !pMsk */
        if (pGeode->exaBfrOffset == 0 || !pMsk)
            GEODE_FALLBACK(("Multipass operation requires off-screen buffer\n"));
    }

    /* Check that the filter matches what we support */

    switch (pSrc->filter) {
    case PictFilterNearest:
    case PictFilterFast:
    case PictFilterGood:
    case PictFilterBest:
        break;

    default:
        GEODE_FALLBACK(("Bilinear or convolution filters are not supported\n"));
    }

    if (pMsk && pMsk->transform)
        GEODE_FALLBACK(("Mask transforms are not supported\n"));

    /* Keep an eye out for source rotation transforms - those we can
     * do something about */

    exaScratch.rotate = RR_Rotate_0;
    exaScratch.transform = NULL;

    if (pSrc->transform && !lx_process_transform(pSrc))
        GEODE_FALLBACK(("Transform operation is non-trivial\n"));

    /* XXX - I don't understand PICT_a8 enough - so I'm punting */
    if ((op != PictOpAdd) && (pSrc->format == PICT_a8 ||
                              pDst->format == PICT_a8))
        GEODE_FALLBACK(("PICT_a8 as src or dst format is unsupported\n"));

    if (pMsk && op != PictOpClear) {
        struct blend_ops_t *opPtr = &lx_alpha_ops[op * 2];
        int direction = (opPtr->channel == CIMGP_CHANNEL_A_SOURCE) ? 0 : 1;

        /* Direction 0 indicates src->dst, 1 indicates dst->src */
        if (((direction == 0) &&
             (pSrc->pDrawable && pSrc->pDrawable->bitsPerPixel < 16)) ||
            ((direction == 1) && (pDst->pDrawable->bitsPerPixel < 16))) {
            ErrorF("Mask blending unsupported with <16bpp\n");
            return FALSE;
        }
        if (pMsk->format != PICT_a8 && pMsk->format != PICT_a4)
            GEODE_FALLBACK(("Masks can be only done with a 8bpp or 4bpp depth\n"));

        /* The pSrc should be 1x1 pixel if the pMsk is not zero */
        if (pSrc->pDrawable &&
            (pSrc->pDrawable->width != 1 || pSrc->pDrawable->height != 1))
            GEODE_FALLBACK(("pSrc should be 1x1 pixel if pMsk is not zero\n"));
        /* FIXME: In lx_prepare_composite, there are no variables to record the
         * one pixel source's width and height when the mask is not zero.
         * That will lead to bigger region to render instead of one pixel in lx
         * _do_composite, so we should fallback currently to avoid this */
        /* Not an issue for solid pictures, because we'll treat it as 1x1R too */
        if (!pSrc->repeat &&
            !(pSrc->pSourcePict &&
              pSrc->pSourcePict->type == SourcePictTypeSolidFill)) {
            GEODE_FALLBACK(("FIXME: unzero mask might lead to bigger rendering region than 1x1 pixels\n"));
        }
    }
    else {
        if (pSrc->pSourcePict)
            GEODE_FALLBACK(("Solid source pictures without a mask are not supported\n"));
    }

    /* Get the formats for the source and destination */

    if ((srcFmt = lx_get_format(pSrc)) == NULL)
        GEODE_FALLBACK(("Unsupported source format %x\n", pSrc->format));

    if ((dstFmt = lx_get_format(pDst)) == NULL)
        GEODE_FALLBACK(("Unsupported destination format %x\n", pDst->format));

    /* Make sure operations that need alpha bits have them */
    /* If a mask is enabled, the alpha will come from there */

    if (!pMsk && (!srcFmt->alphabits && usesSrcAlpha(op)))
        GEODE_FALLBACK(("Operation requires src alpha, but alphabits is unset\n"));

    if (!pMsk && (!dstFmt->alphabits && usesDstAlpha(op)))
        GEODE_FALLBACK(("Operation requires dst alpha, but alphabits is unset\n"));

    /* FIXME: See a way around this! */
    if (srcFmt->alphabits == 0 && dstFmt->alphabits != 0)
        GEODE_FALLBACK(("src_alphabits=0, dst_alphabits!=0\n"));

    /* If this is a rotate operation, then make sure the src and dst
     * formats are the same */
    if (exaScratch.rotate != RR_Rotate_0 && srcFmt != dstFmt) {
        ErrorF("EXA: Unable to rotate and convert formats at the same time\n");
        return FALSE;
    }
    return TRUE;
}

static Bool
lx_prepare_composite(int op, PicturePtr pSrc, PicturePtr pMsk,
                     PicturePtr pDst, PixmapPtr pxSrc, PixmapPtr pxMsk,
                     PixmapPtr pxDst)
{
    GeodeRec *pGeode = GEODEPTR_FROM_PIXMAP(pxDst);
    const struct exa_format_t *srcFmt, *dstFmt;

    /* Get the formats for the source and destination */

    srcFmt = lx_get_format(pSrc);
    dstFmt = lx_get_format(pDst);

    /* Set up the scratch buffer with the information we need */

    exaScratch.srcFormat = (struct exa_format_t *) srcFmt;
    exaScratch.dstFormat = (struct exa_format_t *) dstFmt;
    exaScratch.op = op;
    exaScratch.repeat = pSrc->repeat;
    exaScratch.bufferOffset = pGeode->exaBfrOffset;

    if (pMsk && op != PictOpClear) {
        /* Get the source color */
        if (pSrc->pSourcePict) {
            exaScratch.srcColor = pSrc->pSourcePict->solidFill.color;
        }
        else {
            /* If the op is PictOpOver(or PictOpOutReverse, PictOpInReverse,
             * PictOpIn, PictOpOut, PictOpOverReverse), we should get the
             * ARGB32 source format */

            if ((op == PictOpOver || op == PictOpOutReverse || op ==
                 PictOpInReverse || op == PictOpIn || op == PictOpOut ||
                 op == PictOpOverReverse) && (srcFmt->alphabits != 0))
                exaScratch.srcColor = exaGetPixmapFirstPixel(pxSrc);
            else if ((op == PictOpOver || op == PictOpOutReverse || op ==
                      PictOpInReverse || op == PictOpIn || op == PictOpOut ||
                      op == PictOpOverReverse) && (srcFmt->alphabits == 0))
                exaScratch.srcColor = lx_get_source_color(pxSrc, pSrc->format,
                                                          PICT_a8r8g8b8);
            else
                exaScratch.srcColor = lx_get_source_color(pxSrc, pSrc->format,
                                                          pDst->format);
        }

        /* Save off the info we need (reuse the source values to save space) */
        exaScratch.type = COMP_TYPE_MASK;
        exaScratch.maskrepeat = pMsk->repeat;

        exaScratch.srcOffset = exaGetPixmapOffset(pxMsk);
        exaScratch.srcPitch = exaGetPixmapPitch(pxMsk);
        exaScratch.srcBpp = (pxMsk->drawable.bitsPerPixel + 7) / 8;

        exaScratch.srcWidth = pMsk->pDrawable->width;
        exaScratch.srcHeight = pMsk->pDrawable->height;

        /* Flag to indicate if this a 8BPP or a 4BPP mask */
        exaScratch.fourBpp = (pxMsk->drawable.bitsPerPixel == 4) ? 1 : 0;
    }
    else {
        if (usesPasses(op))
            exaScratch.type = COMP_TYPE_TWOPASS;
        else if (exaScratch.rotate != RR_Rotate_0)
            exaScratch.type = COMP_TYPE_ROTATE;
        else
            exaScratch.type = COMP_TYPE_ONEPASS;

        exaScratch.srcOffset = exaGetPixmapOffset(pxSrc);
        exaScratch.srcPitch = exaGetPixmapPitch(pxSrc);
        exaScratch.srcBpp = (pxSrc->drawable.bitsPerPixel + 7) / 8;

        exaScratch.srcWidth = pSrc->pDrawable->width;
        exaScratch.srcHeight = pSrc->pDrawable->height;
    }

    return TRUE;
}

static int
lx_get_bpp_from_format(int format)
{

    switch (format) {
    case CIMGP_SOURCE_FMT_8_8_8_8:
    case CIMGP_SOURCE_FMT_32BPP_BGR:
        return 32;

    case CIMGP_SOURCE_FMT_4_4_4_4:
        return 12;

    case CIMGP_SOURCE_FMT_0_5_6_5:
    case CIMGP_SOURCE_FMT_16BPP_BGR:
        return 16;

    case CIMGP_SOURCE_FMT_1_5_5_5:
    case CIMGP_SOURCE_FMT_15BPP_BGR:
        return 15;

    case CIMGP_SOURCE_FMT_3_3_2:
        return 8;
    }

    return 0;
}

/* BGR needs to be set in the source for it to take - so adjust the source
 * to enable BGR if the two formats are different, and disable it if they
 * are the same
 */

static void
lx_set_source_format(int srcFormat, int dstFormat)
{
    if (!(srcFormat & 0x10) && (dstFormat & 0x10))
        gp_set_source_format(srcFormat | 0x10);
    else if ((srcFormat & 0x10) && (dstFormat & 0x10))
        gp_set_source_format(srcFormat & ~0x10);
    else
        gp_set_source_format(srcFormat);
}

/* If we are converting colors and we need the channel A alpha,
 * then use a special alpha type that preserves the alpha before
 * converting the format
 */

static inline int
get_op_type(struct exa_format_t *src, struct exa_format_t *dst, int type)
{
    return (type == CIMGP_CHANNEL_A_ALPHA &&
            src->alphabits != dst->alphabits) ? CIMGP_CONVERTED_ALPHA : type;
}

/* Note - this is the preferred onepass method.  The other will remain
 * ifdefed out until such time that we are sure its not needed
 */

#define GetPixmapOffset(px, x, y) ( exaGetPixmapOffset((px)) + \
  (exaGetPixmapPitch((px)) * (y)) + \
  ((((px)->drawable.bitsPerPixel + 7) / 8) * (x)) )

#define GetSrcOffset(_x, _y) (exaScratch.srcOffset + ((_y) * exaScratch.srcPitch) + \
			      ((_x) * exaScratch.srcBpp))

static void
lx_composite_onepass_add_a8(PixmapPtr pxDst, unsigned long dstOffset,
                            unsigned long srcOffset, int width, int height,
                            int opX, int opY, int srcX, int srcY)
{
    struct blend_ops_t *opPtr;
    int apply, type;
    int optempX, optempY;
    int i, j;
    unsigned long pixmapOffset, pixmapPitch, calBitsPixel;

    pixmapOffset = exaGetPixmapOffset(pxDst);
    pixmapPitch = exaGetPixmapPitch(pxDst);
    calBitsPixel = (pxDst->drawable.bitsPerPixel + 7) / 8;

    /* Keep this GP idle judge here. Otherwise the SW method has chance to
     * conflict with the HW rendering method */
    gp_wait_until_idle();

    if (opX % 4 == 0 && srcX % 4 == 0) {
        /* HW acceleration */
        opPtr = &lx_alpha_ops[exaScratch.op * 2];
        apply = CIMGP_APPLY_BLEND_TO_ALL;
        gp_declare_blt(0);
        gp_set_bpp(32);
        gp_set_strides(exaGetPixmapPitch(pxDst), exaScratch.srcPitch);
        gp_set_source_format(8);
        type = opPtr->type;
        gp_set_alpha_operation(opPtr->operation, type, opPtr->channel, apply,
                               0);
        gp_screen_to_screen_convert(dstOffset, srcOffset, width / 4, height, 0);
        /* Calculate the pixels in the tail of each line */
        for (j = srcY; j < srcY + height; j++)
            for (i = srcX + (width / 4) * 4; i < srcX + width; i++) {
                srcOffset = GetSrcOffset(i, j);
                optempX = opX + i - srcX;
                optempY = opY + j - srcY;
                dstOffset = pixmapOffset + pixmapPitch * optempY +
                    calBitsPixel * optempX;
                *(cim_fb_ptr + dstOffset) = (*(cim_fb_ptr + srcOffset)
                                             + *(cim_fb_ptr + dstOffset) <=
                                             0xff) ? *(cim_fb_ptr + srcOffset) +
                    *(cim_fb_ptr + dstOffset) : 0xff;
            }
    }
    else {
        for (j = srcY; j < srcY + height; j++)
            for (i = srcX; i < srcX + width; i++) {
                srcOffset = GetSrcOffset(i, j);
                optempX = opX + i - srcX;
                optempY = opY + j - srcY;
                dstOffset = pixmapOffset + pixmapPitch * optempY +
                    calBitsPixel * optempX;
                *(cim_fb_ptr + dstOffset) = (*(cim_fb_ptr + srcOffset) +
                                             *(cim_fb_ptr + dstOffset) <=
                                             0xff) ? *(cim_fb_ptr + srcOffset) +
                    *(cim_fb_ptr + dstOffset) : 0xff;
            }
    }
}

static void
lx_composite_onepass(PixmapPtr pxDst, unsigned long dstOffset,
                     unsigned long srcOffset, int width, int height)
{
    struct blend_ops_t *opPtr;
    int apply, type;

    opPtr = &lx_alpha_ops[exaScratch.op * 2];

    apply = (exaScratch.dstFormat->alphabits != 0 &&
             exaScratch.srcFormat->alphabits != 0) ?
        CIMGP_APPLY_BLEND_TO_ALL : CIMGP_APPLY_BLEND_TO_RGB;

    gp_declare_blt(0);
    gp_set_bpp(lx_get_bpp_from_format(exaScratch.dstFormat->fmt));
    gp_set_strides(exaGetPixmapPitch(pxDst), exaScratch.srcPitch);

    lx_set_source_format(exaScratch.srcFormat->fmt, exaScratch.dstFormat->fmt);

    type = get_op_type(exaScratch.srcFormat, exaScratch.dstFormat, opPtr->type);

    gp_set_alpha_operation(opPtr->operation, type, opPtr->channel, apply, 0);

    gp_screen_to_screen_convert(dstOffset, srcOffset, width, height, 0);
}

static void
lx_composite_all_black(unsigned long srcOffset, int width, int height)
{
    struct blend_ops_t *opPtr;
    int apply, type;

    opPtr = &lx_alpha_ops[0];
    apply = (exaScratch.srcFormat->alphabits != 0) ?
        CIMGP_APPLY_BLEND_TO_ALL : CIMGP_APPLY_BLEND_TO_RGB;
    gp_declare_blt(0);
    gp_set_bpp(lx_get_bpp_from_format(exaScratch.srcFormat->fmt));
    gp_set_strides(exaScratch.srcPitch, exaScratch.srcPitch);
    lx_set_source_format(exaScratch.srcFormat->fmt, exaScratch.srcFormat->fmt);
    type = get_op_type(exaScratch.srcFormat, exaScratch.srcFormat, opPtr->type);
    gp_set_alpha_operation(opPtr->operation, type, opPtr->channel, apply, 0);
    gp_screen_to_screen_convert(srcOffset, srcOffset, width, height, 0);

}

static void
lx_composite_onepass_special(PixmapPtr pxDst, int width, int height, int opX,
                             int opY, int srcX, int srcY)
{
    struct blend_ops_t *opPtr;
    int apply, type;
    int opWidth, opHeight;
    int optempX, optempY;
    unsigned int dstOffset, srcOffset = 0;

    optempX = opX;
    optempY = opY;

    /* Make sure srcX and srcY are in source region */
    srcX = ((srcX % (int) exaScratch.srcWidth) + (int) exaScratch.srcWidth)
        % (int) exaScratch.srcWidth;
    srcY = ((srcY % (int) exaScratch.srcHeight) + (int) exaScratch.srcHeight)
        % (int) exaScratch.srcHeight;

    opWidth = exaScratch.srcWidth - srcX;
    opHeight = exaScratch.srcHeight - srcY;

    srcOffset = GetSrcOffset(srcX, srcY);

    if (width < opWidth)
        opWidth = width;
    if (height < opHeight)
        opHeight = height;

    while (1) {
        gp_wait_until_idle();
        dstOffset = GetPixmapOffset(pxDst, optempX, optempY);
        opPtr = &lx_alpha_ops[exaScratch.op * 2];
        apply = (exaScratch.dstFormat->alphabits != 0 &&
                 exaScratch.srcFormat->alphabits != 0) ?
            CIMGP_APPLY_BLEND_TO_ALL : CIMGP_APPLY_BLEND_TO_RGB;
        gp_declare_blt(0);
        gp_set_bpp(lx_get_bpp_from_format(exaScratch.dstFormat->fmt));
        gp_set_strides(exaGetPixmapPitch(pxDst), exaScratch.srcPitch);
        lx_set_source_format(exaScratch.srcFormat->fmt,
                             exaScratch.dstFormat->fmt);
        type = get_op_type(exaScratch.srcFormat, exaScratch.dstFormat,
                           opPtr->type);
        gp_set_alpha_operation(opPtr->operation, type, opPtr->channel,
                               apply, 0);
        gp_screen_to_screen_convert(dstOffset, srcOffset, opWidth, opHeight, 0);

        optempX += opWidth;
        if (optempX >= opX + width) {
            optempX = opX;
            optempY += opHeight;
            if (optempY >= opY + height)
                break;
        }
        if (optempX == opX) {
            srcOffset = GetSrcOffset(srcX, 0);
            opWidth = ((opX + width) - optempX) > (exaScratch.srcWidth - srcX)
                ? (exaScratch.srcWidth - srcX) : ((opX + width) - optempX);
            opHeight = ((opY + height) - optempY) > exaScratch.srcHeight
                ? exaScratch.srcHeight : ((opY + height) - optempY);
        }
        else if (optempY == opY) {
            srcOffset = GetSrcOffset(0, srcY);
            opWidth = ((opX + width) - optempX) > exaScratch.srcWidth
                ? exaScratch.srcWidth : ((opX + width) - optempX);
            opHeight = ((opY + height) - optempY) > (exaScratch.srcHeight -
                                                     srcY)
                ? (exaScratch.srcHeight - srcY) : ((opY + height)
                                                   - optempY);
        }
        else {
            srcOffset = GetSrcOffset(0, 0);
            opWidth = ((opX + width) - optempX) > exaScratch.srcWidth
                ? exaScratch.srcWidth : ((opX + width) - optempX);
            opHeight = ((opY + height) - optempY) > exaScratch.srcHeight
                ? exaScratch.srcHeight : ((opY + height) - optempY);
        }
    }
}

/* This function handles the multipass blend functions */

static void
lx_composite_multipass(PixmapPtr pxDst, unsigned long dstOffset,
                       unsigned long srcOffset, int width, int height)
{
    struct blend_ops_t *opPtr;
    int sbpp = lx_get_bpp_from_format(exaScratch.srcFormat->fmt);
    int apply, type;

    /* Wait until the GP is idle - this will ensure that the scratch buffer
     * isn't occupied */

    gp_wait_until_idle();

    /* Copy the destination to the scratch buffer, and convert it to the
     * source format */

    gp_declare_blt(0);

    gp_set_bpp(sbpp);
    gp_set_source_format(exaScratch.dstFormat->fmt);
    gp_set_raster_operation(0xCC);
    gp_set_strides(exaScratch.srcPitch, exaGetPixmapPitch(pxDst));
    gp_screen_to_screen_convert(exaScratch.bufferOffset, dstOffset,
                                width, height, 0);

    /* Do the first blend from the source to the scratch buffer */

    gp_declare_blt(CIMGP_BLTFLAGS_HAZARD);
    gp_set_bpp(sbpp);
    gp_set_source_format(exaScratch.srcFormat->fmt);
    gp_set_strides(exaScratch.srcPitch, exaScratch.srcPitch);

    opPtr = &lx_alpha_ops[exaScratch.op * 2];

    apply = (exaScratch.srcFormat->alphabits == 0) ?
        CIMGP_APPLY_BLEND_TO_RGB : CIMGP_APPLY_BLEND_TO_ALL;

    /* If we're destroying the source alpha bits, then make sure we
     * use the alpha before the color conversion
     */

    gp_screen_to_screen_blt(exaScratch.bufferOffset, srcOffset, width, height,
                            0);

    /* Finally, do the second blend back to the destination */

    opPtr = &lx_alpha_ops[(exaScratch.op * 2) + 1];

    apply = (exaScratch.dstFormat->alphabits == 0) ?
        CIMGP_APPLY_BLEND_TO_RGB : CIMGP_APPLY_BLEND_TO_ALL;

    gp_declare_blt(CIMGP_BLTFLAGS_HAZARD);
    gp_set_bpp(lx_get_bpp_from_format(exaScratch.dstFormat->fmt));

    lx_set_source_format(exaScratch.srcFormat->fmt, exaScratch.dstFormat->fmt);

    type = get_op_type(exaScratch.srcFormat, exaScratch.dstFormat, opPtr->type);

    gp_set_alpha_operation(opPtr->operation, type, opPtr->channel, apply, 0);

    gp_screen_to_screen_convert(dstOffset, exaScratch.bufferOffset,
                                width, height, 0);
}

static void
lx_composite_rotate(PixmapPtr pxDst, unsigned long dstOffset,
                    unsigned int srcOffset, int width, int height)
{
    int degrees = 0;

    gp_declare_blt(0);
    gp_set_bpp(lx_get_bpp_from_format(exaScratch.dstFormat->fmt));
    gp_set_strides(exaGetPixmapPitch(pxDst), exaScratch.srcPitch);

    lx_set_source_format(exaScratch.srcFormat->fmt, exaScratch.dstFormat->fmt);

    gp_set_raster_operation(0xCC);

    /* RandR rotation is counter-clockwise, our rotation
     * is clockwise, so adjust the numbers accordingly */

    switch (exaScratch.rotate) {
    case RR_Rotate_90:
        degrees = 270;
        break;
    case RR_Rotate_180:
        degrees = 180;
        break;
    case RR_Rotate_270:
        degrees = 90;
        break;
    }

    gp_rotate_blt(dstOffset, srcOffset, width, height, degrees);
}

static void
lx_do_composite_mask(PixmapPtr pxDst, unsigned long dstOffset,
                     unsigned int maskOffset, int width, int height)
{
    struct blend_ops_t *opPtr = &lx_alpha_ops[exaScratch.op * 2];

    gp_declare_blt(0);

    gp_set_source_format(exaScratch.srcFormat->fmt);
    gp_set_strides(exaGetPixmapPitch(pxDst), exaScratch.srcPitch);
    gp_set_bpp(lx_get_bpp_from_format(exaScratch.dstFormat->fmt));
    gp_set_solid_source(exaScratch.srcColor);

    gp_blend_mask_blt(dstOffset, 0, width, height, maskOffset,
                      exaScratch.srcPitch, opPtr->operation,
                      exaScratch.fourBpp);
}

static void
lx_do_composite_mask_two_pass(PixmapPtr pxDst, unsigned long dstOffset,
                              unsigned int maskOffset, int width, int height,
                              int opX, int opY, xPointFixed srcPoint)
{
    int apply, type;
    struct blend_ops_t *opPtr;
    int opWidth, opHeight;
    int opoverX, opoverY;

    opoverX = opX;
    opoverY = opY;

    /* The rendering region should not be bigger than off-screen memory size
     * which equals to DEFAULT_EXA_SCRATCH_BFRSZ. If that happens, we split
     * the PictOpOver rendering region into several 256KB chunks. And because
     * of the Pitch(stride) parameter, so we use maximum width of mask picture.
     * that is to say it is a scanline rendering process */
    if (width * height * 4 > DEFAULT_EXA_SCRATCH_BFRSZ) {
        opWidth = width;
        opHeight = DEFAULT_EXA_SCRATCH_BFRSZ / (width * 4);
    }
    else {
        opWidth = width;
        opHeight = height;
    }

    while (1) {

        /* Wait until the GP is idle - this will ensure that the scratch buffer
         * isn't occupied */

        gp_wait_until_idle();

        /* Copy the source to the scratch buffer, and do a src * mask raster
         * operation */

        gp_declare_blt(0);
        opPtr = &lx_alpha_ops[(exaScratch.op * 2) + 1];
        gp_set_source_format(CIMGP_SOURCE_FMT_8_8_8_8);
        gp_set_strides(opWidth * 4, exaScratch.srcPitch);
        gp_set_bpp(lx_get_bpp_from_format(CIMGP_SOURCE_FMT_8_8_8_8));
        gp_set_solid_source(exaScratch.srcColor);
        gp_blend_mask_blt(exaScratch.bufferOffset, 0, opWidth, opHeight,
                          maskOffset, exaScratch.srcPitch, opPtr->operation,
                          exaScratch.fourBpp);

        /* Do a relative operation(refer rendercheck ops.c), and copy the
         * operation result to destination */

        gp_declare_blt(CIMGP_BLTFLAGS_HAZARD);
        opPtr = &lx_alpha_ops[exaScratch.op * 2];
        apply = (exaScratch.dstFormat->alphabits == 0) ?
            CIMGP_APPLY_BLEND_TO_RGB : CIMGP_APPLY_BLEND_TO_ALL;
        gp_set_source_format(CIMGP_SOURCE_FMT_8_8_8_8);
        gp_set_strides(exaGetPixmapPitch(pxDst), opWidth * 4);
        gp_set_bpp(lx_get_bpp_from_format(exaScratch.dstFormat->fmt));
        type = CIMGP_CONVERTED_ALPHA;
        gp_set_alpha_operation(opPtr->operation, type, opPtr->channel,
                               apply, 0);
        gp_screen_to_screen_convert(dstOffset, exaScratch.bufferOffset,
                                    opWidth, opHeight, 0);

        if (width * height * 4 > DEFAULT_EXA_SCRATCH_BFRSZ) {
            /* Finish the rendering */
            if (opoverY + opHeight == opY + height)
                break;
            /* Recalculate the Dest and Mask rendering start point */
            srcPoint.y = srcPoint.y + F(opHeight);
            opoverY = opoverY + opHeight;
            if (opoverY + opHeight > opY + height)
                opHeight = opY + height - opoverY;
            dstOffset = GetPixmapOffset(pxDst, opoverX, opoverY);
            maskOffset = GetSrcOffset(I(srcPoint.x), I(srcPoint.y));
        }
        else
            break;
    }
}

static void
transformPoint(PictTransform *t, xPointFixed *point)
{
    PictVector v;

    v.vector[0] = point->x;
    v.vector[1] = point->y;
    v.vector[2] = xFixed1;

    if (t != NULL)
        PictureTransformPoint(t, &v);

    point->x = v.vector[0];
    point->y = v.vector[1];
}

static void
lx_do_composite(PixmapPtr pxDst, int srcX, int srcY, int maskX,
                int maskY, int dstX, int dstY, int width, int height)
{
    unsigned int dstOffset, srcOffset = 0;

    xPointFixed srcPoint;

    int opX = dstX;
    int opY = dstY;
    int opWidth = width;
    int opHeight = height;

    /* Transform the source coordinates */

    if (exaScratch.type == COMP_TYPE_MASK) {
        srcPoint.x = F(maskX);
        srcPoint.y = F(maskY);
    }
    else {
        srcPoint.x = F(srcX);
        srcPoint.y = F(srcY);
    }

    /* srcX, srcY point to the upper right side of the bounding box
     * in the unrotated coordinate space.  Depending on the orientation,
     * we have to translate the coordinates to point to the origin of
     * the rectangle in the source pixmap */

    switch (exaScratch.rotate) {
    case RR_Rotate_270:
        srcPoint.x += F(width);

        opWidth = height;
        opHeight = width;
        break;

    case RR_Rotate_180:
        srcPoint.x += F(width);
        srcPoint.y += F(height);

        srcX += width;
        srcY += height;
        break;

    case RR_Rotate_90:
        srcPoint.y += F(height);

        opWidth = height;
        opHeight = width;
        break;
    }

    transformPoint(exaScratch.transform, &srcPoint);

    /* Adjust the point to fit into the pixmap */

    if (I(srcPoint.x) < 0) {
        opWidth += I(srcPoint.x);
        srcPoint.x = F(0);
    }

    if (I(srcPoint.y) < 0) {
        opHeight += I(srcPoint.y);
        srcPoint.y = F(0);
    }

    /* Get the source point offset position */

    srcOffset = GetSrcOffset(I(srcPoint.x), I(srcPoint.y));

    /* When mask exists, exaScratch.srcWidth and exaScratch.srcHeight are
     * the source width and source height; Otherwise, they are mask width
     * and mask height */
    /* exaScratch.repeat is the source repeat attribute
     * exaScratch.maskrepeat is the mask repeat attribute */
    /* If type is COMP_TYPE_MASK, maskX and maskY are not zero, we should
     * subtract them to do the operation in the correct region */

    /* FIXME:  Please add the code to handle the condition when the maskX
     * and maskY coordinate are negative or greater than
     * exaScratch.srcWidth and exaScratch.srcHeight */

    if (exaScratch.type == COMP_TYPE_MASK) {
        if ((exaScratch.srcWidth - maskX) < opWidth)
            opWidth = exaScratch.srcWidth - maskX;
        if ((exaScratch.srcHeight - maskY) < opHeight)
            opHeight = exaScratch.srcHeight - maskY;
    }
    else {
        if (exaScratch.type == COMP_TYPE_ONEPASS) {
            /* This is the condition srcX or/and srcY is/are out of source
             * region */
            if (((srcY >= 0 && srcY >= exaScratch.srcHeight)
                 || (srcX >= 0 && srcX >= exaScratch.srcWidth)) &&
                (exaScratch.op == PictOpOver || exaScratch.op == PictOpSrc)) {
                if (exaScratch.repeat == 1) {
                    opWidth = width;
                    opHeight = height;
                }
                else {
                    if (exaScratch.op == PictOpOver)
                        return;
                    else {
                        exaScratch.op = PictOpClear;
                        opWidth = width;
                        opHeight = height;
                    }
                }
                /* This is the condition srcX or/and srcY is/are in the source
                 * region */
            }
            else if (srcX >= 0 && srcY >= 0 &&
                     (exaScratch.op == PictOpOver ||
                      exaScratch.op == PictOpSrc)) {
                if (exaScratch.repeat == 1) {
                    opWidth = width;
                    opHeight = height;
                }
                else {
                    if ((exaScratch.srcWidth - srcX) < opWidth)
                        opWidth = exaScratch.srcWidth - srcX;
                    if ((exaScratch.srcHeight - srcY) < opHeight)
                        opHeight = exaScratch.srcHeight - srcY;
                }
                /* This is the condition srcX or/and srcY is/are negative */
            }
            else if ((srcX < 0 || srcY < 0) &&
                     (exaScratch.op == PictOpOver ||
                      exaScratch.op == PictOpSrc)) {
                if (exaScratch.repeat == 1) {
                    opWidth = width;
                    opHeight = height;
                }
                else {
                    /* FIXME: We can't support negative srcX/Y for all corner cases in
                     * a sane way without a bit bigger refactoring. So as to avoid
                     * gross misrenderings (e.g missing tray icons) in current real-world
                     * applications, just shift destination appropriately for now and
                     * ignore out of bounds source pixmap zero-vector handling. This is
                     * actually correct for PictOpOver, but PictOpSrc out of bounds regions
                     * should be blacked out, but aren't - without this workaround however
                     * it'd be simply all black instead, which is probably worse till a full
                     * clean solution solves it for all cases. */
                    if (srcX < 0) {
                        opX -= srcX;
                        srcX = 0;
                    }

                    if (srcY < 0) {
                        opY -= srcY;
                        srcY = 0;
                    }

                    /* EXA has taken care of adjusting srcWidth if it gets cut on the right */
                    width = opWidth = exaScratch.srcWidth;
                    /* EXA has taken care of adjusting srcHeight if it gets cut on the bottom */
                    height = opHeight = exaScratch.srcHeight;
                }
            }
            else {
                if (exaScratch.srcWidth < opWidth)
                    opWidth = exaScratch.srcWidth;
                if (exaScratch.srcHeight < opHeight)
                    opHeight = exaScratch.srcHeight;
            }
        }
        else {
            if (exaScratch.rotate == RR_Rotate_180) {
            }
            else {
                if ((exaScratch.srcWidth - srcY) < opWidth)
                    opWidth = exaScratch.srcWidth - srcY;
                if ((exaScratch.srcHeight - srcX) < opHeight)
                    opHeight = exaScratch.srcHeight - srcX;
            }
        }
    }

    while (1) {

        dstOffset = GetPixmapOffset(pxDst, opX, opY);

        switch (exaScratch.type) {

        case COMP_TYPE_MASK:{
            if (exaScratch.op == PictOpOver || exaScratch.op ==
                PictOpOutReverse || exaScratch.op == PictOpInReverse ||
                exaScratch.op == PictOpIn || exaScratch.op == PictOpOut ||
                exaScratch.op == PictOpOverReverse)
                lx_do_composite_mask_two_pass(pxDst, dstOffset,
                                              srcOffset, opWidth, opHeight, opX,
                                              opY, srcPoint);
            else
                lx_do_composite_mask(pxDst, dstOffset, srcOffset,
                                     opWidth, opHeight);
        }
            break;

        case COMP_TYPE_ONEPASS:
            if ((exaScratch.op == PictOpOver || exaScratch.op == PictOpSrc)
                && (exaScratch.repeat == 1)) {
                lx_composite_onepass_special(pxDst, opWidth, opHeight, opX, opY,
                                             srcX, srcY);
                return;
            }
            else if ((exaScratch.op == PictOpAdd) && (exaScratch.srcFormat->exa
                                                      == PICT_a8) &&
                     (exaScratch.dstFormat->exa == PICT_a8))
                lx_composite_onepass_add_a8(pxDst, dstOffset, srcOffset,
                                            opWidth, opHeight, opX, opY, srcX,
                                            srcY);
            else
                lx_composite_onepass(pxDst, dstOffset, srcOffset, opWidth,
                                     opHeight);
            break;

        case COMP_TYPE_TWOPASS:
            lx_composite_multipass(pxDst, dstOffset, srcOffset, opWidth,
                                   opHeight);

        case COMP_TYPE_ROTATE:
            lx_composite_rotate(pxDst, dstOffset, srcOffset, opWidth, opHeight);
            break;
        }

        opX += opWidth;

        if (opX >= dstX + width) {
            opX = dstX;
            opY += opHeight;

            if (opY >= dstY + height)
                break;
        }

        /* FIXME:  Please add the code to handle the condition when the maskX
         * and maskY coordinate are negative or greater than
         * exaScratch.srcWidth and exaScratch.srcHeight */

        if (exaScratch.type == COMP_TYPE_MASK) {
            opWidth = ((dstX + width) - opX) > (exaScratch.srcWidth - maskX)
                ? (exaScratch.srcWidth - maskX) : (dstX + width) - opX;
            opHeight = ((dstY + height) - opY) > (exaScratch.srcHeight - maskY)
                ? (exaScratch.srcHeight - maskY) : (dstY + height) - opY;
            /* All black out of the mask */
            if (!exaScratch.maskrepeat)
                exaScratch.srcColor = 0x0;
        }
        else {
            if (exaScratch.type == COMP_TYPE_ONEPASS) {
                if (srcX >= 0 && srcY >= 0 && (exaScratch.op == PictOpOver ||
                                               exaScratch.op == PictOpSrc ||
                                               exaScratch.op == PictOpClear)) {
                    opWidth =
                        ((dstX + width) - opX) >
                        (exaScratch.srcWidth - srcX) ? (exaScratch.srcWidth -
                                                        srcX) : (dstX + width)
                        - opX;
                    opHeight = ((dstY + height) - opY) >
                        (exaScratch.srcHeight - srcY) ?
                        (exaScratch.srcHeight - srcY) : (dstY + height) - opY;
                }
                else {
                    opWidth = ((dstX + width) - opX) > exaScratch.srcWidth ?
                        exaScratch.srcWidth : (dstX + width) - opX;
                    opHeight = ((dstY + height) - opY) > exaScratch.srcHeight ?
                        exaScratch.srcHeight : (dstY + height) - opY;
                }
            }
            else {
                opWidth = ((dstX + width) - opX) > (exaScratch.srcWidth - srcY)
                    ? (exaScratch.srcWidth - srcY) : (dstX + width) - opX;
                opHeight =
                    ((dstY + height) - opY) >
                    (exaScratch.srcHeight - srcX) ? (exaScratch.srcHeight -
                                                     srcX) : (dstY + height) -
                    opY;
            }
            /* All black out of the source */
            if (!exaScratch.repeat && (exaScratch.type == COMP_TYPE_ONEPASS)) {
                /* FIXME: We black out the source here, so that any further regions
                 * in the loop get handled as a source that's a zero-vector (as
                 * defined for out-of-bounds from source pixmap for RepeatModeNone),
                 * but this will likely interfere with cases where srcX and/or srcY
                 * is negative - as opposed to e.g width being larger than srcWidth,
                 * which is exercised in rendercheck (always rectangle in top-left
                 * corner).
                 * Additionally it forces the drawing into tiles of srcWidth/srcHeight
                 * for non-repeat modes too, where we don't really need to tile it like
                 * this and could draw the out of bound regions all at once (or at most
                 * in 4 operations without the big loop). */
                lx_composite_all_black(srcOffset, exaScratch.srcWidth,
                                       exaScratch.srcHeight);
            }
            if (!exaScratch.repeat && (exaScratch.type == COMP_TYPE_ROTATE))
                break;
        }
    }
}

static void
lx_wait_marker(ScreenPtr PScreen, int marker)
{
    gp_wait_until_idle();
}

static void
lx_done(PixmapPtr ptr)
{
}

#if 0
static void
lx_upload_to_screen(PixmapPtr pxDst, int x, int y, int w, int h,
                    char *src, int src_pitch)
{
    GeodeRec *pGeode = GEODEPTR_FROM_PIXMAP(pxDst);
    int dst_pitch = exaGetPixmapPitch(pxDst);
    int cpp = (pxDst->drawable.bitsPerPixel + 7) / 8;

    char *dst;
    int offset = exaGetPixmapOffset(pxDst);

    dst = (char *) (pGeode->FBBase + offset + (y * dst_pitch) + (x * cpp));
    int i;

    for (i = 0; i < h; i++) {
        memcpy(dst, src, w * cpp);
        dst += dst_pitch;
        src += src_pitch;
    }
}
#endif

#if EXA_VERSION_MAJOR > 2 || (EXA_VERSION_MAJOR == 2 && EXA_VERSION_MINOR >= 2)

static Bool
lx_exa_pixmap_is_offscreen(PixmapPtr pPixmap)
{
    ScrnInfoPtr pScrni = xf86ScreenToScrn(pPixmap->drawable.pScreen);
    GeodeRec *pGeode = GEODEPTR(pScrni);
    void *start = (void *) (pGeode->FBBase);
    void *end =
        (void *) (pGeode->FBBase + pGeode->offscreenStart +
                  pGeode->offscreenSize);

    if ((void *) pPixmap->devPrivate.ptr >= start &&
        (void *) pPixmap->devPrivate.ptr < end)
        return TRUE;

    return FALSE;
}

#endif

Bool
LXExaInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrni = xf86ScreenToScrn(pScreen);
    GeodeRec *pGeode = GEODEPTR(pScrni);
    ExaDriverPtr pExa = pGeode->pExa;

    pExa->exa_major = EXA_VERSION_MAJOR;
    pExa->exa_minor = EXA_VERSION_MINOR;

    pExa->WaitMarker = lx_wait_marker;

    pExa->PrepareSolid = lx_prepare_solid;
    pExa->Solid = lx_do_solid;
    pExa->DoneSolid = lx_done;

    pExa->PrepareCopy = lx_prepare_copy;
    pExa->Copy = lx_do_copy;
    pExa->DoneCopy = lx_done;

    /* Composite */
    pExa->CheckComposite = lx_check_composite;
    pExa->PrepareComposite = lx_prepare_composite;
    pExa->Composite = lx_do_composite;
    pExa->DoneComposite = lx_done;
    //pExa->UploadToScreen =  lx_upload_to_screen;

#if EXA_VERSION_MAJOR > 2 || (EXA_VERSION_MAJOR == 2 && EXA_VERSION_MINOR >= 2)
    pExa->PixmapIsOffscreen = lx_exa_pixmap_is_offscreen;
#endif

    //pExa->flags = EXA_OFFSCREEN_PIXMAPS;

    return exaDriverInit(pScreen, pGeode->pExa);
}
