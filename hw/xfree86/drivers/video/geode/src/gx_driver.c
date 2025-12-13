/* Copyright (c) 2003-2006 Advanced Micro Devices, Inc.
 *
 * Portioned modeled from xf86-video-intel/src/i830_driver.c
 * Copyright 2001 VA Linux Systems Inc., Fremont, California.
 * Copyright \ufffd 2002 by David Dawes

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Neither the name of the Advanced Micro Devices, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 */
#include <xorg-config.h>

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86cmap.h"
#include "compiler.h"
#include "mipointer.h"
#include "shadow.h"
#include <X11/extensions/randr.h>
#include "fb.h"
#include "miscstruct.h"
#include "micmap.h"
#include "vbe.h"
#include "fb.h"
#include "randrstr.h"

#include "geode.h"
#include "gfx_defs.h"
#include "gfx_regs.h"
#include "panel.h"

/* Bring in VGA functions */
#include "gx_vga.c"

#define GX_MIN_PITCH 1024
#define GX_MAX_PITCH 8192
#define GX_MAX_WIDTH  1600
#define GX_MIN_HEIGHT  400
#define GX_MAX_HEIGHT 1200
#define GX_CB_PITCH   544
#define GX_CB_SIZE    544

#define GX_CPU_REG_SIZE 0x4000
#define GX_GP_REG_SIZE  0x4000
#define GX_VID_REG_SIZE 0x4000

extern OptionInfoRec GX_GeodeOptions[];

extern unsigned char *XpressROMPtr;

static inline void
gx_enable_dac_power(void)
{
    gfx_write_vid32(RCDF_VID_MISC,
                    gfx_read_vid32(RCDF_VID_MISC) & RCDF_GAMMA_BYPASS_BOTH);
}

static inline void
gx_disable_dac_power(void)
{
    gfx_write_vid32(RCDF_VID_MISC,
                    RCDF_DAC_POWER_DOWN | RCDF_ANALOG_POWER_DOWN |
                    (gfx_read_vid32(RCDF_VID_MISC) & RCDF_GAMMA_BYPASS_BOTH));
}

static void
GXInitEXAMemory(ScrnInfoPtr pScrni, unsigned int *offset, unsigned int *avail)
{
    GeodePtr pGeode = GEODEPTR(pScrni);

    if (pGeode->exaBfrSz > 0 && pGeode->exaBfrSz <= *avail) {
        pGeode->exaBfrOffset = *offset;
        *offset += pGeode->exaBfrOffset;
        *avail -= pGeode->exaBfrOffset;
    }
}

static Bool
GXAllocateMemory(ScreenPtr pScrn, ScrnInfoPtr pScrni, int rotate)
{
    GeodePtr pGeode = GEODEPTR(pScrni);

    unsigned int fboffset, fbavail;
    unsigned int size;
    unsigned int bytpp = (pScrni->bitsPerPixel + 7) / 8;
    BOOL ret = TRUE;

    if (pGeode->tryCompression)
        pGeode->displayPitch =
            GeodeCalculatePitchBytes(pScrni->virtualX, pScrni->bitsPerPixel);
    else
        pGeode->displayPitch =
            ((pScrni->virtualX + 3) & ~3) * (pScrni->bitsPerPixel >> 3);

    pGeode->Pitch = pGeode->displayPitch;
    pGeode->displayWidth = pGeode->displayPitch / bytpp;
    pScrni->displayWidth = pGeode->displayWidth;

    fbavail = pGeode->FBAvail - 0x4000;

    pGeode->displayOffset = fboffset = 0;
    pGeode->displaySize = pScrni->virtualY * pGeode->displayPitch;

    fbavail -= pGeode->displaySize;
    fboffset += pGeode->displaySize;

    if (pGeode->tryCompression) {
        size = pScrni->virtualY * GX_CB_PITCH;

        if (size <= fbavail) {
            pGeode->CBData.compression_offset = fboffset;

            fboffset += size;
            fbavail -= size;

            pGeode->Compression = TRUE;
        }
        else {
            xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                       "Not enough memory for compression\n");
            pGeode->Compression = FALSE;
        }
    }

    if (pGeode->tryHWCursor) {

        if (fbavail >= 1024) {
            pGeode->CursorStartOffset = fboffset;
            fboffset += 1024;
            fbavail -= 1024;
            pGeode->HWCursor = TRUE;
        }
        else {
            xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                       "Not enough memory for the hardware cursor\n");
            pGeode->HWCursor = FALSE;
        }
    }

    if (!pGeode->NoAccel && pGeode->useEXA)
        GXInitEXAMemory(pScrni, &fboffset, &fbavail);

    pGeode->shadowSize = 0;

    if (rotate != RR_Rotate_0) {
        if (rotate & (RR_Rotate_90 | RR_Rotate_270))
            size = pGeode->displayPitch * pScrni->virtualX;
        else
            size = pGeode->displayPitch * pScrni->virtualY;

        if (size <= fbavail) {
            pGeode->shadowOffset = fboffset;
            pGeode->shadowSize = size;

            fboffset += size;
            fbavail -= size;
        }
        else {
            xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                       "Not enough memory for the shadow framebuffer\n");
            ret = FALSE;
        }
    }

    /* XAA always exists - we can't remove it on demand like we can with EXA.
     * So we assume the worse, and only give XAA enough offspace room to
     * account for any eventuality that RandR might throw at us. */

    if (!pGeode->NoAccel) {

        if (pGeode->useEXA && pGeode->pExa) {
            ExaDriverPtr pExa = pGeode->pExa;

            pExa->offScreenBase = fboffset;
            pExa->memorySize = fboffset + fbavail;
        }
    }
    return ret;
}

static Bool
GXSaveScreen(ScreenPtr pScrn, int mode)
{
    ScrnInfoPtr pScrni = xf86ScreenToScrn(pScrn);
    GeodePtr pGeode = GEODEPTR(pScrni);

    if (pGeode->useVGA && !pScrni->vtSema)
        return vgaHWSaveScreen(pScrn, mode);

    return TRUE;
}

/* Common function - used by the LX too */
#ifdef XSERVER_LIBPCIACCESS
static inline void *
map_pci_mem(ScrnInfoPtr pScrni, int vram,
            struct pci_device *dev, int bar, int size)
{
    void *ptr;
    int map_size = size ? size : dev->regions[bar].size;

    int err = pci_device_map_range(dev,
                                   dev->regions[bar].base_addr,
                                   map_size,
                                   PCI_DEV_MAP_FLAG_WRITABLE |
                                   (vram ? PCI_DEV_MAP_FLAG_WRITE_COMBINE : 0),
                                   &ptr);

    if (err)
        return NULL;
    return ptr;
}

static inline int
unmap_pci_mem(ScrnInfoPtr pScrni, struct pci_device *dev, void *ptr, int size)
{
    return pci_device_unmap_range(dev, ptr, size);
}
#endif

extern unsigned long gfx_gx2_scratch_base;

static Bool
GXMapMem(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    int index = pScrni->scrnIndex;

    pciVideoPtr pci = xf86GetPciInfoForEntity(pGeode->pEnt->index);

#ifndef XSERVER_LIBPCIACCESS
    gfx_virt_regptr = (unsigned char *) xf86MapVidMem(index, VIDMEM_MMIO,
                                                      pci->memBase[2],
                                                      pci->size[2]);

    gfx_virt_gpptr = (unsigned char *) xf86MapVidMem(index, VIDMEM_MMIO,
                                                     pci->memBase[1],
                                                     pci->size[1]);

    gfx_virt_vidptr = (unsigned char *) xf86MapVidMem(index, VIDMEM_MMIO,
                                                      pci->memBase[3],
                                                      pci->size[3]);

    gfx_virt_fbptr = (unsigned char *) xf86MapVidMem(index, VIDMEM_FRAMEBUFFER,
                                                     pci->memBase[0],
                                                     pGeode->FBAvail);
#else
    gfx_virt_regptr = map_pci_mem(pScrni, 0, pci, 2, 0);
    gfx_virt_gpptr = map_pci_mem(pScrni, 0, pci, 1, 0);
    gfx_virt_vidptr = map_pci_mem(pScrni, 0, pci, 3, 0);
    gfx_virt_fbptr = map_pci_mem(pScrni, 1, pci, 0, pGeode->FBAvail);
#endif

    gfx_gx2_scratch_base = pGeode->FBAvail - 0x4000;

#ifndef XSERVER_LIBPCIACCESS
    XpressROMPtr = xf86MapVidMem(index, VIDMEM_FRAMEBUFFER, 0xF0000, 0x10000);
#else
    {
        int fd = open("/dev/mem", O_RDWR);

        if (fd < 0) {
            xf86DrvMsg(index, X_ERROR, "Failed to open /dev/mem: %m\n");
            return FALSE;
        }
        XpressROMPtr =
            mmap(NULL, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                 0xF0000);
        close(fd);
    }
#endif
    pGeode->FBBase = gfx_virt_fbptr;

    if ((!gfx_virt_regptr) || (!gfx_virt_gpptr) ||
        (!gfx_virt_vidptr) || (!gfx_virt_fbptr))
        return FALSE;

    if (!pGeode->NoAccel && pGeode->useEXA)
        pGeode->pExa->memoryBase = pGeode->FBBase;

    xf86DrvMsg(index, X_INFO, "Found Geode %x %p\n",
               pGeode->FBAvail, pGeode->FBBase);

    return TRUE;
}

/* Check to see if VGA exists - we map the space and look for a
   signature - if it doesn't match exactly, then we assume no VGA.
*/

static Bool
GXCheckVGA(ScrnInfoPtr pScrni, EntityInfoPtr pEnt)
{
#ifndef XSERVER_LIBPCIACCESS
    unsigned char *ptr;
    const char *vgasig = "IBM VGA Compatible";
    int ret;

    ptr =
        xf86MapVidMem(pScrni->scrnIndex, VIDMEM_FRAMEBUFFER, 0xC001E,
                      strlen(vgasig));

    if (ptr == NULL)
        return FALSE;

    ret = memcmp(ptr, vgasig, strlen(vgasig));
    xf86UnMapVidMem(pScrni->scrnIndex, (pointer) ptr, strlen(vgasig));

    return ret ? FALSE : TRUE;
#else
    pciVideoPtr pci = xf86GetPciInfoForEntity(pEnt->index);

    return pci_device_is_boot_vga(pci);
#endif
}

static Bool
GXPreInit(ScrnInfoPtr pScrni, int flags)
{
    GeodePtr pGeode;
    ClockRangePtr GeodeClockRange;
    OptionInfoRec *GeodeOptions = &GX_GeodeOptions[0];
    int ret;
    QQ_WORD msrValue;
    EntityInfoPtr pEnt;
    rgb defaultWeight = { 0, 0, 0 };
    int modecnt;
    const char *s, *panelgeo;
    Bool useVGA;

    if (pScrni->numEntities != 1)
        return FALSE;

    pEnt = xf86GetEntityInfo(pScrni->entityList[0]);
#ifndef XSERVER_LIBPCIACCESS
    if (pEnt->resources)
        return FALSE;
#endif

    pGeode = pScrni->driverPrivate = XNFcallocarray(1, sizeof(GeodeRec));

    if (pGeode == NULL)
        return FALSE;

    useVGA = GXCheckVGA(pScrni, pEnt);

    if (flags & PROBE_DETECT) {
        GeodeProbeDDC(pScrni, pEnt->index);
        return TRUE;
    }

    /* Probe for VGA */
    pGeode->useVGA = useVGA;
    pGeode->pEnt = pEnt;

    if (pGeode->useVGA) {
        if (!xf86LoadSubModule(pScrni, "vgahw") || !vgaHWGetHWRec(pScrni))
            pGeode->useVGA = FALSE;
        else
            vgaHWSetStdFuncs(VGAHWPTR(pScrni));

#if INT10_SUPPORT
        pGeode->vesa = calloc(1, sizeof(VESARec));
#endif
    }

    gfx_msr_init();

    ret = gfx_msr_read(RC_ID_DF, MBD_MSR_CONFIG, &msrValue);

    if (!ret) {
        pGeode->Output =
            ((msrValue.low & RCDF_CONFIG_FMT_MASK) ==
             RCDF_CONFIG_FMT_FP) ? OUTPUT_PANEL : OUTPUT_CRT;
    }

    /* Fill in the monitor information */
    pScrni->monitor = pScrni->confScreen->monitor;

    if (!xf86SetDepthBpp(pScrni, 16, 16, 16, Support24bppFb | Support32bppFb))
        return FALSE;

    switch (pScrni->depth) {
    case 8:
        pScrni->rgbBits = 8;
    case 16:
    case 24:
    case 32:
        break;
    default:
        xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                   "The driver does not support %d as a depth.\n",
                   pScrni->depth);
        return FALSE;
    }

    xf86PrintDepthBpp(pScrni);

    if (!xf86SetWeight(pScrni, defaultWeight, defaultWeight))
        return FALSE;

    if (!xf86SetDefaultVisual(pScrni, -1))
        return FALSE;

    /*
     * If the driver can do gamma correction, it should call xf86SetGamma()
     * here.
     */
    {
        Gamma zeros = { 0.0, 0.0, 0.0 };

        if (!xf86SetGamma(pScrni, zeros)) {
            return FALSE;
        }
    }

    pScrni->progClock = TRUE;
    xf86CollectOptions(pScrni, NULL);
    xf86ProcessOptions(pScrni->scrnIndex, pScrni->options, GeodeOptions);

    /* Set up our various options that may get reversed as we go on */

    pGeode->FBVGAActive = FALSE;
    pGeode->tryHWCursor = TRUE;
    pGeode->tryCompression = TRUE;

    pGeode->NoAccel = FALSE;
    pGeode->useEXA = FALSE;

    pGeode->Panel = (pGeode->Output & OUTPUT_PANEL) ? TRUE : FALSE;

    pGeode->exaBfrSz = DEFAULT_EXA_SCRATCH_BFRSZ;

    xf86GetOptValBool(GeodeOptions, GX_OPTION_HW_CURSOR, &pGeode->tryHWCursor);

    if (!xf86GetOptValInteger(GeodeOptions, GX_OPTION_FBSIZE,
                              (int *) &(pGeode->FBAvail)))
        pGeode->FBAvail = 0;

    /* For compatibility - allow SWCursor too */

    if (xf86ReturnOptValBool(GeodeOptions, GX_OPTION_SW_CURSOR, FALSE))
        pGeode->tryHWCursor = FALSE;

    if (xf86ReturnOptValBool(GeodeOptions, GX_OPTION_NOCOMPRESSION, FALSE))
        pGeode->tryCompression = FALSE;

    if (xf86ReturnOptValBool(GeodeOptions, GX_OPTION_NOACCEL, FALSE))
        pGeode->NoAccel = TRUE;

    pGeode->rotation = RR_Rotate_0;

    if ((s = xf86GetOptValString(GeodeOptions, GX_OPTION_ROTATE))) {

        if (!xf86NameCmp(s, "LEFT"))
            pGeode->rotation = RR_Rotate_90;
        else if (!xf86NameCmp(s, "INVERT"))
            pGeode->rotation = RR_Rotate_180;
        else if (!xf86NameCmp(s, "CCW"))
            pGeode->rotation = RR_Rotate_270;
        else
            xf86DrvMsg(pScrni->scrnIndex, X_ERROR, "Invalid rotation %s.\n", s);
    }

    if ((s = xf86GetOptValString(GeodeOptions, GX_OPTION_OSM_IMG_BUFS)))
        xf86DrvMsg(pScrni->scrnIndex, X_WARNING,
                   "XAA not present so OSMImageBuffers is ignored");

    xf86GetOptValInteger(GeodeOptions, GX_OPTION_OSM_CLR_BUFS,
                         (int *) &(pGeode->exaBfrSz));

    if (pGeode->exaBfrSz <= 0)
        pGeode->exaBfrSz = 0;

    if (pGeode->Panel == TRUE) {
        if (xf86ReturnOptValBool(GeodeOptions, GX_OPTION_NOPANEL, FALSE))
            pGeode->Panel = FALSE;
    }

    panelgeo = xf86GetOptValString(GeodeOptions, GX_OPTION_PANEL_GEOMETRY);

    if ((s = xf86GetOptValString(GeodeOptions, GX_OPTION_ACCEL_METHOD))) {
#if defined(XF86EXA)
        pGeode->useEXA = TRUE;
#else
        pGeode->useEXA = FALSE;
        pGeode->NoAccel = TRUE;
#endif
    }

    xf86DrvMsg(pScrni->scrnIndex, X_INFO,
               "Using %s acceleration architecture\n",
               pGeode->useEXA ? "EXA" : "none");

    /* Set up the panel */

    if (pGeode->Panel) {
        if (panelgeo != NULL) {
            if (GeodeGetFPGeometry(panelgeo, &pGeode->PanelX, &pGeode->PanelY))
                pGeode->Panel = FALSE;
        }
#ifdef PNL_SUP
        else {
            int b, f;

            /* The bitdepth and refresh isn't used anywhere else in the driver */

            if ((pGeode->Panel = Pnl_IsPanelEnabledInBIOS()))
                Pnl_GetPanelInfoFromBIOS(&pGeode->PanelX, &pGeode->PanelY, &b,
                                         &f);
        }
#endif
    }

    /* Set up the VGA */

    if (pGeode->useVGA) {
#if INT10_SUPPORT
        VESARec *pVesa;

        if (!xf86LoadSubModule(pScrni, "int10"))
            return FALSE;

        pVesa = pGeode->vesa;

        if ((pVesa->pInt = xf86InitInt10(pGeode->pEnt->index)) == NULL) {
            xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                       "Unable to initialize 1NT10 support\n");
            pGeode->useVGA = FALSE;
        }
#endif
    }

    /* First try to get the framebuffer size from the framebuffer,
     * and if that fails, revert all  the way back to the legacy
     * method
     */

    if (pGeode->FBAvail == 0) {
        if (GeodeGetSizeFromFB(&pGeode->FBAvail))
            pGeode->FBAvail = gfx_get_frame_buffer_size();
    }

    if (pScrni->memPhysBase == 0)
        pScrni->memPhysBase = gfx_get_frame_buffer_base();

    pScrni->fbOffset = 0;

    if (pGeode->pEnt->device->videoRam == 0)
        pScrni->videoRam = pGeode->FBAvail / 1024;
    else
        pScrni->videoRam = pGeode->pEnt->device->videoRam;

    GeodeClockRange = (ClockRangePtr) XNFcallocarray(1, sizeof(ClockRange));
    GeodeClockRange->next = NULL;
    GeodeClockRange->minClock = 25175;
    GeodeClockRange->maxClock = 229500;
    GeodeClockRange->clockIndex = -1;
    GeodeClockRange->interlaceAllowed = TRUE;
    GeodeClockRange->doubleScanAllowed = FALSE;

    pScrni->monitor->DDC = GeodeDoDDC(pScrni, pGeode->pEnt->index);

    /* I'm still not 100% sure this uses the right values */

    modecnt = xf86ValidateModes(pScrni,
                                pScrni->monitor->Modes,
                                pScrni->display->modes,
                                GeodeClockRange,
                                NULL, GX_MIN_PITCH, GX_MAX_PITCH,
                                32, GX_MIN_HEIGHT, GX_MAX_HEIGHT,
                                pScrni->display->virtualX,
                                pScrni->display->virtualY, pGeode->FBAvail,
                                LOOKUP_BEST_REFRESH);

    if (modecnt <= 0) {
        xf86DrvMsg(pScrni->scrnIndex, X_ERROR, "No valid modes were found\n");
        return FALSE;
    }

    xf86PruneDriverModes(pScrni);

    if (pScrni->modes == NULL) {
        xf86DrvMsg(pScrni->scrnIndex, X_ERROR, "No valid modes were found\n");
        return FALSE;
    }

    xf86SetCrtcForModes(pScrni, 0);
    pScrni->currentMode = pScrni->modes;

    xf86PrintModes(pScrni);
    xf86SetDpi(pScrni, 0, 0);

    /* Load the modules we'll need */

    if (xf86LoadSubModule(pScrni, "fb") == NULL) {
        return FALSE;
    }

    if (pGeode->NoAccel == FALSE) {
        const char *module = (pGeode->useEXA) ? "exa" : "xaa";

        if (!xf86LoadSubModule(pScrni, module)) {
            return FALSE;
        }
    }

    if (pGeode->tryHWCursor == TRUE) {
        if (!xf86LoadSubModule(pScrni, "ramdac")) {
            return FALSE;
        }
    }

#ifndef XSERVER_LIBPCIACCESS
    if (xf86RegisterResources(pGeode->pEnt->index, NULL, ResExclusive)) {
        xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                   "Couldn't register the resources.\n");
        return FALSE;
    }
#endif
    return TRUE;
}

static void
GXRestore(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    if (pGeode->useVGA && pGeode->FBVGAActive) {
        vgaHWPtr pvgaHW = VGAHWPTR(pScrni);

        vgaHWProtect(pScrni, TRUE);
        vgaHWRestore(pScrni, &pvgaHW->SavedReg, VGA_SR_ALL);
        vgaHWProtect(pScrni, FALSE);
    }
}

static Bool
GXUnmapMem(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    /* unmap all the memory map's */
#ifndef XSERVER_LIBPCIACCESS
    xf86UnMapVidMem(pScrni->scrnIndex, gfx_virt_regptr, GX_CPU_REG_SIZE);
    xf86UnMapVidMem(pScrni->scrnIndex, gfx_virt_gpptr, GX_GP_REG_SIZE);
    xf86UnMapVidMem(pScrni->scrnIndex, gfx_virt_vidptr, GX_VID_REG_SIZE);
    xf86UnMapVidMem(pScrni->scrnIndex, gfx_virt_fbptr, pGeode->FBAvail);
#else
    pciVideoPtr pci = xf86GetPciInfoForEntity(pGeode->pEnt->index);

    unmap_pci_mem(pScrni, pci, gfx_virt_regptr, GX_CPU_REG_SIZE);
    unmap_pci_mem(pScrni, pci, gfx_virt_gpptr, GX_GP_REG_SIZE);
    unmap_pci_mem(pScrni, pci, gfx_virt_vidptr, GX_VID_REG_SIZE);
    unmap_pci_mem(pScrni, pci, gfx_virt_fbptr, pGeode->FBAvail);

    munmap(XpressROMPtr, 0x10000);
#endif
    return TRUE;
}

static void
GXSetDvLineSize(unsigned int pitch)
{
    unsigned long temp, dv_size = MDC_DV_LINE_SIZE_1024;

    if (pitch > 1024) {
        dv_size = MDC_DV_LINE_SIZE_2048;
    }
    if (pitch > 2048) {
        dv_size = MDC_DV_LINE_SIZE_4096;
    }
    if (pitch > 4096) {
        dv_size = MDC_DV_LINE_SIZE_8192;
    }

    /* WRITE DIRTY/VALID CONTROL WITH LINE LENGTH */

    temp = READ_REG32(MDC_DV_CTL);
    WRITE_REG32(MDC_DV_CTL, (temp & ~MDC_DV_LINE_SIZE_MASK) | dv_size);
}

/* XXX - this is nothing like the original function - not sure exactly what the purpose is for this quite yet */

static void
GXAdjustFrame(ScrnInfoPtr pScrni, int x, int y)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    unsigned long offset;

    offset = y * pGeode->Pitch + x * (pScrni->bitsPerPixel >> 3);

    gfx_set_display_offset(offset);
}

static Bool
GXSetVideoMode(ScrnInfoPtr pScrni, DisplayModePtr pMode)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    int flags = 0;
    int custom = 0;

    pScrni->vtSema = TRUE;

    gx_disable_dac_power();

    if (pMode->Flags & V_NHSYNC)
        flags |= 1;
    if (pMode->Flags & V_NVSYNC)
        flags |= 2;

    /* Check to see if we should use custom or built-in timings */

    if (pGeode->Panel)
        custom = (pMode->type & M_T_USERDEF);
    else
        custom = !(pMode->type & (M_T_BUILTIN | M_T_DEFAULT));

    /* If we're not doing a custom mode, then just set the fixed timings,
     * otherwise, do the whole shooting match */

    if (!custom) {
        GFX(set_fixed_timings(pGeode->PanelX, pGeode->PanelY,
                              pMode->CrtcHDisplay, pMode->CrtcVDisplay,
                              pScrni->bitsPerPixel));
    }
    else {
        if (pGeode->Panel)
            GFX(set_panel_present(pGeode->PanelX, pGeode->PanelY,
                                  pMode->CrtcHDisplay, pMode->CrtcVDisplay,
                                  pScrni->bitsPerPixel));

        GFX(set_display_timings(pScrni->bitsPerPixel, flags,
                                pMode->CrtcHDisplay, pMode->CrtcHBlankStart,
                                pMode->CrtcHSyncStart, pMode->CrtcHSyncEnd,
                                pMode->CrtcHBlankEnd, pMode->CrtcHTotal,
                                pMode->CrtcVDisplay, pMode->CrtcVBlankStart,
                                pMode->CrtcVSyncStart, pMode->CrtcVSyncEnd,
                                pMode->CrtcVBlankEnd, pMode->CrtcVTotal,
                                (int) ((pMode->SynthClock / 1000.0) *
                                       0x10000)));
    }

    GFX(set_crt_enable(CRT_ENABLE));
    GFX(set_display_pitch(pGeode->displayPitch));
    GFX(set_display_offset(0L));
    GFX(wait_vertical_blank());

    if (pGeode->Compression) {
        GXSetDvLineSize(pGeode->Pitch);

        gfx_set_compression_offset(pGeode->CBData.compression_offset);
        gfx_set_compression_pitch(GX_CB_PITCH);
        gfx_set_compression_size(GX_CB_SIZE);

        gfx_set_compression_enable(1);
    }

    if (pGeode->HWCursor && !(pMode->Flags & V_DBLSCAN)) {
        GXLoadCursorImage(pScrni, NULL);
        GFX(set_cursor_position(pGeode->CursorStartOffset, 0, 0, 0, 0));
        GXShowCursor(pScrni);
    }
    else {
        GFX(set_cursor_enable(0));
        pGeode->HWCursor = FALSE;
    }

    GXAdjustFrame(pScrni, pScrni->frameX0, pScrni->frameY0);
    gx_enable_dac_power();

    return TRUE;
}

static Bool
GXSwitchMode(ScrnInfoPtr pScrni, DisplayModePtr pMode)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    int ret = TRUE;
    int rotate;

    /* Syn the engine and shutdown the DAC momentarily */

    gfx_wait_until_idle();

    /* Set up the memory for the new mode */
    rotate = GXGetRotation(pScrni->pScreen);
    ret = GXAllocateMemory(pScrni->pScreen, pScrni, rotate);

    if (ret) {
        if (pGeode->curMode != pMode)
            ret = GXSetVideoMode(pScrni, pMode);
    }

    if (ret)
        ret = GXRotate(pScrni, pMode);

    /* Go back the way it was */

    if (ret == FALSE) {
        if (!GXSetVideoMode(pScrni, pGeode->curMode))
            xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                       "Could not restore the previous mode\n");
    }
    else
        pGeode->curMode = pMode;

    return ret;
}

static void
GXLeaveGraphics(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    gfx_wait_until_idle();

    /* Restore VG registers */
    gfx_set_display_timings(pGeode->FBgfxdisplaytiming.wBpp,
                            pGeode->FBgfxdisplaytiming.wPolarity,
                            pGeode->FBgfxdisplaytiming.wHActive,
                            pGeode->FBgfxdisplaytiming.wHBlankStart,
                            pGeode->FBgfxdisplaytiming.wHSyncStart,
                            pGeode->FBgfxdisplaytiming.wHSyncEnd,
                            pGeode->FBgfxdisplaytiming.wHBlankEnd,
                            pGeode->FBgfxdisplaytiming.wHTotal,
                            pGeode->FBgfxdisplaytiming.wVActive,
                            pGeode->FBgfxdisplaytiming.wVBlankStart,
                            pGeode->FBgfxdisplaytiming.wVSyncStart,
                            pGeode->FBgfxdisplaytiming.wVSyncEnd,
                            pGeode->FBgfxdisplaytiming.wVBlankEnd,
                            pGeode->FBgfxdisplaytiming.wVTotal,
                            pGeode->FBgfxdisplaytiming.dwDotClock);

    gfx_set_compression_enable(0);

    /* Restore the previous Compression state */
    if (pGeode->FBCompressionEnable) {
        gfx_set_compression_offset(pGeode->FBCompressionOffset);
        gfx_set_compression_pitch(pGeode->FBCompressionPitch);
        gfx_set_compression_size(pGeode->FBCompressionSize);
        gfx_set_compression_enable(1);
    }

    gfx_set_display_pitch(pGeode->FBgfxdisplaytiming.wPitch);

    gfx_set_display_offset(pGeode->FBDisplayOffset);

    /* Restore Cursor */
    gfx_set_cursor_position(pGeode->FBCursorOffset, 0, 0, 0, 0);

    if (pGeode->useVGA) {
        pGeode->vesa->pInt->num = 0x10;
        pGeode->vesa->pInt->ax = 0x0 | pGeode->FBBIOSMode;
        pGeode->vesa->pInt->bx = 0;
        xf86ExecX86int10(pGeode->vesa->pInt);
        gfx_delay_milliseconds(3);
    }

    GXRestore(pScrni);

    gx_enable_dac_power();
}

static Bool
GXCloseScreen(ScreenPtr pScrn)
{
    ScrnInfoPtr pScrni = xf86ScreenToScrn(pScrn);
    GeodeRec *pGeode = GEODEPTR(pScrni);

    if (pScrni->vtSema)
        GXLeaveGraphics(pScrni);

    if (pGeode->pExa) {
        exaDriverFini(pScrn);
        free(pGeode->pExa);
        pGeode->pExa = NULL;
    }

    pScrni->vtSema = FALSE;

    GXUnmapMem(pScrni);

    pScrni->PointerMoved = pGeode->PointerMoved;
    pScrn->CloseScreen = pGeode->CloseScreen;

    if (pScrn->CloseScreen)
        return (*pScrn->CloseScreen) (pScrn);

    return TRUE;
}

static Bool
GXEnterGraphics(ScreenPtr pScrn, ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    if (!GXMapMem(pScrni))
        return FALSE;

    if (pGeode->useVGA)
        pGeode->FBVGAActive = gu2_get_vga_active();

    gfx_wait_until_idle();

    /* Save off the current state (should this be somewhere else)? */

    pGeode->FBgfxdisplaytiming.dwDotClock = gfx_get_clock_frequency();
    pGeode->FBgfxdisplaytiming.wPitch = gfx_get_display_pitch();
    pGeode->FBgfxdisplaytiming.wBpp = gfx_get_display_bpp();
    pGeode->FBgfxdisplaytiming.wHTotal = gfx_get_htotal();
    pGeode->FBgfxdisplaytiming.wHActive = gfx_get_hactive();
    pGeode->FBgfxdisplaytiming.wHSyncStart = gfx_get_hsync_start();
    pGeode->FBgfxdisplaytiming.wHSyncEnd = gfx_get_hsync_end();
    pGeode->FBgfxdisplaytiming.wHBlankStart = gfx_get_hblank_start();
    pGeode->FBgfxdisplaytiming.wHBlankEnd = gfx_get_hblank_end();
    pGeode->FBgfxdisplaytiming.wVTotal = gfx_get_vtotal();
    pGeode->FBgfxdisplaytiming.wVActive = gfx_get_vactive();
    pGeode->FBgfxdisplaytiming.wVSyncStart = gfx_get_vsync_start();
    pGeode->FBgfxdisplaytiming.wVSyncEnd = gfx_get_vsync_end();
    pGeode->FBgfxdisplaytiming.wVBlankStart = gfx_get_vblank_start();
    pGeode->FBgfxdisplaytiming.wVBlankEnd = gfx_get_vblank_end();
    pGeode->FBgfxdisplaytiming.wPolarity = gfx_get_sync_polarities();

    pGeode->FBDisplayOffset = gfx_get_display_offset();

    if (pGeode->useVGA) {
        vgaHWPtr pvgaHW = VGAHWPTR(pScrni);

        pGeode->FBBIOSMode = pvgaHW->readCrtc(pvgaHW, 0x040);
    }

    pGeode->FBCompressionEnable = gfx_get_compression_enable();
    pGeode->FBCompressionOffset = gfx_get_compression_offset();
    pGeode->FBCompressionPitch = gfx_get_compression_pitch();
    pGeode->FBCompressionSize = gfx_get_compression_size();

#ifdef PNL_SUP
    Pnl_SavePanelState();
#endif

    /* Turn off the VGA */

    if (pGeode->useVGA && pGeode->FBVGAActive) {
        unsigned short sequencer;
        vgaHWPtr pvgaHW = VGAHWPTR(pScrni);

        /* Map VGA aperture */
        if (!vgaHWMapMem(pScrni))
            return FALSE;

        /* Unlock VGA registers */
        vgaHWUnlock(pvgaHW);

        /* Save the current state and setup the current mode */
        vgaHWSave(pScrni, &VGAHWPTR(pScrni)->SavedReg, VGA_SR_ALL);

        /* DISABLE VGA SEQUENCER */
        /* This allows the VGA state machine to terminate. We must delay */
        /* such that there are no pending MBUS requests.  */

        gfx_outb(MDC_SEQUENCER_INDEX, MDC_SEQUENCER_CLK_MODE);
        sequencer = gfx_inb(MDC_SEQUENCER_DATA);
        sequencer |= MDC_CLK_MODE_SCREEN_OFF;
        gfx_outb(MDC_SEQUENCER_DATA, sequencer);

        gfx_delay_milliseconds(1);

        /* BLANK THE VGA DISPLAY */
        gfx_outw(MDC_SEQUENCER_INDEX, MDC_SEQUENCER_RESET);
        sequencer = gfx_inb(MDC_SEQUENCER_DATA);
        sequencer &= ~MDC_RESET_VGA_DISP_ENABLE;
        gfx_outb(MDC_SEQUENCER_DATA, sequencer);

        gfx_delay_milliseconds(1);
    }

    /* Set up the memory */
    /* XXX - FIXME - when we allow initial rotation, it should be here */
    GXAllocateMemory(pScrn, pScrni, pGeode->rotation);

    /* Clear the framebuffer */
    memset(pGeode->FBBase + pGeode->displayOffset, 0, pGeode->displaySize);

    /* Set up the video mode */
    GXSetVideoMode(pScrni, pScrni->currentMode);
    pGeode->curMode = pScrni->currentMode;

    return TRUE;
}

static void
GXLoadPalette(ScrnInfoPtr pScrni,
              int numColors, int *indizes, LOCO *colors, VisualPtr pVisual)
{
    int i, index, color;

    for (i = 0; i < numColors; i++) {
        index = indizes[i] & 0xFF;
        color = (((unsigned long) (colors[index].red & 0xFF)) << 16) |
            (((unsigned long) (colors[index].green & 0xFF)) << 8) |
            ((unsigned long) (colors[index].blue & 0xFF));

        GFX(set_display_palette_entry(index, color));
    }
}

#ifdef DPMSExtension
static void
GXPanelPower(int enable)
{
    unsigned long power = READ_VID32(RCDF_POWER_MANAGEMENT);

    if (enable != 0)
        power |= RCDF_PM_PANEL_POWER_ON;
    else
        power &= ~RCDF_PM_PANEL_POWER_ON;

    WRITE_VID32(RCDF_POWER_MANAGEMENT, power);
}

static void
GXDPMSSet(ScrnInfoPtr pScrni, int mode, int flags)
{
    GeodeRec *pGeode;

    pGeode = GEODEPTR(pScrni);

    if (!pScrni->vtSema)
        return;

    switch (mode) {
    case DPMSModeOn:
        /* Screen: On; HSync: On; VSync: On */
        GFX(set_crt_enable(CRT_ENABLE));
#if defined(PNL_SUP)
        if (pGeode->Panel) {
            Pnl_PowerUp();
            GXPanelPower(1);
        }
#endif
        break;

    case DPMSModeStandby:
        /* Screen: Off; HSync: Off; VSync: On */
        GFX(set_crt_enable(CRT_STANDBY));
#if defined(PNL_SUP)
        if (pGeode->Panel) {
            Pnl_PowerDown();
            GXPanelPower(0);
        }
#endif
        break;

    case DPMSModeSuspend:
        /* Screen: Off; HSync: On; VSync: Off */
        GFX(set_crt_enable(CRT_SUSPEND));
#if defined(PNL_SUP)
        if (pGeode->Panel) {
            Pnl_PowerDown();
            GXPanelPower(0);
        }
#endif
        break;

    case DPMSModeOff:
        /* Screen: Off; HSync: Off; VSync: Off */
        GFX(set_crt_enable(CRT_DISABLE));
#if defined(PNL_SUP)
        if (pGeode->Panel) {
            Pnl_PowerDown();
            GXPanelPower(0);
        }
#endif
        break;
    }
}
#endif

static Bool
GXCreateScreenResources(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrni = xf86ScreenToScrn(pScreen);
    GeodeRec *pGeode = GEODEPTR(pScrni);

    pScreen->CreateScreenResources = pGeode->CreateScreenResources;
    if (!(*pScreen->CreateScreenResources) (pScreen))
        return FALSE;

    if (pGeode->rotation != RR_Rotate_0) {
        RRScreenSize p;
        Rotation requestedRotation = pGeode->rotation;

        pGeode->rotation = RR_Rotate_0;

        /* Just setup enough for an initial rotate */

        p.width = pScreen->width;
        p.height = pScreen->height;
        p.mmWidth = pScreen->mmWidth;
        p.mmHeight = pScreen->mmHeight;

        pGeode->starting = TRUE;
        GXRandRSetConfig(pScreen, requestedRotation, 0, &p);
        pGeode->starting = FALSE;
    }

    return TRUE;
}

static Bool
GXScreenInit(ScreenPtr pScrn, int argc, char **argv)
{
    ScrnInfoPtr pScrni = xf86ScreenToScrn(pScrn);
    GeodeRec *pGeode = GEODEPTR(pScrni);
    XF86ModReqInfo shadowReq;
    int maj, min, ret, rotate;

    pGeode->starting = TRUE;

    /* If we are using VGA then go ahead and map the memory */

    if (pGeode->useVGA) {

        if (!vgaHWMapMem(pScrni))
            return FALSE;

        vgaHWGetIOBase(VGAHWPTR(pScrni));
    }

    if (!pGeode->NoAccel && pGeode->useEXA) {
        if (!(pGeode->pExa = exaDriverAlloc())) {
            xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                       "Couldn't allocate the EXA structure.\n");
            pGeode->NoAccel = TRUE;
        }
        else {
            ExaDriverPtr pExa = pGeode->pExa;

            /* THis is set in GXAllocMem */
            pExa->memoryBase = 0;

            /* This is set in GXAllocateMemory */
            pExa->memorySize = 0;

            pExa->pixmapOffsetAlign = 32;
            pExa->pixmapPitchAlign = 32;
            pExa->flags = EXA_OFFSCREEN_PIXMAPS;
            pExa->maxX = GX_MAX_WIDTH - 1;
            pExa->maxY = GX_MAX_HEIGHT - 1;
        }
    }

    /* XXX FIXME - Take down any of the structures on failure? */

    if (!GXEnterGraphics(pScrn, pScrni))
        return FALSE;

    miClearVisualTypes();

    /* XXX Again - take down anything? */

    if (pScrni->bitsPerPixel > 8) {
        if (!miSetVisualTypes(pScrni->depth,
                              TrueColorMask, pScrni->rgbBits,
                              pScrni->defaultVisual)) {
            return FALSE;
        }
    }
    else {
        if (!miSetVisualTypes(pScrni->depth,
                              miGetDefaultVisualMask(pScrni->depth),
                              pScrni->rgbBits, pScrni->defaultVisual)) {
            return FALSE;
        }
    }

    miSetPixmapDepths();

    /* Point at the visible area to start */

    ret = fbScreenInit(pScrn, pGeode->FBBase + pGeode->displayOffset,
                       pScrni->virtualX, pScrni->virtualY,
                       pScrni->xDpi, pScrni->yDpi, pGeode->displayWidth,
                       pScrni->bitsPerPixel);

    if (!ret)
        return FALSE;

    xf86SetBlackWhitePixels(pScrn);

    /* Set up the color ordering */

    if (pScrni->bitsPerPixel > 8) {
        VisualPtr visual = pScrn->visuals + pScrn->numVisuals;

        while (--visual >= pScrn->visuals) {
            if ((visual->class | DynamicClass) == DirectColor) {
                visual->offsetRed = pScrni->offset.red;
                visual->offsetGreen = pScrni->offset.green;
                visual->offsetBlue = pScrni->offset.blue;
                visual->redMask = pScrni->mask.red;
                visual->greenMask = pScrni->mask.green;
                visual->blueMask = pScrni->mask.blue;
            }
        }
    }

    /* Must follow the color ordering */
    fbPictureInit(pScrn, 0, 0);

    if (!pGeode->NoAccel)
        GXAccelInit(pScrn);

    xf86SetBackingStore(pScrn);

    /* Set up the soft cursor */
    miDCInitialize(pScrn, xf86GetPointerScreenFuncs());

    /* Set up the HW cursor - must follow the soft cursor init */

    if (pGeode->tryHWCursor) {
        if (!GXHWCursorInit(pScrn))
            xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                       "Hardware cursor initialization failed.\n");
    }

    /* Set up the color map */

    if (!miCreateDefColormap(pScrn))
        return FALSE;

    if (pScrni->bitsPerPixel == 8) {
        /* Must follow initialization of the default colormap */

        if (!xf86HandleColormaps(pScrn, 256, 8,
                                 GXLoadPalette, NULL,
                                 CMAP_PALETTED_TRUECOLOR |
                                 CMAP_RELOAD_ON_MODE_SWITCH)) {
            return FALSE;
        }
    }
#ifdef DPMSExtension
    xf86DPMSInit(pScrn, GXDPMSSet, 0);
#endif

    GXInitVideo(pScrn);

    /* Set up RandR */

#if GET_ABI_MAJOR(ABI_VIDEODRV_VERSION) < 24
    xf86DisableRandR();         /* We provide our own RandR goodness */
#endif

    /* Try to set up the shadow FB for rotation */

    memset(&shadowReq, 0, sizeof(shadowReq));
    shadowReq.majorversion = 1;
    shadowReq.minorversion = 1;

    if (LoadSubModule(pScrni->module, "shadow",
                      NULL, NULL, NULL, &shadowReq, &maj, &min)) {

        rotate = RR_Rotate_0 | RR_Rotate_90 | RR_Rotate_180 | RR_Rotate_270;
        shadowSetup(pScrn);
    }
    else {
        LoaderErrorMsg(NULL, "shadow", maj, min);
        xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                   "Error loading shadow - rotation not available.\n");

        if (pGeode->rotation != RR_Rotate_0)
            xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                       "Reverting back to normal rotation.\n");

        rotate = pGeode->rotation = RR_Rotate_0;
    }

    GXRandRInit(pScrn, rotate);

    pGeode->PointerMoved = pScrni->PointerMoved;
    pScrni->PointerMoved = GeodePointerMoved;

    pGeode->CreateScreenResources = pScrn->CreateScreenResources;
    pScrn->CreateScreenResources = GXCreateScreenResources;

    pGeode->CloseScreen = pScrn->CloseScreen;
    pScrn->CloseScreen = GXCloseScreen;
    pScrn->SaveScreen = GXSaveScreen;

    if (serverGeneration == 1)
        xf86ShowUnusedOptions(pScrni->scrnIndex, pScrni->options);

    pGeode->starting = FALSE;

    return TRUE;
}

static int
GXValidMode(ScrnInfoPtr pScrni, DisplayModePtr pMode, Bool Verbose, int flags)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    int p;
    int custom = 0;

    if (pGeode->Panel)
        custom = (pMode->type & M_T_USERDEF);
    else
        custom = (pMode->type & (M_T_BUILTIN | M_T_DEFAULT));

    /* Use the durango lookup for !custom modes */

    if (!custom && pGeode->Panel) {
        if (pMode->CrtcHDisplay > pGeode->PanelX ||
            pMode->CrtcVDisplay > pGeode->PanelY ||
            gfx_is_panel_mode_supported(pGeode->PanelX,
                                        pGeode->PanelY,
                                        pMode->CrtcHDisplay,
                                        pMode->CrtcVDisplay,
                                        pScrni->bitsPerPixel) < 0) {

            return MODE_BAD;
        }
    }

    if (gfx_is_display_mode_supported(pMode->CrtcHDisplay,
                                      pMode->CrtcVDisplay,
                                      pScrni->bitsPerPixel,
                                      GeodeGetRefreshRate(pMode)) < 0) {
        return MODE_BAD;
    }

    if (pMode->Flags & V_INTERLACE)
        return MODE_NO_INTERLACE;

    if (pGeode->tryCompression)
        p = GeodeCalculatePitchBytes(pMode->CrtcHDisplay, pScrni->bitsPerPixel);
    else
        p = ((pMode->CrtcHDisplay + 3) & ~3) * (pScrni->bitsPerPixel >> 3);

    if (p * pMode->CrtcVDisplay > pGeode->FBAvail)
        return MODE_MEM;

    return MODE_OK;
}

/* XXX - Way more to do here */

static Bool
GXEnterVT(ScrnInfoPtr pScrni)
{
    return GXEnterGraphics(NULL, pScrni);
}

static void
GXLeaveVT(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    pGeode->PrevDisplayOffset = gfx_get_display_offset();
    GXLeaveGraphics(pScrni);
}

void
GXSetupChipsetFPtr(ScrnInfoPtr pScrn)
{
    pScrn->PreInit = GXPreInit;
    pScrn->ScreenInit = GXScreenInit;
    pScrn->SwitchMode = GXSwitchMode;
    pScrn->AdjustFrame = GXAdjustFrame;
    pScrn->EnterVT = GXEnterVT;
    pScrn->LeaveVT = GXLeaveVT;
    pScrn->FreeScreen = GeodeFreeScreen;
    pScrn->ValidMode = GXValidMode;
}

/* ====== Common functions ======
 * These are all the common functions that we use for both GX and LX - They live here
 * because most of them came along for the GX first, and then were adapted to the LX.
 *  We could move these to a common function, but there is no hurry
 * ============================== */

void
GeodePointerMoved(ScrnInfoPtr pScrni, int x, int y)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    int newX = x, newY = y;

    switch (pGeode->rotation) {
    case RR_Rotate_0:
        break;
    case RR_Rotate_90:
        newX = y;
        newY = pScrni->pScreen->width - x - 1;
        break;
    case RR_Rotate_180:
        newX = pScrni->pScreen->width - x - 1;
        newY = pScrni->pScreen->height - y - 1;
        break;
    case RR_Rotate_270:
        newX = pScrni->pScreen->height - y - 1;
        newY = x;
        break;
    }

    (*pGeode->PointerMoved) (pScrni, newX, newY);
}

int
GeodeGetFPGeometry(const char *str, int *width, int *height)
{

    int ret = sscanf(str, "%dx%d", width, height);

    return (ret == 2) ? 0 : 1;
}

static void
GeodeFreeRec(ScrnInfoPtr pScrni)
{
    if (pScrni->driverPrivate != NULL) {
        free(pScrni->driverPrivate);
        pScrni->driverPrivate = NULL;
    }
}

void
GeodeFreeScreen(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    if (pGeode == NULL)
        return;

    if (pGeode->useVGA) {
        if (xf86LoaderCheckSymbol("vgaHWFreeHWRec"))
            vgaHWFreeHWRec(pScrni);
    }

    GeodeFreeRec(pScrni);
}

int
GeodeCalculatePitchBytes(unsigned int width, unsigned int bpp)
{

    int delta = width * (bpp >> 3);

    /* Less then 640 has doubling enabled */

    if (width < 640)
        delta <<= 1;

    /* Calculate the pitch (compression requires a power of 2) */

    if (delta > 4096)
        delta = 8192;
    else if (delta > 2048)
        delta = 4096;
    else if (delta > 1024)
        delta = 2048;
    else
        delta = 1024;

    return delta;
}
