/*
 * Driver for CL-GD546x -- The Laguna family
 *
 * lg_driver.c
 *
 * (c) 1998 Corin Anderson.
 *          corina@the4cs.com
 *          Tukwila, WA
 *
 * This driver is derived from the cir_driver.c module.
 * Original authors and contributors list include:
 *      Radoslaw Kapitan, Andrew Vanderstock, Dirk Hohndel,
 *      David Dawes, Andrew E. Mileski, Leonard N. Zubkoff,
 *      Guy DESBIEF, Itai Nahshon.
 */
#include <xorg-config.h>

#define EXPERIMENTAL

/*
 * All drivers should typically include these.
 */
#include "xf86.h"
#include "xf86_OSproc.h"

/*
 * All drivers need this.
 */

#include "compiler.h"

/*
 * Drivers that need to access the PCI config space directly need
 * this.
 */
#include "xf86Pci.h"

/*
 * All drivers using the vgahw module need this.  This driver needs
 * to be modified to not use vgaHW for multihead operation.
 */
#include "vgaHW.h"

/*
 * All drivers initialising the SW cursor need this.
 */
#include "mipointer.h"

/*
 * Need this for inputInfo.
 */
#include "inputstr.h"

#include "micmap.h"

/*
 * Needed by the shadowfb.
 */
#include "shadowfb.h"

#include "xf86int10.h"

#include "fb.h"

#include "xf86DDC.h"

#undef LG_DEBUG

#include "cir.h"
#define _LG_PRIVATE_
#include "lg.h"

#include "xf86xv.h"
#include <X11/extensions/Xv.h>

/*
 * Forward definitions for the functions that make up the driver.
 */

/*
 * Mandatory functions
 */
Bool LgPreInit(ScrnInfoPtr pScrn, int flags);
Bool LgScreenInit(ScreenPtr pScreen, int argc, char **argv);
Bool LgEnterVT(ScrnInfoPtr arg);
void LgLeaveVT(ScrnInfoPtr arg);
static Bool LgCloseScreen(ScreenPtr pScreen);
static Bool LgSaveScreen(ScreenPtr pScreen, Bool mode);

/*
 * Required if the driver supports mode switching.
 */
Bool LgSwitchMode(ScrnInfoPtr arg, DisplayModePtr mode);
/*
 * Required if the driver supports moving the viewport.
 */
void LgAdjustFrame(ScrnInfoPtr arg, int x, int y);

/*
 * Optional functions
 */
void LgFreeScreen(ScrnInfoPtr arg);
ModeStatus LgValidMode(ScrnInfoPtr arg, DisplayModePtr mode,
                        Bool verbose, int flags);

/*
 * Internal functions
 */
static void LgRestoreLgRegs(ScrnInfoPtr pScrn, LgRegPtr lgReg);
static int LgFindLineData(int displayWidth, int bpp);
static CARD16 LgSetClock(CirPtr pCir, vgaHWPtr hwp, int freq);
static void lg_vgaHWSetMmioFunc(vgaHWPtr hwp, CARD8 *base);

static void LgDisplayPowerManagementSet(ScrnInfoPtr pScrn,
                                        int PowerManagementMode,
                                        int flags);

/*
 * This is intentionally screen-independent.  It indicates the binding
 * choice made in the first PreInit.
 */
static int pix24bpp = 0;

/*
 * This contains the functions needed by the server after loading the
 * driver module.  It must be supplied, and gets added the driver list
 * by the Module Setup function in the dynamic case.  In the static
 * case a reference to this is compiled in, and this requires that the
 * name of this DriverRec be an upper-case version of the driver name.
 */

typedef enum {
    OPTION_HW_CURSOR,
    OPTION_PCI_RETRY,
    OPTION_ROTATE,
    OPTION_SHADOW_FB,
    OPTION_NOACCEL
} LgOpts;

static const OptionInfoRec LgOptions[] = {
    { OPTION_HW_CURSOR, "HWcursor", OPTV_BOOLEAN,   {0},    FALSE },
    { OPTION_NOACCEL,   "NoAccel",  OPTV_BOOLEAN,   {0},    FALSE },
    { OPTION_SHADOW_FB, "ShadowFB", OPTV_BOOLEAN,   {0},    FALSE },
    { OPTION_ROTATE,    "Rotate",   OPTV_ANYSTR,    {0},    FALSE },
/*
 * fifo_conservative/aggressive; fast/med/slow_dram; ...
 */
    { -1,               NULL,       OPTV_NONE,      {0},    FALSE }
};

/*
 *                       1/4 bpp  8 bpp 15/16 bpp 24 bpp  32 bpp
 */
static int gd5462_MaxClocks[] =
                        { 170000, 170000, 135100, 135100,  85500 };
static int gd5464_MaxClocks[] =
                        { 170000, 250000, 170000, 170000, 135100 };
static int gd5465_MaxClocks[] =
                        { 170000, 250000, 170000, 170000, 135100 };

/*
 * We're rather use skinny tiles, so put all of them at the head of
 * the table.
 */
LgLineDataRec LgLineData[] = {
    { 5,  640, 0},
    { 8, 1024, 0},
    {10, 1280, 0},
    {13, 1664, 0},
    {16, 2048, 0},
    {20, 2560, 0},
    {10, 2560, 1},
    {26, 3328, 0},
    { 5, 1280, 1},
    { 8, 2048, 1},
    {13, 3328, 1},
    {16, 4096, 1},
    {20, 5120, 1},
    {26, 6656, 1},
/*
 * Sentinel to indicate end of table.
 */
    {-1, -1,   -1}
};

static int LgLinePitches[4][11] = {
/*
 *  8 bpp
 */
    { 640, 1024, 1280, 1664, 2048, 2560, 3328, 4096, 5120, 6656, 0 },
/*
 * 16 bpp
 */
    { 320,  512,  640,  832, 1024, 1280, 1664, 2048, 2560, 3328, 0 },
/*
 * 24 bpp
 */
    { 213,  341,  426,  554,  682,  853, 1109, 1365, 1706, 2218, 0 },
/*
 * 32 bpp
 */
    { 160,  256,  320,  416,  512,  640,  832, 1024, 1280, 1664, 0 }
};

#define LG_MAJOR_VERSION 1
#define LG_MINOR_VERSION 0
#define LG_PATCHLEVEL 0

static XF86ModuleVersionInfo lgVersRec =
{
    "cirrus_laguna",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    LG_MAJOR_VERSION, LG_MINOR_VERSION, LG_PATCHLEVEL,
/*
 * This is a video driver.
 */
    ABI_CLASS_VIDEODRV,
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_NONE,
    {0,0,0,0}
};

/*
 * This is the module init data.
 * Its name has to be the driver name followed by ModuleData.
 */
_X_EXPORT XF86ModuleData cirrus_lagunaModuleData = {
    &lgVersRec,
    NULL,
    NULL
};

_X_EXPORT const OptionInfoRec *
LgAvailableOptions(int chipid) {
    return LgOptions;
}

_X_EXPORT ScrnInfoPtr
LgProbe(int entity)
{
    ScrnInfoPtr pScrn = NULL;
    if ((pScrn = xf86ConfigPciEntity(pScrn, 0, entity,
                                        CIRPciChipsets,
                                        NULL, NULL, NULL,
                                        NULL, NULL))) {
        pScrn->PreInit = LgPreInit;
        pScrn->ScreenInit = LgScreenInit;
        pScrn->SwitchMode = LgSwitchMode;
        pScrn->AdjustFrame = LgAdjustFrame;
        pScrn->EnterVT = LgEnterVT;
        pScrn->LeaveVT = LgLeaveVT;
        pScrn->FreeScreen = LgFreeScreen;
        pScrn->ValidMode = LgValidMode;
    }
    return pScrn;
}

static Bool
LgGetRec(ScrnInfoPtr pScrn)
{
    CirPtr pCir;

    if (pScrn->driverPrivate != NULL)
        return TRUE;

    pScrn->driverPrivate = XNFcallocarray(sizeof(CirRec), 1);
    ((CirPtr) pScrn->driverPrivate)->chip.lg =
                                        XNFcallocarray(sizeof(LgRec), 1);

    /*
     * Initialize it.
     */
    pCir = CIRPTR(pScrn);
    pCir->chip.lg->oldBitmask = 0x00000000;

    return TRUE;
}

static void
LgFreeRec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate == NULL)
        return;
    free(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
}

/*
 * LgCountRAM --
 *
 * Counts amount of installed RAM
 */
/*
 * XXX We need to get rid of this PIO (MArk)
 */
static int
LgCountRam(ScrnInfoPtr pScrn)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CARD8 SR14;

    vgaHWProtect(pScrn, TRUE);

    /*
     * The ROM BIOS scratch pad registers contain, among other things,
     * the amount of installed RDRAM for the Laguna chip.
     */
    SR14 = hwp->readSeq(hwp, 0x14);

    ErrorF("Scratch Pads: 0:%02x 1:%02x 2:%02x 3:%02x\n",
            hwp->readSeq(hwp, 9),
            hwp->readSeq(hwp, 10),
            SR14,
            hwp->readSeq(hwp, 0x15));

    vgaHWProtect(pScrn, FALSE);

    return 1024 * ((SR14 & 0x7) + 1);

    /*
     * !!! This function seems to be incorrect...
     */
}

static xf86MonPtr
LgDoDDC(ScrnInfoPtr pScrn)
{
    CirPtr pCir = CIRPTR(pScrn);
    xf86MonPtr MonInfo = NULL;

    /*
     * Map the CIR memory and MMIO areas.
     */
    if (!CirMapMem(pCir, pScrn->scrnIndex))
        return FALSE;

    if (!LgI2CInit(pScrn)) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                    "I2C initialization failed\n");
        goto unmap_out;
    }

    /*
     * Read and output monitor info using DDC2 over I2C bus.
     */
    MonInfo = xf86DoEDID_DDC2(pScrn, pCir->I2CPtr1);
    if (!MonInfo) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                    "Failed to obtain EDID.\n");
        goto unmap_out;
    }

    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                "I2C Monitor info: %p\n", (void *)MonInfo);
    xf86PrintEDID(MonInfo);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                "end of I2C Monitor info\n\n");

    xf86SetDDCproperties(pScrn, MonInfo);

unmap_out:
    CirUnmapMem(pCir, pScrn->scrnIndex);

    return MonInfo;
}

/*
 * Mandatory
 */
Bool
LgPreInit(ScrnInfoPtr pScrn, int flags)
{
    CirPtr pCir;
    vgaHWPtr hwp;
    MessageType from;
    int i;
    ClockRangePtr clockRanges;
    int fbPCIReg, ioPCIReg;
    const char *s;

    if (flags & PROBE_DETECT) {
        cirProbeDDC(pScrn,
                    xf86GetEntityInfo(pScrn->entityList[0])->index);
        return TRUE;
    }

#ifdef LG_DEBUG
    ErrorF("LgPreInit\n");
#endif

    /*
     * Check the number of entities, and fail if it isn't one.
     */
    if (pScrn->numEntities != 1)
        return FALSE;

    /*
     * The vgahw module should be loaded here when needed.
     */
    if (!xf86LoadSubModule(pScrn, "vgahw"))
        return FALSE;

    /*
     * Allocate a vgaHWRec.
     */
    if (!vgaHWGetHWRec(pScrn))
        return FALSE;

    hwp = VGAHWPTR(pScrn);
    vgaHWSetStdFuncs(hwp);
    vgaHWGetIOBase(hwp);

    /*
     * Allocate the LgRec driverPrivate.
     */
    if (!LgGetRec(pScrn))
        return FALSE;

    pCir = CIRPTR(pScrn);
    pCir->pScrn = pScrn;
    pCir->PIOReg = 0x3CE; /* was hwp->PIOOffset + 0x3CE */

    /*
     * Get the entity, and make sure it is PCI.
     */
    pCir->pEnt = xf86GetEntityInfo(pScrn->entityList[0]);
    if (pCir->pEnt->location.type != BUS_PCI)
        return FALSE;
    pCir->Chipset = pCir->pEnt->chipset;

    /*
     * Find the PCI info for this screen.
     */
    pCir->PciInfo = xf86GetPciInfoForEntity(pCir->pEnt->index);
#ifndef XSERVER_LIBPCIACCESS
    pCir->PciTag = pciTag(PCI_DEV_BUS(pCir->PciInfo),
            PCI_DEV_DEV(pCir->PciInfo), PCI_DEV_FUNC(pCir->PciInfo));
#endif

    if (xf86LoadSubModule(pScrn, "int10")) {
        xf86Int10InfoPtr int10InfoPtr;

        int10InfoPtr = xf86InitInt10(pCir->pEnt->index);

        if (int10InfoPtr)
            xf86FreeInt10(int10InfoPtr);
    }

    /*
     * Set pScrn->monitor.
     */
    pScrn->monitor = pScrn->confScreen->monitor;

    /*
     * The first thing we should figure out is the depth, bpp, etc.
     * We support both 24 bpp and 32 bpp layouts, so indicate that.
     */
    if (!xf86SetDepthBpp(pScrn, 0, 0, 0, Support24bppFb |
                                            Support32bppFb |
                                            SupportConvert32to24 |
                                            PreferConvert32to24)) {
        return FALSE;
    }
    /*
     * Check that the returned depth is one we support.
     */
    switch (pScrn->depth) {
    case 8:
    case 15:
    case 16:
    case 24:
    case 32:
        /*
         * OK
         */
        break;
    default:
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                    "Given depth (%d) is not supported by this "
                    "driver\n", pScrn->depth);
        return FALSE;
    }
    xf86PrintDepthBpp(pScrn);

    /*
     * Get the depth24 pixmap format.
     */
    if (pScrn->depth == 24 && pix24bpp == 0)
        pix24bpp = xf86GetBppFromDepth(pScrn, 24);

    /*
     * This must happen after pScrn->display has been set because
     * xf86SetWeight references it.
     */
    if (pScrn->depth > 8) {
        /*
         * The defaults are OK for us.
         */
        rgb zeros = { 0, 0, 0 };

        /*
         * !!! I think we can force 5-6-5 weight for 16bpp here for
         * the 5462.
         */

        if (!xf86SetWeight(pScrn, zeros, zeros)) {
            return FALSE;
        } else {
            /*
             * XXX Check that weight returned is supported.
             */
            ;
        }
    }

    if (!xf86SetDefaultVisual(pScrn, -1))
        return FALSE;

    /*
     * Collect all of the relevant option flags (fill in
     * pScrn->options).
     */
    xf86CollectOptions(pScrn, NULL);

    /*
     * Process the options.
     */
    if (!(pCir->Options = malloc(sizeof(LgOptions))))
        return FALSE;
    memcpy(pCir->Options, LgOptions, sizeof(LgOptions));
    xf86ProcessOptions(pScrn->scrnIndex,
                        pScrn->options,
                        pCir->Options);

    pScrn->rgbBits = 6;
    from = X_DEFAULT;
    pCir->HWCursor = FALSE;
    if (xf86GetOptValBool(pCir->Options, OPTION_HW_CURSOR,
        &pCir->HWCursor))
        from = X_CONFIG;

    xf86DrvMsg(pScrn->scrnIndex, from, "Using %s cursor\n",
            pCir->HWCursor ? "HW" : "SW");
    if (xf86ReturnOptValBool(pCir->Options, OPTION_NOACCEL, FALSE)) {
        pCir->NoAccel = TRUE;
        xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
                    "Acceleration disabled\n");
    }
    if (pScrn->bitsPerPixel < 8) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                    "Cannot use in less than 8 bpp\n");
        return FALSE;
    }
    /*
     * Set the ChipRev, allowing config file entries to override.
     */
    if (pCir->pEnt->device->chipRev >= 0) {
        pCir->ChipRev = pCir->pEnt->device->chipRev;
        xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
                    "ChipRev override: %d\n", pCir->ChipRev);
    } else {
        pCir->ChipRev = PCI_DEV_REVISION(pCir->PciInfo);
    }

    /*
     * Cirrus Logic swapped the FB and IO registers in the 5465
     * (by design).
     */
    if (PCI_CHIP_GD5465 == pCir->Chipset) {
        fbPCIReg = 0;
        ioPCIReg = 1;
    } else {
        fbPCIReg = 1;
        ioPCIReg = 0;
    }

    /*
     * Find the frame buffer base address.
     */
    if (pCir->pEnt->device->MemBase != 0) {
        /*
         * Require that the config file value matches one of the PCI
         * values.
         */
        if (!xf86CheckPciMemBase(pCir->PciInfo,
            pCir->pEnt->device->MemBase)) {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                        "MemBase 0x%08lX doesn't match any PCI base "
                        "register.\n",
                        pCir->pEnt->device->MemBase);
            return FALSE;
        }
        pCir->FbAddress = pCir->pEnt->device->MemBase;
        from = X_CONFIG;
    } else {
        if (PCI_REGION_BASE(pCir->PciInfo,
                            fbPCIReg,
                            REGION_MEM) != 0) {
            pCir->FbAddress = PCI_REGION_BASE(pCir->PciInfo,
                                                fbPCIReg,
                                                REGION_MEM) &
                                0xff000000;
            from = X_PROBED;
        } else {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                        "No valid FB address in PCI config space\n");
            LgFreeRec(pScrn);
            return FALSE;
        }
    }
    xf86DrvMsg(pScrn->scrnIndex, from,
                "Linear framebuffer at 0x%lX\n",
                (unsigned long) pCir->FbAddress);

    /*
     * Find the MMIO base address.
     */
    if (pCir->pEnt->device->IOBase != 0) {
        /*
         * Require that the config file value matches one of the PCI
         * values.
         */
        if (!xf86CheckPciMemBase(pCir->PciInfo,
                                    pCir->pEnt->device->IOBase)) {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                        "IOBase 0x%08lX doesn't match any PCI base "
                        "register.\n",
                        pCir->pEnt->device->IOBase);
            return FALSE;
        }
        pCir->IOAddress = pCir->pEnt->device->IOBase;
        from = X_CONFIG;
    } else {
        if (PCI_REGION_BASE(pCir->PciInfo,
                            ioPCIReg,
                            REGION_MEM) != 0) {
            pCir->IOAddress = PCI_REGION_BASE(pCir->PciInfo,
                                                ioPCIReg,
                                                REGION_MEM) &
                                0xfffff000;
            from = X_PROBED;
        } else {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                        "No valid MMIO address in PCI config "
                        "space\n");
        }
    }
    xf86DrvMsg(pScrn->scrnIndex, from,
                "MMIO registers at 0x%lX\n",
                (unsigned long) pCir->IOAddress);

    /*
     * If the user has specified the amount of memory in the
     * XF86Config file, we respect that setting.
     */
    if (pCir->pEnt->device->videoRam != 0) {
        pScrn->videoRam = pCir->pEnt->device->videoRam;
        from = X_CONFIG;
    } else {
        pScrn->videoRam = LgCountRam(pScrn);
        from = X_PROBED;
    }
    if (2048 == pScrn->videoRam) {
        /*
         * Two-way interleaving
         */
        pCir->chip.lg->memInterleave = 0x40;
    } else if (4096 == pScrn->videoRam || 8192 == pScrn->videoRam) {
        /*
         * Four-way interleaving
         */
        pCir->chip.lg->memInterleave = 0x80;
    } else {
        /*
         * One-way interleaving
         */
        pCir->chip.lg->memInterleave = 0x00;
    }

    xf86DrvMsg(pScrn->scrnIndex, from,
                "VideoRAM: %d kByte\n", pScrn->videoRam);

    pCir->FbMapSize = pScrn->videoRam * 1024;
    /*
     * 16K for moment, will increase.
     */
    pCir->IoMapSize = 0x4000;

#ifndef XSERVER_LIBPCIACCESS
    pScrn->racIoFlags = RAC_COLORMAP
#ifndef EXPERIMENTAL
            | RAC_VIEWPORT
#endif
            ;
    xf86SetOperatingState(resVgaMem, pCir->pEnt->index, ResUnusedOpr);

    /*
     * Register the PCI-assigned resources.
     */
    if (xf86RegisterResources(pCir->pEnt->index, NULL, ResExclusive)) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                    "xf86RegisterResources() found resource "
                    "conflicts\n");
        return FALSE;
    }
#endif
    if (!xf86LoadSubModule(pScrn, "ddc")) {
        LgFreeRec(pScrn);
        return FALSE;
    }

#if LGuseI2C
    if (!xf86LoadSubModule(pScrn, "i2c")) {
        LgFreeRec(pScrn);
        return FALSE;
    }
#endif

    /*
     * Read and print the monitor DDC information.
     */
    pScrn->monitor->DDC = LgDoDDC(pScrn);

    /*
     * The gamma fields must be initialised when using the new cmap
     * code.
     */
    if (pScrn->depth > 1) {
        Gamma zeros = { 0.0, 0.0, 0.0 };

        if (!xf86SetGamma(pScrn, zeros))
            return FALSE;
    }
    if (xf86GetOptValBool(pCir->Options,
                            OPTION_SHADOW_FB,
                            &pCir->shadowFB))
        xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
                    "ShadowFB %s.\n",
                    pCir->shadowFB ? "enabled" : "disabled");

    if ((s = xf86GetOptValString(pCir->Options, OPTION_ROTATE))) {
        if (!xf86NameCmp(s, "CW")) {
            /*
             * Acceleration is disabled below for shadowfb.
             */
            pCir->shadowFB = TRUE;
            pCir->rotate = 1;
            xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
                        "Rotating screen clockwise - "
                        "acceleration disabled\n");
        } else if (!xf86NameCmp(s, "CCW")) {
            pCir->shadowFB = TRUE;
            pCir->rotate = -1;
            xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
                        "Rotating screen counter clockwise - "
                        "acceleration disabled\n");
        } else {
            xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
                        "\"%s\" is not a valid value for Option "
                        "\"Rotate\"\n", s);
            xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                        "Valid options are \"CW\" or \"CCW\"\n");
        }
    }

    if (pCir->shadowFB && !pCir->NoAccel) {
        xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
                    "HW acceleration not supported with "
                    "\"shadowFB\".\n");
        pCir->NoAccel = TRUE;
    }

    if (pCir->rotate && pCir->HWCursor) {
        xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
                    "HW cursor not supported with \"rotate\".\n");
        pCir->HWCursor = FALSE;
    }

    /*
     * We use a programmable clock.
     */
    pScrn->progClock = TRUE;

    /*
     * XXX Set HW cursor use.
     */

    /*
     * Set the min pixel clock.
     */
    /*
     * XXX Guess, need to check this.
     */
    pCir->MinClock = 12000;
    xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT,
                "Min pixel clock is %d MHz\n",
                pCir->MinClock / 1000);
    /*
     * If the user has specified RAMDAC speed in the XF86Config
     * file, we respect that setting.
     */
    if (pCir->pEnt->device->dacSpeeds[0]) {
        ErrorF("Do not specify a Clocks line for Cirrus chips\n");
        return FALSE;
    } else {
        int speed;
        int *p;
        switch (pCir->Chipset) {
        case PCI_CHIP_GD5462:
            p = gd5462_MaxClocks;
            break;
        case PCI_CHIP_GD5464:
        case PCI_CHIP_GD5464BD:
            p = gd5464_MaxClocks;
            break;
        case PCI_CHIP_GD5465:
            p = gd5465_MaxClocks;
            break;
        default:
            ErrorF("???\n");
            return FALSE;
        }
        switch (pScrn->bitsPerPixel) {
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
            /*
             * Should not get here.
             */
            speed = 0;
            break;
        }
        pCir->MaxClock = speed;
        from = X_PROBED;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Max pixel clock is %d MHz\n",
            pCir->MaxClock / 1000);

    /*
     * Setup the ClockRanges, which describe what clock ranges are
     * available, and what sort of modes they can be used for.
     */
    clockRanges = XNFcallocarray(sizeof(ClockRange), 1);
    clockRanges->next = NULL;
    clockRanges->minClock = pCir->MinClock;
    clockRanges->maxClock = pCir->MaxClock;
    /*
     * programmable
     */
    clockRanges->clockIndex = -1;
    /*
     * XXX Check this.
     */
    clockRanges->interlaceAllowed = FALSE;
    /*
     * XXX Check this.
     */
    clockRanges->doubleScanAllowed = FALSE;
    /*
     * XXX Check this.
     */
    clockRanges->doubleScanAllowed = FALSE;
    /*
     * XXX Check this.
     */
    clockRanges->doubleScanAllowed = FALSE;
    clockRanges->ClockMulFactor = 1;
    clockRanges->ClockDivFactor = 1;
    clockRanges->PrivFlags = 0;

    /*
     * Depending upon what sized tiles used, either 128 or 256.
     */
    /*
     * Aw, heck.  Just say 128.
     */
    pCir->Rounding = 128 >> pCir->BppShift;

    /*
     * xf86ValidateModes will check that the mode HTotal and VTotal
     * values don't exceed the chipset's limit if pScrn->maxHValue
     * and pScrn->maxVValue are set.  Since our CIRValidMode()
     * already takes care of this, we don't worry about setting them
     * here.
     */

    i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
                            pScrn->display->modes, clockRanges,
                            LgLinePitches[pScrn->bitsPerPixel / 8 - 1],
                            0, 0,
                            128 * 8, 0,
                            /*
                             * Any virtual height is allowed.
                             */
                            0,
                            pScrn->display->virtualX,
                            pScrn->display->virtualY,
                            pCir->FbMapSize, LOOKUP_BEST_REFRESH);

    pCir->chip.lg->lineDataIndex =
                                LgFindLineData(pScrn->displayWidth,
                                                pScrn->bitsPerPixel);

    if (i == -1) {
        LgFreeRec(pScrn);
        return FALSE;
    }

    /*
     * Prune the modes marked as invalid.
     */
    xf86PruneDriverModes(pScrn);

    if (i == 0 || pScrn->modes == NULL) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                    "No valid modes found\n");
        LgFreeRec(pScrn);
        return FALSE;
    }

    /*
     * Set the CRTC parameters for all of the modes based on the type
     * of mode, and the chipset's interlace requirements.  Calling
     * this is required if the mode->Crtc* values are used by the
     * driver and if the driver doesn't provide code to set
     * them.  They are not pre-initialised at all.
     */
    xf86SetCrtcForModes(pScrn, INTERLACE_HALVE_V);

    /*
     * Set the current mode to the first in the list.
     */
    pScrn->currentMode = pScrn->modes;

    /*
     * Print the list of modes being used.
     */
    xf86PrintModes(pScrn);

    /*
     * Set display resolution.
     */
    xf86SetDpi(pScrn, 0, 0);

    /*
     * Load fb module.
     */
    if (xf86LoadSubModule(pScrn, "fb") == NULL) {
	LgFreeRec(pScrn);
	return FALSE;
    }

    /*
     * Use shadowfb for acceleration.
     */
    if (!pCir->NoAccel) {
	xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		   "Falling back to shadowfb\n");
	pCir->NoAccel = TRUE;
	pCir->shadowFB = TRUE;
    }

    /*
     * Load RAMDAC if needed.
     */
    if (pCir->HWCursor) {
        if (!xf86LoadSubModule(pScrn, "ramdac")) {
            LgFreeRec(pScrn);
            return FALSE;
        }
    }

    if (pCir->shadowFB) {
        if (!xf86LoadSubModule(pScrn, "shadowfb")) {
            LgFreeRec(pScrn);
            return FALSE;
        }
    }

    return TRUE;
}

/*
 * This function saves the video state.
 */
static void
LgSave(ScrnInfoPtr pScrn)
{
    CirPtr pCir = CIRPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);

#ifdef LG_DEBUG
    ErrorF("LgSave\n");
#endif

    vgaHWSave(pScrn, &VGAHWPTR(pScrn)->SavedReg, VGA_SR_ALL);

    pCir->chip.lg->ModeReg.ExtVga[CR1A] =
    pCir->chip.lg->SavedReg.ExtVga[CR1A] = hwp->readCrtc(hwp, 0x1A);
    pCir->chip.lg->ModeReg.ExtVga[CR1B] =
    pCir->chip.lg->SavedReg.ExtVga[CR1B] = hwp->readCrtc(hwp, 0x1B);
    pCir->chip.lg->ModeReg.ExtVga[CR1D] =
    pCir->chip.lg->SavedReg.ExtVga[CR1D] = hwp->readCrtc(hwp, 0x1D);
    pCir->chip.lg->ModeReg.ExtVga[CR1E] =
    pCir->chip.lg->SavedReg.ExtVga[CR1E] = hwp->readCrtc(hwp, 0x1E);
    pCir->chip.lg->ModeReg.ExtVga[SR07] =
    pCir->chip.lg->SavedReg.ExtVga[SR07] = hwp->readSeq(hwp, 0x07);
    pCir->chip.lg->ModeReg.ExtVga[SR0E] =
    pCir->chip.lg->SavedReg.ExtVga[SR0E] = hwp->readSeq(hwp, 0x0E);
    pCir->chip.lg->ModeReg.ExtVga[SR1E] =
    pCir->chip.lg->SavedReg.ExtVga[SR1E] = hwp->readSeq(hwp, 0x1E);

    pCir->chip.lg->ModeReg.FORMAT =
    pCir->chip.lg->SavedReg.FORMAT = memrw(0xC0);

    pCir->chip.lg->ModeReg.VSC =
    pCir->chip.lg->SavedReg.VSC = memrl(0x3FC);

    pCir->chip.lg->ModeReg.DTTC =
    pCir->chip.lg->SavedReg.DTTC = memrw(0xEA);

    if (pCir->Chipset == PCI_CHIP_GD5465) {
        pCir->chip.lg->ModeReg.TileCtrl =
        pCir->chip.lg->SavedReg.TileCtrl = memrw(0x2C4);
    }

    pCir->chip.lg->ModeReg.TILE =
    pCir->chip.lg->SavedReg.TILE = memrb(0x407);

    if (pCir->Chipset == PCI_CHIP_GD5465)
        pCir->chip.lg->ModeReg.BCLK =
        pCir->chip.lg->SavedReg.BCLK = memrb(0x2C0);
    else
        pCir->chip.lg->ModeReg.BCLK =
        pCir->chip.lg->SavedReg.BCLK = memrb(0x8C);

    pCir->chip.lg->ModeReg.CONTROL =
    pCir->chip.lg->SavedReg.CONTROL = memrw(0x402);

    pCir->chip.lg->ModeReg.RIFCtrl =
    pCir->chip.lg->SavedReg.RIFCtrl = memrw(0x200);

    pCir->chip.lg->ModeReg.RACCtrl =
    pCir->chip.lg->SavedReg.RACCtrl = memrw(0x202);
}

/*
 * Initialise a new mode.  This is currently still using the old
 * "initialise struct, restore/write struct to HW" model.  That could
 * be changed.
 */
static Bool
LgModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    vgaHWPtr hwp;
    CirPtr pCir;
    int width;
    Bool VDiv2 = FALSE;
    CARD16 clockData;
    LgLineDataPtr lineData;

#ifdef LG_DEBUG
    ErrorF("LgModeInit %d bpp,   %d   %d %d %d %d   %d %d %d %d\n",
            pScrn->bitsPerPixel, mode->Clock,
            mode->HDisplay, mode->HSyncStart,
            mode->HSyncEnd, mode->HTotal,
            mode->VDisplay, mode->VSyncStart,
            mode->VSyncEnd, mode->VTotal);

    ErrorF("LgModeInit: depth %d bits\n", pScrn->depth);
#endif

    pCir = CIRPTR(pScrn);
    hwp = VGAHWPTR(pScrn);
    vgaHWUnlock(hwp);

    if (mode->VTotal >= 1024 && !(mode->Flags & V_INTERLACE)) {
        /*
         * For non-interlaced vertical timing >= 1024, the vertical
         * timings are divided by 2 and VGA CRTC 0x17 bit 2 is set.
         */
        if (!mode->CrtcVAdjusted) {
            mode->CrtcVDisplay >>= 1;
            mode->CrtcVSyncStart >>= 1;
            mode->CrtcVSyncEnd >>= 1;
            mode->CrtcVTotal >>= 1;
            mode->CrtcVAdjusted = TRUE;
        }
        VDiv2 = TRUE;
    }

    /*
     * Initialise the ModeReg values.
     */
    if (!vgaHWInit(pScrn, mode))
        return FALSE;
    pScrn->vtSema = TRUE;

    if (VDiv2)
        hwp->ModeReg.CRTC[0x17] |= 0x04;

#ifdef LG_DEBUG
    ErrorF("SynthClock = %d\n", mode->SynthClock);
#endif
    hwp->IOBase = 0x3D0;
    hwp->ModeReg.MiscOutReg |= 0x01;
/*
 * Mono address
 */
#if 0
    hwp->IOBase = 0x3B0;
    hwp->ModeReg.MiscOutReg &= ~0x01;
#endif

    /*
     * ??? Should these be both ...End or ...Start, not one of each?
     */
    pCir->chip.lg->ModeReg.ExtVga[CR1A] =
                        (((mode->CrtcVSyncStart + 1) & 0x300) >> 2) |
                        (((mode->CrtcHSyncEnd >> 3) & 0xC0) >> 2);

    width = pScrn->displayWidth * pScrn->bitsPerPixel / 8;
    if (pScrn->bitsPerPixel == 1)
        width <<= 2;
    hwp->ModeReg.CRTC[0x13] = (width + 7) >> 3;
    /*
     * Offset extension (see CR13)
     */
    pCir->chip.lg->ModeReg.ExtVga[CR1B] &= 0xEF;
    pCir->chip.lg->ModeReg.ExtVga[CR1B] |=
            (((width + 7) >> 3) & 0x100) ? 0x10 : 0x00;
    pCir->chip.lg->ModeReg.ExtVga[CR1B] |= 0x22;
    pCir->chip.lg->ModeReg.ExtVga[CR1D] =
            (((width + 7) >> 3) & 0x200) ? 0x01 : 0x00;

    /*
     * Set the 28th bit to enable extended modes.
     */
    pCir->chip.lg->ModeReg.VSC = 0x10000000;

    /*
     * Overflow register (sure are a lot of overflow bits around...)
     */
    pCir->chip.lg->ModeReg.ExtVga[CR1E] = 0x00;
    pCir->chip.lg->ModeReg.ExtVga[CR1E] |= (
            (mode->CrtcHTotal >> 3 & 0x0100) ? 1 : 0) << 7;
    pCir->chip.lg->ModeReg.ExtVga[CR1E] |= (
            (mode->CrtcHDisplay >> 3 & 0x0100) ? 1 : 0) << 6;
    pCir->chip.lg->ModeReg.ExtVga[CR1E] |= (
            (mode->CrtcHSyncStart >> 3 & 0x0100) ? 1 : 0) << 5;
    pCir->chip.lg->ModeReg.ExtVga[CR1E] |= (
            (mode->CrtcHSyncStart >> 3 & 0x0100) ? 1 : 0) << 4;
    pCir->chip.lg->ModeReg.ExtVga[CR1E] |=
            ((mode->CrtcVTotal & 0x0400) ? 1 : 0) << 3;
    pCir->chip.lg->ModeReg.ExtVga[CR1E] |= (
            (mode->CrtcVDisplay & 0x0400) ? 1 : 0) << 2;
    pCir->chip.lg->ModeReg.ExtVga[CR1E] |= (
            (mode->CrtcVSyncStart & 0x0400) ? 1 : 0) << 1;
    pCir->chip.lg->ModeReg.ExtVga[CR1E] |= (
            (mode->CrtcVSyncStart & 0x0400) ? 1 : 0) << 0;

    lineData = &LgLineData[pCir->chip.lg->lineDataIndex];

    pCir->chip.lg->ModeReg.TILE = lineData->tilesPerLine & 0x3F;

    if (8 == pScrn->bitsPerPixel) {
        pCir->chip.lg->ModeReg.FORMAT = 0x0000;

        pCir->chip.lg->ModeReg.DTTC =
                (pCir->chip.lg->ModeReg.TILE << 8) |
                0x0080 |
                (lineData->width << 6);
        pCir->chip.lg->ModeReg.CONTROL = 0x0000 |
                                            (lineData->width << 11);

        /*
         * There is an optimal FIFO threshold value (lower
         * 5 bits of DTTC) for every resolution and color depth
         * combination.  We'll hit the highlights here, and get
         * close for anything that's not covered.
         */
        if (mode->CrtcHDisplay <= 640) {
            /*
             * BAD numbers: 0x1E
             * GOOD numbers: 0x14
             */
            pCir->chip.lg->ModeReg.DTTC =
                            (pCir->chip.lg->ModeReg.DTTC & 0xFFE0) |
                            (0x0014);
        } else if (mode->CrtcHDisplay <= 800) {
            /*
             * BAD numbers: 0x16
             * GOOD numbers: 0x13 0x14
             */
            pCir->chip.lg->ModeReg.DTTC =
                            (pCir->chip.lg->ModeReg.DTTC & 0xFFE0) |
                            (0x0014);
        } else if (mode->CrtcHDisplay <= 1024) {
            /*
             * BAD numbers:
             * GOOD numbers: 0x15
             */
            pCir->chip.lg->ModeReg.DTTC =
                            (pCir->chip.lg->ModeReg.DTTC & 0xFFE0) |
                            (0x0015);
        } else if (mode->CrtcHDisplay <= 1280) {
            /*
             * BAD numbers:
             * GOOD numbers: 0x16
             */
            pCir->chip.lg->ModeReg.DTTC =
                            (pCir->chip.lg->ModeReg.DTTC & 0xFFE0) |
                            (0x0016);
        } else {
            /*
             * BAD numbers:
             * GOOD numbers:
             */
            pCir->chip.lg->ModeReg.DTTC =
                            (pCir->chip.lg->ModeReg.DTTC & 0xFFE0) |
                            (0x0017);
        }
    } else if (16 == pScrn->bitsPerPixel) {
        /*
         * !!! Assume 5-6-5 RGB mode (for now...).
         */
        pCir->chip.lg->ModeReg.FORMAT = 0x1400;

        if (pScrn->depth == 15)
            pCir->chip.lg->ModeReg.FORMAT = 0x1600;

        pCir->chip.lg->ModeReg.DTTC =
                                (pCir->chip.lg->ModeReg.TILE << 8) |
                                0x0080 |
                                (lineData->width << 6);
        pCir->chip.lg->ModeReg.CONTROL = 0x2000 |
                                            (lineData->width << 11);

        if (mode->CrtcHDisplay <= 640) {
            /*
             * BAD numbers: 0x12
             * GOOD numbers: 0x10
             */
            pCir->chip.lg->ModeReg.DTTC =
                    (pCir->chip.lg->ModeReg.DTTC & 0xFFE0) |
                    (0x0010);
        } else if (mode->CrtcHDisplay <= 800) {
            /*
             * BAD numbers: 0x13
             * GOOD numbers: 0x11
             */
            pCir->chip.lg->ModeReg.DTTC =
                            (pCir->chip.lg->ModeReg.DTTC & 0xFFE0) |
                            (0x0011);
        } else if (mode->CrtcHDisplay <= 1024) {
            /*
             * BAD numbers: 0x14
             * GOOD numbers: 0x12
             */
            pCir->chip.lg->ModeReg.DTTC =
                            (pCir->chip.lg->ModeReg.DTTC & 0xFFE0) |
                            (0x0012);
        } else if (mode->CrtcHDisplay <= 1280) {
            /*
             * BAD numbers: 0x08 0x10
             * Borderline numbers: 0x12
             * GOOD numbers: 0x15
             */
            pCir->chip.lg->ModeReg.DTTC =
                            (pCir->chip.lg->ModeReg.DTTC & 0xFFE0) |
                            (0x0015);
        } else {
            pCir->chip.lg->ModeReg.DTTC =
                            (pCir->chip.lg->ModeReg.DTTC & 0xFFE0) |
                            (0x0017);
        }
    } else if (24 == pScrn->bitsPerPixel) {
        pCir->chip.lg->ModeReg.FORMAT = 0x2400;

        pCir->chip.lg->ModeReg.DTTC =
                                (pCir->chip.lg->ModeReg.TILE << 8) |
                                0x0080 |
                                (lineData->width << 6);
        pCir->chip.lg->ModeReg.CONTROL = 0x4000 |
                                            (lineData->width << 11);

        if (mode->CrtcHDisplay <= 640) {
            /*
             * BAD numbers:
             * GOOD numbers: 0x10
             */
            pCir->chip.lg->ModeReg.DTTC =
                            (pCir->chip.lg->ModeReg.DTTC & 0xFFE0) |
                            (0x0010);
        } else if (mode->CrtcHDisplay <= 800) {
            /*
             * BAD numbers:
             * GOOD numbers: 0x11
             */
            pCir->chip.lg->ModeReg.DTTC =
                            (pCir->chip.lg->ModeReg.DTTC & 0xFFE0) |
                            (0x0011);
        } else if (mode->CrtcHDisplay <= 1024) {
            /*
             * BAD numbers: 0x12 0x13
             * Borderline numbers: 0x15
             * GOOD numbers: 0x17
             */
            pCir->chip.lg->ModeReg.DTTC =
                            (pCir->chip.lg->ModeReg.DTTC & 0xFFE0) |
                            (0x0017);
        } else if (mode->CrtcHDisplay <= 1280) {
            /*
             * BAD numbers:
             * GOOD numbers: 0x1E
             */
            pCir->chip.lg->ModeReg.DTTC =
                            (pCir->chip.lg->ModeReg.DTTC & 0xFFE0) |
                            (0x001E);
        } else {
            /*
             * BAD numbers:
             * GOOD numbers:
             */
            pCir->chip.lg->ModeReg.DTTC =
                            (pCir->chip.lg->ModeReg.DTTC & 0xFFE0) |
                            (0x0020);
        }
    } else if (32 == pScrn->bitsPerPixel) {
        pCir->chip.lg->ModeReg.FORMAT = 0x3400;

        pCir->chip.lg->ModeReg.DTTC =
                                (pCir->chip.lg->ModeReg.TILE << 8) |
                                0x0080 |
                                (lineData->width << 6);
        pCir->chip.lg->ModeReg.CONTROL = 0x6000 |
                                            (lineData->width << 11);

        if (mode->CrtcHDisplay <= 640) {
            /*
             * GOOD numbers: 0x0E
             * BAD numbers:
             */
            pCir->chip.lg->ModeReg.DTTC =
                            (pCir->chip.lg->ModeReg.DTTC & 0xFFE0) |
                            (0x000E);
        } else if (mode->CrtcHDisplay <= 800) {
            /*
             * GOOD numbers: 0x17
             * BAD numbers:
             */
            pCir->chip.lg->ModeReg.DTTC =
                            (pCir->chip.lg->ModeReg.DTTC & 0xFFE0) |
                            (0x0017);
        } else if (mode->CrtcHDisplay <= 1024) {
            /*
             * GOOD numbers: 0x1D
             * OKAY numbers: 0x15 0x14 0x16 0x18 0x19
             * BAD numbers: 0x0E 0x12 0x13 0x0D
             */
            pCir->chip.lg->ModeReg.DTTC =
                            (pCir->chip.lg->ModeReg.DTTC & 0xFFE0) |
                            (0x001D);
        } else if (mode->CrtcHDisplay <= 1280) {
            /*
             * GOOD numbers:
             * BAD numbers:
             */
            /*
             * 10
             */
            pCir->chip.lg->ModeReg.DTTC =
                            (pCir->chip.lg->ModeReg.DTTC & 0xFFE0) |
                            (0x0022);
        } else {
            pCir->chip.lg->ModeReg.DTTC =
                            (pCir->chip.lg->ModeReg.DTTC & 0xFFE0) |
                            (0x0024);
        }
    } else {
        /*
         * ??? What could it be?  Use some sane numbers.
         */
    }

    /*
     * Setup the appropriate memory interleaving.
     */
    pCir->chip.lg->ModeReg.DTTC |= (pCir->chip.lg->memInterleave << 8);
    pCir->chip.lg->ModeReg.TILE |= pCir->chip.lg->memInterleave & 0xC0;

    if (PCI_CHIP_GD5465 == pCir->Chipset) {
        /*
         * The tile control information in the DTTC is also mirrored
         * elsewhere.
         */
        pCir->chip.lg->ModeReg.TileCtrl =
                                pCir->chip.lg->ModeReg.DTTC & 0xFFC0;

        /*
         * The 5465's DTTC records _fetches_ per line, not tiles per
         * line.  Fetches are 128-byte fetches.
         */
        if (pCir->chip.lg->ModeReg.DTTC & 0x0040) {
            /*
             * Using 256-byte wide tiles.  Double the fetches per
             * line field.
             */
            pCir->chip.lg->ModeReg.DTTC =
                        (pCir->chip.lg->ModeReg.DTTC & 0xC0FF) |
                        ((pCir->chip.lg->ModeReg.DTTC & 0x3F00) << 1);
        }
    }

    /*
     * Programme the registers.
     */
    vgaHWProtect(pScrn, TRUE);
    hwp->writeMiscOut(hwp, hwp->ModeReg.MiscOutReg);

    clockData = LgSetClock(pCir, hwp, mode->SynthClock);
    pCir->chip.lg->ModeReg.ExtVga[SR0E] = (clockData >> 8) & 0xFF;
    pCir->chip.lg->ModeReg.ExtVga[SR1E] = clockData & 0xFF;

    /*
     * Write those registers out to the card.
     */
    LgRestoreLgRegs(pScrn, &pCir->chip.lg->ModeReg);

    /*
     * Programme the registers.
     */
    vgaHWRestore(pScrn, &hwp->ModeReg, VGA_SR_MODE | VGA_SR_CMAP);

    vgaHWProtect(pScrn, FALSE);

    return TRUE;
}

static int
LgFindLineData(int displayWidth, int bpp)
{
    /*
     * Find the smallest tile-line-pitch such that the total byte
     * pitch is greater than or equal to displayWidth * Bpp.
     */
    int i;

    /*
     * Some pitch sizes are duplicates in the table.  BUT, the
     * invariant is that the _first_ time a pitch occurs in the table
     * is always _before_ all other pitches greater than it.  Said in
     * another way... if all duplicate entries from the table were
     * removed, then the resulting pitch values are strictly
     * increasing.
     */

    for (i = 0; LgLineData[i].pitch > 0; i++)
        if (LgLineData[i].pitch >= displayWidth * bpp >> 3)
            return i;

    /*
     * Um, uh oh!
     */
    return -1;
}

static void
LgRestoreLgRegs(ScrnInfoPtr pScrn, LgRegPtr lgReg)
{
    CirPtr pCir;
    vgaHWPtr hwp;
    CARD8 cr1D;

    pCir = CIRPTR(pScrn);

    /*
     * First, VGAish registers.
     */
    hwp = VGAHWPTR(pScrn);
    hwp->writeCrtc(hwp, 0x1A, lgReg->ExtVga[CR1A]);
    hwp->writeCrtc(hwp, 0x1B, lgReg->ExtVga[CR1B]);
    cr1D = (hwp->readCrtc(hwp, 0x1D) & ~1) |
            (lgReg->ExtVga[CR1D] & 0x01);
    hwp->writeCrtc(hwp, 0x1D, cr1D);
    hwp->writeCrtc(hwp, 0x1E, lgReg->ExtVga[CR1E]);

    hwp->writeSeq(hwp, 0x07, lgReg->ExtVga[SR07]);
    hwp->writeSeq(hwp, 0x0E, lgReg->ExtVga[SR0E]);
    hwp->writeSeq(hwp, 0x1E, lgReg->ExtVga[SR1E]);
    memww(0xC0, lgReg->FORMAT);

    /*
     * Vendor Specific Control is touchy.  Only bit 28 is of concern.
     */
    memwl(0x3FC, ((memrl(0x3FC) & ~(1 << 28)) |
                    (lgReg->VSC & (1 << 28))));

    memww(0xEA, lgReg->DTTC);

    if (pCir->Chipset == PCI_CHIP_GD5465) {
        memww(0x2C4, lgReg->TileCtrl);
    }

    memwb(0x407, lgReg->TILE);

    if (pCir->Chipset == PCI_CHIP_GD5465)
        memwb(0x2C0, lgReg->BCLK);
    else
        memwb(0x8C, lgReg->BCLK);

    memww(0x402, lgReg->CONTROL);

    memww(0x200, lgReg->RIFCtrl);
    memww(0x202, lgReg->RACCtrl);
}

/*
 * Restore the initial (text) mode.
 */
static void
LgRestore(ScrnInfoPtr pScrn)
{
    vgaHWPtr hwp;
    vgaRegPtr vgaReg;
    CirPtr pCir;
    LgRegPtr lgReg;

#ifdef LG_DEBUG
    ErrorF("LgRestore  pScrn = %p\n", (void *) pScrn);
#endif

    pCir = CIRPTR(pScrn);
    hwp = VGAHWPTR(pScrn);
    vgaReg = &hwp->SavedReg;
    lgReg = &pCir->chip.lg->SavedReg;

    vgaHWProtect(pScrn, TRUE);

    LgRestoreLgRegs(pScrn, lgReg);

    vgaHWRestore(pScrn, vgaReg, VGA_SR_ALL);
    vgaHWProtect(pScrn, FALSE);
}

/*
 * This gets called at the start of each server generation
 */
/*
 * Mandatory
 */
Bool
LgScreenInit(ScreenPtr pScreen, int argc, char **argv)
{
    /*
     * The vgaHW references will disappear one day
     */
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    vgaHWPtr hwp;
    CirPtr pCir;
    int i, ret;
    VisualPtr visual;
    int displayWidth, width, height;
    unsigned char * FbBase = NULL;

#ifdef LG_DEBUG
    ErrorF("LgScreenInit\n");
#endif

    hwp = VGAHWPTR(pScrn);

    /*
     * Standard 64k VGA window
     */
    hwp->MapSize = 0x10000;

    pCir = CIRPTR(pScrn);

    /*
     * Map the VGA memory and get the VGA IO base.
     */
    if (!vgaHWMapMem(pScrn))
        return FALSE;

    /*
     * Map the CIR memory and MMIO areas.
     */
    if (!CirMapMem(pCir, pScrn->scrnIndex))
        return FALSE;
#ifdef EXPERIMENTAL
    lg_vgaHWSetMmioFunc(hwp, pCir->IOBase);
#endif
    vgaHWGetIOBase(hwp);

    /*
     * Save the current state.
     */
    LgSave(pScrn);

    /*
     * Initialise the first mode.
     */
    if (!LgModeInit(pScrn, pScrn->currentMode))
        return FALSE;

    /*
     * Make things beautiful.
     */
    LgSaveScreen(pScreen, SCREEN_SAVER_ON);

    /*
     * Set the viewport.
     */
    LgAdjustFrame(pScrn, pScrn->frameX0, pScrn->frameY0);

    /*
     * The next step is to setup the screen's visuals, and initialise
     * the framebuffer code.  In cases where the framebuffer's default
     * choices for things like visual layouts and bits per RGB are OK,
     * this may be as simple as calling the framebuffer's ScreenInit()
     * function.  If not, the visuals will need to be setup before
     * calling a fb ScreenInit() function and fixed up after.
     */

    /*
     * Reset the visual list.
     */
    miClearVisualTypes();

    /*
     * Setup the visuals we support.
     */
    if (!miSetVisualTypes(pScrn->depth,
                            miGetDefaultVisualMask(pScrn->depth),
                            pScrn->rgbBits,
                            pScrn->defaultVisual))
        return FALSE;

    miSetPixmapDepths();

#ifdef LG_DEBUG
    ErrorF("LgScreenInit after miSetVisualTypes\n");
#endif
    displayWidth = pScrn->displayWidth;
    if (pCir->rotate) {
        height = pScrn->virtualX;
        width = pScrn->virtualY;
    } else {
        width = pScrn->virtualX;
        height = pScrn->virtualY;
    }

    if (pCir->shadowFB) {
        pCir->ShadowPitch = BitmapBytePad(pScrn->bitsPerPixel * width);
        pCir->ShadowPtr = malloc(pCir->ShadowPitch * height);
        displayWidth = pCir->ShadowPitch / (pScrn->bitsPerPixel >> 3);
        FbBase = pCir->ShadowPtr;
    } else {
        pCir->ShadowPtr = NULL;
        FbBase = pCir->FbBase;
    }

    /*
     * Call the framebuffer layer's ScreenInit function, and fill in
     * other pScreen fields.
     */
    switch (pScrn->bitsPerPixel) {
    case 8:
    case 16:
    case 24:
    case 32:
        ret = fbScreenInit(pScreen, FbBase, width, height,
                            pScrn->xDpi, pScrn->yDpi,
                            displayWidth, pScrn->bitsPerPixel);
        break;
    default:
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                    "X11: Internal error: invalid bpp (%d) in "
                    "LgScreenInit\n",
                    pScrn->bitsPerPixel);
        ret = FALSE;
        break;
    }
    if (!ret)
        return FALSE;

#ifdef LG_DEBUG
    ErrorF("LgScreenInit after depth dependent init\n");
#endif

    /*
     * Override the default mask/offset settings.
     */
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

    /*
     * Must be after RGB ordering fixed.
     */

    fbPictureInit(pScreen, 0, 0);

    /*
     * Set initial black & white colourmap indices.
     */
    xf86SetBlackWhitePixels(pScreen);

#if 1
    pCir->DGAModeInit = LgModeInit;
    if (!CirDGAInit(pScreen))
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                    "DGA initialization failed\n");
#endif
    xf86SetSilkenMouse(pScreen);

    /*
     * Initialise cursor functions.
     */
    miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

    /*
     * Initialize HW cursor layer.
     */
    if (pCir->HWCursor) {
        if (!LgHWCursorInit(pScreen))
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                    "Hardware cursor initialization failed\n");
    }

    /*
     * Initialise default colourmap.
     */
    if (!miCreateDefColormap(pScreen))
        return FALSE;

    if (pScrn->bitsPerPixel > 1 && pScrn->bitsPerPixel <= 8)
        vgaHWHandleColormaps(pScreen);

    xf86DPMSInit(pScreen, LgDisplayPowerManagementSet, 0);

    pScrn->memPhysBase = pCir->FbAddress;
    pScrn->fbOffset = 0;

    {
        XF86VideoAdaptorPtr *ptr;
        int n;

        n = xf86XVListGenericAdaptors(pScrn, &ptr);
        if (n)
            xf86XVScreenInit(pScreen, ptr, n);
    }

    /*
     * Wrap the CloseScreen vector and set SaveScreen.
     */
    pScreen->SaveScreen = LgSaveScreen;
    pCir->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = LgCloseScreen;

    /*
     * Report any unused options (only for the first generation).
     */
    if (serverGeneration == 1)
        xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);

    /*
     * Done
     */
    return TRUE;
}

/*
 * Usually mandatory
 */
Bool
LgSwitchMode(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    return LgModeInit(pScrn, mode);
}

#define ROUND_DOWN(x, mod) (((x) / (mod)) * (mod))
#define ROUND_UP(x, mod) ((((x) + (mod) - 1) / (mod)) * (mod))

/*
 * This function is used to initialize the Start Address - the first
 * displayed location in the video memory.
 */
/*
 * Usually mandatory
 */
void LgAdjustFrame(ScrnInfoPtr pScrn, int x, int y) {
    int Base, tmp;
    CirPtr pCir = CIRPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    int cursorX, cursorY;
    int middleX, middleY;
    const LgLineDataPtr lineData =
                            &LgLineData[pCir->chip.lg->lineDataIndex];
    const int viewportXRes =
            (PCI_CHIP_GD5465 == pCir->Chipset) ?
                    (24 == pScrn->bitsPerPixel ? 24 : 1) :
                    (lineData->width ? 256 : 128)
                            / (24 == pScrn->bitsPerPixel ?
                                    1 : (pScrn->bitsPerPixel >> 3));
    const int viewportYRes =
            (PCI_CHIP_GD5465 == pCir->Chipset) ?
                    1 : (24 == pScrn->bitsPerPixel ? 3 : 1);

    /*
     * Where's the pointer?
     */
    miPointerGetPosition(inputInfo.pointer, &cursorX, &cursorY);

    /*
     * Where's the middle of the screen?  We want to eventually know
     * which side of the screen the pointer is on.
     */
    middleX = (pScrn->frameX1 + pScrn->frameX0) / 2;
    middleY = (pScrn->frameY1 + pScrn->frameY0) / 2;

    if (cursorX < middleX) {
        /*
         * Pointer is on left side of screen.  Round the frame value
         * down.
         */
        pScrn->frameX0 = ROUND_DOWN(pScrn->frameX0, viewportXRes);
    } else {
        /*
         * Pointer is on right side of screen.  Round the frame value
         * up.  A side effect of this rounding up is that we might
         * expose a part of the screen that's actually on the far
         * /left/ of the frame buffer.  That's because, although the
         * virtual desktop might be an integral number of tiles, the
         * display might not.  We'll just live with this artifact.
         */
        pScrn->frameX0 = ROUND_UP(pScrn->frameX0, viewportXRes);
    }
    pScrn->frameX1 = pScrn->frameX0 +
                        pScrn->currentMode->HDisplay - 1;

    if (cursorY < middleY) {
        pScrn->frameY0 = ROUND_DOWN(pScrn->frameY0, viewportYRes);
    } else {
        pScrn->frameY0 = ROUND_UP(pScrn->frameY0, viewportYRes);
    }
    pScrn->frameY1 = pScrn->frameY0 +
                        pScrn->currentMode->VDisplay - 1;

    if (x != pScrn->frameX0 || y != pScrn->frameY0) {
        /*
         * !!!
         */
        /*
         * We moved the frame from where xf86SetViewport() placed
         * it.  If we're using a SW cursor, that's okay -- the
         * pointer exists in the framebuffer, and those bits are
         * still all aligned.  But if we're using a HW cursor, then
         * we need to re-align the pointer.  Call SetCursorPosition()
         * with the appropriate new pointer values, adjusted to be
         * wrt the new frame.
         */
        x = pScrn->frameX0;
        y = pScrn->frameY0;
    }

    /*
     * ??? Will this work for 1bpp?
     */
    Base = (y * lineData->pitch + (x * pScrn->bitsPerPixel / 8)) / 4;

    if ((Base & ~0x000FFFFF) != 0) {
        /*
         * ???
         */
        ErrorF("X11: Internal error: LgAdjustFrame: cannot handle "
                "overflow\n");
        return;
    }

    hwp->writeCrtc(hwp, 0x0C, (Base >> 8) & 0xFF);
    hwp->writeCrtc(hwp, 0x0D, Base & 0xFF);
    tmp = hwp->readCrtc(hwp, 0x1B) & 0xF2;
    tmp |= (Base >> 16) & 0x01;
    tmp |= (Base >> 15) & 0x0C;
    hwp->writeCrtc(hwp, 0x1B, tmp);
    tmp = hwp->readCrtc(hwp, 0x1D) & 0xE7;
    tmp |= (Base >> 16) & 0x18;
    hwp->writeCrtc(hwp, 0x1D, tmp);
}

/*
 * This is called when VT switching back to the X server.  Its job is
 * to reinitialise the video mode.  We may wish to unmap video/MMIO
 * memory too.
 */
/*
 * Mandatory
 */
Bool
LgEnterVT(ScrnInfoPtr pScrn)
{
    CirPtr pCir = CIRPTR(pScrn);
#ifdef LG_DEBUG
    ErrorF("LgEnterVT\n");
#endif

    /*
     * XXX Shouldn't this be in LeaveVT?
     */
    /*
     * Disable HW cursor.
     */
    if (pCir->HWCursor)
        LgHideCursor(pScrn);

    /*
     * Should we re-save the text mode on each VT enter?
     */
    return LgModeInit(pScrn, pScrn->currentMode);
}

/*
 * This is called when VT switching away from the X server.  Its job
 * is to restore the previous (text) mode.  We may wish to remap
 * video/MMIO memory too.
 */
/*
 * Mandatory
 */
void LgLeaveVT(ScrnInfoPtr pScrn) {
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CirPtr pCir = CIRPTR(pScrn);
#ifdef LG_DEBUG
    ErrorF("LgLeaveVT\n");
#endif

    /*
     * XXX Shouldn't this be in EnterVT?
     */
    /*
     * Enable HW cursor.
     */
    if (pCir->HWCursor)
        LgShowCursor(pScrn);

    LgRestore(pScrn);
    vgaHWLock(hwp);
}

/*
 * This is called at the end of each server generation.  It restores
 * the original (text) mode.  It should also unmap the video memory,
 * and free any per-generation data allocated by the driver.  It
 * should finish by unwrapping and calling the saved CloseScreen
 * function.
 */
/*
 * Mandatory
 */
static Bool
LgCloseScreen(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CirPtr pCir = CIRPTR(pScrn);

    if (pScrn->vtSema) {
        LgRestore(pScrn);
        if (pCir->HWCursor)
            LgHideCursor(pScrn);

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

/*
 * Free up any persistent data structures.
 */
/*
 * Optional
 */
void
LgFreeScreen(ScrnInfoPtr pScrn)
{
#ifdef LG_DEBUG
    ErrorF("LgFreeScreen\n");
#endif
    /*
     * This only gets called when a screen is being deleted.  It does
     * not get called routinely at the end of a server generation.
     */
    if (xf86LoaderCheckSymbol("vgaHWFreeHWRec"))
        vgaHWFreeHWRec(pScrn);
    LgFreeRec(pScrn);
}

/*
 * Checks if a mode is suitable for the selected chipset.
 */
/*
 * Optional
 */
ModeStatus
LgValidMode(ScrnInfoPtr arg, DisplayModePtr mode,
            Bool verbose, int flags)
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
        return (MODE_OK);
    }
    return (MODE_BAD);
}

/*
 * Do screen blanking.
 */
/*
 * Mandatory
 */
static Bool
LgSaveScreen(ScreenPtr pScreen, int mode)
{
    CirPtr pCir = CIRPTR(xf86ScreenToScrn(pScreen));
    ScrnInfoPtr pScrn = NULL;
    Bool unblank;

    unblank = xf86IsUnblank(mode);

    if (pScreen != NULL)
        pScrn = xf86ScreenToScrn(pScreen);

    if (pScrn != NULL && pScrn->vtSema) {
        if (unblank)
            /*
             * Power up the palette DAC.
             */
            memwb(0xB0, memrb(0xB0) & 0x7F);
        else
            /*
             * Power down the palette DAC.
             */
            memwb(0xB0, memrb(0xB0) | 0x80);
    }

    return vgaHWSaveScreen(pScreen, mode);
}

static CARD16
LgSetClock(CirPtr pCir, vgaHWPtr hwp, int freq)
{
    int ffreq, num, den;
    CARD8 tmp;

    ErrorF("LgSetClock freq=%d.%03dMHz\n", freq / 1000, freq % 1000);

    ffreq = freq;
    if (!CirrusFindClock(&ffreq, pCir->MaxClock, &num, &den))
        return 0;

    ErrorF("LgSetClock: nom=%x den=%x ffreq=%d.%03dMHz\n",
            num, den, ffreq / 1000, ffreq % 1000);

    /*
     * Set VCLK3.
     */
    /*
     * The numerator and denominator registers are switched around
     * in the Laguna chips.
     */
    tmp = hwp->readSeq(hwp, 0x0E);
    hwp->writeSeq(hwp, 0x0E, (tmp & 0x80) | den);
    hwp->writeSeq(hwp, 0x1E, num);

    return (den << 8) | num;
}

/*
 * LgDisplayPowerManagementSet --
 *
 * Sets VESA Display Power Management Signaling (DPMS) Mode.
 */
static void
LgDisplayPowerManagementSet(ScrnInfoPtr pScrn,
                            int PowerManagementMode,
                            int flags)
{
    unsigned char sr01, cr1a;
    vgaHWPtr hwp;

#ifdef LG_DEBUG
    ErrorF("LgDisplayPowerManagementSet: %d\n", PowerManagementMode);
#endif

    hwp = VGAHWPTR(pScrn);

    switch (PowerManagementMode) {
    case DPMSModeOn:
        /*
         * Screen: On; HSync: On, VSync: On
         */
        sr01 = 0x00;
        cr1a = 0x00;
        break;
    case DPMSModeStandby:
        /*
         * Screen: Off; HSync: Off, VSync: On
         */
        sr01 = 0x20;
        cr1a = 0x08;
        break;
    case DPMSModeSuspend:
        /*
         * Screen: Off; HSync: On, VSync: Off
         */
        sr01 = 0x20;
        cr1a = 0x04;
        break;
    case DPMSModeOff:
        /*
         * Screen: Off; HSync: Off, VSync: Off
         */
        sr01 = 0x20;
        cr1a = 0x0c;
        break;
    default:
        return;
    }

    sr01 |= hwp->readSeq(hwp, 0x01) & ~0x20;
    hwp->writeSeq(hwp, 0x01, sr01);
    cr1a |= hwp->readCrtc(hwp, 0x1A) & ~0x0C;
    hwp->writeCrtc(hwp, 0x1A, cr1a);
}

#define minb(p) MMIO_IN8(hwp->MMIOBase, (p))
#define moutb(p,v) MMIO_OUT8(hwp->MMIOBase, (p),(v))

static void
mmioWriteCrtc(vgaHWPtr hwp, CARD8 index, CARD8 value)
{
    moutb(index << 2, value);
}

static CARD8
mmioReadCrtc(vgaHWPtr hwp, CARD8 index)
{
    return minb(index << 2);
}

static void
lg_vgaHWSetMmioFunc(vgaHWPtr hwp, CARD8 *base)
{
    hwp->writeCrtc = mmioWriteCrtc;
    hwp->readCrtc = mmioReadCrtc;
    hwp->MMIOBase = base;
    hwp->MMIOOffset = 0;
}
