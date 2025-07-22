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
#include <dix-config.h>

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
#include "ast_mode.h"
#include "ast_vgatool.h"
#include "ast_2dtool.h"

/* Mandatory functions */
static void ASTIdentify(int flags);
const OptionInfoRec *ASTAvailableOptions(int chipid, int busid);
static Bool ASTProbe(DriverPtr drv, int flags);
static Bool ASTPreInit(ScrnInfoPtr pScrn, int flags);
static Bool ASTScreenInit(ScreenPtr pScreen, int argc, char **argv);
Bool ASTSwitchMode(ScrnInfoPtr pScrn, DisplayModePtr mode);
void ASTAdjustFrame(ScrnInfoPtr pScrn, int x, int y);
static Bool ASTEnterVT(ScrnInfoPtr pScrn);
static void ASTLeaveVT(ScrnInfoPtr pScrn);
static void ASTFreeScreen(ScrnInfoPtr pScrn);
static ModeStatus ASTValidMode(ScrnInfoPtr pScrn, DisplayModePtr mode, Bool verbose, int flags);

/* Internally used functions */
static Bool ASTGetRec(ScrnInfoPtr pScrn);
static void ASTFreeRec(ScrnInfoPtr pScrn);
static Bool ASTSaveScreen(ScreenPtr pScreen, Bool unblack);
static Bool ASTCloseScreen(ScreenPtr pScreen);
static void ASTSave(ScrnInfoPtr pScrn);
static void ASTRestore(ScrnInfoPtr pScrn);
static void ASTProbeDDC(ScrnInfoPtr pScrn, int index);
static xf86MonPtr ASTDoDDC(ScrnInfoPtr pScrn, int index);
static void vFillASTModeInfo (ScrnInfoPtr pScrn);
static Bool ASTModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);

#ifdef AstVideo
/* video function */
static void ASTInitVideo(ScreenPtr pScreen);
static int  ASTPutImage( ScrnInfoPtr,
        short, short, short, short, short, short, short, short,
        int, unsigned char*, short, short, Bool, RegionPtr, pointer, DrawablePtr);
#endif

/*
 * This is intentionally screen-independent.  It indicates the binding
 * choice made in the first PreInit.
 */
_X_EXPORT DriverRec AST = {
   AST_VERSION,
   AST_DRIVER_NAME,
   ASTIdentify,
   ASTProbe,
   ASTAvailableOptions,
   NULL,
   0
};

/* Chipsets */
static SymTabRec ASTChipsets[] = {
   {PCI_CHIP_AST2000,	"ASPEED Graphics Family"},
   {PCI_CHIP_AST2100,	"ASPEED Graphics Family"},
   {PCI_CHIP_AST1180,	"ASPEED AST1180 Graphics"},
   {-1,			NULL}
};

static PciChipsets ASTPciChipsets[] = {
   {PCI_CHIP_AST2000,		PCI_CHIP_AST2000,	RES_SHARED_VGA},
   {PCI_CHIP_AST2100,		PCI_CHIP_AST2100,	RES_SHARED_VGA},
   {PCI_CHIP_AST1180,		PCI_CHIP_AST1180,	RES_SHARED_VGA},
   {-1,				-1, 			RES_UNDEFINED }
};

typedef enum {
   OPTION_NOACCEL,
   OPTION_MMIO2D,
   OPTION_SW_CURSOR,
   OPTION_HWC_NUM,
   OPTION_ENG_CAPS,
   OPTION_DBG_SELECT,
   OPTION_NO_DDC,
   OPTION_VGA2_CLONE,
   OPTION_SHADOW_FB
} ASTOpts;

static const OptionInfoRec ASTOptions[] = {
   {OPTION_NOACCEL,	      "NoAccel",	OPTV_BOOLEAN,	{0},	FALSE},
   {OPTION_MMIO2D,	       "MMIO2D",	OPTV_BOOLEAN,	{0},	FALSE},
   {OPTION_SW_CURSOR,	 "SWCursor",	OPTV_BOOLEAN,	{0},	FALSE},
   {OPTION_HWC_NUM,	    "HWCNumber",	OPTV_INTEGER,	{0},	FALSE},
   {OPTION_ENG_CAPS,	  "ENGCaps",	OPTV_INTEGER,	{0},	FALSE},
   {OPTION_DBG_SELECT,	"DBGSelect",	OPTV_INTEGER,	{0},	FALSE},
   {OPTION_NO_DDC,	        "NoDDC",	OPTV_BOOLEAN,	{0}, 	FALSE},
   {OPTION_VGA2_CLONE,	"VGA2Clone",	OPTV_BOOLEAN,	{0}, 	FALSE},
   {OPTION_SHADOW_FB,    "ShadowFB",	OPTV_BOOLEAN,	{0},	FALSE},
   {-1,			               NULL,	   OPTV_NONE,	{0}, 	FALSE}
};

#ifdef XFree86LOADER

static MODULESETUPPROTO(astSetup);

static XF86ModuleVersionInfo astVersRec = {
   AST_DRIVER_NAME,
   MODULEVENDORSTRING,
   MODINFOSTRING1,
   MODINFOSTRING2,
   XORG_VERSION_CURRENT,
   AST_MAJOR_VERSION, AST_MINOR_VERSION, AST_PATCH_VERSION,
   ABI_CLASS_VIDEODRV,
#ifdef PATCH_ABI_VERSION
   ABI_VIDEODRV_VERSION_PATCH,
#else
   ABI_VIDEODRV_VERSION,
#endif
   MOD_CLASS_VIDEODRV,
   {0, 0, 0, 0}
};

_X_EXPORT XF86ModuleData astModuleData = { &astVersRec, astSetup, NULL };

static pointer
astSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
   static Bool setupDone = FALSE;

   /* This module should be loaded only once, but check to be sure.
    */
   if (!setupDone) {
      setupDone = TRUE;
      xf86AddDriver(&AST, module, 0);

      /*
       * The return value must be non-NULL on success even though there
       * is no TearDownProc.
       */
      return (pointer) TRUE;
   } else {
      if (errmaj)
	 *errmaj = LDR_ONCEONLY;
      return NULL;
   }
}

#endif	/* XFree86LOADER */

/*
 * ASTIdentify --
 *
 * Returns the string name for the driver based on the chipset. In this
 * case it will always be an AST, so we can return a static string.
 *
 */
static void
ASTIdentify(int flags)
{
   xf86PrintChipsets(AST_NAME, "Driver for ASPEED Graphics Chipsets",
		     ASTChipsets);
}

const OptionInfoRec *
ASTAvailableOptions(int chipid, int busid)
{

   return ASTOptions;

}

/*
 * ASTProbe --
 *
 * Look through the PCI bus to find cards that are AST boards.
 * Setup the dispatch table for the rest of the driver functions.
 *
 */
static Bool
ASTProbe(DriverPtr drv, int flags)
{
    int i, numUsed, numDevSections, *usedChips;
    Bool foundScreen = FALSE;
    GDevPtr *devSections;

   /*
    * Find the config file Device sections that match this
    * driver, and return if there are none.
    */
    if ((numDevSections =
	xf86MatchDevice(AST_DRIVER_NAME, &devSections)) <= 0) {
      return FALSE;
    }

#ifndef XSERVER_LIBPCIACCESS
   /*
    * This probing is just checking the PCI data the server already
    * collected.
    */
    if (xf86GetPciVideoInfo() == NULL) {
	return FALSE;
    }
#endif

    numUsed = xf86MatchPciInstances(AST_NAME, PCI_VENDOR_AST,
				   ASTChipsets, ASTPciChipsets,
				   devSections, numDevSections,
				   drv, &usedChips);

    if (numUsed <= 0) {
	free(devSections);
	return FALSE;
    }

    if (flags & PROBE_DETECT) {
        if (numUsed > 0)
	    foundScreen = TRUE;
    } else {
        for (i = 0; i < numUsed; i++) {
	    ScrnInfoPtr pScrn = NULL;

#ifdef XSERVER_LIBPCIACCESS
            {
                struct pci_device *pPci = xf86GetPciInfoForEntity(usedChips[i]);

                if (pci_device_has_kernel_driver(pPci)) {
                    xf86DrvMsg(0, X_ERROR,
                               "ast: The PCI device 0x%x at %2.2d@%2.2d:%2.2d:%1.1d has a kernel module claiming it.\n",
                               pPci->device_id, pPci->bus, pPci->domain, pPci->dev, pPci->func);
                    xf86DrvMsg(0, X_ERROR,
                               "ast: This driver cannot operate until it has been unloaded.\n");
                    xf86UnclaimPciSlot(pPci, devSections[0]);
                    free(devSections);
                    return FALSE;
                }
            }
#endif

	    /* Allocate new ScrnInfoRec and claim the slot */
	    if ((pScrn = xf86ConfigPciEntity(pScrn, 0, usedChips[i],
					     ASTPciChipsets, 0, 0, 0, 0, 0)))
            {
	        pScrn->driverVersion = AST_VERSION;
	        pScrn->driverName = AST_DRIVER_NAME;
	        pScrn->name = AST_NAME;

	        pScrn->Probe = ASTProbe;
	        pScrn->PreInit = ASTPreInit;
	        pScrn->ScreenInit = ASTScreenInit;
	        pScrn->SwitchMode = ASTSwitchMode;
	        pScrn->AdjustFrame = ASTAdjustFrame;
	        pScrn->EnterVT = ASTEnterVT;
	        pScrn->LeaveVT = ASTLeaveVT;
	        pScrn->FreeScreen = ASTFreeScreen;
	        pScrn->ValidMode = ASTValidMode;

	        foundScreen = TRUE;

	    } /* end of if */
        }  /* end of for-loop */
    } /* end of if flags */

    free(devSections);
    free(usedChips);

    return foundScreen;
}

#ifdef	Support_ShadowFB
static void *
ASTWindowLinear(ScreenPtr pScreen, CARD32 row, CARD32 offset, int mode,
		        CARD32 *size, void *closure)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    ASTPtr pAST = ASTPTR(pScrn);
    int stride = pScrn->displayWidth * ((pScrn->bitsPerPixel + 1) / 8);

    *size =  stride;
    return ((uint8_t *)pAST->FBVirtualAddr + pScrn->fbOffset + row * stride + offset);

}

static void
ASTUpdatePacked(ScreenPtr pScreen, shadowBufPtr pBuf)
{
    shadowUpdatePacked(pScreen, pBuf);
}

static Bool
ASTCreateScreenResources(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    ASTPtr pAST = ASTPTR(pScrn);
    Bool ret;

    pScreen->CreateScreenResources = pAST->CreateScreenResources;
    ret = pScreen->CreateScreenResources(pScreen);
    pScreen->CreateScreenResources = ASTCreateScreenResources;
    shadowAdd(pScreen, pScreen->GetScreenPixmap(pScreen), pAST->update,
	          pAST->window, 0, 0);

    return ret;
}
#endif	/* Support_ShadowFB */

/*
 * ASTPreInit --
 *
 * Do initial setup of the board before we know what resolution we will
 * be running at.
 *
 */
static Bool
ASTPreInit(ScrnInfoPtr pScrn, int flags)
{
   EntityInfoPtr pEnt;
   int flags24;
   rgb defaultWeight = { 0, 0, 0 };
#if	!(defined(__sparc__)) && !(defined(__mips__))
   vgaHWPtr hwp;
#endif

   ASTRecPtr pAST;

   ClockRangePtr clockRanges;
   int i;
   MessageType from;
   int maxPitch, maxHeight;

   /* Support one adapter only now */
   if (pScrn->numEntities != 1)
       return FALSE;

   pEnt = xf86GetEntityInfo(pScrn->entityList[0]);

   if (flags & PROBE_DETECT) {
       ASTProbeDDC(pScrn, pEnt->index);
       return TRUE;
   }

   if (pEnt->location.type != BUS_PCI)
       return FALSE;

#ifndef XSERVER_LIBPCIACCESS
   if (xf86RegisterResources(pEnt->index, 0, ResExclusive))
       return FALSE;
#endif

#if	!(defined(__sparc__)) && !(defined(__mips__))
   /* The vgahw module should be loaded here when needed */
   if (!xf86LoadSubModule(pScrn, "vgahw"))
      return FALSE;
#endif

   /* The fb module should be loaded here when needed */
   if (!xf86LoadSubModule(pScrn, "fb"))
      return FALSE;

#if	!(defined(__sparc__)) && !(defined(__mips__))
   /* Allocate a vgaHWRec */
   if (!vgaHWGetHWRec(pScrn))
       return FALSE;
   hwp = VGAHWPTR(pScrn);
   vgaHWSetStdFuncs(hwp);
#endif

   /* Color Depth Check */
   flags24 = Support32bppFb;
   if (!xf86SetDepthBpp(pScrn, 0, 0, 0, flags24)) {
      return FALSE;
   } else {
      switch (pScrn->depth) {
      case 8:
      case 16:
      case 24:
	 break;
      default:
	 xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "Given depth (%d) is not supported by ast driver\n",
		    pScrn->depth);
	 return FALSE;
      }
   }
   xf86PrintDepthBpp(pScrn);

   switch (pScrn->bitsPerPixel) {
   case 8:
   case 16:
   case 32:
      break;
   default:
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "Given bpp (%d) is not supported by ast driver\n",
		 pScrn->bitsPerPixel);
      return FALSE;
   }

   /* fill pScrn misc. */
   pScrn->progClock = TRUE;
   pScrn->rgbBits = 6;
   pScrn->monitor = pScrn->confScreen->monitor; /* should be initialized before set gamma */
#ifndef XSERVER_LIBPCIACCESS
   pScrn->racMemFlags = RAC_FB | RAC_COLORMAP | RAC_CURSOR | RAC_VIEWPORT;
   pScrn->racIoFlags = RAC_COLORMAP | RAC_CURSOR | RAC_VIEWPORT;
#endif

   /*
    * If the driver can do gamma correction, it should call xf86SetGamma()
    * here.
    */
   {
      Gamma zeros = { 0.0, 0.0, 0.0 };

      if (!xf86SetGamma(pScrn, zeros)) {
         xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "call xf86SetGamma failed \n");
	 return FALSE;
      }
   }


   if (!xf86SetWeight(pScrn, defaultWeight, defaultWeight)) {
       return FALSE;
   }

   if (!xf86SetDefaultVisual(pScrn, -1)) {
       return FALSE;
   }

   /* Allocate driverPrivate */
   if (!ASTGetRec(pScrn)) {
       xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "call ASTGetRec failed \n");
       return FALSE;
   }

   /* Fill AST Info */
   pAST = ASTPTR(pScrn);
   pAST->pEnt    = xf86GetEntityInfo(pScrn->entityList[0]);
   pAST->PciInfo = xf86GetPciInfoForEntity(pAST->pEnt->index);
#ifndef XSERVER_LIBPCIACCESS
   pAST->PciTag  = pciTag(pAST->PciInfo->bus, pAST->PciInfo->device,
			  pAST->PciInfo->func);
#endif

   /* Process the options
    * pScrn->confScreen, pScrn->display, pScrn->monitor, pScrn->numEntities,
    * and pScrn->entityList should be initialized before
    */
   xf86CollectOptions(pScrn, NULL);
   if (!(pAST->Options = malloc(sizeof(ASTOptions))))
   {
      ASTFreeRec(pScrn);
      return FALSE;
   }
   memcpy(pAST->Options, ASTOptions, sizeof(ASTOptions));
   xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, pAST->Options);

   /*
    * Set the Chipset and ChipRev, allowing config file entries to
    * override.
    */
   if (pAST->pEnt->device->chipset && *pAST->pEnt->device->chipset) {
      pScrn->chipset = pAST->pEnt->device->chipset;
      from = X_CONFIG;
   } else if (pAST->pEnt->device->chipID >= 0) {
      pScrn->chipset = (char *)xf86TokenToString(ASTChipsets,
						 pAST->pEnt->device->chipID);
      from = X_CONFIG;
      xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipID override: 0x%04X\n",
		 pAST->pEnt->device->chipID);
   } else {
      from = X_PROBED;
      pScrn->chipset = (char *)xf86TokenToString(ASTChipsets,
						 PCI_DEV_DEVICE_ID(pAST->PciInfo));
   }
   if (pAST->pEnt->device->chipRev >= 0) {
      xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipRev override: %d\n",
		 pAST->pEnt->device->chipRev);
   }

   xf86DrvMsg(pScrn->scrnIndex, from, "Chipset: \"%s\"\n",
	      (pScrn->chipset != NULL) ? pScrn->chipset : "Unknown ast");


   pAST->RelocateIO = (PCI_REGION_BASE(pAST->PciInfo, 2, REGION_IO));


   if (pAST->pEnt->device->MemBase != 0) {
      pAST->FBPhysAddr = pAST->pEnt->device->MemBase;
      from = X_CONFIG;
   } else {
      if (PCI_REGION_BASE(pAST->PciInfo, 0, REGION_MEM) != 0) {
	 pAST->FBPhysAddr = PCI_REGION_BASE(pAST->PciInfo, 0, REGION_MEM) & 0xFFF00000;
	 from = X_PROBED;
      } else {
	 xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "No valid FB address in PCI config space\n");
	 ASTFreeRec(pScrn);
	 return FALSE;
      }
   }
   xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Linear framebuffer at 0x%lX\n",
	      (unsigned long) pAST->FBPhysAddr);

   if (pAST->pEnt->device->IOBase != 0) {
      pAST->MMIOPhysAddr = pAST->pEnt->device->IOBase;
      from = X_CONFIG;
   } else {
      if (PCI_REGION_BASE(pAST->PciInfo, 1, REGION_MEM)) {
	 pAST->MMIOPhysAddr = PCI_REGION_BASE(pAST->PciInfo, 1, REGION_MEM) & 0xFFFF0000;
	 from = X_PROBED;
      } else {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		    "No valid MMIO address in PCI config space\n");
	 ASTFreeRec(pScrn);
	 return FALSE;
      }
   }
   xf86DrvMsg(pScrn->scrnIndex, X_INFO, "IO registers at addr 0x%lX\n",
	      (unsigned long) pAST->MMIOPhysAddr);

   /* Map MMIO */
   pAST->MMIOMapSize = DEFAULT_MMIO_SIZE;
   if (!ASTMapMMIO(pScrn)) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Map Memory Map IO Failed \n");
      return FALSE;
   }

   if (PCI_DEV_DEVICE_ID(pAST->PciInfo) == PCI_CHIP_AST1180)
   {
       pAST->jChipType = AST1180;

       /* validate mode */
       if ( (pScrn->bitsPerPixel == 8) || (pScrn->depth == 8) )
       {
           xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		      "Given bpp (%d) is not supported by ast driver\n",
		      pScrn->bitsPerPixel);
           return FALSE;
       }

       /* Init AST1180 */
       bASTInitAST1180(pScrn);

       /* Get AST1180 Information */
       ASTGetAST1180DRAMInfo(pScrn);
       pScrn->videoRam = pAST->ulVRAMSize / 1024;

   }
   else
   {
       /* Enable VGA MMIO Access */
       vASTEnableVGAMMIO(pScrn);

       /* Get Chip Type */
       if (PCI_DEV_REVISION(pAST->PciInfo) >= 0x40)
           pAST->jChipType = AST2500;
       else if (PCI_DEV_REVISION(pAST->PciInfo) >= 0x30)
           pAST->jChipType = AST2400;
       else if (PCI_DEV_REVISION(pAST->PciInfo) >= 0x20)
           pAST->jChipType = AST2300;
       else if (PCI_DEV_REVISION(pAST->PciInfo) >= 0x10)
           ASTGetChipType(pScrn);
       else
           pAST->jChipType = AST2000;

       /* Init VGA Adapter */
       if (!xf86IsPrimaryPci(pAST->PciInfo))
       {
           ASTInitVGA(pScrn, 0);
       }

       vASTOpenKey(pScrn);
       bASTRegInit(pScrn);

       /* Get Options from Scratch */
       ASTGetScratchOptions(pScrn);

       /* Get DRAM Info */
       ASTGetDRAMInfo(pScrn);
       pAST->ulVRAMSize = ASTGetVRAMInfo(pScrn);
       pScrn->videoRam  = pAST->ulVRAMSize / 1024;
   }

   /* Map Framebuffer */
   from = X_DEFAULT;
   if (pAST->pEnt->device->videoRam) {
      pScrn->videoRam = pAST->pEnt->device->videoRam;
      from = X_CONFIG;
   }

   pAST->FbMapSize = pScrn->videoRam * 1024;

#if 0
   if (!ASTMapMem(pScrn)) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Map FB Memory Failed \n");
      return FALSE;
   }
#endif

   pScrn->memPhysBase = (ULONG)pAST->FBPhysAddr;
   pScrn->fbOffset = 0;

   /* Do DDC
    * should be done after xf86CollectOptions
    */
   pScrn->monitor->DDC = ASTDoDDC(pScrn, pAST->pEnt->index);

   /* Mode Valid */
   clockRanges = XNFcallocarray(sizeof(ClockRange), 1);
   clockRanges->next = NULL;
   clockRanges->minClock = 9500;
   clockRanges->maxClock = ASTGetMaxDCLK(pScrn) * 1000;
   clockRanges->clockIndex = -1;
   clockRanges->interlaceAllowed = FALSE;
   clockRanges->doubleScanAllowed = FALSE;

   /* Add for AST2100, ycchen@061807 */
   if ((pAST->jChipType == AST2100) || (pAST->jChipType == AST2200) || (pAST->jChipType == AST2300) || (pAST->jChipType == AST2400) || (pAST->jChipType == AST2500) || (pAST->jChipType == AST1180))
   {
       maxPitch  = 1920;
       maxHeight = 1200;
   }
   else
   {
       maxPitch  = 1600;
       maxHeight = 1200;
   }

   i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
			 pScrn->display->modes, clockRanges,
			 0, 320, maxPitch, 8 * pScrn->bitsPerPixel,
			 200, maxHeight,
			 pScrn->display->virtualX, pScrn->display->virtualY,
			 pAST->FbMapSize, LOOKUP_BEST_REFRESH);

   /* fixed some monitors can't get properly validated modes using estimated ratio modes */
   if (i < 2)		/* validate modes are too few */
   {
       i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
			     pScrn->display->modes, clockRanges,
			     0, 320, maxPitch, 8 * pScrn->bitsPerPixel,
			     200, maxHeight,
			     pAST->mon_h_active, pAST->mon_v_active,
			     pAST->FbMapSize, LOOKUP_BEST_REFRESH);
   }

   if (i == -1) {
      ASTFreeRec(pScrn);
      return FALSE;
   }

   xf86PruneDriverModes(pScrn);

   if (!i || !pScrn->modes) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
      ASTFreeRec(pScrn);
      return FALSE;
   }

   xf86SetCrtcForModes(pScrn, INTERLACE_HALVE_V);

   pScrn->currentMode = pScrn->modes;

   xf86PrintModes(pScrn);

   xf86SetDpi(pScrn, 0, 0);

   /* Acceleration Check */
   pAST->noAccel = TRUE;
   pAST->pCMDQPtr = NULL;
   pAST->CMDQInfo.ulCMDQSize 		= 0;
   pAST->CMDQInfo.pjCmdQBasePort    = pAST->MMIOVirtualAddr+ 0x8044;
   pAST->CMDQInfo.pjWritePort       = pAST->MMIOVirtualAddr+ 0x8048;
   pAST->CMDQInfo.pjReadPort        = pAST->MMIOVirtualAddr+ 0x804C;
   pAST->CMDQInfo.pjEngStatePort    = pAST->MMIOVirtualAddr+ 0x804C;

   /* HW Cursor Check */
   pAST->noHWC = TRUE;
   pAST->HWCInfoPtr = NULL;
   pAST->pHWCPtr = NULL;
#ifdef	HWC
   if (!xf86ReturnOptValBool(pAST->Options, OPTION_SW_CURSOR, FALSE)) {
      if (!xf86LoadSubModule(pScrn, "ramdac")) {
	 ASTFreeRec(pScrn);
	 return FALSE;
      }

      pAST->noHWC = FALSE;
      pAST->HWCInfo.HWC_NUM = DEFAULT_HWC_NUM;
      if (!xf86GetOptValInteger(pAST->Options, OPTION_HWC_NUM, &pAST->HWCInfo.HWC_NUM)) {
          xf86DrvMsg(pScrn->scrnIndex, X_INFO, "No HWC_NUM options found\n");
      }

   }
#endif

   /* ShadowFB */
#ifdef	Support_ShadowFB
   pAST->shadowFB = FALSE;
   if (pAST->noAccel == TRUE)	/* enable shadowFB only noAccel */
   {
       if (xf86ReturnOptValBool(pAST->Options, OPTION_SHADOW_FB, TRUE))
       {
	       if (xf86LoadSubModule(pScrn, "shadow")) {
	          xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Using \"Shadow Framebuffer\"\n");
	          pAST->shadowFB = TRUE;
	       }
       }
   }
#endif

#ifndef XSERVER_LIBPCIACCESS
   /*  We won't be using the VGA access after the probe */
   xf86SetOperatingState(resVgaIo, pAST->pEnt->index, ResUnusedOpr);
   xf86SetOperatingState(resVgaMem, pAST->pEnt->index, ResDisableOpr);
#endif

   return TRUE;
}


static Bool
ASTScreenInit(ScreenPtr pScreen, int argc, char **argv)
{
   ScrnInfoPtr pScrn;
   ASTRecPtr pAST;
   VisualPtr visual;
   /* for FB Manager */
   BoxRec FBMemBox;
   int    AvailFBSize;

   pScrn = xf86ScreenToScrn(pScreen);
   pAST = ASTPTR(pScrn);

   if (!ASTMapMem(pScrn)) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Map FB Memory Failed \n");
      return FALSE;
   }

/*   if (!pAST->noAccel) */
   {
       /* AvailFBSize = pAST->FbMapSize - pAST->CMDQInfo.ulCMDQSize; */
       AvailFBSize = pAST->FbMapSize;

       FBMemBox.x1 = 0;
       FBMemBox.y1 = 0;
       FBMemBox.x2 = pScrn->displayWidth;
       FBMemBox.y2 = (AvailFBSize / (pScrn->displayWidth * ((pScrn->bitsPerPixel+1)/8))) - 1;

       if (FBMemBox.y2 < 0)
           FBMemBox.y2 = 32767;
       if (FBMemBox.y2 < pScrn->virtualY)
           return FALSE;

       if (!xf86InitFBManager(pScreen, &FBMemBox)) {
          xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Failed to init memory manager\n");
          return FALSE;
       }

   }

#if	!(defined(__sparc__)) && !(defined(__mips__))
   {
       vgaHWPtr hwp;
       hwp = VGAHWPTR(pScrn);
       vgaHWSetMmioFuncs(hwp, pAST->MMIOVirtualAddr, 0);
   }
#endif

   vFillASTModeInfo (pScrn);

   ASTSave(pScrn);
   if (!ASTModeInit(pScrn, pScrn->currentMode)) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Mode Init Failed \n");
      return FALSE;
   }

   ASTSaveScreen(pScreen, FALSE);
   ASTAdjustFrame(pScrn, pScrn->frameX0, pScrn->frameY0);

   miClearVisualTypes();

   /* Re-implemented Direct Color support, -jens */
   if (!miSetVisualTypes(pScrn->depth, miGetDefaultVisualMask(pScrn->depth),
			 pScrn->rgbBits, pScrn->defaultVisual))
      return FALSE;

   if (!miSetPixmapDepths())
   {
       ASTSaveScreen(pScreen, SCREEN_SAVER_OFF);
       return FALSE;
   }

   /* allocate shadowFB */
#ifdef	Support_ShadowFB
   pAST->shadowFB_validation = FALSE;
   if (pAST->shadowFB) {
      pAST->shadow = calloc(1, pScrn->displayWidth * pScrn->virtualY *
				     ((pScrn->bitsPerPixel + 7) / 8));
	  if (!pAST->shadow) {
	     xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Failed to allocate shadow buffer\n");
	  }
	  else
	     pAST->shadowFB_validation = TRUE;
   }
#endif

   switch(pScrn->bitsPerPixel) {
       case 8:
       case 16:
       case 32:
#ifdef	Support_ShadowFB
           if (!fbScreenInit(pScreen, pAST->shadowFB_validation ? pAST->shadow : (pAST->FBVirtualAddr + pScrn->fbOffset),
  	                         pScrn->virtualX, pScrn->virtualY,
		                     pScrn->xDpi, pScrn->yDpi,
		                     pScrn->displayWidth, pScrn->bitsPerPixel))
#else
	       if (!fbScreenInit(pScreen, pAST->FBVirtualAddr + pScrn->fbOffset,
			                 pScrn->virtualX, pScrn->virtualY,
			                 pScrn->xDpi, pScrn->yDpi,
			                 pScrn->displayWidth, pScrn->bitsPerPixel))
#endif
               return FALSE;
           break;
       default:
           return FALSE;

   }

   if (pScrn->bitsPerPixel > 8) {
      /* Fixup RGB ordering */
      visual = pScreen->visuals + pScreen->numVisuals;
      while (--visual >= pScreen->visuals) {
	 if ((visual->class | DynamicClass) == DirectColor) {
	    visual->offsetRed = pScrn->offset.red;
	    visual->offsetGreen = pScrn->offset.green;
	    visual->offsetBlue = pScrn->offset.blue;
	    visual->redMask = pScrn->mask.red;
	    visual->greenMask = pScrn->mask.green;
	    visual->blueMask = pScrn->mask.blue;
	 }
      }
   }

   /* Must be after RGB order fixed */
   fbPictureInit(pScreen, 0, 0);

   /* shadowFB setup */
#ifdef	Support_ShadowFB
   if (pAST->shadowFB_validation) {
      pAST->update = ASTUpdatePacked;
	  pAST->window = ASTWindowLinear;

      if (!shadowSetup(pScreen))
      {
	     xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Failed to setup shadow buffer\n");
	     return FALSE;
      }

	  pAST->CreateScreenResources = pScreen->CreateScreenResources;
	  pScreen->CreateScreenResources = ASTCreateScreenResources;
   }
#endif

   xf86SetBlackWhitePixels(pScreen);
   xf86SetBackingStore(pScreen);
   xf86SetSilkenMouse(pScreen);

   miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

   if (!pAST->noHWC)
   {
       if (!ASTCursorInit(pScreen)) {
           xf86DrvMsg(pScrn->scrnIndex, X_ERROR,"Hardware cursor initialization failed\n");
           pAST->noHWC = TRUE;
       }
   }

   if (!miCreateDefColormap(pScreen))
      return FALSE;

   if (pAST->jChipType != AST1180)
   {
       if(!xf86HandleColormaps(pScreen, 256, (pScrn->depth == 8) ? 8 : pScrn->rgbBits,
                               vASTLoadPalette, NULL,
                               CMAP_PALETTED_TRUECOLOR | CMAP_RELOAD_ON_MODE_SWITCH)) {
           return FALSE;
       }
   }

   xf86DPMSInit(pScreen, ASTDisplayPowerManagementSet, 0);

#ifdef AstVideo
   if ( (pAST->jChipType == AST1180) || (pAST->jChipType == AST2300) || (pAST->jChipType == AST2400) || (pAST->jChipType == AST2500) )
   {
       xf86DrvMsg(pScrn->scrnIndex, X_INFO,"AST Initial Video()\n");
       ASTInitVideo(pScreen);
   }
#endif

   pScreen->SaveScreen = ASTSaveScreen;
   pAST->CloseScreen = pScreen->CloseScreen;
   pScreen->CloseScreen = ASTCloseScreen;

   if (serverGeneration == 1)
      xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);

   return TRUE;

} /* ASTScreenInit */


Bool
ASTSwitchMode(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
   ASTRecPtr pAST = ASTPTR(pScrn);

   /* VideoMode validate */
   if (mode->CrtcHDisplay > pScrn->displayWidth)
       return FALSE;
   if ((pAST->VideoModeInfo.ScreenPitch * mode->CrtcVDisplay) > pAST->ulVRAMSize)
       return FALSE;

   /* VideModeInfo Update */
   pAST->VideoModeInfo.ScreenWidth  = mode->CrtcHDisplay;
   pAST->VideoModeInfo.ScreenHeight = mode->CrtcVDisplay;
   pAST->VideoModeInfo.ScreenPitch  = pScrn->displayWidth * ((pScrn->bitsPerPixel + 1) / 8) ;

#ifdef	HWC
   if (pAST->pHWCPtr) {
       xf86FreeOffscreenLinear(pAST->pHWCPtr);		/* free HWC Cache */
       pAST->pHWCPtr = NULL;
   }
   ASTDisableHWC(pScrn);
#endif

#ifdef Accel_2D
   if (pAST->pCMDQPtr) {
       xf86FreeOffscreenLinear(pAST->pCMDQPtr);		/* free CMDQ */
       pAST->pCMDQPtr = NULL;
   }
   vASTDisable2D(pScrn, pAST);
#endif

   /* Fixed display abnormal on the of the screen if run xvidtune, ycchen@122909 */
   /* ASTRestore(pScrn); */

   return ASTModeInit(pScrn, mode);

}

void
ASTAdjustFrame(ScrnInfoPtr pScrn, int x, int y)
{
   ASTRecPtr   pAST  = ASTPTR(pScrn);
   ULONG base;

   base = y * pAST->VideoModeInfo.ScreenPitch + x * ((pAST->VideoModeInfo.bitsPerPixel + 1) / 8);
   /* base = base >> 2; */			/* DW unit */

   vASTSetStartAddressCRT1(pAST, base);

}

/* enter into X Server */
static Bool
ASTEnterVT(ScrnInfoPtr pScrn)
{
   ASTRecPtr pAST = ASTPTR(pScrn);

   /* Fixed suspend can't resume issue */
   if (!bASTIsVGAEnabled(pScrn))
   {
       if (pAST->jChipType == AST1180)
           bASTInitAST1180(pScrn);
       else
       {
           vASTEnableVGAMMIO(pScrn);
           ASTInitVGA(pScrn, 1);
       }
       ASTRestore(pScrn);
   }

   if (!ASTModeInit(pScrn, pScrn->currentMode))
      return FALSE;
   ASTAdjustFrame(pScrn, pScrn->frameX0, pScrn->frameY0);

   return TRUE;

}

/* leave X server */
static void
ASTLeaveVT(ScrnInfoPtr pScrn)
{
   ASTRecPtr pAST = ASTPTR(pScrn);
#if	!(defined(__sparc__)) && !(defined(__mips__))
   vgaHWPtr hwp = VGAHWPTR(pScrn);
#endif

#ifdef	HWC
   if (pAST->pHWCPtr) {
       xf86FreeOffscreenLinear(pAST->pHWCPtr);		/* free HWC Cache */
       pAST->pHWCPtr = NULL;
   }
   ASTDisableHWC(pScrn);
#endif

#ifdef Accel_2D
   if (pAST->pCMDQPtr) {
       xf86FreeOffscreenLinear(pAST->pCMDQPtr);		/* free CMDQ */
       pAST->pCMDQPtr = NULL;
   }
   vASTDisable2D(pScrn, pAST);
#endif

   ASTRestore(pScrn);

   if (pAST->jChipType == AST1180)
       ASTBlankScreen(pScrn, 0);

#if	!(defined(__sparc__)) && !(defined(__mips__))
   vgaHWLock(hwp);
#endif

}

static void
ASTFreeScreen(ScrnInfoPtr pScrn)
{
   ASTFreeRec(pScrn);
#if	!(defined(__sparc__)) && !(defined(__mips__))
   if (xf86LoaderCheckSymbol("vgaHWFreeHWRec"))
      vgaHWFreeHWRec(pScrn);
#endif
}

static ModeStatus
ASTValidMode(ScrnInfoPtr pScrn, DisplayModePtr mode, Bool verbose, int flags)
{
   ASTRecPtr   pAST  = ASTPTR(pScrn);
   ModeStatus Flags = MODE_NOMODE;
   UCHAR jReg;
   ULONG RequestBufferSize;

   if (mode->Flags & V_INTERLACE) {
      if (verbose) {
	 xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		    "Removing interlaced mode \"%s\"\n", mode->name);
      }
      return MODE_NO_INTERLACE;
   }

   if ((mode->CrtcHDisplay > MAX_HResolution) || (mode->CrtcVDisplay > MAX_VResolution)) {
      if (verbose) {
	 xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		    "Removing the mode \"%s\"\n", mode->name);
      }
      return Flags;
   }

   /* Valid Framebuffer size */
   RequestBufferSize = mode->CrtcHDisplay * ((pScrn->bitsPerPixel + 1) / 8) * mode->CrtcVDisplay;
   if (RequestBufferSize > pAST->ulVRAMSize)
      return Flags;

   /* Valid Wide Screen Mode */
   if (pAST->SupportWideScreen)
   {
      if ( (mode->CrtcHDisplay == 1680) && (mode->CrtcVDisplay == 1050) )
          return MODE_OK;
      if ( (mode->CrtcHDisplay == 1280) && (mode->CrtcVDisplay == 800) )
          return MODE_OK;
      if ( (mode->CrtcHDisplay == 1440) && (mode->CrtcVDisplay == 900) )
          return MODE_OK;
      if ( (mode->CrtcHDisplay == 1360) && (mode->CrtcVDisplay == 768) )
          return MODE_OK;
      if ( (mode->CrtcHDisplay == 1600) && (mode->CrtcVDisplay == 900) )
          return MODE_OK;

      if ( (pAST->jChipType == AST2100) || (pAST->jChipType == AST2200) || (pAST->jChipType == AST2300) || (pAST->jChipType == AST2400) || (pAST->jChipType == AST2500) || (pAST->jChipType == AST1180) )
      {
          if ( (mode->CrtcHDisplay == 1920) && (mode->CrtcVDisplay == 1080) )
              return MODE_OK;

          if ( (mode->CrtcHDisplay == 1920) && (mode->CrtcVDisplay == 1200) )
          {
             GetIndexRegMask(CRTC_PORT, 0xD1, 0xFF, jReg);
	     if (jReg & 0x01)
	        return MODE_NOMODE;
	     else
                return MODE_OK;
          }
      }
   }

   switch (mode->CrtcHDisplay)
   {
   case 640:
       if (mode->CrtcVDisplay == 480) Flags=MODE_OK;
       break;
   case 800:
       if (mode->CrtcVDisplay == 600) Flags=MODE_OK;
       break;
   case 1024:
       if (mode->CrtcVDisplay == 768) Flags=MODE_OK;
       break;
   case 1280:
       if (mode->CrtcVDisplay == 1024) Flags=MODE_OK;
       break;
   case 1600:
       if (mode->CrtcVDisplay == 1200) Flags=MODE_OK;
       break;
   default:
       return Flags;
   }

   return Flags;
}

/* Internal used modules */
/*
 * ASTGetRec and ASTFreeRec --
 *
 * Private data for the driver is stored in the screen structure.
 * These two functions create and destroy that private data.
 *
 */
static Bool
ASTGetRec(ScrnInfoPtr pScrn)
{
   if (pScrn->driverPrivate)
      return TRUE;

   pScrn->driverPrivate = XNFcallocarray(sizeof(ASTRec), 1);
   return TRUE;
}

static void
ASTFreeRec(ScrnInfoPtr pScrn)
{
   ASTRecPtr pAST = ASTPTR(pScrn);

   if (!pScrn)
      return;
   if (!pScrn->driverPrivate)
      return;
   if (pAST->pDP501FWBufferVirtualAddress)
       free(pAST->pDP501FWBufferVirtualAddress);
   free(pScrn->driverPrivate);
   pScrn->driverPrivate = 0;
}

static Bool
ASTSaveScreen(ScreenPtr pScreen, Bool unblack)
{
#if	!(defined(__sparc__)) && !(defined(__mips__))
   /* replacement of vgaHWBlankScreen(pScrn, unblank) without seq reset */
   /* return vgaHWSaveScreen(pScreen, unblack); */
   ScrnInfoPtr pScrn = NULL;

   if (pScreen != NULL)
      pScrn = xf86ScreenToScrn(pScreen);

   if ((pScrn != NULL) && pScrn->vtSema) {
     ASTBlankScreen(pScrn, unblack);
   }
   return (TRUE);
#endif
}

static Bool
ASTCloseScreen(ScreenPtr pScreen)
{
   ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
   ASTRecPtr pAST = ASTPTR(pScrn);
#if	!(defined(__sparc__)) && !(defined(__mips__))
   vgaHWPtr hwp = VGAHWPTR(pScrn);
#endif

   if (pScrn->vtSema == TRUE)
   {
#ifdef	HWC
       if (pAST->pHWCPtr) {
           xf86FreeOffscreenLinear(pAST->pHWCPtr);		/* free HWC Cache */
           pAST->pHWCPtr = NULL;
       }
       ASTDisableHWC(pScrn);
#endif

#ifdef Accel_2D
       if (pAST->pCMDQPtr) {
           xf86FreeOffscreenLinear(pAST->pCMDQPtr);		/* free CMDQ */
           pAST->pCMDQPtr = NULL;
       }
       vASTDisable2D(pScrn, pAST);
#endif

       ASTRestore(pScrn);

       if (pAST->jChipType == AST1180)
           ASTBlankScreen(pScrn, 0);

#if	!(defined(__sparc__)) && !(defined(__mips__))
       vgaHWLock(hwp);
#endif
   }

   ASTUnmapMem(pScrn);
#if	!(defined(__sparc__)) && !(defined(__mips__))
   vgaHWUnmapMem(pScrn);
#endif

   if(pAST->HWCInfoPtr) {
       xf86DestroyCursorInfoRec(pAST->HWCInfoPtr);
       pAST->HWCInfoPtr = NULL;
   }

#ifdef	Support_ShadowFB
   if (pAST->shadowFB_validation) {
      shadowRemove(pScreen, pScreen->GetScreenPixmap(pScreen));
	  free(pAST->shadow);
      pScreen->CreateScreenResources = pAST->CreateScreenResources;
   }
#endif

   pScrn->vtSema = FALSE;
   pScreen->CloseScreen = pAST->CloseScreen;
   return (*pScreen->CloseScreen) (pScreen);
}

static void
ASTSave(ScrnInfoPtr pScrn)
{
   ASTRecPtr pAST;
   ASTRegPtr astReg;
   int i, icount=0;
   ULONG ulData;

   pAST = ASTPTR(pScrn);
   astReg = &pAST->SavedReg;

   if (pAST->jChipType == AST1180)
   {
       for (i=0; i<12; i++)
       {
           ReadAST1180SOC(AST1180_GFX_BASE + AST1180_VGA1_CTRL+i*4, ulData);
           astReg->GFX[i] = ulData;
       }
   }
   else
   {
#if	defined(__sparc__) || defined(__mips__)
       UCHAR jReg;

       /* Save Misc */
       astReg->MISC = GetReg(MISC_PORT_READ);

       /* Save SR */
       for (i=0; i<4; i++)
           GetIndexReg(SEQ_PORT, (UCHAR) (i), astReg->SEQ[i]);

       /* Save CR */
       for (i=0; i<25; i++)
           GetIndexReg(CRTC_PORT, (UCHAR) (i), astReg->CRTC[i]);

       /* Save GR */
       for (i=0; i<9; i++)
           GetIndexReg(GR_PORT, (UCHAR) (i), astReg->GR[i]);

       /* Save AR */
       jReg = GetReg(INPUT_STATUS1_READ);
       for (i=0; i<20; i++)
           GetIndexReg(AR_PORT_WRITE, (UCHAR) (i), astReg->AR[i]);
       jReg = GetReg(INPUT_STATUS1_READ);
       SetReg (AR_PORT_WRITE, 0x20);		/* set POS */
#else
       vgaRegPtr vgaReg;
       vgaReg = &VGAHWPTR(pScrn)->SavedReg;

       /* do save */
       if (xf86IsPrimaryPci(pAST->PciInfo)) {
           vgaHWSave(pScrn, vgaReg, VGA_SR_MODE | VGA_SR_FONTS);
       }
       else {
           vgaHWSave(pScrn, vgaReg, VGA_SR_MODE);
       }
#endif

       /* Save Ext. */
       vASTOpenKey(pScrn);

       /* fixed Console Switch Refresh Rate Incorrect issue, ycchen@051106 */
       for (i=0x81; i<=0xB6; i++)
           GetIndexReg(CRTC_PORT, (UCHAR) (i), astReg->ExtCRTC[icount++]);
       for (i=0xBC; i<=0xC1; i++)
           GetIndexReg(CRTC_PORT, (UCHAR) (i), astReg->ExtCRTC[icount++]);
       GetIndexReg(CRTC_PORT, (UCHAR) (0xBB), astReg->ExtCRTC[icount]);

       /* Save DAC */
       for (i=0; i<256; i++)
           VGA_GET_PALETTE_INDEX (i, astReg->DAC[i][0], astReg->DAC[i][1], astReg->DAC[i][2]);

       /* Save 2D */
       astReg->ENG8044 = 0;
       GetIndexReg(CRTC_PORT, 0xA4, astReg->REGA4);
       if (astReg->REGA4 & 0x01)	/* 2D enabled */
           astReg->ENG8044 = *(ULONG *) (pAST->MMIOVirtualAddr + 0x8044);
   }

}

static void
ASTRestore(ScrnInfoPtr pScrn)
{
   ASTRecPtr pAST;
   ASTRegPtr astReg;
   int i, icount=0;
   ULONG ulData;

   pAST = ASTPTR(pScrn);
   astReg = &pAST->SavedReg;

   ASTDisplayPowerManagementSet(pScrn, DPMSModeOff, 0);

   if (pAST->jChipType == AST1180)
   {
       for (i=0; i<12; i++)
       {
           ulData = astReg->GFX[i];
           WriteAST1180SOC(AST1180_GFX_BASE + AST1180_VGA1_CTRL+i*4, ulData);
       }
   }
   else
   {
#if	defined(__sparc__) || defined(__mips__)
       UCHAR jReg;

       /* Restore Misc */
       SetReg(MISC_PORT_WRITE, astReg->MISC);

       /* Restore SR */
       for (i=0; i<4; i++)
           SetIndexReg(SEQ_PORT, (UCHAR) (i), astReg->SEQ[i]);

       /* Restore CR */
       SetIndexRegMask(CRTC_PORT,0x11, 0x7F, 0x00);
       for (i=0; i<25; i++)
           SetIndexReg(CRTC_PORT, (UCHAR) (i), astReg->CRTC[i]);

       /* Restore GR */
       for (i=0; i<9; i++)
           SetIndexReg(GR_PORT, (UCHAR) (i), astReg->GR[i]);

       /* Restore AR */
       jReg = GetReg(INPUT_STATUS1_READ);
       for (i=0; i<20; i++)
       {
           SetReg(AR_PORT_WRITE, (UCHAR) i);
           SetReg(AR_PORT_WRITE, astReg->AR[i]);
       }
       SetReg(AR_PORT_WRITE, 0x14);
       SetReg(AR_PORT_WRITE, 0x00);

       jReg = GetReg(INPUT_STATUS1_READ);
       SetReg (AR_PORT_WRITE, 0x20);		/* set POS */
#else
       vgaRegPtr vgaReg;
       vgaReg = &VGAHWPTR(pScrn)->SavedReg;

       /* do restore */
       vgaHWProtect(pScrn, TRUE);
       if (xf86IsPrimaryPci(pAST->PciInfo))
           vgaHWRestore(pScrn, vgaReg, VGA_SR_MODE | VGA_SR_FONTS);
       else
           vgaHWRestore(pScrn, vgaReg, VGA_SR_MODE);
       vgaHWProtect(pScrn, FALSE);
#endif

      /* Ext. restore */
      vASTOpenKey(pScrn);

       /* Restore DAC */
       for (i=0; i<256; i++)
          VGA_LOAD_PALETTE_INDEX (i, astReg->DAC[i][0], astReg->DAC[i][1], astReg->DAC[i][2]);

      /* fixed Console Switch Refresh Rate Incorrect issue, ycchen@051106 */
      for (i=0x81; i<=0xB6; i++)
          SetIndexReg(CRTC_PORT, (UCHAR) (i), astReg->ExtCRTC[icount++]);
      for (i=0xBC; i<=0xC1; i++)
          SetIndexReg(CRTC_PORT, (UCHAR) (i), astReg->ExtCRTC[icount++]);
      SetIndexReg(CRTC_PORT, (UCHAR) (0xBB), astReg->ExtCRTC[icount]);
   }

}

static void
ASTProbeDDC(ScrnInfoPtr pScrn, int index)
{
   ASTRecPtr pAST = ASTPTR(pScrn);
   unsigned char DDC_data[128];
   Bool Flags;

   if (xf86LoadSubModule(pScrn, "ddc"))
   {
      if (pAST->jChipType == AST1180)
          Flags = ASTGetVGA2EDID(pScrn, DDC_data);
      else if (pAST->jTxChipType == Tx_DP501)
      {
          Flags = ASTReadEDID_M68K(pScrn, DDC_data);
          if (Flags == FALSE)
              Flags = ASTGetVGAEDID(pScrn, DDC_data);
      }
      else
          Flags = ASTGetVGAEDID(pScrn, DDC_data);

      if (Flags)
      {
          ConfiguredMonitor = xf86InterpretEDID(pScrn->scrnIndex, DDC_data);
      }
      else
          xf86DrvMsg(pScrn->scrnIndex, X_INFO,"[ASTProbeDDC] Can't Get EDID Properly \n");
   }
   else
   {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO,"[ASTProbeDDC] Can't Load DDC Sub-Modules or Read EDID Failed \n");
   }

}

#define SkipDT	0x00
#define DT1	0x01
#define DT2 	0x02

static xf86MonPtr
ASTDoDDC(ScrnInfoPtr pScrn, int index)
{
   xf86MonPtr MonInfo = NULL;
   ASTRecPtr pAST = ASTPTR(pScrn);
   unsigned char DDC_data[128];
   Bool Flags;

   xf86MonPtr MonInfo1 = NULL, MonInfo2 = NULL;
   unsigned long i, j, k;
   struct monitor_ranges ranges, ranges1, ranges2;
   int DTSelect, dclock1=0, h_active1=0, v_active1=0, dclock2=0, h_active2=0, v_active2=0;
   struct std_timings stdtiming, *stdtiming1, *stdtiming2;

   /* Honour Option "noDDC" */
   if (xf86ReturnOptValBool(pAST->Options, OPTION_NO_DDC, FALSE)) {
      return MonInfo;
   }

   if (xf86LoadSubModule(pScrn, "ddc"))
   {
      if (pAST->jChipType == AST1180)
          Flags = ASTGetVGA2EDID(pScrn, DDC_data);
      else if (pAST->jTxChipType == Tx_DP501)
      {
	      pAST->DP501_MaxVCLK = 0xFF;
          Flags = ASTReadEDID_M68K(pScrn, DDC_data);
          if (Flags) pAST->DP501_MaxVCLK = ASTGetLinkMaxCLK(pScrn);
          else
              Flags = ASTGetVGAEDID(pScrn, DDC_data);
      }
      else
          Flags = ASTGetVGAEDID(pScrn, DDC_data);

      if (Flags)
      {
          MonInfo = MonInfo1 = xf86InterpretEDID(pScrn->scrnIndex, DDC_data);

          /* Valid Wide Screen Support */
          if ( (MonInfo) && (MonInfo->det_mon[0].type == 0x00) )
          {
	          if ( (MonInfo->det_mon[0].section.d_timings.h_active * 10 / MonInfo->det_mon[0].section.d_timings.v_active) < 14 )
                  pAST->SupportWideScreen = FALSE;
          }
      }

      /* For VGA2 CLONE Support, ycchen@012508 */
      if ((xf86ReturnOptValBool(pAST->Options, OPTION_VGA2_CLONE, FALSE)) || pAST->VGA2Clone) {
          if (ASTGetVGA2EDID(pScrn, DDC_data) == TRUE) {
              xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Get VGA2 EDID Correctly!! \n");
              MonInfo2 = xf86InterpretEDID(pScrn->scrnIndex, DDC_data);
              if (MonInfo1 == NULL)	/* No DDC1 EDID */
                  MonInfo = MonInfo2;
              else {			/* Check with VGA1 & VGA2 EDID */
                   /* Update establishment timing */
                   MonInfo->timings1.t1 = MonInfo1->timings1.t1 & MonInfo2->timings1.t1;
                   MonInfo->timings1.t2 = MonInfo1->timings1.t2 & MonInfo2->timings1.t2;
                   MonInfo->timings1.t_manu = MonInfo1->timings1.t_manu & MonInfo2->timings1.t_manu;

                   /* Update Std. Timing */
                   for (i=0; i<8; i++) {
                       stdtiming.hsize = stdtiming.vsize = stdtiming.refresh = stdtiming.id = 0;
                       for (j=0; j<8; j++) {
                           if ((MonInfo1->timings2[i].hsize == MonInfo2->timings2[j].hsize) && \
                               (MonInfo1->timings2[i].vsize == MonInfo2->timings2[j].vsize) && \
                               (MonInfo1->timings2[i].refresh == MonInfo2->timings2[j].refresh)) {
                                stdtiming = MonInfo1->timings2[i];
                                break;
                           }
                       }

                       MonInfo->timings2[i] = stdtiming;
                   } /* Std. Timing */

                   /* Get Detailed Timing */
                   for (i=0;i<4;i++) {
                      if (MonInfo1->det_mon[i].type == 0xFD)
                         ranges1 = MonInfo1->det_mon[i].section.ranges;
                      else if (MonInfo1->det_mon[i].type == 0xFA)
                         stdtiming1 = MonInfo1->det_mon[i].section.std_t;
                      else if (MonInfo1->det_mon[i].type == 0x00) {
                         if (MonInfo1->det_mon[i].section.d_timings.clock > dclock1)
                             dclock1 = MonInfo1->det_mon[i].section.d_timings.clock;
                         if (MonInfo1->det_mon[i].section.d_timings.h_active > h_active1)
                             h_active1 = MonInfo1->det_mon[i].section.d_timings.h_active;
                         if (MonInfo1->det_mon[i].section.d_timings.v_active > v_active1)
                             v_active1 = MonInfo1->det_mon[i].section.d_timings.v_active;
                      }
                      if (MonInfo2->det_mon[i].type == 0xFD)
                         ranges2 = MonInfo2->det_mon[i].section.ranges;
                      else if (MonInfo1->det_mon[i].type == 0xFA)
                         stdtiming2 = MonInfo2->det_mon[i].section.std_t;
                      else if (MonInfo2->det_mon[i].type == 0x00) {
                         if (MonInfo2->det_mon[i].section.d_timings.clock > dclock2)
                             dclock2 = MonInfo2->det_mon[i].section.d_timings.clock;
                         if (MonInfo2->det_mon[i].section.d_timings.h_active > h_active2)
                             h_active2 = MonInfo2->det_mon[i].section.d_timings.h_active;
                         if (MonInfo2->det_mon[i].section.d_timings.v_active > v_active2)
                             v_active2 = MonInfo2->det_mon[i].section.d_timings.v_active;
                      }
                   } /* Get Detailed Timing */

                   /* Chk Detailed Timing */
                   if ((dclock1 >= dclock2) && (h_active1 >= h_active2) && (v_active1 >= v_active2))
                       DTSelect = DT2;
                   else if ((dclock2 >= dclock1) && (h_active2 >= h_active1) && (v_active2 >= v_active1))
                       DTSelect = DT1;
                   else
                       DTSelect = SkipDT;

                   /* Chk Monitor Descriptor */
                   ranges = ranges1;
                   ranges.min_h = ranges1.min_h > ranges2.min_h ? ranges1.min_h:ranges2.min_h;
                   ranges.min_v = ranges1.min_v > ranges2.min_v ? ranges1.min_v:ranges2.min_v;
                   ranges.max_h = ranges1.max_h < ranges2.max_h ? ranges1.max_h:ranges2.max_h;
                   ranges.max_v = ranges1.max_v < ranges2.max_v ? ranges1.max_v:ranges2.max_v;
                   ranges.max_clock = ranges1.max_clock < ranges2.max_clock ? ranges1.max_clock:ranges2.max_clock;

                   /* Update Detailed Timing */
                   for (i=0; i<4; i++)
                   {
                       if (MonInfo->det_mon[i].type == 0xFD) {
                           MonInfo->det_mon[i].section.ranges = ranges;
                       }
                       else if (MonInfo->det_mon[i].type == 0xFA) {
                           for (j=0; j<5; j++) {
         	                  stdtiming.hsize = stdtiming.vsize = stdtiming.refresh = stdtiming.id = 0;
                               for (k=0; k<5; k++) {
                                   if ((stdtiming1[j].hsize == stdtiming2[k].hsize) && \
                                       (stdtiming1[j].vsize == stdtiming2[k].vsize) && \
                                       (stdtiming1[j].refresh == stdtiming2[k].refresh)) {
                                        stdtiming = stdtiming1[j];
                                        break;
                                   }
                               }
                               stdtiming1[j] = stdtiming;
                           } /* Std. Timing */
                       } /* FA */
                       else if (MonInfo->det_mon[i].type == 0x00) {
                           if (DTSelect == DT2)
                               MonInfo->det_mon[i] = MonInfo2->det_mon[i];
                           else if (DTSelect == DT1)
                               MonInfo->det_mon[i] = MonInfo1->det_mon[i];
                           else /* SkipDT */
                           {   /* use 1024x768 as default */
                               MonInfo->det_mon[i] = MonInfo1->det_mon[i];
                               MonInfo->det_mon[i].section.d_timings.clock = 65000000;
                               MonInfo->det_mon[i].section.d_timings.h_active = 1024;
                               MonInfo->det_mon[i].section.d_timings.h_blanking = 320;
                               MonInfo->det_mon[i].section.d_timings.v_active = 768;
                               MonInfo->det_mon[i].section.d_timings.v_blanking = 38;
                               MonInfo->det_mon[i].section.d_timings.h_sync_off = 24;
                               MonInfo->det_mon[i].section.d_timings.h_sync_width = 136;
                               MonInfo->det_mon[i].section.d_timings.v_sync_off = 3;
                               MonInfo->det_mon[i].section.d_timings.v_sync_width = 6;
                           }
                       } /* 00 */
                       else { /* use Monitor 1 as default */
                           MonInfo->det_mon[i] = MonInfo1->det_mon[i];
                       }

                   } /* Update Detailed Timing */

                   /* set feature size */
                   if (DTSelect == DT2)  {
                       MonInfo->features.hsize = MonInfo2->features.hsize;
                       MonInfo->features.vsize = MonInfo2->features.vsize;
                   }
                   else if (DTSelect == DT1)  {
                       MonInfo->features.hsize = MonInfo1->features.hsize;
                       MonInfo->features.vsize = MonInfo1->features.vsize;
                   }
                   else	/* Skip DT */
                   {   /* use 1024x768 as default */
                       MonInfo->features.hsize = 0x20;
                       MonInfo->features.vsize = 0x18;
                   }

               } /* Check with VGA1 & VGA2 EDID */

           } /* ASTGetVGA2EDID */
           else {
               xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Can't Get VGA2 EDID Correctly!! \n");
           }
      } /* VGA2Clone */

      xf86PrintEDID(MonInfo);
      xf86SetDDCproperties(pScrn, MonInfo);
   }
   else
   {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO,"[ASTDoDDC] Can't Load DDC Sub-Modules or Read EDID Failed \n");
   }

   /* Fill pAST Monitor Info */
   if (MonInfo == NULL)
   {	/* default for Non-EDID */
       pAST->mon_h_active = 1024;
       pAST->mon_v_active = 768;
   }
   else
   {	/* save MonInfo to Private */
       pAST->mon_h_active = MonInfo->det_mon[0].section.d_timings.h_active;
       pAST->mon_v_active = MonInfo->det_mon[0].section.d_timings.v_active;
   }

   return MonInfo;
}

static void
vFillASTModeInfo (ScrnInfoPtr pScrn)
{
    ASTRecPtr pAST;

    pAST = ASTPTR(pScrn);

    pAST->VideoModeInfo.ScreenWidth = pScrn->virtualX;
    pAST->VideoModeInfo.ScreenHeight = pScrn->virtualY;
    pAST->VideoModeInfo.bitsPerPixel = pScrn->bitsPerPixel;
    /* Fixed screen pitch incorrect in some specific monitor, ycchen@071707 */
    pAST->VideoModeInfo.ScreenPitch = pScrn->displayWidth * ((pScrn->bitsPerPixel + 1) / 8) ;

}

static Bool
ASTModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    ASTRecPtr pAST;

    pAST = ASTPTR(pScrn);

    pScrn->vtSema = TRUE;
    pAST->ModePtr = mode;

#if	defined(__sparc__) || defined(__mips__)
    if (!ASTSetMode(pScrn, mode))
      return FALSE;
#else
    {
        vgaHWPtr hwp;

        hwp = VGAHWPTR(pScrn);

        vgaHWUnlock(hwp);
    }

    if (!vgaHWInit(pScrn, mode))
      return FALSE;

    pScrn->vtSema = TRUE;
    pAST->ModePtr = mode;

    if (!ASTSetMode(pScrn, mode))
      return FALSE;

    vgaHWProtect(pScrn, FALSE);
#endif

    return TRUE;
}

#ifdef AstVideo
/*
 * Video Part by ic_yang
 */
#include "fourcc.h"

#define NUM_ATTRIBUTES  	8
#define NUM_IMAGES 		8
#define NUM_FORMATS     	3

#define IMAGE_MIN_WIDTH         32
#define IMAGE_MIN_HEIGHT        24
#define IMAGE_MAX_WIDTH         1920
#define IMAGE_MAX_HEIGHT        1080

#define MAKE_ATOM(a) MakeAtom(a, sizeof(a) - 1, TRUE)

static XF86ImageRec ASTImages[NUM_IMAGES] =
{
    XVIMAGE_YUY2, /* If order is changed, ASTOffscreenImages must be adapted */
};

static XF86VideoFormatRec ASTFormats[NUM_FORMATS] =
{
   { 8, PseudoColor},
   {16, TrueColor},
   {24, TrueColor}
};

/* client libraries expect an encoding */
static XF86VideoEncodingRec DummyEncoding =
{
   0,
   "XV_IMAGE",
   0, 0,                /* Will be filled in */
   {1, 1}
};

static char astxvcolorkey[] 				= "XV_COLORKEY";
static char astxvbrightness[] 				= "XV_BRIGHTNESS";
static char astxvcontrast[] 				= "XV_CONTRAST";
static char astxvsaturation[] 				= "XV_SATURATION";
static char astxvhue[] 				        = "XV_HUE";
static char astxvgammared[] 				= "XV_GAMMA_RED";
static char astxvgammagreen[] 				= "XV_GAMMA_GREEN";
static char astxvgammablue[] 				= "XV_GAMMA_BLUE";

static XF86AttributeRec ASTAttributes[NUM_ATTRIBUTES] =
{
   {XvSettable | XvGettable, 0, (1 << 24) - 1, astxvcolorkey},
   {XvSettable | XvGettable, -128, 127, astxvbrightness},
   {XvSettable | XvGettable, 0, 255, astxvcontrast},
   {XvSettable | XvGettable, -180, 180, astxvsaturation},
   {XvSettable | XvGettable, -180, 180, astxvhue},
   {XvSettable | XvGettable, 100, 10000, astxvgammared},
   {XvSettable | XvGettable, 100, 10000, astxvgammagreen},
   {XvSettable | XvGettable, 100, 10000, astxvgammablue},
};

static void ASTStopVideo(ScrnInfoPtr pScrn, pointer data, Bool exit)
{
    ASTPortPrivPtr pPriv = (ASTPortPrivPtr)data;

    REGION_EMPTY(pScrn->pScreen, &pPriv->clip);

    if(exit)
    {
        if(pPriv->fbAreaPtr)
        {
            xf86FreeOffscreenArea(pPriv->fbAreaPtr);
            pPriv->fbAreaPtr = NULL;
            pPriv->fbSize = 0;
        }
        /* clear all flag */
        pPriv->videoStatus = 0;
    }
    else
    {
#if 0
        if(pPriv->videoStatus & CLIENT_VIDEO_ON)
        {
            pPriv->videoStatus |= OFF_TIMER;

        }
#endif
    }
}

static int ASTSetPortAttribute(ScrnInfoPtr pScrn, Atom attribute, INT32 value, pointer data)
{
    ASTPortPrivPtr pPriv = (ASTPortPrivPtr)data;
    ASTPtr pAST = ASTPTR(pScrn);

    xf86DrvMsg(pScrn->scrnIndex, X_INFO,"ASTSetPortAttribute(),attribute=%x\n", attribute);

    if (attribute == pAST->xvBrightness)
    {
        if((value < -128) || (value > 127))
         return BadValue;

        pPriv->brightness = value;
    }
    else if (attribute == pAST->xvContrast)
    {
        if ((value < 0) || (value > 255))
         return BadValue;

        pPriv->contrast = value;
    }
    else if (attribute == pAST->xvSaturation)
    {
        if ((value < -180) || (value > 180))
         return BadValue;

        pPriv->saturation = value;
    }
    else if (attribute == pAST->xvHue)
    {
        if ((value < -180) || (value > 180))
         return BadValue;

        pPriv->hue = value;
    }
    else if (attribute == pAST->xvColorKey)
    {
          pPriv->colorKey = value;
          REGION_EMPTY(pScrn->pScreen, &pPriv->clip);
    }
    else if(attribute == pAST->xvGammaRed)
    {
        if((value < 100) || (value > 10000))
            return BadValue;
        pPriv->gammaR = value;
    }
    else if(attribute == pAST->xvGammaGreen)
    {
        if((value < 100) || (value > 10000))
            return BadValue;
        pPriv->gammaG = value;
    }
    else if(attribute == pAST->xvGammaBlue)
    {
        if((value < 100) || (value > 10000))
            return BadValue;
        pPriv->gammaB = value;
    }
    else
    {
        return BadMatch;
    }

    return Success;
}

static int ASTGetPortAttribute(ScrnInfoPtr pScrn, Atom attribute, INT32 *value, pointer data)
{
    ASTPortPrivPtr pPriv = (ASTPortPrivPtr)data;
    ASTPtr pAST = ASTPTR(pScrn);

    xf86DrvMsg(pScrn->scrnIndex, X_INFO,"ASTGetPortAttribute(),attribute=%x\n", attribute);

    if (attribute == pAST->xvBrightness)
    {
        *value = pPriv->brightness;
    }
    else if (attribute == pAST->xvContrast)
    {
        *value = pPriv->contrast;
    }
    else if (attribute == pAST->xvSaturation)
    {
        *value = pPriv->saturation;
    }
    else if (attribute == pAST->xvHue)
    {
        *value = pPriv->hue;
    }
    else if(attribute == pAST->xvGammaRed)
    {
        *value = pPriv->gammaR;

    }
    else if(attribute == pAST->xvGammaGreen)
    {
        *value = pPriv->gammaG;
    }
    else if(attribute == pAST->xvGammaBlue)
    {
        *value = pPriv->gammaB;
    }
    else if (attribute == pAST->xvColorKey)
    {
        *value = pPriv->colorKey;
    }
    else
        return BadMatch;

    return Success;
}

static void ASTQueryBestSize(ScrnInfoPtr pScrn, Bool motion,
                                short vid_w, short vid_h,
                                short drw_w, short drw_h,
                                unsigned int *p_w, unsigned int *p_h,
                                pointer data)
{
    *p_w = drw_w;
    *p_h = drw_h;
    xf86DrvMsg(pScrn->scrnIndex, X_INFO,"ASTQueryBestSize()\n");
  /* TODO: report the HW limitation */
}

static int ASTQueryImageAttributes(ScrnInfoPtr pScrn, int id,
                                    unsigned short *w, unsigned short *h,
                                    int *pitches, int *offsets)
{
    int pitchY, pitchUV;
    int size, sizeY, sizeUV;

    xf86DrvMsg(pScrn->scrnIndex, X_INFO,"ASTQueryImageAttributes()\n");

    if(*w < IMAGE_MIN_WIDTH) *w = IMAGE_MIN_WIDTH;
    if(*h < IMAGE_MIN_HEIGHT) *h = IMAGE_MIN_HEIGHT;

    switch(id) {
    case PIXEL_FMT_YV12:
        *w = (*w + 7) & ~7;
        *h = (*h + 1) & ~1;
        pitchY = *w;
        pitchUV = *w >> 1;
        if(pitches) {
          pitches[0] = pitchY;
          pitches[1] = pitches[2] = pitchUV;
        }
        sizeY = pitchY * (*h);
        sizeUV = pitchUV * ((*h) >> 1);
        if(offsets) {
          offsets[0] = 0;
          offsets[1] = sizeY;
          offsets[2] = sizeY + sizeUV;
        }
        size = sizeY + (sizeUV << 1);
        break;
    case PIXEL_FMT_NV12:
    case PIXEL_FMT_NV21:
        *w = (*w + 7) & ~7;
        *h = (*h + 1) & ~1;
		pitchY = *w;
    	pitchUV = *w;
    	if(pitches) {
      	    pitches[0] = pitchY;
            pitches[1] = pitchUV;
        }
    	sizeY = pitchY * (*h);
    	sizeUV = pitchUV * ((*h) >> 1);
    	if(offsets) {
          offsets[0] = 0;
          offsets[1] = sizeY;
        }
        size = sizeY + (sizeUV << 1);
        break;
    case PIXEL_FMT_YUY2:
    case PIXEL_FMT_UYVY:
    case PIXEL_FMT_YVYU:
    case PIXEL_FMT_RGB6:
    case PIXEL_FMT_RGB5:
    default:
        *w = (*w + 1) & ~1;
        pitchY = *w << 1;
        if(pitches) pitches[0] = pitchY;
        if(offsets) offsets[0] = 0;
        size = pitchY * (*h);
        break;
    }

    return size;
}

static int ASTPutImage(ScrnInfoPtr pScrn,
                          short src_x, short src_y,
                          short drw_x, short drw_y,
                          short src_w, short src_h,
                          short drw_w, short drw_h,
                          int id, unsigned char* buf,
                          short width, short height,
                          Bool sync,
                          RegionPtr clipBoxes, pointer data, DrawablePtr pDraw)
{
    ASTPtr pAST = ASTPTR(pScrn);
    ASTPortPrivPtr pPriv = (ASTPortPrivPtr)data;
    int i;
    int totalSize=0;

    xf86DrvMsg(pScrn->scrnIndex, X_INFO,"ASTPutImage()\n");
    /*   int depth = pAST->CurrentLayout.bitsPerPixel >> 3; */

    pPriv->drw_x = drw_x;
    pPriv->drw_y = drw_y;
    pPriv->drw_w = drw_w;
    pPriv->drw_h = drw_h;
    pPriv->src_x = src_x;
    pPriv->src_y = src_y;
    pPriv->src_w = src_w;
    pPriv->src_h = src_h;
    pPriv->id = id;
    pPriv->height = height;

    switch(id)
    {
    case PIXEL_FMT_YV12:
    case PIXEL_FMT_NV12:
    case PIXEL_FMT_NV21:
        pPriv->srcPitch = (width + 7) & ~7;
        totalSize = (pPriv->srcPitch * height * 3) >> 1; /* Verified */
    break;
    case PIXEL_FMT_YUY2:
    case PIXEL_FMT_UYVY:
    case PIXEL_FMT_YVYU:
    case PIXEL_FMT_RGB6:
    case PIXEL_FMT_RGB5:
    default:
        pPriv->srcPitch = ((width << 1) + 3) & ~3;	/* Verified */
        totalSize = pPriv->srcPitch * height;
    }

    totalSize += 15;
    totalSize &= ~15;
    /* allocate memory */

    if(totalSize == pPriv->fbSize)
    {
        ;
    }
    else
    {
        int lines, pitch, depth;
        BoxPtr pBox = NULL;

        pPriv->fbSize = totalSize;

        if(pPriv->fbAreaPtr)
        {
             xf86FreeOffscreenArea(pPriv->fbAreaPtr);
        }

        depth = (pScrn->bitsPerPixel + 7 ) / 8;
        pitch = pScrn->displayWidth * depth;
        lines = ((totalSize * 2) / pitch) + 1;
        xf86DrvMsg(pScrn->scrnIndex, X_INFO,"ASTPutImagelines=%x, pitch=%x, displayWidth=%x\n", lines, pitch, pScrn->displayWidth);


        pPriv->fbAreaPtr = xf86AllocateOffscreenArea(pScrn->pScreen,
                                 pScrn->displayWidth,
                                lines, 0, NULL, NULL, NULL);

        if(!pPriv->fbAreaPtr)
        {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Allocate video memory fails\n");
            return BadAlloc;
        }

        pBox = &(pPriv->fbAreaPtr->box);
        pPriv->bufAddr[0] = (pBox->y1 * pitch) + (pBox->x1 * depth);
        pPriv->bufAddr[1] = pPriv->bufAddr[0] + totalSize;
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Put Image, pPriv->bufAddr[0]=0x%08X\n", pPriv->bufAddr[0]);

    }

    /* copy data */
    if(totalSize < 16)
    {
      #ifdef NewPath
        memcpy(pAST->FBVirtualAddr + pPriv->bufAddr[pPriv->currentBuf], buf, totalSize);
      #else /* NewPath */
        switch(id)
        {
        case PIXEL_FMT_YUY2:
        case PIXEL_FMT_UYVY:
        case PIXEL_FMT_YVYU:
        {
             BYTE *Base = (BYTE *)(pAST->FBVirtualAddr + pPriv->bufAddr[pPriv->currentBuf]);
             for(i=0; i<height; i++)
                  memcpy( Base + i * pPriv->srcPitch, buf + i*width*2, width*2);
             break;
        }
        default:
            memcpy(pAST->FBVirtualAddr + pPriv->bufAddr[pPriv->currentBuf], buf, totalSize);
            break;
        } /* switch */
      #endif /* NewPath */
    }
    else
    {
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Put Image, copy buf\n");

      #ifdef NewPath
       	memcpy(pAST->FBVirtualAddr + pPriv->bufAddr[pPriv->currentBuf], buf, totalSize);
      #else     /* NewPath */
        switch(id)
        {
        case PIXEL_FMT_YUY2:
        case PIXEL_FMT_UYVY:
        case PIXEL_FMT_YVYU:
        {
            BYTE *Base = (BYTE *)(pAST->FBVirtualAddr + pPriv->bufAddr[pPriv->currentBuf]);
            for(i=0; i<height; i++)
                  memcpy( Base + i * pPriv->srcPitch, buf + i*width*2, width*2);

            /*for(i=0; i<height; i++)
                for(j=0; j<width*2; j++)
                    *(Base+i*pPriv->srcPitch+j) = *(buf + width*i + j);*/
            break;
        }
        default:
        {    BYTE *Base = (BYTE *)(pAST->FBVirtualAddr + pPriv->bufAddr[pPriv->currentBuf]);
            int j;
            for(i=0; i<height; i++)
                for(j=0; j<width; j++)
                   *(Base + width*i + j) = *(buf + width * i + j);
        break;
        }
        } /* end of switch */
      #endif    /* NewPath */
    }

    ASTDisplayVideo(pScrn, pPriv, clipBoxes, id);

    /* update cliplist
    if(!REGION_EQUAL(pScrn->pScreen, &pPriv->clip, clipBoxes))
    {
        REGION_COPY(pScrn->pScreen, &pPriv->clip, clipBoxes);
    }
    else
    {
        xf86XVFillKeyHelper(pScrn->pScreen, 0xFFFFFFFF, clipBoxes);
    }
    */
    pPriv->currentBuf ^= 1;

    return Success;
}

static XF86VideoAdaptorPtr ASTSetupImageVideo(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    ASTPtr pAST = ASTPTR(pScrn);
    XF86VideoAdaptorPtr adapt;
    ASTPortPrivPtr pPriv;


    if(!(adapt = calloc(1, sizeof(XF86VideoAdaptorRec) +
                            sizeof(DevUnion) +
                            sizeof(ASTPortPrivRec))))
        return NULL;

    adapt->type = XvWindowMask | XvInputMask | XvImageMask | XvVideoMask;
    adapt->flags = VIDEO_OVERLAID_IMAGES | VIDEO_CLIP_TO_VIEWPORT;
    adapt->name = "AST Video";

    adapt->nEncodings = 1;
    adapt->pEncodings = &DummyEncoding;

    adapt->nFormats = NUM_FORMATS;
    adapt->pFormats = ASTFormats;
    adapt->nPorts = 1;
    adapt->pPortPrivates = (DevUnion*)(&adapt[1]);

    pPriv = (ASTPortPrivPtr)(&adapt->pPortPrivates[1]);

    adapt->pPortPrivates->ptr = (pointer)(pPriv);
    adapt->pAttributes = ASTAttributes;
    adapt->nAttributes = NUM_ATTRIBUTES;
	adapt->nImages = NUM_IMAGES;
    adapt->pImages = ASTImages;

    adapt->PutVideo = NULL;

    adapt->PutStill = NULL;
    adapt->GetVideo = NULL;
    adapt->GetStill = NULL;
    adapt->StopVideo = ASTStopVideo;
    adapt->SetPortAttribute = ASTSetPortAttribute;
    adapt->GetPortAttribute = ASTGetPortAttribute;
    adapt->QueryBestSize = ASTQueryBestSize;
    adapt->PutImage = ASTPutImage;
    adapt->QueryImageAttributes = ASTQueryImageAttributes;


    pPriv->currentBuf   = 0;
    pPriv->linear       = NULL;
    pPriv->fbAreaPtr    = NULL;
    pPriv->fbSize = 0;
	pPriv->videoStatus  = 0;

    pPriv->colorKey     = 0x000101fe;
    pPriv->brightness   = 0;
    pPriv->contrast     = 128;
    pPriv->saturation   = 0;
    pPriv->hue          = 0;

    /* gotta uninit this someplace */
    REGION_NULL(pScreen, &pPriv->clip);

	pAST->adaptor = adapt;

	pAST->xvBrightness = MAKE_ATOM(astxvbrightness);
	pAST->xvContrast   = MAKE_ATOM(astxvcontrast);
	pAST->xvColorKey   = MAKE_ATOM(astxvcolorkey);
	pAST->xvSaturation = MAKE_ATOM(astxvsaturation);
	pAST->xvHue 	   = MAKE_ATOM(astxvhue);
	pAST->xvGammaRed   = MAKE_ATOM(astxvgammared);
    pAST->xvGammaGreen = MAKE_ATOM(astxvgammagreen);
    pAST->xvGammaBlue  = MAKE_ATOM(astxvgammablue);

    return adapt;
}

void ASTInitVideo(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    XF86VideoAdaptorPtr *adaptors, *newAdaptors = NULL;
    XF86VideoAdaptorPtr ASTAdaptor = NULL;
    int num_adaptors;

    ASTAdaptor = ASTSetupImageVideo(pScreen);

    num_adaptors = xf86XVListGenericAdaptors(pScrn, &adaptors);

    if(ASTAdaptor)
    {
        if(!num_adaptors)
        {
            num_adaptors = 1;
            adaptors = &ASTAdaptor;
        }
        else
        {
            newAdaptors = malloc((num_adaptors + 1) * sizeof(XF86VideoAdaptorPtr*));
            if(newAdaptors)
            {
                memcpy(newAdaptors, adaptors, num_adaptors *
                                        sizeof(XF86VideoAdaptorPtr));
                newAdaptors[num_adaptors] = ASTAdaptor;
                adaptors = newAdaptors;
                num_adaptors++;
            }
        }
    }

    if(num_adaptors)
        xf86XVScreenInit(pScreen, adaptors, num_adaptors);

    if(newAdaptors)
        free(newAdaptors);

}
#endif /* AstVideo */
