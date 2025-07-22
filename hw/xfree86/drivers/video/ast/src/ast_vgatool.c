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
#include <xorg-config.h>

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

/* usleep() */
#include <unistd.h>

/* Driver specific headers */
#include "ast.h"
#include "ast_vgatool.h"
#include "ast_dp501fw.h"

__inline ULONG MIndwm(UCHAR *mmiobase, ULONG r)
{
    ULONG ulData;

    *(ULONG *) (mmiobase + 0xF004) = r & 0xFFFF0000;
    *(ULONG *) (mmiobase + 0xF000) = 0x1;

    do {
        ulData = *(volatile ULONG *) (mmiobase + 0xF004) & 0xFFFF0000;
    } while (ulData != (r & 0xFFFF0000) );

    return ( *(volatile ULONG *) (mmiobase + 0x10000 + (r & 0x0000FFFF)) );

}

__inline void MOutdwm(UCHAR *mmiobase, ULONG r, ULONG v)
{
    ULONG ulData;

    *(ULONG *) (mmiobase + 0xF004) = r & 0xFFFF0000;
    *(ULONG *) (mmiobase + 0xF000) = 0x1;

    do {
        ulData = *(volatile ULONG *) (mmiobase + 0xF004) & 0xFFFF0000;
    } while (ulData != (r & 0xFFFF0000) );

    *(volatile ULONG *) (mmiobase + 0x10000 + (r & 0x0000FFFF)) = v;
}

/*
 * BMCI2C
 */
#define I2C_BASE	0x1e780000


static ULONG GetFWBase(ScrnInfoPtr pScrn)
{
    ASTRecPtr pAST = ASTPTR(pScrn);
    UCHAR *mmiobase;

    mmiobase = pAST->MMIOVirtualAddr;
    return (MIndwm(mmiobase, 0x1e6e2104) & 0x7FFFFFFF);
}

static void send_ack(ScrnInfoPtr pScrn)
{
    ASTRecPtr pAST = ASTPTR(pScrn);
    UCHAR SendACK;

    GetIndexRegMask(CRTC_PORT, 0x9b, 0xFF, SendACK);
	SendACK |= 0x80;
    SetIndexRegMask(CRTC_PORT, 0x9B, 0x00, SendACK);
}

static void send_nack(ScrnInfoPtr pScrn)
{
    ASTRecPtr pAST = ASTPTR(pScrn);
    UCHAR SendACK;

    GetIndexRegMask(CRTC_PORT, 0x9b, 0xFF, SendACK);
	SendACK &= ~0x80;
    SetIndexRegMask(CRTC_PORT, 0x9B, 0x00, SendACK);
}

static Bool wait_ack(ScrnInfoPtr pScrn)
{
    ASTRecPtr pAST = ASTPTR(pScrn);
    UCHAR WaitACK;
    ULONG retry=0;

 	do {
        GetIndexRegMask(CRTC_PORT, 0xd2, 0xFF, WaitACK);
        WaitACK &= 0x80;
	 	DelayUS(100);
    } while ( (!WaitACK) && (retry++ < 1000) );

    if (retry < 1000)
        return TRUE;
    else
        return FALSE;
}

static Bool wait_nack(ScrnInfoPtr pScrn)
{
    ASTRecPtr pAST = ASTPTR(pScrn);
    UCHAR WaitACK;
    ULONG retry=0;

 	do {
        GetIndexRegMask(CRTC_PORT, 0xd2, 0xFF, WaitACK);
        WaitACK &= 0x80;
	 	DelayUS(100);
    } while ( (WaitACK) && (retry++ < 1000) );

    if (retry < 1000)
        return TRUE;
    else
        return FALSE;
}

static void set_cmd_trigger(ScrnInfoPtr pScrn)
{
    ASTRecPtr pAST = ASTPTR(pScrn);

    SetIndexRegMask(CRTC_PORT, 0x9B, ~0x40, 0x40);
}

static void clear_cmd_trigger(ScrnInfoPtr pScrn)
{
    ASTRecPtr pAST = ASTPTR(pScrn);

    SetIndexRegMask(CRTC_PORT, 0x9B, ~0x40, 0x00);
}

static Bool write_cmd(ScrnInfoPtr pScrn, UCHAR data)
{
    ASTRecPtr pAST = ASTPTR(pScrn);
	UCHAR retry = 0;

	if (wait_nack(pScrn))
	{
        send_nack(pScrn);
        SetIndexRegMask(CRTC_PORT, 0x9a, 0x00, data);
        send_ack(pScrn);
        set_cmd_trigger(pScrn);
        do {
		    if (wait_ack(pScrn))
		    {
			    clear_cmd_trigger(pScrn);
			    send_nack(pScrn);
		        return TRUE;
	        }
        } while (retry++ < 100);
    }

    clear_cmd_trigger(pScrn);
    send_nack(pScrn);
    return FALSE;
}

static Bool write_data(ScrnInfoPtr pScrn, UCHAR data)
{
    ASTRecPtr pAST = ASTPTR(pScrn);

	if (wait_nack(pScrn))
	{
        send_nack(pScrn);
        SetIndexRegMask(CRTC_PORT, 0x9a, 0x00, data);
        send_ack(pScrn);
		if (wait_ack(pScrn))
		{
			send_nack(pScrn);
		    return TRUE;
	    }
    }

    send_nack(pScrn);
    return FALSE;
}

static void SetDP501VideoOutput(ScrnInfoPtr pScrn, UCHAR Mode)
{
    write_cmd(pScrn, 0x40);
    write_data(pScrn, Mode);

    DelayMS(10);						/* delay 10ms */

} /* SetDP501VideoOutput */

static BOOL BackupM68KFW(ScrnInfoPtr pScrn, UCHAR *addr, ULONG size)
{
    ASTRecPtr pAST  = ASTPTR(pScrn);
    UCHAR *mmiobase = pAST->MMIOVirtualAddr;
    ULONG i, Data;
    ULONG BootAddress;

    Data = MIndwm(mmiobase, 0x1e6e2100) & 0x01;
    if (Data)	/* FW had been load */
	{
        /* copy image to buffer */
        BootAddress = GetFWBase(pScrn);
        for (i=0; i<size; i+=4)
        {
            *(ULONG *)(addr + i) = MIndwm(mmiobase, BootAddress + i);
        }
        return TRUE;
    } /* UEFI Driver Handling */

    return FALSE;
} /* BackupM68KFW */

static BOOL LaunchM68K(ScrnInfoPtr pScrn)
{
    ASTRecPtr pAST  = ASTPTR(pScrn);
    UCHAR *mmiobase = pAST->MMIOVirtualAddr;
    ULONG i, Data, Len;
    ULONG BootAddress;
    UCHAR *pFWAddr;
    UCHAR jReg;

    Data = MIndwm(mmiobase, 0x1e6e2100) & 0x03;
    if (Data != 0x01)	/* UEFI Driver Handling */
    {
        /* Reset Co-processor */
        MOutdwm(mmiobase, 0x1e6e2100, 0x03);
        do {
            Data = MIndwm(mmiobase, 0x1e6e2100);
        } while (Data != 0x03);

	if (pAST->pDP501FWBufferVirtualAddress)
	{
	    pFWAddr = pAST->pDP501FWBufferVirtualAddress;
	    Len     = 32*1024;		/* 32K */
	}
	else
	{
	    pFWAddr = AST_DP501_firmware;
            Len     = sizeof(AST_DP501_firmware) / sizeof(AST_DP501_firmware[0]);
	}

        /* Get BootAddress */
        MOutdwm(mmiobase, 0x1e6e2000, 0x1688a8a8);				/* open passwd */
        Data = MIndwm(mmiobase, 0x1e6e0004);
        switch (Data & 0x03)
        {
	    case 0x00:	/* 64M */
	        BootAddress = 0x44000000;
	        break;
	    default:
	    case 0x01:	/* 128MB */
	        BootAddress = 0x48000000;
	        break;
	    case 0x02:	/* 256MB */
	        BootAddress = 0x50000000;
	        break;
	    case 0x03:	/* 512MB */
	        BootAddress = 0x60000000;
	        break;
        }
        BootAddress -= 0x200000;	/* - 2MB */

        /* copy image to buffer */
        for (i=0; i<Len; i+=4)
        {
	        Data = *(ULONG *)(pFWAddr + i);
            MOutdwm(mmiobase, BootAddress + i, Data);
        }

        /* Init SCU */
        MOutdwm(mmiobase, 0x1e6e2000, 0x1688a8a8);				/* open passwd */

        /* Launch FW */
        MOutdwm(mmiobase, 0x1e6e2104, 0x80000000 + BootAddress);
        MOutdwm(mmiobase, 0x1e6e2100, 1);

        /* Update Scratch */
        Data  = MIndwm(mmiobase, 0x1e6e2040) & 0xFFFFF1FF;		/* D[11:9] = 100b: UEFI handling */
        Data |= 0x0800;
        MOutdwm(mmiobase, 0x1e6e2040, Data);

        GetIndexRegMask(CRTC_PORT, 0x99, 0xFC, jReg);			/* D[1:0]: Reserved Video Buffer */
        jReg |= 0x02;										    /* 2MB */
        SetIndexReg(CRTC_PORT, 0x99, jReg);

    } /* UEFI Driver Handling */

    return TRUE;
} /* LaunchM68K */

/*
 * DP501 external
 */
Bool ASTReadEDID_M68K(ScrnInfoPtr pScrn, BYTE *pEDIDData)
{
    ASTRecPtr pAST = ASTPTR(pScrn);
    UCHAR *mmiobase;
    ULONG i, BootAddress, Offset, Data;

    /* init value */
    mmiobase = pAST->MMIOVirtualAddr;
    BootAddress = GetFWBase(pScrn);

    /* validate FW version */
    Offset = 0xF000;
    Data = MIndwm(mmiobase, BootAddress + Offset);
    if ((Data & 0xF0) != 0x10)	/* version: 1x */
        return FALSE;

    /* validate PnP Monitor */
    Offset = 0xF010;
    Data = MIndwm(mmiobase, BootAddress + Offset);
    if (!(Data & 0x01))
        return FALSE;

    /* Read EDID */
    Offset = 0xF020;
    for (i=0; i<128; i+=4)
    {
        Data = MIndwm(mmiobase, BootAddress + Offset + i);
        *(ULONG *)(pEDIDData + i) = Data;
    }

    return TRUE;
} /* ReadEDID_M68K */

UCHAR ASTGetLinkMaxCLK(ScrnInfoPtr pScrn)
{
    ASTRecPtr pAST = ASTPTR(pScrn);
    UCHAR *mmiobase;
    ULONG BootAddress, Offset, Data;
    UCHAR LinkCap[4], LinkRate, LinkLanes, MaxClk = 0xFF;

    /* init value */
    mmiobase = pAST->MMIOVirtualAddr;
    BootAddress = GetFWBase(pScrn);

    /* validate FW version */
    Offset = 0xF000;
    Data = MIndwm(mmiobase, BootAddress + Offset);
    if ((Data & 0xF0) != 0x10)	/* version: 1x */
        return MaxClk;

    /* Read Link Capability */
    Offset = 0xF014;
    *(ULONG *)(LinkCap) = MIndwm(mmiobase, BootAddress + Offset);
    if (LinkCap[2] == 0)		/* no Max. CLK Assigned */
    {
	    LinkRate  = LinkCap[0];
	    LinkLanes = LinkCap[1];
	    Data = (LinkRate == 0x0A) ? (90 * LinkLanes): (54 * LinkLanes);
	    if (Data > 255) Data = 255;				/* Max. */
	    MaxClk = (UCHAR)(Data);
    }

    return MaxClk;

} /* ASTGetLinkMaxCLK */

/*
 * VGA Modules
 */
void
vASTOpenKey(ScrnInfoPtr pScrn)
{
   ASTRecPtr pAST = ASTPTR(pScrn);

   SetIndexReg(CRTC_PORT,0x80, 0xA8);

}

Bool
bASTRegInit(ScrnInfoPtr pScrn)
{
   ASTRecPtr pAST = ASTPTR(pScrn);

   /* Enable MMIO */
   SetIndexRegMask(CRTC_PORT,0xA1, 0xFF, 0x04);

   /* Enable Big-Endian */
#if	defined(__sparc__)
   SetIndexRegMask(CRTC_PORT,0xA2, 0xFF, 0x80);
#endif

   return (TRUE);

}

void
ASTGetDRAMInfo(ScrnInfoPtr pScrn)
{
    ASTRecPtr pAST = ASTPTR(pScrn);
    ULONG ulRefPLL, ulDeNumerator, ulNumerator, ulDivider;
    ULONG ulData, ulData2;

    *(ULONG *) (pAST->MMIOVirtualAddr + 0xF004) = 0x1e6e0000;
    *(ULONG *) (pAST->MMIOVirtualAddr + 0xF000) = 0x1;

    /* Based on the Linux DRM driver we might not be able to access this
     * If we can't just use some sane defaults.
     *
     * See drm/drivers/gpu/drm/ast/ast_main.c line 295.
     */

    *(ULONG *) (pAST->MMIOVirtualAddr + 0x10000) = 0xFC600309;
    for (ulData = 10000; ulData > 0; ulData--)
	if (*(volatile ULONG *) (pAST->MMIOVirtualAddr + 0x10000) == 0x01)
	    break;

    if (ulData == 0) {
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Unable to read DRAM information, using defaults\n");
	pAST->ulDRAMBusWidth = 16;
	pAST->jDRAMType = DRAMTYPE_1Gx16;
	if (pAST->jChipType == AST2500)
	    pAST->ulMCLK = 800;
	else
	    pAST->ulMCLK = 396;
	return;
    }
    ulData = *(volatile ULONG *) (pAST->MMIOVirtualAddr + 0x10004);

    /* Get BusWidth */
    if (ulData & 0x40)
       pAST->ulDRAMBusWidth = 16;
    else
       pAST->ulDRAMBusWidth = 32;

    /* Get DRAM Type */
    if ((pAST->jChipType == AST2300) || (pAST->jChipType == AST2400) || (pAST->jChipType == AST2500))
    {
        switch (ulData & 0x03)
        {
        case 0x00:
            pAST->jDRAMType = DRAMTYPE_512Mx16;
            break;
        default:
        case 0x01:
            pAST->jDRAMType = DRAMTYPE_1Gx16;
            break;
        case 0x02:
            pAST->jDRAMType = DRAMTYPE_2Gx16;
            break;
        case 0x03:
            pAST->jDRAMType = DRAMTYPE_4Gx16;
            break;
        }
    }
    else
    {
        switch (ulData & 0x0C)
        {
        case 0x00:
        case 0x04:
            pAST->jDRAMType = DRAMTYPE_512Mx16;
            break;

        case 0x08:
            if (ulData & 0x40)		/* 16bits */
                pAST->jDRAMType = DRAMTYPE_1Gx16;
            else			/* 32bits */
                pAST->jDRAMType = DRAMTYPE_512Mx32;
            break;

        case 0x0C:
            pAST->jDRAMType = DRAMTYPE_1Gx32;
            break;
        }
    }

    /* Get MCLK */
    ulData = *(ULONG *) (pAST->MMIOVirtualAddr + 0x10120);
    ulData2 = *(ULONG *) (pAST->MMIOVirtualAddr + 0x10170);
    if (ulData2 & 0x2000)
        ulRefPLL = 14318;
    else
        ulRefPLL = 12000;

    ulDeNumerator = ulData & 0x1F;
    ulNumerator = (ulData & 0x3FE0) >> 5;

    ulData = (ulData & 0xC000) >> 14;
    switch (ulData)
    {
    case 0x03:
        ulDivider = 0x04;
        break;
    case 0x02:
    case 0x01:
        ulDivider = 0x02;
        break;
    default:
        ulDivider = 0x01;
    }
    pAST->ulMCLK = ulRefPLL * (ulNumerator + 2) / ((ulDeNumerator + 2) * ulDivider * 1000);

} /* ASTGetDRAMInfo */

ULONG
ASTGetVRAMInfo(ScrnInfoPtr pScrn)
{
   ASTRecPtr pAST = ASTPTR(pScrn);
   ULONG ulVRAMSize;
   UCHAR jReg;

   vASTOpenKey(pScrn);

   /* Get VRAMSize from H/W Trapping */
   GetIndexRegMask(CRTC_PORT, 0xAA, 0xFF, jReg);
   switch (jReg & 0x03)
   {
   default:
   case 0x00:
       ulVRAMSize = VIDEOMEM_SIZE_08M;
       break;
   case 0x01:
       ulVRAMSize = VIDEOMEM_SIZE_16M;
       break;
   case 0x02:
       ulVRAMSize = VIDEOMEM_SIZE_32M;
       break;
   case 0x03:
       ulVRAMSize = VIDEOMEM_SIZE_64M;
       break;
   }

   /* Adjust VRAMSize from Scratch */
   GetIndexRegMask(CRTC_PORT, 0x99, 0xFF, jReg);
   switch (jReg & 0x03)
   {
   case 0x01:
       ulVRAMSize -= 0x100000;
       break;
   case 0x02:
       ulVRAMSize -= 0x200000;
       break;
   case 0x03:
       ulVRAMSize -= 0x400000;
       break;
   }

   return (ulVRAMSize);
}

ULONG
ASTGetMaxDCLK(ScrnInfoPtr pScrn)
{
   ASTRecPtr pAST = ASTPTR(pScrn);
   UCHAR jReg;
   ULONG ulDRAMBusWidth, ulMCLK, ulDRAMBandwidth, ActualDRAMBandwidth, DRAMEfficiency = 500;
   ULONG ulDCLK;

   ulMCLK = pAST->ulMCLK;
   ulDRAMBusWidth = pAST->ulDRAMBusWidth;

   /* Get Bandwidth */
   /* Modify DARM utilization to 60% for AST1100/2100 16bits DRAM, ycchen@032508 */
   if ( ((pAST->jChipType == AST2100) || (pAST->jChipType == AST1100) || (pAST->jChipType == AST2200) || (pAST->jChipType == AST2150)) && (ulDRAMBusWidth == 16) )
       DRAMEfficiency = 600;
   else if ((pAST->jChipType == AST2300) || (pAST->jChipType == AST2400) || (pAST->jChipType == AST2500))
       DRAMEfficiency = 400;
   ulDRAMBandwidth = ulMCLK * ulDRAMBusWidth * 2 / 8;
   ActualDRAMBandwidth = ulDRAMBandwidth * DRAMEfficiency / 1000;

   /* Get Max DCLK */
   if (pAST->jChipType == AST1180)
   {
       ulDCLK = ActualDRAMBandwidth / ((pScrn->bitsPerPixel+1) / 8);
   }
   else
   {
       /* Fixed Fixed KVM + CRT threshold issue on AST2100 8bpp modes, ycchen@100708 */
       GetIndexRegMask(CRTC_PORT, 0xD0, 0xFF, jReg);
       if ((jReg & 0x08) && (pAST->jChipType == AST2000))
           ulDCLK = ActualDRAMBandwidth / ((pScrn->bitsPerPixel+1+16) / 8);
       else if ((jReg & 0x08) && (pScrn->bitsPerPixel == 8))
           ulDCLK = ActualDRAMBandwidth / ((pScrn->bitsPerPixel+1+24) / 8);
       else
           ulDCLK = ActualDRAMBandwidth / ((pScrn->bitsPerPixel+1) / 8);
   }

   /* Validate for DP501 */
   if (pAST->jTxChipType == Tx_DP501)
   {
       if (ulDCLK > pAST->DP501_MaxVCLK) ulDCLK = pAST->DP501_MaxVCLK;
   }

   /* Add for AST2100, ycchen@061807 */
   if ((pAST->jChipType == AST2100) || (pAST->jChipType == AST2200) || (pAST->jChipType == AST2300) || (pAST->jChipType == AST2400) || (pAST->jChipType == AST2500) || (pAST->jChipType == AST1180) )
   {
       if (ulDCLK > 200) ulDCLK = 200;
   }
   else
   {
       if (ulDCLK > 165) ulDCLK = 165;
   }

   return(ulDCLK);

}

void
ASTGetChipType(ScrnInfoPtr pScrn)
{
   ASTRecPtr pAST = ASTPTR(pScrn);
   ULONG ulData;

   pAST->jChipType = AST2100;

   *(ULONG *) (pAST->MMIOVirtualAddr + 0xF004) = 0x1e6e0000;
   *(ULONG *) (pAST->MMIOVirtualAddr + 0xF000) = 0x1;
   ulData = *(ULONG *) (pAST->MMIOVirtualAddr + 0x1207c);
   switch (ulData & 0x0300)
   {
   case 0x0200:
       xf86DrvMsg(pScrn->scrnIndex, X_INFO, "AST1100 Detected.\n");
       pAST->jChipType = AST1100;
       break;
   case 0x0100:
       xf86DrvMsg(pScrn->scrnIndex, X_INFO, "AST2200 Detected.\n");
       pAST->jChipType = AST2200;
       break;
   case 0x0000:
       xf86DrvMsg(pScrn->scrnIndex, X_INFO, "AST2150 Detected.\n");
       pAST->jChipType = AST2150;
       break;
   default:
       xf86DrvMsg(pScrn->scrnIndex, X_INFO, "AST2100 Detected.\n");
       pAST->jChipType = AST2100;
   }

}

void
ASTGetScratchOptions(ScrnInfoPtr pScrn)
{
   ASTRecPtr pAST = ASTPTR(pScrn);
   ULONG ulData;
   UCHAR jReg;

   /* VGA2 Clone Support */
   GetIndexRegMask(CRTC_PORT, 0x90, 0xFF, jReg);
   if (jReg & 0x10)
       pAST->VGA2Clone = TRUE;

   /* 3rd Tx Check */
   pAST->pDP501FWBufferVirtualAddress = NULL;
   pAST->jTxChipType = Tx_NONE;
   /* Get 3rd Tx Info from HW Reg. */
   GetIndexRegMask(CRTC_PORT, 0xA3, 0xFF, jReg);
   if (jReg & 0x80)
       pAST->jTxChipType = Tx_Sil164;
   /* Get 3rd Tx Info from BMC Scratch */
   if ((pAST->jChipType == AST2300) || (pAST->jChipType == AST2400) || (pAST->jChipType == AST2500))
   {
       GetIndexRegMask(CRTC_PORT, 0xD1, 0x0E, jReg);
       switch (jReg)
       {
       case 0x04:
           pAST->jTxChipType = Tx_Sil164;
           break;
       case 0x08:
           pAST->pDP501FWBufferVirtualAddress = (UCHAR*) calloc(1, 32*1024);
           if	(pAST->pDP501FWBufferVirtualAddress)
           {
               if (BackupM68KFW(pScrn, pAST->pDP501FWBufferVirtualAddress, 32*1024) == FALSE)
               {
                   free(pAST->pDP501FWBufferVirtualAddress);
                   pAST->pDP501FWBufferVirtualAddress = NULL;
               }
           } /* Backup DP501 FW */
       case 0x0c:
           pAST->jTxChipType = Tx_DP501;
           break;
       }
   }

   /* WideScreen Support */
   switch (pAST->jChipType)
   {
   case AST1180:
       pAST->SupportWideScreen = TRUE;
       break;
   case AST2000:
       pAST->SupportWideScreen = FALSE;
       break;
   default:
       GetIndexRegMask(CRTC_PORT, 0xD0, 0xFF, jReg);
       if (!(jReg & 0x80))
           pAST->SupportWideScreen = TRUE;
       else if (jReg & 0x01)
           pAST->SupportWideScreen = TRUE;
       else
       {
           pAST->SupportWideScreen = FALSE;
           *(ULONG *) (pAST->MMIOVirtualAddr + 0xF004) = 0x1e6e0000;
           *(ULONG *) (pAST->MMIOVirtualAddr + 0xF000) = 0x1;
           ulData  = *(ULONG *) (pAST->MMIOVirtualAddr + 0x1207c);
           ulData &= 0x0300;
           if ((pAST->jChipType == AST2300) && (ulData == 0x0000))	/* AST1300 */
               pAST->SupportWideScreen = TRUE;
           if ((pAST->jChipType == AST2400) && (ulData == 0x0100))	/* AST1400 */
               pAST->SupportWideScreen = TRUE;
       }
    } /* switch case */

} /* GetScratchOptions */

void
vASTSetStartAddressCRT1(ASTRecPtr pAST, ULONG base)
{
    ULONG addr;

    if (pAST->jChipType == AST1180)
    {
        addr = pAST->ulVRAMBase + base;
        WriteAST1180SOC(AST1180_GFX_BASE + AST1180_VGA1_STARTADDR, addr);
    }
    else
    {
        addr = base >> 2;			/* DW unit */

        SetIndexReg(CRTC_PORT,0x0D, (UCHAR) (addr & 0xFF));
        SetIndexReg(CRTC_PORT,0x0C, (UCHAR) ((addr >> 8) & 0xFF));
        SetIndexReg(CRTC_PORT,0xAF, (UCHAR) ((addr >> 16) & 0xFF));
    }

}

void
vAST1000DisplayOff(ScrnInfoPtr pScrn)
{
    ASTRecPtr pAST = ASTPTR(pScrn);
    ULONG ulData;

	/* 3rd Tx */
    if (pAST->jTxChipType == Tx_DP501)
	    SetDP501VideoOutput(pScrn, 0);

    if (pAST->jChipType == AST1180)
    {
        ReadAST1180SOC(AST1180_GFX_BASE + AST1180_VGA1_CTRL, ulData);
        ulData |= 0x00100000;
        WriteAST1180SOC(AST1180_GFX_BASE + AST1180_VGA1_CTRL, ulData);
    }
    else
        SetIndexRegMask(SEQ_PORT,0x01, 0xDF, 0x20);

}

void
vAST1000DisplayOn(ScrnInfoPtr pScrn)
{

    ASTRecPtr pAST = ASTPTR(pScrn);
    ULONG ulData;

    if (pAST->jChipType == AST1180)
    {
        ReadAST1180SOC(AST1180_GFX_BASE + AST1180_VGA1_CTRL, ulData);
        ulData &= 0xFFEFFFFF;
        WriteAST1180SOC(AST1180_GFX_BASE + AST1180_VGA1_CTRL, ulData);
    }
    else
        SetIndexRegMask(SEQ_PORT,0x01, 0xDF, 0x00);

    /* 3rd Tx */
    if (pAST->jTxChipType == Tx_DP501)
	    SetDP501VideoOutput(pScrn, 1);
}

void ASTBlankScreen(ScrnInfoPtr pScrn, Bool unblack)
{
    if (unblack)
        vAST1000DisplayOn(pScrn);
    else
        vAST1000DisplayOff(pScrn);
}

void
vASTLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices, LOCO *colors,
               VisualPtr pVisual)
{

    ASTRecPtr  pAST = ASTPTR(pScrn);
    int     i, j, index;
    UCHAR DACIndex, DACR, DACG, DACB;

    switch (pScrn->bitsPerPixel) {
    case 15:
        for(i=0; i<numColors; i++) {
            index = indices[i];
	    for(j=0; j<8; j++) {
                DACIndex = (index * 8) + j;
                DACR = colors[index].red << (8- pScrn->rgbBits);
                DACG = colors[index].green << (8- pScrn->rgbBits);
                DACB = colors[index].blue << (8- pScrn->rgbBits);

                VGA_LOAD_PALETTE_INDEX (DACIndex, DACR, DACG, DACB);
	    }
        }
        break;

    case 16:
        for(i=0; i<numColors; i++) {
            index = indices[i];
	    for(j=0; j<4; j++) {
                DACIndex = (index * 4) + j;
                DACR = colors[index/2].red << (8- pScrn->rgbBits);
                DACG = colors[index].green << (8- pScrn->rgbBits);
                DACB = colors[index/2].blue << (8- pScrn->rgbBits);

                VGA_LOAD_PALETTE_INDEX (DACIndex, DACR, DACG, DACB);
	    }
        }
        break;

    case 24:
        for(i=0; i<numColors; i++) {
            index = indices[i];
            DACIndex = index;
            DACR = colors[index].red;
            DACG = colors[index].green;
            DACB = colors[index].blue;

            VGA_LOAD_PALETTE_INDEX (DACIndex, DACR, DACG, DACB);
        }
        break;

    default:
        for(i=0; i<numColors; i++) {
            index = indices[i];
            DACIndex = index;
            DACR = colors[index].red >> (8 - pScrn->rgbBits);
            DACG = colors[index].green >> (8 - pScrn->rgbBits);
            DACB = colors[index].blue >> (8 - pScrn->rgbBits);

            VGA_LOAD_PALETTE_INDEX (DACIndex, DACR, DACG, DACB);
        }

    } /* end of switch */

} /* end of vASTLoadPalette */

void
ASTDisplayPowerManagementSet(ScrnInfoPtr pScrn, int PowerManagementMode, int flags)
{
   ASTRecPtr pAST;
   UCHAR SEQ01, CRB6;
   ULONG ulData, ulTemp;

   pAST = ASTPTR(pScrn);
   SEQ01=CRB6=0;
   ulData = 0;

   vASTOpenKey(pScrn);

   switch (PowerManagementMode) {
   case DPMSModeOn:
      /* Screen: On; HSync: On, VSync: On */
      SEQ01 = 0x00;
      CRB6 = 0x00;
      ulData = 0x00000000;
      break;
   case DPMSModeStandby:
      /* Screen: Off; HSync: Off, VSync: On */
      SEQ01 = 0x20;
      CRB6  = 0x01;
      ulData = 0x00140000;
      break;
   case DPMSModeSuspend:
      /* Screen: Off; HSync: On, VSync: Off */
      SEQ01 = 0x20;
      CRB6  = 0x02;
      ulData = 0x00180000;
      break;
   case DPMSModeOff:
      /* Screen: Off; HSync: Off, VSync: Off */
      SEQ01 = 0x20;
      CRB6  = 0x03;
      ulData = 0x001C0000;
      break;
   }

   if (PowerManagementMode != DPMSModeOn)
   {  /* 3rd Tx */
      if (pAST->jTxChipType == Tx_DP501) SetDP501VideoOutput(pScrn, 0);
   }

   if (pAST->jChipType == AST1180)
   {
       ReadAST1180SOC(AST1180_GFX_BASE + AST1180_VGA1_CTRL, ulTemp);
       ulTemp &= 0xFFE3FFFF;
       ulTemp |= ulData;
       WriteAST1180SOC(AST1180_GFX_BASE + AST1180_VGA1_CTRL, ulTemp);
   }
   else
   {
       SetIndexRegMask(SEQ_PORT,0x01, 0xDF, SEQ01);
       SetIndexRegMask(CRTC_PORT,0xB6, 0xFC, CRB6);
   }

   if (PowerManagementMode == DPMSModeOn)
   {  /* 3rd Tx */
      if (pAST->jTxChipType == Tx_DP501) SetDP501VideoOutput(pScrn, 1);
   }
}


#ifndef I2C_BASE
#define I2C_BASE			0x1e780000
#endif
#define I2C_OFFSET			(0xA000 + 0x40 * 4)	/* port4 */
#define I2C_DEVICEADDR		0x0A0				/* slave addr */

#define I2C_BASE_AST1180	0x80fc0000
#define I2C_OFFSET_AS1180	(0xB000 + 0x40 * 2)	/* port2 */
#define I2C_DEVICEADDR_AST1180	0x0A0			/* slave addr */

Bool
ASTGetVGA2EDID(ScrnInfoPtr pScrn, unsigned char *pEDIDBuffer)
{
    ASTRecPtr pAST = ASTPTR(pScrn);
    ULONG i, ulData;
    UCHAR *pjEDID;
    ULONG base, deviceaddr;
    UCHAR *offset;

    pjEDID = pEDIDBuffer;

    if (pAST->jChipType == AST1180)
    {
        base   = I2C_BASE_AST1180;
        offset = pAST->MMIOVirtualAddr + 0x10000 + I2C_OFFSET_AS1180;
        deviceaddr = I2C_DEVICEADDR_AST1180;
    }
    else
    {
        base   = I2C_BASE;
        offset = pAST->MMIOVirtualAddr + 0x10000 + I2C_OFFSET;
        deviceaddr = I2C_DEVICEADDR;

        /* SCU settings */
        *(ULONG *) (pAST->MMIOVirtualAddr + 0xF004) = 0x1e6e0000;
        *(ULONG *) (pAST->MMIOVirtualAddr + 0xF000) = 0x1;
        usleep(10000);

        *(ULONG *) (pAST->MMIOVirtualAddr + 0x12000) = 0x1688A8A8;
        ulData = *(ULONG *) (pAST->MMIOVirtualAddr + 0x12004);
        ulData &= 0xfffffffb;
        *(ULONG *) (pAST->MMIOVirtualAddr + 0x12004) = ulData;
        usleep(10000);
    }

    /* I2C settings */
    *(ULONG *) (pAST->MMIOVirtualAddr + 0xF004) = base;
    *(ULONG *) (pAST->MMIOVirtualAddr + 0xF000) = 0x1;
    usleep(10000);

    /* I2C Start */
    *(ULONG *) (offset + 0x00) = 0x0;
    *(ULONG *) (offset + 0x04) = 0x77777355;
    *(ULONG *) (offset + 0x08) = 0x0;
    *(ULONG *) (offset + 0x10) = 0xffffffff;
    *(ULONG *) (offset + 0x00) = 0x1;
    *(ULONG *) (offset + 0x0C) = 0xAF;
    *(ULONG *) (offset + 0x20) = deviceaddr;
    *(ULONG *) (offset + 0x14) = 0x03;
    do {
        ulData = *(volatile ULONG *) (offset + 0x10);
    } while (!(ulData & 0x03));
    if (ulData & 0x02)				/* NACK */
        return (FALSE);
    *(ULONG *) (offset + 0x10) = 0xffffffff;
    *(ULONG *) (offset + 0x20) = (ULONG) 0;	/* Offset */
    *(ULONG *) (offset + 0x14) = 0x02;
    do {
        ulData = *(volatile ULONG *) (offset + 0x10);
    } while (!(ulData & 0x01));
    *(ULONG *) (offset + 0x10) = 0xffffffff;
    *(ULONG *) (offset + 0x20) = deviceaddr + 1;
    *(ULONG *) (offset + 0x14) = 0x03;
    do {
        ulData = *(volatile ULONG *) (offset + 0x10);
    } while (!(ulData & 0x01));

    /* I2C Read */
    for (i=0; i<127; i++)
    {
        *(ULONG *) (offset + 0x10) = 0xffffffff;
        *(ULONG *) (offset + 0x0C) |= 0x10;
        *(ULONG *) (offset + 0x14) = 0x08;
        do {
            ulData = *(volatile ULONG *) (offset + 0x10);
        } while (!(ulData & 0x04));
        *(ULONG *) (offset + 0x10) = 0xffffffff;
        *(UCHAR *) (pjEDID++) = (UCHAR) ((*(ULONG *) (offset + 0x20) & 0xFF00) >> 8);
    }

    /* Read Last Byte */
    *(ULONG *) (offset + 0x10) = 0xffffffff;
    *(ULONG *) (offset + 0x0C) |= 0x10;
    *(ULONG *) (offset + 0x14) = 0x18;
    do {
        ulData = *(volatile ULONG *) (offset + 0x10);
    } while (!(ulData & 0x04));
    *(ULONG *) (offset + 0x10) = 0xffffffff;
    *(UCHAR *) (pjEDID++) = (UCHAR) ((*(ULONG *) (offset + 0x20) & 0xFF00) >> 8);

    /* I2C Stop	 */
    *(ULONG *) (offset + 0x10) = 0xffffffff;
    *(ULONG *) (offset + 0x14) = 0x20;
    do {
        ulData = *(volatile ULONG *) (offset + 0x10);
    } while (!(ulData & 0x10));
    *(ULONG *) (offset + 0x0C) &= 0xffffffef;
    *(ULONG *) (offset + 0x10) = 0xffffffff;

    return (TRUE);

} /* ASTGetVGA2EDID */

/* Init VGA */
Bool bASTIsVGAEnabled(ScrnInfoPtr pScrn)
{
    ASTRecPtr pAST;
    UCHAR ch;
    ULONG ulData;

    pAST = ASTPTR(pScrn);

    if (pAST->jChipType == AST1180)
    {
        WriteAST1180SOC(AST1180_MMC_BASE+0x00, 0xFC600309);	/* unlock */
        ReadAST1180SOC(AST1180_MMC_BASE+0x08, ulData);
        return (ulData);
    }
    else
    {
        ch = inb(pAST->RelocateIO + 0x43);

        if (ch == 0x01)
        {
            outw(pAST->RelocateIO + 0x54, 0xa880);
            outb(pAST->RelocateIO + 0x54, 0xb6);
	    ch = inb(pAST->RelocateIO + 0x55);

            return (ch & 0x04);
        }
    }

    return (0);
}

static void vEnableVGA(ScrnInfoPtr pScrn)
{
    ASTRecPtr pAST;

    pAST = ASTPTR(pScrn);

    SetReg(VGA_ENABLE_PORT, 0x01);
    SetReg(MISC_PORT_WRITE, 0x01);

}

static UCHAR ExtRegInfo[] = {
    0x0F,
    0x04,
    0x1C,
    0xFF
};

static UCHAR ExtRegInfo_AST2300A0[] = {
    0x0F,
    0x04,
    0x1C,
    0xFF
};

static UCHAR ExtRegInfo_AST2300[] = {
    0x0F,
    0x04,
    0x1F,
    0xFF
};

static void vSetDefExtReg(ScrnInfoPtr pScrn)
{
    ASTRecPtr pAST;
    UCHAR i, jIndex, jReg, *pjExtRegInfo;

    pAST = ASTPTR(pScrn);

    /* Reset Scratch */
    for (i=0x81; i<=0x8F; i++)
    {
        SetIndexReg(CRTC_PORT, i, 0x00);
    }

    /* Set Ext. Reg */
    if ((pAST->jChipType == AST2300) || (pAST->jChipType == AST2400) || (pAST->jChipType == AST2500))
    {
       if (PCI_DEV_REVISION(pAST->PciInfo) > 0x20)
           pjExtRegInfo = ExtRegInfo_AST2300;
       else
           pjExtRegInfo = ExtRegInfo_AST2300A0;
    }
    else
        pjExtRegInfo = ExtRegInfo;

    jIndex = 0xA0;
    while (*(UCHAR *) (pjExtRegInfo) != 0xFF)
    {
        SetIndexRegMask(CRTC_PORT,jIndex, 0x00, *(UCHAR *) (pjExtRegInfo));
        jIndex++;
        pjExtRegInfo++;
    }

    /* disable standard IO/MEM decode if secondary */
    if (!xf86IsPrimaryPci(pAST->PciInfo))
        SetIndexRegMask(CRTC_PORT,0xA1, 0xFF, 0x03);

    /* Set Ext. Default */
    SetIndexRegMask(CRTC_PORT,0x8C, 0x00, 0x01);
    SetIndexRegMask(CRTC_PORT,0xB7, 0x00, 0x00);

    /* Enable RAMDAC for A1, ycchen@113005 */
    jReg = 0x04;
    if ((pAST->jChipType == AST2300) || (pAST->jChipType == AST2400) || (pAST->jChipType == AST2500))
        jReg |= 0x20;
    SetIndexRegMask(CRTC_PORT,0xB6, 0xFF, jReg);

}

static void vSetDefVCLK(ScrnInfoPtr pScrn)
{
    ASTRecPtr pAST = ASTPTR(pScrn);

    if ((pAST->jChipType == AST2500) && (PCI_DEV_REVISION(pAST->PciInfo) == 0x40))
    {
        SetIndexRegMask(CRTC_PORT, 0xbc, 0x00, 0x40);
        SetIndexRegMask(CRTC_PORT, 0xbd, 0x00, 0x38);
        SetIndexRegMask(CRTC_PORT, 0xbe, 0x00, 0x3a);
        SetIndexRegMask(CRTC_PORT, 0xbf, 0x00, 0x38);
        SetIndexRegMask(CRTC_PORT, 0xcf, 0x00, 0x70);
        SetIndexRegMask(CRTC_PORT, 0xb5, 0x00, 0xa8);
        SetIndexRegMask(CRTC_PORT, 0xbb, 0x00, 0x43);
    }
}

/*
 * AST2100/2150 DLL CBR Setting
 */
#define CBR_SIZE_AST2150             ((16 << 10) - 1)
#define CBR_PASSNUM_AST2150          5
#define CBR_THRESHOLD_AST2150        10
#define CBR_THRESHOLD2_AST2150       10
#define TIMEOUT_AST2150              5000000

#define CBR_PATNUM_AST2150           8

static ULONG pattern_AST2150[14] ={
0xFF00FF00,
0xCC33CC33,
0xAA55AA55,
0xFFFE0001,
0x683501FE,
0x0F1929B0,
0x2D0B4346,
0x60767F02,
0x6FBE36A6,
0x3A253035,
0x3019686D,
0x41C6167E,
0x620152BF,
0x20F050E0};

typedef struct _AST2150DRAMParam {
    UCHAR	*pjMMIOVirtualAddress;
} AST2150DRAMParam, *PAST2150DRAMParam;

static ULONG MMCTestBurst2_AST2150(PAST2150DRAMParam  param, ULONG datagen)
{
  ULONG data, timeout;
  UCHAR *mmiobase;

  mmiobase = param->pjMMIOVirtualAddress;

  MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0070, 0x00000001 | (datagen << 3));
  timeout = 0;
  do{
    data = MIndwm(mmiobase, 0x1E6E0070) & 0x40;
    if(++timeout > TIMEOUT_AST2150){
      MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
      return(-1);
    }
  }while(!data);
  MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0070, 0x00000003 | (datagen << 3));
  timeout = 0;
  do{
    data = MIndwm(mmiobase, 0x1E6E0070) & 0x40;
    if(++timeout > TIMEOUT_AST2150){
      MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
      return(-1);
    }
  }while(!data);
  data = (MIndwm(mmiobase, 0x1E6E0070) & 0x80) >> 7;
  MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
  return(data);
}

static ULONG MMCTestSingle2_AST2150(PAST2150DRAMParam  param, ULONG datagen)
{
  ULONG data, timeout;
  UCHAR *mmiobase;

  mmiobase = param->pjMMIOVirtualAddress;

  MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0070, 0x00000005 | (datagen << 3));
  timeout = 0;
  do{
    data = MIndwm(mmiobase, 0x1E6E0070) & 0x40;
    if(++timeout > TIMEOUT_AST2150){
      MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
      return(-1);
    }
  }while(!data);
  data = (MIndwm(mmiobase, 0x1E6E0070) & 0x80) >> 7;
  MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
  return(data);
}

static int CBRTest_AST2150(PAST2150DRAMParam  param)
{
  if(MMCTestBurst2_AST2150(param, 0) ) return(0);
  if(MMCTestBurst2_AST2150(param, 1) ) return(0);
  if(MMCTestBurst2_AST2150(param, 2) ) return(0);
  if(MMCTestBurst2_AST2150(param, 3) ) return(0);
  if(MMCTestBurst2_AST2150(param, 4) ) return(0);
  if(MMCTestBurst2_AST2150(param, 5) ) return(0);
  if(MMCTestBurst2_AST2150(param, 6) ) return(0);
  if(MMCTestBurst2_AST2150(param, 7) ) return(0);
  return(1);
}

static int CBRScan_AST2150(PAST2150DRAMParam  param, int busw)
{
  ULONG patcnt, loop;
  UCHAR *mmiobase;

  mmiobase = param->pjMMIOVirtualAddress;

  for(patcnt = 0;patcnt < CBR_PATNUM_AST2150;patcnt++){
    MOutdwm(mmiobase, 0x1E6E007C, pattern_AST2150[patcnt]);
    for(loop = 0;loop < CBR_PASSNUM_AST2150;loop++){
      if(CBRTest_AST2150(param)){
        break;
      }
    }
    if(loop == CBR_PASSNUM_AST2150){
      return(0);
    }
  }
  return(1);

}

static void CBRDLLI_AST2150(PAST2150DRAMParam  param, int busw)
{
  ULONG dllmin[4], dllmax[4], dlli, data, passcnt;
  UCHAR *mmiobase;

  mmiobase = param->pjMMIOVirtualAddress;

  CBR_START:
  dllmin[0] = dllmin[1] = dllmin[2] = dllmin[3] = 0xff;
  dllmax[0] = dllmax[1] = dllmax[2] = dllmax[3] = 0x0;
  passcnt = 0;
  MOutdwm(mmiobase, 0x1E6E0074, CBR_SIZE_AST2150);
  for(dlli = 0;dlli < 100;dlli++){
    MOutdwm(mmiobase, 0x1E6E0068, dlli | (dlli << 8) | (dlli << 16) | (dlli << 24));
    data = CBRScan_AST2150(param, busw);
    if(data != 0){
      if(data & 0x1){
        if(dllmin[0] > dlli){
          dllmin[0] = dlli;
        }
        if(dllmax[0] < dlli){
          dllmax[0] = dlli;
        }
      }
      passcnt++;
    }else if(passcnt >= CBR_THRESHOLD_AST2150){
      break;
    }
  }
  if(dllmax[0] == 0 || (dllmax[0]-dllmin[0]) < CBR_THRESHOLD_AST2150){
    goto CBR_START;
  }
  dlli = dllmin[0] + (((dllmax[0] - dllmin[0]) * 7) >> 4);
  MOutdwm(mmiobase, 0x1E6E0068, dlli | (dlli << 8) | (dlli << 16) | (dlli << 24));
}

typedef struct _AST_DRAMStruct {

    USHORT 	Index;
    ULONG	Data;

} AST_DRAMStruct, *PAST_DRAMStruct;

static AST_DRAMStruct AST2000DRAMTableData[] = {
    { 0x0108, 0x00000000 },
    { 0x0120, 0x00004a21 },
    { 0xFF00, 0x00000043 },
    { 0x0000, 0xFFFFFFFF },
    { 0x0004, 0x00000089 },
    { 0x0008, 0x22331353 },
    { 0x000C, 0x0d07000b },
    { 0x0010, 0x11113333 },
    { 0x0020, 0x00110350 },
    { 0x0028, 0x1e0828f0 },
    { 0x0024, 0x00000001 },
    { 0x001C, 0x00000000 },
    { 0x0014, 0x00000003 },
    { 0xFF00, 0x00000043 },
    { 0x0018, 0x00000131 },
    { 0x0014, 0x00000001 },
    { 0xFF00, 0x00000043 },
    { 0x0018, 0x00000031 },
    { 0x0014, 0x00000001 },
    { 0xFF00, 0x00000043 },
    { 0x0028, 0x1e0828f1 },
    { 0x0024, 0x00000003 },
    { 0x002C, 0x1f0f28fb },
    { 0x0030, 0xFFFFFE01 },
    { 0xFFFF, 0xFFFFFFFF }
};

static AST_DRAMStruct AST1100DRAMTableData[] = {
    { 0x2000, 0x1688a8a8 },
    { 0x2020, 0x000041f0 },
    { 0xFF00, 0x00000043 },
    { 0x0000, 0xfc600309 },
    { 0x006C, 0x00909090 },
    { 0x0064, 0x00050000 },
    { 0x0004, 0x00000585 },
    { 0x0008, 0x0011030f },
    { 0x0010, 0x22201724 },
    { 0x0018, 0x1e29011a },
    { 0x0020, 0x00c82222 },
    { 0x0014, 0x01001523 },
    { 0x001C, 0x1024010d },
    { 0x0024, 0x00cb2522 },
    { 0x0038, 0xffffff82 },
    { 0x003C, 0x00000000 },
    { 0x0040, 0x00000000 },
    { 0x0044, 0x00000000 },
    { 0x0048, 0x00000000 },
    { 0x004C, 0x00000000 },
    { 0x0050, 0x00000000 },
    { 0x0054, 0x00000000 },
    { 0x0058, 0x00000000 },
    { 0x005C, 0x00000000 },
    { 0x0060, 0x032aa02a },
    { 0x0064, 0x002d3000 },
    { 0x0068, 0x00000000 },
    { 0x0070, 0x00000000 },
    { 0x0074, 0x00000000 },
    { 0x0078, 0x00000000 },
    { 0x007C, 0x00000000 },
    { 0x0034, 0x00000001 },
    { 0xFF00, 0x00000043 },
    { 0x002C, 0x00000732 },
    { 0x0030, 0x00000040 },
    { 0x0028, 0x00000005 },
    { 0x0028, 0x00000007 },
    { 0x0028, 0x00000003 },
    { 0x0028, 0x00000001 },
    { 0x000C, 0x00005a08 },
    { 0x002C, 0x00000632 },
    { 0x0028, 0x00000001 },
    { 0x0030, 0x000003c0 },
    { 0x0028, 0x00000003 },
    { 0x0030, 0x00000040 },
    { 0x0028, 0x00000003 },
    { 0x000C, 0x00005a21 },
    { 0x0034, 0x00007c03 },
    { 0x0120, 0x00004c41 },
    { 0xffff, 0xffffffff },
};

static AST_DRAMStruct AST2100DRAMTableData[] = {
    { 0x2000, 0x1688a8a8 },
    { 0x2020, 0x00004120 },
    { 0xFF00, 0x00000043 },
    { 0x0000, 0xfc600309 },
    { 0x006C, 0x00909090 },
    { 0x0064, 0x00070000 },
    { 0x0004, 0x00000489 },
    { 0x0008, 0x0011030f },
    { 0x0010, 0x32302926 },
    { 0x0018, 0x274c0122 },
    { 0x0020, 0x00ce2222 },
    { 0x0014, 0x01001523 },
    { 0x001C, 0x1024010d },
    { 0x0024, 0x00cb2522 },
    { 0x0038, 0xffffff82 },
    { 0x003C, 0x00000000 },
    { 0x0040, 0x00000000 },
    { 0x0044, 0x00000000 },
    { 0x0048, 0x00000000 },
    { 0x004C, 0x00000000 },
    { 0x0050, 0x00000000 },
    { 0x0054, 0x00000000 },
    { 0x0058, 0x00000000 },
    { 0x005C, 0x00000000 },
    { 0x0060, 0x0f2aa02a },
    { 0x0064, 0x003f3005 },
    { 0x0068, 0x02020202 },
    { 0x0070, 0x00000000 },
    { 0x0074, 0x00000000 },
    { 0x0078, 0x00000000 },
    { 0x007C, 0x00000000 },
    { 0x0034, 0x00000001 },
    { 0xFF00, 0x00000043 },
    { 0x002C, 0x00000942 },
    { 0x0030, 0x00000040 },
    { 0x0028, 0x00000005 },
    { 0x0028, 0x00000007 },
    { 0x0028, 0x00000003 },
    { 0x0028, 0x00000001 },
    { 0x000C, 0x00005a08 },
    { 0x002C, 0x00000842 },
    { 0x0028, 0x00000001 },
    { 0x0030, 0x000003c0 },
    { 0x0028, 0x00000003 },
    { 0x0030, 0x00000040 },
    { 0x0028, 0x00000003 },
    { 0x000C, 0x00005a21 },
    { 0x0034, 0x00007c03 },
    { 0x0120, 0x00005061 },
    { 0xffff, 0xffffffff },
};

static void vInitDRAMReg(ScrnInfoPtr pScrn)
{
    AST_DRAMStruct *pjDRAMRegInfo;
    ASTRecPtr pAST = ASTPTR(pScrn);
    ULONG i, ulTemp, ulData;
    UCHAR jReg;
    AST2150DRAMParam param;

    GetIndexRegMask(CRTC_PORT, 0xD0, 0xFF, jReg);

    if ((jReg & 0x80) == 0)			/* VGA only */
    {
    	if (pAST->jChipType == AST2000)
    	{
            pjDRAMRegInfo = AST2000DRAMTableData;

            *(ULONG *) (pAST->MMIOVirtualAddr + 0xF004) = 0x1e6e0000;
            *(ULONG *) (pAST->MMIOVirtualAddr + 0xF000) = 0x1;
            *(ULONG *) (pAST->MMIOVirtualAddr + 0x10100) = 0xa8;

            do {
               ;
            } while (*(volatile ULONG *) (pAST->MMIOVirtualAddr + 0x10100) != 0xa8);

        }
    	else	/* AST2100/1100 */
    	{
    	    if ((pAST->jChipType == AST2100) || (pAST->jChipType == AST2200))
                pjDRAMRegInfo = AST2100DRAMTableData;
    	    else
                pjDRAMRegInfo = AST1100DRAMTableData;

            *(ULONG *) (pAST->MMIOVirtualAddr + 0xF004) = 0x1e6e0000;
            *(ULONG *) (pAST->MMIOVirtualAddr + 0xF000) = 0x1;

            *(ULONG *) (pAST->MMIOVirtualAddr + 0x12000) = 0x1688A8A8;
            do {
               ;
            } while (*(volatile ULONG *) (pAST->MMIOVirtualAddr + 0x12000) != 0x01);

            *(ULONG *) (pAST->MMIOVirtualAddr + 0x10000) = 0xFC600309;
            do {
               ;
            } while (*(volatile ULONG *) (pAST->MMIOVirtualAddr + 0x10000) != 0x01);

        }

        while (pjDRAMRegInfo->Index != 0xFFFF)
        {
            if (pjDRAMRegInfo->Index == 0xFF00)			/* Delay function */
            {
            	for (i=0; i<15; i++)
        	    usleep(pjDRAMRegInfo->Data);
            }
            else if ( (pjDRAMRegInfo->Index == 0x0004) && (pAST->jChipType != AST2000) )
            {
            	ulData = pjDRAMRegInfo->Data;

            	if (pAST->jDRAMType == DRAMTYPE_1Gx16)
            	    ulData = 0x00000d89;
            	else if (pAST->jDRAMType == DRAMTYPE_1Gx32)
            	    ulData = 0x00000c8d;

                ulTemp = *(ULONG *) (pAST->MMIOVirtualAddr + 0x12070);
                ulTemp &= 0x0000000C;
                ulTemp <<= 2;
                *(ULONG *) (pAST->MMIOVirtualAddr + 0x10000 + pjDRAMRegInfo->Index) = (ulData | ulTemp);
            }
            else
            {
                *(ULONG *) (pAST->MMIOVirtualAddr + 0x10000 + pjDRAMRegInfo->Index) = pjDRAMRegInfo->Data;
            }
            pjDRAMRegInfo++;
        }

        /* AST2100/2150 DRAM Calibration, ycchen@021511 */
        ulData = *(ULONG *) (pAST->MMIOVirtualAddr + 0x10120);
        if (ulData == 0x5061)		/* 266MHz */
        {
            param.pjMMIOVirtualAddress = pAST->MMIOVirtualAddr;
            ulData = *(ULONG *) (pAST->MMIOVirtualAddr + 0x10004);
            if (ulData & 0x40)
                CBRDLLI_AST2150(&param, 16);		/* 16bits */
            else
                CBRDLLI_AST2150(&param, 32);            /* 32bits */
        }

        switch (pAST->jChipType)
        {
        case AST2000:
            *(ULONG *) (pAST->MMIOVirtualAddr + 0x10140) |= 0x40;
            break;

        case AST1100:
        case AST2100:
        case AST2200:
        case AST2150:
            ulTemp = *(ULONG *) (pAST->MMIOVirtualAddr + 0x1200c);
            *(ULONG *) (pAST->MMIOVirtualAddr + 0x1200c) = (ulTemp & 0xFFFFFFFD);

            *(ULONG *) (pAST->MMIOVirtualAddr + 0x12040) |= 0x40;
            break;
        }

    } /* Init DRAM */

    /* wait ready */
    do {
        GetIndexRegMask(CRTC_PORT, 0xD0, 0xFF, jReg);
    } while ((jReg & 0x40) == 0);

} /* vInitDRAMReg */

/*
 * AST2300 DRAM settings modules
 */
#define	DDR3		0
#define	DDR2		1

typedef struct _AST2300DRAMParam {
    UCHAR	*pjMMIOVirtualAddress;
    ULONG	DRAM_Type;
    ULONG	DRAM_ChipID;
    ULONG	DRAM_Freq;
    ULONG       VRAM_Size;
    ULONG	ODT;			/* 0/75/150 */
    ULONG	WODT;			/* 0/40/60/120 */
    ULONG	RODT;

    ULONG	DRAM_CONFIG;
    ULONG	REG_PERIOD;
    ULONG  	REG_MADJ;
    ULONG	REG_SADJ;
    ULONG	REG_MRS;
    ULONG	REG_EMRS;
    ULONG	REG_AC1;
    ULONG	REG_AC2;
    ULONG	REG_DQSIC;
    ULONG	REG_DRV;
    ULONG	REG_IOZ;
    ULONG	REG_DQIDLY;
    ULONG	REG_FREQ;
    ULONG       MADJ_MAX;
    ULONG       DLL2_FINETUNE_STEP;

} AST2300DRAMParam, *PAST2300DRAMParam;

/*
 * DQSI DLL CBR Setting
 */
#define CBR_SIZE0            ((1  << 10) - 1)
#define CBR_SIZE1            ((4  << 10) - 1)
#define CBR_SIZE2            ((64 << 10) - 1)
#define CBR_PASSNUM          5
#define CBR_PASSNUM2         5
#define CBR_THRESHOLD        10
#define CBR_THRESHOLD2       10
#define TIMEOUT              5000000
#define CBR_PATNUM           8

ULONG pattern[8] ={
0xFF00FF00,
0xCC33CC33,
0xAA55AA55,
0x88778877,
0x92CC4D6E,
0x543D3CDE,
0xF1E843C7,
0x7C61D253};

static int MMCTestBurst(PAST2300DRAMParam  param, ULONG datagen)
{
  ULONG data, timeout;
  UCHAR *mmiobase;

  mmiobase = param->pjMMIOVirtualAddress;

  MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0070, 0x000000C1 | (datagen << 3));
  timeout = 0;
  do{
    data = MIndwm(mmiobase, 0x1E6E0070) & 0x3000;
    if(data & 0x2000){
      return(0);
    }
    if(++timeout > TIMEOUT){
      MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
      return(0);
    }
  }while(!data);
  MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
  return(1);
}

static int MMCTestBurst2(PAST2300DRAMParam  param, ULONG datagen)
{
  ULONG data, timeout;
  UCHAR *mmiobase;

  mmiobase = param->pjMMIOVirtualAddress;

  MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0070, 0x00000041 | (datagen << 3));
  timeout = 0;
  do{
    data = MIndwm(mmiobase, 0x1E6E0070) & 0x1000;
    if(++timeout > TIMEOUT){
      MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
      return(-1);
    }
  }while(!data);
  data = MIndwm(mmiobase, 0x1E6E0078);
  data = (data | (data >> 16)) & 0xFFFF;
  MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
  return(data);
}

static int MMCTestSingle(PAST2300DRAMParam  param, ULONG datagen)
{
  ULONG data, timeout;
  UCHAR *mmiobase;

  mmiobase = param->pjMMIOVirtualAddress;

  MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0070, 0x000000C5 | (datagen << 3));
  timeout = 0;
  do{
    data = MIndwm(mmiobase, 0x1E6E0070) & 0x3000;
    if(data & 0x2000){
      return(0);
    }
    if(++timeout > TIMEOUT){
      MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
      return(0);
    }
  }while(!data);
  MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
  return(1);
}

static int MMCTestSingle2(PAST2300DRAMParam  param, ULONG datagen)
{
  ULONG data, timeout;
  UCHAR *mmiobase;

  mmiobase = param->pjMMIOVirtualAddress;

  MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0070, 0x00000005 | (datagen << 3));
  timeout = 0;
  do{
    data = MIndwm(mmiobase, 0x1E6E0070) & 0x1000;
    if(++timeout > TIMEOUT){
      MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
      return(-1);
    }
  }while(!data);
  data = MIndwm(mmiobase, 0x1E6E0078);
  data = (data | (data >> 16)) & 0xFFFF;
  MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
  return(data);
}

static int CBRTest(PAST2300DRAMParam  param)
{
  ULONG data;

  data  = MMCTestSingle2(param, 0);  if((data & 0xff) && (data & 0xff00)) return(0);
  data |= MMCTestBurst2(param, 00);  if((data & 0xff) && (data & 0xff00)) return(0);
  data |= MMCTestBurst2(param, 01);  if((data & 0xff) && (data & 0xff00)) return(0);
  data |= MMCTestBurst2(param, 02);  if((data & 0xff) && (data & 0xff00)) return(0);
  data |= MMCTestBurst2(param, 03);  if((data & 0xff) && (data & 0xff00)) return(0);
  data |= MMCTestBurst2(param, 04);  if((data & 0xff) && (data & 0xff00)) return(0);
  data |= MMCTestBurst2(param, 05);  if((data & 0xff) && (data & 0xff00)) return(0);
  data |= MMCTestBurst2(param, 06);  if((data & 0xff) && (data & 0xff00)) return(0);
  data |= MMCTestBurst2(param, 07);  if((data & 0xff) && (data & 0xff00)) return(0);
  if(!data)            return(3);
  else if(data & 0xff) return(2);

  return(1);
}

static int CBRScan(PAST2300DRAMParam  param)
{
  ULONG data, data2, patcnt, loop;
  UCHAR *mmiobase;

  mmiobase = param->pjMMIOVirtualAddress;

  data2 = 3;
  for(patcnt = 0;patcnt < CBR_PATNUM;patcnt++){
    MOutdwm(mmiobase, 0x1E6E007C, pattern[patcnt]);
    for(loop = 0;loop < CBR_PASSNUM2;loop++){
      if((data = CBRTest(param)) != 0){
        data2 &= data;
        if(!data2){
          return(0);
        }
        break;
      }
    }
    if(loop == CBR_PASSNUM2){
      return(0);
    }
  }
  return(data2);
}

static ULONG CBRTest2(PAST2300DRAMParam  param)
{
  ULONG data;

  data  = MMCTestBurst2(param, 0);  if(data == 0xffff) return(0);
  data |= MMCTestSingle2(param, 0); if(data == 0xffff) return(0);
  return(~data & 0xffff);
}

static ULONG CBRScan2(PAST2300DRAMParam  param)
{
  ULONG data, data2, patcnt, loop;
  UCHAR *mmiobase;

  mmiobase = param->pjMMIOVirtualAddress;

  data2 = 0xffff;
  for(patcnt = 0;patcnt < CBR_PATNUM;patcnt++){
    MOutdwm(mmiobase, 0x1E6E007C, pattern[patcnt]);
    for(loop = 0;loop < CBR_PASSNUM2;loop++){
      if((data = CBRTest2(param)) != 0){
        data2 &= data;
        if(!data2){
          return(0);
        }
        break;
      }
    }
    if(loop == CBR_PASSNUM2){
      return(0);
    }
  }
  return(data2);
}

static ULONG CBRTest3(PAST2300DRAMParam  param)
{
  if(!MMCTestBurst(param, 0)) return(0);
  if(!MMCTestSingle(param, 0)) return(0);
  return(1);
}

static ULONG CBRScan3(PAST2300DRAMParam  param)
{
  ULONG patcnt, loop;
  UCHAR *mmiobase;

  mmiobase = param->pjMMIOVirtualAddress;

  for(patcnt = 0;patcnt < CBR_PATNUM;patcnt++){
    MOutdwm(mmiobase, 0x1E6E007C, pattern[patcnt]);
    for(loop = 0;loop < 2;loop++){
      if(CBRTest3(param)){
        break;
      }
    }
    if(loop == 2){
      return(0);
    }
  }
  return(1);
}

static Bool finetuneDQI_L(PAST2300DRAMParam  param)
{
  ULONG gold_sadj[2], dllmin[16], dllmax[16], dlli, data, cnt, mask, passcnt, retry = 0;
  UCHAR *mmiobase;
  Bool status = FALSE;

  mmiobase = param->pjMMIOVirtualAddress;

  FINETUNE_START:
  for(cnt = 0;cnt < 16;cnt++){
    dllmin[cnt] = 0xff;
    dllmax[cnt] = 0x0;
  }
  passcnt = 0;
  for(dlli = 0;dlli < 76;dlli++){
    MOutdwm(mmiobase, 0x1E6E0068, 0x00001400 | (dlli << 16) | (dlli << 24));
    MOutdwm(mmiobase, 0x1E6E0074, CBR_SIZE1);
    data = CBRScan2(param);
    if(data != 0){
      mask = 0x00010001;
      for(cnt = 0;cnt < 16;cnt++){
        if(data & mask){
          if(dllmin[cnt] > dlli){
            dllmin[cnt] = dlli;
          }
          if(dllmax[cnt] < dlli){
            dllmax[cnt] = dlli;
          }
        }
        mask <<= 1;
      }
      passcnt++;
    }else if(passcnt >= CBR_THRESHOLD2){
      break;
    }
  }
  gold_sadj[0] = 0x0;
  passcnt = 0;
  for(cnt = 0;cnt < 16;cnt++){
    if((dllmax[cnt] > dllmin[cnt]) && ((dllmax[cnt] - dllmin[cnt]) >= CBR_THRESHOLD2)){
      gold_sadj[0] += dllmin[cnt];
      passcnt++;
    }
  }
  if (retry++ > 10)
      goto FINETUNE_DONE;
  if(passcnt != 16){
    goto FINETUNE_START;
  }
  status = TRUE;

FINETUNE_DONE:
  gold_sadj[0] = gold_sadj[0] >> 4;
  gold_sadj[1] = gold_sadj[0];

  data = 0;
  for(cnt = 0;cnt < 8;cnt++){
    data >>= 3;
    if((dllmax[cnt] > dllmin[cnt]) && ((dllmax[cnt] - dllmin[cnt]) >= CBR_THRESHOLD2)){
      dlli = dllmin[cnt];
      if(gold_sadj[0] >= dlli){
        dlli = ((gold_sadj[0] - dlli) * 19) >> 5;
        if(dlli > 3){
          dlli = 3;
        }
      }else{
        dlli = ((dlli - gold_sadj[0]) * 19) >> 5;
        if(dlli > 4){
          dlli = 4;
        }
        dlli = (8 - dlli) & 0x7;
      }
      data |= dlli << 21;
    }
  }
  MOutdwm(mmiobase, 0x1E6E0080, data);

  data = 0;
  for(cnt = 8;cnt < 16;cnt++){
    data >>= 3;
    if((dllmax[cnt] > dllmin[cnt]) && ((dllmax[cnt] - dllmin[cnt]) >= CBR_THRESHOLD2)){
      dlli = dllmin[cnt];
      if(gold_sadj[1] >= dlli){
        dlli = ((gold_sadj[1] - dlli) * 19) >> 5;
        if(dlli > 3){
          dlli = 3;
        }else{
          dlli = (dlli - 1) & 0x7;
        }
      }else{
        dlli = ((dlli - gold_sadj[1]) * 19) >> 5;
        dlli += 1;
        if(dlli > 4){
          dlli = 4;
        }
        dlli = (8 - dlli) & 0x7;
      }
      data |= dlli << 21;
    }
  }
  MOutdwm(mmiobase, 0x1E6E0084, data);

  return status;

} /* finetuneDQI_L */

static void finetuneDQSI(PAST2300DRAMParam  param)
{
  ULONG dlli, dqsip, dqidly;
  ULONG reg_mcr18, reg_mcr0c, passcnt[2], diff;
  ULONG g_dqidly, g_dqsip, g_margin, g_side;
  unsigned short pass[32][2][2];
  char tag[2][76];
  UCHAR *mmiobase;

  mmiobase = param->pjMMIOVirtualAddress;

  /* Disable DQI CBR */
  reg_mcr0c  = MIndwm(mmiobase, 0x1E6E000C);
  reg_mcr18  = MIndwm(mmiobase, 0x1E6E0018);
  reg_mcr18 &= 0x0000ffff;
  MOutdwm(mmiobase, 0x1E6E0018, reg_mcr18);

  for(dlli = 0;dlli < 76;dlli++){
    tag[0][dlli] = 0x0;
    tag[1][dlli] = 0x0;
  }
  for(dqidly = 0;dqidly < 32;dqidly++){
    pass[dqidly][0][0] = 0xff;
    pass[dqidly][0][1] = 0x0;
    pass[dqidly][1][0] = 0xff;
    pass[dqidly][1][1] = 0x0;
  }
  for(dqidly = 0;dqidly < 32;dqidly++){
    passcnt[0] = passcnt[1] = 0;
    for(dqsip = 0;dqsip < 2;dqsip++){
      MOutdwm(mmiobase, 0x1E6E000C, 0);
      MOutdwm(mmiobase, 0x1E6E0018, reg_mcr18 | (dqidly << 16) | (dqsip << 23));
      MOutdwm(mmiobase, 0x1E6E000C, reg_mcr0c);
      for(dlli = 0;dlli < 76;dlli++){
        MOutdwm(mmiobase, 0x1E6E0068, 0x00001300 | (dlli << 16) | (dlli << 24));
        MOutdwm(mmiobase, 0x1E6E0070, 0);
        MOutdwm(mmiobase, 0x1E6E0074, CBR_SIZE0);
        if(CBRScan3(param)){
          if(dlli == 0){
            break;
          }
          passcnt[dqsip]++;
          tag[dqsip][dlli] = 'P';
          if(dlli < pass[dqidly][dqsip][0]){
            pass[dqidly][dqsip][0] = (USHORT) dlli;
          }
          if(dlli > pass[dqidly][dqsip][1]){
            pass[dqidly][dqsip][1] = (USHORT) dlli;
          }
        }else if(passcnt[dqsip] >= 5){
          break;
        }else{
          pass[dqidly][dqsip][0] = 0xff;
          pass[dqidly][dqsip][1] = 0x0;
        }
      }
    }
    if(passcnt[0] == 0 && passcnt[1] == 0){
      dqidly++;
    }
  }
  /* Search margin */
  g_dqidly = g_dqsip = g_margin = g_side = 0;

  for(dqidly = 0;dqidly < 32;dqidly++){
    for(dqsip = 0;dqsip < 2;dqsip++){
      if(pass[dqidly][dqsip][0] > pass[dqidly][dqsip][1]){
        continue;
      }
      diff = pass[dqidly][dqsip][1] - pass[dqidly][dqsip][0];
      if((diff+2) < g_margin){
        continue;
      }
      passcnt[0] = passcnt[1] = 0;
      for(dlli = pass[dqidly][dqsip][0];dlli > 0  && tag[dqsip][dlli] != 0;dlli--,passcnt[0]++);
      for(dlli = pass[dqidly][dqsip][1];dlli < 76 && tag[dqsip][dlli] != 0;dlli++,passcnt[1]++);
      if(passcnt[0] > passcnt[1]){
        passcnt[0] = passcnt[1];
      }
      passcnt[1] = 0;
      if(passcnt[0] > g_side){
        passcnt[1] = passcnt[0] - g_side;
      }
      if(diff > (g_margin+1) && (passcnt[1] > 0 || passcnt[0] > 8)){
        g_margin = diff;
        g_dqidly = dqidly;
        g_dqsip  = dqsip;
        g_side   = passcnt[0];
      }else if(passcnt[1] > 1 && g_side < 8){
        if(diff > g_margin){
          g_margin = diff;
        }
        g_dqidly = dqidly;
        g_dqsip  = dqsip;
        g_side   = passcnt[0];
      }
    }
  }
  reg_mcr18 = reg_mcr18 | (g_dqidly << 16) | (g_dqsip << 23);
  MOutdwm(mmiobase, 0x1E6E0018, reg_mcr18);
} /* finetuneDQSI */

static Bool CBRDLL2(PAST2300DRAMParam  param)
{
  ULONG dllmin[2], dllmax[2], dlli, data, passcnt, retry=0;
  UCHAR *mmiobase;
  BOOL status = FALSE;

  mmiobase = param->pjMMIOVirtualAddress;

  finetuneDQSI(param);
  if (finetuneDQI_L(param) == FALSE)
      return status;

  CBR_START2:
  dllmin[0] = dllmin[1] = 0xff;
  dllmax[0] = dllmax[1] = 0x0;
  passcnt = 0;
  for(dlli = 0;dlli < 76;dlli++){
    MOutdwm(mmiobase, 0x1E6E0068, 0x00001300 | (dlli << 16) | (dlli << 24));
    MOutdwm(mmiobase, 0x1E6E0074, CBR_SIZE2);
    data = CBRScan(param);
    if(data != 0){
      if(data & 0x1){
        if(dllmin[0] > dlli){
          dllmin[0] = dlli;
        }
        if(dllmax[0] < dlli){
          dllmax[0] = dlli;
        }
      }
      if(data & 0x2){
        if(dllmin[1] > dlli){
          dllmin[1] = dlli;
        }
        if(dllmax[1] < dlli){
          dllmax[1] = dlli;
        }
      }
      passcnt++;
    }else if(passcnt >= CBR_THRESHOLD){
      break;
    }
  }
  if (retry++ > 10)
      goto CBR_DONE2;
  if(dllmax[0] == 0 || (dllmax[0]-dllmin[0]) < CBR_THRESHOLD){
    goto CBR_START2;
  }
  if(dllmax[1] == 0 || (dllmax[1]-dllmin[1]) < CBR_THRESHOLD){
    goto CBR_START2;
  }
  status = TRUE;

CBR_DONE2:
  dlli  = (dllmin[1] + dllmax[1]) >> 1;
  dlli <<= 8;
  dlli += (dllmin[0] + dllmax[0]) >> 1;
  MOutdwm(mmiobase, 0x1E6E0068, MIndwm(mmiobase, 0x1E720058) | (dlli << 16));

  return status;

} /* CBRDLL2 */

static void GetDDR2Info(PAST2300DRAMParam param)
{
  UCHAR *mmiobase;
  ULONG trap, TRAP_AC2, TRAP_MRS;

  mmiobase = param->pjMMIOVirtualAddress;
  MOutdwm(mmiobase, 0x1E6E2000, 0x1688A8A8);

  /* Ger trap info */
  trap = (MIndwm(mmiobase, 0x1E6E2070) >> 25) & 0x3;
  TRAP_AC2  = (trap << 20) | (trap << 16);
  TRAP_AC2 += 0x00110000;
  TRAP_MRS  = 0x00000040 | (trap << 4);


  param->REG_MADJ 	= 0x00034C4C;
  param->REG_SADJ	= 0x00001800;
  param->REG_DRV        = 0x000000F0;
  param->REG_PERIOD     = param->DRAM_Freq;
  param->RODT		= 0;

  switch(param->DRAM_Freq){
    case 264 : MOutdwm(mmiobase, 0x1E6E2020, 0x0130);
               param->WODT          = 0;
               param->REG_AC1       = 0x11101513;
               param->REG_AC2       = 0x78117011;
               param->REG_DQSIC     = 0x00000092;
               param->REG_MRS       = 0x00000842;
               param->REG_EMRS      = 0x00000000;
               param->REG_DRV       = 0x000000F0;
               param->REG_IOZ       = 0x00000034;
               param->REG_DQIDLY    = 0x0000005A;
               param->REG_FREQ      = 0x00004AC0;
               param->MADJ_MAX      = 138;
               param->DLL2_FINETUNE_STEP = 3;
               break;
    case 336 : MOutdwm(mmiobase, 0x1E6E2020, 0x0331);
               param->WODT          = 1;
               param->REG_AC1       = 0x22202613;
               param->REG_AC2       = 0xAA009016 | TRAP_AC2;
               param->REG_DQSIC     = 0x000000BA;
               param->REG_MRS       = 0x00000A02 | TRAP_MRS;
               param->REG_EMRS      = 0x00000040;
               param->REG_DRV       = 0x000000FA;
               param->REG_IOZ       = 0x00000013;
               param->REG_DQIDLY    = 0x00000074;
               param->REG_FREQ      = 0x00004DC0;
               param->MADJ_MAX      = 96;
               param->DLL2_FINETUNE_STEP = 3;

               switch (param->DRAM_ChipID)
               {
               case DRAMTYPE_512Mx16:
                   param->REG_AC2   = 0xAA009012 | TRAP_AC2;
                   break;
               default:
               case DRAMTYPE_1Gx16:
                   param->REG_AC2   = 0xAA009016 | TRAP_AC2;
                   break;
               case DRAMTYPE_2Gx16:
                   param->REG_AC2   = 0xAA009023 | TRAP_AC2;
                   break;
               case DRAMTYPE_4Gx16:
                   param->REG_AC2   = 0xAA00903B | TRAP_AC2;
                   break;
               }

               break;
    default:
    case 396 : MOutdwm(mmiobase, 0x1E6E2020, 0x03F1);
               param->WODT          = 1;
               param->RODT          = 0;
               param->REG_AC1       = 0x33302714;
               param->REG_AC2       = 0xCC00B01B | TRAP_AC2;
               param->REG_DQSIC     = 0x000000E2;
               param->REG_MRS       = 0x00000C02 | TRAP_MRS;
               param->REG_EMRS      = 0x00000040;
               param->REG_DRV       = 0x000000FA;
               param->REG_IOZ       = 0x00000034;
               param->REG_DQIDLY    = 0x00000089;
               param->REG_FREQ      = 0x00005040;
               param->MADJ_MAX      = 96;
               param->DLL2_FINETUNE_STEP = 4;

               switch (param->DRAM_ChipID)
               {
               case DRAMTYPE_512Mx16:
                   param->REG_AC2   = 0xCC00B016 | TRAP_AC2;
                   break;
               default:
               case DRAMTYPE_1Gx16:
                   param->REG_AC2   = 0xCC00B01B | TRAP_AC2;
                   break;
               case DRAMTYPE_2Gx16:
                   param->REG_AC2   = 0xCC00B02B | TRAP_AC2;
                   break;
               case DRAMTYPE_4Gx16:
                   param->REG_AC2   = 0xCC00B03F | TRAP_AC2;
                   break;
               }

               break;

    case 408 : MOutdwm(mmiobase, 0x1E6E2020, 0x01F0);
               param->WODT          = 1;
               param->RODT          = 0;
               param->REG_AC1       = 0x33302714;
               param->REG_AC2       = 0xCC00B01B | TRAP_AC2;
               param->REG_DQSIC     = 0x000000E2;
               param->REG_MRS       = 0x00000C02 | TRAP_MRS;
               param->REG_EMRS      = 0x00000040;
               param->REG_DRV       = 0x000000FA;
               param->REG_IOZ       = 0x00000034;
               param->REG_DQIDLY    = 0x00000089;
               param->REG_FREQ      = 0x000050C0;
               param->MADJ_MAX      = 96;
               param->DLL2_FINETUNE_STEP = 4;

               switch (param->DRAM_ChipID)
               {
               case DRAMTYPE_512Mx16:
                   param->REG_AC2   = 0xCC00B016 | TRAP_AC2;
                   break;
               default:
               case DRAMTYPE_1Gx16:
                   param->REG_AC2   = 0xCC00B01B | TRAP_AC2;
                   break;
               case DRAMTYPE_2Gx16:
                   param->REG_AC2   = 0xCC00B02B | TRAP_AC2;
                   break;
               case DRAMTYPE_4Gx16:
                   param->REG_AC2   = 0xCC00B03F | TRAP_AC2;
                   break;
               }

               break;
    case 456 : MOutdwm(mmiobase, 0x1E6E2020, 0x0230);
               param->WODT          = 0;
               param->REG_AC1       = 0x33302815;
               param->REG_AC2       = 0xCD44B01E;
               param->REG_DQSIC     = 0x000000FC;
               param->REG_MRS       = 0x00000E72;
               param->REG_EMRS      = 0x00000000;
               param->REG_DRV       = 0x00000000;
               param->REG_IOZ       = 0x00000034;
               param->REG_DQIDLY    = 0x00000097;
               param->REG_FREQ      = 0x000052C0;
               param->MADJ_MAX      = 88;
               param->DLL2_FINETUNE_STEP = 3;
               break;
    case 504 : MOutdwm(mmiobase, 0x1E6E2020, 0x0261);
               param->WODT          = 1;
               param->RODT          = 1;
               param->REG_AC1       = 0x33302815;
               param->REG_AC2       = 0xDE44C022;
               param->REG_DQSIC     = 0x00000117;
               param->REG_MRS       = 0x00000E72;
               param->REG_EMRS      = 0x00000040;
               param->REG_DRV       = 0x0000000A;
               param->REG_IOZ       = 0x00000045;
               param->REG_DQIDLY    = 0x000000A0;
               param->REG_FREQ      = 0x000054C0;
               param->MADJ_MAX      = 79;
               param->DLL2_FINETUNE_STEP = 3;
               break;
    case 528 : MOutdwm(mmiobase, 0x1E6E2020, 0x0120);
               param->WODT          = 1;
               param->RODT          = 1;
               param->REG_AC1       = 0x33302815;
               param->REG_AC2       = 0xEF44D024;
               param->REG_DQSIC     = 0x00000125;
               param->REG_MRS       = 0x00000E72;
               param->REG_EMRS      = 0x00000004;
               param->REG_DRV       = 0x000000F9;
               param->REG_IOZ       = 0x00000045;
               param->REG_DQIDLY    = 0x000000A7;
               param->REG_FREQ      = 0x000055C0;
               param->MADJ_MAX      = 76;
               param->DLL2_FINETUNE_STEP = 3;
               break;
    case 552 : MOutdwm(mmiobase, 0x1E6E2020, 0x02A1);
               param->WODT          = 1;
               param->RODT          = 1;
               param->REG_AC1       = 0x43402915;
               param->REG_AC2       = 0xFF44E025;
               param->REG_DQSIC     = 0x00000132;
               param->REG_MRS       = 0x00000E72;
               param->REG_EMRS      = 0x00000040;
               param->REG_DRV       = 0x0000000A;
               param->REG_IOZ       = 0x00000045;
               param->REG_DQIDLY    = 0x000000AD;
               param->REG_FREQ      = 0x000056C0;
               param->MADJ_MAX      = 76;
               param->DLL2_FINETUNE_STEP = 3;
               break;
    case 576 : MOutdwm(mmiobase, 0x1E6E2020, 0x0140);
               param->WODT          = 1;
               param->RODT          = 1;
               param->REG_AC1       = 0x43402915;
               param->REG_AC2       = 0xFF44E027;
               param->REG_DQSIC     = 0x0000013F;
               param->REG_MRS       = 0x00000E72;
               param->REG_EMRS      = 0x00000004;
               param->REG_DRV       = 0x000000F5;
               param->REG_IOZ       = 0x00000045;
               param->REG_DQIDLY    = 0x000000B3;
               param->REG_FREQ      = 0x000057C0;
               param->MADJ_MAX      = 76;
               param->DLL2_FINETUNE_STEP = 3;
               break;
  }

  switch (param->DRAM_ChipID)
  {
  case DRAMTYPE_512Mx16:
      param->DRAM_CONFIG = 0x100;
      break;
  default:
  case DRAMTYPE_1Gx16:
      param->DRAM_CONFIG = 0x121;
      break;
   case DRAMTYPE_2Gx16:
      param->DRAM_CONFIG = 0x122;
      break;
  case DRAMTYPE_4Gx16:
      param->DRAM_CONFIG = 0x123;
      break;
  }; /* switch size */

  switch (param->VRAM_Size)
  {
  default:
  case VIDEOMEM_SIZE_08M:
      param->DRAM_CONFIG |= 0x00;
      break;
  case VIDEOMEM_SIZE_16M:
      param->DRAM_CONFIG |= 0x04;
      break;
  case VIDEOMEM_SIZE_32M:
      param->DRAM_CONFIG |= 0x08;
      break;
  case VIDEOMEM_SIZE_64M:
      param->DRAM_CONFIG |= 0x0c;
      break;
  }

}

static void GetDDR3Info(PAST2300DRAMParam param)
{
  UCHAR *mmiobase;
  ULONG trap, TRAP_AC2, TRAP_MRS;

  mmiobase = param->pjMMIOVirtualAddress;
  MOutdwm(mmiobase, 0x1E6E2000, 0x1688A8A8);

  /* Ger trap info */
  trap = (MIndwm(mmiobase, 0x1E6E2070) >> 25) & 0x3;
  TRAP_AC2  = 0x00020000 + (trap << 16);
  TRAP_AC2 |= 0x00300000 +((trap & 0x2) << 19);
  TRAP_MRS  = 0x00000010 + (trap << 4);
  TRAP_MRS |=             ((trap & 0x2) << 18);

  param->REG_MADJ 	= 0x00034C4C;
  param->REG_SADJ	= 0x00001800;
  param->REG_DRV        = 0x000000F0;
  param->REG_PERIOD     = param->DRAM_Freq;
  param->RODT		= 0;

  switch(param->DRAM_Freq){
    case 336 : MOutdwm(mmiobase, 0x1E6E2020, 0x0331);
               param->WODT   	    = 0;
               param->REG_AC1       = 0x22202725;
               param->REG_AC2       = 0xAA007613 | TRAP_AC2;
               param->REG_DQSIC     = 0x000000BA;
               param->REG_MRS       = 0x04001400 | TRAP_MRS;
               param->REG_EMRS      = 0x00000000;
               param->REG_IOZ       = 0x00000023;
               param->REG_DQIDLY    = 0x00000074;
               param->REG_FREQ      = 0x00004DC0;
               param->MADJ_MAX      = 96;
               param->DLL2_FINETUNE_STEP = 3;

               switch (param->DRAM_ChipID)
               {
               default:
               case DRAMTYPE_512Mx16:
               case DRAMTYPE_1Gx16:
                   param->REG_AC2   = 0xAA007613 | TRAP_AC2;
                   break;
               case DRAMTYPE_2Gx16:
                   param->REG_AC2   = 0xAA00761c | TRAP_AC2;
                   break;
               case DRAMTYPE_4Gx16:
                   param->REG_AC2   = 0xAA007636 | TRAP_AC2;
                   break;
               }

               break;
    default:
    case 396 : MOutdwm(mmiobase, 0x1E6E2020, 0x03F1);
               param->WODT          = 1;
               param->REG_AC1       = 0x33302825;
               param->REG_AC2       = 0xCC009617 | TRAP_AC2;
               param->REG_DQSIC     = 0x000000E2;
               param->REG_MRS       = 0x04001600 | TRAP_MRS;
               param->REG_EMRS      = 0x00000000;
               param->REG_IOZ       = 0x00000023;
               param->REG_DRV       = 0x000000FA;
               param->REG_DQIDLY    = 0x00000089;
               param->REG_FREQ      = 0x00005040;
               param->MADJ_MAX      = 96;
               param->DLL2_FINETUNE_STEP = 4;

               switch (param->DRAM_ChipID)
               {
               default:
               case DRAMTYPE_512Mx16:
               case DRAMTYPE_1Gx16:
                   param->REG_AC2   = 0xCC009617 | TRAP_AC2;
                   break;
               case DRAMTYPE_2Gx16:
                   param->REG_AC2   = 0xCC009622 | TRAP_AC2;
                   break;
               case DRAMTYPE_4Gx16:
                   param->REG_AC2   = 0xCC00963F | TRAP_AC2;
                   break;
               }

               break;

    case 408 : MOutdwm(mmiobase, 0x1E6E2020, 0x01F0);
               param->WODT          = 1;
               param->REG_AC1       = 0x33302825;
               param->REG_AC2       = 0xCC009617 | TRAP_AC2;
               param->REG_DQSIC     = 0x000000E2;
               param->REG_MRS       = 0x04001600 | TRAP_MRS;
               param->REG_EMRS      = 0x00000000;
               param->REG_IOZ       = 0x00000023;
               param->REG_DRV       = 0x000000FA;
               param->REG_DQIDLY    = 0x00000089;
               param->REG_FREQ      = 0x000050C0;
               param->MADJ_MAX      = 96;
               param->DLL2_FINETUNE_STEP = 4;

               switch (param->DRAM_ChipID)
               {
               default:
               case DRAMTYPE_512Mx16:
               case DRAMTYPE_1Gx16:
                   param->REG_AC2   = 0xCC009617 | TRAP_AC2;
                   break;
               case DRAMTYPE_2Gx16:
                   param->REG_AC2   = 0xCC009622 | TRAP_AC2;
                   break;
               case DRAMTYPE_4Gx16:
                   param->REG_AC2   = 0xCC00963F | TRAP_AC2;
                   break;
               }

               break;
    case 456 : MOutdwm(mmiobase, 0x1E6E2020, 0x0230);
               param->WODT          = 0;
               param->REG_AC1       = 0x33302926;
               param->REG_AC2       = 0xCD44961A;
               param->REG_DQSIC     = 0x000000FC;
               param->REG_MRS       = 0x00081830;
               param->REG_EMRS      = 0x00000000;
               param->REG_IOZ       = 0x00000045;
               param->REG_DQIDLY    = 0x00000097;
               param->REG_FREQ      = 0x000052C0;
               param->MADJ_MAX      = 88;
               param->DLL2_FINETUNE_STEP = 4;
               break;
    case 504 : MOutdwm(mmiobase, 0x1E6E2020, 0x0270);
               param->WODT          = 1;
               param->REG_AC1       = 0x33302926;
               param->REG_AC2       = 0xDE44A61D;
               param->REG_DQSIC     = 0x00000117;
               param->REG_MRS       = 0x00081A30;
               param->REG_EMRS      = 0x00000000;
               param->REG_IOZ       = 0x070000BB;
               param->REG_DQIDLY    = 0x000000A0;
               param->REG_FREQ      = 0x000054C0;
               param->MADJ_MAX      = 79;
               param->DLL2_FINETUNE_STEP = 4;
               break;
    case 528 : MOutdwm(mmiobase, 0x1E6E2020, 0x0290);
               param->WODT          = 1;
               param->RODT          = 1;
               param->REG_AC1       = 0x33302926;
               param->REG_AC2       = 0xEF44B61E;
               param->REG_DQSIC     = 0x00000125;
               param->REG_MRS       = 0x00081A30;
               param->REG_EMRS      = 0x00000040;
               param->REG_DRV       = 0x000000F5;
               param->REG_IOZ       = 0x00000023;
               param->REG_DQIDLY    = 0x00000088;
               param->REG_FREQ      = 0x000055C0;
               param->MADJ_MAX      = 76;
               param->DLL2_FINETUNE_STEP = 3;
               break;
    case 576 : MOutdwm(mmiobase, 0x1E6E2020, 0x0140);
               param->REG_MADJ 	    = 0x00136868;
               param->REG_SADJ	    = 0x00004534;
               param->WODT          = 1;
               param->RODT          = 1;
               param->REG_AC1       = 0x33302A37;
               param->REG_AC2       = 0xEF56B61E;
               param->REG_DQSIC     = 0x0000013F;
               param->REG_MRS       = 0x00101A50;
               param->REG_EMRS      = 0x00000040;
               param->REG_DRV       = 0x000000FA;
               param->REG_IOZ       = 0x00000023;
               param->REG_DQIDLY    = 0x00000078;
               param->REG_FREQ      = 0x000057C0;
               param->MADJ_MAX      = 136;
               param->DLL2_FINETUNE_STEP = 3;
               break;
    case 600 : MOutdwm(mmiobase, 0x1E6E2020, 0x02E1);
               param->REG_MADJ 	    = 0x00136868;
               param->REG_SADJ	    = 0x00004534;
               param->WODT          = 1;
               param->RODT          = 1;
               param->REG_AC1       = 0x32302A37;
               param->REG_AC2       = 0xDF56B61F;
               param->REG_DQSIC     = 0x0000014D;
               param->REG_MRS       = 0x00101A50;
               param->REG_EMRS      = 0x00000004;
               param->REG_DRV       = 0x000000F5;
               param->REG_IOZ       = 0x00000023;
               param->REG_DQIDLY    = 0x00000078;
               param->REG_FREQ      = 0x000058C0;
               param->MADJ_MAX      = 132;
               param->DLL2_FINETUNE_STEP = 3;
               break;
    case 624 : MOutdwm(mmiobase, 0x1E6E2020, 0x0160);
               param->REG_MADJ 	    = 0x00136868;
               param->REG_SADJ	    = 0x00004534;
               param->WODT          = 1;
               param->RODT          = 1;
               param->REG_AC1       = 0x32302A37;
               param->REG_AC2       = 0xEF56B621;
               param->REG_DQSIC     = 0x0000015A;
               param->REG_MRS       = 0x02101A50;
               param->REG_EMRS      = 0x00000004;
               param->REG_DRV       = 0x000000F5;
               param->REG_IOZ       = 0x00000034;
               param->REG_DQIDLY    = 0x00000078;
               param->REG_FREQ      = 0x000059C0;
               param->MADJ_MAX      = 128;
               param->DLL2_FINETUNE_STEP = 3;
               break;
  } /* switch freq */

  switch (param->DRAM_ChipID)
  {
  case DRAMTYPE_512Mx16:
      param->DRAM_CONFIG = 0x130;
      break;
  default:
  case DRAMTYPE_1Gx16:
      param->DRAM_CONFIG = 0x131;
      break;
   case DRAMTYPE_2Gx16:
      param->DRAM_CONFIG = 0x132;
      break;
  case DRAMTYPE_4Gx16:
      param->DRAM_CONFIG = 0x133;
      break;
  }; /* switch size */

  switch (param->VRAM_Size)
  {
  default:
  case VIDEOMEM_SIZE_08M:
      param->DRAM_CONFIG |= 0x00;
      break;
  case VIDEOMEM_SIZE_16M:
      param->DRAM_CONFIG |= 0x04;
      break;
  case VIDEOMEM_SIZE_32M:
      param->DRAM_CONFIG |= 0x08;
      break;
  case VIDEOMEM_SIZE_64M:
      param->DRAM_CONFIG |= 0x0c;
      break;
  }

}

static void DDR2_Init(PAST2300DRAMParam param)
{
  ULONG data, data2, retry = 0;
  UCHAR *mmiobase;

  mmiobase = param->pjMMIOVirtualAddress;

DDR2_Init_Start:
  MOutdwm(mmiobase, 0x1E6E0000, 0xFC600309);
  MOutdwm(mmiobase, 0x1E6E0064, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0034, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0018, 0x00000100);
  MOutdwm(mmiobase, 0x1E6E0024, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0064, param->REG_MADJ);
  MOutdwm(mmiobase, 0x1E6E0068, param->REG_SADJ);
  usleep(10);
  MOutdwm(mmiobase, 0x1E6E0064, param->REG_MADJ | 0xC0000);
  usleep(10);

  MOutdwm(mmiobase, 0x1E6E0004, param->DRAM_CONFIG);
  MOutdwm(mmiobase, 0x1E6E0008, 0x90040f);
  MOutdwm(mmiobase, 0x1E6E0010, param->REG_AC1);
  MOutdwm(mmiobase, 0x1E6E0014, param->REG_AC2);
  MOutdwm(mmiobase, 0x1E6E0020, param->REG_DQSIC);
  MOutdwm(mmiobase, 0x1E6E0080, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0084, 0x00FFFFFF);
  MOutdwm(mmiobase, 0x1E6E0088, param->REG_DQIDLY);
  MOutdwm(mmiobase, 0x1E6E0018, 0x4000A130);
  MOutdwm(mmiobase, 0x1E6E0018, 0x00002330);
  MOutdwm(mmiobase, 0x1E6E0038, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0040, 0xFF808000);
  MOutdwm(mmiobase, 0x1E6E0044, 0x88848466);
  MOutdwm(mmiobase, 0x1E6E0048, 0x44440008);
  MOutdwm(mmiobase, 0x1E6E004C, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0050, 0x80000000);
  MOutdwm(mmiobase, 0x1E6E0050, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0054, 0);
  MOutdwm(mmiobase, 0x1E6E0060, param->REG_DRV);
  MOutdwm(mmiobase, 0x1E6E006C, param->REG_IOZ);
  MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0074, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0078, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E007C, 0x00000000);

  /* Wait MCLK2X lock to MCLK */
  do{
    data = MIndwm(mmiobase, 0x1E6E001C);
  }while(!(data & 0x08000000));
  data = MIndwm(mmiobase, 0x1E6E001C);
  data = (data >> 8) & 0xff;
  while((data & 0x08) || ((data & 0x7) < 2) || (data < 4)){
    data2 = (MIndwm(mmiobase, 0x1E6E0064) & 0xfff3ffff) + 4;
    if((data2 & 0xff) > param->MADJ_MAX){
      break;
    }
    MOutdwm(mmiobase, 0x1E6E0064, data2);
    if(data2 & 0x00100000){
      data2 = ((data2 & 0xff) >> 3) + 3;
    }else{
      data2 = ((data2 & 0xff) >> 2) + 5;
    }
    data = MIndwm(mmiobase, 0x1E6E0068) & 0xffff00ff;
    data2 += data & 0xff;
    data = data | (data2 << 8);
    MOutdwm(mmiobase, 0x1E6E0068, data);
    usleep(10);
    MOutdwm(mmiobase, 0x1E6E0064, MIndwm(mmiobase, 0x1E6E0064) | 0xC0000);
    usleep(10);
    data = MIndwm(mmiobase, 0x1E6E0018) & 0xfffff1ff;
    MOutdwm(mmiobase, 0x1E6E0018, data);
    data = data | 0x200;
    MOutdwm(mmiobase, 0x1E6E0018, data);
    do{
      data = MIndwm(mmiobase, 0x1E6E001C);
    }while(!(data & 0x08000000));

    data = MIndwm(mmiobase, 0x1E6E001C);
    data = (data >> 8) & 0xff;
  }
  MOutdwm(mmiobase, 0x1E720058, MIndwm(mmiobase, 0x1E6E0068) & 0xffff);
  data = MIndwm(mmiobase, 0x1E6E0018) | 0xC00;
  MOutdwm(mmiobase, 0x1E6E0018, data);

  MOutdwm(mmiobase, 0x1E6E0034, 0x00000001);
  MOutdwm(mmiobase, 0x1E6E000C, 0x00000000);
  usleep(50);
  /* Mode Register Setting */
  MOutdwm(mmiobase, 0x1E6E002C, param->REG_MRS | 0x100);
  MOutdwm(mmiobase, 0x1E6E0030, param->REG_EMRS);
  MOutdwm(mmiobase, 0x1E6E0028, 0x00000005);
  MOutdwm(mmiobase, 0x1E6E0028, 0x00000007);
  MOutdwm(mmiobase, 0x1E6E0028, 0x00000003);
  MOutdwm(mmiobase, 0x1E6E0028, 0x00000001);
  MOutdwm(mmiobase, 0x1E6E000C, 0x00005C08);
  MOutdwm(mmiobase, 0x1E6E002C, param->REG_MRS);
  MOutdwm(mmiobase, 0x1E6E0028, 0x00000001);
  MOutdwm(mmiobase, 0x1E6E0030, param->REG_EMRS | 0x380);
  MOutdwm(mmiobase, 0x1E6E0028, 0x00000003);
  MOutdwm(mmiobase, 0x1E6E0030, param->REG_EMRS);
  MOutdwm(mmiobase, 0x1E6E0028, 0x00000003);

  MOutdwm(mmiobase, 0x1E6E000C, 0x7FFF5C01);
  data = 0;
  if(param->WODT){
    data = 0x500;
  }
  if(param->RODT){
    data = data | 0x3000 | ((param->REG_AC2 & 0x60000) >> 3);
  }
  MOutdwm(mmiobase, 0x1E6E0034, data | 0x3);
  MOutdwm(mmiobase, 0x1E6E0120, param->REG_FREQ);

  /* Calibrate the DQSI delay */
  if ((CBRDLL2(param)==FALSE) && (retry++ < 10))
      goto DDR2_Init_Start;


  /* ECC Memory Initialization */
#ifdef ECC
  MOutdwm(mmiobase, 0x1E6E007C, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0070, 0x221);
  do{
    data = MIndwm(mmiobase, 0x1E6E0070);
  }while(!(data & 0x00001000));
  MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0050, 0x80000000);
  MOutdwm(mmiobase, 0x1E6E0050, 0x00000000);
#endif
}

static void DDR3_Init(PAST2300DRAMParam param)
{
  ULONG data, data2, retry = 0;
  UCHAR *mmiobase;

  mmiobase = param->pjMMIOVirtualAddress;

DDR3_Init_Start:
  MOutdwm(mmiobase, 0x1E6E0000, 0xFC600309);
  MOutdwm(mmiobase, 0x1E6E0064, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0034, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0018, 0x00000100);
  MOutdwm(mmiobase, 0x1E6E0024, 0x00000000);
  usleep(10);
  MOutdwm(mmiobase, 0x1E6E0064, param->REG_MADJ);
  MOutdwm(mmiobase, 0x1E6E0068, param->REG_SADJ);
  usleep(10);
  MOutdwm(mmiobase, 0x1E6E0064, param->REG_MADJ | 0xC0000);
  usleep(10);

  MOutdwm(mmiobase, 0x1E6E0004, param->DRAM_CONFIG);
  MOutdwm(mmiobase, 0x1E6E0008, 0x90040f);
  MOutdwm(mmiobase, 0x1E6E0010, param->REG_AC1);
  MOutdwm(mmiobase, 0x1E6E0014, param->REG_AC2);
  MOutdwm(mmiobase, 0x1E6E0020, param->REG_DQSIC);
  MOutdwm(mmiobase, 0x1E6E0080, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0084, 0x00FFFFFF);
  MOutdwm(mmiobase, 0x1E6E0088, param->REG_DQIDLY);
  MOutdwm(mmiobase, 0x1E6E0018, 0x4000A170);
  MOutdwm(mmiobase, 0x1E6E0018, 0x00002370);
  MOutdwm(mmiobase, 0x1E6E0038, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0040, 0xFF444444);
  MOutdwm(mmiobase, 0x1E6E0044, 0x22222222);
  MOutdwm(mmiobase, 0x1E6E0048, 0x22222222);
  MOutdwm(mmiobase, 0x1E6E004C, 0x00000002);
  MOutdwm(mmiobase, 0x1E6E0050, 0x80000000);
  MOutdwm(mmiobase, 0x1E6E0050, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0054, 0);
  MOutdwm(mmiobase, 0x1E6E0060, param->REG_DRV);
  MOutdwm(mmiobase, 0x1E6E006C, param->REG_IOZ);
  MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0074, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0078, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E007C, 0x00000000);

  /* Wait MCLK2X lock to MCLK */
  do{
    data = MIndwm(mmiobase, 0x1E6E001C);
  }while(!(data & 0x08000000));
  data = MIndwm(mmiobase, 0x1E6E001C);
  data = (data >> 8) & 0xff;
  while((data & 0x08) || ((data & 0x7) < 2) || (data < 4)){
    data2 = (MIndwm(mmiobase, 0x1E6E0064) & 0xfff3ffff) + 4;
    if((data2 & 0xff) > param->MADJ_MAX){
      break;
    }
    MOutdwm(mmiobase, 0x1E6E0064, data2);
    if(data2 & 0x00100000){
      data2 = ((data2 & 0xff) >> 3) + 3;
    }else{
      data2 = ((data2 & 0xff) >> 2) + 5;
    }
    data = MIndwm(mmiobase, 0x1E6E0068) & 0xffff00ff;
    data2 += data & 0xff;
    data = data | (data2 << 8);
    MOutdwm(mmiobase, 0x1E6E0068, data);
    usleep(10);
    MOutdwm(mmiobase, 0x1E6E0064, MIndwm(mmiobase, 0x1E6E0064) | 0xC0000);
    usleep(10);
    data = MIndwm(mmiobase, 0x1E6E0018) & 0xfffff1ff;
    MOutdwm(mmiobase, 0x1E6E0018, data);
    data = data | 0x200;
    MOutdwm(mmiobase, 0x1E6E0018, data);
    do{
      data = MIndwm(mmiobase, 0x1E6E001C);
    }while(!(data & 0x08000000));

    data = MIndwm(mmiobase, 0x1E6E001C);
    data = (data >> 8) & 0xff;
  }
  MOutdwm(mmiobase, 0x1E720058, MIndwm(mmiobase, 0x1E6E0068) & 0xffff);
  data = MIndwm(mmiobase, 0x1E6E0018) | 0xC00;
  MOutdwm(mmiobase, 0x1E6E0018, data);

  MOutdwm(mmiobase, 0x1E6E0034, 0x00000001);
  MOutdwm(mmiobase, 0x1E6E000C, 0x00000040);
  usleep(50);
  /* Mode Register Setting */
  MOutdwm(mmiobase, 0x1E6E002C, param->REG_MRS | 0x100);
  MOutdwm(mmiobase, 0x1E6E0030, param->REG_EMRS);
  MOutdwm(mmiobase, 0x1E6E0028, 0x00000005);
  MOutdwm(mmiobase, 0x1E6E0028, 0x00000007);
  MOutdwm(mmiobase, 0x1E6E0028, 0x00000003);
  MOutdwm(mmiobase, 0x1E6E0028, 0x00000001);
  MOutdwm(mmiobase, 0x1E6E002C, param->REG_MRS);
  MOutdwm(mmiobase, 0x1E6E000C, 0x00005C08);
  MOutdwm(mmiobase, 0x1E6E0028, 0x00000001);

  MOutdwm(mmiobase, 0x1E6E000C, 0x00005C01);
  data = 0;
  if(param->WODT){
    data = 0x300;
  }
  if(param->RODT){
    data = data | 0x3000 | ((param->REG_AC2 & 0x60000) >> 3);
  }
  MOutdwm(mmiobase, 0x1E6E0034, data | 0x3);

  /* Calibrate the DQSI delay */
  if ((CBRDLL2(param)==FALSE) && (retry++ < 10))
      goto DDR3_Init_Start;

  MOutdwm(mmiobase, 0x1E6E0120, param->REG_FREQ);
  /* ECC Memory Initialization */
#ifdef ECC
  MOutdwm(mmiobase, 0x1E6E007C, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0070, 0x221);
  do{
    data = MIndwm(mmiobase, 0x1E6E0070);
  }while(!(data & 0x00001000));
  MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0050, 0x80000000);
  MOutdwm(mmiobase, 0x1E6E0050, 0x00000000);
#endif
}

static void vInitAST2300DRAMReg(ScrnInfoPtr pScrn)
{
    ASTRecPtr pAST = ASTPTR(pScrn);
    AST2300DRAMParam param;
    ULONG ulTemp;
    UCHAR jReg;

    GetIndexRegMask(CRTC_PORT, 0xD0, 0xFF, jReg);

    if ((jReg & 0x80) == 0)			/* VGA only */
    {
        *(ULONG *) (pAST->MMIOVirtualAddr + 0xF004) = 0x1e6e0000;
        *(ULONG *) (pAST->MMIOVirtualAddr + 0xF000) = 0x1;

        *(ULONG *) (pAST->MMIOVirtualAddr + 0x12000) = 0x1688A8A8;
        do {
           ;
        } while (*(volatile ULONG *) (pAST->MMIOVirtualAddr + 0x12000) != 0x01);

        *(ULONG *) (pAST->MMIOVirtualAddr + 0x10000) = 0xFC600309;
        do {
          ;
        } while (*(volatile ULONG *) (pAST->MMIOVirtualAddr + 0x10000) != 0x01);

    	/* Slow down CPU/AHB CLK in VGA only mode */
        ulTemp  = *(ULONG *) (pAST->MMIOVirtualAddr + 0x12008);
        ulTemp |= 0x73;
        *(ULONG *) (pAST->MMIOVirtualAddr + 0x12008) = ulTemp;

        param.pjMMIOVirtualAddress = pAST->MMIOVirtualAddr;
        param.DRAM_Type = DDR3;			/* DDR3 */
        ulTemp = MIndwm(param.pjMMIOVirtualAddress, 0x1E6E2070);
        if (ulTemp & 0x01000000)
            param.DRAM_Type = DDR2;		/* DDR2 */
        param.DRAM_ChipID = (ULONG) pAST->jDRAMType;
        param.DRAM_Freq = pAST->ulMCLK;
        param.VRAM_Size = pAST->ulVRAMSize;

        if (param.DRAM_Type == DDR3)
        {
            GetDDR3Info(&param);
            DDR3_Init(&param);
        }
        else
        {
            GetDDR2Info(&param);
            DDR2_Init(&param);
        }

        ulTemp  = MIndwm(param.pjMMIOVirtualAddress, 0x1E6E2040);
        MOutdwm(param.pjMMIOVirtualAddress, 0x1E6E2040, ulTemp | 0x40);
    }

    /* wait ready */
    do {
        GetIndexRegMask(CRTC_PORT, 0xD0, 0xFF, jReg);
    } while ((jReg & 0x40) == 0);

} /* vInitAST2300DRAMReg */

/*
 * AST2500 DRAM settings modules
 */
#define REGTBL_NUM           17
#define REGIDX_010           0
#define REGIDX_014           1
#define REGIDX_018           2
#define REGIDX_020           3
#define REGIDX_024           4
#define REGIDX_02C           5
#define REGIDX_030           6
#define REGIDX_214           7
#define REGIDX_2E0           8
#define REGIDX_2E4           9
#define REGIDX_2E8           10
#define REGIDX_2EC           11
#define REGIDX_2F0           12
#define REGIDX_2F4           13
#define REGIDX_2F8           14
#define REGIDX_RFC           15
#define REGIDX_PLL           16

ULONG ddr3_1600_timing_table[REGTBL_NUM] = {
0x64604D38,                  /* 0x010 */
0x29690599,                  /* 0x014 */
0x00000300,                  /* 0x018 */
0x00000000,                  /* 0x020 */
0x00000000,                  /* 0x024 */
0x02181E70,                  /* 0x02C */
0x00000040,                  /* 0x030 */
0x00000024,                  /* 0x214 */
0x02001300,                  /* 0x2E0 */
0x0E0000A0,                  /* 0x2E4 */
0x000E001B,                  /* 0x2E8 */
0x35B8C105,                  /* 0x2EC */
0x08090408,                  /* 0x2F0 */
0x9B000800,                  /* 0x2F4 */
0x0E400A00,                  /* 0x2F8 */
0x9971452F,                  /* tRFC  */
0x000071C1};                 /* PLL   */

ULONG ddr4_1600_timing_table[REGTBL_NUM] = {
0x63604E37,                  /* 0x010 */
0xE97AFA99,                  /* 0x014 */
0x00019000,                  /* 0x018 */
0x08000000,                  /* 0x020 */
0x00000400,                  /* 0x024 */
0x00000410,                  /* 0x02C */
0x00000101,                  /* 0x030 */
0x00000024,                  /* 0x214 */
0x03002900,                  /* 0x2E0 */
0x0E0000A0,                  /* 0x2E4 */
0x000E001C,                  /* 0x2E8 */
0x35B8C106,                  /* 0x2EC */
0x08080607,                  /* 0x2F0 */
0x9B000900,                  /* 0x2F4 */
0x0E400A00,                  /* 0x2F8 */
0x99714545,                  /* tRFC  */
0x000071C1};                 /* PLL   */

static int MMCTestBurst_AST2500(ScrnInfoPtr pScrn, ULONG datagen)
{
  ASTRecPtr pAST = ASTPTR(pScrn);
  UCHAR *mmiobase = pAST->MMIOVirtualAddr;
  ULONG data, timecnt;

  MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0070, 0x000000C1 | (datagen << 3));
  timecnt = 0;
  do{
    data = MIndwm(mmiobase, 0x1E6E0070) & 0x3000;
    if(data & 0x2000){
      return(0);
    }
    if(++timecnt > TIMEOUT){
      MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
      return(0);
    }
  }while(!data);
  MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
  return(1);
}

static int MMCTestSingle_AST2500(ScrnInfoPtr pScrn, ULONG datagen)
{
  ASTRecPtr pAST = ASTPTR(pScrn);
  UCHAR *mmiobase = pAST->MMIOVirtualAddr;
  ULONG data, timecnt;

  MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
  MOutdwm(mmiobase, 0x1E6E0070, 0x00000085 | (datagen << 3));
  timecnt = 0;
  do{
    data = MIndwm(mmiobase, 0x1E6E0070) & 0x3000;
    if(data & 0x2000){
      return(0);
    }
    if(++timecnt > TIMEOUT){
      MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
      return(0);
    }
  }while(!data);
  MOutdwm(mmiobase, 0x1E6E0070, 0x00000000);
  return(1);
}

static ULONG CBRTest_AST2500(ScrnInfoPtr pScrn)
{
  ASTRecPtr pAST = ASTPTR(pScrn);
  UCHAR *mmiobase = pAST->MMIOVirtualAddr;

  MOutdwm(mmiobase, 0x1E6E0074, 0x0000FFFF);
  MOutdwm(mmiobase, 0x1E6E007C, 0xFF00FF00);
  if(!MMCTestBurst_AST2500(pScrn, 0)) return(0);
  if(!MMCTestSingle_AST2500(pScrn, 0)) return(0);
  return(1);
}

static void DDR_Init_Common(ScrnInfoPtr pScrn)
{
  ASTRecPtr pAST = ASTPTR(pScrn);
  UCHAR *mmiobase = pAST->MMIOVirtualAddr;

  MOutdwm(mmiobase, 0x1E6E0034,0x00020080);
  MOutdwm(mmiobase, 0x1E6E0008,0x2003000F);
  MOutdwm(mmiobase, 0x1E6E0038,0x00000FFF);
  MOutdwm(mmiobase, 0x1E6E0040,0x88448844);
  MOutdwm(mmiobase, 0x1E6E0044,0x24422288);
  MOutdwm(mmiobase, 0x1E6E0048,0x22222222);
  MOutdwm(mmiobase, 0x1E6E004C,0x22222222);
  MOutdwm(mmiobase, 0x1E6E0050,0x80000000);
  MOutdwm(mmiobase, 0x1E6E0208,0x00000000);
  MOutdwm(mmiobase, 0x1E6E0218,0x00000000);
  MOutdwm(mmiobase, 0x1E6E0220,0x00000000);
  MOutdwm(mmiobase, 0x1E6E0228,0x00000000);
  MOutdwm(mmiobase, 0x1E6E0230,0x00000000);
  MOutdwm(mmiobase, 0x1E6E02A8,0x00000000);
  MOutdwm(mmiobase, 0x1E6E02B0,0x00000000);
  MOutdwm(mmiobase, 0x1E6E0240,0x86000000);
  MOutdwm(mmiobase, 0x1E6E0244,0x00008600);
  MOutdwm(mmiobase, 0x1E6E0248,0x80000000);
  MOutdwm(mmiobase, 0x1E6E024C,0x80808080);
}

static void Do_DDRPHY_Init(ScrnInfoPtr pScrn)
{
  ASTRecPtr pAST = ASTPTR(pScrn);
  UCHAR *mmiobase = pAST->MMIOVirtualAddr;
  ULONG data, pass, timecnt;

  pass = 0;
  MOutdwm(mmiobase, 0x1E6E0060,0x00000005);
  while(!pass){
    for(timecnt = 0;timecnt < TIMEOUT;timecnt++){
      data = MIndwm(mmiobase, 0x1E6E0060) & 0x1;
      if(!data){
        break;
      }
    }
    if(timecnt != TIMEOUT){
      data = MIndwm(mmiobase, 0x1E6E0300) & 0x000A0000;
      if(!data){
        pass = 1;
      }
    }
    if(!pass){
      MOutdwm(mmiobase, 0x1E6E0060,0x00000000);
      usleep(10); /* delay 10 us */
      MOutdwm(mmiobase, 0x1E6E0060,0x00000005);
    }
  }

  MOutdwm(mmiobase, 0x1E6E0060,0x00000006);
}

/******************************************************************************
 Check DRAM Size
 1Gb : 0x80000000 ~ 0x87FFFFFF
 2Gb : 0x80000000 ~ 0x8FFFFFFF
 4Gb : 0x80000000 ~ 0x9FFFFFFF
 8Gb : 0x80000000 ~ 0xBFFFFFFF
 *****************************************************************************/
static void Check_DRAM_Size(ScrnInfoPtr pScrn, ULONG tRFC)
{
  ASTRecPtr pAST = ASTPTR(pScrn);
  UCHAR *mmiobase = pAST->MMIOVirtualAddr;
  ULONG reg_04, reg_14;

  reg_04 = MIndwm(mmiobase, 0x1E6E0004) & 0xfffffffc;
  reg_14 = MIndwm(mmiobase, 0x1E6E0014) & 0xffffff00;

  MOutdwm(mmiobase, 0xA0100000, 0x41424344);
  MOutdwm(mmiobase, 0x90100000, 0x35363738);
  MOutdwm(mmiobase, 0x88100000, 0x292A2B2C);
  MOutdwm(mmiobase, 0x80100000, 0x1D1E1F10);

  /* Check 8Gbit */
  if(MIndwm(mmiobase, 0xA0100000) == 0x41424344){
    reg_04 |= 0x03;
    reg_14 |= (tRFC >> 24) & 0xFF;
  /* Check 4Gbit */
  }else if(MIndwm(mmiobase, 0x90100000) == 0x35363738){
    reg_04 |= 0x02;
    reg_14 |= (tRFC >> 16) & 0xFF;
  /* Check 2Gbit */
  }else if(MIndwm(mmiobase, 0x88100000) == 0x292A2B2C){
    reg_04 |= 0x01;
    reg_14 |= (tRFC >> 8) & 0xFF;
  }else{
    reg_14 |= tRFC & 0xFF;
  }
  MOutdwm(mmiobase, 0x1E6E0004, reg_04);
  MOutdwm(mmiobase, 0x1E6E0014, reg_14);
}

static void Enable_Cache(ScrnInfoPtr pScrn)
{
  ASTRecPtr pAST = ASTPTR(pScrn);
  UCHAR *mmiobase = pAST->MMIOVirtualAddr;
  ULONG reg_04, data;

  reg_04 = MIndwm(mmiobase, 0x1E6E0004);
  MOutdwm(mmiobase, 0x1E6E0004, reg_04 | 0x1000);

  do{
    data = MIndwm(mmiobase, 0x1E6E0004);
  }while(!(data & 0x80000));
  MOutdwm(mmiobase, 0x1E6E0004, reg_04 | 0x400);
}

static void Set_MPLL(ScrnInfoPtr pScrn)
{
  ASTRecPtr pAST = ASTPTR(pScrn);
  UCHAR *mmiobase = pAST->MMIOVirtualAddr;
  ULONG addr, data, param;

  /* Reset MMC */
  MOutdwm(mmiobase, 0x1E6E0000,0xFC600309);
  MOutdwm(mmiobase, 0x1E6E0034,0x00020080);
  for(addr = 0x1e6e0004;addr < 0x1e6e0090;){
    MOutdwm(mmiobase, addr, 0x0);
    addr += 4;
  }
  MOutdwm(mmiobase, 0x1E6E0034,0x00020000);

  MOutdwm(mmiobase, 0x1E6E2000, 0x1688A8A8);
  data = MIndwm(mmiobase, 0x1E6E2070) & 0x00800000;
  if(data){                  /* CLKIN = 25MHz */
    param = 0x930023E0;
  }else{					 /* CLKIN = 24MHz */
    param = 0x93002400;
  }
  MOutdwm(mmiobase, 0x1E6E2020, param);
  usleep(100);
}

static void DDR3_Init_AST2500(ScrnInfoPtr pScrn, ULONG *ddr_table)
{
  ASTRecPtr pAST = ASTPTR(pScrn);
  UCHAR *mmiobase = pAST->MMIOVirtualAddr;

  MOutdwm(mmiobase, 0x1E6E0004,0x00000303);
  MOutdwm(mmiobase, 0x1E6E0010,ddr_table[REGIDX_010]);
  MOutdwm(mmiobase, 0x1E6E0014,ddr_table[REGIDX_014]);
  MOutdwm(mmiobase, 0x1E6E0018,ddr_table[REGIDX_018]);
  MOutdwm(mmiobase, 0x1E6E0020,ddr_table[REGIDX_020]);          /* MODEREG4/6 */
  MOutdwm(mmiobase, 0x1E6E0024,ddr_table[REGIDX_024]);          /* MODEREG5 */
  MOutdwm(mmiobase, 0x1E6E002C,ddr_table[REGIDX_02C] | 0x100);  /* MODEREG0/2 */
  MOutdwm(mmiobase, 0x1E6E0030,ddr_table[REGIDX_030]);          /* MODEREG1/3 */

  /* DDR PHY Setting */
  MOutdwm(mmiobase, 0x1E6E0200,0x02492AAE);
  MOutdwm(mmiobase, 0x1E6E0204,0x00001001);
  MOutdwm(mmiobase, 0x1E6E020C,0x55E00B0B);
  MOutdwm(mmiobase, 0x1E6E0210,0x20000000);
  MOutdwm(mmiobase, 0x1E6E0214,ddr_table[REGIDX_214]);
  MOutdwm(mmiobase, 0x1E6E02E0,ddr_table[REGIDX_2E0]);
  MOutdwm(mmiobase, 0x1E6E02E4,ddr_table[REGIDX_2E4]);
  MOutdwm(mmiobase, 0x1E6E02E8,ddr_table[REGIDX_2E8]);
  MOutdwm(mmiobase, 0x1E6E02EC,ddr_table[REGIDX_2EC]);
  MOutdwm(mmiobase, 0x1E6E02F0,ddr_table[REGIDX_2F0]);
  MOutdwm(mmiobase, 0x1E6E02F4,ddr_table[REGIDX_2F4]);
  MOutdwm(mmiobase, 0x1E6E02F8,ddr_table[REGIDX_2F8]);
  MOutdwm(mmiobase, 0x1E6E0290,0x00100008);
  MOutdwm(mmiobase, 0x1E6E02C0,0x00000006);

  /* Controller Setting */
  MOutdwm(mmiobase, 0x1E6E0034,0x00020091);

  /* Wait DDR PHY init done */
  Do_DDRPHY_Init(pScrn);

  MOutdwm(mmiobase, 0x1E6E0120,ddr_table[REGIDX_PLL]);
  MOutdwm(mmiobase, 0x1E6E000C,0x42AA5C81);
  MOutdwm(mmiobase, 0x1E6E0034,0x0001AF93);

  Check_DRAM_Size(pScrn, ddr_table[REGIDX_RFC]);
  Enable_Cache(pScrn);
  MOutdwm(mmiobase, 0x1E6E001C,0x00000008);
  MOutdwm(mmiobase, 0x1E6E0038,0xFFFFFF00);
}

static void DDR4_Init_AST2500(ScrnInfoPtr pScrn, ULONG *ddr_table)
{
  ASTRecPtr pAST = ASTPTR(pScrn);
  UCHAR *mmiobase = pAST->MMIOVirtualAddr;
  ULONG data, data2, pass;
  ULONG ddr_vref, phy_vref;
  ULONG min_ddr_vref, min_phy_vref;
  ULONG max_ddr_vref, max_phy_vref;

  MOutdwm(mmiobase, 0x1E6E0004,0x00000313);
  MOutdwm(mmiobase, 0x1E6E0010,ddr_table[REGIDX_010]);
  MOutdwm(mmiobase, 0x1E6E0014,ddr_table[REGIDX_014]);
  MOutdwm(mmiobase, 0x1E6E0018,ddr_table[REGIDX_018]);
  MOutdwm(mmiobase, 0x1E6E0020,ddr_table[REGIDX_020]);          /* MODEREG4/6 */
  MOutdwm(mmiobase, 0x1E6E0024,ddr_table[REGIDX_024]);          /* MODEREG5 */
  MOutdwm(mmiobase, 0x1E6E002C,ddr_table[REGIDX_02C] | 0x100);  /* MODEREG0/2 */
  MOutdwm(mmiobase, 0x1E6E0030,ddr_table[REGIDX_030]);          /* MODEREG1/3 */

  /* DDR PHY Setting */
  MOutdwm(mmiobase, 0x1E6E0200,0x42492AAE);
  MOutdwm(mmiobase, 0x1E6E0204,0x09002000);
  MOutdwm(mmiobase, 0x1E6E020C,0x55E00B0B);
  MOutdwm(mmiobase, 0x1E6E0210,0x20000000);
  MOutdwm(mmiobase, 0x1E6E0214,ddr_table[REGIDX_214]);
  MOutdwm(mmiobase, 0x1E6E02E0,ddr_table[REGIDX_2E0]);
  MOutdwm(mmiobase, 0x1E6E02E4,ddr_table[REGIDX_2E4]);
  MOutdwm(mmiobase, 0x1E6E02E8,ddr_table[REGIDX_2E8]);
  MOutdwm(mmiobase, 0x1E6E02EC,ddr_table[REGIDX_2EC]);
  MOutdwm(mmiobase, 0x1E6E02F0,ddr_table[REGIDX_2F0]);
  MOutdwm(mmiobase, 0x1E6E02F4,ddr_table[REGIDX_2F4]);
  MOutdwm(mmiobase, 0x1E6E02F8,ddr_table[REGIDX_2F8]);
  MOutdwm(mmiobase, 0x1E6E0290,0x00100008);
  MOutdwm(mmiobase, 0x1E6E02C4,0x3C183C3C);
  MOutdwm(mmiobase, 0x1E6E02C8,0x00631E0E);

  /* Controller Setting */
  MOutdwm(mmiobase, 0x1E6E0034,0x0001A991);

  /* Train PHY Vref first */
  min_phy_vref = max_phy_vref = 0x0;
  pass = 0;
  MOutdwm(mmiobase, 0x1E6E02C0,0x00001C06);
  for(phy_vref = 0x40;phy_vref < 0x80;phy_vref++){
    MOutdwm(mmiobase, 0x1E6E000C,0x00000000);
    MOutdwm(mmiobase, 0x1E6E0060,0x00000000);
    MOutdwm(mmiobase, 0x1E6E02CC,phy_vref | (phy_vref << 8));
    /* Fire DFI Init */
    Do_DDRPHY_Init(pScrn);
    MOutdwm(mmiobase, 0x1E6E000C,0x00005C01);
    if(CBRTest_AST2500(pScrn)){
      pass++;
      data = MIndwm(mmiobase, 0x1E6E03D0);
      data2 = data >> 8;
      data  = data & 0xff;
      if(data > data2){
        data = data2;
      }

      if(max_phy_vref < data){
        max_phy_vref = data;
        min_phy_vref = phy_vref;
      }
    }else if(pass > 0){
      break;
    }
  }
  MOutdwm(mmiobase, 0x1E6E02CC,min_phy_vref | (min_phy_vref << 8));

  /* Train DDR Vref next */
  min_ddr_vref = 0xFF;
  max_ddr_vref = 0x0;
  pass = 0;
  for(ddr_vref = 0x00;ddr_vref < 0x40;ddr_vref++){
    MOutdwm(mmiobase, 0x1E6E000C,0x00000000);
    MOutdwm(mmiobase, 0x1E6E0060,0x00000000);
    MOutdwm(mmiobase, 0x1E6E02C0,0x00000006 | (ddr_vref << 8));
    /* Fire DFI Init */
    Do_DDRPHY_Init(pScrn);
    MOutdwm(mmiobase, 0x1E6E000C,0x00005C01);
    if(CBRTest_AST2500(pScrn)){
      pass++;
      if(min_ddr_vref > ddr_vref){
        min_ddr_vref = ddr_vref;
      }
      if(max_ddr_vref < ddr_vref){
        max_ddr_vref = ddr_vref;
      }
    }else if(pass != 0){
      break;
    }
  }
  MOutdwm(mmiobase, 0x1E6E000C,0x00000000);
  MOutdwm(mmiobase, 0x1E6E0060,0x00000000);
  ddr_vref = (min_ddr_vref + max_ddr_vref + 1) >> 1;
  MOutdwm(mmiobase, 0x1E6E02C0,0x00000006 | (ddr_vref << 8));

  /* Wait DDR PHY init done */
  Do_DDRPHY_Init(pScrn);

  MOutdwm(mmiobase, 0x1E6E0120,ddr_table[REGIDX_PLL]);
  MOutdwm(mmiobase, 0x1E6E000C,0x42AA5C81);
  MOutdwm(mmiobase, 0x1E6E0034,0x0001AF93);

  Check_DRAM_Size(pScrn, ddr_table[REGIDX_RFC]);
  Enable_Cache(pScrn);
  MOutdwm(mmiobase, 0x1E6E001C,0x00000008);
  MOutdwm(mmiobase, 0x1E6E0038,0xFFFFFF00);
}

static int DRAM_Init_AST2500(ScrnInfoPtr pScrn)
{
  ASTRecPtr pAST = ASTPTR(pScrn);
  UCHAR *mmiobase = pAST->MMIOVirtualAddr;
  ULONG data;

  Set_MPLL(pScrn);
  DDR_Init_Common(pScrn);
  data = MIndwm(mmiobase, 0x1E6E2070);
  if(data & 0x01000000){
    DDR4_Init_AST2500(pScrn, ddr4_1600_timing_table);
  }else{
    DDR3_Init_AST2500(pScrn, ddr3_1600_timing_table);
  }
  MOutdwm(mmiobase, 0x1E6E2040, MIndwm(mmiobase, 0x1E6E2040) | 0x41);
  /* Patch code */
  data = MIndwm(mmiobase, 0x1E6E200C) & 0xF9FFFFFF;
  MOutdwm(mmiobase, 0x1E6E200C, data | 0x10000000);
  return(1);
}

static void vInitAST2500DRAMReg(ScrnInfoPtr pScrn)
{
    ASTRecPtr pAST = ASTPTR(pScrn);
    ULONG ulTemp;
    UCHAR jReg;

    GetIndexRegMask(CRTC_PORT, 0xD0, 0xFF, jReg);

    if ((jReg & 0x80) == 0)			/* VGA only */
    {
        *(ULONG *) (pAST->MMIOVirtualAddr + 0xF004) = 0x1e6e0000;
        *(ULONG *) (pAST->MMIOVirtualAddr + 0xF000) = 0x1;

        *(ULONG *) (pAST->MMIOVirtualAddr + 0x12000) = 0x1688A8A8;
        do {
           ;
        } while (*(volatile ULONG *) (pAST->MMIOVirtualAddr + 0x12000) != 0x01);

        *(ULONG *) (pAST->MMIOVirtualAddr + 0x10000) = 0xFC600309;
        do {
          ;
        } while (*(volatile ULONG *) (pAST->MMIOVirtualAddr + 0x10000) != 0x01);

    	/* Slow down CPU/AHB CLK in VGA only mode */
        ulTemp  = *(ULONG *) (pAST->MMIOVirtualAddr + 0x12008);
        ulTemp |= 0x73;
        *(ULONG *) (pAST->MMIOVirtualAddr + 0x12008) = ulTemp;

		DRAM_Init_AST2500(pScrn);

        ulTemp  = MIndwm(pAST->MMIOVirtualAddr, 0x1E6E2040);
        MOutdwm(pAST->MMIOVirtualAddr, 0x1E6E2040, ulTemp | 0x40);
    }

    /* wait ready */
    do {
        GetIndexRegMask(CRTC_PORT, 0xD0, 0xFF, jReg);
    } while ((jReg & 0x40) == 0);

} /* vInitAST2500DRAMReg */

static void vGetDefaultSettings(ScrnInfoPtr pScrn)
{
    ASTRecPtr pAST = ASTPTR(pScrn);
    ULONG ulData;

    if ((pAST->jChipType == AST2300) || (pAST->jChipType == AST2400) || (pAST->jChipType == AST2500))
    {
        *(ULONG *) (pAST->MMIOVirtualAddr + 0xF004) = 0x1e6e0000;
        *(ULONG *) (pAST->MMIOVirtualAddr + 0xF000) = 0x1;
        ulData = *(ULONG *) (pAST->MMIOVirtualAddr + 0x12070);
        switch (ulData & 0x18000000)
        {
        case 0x00000000:
            pAST->jDRAMType = DRAMTYPE_512Mx16;
            break;
        case 0x08000000:
            pAST->jDRAMType = DRAMTYPE_1Gx16;
            break;
        case 0x10000000:
            pAST->jDRAMType = DRAMTYPE_2Gx16;
            break;
        case 0x18000000:
            pAST->jDRAMType = DRAMTYPE_4Gx16;
            break;
        }
    }
    else if ((pAST->jChipType == AST2100) || (pAST->jChipType == AST2200))
    {
        pAST->jDRAMType = DRAMTYPE_512Mx32;
    }
    else if ((pAST->jChipType == AST1100) || (pAST->jChipType == AST2150))
    {
        pAST->jDRAMType = DRAMTYPE_1Gx16;
    }

} /* vGetDefaultSettings */

static Bool InitDVO(ScrnInfoPtr pScrn)
{
    ASTRecPtr pAST = ASTPTR(pScrn);
    ULONG ulData;
    UCHAR jReg;

    *(ULONG *) (pAST->MMIOVirtualAddr + 0xF004)  = 0x1e6e0000;
    *(ULONG *) (pAST->MMIOVirtualAddr + 0xF000)  = 0x1;
    *(ULONG *) (pAST->MMIOVirtualAddr + 0x12000) = 0x1688A8A8;

    GetIndexRegMask(CRTC_PORT, 0xD0, 0xFF, jReg);
    if (!(jReg & 0x80))														/* Init SCU DVO Settings */
    {
        ulData  = *(ULONG *) (pAST->MMIOVirtualAddr + 0x12008);				/* delay phase */
        ulData &= 0xfffff8ff;
        ulData |= 0x00000500;
        *(ULONG *) (pAST->MMIOVirtualAddr + 0x12008) = ulData;

        if (pAST->jChipType == AST2300)
        {
            ulData  = *(ULONG *) (pAST->MMIOVirtualAddr + 0x12084);			/* multi-pins for DVO single-edge */
            ulData |= 0xfffe0000;
            *(ULONG *) (pAST->MMIOVirtualAddr + 0x12084) = ulData;

            ulData  = *(ULONG *) (pAST->MMIOVirtualAddr + 0x12088);			/* multi-pins for DVO single-edge */
            ulData |= 0x000fffff;
            *(ULONG *) (pAST->MMIOVirtualAddr + 0x12088) = ulData;

            ulData  = *(ULONG *) (pAST->MMIOVirtualAddr + 0x12090);			/* multi-pins for DVO single-edge */
            ulData &= 0xffffffcf;
            ulData |= 0x00000020;
            *(ULONG *) (pAST->MMIOVirtualAddr + 0x12090) = ulData;
        }
        else	/* AST2400 */
        {
            ulData  = *(ULONG *) (pAST->MMIOVirtualAddr + 0x12088);			/* multi-pins for DVO single-edge */
            ulData |= 0x30000000;
            *(ULONG *) (pAST->MMIOVirtualAddr + 0x12088) = ulData;

            ulData  = *(ULONG *) (pAST->MMIOVirtualAddr + 0x1208c);			/* multi-pins for DVO single-edge */
            ulData |= 0x000000cf;
            *(ULONG *) (pAST->MMIOVirtualAddr + 0x1208c) = ulData;

            ulData  = *(ULONG *) (pAST->MMIOVirtualAddr + 0x120a4);			/* multi-pins for DVO single-edge */
            ulData |= 0xffff0000;
            *(ULONG *) (pAST->MMIOVirtualAddr + 0x120a4) = ulData;

            ulData  = *(ULONG *) (pAST->MMIOVirtualAddr + 0x120a8);			/* multi-pins for DVO single-edge */
            ulData |= 0x0000000f;
            *(ULONG *) (pAST->MMIOVirtualAddr + 0x120a8) = ulData;

            ulData  = *(ULONG *) (pAST->MMIOVirtualAddr + 0x12094);			/* multi-pins for DVO single-edge */
            ulData |= 0x00000002;
            *(ULONG *) (pAST->MMIOVirtualAddr + 0x12094) = ulData;
        }
    }

    /* Force to DVO */
    ulData  = *(ULONG *) (pAST->MMIOVirtualAddr + 0x1202c);
    ulData &= 0xfffbffff;
	*(ULONG *) (pAST->MMIOVirtualAddr + 0x1202c) = ulData;

    /* Init VGA DVO Settings */
    SetIndexRegMask(CRTC_PORT, 0xA3, 0xCF, 0x80);							/* enable DVO, single-edge */

    return TRUE;
} /* InitDVO */

static void vInit3rdTX(ScrnInfoPtr pScrn)
{
    ASTRecPtr pAST = ASTPTR(pScrn);
    ULONG ulData;
    UCHAR jReg;

    /* Only support on AST2300/2400 */
    if ((pAST->jChipType == AST2300) || (pAST->jChipType == AST2400) || (pAST->jChipType == AST2500))
    {
        GetIndexRegMask(CRTC_PORT, 0xD1, 0xFF, jReg);      /* D[1]: DVO Enable */
        switch (jReg & 0x0E)	/* D[11:9] */
        {
	    case 0x04:				/* Sil164 */
            InitDVO(pScrn);
            break;
	    case 0x08:	    		/* DP501 with VBIOS launch FW */
	        LaunchM68K(pScrn);
	    case 0x0C:				/* DP501 with BMC launch FW */
            InitDVO(pScrn);
	        break;
	    default:    			/* Force to VGA */
            if (pAST->jTxChipType == Tx_Sil164)
                InitDVO(pScrn);
            else
            {
                *(ULONG *)(pAST->MMIOVirtualAddr + 0x12000) = 0x1688A8A8;
                ulData  = *(ULONG *) (pAST->MMIOVirtualAddr + 0x1202c);
                ulData &= 0xfffcffff;
	            *(ULONG *)(pAST->MMIOVirtualAddr) = ulData;
            }
        }
    }

} /* vInit3rdTX */

/*
 * Flags: 0: POST init
 *        1: resume from power management
 */
Bool ASTInitVGA(ScrnInfoPtr pScrn, ULONG Flags)
{
   ASTRecPtr pAST;
   uint32_t ulData;

   pAST = ASTPTR(pScrn);

   {
       /* Enable PCI */
       PCI_READ_LONG(pAST->PciInfo, &ulData, 0x04);
       ulData |= 0x03;
       PCI_WRITE_LONG(pAST->PciInfo, ulData, 0x04);

       /* Enable VGA */
       vEnableVGA(pScrn);

       vASTOpenKey(pScrn);
       vSetDefVCLK(pScrn);
       vSetDefExtReg(pScrn);

       if (Flags == 0)
           vGetDefaultSettings(pScrn);

       if (pAST->jChipType == AST2500)
           vInitAST2500DRAMReg(pScrn);
       else if ((pAST->jChipType == AST2300) || (pAST->jChipType == AST2400))
           vInitAST2300DRAMReg(pScrn);
       else
           vInitDRAMReg(pScrn);

       vInit3rdTX(pScrn);
   }

   return (TRUE);
} /* Init VGA */

/* Get EDID */
static void
I2CWriteClock(ASTRecPtr pAST, UCHAR data)
{
    UCHAR       ujCRB7, jtemp;
    ULONG	i;

    for (i=0;i<0x10000; i++)
    {
        ujCRB7 = ((data & 0x01) ? 0:1);			/* low active */
        SetIndexRegMask(CRTC_PORT, 0xB7, 0xFE, ujCRB7);
        GetIndexRegMask(CRTC_PORT, 0xB7, 0x01, jtemp);
        if (ujCRB7 == jtemp) break;
    }

}

static void
I2CWriteData(ASTRecPtr pAST, UCHAR data)
{
    UCHAR       volatile ujCRB7, jtemp;
    ULONG	i;

    for (i=0;i<0x1000; i++)
    {
        ujCRB7 = ((data & 0x01) ? 0:1) << 2;		/* low active */
        SetIndexRegMask(CRTC_PORT, 0xB7, 0xFB, ujCRB7);
        GetIndexRegMask(CRTC_PORT, 0xB7, 0x04, jtemp);
        if (ujCRB7 == jtemp) break;
    }

}

static Bool
I2CReadClock(ASTRecPtr pAST)
{
    UCHAR       volatile ujCRB7;

    GetIndexRegMask(CRTC_PORT, 0xB7, 0x10, ujCRB7);
    ujCRB7 >>= 4;

    return ((ujCRB7 & 0x01) ? 1:0);
}

static Bool
I2CReadData(ASTRecPtr pAST)
{
    UCHAR	volatile ujCRB7;

    GetIndexRegMask(CRTC_PORT, 0xB7, 0x20, ujCRB7);
    ujCRB7 >>= 5;

    return ((ujCRB7 & 0x01) ? 1:0);

}


static void
I2CDelay(ASTRecPtr pAST)
{
    for (ULONG i=0;i<150;i++)
        GetReg(SEQ_PORT);
}

static void
I2CStart(ASTRecPtr pAST)
{
    I2CWriteClock(pAST, 0x00);				/* Set Clk Low */
    I2CDelay(pAST);
    I2CWriteData(pAST, 0x01);				/* Set Data High */
    I2CDelay(pAST);
    I2CWriteClock(pAST, 0x01);				/* Set Clk High */
    I2CDelay(pAST);
    I2CWriteData(pAST, 0x00);				/* Set Data Low */
    I2CDelay(pAST);
    I2CWriteClock(pAST, 0x01);				/* Set Clk High */
    I2CDelay(pAST);
}

static void
I2CStop(ASTRecPtr pAST)
{
    I2CWriteClock(pAST, 0x00);				/* Set Clk Low */
    I2CDelay(pAST);
    I2CWriteData(pAST, 0x00);				/* Set Data Low */
    I2CDelay(pAST);
    I2CWriteClock(pAST, 0x01);				/* Set Clk High */
    I2CDelay(pAST);
    I2CWriteData(pAST, 0x01);				/* Set Data High */
    I2CDelay(pAST);
    I2CWriteClock(pAST, 0x01);				/* Set Clk High */
    I2CDelay(pAST);

}

static Bool
CheckACK(ASTRecPtr pAST)
{
    UCHAR Data;

    I2CWriteClock(pAST, 0x00);				/* Set Clk Low */
    I2CDelay(pAST);
    I2CWriteData(pAST, 0x01);				/* Set Data High */
    I2CDelay(pAST);
    I2CWriteClock(pAST, 0x01);				/* Set Clk High */
    I2CDelay(pAST);
    Data = (UCHAR) I2CReadData(pAST);			/* Set Data High */

    return ((Data & 0x01) ? 0:1);

}


static void
SendACK(ASTRecPtr pAST)
{

    I2CWriteClock(pAST, 0x00);				/* Set Clk Low */
    I2CDelay(pAST);
    I2CWriteData(pAST, 0x00);				/* Set Data low */
    I2CDelay(pAST);
    I2CWriteClock(pAST, 0x01);				/* Set Clk High */
    I2CDelay(pAST);

}

static void
SendNACK(ASTRecPtr pAST)
{

    I2CWriteClock(pAST, 0x00);				/* Set Clk Low */
    I2CDelay(pAST);
    I2CWriteData(pAST, 0x01);				/* Set Data high */
    I2CDelay(pAST);
    I2CWriteClock(pAST, 0x01);				/* Set Clk High */
    I2CDelay(pAST);

}

static void
SendI2CDataByte(ASTRecPtr pAST, UCHAR data)
{
    UCHAR jData;
    LONG i;

    for (i=7;i>=0;i--)
    {
        I2CWriteClock(pAST, 0x00);				/* Set Clk Low */
        I2CDelay(pAST);

    	jData = ((data >> i) & 0x01) ? 1:0;
        I2CWriteData(pAST, jData);				/* Set Data Low */
        I2CDelay(pAST);

        I2CWriteClock(pAST, 0x01);				/* Set Clk High */
        I2CDelay(pAST);
    }
}

static UCHAR
ReceiveI2CDataByte(ASTRecPtr pAST)
{
    UCHAR jData=0, jTempData;
    LONG i, j;

    for (i=7;i>=0;i--)
    {
        I2CWriteClock(pAST, 0x00);				/* Set Clk Low */
        I2CDelay(pAST);

        I2CWriteData(pAST, 0x01);				/* Set Data High */
        I2CDelay(pAST);

        I2CWriteClock(pAST, 0x01);				/* Set Clk High */
        I2CDelay(pAST);

        for (j=0; j<0x1000; j++)
        {
            if (I2CReadClock(pAST)) break;
        }

    	jTempData =  I2CReadData(pAST);
    	jData |= ((jTempData & 0x01) << i);

        I2CWriteClock(pAST, 0x0);				/* Set Clk Low */
        I2CDelay(pAST);
    }

    return ((UCHAR)jData);
}

Bool
ASTGetVGAEDID(ScrnInfoPtr pScrn, unsigned char *pEDIDBuffer)
{
    ASTRecPtr pAST;
    UCHAR *pjDstEDID;
    UCHAR jData;
    ULONG i;

    pAST = ASTPTR(pScrn);
    pjDstEDID = (UCHAR *) pEDIDBuffer;

    /* Force to DDC2 */
    I2CWriteClock(pAST, 0x01);				/* Set Clk High */
    I2CDelay(pAST);
    I2CWriteClock(pAST, 0x00);				/* Set Clk Low */
    I2CDelay(pAST);
    I2CWriteClock(pAST, 0x01);				/* Set Clk High */
    I2CDelay(pAST);

    /* Validate SCL */
    if (I2CReadClock(pAST) == 0)			/* chk SCL failed */
    {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO, "[GetVGAEDID] Check SCL Failed \n");
    	 return (FALSE);
    }

    I2CStart(pAST);

    SendI2CDataByte(pAST, 0xA0);
    if (!CheckACK(pAST))
    {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO, "[GetVGAEDID] Check ACK Failed \n");
    	 return (FALSE);
    }

    SendI2CDataByte(pAST, 0x00);
    if (!CheckACK(pAST))
    {
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "[GetVGAEDID] Check ACK Failed \n");
    	return (FALSE);
    }

    I2CStart(pAST);

    SendI2CDataByte(pAST, 0xA1);
    if (!CheckACK(pAST))
    {
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "[GetVGAEDID] Check ACK Failed \n");
    	return (FALSE);
    }

    for (i=0; i<127; i++)
    {
        jData = ReceiveI2CDataByte(pAST);
        SendACK(pAST);

        *pjDstEDID++ = jData;
    }

    jData = ReceiveI2CDataByte(pAST);
    SendNACK(pAST);
    *pjDstEDID = jData;

    I2CStop(pAST);

    return (TRUE);

} /* ASTGetVGAEDID */

Bool bASTInitAST1180(ScrnInfoPtr pScrn)
{
    ASTRecPtr pAST;
    uint32_t ulData;

    pAST = ASTPTR(pScrn);

    /* Enable PCI */
    PCI_READ_LONG(pAST->PciInfo, &ulData, 0x04);
    ulData |= 0x03;
    PCI_WRITE_LONG(pAST->PciInfo, ulData, 0x04);

    /* init DRAM if no F/W */
    /* TODO */

    WriteAST1180SOC(AST1180_MMC_BASE+0x00, 0xFC600309);	/* unlock */
    WriteAST1180SOC(AST1180_SCU_BASE+0x00, 0x1688a8a8);	/* unlock */
    usleep(100);

    WriteAST1180SOC(AST1180_MMC_BASE+0x08, 0x000011e3);	/* req. */

    /* init SCU */
#if 0
    ReadAST1180SOC(AST1180_SCU_BASE+0x08, ulData);	/* delay compensation */
    ulData &= 0xFFFFE0FF;
    ulData |= 0x00000C00;
    WriteAST1180SOC(AST1180_SCU_BASE+0x08, ulData);
#endif

    ReadAST1180SOC(AST1180_SCU_BASE+0x0c, ulData);	/* 2d clk */
    ulData &= 0xFFFFFFFD;
    WriteAST1180SOC(AST1180_SCU_BASE+0x0c, ulData);

    return (TRUE);

} /* bASTInitAST1180 */

void ASTGetAST1180DRAMInfo(ScrnInfoPtr pScrn)
{
    ASTRecPtr pAST = ASTPTR(pScrn);
    ULONG ulData;

    WriteAST1180SOC(AST1180_MMC_BASE+0x00, 0xFC600309);	/* unlock */
    ReadAST1180SOC(AST1180_MMC_BASE+0x04, ulData);
    pAST->ulDRAMBusWidth = 32;
    if (ulData & 0x40)
        pAST->ulDRAMBusWidth = 16;

    /* DRAM size */
    switch (ulData & 0x0C)
    {
    case 0x00:
       pAST->ulDRAMSize = DRAM_SIZE_032M;
       break;
    case 0x04:
       pAST->ulDRAMSize = DRAM_SIZE_064M;
       break;
    case 0x08:
       pAST->ulDRAMSize = DRAM_SIZE_128M;
       break;
    case 0x0c:
       pAST->ulDRAMSize = DRAM_SIZE_256M;
       break;
    }

    /* Get framebuffer size */
    switch (ulData & 0x30)
    {
    case 0x00:
        pAST->ulVRAMSize = DRAM_SIZE_016M;
        break;
    case 0x10:
        pAST->ulVRAMSize = DRAM_SIZE_032M;
        break;
    case 0x20:
        pAST->ulVRAMSize = DRAM_SIZE_064M;
        break;
    case 0x30:
        pAST->ulVRAMSize = DRAM_SIZE_128M;
        break;
    }

    /* VRAM base */
    if (pAST->ulVRAMSize >= pAST->ulDRAMSize)
        pAST->ulVRAMSize = pAST->ulDRAMSize;
    pAST->ulVRAMBase = pAST->ulDRAMSize - pAST->ulVRAMSize;

    /* MCLK */
    pAST->ulMCLK = 200;

} /* ASTGetAST1180DRAMInfo */

void vASTEnableVGAMMIO(ScrnInfoPtr pScrn)
{
    ASTRecPtr pAST = ASTPTR(pScrn);
    uint32_t ulData;
    UCHAR jReg;

    jReg = inb(pAST->RelocateIO + 0x43);
    if (jReg != 0x01)
    {
       /* Enable PCI */
       PCI_READ_LONG(pAST->PciInfo, &ulData, 0x04);
       ulData |= 0x03;
       PCI_WRITE_LONG(pAST->PciInfo, ulData, 0x04);

       outb(pAST->RelocateIO + 0x43, 0x01);
       outb(pAST->RelocateIO + 0x42, 0x01);
    }

    jReg = GetReg(VGA_ENABLE_PORT);
    if (jReg == 0xFF)	/* MMIO Access is disabled */
    {
       outw(pAST->RelocateIO + 0x54, 0xa880);
       outw(pAST->RelocateIO + 0x54, 0x04a1);
    }

} /* vEnableASTVGAMMIO */
