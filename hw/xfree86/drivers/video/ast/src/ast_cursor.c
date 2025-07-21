/*
 * Copyright (c) 2005 ASPEED Technology Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the authors not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The authors makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THE AUTHORS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86cmap.h"
#include "compiler.h"
#include "vgaHW.h"
#include "mipointer.h"
#include "micmap.h"

#include "fb.h"
#include "regionstr.h"
#include "xf86xv.h"
#include <X11/extensions/Xv.h>

#include "xf86Pci.h"

/* framebuffer offscreen manager */
#include "xf86fbman.h"

/* include xaa includes */
#include "xaarop.h"

/* H/W cursor support */
#include "xf86Cursor.h"
#include "cursorstr.h"

/* Driver specific headers */
#include "ast.h"
#include "ast_vgatool.h"
#include "ast_cursor.h"

#ifdef	HWC
/* Prototype type declaration */
static void ASTShowCursor(ScrnInfoPtr pScrn);
static void ASTHideCursor(ScrnInfoPtr pScrn);
static void ASTSetCursorPosition(ScrnInfoPtr pScrn, int x, int y);
static void ASTSetCursorColors(ScrnInfoPtr pScrn, int bg, int fg);
static void ASTLoadCursorImage(ScrnInfoPtr pScrn, UCHAR *src);
static Bool ASTUseHWCursor(ScreenPtr pScreen, CursorPtr pCurs);
static void ASTLoadCursorARGB(ScrnInfoPtr pScrn, CursorPtr pCurs);
static Bool ASTUseHWCursorARGB(ScreenPtr pScreen, CursorPtr pCurs);
static void ASTFireCursor(ScrnInfoPtr pScrn);
static void ASTShowCursor_AST1180(ScrnInfoPtr pScrn);
static void ASTHideCursor_AST1180(ScrnInfoPtr pScrn);
static void ASTSetCursorPosition_AST1180(ScrnInfoPtr pScrn, int x, int y);

Bool
ASTCursorInit(ScreenPtr pScreen)
{
    ScrnInfoPtr	pScrn = xf86ScreenToScrn(pScreen);
    ASTRecPtr 	pAST = ASTPTR(pScrn);
    xf86CursorInfoPtr infoPtr;

    infoPtr = xf86CreateCursorInfoRec();
    if(!infoPtr) return FALSE;

    pAST->HWCInfoPtr = infoPtr;

    infoPtr->Flags = HARDWARE_CURSOR_TRUECOLOR_AT_8BPP |
                     HARDWARE_CURSOR_INVERT_MASK |
                     HARDWARE_CURSOR_BIT_ORDER_MSBFIRST;

    infoPtr->MaxWidth  = MAX_HWC_WIDTH;
    infoPtr->MaxHeight = MAX_HWC_HEIGHT;
    if (pAST->jChipType == AST1180)
    {
        infoPtr->ShowCursor = ASTShowCursor_AST1180;
        infoPtr->HideCursor = ASTHideCursor_AST1180;
        infoPtr->SetCursorPosition = ASTSetCursorPosition_AST1180;
    }
    else
    {
        infoPtr->ShowCursor = ASTShowCursor;
        infoPtr->HideCursor = ASTHideCursor;
        infoPtr->SetCursorPosition = ASTSetCursorPosition;
    }
    infoPtr->SetCursorColors = ASTSetCursorColors;
    infoPtr->LoadCursorImage = ASTLoadCursorImage;
    infoPtr->UseHWCursor = ASTUseHWCursor;
#ifdef ARGB_CURSOR
    infoPtr->UseHWCursorARGB = ASTUseHWCursorARGB;
    infoPtr->LoadCursorARGB = ASTLoadCursorARGB;
#endif

    return(xf86InitCursor(pScreen, infoPtr));

}

Bool bASTInitHWC(ScrnInfoPtr pScrn, ASTRecPtr pAST)
{
    ScreenPtr	pScreen;

    /* init cursor cache info */
    /* Set HWC_NUM in Options instead */
    /* pAST->HWCInfo.HWC_NUM = DEFAULT_HWC_NUM; */
    pAST->HWCInfo.HWC_NUM_Next = 0;

    /* allocate HWC cache */
    if (!pAST->pHWCPtr) {
        pScreen = xf86ScrnToScreen(pScrn);
        pAST->pHWCPtr = xf86AllocateOffscreenLinear (pScreen, (HWC_SIZE+HWC_SIGNATURE_SIZE)*pAST->HWCInfo.HWC_NUM, HWC_ALIGN, NULL, NULL, NULL);

        if (!pAST->pHWCPtr) {
           xf86DrvMsg(pScrn->scrnIndex, X_ERROR,"Allocate HWC Cache failed \n");
           return (FALSE);
        }

        pAST->HWCInfo.ulHWCOffsetAddr  = pAST->pHWCPtr->offset*((pScrn->bitsPerPixel + 1) / 8);
        pAST->HWCInfo.pjHWCVirtualAddr = pAST->FBVirtualAddr + pAST->HWCInfo.ulHWCOffsetAddr;
    }

    return (TRUE);
}

void ASTDisableHWC(ScrnInfoPtr pScrn)
{
    ASTRecPtr   pAST = ASTPTR(pScrn);

    if (pAST->jChipType == AST1180)
        ASTHideCursor_AST1180(pScrn);
    else
        ASTHideCursor(pScrn);
}

static void
ASTShowCursor(ScrnInfoPtr pScrn)
{
    ASTRecPtr   pAST = ASTPTR(pScrn);
    UCHAR 	jReg;

    jReg= 0x02;
    if (pAST->HWCInfo.cursortype ==HWC_COLOR)
        jReg |= 0x01;

    SetIndexRegMask(CRTC_PORT, 0xCB, 0xFC, jReg);	/* enable mono */

}

static void
ASTHideCursor(ScrnInfoPtr pScrn)
{
    ASTRecPtr  pAST = ASTPTR(pScrn);

    SetIndexRegMask(CRTC_PORT, 0xCB, 0xFC, 0x00);	/* disable HWC */

}

static void
ASTSetCursorPosition(ScrnInfoPtr pScrn, int x, int y)
{
    ASTRecPtr	pAST = ASTPTR(pScrn);
    DisplayModePtr mode = pAST->ModePtr;
    int		x_offset, y_offset;
    UCHAR 	*pjSignature;

    /* Set cursor info to Offscreen */
    pjSignature = (UCHAR *) pAST->HWCInfo.pjHWCVirtualAddr + (HWC_SIZE+HWC_SIGNATURE_SIZE)*pAST->HWCInfo.HWC_NUM_Next + HWC_SIZE;
    *((ULONG *) (pjSignature + HWC_SIGNATURE_X)) = x;
    *((ULONG *) (pjSignature + HWC_SIGNATURE_Y)) = y;

    x_offset = pAST->HWCInfo.offset_x;
    y_offset = pAST->HWCInfo.offset_y;

    if(x < 0) {
       x_offset = (-x) + pAST->HWCInfo.offset_x;
       x = 0;
    }

    if(y < 0) {
       y_offset = (-y) + pAST->HWCInfo.offset_y;
       y = 0;
    }

    if(mode->Flags & V_DBLSCAN)  y *= 2;

    /* Set to Reg. */
    SetIndexReg(CRTC_PORT, 0xC2, (UCHAR) (x_offset));
    SetIndexReg(CRTC_PORT, 0xC3, (UCHAR) (y_offset));
    SetIndexReg(CRTC_PORT, 0xC4, (UCHAR) (x & 0xFF));
    SetIndexReg(CRTC_PORT, 0xC5, (UCHAR) ((x >> 8) & 0x0F));
    SetIndexReg(CRTC_PORT, 0xC6, (UCHAR) (y & 0xFF));
    SetIndexReg(CRTC_PORT, 0xC7, (UCHAR) ((y >> 8) & 0x07));

    /* Fire HWC */
    ASTFireCursor(pScrn);

}

static void
ASTSetCursorColors(ScrnInfoPtr pScrn, int bg, int fg)
{
    ASTRecPtr 	pAST = ASTPTR(pScrn);
    ULONG fg1, bg1;

    fg1 = (fg & 0x0F) | (((fg>>8) & 0x0F) << 4) | (((fg>>16) & 0x0F) << 8);
    bg1 = (bg & 0x0F) | (((bg>>8) & 0x0F) << 4) | (((bg>>16) & 0x0F) << 8);

    /* Fixed xorg bugzilla #20609, ycchen@031209 */
    if ( (fg1 != pAST->HWCInfo.fg) || (bg1 != pAST->HWCInfo.bg) )
    {
    	pAST->HWCInfo.fg = fg1;
    	pAST->HWCInfo.bg = bg1;
        ASTLoadCursorImage(pScrn, pAST->HWCInfo.cursorpattern);
    }

}

static void
ASTLoadCursorImage(ScrnInfoPtr pScrn, UCHAR *src)
{
    ASTRecPtr	pAST = ASTPTR(pScrn);
    int 	i, j, k;
    UCHAR 	*pjSrcAnd, *pjSrcXor, *pjDstData;
    ULONG   	ulTempDstAnd32[2], ulTempDstXor32[2], ulTempDstData32[2];
    UCHAR    	jTempSrcAnd32, jTempSrcXor32;
    ULONG	ulCheckSum = 0;
    ULONG 	ulPatternAddr;

    /* init cursor info. */
    pAST->HWCInfo.cursortype = HWC_MONO;
    pAST->HWCInfo.width  = (USHORT) MAX_HWC_WIDTH;
    pAST->HWCInfo.height = (USHORT) MAX_HWC_HEIGHT;
    pAST->HWCInfo.offset_x = MAX_HWC_WIDTH - pAST->HWCInfo.width;
    pAST->HWCInfo.offset_y = MAX_HWC_HEIGHT - pAST->HWCInfo.height;

    /* copy to hwc info */
    for (i=0; i< MAX_HWC_WIDTH*MAX_HWC_HEIGHT/4; i+=4)
       *(ULONG *) (pAST->HWCInfo.cursorpattern + i) = *(ULONG *) (src + i);

    /* copy cursor image to cache */
    pjSrcXor = src;
    pjSrcAnd = src + (MAX_HWC_WIDTH*MAX_HWC_HEIGHT/8);
    pjDstData =  pAST->HWCInfo.pjHWCVirtualAddr+(HWC_SIZE+HWC_SIGNATURE_SIZE)*pAST->HWCInfo.HWC_NUM_Next;

    for (j = 0; j < MAX_HWC_HEIGHT; j++)
    {
       for (i = 0; i < (MAX_HWC_WIDTH/8); i++ )
       {
       	    for (k=7; k>0; k-=2)
       	    {
                jTempSrcAnd32 = *((UCHAR *) pjSrcAnd);
                jTempSrcXor32 = *((UCHAR *) pjSrcXor);
                ulTempDstAnd32[0] = ((jTempSrcAnd32 >> k) & 0x01) ? 0x00008000L:0x00L;
                ulTempDstXor32[0] = ((jTempSrcXor32 >> k) & 0x01) ? 0x00004000L:0x00L;
                ulTempDstData32[0] = ((jTempSrcXor32 >> k) & 0x01) ? pAST->HWCInfo.fg:pAST->HWCInfo.bg;
                ulTempDstAnd32[1] = ((jTempSrcAnd32 >> (k-1)) & 0x01) ? 0x80000000L:0x00L;
                ulTempDstXor32[1] = ((jTempSrcXor32 >> (k-1)) & 0x01) ? 0x40000000L:0x00L;
                ulTempDstData32[1] = ((jTempSrcXor32 >> (k-1)) & 0x01) ? (pAST->HWCInfo.fg << 16):(pAST->HWCInfo.bg << 16);
                /* No inverse for X Window cursor, ycchen@111808 */
                if (ulTempDstAnd32[0])
                    ulTempDstXor32[0] = 0;
                if (ulTempDstAnd32[1])
                    ulTempDstXor32[1] = 0;
                *((ULONG *) pjDstData) = ulTempDstAnd32[0] | ulTempDstXor32[0] | ulTempDstData32[0] | ulTempDstAnd32[1] | ulTempDstXor32[1] | ulTempDstData32[1];
                ulCheckSum += *((ULONG *) pjDstData);
                pjDstData += 4;

            }
            pjSrcAnd ++;
            pjSrcXor ++;

       }

    }

    if (pAST->jChipType == AST1180)
    {
        ulPatternAddr = pAST->ulVRAMBase + (pAST->HWCInfo.ulHWCOffsetAddr+(HWC_SIZE+HWC_SIGNATURE_SIZE)*pAST->HWCInfo.HWC_NUM_Next);
        WriteAST1180SOC(AST1180_GFX_BASE+AST1180_HWC1_PATTERNADDR, ulPatternAddr);
    }
    else
    {
        /* Write Checksum as signature */
        pjDstData = (UCHAR *) pAST->HWCInfo.pjHWCVirtualAddr + (HWC_SIZE+HWC_SIGNATURE_SIZE)*pAST->HWCInfo.HWC_NUM_Next + HWC_SIZE;
        *((ULONG *) pjDstData) = ulCheckSum;
        *((ULONG *) (pjDstData + HWC_SIGNATURE_SizeX)) = pAST->HWCInfo.width;
        *((ULONG *) (pjDstData + HWC_SIGNATURE_SizeY)) = pAST->HWCInfo.height;
        *((ULONG *) (pjDstData + HWC_SIGNATURE_HOTSPOTX)) = 0;
        *((ULONG *) (pjDstData + HWC_SIGNATURE_HOTSPOTY)) = 0;

        /* set pattern offset */
        ulPatternAddr = ((pAST->HWCInfo.ulHWCOffsetAddr+(HWC_SIZE+HWC_SIGNATURE_SIZE)*pAST->HWCInfo.HWC_NUM_Next) >> 3);
        SetIndexReg(CRTC_PORT, 0xC8, (UCHAR) (ulPatternAddr & 0xFF));
        SetIndexReg(CRTC_PORT, 0xC9, (UCHAR) ((ulPatternAddr >> 8) & 0xFF));
        SetIndexReg(CRTC_PORT, 0xCA, (UCHAR) ((ulPatternAddr >> 16) & 0xFF));
    }

    /* update HWC_NUM_Next */
    pAST->HWCInfo.HWC_NUM_Next = (pAST->HWCInfo.HWC_NUM_Next+1) % pAST->HWCInfo.HWC_NUM;

}

static Bool
ASTUseHWCursor(ScreenPtr pScreen, CursorPtr pCurs)
{
    if ( (pCurs->bits->width > MAX_HWC_WIDTH) || (pCurs->bits->height > MAX_HWC_HEIGHT) )
        return FALSE;

    return TRUE;
}

static void
ASTLoadCursorARGB(ScrnInfoPtr pScrn, CursorPtr pCurs)
{
    ASTRecPtr 	pAST = ASTPTR(pScrn);

    UCHAR	*pjDstXor, *pjSrcXor;
    ULONG       i, j, ulSrcWidth, ulSrcHeight;
    ULONG	ulPerPixelCopy, ulTwoPixelCopy;
    LONG        lAlphaDstDelta, lLastAlphaDstDelta;
    union
    {
        ULONG   ul;
        UCHAR   b[4];
    } ulSrcData32[2], ulData32;
    union
    {
        USHORT  us;
        UCHAR   b[2];
    } usData16;
    ULONG	ulCheckSum = 0;
    ULONG 	ulPatternAddr;

    /* init cursor info. */
    pAST->HWCInfo.cursortype = HWC_COLOR;
    pAST->HWCInfo.width  = pCurs->bits->width;
    pAST->HWCInfo.height = pCurs->bits->height;
    pAST->HWCInfo.offset_x = MAX_HWC_WIDTH - pAST->HWCInfo.width;
    pAST->HWCInfo.offset_y = MAX_HWC_HEIGHT - pAST->HWCInfo.height;

    /* copy cursor image to cache */
    ulSrcWidth  =  pAST->HWCInfo.width;
    ulSrcHeight =  pAST->HWCInfo.height;

    lAlphaDstDelta = MAX_HWC_WIDTH << 1;
    lLastAlphaDstDelta = lAlphaDstDelta - (ulSrcWidth << 1);

    pjSrcXor  = (UCHAR *) pCurs->bits->argb;;
    pjDstXor  = (UCHAR *) pAST->HWCInfo.pjHWCVirtualAddr + (HWC_SIZE+HWC_SIGNATURE_SIZE)*pAST->HWCInfo.HWC_NUM_Next
                        + lLastAlphaDstDelta + (MAX_HWC_HEIGHT - ulSrcHeight) * lAlphaDstDelta;

    ulPerPixelCopy =  ulSrcWidth & 1;
    ulTwoPixelCopy =  ulSrcWidth >> 1;

    for (j = 0; j < ulSrcHeight; j++)
    {

        for (i = 0; i < ulTwoPixelCopy; i++ )
        {
            ulSrcData32[0].ul = *((ULONG *) pjSrcXor) & 0xF0F0F0F0;
            ulSrcData32[1].ul = *((ULONG *) (pjSrcXor+4)) & 0xF0F0F0F0;
            ulData32.b[0] = ulSrcData32[0].b[1] | (ulSrcData32[0].b[0] >> 4);
            ulData32.b[1] = ulSrcData32[0].b[3] | (ulSrcData32[0].b[2] >> 4);
            ulData32.b[2] = ulSrcData32[1].b[1] | (ulSrcData32[1].b[0] >> 4);
            ulData32.b[3] = ulSrcData32[1].b[3] | (ulSrcData32[1].b[2] >> 4);
            *((ULONG *) pjDstXor) = ulData32.ul;
            ulCheckSum += (ULONG) ulData32.ul;
            pjDstXor += 4;
            pjSrcXor += 8;
        }

        for (i = 0; i < ulPerPixelCopy; i++ )
        {
            ulSrcData32[0].ul = *((ULONG *) pjSrcXor) & 0xF0F0F0F0;
            usData16.b[0] = ulSrcData32[0].b[1] | (ulSrcData32[0].b[0] >> 4);
            usData16.b[1] = ulSrcData32[0].b[3] | (ulSrcData32[0].b[2] >> 4);
            *((USHORT *) pjDstXor) = usData16.us;
            ulCheckSum += (ULONG) usData16.us;
            pjDstXor += 2;
            pjSrcXor += 4;
        }

        /* Point to next source and dest scans */
        pjDstXor += lLastAlphaDstDelta;

    } /* end of for-loop */

    if (pAST->jChipType == AST1180)
    {
        ulPatternAddr = pAST->ulVRAMBase + (pAST->HWCInfo.ulHWCOffsetAddr+(HWC_SIZE+HWC_SIGNATURE_SIZE)*pAST->HWCInfo.HWC_NUM_Next);
        WriteAST1180SOC(AST1180_GFX_BASE+AST1180_HWC1_PATTERNADDR, ulPatternAddr);
    }
    else
    {
        /* Write Checksum as signature */
        pjDstXor = (UCHAR *) pAST->HWCInfo.pjHWCVirtualAddr + (HWC_SIZE+HWC_SIGNATURE_SIZE)*pAST->HWCInfo.HWC_NUM_Next + HWC_SIZE;
        *((ULONG *) pjDstXor) = ulCheckSum;
        *((ULONG *) (pjDstXor + HWC_SIGNATURE_SizeX)) = pAST->HWCInfo.width;
        *((ULONG *) (pjDstXor + HWC_SIGNATURE_SizeY)) = pAST->HWCInfo.height;
        *((ULONG *) (pjDstXor + HWC_SIGNATURE_HOTSPOTX)) = 0;
        *((ULONG *) (pjDstXor + HWC_SIGNATURE_HOTSPOTY)) = 0;

        /* set pattern offset */
        ulPatternAddr = ((pAST->HWCInfo.ulHWCOffsetAddr +(HWC_SIZE+HWC_SIGNATURE_SIZE)*pAST->HWCInfo.HWC_NUM_Next) >> 3);
        SetIndexReg(CRTC_PORT, 0xC8, (UCHAR) (ulPatternAddr & 0xFF));
        SetIndexReg(CRTC_PORT, 0xC9, (UCHAR) ((ulPatternAddr >> 8) & 0xFF));
        SetIndexReg(CRTC_PORT, 0xCA, (UCHAR) ((ulPatternAddr >> 16) & 0xFF));
    }

    /* update HWC_NUM_Next */
    pAST->HWCInfo.HWC_NUM_Next = (pAST->HWCInfo.HWC_NUM_Next+1) % pAST->HWCInfo.HWC_NUM;

}

static Bool
ASTUseHWCursorARGB(ScreenPtr pScreen, CursorPtr pCurs)
{
    if ( (pCurs->bits->width > MAX_HWC_WIDTH) || (pCurs->bits->height > MAX_HWC_HEIGHT) )
        return FALSE;

    return TRUE;
}

static void
ASTFireCursor(ScrnInfoPtr pScrn)
{
    ASTRecPtr  pAST = ASTPTR(pScrn);

    SetIndexRegMask(CRTC_PORT, 0xCB, 0xFF, 0x00);	/* dummy write to fire HWC */

}

/* AST1180 */
static void
ASTShowCursor_AST1180(ScrnInfoPtr pScrn)
{
    ASTRecPtr   pAST = ASTPTR(pScrn);
    ULONG 	ulData, ulTemp;

    ReadAST1180SOC(AST1180_GFX_BASE+AST1180_HWC1_POSITION, ulTemp);

    ReadAST1180SOC(AST1180_GFX_BASE+AST1180_VGA1_CTRL, ulData);
    ulData &= ~AST1180_ALPHAHWC;
    if (pAST->HWCInfo.cursortype ==HWC_COLOR)
        ulData |= AST1180_ALPHAHWC;
    ulData |= AST1180_ENABLEHWC;
    WriteAST1180SOC(AST1180_GFX_BASE+AST1180_VGA1_CTRL, ulData);

    /* fire cursor */
    WriteAST1180SOC(AST1180_GFX_BASE+AST1180_HWC1_POSITION, ulTemp);

} /* ASTShowCursor_AST1180 */

static void
ASTHideCursor_AST1180(ScrnInfoPtr pScrn)
{
    ASTRecPtr  pAST = ASTPTR(pScrn);
    ULONG 	ulData;

    ReadAST1180SOC(AST1180_GFX_BASE+AST1180_VGA1_CTRL, ulData);
    ulData &= ~AST1180_ENABLEHWC;
    WriteAST1180SOC(AST1180_GFX_BASE+AST1180_VGA1_CTRL, ulData);

    /* fire cursor */
    WriteAST1180SOC(AST1180_GFX_BASE+AST1180_HWC1_POSITION, 0x07ff07ff);

} /* ASTHideCursor_AST1180 */

static void
ASTSetCursorPosition_AST1180(ScrnInfoPtr pScrn, int x, int y)
{
    ASTRecPtr	pAST = ASTPTR(pScrn);
    DisplayModePtr mode = pAST->ModePtr;
    int		x_offset, y_offset;
    ULONG	ulData;

    x_offset = pAST->HWCInfo.offset_x;
    y_offset = pAST->HWCInfo.offset_y;

    if(x < 0) {
       x_offset = (-x) + pAST->HWCInfo.offset_x;
       x = 0;
    }

    if(y < 0) {
       y_offset = (-y) + pAST->HWCInfo.offset_y;
       y = 0;
    }

    if(mode->Flags & V_DBLSCAN)  y *= 2;

    /* Set to Reg. */
    ulData = (x_offset) | (y_offset << 8);
    WriteAST1180SOC(AST1180_GFX_BASE+AST1180_HWC1_OFFSET, ulData);
    ulData = (x) | (y << 16);
    WriteAST1180SOC(AST1180_GFX_BASE+AST1180_HWC1_POSITION, ulData);

} /* ASTSetCursorPosition_AST1180 */

#endif	/* End of HWC */
