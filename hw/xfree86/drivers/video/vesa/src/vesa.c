#define DEBUG_VERB 2
/*
 * Copyright (c) 2000 by Conectiva S.A. (http://www.conectiva.com)
 * Copyright 2008 Red Hat, Inc.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * CONECTIVA LINUX BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Conectiva Linux shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from
 * Conectiva Linux.
 *
 * Authors: Paulo César Pereira de Andrade <pcpa@conectiva.com.br>
 *          David Dawes <dawes@xfree86.org>
 *          Adam Jackson <ajax@redhat.com>
 */

/*
 * TODO:
 * - Port to RANDR 1.2 setup to make mode selection slightly better
 * - Port to RANDR 1.2 to drop the old-school DGA junk
 * - VBE/SCI for secondary DDC method?
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include "vesa.h"

/* All drivers initialising the SW cursor need this */
#include "mipointer.h"

/* Colormap handling */
#include "micmap.h"
#include "xf86cmap.h"
#include "xf86Modes.h"

/* DPMS */
#include <X11/extensions/dpmsconst.h>

/* Mandatory functions */
static const OptionInfoRec * VESAAvailableOptions(int chipid, int busid);
static void VESAIdentify(int flags);
static Bool VESAPciProbe(DriverPtr drv, int entity_num,
     struct pci_device *dev, intptr_t match_data);
static Bool VESAPreInit(ScrnInfoPtr pScrn, int flags);
static Bool VESAScreenInit(ScreenPtr pScreen, int argc, char **argv);
static Bool VESAEnterVT(ScrnInfoPtr pScrn);
static void VESALeaveVT(ScrnInfoPtr pScrn);
static Bool VESACloseScreen(ScreenPtr pScreen);
static Bool VESASaveScreen(ScreenPtr pScreen, int mode);

static Bool VESASwitchMode(ScrnInfoPtr pScrn, DisplayModePtr mode);
static Bool VESASetMode(ScrnInfoPtr pScrn, DisplayModePtr pMode);
static void VESAAdjustFrame(ScrnInfoPtr pScrn, int x, int y);
static void VESAFreeScreen(ScrnInfoPtr pScrn);
static void VESAFreeRec(ScrnInfoPtr pScrn);
static VESAPtr VESAGetRec(ScrnInfoPtr pScrn);

static void
VESADisplayPowerManagementSet(ScrnInfoPtr pScrn, int mode,
                int flags);

/* locally used functions */
static Bool VESAMapVidMem(ScrnInfoPtr pScrn);
static void VESAUnmapVidMem(ScrnInfoPtr pScrn);
static int VESABankSwitch(ScreenPtr pScreen, unsigned int iBank);
static void VESALoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices,
                            LOCO *colors, VisualPtr pVisual);
static void SaveFonts(ScrnInfoPtr pScrn);
static void RestoreFonts(ScrnInfoPtr pScrn);
static Bool
VESASaveRestore(ScrnInfoPtr pScrn, vbeSaveRestoreFunction function);

static void *
VESAWindowLinear(ScreenPtr pScreen, CARD32 row, CARD32 offset, int mode,
                 CARD32 *size, void *closure)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    VESAPtr pVesa = VESAGetRec(pScrn);

    *size = pVesa->maxBytesPerScanline;
    return ((CARD8 *)pVesa->base + row * pVesa->maxBytesPerScanline + offset);
}

static void *
VESAWindowWindowed(ScreenPtr pScreen, CARD32 row, CARD32 offset, int mode,
                   CARD32 *size, void *closure)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    VESAPtr pVesa = VESAGetRec(pScrn);
    VbeModeInfoBlock *data = ((VbeModeInfoData*)(pScrn->currentMode->Private))->data;
    int window;

    offset += pVesa->maxBytesPerScanline * row;
    window = offset / (data->WinGranularity * 1024);
    pVesa->windowAoffset = window * data->WinGranularity * 1024;
    VESABankSwitch(pScreen, window);
    *size = data->WinSize * 1024 - (offset - pVesa->windowAoffset);

    return (void *)((unsigned long)pVesa->base + (offset - pVesa->windowAoffset));
}

static void
vesaUpdatePacked(ScreenPtr pScreen, shadowBufPtr pBuf)
{
    shadowUpdatePacked(pScreen, pBuf);
}

static Bool VESADGAInit(ScrnInfoPtr pScrn, ScreenPtr pScreen);

enum GenericTypes
{
    CHIP_VESA_GENERIC
};

static const struct pci_id_match vesa_device_match[] = {
    {
        PCI_MATCH_ANY, PCI_MATCH_ANY, PCI_MATCH_ANY, PCI_MATCH_ANY,
        0x00030000, 0x00ffffff, CHIP_VESA_GENERIC
    },
    { 0, 0, 0 },
};

/* Supported chipsets */
static SymTabRec VESAChipsets[] =
{
    {CHIP_VESA_GENERIC, "vesa"},
    {-1,                NULL}
};

/*
 * This contains the functions needed by the server after loading the
 * driver module.  It must be supplied, and gets added the driver list by
 * the Module Setup function in the dynamic case.  In the static case a
 * reference to this is compiled in, and this requires that the name of
 * this DriverRec be an upper-case version of the driver name.
 */
_X_EXPORT DriverRec VESA = {
    VESA_VERSION,
    VESA_DRIVER_NAME,
    VESAIdentify,
    NULL,
    VESAAvailableOptions,
    NULL,
    0,
    NULL,
    vesa_device_match,
    VESAPciProbe
};


typedef enum {
    OPTION_SHADOW_FB,
    OPTION_DFLT_REFRESH,
    OPTION_MODESET_CLEAR_SCREEN
} VESAOpts;

static const OptionInfoRec VESAOptions[] = {
    { OPTION_SHADOW_FB,            "ShadowFB",           OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_DFLT_REFRESH,         "DefaultRefresh",     OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_MODESET_CLEAR_SCREEN, "ModeSetClearScreen", OPTV_BOOLEAN, {0}, FALSE },
    { -1,                          NULL,                 OPTV_NONE,    {0}, FALSE }
};

#ifdef XFree86LOADER

/* Module loader interface */
static MODULESETUPPROTO(vesaSetup);

static XF86ModuleVersionInfo vesaVersionRec =
{
    VESA_DRIVER_NAME,
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    VESA_MAJOR_VERSION,
    VESA_MINOR_VERSION,
    VESA_PATCHLEVEL,
    ABI_CLASS_VIDEODRV,
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_VIDEODRV,
    {0, 0, 0, 0}
};

/*
 * This data is accessed by the loader.  The name must be the module name
 * followed by "ModuleData".
 */
_X_EXPORT XF86ModuleData vesaModuleData = { &vesaVersionRec, vesaSetup, NULL };

static void*
vesaSetup(void *Module, void *Options, int *ErrorMajor, int *ErrorMinor)
{
    static Bool Initialised = FALSE;

    if (!Initialised)
    {
        Initialised = TRUE;
        xf86AddDriver(&VESA, Module, 1);
        return (void*)TRUE;
    }

    if (ErrorMajor)
        *ErrorMajor = LDR_ONCEONLY;

    return (NULL);
}

#endif

static const OptionInfoRec *
VESAAvailableOptions(int chipid, int busid)
{
    return (VESAOptions);
}

static void
VESAIdentify(int flags)
{
    xf86PrintChipsets(VESA_NAME, "driver for VESA chipsets", VESAChipsets);
}

static VESAPtr
VESAGetRec(ScrnInfoPtr pScrn)
{
    if (!pScrn->driverPrivate)
        pScrn->driverPrivate = calloc(1, sizeof(VESARec));

    return ((VESAPtr)pScrn->driverPrivate);
}

/* Only a little like VBESetModeParameters */
static void
VESASetModeParameters(vbeInfoPtr pVbe, DisplayModePtr vbemode,
                      DisplayModePtr ddcmode)
{
    VbeModeInfoData *data;
    int clock;

    data = (VbeModeInfoData *)vbemode->Private;

    data->block = calloc(1, sizeof(VbeCRTCInfoBlock));
    data->block->HorizontalTotal = ddcmode->HTotal;
    data->block->HorizontalSyncStart = ddcmode->HSyncStart;
    data->block->HorizontalSyncEnd = ddcmode->HSyncEnd;
    data->block->VerticalTotal = ddcmode->VTotal;
    data->block->VerticalSyncStart = ddcmode->VSyncStart;
    data->block->VerticalSyncEnd = ddcmode->VSyncEnd;
    data->block->Flags = ((ddcmode->Flags & V_NHSYNC) ? CRTC_NHSYNC : 0) |
                         ((ddcmode->Flags & V_NVSYNC) ? CRTC_NVSYNC : 0);
    data->block->PixelClock = ddcmode->Clock * 1000;

    /* ask the BIOS to figure out the real clock */
    clock = VBEGetPixelClock(pVbe, data->mode, data->block->PixelClock);
    if (clock)
        data->block->PixelClock = clock;

    data->mode |= (1 << 11);
    data->block->RefreshRate = 100 * ((double)(data->block->PixelClock) /
                                     (double)(ddcmode->HTotal * ddcmode->VTotal));
}

/*
 * Despite that VBE gives you pixel granularity for mode sizes, some BIOSes
 * think they can only give sizes in multiples of character cells; and
 * indeed, the reference CVT and GTF formulae only give results where
 * (h % 8) == 0.  Whatever, let's just try to cope.  What we're looking for
 * here is cases where the display says 1366x768 and the BIOS says 1360x768.
 */
static Bool
vesaModesCloseEnough(DisplayModePtr edid, DisplayModePtr vbe)
{
    if (!(edid->type & M_T_DRIVER))
        return FALSE;

    /* never seen a height granularity... */
    if (edid->VDisplay != vbe->VDisplay)
        return FALSE;

    if (edid->HDisplay >= vbe->HDisplay &&
        (edid->HDisplay & ~7) == (vbe->HDisplay & ~7))
        return TRUE;

    return FALSE;
}

static ModeStatus
VESAValidMode(ScrnInfoPtr pScrn, DisplayModePtr p, Bool flag, int pass)
{
    static int warned = 0;
    int found = 0;
    VESAPtr pVesa = VESAGetRec(pScrn);
    MonPtr mon = pScrn->monitor;
    ModeStatus ret = MODE_BAD;
    DisplayModePtr mode;
    float v;

    if (pass != MODECHECK_FINAL) {
        if (!warned) {
            xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "VESAValidMode called unexpectedly\n");
            warned = 1;
        }
        return MODE_OK;
    }

    /*
     * This is suboptimal.  We pass in just the barest description of a mode
     * we can get away with to VBEValidateModes, so it can't really throw
     * out anything we give it.  But we need to filter the list so that we
     * don't populate the mode list with things the monitor can't do.
     *
     * So first off, if this isn't a mode we handed to the server (ie,
     * M_T_BUILTIN), then we know we can't do it.
     */
    if (!(p->type & M_T_BUILTIN))
        return MODE_NOMODE;

    if (pVesa->strict_validation) {
        /*
         * If it's our first pass at mode validation, we'll try for a strict
         * intersection between the VBE and DDC mode lists.
         */
        if (pScrn->monitor->DDC) {
            for (mode = pScrn->monitor->Modes; mode; mode = mode->next) {
                if (vesaModesCloseEnough(mode, p)) {
                    if (xf86CheckModeForMonitor(mode, mon) == MODE_OK) {
                        found = 1;
                        break;
                    }
                }
            if (mode == pScrn->monitor->Last)
                break;
            }
            if (!found)
                return MODE_NOMODE;

            /* having found a matching mode, stash the CRTC values aside */
            VESASetModeParameters(pVesa->pVbe, p, mode);
            return MODE_OK;
        }

        /* No DDC and no modes make Homer something something... */
        return MODE_NOMODE;
    }

    /*
     * Finally, walk through the vsync rates 1Hz at a time looking for a mode
     * that will fit.  This is assuredly a terrible way to do this, but
     * there's no obvious method for computing a mode of a given size that
     * will pass xf86CheckModeForMonitor.  XXX this path is terrible, but
     * then, by this point, you're well into despair territory.
     */
    for (v = mon->vrefresh[0].lo; v <= mon->vrefresh[0].hi; v++) {
        mode = xf86GTFMode(p->HDisplay, p->VDisplay, v, 0, 0);
        ret = xf86CheckModeForMonitor(mode, mon);
        free((char*)mode->name);
        free(mode);
        if (ret == MODE_OK)
            break;
    }

    return ret;
}

static void
VESAInitScrn(ScrnInfoPtr pScrn)
{
    pScrn->driverVersion = VESA_VERSION;
    pScrn->driverName    = VESA_DRIVER_NAME;
    pScrn->name          = VESA_NAME;
    pScrn->Probe         = NULL;
    pScrn->PreInit       = VESAPreInit;
    pScrn->ScreenInit    = VESAScreenInit;
    pScrn->SwitchMode    = VESASwitchMode;
    pScrn->ValidMode     = VESAValidMode;
    pScrn->AdjustFrame   = VESAAdjustFrame;
    pScrn->EnterVT       = VESAEnterVT;
    pScrn->LeaveVT       = VESALeaveVT;
    pScrn->FreeScreen    = VESAFreeScreen;
}

#ifdef __linux__
/*
 * check if a file exists in directory and can be opened
 * should be equivalent to a glob ${directory}/${prefix}*
 */

#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static Bool
VESAFileExistsPrefix(const char *directory, const char *prefix) {
    DIR *dir;
    struct dirent *entry;
    int dirlen = strlen(directory);
    int len = strlen(prefix);
    char filepath[PATH_MAX];

    /* Never happens, since we know the args to this function */
    if (dirlen + 1 + len + 1 > PATH_MAX) {
        return FALSE;
    }

    dir = opendir(directory);
    if (!dir)
        return FALSE;

    memcpy(filepath, directory, dirlen);
    filepath[dirlen] = '/';
    char *filename = filepath + dirlen + 1;

    while ((entry = readdir(dir)) != NULL) {
        int filelen = strlen(entry->d_name);
        if (filelen > len &&
            !memcmp(entry->d_name, prefix, len)) {
            if (dirlen + 1 + filelen + 1 > PATH_MAX) {
                /* Filename too long */
                continue;
            }
            memcpy(filename, entry->d_name, filelen + 1);
            int fd = open(filepath, O_RDONLY);
            if (fd > -1) {
                close(fd);
                closedir(dir);
                return TRUE;
            }
        }
    }
    closedir(dir);
    return FALSE;
}
#endif

/*
 * This function is called once, at the start of the first server generation to
 * do a minimal probe for supported hardware.
 */
static Bool
VESAPciProbe(DriverPtr drv, int entity_num, struct pci_device *dev,
             intptr_t match_data)
{
    ScrnInfoPtr pScrn;

#ifdef __linux__
    /* XXX Is this really needed? We check for a bound kernel driver bellow. XXX */
    if (VESAFileExistsPrefix("/dev/dri", "card")) {
        ErrorF("vesa: Refusing to run, dri device present\n");
        return FALSE;
    }
#endif

    pScrn = xf86ConfigPciEntity(NULL, 0, entity_num, NULL,
                                NULL, NULL, NULL, NULL, NULL);
    if (pScrn != NULL) {
        VESAPtr pVesa;

#if !defined (__FreeBSD__) && !defined (__DragonFly__)
        if (pci_device_has_kernel_driver(dev)) {
            ErrorF("vesa: Ignoring device with a bound kernel driver\n");
            return FALSE;
        }
#else
        xf86DrvMsg(-1,X_WARNING,"vesa: Not checking for attached KMS drivers on FreeBSD and DragonFlyBSD.\n\
      Using this driver with a kernel KMS driver loaded can cause a blackscreen!\n");
#endif

        pVesa = VESAGetRec(pScrn);
        VESAInitScrn(pScrn);
        pVesa->pciInfo = dev;
    }

    return (pScrn != NULL);
}

static void
VESAFreeRec(ScrnInfoPtr pScrn)
{
    VESAPtr pVesa = VESAGetRec(pScrn);

    free(pVesa->monitor);
    if (pVesa->vbeInfo)
        VBEFreeVBEInfo(pVesa->vbeInfo);
    free(pVesa->pal);
    free(pVesa->savedPal);
    free(pVesa->fonts);
    free(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
}

static int
VESAValidateModes(ScrnInfoPtr pScrn)
{
    VESAPtr pVesa = VESAGetRec(pScrn);
    DisplayModePtr mode;

    for (mode = pScrn->monitor->Modes; mode; mode = mode->next)
        mode->status = MODE_OK;

    return VBEValidateModes(pScrn, NULL, pScrn->display->modes,
                            NULL, NULL, 0, 32767, 1, 0, 32767,
                            pScrn->display->virtualX,
                            pScrn->display->virtualY,
                            pVesa->mapSize, LOOKUP_BEST_REFRESH);
}

/*
 * This function is called once for each screen at the start of the first
 * server generation to initialise the screen for all server generations.
 */
static Bool
VESAPreInit(ScrnInfoPtr pScrn, int flags)
{
    VESAPtr pVesa;
    VbeInfoBlock *vbe;
    DisplayModePtr pMode;
    VbeModeInfoBlock *mode;
    Gamma gzeros = {0.0, 0.0, 0.0};
    rgb rzeros = {0, 0, 0};
    void *pDDCModule;
    int i;
    int flags24 = 0;
    int defaultDepth = 0;
    int defaultBpp = 0;
    int depths = 0;

    if (flags & PROBE_DETECT)
        return (FALSE);

    pVesa = VESAGetRec(pScrn);
    pVesa->pEnt = xf86GetEntityInfo(pScrn->entityList[0]);

    /* Load vbe module */
    if (!xf86LoadSubModule(pScrn, "vbe"))
        return (FALSE);

    if ((pVesa->pVbe = VBEExtendedInit(NULL, pVesa->pEnt->index,
                            SET_BIOS_SCRATCH | RESTORE_BIOS_SCRATCH)) == NULL)
        return (FALSE);

    pScrn->chipset = "vesa";
    pScrn->monitor = pScrn->confScreen->monitor;
    pScrn->progClock = TRUE;
    pScrn->rgbBits = 8;

    if ((vbe = VBEGetVBEInfo(pVesa->pVbe)) == NULL)
        return (FALSE);
    pVesa->major = (unsigned)(vbe->VESAVersion >> 8);
    pVesa->minor = vbe->VESAVersion & 0xff;
    pVesa->vbeInfo = vbe;
    pScrn->videoRam = vbe->TotalMemory * 64;

    /*
     * Find what depths are available.
     */
    depths = VBEFindSupportedDepths(pVesa->pVbe, pVesa->vbeInfo, &flags24, V_MODETYPE_VBE);

    /* Preferred order for default depth selection. */
    if (depths & V_DEPTH_24 && (flags24 & Support32bppFb))
        defaultDepth = 24;
    else if (depths & V_DEPTH_16)
        defaultDepth = 16;
    else if (depths & V_DEPTH_15)
        defaultDepth = 15;
    else if (depths & V_DEPTH_24)
        defaultDepth = 24; /* ew though */
    else if (depths & V_DEPTH_8)
        defaultDepth = 8;
    else if (depths & V_DEPTH_4)
        defaultDepth = 4;
    else if (depths & V_DEPTH_1)
        defaultDepth = 1;

    if (defaultDepth == 24 && !(flags24 & Support32bppFb))
        defaultBpp = 24;

    /* Prefer 32bpp because 1999 called and wants its packed pixels back */
    if (flags24 & Support32bppFb)
        flags24 |= SupportConvert24to32 | PreferConvert24to32;
    if (flags24 & Support24bppFb)
        flags24 |= SupportConvert32to24;

    if (!xf86SetDepthBpp(pScrn, defaultDepth, 0, defaultBpp, flags24)) {
        vbeFree(pVesa->pVbe);
        return (FALSE);
    }
    xf86PrintDepthBpp(pScrn);

    /* color weight */
    if (pScrn->depth > 8 && !xf86SetWeight(pScrn, rzeros, rzeros)) {
        vbeFree(pVesa->pVbe);
        return (FALSE);
    }
    /* visual init */
    if (!xf86SetDefaultVisual(pScrn, -1)) {
        vbeFree(pVesa->pVbe);
        return (FALSE);
    }

    xf86SetGamma(pScrn, gzeros);

    /* set up options before loading any modules that may look at them */
    xf86CollectOptions(pScrn, NULL);

    if (pVesa->major >= 2) {
        /* Load ddc module */
        if ((pDDCModule = xf86LoadSubModule(pScrn, "ddc")) == NULL) {
            vbeFree(pVesa->pVbe);
            return (FALSE);
        }

        if ((pVesa->monitor = vbeDoEDID(pVesa->pVbe, pDDCModule)) != NULL) {
            xf86PrintEDID(pVesa->monitor);
        }

        xf86UnloadSubModule(pDDCModule);
    }

    if ((pScrn->monitor->DDC = pVesa->monitor) != NULL)
        xf86SetDDCproperties(pScrn, pVesa->monitor);
    else {
        void *panelid = VBEReadPanelID(pVesa->pVbe);
        VBEInterpretPanelID(pScrn, panelid);
        free(panelid);
    }

    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, DEBUG_VERB,
                   "Searching for matching VESA mode(s):\n");

    /*
     * Check the available BIOS modes, and extract those that match the
     * requirements into the modePool.  Note: modePool is a NULL-terminated
     * list.
     */
    pScrn->modePool = VBEGetModePool (pScrn, pVesa->pVbe, pVesa->vbeInfo, V_MODETYPE_VBE);

    xf86ErrorFVerb(DEBUG_VERB, "\n");
    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, DEBUG_VERB,
                   "Total Memory: %d 64KB banks (%dkB)\n", vbe->TotalMemory,
                   (vbe->TotalMemory * 65536) / 1024);

    pVesa->mapSize = vbe->TotalMemory * 65536;
    if (pScrn->modePool == NULL) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No matching modes\n");
        vbeFree(pVesa->pVbe);
        return (FALSE);
    }

    VBESetModeNames(pScrn->modePool);

    pVesa->strict_validation = TRUE;
    i = VESAValidateModes(pScrn);

    if (i <= 0) {
        xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
                   "No valid modes left. Trying less strict filter...\n");
        pVesa->strict_validation = FALSE;
        i = VESAValidateModes(pScrn);
    }

    if (i <= 0) do {
        Bool changed = FALSE;
        /* maybe there's more modes at the bottom... */
        if (pScrn->monitor->vrefresh[0].lo > 50) {
            changed = TRUE;
            pScrn->monitor->vrefresh[0].lo = 50;
        }
        if (pScrn->monitor->hsync[0].lo > 31.5) {
            changed = TRUE;
            pScrn->monitor->hsync[0].lo = 31.5;
        }

        if (!changed)
            break;

        xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
                   "No valid modes left. Trying aggressive sync range...\n");
        i = VESAValidateModes(pScrn);
    } while (0);

    if (i <= 0) {
        /* alright, i'm out of ideas */
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes\n");
        vbeFree(pVesa->pVbe);
        return (FALSE);
    }

    xf86PruneDriverModes(pScrn);

    pMode = pScrn->modes;
    do {
        mode = ((VbeModeInfoData*)pMode->Private)->data;
        if (mode->BytesPerScanline > pVesa->maxBytesPerScanline) {
            pVesa->maxBytesPerScanline = mode->BytesPerScanline;
        }
        pMode = pMode->next;
    } while (pMode != pScrn->modes);

    pScrn->currentMode = pScrn->modes;
    pScrn->displayWidth = pScrn->virtualX;

    VBEPrintModes(pScrn);

    /* Set display resolution */
    xf86SetDpi(pScrn, 0, 0);

    if (pScrn->modes == NULL) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No modes\n");
        vbeFree(pVesa->pVbe);
        return (FALSE);
    }

    /* options */
    if (!(pVesa->Options = malloc(sizeof(VESAOptions)))) {
        vbeFree(pVesa->pVbe);
        return FALSE;
    }
    memcpy(pVesa->Options, VESAOptions, sizeof(VESAOptions));
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, pVesa->Options);

    /* Use shadow by default, for non-virt hardware */
    if (!xf86GetOptValBool(pVesa->Options, OPTION_SHADOW_FB, &pVesa->shadowFB))
    {
        switch (pVesa->pciInfo->vendor_id) {
            case 0x1234: /* bochs vga (not in pci.ids) */
            case 0x15ad: /* vmware */
            case 0x1b36: /* qemu qxl */
            case 0x80ee: /* virtualbox */
            case 0xaaaa: /* parallels (not in pci.ids) */
                pVesa->shadowFB = FALSE;
                break;

            case 0x1013: /* qemu's cirrus emulation */
                if (pVesa->pciInfo->subvendor_id == 0x1af4)
                    pVesa->shadowFB = FALSE;
                else
                    pVesa->shadowFB = TRUE;
                break;

            case 0x1414: /* microsoft hyper-v */
                if (pVesa->pciInfo->device_id == 0x5353)
                    pVesa->shadowFB = FALSE;
                else
                    pVesa->shadowFB = TRUE;
                break;

            default:
                pVesa->shadowFB = TRUE;
                break;
        }
    }

    /*  Use default refresh by default. Too many VBE 3.0
     *   BIOSes are incorrectly implemented.
     */
    pVesa->defaultRefresh = xf86ReturnOptValBool(pVesa->Options,
                                                 OPTION_DFLT_REFRESH, TRUE);

    pVesa->ModeSetClearScreen =
        xf86ReturnOptValBool(pVesa->Options,
                             OPTION_MODESET_CLEAR_SCREEN, FALSE);

    if (!pVesa->defaultRefresh && !pVesa->strict_validation)
        VBESetModeParameters(pScrn, pVesa->pVbe);

    mode = ((VbeModeInfoData*)pScrn->modes->Private)->data;
    switch (mode->MemoryModel) {
        case 0x4: /* Packed pixel */
        case 0x6: /* Direct Color */
            pScrn->bitmapBitOrder = BITMAP_BIT_ORDER;

            switch (pScrn->bitsPerPixel) {
                case 8:
                case 16:
                case 24:
                case 32:
                    break;
                default:
                    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                               "Unsupported bpp: %d\n", pScrn->bitsPerPixel);
                    vbeFree(pVesa->pVbe);
                    return FALSE;
            }
            break;
        default:
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                       "Unsupported Memory Model: %d\n", mode->MemoryModel);
            return FALSE;
    }

    if (pVesa->shadowFB) {
        xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Using \"Shadow Framebuffer\"\n");
        if (!xf86LoadSubModule(pScrn, "shadow")) {
            vbeFree(pVesa->pVbe);
            return (FALSE);
        }
    }

    if (xf86LoadSubModule(pScrn, "fb") == NULL) {
        VESAFreeRec(pScrn);
        vbeFree(pVesa->pVbe);
        return (FALSE);
    }

    vbeFree(pVesa->pVbe);

    return (TRUE);
}

static Bool
vesaCreateScreenResources(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    VESAPtr pVesa = VESAGetRec(pScrn);
    Bool ret;

    pScreen->CreateScreenResources = pVesa->CreateScreenResources;
    ret = pScreen->CreateScreenResources(pScreen);
    pScreen->CreateScreenResources = vesaCreateScreenResources;

    shadowAdd(pScreen, pScreen->GetScreenPixmap(pScreen), pVesa->update,
              pVesa->window, 0, 0);

    return ret;
}

static void
vesaEnableDisableFBAccess(ScrnInfoPtr pScrn, Bool enable)
{
    VESAPtr pVesa = VESAGetRec(pScrn);

    pVesa->accessEnabled = enable;
    pVesa->EnableDisableFBAccess(pScrn, enable);
}

static Bool
VESAScreenInit(ScreenPtr pScreen, int argc, char **argv)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    VESAPtr pVesa = VESAGetRec(pScrn);
    VisualPtr visual;
    VbeModeInfoBlock *mode;
    int flags;

    if ((pVesa->pVbe = VBEExtendedInit(NULL, pVesa->pEnt->index,
                           SET_BIOS_SCRATCH | RESTORE_BIOS_SCRATCH)) == NULL)
        return (FALSE);

    if (pVesa->mapPhys == 0) {
        mode = ((VbeModeInfoData*)(pScrn->currentMode->Private))->data;
        pScrn->videoRam = pVesa->mapSize;
        pVesa->mapPhys = mode->PhysBasePtr;
        pVesa->mapOff = 0;
    }

    if (pVesa->mapPhys == 0) {
        pVesa->mapPhys = 0xa0000;
        pVesa->mapSize = 0x10000;
    }

    if (!VESAMapVidMem(pScrn)) {
        if (pVesa->mapPhys != 0xa0000) {
            pVesa->mapPhys = 0xa0000;
            pVesa->mapSize = 0x10000;
            if (!VESAMapVidMem(pScrn))
                return (FALSE);
        }
        else
            return (FALSE);
    }

    /* Set bpp to 8 for depth 4 when using shadowfb. */
    if (pVesa->shadowFB && pScrn->bitsPerPixel == 4)
        pScrn->bitsPerPixel = 8;

    if (pVesa->shadowFB) {
        pVesa->shadow = calloc(1, pScrn->displayWidth * pScrn->virtualY *
                                   ((pScrn->bitsPerPixel + 7) / 8));
        if (!pVesa->shadow) {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                       "Failed to allocate shadow buffer\n");
            return FALSE;
        }
    }

    /* save current video state */
    VESASaveRestore(pScrn, MODE_SAVE);
    pVesa->savedPal = VBESetGetPaletteData(pVesa->pVbe, FALSE, 0, 256,
                                           NULL, FALSE, FALSE);

    /* set first video mode */
    if (!VESASetMode(pScrn, pScrn->currentMode))
        return (FALSE);

    /* set the viewport */
    VESAAdjustFrame(pScrn, pScrn->frameX0, pScrn->frameY0);

    /* Blank the screen for aesthetic reasons. */
    VESASaveScreen(pScreen, SCREEN_SAVER_ON);

    /* mi layer */
    miClearVisualTypes();
    if (!xf86SetDefaultVisual(pScrn, -1))
        return (FALSE);
    if (pScrn->bitsPerPixel > 8) {
        if (!miSetVisualTypes(pScrn->depth, TrueColorMask, pScrn->rgbBits, TrueColor))
            return (FALSE);
    }
    else {
        if (!miSetVisualTypes(pScrn->depth,
                              miGetDefaultVisualMask(pScrn->depth),
                              pScrn->rgbBits, pScrn->defaultVisual))
            return (FALSE);
    }
    if (!miSetPixmapDepths())
        return (FALSE);

    mode = ((VbeModeInfoData*)pScrn->modes->Private)->data;
    switch (mode->MemoryModel) {
        case 0x4: /* Packed pixel */
        case 0x6: /* Direct Color */
            switch (pScrn->bitsPerPixel) {
                case 8:
                case 16:
                case 24:
                case 32:
                    if (!fbScreenInit(pScreen,
                                      pVesa->shadowFB ? pVesa->shadow : pVesa->base,
                                      pScrn->virtualX, pScrn->virtualY,
                                      pScrn->xDpi, pScrn->yDpi,
                                      pScrn->displayWidth, pScrn->bitsPerPixel))
                    return (FALSE);
                    break;
                default:
                    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                       "Unsupported bpp: %d", pScrn->bitsPerPixel);
                    return (FALSE);
            }
            break;
        default:
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                       "Unsupported Memory Model: %d", mode->MemoryModel);
            return (FALSE);
    }


    if (pScrn->bitsPerPixel > 8) {
        /* Fixup RGB ordering */
        visual = pScreen->visuals + pScreen->numVisuals;
        while (--visual >= pScreen->visuals) {
            if ((visual->class | DynamicClass) == DirectColor) {
                visual->offsetRed   = pScrn->offset.red;
                visual->offsetGreen = pScrn->offset.green;
                visual->offsetBlue  = pScrn->offset.blue;
                visual->redMask     = pScrn->mask.red;
                visual->greenMask   = pScrn->mask.green;
                visual->blueMask    = pScrn->mask.blue;
            }
        }
    }

    /* must be after RGB ordering fixed */
    fbPictureInit(pScreen, 0, 0);

    if (pVesa->shadowFB) {
        if (pVesa->mapPhys == 0xa0000) { /* Windowed */
            pVesa->update = vesaUpdatePacked;
            pVesa->window = VESAWindowWindowed;
        }
        else { /* Linear */
            pVesa->update = vesaUpdatePacked;
            pVesa->window = VESAWindowLinear;
        }

        if (!shadowSetup(pScreen))
            return FALSE;
        pVesa->CreateScreenResources = pScreen->CreateScreenResources;
        pScreen->CreateScreenResources = vesaCreateScreenResources;
    }
    else if (pVesa->mapPhys == 0xa0000) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                   "Banked framebuffer requires ShadowFB\n");
        return FALSE;
    }

    VESADGAInit(pScrn, pScreen);

    xf86SetBlackWhitePixels(pScreen);
    xf86SetBackingStore(pScreen);

    /* software cursor */
    miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

    /* colormap */
    if (!miCreateDefColormap(pScreen))
        return (FALSE);

    flags = CMAP_RELOAD_ON_MODE_SWITCH;

    if(!xf86HandleColormaps(pScreen, 256,
        pVesa->vbeInfo->Capabilities[0] & 0x01 ? 8 : 6,
        VESALoadPalette, NULL, flags))
        return (FALSE);

    pVesa->accessEnabled = TRUE;
    pVesa->EnableDisableFBAccess = pScrn->EnableDisableFBAccess;
    pScrn->EnableDisableFBAccess = vesaEnableDisableFBAccess;

    pVesa->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = VESACloseScreen;
    pScreen->SaveScreen = VESASaveScreen;

    xf86DPMSInit(pScreen, VESADisplayPowerManagementSet, 0);

    /* Report any unused options (only for the first generation) */
    if (serverGeneration == 1)
        xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);

    return (TRUE);
}

static Bool
VESAEnterVT(ScrnInfoPtr pScrn)
{
    if (!VESASetMode(pScrn, pScrn->currentMode))
        return FALSE;
    VESAAdjustFrame(pScrn, pScrn->frameX0, pScrn->frameY0);
    return TRUE;
}

static void
VESALeaveVT(ScrnInfoPtr pScrn)
{
    VESASaveRestore(pScrn, MODE_RESTORE);
}

static Bool
VESACloseScreen(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    VESAPtr pVesa = VESAGetRec(pScrn);

    if (pScrn->vtSema) {
        VESASaveRestore(pScrn, MODE_RESTORE);
        if (pVesa->savedPal)
            VBESetGetPaletteData(pVesa->pVbe, TRUE, 0, 256,
                                 pVesa->savedPal, FALSE, TRUE);
        VESAUnmapVidMem(pScrn);
    }
    if (pVesa->shadowFB && pVesa->shadow) {
        shadowRemove(pScreen, pScreen->GetScreenPixmap(pScreen));
        free(pVesa->shadow);
    }
    if (pVesa->pDGAMode) {
        free(pVesa->pDGAMode);
        pVesa->pDGAMode = NULL;
        pVesa->nDGAMode = 0;
    }
    pScrn->vtSema = FALSE;

    pScrn->EnableDisableFBAccess = pVesa->EnableDisableFBAccess;
    pScreen->CreateScreenResources = pVesa->CreateScreenResources;
    pScreen->CloseScreen = pVesa->CloseScreen;
    return pScreen->CloseScreen(pScreen);
}

static Bool
VESASwitchMode(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    VESAPtr pVesa = VESAGetRec(pScrn);
    Bool ret, disableAccess = pVesa->ModeSetClearScreen && pVesa->accessEnabled;

    if (disableAccess)
        pScrn->EnableDisableFBAccess(pScrn,FALSE);
    ret = VESASetMode(pScrn, mode);
    if (disableAccess)
        pScrn->EnableDisableFBAccess(pScrn,TRUE);
    return ret;
}

/* Set a graphics mode */
static Bool
VESASetMode(ScrnInfoPtr pScrn, DisplayModePtr pMode)
{
    VESAPtr pVesa;
    VbeModeInfoData *data;
    int mode;

    pVesa = VESAGetRec(pScrn);

    data = (VbeModeInfoData*)pMode->Private;

    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
               "Setting up VESA Mode 0x%X (%dx%d)\n",
               data->mode & 0x7FF, pMode->HDisplay, pMode->VDisplay);

    /* careful, setting the bit means don't clear the screen */
    mode = data->mode | (pVesa->ModeSetClearScreen ? 0 : (1U << 15));

    /* enable linear addressing */
    if (pVesa->mapPhys != 0xa0000)
        mode |= 1 << 14;

    if (VBESetVBEMode(pVesa->pVbe, mode, data->block) == FALSE) {
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "VBESetVBEMode failed");
        if ((data->block || (data->mode & (1 << 11))) &&
            VBESetVBEMode(pVesa->pVbe, (mode & ~(1 << 11)), NULL) == TRUE) {
            /* Some cards do not like setting the clock.
             * Free it as it will not be any longer useful
             */
            xf86ErrorF(", mode set without customized refresh.\n");
            free(data->block);
            data->block = NULL;
            data->mode &= ~(1 << 11);
        }
        else {
            ErrorF("\n");
            return (FALSE);
        }
    }

    pVesa->bankSwitchWindowB =
        !((data->data->WinBSegment == 0) && (data->data->WinBAttributes == 0));

    if (data->data->XResolution != pScrn->displayWidth)
        VBESetLogicalScanline(pVesa->pVbe, pScrn->displayWidth);

    if (pScrn->bitsPerPixel == 8 && pVesa->vbeInfo->Capabilities[0] & 0x01 &&
        !(data->data->MemoryModel == 0x6 || data->data->MemoryModel == 0x7))
        VBESetGetDACPaletteFormat(pVesa->pVbe, 8);

    pScrn->vtSema = TRUE;

    return (TRUE);
}

static void
VESAAdjustFrame(ScrnInfoPtr pScrn, int x, int y)
{
    VESAPtr pVesa = VESAGetRec(pScrn);

    VBESetDisplayStart(pVesa->pVbe, x, y, TRUE);
}

static void
VESAFreeScreen(ScrnInfoPtr pScrn)
{
    VESAFreeRec(pScrn);
}

static Bool
VESAMapVidMem(ScrnInfoPtr pScrn)
{
    VESAPtr pVesa = VESAGetRec(pScrn);

    if (pVesa->base != NULL)
        return (TRUE);

    pScrn->memPhysBase = pVesa->mapPhys;
    pScrn->fbOffset = pVesa->mapOff;

    if (pVesa->pciInfo != NULL) {
        if (pVesa->mapPhys != 0xa0000) {
            (void) pci_device_map_range(pVesa->pciInfo, pScrn->memPhysBase,
                                        pVesa->mapSize,
                                        (PCI_DEV_MAP_FLAG_WRITABLE
                                         | PCI_DEV_MAP_FLAG_WRITE_COMBINE),
                                        & pVesa->base);

            if (pVesa->base)
                (void) pci_device_map_legacy(pVesa->pciInfo, 0xa0000, 0x10000,
                                             PCI_DEV_MAP_FLAG_WRITABLE,
                                             & pVesa->VGAbase);
        }
        else {
            (void) pci_device_map_legacy(pVesa->pciInfo, pScrn->memPhysBase,
                                         pVesa->mapSize,
                                         PCI_DEV_MAP_FLAG_WRITABLE,
                                         & pVesa->base);

            if (pVesa->base)
                pVesa->VGAbase = pVesa->base;
        }
    }

    pVesa->ioBase = 0;

    xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, DEBUG_VERB,
                   "virtual address = %p, VGAbase = %p\n"
                   "\tphysical address = 0x%lx, size = %ld\n",
                   pVesa->base, pVesa->VGAbase,
                   pScrn->memPhysBase, pVesa->mapSize);

    return (pVesa->base != NULL);
}

static void
VESAUnmapVidMem(ScrnInfoPtr pScrn)
{
    VESAPtr pVesa = VESAGetRec(pScrn);

    if (pVesa->base == NULL)
        return;

    if (pVesa->mapPhys != 0xa0000) {
        (void) pci_device_unmap_range(pVesa->pciInfo, pVesa->base,
                                      pVesa->mapSize);
        (void) pci_device_unmap_legacy(pVesa->pciInfo, pVesa->VGAbase,
                                       0x10000);
    }
    else {
        (void) pci_device_unmap_legacy(pVesa->pciInfo, pVesa->base,
                                       pVesa->mapSize);
    }
    pVesa->base = NULL;
}

/* This code works, but is very slow for programs that use it intensively */
static void
VESALoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices,
                LOCO *colors, VisualPtr pVisual)
{
    VESAPtr pVesa = VESAGetRec(pScrn);
    int i, idx;
    int base;

    if (!pVesa->savedPal) {
#define VESADACDelay() \
        do {                                                                   \
           (void)inb(pVesa->ioBase + VGA_IOBASE_COLOR + VGA_IN_STAT_1_OFFSET); \
           (void)inb(pVesa->ioBase + VGA_IOBASE_COLOR + VGA_IN_STAT_1_OFFSET); \
        } while (0)

        for (i = 0; i < numColors; i++) {
            idx = indices[i];
            outb(pVesa->ioBase + VGA_DAC_WRITE_ADDR, idx);
            VESADACDelay();
            outb(pVesa->ioBase + VGA_DAC_DATA, colors[idx].red);
            VESADACDelay();
            outb(pVesa->ioBase + VGA_DAC_DATA, colors[idx].green);
            VESADACDelay();
            outb(pVesa->ioBase + VGA_DAC_DATA, colors[idx].blue);
            VESADACDelay();
        }
        return;
    }

    if (pVesa->pal == NULL)
        pVesa->pal = calloc(1, sizeof(CARD32) * 256);

    for (i = 0, base = idx = indices[i]; i < numColors; i++, idx++) {
        int j = indices[i];

        if (j < 0 || j >= 256)
            continue;
        pVesa->pal[j] = colors[j].blue |
                        (colors[j].green << 8) |
                        (colors[j].red << 16);
        if (j != idx) {
            VBESetGetPaletteData(pVesa->pVbe, TRUE, base, idx - base,
                                 pVesa->pal + base, FALSE, TRUE);
            idx = base = j;
        }
    }

    if (idx - 1 == indices[i - 1])
        VBESetGetPaletteData(pVesa->pVbe, TRUE, base, idx - base,
                             pVesa->pal + base, FALSE, TRUE);
}

/*
 * Just adapted from the std* functions in vgaHW.c
 */
static void
WriteAttr(VESAPtr pVesa, int index, int value)
{
    (void) inb(pVesa->ioBase + VGA_IOBASE_COLOR + VGA_IN_STAT_1_OFFSET);

    index |= 0x20;
    outb(pVesa->ioBase + VGA_ATTR_INDEX, index);
    outb(pVesa->ioBase + VGA_ATTR_DATA_W, value);
}

static int
ReadAttr(VESAPtr pVesa, int index)
{
    (void) inb(pVesa->ioBase + VGA_IOBASE_COLOR + VGA_IN_STAT_1_OFFSET);

    index |= 0x20;
    outb(pVesa->ioBase + VGA_ATTR_INDEX, index);
    return (inb(pVesa->ioBase + VGA_ATTR_DATA_R));
}

#define WriteMiscOut(value)     outb(pVesa->ioBase + VGA_MISC_OUT_W, value)
#define ReadMiscOut()           inb(pVesa->ioBase + VGA_MISC_OUT_R)
#define WriteSeq(index, value)  outb(pVesa->ioBase + VGA_SEQ_INDEX, index);\
                                outb(pVesa->ioBase + VGA_SEQ_DATA, value)

static int
ReadSeq(VESAPtr pVesa, int index)
{
    outb(pVesa->ioBase + VGA_SEQ_INDEX, index);

    return (inb(pVesa->ioBase + VGA_SEQ_DATA));
}

#define WriteGr(index, value) \
    outb(pVesa->ioBase + VGA_GRAPH_INDEX, index); \
    outb(pVesa->ioBase + VGA_GRAPH_DATA, value)

static int
ReadGr(VESAPtr pVesa, int index)
{
    outb(pVesa->ioBase + VGA_GRAPH_INDEX, index);

    return (inb(pVesa->ioBase + VGA_GRAPH_DATA));
}

static void
SeqReset(VESAPtr pVesa, Bool start)
{
    if (start) {
        WriteSeq(0x00, 0x01); /* Synchronous Reset */
    }
    else {
        WriteSeq(0x00, 0x03); /* End Reset */
    }
}

static void
SaveFonts(ScrnInfoPtr pScrn)
{
    VESAPtr pVesa = VESAGetRec(pScrn);
    unsigned char miscOut, attr10, gr4, gr5, gr6, seq2, seq4, scrn;

    if (pVesa->fonts != NULL)
        return;

    /* If in graphics mode, don't save anything */
    attr10 = ReadAttr(pVesa, 0x10);
    if (attr10 & 0x01)
        return;

    pVesa->fonts = malloc(16384);

    /* save the registers that are needed here */
    miscOut = ReadMiscOut();
    gr4 = ReadGr(pVesa, 0x04);
    gr5 = ReadGr(pVesa, 0x05);
    gr6 = ReadGr(pVesa, 0x06);
    seq2 = ReadSeq(pVesa, 0x02);
    seq4 = ReadSeq(pVesa, 0x04);

    /* Force into colour mode */
    WriteMiscOut(miscOut | 0x01);

    scrn = ReadSeq(pVesa, 0x01) | 0x20;
    SeqReset(pVesa, TRUE);
    WriteSeq(0x01, scrn);
    SeqReset(pVesa, FALSE);

    WriteAttr(pVesa, 0x10, 0x01); /* graphics mode */

    /*font1 */
    WriteSeq(0x02, 0x04); /* write to plane 2 */
    WriteSeq(0x04, 0x06); /* enable plane graphics */
    WriteGr(0x04, 0x02);  /* read plane 2 */
    WriteGr(0x05, 0x00);  /* write mode 0, read mode 0 */
    WriteGr(0x06, 0x05);  /* set graphics */
    slowbcopy_frombus(pVesa->VGAbase, pVesa->fonts, 8192);

    /* font2 */
    WriteSeq(0x02, 0x08); /* write to plane 3 */
    WriteSeq(0x04, 0x06); /* enable plane graphics */
    WriteGr(0x04, 0x03);  /* read plane 3 */
    WriteGr(0x05, 0x00);  /* write mode 0, read mode 0 */
    WriteGr(0x06, 0x05); /* set graphics */
    slowbcopy_frombus(pVesa->VGAbase, pVesa->fonts + 8192, 8192);

    scrn = ReadSeq(pVesa, 0x01) & ~0x20;
    SeqReset(pVesa, TRUE);
    WriteSeq(0x01, scrn);
    SeqReset(pVesa, FALSE);

    /* Restore clobbered registers */
    WriteAttr(pVesa, 0x10, attr10);
    WriteSeq(0x02, seq2);
    WriteSeq(0x04, seq4);
    WriteGr(0x04, gr4);
    WriteGr(0x05, gr5);
    WriteGr(0x06, gr6);
    WriteMiscOut(miscOut);
}

static void
RestoreFonts(ScrnInfoPtr pScrn)
{
    VESAPtr pVesa = VESAGetRec(pScrn);
    unsigned char miscOut, attr10, gr1, gr3, gr4, gr5, gr6, gr8, seq2, seq4, scrn;

    if (pVesa->fonts == NULL)
        return;

    if (pVesa->mapPhys == 0xa0000 && pVesa->curBank != 0)
        VESABankSwitch(pScrn->pScreen, 0);

    /* save the registers that are needed here */
    miscOut = ReadMiscOut();
    attr10 = ReadAttr(pVesa, 0x10);
    gr1 = ReadGr(pVesa, 0x01);
    gr3 = ReadGr(pVesa, 0x03);
    gr4 = ReadGr(pVesa, 0x04);
    gr5 = ReadGr(pVesa, 0x05);
    gr6 = ReadGr(pVesa, 0x06);
    gr8 = ReadGr(pVesa, 0x08);
    seq2 = ReadSeq(pVesa, 0x02);
    seq4 = ReadSeq(pVesa, 0x04);

    /* Force into colour mode */
    WriteMiscOut(miscOut | 0x01);

    scrn = ReadSeq(pVesa, 0x01) | 0x20;
    SeqReset(pVesa, TRUE);
    WriteSeq(0x01, scrn);
    SeqReset(pVesa, FALSE);

    WriteAttr(pVesa, 0x10, 0x01); /* graphics mode */
    if (pScrn->depth == 4) {
        /* GJA */
        WriteGr(0x03, 0x00); /* don't rotate, write unmodified */
        WriteGr(0x08, 0xFF); /* write all bits in a byte */
        WriteGr(0x01, 0x00); /* all planes come from CPU */
    }

    WriteSeq(0x02, 0x04);   /* write to plane 2 */
    WriteSeq(0x04, 0x06);   /* enable plane graphics */
    WriteGr(0x04, 0x02);    /* read plane 2 */
    WriteGr(0x05, 0x00);    /* write mode 0, read mode 0 */
    WriteGr(0x06, 0x05);    /* set graphics */
    slowbcopy_tobus(pVesa->fonts, pVesa->VGAbase, 8192);

    WriteSeq(0x02, 0x08);   /* write to plane 3 */
    WriteSeq(0x04, 0x06);   /* enable plane graphics */
    WriteGr(0x04, 0x03);    /* read plane 3 */
    WriteGr(0x05, 0x00);    /* write mode 0, read mode 0 */
    WriteGr(0x06, 0x05);    /* set graphics */
    slowbcopy_tobus(pVesa->fonts + 8192, pVesa->VGAbase, 8192);

    scrn = ReadSeq(pVesa, 0x01) & ~0x20;
    SeqReset(pVesa, TRUE);
    WriteSeq(0x01, scrn);
    SeqReset(pVesa, FALSE);

    /* restore the registers that were changed */
    WriteMiscOut(miscOut);
    WriteAttr(pVesa, 0x10, attr10);
    WriteGr(0x01, gr1);
    WriteGr(0x03, gr3);
    WriteGr(0x04, gr4);
    WriteGr(0x05, gr5);
    WriteGr(0x06, gr6);
    WriteGr(0x08, gr8);
    WriteSeq(0x02, seq2);
    WriteSeq(0x04, seq4);
}

static Bool
VESASaveScreen(ScreenPtr pScreen, int mode)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    VESAPtr pVesa = VESAGetRec(pScrn);
    Bool on = xf86IsUnblank(mode);

    if (on)
        SetTimeSinceLastInputEvent();

    if (pScrn->vtSema) {
        unsigned char scrn = ReadSeq(pVesa, 0x01);

        if (on)
            scrn &= ~0x20;
        else
            scrn |= 0x20;
        SeqReset(pVesa, TRUE);
        WriteSeq(0x01, scrn);
        SeqReset(pVesa, FALSE);
    }

    return (TRUE);
}

static int
VESABankSwitch(ScreenPtr pScreen, unsigned int iBank)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    VESAPtr pVesa = VESAGetRec(pScrn);

    if (pVesa->curBank == iBank)
        return (0);
    if (!VBEBankSwitch(pVesa->pVbe, iBank, 0))
        return (1);
    if (pVesa->bankSwitchWindowB) {
        if (!VBEBankSwitch(pVesa->pVbe, iBank, 1))
        return (1);
    }
    pVesa->curBank = iBank;

    return (0);
}

Bool
VESASaveRestore(ScrnInfoPtr pScrn, vbeSaveRestoreFunction function)
{
    VESAPtr pVesa;

    if (function < MODE_QUERY || function > MODE_RESTORE)
        return (FALSE);

    pVesa = VESAGetRec(pScrn);


    /* Query amount of memory to save state */
    if (function == MODE_QUERY ||
        (function == MODE_SAVE && pVesa->state == NULL)) {

        /* Make sure we save at least this information in case of failure */
        (void)VBEGetVBEMode(pVesa->pVbe, &pVesa->stateMode);
        SaveFonts(pScrn);

        if (pVesa->major > 1) {
            if (!VBESaveRestore(pVesa->pVbe,function,(void*)&pVesa->state,
                                &pVesa->stateSize,&pVesa->statePage))
                return FALSE;
        }
    }

    /* Save/Restore Super VGA state */
    if (function != MODE_QUERY) {
        Bool retval = TRUE;

        if (pVesa->major > 1) {
            if (function == MODE_RESTORE)
                memcpy(pVesa->state, pVesa->pstate, pVesa->stateSize);

            if ((retval = VBESaveRestore(pVesa->pVbe,function,
                                         (void*)&pVesa->state,
                                         &pVesa->stateSize,&pVesa->statePage))
                        && function == MODE_SAVE) {
                /* don't rely on the memory not being touched */
                if (pVesa->pstate == NULL)
                    pVesa->pstate = malloc(pVesa->stateSize);
                memcpy(pVesa->pstate, pVesa->state, pVesa->stateSize);
            }
        }

        if (function == MODE_RESTORE) {
            VBESetVBEMode(pVesa->pVbe, pVesa->stateMode, NULL);
            RestoreFonts(pScrn);
        }

        if (!retval)
        return (FALSE);

    }

    return (TRUE);
}

static void
VESADisplayPowerManagementSet(ScrnInfoPtr pScrn, int mode,
                int flags)
{
    VESAPtr pVesa = VESAGetRec(pScrn);

    if (!pScrn->vtSema)
        return;

    VBEDPMSSet(pVesa->pVbe, mode);
}

/***********************************************************************
 * DGA stuff
 ***********************************************************************/
static Bool VESADGAOpenFramebuffer(ScrnInfoPtr pScrn, char **DeviceName,
                                   unsigned char **ApertureBase,
                                   int *ApertureSize, int *ApertureOffset,
                                   int *flags);
static Bool VESADGASetMode(ScrnInfoPtr pScrn, DGAModePtr pDGAMode);
static void VESADGASetViewport(ScrnInfoPtr pScrn, int x, int y, int flags);

static Bool
VESADGAOpenFramebuffer(ScrnInfoPtr pScrn, char **DeviceName,
                       unsigned char **ApertureBase, int *ApertureSize,
                       int *ApertureOffset, int *flags)
{
    VESAPtr pVesa = VESAGetRec(pScrn);

    *DeviceName = NULL; /* No special device */
    *ApertureBase = (unsigned char *)(long)(pVesa->mapPhys);
    *ApertureSize = pVesa->mapSize;
    *ApertureOffset = pVesa->mapOff;
    *flags = DGA_NEED_ROOT;

    return (TRUE);
}

static Bool
VESADGASetMode(ScrnInfoPtr pScrn, DGAModePtr pDGAMode)
{
    DisplayModePtr pMode;
    int frameX0, frameY0;

    if (pDGAMode) {
        pMode = pDGAMode->mode;
        frameX0 = frameY0 = 0;
    }
    else {
        if (!(pMode = pScrn->currentMode))
            return (TRUE);

        frameX0 = pScrn->frameX0;
        frameY0 = pScrn->frameY0;
    }

    if (!(*pScrn->SwitchMode)(pScrn, pMode))
        return (FALSE);
    (*pScrn->AdjustFrame)(pScrn, frameX0, frameY0);

    return (TRUE);
}

static void
VESADGASetViewport(ScrnInfoPtr pScrn, int x, int y, int flags)
{
    (*pScrn->AdjustFrame)(pScrn, x, y);
}

static int
VESADGAGetViewport(ScrnInfoPtr pScrn)
{
    return (0);
}

static DGAFunctionRec VESADGAFunctions =
{
    VESADGAOpenFramebuffer,
    NULL,       /* CloseFramebuffer */
    VESADGASetMode,
    VESADGASetViewport,
    VESADGAGetViewport,
    NULL,       /* Sync */
    NULL,       /* FillRect */
    NULL,       /* BlitRect */
    NULL,       /* BlitTransRect */
};

static void
VESADGAAddModes(ScrnInfoPtr pScrn)
{
    VESAPtr pVesa = VESAGetRec(pScrn);
    DisplayModePtr pMode = pScrn->modes;
    DGAModePtr pDGAMode;

    do {
        pDGAMode = realloc(pVesa->pDGAMode,
                           (pVesa->nDGAMode + 1) * sizeof(DGAModeRec));
        if (!pDGAMode)
            break;

        pVesa->pDGAMode = pDGAMode;
        pDGAMode += pVesa->nDGAMode;
        (void)memset(pDGAMode, 0, sizeof(DGAModeRec));

        ++pVesa->nDGAMode;
        pDGAMode->mode = pMode;
        pDGAMode->flags = DGA_CONCURRENT_ACCESS | DGA_PIXMAP_AVAILABLE;
        pDGAMode->byteOrder = pScrn->imageByteOrder;
        pDGAMode->depth = pScrn->depth;
        pDGAMode->bitsPerPixel = pScrn->bitsPerPixel;
        pDGAMode->red_mask = pScrn->mask.red;
        pDGAMode->green_mask = pScrn->mask.green;
        pDGAMode->blue_mask = pScrn->mask.blue;
        pDGAMode->visualClass = pScrn->bitsPerPixel > 8 ?
            TrueColor : PseudoColor;
        pDGAMode->xViewportStep = 1;
        pDGAMode->yViewportStep = 1;
        pDGAMode->viewportWidth = pMode->HDisplay;
        pDGAMode->viewportHeight = pMode->VDisplay;
        pDGAMode->bytesPerScanline = pVesa->maxBytesPerScanline;
        pDGAMode->imageWidth = pMode->HDisplay;
        pDGAMode->imageHeight =  pMode->VDisplay;
        pDGAMode->pixmapWidth = pDGAMode->imageWidth;
        pDGAMode->pixmapHeight = pDGAMode->imageHeight;
        pDGAMode->maxViewportX = pScrn->virtualX -
                                 pDGAMode->viewportWidth;
        pDGAMode->maxViewportY = pScrn->virtualY -
                                 pDGAMode->viewportHeight;
        pDGAMode->address = pVesa->base;

        pMode = pMode->next;
    } while (pMode != pScrn->modes);
}

static Bool
VESADGAInit(ScrnInfoPtr pScrn, ScreenPtr pScreen)
{
    VESAPtr pVesa = VESAGetRec(pScrn);

    if (pScrn->depth < 8 || pVesa->mapPhys == 0xa0000L)
        return (FALSE);

    if (!pVesa->nDGAMode)
        VESADGAAddModes(pScrn);

    return (DGAInit(pScreen, &VESADGAFunctions,
            pVesa->pDGAMode, pVesa->nDGAMode));
}
