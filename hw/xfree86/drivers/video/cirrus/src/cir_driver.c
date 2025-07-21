/*
 * Driver for CL-GD5480.
 * Itai Nahshon.
 *
 * Support for the CL-GD7548: David Monniaux
 *
 * This is mainly a cut & paste from the MGA driver.
 * Original authors and contributors list include:
 *	Radoslaw Kapitan, Andrew Vanderstock, Dirk Hohndel,
 *	David Dawes, Andrew E. Mileski, Leonard N. Zubkoff,
 *	Guy DESBIEF
 */
#include <dix-config.h>

/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"

/* Drivers that need to access the PCI config space directly need this */
#include "xf86Pci.h"

#include "cir.h"
#include "alp.h"
#include "lg.h"

#include "vbe.h"

typedef void *pointer;

/*
 * Forward definitions for the functions that make up the driver.
 */

/* Mandatory functions */

static const OptionInfoRec *	CIRAvailableOptions(int chipid, int busid);
static void	CIRIdentify(int flags);
static Bool	CIRProbe(DriverPtr drv, int flags);

#define CIR_VERSION 4000
#define CIR_NAME "CIRRUS"
#define CIR_DRIVER_NAME "cirrus"
#define CIR_MAJOR_VERSION PACKAGE_VERSION_MAJOR
#define CIR_MINOR_VERSION PACKAGE_VERSION_MINOR
#define CIR_PATCHLEVEL PACKAGE_VERSION_PATCHLEVEL

/*
 * This contains the functions needed by the server after loading the
 * driver module.  It must be supplied, and gets added to the driver list by
 * the Module Setup function in the dynamic case.  In the static case a
 * reference to this is compiled in, and this requires that the name of
 * this DriverRec be an upper-case version of the driver name.
 */

_X_EXPORT DriverRec CIRRUS = {
	CIR_VERSION,
	CIR_DRIVER_NAME,
	CIRIdentify,
	CIRProbe,
	CIRAvailableOptions,
	NULL,
	0
};

/* Supported chipsets */
SymTabRec CIRChipsets[] = {
	{ PCI_CHIP_GD5430,		"CLGD5430" },
	{ PCI_CHIP_GD5434_4,	"CLGD5434-4" },
	{ PCI_CHIP_GD5434_8,	"CLGD5434-8" },
	{ PCI_CHIP_GD5436,		"CLGD5436" },
/*  { PCI_CHIP_GD5440,		"CLGD5440" }, */
	{ PCI_CHIP_GD5446,		"CLGD5446" },
	{ PCI_CHIP_GD5480,		"CLGD5480" },
	{ PCI_CHIP_GD5462,		"CL-GD5462" },
	{ PCI_CHIP_GD5464,		"CL-GD5464" },
	{ PCI_CHIP_GD5464BD,	"CL-GD5464BD" },
	{ PCI_CHIP_GD5465,		"CL-GD5465" },
	{ PCI_CHIP_GD7548,              "CL-GD7548" },
	{ PCI_CHIP_GD7555,              "CL-GD7555" },
	{ PCI_CHIP_GD7556,              "CL-GD7556" },
	{-1,					NULL }
};

/* List of PCI chipset names */
_X_EXPORT PciChipsets CIRPciChipsets[] = {
	{ PCI_CHIP_GD5430,	PCI_CHIP_GD5430,	RES_SHARED_VGA },
	{ PCI_CHIP_GD5434_4,PCI_CHIP_GD5434_4,	RES_SHARED_VGA },
	{ PCI_CHIP_GD5434_8,PCI_CHIP_GD5434_8,	RES_SHARED_VGA },
	{ PCI_CHIP_GD5436,	PCI_CHIP_GD5436,	RES_SHARED_VGA },
/*  { PCI_CHIP_GD5440,	PCI_CHIP_GD5440,	RES_SHARED_VGA }, */
	{ PCI_CHIP_GD5446,	PCI_CHIP_GD5446,	RES_SHARED_VGA },
	{ PCI_CHIP_GD5480,	PCI_CHIP_GD5480,	RES_SHARED_VGA },
	{ PCI_CHIP_GD5462,	PCI_CHIP_GD5462,	RES_SHARED_VGA },
	{ PCI_CHIP_GD5464,	PCI_CHIP_GD5464,	RES_SHARED_VGA },
	{ PCI_CHIP_GD5464BD,PCI_CHIP_GD5464BD,	RES_SHARED_VGA },
	{ PCI_CHIP_GD5465,	PCI_CHIP_GD5465,	RES_SHARED_VGA },
	{ PCI_CHIP_GD7548,	PCI_CHIP_GD7548,	RES_SHARED_VGA },
	{ PCI_CHIP_GD7555,	PCI_CHIP_GD7555,	RES_SHARED_VGA },
	{ PCI_CHIP_GD7556,	PCI_CHIP_GD7556,	RES_SHARED_VGA },
	{ -1,				-1,					RES_UNDEFINED}
};

#ifdef XFree86LOADER

static MODULESETUPPROTO(cirSetup);

static XF86ModuleVersionInfo cirVersRec =
{
	"cirrus",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XORG_VERSION_CURRENT,
	CIR_MAJOR_VERSION, CIR_MINOR_VERSION, CIR_PATCHLEVEL,
	ABI_CLASS_VIDEODRV,			/* This is a video driver */
	ABI_VIDEODRV_VERSION,
	MOD_CLASS_VIDEODRV,
	{0,0,0,0}
};

/*
 * This is the module init data.
 * Its name has to be the driver name followed by ModuleData.
 */
_X_EXPORT XF86ModuleData cirrusModuleData = { &cirVersRec, cirSetup, NULL };

static pointer
cirSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
	static Bool setupDone = FALSE;

	/* This module should be loaded only once, but check to be sure. */

	if (!setupDone) {
		setupDone = TRUE;
		xf86AddDriver(&CIRRUS, module, 0);

		return (pointer)1;
	}
	if (errmaj) *errmaj = LDR_ONCEONLY;
	return NULL;
}

#endif /* XFree86LOADER */

/* Mandatory */
static void
CIRIdentify(int flags)
{
	xf86PrintChipsets(CIR_NAME, "driver for Cirrus chipsets", CIRChipsets);
}

static const OptionInfoRec *
CIRAvailableOptions(int chipid, int busid)
{
	int chip = chipid & 0xffff;

        switch (chip)
	{
	case PCI_CHIP_GD5462:
	case PCI_CHIP_GD5464:
	case PCI_CHIP_GD5464BD:
	case PCI_CHIP_GD5465:
		return LgAvailableOptions(chipid);

	default:
		return AlpAvailableOptions(chipid);
	}
}

static Bool
CIRProbe(DriverPtr drv, int flags)
{
    int i;
    GDevPtr *devSections;
    pciVideoPtr pPci;
    int *usedChips;
    int numDevSections;
    int numUsed;
    Bool foundScreen = FALSE;
    ScrnInfoPtr pScrn;

#ifdef CIR_DEBUG
    ErrorF("CirProbe\n");
#endif
  
    if ((numDevSections = xf86MatchDevice(CIR_DRIVER_NAME,
					  &devSections)) <= 0) {
	return FALSE;
    }

#ifndef XSERVER_LIBPCIACCESS    
    if (xf86GetPciVideoInfo() == NULL) {
	/*
	 * We won't let anything in the config file override finding no
	 * PCI video cards at all.  This seems reasonable now, but we'll see.
	 */
	return FALSE;
    }
#endif
  
    numUsed = xf86MatchPciInstances(CIR_NAME, PCI_VENDOR_CIRRUS,
				    CIRChipsets, CIRPciChipsets, devSections,
 				    numDevSections, drv, &usedChips);
    /* Free it since we don't need that list after this */
    if (numUsed <= 0) {
        free(devSections);
 	return FALSE;
    }
    if (flags & PROBE_DETECT)
 	foundScreen = TRUE;
    else for (i = 0; i < numUsed; i++) {
 	/* The Laguna family of chips is so different from the Alpine
 	   family that we won't share even the highest-level of
 	   functions.  But, the Laguna chips /are/ Cirrus chips, so
 	   they should be handled in this driver (as opposed to their
 	   own driver). */
	pPci = xf86GetPciInfoForEntity(usedChips[i]);

#ifdef XSERVER_LIBPCIACCESS
    if (pci_device_has_kernel_driver(pPci)) {
        xf86DrvMsg(0, X_ERROR,
                   "cirrus: The PCI device 0x%x at %2.2d@%2.2d:%2.2d:%1.1d has a kernel module claiming it.\n",
                   pPci->device_id, pPci->bus, pPci->domain, pPci->dev, pPci->func);
        xf86DrvMsg(0, X_ERROR,
                   "cirrus: This driver cannot operate until it has been unloaded.\n");
	xf86UnclaimPciSlot(pPci, devSections[0]);
        free(devSections);
        return FALSE;
    }
#endif
	pScrn = NULL;
 	if (pPci && (PCI_DEV_DEVICE_ID(pPci) == PCI_CHIP_GD5462 ||
		     PCI_DEV_DEVICE_ID(pPci) == PCI_CHIP_GD5464 ||
		     PCI_DEV_DEVICE_ID(pPci) == PCI_CHIP_GD5464BD ||
		     PCI_DEV_DEVICE_ID(pPci) == PCI_CHIP_GD5465)) {
	    pScrn = LgProbe(usedChips[i]);
 	} else {
 	    pScrn = AlpProbe(usedChips[i]);
 	}
 	
 	if (pScrn) {
 	    foundScreen = TRUE;
 	    /* Fill in what we can of the ScrnInfoRec */
 	    pScrn->driverVersion = CIR_VERSION;
 	    pScrn->driverName	 = CIR_DRIVER_NAME;
 	    pScrn->name		 = CIR_NAME;
 	    pScrn->Probe	 = NULL;
 	}
    }
    free(devSections);
    free(usedChips);
     
    return foundScreen;
}

/*
 * Map the framebuffer and MMIO memory.
 */

_X_EXPORT Bool
CirMapMem(CirPtr pCir, int scrnIndex)
{
#ifdef CIR_DEBUG
	ErrorF("CirMapMem\n");
#endif

	/*
	 * Map the frame buffer.
	 */
	if (pCir->FbMapSize) {

#ifndef XSERVER_LIBPCIACCESS
	    
	    pCir->FbBase = xf86MapPciMem(scrnIndex, VIDMEM_FRAMEBUFFER,
					 pCir->PciTag, pCir->FbAddress,
					 pCir->FbMapSize);
	    if (pCir->FbBase == NULL)
		return FALSE;

#else
	    void** result = (void**)&pCir->FbBase;
	    int err = pci_device_map_range(pCir->PciInfo,
					   pCir->FbAddress,
					   pCir->FbMapSize,
					   PCI_DEV_MAP_FLAG_WRITABLE |
					   PCI_DEV_MAP_FLAG_WRITE_COMBINE,
					   result);

	    if (err) 
	      return FALSE;
#endif
	}
	
#ifdef CIR_DEBUG
	ErrorF("CirMapMem pCir->FbBase=0x%08x\n", pCir->FbBase);
#endif

	/*
	 * Map IO registers to virtual address space
	 */
	if (pCir->IOAddress == 0) {
		pCir->IOBase = NULL; /* Until we are ready to use MMIO */
	} else {

#ifndef XSERVER_LIBPCIACCESS
		/*
		 * For Alpha, we need to map SPARSE memory, since we need
		 * byte/short access.  Common-level will automatically use
		 * sparse mapping for MMIO.
		 */

		pCir->IOBase =
		  xf86MapPciMem(scrnIndex, VIDMEM_MMIO, pCir->PciTag,
		       	        pCir->IOAddress, pCir->IoMapSize);
		if (pCir->IOBase == NULL)
			return FALSE;

#else
		void** result = (void**)&pCir->IOBase;
		int err = pci_device_map_range(pCir->PciInfo,
					       pCir->IOAddress,
					       pCir->IoMapSize,
					       PCI_DEV_MAP_FLAG_WRITABLE,
					       result);
		
		if (err) 
			return FALSE;
		
#endif
	}

#ifdef CIR_DEBUG
	ErrorF("CirMapMem pCir->IOBase=0x%08x [length=%08x] from PCI=%08x\n",
	       pCir->IOBase, pCir->IoMapSize, pCir->IOAddress);
	ErrorF("MMIO[GR31] = %2X\n", (int)
	       ((volatile unsigned char*) pCir->IOBase)[0x40]);
#endif

	return TRUE;
}


/*
 * Unmap the framebuffer and MMIO memory.
 */

_X_EXPORT Bool
CirUnmapMem(CirPtr pCir, int scrnIndex)
{
#ifdef CIR_DEBUG
	ErrorF("CirUnmapMem\n");
#endif

	if (pCir->IOBase != NULL) {
		/*
		 * Unmap IO registers to virtual address space
		 */
#ifndef XSERVER_LIBPCIACCESS
		xf86UnMapVidMem(scrnIndex, (pointer)pCir->IOBase, pCir->IoMapSize);
#else
		pci_device_unmap_range(pCir->PciInfo, (pointer)pCir->IOBase, pCir->IoMapSize);
#endif
		pCir->IOBase = NULL;
	}

#ifndef XSERVER_LIBPCIACCESS
	xf86UnMapVidMem(scrnIndex, (pointer)pCir->FbBase, pCir->FbMapSize);
#else
	pci_device_unmap_range(pCir->PciInfo, (pointer)pCir->FbBase, pCir->FbMapSize);
#endif
	pCir->FbBase = NULL;
	return TRUE;
}

_X_EXPORT void
cirProbeDDC(ScrnInfoPtr pScrn, int index)
{
    vbeInfoPtr pVbe;

    if (xf86LoadSubModule(pScrn, "vbe")) {
        pVbe = VBEInit(NULL,index);
        ConfiguredMonitor = vbeDoEDID(pVbe, NULL);
	vbeFree(pVbe);
    }
}
