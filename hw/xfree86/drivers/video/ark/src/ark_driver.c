/*
 *	Copyright 2000	Ani Joshi <ajoshi@unixbox.com>
 *
 *	XFree86 4.x driver for ARK Logic chipset
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation and   
 * that the name of Ani Joshi not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Ani Joshi makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as-is" without express or implied warranty.
 *
 * ANI JOSHI DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ANI JOSHI BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 *
 *	Based on the 3.3.x driver by:
 *		Harm Hanemaayer <H.Hanemaayer@inter.nl.net>
 *
 */
#include <xorg-config.h>

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Pci.h"
#include "xf86fbman.h"
#include "xf86cmap.h"
#include "compiler.h"
#include "mipointer.h"
#include "micmap.h"
#include "fb.h"
#include "ark.h"

#define PIOOFFSET 0  /* was hwp->PIOOffset in ABI_VIDEODRV_VERSION < 12 */

#include <string.h>

/*
 * prototypes
 */
static const OptionInfoRec * ARKAvailableOptions(int chipid, int busid);
static void ARKIdentify(int flags);
static Bool ARKProbe(DriverPtr drv, int flags);
static Bool ARKPreInit(ScrnInfoPtr pScrn, int flags);
static Bool ARKEnterVT(ScrnInfoPtr pScrn);
static void ARKLeaveVT(ScrnInfoPtr pScrn);
static void ARKSave(ScrnInfoPtr pScrn);
static Bool ARKScreenInit(ScreenPtr pScreen, int argc, char **argv);
static Bool ARKMapMem(ScrnInfoPtr pScrn);
static void ARKUnmapMem(ScrnInfoPtr pScrn);
static Bool ARKModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
static void ARKAdjustFrame(ScrnInfoPtr pScrn, int x, int y);
Bool ARKSwitchMode(ScrnInfoPtr pScrn, DisplayModePtr mode);
Bool ARKCloseScreen(ScreenPtr pScreen);
Bool ARKSaveScreen(ScreenPtr pScreen, int mode);
static void ARKFreeScreen(ScrnInfoPtr pScrn);
static void ARKLoadPalette(ScrnInfoPtr pScrn, int numColors,
			   int *indices, LOCO *colors,
			   VisualPtr pVisual);
static void ARKWriteMode(ScrnInfoPtr pScrn, vgaRegPtr pVga, ARKRegPtr new);

/* helpers */
static unsigned char get_daccomm(unsigned long);
static unsigned char set_daccom(unsigned long, unsigned char comm);


_X_EXPORT DriverRec ARK =
{
	ARK_VERSION,
	DRIVER_NAME,
	ARKIdentify,
	ARKProbe,
	ARKAvailableOptions,
	NULL,
	0
};

/* supported chipsets */
static SymTabRec ARKChipsets[] = {
	{ PCI_CHIP_1000PV, "ark1000pv" },
	{ PCI_CHIP_2000PV, "ark2000pv" },
	{ PCI_CHIP_2000MT, "ark2000mt" },
	{ -1,		   NULL }
};

static PciChipsets ARKPciChipsets[] = {
	{ PCI_CHIP_1000PV, PCI_CHIP_1000PV, RES_SHARED_VGA },
	{ PCI_CHIP_2000PV, PCI_CHIP_2000PV, RES_SHARED_VGA },
	{ PCI_CHIP_2000MT, PCI_CHIP_2000MT, RES_SHARED_VGA },
	{ -1,		   -1,		    RES_UNDEFINED }
};

typedef enum {
	OPTION_NOACCEL
} ARKOpts;

static const OptionInfoRec ARKOptions[] = {
	{ OPTION_NOACCEL, "noaccel", OPTV_BOOLEAN, {0}, FALSE },
	{ -1,		  NULL,	     OPTV_NONE,	   {0}, FALSE }
};

#ifdef XFree86LOADER

MODULESETUPPROTO(ARKSetup);

static XF86ModuleVersionInfo ARKVersRec = {
	"ark",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XORG_VERSION_CURRENT,
	VERSION_MAJOR, VERSION_MINOR, PATCHLEVEL,
	ABI_CLASS_VIDEODRV,
	ABI_VIDEODRV_VERSION,
	MOD_CLASS_VIDEODRV,
	{0, 0, 0, 0}
};

_X_EXPORT XF86ModuleData arkModuleData = { &ARKVersRec, ARKSetup, NULL };

pointer ARKSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
	static Bool setupDone = FALSE;

	if (!setupDone) {
		setupDone = TRUE;
		xf86AddDriver(&ARK, module, 0);
		return (pointer) 1;
	} else {
		if (errmaj)
			*errmaj = LDR_ONCEONLY;
		return NULL;
	}
}

#endif /* XFree86LOADER */


static Bool ARKGetRec(ScrnInfoPtr pScrn)
{
	if (pScrn->driverPrivate)
		return TRUE;

	pScrn->driverPrivate = XNFcallocarray(sizeof(ARKRec), 1);

	return TRUE;
}

static void ARKFreeRec(ScrnInfoPtr pScrn)
{
	free(pScrn->driverPrivate);
	pScrn->driverPrivate = NULL;
}

static const OptionInfoRec * ARKAvailableOptions(int chipid, int busid)
{
	return ARKOptions;
}

static void ARKIdentify(int flags)
{
	xf86PrintChipsets("ARK", "driver (version " DRIVER_VERSION " for ARK Logic chipset",
			  ARKChipsets);
}

static Bool ARKProbe(DriverPtr drv, int flags)
{
	int i;
	GDevPtr *devSections;
	int *usedChips;
	int numDevSections;
	int numUsed;
	Bool foundScreen = FALSE;

	/* sanity check */
	if ((numDevSections = xf86MatchDevice("ark", &devSections)) <= 0)
		return FALSE;

	/* do ISA later */
	numUsed = xf86MatchPciInstances("ark", PCI_VENDOR_ARK,
					ARKChipsets, ARKPciChipsets,
					devSections, numDevSections, drv,
					&usedChips);

	free(devSections);

	if (numUsed <= 0)
		return FALSE;

	if (flags & PROBE_DETECT)
		foundScreen = TRUE;
	else for (i=0; i<numUsed; i++) {
	        ScrnInfoPtr pScrn = NULL;

		pScrn = xf86ConfigPciEntity(pScrn, 0, usedChips[i], ARKPciChipsets,
					    NULL, NULL, NULL, NULL, NULL);

		pScrn->driverVersion = VERSION_MAJOR;
		pScrn->driverName = DRIVER_NAME;
		pScrn->name = "ark";
		pScrn->Probe = ARKProbe;
		pScrn->PreInit = ARKPreInit;
		pScrn->ScreenInit = ARKScreenInit;
		pScrn->SwitchMode = ARKSwitchMode;
		pScrn->AdjustFrame = ARKAdjustFrame;
		pScrn->EnterVT = ARKEnterVT;
		pScrn->LeaveVT = ARKLeaveVT;
		pScrn->FreeScreen = ARKFreeScreen;
		foundScreen = TRUE;
	}

	free(usedChips);

	return foundScreen;
}


static Bool ARKPreInit(ScrnInfoPtr pScrn, int flags)
{
	EntityInfoPtr pEnt;
	ARKPtr pARK;
	vgaHWPtr hwp;
	int i;
	ClockRangePtr clockRanges;
	rgb zeros = {0, 0, 0};
	Gamma gzeros = {0.0, 0.0, 0.0};
	unsigned char tmp;

	if (flags & PROBE_DETECT)
		return FALSE;

	if (!xf86LoadSubModule(pScrn, "vgahw"))
		return FALSE;

	if (!vgaHWGetHWRec(pScrn))
		return FALSE;

	hwp = VGAHWPTR(pScrn);
	vgaHWSetStdFuncs(hwp);
	vgaHWGetIOBase(hwp);

	pScrn->monitor = pScrn->confScreen->monitor;

	if (!xf86SetDepthBpp(pScrn, 0, 0, 0, Support24bppFb | Support32bppFb))
		return FALSE;
	else {
		switch (pScrn->depth) {
			case 8:
			case 16:
			case 24:
			case 32:
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

	if (pScrn->depth > 8) {
		if (!xf86SetWeight(pScrn, zeros, zeros))
			return FALSE;
	}

	if (pScrn->depth == 8)
		pScrn->rgbBits = 8;

	if (!xf86SetDefaultVisual(pScrn, -1))
		return FALSE;

	pScrn->progClock = TRUE;

	if (!ARKGetRec(pScrn))
		return FALSE;

	pARK = ARKPTR(pScrn);

	xf86CollectOptions(pScrn, NULL);
	if (!(pARK->Options = malloc(sizeof(ARKOptions))))
		return FALSE;
	memcpy(pARK->Options, ARKOptions, sizeof(ARKOptions));
	xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, pARK->Options);

	if (xf86ReturnOptValBool(pARK->Options, OPTION_NOACCEL, FALSE)) {
		pARK->NoAccel = TRUE;
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Option: NoAccel - acceleration disabled\n");
	} else
		pARK->NoAccel = FALSE;

	if (pScrn->numEntities > 1) {
		ARKFreeRec(pScrn);
		return FALSE;
	}

	pEnt = xf86GetEntityInfo(pScrn->entityList[0]);
	pARK->PciInfo = xf86GetPciInfoForEntity(pEnt->index);

	if (pEnt->device->chipset && *pEnt->device->chipset) {
		pScrn->chipset = pEnt->device->chipset;
		pARK->Chipset = xf86StringToToken(ARKChipsets, pScrn->chipset);
	} else if (pEnt->device->chipID >= 0) {
		pARK->Chipset = pEnt->device->chipID;
		pScrn->chipset = (char *)xf86TokenToString(ARKChipsets,
							   pARK->Chipset);
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipID override: 0x%04X\n",
			   pARK->Chipset);
	} else {
#ifndef XSERVER_LIBPCIACCESS
		pARK->Chipset = pARK->PciInfo->chipType;
#else
		pARK->Chipset = pARK->PciInfo->device_id;
#endif
		pScrn->chipset = (char *)xf86TokenToString(ARKChipsets,
							   pARK->Chipset);
	}

	if (pEnt->device->chipRev >= 0) {
		pARK->ChipRev = pEnt->device->chipRev;
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipRev override: %d\n",
			   pARK->ChipRev);
	} else {
#ifndef XSERVER_LIBPCIACCESS
		pARK->ChipRev = pARK->PciInfo->chipRev;
#else
		pARK->ChipRev = pARK->PciInfo->revision;
#endif
	}
	free(pEnt);

	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Chipset: \"%s\"\n", pScrn->chipset);

#ifndef XSERVER_LIBPCIACCESS
	pARK->PciTag = pciTag(pARK->PciInfo->bus, pARK->PciInfo->device,
			      pARK->PciInfo->func);
#endif

	/* unlock CRTC[0-7] */
	outb(PIOOFFSET + hwp->IOBase + 4, 0x11);
	tmp = inb(PIOOFFSET + hwp->IOBase + 5);
	outb(PIOOFFSET + hwp->IOBase + 5, tmp & 0x7f);
	modinx(PIOOFFSET + 0x3c4, 0x1d, 0x01, 0x01);

#ifndef XSERVER_LIBPCIACCESS
	pScrn->memPhysBase = pARK->PciInfo->memBase[0];
#else
	pScrn->memPhysBase = pARK->PciInfo->regions[0].base_addr;
#endif

	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Framebuffer @ 0x%lx\n",
		   (unsigned long)pScrn->memPhysBase);

	if (!xf86SetGamma(pScrn, gzeros))
		return FALSE;

	if (!pScrn->videoRam) {
		unsigned char sr10;

		sr10 = rdinx(PIOOFFSET + 0x3c4, 0x10);
		if (pARK->Chipset == PCI_CHIP_1000PV) {
			if ((sr10 & 0x40) == 0)
				pScrn->videoRam = 1024;
			else
				pScrn->videoRam = 2048;
		}
		if (pARK->Chipset == PCI_CHIP_2000PV ||
		    pARK->Chipset == PCI_CHIP_2000MT) {
			if ((sr10 & 0xc0) == 0)
				pScrn->videoRam = 1024;
			else if ((sr10 & 0xc0) == 0x40)
				pScrn->videoRam = 2048;
			else
				pScrn->videoRam = 4096;
		}

		xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Detected %d bytes video ram\n",
			   pScrn->videoRam);
	}

	/* try to detect the RAMDAC */
	{
		int man_id, dev_id;

		inb(PIOOFFSET + 0x3c6);		/* skip cmd register */
		man_id = inb(PIOOFFSET + 0x3c6);	/* manufacturer id */
		dev_id = inb(PIOOFFSET + 0x3c6);	/* device id */
		if (man_id == 0x84 && dev_id == 0x98) {
			pARK->ramdac = ZOOMDAC;
			pARK->dac_width = 16;
			pARK->multiplex_threshold = 40000;
			xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
				   "Detected ZOOMDAC\n");
		}
	}

	/* hack for this Bali32 */
	pARK->ramdac = ATT490;
	pARK->dac_width = 8;

	pARK->clock_mult = 1;
	if (pARK->dac_width == 16) {
		if (pScrn->bitsPerPixel == 32)
			pARK->clock_mult = 2;
	}

	clockRanges = XNFcallocarray(sizeof(ClockRange), 1);
	clockRanges->next = NULL;
	clockRanges->minClock = 20000;
	clockRanges->maxClock = 80000;
	clockRanges->clockIndex = -1;
	clockRanges->interlaceAllowed = FALSE; /* ? */
	clockRanges->doubleScanAllowed = FALSE; /* ? */

	i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
			      pScrn->display->modes, clockRanges,
			      NULL, 256, 2048, pScrn->bitsPerPixel,
			      128, 2048, pScrn->display->virtualX,
			      pScrn->display->virtualY, pARK->videoRam * 1024,
			      LOOKUP_BEST_REFRESH);
	if (i == -1) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "no valid modes left\n");
		ARKFreeRec(pScrn);
		return FALSE;
	}

	xf86PruneDriverModes(pScrn);

	if (i == 0 || pScrn->modes == NULL) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "no valid modes found\n");
		ARKFreeRec(pScrn);
		return FALSE;
	}

	xf86SetCrtcForModes(pScrn, 0);
	pScrn->currentMode = pScrn->modes;
	xf86PrintModes(pScrn);
	xf86SetDpi(pScrn, 0, 0);

	if (!xf86LoadSubModule(pScrn, "fb")) {
	    ARKFreeRec(pScrn);
	    return FALSE;
	}

	if (!pARK->NoAccel) {
		if (!xf86LoadSubModule(pScrn, "xaa")) {
			xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
				   "XAA not available\n");
			pARK->NoAccel = 1;
		}
	}

	return TRUE;
}

static Bool ARKScreenInit(ScreenPtr pScreen, int argc, char **argv)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	ARKPtr pARK = ARKPTR(pScrn);

	pScrn->fbOffset = 0;

	if (!ARKMapMem(pScrn)) {
		ARKFreeRec(pScrn);
		return FALSE;
	}

	ARKSave(pScrn);

	vgaHWBlankScreen(pScrn, TRUE);

	if (!ARKModeInit(pScrn, pScrn->currentMode))
		return FALSE;

	ARKSaveScreen(pScreen, SCREEN_SAVER_ON);

	pScrn->AdjustFrame(pScrn, pScrn->frameX0, pScrn->frameY0);

	miClearVisualTypes();
	if (pScrn->bitsPerPixel > 8) {
		if (!miSetVisualTypes(pScrn->depth, TrueColorMask,
				      pScrn->rgbBits, pScrn->defaultVisual))
			return FALSE;
	} else {
		if (!miSetVisualTypes(pScrn->depth, miGetDefaultVisualMask(pScrn->depth),
				      pScrn->rgbBits, pScrn->defaultVisual))
			return FALSE;
	}

	miSetPixmapDepths ();

	if (!fbScreenInit(pScreen, pARK->FBBase, pScrn->virtualX,
			  pScrn->virtualY, pScrn->xDpi, pScrn->yDpi,
			  pScrn->displayWidth, pScrn->bitsPerPixel))
		return FALSE;

	xf86SetBlackWhitePixels(pScreen);

	if (pScrn->bitsPerPixel > 8) {
		VisualPtr pVis;

		pVis = pScreen->visuals + pScreen->numVisuals;
		while (--pVis >= pScreen->visuals) {
			if ((pVis->class | DynamicClass) == DirectColor) {
				pVis->offsetRed = pScrn->offset.red;
				pVis->offsetGreen = pScrn->offset.green;
				pVis->offsetBlue = pScrn->offset.blue;
				pVis->redMask = pScrn->mask.red;
				pVis->greenMask = pScrn->mask.green;
				pVis->blueMask = pScrn->mask.blue;
			}
		}
	}

	/* must be after RGB order fixed */

	fbPictureInit (pScreen, 0, 0);

	xf86SetBackingStore(pScreen);

	if (!pARK->NoAccel) {
            xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Acceleration not available\n");
	} else {
            xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Acceleration disabled\n");
	}

	miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

	if (!miCreateDefColormap(pScreen))
		return FALSE;

	if (!xf86HandleColormaps(pScreen, 256, 8, ARKLoadPalette, NULL,
				 CMAP_RELOAD_ON_MODE_SWITCH))
		return FALSE;

	vgaHWBlankScreen(pScrn, FALSE);

	pScreen->SaveScreen = ARKSaveScreen;
	pARK->CloseScreen = pScreen->CloseScreen;
	pScreen->CloseScreen = ARKCloseScreen;

	return TRUE;
}



static void ARKSave(ScrnInfoPtr pScrn)
{
	ARKPtr pARK = ARKPTR(pScrn);
	ARKRegPtr save = &pARK->SavedRegs;
	vgaHWPtr hwp = VGAHWPTR(pScrn);
	unsigned long isaIOBase = PIOOFFSET;
	unsigned long vgaIOBase = isaIOBase + hwp->IOBase;

	vgaHWUnlock(hwp);
	vgaHWSave(pScrn, &hwp->SavedReg, VGA_SR_ALL);
	vgaHWLock(hwp);

	/* set read and write aperture index to 0 */
	wrinx(isaIOBase + 0x3c4, 0x15, 0x00);
	wrinx(isaIOBase + 0x3c4, 0x16, 0x00);
	outb(isaIOBase + 0x3c8, 0);	/* reset DAC register access mode */

	save->sr10 = rdinx(isaIOBase + 0x3c4, 0x10);
	save->sr11 = rdinx(isaIOBase + 0x3c4, 0x11);
	save->sr12 = rdinx(isaIOBase + 0x3c4, 0x12);
	save->sr13 = rdinx(isaIOBase + 0x3c4, 0x13);
	save->sr14 = rdinx(isaIOBase + 0x3c4, 0x14);
	save->sr15 = rdinx(isaIOBase + 0x3c4, 0x15);
	save->sr16 = rdinx(isaIOBase + 0x3c4, 0x16);
	save->sr17 = rdinx(isaIOBase + 0x3c4, 0x17);
	save->sr18 = rdinx(isaIOBase + 0x3c4, 0x18);

#if 0
	save->sr1d = rdinx(isaIOBase + 0x3c4, 0x1d);
	save->sr1c = rdinx(isaIOBase + 0x3c4, 0x1c);

	save->sr20 = rdinx(isaIOBase + 0x3c4, 0x20);
	save->sr21 = rdinx(isaIOBase + 0x3c4, 0x21);
	save->sr22 = rdinx(isaIOBase + 0x3c4, 0x22);
	save->sr23 = rdinx(isaIOBase + 0x3c4, 0x23);
	save->sr24 = rdinx(isaIOBase + 0x3c4, 0x24);
	save->sr25 = rdinx(isaIOBase + 0x3c4, 0x25);
	save->sr26 = rdinx(isaIOBase + 0x3c4, 0x26);
	save->sr27 = rdinx(isaIOBase + 0x3c4, 0x27);
	save->sr29 = rdinx(isaIOBase + 0x3c4, 0x29);
	save->sr2a = rdinx(isaIOBase + 0x3c4, 0x2a);
	if ((pARK->Chipset == PCI_CHIP_2000PV) ||
	    (pARK->Chipset == PCI_CHIP_2000MT)) {
		save->sr28 = rdinx(isaIOBase + 0x3c4, 0x28);
		save->sr2b = rdinx(isaIOBase + 0x3c4, 0x2b);
	}
#endif

	save->cr40 = rdinx(vgaIOBase + 4, 0x40);
	save->cr41 = rdinx(vgaIOBase + 4, 0x41);
	save->cr42 = rdinx(vgaIOBase + 4, 0x42);
	save->cr44 = rdinx(vgaIOBase + 4, 0x44);

	if ((pARK->Chipset == PCI_CHIP_2000PV) ||
	    (pARK->Chipset == PCI_CHIP_2000MT))
		save->cr46 = rdinx(vgaIOBase + 4, 0x46);

	/* save RAMDAC regs here, based on type */
	save->dac_command = get_daccomm(isaIOBase);
}



static Bool ARKModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
	ARKPtr pARK = ARKPTR(pScrn);
	ARKRegPtr new = &pARK->ModeRegs;
	int multiplexing, dac16, modepitch;
	vgaHWPtr hwp = VGAHWPTR(pScrn);
	vgaRegPtr pVga = &hwp->ModeReg;
	unsigned long isaIOBase = PIOOFFSET;
	unsigned long vgaIOBase = isaIOBase + hwp->IOBase;
	unsigned char tmp;
	int offset;

	multiplexing = 0;

	if ((pScrn->bitsPerPixel == 8) && (pARK->dac_width == 16) &&
	    (mode->Clock > pARK->multiplex_threshold))
		multiplexing = 1;

	if (pARK->clock_mult == 2) {
		if (!mode->CrtcHAdjusted) {
			mode->CrtcHDisplay <<= 1;
			mode->CrtcHSyncStart <<= 1;
			mode->CrtcHSyncEnd <<= 1;
			mode->CrtcHTotal <<= 1;
			mode->CrtcHSkew <<= 1;
			mode->CrtcHAdjusted = TRUE;
		}
	}

	if (multiplexing) {
		if (!mode->CrtcHAdjusted) {
			mode->CrtcHDisplay >>= 1;
			mode->CrtcHSyncStart >>= 1;
			mode->CrtcHSyncEnd >>= 1;
			mode->CrtcHTotal >>= 1;
			mode->CrtcHSkew >>= 1;
			mode->CrtcHAdjusted = TRUE;
		}
	}

	if (!vgaHWInit(pScrn, mode))
		return FALSE;

	if ((pARK->Chipset == PCI_CHIP_2000PV) ||
	    (pARK->Chipset == PCI_CHIP_2000MT)) {
		new->cr46 = rdinx(vgaIOBase + 4, 0x46) & ~0x04;
		dac16 = 0;
		if (pScrn->bitsPerPixel > 8)
			dac16 = 1;
		if (dac16)
			new->cr46 |= 0x04;
	}

	offset = (pScrn->displayWidth * (pScrn->bitsPerPixel / 8)) >> 3;
	pVga->CRTC[0x13] = offset;
	new->cr41 = (offset & 0x100) >> 5;

	new->sr11 = 0x90;
	switch (pScrn->bitsPerPixel) {
		case 8:
			new->sr11 |= 0x06;
			break;
		case 16:
			new->sr11 |= 0x0a;
			break;
		case 24:
			new->sr11 |= 0x06;
			break;
		case 32:
			if ((pARK->Chipset == PCI_CHIP_2000PV) ||
			    (pARK->Chipset == PCI_CHIP_2000MT))
				new->sr11 |= 0x0e;
			else
				new->sr11 |= 0x0a;
			break;
		default:
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
				   "Unsupported screen depth %d\n",
				   pScrn->bitsPerPixel);
			return FALSE;
	}

	switch (pScrn->displayWidth) {
		case 640:
			modepitch = 0;
			break;
		case 800:
			modepitch = 1;
			break;
		case 1024:
			modepitch = 2;
			break;
		case 1280:
			modepitch = 4;
			break;
		case 1600:
			modepitch = 5;
			break;
		case 2048:
			modepitch = 6;
			break;
		default:
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
				   "Unsupported screen width %d\n",
				   pScrn->displayWidth);
			return FALSE;
	}	

	new->sr17 &= ~0xc7;
	new->sr17 |= modepitch;

	new->sr10 = rdinx(isaIOBase + 0x3c4, 0x10) & ~0x1f;
	new->sr10 |= 0x1f;

#ifndef XSERVER_LIBPCIACCESS
	new->sr13 = pARK->PciInfo->memBase[0] >> 16;
	new->sr14 = pARK->PciInfo->memBase[0] >> 24;
#else
	new->sr13 = pARK->PciInfo->regions[0].base_addr >> 16;
	new->sr14 = pARK->PciInfo->regions[0].base_addr >> 24;
#endif

	new->sr12 = rdinx(isaIOBase + 0x3c4, 0x12) & ~0x03;
	switch (pScrn->videoRam) {
		case 1024:
			new->sr12 |= 0x01;
			break;
		case 2048:
			new->sr12 |= 0x02;
			break;
		case 4096:
			new->sr12 |= 0x03;
			break;
		default:
			new->sr12 |= 0x01;
			break;
	}

	new->sr15 = new->sr16 = 0;

	tmp = 0;
	if ((mode->CrtcVTotal - 2) & 0x400)
		tmp |= 0x80;
	if ((mode->CrtcVDisplay - 1) & 0x400)
		tmp |= 0x40;
	if (mode->CrtcVSyncStart & 0x400)
		tmp |= 0x10;
	new->cr40 = tmp;

	tmp = new->cr41;	/* initialized earlier */
	if ((mode->CrtcHTotal / 8 - 5) & 0x100)
		tmp |= 0x80;
	if ((mode->CrtcHDisplay / 8 - 1) & 0x100)
		tmp |= 0x40;
	if ((mode->CrtcHSyncStart / 8 - 1) & 0x100)
		tmp |= 0x20;
	if ((mode->CrtcHSyncStart / 8) & 0x100)
		tmp |= 0x10;
	new->cr41 |= tmp;

	new->cr44 = rdinx(vgaIOBase + 4, 0x44) & ~0x34;
	new->cr44 &= ~0x01;
	new->cr42 = 0;

	/* check interlace here later */

	/* set display FIFO threshold */
	{
		int threshold;
		int bandwidthused, percentused;

		/* mostly guesses here as I would need to know more about
		 * and from the ramdac...
		 */
		bandwidthused = (mode->Clock / pARK->clock_mult) *
				(pScrn->bitsPerPixel / 8);
		/* 120000 is another guess */
		percentused = (bandwidthused * 100) / 120000;
		tmp = rdinx(isaIOBase + 0x3c4, 0x18);
		if (pARK->Chipset == PCI_CHIP_1000PV) {
			threshold = 4;
			tmp |= 0x08;	/* enable full FIFO (8 deep) */
			tmp &= ~0x07;
			tmp |= threshold;
		}
		if ((pARK->Chipset == PCI_CHIP_2000PV) ||
		    (pARK->Chipset == PCI_CHIP_2000MT)) {
			threshold = 12;
			if (percentused >= 45)
				threshold = 8;
			if (percentused >= 70)
				threshold = 4;
			tmp &= 0x40;
			tmp |= 0x10;
			tmp |= (threshold & 0x0e) >> 1;
			if (threshold & 0x01)
				tmp |= 0x80;
			if (threshold & 0x10)
				tmp |= 0x20;
		}
		new->sr18 = tmp;
	}

	/* setup the RAMDAC regs */
	if (pARK->ramdac == ZOOMDAC) {
		new->dac_command = 0x04;
		if ((pScrn->bitsPerPixel == 8) && multiplexing)
			new->dac_command = 0x24;
		if ((pScrn->bitsPerPixel == 16) && (pARK->dac_width == 16))
			/* assuming green weight is not 5 */
			new->dac_command = 0x34;
		if ((pScrn->bitsPerPixel == 16) && (pARK->dac_width == 8))
			new->dac_command = 0x64;
		if ((pScrn->bitsPerPixel == 24) && (pARK->dac_width == 16))
			new->dac_command = 0xb4;	/* packed */
		if ((pScrn->bitsPerPixel == 32) && (pARK->dac_width == 16))
			new->dac_command = 0x54;
	} else if (pARK->ramdac == ATT490) {
		new->dac_command = 0x00;
		if (pScrn->bitsPerPixel == 16)
			/* assuming green weight is 6 */
			new->dac_command = 0xc0;
		if (pScrn->bitsPerPixel == 24)
			new->dac_command = 0xe0;
	}

#if 0
	/* hw cursor regs */
	new->sr20 = rdinx(isaIOBase + 0x3c4, 0x20);
	new->sr21 = rdinx(isaIOBase + 0x3c4, 0x21);
	new->sr22 = rdinx(isaIOBase + 0x3c4, 0x22);
	new->sr23 = rdinx(isaIOBase + 0x3c4, 0x23);
	new->sr24 = rdinx(isaIOBase + 0x3c4, 0x24);
	new->sr25 = rdinx(isaIOBase + 0x3c4, 0x25);
	new->sr26 = rdinx(isaIOBase + 0x3c4, 0x26);
	new->sr27 = rdinx(isaIOBase + 0x3c4, 0x27);
	new->sr29 = rdinx(isaIOBase + 0x3c4, 0x29);
	new->sr2a = rdinx(isaIOBase + 0x3c4, 0x2a);
	if ((pARK->Chipset == PCI_CHIP_2000PV) ||
	    (pARK->Chipset == PCI_CHIP_2000MT)) {
		new->sr28 = rdinx(isaIOBase + 0x3c4, 0x28);
		new->sr2b = rdinx(isaIOBase + 0x3c4, 0x3b);
	}
#endif


	ARKWriteMode(pScrn, pVga, new);

	return TRUE;
}


static void ARKAdjustFrame(ScrnInfoPtr pScrn, int x, int y)
{
	ARKPtr pARK = ARKPTR(pScrn);
	vgaHWPtr hwp = VGAHWPTR(pScrn);
	unsigned long vgaIOBase = PIOOFFSET + hwp->IOBase;
	int base;

	base = ((y * pScrn->displayWidth + x) *
		(pScrn->bitsPerPixel / 8));

	if (((pARK->Chipset == PCI_CHIP_2000PV) ||
	     (pARK->Chipset == PCI_CHIP_2000MT)) &&
	     (pScrn->videoRam >= 2048))
		base >>= 3;
	else
		base >>= 2;
	if (pScrn->bitsPerPixel == 24)
		base -= base % 3;

	outw(vgaIOBase + 4, (base & 0x00ff00) | 0x0c);
	outw(vgaIOBase + 4, ((base & 0x00ff) << 8) | 0x0d);

	modinx(vgaIOBase + 4, 0x40, 0x07, (base & 0x070000) >> 16);
}



static void ARKWriteMode(ScrnInfoPtr pScrn, vgaRegPtr pVga, ARKRegPtr new)
{
	ARKPtr pARK = ARKPTR(pScrn);
	vgaHWPtr hwp = VGAHWPTR(pScrn);
	unsigned long isaIOBase = PIOOFFSET;
	unsigned long vgaIOBase = isaIOBase + hwp->IOBase;

	vgaHWProtect(pScrn, TRUE);

	/* set read and write aperture index to 0 */
	wrinx(isaIOBase + 0x3c4, 0x15, 0x00);
	wrinx(isaIOBase + 0x3c4, 0x16, 0x00);

	/* write the extended registers first so that textmode font
	 * restoration can succeed
	 */
	wrinx(isaIOBase + 0x3c4, 0x10, new->sr10);
	wrinx(isaIOBase + 0x3c4, 0x11, new->sr11);
	wrinx(isaIOBase + 0x3c4, 0x12, new->sr12);
	wrinx(isaIOBase + 0x3c4, 0x13, new->sr13);
	wrinx(isaIOBase + 0x3c4, 0x14, new->sr14);
	wrinx(isaIOBase + 0x3c4, 0x15, new->sr15);
	wrinx(isaIOBase + 0x3c4, 0x16, new->sr16);
	wrinx(isaIOBase + 0x3c4, 0x17, new->sr17);

#if 0
	wrinx(isaIOBase + 0x3c4, 0x1c, new->sr1c);
	wrinx(isaIOBase + 0x3c4, 0x1d, new->sr1d);

	/* hw cursor regs */
	wrinx(isaIOBase + 0x3c4, 0x20, new->sr20);
	wrinx(isaIOBase + 0x3c4, 0x21, new->sr21);
	wrinx(isaIOBase + 0x3c4, 0x22, new->sr22);
	wrinx(isaIOBase + 0x3c4, 0x23, new->sr23);
	wrinx(isaIOBase + 0x3c4, 0x24, new->sr24);
	wrinx(isaIOBase + 0x3c4, 0x25, new->sr25);
	wrinx(isaIOBase + 0x3c4, 0x26, new->sr26);
	wrinx(isaIOBase + 0x3c4, 0x27, new->sr27);
	wrinx(isaIOBase + 0x3c4, 0x29, new->sr29);
	wrinx(isaIOBase + 0x3c4, 0x2a, new->sr2a);
#endif

	if ((pARK->Chipset == PCI_CHIP_2000PV) ||
	    (pARK->Chipset == PCI_CHIP_2000MT)) {
		wrinx(isaIOBase + 0x3c4, 0x28, new->sr28);
		wrinx(isaIOBase + 0x3c4, 0x2B, new->sr2b);
	}

	wrinx(vgaIOBase + 4, 0x40, new->cr40);
	wrinx(vgaIOBase + 4, 0x41, new->cr41);
	wrinx(vgaIOBase + 4, 0x42, new->cr42);
	wrinx(vgaIOBase + 4, 0x44, new->cr44);

	if ((pARK->Chipset == PCI_CHIP_2000PV) ||
	    (pARK->Chipset == PCI_CHIP_2000MT))
		wrinx(vgaIOBase + 4, 0x46, new->cr46);

	/* RAMDAC regs */
	if (pARK->ramdac == ZOOMDAC) {
		set_daccom(isaIOBase, new->dac_command);
	}

	if (xf86IsPrimaryPci(pARK->PciInfo))
		vgaHWRestore(pScrn, pVga, VGA_SR_ALL);
	else
		vgaHWRestore(pScrn, pVga, VGA_SR_MODE);

	vgaHWProtect(pScrn, FALSE);
}


static Bool ARKEnterVT(ScrnInfoPtr pScrn)
{
	if (!ARKModeInit(pScrn, pScrn->currentMode))
		return FALSE;

	ARKAdjustFrame(pScrn, pScrn->frameX0, pScrn->frameY0);

	return TRUE;
}



static void ARKLeaveVT(ScrnInfoPtr pScrn)
{
	ARKPtr pARK = ARKPTR(pScrn);
	ARKRegPtr old = &pARK->SavedRegs;
	vgaHWPtr hwp = VGAHWPTR(pScrn);

	ARKWriteMode(pScrn, &hwp->ModeReg, old);

	vgaHWUnlock(hwp);
	vgaHWRestore(pScrn, &hwp->SavedReg, VGA_SR_MODE | VGA_SR_FONTS);
	vgaHWLock(hwp);
}


static Bool ARKMapMem(ScrnInfoPtr pScrn)
{
	ARKPtr pARK = ARKPTR(pScrn);
	vgaHWPtr hwp = VGAHWPTR(pScrn);

	/* extended to cover MMIO space at 0xB8000 */
	hwp->MapSize = 0x20000;

#ifndef XSERVER_LIBPCIACCESS
	pARK->MMIOBase = xf86MapDomainMemory(pScrn->scrnIndex, VIDMEM_MMIO,
					     pARK->PciTag, 0xb8000, 0x8000);

	pARK->FBBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER,
				     pARK->PciTag, pARK->PciInfo->memBase[0],
				     pScrn->videoRam * 1024);
#else

	(void) pci_device_map_legacy(pARK->PciInfo, 0xb8000, 0x8000,
				     PCI_DEV_MAP_FLAG_WRITABLE,
				     &pARK->MMIOBase);

	{
		void** result = &pARK->FBBase;
		int err = pci_device_map_range(pARK->PciInfo,
					       pARK->PciInfo->regions[0].base_addr,
					       pScrn->videoRam * 1024,
					       PCI_DEV_MAP_FLAG_WRITABLE |
					       PCI_DEV_MAP_FLAG_WRITE_COMBINE,
					       result);
		
		if (err) {
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "Could not map framebuffer: %d\n", err);
			return FALSE;
		}
	}
#endif

	if (!pARK->FBBase) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "Could not map framebuffer\n");
		return FALSE;
	}

	return TRUE;
}


static void ARKUnmapMem(ScrnInfoPtr pScrn)
{
	ARKPtr pARK = ARKPTR(pScrn);

	/* XXX vgaHWMapMem() isn't called explicitly, so is this correct? */
	vgaHWUnmapMem(pScrn);

#ifndef XSERVER_LIBPCIACCESS
	xf86UnMapVidMem(pScrn->scrnIndex, pARK->FBBase,
			pScrn->videoRam * 1024);
#else
	pci_device_unmap_range(pARK->PciInfo, pARK->FBBase, pScrn->videoRam * 1024);
#endif
}


Bool ARKCloseScreen(ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	ARKPtr pARK = ARKPTR(pScrn);
	vgaHWPtr hwp = VGAHWPTR(pScrn);

	if (pScrn->vtSema) {
		vgaHWUnlock(hwp);
		ARKWriteMode(pScrn, &hwp->SavedReg, &pARK->SavedRegs);
		vgaHWLock(hwp);
		ARKUnmapMem(pScrn);
	}

	pScrn->vtSema = FALSE;
	pScreen->CloseScreen = pARK->CloseScreen;

	/* XXX Shouldn't XAADestroyInfoRec() be called? */

	return (*pScreen->CloseScreen)(pScreen);
}


Bool ARKSaveScreen(ScreenPtr pScreen, int mode)
{
	return vgaHWSaveScreen(pScreen, mode);
}


Bool ARKSwitchMode(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
	return ARKModeInit(pScrn, mode);
}


static void ARKLoadPalette(ScrnInfoPtr pScrn, int numColors,
			   int *indices, LOCO *colors,
			   VisualPtr pVisual)
{
	unsigned long isaIOBase = 0;
	int i, index;

	/* isaIOBase += pScrn->domainIOBase; */
	for (i=0; i<numColors; i++) {
		index = indices[i];
		outb(isaIOBase + 0x3c8, index);
		outb(isaIOBase + 0x3c9, colors[index].red);
		outb(isaIOBase + 0x3c9, colors[index].green);
		outb(isaIOBase + 0x3c9, colors[index].blue);
	}
}


static void ARKFreeScreen(ScrnInfoPtr pScrn)
{
	vgaHWFreeHWRec(pScrn);

	ARKFreeRec(pScrn);
}


static unsigned char get_daccomm(unsigned long isaIOBase)
{
	unsigned char tmp;

	outb(isaIOBase + 0x3c8, 0);
	inb(isaIOBase + 0x3c6);
	inb(isaIOBase + 0x3c6);
	inb(isaIOBase + 0x3c6);
	inb(isaIOBase + 0x3c6);
	tmp = inb(isaIOBase + 0x3c6);
	outb(isaIOBase + 0x3c8, 0);

	return tmp;
}


static unsigned char set_daccom(unsigned long isaIOBase, unsigned char comm)
{
#if 0
	outb(isaIOBase + 0x3c8, 0);
#else
	inb(isaIOBase + 0x3c8);
#endif
	inb(isaIOBase + 0x3c6);
	inb(isaIOBase + 0x3c6);
	inb(isaIOBase + 0x3c6);
	inb(isaIOBase + 0x3c6);
	outb(isaIOBase + 0x3c6, comm);
#if 0
	outb(isaIOBase + 0x3c8, 0);
#else
	inb(isaIOBase + 0x3c8);
#endif

	return inb(isaIOBase + 0x3c6);
}
