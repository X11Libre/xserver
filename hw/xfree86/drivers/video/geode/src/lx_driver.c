/* Copyright (c) 2003-2008 Advanced Micro Devices, Inc.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86i2c.h"
#include "xf86Crtc.h"
#include "xf86cmap.h"
#include "compiler.h"
#include "mipointer.h"
#include "fb.h"
#include "miscstruct.h"
#include "micmap.h"
#include "vbe.h"
#include "fb.h"
#include "cim_defs.h"
#include "cim_regs.h"
#include "geode.h"

/* Bring in VGA functions */
#include "lx_vga.c"

#define LX_MAX_WIDTH  1940
#define LX_MAX_HEIGHT 1600

/* Size of the register blocks */

#define LX_GP_REG_SIZE  0x4000
#define LX_VG_REG_SIZE  0x4000
#define LX_VID_REG_SIZE 0x4000
#define LX_VIP_REG_SIZE 0x4000

/* Size of the Cimarron command buffer */
#define CIM_CMD_BFR_SZ 0x200000

extern OptionInfoRec LX_GeodeOptions[];

extern unsigned char *XpressROMPtr;

static Bool
LXSaveScreen(ScreenPtr pScrn, int mode)
{
    ScrnInfoPtr pScrni = xf86ScreenToScrn(pScrn);
    GeodePtr pGeode = GEODEPTR(pScrni);

    if (pGeode->useVGA && !pScrni->vtSema)
        return vgaHWSaveScreen(pScrn, mode);

    return TRUE;
}

/* This is an overly complex MSR read mechanism */

/* From Cimarron - the VSAII read/write methods - we use these as fallback */

#define LX_MSR_READ(adr,lo,hi)              \
     __asm__ __volatile__(                      \
        " mov $0x0AC1C, %%edx\n"                \
        " mov $0xFC530007, %%eax\n"             \
        " out %%eax,%%dx\n"                     \
        " add $2,%%dl\n"                        \
        " in %%dx, %%ax"                        \
        : "=a" (lo), "=d" (hi)                  \
        : "c" (adr))

#if __WORDSIZE == 64

#define LX_MSR_WRITE(adr,low,high) \
  { int32_t d0, d1, d2, d3, d4;        \
  __asm__ __volatile__(            \
    " push %%rbx\n"                \
    " mov $0x0AC1C, %%edx\n"       \
    " mov $0xFC530007, %%eax\n"    \
    " out %%eax,%%dx\n"            \
    " add $2,%%dl\n"               \
    " mov %6, %%ebx\n"             \
    " mov %7, %0\n"                \
    " mov %5, %3\n"                \
    " xor %2, %2\n"                \
    " xor %1, %1\n"                \
    " out %%ax, %%dx\n"            \
    " pop %%rbx\n"                 \
    : "=a"(d0),"=&D"(d1),"=&S"(d2), \
      "=c"(d3),"=d"(d4)  \
    : "1"(adr),"2"(high),"3"(low)); \
  }

#else

#define LX_MSR_WRITE(adr,low,high) \
  { int32_t d0, d1, d2, d3, d4;        \
  __asm__ __volatile__(            \
    " push %%ebx\n"                \
    " mov $0x0AC1C, %%edx\n"       \
    " mov $0xFC530007, %%eax\n"    \
    " out %%eax,%%dx\n"            \
    " add $2,%%dl\n"               \
    " mov %6, %%ebx\n"             \
    " mov %7, %0\n"                \
    " mov %5, %3\n"                \
    " xor %2, %2\n"                \
    " xor %1, %1\n"                \
    " out %%ax, %%dx\n"            \
    " pop %%ebx\n"                 \
    : "=a"(d0),"=&D"(d1),"=&S"(d2), \
      "=c"(d3),"=d"(d4)  \
    : "1"(adr),"2"(high),"3"(low)); \
  }

#endif

static void
LXReadMSR(uint32_t addr, uint32_t *lo, uint32_t *hi)
{
    if (GeodeReadMSR(addr, lo, hi) == -1) {
        unsigned int l, h;

        LX_MSR_READ(addr, l, h);
        *lo = l;
        *hi = h;
    }
}

static void
LXWriteMSR(uint32_t addr, uint32_t lo, uint32_t hi)
{
    if (GeodeWriteMSR(addr, lo, hi) == -1)
        LX_MSR_WRITE(addr, lo, hi);
}

static unsigned int
LXCalcPitch(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    if (pGeode->tryCompression)
        return GeodeCalculatePitchBytes(pScrni->virtualX, pScrni->bitsPerPixel);
    else
        return ((pScrni->virtualX + 3) & ~3) * (pScrni->bitsPerPixel >> 3);
}

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

static Bool
LXMapMem(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    int index = pScrni->scrnIndex;
    unsigned long cmd_bfr_phys;

    pciVideoPtr pci = xf86GetPciInfoForEntity(pGeode->pEnt->index);

#ifndef XSERVER_LIBPCIACCESS
    PCITAG tag;

    tag = pciTag(pci->bus, pci->device, pci->func);

    cim_gp_ptr = (unsigned char *) xf86MapPciMem(index, VIDMEM_MMIO,
                                                 tag, pci->memBase[1],
                                                 LX_GP_REG_SIZE);

    cim_vg_ptr = (unsigned char *) xf86MapPciMem(index, VIDMEM_MMIO,
                                                 tag, pci->memBase[2],
                                                 LX_VG_REG_SIZE);

    cim_vid_ptr = (unsigned char *) xf86MapPciMem(index, VIDMEM_MMIO,
                                                  tag, pci->memBase[3],
                                                  LX_VID_REG_SIZE);

    cim_vip_ptr = (unsigned char *) xf86MapPciMem(index, VIDMEM_MMIO,
                                                  tag, pci->memBase[4],
                                                  LX_VIP_REG_SIZE);

    cim_fb_ptr = (unsigned char *) xf86MapPciMem(index, VIDMEM_FRAMEBUFFER,
                                                 tag, pci->memBase[0],
                                                 pGeode->FBAvail +
                                                 CIM_CMD_BFR_SZ);
#else
    cim_gp_ptr = map_pci_mem(pScrni, 0, pci, 1, LX_GP_REG_SIZE);
    cim_vg_ptr = map_pci_mem(pScrni, 0, pci, 2, LX_VG_REG_SIZE);
    cim_vid_ptr = map_pci_mem(pScrni, 0, pci, 3, LX_VID_REG_SIZE);
    cim_vip_ptr = map_pci_mem(pScrni, 0, pci, 4, LX_VIP_REG_SIZE);
    cim_fb_ptr =
        map_pci_mem(pScrni, 1, pci, 0, pGeode->FBAvail + CIM_CMD_BFR_SZ);
#endif

    if (pScrni->memPhysBase == 0)
        pScrni->memPhysBase = PCI_REGION_BASE(pci, 0, REGION_MEM);

    cmd_bfr_phys = PCI_REGION_BASE(pci, 0, REGION_MEM) + pGeode->CmdBfrOffset;
    cim_cmd_base_ptr = cim_fb_ptr + pGeode->CmdBfrOffset;

    if (!cim_gp_ptr || !cim_vg_ptr || !cim_vid_ptr || !cim_fb_ptr ||
        !cim_vip_ptr)
        return FALSE;

    gp_set_frame_buffer_base(PCI_REGION_BASE(pci, 0, REGION_MEM),
                             pGeode->FBAvail);
    gp_set_command_buffer_base(cmd_bfr_phys, 0, pGeode->CmdBfrSize);

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

    pGeode->FBBase = cim_fb_ptr;

    if (!pGeode->NoAccel)
        pGeode->pExa->memoryBase = pGeode->FBBase;

    xf86DrvMsg(index, X_INFO, "Geode LX video memory %x bytes at %p\n",
               pGeode->FBAvail, pGeode->FBBase);

    return TRUE;
}

/* Check to see if VGA exists - we map the space and look for a
   signature - if it doesn't match exactly, then we assume no VGA.
*/

static Bool
LXCheckVGA(ScrnInfoPtr pScrni, EntityInfoPtr pEnt)
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
LXCrtcResize(ScrnInfoPtr pScrni, int width, int height)
{
    return TRUE;
}

static const xf86CrtcConfigFuncsRec lx_xf86crtc_config_funcs = {
    LXCrtcResize,
};

static Bool
LXPreInit(ScrnInfoPtr pScrni, int flags)
{
    GeodePtr pGeode;
    EntityInfoPtr pEnt;
    OptionInfoRec *GeodeOptions = &LX_GeodeOptions[0];
    rgb defaultWeight = { 0, 0, 0 };
    const char *s;

    if (pScrni->numEntities != 1)
        return FALSE;

    pEnt = xf86GetEntityInfo(pScrni->entityList[0]);
#ifndef XSERVER_LIBPCIACCESS
    if (pEnt->resources)
        return FALSE;
#endif
    if (flags & PROBE_DETECT) {
        GeodeProbeDDC(pScrni, pEnt->index);
        return TRUE;
    }

    pGeode = pScrni->driverPrivate = XNFcallocarray(1, sizeof(GeodeRec));

    if (pGeode == NULL)
        return FALSE;

    pGeode->useVGA = LXCheckVGA(pScrni, pEnt);
    pGeode->VGAActive = FALSE;
    pGeode->pEnt = pEnt;

    if (pGeode->useVGA) {
        if (!xf86LoadSubModule(pScrni, "vgahw") || !vgaHWGetHWRec(pScrni))
            pGeode->useVGA = FALSE;
        else
            vgaHWSetStdFuncs(VGAHWPTR(pScrni));

        pGeode->vesa = calloc(1, sizeof(VESARec));
    }

    cim_rdmsr = LXReadMSR;
    cim_wrmsr = LXWriteMSR;

    /* Set up the Cimarron MSR tables */
    msr_init_table();

    /* By default, we support panel and CRT - the config file should
     * disable the ones we don't want
     */

    pGeode->Output = OUTPUT_PANEL | OUTPUT_CRT;

    /* Fill in the monitor information */
    pScrni->monitor = pScrni->confScreen->monitor;

    if (!xf86SetDepthBpp(pScrni, 0, 0, 0, Support24bppFb | Support32bppFb))
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

    pGeode->tryHWCursor = TRUE;
    pGeode->tryCompression = TRUE;

    /* Protect against old versions of EXA */

#if (EXA_VERSION_MAJOR < 2)
    pGeode->NoAccel = TRUE;
    xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
               "*** This driver was compiled with EXA version %d\n");
    xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
               "*** we need version 2 or greater\n");
    xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
               "*** All accelerations are being turned off.\n");
#else
    pGeode->NoAccel = FALSE;
#endif

    pGeode->exaBfrSz = DEFAULT_EXA_SCRATCH_BFRSZ;

    xf86GetOptValBool(GeodeOptions, LX_OPTION_HW_CURSOR, &pGeode->tryHWCursor);

    if (!xf86GetOptValInteger(GeodeOptions, LX_OPTION_FBSIZE,
                              (int *) &(pGeode->FBAvail)))
        pGeode->FBAvail = 0;

    /* For compatibility - allow SWCursor too */

    if (xf86ReturnOptValBool(GeodeOptions, LX_OPTION_SW_CURSOR, FALSE))
        pGeode->tryHWCursor = FALSE;

    if (xf86ReturnOptValBool(GeodeOptions, LX_OPTION_NOCOMPRESSION, FALSE))
        pGeode->tryCompression = FALSE;

    if (xf86ReturnOptValBool(GeodeOptions, LX_OPTION_NOACCEL, FALSE))
        pGeode->NoAccel = TRUE;

    pGeode->rotation = RR_Rotate_0;

    if ((s = xf86GetOptValString(GeodeOptions, LX_OPTION_ROTATE))) {

        if (!xf86NameCmp(s, "LEFT"))
            pGeode->rotation = RR_Rotate_90;
        else if (!xf86NameCmp(s, "INVERT"))
            pGeode->rotation = RR_Rotate_180;
        else if (!xf86NameCmp(s, "CCW"))
            pGeode->rotation = RR_Rotate_270;
        else
            xf86DrvMsg(pScrni->scrnIndex, X_ERROR, "Invalid rotation %s.\n", s);
    }

    xf86GetOptValInteger(GeodeOptions, LX_OPTION_EXA_SCRATCH_BFRSZ,
                         (int *) &(pGeode->exaBfrSz));

    if (pGeode->exaBfrSz <= 0)
        pGeode->exaBfrSz = 0;

    if (pGeode->Output & OUTPUT_PANEL) {
        if (xf86ReturnOptValBool(GeodeOptions, LX_OPTION_NOPANEL, FALSE))
            pGeode->Output &= ~OUTPUT_PANEL;
    }

    /* Panel detection code -
     * 1.  See if an OLPC DCON is attached - we can make some assumptions
     * about the panel if so.
     * 2.  Use panel mode specified in the config
     * 3.  "Autodetect" the panel through VSA
     */

    if (dcon_init(pScrni)) {
        pGeode->Output = OUTPUT_PANEL | OUTPUT_DCON;
    }
    else if (pGeode->Output & OUTPUT_PANEL) {
        const char *pmode =
            xf86GetOptValString(GeodeOptions, LX_OPTION_PANEL_MODE);

        if (pmode != NULL)
            pGeode->panelMode = LXGetManualPanelMode(pmode);

        if (pGeode->panelMode == NULL)
            pGeode->panelMode = LXGetLegacyPanelMode(pScrni);

        if (pGeode->panelMode == NULL)
            pGeode->Output &= ~OUTPUT_PANEL;
    }

    /* Default to turn scaling on for panels */

    if (pGeode->Output & OUTPUT_PANEL)
        pGeode->Scale = TRUE;

    xf86DrvMsg(pScrni->scrnIndex, X_INFO, "LX output options:\n");
    xf86DrvMsg(pScrni->scrnIndex, X_INFO, " CRT: %s\n",
               pGeode->Output & OUTPUT_CRT ? "YES" : "NO");

    xf86DrvMsg(pScrni->scrnIndex, X_INFO, " PANEL: %s\n",
               pGeode->Output & OUTPUT_PANEL ? "YES" : "NO");

    xf86DrvMsg(pScrni->scrnIndex, X_INFO, " DCON: %s\n",
               pGeode->Output & OUTPUT_DCON ? "YES" : "NO");

    xf86DrvMsg(pScrni->scrnIndex, X_INFO, " VGA: %s\n",
               pGeode->useVGA ? "YES" : "NO");

    /* Set up VGA */

    if (pGeode->useVGA) {
        VESARec *pVesa;

        if (!xf86LoadSubModule(pScrni, "int10"))
            return FALSE;

        pVesa = pGeode->vesa;

        if ((pVesa->pInt = xf86InitInt10(pGeode->pEnt->index)) == NULL) {
            xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                       "Unable to initialize 1NT10 support\n");
            pGeode->useVGA = FALSE;
        }
    }

    /* Read the amount of framebuffer memory */
    /* First try to read it from the framebuffer, and if that fails,
     * do it the legacy way
     */

    if (pGeode->FBAvail == 0) {
        if (GeodeGetSizeFromFB(&pGeode->FBAvail)) {
            unsigned long value;

            cim_outw(0xAC1C, 0xFC53);
            cim_outw(0xAC1C, 0x0200);

            value = (unsigned long) (cim_inw(0xAC1E)) & 0xFE;
            pGeode->FBAvail = value << 20;
        }
    }

    pScrni->fbOffset = 0;

    if (pGeode->pEnt->device->videoRam == 0)
        pScrni->videoRam = pGeode->FBAvail / 1024;
    else {
        pScrni->videoRam = pGeode->pEnt->device->videoRam;
        pGeode->FBAvail = pScrni->videoRam << 10;
    }

    /* If we have <= 16Mb of memory then compression is going
       to hurt - so warn and disable */

    if (pGeode->tryCompression && pGeode->FBAvail <= 0x1000000) {
        xf86DrvMsg(pScrni->scrnIndex, X_INFO,
                   "%x bytes of video memory is less then optimal\n",
                   pGeode->FBAvail);
        xf86DrvMsg(pScrni->scrnIndex, X_INFO,
                   "when compression is on. Disabling compression.\n");
        pGeode->tryCompression = FALSE;
    }

    /* Carve out some memory for the command buffer */

    pGeode->CmdBfrSize = CIM_CMD_BFR_SZ;
    pGeode->FBAvail -= CIM_CMD_BFR_SZ;

    pGeode->CmdBfrOffset = pGeode->FBAvail;

    /* Allocate a a CRTC config structure */
    xf86CrtcConfigInit(pScrni, &lx_xf86crtc_config_funcs);

    /* Set up the GPU CRTC */
    LXSetupCrtc(pScrni);

    xf86CrtcSetSizeRange(pScrni, 320, 200, LX_MAX_WIDTH, LX_MAX_HEIGHT);

    /* Setup the output */
    LXSetupOutput(pScrni);

    if (!xf86InitialConfiguration(pScrni, FALSE)) {
        xf86DrvMsg(pScrni->scrnIndex, X_ERROR, "No valid modes.\n");
        return FALSE;
    }

    xf86PrintModes(pScrni);

    pScrni->currentMode = pScrni->modes;

    pGeode->Pitch = LXCalcPitch(pScrni);

    xf86SetDpi(pScrni, 0, 0);

    /* Load the modules we'll need */

    if (xf86LoadSubModule(pScrni, "fb") == NULL) {
        return FALSE;
    }

    if (!pGeode->NoAccel) {
        if (!xf86LoadSubModule(pScrni, "exa"))
            return FALSE;
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
LXRestore(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    if (pGeode->useVGA) {
        vgaHWPtr pvgaHW = VGAHWPTR(pScrni);

        vgaHWProtect(pScrni, TRUE);
        vgaHWRestore(pScrni, &pvgaHW->SavedReg, VGA_SR_ALL);
        vgaHWProtect(pScrni, FALSE);
    }
}

static Bool
LXUnmapMem(ScrnInfoPtr pScrni)
{
#ifndef XSERVER_LIBPCIACCESS
    xf86UnMapVidMem(pScrni->scrnIndex, (pointer) cim_gp_ptr, LX_GP_REG_SIZE);
    xf86UnMapVidMem(pScrni->scrnIndex, (pointer) cim_vg_ptr, LX_VG_REG_SIZE);
    xf86UnMapVidMem(pScrni->scrnIndex, (pointer) cim_vid_ptr, LX_VID_REG_SIZE);
    xf86UnMapVidMem(pScrni->scrnIndex, (pointer) cim_vip_ptr, LX_VIP_REG_SIZE);

    xf86UnMapVidMem(pScrni->scrnIndex, XpressROMPtr, 0x10000);
#else
    GeodeRec *pGeode = GEODEPTR(pScrni);
    pciVideoPtr pci = xf86GetPciInfoForEntity(pGeode->pEnt->index);

    unmap_pci_mem(pScrni, pci, cim_gp_ptr, LX_GP_REG_SIZE);
    unmap_pci_mem(pScrni, pci, cim_vg_ptr, LX_VG_REG_SIZE);
    unmap_pci_mem(pScrni, pci, cim_vid_ptr, LX_VID_REG_SIZE);
    unmap_pci_mem(pScrni, pci, cim_vip_ptr, LX_VIP_REG_SIZE);
    unmap_pci_mem(pScrni, pci, cim_fb_ptr, pGeode->FBAvail + CIM_CMD_BFR_SZ);

    munmap(XpressROMPtr, 0x10000);
#endif

    return TRUE;
}

/* These should be correctly accounted for rotation */

void
LXAdjustFrame(ScrnInfoPtr pScrni, int x, int y)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    unsigned long offset;

    offset = (y * pGeode->Pitch);
    offset += x * (pScrni->bitsPerPixel >> 3);

    vg_set_display_offset(offset);
}

static Bool
LXSwitchMode(ScrnInfoPtr pScrni, DisplayModePtr pMode)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    /* Set the new mode */
    return xf86SetSingleMode(pScrni, pMode, pGeode->rotation);
}

static void
LXLeaveGraphics(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    VG_PANNING_COORDINATES panning;

    gp_wait_until_idle();

    lx_disable_dac_power(pScrni, DF_CRT_DISABLE);

    vg_set_custom_mode(&(pGeode->FBcimdisplaytiming.vgDisplayMode),
                       pGeode->FBcimdisplaytiming.wBpp);

    vg_set_compression_enable(0);

    /* Restore the previous Compression state */
    if (pGeode->FBCompressionEnable) {
        vg_configure_compression(&(pGeode->FBCBData));
        vg_set_compression_enable(1);
    }

    vg_set_display_pitch(pGeode->FBcimdisplaytiming.wPitch);
    vg_set_display_offset(pGeode->FBDisplayOffset);

    /* Restore Cursor */
    vg_set_cursor_position(pGeode->FBCursor.cursor_x,
                           pGeode->FBCursor.cursor_y, &panning);

    LXRestore(pScrni);

    if (pGeode->useVGA && pGeode->VGAActive) {
        pGeode->vesa->pInt->num = 0x10;
        pGeode->vesa->pInt->ax = 0x0 | pGeode->FBBIOSMode;
        pGeode->vesa->pInt->bx = 0;
        xf86ExecX86int10(pGeode->vesa->pInt);
        vg_delay_milliseconds(3);
    }

    lx_enable_dac_power(pScrni, 1);
    pScrni->vtSema = FALSE;
}

static Bool
LXCloseScreen(ScreenPtr pScrn)
{
    ScrnInfoPtr pScrni = xf86ScreenToScrn(pScrn);
    GeodeRec *pGeode = GEODEPTR(pScrni);

    if (pScrni->vtSema)
        LXLeaveGraphics(pScrni);

    if (pGeode->pExa) {
        exaDriverFini(pScrn);
        free(pGeode->pExa);
        pGeode->pExa = NULL;
    }

    /* Unmap the offscreen allocations */
    GeodeCloseOffscreen(pScrni);

    LXUnmapMem(pScrni);

    if (pGeode->useVGA)
        vgaHWUnmapMem(pScrni);

    pScrni->PointerMoved = pGeode->PointerMoved;
    pScrn->CloseScreen = pGeode->CloseScreen;

    if (pScrn->CloseScreen)
        return (*pScrn->CloseScreen) (pScrn);

    return TRUE;
}

static Bool
LXEnterGraphics(ScreenPtr pScrn, ScrnInfoPtr pScrni)
{
    int bpp;
    GeodeRec *pGeode = GEODEPTR(pScrni);

    pGeode->VGAActive = gu3_get_vga_active();

    gp_wait_until_idle();

    vg_get_current_display_mode(&pGeode->FBcimdisplaytiming.vgDisplayMode,
                                &bpp);

    pGeode->FBcimdisplaytiming.wBpp = bpp;
    pGeode->FBcimdisplaytiming.wPitch = vg_get_display_pitch();

    pGeode->FBDisplayOffset = vg_get_display_offset();

    if (pGeode->useVGA && pGeode->VGAActive) {
        vgaHWPtr pvgaHW = VGAHWPTR(pScrni);

        pGeode->FBBIOSMode = pvgaHW->readCrtc(pvgaHW, 0x040);
    }

    pGeode->FBCompressionEnable = vg_get_compression_enable();
    vg_get_compression_info(&(pGeode->FBCBData));

    /* Save Cursor offset */
    vg_get_cursor_info(&pGeode->FBCursor);

    /* Turn off the VGA */

    if (pGeode->useVGA) {
        unsigned short sequencer;
        vgaHWPtr pvgaHW = VGAHWPTR(pScrni);

        /* Unlock VGA registers */
        vgaHWUnlock(pvgaHW);

        /* Save the current state and setup the current mode */
        vgaHWSave(pScrni, &VGAHWPTR(pScrni)->SavedReg, VGA_SR_ALL);

        /* DISABLE VGA SEQUENCER */
        /* This allows the VGA state machine to terminate. We must delay */
        /* such that there are no pending MBUS requests.  */

        cim_outb(DC3_SEQUENCER_INDEX, DC3_SEQUENCER_CLK_MODE);
        sequencer = cim_inb(DC3_SEQUENCER_DATA);
        sequencer |= DC3_CLK_MODE_SCREEN_OFF;
        cim_outb(DC3_SEQUENCER_DATA, sequencer);

        vg_delay_milliseconds(1);

        /* BLANK THE VGA DISPLAY */
        cim_outw(DC3_SEQUENCER_INDEX, DC3_SEQUENCER_RESET);
        sequencer = cim_inb(DC3_SEQUENCER_DATA);
        sequencer &= ~DC3_RESET_VGA_DISP_ENABLE;
        cim_outb(DC3_SEQUENCER_DATA, sequencer);

        vg_delay_milliseconds(1);
    }

    /* Clear the framebuffer */
    memset(pGeode->FBBase, 0, pGeode->displaySize);

    /* Set the modes */
    if (!xf86SetDesiredModes(pScrni))
        return FALSE;

    pScrni->vtSema = TRUE;

    return TRUE;
}

static void
LXLoadPalette(ScrnInfoPtr pScrni,
              int numColors, int *indizes, LOCO *colors, VisualPtr pVisual)
{
    int i, index, color;

    for (i = 0; i < numColors; i++) {
        index = indizes[i] & 0xFF;
        color = (((unsigned long) (colors[index].red & 0xFF)) << 16) |
            (((unsigned long) (colors[index].green & 0xFF)) << 8) |
            ((unsigned long) (colors[index].blue & 0xFF));

        vg_set_display_palette_entry(index, color);
    }
}

static Bool
LXScreenInit(ScreenPtr pScrn, int argc, char **argv)
{
    ScrnInfoPtr pScrni = xf86ScreenToScrn(pScrn);
    GeodeRec *pGeode = GEODEPTR(pScrni);
    int ret;
    unsigned int dwidth;

    pGeode->starting = TRUE;

    /* If we are using VGA then go ahead and map the memory */

    if (pGeode->useVGA) {

        if (!vgaHWMapMem(pScrni))
            return FALSE;

        vgaHWGetIOBase(VGAHWPTR(pScrni));
    }

    if (!pGeode->NoAccel) {

        pGeode->pExa = exaDriverAlloc();

        if (pGeode->pExa) {

            pGeode->pExa->memoryBase = 0;
            pGeode->pExa->memorySize = 0;

            pGeode->pExa->pixmapOffsetAlign = 32;
            pGeode->pExa->pixmapPitchAlign = 32;
            pGeode->pExa->flags = EXA_OFFSCREEN_PIXMAPS;
            pGeode->pExa->maxX = LX_MAX_WIDTH - 1;
            pGeode->pExa->maxY = LX_MAX_HEIGHT - 1;
        }
        else {
            xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                       "Couldn't allocate the EXA structure.\n");
            pGeode->NoAccel = TRUE;
        }
    }

    /* Map the memory here before doing anything else */

    if (!LXMapMem(pScrni))
        return FALSE;

    LXInitOffscreen(pScrni);

    /* XXX FIXME - Take down any of the structures on failure? */
    if (!LXEnterGraphics(pScrn, pScrni))
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

    if (pScrni->virtualX > pScrni->displayWidth)
        pScrni->displayWidth = pScrni->virtualX;

    /* Point at the visible area to start */

    /* fbScreenInit assumes that the stride is display width *
     * bytes per pixel.  If compression is on, then our stride might
     * be completely different, so we divide the pitch by the
     * bytes per pixel to fake fbScreenInit into doing the right thing */

    dwidth = pGeode->Pitch / ((pScrni->bitsPerPixel + 7) / 8);

    ret = fbScreenInit(pScrn, pGeode->FBBase,
                       pScrni->virtualX, pScrni->virtualY,
                       pScrni->xDpi, pScrni->yDpi, dwidth,
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
        pGeode->NoAccel = LXExaInit(pScrn) ? FALSE : TRUE;

    xf86SetBackingStore(pScrn);

    /* Set up the soft cursor */
    miDCInitialize(pScrn, xf86GetPointerScreenFuncs());

    /* Set up the HW cursor - must follow the soft cursor init */

    if (pGeode->tryHWCursor) {
        if (!LXCursorInit(pScrn))
            xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                       "Hardware cursor initialization failed.\n");
    }

    /* Set up the color map */

    if (!miCreateDefColormap(pScrn))
        return FALSE;

    if (pScrni->bitsPerPixel == 8) {
        /* Must follow initialization of the default colormap */

        if (!xf86HandleColormaps(pScrn, 256, 8,
                                 LXLoadPalette, NULL,
                                 CMAP_PALETTED_TRUECOLOR |
                                 CMAP_RELOAD_ON_MODE_SWITCH)) {
            return FALSE;
        }
    }
    xf86DPMSInit(pScrn, xf86DPMSSet, 0);

    LXInitVideo(pScrn);

    pGeode->PointerMoved = pScrni->PointerMoved;
    pScrni->PointerMoved = GeodePointerMoved;

    pGeode->CloseScreen = pScrn->CloseScreen;
    pScrn->CloseScreen = LXCloseScreen;
    pScrn->SaveScreen = LXSaveScreen;

    if (!xf86CrtcScreenInit(pScrn)) {
        xf86DrvMsg(pScrni->scrnIndex, X_ERROR, "CRTCScreenInit failed.\n");
        return FALSE;
    }

    if (serverGeneration == 1)
        xf86ShowUnusedOptions(pScrni->scrnIndex, pScrni->options);

    pGeode->starting = FALSE;

    return TRUE;
}

static int
LXValidMode(ScrnInfoPtr pScrn, DisplayModePtr pMode, Bool Verbose, int flags)
{
    return MODE_OK;
}

static Bool
LXEnterVT(ScrnInfoPtr pScrni)
{
    return LXEnterGraphics(NULL, pScrni);
}

static void
LXLeaveVT(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    pGeode->PrevDisplayOffset = vg_get_display_offset();
    LXLeaveGraphics(pScrni);
}

void
LXSetupChipsetFPtr(ScrnInfoPtr pScrn)
{
    pScrn->PreInit = LXPreInit;
    pScrn->ScreenInit = LXScreenInit;
    pScrn->SwitchMode = LXSwitchMode;
    pScrn->AdjustFrame = LXAdjustFrame;
    pScrn->EnterVT = LXEnterVT;
    pScrn->LeaveVT = LXLeaveVT;
    pScrn->FreeScreen = GeodeFreeScreen;
    pScrn->ValidMode = LXValidMode;
}
