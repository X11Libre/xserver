/*
 * Driver for CL-GD5480.
 * Itai Nahshon.
 *
 * Support for the CL-GD7548: David Monniaux
 *
 * This is mainly a cut & paste from the MGA driver.
 * Original authors and contributors list include:
 *    Radoslaw Kapitan, Andrew Vanderstock, Dirk Hohndel,
 *    David Dawes, Andrew E. Mileski, Leonard N. Zubkoff,
 *    Guy DESBIEF
 */
#include <dix-config.h>

/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"

/* All drivers need this */

/* Everything using inb/outb, etc needs "compiler.h" */
#include "compiler.h"

/* Drivers that need to access the PCI config space directly need this */
#include "xf86Pci.h"

/* All drivers using the vgahw module need this */
/* This driver needs to be modified to not use vgaHW for multihead operation */
#include "vgaHW.h"

/* All drivers initialising the SW cursor need this */
#include "mipointer.h"

#include "micmap.h"

/* Needed by the Shadow Framebuffer */
#include "shadowfb.h"

/* Note: can HWCUR64 be set even though the hw cursor is disabled for
   want of memory ? */

/* Framebuffer memory manager */
#include "xf86fbman.h"

#include "fb.h"


#include "xf86DDC.h"
#include "xf86int10.h"

#include "cir.h"
#define _ALP_PRIVATE_
#include "alp.h"

#include "xf86xv.h"
#include <X11/extensions/Xv.h>

#ifdef ALPPROBEI2C
/* For debugging... should go away. */
static void AlpProbeI2C(int scrnIndex);
#endif

/*
 * Forward definitions for the functions that make up the driver.
 */

/* Mandatory functions */

Bool AlpPreInit(ScrnInfoPtr pScrn, int flags);
Bool AlpScreenInit(ScreenPtr pScreen, int argc, char **argv);
Bool AlpEnterVT(ScrnInfoPtr arg);
void AlpLeaveVT(ScrnInfoPtr arg);
static Bool	AlpCloseScreen(ScreenPtr pScreen);
static Bool	AlpSaveScreen(ScreenPtr pScreen, int mode);

/* Required if the driver supports mode switching */
Bool AlpSwitchMode(ScrnInfoPtr arg, DisplayModePtr mode);
/* Required if the driver supports moving the viewport */
void AlpAdjustFrame(ScrnInfoPtr arg, int x, int y);

/* Optional functions */
void AlpFreeScreen(ScrnInfoPtr arg);
ModeStatus AlpValidMode(ScrnInfoPtr arg, DisplayModePtr mode,
			Bool verbose, int flags);
/* Internally used functions */
static void	AlpSave(ScrnInfoPtr pScrn);
static void	AlpRestore(ScrnInfoPtr pScrn);
static Bool	AlpModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);

static void AlpProbeLCD(ScrnInfoPtr pScrn);

static void AlpSetClock(CirPtr pCir, vgaHWPtr hwp, int freq);

static void AlpOffscreenAccelInit(ScrnInfoPtr pScrn);

static void	AlpDisplayPowerManagementSet(ScrnInfoPtr pScrn,
                                             int PowerManagementMode, int flags);

/*
 * This is intentionally screen-independent.  It indicates the binding
 * choice made in the first PreInit.
 */
static int pix24bpp = 0;

typedef enum {
	OPTION_HW_CURSOR,
	OPTION_PCI_RETRY,
	OPTION_NOACCEL,
	OPTION_MMIO,
	OPTION_ROTATE,
	OPTION_SHADOW_FB,
	OPTION_MEMCFG1,
	OPTION_MEMCFG2
} CirOpts;

static const OptionInfoRec CirOptions[] = {
	{ OPTION_HW_CURSOR,	"HWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
	{ OPTION_NOACCEL,	"NoAccel",	OPTV_BOOLEAN,	{0}, FALSE },
	{ OPTION_MMIO,		"MMIO",		OPTV_BOOLEAN,	{0}, FALSE },
	{ OPTION_SHADOW_FB,	"ShadowFB",	OPTV_BOOLEAN,	{0}, FALSE },
	{ OPTION_ROTATE, 	 "Rotate",	OPTV_ANYSTR,	{0}, FALSE },
	{ OPTION_MEMCFG1,	"MemCFG1",	OPTV_INTEGER,	{0}, -1 },
	{ OPTION_MEMCFG2,	"MemCFG2",	OPTV_INTEGER,	{0}, -1 },
	{ -1,				NULL,		OPTV_NONE,		{0}, FALSE }
};

/*                                1/4bpp   8bpp   15/16bpp  24bpp  32bpp
static int unsupp_MaxClocks[] = {      0,      0,      0,      0,      0 }; */
static int gd5430_MaxClocks[] = {  85500,  85500,  50000,  28500,      0 };
static int gd5446_MaxClocks[] = { 135100, 135100,  85500,  85500,      0 };
static int gd5480_MaxClocks[] = { 135100, 200000, 200000, 135100, 135100 };
static int gd7548_MaxClocks[] = {  80100,  80100,  80100,  80100,  80100 };
static int gd7555_MaxClocks[] = {  80100,  80100,  80100,  80100,  80100 };
static int gd7556_MaxClocks[] = {  80100,  80100,  80100,  80100,  80100 };

#ifdef XFree86LOADER

#define ALP_MAJOR_VERSION 1
#define ALP_MINOR_VERSION 0
#define ALP_PATCHLEVEL 0

static XF86ModuleVersionInfo alpVersRec =
{
	"cirrus_alpine",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XORG_VERSION_CURRENT,
	ALP_MAJOR_VERSION, ALP_MINOR_VERSION, ALP_PATCHLEVEL,
	ABI_CLASS_VIDEODRV,			/* This is a video driver */
	ABI_VIDEODRV_VERSION,
	MOD_CLASS_NONE,
	{0,0,0,0}
};

/*
 * This is the module init data.
 * Its name has to be the driver name followed by ModuleData.
 */
_X_EXPORT XF86ModuleData cirrus_alpineModuleData = {
    &alpVersRec,
    NULL,
    NULL
};

#endif /* XFree86LOADER */

_X_EXPORT const OptionInfoRec *
AlpAvailableOptions(int chipid)
{
    return CirOptions;
}

_X_EXPORT ScrnInfoPtr
AlpProbe(int entity)
{
    ScrnInfoPtr pScrn = NULL;
    
    if ((pScrn = xf86ConfigPciEntity(pScrn, 0, entity, CIRPciChipsets,
					   NULL,NULL, NULL, NULL, NULL))) {
	pScrn->PreInit		= AlpPreInit;
	pScrn->ScreenInit	= AlpScreenInit;
	pScrn->SwitchMode	= AlpSwitchMode;
	pScrn->AdjustFrame	= AlpAdjustFrame;
	pScrn->EnterVT		= AlpEnterVT;
	pScrn->LeaveVT		= AlpLeaveVT;
	pScrn->FreeScreen	= AlpFreeScreen;
	pScrn->ValidMode	= AlpValidMode;
    }

    return pScrn;
}


static Bool
AlpGetRec(ScrnInfoPtr pScrn)
{
#ifdef ALP_DEBUG
	ErrorF("AlpGetRec\n");
#endif
	if (pScrn->driverPrivate != NULL)
		return TRUE;

	pScrn->driverPrivate = XNFcallocarray(sizeof(CirRec), 1);
	((CirPtr)pScrn->driverPrivate)->chip.alp = XNFcallocarray(sizeof(AlpRec),1);

#ifdef ALP_DEBUG
	ErrorF("AlpGetRec 0x%x\n", CIRPTR(pScrn));
#endif
	return TRUE;
}

static void
AlpFreeRec(ScrnInfoPtr pScrn)
{
	if (pScrn->driverPrivate == NULL)
		return;
	free(pScrn->driverPrivate);
	pScrn->driverPrivate = NULL;
}


/*
 * AlpCountRAM --
 *
 * Counts amount of installed RAM
 *
 * XXX Can use options to configure memory on non-primary cards.
 */
static int
AlpCountRam(ScrnInfoPtr pScrn)
{
    CirPtr pCir = CIRPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    MessageType from;
    int videoram = 0;
    
    /* Map the Alp memory and MMIO areas */
    pCir->FbMapSize = 1024*1024; /* XX temp */
    if (!pCir->IoMapSize)
    	pCir->IoMapSize = 0x4000;	/* 16K for moment */
    if (!CirMapMem(pCir, pScrn->scrnIndex))
	return 0;

    /* The 754x supports MMIO for the BitBlt engine but
       not for the VGA registers */
    switch (pCir->Chipset)
    {
    case PCI_CHIP_GD7548:
      break;
    default:
      if (pCir->UseMMIO)
	vgaHWSetMmioFuncs(hwp, pCir->IOBase, -0x3C0);
    }

    if (pCir->chip.alp->sr0f != (CARD32)-1) {
	from = X_CONFIG;
	hwp->writeSeq(hwp, 0x0F, pCir->chip.alp->sr0f);
    } else {
	from = X_PROBED;
	pCir->chip.alp->sr0f = hwp->readSeq(hwp, 0x0F);
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Memory Config reg 1 is 0x%02X\n",
	       (unsigned int)pCir->chip.alp->sr0f);
    
    switch (pCir->Chipset) {
    case PCI_CHIP_GD5430:
/*  case PCI_CHIP_GD5440: */
	switch (pCir->chip.alp->sr0f & 0x18) {
	case 0x08:
	    videoram =  512;
	    break;
	case 0x10:
	    videoram = 1024;
	    break;
	case 0x18:
	    videoram = 2048;
	    break;
	}
	break;
	
    case PCI_CHIP_GD5434_4:
    case PCI_CHIP_GD5434_8:
    case PCI_CHIP_GD5436:
	switch (pCir->chip.alp->sr0f & 0x18) {
	case 0x10:
	    videoram = 1024;
	    break;
	case 0x18:
	    videoram = 2048;
	    if (pCir->chip.alp->sr0f & 0x80)
		videoram = 4096;
	    break;
	}
	
    case PCI_CHIP_GD5446:
	videoram = 1024;
	
	if (pCir->chip.alp->sr17 != (CARD32)-1) {
	    from = X_CONFIG;
	    hwp->writeSeq(hwp, 0x17, pCir->chip.alp->sr17);
	} else {
	    from = X_PROBED;
	    pCir->chip.alp->sr17 = hwp->readSeq(hwp, 0x17);
	}
	xf86DrvMsg(pScrn->scrnIndex, from, "Memory Config reg 2 is 0x%02X\n",
		   (unsigned int)pCir->chip.alp->sr17);
	
	if ((pCir->chip.alp->sr0f & 0x18) == 0x18) {
	    if (pCir->chip.alp->sr0f & 0x80) {
		if (pCir->chip.alp->sr17 & 0x80)
		    videoram = 2048;
		else if (pCir->chip.alp->sr17 & 0x02)
		    videoram = 3072;
		else
		    videoram = 4096;
	    } else {
		if ((pCir->chip.alp->sr17 & 80) == 0)
		    videoram = 2048;
	    }
	}
	break;
	
    case PCI_CHIP_GD5480:
	if (pCir->chip.alp->sr17 != (CARD32)-1) {
	    from = X_CONFIG;
	    hwp->writeSeq(hwp, 0x17, pCir->chip.alp->sr17);
	} else {
	    from = X_PROBED;
	    pCir->chip.alp->sr17 = hwp->readSeq(hwp, 0x17);
	}
	xf86DrvMsg(pScrn->scrnIndex, from, "Memory Config reg 2 is 0x%02X\n",
		   (unsigned int)pCir->chip.alp->sr17);
	videoram = 1024;
	if ((pCir->chip.alp->sr0f & 0x18) == 0x18) {	/* 2 or 4 MB */
	    videoram = 2048;
	    if (pCir->chip.alp->sr0f & 0x80)	/* Second bank enable */
		videoram = 4096;
	}
	if (pCir->chip.alp->sr17 & 0x80)
	    videoram <<= 1;
	break;
	
    case PCI_CHIP_GD7548:
	videoram = 1024;
	switch (pCir->chip.alp->sr0f & 0x90) {
		case 0x10:
			/* TODO: 2 256K X 16 DRAMs (1024) or 4 512K X 8 DRAMs (2048)? */
			break;
		case 0x90:
			videoram <<= 1;
			break;
	}
	break;

    case PCI_CHIP_GD7555:
    case PCI_CHIP_GD7556:
	videoram = 2048;   /*  for PC-9821 La13 etc.  */
	break;
    }

    /* UNMap the Alp memory and MMIO areas */
    if (!CirUnmapMem(pCir, pScrn->scrnIndex))
	return 0;
    vgaHWSetStdFuncs(hwp);    
    
    return videoram;
}


/*
 * GetAccelPitchValues -
 *
 * This function returns a list of display width (pitch) values that can
 * be used in accelerated mode.
 */
static int *
GetAccelPitchValues(ScrnInfoPtr pScrn)
{
	int *linePitches = NULL;
	int i, n = 0;
	int max_pitch;

	CirPtr pCir = CIRPTR(pScrn);

	/* XXX ajv - 512, 576, and 1536 may not be supported
	   line pitches. see sdk pp 4-59 for more
	   details. Why anyone would want less than 640 is
	   bizarre. (maybe lots of pixels tall?) */

	/* The only line pitches the accelerator supports */
#if 1
	const int accelWidths[] = { 640, 768, 800, 960, 1024, 1152, 1280,
							1600, 1920, 2048, 0 };
#else
	const int accelWidths[] = { 512, 576, 640, 768, 800, 960, 1024, 1152,
                                            1280, 1536, 1600, 1920, 2048, 0 };
#endif

	switch (pCir->Chipset) {
	case PCI_CHIP_GD5436:
	case PCI_CHIP_GD5446:
		max_pitch = 0x1ff << 3;
		break;

	default:
		/* FIXME max_pitch for other chipsets? */
		max_pitch = (pScrn->bitsPerPixel / 8) * 2048;
		break;
	}

	for (i = 0; accelWidths[i] != 0; i++) {
		if ((accelWidths[i] % pCir->Rounding == 0)
		 && ((accelWidths[i] * pScrn->bitsPerPixel / 8) <= max_pitch)) {
			n++;
			linePitches = XNFrealloc(linePitches, n * sizeof(int));
			linePitches[n - 1] = accelWidths[i];
		}
	}
	/* Mark the end of the list */
	if (n > 0) {
		linePitches = XNFrealloc(linePitches, (n + 1) * sizeof(int));
		linePitches[n] = 0;
	}
	return linePitches;
}


/* Mandatory */
Bool
AlpPreInit(ScrnInfoPtr pScrn, int flags)
{
	CirPtr pCir;
	vgaHWPtr hwp;
	MessageType from, from1;
	int i;
	int depth_flags;
	ClockRangePtr clockRanges;
	const char *s;
 	xf86Int10InfoPtr pInt = NULL;

	if (flags & PROBE_DETECT)  {
	  cirProbeDDC( pScrn, xf86GetEntityInfo(pScrn->entityList[0])->index );
	  return TRUE;
	}

#ifdef ALP_DEBUG
	ErrorF("AlpPreInit\n");
#endif

	/* Check the number of entities, and fail if it isn't one. */
	if (pScrn->numEntities != 1)
		return FALSE;

	if (!xf86LoadSubModule(pScrn, "vgahw"))
		return FALSE;

	/*
	 * Allocate a vgaHWRec
	 */
	if (!vgaHWGetHWRec(pScrn))
		return FALSE;
	hwp = VGAHWPTR(pScrn);
	vgaHWSetStdFuncs(hwp);
	vgaHWGetIOBase(hwp);

	/* Allocate the AlpRec driverPrivate */
	if (!AlpGetRec(pScrn))
		return FALSE;

	pCir = CIRPTR(pScrn);
	pCir->pScrn = pScrn;
	pCir->PIOReg = 0x3CE; /* was hwp->PIOOffset + 0x3CE */

	/* Get the entity, and make sure it is PCI. */
	pCir->pEnt = xf86GetEntityInfo(pScrn->entityList[0]);
	if (pCir->pEnt->location.type != BUS_PCI) {
		free(pCir->pEnt);
		return FALSE;
	}

	pCir->Chipset = pCir->pEnt->chipset;
	/* Find the PCI info for this screen */
	pCir->PciInfo = xf86GetPciInfoForEntity(pCir->pEnt->index);
#ifndef XSERVER_LIBPCIACCESS
	pCir->PciTag = pciTag(PCI_DEV_BUS(pCir->PciInfo),
			      PCI_DEV_DEV(pCir->PciInfo),
			      PCI_DEV_FUNC(pCir->PciInfo));
#endif

    if (xf86LoadSubModule(pScrn, "int10"))
    {
	xf86DrvMsg(pScrn->scrnIndex,X_INFO,"initializing int10\n");
	pInt = xf86InitInt10(pCir->pEnt->index);
	xf86FreeInt10(pInt);
	/*
	 * This is a hack: We restore the PCI base regs as some Colorgraphic
	 * BIOSes tend to mess them up
	 */

	PCI_WRITE_LONG(pCir->PciInfo, PCI_REGION_BASE(pCir->PciInfo, 0, REGION_MEM), 0x10);
	PCI_WRITE_LONG(pCir->PciInfo, PCI_REGION_BASE(pCir->PciInfo, 1, REGION_MEM), 0x14);
    }

    /* Set pScrn->monitor */
	pScrn->monitor = pScrn->confScreen->monitor;

	/* 32bpp only works on 5480 and 7548 */
	depth_flags = Support24bppFb;
	if (pCir->Chipset == PCI_CHIP_GD5480 || pCir->Chipset ==PCI_CHIP_GD7548)
	    depth_flags |= Support32bppFb |
			   SupportConvert32to24 |
			   PreferConvert32to24;

	/*
	 * The first thing we should figure out is the depth, bpp, etc.
	 * We support both 24bpp and 32bpp layouts, so indicate that.
	 */
	if (!xf86SetDepthBpp(pScrn, 0, 0, 16, depth_flags)) {
		return FALSE;
	} else {
		/* Check that the returned depth is one we support */
		switch (pScrn->depth) {
		case 1:
		case 4:
		case 8:
		case 15:
		case 16:
		case 24:
			/* OK */
			break;
		default:
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
				"Given depth (%d) is not supported by this driver\n",
				pScrn->depth);
			return FALSE;
		}
	}
	xf86PrintDepthBpp(pScrn);

	/* Get the depth24 pixmap format */
	if (pScrn->depth == 24 && pix24bpp == 0)
		pix24bpp = xf86GetBppFromDepth(pScrn, 24);

	/*
	 * This must happen after pScrn->display has been set because
	 * xf86SetWeight references it.
	 */
	if (pScrn->depth > 8) {
		/* The defaults are OK for us */
		rgb zeros = {0, 0, 0};

		if (!xf86SetWeight(pScrn, zeros, zeros)) {
			return FALSE;
		} else {
			/* XXX check that weight returned is supported */
			;
		}
	}

	if (!xf86SetDefaultVisual(pScrn, -1)) {
		return FALSE;
	} 
	/* Collect all of the relevant option flags (fill in pScrn->options) */
	xf86CollectOptions(pScrn, NULL);

	/* Process the options */
	if (!(pCir->Options = malloc(sizeof(CirOptions))))
		return FALSE;
	memcpy(pCir->Options, CirOptions, sizeof(CirOptions));
	xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, pCir->Options);

	if (!xf86IsPrimaryPci(pCir->PciInfo) 
	    && !(pInt || (xf86IsOptionSet(pCir->Options,OPTION_MEMCFG1)
			   && xf86IsOptionSet(pCir->Options,OPTION_MEMCFG2))))
	    return FALSE;
					   
	if (pScrn->depth == 8) 
	    pScrn->rgbBits = 6;

	from = X_DEFAULT;
	pCir->HWCursor = FALSE;

	switch (pCir->Chipset) {
	case PCI_CHIP_GD7555:
	case PCI_CHIP_GD7556:
	  pCir->HWCursor = TRUE;
	  break;
	default:
	  break;
	}

	if (xf86GetOptValBool(pCir->Options, OPTION_HW_CURSOR, &pCir->HWCursor))
		from = X_CONFIG;

	xf86DrvMsg(pScrn->scrnIndex, from, "Using %s cursor\n",
		pCir->HWCursor ? "HW" : "SW");
	if (xf86ReturnOptValBool(pCir->Options, OPTION_NOACCEL, FALSE)) {
		pCir->NoAccel = TRUE;
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Acceleration disabled\n");
	}
	if(pScrn->bitsPerPixel < 8) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			"Cannot use accelerations in less than 8 bpp\n");
		pCir->NoAccel = TRUE;
	}

	/*
	 * Set the ChipRev, allowing config file entries to
	 * override.
	 */
	if (pCir->pEnt->device->chipRev >= 0) {
		pCir->ChipRev = pCir->pEnt->device->chipRev;
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipRev override: %d\n",
			pCir->ChipRev);
	} else {
 	        pCir->ChipRev = PCI_DEV_REVISION(pCir->PciInfo);
	}

	/* Find the frame buffer base address */
	if (pCir->pEnt->device->MemBase != 0) {
	    if (!xf86CheckPciMemBase(pCir->PciInfo, pCir->pEnt->device->MemBase)) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "MemBase 0x%08lX doesn't match any PCI base register.\n",
			   pCir->pEnt->device->MemBase);
		return FALSE;
		}
		pCir->FbAddress = pCir->pEnt->device->MemBase;
		from = X_CONFIG;
	} else {
		if (PCI_REGION_BASE(pCir->PciInfo, 0, REGION_MEM) != 0) {
			/* 5446B and 5480 use mask of 0xfe000000.
			   5446A uses 0xff000000. */
			pCir->FbAddress = PCI_REGION_BASE(pCir->PciInfo, 0, REGION_MEM) & 0xff000000;
			from = X_PROBED;
		} else {
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
				"No valid FB address in PCI config space\n");
			AlpFreeRec(pScrn);
			return FALSE;
		}
	}
	xf86DrvMsg(pScrn->scrnIndex, from, "Linear framebuffer at 0x%lX\n",
		(unsigned long)pCir->FbAddress);

	if (pCir->pEnt->device->IOBase != 0) {
	    /* Require that the config file value matches one of the PCI values. */
	    if (!xf86CheckPciMemBase(pCir->PciInfo, pCir->pEnt->device->IOBase)) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "IOBase 0x%08lX doesn't match any PCI base register.\n",
			   pCir->pEnt->device->IOBase);
		return FALSE;
	    }
	    pCir->IOAddress = pCir->pEnt->device->IOBase;
		from = X_CONFIG;
	} else {
		if (PCI_REGION_BASE(pCir->PciInfo, 1, REGION_MEM) != 0) {
			pCir->IOAddress = PCI_REGION_BASE(pCir->PciInfo, 1, REGION_MEM) & 0xfffff000;
			pCir->IoMapSize = PCI_REGION_SIZE(pCir->PciInfo, 1);
			from = X_PROBED;
		} else {
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
				"No valid MMIO address in PCI config space\n");
			/* 5446 rev A do not use a separate MMIO segment */
			/* We do not really need that YET. */
		}
	}

	/* User options can override the MMIO default */
#if 0
	/* Will we ever support MMIO on 5446A or older? */
	if (xf86ReturnOptValBool(pCir->Options, OPTION_MMIO, FALSE)) {
		pCir->UseMMIO = TRUE;
		from = X_CONFIG;
	}
#endif
	if (!xf86ReturnOptValBool(pCir->Options, OPTION_MMIO, TRUE)) {
		pCir->UseMMIO = FALSE;
		from1 = X_CONFIG;
 	} else if (pCir->IOAddress) {
 	  /* Default to MMIO if we have a separate IOAddress and
 	       not in monochrome mode (IO 0x3Bx is not relocated!) */
 	    if (pScrn->bitsPerPixel != 1) {
 	        pCir->UseMMIO = TRUE;
 		from1 = X_PROBED;
 	    } else {
 	        pCir->UseMMIO = FALSE;
 	        from1 = X_PROBED;
 	    }	      
 	} else {	        
 	    pCir->UseMMIO = FALSE;
 	    from1 = X_PROBED;
 	}	      
 
 	if (pCir->UseMMIO) {
 		xf86DrvMsg(pScrn->scrnIndex, from1, "Using MMIO\n");
 		xf86DrvMsg(pScrn->scrnIndex, from, "MMIO registers at 0x%lX\n",
 			(unsigned long)pCir->IOAddress);
 	} else 
 	    xf86DrvMsg(pScrn->scrnIndex, from1, "Not Using MMIO\n");

#ifndef XSERVER_LIBPCIACCESS
     /*
      * XXX Check if this is correct
      */
     if (!pCir->UseMMIO) {
         pScrn->racIoFlags = RAC_COLORMAP | RAC_CURSOR | RAC_VIEWPORT | RAC_FB;
         xf86SetOperatingState(resVgaMem, pCir->pEnt->index, ResUnusedOpr);
     } else {
         xf86SetOperatingState(resVga, pCir->pEnt->index, ResUnusedOpr);
     }

     /* Register the PCI-assigned resources. */
     if (xf86RegisterResources(pCir->pEnt->index, NULL, ResExclusive)) {
	 xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "xf86RegisterResources() found resource conflicts\n");
	 return FALSE;
     }
#endif

     if (!xf86LoadSubModule(pScrn, "i2c")) {
	 AlpFreeRec(pScrn);
 	return FALSE;
     }
 
     if (!xf86LoadSubModule(pScrn, "ddc")) {
 	AlpFreeRec(pScrn);
 	return FALSE;
     }
 
     if(!AlpI2CInit(pScrn)) {
         xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
             "I2C initialization failed\n");
     }
     else
 	xf86SetDDCproperties(pScrn,xf86PrintEDID(
		 xf86DoEDID_DDC2(pScrn,pCir->I2CPtr1)));

#ifdef XSERVER_LIBPCIACCESS
     #ifndef PCI_CHIP_QEMU
     #define PCI_CHIP_QEMU 0x1af4
     #endif
     if (!pScrn->monitor->DDC &&
	((pCir->PciInfo->subvendor_id & 0xffff) == PCI_CHIP_QEMU)) {
	pCir->NoAccel = TRUE;
     }
#endif
 
     /* Probe the possible LCD display */
     AlpProbeLCD(pScrn);

#ifdef CIRPROBEI2C
     CirProbeI2C(pScrn->scrnIndex);
#endif
 
     /* The gamma fields must be initialised when using the new cmap code */
     if (pScrn->depth > 1) {
 	Gamma zeros = {0.0, 0.0, 0.0};
 
 	if (!xf86SetGamma(pScrn, zeros))
 	    return FALSE;
     }
 
	/* XXX If UseMMIO == TRUE and for any reason we cannot do MMIO,
	   abort here */

	if (xf86GetOptValBool(pCir->Options,
			      OPTION_SHADOW_FB,&pCir->shadowFB))
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ShadowFB %s.\n",
		       pCir->shadowFB ? "enabled" : "disabled");
	    
	if ((s = xf86GetOptValString(pCir->Options, OPTION_ROTATE))) {
	    if(!xf86NameCmp(s, "CW")) {
		/* accel is disabled below for shadowFB */
		pCir->shadowFB = TRUE;
		pCir->rotate = 1;
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
			   "Rotating screen clockwise - acceleration disabled\n");
	    } else if(!xf86NameCmp(s, "CCW")) {
		pCir->shadowFB = TRUE;
		pCir->rotate = -1;
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,  "Rotating screen"
			   "counter clockwise - acceleration disabled\n");
	    } else {
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "\"%s\" is not a valid"
			   "value for Option \"Rotate\"\n", s);
		xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
			   "Valid options are \"CW\" or \"CCW\"\n");
	    }
	}
	if (pCir->shadowFB && (pScrn->depth < 8)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		       "shadowFB not supported at this depth.\n");
	    pCir->shadowFB = FALSE;
	    pCir->rotate = 0;
	}

	if (pCir->shadowFB && !pCir->NoAccel) {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		       "HW acceleration not supported with \"shadowFB\".\n");
	    pCir->NoAccel = TRUE;
	}

	if (pCir->rotate && pCir->HWCursor) {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		       "HW cursor not supported with \"rotate\".\n");
	    pCir->HWCursor = FALSE;
	}
	
	/* XXX We do not know yet how to configure memory on this card.
	   Use options MemCFG1 and MemCFG2 to set registers SR0F and
	   SR17 before trying to count ram size. */

	pCir->chip.alp->sr0f = (CARD32)-1;
	pCir->chip.alp->sr17 = (CARD32)-1;

	(void) xf86GetOptValULong(pCir->Options, OPTION_MEMCFG1, (unsigned long *)&pCir->chip.alp->sr0f);
	(void) xf86GetOptValULong(pCir->Options, OPTION_MEMCFG2, (unsigned long *)&pCir->chip.alp->sr17);
	/*
	 * If the user has specified the amount of memory in the XF86Config
	 * file, we respect that setting.
	 */
	if (pCir->pEnt->device->videoRam != 0) {
		pScrn->videoRam = pCir->pEnt->device->videoRam;
		pCir->IoMapSize = 0x4000;	/* 16K for moment */
		from = X_CONFIG;
	} else {
		pScrn->videoRam = AlpCountRam(pScrn);
		from = X_PROBED;
	}
	xf86DrvMsg(pScrn->scrnIndex, from, "VideoRAM: %d kByte\n", pScrn->videoRam);

	pCir->FbMapSize = pScrn->videoRam * 1024;

	/* properties */
	pCir->properties = 0;

	if ((pCir->chip.alp->sr0f & 0x18) > 0x8)
	  pCir->properties |= HWCUR64;

	switch (pCir->Chipset) {
        case PCI_CHIP_GD7548:
	  pCir->properties |= HWCUR64;
	  pCir->properties |= ACCEL_AUTOSTART;
	  break;
	case PCI_CHIP_GD5436:
	case PCI_CHIP_GD5480:
	  pCir->properties |= ACCEL_AUTOSTART;
	  break;
	default:
	  break;
	}

     /* We use a programmable clock */
     pScrn->progClock = TRUE;
 
	/* XXX Set HW cursor use */

	/* Set the min pixel clock */
	pCir->MinClock = 12000;	/* XXX Guess, need to check this */
	xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT, "Min pixel clock is %d MHz\n",
		pCir->MinClock / 1000);
	/*
	 * If the user has specified ramdac speed in the XF86Config
	 * file, we respect that setting.
	 */
	if (pCir->pEnt->device->dacSpeeds[0]) {
		ErrorF("Do not specily a Clocks line for Cirrus chips\n");
		return FALSE;
	} else {
		int speed;
		int *p = NULL;
		switch (pCir->Chipset) {
		case PCI_CHIP_GD5430:
		case PCI_CHIP_GD5434_4:
		case PCI_CHIP_GD5434_8:
	/*	case PCI_CHIP_GD5440: */
			p = gd5430_MaxClocks;
			break;
		case PCI_CHIP_GD5436:
		case PCI_CHIP_GD5446:
			p = gd5446_MaxClocks;
			break;
		case PCI_CHIP_GD5480:
			p = gd5480_MaxClocks;
			break;
		case PCI_CHIP_GD7548:
		        p = gd7548_MaxClocks;
                        break;
		case PCI_CHIP_GD7555:
		        p = gd7555_MaxClocks;
                        break;
		case PCI_CHIP_GD7556:
		        p = gd7556_MaxClocks;
                        break;
		}
		if (!p)
			return FALSE;
		switch(pScrn->bitsPerPixel) {
		case 1:
		case 4:
			speed = p[0];
			break;
		case 8:
			speed = p[1];
			break;
		case 15:
		case 16:
			speed = p[2];
			break;
		case 24:
			speed = p[3];
			break;
		case 32:
			speed = p[4];
			break;
		default:
			/* Should not get here */
			speed = 0;
			break;
		}
		pCir->MaxClock = speed;
		from = X_PROBED;
	}
	xf86DrvMsg(pScrn->scrnIndex, from, "Max pixel clock is %d MHz\n",
	pCir->MaxClock / 1000);

	/*
	 * Setup the ClockRanges, which describe what clock ranges are available,
	 * and what sort of modes they can be used for.
	 */
	clockRanges = XNFcallocarray(sizeof(ClockRange), 1);
	clockRanges->next = NULL;
	clockRanges->minClock = pCir->MinClock;
	clockRanges->maxClock = pCir->MaxClock;
	clockRanges->clockIndex = -1;		/* programmable */
	clockRanges->interlaceAllowed = FALSE;	/* XXX check this */
	clockRanges->doubleScanAllowed = FALSE;	/* XXX check this */
	clockRanges->doubleScanAllowed = FALSE;	/* XXX check this */
	clockRanges->doubleScanAllowed = FALSE;	/* XXX check this */
	clockRanges->ClockMulFactor = 1;
	clockRanges->ClockDivFactor = 1;
	clockRanges->PrivFlags = 0;

	switch (pCir->Chipset)
	{
	case PCI_CHIP_GD7548:
	  pCir->Rounding = 1;
	  break;

	default:
	  pCir->Rounding = 128 >> pCir->BppShift;
        }

#if 0
	if (pCir->Chipset != PCI_CHIP_GD5446 &&
		pCir->Chipset != PCI_CHIP_GD5480) {
		/* XXX Kludge */
		pCir->NoAccel = TRUE;
	}
#endif

	/*
	 * xf86ValidateModes will check that the mode HTotal and VTotal values
	 * don't exceed the chipset's limit if pScrn->maxHValue and
	 * pScrn->maxVValue are set.  Since our AlpValidMode() already takes
	 * care of this, we don't worry about setting them here.
	 */

	/* Select valid modes from those available */
	if (pCir->NoAccel) {
		/*
		 * XXX Assuming min pitch 256, max 2048
		 * XXX Assuming min height 128, max 2048
		 */
		i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
						pScrn->display->modes, clockRanges,
						NULL, 256, 2048,
						pCir->Rounding * pScrn->bitsPerPixel, 128, 2048,
						pScrn->display->virtualX,
						pScrn->display->virtualY,
						pCir->FbMapSize,
						LOOKUP_BEST_REFRESH);
	} else {
		/*
		 * XXX Assuming min height 128, max 2048
		 */
		i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
						pScrn->display->modes, clockRanges,
						GetAccelPitchValues(pScrn), 0, 0,
						pCir->Rounding * pScrn->bitsPerPixel, 128, 2048,
						pScrn->display->virtualX,
						pScrn->display->virtualY,
						pCir->FbMapSize,
						LOOKUP_BEST_REFRESH);
	}
	if (i == -1) {
		AlpFreeRec(pScrn);
		return FALSE;
	}

	/* Prune the modes marked as invalid */
	xf86PruneDriverModes(pScrn);

	if (i == 0 || pScrn->modes == NULL) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
		AlpFreeRec(pScrn);
		return FALSE;
	}

	/*
	 * Set the CRTC parameters for all of the modes based on the type
	 * of mode, and the chipset's interlace requirements.
	 *
	 * Calling this is required if the mode->Crtc* values are used by the
	 * driver and if the driver doesn't provide code to set them.  They
	 * are not pre-initialised at all.
	 */
	xf86SetCrtcForModes(pScrn, INTERLACE_HALVE_V);

	/* Set the current mode to the first in the list */
	pScrn->currentMode = pScrn->modes;

	/* Print the list of modes being used */
	xf86PrintModes(pScrn);

	/* Set display resolution */
	xf86SetDpi(pScrn, 0, 0);

	/* Load fb module */
	if (xf86LoadSubModule(pScrn, "fb") == NULL) {
	    AlpFreeRec(pScrn);
	    return FALSE;
	}

	/* Use shadowfb for acceleration */
	if (!pCir->NoAccel) {
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		       "Falling back to shadowfb\n");
	    pCir->NoAccel = TRUE;
	    pCir->shadowFB = TRUE;
	}

	/* Load ramdac if needed */
	if (pCir->HWCursor) {
		if (!xf86LoadSubModule(pScrn, "ramdac")) {
			AlpFreeRec(pScrn);
			return FALSE;
		}
	}

	if (pCir->shadowFB) {
	    if (!xf86LoadSubModule(pScrn, "shadowfb")) {
		AlpFreeRec(pScrn);
		return FALSE;
	    }
	}

	return TRUE;
}

/*
 * This function saves the video state.
 */
static void
AlpSave(ScrnInfoPtr pScrn)
{
	CirPtr pCir = CIRPTR(pScrn); 
	vgaHWPtr hwp = VGAHWPTR(pScrn);

#ifdef ALP_DEBUG
	ErrorF("AlpSave\n");
#endif
	vgaHWSave(pScrn, &VGAHWPTR(pScrn)->SavedReg, VGA_SR_ALL);

	pCir->chip.alp->ModeReg.ExtVga[CR1A] = pCir->chip.alp->SavedReg.ExtVga[CR1A] = hwp->readCrtc(hwp, 0x1A);
	pCir->chip.alp->ModeReg.ExtVga[CR1B] = pCir->chip.alp->SavedReg.ExtVga[CR1B] = hwp->readCrtc(hwp, 0x1B);
	pCir->chip.alp->ModeReg.ExtVga[CR1D] = pCir->chip.alp->SavedReg.ExtVga[CR1D] = hwp->readCrtc(hwp, 0x1D);
	pCir->chip.alp->ModeReg.ExtVga[SR07] = pCir->chip.alp->SavedReg.ExtVga[SR07] = hwp->readSeq(hwp, 0x07);
	pCir->chip.alp->ModeReg.ExtVga[SR0E] = pCir->chip.alp->SavedReg.ExtVga[SR0E] = hwp->readSeq(hwp, 0x0E);
	pCir->chip.alp->ModeReg.ExtVga[SR12] = pCir->chip.alp->SavedReg.ExtVga[SR12] = hwp->readSeq(hwp, 0x12);
	pCir->chip.alp->ModeReg.ExtVga[SR13] = pCir->chip.alp->SavedReg.ExtVga[SR13] = hwp->readSeq(hwp, 0x13);
	pCir->chip.alp->ModeReg.ExtVga[SR17] = pCir->chip.alp->SavedReg.ExtVga[SR17] = hwp->readSeq(hwp, 0x17);
	pCir->chip.alp->ModeReg.ExtVga[SR1E] = pCir->chip.alp->SavedReg.ExtVga[SR1E] = hwp->readSeq(hwp, 0x1E);
	pCir->chip.alp->ModeReg.ExtVga[SR21] = pCir->chip.alp->SavedReg.ExtVga[SR21] = hwp->readSeq(hwp, 0x21);
	pCir->chip.alp->ModeReg.ExtVga[SR2D] = pCir->chip.alp->SavedReg.ExtVga[SR2D] = hwp->readSeq(hwp, 0x2D);
	pCir->chip.alp->ModeReg.ExtVga[GR17] = pCir->chip.alp->SavedReg.ExtVga[GR17] = hwp->readGr(hwp, 0x17);
	pCir->chip.alp->ModeReg.ExtVga[GR18] = pCir->chip.alp->SavedReg.ExtVga[GR18] = hwp->readGr(hwp, 0x18);
	/* The first 4 reads are for the pixel mask register. After 4 times that
	   this register is accessed in succession reading/writing this address
	   accesses the HDR. */
	hwp->readDacMask(hwp);
	hwp->readDacMask(hwp);
	hwp->readDacMask(hwp);
	hwp->readDacMask(hwp);
	pCir->chip.alp->ModeReg.ExtVga[HDR] = pCir->chip.alp->SavedReg.ExtVga[HDR] = hwp->readDacMask(hwp);
}

/* XXX */
static void
AlpFix1bppColorMap(ScrnInfoPtr pScrn)
{
	vgaHWPtr hwp = VGAHWPTR(pScrn);
/* In 1 bpp we have color 0 at LUT 0 and color 1 at LUT 0x3f.
   This makes white and black look right (otherwise they were both
   black. I'm sure there's a better way to do that, just lazy to
   search the docs.  */

	hwp->writeDacWriteAddr(hwp, 0x00);
	hwp->writeDacData(hwp, 0x00); hwp->writeDacData(hwp, 0x00); hwp->writeDacData(hwp, 0x00);
	hwp->writeDacWriteAddr(hwp, 0x3F);
	hwp->writeDacData(hwp, 0x3F); hwp->writeDacData(hwp, 0x3F); hwp->writeDacData(hwp, 0x3F);
}

static void
alpRestore(vgaHWPtr hwp, AlpRegPtr cirReg)
{
    hwp->writeCrtc(hwp, 0x1A, cirReg->ExtVga[CR1A]);
    hwp->writeCrtc(hwp, 0x1B, cirReg->ExtVga[CR1B]);
    hwp->writeCrtc(hwp, 0x1D, cirReg->ExtVga[CR1D]);
    hwp->writeSeq(hwp, 0x07, cirReg->ExtVga[SR07]);
    hwp->writeSeq(hwp, 0x0E, cirReg->ExtVga[SR0E]);
    hwp->writeSeq(hwp, 0x12, cirReg->ExtVga[SR12]);
    hwp->writeSeq(hwp, 0x13, cirReg->ExtVga[SR13]);
    hwp->writeSeq(hwp, 0x17, cirReg->ExtVga[SR17]);
    hwp->writeSeq(hwp, 0x1E, cirReg->ExtVga[SR1E]);
    hwp->writeSeq(hwp, 0x21, cirReg->ExtVga[SR21]);
    hwp->writeSeq(hwp, 0x2D, cirReg->ExtVga[SR2D]);
    hwp->writeGr(hwp, 0x17, cirReg->ExtVga[GR17]);
    hwp->writeGr(hwp, 0x18, cirReg->ExtVga[GR18]);
    /* The first 4 reads are for the pixel mask register. After 4 times that
       this register is accessed in succession reading/writing this address
       accesses the HDR. */
    hwp->readDacMask(hwp); 
    hwp->readDacMask(hwp); 
    hwp->readDacMask(hwp); 
    hwp->readDacMask(hwp);
    hwp->writeDacMask(hwp, cirReg->ExtVga[HDR ]);
}


/*
 * Initialise a new mode.  This is currently still using the old
 * "initialise struct, restore/write struct to HW" model.  That could
 * be changed.
 * Why?? (EE)
 */

static Bool
AlpModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
	vgaHWPtr hwp;
	CirPtr pCir;
	int depthcode;
	int width;
	Bool HDiv2 = FALSE, VDiv2 = FALSE;

#ifdef ALP_DEBUG
	ErrorF("AlpModeInit %d bpp,   %d   %d %d %d %d   %d %d %d %d\n",
		pScrn->bitsPerPixel,
		mode->Clock,
		mode->HDisplay,
		mode->HSyncStart,
		mode->HSyncEnd,
		mode->HTotal,
		mode->VDisplay,
		mode->VSyncStart,
		mode->VSyncEnd,
		mode->VTotal);

	ErrorF("AlpModeInit: depth %d bits\n", pScrn->depth);
#endif

	pCir = CIRPTR(pScrn);
	hwp = VGAHWPTR(pScrn);
	vgaHWUnlock(hwp);

	pCir->pitch = pScrn->displayWidth * pScrn->bitsPerPixel >> 3;

	depthcode = pScrn->depth;
	if (pScrn->bitsPerPixel == 32)
		depthcode = 32;

	if ((pCir->Chipset == PCI_CHIP_GD5480 && mode->Clock > 135100) ||
		(pCir->Chipset == PCI_CHIP_GD5446 && mode->Clock >  85500)) {
		/* The actual DAC register value is set later. */
		/* The CRTC is clocked at VCLK / 2, so we must half the */
		/* horizontal timings. */
		if (!mode->CrtcHAdjusted) {
			mode->CrtcHDisplay >>= 1;
			mode->CrtcHSyncStart >>= 1;
			mode->CrtcHTotal >>= 1;
			mode->CrtcHSyncEnd >>= 1;
			mode->SynthClock >>= 1;
			mode->CrtcHAdjusted = TRUE;
		}
		depthcode += 64;
		HDiv2 = TRUE;
	}

	if (mode->VTotal >= 1024 && !(mode->Flags & V_INTERLACE)) {
		/* For non-interlaced vertical timing >= 1024, the vertical timings */
		/* are divided by 2 and VGA CRTC 0x17 bit 2 is set. */
		if (!mode->CrtcVAdjusted) {
			mode->CrtcVDisplay >>= 1;
			mode->CrtcVSyncStart >>= 1;
			mode->CrtcVSyncEnd >>= 1;
			mode->CrtcVTotal >>= 1;
			mode->CrtcVAdjusted = TRUE;
		}
		VDiv2 = TRUE;
	}

	/* Initialise the ModeReg values */
	if (!vgaHWInit(pScrn, mode))
		return FALSE;
	pScrn->vtSema = TRUE;

	/* Turn off HW cursor, gamma correction, overscan color protect.  */
	pCir->chip.alp->ModeReg.ExtVga[SR12] = 0;
	if ((pCir->properties & HWCUR64) == HWCUR64)
	{
            pCir->chip.alp->ModeReg.ExtVga[SR12] = 0x4;
            switch (pCir->Chipset)
	    {
            case PCI_CHIP_GD7548:
	      pCir->chip.alp->ModeReg.ExtVga[SR21] |= 0x10;
	      break;
	    }

	}
	else
	    pCir->chip.alp->ModeReg.ExtVga[SR12] = 0;


	if(VDiv2)
		hwp->ModeReg.CRTC[0x17] |= 0x04;

#ifdef ALP_DEBUG
	ErrorF("SynthClock = %d\n", mode->SynthClock);
#endif

	/* Disable DCLK pin driver, interrupts. */
	pCir->chip.alp->ModeReg.ExtVga[GR17] |= 0x08;
	pCir->chip.alp->ModeReg.ExtVga[GR17] &= ~0x04;

	pCir->chip.alp->ModeReg.ExtVga[HDR] = 0;
	/* Enable linear mode and high-res packed pixel mode */
	pCir->chip.alp->ModeReg.ExtVga[SR07] &= 0xe0;
#ifdef ALP_DEBUG
	ErrorF("depthcode = %d\n", depthcode);
#endif

	if (pScrn->bitsPerPixel == 1) {
		hwp->IOBase = 0x3B0;
		hwp->ModeReg.MiscOutReg &= ~0x01;
	} else {
		hwp->IOBase = 0x3D0;
		hwp->ModeReg.MiscOutReg |= 0x01;
	}

	switch (depthcode) {
	case 1:
	case 4:
		pCir->chip.alp->ModeReg.ExtVga[SR07] |= 0x10;
		break;
	case 8:
		pCir->chip.alp->ModeReg.ExtVga[SR07] |= 0x11;
		break;
	case 64+8:
		pCir->chip.alp->ModeReg.ExtVga[SR07] |= 0x17;
		break;
	case 15:
		pCir->chip.alp->ModeReg.ExtVga[SR07] |= 0x17;
		pCir->chip.alp->ModeReg.ExtVga[HDR ]  = 0xC0;
		break;
	case 64+15:
		pCir->chip.alp->ModeReg.ExtVga[SR07] |= 0x19;
		pCir->chip.alp->ModeReg.ExtVga[HDR ]  = 0xC0;
		break;
	case 16:
		pCir->chip.alp->ModeReg.ExtVga[SR07] |= 0x17;
		pCir->chip.alp->ModeReg.ExtVga[HDR ]  = 0xC1;
		break;
	case 64+16:
		pCir->chip.alp->ModeReg.ExtVga[SR07] |= 0x19;
		pCir->chip.alp->ModeReg.ExtVga[HDR ]  = 0xC1;
		break;
	case 24:
		pCir->chip.alp->ModeReg.ExtVga[SR07] |= 0x15;
		pCir->chip.alp->ModeReg.ExtVga[HDR ]  = 0xC5;
		break;
	case 32:
		pCir->chip.alp->ModeReg.ExtVga[SR07] |= 0x19;
		pCir->chip.alp->ModeReg.ExtVga[HDR ]  = 0xC5;
		break;
	default:
		ErrorF("X11: Internal error: AlpModeInit: Cannot Initialize display to requested mode\n");
#ifdef ALP_DEBUG
		ErrorF("AlpModeInit returning FALSE on depthcode %d\n", depthcode);
#endif
		return FALSE;
	}
	if (HDiv2)
		pCir->chip.alp->ModeReg.ExtVga[GR18] |= 0x20;
	else
		pCir->chip.alp->ModeReg.ExtVga[GR18] &= ~0x20;


	/* Some extra init stuff */
	switch (pCir->Chipset)
        {
          case PCI_CHIP_GD7548:
	    /* Do we use MMIO ?
	       If we do and we are on a 7548, we need to tell the board
	       that we want MMIO. */
	    if (pCir->UseMMIO)
	    {
	      pCir->chip.alp->ModeReg.ExtVga[SR17] =
		(pCir->chip.alp->ModeReg.ExtVga[SR17] & ~0x40) | 4;
	      ErrorF("UseMMIO: SR17=%2X\n", (int) (pCir->chip.alp->ModeReg.ExtVga[SR17]));
	    }
#ifdef ALP_SETUP
	    ErrorF("SR2D=%2X\n", (int) (pCir->chip.alp->ModeReg.ExtVga[SR17]));
#endif
	    pCir->chip.alp->ModeReg.ExtVga[SR2D] |= 0xC0;
	    break;
	}

	/* No support for interlace (yet) */
	pCir->chip.alp->ModeReg.ExtVga[CR1A] = 0x00;

	width = pScrn->displayWidth * pScrn->bitsPerPixel / 8;
	if (pScrn->bitsPerPixel == 1)
		width <<= 2;
	hwp->ModeReg.CRTC[0x13] = width >> 3;
	/* Offset extension (see CR13) */
	pCir->chip.alp->ModeReg.ExtVga[CR1B] &= 0xAF;
	pCir->chip.alp->ModeReg.ExtVga[CR1B] |= (width >> (3+4)) & 0x10;
	pCir->chip.alp->ModeReg.ExtVga[CR1B] |= (width >> (3+3)) & 0x40;
	pCir->chip.alp->ModeReg.ExtVga[CR1B] |= 0x22;

	/* Programme the registers */
	vgaHWProtect(pScrn, TRUE);
	hwp->writeMiscOut(hwp, hwp->ModeReg.MiscOutReg); 
	alpRestore(hwp,&pCir->chip.alp->ModeReg);
	AlpSetClock(pCir, hwp, mode->SynthClock);

	vgaHWRestore(pScrn, &hwp->ModeReg, VGA_SR_MODE | VGA_SR_CMAP);

	/* XXX */
	if (pScrn->bitsPerPixel == 1)
		AlpFix1bppColorMap(pScrn);

	vgaHWProtect(pScrn, FALSE);

	return TRUE;
}

/*
 * Restore the initial (text) mode.
 */
static void
AlpRestore(ScrnInfoPtr pScrn)
{
	vgaHWPtr hwp;
	vgaRegPtr vgaReg;
	CirPtr pCir;
	AlpRegPtr alpReg;

#ifdef ALP_DEBUG
	ErrorF("AlpRestore\n");
#endif

	hwp = VGAHWPTR(pScrn);
	pCir = CIRPTR(pScrn);
	vgaReg = &hwp->SavedReg;
	alpReg = &pCir->chip.alp->SavedReg;

	vgaHWProtect(pScrn, TRUE);

	alpRestore(hwp,alpReg);
	
	vgaHWRestore(pScrn, vgaReg, VGA_SR_ALL);
	vgaHWProtect(pScrn, FALSE);
}

/* Mandatory */

/* This gets called at the start of each server generation */

Bool
AlpScreenInit(ScreenPtr pScreen, int argc, char **argv)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	vgaHWPtr hwp;
	CirPtr pCir;
	int i, ret;
	int init_picture = 0;
	VisualPtr visual;
	int displayWidth,width,height;
	unsigned char * FbBase = NULL;
	int cursor_size = 0;
	
#ifdef ALP_DEBUG
	ErrorF("AlpScreenInit\n");
#endif

	hwp = VGAHWPTR(pScrn);
	pCir = CIRPTR(pScrn);
	
	/* Map the VGA memory when the primary video */
	if (!vgaHWMapMem(pScrn))
	    return FALSE;

	/* Map the Alp memory and MMIO areas */
	if (!CirMapMem(pCir, pScrn->scrnIndex))
		return FALSE;

	/* The 754x supports MMIO for the BitBlt engine but
	   not for the VGA registers */
	switch (pCir->Chipset)
	{
	case PCI_CHIP_GD7548:
	  break;
	default:
	  if(pCir->UseMMIO)
		vgaHWSetMmioFuncs(hwp, pCir->IOBase, -0x3C0);
        }

	vgaHWGetIOBase(hwp);

	/* Save the current state */
	AlpSave(pScrn);

	/* Initialise the first mode */
	if (!AlpModeInit(pScrn, pScrn->currentMode))
		return FALSE;

	/* Make things beautiful */
	AlpSaveScreen(pScreen, SCREEN_SAVER_ON);

	/* Set the viewport */
	AlpAdjustFrame(pScrn, pScrn->frameX0, pScrn->frameY0);

	/*
	 * The next step is to setup the screen's visuals, and initialise the
	 * framebuffer code.  In cases where the framebuffer's default
	 * choices for things like visual layouts and bits per RGB are OK,
	 * this may be as simple as calling the framebuffer's ScreenInit()
	 * function.  If not, the visuals will need to be setup before calling
	 * a fb ScreenInit() function and fixed up after.
	 *
	 */

	/*
	 * Reset the visual list.
	 */
	miClearVisualTypes();

	/* Setup the visuals we support. */

	if (!miSetVisualTypes(pScrn->depth,
			      miGetDefaultVisualMask(pScrn->depth),
			      pScrn->rgbBits, pScrn->defaultVisual))
			return FALSE;

	miSetPixmapDepths ();

	displayWidth = pScrn->displayWidth;
	if (pCir->rotate) {
	    height = pScrn->virtualX;
	    width = pScrn->virtualY;
	} else {
	    width = pScrn->virtualX;
	    height = pScrn->virtualY;
	}
	
	if(pCir->shadowFB) {
	    pCir->ShadowPitch = BitmapBytePad(pScrn->bitsPerPixel * width);
	    pCir->ShadowPtr = malloc(pCir->ShadowPitch * height);
	    displayWidth = pCir->ShadowPitch / (pScrn->bitsPerPixel >> 3);
	    FbBase = pCir->ShadowPtr;
	} else {
	    pCir->ShadowPtr = NULL;
	    FbBase = pCir->FbBase;
	}
	
	/*
	 * Call the framebuffer layer's ScreenInit function, and fill in other
	 * pScreen fields.
	 */

	switch (pScrn->bitsPerPixel) {
	case 1:
	case 4:
	case 8:
	case 16:
	case 24:
	case 32:
	    ret = fbScreenInit(pScreen, FbBase,
				width,height,
				pScrn->xDpi, pScrn->yDpi,
				displayWidth,pScrn->bitsPerPixel);
	    init_picture = 1;
	    break;
	default:
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "X11: Internal error: invalid bpp (%d) in AlpScreenInit\n",
		       pScrn->bitsPerPixel);
	    ret = FALSE;
	    break;
	}
	if (!ret)
		return FALSE;

#ifdef ALP_DEBUG
	ErrorF("AlpScreenInit after depth dependent init\n");
#endif

	/* Override the default mask/offset settings */
	if (pScrn->bitsPerPixel > 8) {
		for (i = 0; i < pScreen->numVisuals; i++) {
			visual = &pScreen->visuals[i];
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

	/* must be after RGB ordering fixed */
	if (init_picture)
		fbPictureInit (pScreen, 0, 0);
    
	/*
	 * Set initial black & white colourmap indices.
	 */
	xf86SetBlackWhitePixels(pScreen);

	/* 
	   Allocation of off-screen memory to various stuff
	   (hardware cursor, 8x8 mono pattern...)
	   Allocation goes top-down in memory, since the cursor
	   *must* be in the last videoram locations 
	*/
	pCir->offscreen_offset = pScrn->videoRam*1024;
	pCir->offscreen_size = pScrn->videoRam * 1024 - pScrn->virtualY *
	    (BitmapBytePad(pScrn->displayWidth * pScrn->bitsPerPixel));

#ifdef ALP_DEBUG
	ErrorF("offscreen_offset=%d, offscreen_size=%d\n",
	       pCir->offscreen_offset, pCir->offscreen_size);
#endif
	    
	/* Initialise cursor functions */
	if (pCir->HWCursor) { /* Initialize HW cursor layer */
	
	    if ((pCir->properties & HWCUR64) 
		&& (pCir->offscreen_size >= 64*8*2)) {
	        cursor_size = 64;
	        pCir->offscreen_size -= 64*8*2;
	        pCir->offscreen_offset -= 64*8*2;
	    } else if (pCir->offscreen_size >= 32*4*2) {
	        cursor_size = 32;
		pCir->offscreen_size -= 32*8*2;
		pCir->offscreen_offset -= 32*8*2;
	    }
	}

	if (!pCir->NoAccel) { /* Initialize acceleration functions */
	    AlpOffscreenAccelInit(pScrn);
	}

#if 1
	pCir->DGAModeInit = AlpModeInit;
	if (!CirDGAInit(pScreen))
	  xf86DrvMsg(pScrn->scrnIndex, X_ERROR, 
		     "DGA initialization failed\n");
#endif
        xf86SetSilkenMouse(pScreen);

	/* Initialise cursor functions */
	miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

	if (pCir->HWCursor) {
	    if (!AlpHWCursorInit(pScreen, cursor_size))
	        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "Hardware cursor initialization failed\n"); 
#ifdef ALP_DEBUG
	    ErrorF("AlpHWCursorInit() complete\n");
#endif
	}

	if (pCir->shadowFB) {
	    RefreshAreaFuncPtr refreshArea = cirRefreshArea;
	    
	    if(pCir->rotate) {
		if (!pCir->PointerMoved) {
		    pCir->PointerMoved = pScrn->PointerMoved;
		    pScrn->PointerMoved = cirPointerMoved;
		}
		
		switch(pScrn->bitsPerPixel) {
		case 8:	refreshArea = cirRefreshArea8;	break;
		case 16:	refreshArea = cirRefreshArea16;	break;
		case 24:	refreshArea = cirRefreshArea24;	break;
		case 32:	refreshArea = cirRefreshArea32;	break;
		}
	    }
	    
	    ShadowFBInit(pScreen, refreshArea);
	}

	/* Initialise default colourmap */
	if (!miCreateDefColormap(pScreen))
		return FALSE;

	if (pScrn->bitsPerPixel > 1 && pScrn->bitsPerPixel <= 8)
		vgaHWHandleColormaps(pScreen);

	xf86DPMSInit(pScreen, AlpDisplayPowerManagementSet, 0);

	pScrn->memPhysBase = pCir->FbAddress;
	pScrn->fbOffset = 0;

	{
		XF86VideoAdaptorPtr *ptr;
		int n;

		n = xf86XVListGenericAdaptors(pScrn,&ptr);
		if (n)
			xf86XVScreenInit(pScreen, ptr, n);
	}

	/*
	 * Wrap the CloseScreen vector and set SaveScreen.
	 */
	pScreen->SaveScreen = AlpSaveScreen;
	pCir->CloseScreen = pScreen->CloseScreen;
	pScreen->CloseScreen = AlpCloseScreen;

	/* Report any unused options (only for the first generation) */
	if (serverGeneration == 1)
		xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);

	/* Done */
	return TRUE;
}


/* Usually mandatory */
Bool
AlpSwitchMode(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
	return AlpModeInit(pScrn, mode);
}


/*
 * This function is used to initialize the Start Address - the first
 * displayed location in the video memory.
 */
/* Usually mandatory */
void
AlpAdjustFrame(ScrnInfoPtr pScrn, int x, int y)
{
	int Base, tmp;
	vgaHWPtr hwp;

	hwp = VGAHWPTR(pScrn);

	Base = ((y * pScrn->displayWidth + x) / 8);
	if (pScrn->bitsPerPixel != 1)
		Base *= (pScrn->bitsPerPixel/4);

#ifdef ALP_DEBUG
	ErrorF("AlpAdjustFrame %d %d 0x%x %d %x\n", x, y, flags, Base, Base);
#endif

	if ((Base & ~0x000FFFFF) != 0) {
		ErrorF("X11: Internal error: AlpAdjustFrame: cannot handle overflow\n");
		return;
	}

	hwp->writeCrtc(hwp, 0x0C, (Base >> 8) & 0xff);
	hwp->writeCrtc(hwp, 0x0D, Base & 0xff);
	tmp = hwp->readCrtc(hwp, 0x1B);
	tmp &= 0xF2;
	tmp |= (Base >> 16) & 0x01;
	tmp |= (Base >> 15) & 0x0C;
	hwp->writeCrtc(hwp, 0x1B, tmp);
	tmp = hwp->readCrtc(hwp, 0x1D);
	tmp &= 0x7F;
	tmp |= (Base >> 12) & 0x80;
	hwp->writeCrtc(hwp, 0x1D, tmp);
}

/*
 * This is called when VT switching back to the X server.  Its job is
 * to reinitialise the video mode.
 *
 * We may wish to unmap video/MMIO memory too.
 */

/* Mandatory */
Bool
AlpEnterVT(ScrnInfoPtr pScrn)
{
	CirPtr pCir = CIRPTR(pScrn);
	Bool ret;

#ifdef ALP_DEBUG
	ErrorF("AlpEnterVT\n");
#endif

	/* Should we re-save the text mode on each VT enter? */
	if (!(ret = AlpModeInit(pScrn, pScrn->currentMode)))
		return FALSE;
	
	if (!pCir->NoAccel)
		pCir->InitAccel(pScrn);

	return ret;
}


/*
 * This is called when VT switching away from the X server.  Its job is
 * to restore the previous (text) mode.
 *
 * We may wish to remap video/MMIO memory too.
 */

/* Mandatory */
void
AlpLeaveVT(ScrnInfoPtr pScrn)
{
	vgaHWPtr hwp = VGAHWPTR(pScrn);
#ifdef ALP_DEBUG
	ErrorF("AlpLeaveVT\n");
#endif

	AlpRestore(pScrn);
	vgaHWLock(hwp);
}


/*
 * This is called at the end of each server generation.  It restores the
 * original (text) mode.  It should also unmap the video memory, and free
 * any per-generation data allocated by the driver.  It should finish
 * by unwrapping and calling the saved CloseScreen function.
 */

/* Mandatory */
static Bool
AlpCloseScreen(ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	vgaHWPtr hwp = VGAHWPTR(pScrn);
	CirPtr pCir = CIRPTR(pScrn);

	if(pScrn->vtSema) {
	    AlpRestore(pScrn);
	    vgaHWLock(hwp);
	    CirUnmapMem(pCir, pScrn->scrnIndex);
	}

	if (pCir->CursorInfoRec)
		xf86DestroyCursorInfoRec(pCir->CursorInfoRec);
	pCir->CursorInfoRec = NULL;
	if (pCir->DGAModes)
		free(pCir->DGAModes);
	pCir->DGAnumModes = 0;
	pCir->DGAModes = NULL;

	pScrn->vtSema = FALSE;

	pScreen->CloseScreen = pCir->CloseScreen;
	return (*pScreen->CloseScreen)(pScreen);
}


/* Free up any persistent data structures */

/* Optional */
void
AlpFreeScreen(ScrnInfoPtr pScrn)
{
#ifdef ALP_DEBUG
	ErrorF("AlpFreeScreen\n");
#endif
	/*
	 * This only gets called when a screen is being deleted.  It does not
	 * get called routinely at the end of a server generation.
	 */
	if (xf86LoaderCheckSymbol("vgaHWFreeHWRec"))
		vgaHWFreeHWRec(pScrn);
	AlpFreeRec(pScrn);
}


/* Checks if a mode is suitable for the selected chipset. */

/* Optional */
ModeStatus
AlpValidMode(ScrnInfoPtr arg, DisplayModePtr mode, Bool verbose, int flags)
{
	int lace;

	lace = 1 + ((mode->Flags & V_INTERLACE) != 0);

	if ((mode->CrtcHDisplay <= 2048) &&
		(mode->CrtcHSyncStart <= 4096) &&
		(mode->CrtcHSyncEnd <= 4096) &&
		(mode->CrtcHTotal <= 4096) &&
		(mode->CrtcVDisplay <= 2048 * lace) &&
		(mode->CrtcVSyncStart <= 4096 * lace) &&
		(mode->CrtcVSyncEnd <= 4096 * lace) &&
		(mode->CrtcVTotal <= 4096 * lace)) {
		return(MODE_OK);
	} else {
		return(MODE_BAD);
	}
}

/* Do screen blanking */

/* Mandatory */
static Bool
AlpSaveScreen(ScreenPtr pScreen, int mode)
{
	return vgaHWSaveScreen(pScreen, mode);
}

/*
 * Set the clock to the requested frequency.  If the MCLK is very close
 * to the requested frequency, it sets a flag so that the MCLK can be used
 * as VCLK.  However this flag is not yet acted upon.
 */
static void
AlpSetClock(CirPtr pCir, vgaHWPtr hwp, int freq)
{
	int num, den, ffreq;
	CARD8 tmp;

#ifdef ALP_DEBUG
	ErrorF("AlpSetClock freq=%d.%03dMHz\n", freq / 1000, freq % 1000);
#endif

	ffreq = freq;
	if (!CirrusFindClock(&ffreq, pCir->MaxClock, &num, &den))
		return;

#ifdef ALP_DEBUG
	ErrorF("AlpSetClock: nom=%x den=%x ffreq=%d.%03dMHz\n",
		num, den, ffreq / 1000, ffreq % 1000);
#endif
	/* So - how do we use MCLK here for the VCLK ? */

	/* Set VCLK3. */
	tmp = hwp->readSeq(hwp, 0x0E);
	hwp->writeSeq(hwp, 0x0E, (tmp & 0x80) | num);
	hwp->writeSeq(hwp, 0x1E, den);
}

/*
 * AlpDisplayPowerManagementSet --
 *
 * Sets VESA Display Power Management Signaling (DPMS) Mode.
 */
static void
AlpDisplayPowerManagementSet(ScrnInfoPtr pScrn, int PowerManagementMode,
			     int flags)
{
	unsigned char sr01, gr0e;
	vgaHWPtr hwp;

#ifdef ALP_DEBUG
	ErrorF("AlpDisplayPowerManagementSet\n");
#endif

	hwp = VGAHWPTR(pScrn);

#ifdef ALP_DEBUG
	ErrorF("AlpDisplayPowerManagementSet: %d\n", PowerManagementMode);
#endif

	switch (PowerManagementMode) {
	case DPMSModeOn:
		/* Screen: On; HSync: On, VSync: On */
		sr01 = 0x00;
		gr0e = 0x00;
		break;
	case DPMSModeStandby:
		/* Screen: Off; HSync: Off, VSync: On */
		sr01 = 0x20;
		gr0e = 0x02;
		break;
	case DPMSModeSuspend:
		/* Screen: Off; HSync: On, VSync: Off */
		sr01 = 0x20;
		gr0e = 0x04;
		break;
	case DPMSModeOff:
		/* Screen: Off; HSync: Off, VSync: Off */
		sr01 = 0x20;
		gr0e = 0x06;
		break;
	default:
		return;
	}

	sr01 |= hwp->readSeq(hwp, 0x01) & ~0x20;
	hwp->writeSeq(hwp, 0x01, sr01);
	gr0e |= hwp->readGr(hwp, 0x0E) & ~0x06;
	hwp->writeGr(hwp, 0x0E, gr0e);
}

#ifdef ALPPROBEI2C
static void AlpProbeI2C(int scrnIndex)
{
	int i;
	I2CBusPtr b;

	b = xf86I2CFindBus(scrnIndex, "I2C bus 1");
	if (b == NULL)
		ErrorF("Could not find I2C bus \"%s\"\n", "I2C bus 1");
	else {
		for (i = 2; i < 256; i += 2)
			if (xf86I2CProbeAddress(b, i))
				ErrorF("Found device 0x%02x on bus \"%s\"\n", i, b->BusName);
	}
	b = xf86I2CFindBus(scrnIndex, "I2C bus 2");
	if (b == NULL)
		ErrorF("Could not find I2C bus \"%s\"\n", "I2C bus 2");
	else {
		for (i = 2; i < 256; i += 2)
			if (xf86I2CProbeAddress(b, i))
				ErrorF("Found device 0x%02x on bus \"%s\"\n", i, b->BusName);
	}
}
#endif

static void
AlpProbeLCD(ScrnInfoPtr pScrn)
{
    CirPtr pCir = CIRPTR(pScrn); 
    AlpPtr pAlp = ALPPTR(pCir);

    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CARD8 lcdCrtl;

    static const char* lcd_type_names[] =
    {
        "none",
	"dual-scan monochrome",
	"unknown",
	"DSTN (dual scan color)",
	"TFT (active matrix)"
    };


    pAlp->lcdType = LCD_NONE;

    switch (pCir->Chipset)  {
    case PCI_CHIP_GD7548:
        switch (hwp->readCrtc(hwp, 0x2C) >> 6) {
	case 0: pAlp->lcdType = LCD_DUAL_MONO; break;
	case 1: pAlp->lcdType = LCD_UNKNOWN; break;
	case 2: pAlp->lcdType = LCD_DSTN; break;
	case 3: pAlp->lcdType = LCD_TFT; break;
	}

	/* Enable LCD control registers instead of normal CRTC registers */
	lcdCrtl = hwp->readCrtc(hwp, 0x2D);
	hwp->writeCrtc(hwp, 0x2D, lcdCrtl | 0x80);

	switch ((hwp->readCrtc(hwp, 0x9) >> 2) & 3)  {
	  case 0:
	      pAlp->lcdWidth = 640;
	      pAlp->lcdHeight = 480;
	      break;

	 case 1:
	      pAlp->lcdWidth = 800;
	      pAlp->lcdHeight = 600;
	      break;

	  case 2:
	      pAlp->lcdWidth = 1024;
	      pAlp->lcdHeight = 768;
	      break;

	  case 3:
	      pAlp->lcdWidth = 0;
	      pAlp->lcdHeight = 0;
	      break;
	}
	
	/* Disable LCD control registers */
	hwp->writeCrtc(hwp, 0x2D, lcdCrtl);
	break;
    }

    if (pAlp->lcdType != LCD_NONE) {
      xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		 "LCD display: %dx%d %s\n",
		 pAlp->lcdWidth, pAlp->lcdHeight,
		 lcd_type_names[pAlp->lcdType]);
    }
}

static void
AlpOffscreenAccelInit(ScrnInfoPtr pScrn)
{
    CirPtr pCir = CIRPTR(pScrn);
    AlpPtr pAlp = ALPPTR(pCir);

    if (pCir->offscreen_size >= 8  && pCir->Chipset == PCI_CHIP_GD7548) {
        pCir->offscreen_offset -= 8;
	pCir->offscreen_size -= 8;
	pAlp->monoPattern8x8 = pCir->offscreen_offset;
#ifdef ALP_DEBUG
	ErrorF("monoPattern8x8=%d\n", pAlp->monoPattern8x8);
#endif
    }  else pAlp->monoPattern8x8 = 0;

    {
    /* TODO: probably not correct if rotated */
	BoxRec box = {
	    .x1 = 0,
	    .y1 = 0,
	    .x2 = pScrn->virtualX,
	    .y2 = pCir->offscreen_offset / pCir->pitch
	};
	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		   "Using %d lines for offscreen memory\n",
		   box.y2 - pScrn->virtualY);
    }
}
