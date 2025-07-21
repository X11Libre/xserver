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

/* H/W cursor support */
#include "xf86Cursor.h"

/* Driver specific headers */
#include "ast.h"
#include "ast_2dtool.h"

#ifdef	Accel_2D

#ifdef	AstVideo
/*
 * Video Part
 * by ic_yang
 */
#include "fourcc.h"

void ASTDisplayVideo(ScrnInfoPtr pScrn, ASTPortPrivPtr pPriv, RegionPtr clipBoxes, int id)
{
    ASTPtr              pAST = ASTPTR(pScrn);
    int                 nBoxs;
    int                 ScaleFactorH, ScaleFactorV;
    ULONG               InitScaleFactorH, InitScaleFactorV;
    BURSTSCALECMD       CopyCmd = {0};
    PBURSTSCALECMD      pCopyCmd = NULL;
    float               fScaleX = 0, fScaleY = 0;
    xRectangle          rect;
    BoxPtr              pBox = NULL;
    short               lSrcX, lSrcY;
    ULONG               dwCmd = 0;
    int                 i;

    pBox = REGION_RECTS(clipBoxes);
    nBoxs = REGION_NUM_RECTS(clipBoxes);

    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "MMIO=%x,pBox=%lx, nBoxs=%x\n", pAST->MMIO2D, (uintptr_t) pBox, nBoxs);

    if(0==pPriv->drw_w || 0==pPriv->drw_h)
    {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "drwx=%x, drwy=%x\n", pPriv->drw_w, pPriv->drw_h);
        return;
    }

    /* calc scaling factor */
    fScaleX = (float)pPriv->src_w /(float)pPriv->drw_w;
    fScaleY = (float)pPriv->src_h /(float)pPriv->drw_h;

    if (pPriv->src_w == pPriv->drw_w)
        ScaleFactorH = 0x8000;
    else
        ScaleFactorH = (ULONG)((pPriv->src_w-1)*0x8000)/pPriv->drw_w;

    if (pPriv->src_h == pPriv->drw_h)
    {
        ScaleFactorV = 0x8000;
		dwCmd |= SCALE_EQUAL_VER; /* Setting it save the bandwidtch */
   	}
    else
   	{
        ScaleFactorV = (ULONG)((pPriv->src_h-1)*0x8000)/pPriv->drw_h;
    }

    if (pPriv->drw_w >= pPriv->src_w)
        InitScaleFactorH = 0;
    else
        InitScaleFactorH = 0x4000;

    if (pPriv->drw_h >= pPriv->src_h)
        InitScaleFactorV = 0;
    else
        InitScaleFactorV = 0x4000;

    switch(pScrn->bitsPerPixel)
    {
    case 32:
        dwCmd   = CMD_COLOR_32;
        break;
    case 16:
        dwCmd   = CMD_COLOR_16;
        break;
    case  8:
        dwCmd   = CMD_COLOR_08;
        break;
    }

    dwCmd |= CMD_TYPE_SCALE;
    if (pPriv->drw_w >= pPriv->src_w)
        dwCmd |= SCALE_SEG_NUM_1;
    else
        dwCmd |= SCALE_SEG_NUM_2;

    dwCmd |= SCALE_FORMAT_YUV2RGB;
    switch(id)
    {
    case PIXEL_FMT_YUY2:
        dwCmd |= YUV_FORMAT_YUYV;
        break;
    case PIXEL_FMT_UYVY:
        dwCmd |= YUV_FORMAT_UYVY;
        break;
    default:
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Other pix format:%x\n", id);
        break;
    }

    for(i = 0; i < nBoxs; i++, pBox++)
    {
        rect.x = pBox->x1 - pPriv->drw_x;
        rect.y = pBox->y1 - pPriv->drw_y;
        rect.width = pBox->x2 - pBox->x1;
        rect.height = pBox->y2 - pBox->y1;

        lSrcX = (ULONG)((float)rect.x * fScaleX + pPriv->src_x + 0.5f);
        lSrcY = (ULONG)((float)rect.y * fScaleY + pPriv->src_y + 0.5f);

        pCopyCmd = (BURSTSCALECMD*)pASTjRequestCMDQ(pAST, PKT_TYPESCALE_LENGTH);

        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "pCopyCmd=%p, pBox=%x,%x,%x,%x\n", pCopyCmd, pBox->x1, pBox->y1, pBox->x2, pBox->y2);

        CopyCmd.dwHeader0        = (ULONG)  PKT_BURST_CMD_HEADER0 |
                                             PKT_TYPESCALE_DATALENGTH |
                                             PKT_TYPESCALE_ADDRSTART  |
                                             BURST_FORCE_CMD;

        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "CopyCmd.dwHeader0=%x\n", CopyCmd.dwHeader0);

        CopyCmd.dwSrcPitch       = (ULONG)(pPriv->srcPitch << 16);

        CopyCmd.dwDstHeightPitch = (ULONG)  ((pAST->VideoModeInfo.ScreenPitch << 16) | 0xFFFF);

        CopyCmd.dwDstXY          = (ULONG) ((pBox->x1 << 16) | (pBox->y1 & 0xFFFF));
        CopyCmd.dwSrcXY          = (ULONG) ((lSrcX << 16) | (lSrcY & 0xFFFF));
        CopyCmd.dwRecHeightWidth = (ULONG) ((rect.width << 16) | rect.height);

        CopyCmd.dwInitScaleFactorH = InitScaleFactorH;
        CopyCmd.dwInitScaleFactorV = InitScaleFactorV;
        CopyCmd.dwScaleFactorH   = ScaleFactorH;
        CopyCmd.dwScaleFactorV   = ScaleFactorV;

        CopyCmd.dwSrcBaseAddr = pPriv->bufAddr[pPriv->currentBuf];
        CopyCmd.dwDstBaseAddr = 0;
        CopyCmd.dwCmd = dwCmd;
        CopyCmd.NullData[0] = 0;    /* for alignment */
        memcpy(pCopyCmd, &CopyCmd, sizeof(CopyCmd));

        mUpdateWritePointer;

#if 0
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "%08x, %08x, %08x, %08x\n",
                        *(ULONG *)(pAST->MMIOVirtualAddr+0x8000),
                        *(ULONG *)(pAST->MMIOVirtualAddr+0x8004),
                        *(ULONG *)(pAST->MMIOVirtualAddr+0x8008),
                        *(ULONG *)(pAST->MMIOVirtualAddr+0x800C));
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "%08x, %08x, %08x, %08x\n",
                        *(ULONG *)(pAST->MMIOVirtualAddr+0x8010),
                        *(ULONG *)(pAST->MMIOVirtualAddr+0x8014),
                        *(ULONG *)(pAST->MMIOVirtualAddr+0x8018),
                        *(ULONG *)(pAST->MMIOVirtualAddr+0x801C));
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "%08x, %08x, %08x, %08x\n",
                        *(ULONG *)(pAST->MMIOVirtualAddr+0x8020),
                        *(ULONG *)(pAST->MMIOVirtualAddr+0x8024),
                        *(ULONG *)(pAST->MMIOVirtualAddr+0x8028),
                        *(ULONG *)(pAST->MMIOVirtualAddr+0x802C));
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "%08x, %08x, %08x, %08x\n",
                        *(ULONG *)(pAST->MMIOVirtualAddr+0x8030),
                        *(ULONG *)(pAST->MMIOVirtualAddr+0x8034),
                        *(ULONG *)(pAST->MMIOVirtualAddr+0x8038),
                        *(ULONG *)(pAST->MMIOVirtualAddr+0x803C));
#endif

    } /* End of for-loop */

} /* ASTDisplayVideo */
#endif	/* AstVideo */

#endif	/* end of Accel_2D */
