/*
 * Copyright 1993 by Jon Block <block@frc.com>
 * Modified by Mike Hollick <hollick@graphics.cis.upenn.edu>
 * Modified 1994 by Régis Cridlig <cridlig@dmi.ens.fr>
 *
 * Major Contributors to XFree86 3.2
 *   Modified 1995/6 by Nozomi Ytow
 *   Modified 1996 by Egbert Eich <eich@xfree86.org>
 *   Modified 1996 by David Bateman <dbateman@club-internet.fr>
 *   Modified 1996 by Xavier Ducoin <xavier@rd.lectra.fr>
 *
 * Contributors to XFree86 3.2
 *   Modified 1995/6 by Ken Raeburn <raeburn@raeburn.org>
 *   Modified 1996 by Shigehiro Nomura <nomura@sm.sony.co.jp>
 *   Modified 1996 by Marc de Courville <marc@courville.org>
 *   Modified 1996 by Adam Sulmicki <adam@cfar.umd.edu>
 *   Modified 1996 by Jens Maurer <jmaurer@cck.uni-kl.de>
 *
 * Large parts rewritten for XFree86 4.0
 *   Modified 1998 by David Bateman <dbateman@club-internet.fr>
 *   Modified 1998 by Egbert Eich <eich@xfree86.org>
 *   Modified 1998 by Nozomi Ytow
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
/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 * This software is furnished under license and may be used and copied only in 
 * accordance with the following terms and conditions.  Subject to these 
 * conditions, you may download, copy, install, use, modify and distribute 
 * this software in source and/or binary form. No title or ownership is 
 * transferred hereby.
 * 1) Any source code used, modified or distributed must reproduce and retain 
 *    this copyright notice and list of conditions as they appear in the 
 *    source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of Digital 
 *    Equipment Corporation. Neither the "Digital Equipment Corporation" name 
 *    nor any trademark or logo of Digital Equipment Corporation may be used 
 *    to endorse or promote products derived from this software without the 
 *    prior written permission of Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied warranties,
 *    including but not limited to, any implied warranties of merchantability,
 *    fitness for a particular purpose, or non-infringement are disclaimed. In
 *    no event shall DIGITAL be liable for any damages whatsoever, and in 
 *    particular, DIGITAL shall not be liable for special, indirect, 
 *    consequential, or incidental damages or damages for lost profits, loss 
 *    of revenue or loss of use, whether such damages arise in contract, 
 *    negligence, tort, under statute, in equity, at law or otherwise, even if
 *    advised of the possibility of such damage. 
 */
#include <dix-config.h>

/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86Priv.h"

/* Everything using inb/outb, etc needs "compiler.h" */
#include "compiler.h"

/* Drivers that need to access the PCI config space directly need this */
#include "xf86Pci.h"

#if GET_ABI_MAJOR(ABI_VIDEODRV_VERSION) < 6
/* Standard resources are defined here */
#include "xf86Resources.h"

/* Needed by Resources Access Control (RAC) */
#include "xf86RAC.h"
#endif

/* All drivers using the vgahw module need this */
#include "vgaHW.h"

/* All drivers initialising the SW cursor need this */
#include "mipointer.h"

#if GET_ABI_MAJOR(ABI_VIDEODRV_VERSION) < 6
#define USE_MIBANK 1
#endif

/* All drivers using the mi banking wrapper need this */
#ifdef USE_MIBANK
#include "mibank.h"
#endif

/* All drivers using the mi colormap manipulation need this */
#include "micmap.h"

#include "fb.h"
#include "fboverlay.h"

/* int10 */
#include "xf86int10.h"
#include "vbe.h"

/* Needed by the Shadow Framebuffer */
#include "shadowfb.h"

/* Needed for replacement LoadPalette function for Gamma Correction */
#include "xf86cmap.h"

#include "dixstruct.h"

#include "xf86fbman.h"
/* Driver specific headers */
#include "ct_driver.h"

/* traditionally these had been defined in xf86_OSlib.h */
#ifndef XMODE_RGB
#define XMODE_RGB   0
#define XMODE_NTSC  1
#define XMODE_PAL   2
#define XMODE_SECAM 3
#endif

#if defined(__arm__) && defined(__NetBSD__)
#define AVOID_VGAHW 1
#endif

/* Mandatory functions */
static const OptionInfoRec *	CHIPSAvailableOptions(int chipid, int busid);
static void     CHIPSIdentify(int flags);
#ifdef XSERVER_LIBPCIACCESS
static Bool     CHIPSPciProbe(DriverPtr drv, int entity_num,
			      struct pci_device *dev, intptr_t match_data);
#else
static Bool     CHIPSProbe(DriverPtr drv, int flags);
#endif
static Bool     CHIPSPreInit(ScrnInfoPtr pScrn, int flags);
static Bool     CHIPSScreenInit(SCREEN_INIT_ARGS_DECL);
static Bool     CHIPSEnterVT(VT_FUNC_ARGS_DECL);
static void     CHIPSLeaveVT(VT_FUNC_ARGS_DECL);
static Bool     CHIPSCloseScreen(CLOSE_SCREEN_ARGS_DECL);
static void     CHIPSFreeScreen(FREE_SCREEN_ARGS_DECL);
static ModeStatus CHIPSValidMode(SCRN_ARG_TYPE arg, DisplayModePtr mode,
                                 Bool verbose, int flags);
static Bool	CHIPSSaveScreen(ScreenPtr pScreen, int mode);

/* Internally used functions */
static Bool     chipsClockSelect(ScrnInfoPtr pScrn, int no);
Bool     chipsModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
static void     chipsSave(ScrnInfoPtr pScrn, vgaRegPtr VgaSave,
			  CHIPSRegPtr ChipsSave);
static void     chipsRestore(ScrnInfoPtr pScrn, vgaRegPtr VgaReg,
				 CHIPSRegPtr ChipsReg, Bool restoreFonts);
static void     chipsLock(ScrnInfoPtr pScrn);
static void     chipsUnlock(ScrnInfoPtr pScrn);
static void     chipsClockSave(ScrnInfoPtr pScrn, CHIPSClockPtr Clock);
static void     chipsClockLoad(ScrnInfoPtr pScrn, CHIPSClockPtr Clock);
static Bool     chipsClockFind(ScrnInfoPtr pScrn, DisplayModePtr mode,
			       int no, CHIPSClockPtr Clock);
static void     chipsCalcClock(ScrnInfoPtr pScrn, int Clock,
				 unsigned char *vclk);
static int      chipsGetHWClock(ScrnInfoPtr pScrn);
static Bool     chipsPreInit655xx(ScrnInfoPtr pScrn, int flags);
static Bool     chipsPreInitHiQV(ScrnInfoPtr pScrn, int flags);
static Bool     chipsPreInitWingine(ScrnInfoPtr pScrn, int flags);
static int      chipsSetMonitor(ScrnInfoPtr pScrn);
static Bool	chipsMapMem(ScrnInfoPtr pScrn);
static Bool	chipsUnmapMem(ScrnInfoPtr pScrn);
static void     chipsProtect(ScrnInfoPtr pScrn, Bool on);
static void	chipsBlankScreen(ScrnInfoPtr pScrn, Bool unblank);
static void     chipsRestoreExtendedRegs(ScrnInfoPtr pScrn, CHIPSRegPtr Regs);
static void     chipsRestoreStretching(ScrnInfoPtr pScrn,
				unsigned char ctHorizontalStretch,
				unsigned char ctVerticalStretch);
static Bool     chipsModeInitHiQV(ScrnInfoPtr pScrn, DisplayModePtr mode);
static Bool     chipsModeInitWingine(ScrnInfoPtr pScrn, DisplayModePtr mode);
static Bool     chipsModeInit655xx(ScrnInfoPtr pScrn, DisplayModePtr mode);
static int      chipsVideoMode(int vgaBitsPerPixel,int displayHSize,
			       int displayVSize);
static void     chipsDisplayPowerManagementSet(ScrnInfoPtr pScrn,
				int PowerManagementMode, int flags);
static void     chipsHWCursorOn(CHIPSPtr cPtr, ScrnInfoPtr pScrn);
static void     chipsHWCursorOff(CHIPSPtr cPtr, ScrnInfoPtr pScrn);
static void     chipsFixResume(ScrnInfoPtr pScrn);
static void     chipsLoadPalette(ScrnInfoPtr pScrn, int numColors,
				int *indices, LOCO *colors, VisualPtr pVisual);
static void     chipsLoadPalette16(ScrnInfoPtr pScrn, int numColors,
				int *indices, LOCO *colors, VisualPtr pVisual);
static void chipsSetPanelType(CHIPSPtr cPtr);
static void chipsBlockHandler(BLOCKHANDLER_ARGS_DECL);

/*
 * This is intentionally screen-independent.  It indicates the binding
 * choice made in the first PreInit.
 */
static int pix24bpp = 0;

/*
 * Index of Entity
 */ 
static int CHIPSEntityIndex = -1;


/* Set the non-documented SAR04 register for overlay/video */
#define SAR04 

/*
 * Initialise some arrays that are used in multiple instances of the
 * acceleration code. Set them up here as its a convenient place to do it.
 */
/* alu to C&T conversion for use with source data */
int ChipsAluConv[] =
{
    0x00,			/* dest =  0; GXclear, 0 */
    0x88,			/* dest &= src; GXand, 0x1 */
    0x44,			/* dest =  src & ~dest; GXandReverse, 0x2 */
    0xCC,			/* dest =  src; GXcopy, 0x3 */
    0x22,			/* dest &= ~src; GXandInverted, 0x4 */
    0xAA,			/* dest =  dest; GXnoop, 0x5 */
    0x66,			/* dest =  ^src; GXxor, 0x6 */
    0xEE,			/* dest |= src; GXor, 0x7 */
    0x11,			/* dest =  ~src & ~dest;GXnor, 0x8 */
    0x99,			/* dest ^= ~src ;GXequiv, 0x9 */
    0x55,			/* dest =  ~dest; GXInvert, 0xA */
    0xDD,			/* dest =  src|~dest ;GXorReverse, 0xB */
    0x33,			/* dest =  ~src; GXcopyInverted, 0xC */
    0xBB,			/* dest |= ~src; GXorInverted, 0xD */
    0x77,			/* dest =  ~src|~dest ;GXnand, 0xE */
    0xFF,			/* dest =  0xFF; GXset, 0xF */
};

/* alu to C&T conversion for use with pattern data */
int ChipsAluConv2[] =
{
    0x00,			/* dest =  0; GXclear, 0 */
    0xA0,			/* dest &= src; GXand, 0x1 */
    0x50,			/* dest =  src & ~dest; GXandReverse, 0x2 */
    0xF0,			/* dest =  src; GXcopy, 0x3 */
    0x0A,			/* dest &= ~src; GXandInverted, 0x4 */
    0xAA,			/* dest =  dest; GXnoop, 0x5 */
    0x5A,			/* dest =  ^src; GXxor, 0x6 */
    0xFA,			/* dest |= src; GXor, 0x7 */
    0x05,			/* dest =  ~src & ~dest;GXnor, 0x8 */
    0xA5,			/* dest ^= ~src ;GXequiv, 0x9 */
    0x55,			/* dest =  ~dest; GXInvert, 0xA */
    0xF5,			/* dest =  src|~dest ;GXorReverse, 0xB */
    0x0F,			/* dest =  ~src; GXcopyInverted, 0xC */
    0xAF,			/* dest |= ~src; GXorInverted, 0xD */
    0x5F,			/* dest =  ~src|~dest ;GXnand, 0xE */
    0xFF,			/* dest =  0xFF; GXset, 0xF */
};

/* alu to C&T conversion for use with pattern data as a planemask */
int ChipsAluConv3[] =
{
    0x0A,			/* dest =  0; GXclear, 0 */
    0x8A,			/* dest &= src; GXand, 0x1 */
    0x4A,			/* dest =  src & ~dest; GXandReverse, 0x2 */
    0xCA,			/* dest =  src; GXcopy, 0x3 */
    0x2A,			/* dest &= ~src; GXandInverted, 0x4 */
    0xAA,			/* dest =  dest; GXnoop, 0x5 */
    0x6A,			/* dest =  ^src; GXxor, 0x6 */
    0xEA,			/* dest |= src; GXor, 0x7 */
    0x1A,			/* dest =  ~src & ~dest;GXnor, 0x8 */
    0x9A,			/* dest ^= ~src ;GXequiv, 0x9 */
    0x5A,			/* dest =  ~dest; GXInvert, 0xA */
    0xDA,			/* dest =  src|~dest ;GXorReverse, 0xB */
    0x3A,			/* dest =  ~src; GXcopyInverted, 0xC */
    0xBA,			/* dest |= ~src; GXorInverted, 0xD */
    0x7A,			/* dest =  ~src|~dest ;GXnand, 0xE */
    0xFA,			/* dest =  0xFF; GXset, 0xF */
};

/* The addresses of the acceleration registers */
unsigned int ChipsReg32HiQV[] =
{
    0x00,		/* BR00 Source and Destination offset register */
    0x04,		/* BR01 Color expansion background color       */
    0x08,		/* BR02 Color expansion foreground color       */
    0x0C,		/* BR03 Monochrome source control register     */
    0x10,		/* BR04 BitBLT control register                */
    0x14,		/* BR05 Pattern address register               */
    0x18,		/* BR06 Source address register                */
    0x1C,		/* BR07 Destination  address register          */
    0x20		/* BR08 Destination width and height register  */
};

unsigned int ChipsReg32[] =
{
  /*BitBLT */
    0x83D0,			       /*DR0 src/dest offset                 */
    0x87D0,			       /*DR1 BitBlt. address of freeVram?    */
    0x8BD0,			       /*DR2 BitBlt. paintBrush, or tile pat.*/
    0x8FD0,                            /*DR3                                 */
    0x93D0,			       /*DR4 BitBlt.                         */
    0x97D0,			       /*DR5 BitBlt. srcAddr, or 0 in VRAM   */
    0x9BD0,			       /*DR6 BitBlt. dest?                   */
    0x9FD0,			       /*DR7 BitBlt. width << 16 | height    */
  /*H/W cursor */
    0xA3D0,			       /*DR8 write/erase cursor              */
		                       /*bit 0-1 if 0  cursor is not shown
		                        * if 1  32x32 cursor
					* if 2  64x64 cursor
					* if 3  128x128 cursor
					*/
                                        /* bit 7 if 1  cursor is not shown   */
		                        /* bit 9 cursor expansion in X       */
		                        /* bit 10 cursor expansion in Y      */
    0xA7D0,			        /* DR9 foreGroundCursorColor         */
    0xABD0,			        /* DR0xA backGroundCursorColor       */
    0xAFD0,			        /* DR0xB cursorPosition              */
		                        /* bit 0-7       x coordinate        */
		                        /* bit 8-14      0                   */
		                        /* bit 15        x signum            */
		                        /* bit 16-23     y coordinate        */
		                        /* bit 24-30     0                   */
		                        /* bit 31        y signum            */
    0xB3D0,			        /* DR0xC address of cursor pattern   */
};

#if defined(__arm__) && defined(__NetBSD__)
/*
 * Built in TV output modes: These modes have been tested on NetBSD with
 * CT65550 and StrongARM. They give what seems to be the best output for
 * a roughly 640x480 display. To enable one of the built in modes, add 
 * the identifier "NTSC" or "PAL" to the list of modes in the appropriate
 * "Display" subsection of the "Screen" section in the XF86Config file.
 * Note that the call to xf86SetTVOut(), which tells the kernel to enable
 * TV output results in hardware specific actions. There must be code to
 * support this in the kernel or TV output won't work.
 */
static DisplayModeRec ChipsPALMode = {
	NULL, NULL,     /* prev, next */
	"PAL",          /* identifier of this mode */
	MODE_OK,        /* mode status */
	M_T_BUILTIN,    /* mode type */
	15000,		/* Clock frequency */
	776,		/* HDisplay */
	800,		/* HSyncStart */
	872,		/* HSyncEnd */
	960,		/* HTotal */
	0,		/* HSkew */
	585,		/* VDisplay */
	590,		/* VSyncStart */
	595,		/* VSyncEnd */
	625,		/* VTotal */
	0,		/* VScan */
	V_INTERLACE,	/* Flags */
	-1,		/* ClockIndex */
	15000,		/* SynthClock */
	776,		/* CRTC HDisplay */
	800,            /* CRTC HBlankStart */
	800,            /* CRTC HSyncStart */
	872,            /* CRTC HSyncEnd */
	872,            /* CRTC HBlankEnd */
	960,            /* CRTC HTotal */
	0,              /* CRTC HSkew */
	585,		/* CRTC VDisplay */
	590,		/* CRTC VBlankStart */
	590,		/* CRTC VSyncStart */
	595,		/* CRTC VSyncEnd */
	595,		/* CRTC VBlankEnd */
	625,		/* CRTC VTotal */
	FALSE,		/* CrtcHAdjusted */
	FALSE,		/* CrtcVAdjusted */
	0,		/* PrivSize */
	NULL,		/* Private */
	0.0,		/* HSync */
	0.0		/* VRefresh */
};

/*
** So far, it looks like SECAM uses the same values as PAL
*/
static DisplayModeRec ChipsSECAMMode = {
	NULL,           /* prev */
	&ChipsPALMode,  /* next */   
	"SECAM",        /* identifier of this mode */
	MODE_OK,        /* mode status */
	M_T_BUILTIN,    /* mode type */
	15000,		/* Clock frequency */
	776,		/* HDisplay */
	800,		/* HSyncStart */
	872,		/* HSyncEnd */
	960,		/* HTotal */
	0,		/* HSkew */
	585,		/* VDisplay */
	590,		/* VSyncStart */
	595,		/* VSyncEnd */
	625,		/* VTotal */
	0,		/* VScan */
	V_INTERLACE,	/* Flags */
	-1,		/* ClockIndex */
	15000,		/* SynthClock */
	776,		/* CRTC HDisplay */
	800,            /* CRTC HBlankStart */
	800,            /* CRTC HSyncStart */
	872,            /* CRTC HSyncEnd */
	872,            /* CRTC HBlankEnd */
	960,            /* CRTC HTotal */
	0,              /* CRTC HSkew */
	585,		/* CRTC VDisplay */
	590,		/* CRTC VBlankStart */
	590,		/* CRTC VSyncStart */
	595,		/* CRTC VSyncEnd */
	595,		/* CRTC VBlankEnd */
	625,		/* CRTC VTotal */
	FALSE,		/* CrtcHAdjusted */
	FALSE,		/* CrtcVAdjusted */
	0,		/* PrivSize */
	NULL,		/* Private */
	0.0,		/* HSync */
	0.0		/* VRefresh */
};


static DisplayModeRec ChipsNTSCMode = {
	NULL,           /* prev */
	&ChipsSECAMMode,/* next */
	"NTSC",         /* identifier of this mode */
	MODE_OK,        /* mode status */
	M_T_BUILTIN,    /* mode type */
	11970,		/* Clock frequency */
	584,		/* HDisplay */
	640,		/* HSyncStart */
	696,		/* HSyncEnd */
	760,		/* HTotal */
	0,		/* HSkew */
	450,		/* VDisplay */
	479,		/* VSyncStart */
	485,		/* VSyncEnd */
	525,		/* VTotal */
	0,		/* VScan */
	V_INTERLACE | V_NVSYNC | V_NHSYNC ,	/* Flags */
	-1,		/* ClockIndex */
	11970,		/* SynthClock */
	584,		/* CRTC HDisplay */
	640,            /* CRTC HBlankStart */
	640,            /* CRTC HSyncStart */
	696,            /* CRTC HSyncEnd */
	696,            /* CRTC HBlankEnd */
	760,            /* CRTC HTotal */
	0,              /* CRTC HSkew */
	450,		/* CRTC VDisplay */
	479,		/* CRTC VBlankStart */
	479,		/* CRTC VSyncStart */
	485,		/* CRTC VSyncEnd */
	485,		/* CRTC VBlankEnd */
	525,		/* CRTC VTotal */
	FALSE,		/* CrtcHAdjusted */
	FALSE,		/* CrtcVAdjusted */
	0,		/* PrivSize */
	NULL,		/* Private */
	0.0,		/* HSync */
	0.0		/* VRefresh */
};
#endif

#define CHIPS_VERSION 4000
#define CHIPS_NAME "CHIPS"
#define CHIPS_DRIVER_NAME "chips"
#define CHIPS_MAJOR_VERSION PACKAGE_VERSION_MAJOR
#define CHIPS_MINOR_VERSION PACKAGE_VERSION_MINOR
#define CHIPS_PATCHLEVEL PACKAGE_VERSION_PATCHLEVEL


#ifdef XSERVER_LIBPCIACCESS

#ifndef _XF86_PCIINFO_H
#define PCI_VENDOR_CHIPSTECH		0x102C
/* Chips & Tech */
#define PCI_CHIP_65545			0x00D8
#define PCI_CHIP_65548			0x00DC
#define PCI_CHIP_65550			0x00E0
#define PCI_CHIP_65554			0x00E4
#define PCI_CHIP_65555			0x00E5
#define PCI_CHIP_68554			0x00F4
#define PCI_CHIP_69000			0x00C0
#define PCI_CHIP_69030			0x0C30
#endif

#define CHIPS_DEVICE_MATCH(d, i) \
  { PCI_VENDOR_CHIPSTECH, (d), PCI_MATCH_ANY, PCI_MATCH_ANY, 0, 0, (i) }

static const struct pci_id_match chips_device_match[] = {
  CHIPS_DEVICE_MATCH(PCI_CHIP_65545, CHIPS_CT65545),
  CHIPS_DEVICE_MATCH(PCI_CHIP_65548, CHIPS_CT65548),
  CHIPS_DEVICE_MATCH(PCI_CHIP_65550, CHIPS_CT65550),
  CHIPS_DEVICE_MATCH(PCI_CHIP_65554, CHIPS_CT65554),
  CHIPS_DEVICE_MATCH(PCI_CHIP_65555, CHIPS_CT65555),
  CHIPS_DEVICE_MATCH(PCI_CHIP_68554, CHIPS_CT68554),
  CHIPS_DEVICE_MATCH(PCI_CHIP_69000, CHIPS_CT69000),
  CHIPS_DEVICE_MATCH(PCI_CHIP_69030, CHIPS_CT69030),
  { 0, 0, 0 },
};
#endif

/*
 * This contains the functions needed by the server after loading the driver
 * module.  It must be supplied, and gets passed back by the SetupProc
 * function in the dynamic case.  In the static case, a reference to this
 * is compiled in, and this requires that the name of this DriverRec be
 * an upper-case version of the driver name.
 */

_X_EXPORT DriverRec CHIPS = {
	CHIPS_VERSION,
	CHIPS_DRIVER_NAME,
	CHIPSIdentify,
#ifdef XSERVER_LIBPCIACCESS
	NULL,
#else
	CHIPSProbe,
#endif
	CHIPSAvailableOptions,
	NULL,
	0,
	NULL,

#ifdef XSERVER_LIBPCIACCESS
	chips_device_match,
	CHIPSPciProbe,
#endif
};

static SymTabRec CHIPSChipsets[] = {
    { CHIPS_CT65520,		"ct65520" },
    { CHIPS_CT65525,		"ct65525" },
    { CHIPS_CT65530,		"ct65530" },
    { CHIPS_CT65535,		"ct65535" },
    { CHIPS_CT65540,		"ct65540" },
    { CHIPS_CT65545,		"ct65545" },
    { CHIPS_CT65546,		"ct65546" },
    { CHIPS_CT65548,		"ct65548" },
    { CHIPS_CT65550,		"ct65550" },
    { CHIPS_CT65554,		"ct65554" },
    { CHIPS_CT65555,		"ct65555" },
    { CHIPS_CT68554,		"ct68554" },
    { CHIPS_CT69000,		"ct69000" },
    { CHIPS_CT69030,		"ct69030" },
    { CHIPS_CT64200,		"ct64200" },
    { CHIPS_CT64300,		"ct64300" },
    { -1,			NULL }
};


/* Conversion PCI ID to chipset name */
static PciChipsets CHIPSPCIchipsets[] = {
    { CHIPS_CT65545, PCI_CHIP_65545, RES_SHARED_VGA },
    { CHIPS_CT65548, PCI_CHIP_65548, RES_SHARED_VGA },
    { CHIPS_CT65550, PCI_CHIP_65550, RES_SHARED_VGA },
    { CHIPS_CT65554, PCI_CHIP_65554, RES_SHARED_VGA },
    { CHIPS_CT65555, PCI_CHIP_65555, RES_SHARED_VGA },
    { CHIPS_CT68554, PCI_CHIP_68554, RES_SHARED_VGA },
    { CHIPS_CT69000, PCI_CHIP_69000, RES_SHARED_VGA },
    { CHIPS_CT69030, PCI_CHIP_69030, RES_SHARED_VGA },
    { -1,	     -1,	     RES_UNDEFINED}
};


/* The options supported by the Chips and Technologies Driver */
typedef enum {
    OPTION_LINEAR,
    OPTION_NOACCEL,
    OPTION_HW_CLKS,
    OPTION_SW_CURSOR,
    OPTION_HW_CURSOR,
    OPTION_STN,
    OPTION_USE_MODELINE,
    OPTION_LCD_STRETCH,
    OPTION_LCD_CENTER,
    OPTION_MMIO,
    OPTION_FULL_MMIO,
    OPTION_SUSPEND_HACK,
    OPTION_RGB_BITS,
    OPTION_SYNC_ON_GREEN,
    OPTION_PANEL_SIZE,
    OPTION_18_BIT_BUS,
    OPTION_SHOWCACHE,
    OPTION_SHADOW_FB,
    OPTION_OVERLAY,
    OPTION_COLOR_KEY,
    OPTION_VIDEO_KEY,
    OPTION_FP_CLOCK_8,
    OPTION_FP_CLOCK_16,
    OPTION_FP_CLOCK_24,
    OPTION_FP_CLOCK_32,
    OPTION_SET_MCLK,
    OPTION_ROTATE,
    OPTION_NO_TMED,
    OPTION_CRT2_MEM,
    OPTION_DUAL_REFRESH,
    OPTION_CRT_CLK_INDX,
    OPTION_FP_CLK_INDX,
    OPTION_FP_MODE
} CHIPSOpts;

static const OptionInfoRec Chips655xxOptions[] = {
    { OPTION_LINEAR,		"Linear",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_NOACCEL,		"NoAccel",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_HW_CLKS,		"HWclocks",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SW_CURSOR,		"SWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_HW_CURSOR,		"HWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_STN,		"STN",		OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_USE_MODELINE,	"UseModeline",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_LCD_STRETCH,	"Stretch",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_LCD_CENTER,	"LcdCenter",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_MMIO,		"MMIO",		OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SUSPEND_HACK,	"SuspendHack",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_PANEL_SIZE,	"FixPanelSize",	OPTV_BOOLEAN,	{0}, FALSE },
#if 0
    { OPTION_RGB_BITS,		"RGBbits",	OPTV_INTEGER,	{0}, FALSE },
#endif
    { OPTION_18_BIT_BUS,	"18BitBus",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SHOWCACHE,		"ShowCache",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SHADOW_FB,		"ShadowFB",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_ROTATE, 	        "Rotate",	OPTV_ANYSTR,	{0}, FALSE },
    { OPTION_SET_MCLK,		"SetMclk",	OPTV_FREQ,      {0}, FALSE },
    { OPTION_FP_CLOCK_8,        "FPClock8",	OPTV_FREQ,      {0}, FALSE },
    { OPTION_FP_CLOCK_16,	"FPClock16",	OPTV_FREQ,      {0}, FALSE },
    { OPTION_FP_CLOCK_24,	"FPClock24",	OPTV_FREQ,      {0}, FALSE },
    { OPTION_FP_MODE,		"FPMode",	OPTV_BOOLEAN,   {0}, FALSE },
    { -1,			NULL,		OPTV_NONE,	{0}, FALSE }
};

static const OptionInfoRec ChipsWingineOptions[] = {
    { OPTION_LINEAR,		"Linear",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_NOACCEL,		"NoAccel",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_HW_CLKS,		"HWclocks",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SW_CURSOR,		"SWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_HW_CURSOR,		"HWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
#if 0
    { OPTION_RGB_BITS,		"RGBbits",	OPTV_INTEGER,	{0}, FALSE },
#endif
    { OPTION_SHOWCACHE,		"ShowCache",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SHADOW_FB,		"ShadowFB",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_ROTATE,  	        "Rotate",	OPTV_ANYSTR,	{0}, FALSE },
    { -1,			NULL,		OPTV_NONE,	{0}, FALSE }
};

static const OptionInfoRec ChipsHiQVOptions[] = {
    { OPTION_LINEAR,		"Linear",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_NOACCEL,		"NoAccel",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SW_CURSOR,		"SWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_HW_CURSOR,		"HWcursor",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_STN,		"STN",		OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_USE_MODELINE,	"UseModeline",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_LCD_STRETCH,	"Stretch",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_LCD_CENTER,	"LcdCenter",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_MMIO,		"MMIO",		OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_FULL_MMIO,		"FullMMIO",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SUSPEND_HACK,	"SuspendHack",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_PANEL_SIZE,	"FixPanelSize",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_RGB_BITS,		"RGBbits",	OPTV_INTEGER,	{0}, FALSE },
    { OPTION_SYNC_ON_GREEN,	"SyncOnGreen",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SHOWCACHE,		"ShowCache",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_SHADOW_FB,		"ShadowFB",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_ROTATE, 	        "Rotate",	OPTV_ANYSTR,	{0}, FALSE },
    { OPTION_OVERLAY,		"Overlay",	OPTV_ANYSTR,	{0}, FALSE },
    { OPTION_COLOR_KEY,		"ColorKey",	OPTV_INTEGER,	{0}, FALSE },
    { OPTION_VIDEO_KEY,		"VideoKey",	OPTV_INTEGER,	{0}, FALSE },
    { OPTION_FP_CLOCK_8,	"FPClock8",	OPTV_FREQ,      {0}, FALSE },
    { OPTION_FP_CLOCK_16,	"FPClock16",	OPTV_FREQ,      {0}, FALSE },
    { OPTION_FP_CLOCK_24,	"FPClock24",	OPTV_FREQ,      {0}, FALSE },
    { OPTION_FP_CLOCK_32,	"FPClock32",	OPTV_FREQ,      {0}, FALSE },
    { OPTION_SET_MCLK,		"SetMclk",	OPTV_FREQ,      {0}, FALSE },
    { OPTION_NO_TMED,		"NoTMED",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_CRT2_MEM,		"Crt2Memory",	OPTV_INTEGER,	{0}, FALSE },
    { OPTION_DUAL_REFRESH,	"DualRefresh",	OPTV_BOOLEAN,	{0}, FALSE },
    { OPTION_CRT_CLK_INDX,	"CrtClkIndx",	OPTV_INTEGER,	{0}, FALSE },
    { OPTION_FP_CLK_INDX,	"FPClkIndx",	OPTV_INTEGER,	{0}, FALSE },
    { OPTION_FP_MODE,		"FPMode",	OPTV_BOOLEAN,   {0}, FALSE },
    { -1,			NULL,		OPTV_NONE,	{0}, FALSE }
};

#ifdef XFree86LOADER

static MODULESETUPPROTO(chipsSetup);

static XF86ModuleVersionInfo chipsVersRec =
{
	"chips",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XORG_VERSION_CURRENT,
	CHIPS_MAJOR_VERSION, CHIPS_MINOR_VERSION, CHIPS_PATCHLEVEL,
	ABI_CLASS_VIDEODRV,
	ABI_VIDEODRV_VERSION,
	MOD_CLASS_VIDEODRV,
	{0,0,0,0}
};

/*
 * This is the module init data.
 * Its name has to be the driver name followed by ModuleData
 */
_X_EXPORT XF86ModuleData chipsModuleData = { &chipsVersRec, chipsSetup, NULL };

static pointer
chipsSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;

    if (!setupDone) {
	setupDone = TRUE;
        xf86AddDriver(&CHIPS, module, HaveDriverFuncs);

	/*
	 * Modules that this driver always requires can be loaded here
	 * by calling LoadSubModule().
	 */

	/*
	 * The return value must be non-NULL on success even though there
	 * is no TearDownProc.
	 */
	return (pointer)1;
    } else {
	if (errmaj) *errmaj = LDR_ONCEONLY;
	return NULL;
    }
}

#endif /* XFree86LOADER */

static Bool
CHIPSGetRec(ScrnInfoPtr pScrn)
{
    /*
     * Allocate a CHIPSRec, and hook it into pScrn->driverPrivate.
     * pScrn->driverPrivate is initialised to NULL, so we can check if
     * the allocation has already been done.
     */
    if (pScrn->driverPrivate != NULL)
	return TRUE;

    pScrn->driverPrivate = XNFcallocarray(sizeof(CHIPSRec), 1);

    if (pScrn->driverPrivate == NULL)
	return FALSE;
    
    return TRUE;
}

static void
CHIPSFreeRec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate == NULL)
	return;
    free(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
}

/* Mandatory */
static void
CHIPSIdentify(int flags)
{
    xf86PrintChipsets(CHIPS_NAME, "Driver for Chips and Technologies chipsets",
			CHIPSChipsets);
}

static const OptionInfoRec *
CHIPSAvailableOptions(int chipid, int busid)
{
    int chip = chipid & 0x0000ffff;

    if (busid == BUS_PCI) {
    	if ((chip >= CHIPS_CT65550) && (chip <= CHIPS_CT69030))
	    return ChipsHiQVOptions;
    }
    return Chips655xxOptions;
}

/* Mandatory */
#ifdef XSERVER_LIBPCIACCESS
Bool
CHIPSPciProbe(DriverPtr drv, int entity_num, struct pci_device * dev,
	    intptr_t match_data)
{
    ScrnInfoPtr pScrn = NULL;
    CHIPSPtr cPtr;

    /* Allocate a ScrnInfoRec and claim the slot */
    pScrn = xf86ConfigPciEntity(pScrn, 0, entity_num, CHIPSPCIchipsets, NULL,
				NULL, NULL, NULL, NULL);
    if (pScrn != NULL) {
	/* Fill in what we can of the ScrnInfoRec */
	pScrn->driverVersion	= CHIPS_VERSION;
	pScrn->driverName	= CHIPS_DRIVER_NAME;
	pScrn->name		= CHIPS_NAME;
	pScrn->Probe		= NULL;
	pScrn->PreInit		= CHIPSPreInit;
	pScrn->ScreenInit	= CHIPSScreenInit;
	pScrn->SwitchMode	= CHIPSSwitchMode;
	pScrn->AdjustFrame	= CHIPSAdjustFrame;
	pScrn->EnterVT		= CHIPSEnterVT;
	pScrn->LeaveVT		= CHIPSLeaveVT;
	pScrn->FreeScreen	= CHIPSFreeScreen;
	pScrn->ValidMode	= CHIPSValidMode;

	if (!CHIPSGetRec(pScrn)) {
		return FALSE;
	}
	cPtr = CHIPSPTR(pScrn);
	cPtr->Chipset = match_data;
	/*
	 * For cards that can do dual head per entity, mark the entity
	 * as shareable. 
	 */
	if (match_data == CHIPS_CT69030) {
	    CHIPSEntPtr cPtrEnt = NULL;
	    DevUnion *pPriv;

	    xf86SetEntitySharable(entity_num);
	    /* Allocate an entity private if necessary */
	    if (CHIPSEntityIndex < 0)
	      CHIPSEntityIndex = xf86AllocateEntityPrivateIndex();
	    pPriv = xf86GetEntityPrivate(pScrn->entityList[0], CHIPSEntityIndex);
	    if (!pPriv->ptr) {
		pPriv->ptr = XNFcallocarray(sizeof(CHIPSEntRec), 1);
		cPtrEnt = pPriv->ptr;
		cPtrEnt->lastInstance = -1;
	    } else {
		cPtrEnt = pPriv->ptr;
	    }
	    /*
	     * Set the entity instance for this instance of the driver.  For
	     * dual head per card, instance 0 is the "master" instance, driving
	     * the primary head, and instance 1 is the "slave".
	     */
	    cPtrEnt->lastInstance++;
	    xf86SetEntityInstanceForScreen(pScrn, pScrn->entityList[0],
					   cPtrEnt->lastInstance);
	}
    }

    return (pScrn != NULL);
}
#else
static Bool
CHIPSProbe(DriverPtr drv, int flags)
{
    Bool foundScreen = FALSE;
    int numDevSections, numUsed;
    GDevPtr *devSections;
    int *usedChips;
    int i;
    
    /*
     * Find the config file Device sections that match this
     * driver, and return if there are none.
     */
    if ((numDevSections = xf86MatchDevice(CHIPS_DRIVER_NAME,
					  &devSections)) <= 0) {
	return FALSE;
    }
    /* PCI BUS */
    if (xf86GetPciVideoInfo() ) {
	numUsed = xf86MatchPciInstances(CHIPS_NAME, PCI_VENDOR_CHIPSTECH,
					CHIPSChipsets, CHIPSPCIchipsets, 
					devSections,numDevSections, drv,
					&usedChips);
	if (numUsed > 0) {
	    if (flags & PROBE_DETECT)
		foundScreen = TRUE;
	    else for (i = 0; i < numUsed; i++) {
		EntityInfoPtr pEnt;
		/* Allocate a ScrnInfoRec  */
		ScrnInfoPtr pScrn = NULL;
		if ((pScrn = xf86ConfigPciEntity(pScrn,0,usedChips[i],
						       CHIPSPCIchipsets,NULL,
						       NULL,NULL,NULL,NULL))){
		    pScrn->driverVersion = CHIPS_VERSION;
		    pScrn->driverName    = CHIPS_DRIVER_NAME;
		    pScrn->name          = CHIPS_NAME;
		    pScrn->Probe         = CHIPSProbe;
		    pScrn->PreInit       = CHIPSPreInit;
		    pScrn->ScreenInit    = CHIPSScreenInit;
		    pScrn->SwitchMode    = CHIPSSwitchMode;
		    pScrn->AdjustFrame   = CHIPSAdjustFrame;
		    pScrn->EnterVT       = CHIPSEnterVT;
		    pScrn->LeaveVT       = CHIPSLeaveVT;
		    pScrn->FreeScreen    = CHIPSFreeScreen;
		    pScrn->ValidMode     = CHIPSValidMode;
		    foundScreen = TRUE;
		}

		/*
		 * For cards that can do dual head per entity, mark the entity
		 * as shareable.
		 */
		pEnt = xf86GetEntityInfo(usedChips[i]);
		if (pEnt->chipset == CHIPS_CT69030) {
		    CHIPSEntPtr cPtrEnt = NULL;
		    DevUnion *pPriv;

		    xf86SetEntitySharable(usedChips[i]);
		    /* Allocate an entity private if necessary */
		    if (CHIPSEntityIndex < 0)
			CHIPSEntityIndex = xf86AllocateEntityPrivateIndex();
		    pPriv = xf86GetEntityPrivate(pScrn->entityList[0], 
				CHIPSEntityIndex);
		    if (!pPriv->ptr) {
			pPriv->ptr = XNFcallocarray(sizeof(CHIPSEntRec), 1);
			cPtrEnt = pPriv->ptr;
			cPtrEnt->lastInstance = -1;
		    } else {
			cPtrEnt = pPriv->ptr;
		    }
		    /*
		     * Set the entity instance for this instance of the 
		     * driver.  For dual head per card, instance 0 is the 
		     * "master" instance, driving the primary head, and 
                     * instance 1 is the "slave".
		     */
		    cPtrEnt->lastInstance++;
		    xf86SetEntityInstanceForScreen(pScrn, pScrn->entityList[0],
						   cPtrEnt->lastInstance);
		}

	    }
	    free(usedChips);
	}
    }

    
    free(devSections);
    return foundScreen;
}
#endif


/* Mandatory */
Bool
CHIPSPreInit(ScrnInfoPtr pScrn, int flags)
{
    pciVideoPtr pciPtr;
    ClockRangePtr clockRanges;
    int i;
    CHIPSPtr cPtr;
    Bool res = FALSE;
    CHIPSEntPtr cPtrEnt = NULL;

    if (flags & PROBE_DETECT) return FALSE;

    /* The vgahw module should be loaded here when needed */
    if (!xf86LoadSubModule(pScrn, "vgahw"))
	return FALSE;

    /* Allocate the ChipsRec driverPrivate */
    if (!CHIPSGetRec(pScrn)) {
	return FALSE;
    }
    cPtr = CHIPSPTR(pScrn);

    /* XXX Check the number of entities, and fail if it isn't one. */
    if (pScrn->numEntities != 1)
	return FALSE;

    /* Since the capabilities are determined by the chipset the very
     * first thing to do is, figure out the chipset and its capabilities
     */

    /* This is the general case */
    for (i = 0; i<pScrn->numEntities; i++) {
	cPtr->pEnt = xf86GetEntityInfo(pScrn->entityList[i]);
#ifndef XSERVER_LIBPCIACCESS
	if (cPtr->pEnt->resources) return FALSE;
#endif
	/* If we are using libpciaccess this is already set in CHIPSPciProbe.
	 * If we are using something else we need to set it here.
	 */
	if (!cPtr->Chipset)
		cPtr->Chipset = cPtr->pEnt->chipset;
	pScrn->chipset = (char *)xf86TokenToString(CHIPSChipsets,
						   cPtr->pEnt->chipset);
	if ((cPtr->Chipset == CHIPS_CT64200) ||
	    (cPtr->Chipset == CHIPS_CT64300)) cPtr->Flags |= ChipsWingine;
	if ((cPtr->Chipset >= CHIPS_CT65550) &&
	    (cPtr->Chipset <= CHIPS_CT69030)) cPtr->Flags |= ChipsHiQV;

	/* This driver can handle ISA and PCI buses */
	if (cPtr->pEnt->location.type == BUS_PCI) {
	    pciPtr = xf86GetPciInfoForEntity(cPtr->pEnt->index);
	    cPtr->PciInfo = pciPtr;
#ifndef XSERVER_LIBPCIACCESS
	    cPtr->PciTag = pciTag(cPtr->PciInfo->bus, 
				  cPtr->PciInfo->device,
				  cPtr->PciInfo->func);
#endif
	}
    }
    /* INT10 */
#if 0
    if (xf86LoadSubModule(pScrn, "int10")) {
 	xf86Int10InfoPtr pInt;
#if 1
	xf86DrvMsg(pScrn->scrnIndex,X_INFO,"initializing int10\n");
	pInt = xf86InitInt10(cPtr->pEnt->index);
	xf86FreeInt10(pInt);
#endif
    }
#endif

    if (xf86LoadSubModule(pScrn, "vbe")) {
	cPtr->pVbe =  VBEInit(NULL,cPtr->pEnt->index);
    }
    
    /* Now that we've identified the chipset, setup the capabilities flags */
    switch (cPtr->Chipset) {
    case CHIPS_CT69030:
	cPtr->Flags |= ChipsDualChannelSupport;
    case CHIPS_CT69000:
	cPtr->Flags |= ChipsFullMMIOSupport;
	/* Fall through */
    case CHIPS_CT65555:
	cPtr->Flags |= ChipsImageReadSupport; /* Does the 69000 support it? */
	/* Fall through */
    case CHIPS_CT68554:
	cPtr->Flags |= ChipsTMEDSupport;
	/* Fall through */
    case CHIPS_CT65554:
    case CHIPS_CT65550:
	cPtr->Flags |= ChipsGammaSupport;
	cPtr->Flags |= ChipsVideoSupport;
	/* Fall through */
    case CHIPS_CT65548:
    case CHIPS_CT65546:
    case CHIPS_CT65545:
	cPtr->Flags |= ChipsMMIOSupport;
	/* Fall through */
    case CHIPS_CT64300:
	cPtr->Flags |= ChipsAccelSupport;
	/* Fall through */
    case CHIPS_CT65540:
	cPtr->Flags |= ChipsHDepthSupport;
	cPtr->Flags |= ChipsDPMSSupport;
	/* Fall through */
    case CHIPS_CT65535:
    case CHIPS_CT65530:
    case CHIPS_CT65525:
	cPtr->Flags |= ChipsLinearSupport;
	/* Fall through */
    case CHIPS_CT64200:
    case CHIPS_CT65520:
	break;
    }

    /* Check for shared entities */
    if (xf86IsEntityShared(pScrn->entityList[0])) {
        if (!(cPtr->Flags & ChipsDualChannelSupport)) 
	    return FALSE;

	/* Make sure entity is PCI for now, though this might not be needed. */
	if (cPtr->pEnt->location.type != BUS_PCI)
	    return FALSE;

	/* Allocate an entity private if necessary */
	if (xf86IsEntityShared(pScrn->entityList[0])) {
	    cPtrEnt = xf86GetEntityPrivate(pScrn->entityList[0],
					CHIPSEntityIndex)->ptr;
	    cPtr->entityPrivate = cPtrEnt;
	}
#if 0
	/* Set cPtr->device to the relevant Device section */
	cPtr->device = xf86GetDevFromEntity(pScrn->entityList[0],
					    pScrn->entityInstanceList[0]);
#endif
    }

    /* Set the driver to use the PIO register functions by default */
    CHIPSSetStdExtFuncs(cPtr);

    /* Call the device specific PreInit */
    if (IS_HiQV(cPtr)) 
	res = chipsPreInitHiQV(pScrn, flags);
    else if (IS_Wingine(cPtr)) 
	res = chipsPreInitWingine(pScrn, flags);
    else 
	res = chipsPreInit655xx(pScrn, flags);

    if (cPtr->UseFullMMIO)
	chipsUnmapMem(pScrn);

    if (!res) {
	vbeFree(cPtr->pVbe);
	cPtr->pVbe = NULL;
	return FALSE;
    }
    
/*********/
    /*
     * Setup the ClockRanges, which describe what clock ranges are available,
     * and what sort of modes they can be used for.
     */
    clockRanges = XNFcallocarray(sizeof(ClockRange), 1);
    clockRanges->next = NULL;
    clockRanges->ClockMulFactor = cPtr->ClockMulFactor;
    clockRanges->minClock = cPtr->MinClock;
    clockRanges->maxClock = cPtr->MaxClock;
    clockRanges->clockIndex = -1;		/* programmable */
    if (cPtr->PanelType & ChipsLCD) {
	clockRanges->interlaceAllowed = FALSE;
	clockRanges->doubleScanAllowed = FALSE;
    } else {
	clockRanges->interlaceAllowed = TRUE;
        clockRanges->doubleScanAllowed = TRUE;
    }
    /* 
     * Reduce the amount of video ram for the modes, so that they
     * don't overlap with the DSTN framebuffer
     */
    pScrn->videoRam -= (cPtr->FrameBufferSize + 1023) / 1024;
    
    cPtr->Rounding = 8 * (pScrn->bitsPerPixel <= 8 ? 8 
			  : pScrn->bitsPerPixel);

    i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
			  pScrn->display->modes, clockRanges,
			  NULL, 256, 2048, cPtr->Rounding,
			  128, 2048, pScrn->display->virtualX,
			  pScrn->display->virtualY, cPtr->FbMapSize,
			  LOOKUP_BEST_REFRESH);

    if (i == -1) {
	vbeFree(cPtr->pVbe);
	cPtr->pVbe = NULL;
	CHIPSFreeRec(pScrn);
	return FALSE;
    }

    /*
     * Put the DSTN framebuffer back into the video ram
     */
    pScrn->videoRam += (cPtr->FrameBufferSize + 1023) / 1024;

    /* Prune the modes marked as invalid */
    xf86PruneDriverModes(pScrn);

    if (i == 0 || pScrn->modes == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
	vbeFree(cPtr->pVbe);
	cPtr->pVbe = NULL;
	CHIPSFreeRec(pScrn);
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

    /* If monitor resolution is set on the command line, use it */
    xf86SetDpi(pScrn, 0, 0);

    /* Load bpp-specific modules */
    switch (pScrn->bitsPerPixel) {
    default:
	if (xf86LoadSubModule(pScrn, "fb") == NULL) {
	    vbeFree(cPtr->pVbe);
	    cPtr->pVbe = NULL;
	    CHIPSFreeRec(pScrn);
	    return FALSE;
	}	
	break;
    }

    if (!xf86LoadSubModule(pScrn, "exa")) {
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Falling back to shadowfb\n");
	cPtr->Flags &= ~(ChipsAccelSupport);
	cPtr->Flags |= ChipsShadowFB;
    }

    if (cPtr->Flags & ChipsShadowFB) {
	if (!xf86LoadSubModule(pScrn, "shadowfb")) {
	    vbeFree(cPtr->pVbe);
	    cPtr->pVbe = NULL;
	    CHIPSFreeRec(pScrn);
	    return FALSE;
	}
    }
    
    if (cPtr->Accel.UseHWCursor) {
	if (!xf86LoadSubModule(pScrn, "ramdac")) {
	    vbeFree(cPtr->pVbe);
	    cPtr->pVbe = NULL;
	    CHIPSFreeRec(pScrn);
	    return FALSE;
	}
    }

#ifndef XSERVER_LIBPCIACCESS
    if (cPtr->Flags & ChipsLinearSupport) 
 	xf86SetOperatingState(resVgaMem, cPtr->pEnt->index, ResDisableOpr);

    if (cPtr->MMIOBaseVGA)
 	xf86SetOperatingState(resVgaIo, cPtr->pEnt->index, ResDisableOpr);
#endif

    vbeFree(cPtr->pVbe);
    cPtr->pVbe = NULL;
    return TRUE;
}

static Bool
chipsPreInitHiQV(ScrnInfoPtr pScrn, int flags)
{
    int bytesPerPixel;
    unsigned char tmp;
    MessageType from;
    int i;
    unsigned int Probed[3], FPclkI, CRTclkI;
    double real;
    int val, indx;
    const char *s;
    pointer pVbeModule = NULL;

    vgaHWPtr hwp;
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSEntPtr cPtrEnt = NULL;
    CHIPSPanelSizePtr Size = &cPtr->PanelSize;
    CHIPSMemClockPtr MemClk = &cPtr->MemClock;
    CHIPSClockPtr SaveClk = &(cPtr->SavedReg.Clock);
#ifndef XSERVER_LIBPCIACCESS
    resRange linearRes[] = { {ResExcMemBlock|ResBios|ResBus,0,0},_END };
#endif

    /* Set pScrn->monitor */
    pScrn->monitor = pScrn->confScreen->monitor;
    
    /* All HiQV chips support 16/24/32 bpp, default to 16bpp for speed & vram */
    if (!xf86SetDepthBpp(pScrn, 16, 0, 0, Support24bppFb | Support32bppFb |
 				SupportConvert32to24 | PreferConvert32to24))
	return FALSE;
    else {
	/* Check that the returned depth is one we support */
	switch (pScrn->depth) {
	case 1:
	case 4:
	case 8:
	case 15:
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

    /* Get the depth24 pixmap format */
    if (pScrn->depth == 24 && pix24bpp == 0)
	pix24bpp = xf86GetBppFromDepth(pScrn, 24);

    /*
     * Allocate a vgaHWRec, this must happen after xf86SetDepthBpp for 1bpp
     */
    if (!vgaHWGetHWRec(pScrn))
        return FALSE;

    hwp = VGAHWPTR(pScrn);
#if defined(__arm__)
    vgaHWSetMmioFuncs(hwp, (CARD8 *)IOPortBase, 0);
#elif defined(__powerpc__)
    vgaHWSetMmioFuncs(hwp, ioBase, 0);
#else
    vgaHWSetStdFuncs(hwp);
#endif
    vgaHWGetIOBase(hwp);
#if GET_ABI_MAJOR(ABI_VIDEODRV_VERSION) < 12
    cPtr->PIOBase = hwp->PIOOffset;
#else
    cPtr->PIOBase = 0;
#endif

    /*
     * Must allow ensure that storage for the 2nd set of vga registers is
     * allocated for dual channel cards
     */
    if ((cPtr->Flags & ChipsDualChannelSupport) && 
		(! xf86IsEntityShared(pScrn->entityList[0])))
	vgaHWAllocDefaultRegs(&(cPtr->VgaSavedReg2));

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

    if (!xf86SetDefaultVisual(pScrn, -1)) 
	return FALSE;

    /* The gamma fields must be initialised when using the new cmap code */
    if (pScrn->depth > 1) {
	Gamma zeros = {0.0, 0.0, 0.0};

	if (!xf86SetGamma(pScrn, zeros))
	    return FALSE;
    }

    bytesPerPixel = max(1, pScrn->bitsPerPixel >> 3);

    /* Collect all of the relevant option flags (fill in pScrn->options) */
    xf86CollectOptions(pScrn, NULL);
    /* Process the options */
    if (!(cPtr->Options = malloc(sizeof(ChipsHiQVOptions))))
	return FALSE;
    memcpy(cPtr->Options, ChipsHiQVOptions, sizeof(ChipsHiQVOptions));
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, cPtr->Options);

    /* Set the bits per RGB */
    if (pScrn->depth > 1) {
	/* Default to 6, is this right for HiQV?? */
	pScrn->rgbBits = 8;
	if (xf86GetOptValInteger(cPtr->Options, OPTION_RGB_BITS, &val)) {
	    if (val == 6 || val == 8) {
		pScrn->rgbBits = val;
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Bits per RGB set to "
			   "%d\n", pScrn->rgbBits);
	    } else 
		xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "Invalid number of "
			   "rgb bits %d\n", val);
	}
    }
    if ((cPtr->Flags & ChipsAccelSupport) &&
	(xf86ReturnOptValBool(cPtr->Options, OPTION_NOACCEL, FALSE))) {
	cPtr->Flags &= ~ChipsAccelSupport;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Acceleration disabled\n");
    }
    
    from = X_DEFAULT;
    if (pScrn->bitsPerPixel < 8) {
	/* Default to SW cursor for 1/4 bpp */
	cPtr->Accel.UseHWCursor = FALSE;
    } else {
	cPtr->Accel.UseHWCursor = TRUE;
    }
    if (xf86GetOptValBool(cPtr->Options, OPTION_HW_CURSOR,
			  &cPtr->Accel.UseHWCursor))
	from = X_CONFIG;
    if (xf86GetOptValBool(cPtr->Options, OPTION_SW_CURSOR,
			  &cPtr->Accel.UseHWCursor)) {
	from = X_CONFIG;
	cPtr->Accel.UseHWCursor = !cPtr->Accel.UseHWCursor;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Using %s cursor\n",
	       (cPtr->Accel.UseHWCursor) ? "HW" : "SW");

    /* Default to nonlinear for < 8bpp and linear for >= 8bpp. */
    if (pScrn->bitsPerPixel < 8) {
	if (!xf86ReturnOptValBool(cPtr->Options, OPTION_LINEAR, FALSE)) {
	cPtr->Flags &= ~ChipsLinearSupport;
	from = X_CONFIG;
	}
    } else if (!xf86ReturnOptValBool(cPtr->Options, OPTION_LINEAR, TRUE)) {
	cPtr->Flags &= ~ChipsLinearSupport;
	from = X_CONFIG;
    }

    if (!(cPtr->Flags & ChipsLinearSupport)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Linear framebuffer required\n");
	return FALSE;
    }

    /* linear base */
    if (cPtr->Flags & ChipsLinearSupport) {
	if (cPtr->pEnt->location.type == BUS_PCI) {
	    /* Tack on 0x800000 to access the big-endian aperture? */
#if X_BYTE_ORDER == X_BIG_ENDIAN
	    if (BE_SWAP_APRETURE(pScrn,cPtr))
	        cPtr->FbAddress =  (PCI_REGION_BASE(cPtr->PciInfo, 0, REGION_MEM) & 0xff800000) + 0x800000L;
	    else
#endif
	        cPtr->FbAddress =  PCI_REGION_BASE(cPtr->PciInfo, 0, REGION_MEM) & 0xff800000;

	    from = X_PROBED;
#ifndef XSERVER_LIBPCIACCESS
	    if (xf86RegisterResources(cPtr->pEnt->index,NULL,ResNone))
		cPtr->Flags &= ~ChipsLinearSupport;
#endif
	} else 	{
	    if (cPtr->pEnt->device->MemBase) {
		cPtr->FbAddress = cPtr->pEnt->device->MemBase;
		from = X_CONFIG;
	    } else {
		cPtr->FbAddress = ((unsigned int)
				   (cPtr->readXR(cPtr, 0x06))) << 24;
		cPtr->FbAddress |= ((unsigned int)
				    (0x80 & (cPtr->readXR(cPtr, 0x05)))) << 16;
		from = X_PROBED;
	    }
#ifndef XSERVER_LIBPCIACCESS
	    linearRes[0].rBegin = cPtr->FbAddress;
	    linearRes[0].rEnd = cPtr->FbAddress + 0x800000;
	    if (xf86RegisterResources(cPtr->pEnt->index,linearRes,ResNone)) {
		cPtr->Flags &= ~ChipsLinearSupport;
		from = X_PROBED;
	    }
#endif
	}
    }
    if (cPtr->Flags & ChipsLinearSupport) {
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		   "Enabling linear addressing\n");
	xf86DrvMsg(pScrn->scrnIndex, from,
		   "base address is set at 0x%lX.\n", cPtr->FbAddress);
#if X_BYTE_ORDER == X_BIG_ENDIAN
	if (BE_SWAP_APRETURE(pScrn,cPtr))
	    cPtr->IOAddress = cPtr->FbAddress - 0x400000L;
	else
#endif
	    cPtr->IOAddress = cPtr->FbAddress + 0x400000L;
 	xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT,
		   "IOAddress is set at 0x%lX.\n",(unsigned long)cPtr->IOAddress);
	
    } else
	xf86DrvMsg(pScrn->scrnIndex, from,
		   "Disabling linear addressing\n");

    if ((s = xf86GetOptValString(cPtr->Options, OPTION_ROTATE))
	|| xf86ReturnOptValBool(cPtr->Options, OPTION_SHADOW_FB, FALSE)) {
	if (!(cPtr->Flags & ChipsLinearSupport)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "Option \"ShadowFB\" ignored. Not supported without linear addressing\n");
	} else if (pScrn->depth < 8) {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "Option \"ShadowFB\" ignored. Not supported at this depth.\n");
	} else {
	    cPtr->Rotate = 0;
	    if (s) {
		if(!xf86NameCmp(s, "CW")) {
		    /* accel is disabled below for shadowFB */
		    cPtr->Flags |= ChipsShadowFB;
		    cPtr->Rotate = 1;
		    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
			       "Rotating screen clockwise\n");
		} else if(!xf86NameCmp(s, "CCW")) {
		    cPtr->Flags |= ChipsShadowFB;
		    cPtr->Rotate = -1;
		    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,  "Rotating screen"
			       "counter clockwise\n");
		} else {
		    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "\"%s\" is not a valid"
			       "value for Option \"Rotate\"\n", s);
		    xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
			       "Valid options are \"CW\" or \"CCW\"\n");
		}
	    } else {
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
			   "Using \"Shadow Framebuffer\"\n");
		cPtr->Flags |= ChipsShadowFB;
	    }
	}
    }
    
    if(xf86GetOptValInteger(cPtr->Options, OPTION_VIDEO_KEY,
	&(cPtr->videoKey))) {
         xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "video key set to 0x%x\n",
		cPtr->videoKey);
    } else {
       cPtr->videoKey =  (1 << pScrn->offset.red) | 
			(1 << pScrn->offset.green) |
			(((pScrn->mask.blue >> pScrn->offset.blue) - 1)
			<< pScrn->offset.blue); 
    }

    if (cPtr->Flags & ChipsShadowFB) {
	if (cPtr->Flags & ChipsAccelSupport) {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING, 
		"HW acceleration is not supported with shadow fb\n");
	    cPtr->Flags &= ~ChipsAccelSupport;
	}
	if (cPtr->Rotate && cPtr->Accel.UseHWCursor) {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING, 
		"HW cursor is not supported with rotate\n");
	    cPtr->Accel.UseHWCursor = FALSE;
	}
    }

    if (xf86ReturnOptValBool(cPtr->Options, OPTION_MMIO, TRUE)) {
        cPtr->UseMMIO = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		   "Using MMIO\n");
    
	/* Are we using MMIO mapping of VGA registers */
	if (xf86ReturnOptValBool(cPtr->Options, OPTION_FULL_MMIO, FALSE)) {
	    if ((cPtr->Flags & ChipsLinearSupport) 
		&& (cPtr->Flags & ChipsFullMMIOSupport) 
		&& (cPtr->pEnt->location.type == BUS_PCI)) {

		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
			   "Enabling Full MMIO\n");
		cPtr->UseFullMMIO = TRUE;
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
			   "Using Full MMIO\n");

		/* 
		 * We need to map the framebuffer to read/write regs.
		 * but can't do that without the FbMapSize. So need to
		 * fake value for PreInit. This isn't a problem as
		 * framebuffer isn't actually used in PreInit
		 */
		cPtr->FbMapSize = 1024 * 1024;

		/* Map the linear framebuffer */
		if (!chipsMapMem(pScrn))
		  return FALSE;
	  
		/* Setup the MMIO register functions */
		if (cPtr->MMIOBaseVGA) {
		  CHIPSSetMmioExtFuncs(cPtr);
		  CHIPSHWSetMmioFuncs(pScrn, cPtr->MMIOBaseVGA, 0x0);
		}
	    } else {
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
			   "FULL_MMIO option ignored\n");
	    }
	}
    } else {
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,"Disabling MMIO: "
		   "no acceleration, no hw_cursor\n");
	cPtr->UseMMIO = FALSE;
	cPtr->Accel.UseHWCursor = FALSE;
	cPtr->Flags &= ~ChipsAccelSupport;
    }


    if (cPtr->Flags & ChipsDualChannelSupport) {

	if (xf86IsEntityShared(pScrn->entityList[0])) {
	    cPtrEnt = xf86GetEntityPrivate(pScrn->entityList[0],
					CHIPSEntityIndex)->ptr;
#if 1
	    /*
	     * XXX This assumes that the lower number screen is always the 
	     * "master" head, and that the "master" is the first CRTC.  This
	     * can result in unexpected behaviour when the config file marks
	     * the primary CRTC as the second screen.
	     */
	    if (xf86IsPrimInitDone(pScrn->entityList[0])) 
#else
	    /*
	     * This is an alternative version that determines which is the 
	     * secondary CRTC from the screen field in cPtr->pEnt->device.
	     * It doesn't currently work because there are things that assume
	     * the primary CRTC is initialised first.
	     */
	    if (cPtr->pEnt->device->screen == 1) 
		
#endif
	    {
		/* This is the second crtc */
		cPtr->SecondCrtc = TRUE;
		cPtr->UseDualChannel = TRUE;
	    } else
		cPtr->SecondCrtc = FALSE;

	} else {
	    if (xf86ReturnOptValBool(cPtr->Options, 
				   OPTION_DUAL_REFRESH, FALSE)) {
		cPtr->Flags |= ChipsDualRefresh;
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
			   "Dual Refresh mode enabled\n");
		cPtr->UseDualChannel = TRUE;
	    }
	}

	/* Store IOSS/MSS so that we can restore them */
	cPtr->storeIOSS = cPtr->readIOSS(cPtr);
	cPtr->storeMSS = cPtr->readMSS(cPtr);
        DUALOPEN;
    }

	    /* memory size */
    if (cPtr->pEnt->device->videoRam != 0) {
	pScrn->videoRam = cPtr->pEnt->device->videoRam;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "VideoRAM: %d kByte\n",
		   pScrn->videoRam);
    } else {
        /* not given, probe it    */
	switch (cPtr->Chipset) {
	case CHIPS_CT69030:
	    /* The ct69030 has 4Mb of SGRAM integrated */
	    pScrn->videoRam = 4096;
	    cPtr->Flags |= Chips64BitMemory;
	    break;
	case CHIPS_CT69000:
	    /* The ct69000 has 2Mb of SGRAM integrated */
	    pScrn->videoRam = 2048;
	    cPtr->Flags |= Chips64BitMemory;
	    break;
	case CHIPS_CT65550:
	    /* XR43: DRAM interface   */
	    /* bit 2-1: memory size   */
	    /*          0: 1024 kB    */
	    /*          1: 2048 kB    */
	    /*          2:  reserved  */
	    /*          3: reserved   */
	    switch (((cPtr->readXR(cPtr, 0x43)) & 0x06) >> 1) {
	    case 0:
		pScrn->videoRam = 1024;
		break;
	    case 1:
	    case 2:
	    case 3:
		pScrn->videoRam = 2048;
		break;
	    }
	    break;
	default:
	    /* XRE0: Software reg     */
	    /* bit 3-0: memory size   */
	    /*          0: 512k       */
	    /*          1: 1024k      */
	    /*          2: 1536k(1.5M)*/
	    /*          3: 2048k      */
	    /*          7: 4096k      */
	    tmp = (cPtr->readXR(cPtr, 0xE0)) & 0xF;
	    switch (tmp) {
	    case 0:
		pScrn->videoRam = 512;
		break;
	    case 1:
		pScrn->videoRam = 1024;
		break;
	    case 2:
		pScrn->videoRam = 1536;
		break;
	    case 3:
		pScrn->videoRam = 2048;
		break;
	    case 7:
		pScrn->videoRam = 4096;
		break;
	    default:
		pScrn->videoRam = 1024;
		break;
	    }
	    /* XR43: DRAM interface        */
	    /* bit 4-5 mem interface width */
	    /* 00: 32Bit		   */
	    /* 01: 64Bit		   */
	    tmp = cPtr->readXR(cPtr, 0x43);
	    if ((tmp & 0x10) == 0x10)
		cPtr->Flags |= Chips64BitMemory;
	    break;
	}
    }

#if X_BYTE_ORDER == X_BIG_ENDIAN
    if (cPtr->pEnt->chipset == CHIPS_CT69030 && ((cPtr->readXR(cPtr, 0x71) & 0x2)) == 0) /* CFG9: Pipeline variable ByteSwap mapping */
	cPtr->dualEndianAp = TRUE;
    else  /* CFG9: Pipeline A/B mapping */
	cPtr->dualEndianAp = FALSE;
#endif

    if ((cPtr->Flags & ChipsDualChannelSupport) &&
		(xf86IsEntityShared(pScrn->entityList[0]))) {
       /* 
	* This takes gives either half or the amount of memory specified
        * with the Crt2Memory option 
        */
	pScrn->memPhysBase = cPtr->FbAddress;

        if(cPtr->SecondCrtc == FALSE) {
	    int crt2mem = -1, adjust;
	  
	    xf86GetOptValInteger(cPtr->Options, OPTION_CRT2_MEM, &crt2mem);
	    if (crt2mem > 0) {
		adjust = crt2mem;
		from = X_CONFIG;
	    } else {
		adjust = pScrn->videoRam / 2;
		from = X_DEFAULT;
	    }
	    xf86DrvMsg(pScrn->scrnIndex, from,
			   "CRT2 will use %dK of VideoRam\n",
			   adjust);

	    cPtrEnt->mastervideoRam = pScrn->videoRam - adjust;
	    pScrn->videoRam = cPtrEnt->mastervideoRam;
	    cPtrEnt->slavevideoRam = adjust;
	    cPtrEnt->masterFbAddress = cPtr->FbAddress;
	    cPtr->FbMapSize = 
	       cPtrEnt->masterFbMapSize = pScrn->videoRam * 1024;
	    cPtrEnt->slaveFbMapSize = cPtrEnt->slavevideoRam * 1024;
	    pScrn->fbOffset = 0;
	} else {
	    cPtrEnt->slaveFbAddress = cPtr->FbAddress + 
				cPtrEnt->masterFbMapSize;
	    cPtr->FbMapSize = cPtrEnt->slaveFbMapSize;
	    pScrn->videoRam = cPtrEnt->slavevideoRam;
	    pScrn->fbOffset = cPtrEnt->masterFbMapSize;
	}
	
        cPtrEnt->refCount++;
    } else {
        /* Normal Handling of video ram etc */
        cPtr->FbMapSize = pScrn->videoRam * 1024;
    }

    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "VideoRAM: %d kByte\n",
		   pScrn->videoRam);

    /* Store register values that might be messed up by a suspend resume */
    /* Do this early as some of the other code in PreInit relies on it   */
    cPtr->SuspendHack.vgaIOBaseFlag = ((hwp->readMiscOut(hwp)) & 0x01);
    cPtr->IOBase = (unsigned int)(cPtr->SuspendHack.vgaIOBaseFlag ?
				  0x3D0 : 0x3B0);

    /* 
     * Do DDC here: if VESA BIOS detects an external monitor it
     * might switch. SetPanelType() will detect this.
     */
    if ((pVbeModule = xf86LoadSubModule(pScrn, "ddc"))) {
	Bool ddc_done = FALSE;
	xf86MonPtr pMon;
	
	if (cPtr->pVbe) {
	    if ((pMon 
		 = xf86PrintEDID(vbeDoEDID(cPtr->pVbe, pVbeModule))) != NULL) {
		ddc_done = TRUE;
		xf86SetDDCproperties(pScrn,pMon);
	    }
	}

	if (!ddc_done)
	    if (xf86LoadSubModule(pScrn, "i2c")) {
		if (chips_i2cInit(pScrn)) {
		    if ((pMon = xf86PrintEDID(xf86DoEDID_DDC2(XF86_SCRN_ARG(pScrn),
						      cPtr->I2C))) != NULL) {
		       ddc_done = TRUE;
		       xf86SetDDCproperties(pScrn,pMon);
		    }
		}
	    }
	if (!ddc_done)
	    chips_ddc1(pScrn);
    }

    /*test STN / TFT */
    tmp = cPtr->readFR(cPtr, 0x10);

    /* XR51 or FR10: DISPLAY TYPE REGISTER                      */
    /* XR51[1-0] or FR10[1:0] for ct65550 : PanelType,          */
    /* 0 = Single Panel Single Drive, 3 = Dual Panel Dual Drive */
    switch (tmp & 0x3) {
    case 0:
	if (xf86ReturnOptValBool(cPtr->Options, OPTION_STN, FALSE)) {
	    cPtr->PanelType |= ChipsSS;
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "SS-STN probed\n");
	} else {
	    cPtr->PanelType |= ChipsTFT;
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "TFT probed\n");
	}
	break;
    case 2:
	cPtr->PanelType |= ChipsDS;
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "DS-STN probed\n");
    case 3:
	cPtr->PanelType |= ChipsDD;
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "DD-STN probed\n");
	break;
    default:
	break;
    }
    
    chipsSetPanelType(cPtr);
    from = X_PROBED;
    {
      Bool fp_mode;
      if (xf86GetOptValBool(cPtr->Options, OPTION_FP_MODE, &fp_mode)) {
	if (fp_mode) {
	  xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Forcing FP Mode on\n");
	  cPtr->PanelType |= ChipsLCD;
	} else {
	  xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Forcing FP Mode off\n");
	  cPtr->PanelType = ~ChipsLCD;
	} 
	from = X_CONFIG;
      }
    }
    if ((cPtr->PanelType & ChipsLCD) && (cPtr->PanelType & ChipsCRT))
	xf86DrvMsg(pScrn->scrnIndex, from, "LCD/CRT\n");
    else if (cPtr->PanelType & ChipsLCD)
        xf86DrvMsg(pScrn->scrnIndex, from, "LCD\n");
    else if (cPtr->PanelType & ChipsCRT) {
        xf86DrvMsg(pScrn->scrnIndex, from, "CRT\n");
	/* monitor info */
#if 1
	cPtr->Monitor = chipsSetMonitor(pScrn);
#endif
    }
    /* screen size */
    /* 
     * In LCD mode / dual mode we want to derive the timing values from
     * the ones preset by bios
     */
    if (cPtr->PanelType & ChipsLCD) {

	/* for 65550 we only need H/VDisplay values for screen size */
	unsigned char fr25, tmp1;
#ifdef DEBUG
	unsigned char fr26;
	char tmp2;
#endif
 	fr25 = cPtr->readFR(cPtr, 0x25);
 	tmp = cPtr->readFR(cPtr, 0x20);
	Size->HDisplay = ((tmp + ((fr25 & 0x0F) << 8)) + 1) << 3;
 	tmp = cPtr->readFR(cPtr, 0x30);
 	tmp1 = cPtr->readFR(cPtr, 0x35);
	Size->VDisplay = ((tmp1 & 0x0F) << 8) + tmp + 1;
#ifdef DEBUG
 	tmp = cPtr->readFR(cPtr, 0x21);
	Size->HRetraceStart = ((tmp + ((fr25 & 0xF0) << 4)) + 1) << 3;
 	tmp1 = cPtr->readFR(cPtr, 0x22);
	tmp2 = (tmp1 & 0x1F) - (tmp & 0x3F);
	Size->HRetraceEnd = ((((tmp2 < 0) ? (tmp2 + 0x40) : tmp2) << 3)
			     + Size->HRetraceStart);
 	tmp = cPtr->readFR(cPtr, 0x23);
 	fr26 = cPtr->readFR(cPtr, 0x26);
	Size->HTotal = ((tmp + ((fr26 & 0x0F) << 8)) + 5) << 3;
	xf86ErrorF("x=%i, y=%i; xSync=%i, xSyncEnd=%i, xTotal=%i\n",
	       Size->HDisplay, Size->VDisplay,
	       Size->HRetraceStart,Size->HRetraceEnd,
	       Size->HTotal);
#endif
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Display Size: x=%i; y=%i\n",
		   Size->HDisplay, Size->VDisplay);
	/* Warn the user if the panel size has been overridden by
	 * the modeline values
	 */
	if (xf86ReturnOptValBool(cPtr->Options, OPTION_PANEL_SIZE, FALSE)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		       "Display size overridden by modelines.\n");
	}
    }

    /* Frame Buffer */                 /* for LCDs          */ 
    if (IS_STN(cPtr->PanelType)) {
	tmp = cPtr->readFR(cPtr, 0x1A); /*Frame Buffer Ctrl. */
	if (tmp & 1) {
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Frame Buffer used\n");
	    if (!(tmp & 0x80)) {
		/* Formula for calculating the size of the framebuffer. 3
		 * bits per pixel 10 pixels per 32 bit dword. If frame
		 * acceleration is enabled the size can be halved.
		 */
		cPtr->FrameBufferSize = ( Size->HDisplay * 
				  Size->VDisplay / 5 ) * ((tmp & 2) ? 1 : 2);
		xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
			   "Using embedded Frame Buffer, size %d bytes\n",
			   cPtr->FrameBufferSize);
	    } else
		xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
			   "Using external Frame Buffer used\n");
	}
	if (tmp & 2)
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		       "Frame accelerator enabled\n");
    }

    /* bus type */
    tmp = (cPtr->readXR(cPtr, 0x08)) & 1;
    if (tmp == 1) {	       /*PCI */
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "PCI Bus\n");
	cPtr->Bus = ChipsPCI;
    } else {   /* XR08: Linear addressing base, not for PCI */
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "VL Bus\n");
	cPtr->Bus = ChipsVLB;
    }

    /* disable acceleration for 1 and 4 bpp */
    if (pScrn->bitsPerPixel < 8) {
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		   "Disabling acceleration for %d bpp\n", pScrn->bitsPerPixel);
	cPtr->Flags &= ~ChipsAccelSupport;
    }
    
    /* Set the flags for Colour transparency. This is dependent
     * on the revision on the chip. Until exactly which chips
     * have this bug are found, only allow 8bpp Colour transparency */
    if ((pScrn->bitsPerPixel == 8) || ((cPtr->Chipset >= CHIPS_CT65555) &&
	    (pScrn->bitsPerPixel >= 8) && (pScrn->bitsPerPixel <= 24)))
        cPtr->Flags |= ChipsColorTransparency;
    else
        cPtr->Flags &= ~ChipsColorTransparency;

    /* DAC info */
    if (!((cPtr->readXR(cPtr, 0xD0)) & 0x01))
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Internal DAC disabled\n");

    /* MMIO address offset */
    cPtr->Regs32 = ChipsReg32HiQV;
  
    /* sync reset ignored on this chipset */
    cPtr->SyncResetIgn = TRUE;   /* !! */

    /* We use a programmable clock */
    pScrn->numClocks = 26;		/* Some number */
    pScrn->progClock = TRUE;
    cPtr->ClockType = HiQV_STYLE | TYPE_PROGRAMMABLE;

    SaveClk->Clock = 0;

    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Using programmable clocks\n");

    /* Set the maximum memory clock. */
    switch (cPtr->Chipset) {
    case CHIPS_CT65550:
	if (((cPtr->readXR(cPtr, 0x04)) & 0xF) < 6)
	    MemClk->Max = 38000; /* Revision A chips */
	else
	    MemClk->Max = 50000; /* Revision B chips */
	break;
    case CHIPS_CT65554:
    case CHIPS_CT65555:
    case CHIPS_CT68554:
	MemClk->Max = 55000;
	break;
    case CHIPS_CT69000:
	MemClk->Max = 83000;
	break;
    case CHIPS_CT69030:
	MemClk->Max = 100000;
	break;
    }

    /* Probe the dot clocks */
    for (i = 0; i < 3; i++) {
      unsigned int N,M,PSN,P,VCO_D;
      int offset = i * 4;
      
      tmp = cPtr->readXR(cPtr,0xC2 + offset);
      M = (cPtr->readXR(cPtr, 0xC0 + offset) 
	   | (tmp & 0x03)) + 2;
      N = (cPtr->readXR(cPtr, 0xC1 + offset) 
	| (( tmp >> 4) & 0x03)) + 2;
      tmp = cPtr->readXR(cPtr, 0xC3 + offset);
      PSN = (cPtr->Chipset == CHIPS_CT69000 || cPtr->Chipset == CHIPS_CT69030)
		? 1 : (((tmp & 0x1) ? 1 : 4) * ((tmp & 0x02) ? 5 : 1));
      VCO_D = ((tmp & 0x04) ? ((cPtr->Chipset == CHIPS_CT69000 || 
				cPtr->Chipset == CHIPS_CT69030) ? 1 : 16) : 4);
      P = ((tmp & 0x70) >> 4);
      Probed[i] = VCO_D * Fref / N;
      Probed[i] = Probed[i] * M / (PSN * (1 << P));
      Probed[i] = Probed[i] / 1000;
    }
    CRTclkI = (hwp->readMiscOut(hwp) >> 2) & 0x03; 
    if (CRTclkI == 3) CRTclkI = 2;
    if (cPtr->Chipset == CHIPS_CT69030) 
	FPclkI = (cPtr->readFR(cPtr, 0x01) >> 2) & 0x3; 
    else
	FPclkI = (cPtr->readFR(cPtr, 0x03) >> 2) & 0x3; 
    if (FPclkI == 3) FPclkI = 2;
    for (i = 0; i < 3; i++) {
      xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		 "Dot clock %i: %7.3f MHz",i,
		 (float)(Probed[i])/1000.);
      if (FPclkI == i) xf86ErrorF(" FPclk");
      if (CRTclkI == i) xf86ErrorF(" CRTclk");
      xf86ErrorF("\n");
    }
    cPtr->FPclock = Probed[FPclkI];
    cPtr->FPclkInx = FPclkI;
    if (CRTclkI == FPclkI) {
      if (FPclkI == 2)
	CRTclkI = 1;
      else
	CRTclkI = 2;
    }
    cPtr->CRTclkInx = CRTclkI;


    /* 
     * Some chips seem to dislike some clocks in one of the PLL's. Give
     * the user the opportunity to change it
     */
    if (xf86GetOptValInteger(cPtr->Options, OPTION_CRT_CLK_INDX, &indx)) {
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Force CRT Clock index to %d\n",
		 indx);
	cPtr->CRTclkInx = indx;

	if (xf86GetOptValInteger(cPtr->Options, OPTION_FP_CLK_INDX, &indx)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
		       "Force FP Clock index to %d\n", indx);
	    cPtr->FPclkInx = indx;
	} else {
	    if (indx == cPtr->FPclkInx) {
		if (indx == 2)
		    cPtr->FPclkInx = 1;
		else
		    cPtr->FPclkInx = indx + 1;
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
			   "FP Clock index forced to %d\n", cPtr->FPclkInx);
	    }
	}
    } else if (xf86GetOptValInteger(cPtr->Options, OPTION_FP_CLK_INDX, 
				    &indx)) {
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
		   "Force FP Clock index to %d\n", indx);
	cPtr->FPclkInx = indx;
	if (indx == cPtr->CRTclkInx) {
	    if (indx == 2)
		cPtr->CRTclkInx = 1;
	    else
		cPtr->CRTclkInx = indx + 1;
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		       "CRT Clock index forced to %d\n", cPtr->CRTclkInx);
	}
    }
    

    /* Probe the memory clock currently in use */
    MemClk->xrCC = cPtr->readXR(cPtr, 0xCC);
    MemClk->M = (MemClk->xrCC  & 0x7F) + 2;
    MemClk->xrCD = cPtr->readXR(cPtr, 0xCD);
    MemClk->N = (MemClk->xrCD & 0x7F) + 2;
    MemClk->xrCE = cPtr->readXR(cPtr, 0xCE);
    MemClk->PSN = (MemClk->xrCE & 0x1) ? 1 : 4;
    MemClk->P = ((MemClk->xrCE & 0x70) >> 4);
    /* Be careful with the calculation of ProbeClk as it can overflow */ 
    MemClk->ProbedClk = 4 * Fref / MemClk->N;
    MemClk->ProbedClk = MemClk->ProbedClk * MemClk->M / (MemClk->PSN * 
							 (1 << MemClk->P));
    MemClk->ProbedClk = MemClk->ProbedClk / 1000;
    MemClk->Clk = MemClk->ProbedClk;

    if (xf86GetOptValFreq(cPtr->Options, OPTION_SET_MCLK, OPTUNITS_MHZ, &real)) {
	int mclk = (int)(real * 1000.0);
	if (mclk <= MemClk->Max) {
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		       "Using memory clock of %7.3f MHz\n",
		       (float)(mclk/1000.));

	    /* Only alter the memory clock if the desired memory clock differs
	     * by 50kHz from the one currently being used.
	     */
	    if (abs(mclk - MemClk->ProbedClk) > 50) {
		unsigned char vclk[3];

		MemClk->Clk = mclk;
		chipsCalcClock(pScrn, MemClk->Clk, vclk);
		MemClk->M = vclk[1] + 2;
		MemClk->N = vclk[2] + 2;
		MemClk->P = (vclk[0] & 0x70) >> 4;
		MemClk->PSN = (vclk[0] & 0x1) ? 1 : 4;
		MemClk->xrCC = vclk[1];
		MemClk->xrCD = vclk[2];
		MemClk->xrCE = 0x80 || vclk[0];
	    }
	} else
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		       "Memory clock of %7.3f MHz exceeds limit of %7.3f MHz\n",
		       (float)(mclk/1000.), 
		       (float)(MemClk->Max/1000.));
    } else 
        xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		   "Probed memory clock of %7.3f MHz\n",
		   (float)(MemClk->ProbedClk/1000.));
    
    cPtr->ClockMulFactor = 1;

    /* Set the min/max pixel clock */
    switch (cPtr->Chipset) {
    case CHIPS_CT69030:
	cPtr->MinClock = 3000;
	cPtr->MaxClock = 170000;
	break;
    case CHIPS_CT69000:
	cPtr->MinClock = 3000;
	cPtr->MaxClock = 135000;
	break;
    case CHIPS_CT68554:
    case CHIPS_CT65555:
	cPtr->MinClock = 1000;
	cPtr->MaxClock = 110000;
	break;
    case CHIPS_CT65554:
	cPtr->MinClock = 1000;
	cPtr->MaxClock = 95000;
	break;
    case CHIPS_CT65550:
	cPtr->MinClock = 1000;
	if (((cPtr->readXR(cPtr, 0x04)) & 0xF) < 6) {
   if ((cPtr->readFR(cPtr, 0x0A)) & 2) {
		/*5V Vcc */
		cPtr->MaxClock = 100000;
	    } else {
		/*3.3V Vcc */
		cPtr->MaxClock = 80000;
	    }
	} else
	    cPtr->MaxClock = 95000; /* Revision B */
	break;
    }
    xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT, "Min pixel clock is %7.3f MHz\n",
	       (float)(cPtr->MinClock / 1000.));
    
    /* Check if maxClock is limited by the MemClk. Only 70% to allow for */
    /* RAS/CAS. Extra byte per memory clock needed if framebuffer used   */
    /* Extra byte if the overlay plane is activated                      */
    /* If flag Chips64BitMemory is set assume a 64bitmemory interface,   */
    /* and 32bits on the others. Thus multiply by a suitable factor      */  
    if (cPtr->Flags & Chips64BitMemory) {
	if (cPtr->FrameBufferSize && (cPtr->PanelType & ChipsLCD))
		cPtr->MaxClock = min(cPtr->MaxClock,
			     MemClk->Clk * 8 * 0.7 / (bytesPerPixel + 1));
	else
		cPtr->MaxClock = min(cPtr->MaxClock, 
			     MemClk->Clk * 8 * 0.7 / bytesPerPixel);
    } else {
	if (cPtr->FrameBufferSize && (cPtr->PanelType & ChipsLCD))
		cPtr->MaxClock = min(cPtr->MaxClock,
			     MemClk->Clk * 4 * 0.7 / (bytesPerPixel + 1));
	else
		cPtr->MaxClock = min(cPtr->MaxClock, 
			     MemClk->Clk * 4 * 0.7 / bytesPerPixel);
    }
    

    
    if (cPtr->pEnt->device->dacSpeeds[0]) {
	int speed = 0;
	switch (pScrn->bitsPerPixel) {
	case 1:
	case 4:
	case 8:
	    speed = cPtr->pEnt->device->dacSpeeds[DAC_BPP8];
	    break;
	case 16:
	    speed = cPtr->pEnt->device->dacSpeeds[DAC_BPP16];
	    break;
	case 24:
	    speed = cPtr->pEnt->device->dacSpeeds[DAC_BPP24];
	    break;
	case 32:
	    speed = cPtr->pEnt->device->dacSpeeds[DAC_BPP32];
	    break;
	}

	if (speed == 0)
	    speed = cPtr->pEnt->device->dacSpeeds[0];
	from = X_CONFIG;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		   "User max pixel clock of %7.3f MHz overrides %7.3f MHz limit\n",
		   (float)(speed / 1000.), (float)(cPtr->MaxClock / 1000.));
	cPtr->MaxClock = speed;
    } else {
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		   "Max pixel clock is %7.3f MHz\n",
		   (float)(cPtr->MaxClock / 1000.));
    }
    /* 
     * Prepare the FPclock: 
     *    if FPclock <= MaxClock : don't modify the FP clock.
     *    else set FPclock to 90% of MaxClock.
     */
    real = 0.;
    switch(bytesPerPixel) {
    case 1:
        if (xf86GetOptValFreq(cPtr->Options, OPTION_FP_CLOCK_8, OPTUNITS_MHZ, &real))
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		       "FP clock %7.3f MHz requested\n",real);
	break;
    case 2:
        if (xf86GetOptValFreq(cPtr->Options, OPTION_FP_CLOCK_16, OPTUNITS_MHZ, &real))
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		       "FP clock %7.3f MHz requested\n",real);
	break;
    case 3:
        if (xf86GetOptValFreq(cPtr->Options, OPTION_FP_CLOCK_24, OPTUNITS_MHZ, &real))
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		       "FP clock %7.3f MHz requested\n",real);
	break;
    case 4:
        if (xf86GetOptValFreq(cPtr->Options, OPTION_FP_CLOCK_32, OPTUNITS_MHZ, &real))
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		       "FP clock %7.3f MHz requested\n",real);
	break;
    }
    val = (int) (real * 1000.);
    if (val && val >= cPtr->MinClock && val <= cPtr->MaxClock)
      cPtr->FPclock = val;
    else if (cPtr->FPclock > cPtr->MaxClock)
        cPtr->FPclock = (int)((float)cPtr->MaxClock * 0.9);
    else
        cPtr->FPclock = 0; /* special value */
    cPtr->FPClkModified = FALSE;
    if (cPtr->FPclock)
        xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		   "FP clock set to %7.3f MHz\n",
		   (float)(cPtr->FPclock / 1000.));

#if defined(__arm__) && defined(__NetBSD__)
    ChipsPALMode.next = pScrn->monitor->Modes;
    pScrn->monitor->Modes = &ChipsNTSCMode;
#endif

    
    if (cPtr->Flags & ChipsDualChannelSupport) {
	if (xf86IsEntityShared(pScrn->entityList[0])) {
	    if (cPtr->SecondCrtc == TRUE) {
		cPtrEnt->slaveActive = FALSE;
	    } else {
		cPtrEnt->masterActive = FALSE;
	    }
	}
	/* Put IOSS/MSS back to normal */
	cPtr->writeIOSS(cPtr, cPtr->storeIOSS);
	cPtr->writeMSS(cPtr, hwp, cPtr->storeMSS);

	xf86SetPrimInitDone(pScrn->entityList[0]);
    }

    return TRUE;
}

static Bool
chipsPreInitWingine(ScrnInfoPtr pScrn, int flags)
{
    int i, bytesPerPixel, NoClocks = 0;
    unsigned char tmp;
    MessageType from;
    vgaHWPtr hwp;
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSClockPtr SaveClk = &(cPtr->SavedReg.Clock);
    Bool useLinear = FALSE;
    const char *s;
#ifndef XSERVER_LIBPCIACCESS
    resRange linearRes[] = { {ResExcMemBlock|ResBios|ResBus,0,0},_END };
#endif

    /* Set pScrn->monitor */
    pScrn->monitor = pScrn->confScreen->monitor;

    if (cPtr->Flags & ChipsHDepthSupport)
	i = xf86SetDepthBpp(pScrn, 0, 0, 0, Support24bppFb |
				SupportConvert32to24 | PreferConvert32to24);
    else
	i = xf86SetDepthBpp(pScrn, 8, 0, 0, NoDepth24Support);

    if (!i)
	return FALSE;
    else {
	/* Check that the returned depth is one we support */
	switch (pScrn->depth) {
	case 1:
	case 4:
	case 8:
	    /* OK */
	    break;
	case 15:
	case 16:
	case 24:
	    if (cPtr->Flags & ChipsHDepthSupport) 
		break; /* OK */
	    /* fall through */
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
     * Allocate a vgaHWRec, this must happen after xf86SetDepthBpp for 1bpp
     */
    if (!vgaHWGetHWRec(pScrn))
        return FALSE;

    hwp = VGAHWPTR(pScrn);
    vgaHWSetStdFuncs(hwp);
    vgaHWGetIOBase(hwp);

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

    if (!xf86SetDefaultVisual(pScrn, -1)) 
	return FALSE;

    /* The gamma fields must be initialised when using the new cmap code */
    if (pScrn->depth > 1) {
	Gamma zeros = {0.0, 0.0, 0.0};

	if (!xf86SetGamma(pScrn, zeros))
	    return FALSE;
    }

    /* Store register values that might be messed up by a suspend resume */
    /* Do this early as some of the other code in PreInit relies on it   */
    cPtr->SuspendHack.xr02 = (cPtr->readXR(cPtr, 0x02)) & 0x18;
    cPtr->SuspendHack.xr03 = (cPtr->readXR(cPtr, 0x03)) & 0x0A;
    cPtr->SuspendHack.xr14 = (cPtr->readXR(cPtr, 0x14)) & 0x20;
    cPtr->SuspendHack.xr15 = cPtr->readXR(cPtr, 0x15);

    cPtr->SuspendHack.vgaIOBaseFlag = ((hwp->readMiscOut(hwp)) & 0x01);
    cPtr->IOBase = (unsigned int)(cPtr->SuspendHack.vgaIOBaseFlag ?
				  0x3D0 : 0x3B0);

    bytesPerPixel = max(1, pScrn->bitsPerPixel >> 3);

    /* Collect all of the relevant option flags (fill in pScrn->options) */
    xf86CollectOptions(pScrn, NULL);

    /* Process the options */
    if (!(cPtr->Options = malloc(sizeof(ChipsWingineOptions))))
	return FALSE;
    memcpy(cPtr->Options, ChipsWingineOptions, sizeof(ChipsWingineOptions));
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, cPtr->Options);

    /* Set the bits per RGB */
    if (pScrn->depth > 1) {
	/* Default to 6, is this right?? */
	pScrn->rgbBits = 6;
#if 0
	if (xf86GetOptValInteger(cPtr->Options, OPTION_RGB_BITS,
				 &pScrn->rgbBits)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Bits per RGB set to %d\n",
		       pScrn->rgbBits);
	}
#endif
    }
    if ((cPtr->Flags & ChipsAccelSupport) &&
	    (xf86ReturnOptValBool(cPtr->Options, OPTION_NOACCEL, FALSE))) {
	cPtr->Flags &= ~ChipsAccelSupport;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Acceleration disabled\n");
    }
    
    from = X_DEFAULT;
    if (pScrn->bitsPerPixel < 8) {
	/* Default to SW cursor for 1/4 bpp */
	cPtr->Accel.UseHWCursor = FALSE;
    } else {
	cPtr->Accel.UseHWCursor = TRUE;
    }
    if (xf86GetOptValBool(cPtr->Options, OPTION_HW_CURSOR,
			  &cPtr->Accel.UseHWCursor))
	from = X_CONFIG;
    if (xf86GetOptValBool(cPtr->Options, OPTION_SW_CURSOR,
			  &cPtr->Accel.UseHWCursor)) {
	from = X_CONFIG;
	cPtr->Accel.UseHWCursor = !cPtr->Accel.UseHWCursor;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Using %s cursor\n",
	       (cPtr->Accel.UseHWCursor) ? "HW" : "SW");

    /* memory size */
    if (cPtr->pEnt->device->videoRam != 0) {
	pScrn->videoRam = cPtr->pEnt->device->videoRam;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "VideoRAM: %d kByte\n",
               pScrn->videoRam);
    } else {
	/* not given, probe it    */
	/* XR0F: Software flags 0 */
	/* bit 1-0: memory size   */
	/*          0: 256 kB     */
	/*          1: 512 kB     */
	/*          2: 1024 kB    */
	/*          3: 1024 kB    */

	switch ((cPtr->readXR(cPtr, 0x0F)) & 3) {
	case 0:
	    pScrn->videoRam = 256;
	    break;
	case 1:
	    pScrn->videoRam = 512;
	    break;
	case 2:
	    pScrn->videoRam = 1024;
	    break;
	case 3:
	    pScrn->videoRam = 2048;
	    break;
	}
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "VideoRAM: %d kByte\n",
               pScrn->videoRam);
    }
    cPtr->FbMapSize = pScrn->videoRam * 1024;

    /* Default to nonlinear for < 8bpp and linear for >= 8bpp. */
    if (cPtr->Flags & ChipsLinearSupport) useLinear = TRUE;
    if (pScrn->bitsPerPixel < 8) {
	if (!xf86ReturnOptValBool(cPtr->Options, OPTION_LINEAR, FALSE)) {
	    useLinear = FALSE;
	    from = X_CONFIG;
	}
    } else if (!xf86ReturnOptValBool(cPtr->Options, OPTION_LINEAR, TRUE)) {
	useLinear = FALSE;
	from = X_CONFIG;
    }

    if (!(cPtr->Flags & ChipsLinearSupport)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Linear framebuffer required\n");
	return FALSE;
    }

    /* linear base */
    if (useLinear) {
	unsigned char mask = 0xF8;
	if (pScrn->videoRam == 1024)
	    mask = 0xF0;
	else if (pScrn->videoRam == 2048)
	    mask = 0xE0;
	if (cPtr->pEnt->device->MemBase) {
	    cPtr->FbAddress = cPtr->pEnt->device->MemBase
		& ((0xFF << 24) | (mask << 16));
	    from = X_CONFIG;	    
	} else {
	    cPtr->FbAddress = ((0xFF & (cPtr->readXR(cPtr, 0x09))) << 24);
	    cPtr->FbAddress |= ((mask  & (cPtr->readXR(cPtr, 0x08))) << 16);
	    from = X_PROBED;
	}
#ifndef XSERVER_LIBPCIACCESS
	linearRes[0].rBegin = cPtr->FbAddress;
	linearRes[0].rEnd = cPtr->FbAddress + 0x800000;
	if (xf86RegisterResources(cPtr->pEnt->index,linearRes,ResNone)) {
	    useLinear = FALSE;
	    from = X_PROBED;
	}
#endif
    }

    if (useLinear) {
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		   "Enabling linear addressing\n");
	xf86DrvMsg(pScrn->scrnIndex, from,
		   "base address is set at 0x%lX.\n", cPtr->FbAddress);
	if (xf86ReturnOptValBool(cPtr->Options, OPTION_MMIO, FALSE) &&
	    (cPtr->Flags & ChipsMMIOSupport)) {
	    cPtr->UseMMIO = TRUE;
	    cPtr->IOAddress = cPtr->FbAddress + 0x200000L;
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Enabling MMIO\n");
	}
    } else {
	if (cPtr->Flags & ChipsLinearSupport)	
	    xf86DrvMsg(pScrn->scrnIndex, from,
		       "Disabling linear addressing\n");
	cPtr->Flags &= ~ChipsLinearSupport;
    }

    if ((s = xf86GetOptValString(cPtr->Options, OPTION_ROTATE))
	|| xf86ReturnOptValBool(cPtr->Options, OPTION_SHADOW_FB, FALSE)) {
	if (!(cPtr->Flags & ChipsLinearSupport)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "Option \"ShadowFB\" ignored. Not supported without linear addressing\n");
	} else if (pScrn->depth < 8) {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "Option \"ShadowFB\" ignored. Not supported at this depth.\n");
	} else {
	    cPtr->Rotate = 0;
	    if (s) {
		if(!xf86NameCmp(s, "CW")) {
		    /* accel is disabled below for shadowFB */
		    cPtr->Flags |= ChipsShadowFB;
		    cPtr->Rotate = 1;
		    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
			       "Rotating screen clockwise\n");
		} else if(!xf86NameCmp(s, "CCW")) {
		    cPtr->Flags |= ChipsShadowFB;
		    cPtr->Rotate = -1;
		    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,  "Rotating screen"
			       "counter clockwise\n");
		} else {
		    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "\"%s\" is not a valid"
			       "value for Option \"Rotate\"\n", s);
		    xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
			       "Valid options are \"CW\" or \"CCW\"\n");
		}
	    } else {
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
			   "Using \"Shadow Framebuffer\"\n");
		cPtr->Flags |= ChipsShadowFB;
	    }
	}
    }
    if (cPtr->Flags & ChipsShadowFB) {
	if (cPtr->Flags & ChipsAccelSupport) {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING, 
		"HW acceleration is not supported with shadow fb\n");
	    cPtr->Flags &= ~ChipsAccelSupport;
	}
	if (cPtr->Rotate && cPtr->Accel.UseHWCursor) {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING, 
		"HW cursor is not supported with rotate\n");
	    cPtr->Accel.UseHWCursor = FALSE;
	}
    }

    cPtr->PanelType |= ChipsCRT;
    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "CRT\n");

    /* monitor info */
    cPtr->Monitor = chipsSetMonitor(pScrn);

    /* bus type */
    tmp = cPtr->readXR(cPtr, 0x01) & 3;
    switch (tmp) {
    case 0:
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "ISA Bus\n");
	cPtr->Bus = ChipsISA;
	break;
    case 3:
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "VL Bus\n");
	cPtr->Bus = ChipsVLB;
	break;
    default:
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Unknown Bus\n");
	cPtr->Bus = ChipsUnknown;
	break;
    }

    /* disable acceleration for 1 and 4 bpp */
    if (pScrn->bitsPerPixel < 8) {
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		 "Disabling acceleration for %d bpp\n", pScrn->bitsPerPixel);
	cPtr->Flags &= ~ChipsAccelSupport;
    }

    /* 32bit register address offsets */
    if ((cPtr->Flags & ChipsAccelSupport) ||
	    (cPtr->Accel.UseHWCursor)) {
	cPtr->Regs32 = XNFalloc(sizeof(ChipsReg32));
	tmp = cPtr->readXR(cPtr, 0x07);
	for( i = 0; i < (sizeof(ChipsReg32) / sizeof(ChipsReg32[0])); i++) {
	    cPtr->Regs32[i] =  ((ChipsReg32[i] & 0x7E03)) | ((tmp & 0x80)
		<< 8)| ((tmp & 0x7F) << 2);
#ifdef DEBUG
	    ErrorF("DR[%X] = %X\n",i,cPtr->Regs32[i]);
#endif
	}
#ifndef XSERVER_LIBPCIACCESS
	linearRes[0].type = ResExcIoSparse | ResBios | ResBus;
	linearRes[0].rBase = cPtr->Regs32[0];
	linearRes[0].rMask = 0x83FC;
	if (xf86RegisterResources(cPtr->pEnt->index,linearRes,ResNone)) {
	    if (cPtr->Flags & ChipsAccelSupport) {
		cPtr->Flags &= ~ChipsAccelSupport;
		xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
			   "Cannot allocate IO registers: "
			   "Disabling acceleration\n");
	    }
	    if (cPtr->Accel.UseHWCursor) {
		cPtr->Accel.UseHWCursor = FALSE;
		xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
			   "Cannot allocate IO registers: "
			   "Disabling HWCursor\n");
	    }
	}
#endif
    }

    cPtr->ClockMulFactor = ((pScrn->bitsPerPixel >= 8) ? bytesPerPixel : 1);
    if (cPtr->ClockMulFactor != 1)
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
	       "Clocks scaled by %d\n", cPtr->ClockMulFactor);

    /* Clock type */
    switch (cPtr->Chipset) {
    case CHIPS_CT64200:
	NoClocks = 4;
	cPtr->ClockType = WINGINE_1_STYLE | TYPE_HW;
	break;
    default:
	if (!((cPtr->readXR(cPtr, 0x01)) & 0x10)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		       "Using external clock generator\n");
	    NoClocks = 4;
	    cPtr->ClockType = WINGINE_1_STYLE | TYPE_HW;
	} else {
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		       "Using internal clock generator\n");
	    if (xf86ReturnOptValBool(cPtr->Options, OPTION_HW_CLKS, FALSE)) {
		NoClocks = 3;
		cPtr->ClockType = WINGINE_2_STYLE | TYPE_HW;
	    } else {
		NoClocks = 26; /* some number */
		cPtr->ClockType = WINGINE_2_STYLE | TYPE_PROGRAMMABLE;
		pScrn->progClock = TRUE;
	    }
	}
    }

    if (cPtr->ClockType & TYPE_PROGRAMMABLE) {
	pScrn->numClocks = NoClocks;
	SaveClk->Clock = CRT_TEXT_CLK_FREQ;
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Using programmable clocks\n");
    } else {  /* TYPE_PROGRAMMABLE */
	SaveClk->Clock = chipsGetHWClock(pScrn);
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Using textclock clock %i.\n",
	       SaveClk->Clock);
	if (!cPtr->pEnt->device->numclocks) {
	    pScrn->numClocks = NoClocks;
	    xf86GetClocks(pScrn, NoClocks, chipsClockSelect,
			  chipsProtect, chipsBlankScreen,
			  cPtr->IOBase + 0x0A, 0x08, 1, 28322);
	    from = X_PROBED;
	} else {
	    pScrn->numClocks = cPtr->pEnt->device->numclocks;
	    if (pScrn->numClocks > NoClocks) {
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
			   "Too many Clocks specified in configuration file.\n");
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
			   "\t\tAt most %d clocks may be specified\n", NoClocks);
		pScrn->numClocks= NoClocks;
	    }
	    for (i = 0; i < pScrn->numClocks; i++)
		pScrn->clock[i] = cPtr->pEnt->device->clock[i];
	    from = X_CONFIG;
	}
	xf86ShowClocks(pScrn, from);
    }
      
    /* Set the min pixel clock */
    /* XXX Guess, need to check this */
    cPtr->MinClock = 11000 / cPtr->ClockMulFactor;
    xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT, "Min pixel clock is %7.3f MHz\n",
	       (float)(cPtr->MinClock / 1000.));
    /* maximal clock */
    switch (cPtr->Chipset) {
    case CHIPS_CT64200:
	cPtr->MaxClock = 80000 / cPtr->ClockMulFactor;
	break;
    case CHIPS_CT64300:
	cPtr->MaxClock = 85000 / cPtr->ClockMulFactor;
	break;
    }

    if (cPtr->pEnt->device->dacSpeeds[0]) {
	int speed = 0;
	switch (pScrn->bitsPerPixel) {
	case 1:
	case 4:
	case 8:
	   speed = cPtr->pEnt->device->dacSpeeds[DAC_BPP8];
	   break;
	case 16:
	   speed = cPtr->pEnt->device->dacSpeeds[DAC_BPP16];
	   break;
	case 24:
	   speed = cPtr->pEnt->device->dacSpeeds[DAC_BPP24];
	   break;
	}
	if (speed == 0)
	    cPtr->MaxClock = cPtr->pEnt->device->dacSpeeds[0];
	from = X_CONFIG;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
	    "User max pixel clock of %7.3f MHz overrides %7.3f MHz limit\n",
	    (float)(cPtr->MaxClock / 1000.), (float)(speed / 1000.));
	cPtr->MaxClock = speed;
    } else {
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		"Max pixel clock is %7.3f MHz\n",
		(float)(cPtr->MaxClock / 1000.));
    }
    
    if (xf86LoadSubModule(pScrn, "ddc")) {
	if (cPtr->pVbe)
	    xf86SetDDCproperties(pScrn,xf86PrintEDID(vbeDoEDID(cPtr->pVbe, NULL)));
    }
    return TRUE;
}

static Bool
chipsPreInit655xx(ScrnInfoPtr pScrn, int flags)
{
    int i, bytesPerPixel, NoClocks = 0;
    unsigned char tmp;
    MessageType from;
    vgaHWPtr hwp;
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSPanelSizePtr Size = &cPtr->PanelSize;
    CHIPSClockPtr SaveClk = &(cPtr->SavedReg.Clock);
    Bool useLinear = FALSE;
    const char *s;
#ifndef XSERVER_LIBPCIACCESS
    resRange linearRes[] = { {ResExcMemBlock|ResBios|ResBus,0,0},_END };
#endif
    
    /* Set pScrn->monitor */
    pScrn->monitor = pScrn->confScreen->monitor;

    if (cPtr->Flags & ChipsHDepthSupport)
	i = xf86SetDepthBpp(pScrn, 0, 0, 0, Support24bppFb |
				SupportConvert32to24 | PreferConvert32to24);
    else
	i = xf86SetDepthBpp(pScrn, 8, 0, 0, NoDepth24Support);

    if (!i)
	return FALSE;
    else {
	/* Check that the returned depth is one we support */
	switch (pScrn->depth) {
	case 1:
	case 4:
	case 8:
	    /* OK */
	    break;
	case 15:
	case 16:
	case 24:
	    if (cPtr->Flags & ChipsHDepthSupport) 
		break; /* OK */
	    /* fall through */
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
     * Allocate a vgaHWRec, this must happen after xf86SetDepthBpp for 1bpp
     */
    if (!vgaHWGetHWRec(pScrn))
        return FALSE;

    hwp = VGAHWPTR(pScrn);
    vgaHWSetStdFuncs(hwp);
    vgaHWGetIOBase(hwp);

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

    if (!xf86SetDefaultVisual(pScrn, -1))
	return FALSE;

    /* The gamma fields must be initialised when using the new cmap code */
    if (pScrn->depth > 1) {
	Gamma zeros = {0.0, 0.0, 0.0};

	if (!xf86SetGamma(pScrn, zeros))
	    return FALSE;
    }

    /* Store register values that might be messed up by a suspend resume */
    /* Do this early as some of the other code in PreInit relies on it   */
    cPtr->SuspendHack.xr02 = (cPtr->readXR(cPtr, 0x02)) & 0x18;
    cPtr->SuspendHack.xr03 = (cPtr->readXR(cPtr, 0x03)) & 0x0A;
    cPtr->SuspendHack.xr14 = (cPtr->readXR(cPtr, 0x14)) & 0x20;
    cPtr->SuspendHack.xr15 = cPtr->readXR(cPtr, 0x15);

    cPtr->SuspendHack.vgaIOBaseFlag = ((hwp->readMiscOut(hwp)) & 0x01);
    cPtr->IOBase = cPtr->SuspendHack.vgaIOBaseFlag ? 0x3D0 : 0x3B0;

    bytesPerPixel = max(1, pScrn->bitsPerPixel >> 3);

    /* Collect all of the relevant option flags (fill in pScrn->options) */
    xf86CollectOptions(pScrn, NULL);

    /* Process the options */
    if (!(cPtr->Options = malloc(sizeof(Chips655xxOptions))))
	return FALSE;
    memcpy(cPtr->Options, Chips655xxOptions, sizeof(Chips655xxOptions));
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, cPtr->Options);

    /* Set the bits per RGB */
    if (pScrn->depth > 1) {
	/* Default to 6, is this right */
	pScrn->rgbBits = 6;
#if 0
	if (xf86GetOptValInteger(cPtr->Options, OPTION_RGB_BITS,
				 &pScrn->rgbBits)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Bits per RGB set to %d\n",
		       pScrn->rgbBits);
	}
#endif
    }
    if ((cPtr->Flags & ChipsAccelSupport) &&
	    (xf86ReturnOptValBool(cPtr->Options, OPTION_NOACCEL, FALSE))) {
	cPtr->Flags &= ~ChipsAccelSupport;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Acceleration disabled\n");
    }
    
    from = X_DEFAULT;
    if (pScrn->bitsPerPixel < 8) {
	/* Default to SW cursor for 1/4 bpp */
	cPtr->Accel.UseHWCursor = FALSE;
    } else {
	cPtr->Accel.UseHWCursor = TRUE;
    }
    if (xf86GetOptValBool(cPtr->Options, OPTION_HW_CURSOR,
			  &cPtr->Accel.UseHWCursor))
	from = X_CONFIG;
    if (xf86GetOptValBool(cPtr->Options, OPTION_SW_CURSOR,
			  &cPtr->Accel.UseHWCursor)) {
	from = X_CONFIG;
	cPtr->Accel.UseHWCursor = !cPtr->Accel.UseHWCursor;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Using %s cursor\n",
	       (cPtr->Accel.UseHWCursor) ? "HW" : "SW");

    /* memory size */
    if (cPtr->pEnt->device->videoRam != 0) {
	pScrn->videoRam = cPtr->pEnt->device->videoRam;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "VideoRAM: %d kByte\n",
               pScrn->videoRam);
    } else {
        /* not given, probe it    */
	/* XR0F: Software flags 0 */
	/* bit 1-0: memory size   */
	/*          0: 256 kB     */
	/*          1: 512 kB     */
	/*          2: 1024 kB    */
	/*          3: 1024 kB    */

	switch ((cPtr->readXR(cPtr, 0x0F)) & 3) {
	case 0:
	    pScrn->videoRam = 256;
	    break;
	case 1:
	    pScrn->videoRam = 512;
	    break;
	case 2:
	case 3:
	    pScrn->videoRam = 1024;
	    break;
	}

	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "VideoRAM: %d kByte\n",
               pScrn->videoRam);
    }
    cPtr->FbMapSize = pScrn->videoRam * 1024;

    /* Default to nonlinear for < 8bpp and linear for >= 8bpp. */
    if (cPtr->Flags & ChipsLinearSupport) useLinear = TRUE;
    if (pScrn->bitsPerPixel < 8) {
	if (!xf86ReturnOptValBool(cPtr->Options, OPTION_LINEAR, FALSE)) {
	    useLinear = FALSE;
	    from = X_CONFIG;
	}
    } else if (!xf86ReturnOptValBool(cPtr->Options, OPTION_LINEAR, TRUE)) {
	useLinear = FALSE;
	from = X_CONFIG;
    }
    
    if (!(cPtr->Flags & ChipsLinearSupport)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Linear framebuffer required\n");
	return FALSE;
    }

    /* linear base */
    if (useLinear) {
	unsigned char mask;
	if (cPtr->Chipset == CHIPS_CT65535) {
	    mask = (pScrn->videoRam > 512) ? 0xF8 :0xFC;
	    if (cPtr->Bus == ChipsISA)
	    mask &= 0x7F;
	} else if (cPtr->Bus == ChipsISA) {
	    mask = 0x0F;
	} else {
	    mask = 0xFF;
	    tmp = cPtr->readXR(cPtr, 0x01);
	    if(tmp & 0x40)
		mask &= 0x3F;
	    if(!(tmp & 0x80))
		mask &= 0xCF;
	}
	if (cPtr->pEnt->location.type == BUS_PCI) {
	    cPtr->FbAddress =  PCI_REGION_BASE(cPtr->PciInfo, 0, REGION_MEM) & 0xff800000;
#ifndef XSERVER_LIBPCIACCESS
	    if (xf86RegisterResources(cPtr->pEnt->index,NULL,ResNone)) {
	        useLinear = FALSE;
		from = X_PROBED;
	    }
#endif
	} else {
	    if (cPtr->pEnt->device->MemBase) {
		cPtr->FbAddress = cPtr->pEnt->device->MemBase;
		if (cPtr->Chipset == CHIPS_CT65535)
		    cPtr->FbAddress &= (mask << 17);
		else if (cPtr->Chipset > CHIPS_CT65535)
		    cPtr->FbAddress &= (mask << 20);
		from = X_CONFIG;
	    } else { 
		if (cPtr->Chipset <= CHIPS_CT65530) {
		    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
			       "base address assumed at  0xC00000!\n");
		    cPtr->FbAddress = 0xC00000;
		    from = X_CONFIG;
		} else if (cPtr->Chipset == CHIPS_CT65535) {
		    cPtr->FbAddress =
			((mask & (cPtr->readXR(cPtr, 0x08))) << 17);
		} else {
		    cPtr->FbAddress =
			((mask & (cPtr->readXR(cPtr, 0x08))) << 20);
		}
		from = X_PROBED;
	    }
#ifndef XSERVER_LIBPCIACCESS
	    linearRes[0].rBegin = cPtr->FbAddress;
	    linearRes[0].rEnd = cPtr->FbAddress + 0x800000;
	    if (xf86RegisterResources(cPtr->pEnt->index,linearRes,ResNone)) {
		useLinear = FALSE;
		from = X_PROBED;
	    }
#endif
	}
    }
    
    if (useLinear) {
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		   "Enabling linear addressing\n");
	xf86DrvMsg(pScrn->scrnIndex, from,
		   "base address is set at 0x%lX.\n", cPtr->FbAddress);
	if (xf86ReturnOptValBool(cPtr->Options, OPTION_MMIO, FALSE) &&
	    (cPtr->Flags & ChipsMMIOSupport)) {
	    cPtr->UseMMIO = TRUE;
	    cPtr->IOAddress = cPtr->FbAddress + 0x200000L;
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Enabling MMIO\n");
	}
    } else {
	if (cPtr->Flags & ChipsLinearSupport)	
	    xf86DrvMsg(pScrn->scrnIndex, from,
		       "Disabling linear addressing\n");
	cPtr->Flags &= ~ChipsLinearSupport;
    }
    
    if ((s = xf86GetOptValString(cPtr->Options, OPTION_ROTATE))
	|| xf86ReturnOptValBool(cPtr->Options, OPTION_SHADOW_FB, FALSE)) {
	if (!(cPtr->Flags & ChipsLinearSupport)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "Option \"ShadowFB\" ignored. Not supported without linear addressing\n");
	} else if (pScrn->depth < 8) {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "Option \"ShadowFB\" ignored. Not supported at this depth.\n");
	} else {
	    cPtr->Rotate = 0;
	    if (s) {
		if(!xf86NameCmp(s, "CW")) {
		    /* accel is disabled below for shadowFB */
		    cPtr->Flags |= ChipsShadowFB;
		    cPtr->Rotate = 1;
		    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
			       "Rotating screen clockwise\n");
		} else if(!xf86NameCmp(s, "CCW")) {
		    cPtr->Flags |= ChipsShadowFB;
		    cPtr->Rotate = -1;
		    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,  "Rotating screen"
			       "counter clockwise\n");
		} else {
		    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "\"%s\" is not a valid"
			       "value for Option \"Rotate\"\n", s);
		    xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
			       "Valid options are \"CW\" or \"CCW\"\n");
		}
	    } else {
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
			   "Using \"Shadow Framebuffer\"\n");
		cPtr->Flags |= ChipsShadowFB;
	    }
	}
    }
    if (cPtr->Flags & ChipsShadowFB) {
	if (cPtr->Flags & ChipsAccelSupport) {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING, 
		"HW acceleration is not supported with shadow fb\n");
	    cPtr->Flags &= ~ChipsAccelSupport;
	}
	if (cPtr->Rotate && cPtr->Accel.UseHWCursor) {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING, 
		"HW cursor is not supported with rotate\n");
	    cPtr->Accel.UseHWCursor = FALSE;
	}
    }

    /*test STN / TFT */
    tmp = cPtr->readXR(cPtr, 0x51);

    /* XR51 or FR10: DISPLAY TYPE REGISTER                      */
    /* XR51[1-0] or FR10[1:0] for ct65550 : PanelType,          */
    /* 0 = Single Panel Single Drive, 3 = Dual Panel Dual Drive */
    switch (tmp & 0x3) {
    case 0:
	if (xf86ReturnOptValBool(cPtr->Options, OPTION_STN, FALSE)) {
	    cPtr->PanelType |= ChipsSS;
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "SS-STN probed\n");
	} else {
	    cPtr->PanelType |= ChipsTFT;
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "TFT probed\n");
	}
	break;
    case 2:
	cPtr->PanelType |= ChipsDS;
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "DS-STN probed\n");
    case 3:
	cPtr->PanelType |= ChipsDD;
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "DD-STN probed\n");
	break;
    default:
	break;
    }

    chipsSetPanelType(cPtr);
    from = X_PROBED;
    {
        Bool fp_mode;
	if (xf86GetOptValBool(cPtr->Options, OPTION_FP_MODE, &fp_mode)) {
	    if (fp_mode) {
	        xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Forcing FP Mode on\n");
		cPtr->PanelType |= ChipsLCD;
	    } else {
	       xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Forcing FP Mode off\n");
	       cPtr->PanelType = ~ChipsLCD;
	    } 
	    from = X_CONFIG;
	}
    }
    if ((cPtr->PanelType & ChipsLCD) && (cPtr->PanelType & ChipsCRT))
	xf86DrvMsg(pScrn->scrnIndex, from, "LCD/CRT\n");
    else if (cPtr->PanelType & ChipsLCD)
        xf86DrvMsg(pScrn->scrnIndex, from, "LCD\n");
    else if (cPtr->PanelType & ChipsCRT) {
        xf86DrvMsg(pScrn->scrnIndex, from, "CRT\n");
	/* monitor info */
	cPtr->Monitor = chipsSetMonitor(pScrn);
    }
      
    /* screen size */
    /* 
     * In LCD mode / dual mode we want to derive the timing values from
     * the ones preset by bios
     */
    if (cPtr->PanelType & ChipsLCD) {
	unsigned char xr17, tmp1;
	char tmp2;

	xr17 = cPtr->readXR(cPtr, 0x17);
	tmp = cPtr->readXR(cPtr, 0x1B);
	Size->HTotal =((tmp + ((xr17 & 0x01) << 8)) + 5) << 3;
	tmp = cPtr->readXR(cPtr, 0x1C);
	Size->HDisplay = ((tmp + ((xr17 & 0x02) << 7)) + 1) << 3;
	tmp = cPtr->readXR(cPtr, 0x19);
	Size->HRetraceStart = ((tmp + ((xr17 & 0x04) << 9)) + 1) << 3;
	tmp1 = cPtr->readXR(cPtr, 0x1A);
	tmp2 = (tmp1 & 0x1F) + ((xr17 & 0x08) << 2) - (tmp & 0x3F);
	Size->HRetraceEnd = ((((tmp2 & 0x080u) ? (tmp2 + 0x40) : tmp2) << 3)
		+ Size->HRetraceStart);
	tmp1 = cPtr->readXR(cPtr, 0x65);
	tmp = cPtr->readXR(cPtr, 0x68);
	Size->VDisplay = ((tmp1 & 0x02) << 7) 
	      + ((tmp1 & 0x40) << 3) + tmp + 1;
	tmp = cPtr->readXR(cPtr, 0x66);
	Size->VRetraceStart = ((tmp1 & 0x04) << 6) 
	      + ((tmp1 & 0x80) << 2) + tmp + 1;
	tmp = cPtr->readXR(cPtr, 0x64);
	Size->VTotal = ((tmp1 & 0x01) << 8)
	      + ((tmp1 & 0x20) << 4) + tmp + 2;
#ifdef DEBUG
	ErrorF("x=%i, y=%i; xSync=%i, xSyncEnd=%i, xTotal=%i\n",
	       Size->HDisplay, Size->VDisplay,
	       Size->HRetraceStart, Size->HRetraceEnd,
	       Size->HTotal);
#endif
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Display Size: x=%i; y=%i\n",
		Size->HDisplay, Size->VDisplay);
	/* Warn the user if the panel size has been overridden by
	 * the modeline values
	 */
	if (xf86ReturnOptValBool(cPtr->Options, OPTION_PANEL_SIZE, FALSE)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		       "Display size overridden by modelines.\n");
	}
    }

    /* Frame Buffer */                 /* for LCDs          */ 
    if (IS_STN(cPtr->PanelType)) {
	tmp = cPtr->readXR(cPtr, 0x6F); /*Frame Buffer Ctrl. */
	if (tmp & 1) {
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Frame Buffer used\n");
	    if ((cPtr->Chipset > CHIPS_CT65530) && !(tmp & 0x80)) {
		/* Formula for calculating the size of the framebuffer. 3
		 * bits per pixel 10 pixels per 32 bit dword. If frame
		 * acceleration is enabled the size can be halved.
		 */
		cPtr->FrameBufferSize = ( Size->HDisplay *
			Size->VDisplay / 5 ) * ((tmp & 2) ? 1 : 2);
		xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
			   "Using embedded Frame Buffer, size %d bytes\n",
			   cPtr->FrameBufferSize);
	    } else
		xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
			   "Using external Frame Buffer used\n");
	}
	if (tmp & 2)
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		       "Frame accelerator enabled\n");
    }

    /* bus type */
    if (cPtr->Chipset > CHIPS_CT65535) {
	tmp = (cPtr->readXR(cPtr, 0x01)) & 7;
	if (tmp == 6) {	       /*PCI */
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "PCI Bus\n");
	    cPtr->Bus = ChipsPCI;
	    if ((cPtr->Chipset == CHIPS_CT65545) ||
		    (cPtr->Chipset == CHIPS_CT65546)) {
		xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
			   "32Bit IO not supported on 65545 PCI\n");
		xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "\tenabling MMIO\n");
		cPtr->UseMMIO = TRUE;
		cPtr->IOAddress = cPtr->FbAddress + 0x200000L;
	    }
	    
	} else {   /* XR08: Linear addressing base, not for PCI */
	    switch (tmp) {
	    case 3:
		xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "CPU Direct\n");
		cPtr->Bus = ChipsCPUDirect;
		break;
	    case 5:
		xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "ISA Bus\n");
		cPtr->Bus = ChipsISA;
		break;
	    case 7:
		xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "VL Bus\n");
		cPtr->Bus = ChipsVLB;
		break;
	    default:
		xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Unknown Bus\n");
	    }
	}
    } else {
	tmp = (cPtr->readXR(cPtr, 0x01)) & 3;
	switch (tmp) {
	case 0:
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "PI Bus\n");
	    cPtr->Bus = ChipsPIB;
	    break;
	case 1:
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "MC Bus\n");
	    cPtr->Bus = ChipsMCB;
	    break;
	case 2:
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "VL Bus\n");
	    cPtr->Bus = ChipsVLB;
	    break;
	case 3:
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "ISA Bus\n");
	    cPtr->Bus = ChipsISA;
	    break;
	}
    }

    if (!(cPtr->Bus == ChipsPCI) && (cPtr->UseMMIO)) {
	cPtr->UseMMIO = FALSE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		   "MMIO only supported on PCI Bus. Disabling MMIO\n");
    }

    /* disable acceleration for 1 and 4 bpp */
    if (pScrn->bitsPerPixel < 8) {
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		 "Disabling acceleration for %d bpp\n", pScrn->bitsPerPixel);
	cPtr->Flags &= ~ChipsAccelSupport;
    }

    if ((cPtr->Chipset == CHIPS_CT65530) &&
	    (cPtr->Flags & ChipsLinearSupport)) {
	/* linear mode is no longer default on ct65530 since it */
	/* requires additional hardware which some manufacturers*/
	/* might not provide.                                   */
	if (!xf86ReturnOptValBool(cPtr->Options, OPTION_LINEAR, FALSE))
	    cPtr->Flags &= ~ChipsLinearSupport;
	
	/* Test wether linear addressing is possible on 65530 */
	/* on the 65530 only the A19 select scheme can be used*/
	/* for linear addressing since MEMW is used on ISA bus*/
	/* systems.                                           */
	/* A19 however is used if video memory is > 512 Mb    */   
	if ((cPtr->Bus == ChipsISA) && (pScrn->videoRam > 512)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		       "User selected linear fb not supported by HW!\n");
	    cPtr->Flags &= ~ChipsLinearSupport;
	}
    }

    /* DAC info */
    if ((cPtr->readXR(cPtr, 0x06)) & 0x02)
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Internal DAC disabled\n");

    /* MMIO address offset */
    if (cPtr->UseMMIO)
	cPtr->Regs32 = ChipsReg32;
    else if ((cPtr->Flags & ChipsAccelSupport) ||
	     (cPtr->Accel.UseHWCursor)) {
	cPtr->Regs32 = XNFalloc(sizeof(ChipsReg32));
	tmp =  cPtr->readXR(cPtr, 0x07);
	for (i = 0; i < (sizeof(ChipsReg32)/sizeof(ChipsReg32[0])); i++) {
	    cPtr->Regs32[i] =
		((ChipsReg32[i] & 0x7E03)) | ((tmp & 0x80)<< 8)
		| ((tmp & 0x7F) << 2);
#ifdef DEBUG
	    ErrorF("DR[%X] = %X\n",i,cPtr->Regs32[i]);
#endif
	}
#ifndef XSERVER_LIBPCIACCESS
	linearRes[0].type = ResExcIoSparse | ResBios | ResBus;
	linearRes[0].rBase = cPtr->Regs32[0];
	linearRes[0].rMask = 0x83FC;
	if (xf86RegisterResources(cPtr->pEnt->index,linearRes,ResNone)) {
	    if (cPtr->Flags & ChipsAccelSupport) {
		cPtr->Flags &= ~ChipsAccelSupport;
		xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
			   "Cannot allocate IO registers: "
			   "Disabling acceleration\n");
	    }
	    if (cPtr->Accel.UseHWCursor) {
		cPtr->Accel.UseHWCursor = FALSE;
		xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
			   "Cannot allocate IO registers: "
			   "Disabling HWCursor\n");
	    }
	}
#endif
    }

    /* sync reset ignored on this chipset */
    if (cPtr->Chipset > CHIPS_CT65530) {
	tmp = cPtr->readXR(cPtr, 0x0E);
	if (tmp & 0x80)
	    cPtr->SyncResetIgn = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		   "Synchronous reset %signored.\n",
		   (cPtr->SyncResetIgn ? "" : "not ")); 
    }

    cPtr->ClockMulFactor = ((pScrn->bitsPerPixel >= 8) ? bytesPerPixel : 1);
    if (cPtr->ClockMulFactor != 1)
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
	       "Clocks scaled by %d\n", cPtr->ClockMulFactor);
    /* We use a programmable clock */
    switch (cPtr->Chipset) {
    case CHIPS_CT65520:
    case CHIPS_CT65525:
    case CHIPS_CT65530:
	NoClocks = 4;		/* Some number */
	cPtr->ClockType = OLD_STYLE | TYPE_HW;
	break;
    default:
	if (xf86ReturnOptValBool(cPtr->Options, OPTION_HW_CLKS, FALSE)) {
	    NoClocks = 5;		/* Some number */
	    cPtr->ClockType = NEW_STYLE | TYPE_HW;
	} else {
	    NoClocks = 26;		/* Some number */
	    cPtr->ClockType = NEW_STYLE | TYPE_PROGRAMMABLE;
	    pScrn->progClock = TRUE;
	}
    }

    if (cPtr->ClockType & TYPE_PROGRAMMABLE) {
	pScrn->numClocks = NoClocks;
        SaveClk->Clock = ((cPtr->PanelType & ChipsLCDProbed) ? 
			  LCD_TEXT_CLK_FREQ : CRT_TEXT_CLK_FREQ);
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Using programmable clocks\n");
    } else {  /* TYPE_PROGRAMMABLE */
	SaveClk->Clock = chipsGetHWClock(pScrn);
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Using textclock clock %i.\n",
	       SaveClk->Clock);
	if (!cPtr->pEnt->device->numclocks) {
	    pScrn->numClocks = NoClocks;
	    xf86GetClocks(pScrn, NoClocks, chipsClockSelect,
			  chipsProtect, chipsBlankScreen,
			  cPtr->IOBase + 0x0A, 0x08, 1, 28322);
	    from = X_PROBED;
	} else { 
	    pScrn->numClocks = cPtr->pEnt->device->numclocks;
	    if (pScrn->numClocks > NoClocks) {
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
			   "Too many Clocks specified in configuration file.\n");
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
			   "\t\tAt most %d clocks may be specified\n", NoClocks);
		pScrn->numClocks = NoClocks;
	    }
	    for (i = 0; i < pScrn->numClocks; i++)
		pScrn->clock[i] = cPtr->pEnt->device->clock[i];
	    from = X_CONFIG;
	}
	xf86ShowClocks(pScrn, from);
    }
    /* Set the min pixel clock */
    /* XXX Guess, need to check this */
    cPtr->MinClock = 11000 / cPtr->ClockMulFactor;
    xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT, "Min pixel clock is %7.3f MHz\n",
	       (float)(cPtr->MinClock / 1000.));
    /* Set the max pixel clock */
    switch (cPtr->Chipset) {
    case CHIPS_CT65546:
    case CHIPS_CT65548:
	/* max VCLK is 80 MHz, max MCLK is 75 MHz for CT65548 */
	/* It is not sure for CT65546, but it works with 60 nsec EDODRAM */
	cPtr->MaxClock = 80000 / cPtr->ClockMulFactor;
	break;
    default:
	if ((cPtr->readXR(cPtr, 0x6C)) & 2) {
	    /*5V Vcc */
	    cPtr->MaxClock = 68000 / cPtr->ClockMulFactor;
	} else {
	    /*3.3V Vcc */
	    cPtr->MaxClock = 56000 / cPtr->ClockMulFactor;
	}
    }
    
    if (cPtr->pEnt->device->dacSpeeds[0]) {
	int speed = 0;
	switch (pScrn->bitsPerPixel) {
	case 1:
	case 4:
	case 8:
	    speed = cPtr->pEnt->device->dacSpeeds[DAC_BPP8];
	    break;
	case 16:
	    speed = cPtr->pEnt->device->dacSpeeds[DAC_BPP16];
	    break;
	case 24:
	    speed = cPtr->pEnt->device->dacSpeeds[DAC_BPP24];
	    break;
	}
	if (speed == 0)
	    cPtr->MaxClock = cPtr->pEnt->device->dacSpeeds[0];
	from = X_CONFIG;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
	    "User max pixel clock of %7.3f MHz overrides %7.3f MHz limit\n",
	    (float)(cPtr->MaxClock / 1000.), (float)(speed / 1000.));
	cPtr->MaxClock = speed;
    } else {
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		   "Max pixel clock is %7.3f MHz\n",
		   (float)(cPtr->MaxClock / 1000.));
    }

    /* FP clock */
    if (cPtr->ClockType & TYPE_PROGRAMMABLE) {
	double real = 0;
	    
	switch(bytesPerPixel) {
	case 1:
	    xf86GetOptValFreq(cPtr->Options, OPTION_FP_CLOCK_8,
			      OPTUNITS_MHZ, &real);
	    break;
	case 2:
	    xf86GetOptValFreq(cPtr->Options, OPTION_FP_CLOCK_16,
			      OPTUNITS_MHZ, &real);
	    break;
	case 3:
	    xf86GetOptValFreq(cPtr->Options, OPTION_FP_CLOCK_24,
			      OPTUNITS_MHZ, &real);
	    break;
	}
	
	if (real > 0) {
	    int val;
	    xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		       "FP clock %7.3f MHz requested\n",real);
	    val = (int) (real * 1000.);
	    if (val && (val >= cPtr->MinClock)
		&& (val <= cPtr->MaxClock))
		cPtr->FPclock = val * cPtr->ClockMulFactor;
	    else if (val > cPtr->MaxClock)
		cPtr->FPclock = (int)((float)cPtr->MaxClock
				      * cPtr->ClockMulFactor * 0.9);
	    else
		cPtr->FPclock = 0; /* special value */
	}  else 
	    cPtr->FPclock = 0; /* special value */
	
	if (cPtr->FPclock)
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		       "FP clock set to %7.3f MHz\n",
		       (float)(cPtr->FPclock / 1000.));
    } else {
	if (xf86IsOptionSet(cPtr->Options, OPTION_SET_MCLK))
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		       "FP clock option not supported for this chipset\n");
    }
    
    /* Memory Clock */
    if (cPtr->ClockType & TYPE_PROGRAMMABLE) {
	double real;
	
	switch (cPtr->Chipset) {
	case CHIPS_CT65546:
	case CHIPS_CT65548:
	    /* max MCLK is 75 MHz for CT65548 */
	    cPtr->MemClock.Max = 75000;
	    break;
	default:
	    if ((cPtr->readXR(cPtr, 0x6C)) & 2) {
		/*5V Vcc */
		cPtr->MemClock.Max = 68000;
	    } else {
		/*3.3V Vcc */
		cPtr->MemClock.Max = 56000;
	    }
	}
	
	if (xf86GetOptValFreq(cPtr->Options, OPTION_SET_MCLK,
			      OPTUNITS_MHZ, &real)) {
	    int mclk = (int)(real * 1000.0);
	    if (mclk <= cPtr->MemClock.Max) {
		xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
			   "Using memory clock of %7.3f MHz\n",
			   (float)(mclk/1000.));
		cPtr->MemClock.Clk = mclk;
	    } else {
		xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
			   "Memory clock of %7.3f MHz exceeds limit of "
			   "%7.3f MHz\n",(float)(mclk/1000.),
			   (float)(cPtr->MemClock.Max/1000.));
		cPtr->MemClock.Clk = cPtr->MemClock.Max * 0.9;
	    }
	} else
	    cPtr->MemClock.Clk = 0;
    } else
	if (xf86IsOptionSet(cPtr->Options, OPTION_SET_MCLK))
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		       "Memory clock option not supported for this chipset\n");
    
    if (xf86LoadSubModule(pScrn, "ddc")) {
	if (cPtr->pVbe)
	    xf86SetDDCproperties(pScrn,xf86PrintEDID(vbeDoEDID(cPtr->pVbe, NULL)));
    }
    return TRUE;
}
    

/* Mandatory */
static Bool
CHIPSEnterVT(VT_FUNC_ARGS_DECL)
{
    SCRN_INFO_PTR(arg);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSEntPtr cPtrEnt;

    if (cPtr->Flags & ChipsDualChannelSupport) {
        cPtrEnt = xf86GetEntityPrivate(pScrn->entityList[0],
					       CHIPSEntityIndex)->ptr;
	DUALOPEN;
    }
    /* Should we re-save the text mode on each VT enter? */
    if(!chipsModeInit(pScrn, pScrn->currentMode))
      return FALSE;
    if ((cPtr->Flags & ChipsVideoSupport)
	&& (cPtr->Flags & ChipsLinearSupport)) 
        CHIPSResetVideo(pScrn); 

    /*usleep(50000);*/
    chipsHWCursorOn(cPtr, pScrn);
    /* cursor settle delay */
    usleep(50000);
    CHIPSAdjustFrame(ADJUST_FRAME_ARGS(pScrn, pScrn->frameX0, pScrn->frameY0));
    usleep(50000);
    return TRUE;
}

/* Mandatory */
static void
CHIPSLeaveVT(VT_FUNC_ARGS_DECL)
{
    SCRN_INFO_PTR(arg);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);
    CHIPSEntPtr cPtrEnt;

    /* Invalidate the cached acceleration registers */
    cAcl->planemask = -1;
    cAcl->fgColor = -1;
    cAcl->bgColor = -1;

    if (cPtr->Flags & ChipsDualChannelSupport) {
        cPtrEnt = xf86GetEntityPrivate(pScrn->entityList[0],
				       CHIPSEntityIndex)->ptr;
	if (cPtr->UseDualChannel)
	    DUALREOPEN;
       	DUALCLOSE;
    } else {
	chipsHWCursorOff(cPtr, pScrn);
	chipsRestore(pScrn, &(VGAHWPTR(pScrn))->SavedReg, &cPtr->SavedReg,
#ifdef AVOID_VGAHW
					FALSE);
#else
					TRUE);
#endif
	chipsLock(pScrn);
    }
}


static void
chipsLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices, LOCO *colors,
		 VisualPtr pVisual)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    int i, index, shift ;
    CHIPSEntPtr cPtrEnt;    

    shift = (pScrn->depth == 15) ? 3 : 0;

    if (cPtr->UseDualChannel) {
        cPtrEnt = xf86GetEntityPrivate(pScrn->entityList[0],
					       CHIPSEntityIndex)->ptr;
	DUALREOPEN;
    }

    for (i = 0; i < numColors; i++) {
	index = indices[i];
	hwp->writeDacWriteAddr(hwp,index << shift);
	DACDelay(hwp);
	hwp->writeDacData(hwp, colors[index].red);
	DACDelay(hwp);
	hwp->writeDacData(hwp, colors[index].green);
	DACDelay(hwp);
	hwp->writeDacData(hwp, colors[index].blue);
	DACDelay(hwp);
    }

    if (cPtr->UseDualChannel && 
		(! xf86IsEntityShared(pScrn->entityList[0]))) {
	unsigned int IOSS, MSS;
	IOSS = cPtr->readIOSS(cPtr);
	MSS = cPtr->readMSS(cPtr);
	cPtr->writeIOSS(cPtr, ((cPtr->storeIOSS & IOSS_MASK) |
			       IOSS_PIPE_B));
	cPtr->writeMSS(cPtr, hwp, ((cPtr->storeMSS & MSS_MASK) | MSS_PIPE_B));

	for (i = 0; i < numColors; i++) {
	    index = indices[i];
	    hwp->writeDacWriteAddr(hwp,index << shift);
	    DACDelay(hwp);
	    hwp->writeDacData(hwp, colors[index].red);
	    DACDelay(hwp);
	    hwp->writeDacData(hwp, colors[index].green);
	    DACDelay(hwp);
	    hwp->writeDacData(hwp, colors[index].blue);
	    DACDelay(hwp);
	}
	cPtr->writeIOSS(cPtr, IOSS);
	cPtr->writeMSS(cPtr, hwp, MSS);
    }
    
    /* This shouldn't be necessary, but we'll play safe. */
    hwp->disablePalette(hwp);
}

static void
chipsLoadPalette16(ScrnInfoPtr pScrn, int numColors, int *indices,
		 LOCO *colors, VisualPtr pVisual)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSEntPtr cPtrEnt;

    int i, index;

    if (cPtr->UseDualChannel) {
        cPtrEnt = xf86GetEntityPrivate(pScrn->entityList[0],
					       CHIPSEntityIndex)->ptr;
	DUALREOPEN;
    }
    
    for (i = 0; i < numColors; i++) {
	index = indices[i];
	hwp->writeDacWriteAddr(hwp, index << 2);
	DACDelay(hwp);
	hwp->writeDacData(hwp, colors[index >> 1].red);
	DACDelay(hwp);
	hwp->writeDacData(hwp, colors[index].green);
	DACDelay(hwp);
	hwp->writeDacData(hwp, colors[index >> 1].blue);
	DACDelay(hwp);
    }


    if (cPtr->UseDualChannel &&
		(! xf86IsEntityShared(pScrn->entityList[0]))) {
	unsigned int IOSS, MSS;
	IOSS = cPtr->readIOSS(cPtr);
	MSS = cPtr->readMSS(cPtr);
	cPtr->writeIOSS(cPtr, ((cPtr->storeIOSS & IOSS_MASK) |
			       IOSS_PIPE_B));
	cPtr->writeMSS(cPtr, hwp, ((cPtr->storeMSS & MSS_MASK) | MSS_PIPE_B));

	for (i = 0; i < numColors; i++) {
	    index = indices[i];
	    hwp->writeDacWriteAddr(hwp, index << 2);
	    DACDelay(hwp);
	    hwp->writeDacData(hwp, colors[index >> 1].red);
	    DACDelay(hwp);
	    hwp->writeDacData(hwp, colors[index].green);
	    DACDelay(hwp);
	    hwp->writeDacData(hwp, colors[index >> 1].blue);
	    DACDelay(hwp);
	}

	cPtr->writeIOSS(cPtr, IOSS);
	cPtr->writeMSS(cPtr, hwp, MSS);
    }

    /* This shouldn't be necessary, but we'll play safe. */
    hwp->disablePalette(hwp);
}

/* Mandatory */
static Bool
CHIPSScreenInit(SCREEN_INIT_ARGS_DECL)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    vgaHWPtr hwp;
    CHIPSPtr cPtr;
    CHIPSACLPtr cAcl;
    int ret;
    int init_picture = 0;
    VisualPtr visual;
    int allocatebase, freespace, currentaddr;
#ifndef XSERVER_LIBPCIACCESS
    unsigned int racflag = 0;
#endif
    unsigned char *FBStart;
    int height, width, displayWidth;
    CHIPSEntPtr cPtrEnt = NULL;
#ifdef DEBUG
    ErrorF("CHIPSScreenInit\n");
#endif    

    /*
     * we need to get the ScrnInfoRec for this screen, so let's allocate
     * one first thing
     */
    cPtr = CHIPSPTR(pScrn);
    cAcl = CHIPSACLPTR(pScrn);

    hwp = VGAHWPTR(pScrn);
    hwp->MapSize = 0x10000;		/* Standard 64k VGA window */

#ifndef AVOID_VGAHW
    /* Map the VGA memory */
    if (!vgaHWMapMem(pScrn))
	return FALSE;
#endif

    /* Map the Chips memory and possible MMIO areas */
    if (!chipsMapMem(pScrn))
	return FALSE;

    /* Setup the MMIO register access functions if need */
    if (cPtr->UseFullMMIO && cPtr->MMIOBaseVGA) {
	CHIPSSetMmioExtFuncs(cPtr);
	CHIPSHWSetMmioFuncs(pScrn, cPtr->MMIOBaseVGA, 0x0);
    }

    if (cPtr->Flags & ChipsDualChannelSupport) {
        cPtrEnt = xf86GetEntityPrivate(pScrn->entityList[0],
					       CHIPSEntityIndex)->ptr;
	DUALOPEN;
    }

#if defined(__arm__) && defined(__NetBSD__)
    if (strcmp(pScrn->currentMode->name,"PAL") == 0) {
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Using built-in PAL TV mode\n");
	cPtr->TVMode = XMODE_PAL;
    } else if (strcmp(pScrn->currentMode->name,"SECAM") == 0){
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		   "Using built-in SECAM TV mode\n");
	cPtr->TVMode = XMODE_SECAM;
    } else if (strcmp(pScrn->currentMode->name,"NTSC") == 0) {
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		   "Using built-in NTSC TV mode\n");
	cPtr->TVMode = XMODE_NTSC;
    } else 
	cPtr->TVMode = XMODE_RGB;
#endif

    /*
     * next we save the current state and setup the first mode
     */
    if ((cPtr->Flags & ChipsDualChannelSupport) && 
		(! xf86IsEntityShared(pScrn->entityList[0]))) {
	unsigned int IOSS, MSS;
	IOSS = cPtr->readIOSS(cPtr);
	MSS = cPtr->readMSS(cPtr);
	cPtr->writeIOSS(cPtr, ((cPtr->storeIOSS & IOSS_MASK) |
					IOSS_PIPE_A));
	cPtr->writeMSS(cPtr, hwp, ((cPtr->storeMSS & MSS_MASK) | MSS_PIPE_A));
	chipsSave(pScrn, &hwp->SavedReg, &cPtr->SavedReg);
	cPtr->writeIOSS(cPtr, ((cPtr->storeIOSS & IOSS_MASK) |
			       IOSS_PIPE_B));
	cPtr->writeMSS(cPtr, hwp, ((cPtr->storeMSS & MSS_MASK) | MSS_PIPE_B));
	chipsSave(pScrn, &cPtr->VgaSavedReg2, &cPtr->SavedReg2);
	cPtr->writeIOSS(cPtr, IOSS);
	cPtr->writeMSS(cPtr, hwp, MSS);
    } else
	chipsSave(pScrn, &hwp->SavedReg, &cPtr->SavedReg);

    if (!chipsModeInit(pScrn,pScrn->currentMode))
	return FALSE;
    CHIPSSaveScreen(pScreen,SCREEN_SAVER_ON);
    CHIPSAdjustFrame(ADJUST_FRAME_ARGS(pScrn, pScrn->frameX0, pScrn->frameY0));
    
    /*
     * The next step is to setup the screen's visuals, and initialise the
     * framebuffer code.  In cases where the framebuffer's default
     * choices for things like visual layouts and bits per RGB are OK,
     * this may be as simple as calling the framebuffer's ScreenInit()
     * function.  If not, the visuals will need to be setup before calling
     * a fb ScreenInit() function and fixed up after.
     *
     * For most PC hardware at depths >= 8, the defaults that cfb uses
     * are not appropriate.  In this driver, we fixup the visuals after.
     */

    /*
     * Reset visual list.
     */
    miClearVisualTypes();

    /* Setup the visuals we support. */
    if (!miSetVisualTypes(pScrn->depth,
			    miGetDefaultVisualMask(pScrn->depth),
			    pScrn->rgbBits, pScrn->defaultVisual))
        return FALSE;
    miSetPixmapDepths ();

    /*
     * Call the framebuffer layer's ScreenInit function, and fill in other
     * pScreen fields.
     */
    if ((cPtr->Flags & ChipsShadowFB) && cPtr->Rotate) {
	height = pScrn->virtualX;
	width = pScrn->virtualY;
    } else {
	width = pScrn->virtualX;
	height = pScrn->virtualY;
    }

    if(cPtr->Flags & ChipsShadowFB) {
	cPtr->ShadowPitch = BitmapBytePad(pScrn->bitsPerPixel * width);
	cPtr->ShadowPtr = malloc(cPtr->ShadowPitch * height);
	displayWidth = cPtr->ShadowPitch / (pScrn->bitsPerPixel >> 3);
	FBStart = cPtr->ShadowPtr;
    } else {
	cPtr->ShadowPtr = NULL;
	displayWidth = pScrn->displayWidth;
	FBStart = cPtr->FbBase;
    }

    switch (pScrn->bitsPerPixel) {
    case 16:
    default:
	ret = fbScreenInit(pScreen, FBStart,
 		        width,height,
			pScrn->xDpi, pScrn->yDpi,
			displayWidth,pScrn->bitsPerPixel);
	init_picture = 1;
	break;
    }

    if (!ret)
	return FALSE;

#if X_BYTE_ORDER == X_BIG_ENDIAN
 	/* TODO : find a better way to do this */
 	if (pScrn->depth == 24) {
 		int dummy ;
 		  /* Fixup RGB ordering in 24 BPP */
 			dummy = pScrn->offset.red ;
 			pScrn->offset.red = pScrn->offset.blue;
 			pScrn->offset.blue = dummy ;
 
 			dummy = pScrn->mask.red ;
 			pScrn->mask.red = pScrn->mask.blue;
 			pScrn->mask.blue = dummy ;
 	}
#endif

    if (pScrn->depth > 8) {
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

    /* must be after RGB ordering fixed */
    if (init_picture)
	fbPictureInit (pScreen, 0, 0);
    
    xf86SetBlackWhitePixels(pScreen);

    cPtr->BlockHandler = pScreen->BlockHandler;
    pScreen->BlockHandler = chipsBlockHandler;

    if ( (pScrn->depth >= 8))
	CHIPSDGAInit(pScreen);

    cPtr->HWCursorShown = FALSE;

#ifdef USE_MIBANK
    if (!(cPtr->Flags & ChipsLinearSupport)) {
	miBankInfoPtr pBankInfo;

	/* Setup the vga banking variables */
	pBankInfo = (miBankInfoPtr)XNFcallocarray(sizeof(miBankInfoRec),1);
	if (pBankInfo == NULL)
	    return FALSE;
	
#if defined(__arm__)
	cPtr->Bank = -1;
#endif
	pBankInfo->pBankA = hwp->Base;
	pBankInfo->pBankB = (unsigned char *)hwp->Base + 0x08000;
	pBankInfo->BankSize = 0x08000;
	pBankInfo->nBankDepth = (pScrn->depth == 4) ? 1 : pScrn->depth;

	if (IS_HiQV(cPtr)) {
	    pBankInfo->pBankB = hwp->Base;
	    pBankInfo->BankSize = 0x10000;
	    if (pScrn->bitsPerPixel < 8) {
		pBankInfo->SetSourceBank =
			(miBankProcPtr)CHIPSHiQVSetReadWritePlanar;
		pBankInfo->SetDestinationBank =
			(miBankProcPtr)CHIPSHiQVSetReadWritePlanar;
		pBankInfo->SetSourceAndDestinationBanks =
			(miBankProcPtr)CHIPSHiQVSetReadWritePlanar;
	    } else {
		pBankInfo->SetSourceBank =
			(miBankProcPtr)CHIPSHiQVSetReadWrite;
		pBankInfo->SetDestinationBank =
			(miBankProcPtr)CHIPSHiQVSetReadWrite;
		pBankInfo->SetSourceAndDestinationBanks =
			(miBankProcPtr)CHIPSHiQVSetReadWrite;
	    }
	} else {
	    if (IS_Wingine(cPtr)) {
		if (pScrn->bitsPerPixel < 8) {
		    pBankInfo->SetSourceBank =
			    (miBankProcPtr)CHIPSWINSetReadPlanar;
		    pBankInfo->SetDestinationBank =
			    (miBankProcPtr)CHIPSWINSetWritePlanar;
		    pBankInfo->SetSourceAndDestinationBanks =
			    (miBankProcPtr)CHIPSWINSetReadWritePlanar;
		} else {
		    pBankInfo->SetSourceBank = (miBankProcPtr)CHIPSWINSetRead;
		    pBankInfo->SetDestinationBank =
			    (miBankProcPtr)CHIPSWINSetWrite;
		    pBankInfo->SetSourceAndDestinationBanks =
			    (miBankProcPtr)CHIPSWINSetReadWrite;
		}
	    } else {
		if (pScrn->bitsPerPixel < 8) {
		    pBankInfo->SetSourceBank =
			    (miBankProcPtr)CHIPSSetReadPlanar;
		    pBankInfo->SetDestinationBank =
			    (miBankProcPtr)CHIPSSetWritePlanar;
		    pBankInfo->SetSourceAndDestinationBanks =
			    (miBankProcPtr)CHIPSSetReadWritePlanar;
		} else {
		    pBankInfo->SetSourceBank = (miBankProcPtr)CHIPSSetRead;
		    pBankInfo->SetDestinationBank =
			    (miBankProcPtr)CHIPSSetWrite;
		    pBankInfo->SetSourceAndDestinationBanks =
			    (miBankProcPtr)CHIPSSetReadWrite;
		}
	    }
	}
	if (!miInitializeBanking(pScreen, pScrn->virtualX, pScrn->virtualY,
				 pScrn->displayWidth, pBankInfo)) {
	    free(pBankInfo);
	    pBankInfo = NULL;
	    return FALSE;
	}
	xf86SetBackingStore(pScreen);

	/* Initialise cursor functions */
	miDCInitialize (pScreen, xf86GetPointerScreenFuncs());

    } else
#endif /* USE_MIBANK */
    {
    /* !!! Only support linear addressing for now. This might change */
	/* Setup pointers to free space in video ram */
#define CHIPSALIGN(size, align) (currentaddr - ((currentaddr - size) & ~align))
	allocatebase = (pScrn->videoRam<<10) - cPtr->FrameBufferSize;
	
	if (pScrn->bitsPerPixel < 8)
	    freespace = allocatebase - pScrn->displayWidth * 
		    pScrn->virtualY / 2;
	else	
	    freespace = allocatebase - pScrn->displayWidth * 
		    pScrn->virtualY * (pScrn->bitsPerPixel >> 3);

	if ((cPtr->Flags & ChipsDualChannelSupport) && 
	    (cPtr->SecondCrtc == TRUE)) {
	    currentaddr = allocatebase + cPtrEnt->masterFbMapSize;
	} else
	    currentaddr = allocatebase;
	if (serverGeneration == 1)
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		   "%d bytes off-screen memory available\n", freespace);

	/* 
	 * Allocate video memory to store the hardware cursor. Allocate 1kB
	 * vram to the cursor, with 1kB alignment for 6554x's and 4kb alignment
	 * for 65550's. Wingine cursor is stored in registers and so no memory
	 * is needed.
	 */
	if (cAcl->UseHWCursor) {
	    cAcl->CursorAddress = -1;
	    if (IS_HiQV(cPtr)) {
		if (CHIPSALIGN(1024, 0xFFF) <= freespace) {
		    currentaddr -= CHIPSALIGN(1024, 0xFFF);
		    freespace -= CHIPSALIGN(1024, 0xFFF);
		    cAcl->CursorAddress = currentaddr;
		}
	    } else if (IS_Wingine(cPtr)) {
		cAcl->CursorAddress = 0;
	    } else if (CHIPSALIGN(1024, 0x3FF) <= freespace) {
		currentaddr -= CHIPSALIGN(1024, 0x3FF);
		freespace -= CHIPSALIGN(1024, 0x3FF);
		cAcl->CursorAddress = currentaddr;
	    }
	    if (cAcl->CursorAddress == -1)
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "Too little space for H/W cursor.\n");
	}
    
	cAcl->CacheEnd = currentaddr;

	/* Setup the acceleration primitives */
	/* Calculate space needed of offscreen pixmaps etc. */
	if (cPtr->Flags & ChipsAccelSupport) {
	    /* 
	     * A scratch area is now allocated in the video ram. This is used
	     * at 8 and 16 bpp to simulate a planemask with a complex ROP, and 
	     * at 24 and 32 bpp to aid in accelerating solid fills
	     */
	    cAcl->ScratchAddress = -1;
	    switch  (pScrn->bitsPerPixel) {
	    case 8:
		if (CHIPSALIGN(64, 0x3F) <= freespace) {
		    currentaddr -= CHIPSALIGN(64, 0x3F);
		    freespace -= CHIPSALIGN(64, 0x3F);
		    cAcl->ScratchAddress = currentaddr;
		}
		break;
	    case 16:
		if (CHIPSALIGN(128, 0x7F) <= freespace) {
		    currentaddr -= CHIPSALIGN(128, 0x7F);
		    freespace -= CHIPSALIGN(128, 0x7F);
		    cAcl->ScratchAddress = currentaddr;
		}
		break;
	    case 24:
		/* One scanline of data used for solid fill */
		if (!IS_HiQV(cPtr)) {
		    if (CHIPSALIGN(3 * (pScrn->displayWidth + 4), 0x3)
			<= freespace) {
			currentaddr -= CHIPSALIGN(3 * (pScrn->displayWidth
					       + 4), 0x3);
			freespace -= CHIPSALIGN(3 * (pScrn->displayWidth + 4),
						0x3);
			cAcl->ScratchAddress = currentaddr;
		    }
		}
		break;
	    case 32:
		/* 16bpp 8x8 mono pattern fill for solid fill. QWORD aligned */
		if (IS_HiQV(cPtr)) {
		    if (CHIPSALIGN(8, 0x7) <= freespace) {
			currentaddr -= CHIPSALIGN(8, 0x7);
			freespace -= CHIPSALIGN(8, 0x7);
			cAcl->ScratchAddress = currentaddr;
		    }
		}
		break;
	    }

	    /* Setup the boundaries of the pixmap cache */
	    cAcl->CacheStart = currentaddr - freespace;
	    cAcl->CacheEnd = currentaddr;

	    if (cAcl->CacheStart >= cAcl->CacheEnd) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "Too little space for pixmap cache.\n");
		cAcl->CacheStart = 0;
		cAcl->CacheEnd = 0;
	    }

	    if (IS_HiQV(cPtr)) 
		cAcl->BltDataWindow = (unsigned char *)cPtr->MMIOBase
		    + 0x10000L;
	    else
		cAcl->BltDataWindow = cPtr->FbBase;
	    
	}
	/*
	 * Initialize FBManager: 
	 * we do even with no acceleration enabled
	 * so that video support can allocate space.
	 */
	   
	{
	    BoxRec AvailFBArea;
	    AvailFBArea.x1 = 0;
	    AvailFBArea.y1 = 0;
	    AvailFBArea.x2 = pScrn->displayWidth;
	    AvailFBArea.y2 = cAcl->CacheEnd /
		(pScrn->displayWidth * (pScrn->bitsPerPixel >> 3));

	    xf86InitFBManager(pScreen, &AvailFBArea); 
	}

	if (cPtr->Flags & ChipsAccelSupport) {
	    if (IS_HiQV(cPtr)) {
		CHIPSInitEXA(pScreen);
	    }
	}
	
	xf86SetBackingStore(pScreen);
#ifdef ENABLE_SILKEN_MOUSE
	xf86SetSilkenMouse(pScreen);
#endif

	/* Initialise cursor functions */
	miDCInitialize (pScreen, xf86GetPointerScreenFuncs());

	if ((cAcl->UseHWCursor) && (cAcl->CursorAddress != -1)) {
	    /* HW cursor functions */
	    if (!CHIPSCursorInit(pScreen)) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "Hardware cursor initialization failed\n");
		return FALSE;
	    }
	}
    }

    if (cPtr->Flags & ChipsShadowFB) {
	RefreshAreaFuncPtr refreshArea = chipsRefreshArea;

	if(cPtr->Rotate) {
	    if (!cPtr->PointerMoved) {
		cPtr->PointerMoved = pScrn->PointerMoved;
		pScrn->PointerMoved = chipsPointerMoved;
	    }
	    
	   switch(pScrn->bitsPerPixel) {
	   case 8:	refreshArea = chipsRefreshArea8;	break;
	   case 16:	refreshArea = chipsRefreshArea16;	break;
	   case 24:	refreshArea = chipsRefreshArea24;	break;
	   case 32:	refreshArea = chipsRefreshArea32;	break;
	   }
	}
	ShadowFBInit(pScreen, refreshArea);
    }
	
    /* Initialise default colourmap */
    if (!miCreateDefColormap(pScreen))
	return FALSE;
    
    if(!xf86HandleColormaps(pScreen, 256, pScrn->rgbBits,
		(pScrn->depth == 16 ? chipsLoadPalette16 : chipsLoadPalette),
		NULL, CMAP_RELOAD_ON_MODE_SWITCH | CMAP_PALETTED_TRUECOLOR))
	return FALSE;
    
#ifndef XSERVER_LIBPCIACCESS
    racflag = RAC_COLORMAP;
    if (cAcl->UseHWCursor)
        racflag |= RAC_CURSOR;
    racflag |= (RAC_FB | RAC_VIEWPORT);
    /* XXX Check if I/O and Mem flags need to be the same. */
    pScrn->racIoFlags = pScrn->racMemFlags = racflag;
#endif
#ifdef ENABLE_SILKEN_MOUSE
	xf86SetSilkenMouse(pScreen);
#endif

	if ((cPtr->Flags & ChipsVideoSupport)
	    && (cPtr->Flags & ChipsLinearSupport)) {
	    CHIPSInitVideo(pScreen);
    }

    pScreen->SaveScreen = CHIPSSaveScreen;

    /* Setup DPMS mode */
    if (cPtr->Flags & ChipsDPMSSupport)
	xf86DPMSInit(pScreen, (DPMSSetProcPtr)chipsDisplayPowerManagementSet,
		     0);
    
    /* Wrap the current CloseScreen function */
    cPtr->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = CHIPSCloseScreen;

    /* Report any unused options (only for the first generation) */
    if (serverGeneration == 1) {
	xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);
    }

    return TRUE;
}

/* Mandatory */
Bool
CHIPSSwitchMode(SWITCH_MODE_ARGS_DECL)
{
    SCRN_INFO_PTR(arg);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSEntPtr cPtrEnt;

#ifdef DEBUG
    ErrorF("CHIPSSwitchMode\n");
#endif
    if (cPtr->UseDualChannel) {
        cPtrEnt = xf86GetEntityPrivate(pScrn->entityList[0],
					       CHIPSEntityIndex)->ptr;
	DUALREOPEN;
    }

    return chipsModeInit(pScrn, mode);
}

/* Mandatory */
void
CHIPSAdjustFrame(ADJUST_FRAME_ARGS_DECL)
{
    SCRN_INFO_PTR(arg);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSEntPtr cPtrEnt;

    int Base;
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    unsigned char tmp;

    if (xf86ReturnOptValBool(cPtr->Options, OPTION_SHOWCACHE, FALSE) && y) {
	int lastline = cPtr->FbMapSize / 
		((pScrn->displayWidth * pScrn->bitsPerPixel) / 8);
	lastline -= pScrn->currentMode->VDisplay;
	y += pScrn->virtualY - 1;
        if (y > lastline) y = lastline;
    }
    
    Base = y * pScrn->displayWidth + x;
    
    /* calculate base bpp dep. */
    switch (pScrn->bitsPerPixel) {
    case 1:
    case 4:
	Base >>= 3;
	break;
    case 16:
	Base >>= 1;
	break;
    case 24:
	if (!IS_HiQV(cPtr))
	    Base = (Base >> 2) * 3;
	else
	    Base = (Base >> 3) * 6;  /* 65550 seems to need 64bit alignment */
	break;
    case 32:
	break;
    default:			     /* 8bpp */
	Base >>= 2;
	break;
    }

    if (cPtr->UseDualChannel) {
        cPtrEnt = xf86GetEntityPrivate(pScrn->entityList[0],
					       CHIPSEntityIndex)->ptr;
	DUALREOPEN;
    }

    /* write base to chip */
    /*
     * These are the generic starting address registers.
     */
    chipsFixResume(pScrn);
    hwp->writeCrtc(hwp, 0x0C, (Base & 0xFF00) >> 8);
    hwp->writeCrtc(hwp, 0x0D, Base & 0xFF);
    if (IS_HiQV(cPtr)) {
	if (((cPtr->readXR(cPtr, 0x09)) & 0x1) == 0x1)
	    hwp->writeCrtc(hwp, 0x40, ((Base & 0x0F0000) >> 16) | 0x80);
    } else {
	tmp = cPtr->readXR(cPtr, 0x0C);
	cPtr->writeXR(cPtr, 0x0C, ((Base & (IS_Wingine(cPtr) ? 0x0F0000 : 
	     0x030000)) >> 16) | (tmp & 0xF8));
    }

    if (cPtr->UseDualChannel &&
		(! xf86IsEntityShared(pScrn->entityList[0]))) {
	unsigned int IOSS, MSS;
	IOSS = cPtr->readIOSS(cPtr);
	MSS = cPtr->readMSS(cPtr);
	cPtr->writeIOSS(cPtr, ((cPtr->storeIOSS & IOSS_MASK) |
			       IOSS_PIPE_B));
	cPtr->writeMSS(cPtr, hwp, ((cPtr->storeMSS & MSS_MASK) | MSS_PIPE_B));

	chipsFixResume(pScrn);
	hwp->writeCrtc(hwp, 0x0C, (Base & 0xFF00) >> 8);
	hwp->writeCrtc(hwp, 0x0D, Base & 0xFF);
	if (((cPtr->readXR(cPtr, 0x09)) & 0x1) == 0x1)
	    hwp->writeCrtc(hwp, 0x40, ((Base & 0x0F0000) >> 16) | 0x80);

	cPtr->writeIOSS(cPtr, IOSS);
	cPtr->writeMSS(cPtr, hwp, MSS);
    }

}

/* Mandatory */
static Bool
CHIPSCloseScreen(CLOSE_SCREEN_ARGS_DECL)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSEntPtr cPtrEnt;    

    if(pScrn->vtSema){   /*§§§*/
	if (cPtr->Flags & ChipsDualChannelSupport) {
  	    cPtrEnt = xf86GetEntityPrivate(pScrn->entityList[0],
					   CHIPSEntityIndex)->ptr;
	    if (cPtr->UseDualChannel)
		DUALREOPEN;
	    DUALCLOSE;
	} else {
	    chipsHWCursorOff(cPtr, pScrn);
	    chipsRestore(pScrn, &(VGAHWPTR(pScrn))->SavedReg, &cPtr->SavedReg,
					TRUE);
	    chipsLock(pScrn);
	}
	chipsUnmapMem(pScrn);
    }

    if (xf86IsEntityShared(pScrn->entityList[0])) {
	DevUnion *pPriv;
	pPriv = xf86GetEntityPrivate(pScrn->entityList[0], CHIPSEntityIndex);
	cPtrEnt = pPriv->ptr;
	cPtrEnt->refCount--;
    }
    if (cPtr->CursorInfoRec)
	xf86DestroyCursorInfoRec(cPtr->CursorInfoRec);
    free(cPtr->ShadowPtr);
    free(cPtr->DGAModes);
    pScrn->vtSema = FALSE;
    if(cPtr->BlockHandler)
	pScreen->BlockHandler = cPtr->BlockHandler;

    pScreen->CloseScreen = cPtr->CloseScreen; /*§§§*/
    xf86ClearPrimInitDone(pScrn->entityList[0]);
    return (*pScreen->CloseScreen)(CLOSE_SCREEN_ARGS);/*§§§*/
}

/* Optional */
static void
CHIPSFreeScreen(FREE_SCREEN_ARGS_DECL)
{
    SCRN_INFO_PTR(arg);
    if (xf86LoaderCheckSymbol("vgaHWFreeHWRec"))
	vgaHWFreeHWRec(pScrn);
    CHIPSFreeRec(pScrn);
}

/* Optional */
static ModeStatus
CHIPSValidMode(SCRN_ARG_TYPE arg, DisplayModePtr mode, Bool verbose, int flags)
{
    SCRN_INFO_PTR(arg);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);

    /* The tests here need to be expanded */
    if ((mode->Flags & V_INTERLACE) && (cPtr->PanelType & ChipsLCD))
	return MODE_NO_INTERLACE;
    if ((cPtr->PanelType & ChipsLCD)
	&& !xf86ReturnOptValBool(cPtr->Options, OPTION_PANEL_SIZE, FALSE)
	&& ((cPtr->PanelSize.HDisplay < mode->HDisplay)
	    || (cPtr->PanelSize.VDisplay < mode->VDisplay)))
      return MODE_PANEL;

    return MODE_OK;
}

/*
 * DPMS Control registers
 *
 * XR73 6554x and 64300 (what about 65535?)
 * XR61 6555x
 *    0   HSync Powerdown data
 *    1   HSync Select 1=Powerdown
 *    2   VSync Powerdown data
 *    3   VSync Select 1=Powerdown
 */

static void
chipsDisplayPowerManagementSet(ScrnInfoPtr pScrn, int PowerManagementMode,
			       int flags)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSEntPtr cPtrEnt;

    unsigned char dpmsreg, seqreg, lcdoff, tmp;
    
    if (!pScrn->vtSema)
	return;

#if GET_ABI_MAJOR(ABI_VIDEODRV_VERSION) < 8
    xf86EnableAccess(pScrn);
#endif
    switch (PowerManagementMode) {
    case DPMSModeOn:
	/* Screen: On; HSync: On, VSync: On */
	dpmsreg = 0x00;
	seqreg = 0x00;
	lcdoff = 0x0;
	break;
    case DPMSModeStandby:
	/* Screen: Off; HSync: Off, VSync: On */
	dpmsreg = 0x02;
	seqreg = 0x20;
	lcdoff = 0x0;
	break;
    case DPMSModeSuspend:
	/* Screen: Off; HSync: On, VSync: Off */
	dpmsreg = 0x08;
	seqreg = 0x20;
	lcdoff = 0x1;
	break;
    case DPMSModeOff:
	/* Screen: Off; HSync: Off, VSync: Off */
	dpmsreg = 0x0A;
	seqreg = 0x20;
	lcdoff = 0x1;
	break;
    default:
	return;
    }

    if (cPtr->UseDualChannel) {
        cPtrEnt = xf86GetEntityPrivate(pScrn->entityList[0],
					       CHIPSEntityIndex)->ptr;
	DUALREOPEN;
    }

    seqreg |= hwp->readSeq(hwp, 0x01) & ~0x20;
    hwp->writeSeq(hwp, 0x01, seqreg);
    if (IS_HiQV(cPtr)) {
	tmp = cPtr->readXR(cPtr, 0x61);
	cPtr->writeXR(cPtr, 0x61, (tmp & 0xF0) | dpmsreg);
    } else {
	tmp = cPtr->readXR(cPtr, 0x73);
	cPtr->writeXR(cPtr, 0x73, (tmp & 0xF0) | dpmsreg);
    }
    
    /* Turn off the flat panel */
    if (cPtr->PanelType & ChipsLCDProbed) {
	if (IS_HiQV(cPtr)) {
	    if (cPtr->Chipset == CHIPS_CT69030) {
#if 0
	        /* Where is this for the 69030?? */
		tmp = cPtr->readFR(cPtr, 0x05);
		if (lcdoff)
		    cPtr->writeFR(cPtr, 0x05, tmp | 0x08);
		else
		    cPtr->writeFR(cPtr, 0x05, tmp & 0xF7);
#endif
	    } else {
		tmp = cPtr->readFR(cPtr, 0x05);
		if (lcdoff)
		    cPtr->writeFR(cPtr, 0x05, tmp | 0x08);
		else
		    cPtr->writeFR(cPtr, 0x05, tmp & 0xF7);
	    }
	} else {
	    tmp = cPtr->readXR(cPtr, 0x52);
	    if (lcdoff)
		cPtr->writeXR(cPtr, 0x52, tmp | 0x08);
	    else
		cPtr->writeXR(cPtr, 0x52, tmp & 0xF7);
	}
    }
}

static Bool
CHIPSSaveScreen(ScreenPtr pScreen, int mode)
{
    ScrnInfoPtr pScrn = NULL;            /* §§§ */
    Bool unblank;

    unblank = xf86IsUnblank(mode);

    if (pScreen != NULL)
	pScrn = xf86ScreenToScrn(pScreen);

    if (unblank)
	SetTimeSinceLastInputEvent();

    if ((pScrn != NULL) && pScrn->vtSema) { /* §§§ */
	chipsBlankScreen(pScrn, unblank);
    }
    return (TRUE);
}

static Bool
chipsClockSelect(ScrnInfoPtr pScrn, int no)
{
    CHIPSClockReg TmpClock;
    CHIPSPtr cPtr = CHIPSPTR(pScrn);

    switch (no) {
    case CLK_REG_SAVE:
	chipsClockSave(pScrn, &cPtr->SaveClock);
	break;

    case CLK_REG_RESTORE:
	chipsClockLoad(pScrn, &cPtr->SaveClock);
	break;

    default:
	if (!chipsClockFind(pScrn, NULL, no, &TmpClock))
	    return (FALSE);
	chipsClockLoad(pScrn, &TmpClock);
    }
    return (TRUE);
}

/*
 * 
 * Fout = (Fref * 4 * M) / (PSN * N * (1 << P) )
 * Fvco = (Fref * 4 * M) / (PSN * N)
 * where
 * M = XR31+2
 * N = XR32+2
 * P = XR30[3:1]
 * PSN = XR30[0]? 1:4
 * 
 * constraints:
 * 4 MHz <= Fref <= 20 MHz (typ. 14.31818 MHz)
 * 150 kHz <= Fref/(PSN * N) <= 2 MHz
 * 48 MHz <= Fvco <= 220 MHz
 * 2 < M < 128
 * 2 < N < 128
 */

static void
chipsClockSave(ScrnInfoPtr pScrn, CHIPSClockPtr Clock)
{
    unsigned char tmp;
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    unsigned char Type = cPtr->ClockType;
    CHIPSEntPtr cPtrEnt;

    Clock->msr = hwp->readMiscOut(hwp)&0xFE; /* save standard VGA clock reg */
    switch (Type & GET_STYLE) {
    case HiQV_STYLE:
	/* save alternate clock select reg.*/
	/* The 69030 FP clock select is at FR01 instead */
      if (cPtr->UseDualChannel) {
        cPtrEnt = xf86GetEntityPrivate(pScrn->entityList[0],
					       CHIPSEntityIndex)->ptr;
	DUALREOPEN;
      }

	if (cPtr->Flags & ChipsDualChannelSupport)
	    Clock->fr03 = cPtr->readFR(cPtr, 0x01);
	else
	    Clock->fr03 = cPtr->readFR(cPtr, 0x03);
	if (!Clock->Clock) {   /* save HiQV console clock           */
	    tmp = cPtr->CRTclkInx << 2;
	    cPtr->CRTClk[0] = cPtr->readXR(cPtr, 0xC0 + tmp);
	    cPtr->CRTClk[1] = cPtr->readXR(cPtr, 0xC1 + tmp);
	    cPtr->CRTClk[2] = cPtr->readXR(cPtr, 0xC2 + tmp);
	    cPtr->CRTClk[3] = cPtr->readXR(cPtr, 0xC3 + tmp);
	    tmp = cPtr->FPclkInx << 2;
	    cPtr->FPClk[0] = cPtr->readXR(cPtr, 0xC0 + tmp);
	    cPtr->FPClk[1] = cPtr->readXR(cPtr, 0xC1 + tmp);
	    cPtr->FPClk[2] = cPtr->readXR(cPtr, 0xC2 + tmp);
	    cPtr->FPClk[3] = cPtr->readXR(cPtr, 0xC3 + tmp);
	}
	break;
    case OLD_STYLE: 
	Clock->fcr = hwp->readFCR(hwp);
	Clock->xr02 = cPtr->readXR(cPtr, 0x02);
	Clock->xr54 = cPtr->readXR(cPtr, 0x54); /* save alternate clock select reg.*/
	break;
    case WINGINE_1_STYLE:
    case WINGINE_2_STYLE:
	break;
    case NEW_STYLE:
	Clock->xr54 = cPtr->readXR(cPtr, 0x54); /* save alternate clock select reg.*/
	Clock->xr33 = cPtr->readXR(cPtr, 0x33); /* get status of MCLK/VCLK sel reg.*/
	break;
    }
#ifdef DEBUG
    ErrorF("saved \n");
#endif
}

static Bool
chipsClockFind(ScrnInfoPtr pScrn, DisplayModePtr mode,
	       int no, CHIPSClockPtr Clock )
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    unsigned char Type = cPtr->ClockType;
    CHIPSEntPtr cPtrEnt;

    if (no > (pScrn->numClocks - 1))
	return (FALSE);

    if (cPtr->UseDualChannel) {
        cPtrEnt = xf86GetEntityPrivate(pScrn->entityList[0],
					       CHIPSEntityIndex)->ptr;
	DUALREOPEN;
    }

    switch (Type & GET_STYLE) {
    case HiQV_STYLE:
	Clock->msr = cPtr->CRTclkInx << 2;
	Clock->fr03 = cPtr->FPclkInx << 2;
	Clock->Clock = mode ? mode->Clock : 0;
	if (xf86ReturnOptValBool(cPtr->Options, OPTION_USE_MODELINE, FALSE)) {
	    Clock->FPClock = mode ? mode->Clock : 0;
	} else
	    Clock->FPClock = cPtr->FPclock;
	break;
    case NEW_STYLE:
	if (Type & TYPE_HW) {
	    Clock->msr = (no == 4 ? 3 << 2: (no & 0x01) << 2);
	    Clock->xr54 = Clock->msr;               
	    Clock->xr33 = no > 1 ? 0x80 : 0;	
	} else {
	    Clock->msr = 3 << 2;
	    Clock->xr33 = 0;
	    Clock->xr54 = Clock->msr;
	    /* update panel type in case somebody switched.
	     * This should be handled more generally:
	     * On mode switch DDC should be reread, all
	     * display dependent data should be reevaluated.
	     * This will be built in when we start Display
	     * HotPlug support.
	     * Until then we have to do it here as somebody
	     * might have switched displays on us and we only
	     * have one programmable clock which needs to
	     * be shared for CRT and LCD.
	     */
	    chipsSetPanelType(cPtr);
	    {
	      Bool fp_m;
	      if (cPtr->Options 
		  && xf86GetOptValBool(cPtr->Options, OPTION_FP_MODE, &fp_m)) {
		   if (fp_m) 
		       cPtr->PanelType |= ChipsLCD;
		   else 
		       cPtr->PanelType = ~ChipsLCD;
	      }
	    }

	    if ((cPtr->PanelType & ChipsLCD) && cPtr->FPclock) 
		Clock->Clock = cPtr->FPclock;
	    else
		Clock->Clock = mode ? mode->SynthClock : 0;
	}
	break;
    case OLD_STYLE:
	if (no > 3) {
	    Clock->msr = 3 << 2;
	    Clock->fcr = no & 0x03;
	    Clock->xr02 = 0;
	    Clock->xr54 = Clock->msr & (Clock->fcr << 4);
	} else {
	    Clock->msr = (no << 2) & 0x4;
	    Clock->fcr = 0;
	    Clock->xr02 = no & 0x02;
	    Clock->xr54 = Clock->msr;
	}
	break;
    case WINGINE_1_STYLE:
	Clock->msr = no << 2;
    case WINGINE_2_STYLE:
	if (Type & TYPE_HW) {
	    Clock->msr = (no == 2 ? 3 << 2: (no & 0x01) << 2);
	    Clock->xr33 = 0;	
	} else {
	    Clock->msr = 3 << 2;
	    Clock->xr33 = 0;
	    Clock->Clock = mode ? mode->SynthClock : 0;
	}
	break;
    }
    Clock->msr |= (hwp->readMiscOut(hwp) & 0xF2);

#ifdef DEBUG
    ErrorF("found\n");
#endif
    return (TRUE);
}
    

static int
chipsGetHWClock(ScrnInfoPtr pScrn)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    unsigned char Type = cPtr->ClockType;
    unsigned char tmp, tmp1;

    if (!(Type & TYPE_HW))
        return 0;		/* shouldn't happen                   */

    switch (Type & GET_STYLE) {
    case WINGINE_1_STYLE:
	return ((hwp->readMiscOut(hwp) & 0x0C) >> 2);
    case WINGINE_2_STYLE:
	tmp = ((hwp->readMiscOut(hwp) & 0x04) >> 2);
	return (tmp > 2) ? 2 : tmp;
    case OLD_STYLE:
	if (!(cPtr->PanelType & ChipsLCDProbed))
	    tmp = hwp->readMiscOut(hwp);
	else
	    tmp = cPtr->readXR(cPtr, 0x54);
	if (tmp & 0x08) {
	    if (!(cPtr->PanelType & ChipsLCDProbed))
		tmp = hwp->readFCR(hwp) & 0x03;
	    else 
		tmp = (tmp >> 4) & 0x03;
	    return (tmp + 4);
	} else {
	    tmp = (tmp >> 2) & 0x01;
	    tmp1 = cPtr->readXR(cPtr, 0x02);
	    return (tmp + (tmp1 & 0x02));
	}
    case NEW_STYLE:
	if (cPtr->PanelType & ChipsLCDProbed) {
	    tmp = cPtr->readXR(cPtr, 0x54);
	} else
	    tmp = hwp->readMiscOut(hwp);
	tmp = (tmp & 0x0C) >> 2;
	if (tmp > 1) return 4;
	tmp1 = cPtr->readXR(cPtr, 0x33);
	tmp1 = (tmp1 & 0x80) >> 6; /* iso mode 25.175/28.322 or 32/36 MHz  */
	return (tmp + tmp1);       /*            ^=0    ^=1     ^=4 ^=5    */
    default:		       /* we should never get here              */
	return (0);
    }
}

static void
chipsClockLoad(ScrnInfoPtr pScrn, CHIPSClockPtr Clock)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    unsigned char Type = cPtr->ClockType;
    volatile unsigned char tmp, tmpmsr, tmpfcr, tmp02;
    volatile unsigned char tmp33, tmp54, tmpf03;
    unsigned char vclk[3];       

    tmpmsr = hwp->readMiscOut(hwp);  /* read msr, needed for all styles */

    switch (Type & GET_STYLE) {
    case HiQV_STYLE:
	/* save alternate clock select reg.  */
	/* The 69030 FP clock select is at FR01 instead */
	if (cPtr->Flags & ChipsDualChannelSupport) {
	    tmpf03 = cPtr->readFR(cPtr, 0x01);
	} else
	    tmpf03 = cPtr->readFR(cPtr, 0x03);
	/* select fixed clock 0  before tampering with VCLK select */
	hwp->writeMiscOut(hwp, (tmpmsr & ~0x0D) |
			   cPtr->SuspendHack.vgaIOBaseFlag);
	/* The 69030 FP clock select is at FR01 instead */
	if (cPtr->Flags & ChipsDualChannelSupport) {
	    cPtr->writeFR(cPtr, 0x01, (tmpf03 & ~0x0C) | 0x04);
	} else
	    cPtr->writeFR(cPtr, 0x03, (tmpf03 & ~0x0C) | 0x04);
	if (!Clock->Clock) {      /* Hack to load saved console clock  */
	    tmp = cPtr->CRTclkInx << 2;
	    cPtr->writeXR(cPtr, 0xC0 + tmp, (cPtr->CRTClk[0] & 0xFF));
	    cPtr->writeXR(cPtr, 0xC1 + tmp, (cPtr->CRTClk[1] & 0xFF));
	    cPtr->writeXR(cPtr, 0xC2 + tmp, (cPtr->CRTClk[2] & 0xFF));
	    cPtr->writeXR(cPtr, 0xC3 + tmp, (cPtr->CRTClk[3] & 0xFF));

	    if (cPtr->FPClkModified) {
	        usleep(10000); /* let VCO stabilize */
	        tmp = cPtr->FPclkInx << 2;
		cPtr->writeXR(cPtr, 0xC0 + tmp, (cPtr->FPClk[0] & 0xFF));
		cPtr->writeXR(cPtr, 0xC1 + tmp, (cPtr->FPClk[1] & 0xFF));
		cPtr->writeXR(cPtr, 0xC2 + tmp, (cPtr->FPClk[2] & 0xFF));
		cPtr->writeXR(cPtr, 0xC3 + tmp, (cPtr->FPClk[3] & 0xFF));
	    }
	} else {
	    /* 
	     * Don't use the extra 2 bits in the M, N registers available
	     * on the HiQV, so write zero to 0xCA 
	     */
	    chipsCalcClock(pScrn, Clock->Clock, vclk);
	    tmp = cPtr->CRTclkInx << 2;
	    cPtr->writeXR(cPtr, 0xC0 + tmp, (vclk[1] & 0xFF));
	    cPtr->writeXR(cPtr, 0xC1 + tmp, (vclk[2] & 0xFF));
	    cPtr->writeXR(cPtr, 0xC2 + tmp, 0x0);
	    cPtr->writeXR(cPtr, 0xC3 + tmp, (vclk[0] & 0xFF));
	    if (Clock->FPClock) { 
	        usleep(10000); /* let VCO stabilize */
    	        chipsCalcClock(pScrn, Clock->FPClock, vclk);
	        tmp = cPtr->FPclkInx << 2;
		cPtr->writeXR(cPtr, 0xC0 + tmp, (vclk[1] & 0xFF));
		cPtr->writeXR(cPtr, 0xC1 + tmp, (vclk[2] & 0xFF));
		cPtr->writeXR(cPtr, 0xC2 + tmp, 0x0);
		cPtr->writeXR(cPtr, 0xC3 + tmp, (vclk[0] & 0xFF));
		cPtr->FPClkModified = TRUE;
	    }
	}
	usleep(10000);		         /* Let VCO stabilise    */
	/* The 69030 FP clock select is at FR01 instead */
	if (cPtr->Flags & ChipsDualChannelSupport) {
	    cPtr->writeFR(cPtr, 0x01, ((tmpf03 & ~0x0C) |
			(Clock->fr03 & 0x0C)));
	} else
	    cPtr->writeFR(cPtr, 0x03, ((tmpf03 & ~0x0C) |
			(Clock->fr03 & 0x0C)));
	break;
    case WINGINE_1_STYLE:
	break;
    case WINGINE_2_STYLE:
	/* Only write to soft clock registers if we really need to */
	if ((Type & GET_TYPE) == TYPE_PROGRAMMABLE) {
	    /* select fixed clock 0  before tampering with VCLK select */
	    hwp->writeMiscOut(hwp, (tmpmsr & ~0x0D) | 
			       cPtr->SuspendHack.vgaIOBaseFlag);
	    chipsCalcClock(pScrn, Clock->Clock, vclk);
	    tmp33 = cPtr->readXR(cPtr, 0x33); /* get status of MCLK/VCLK select reg */
	    cPtr->writeXR(cPtr, 0x33, tmp33 & ~0x20);
	    cPtr->writeXR(cPtr, 0x30, vclk[0]);
	    cPtr->writeXR(cPtr, 0x31, vclk[1]);     /* restore VCLK regs.   */
	    cPtr->writeXR(cPtr, 0x32, vclk[2]);
	    /*  cPtr->writeXR(cPtr, 0x33, tmp33 & ~0x20);*/
	    usleep(10000);		     /* Let VCO stabilise    */
	}
	break;
    case OLD_STYLE:
	tmp02 = cPtr->readXR(cPtr, 0x02);
	tmp54 = cPtr->readXR(cPtr, 0x54);
	tmpfcr = hwp->readFCR(hwp);
	cPtr->writeXR(cPtr, 0x02, ((tmp02 & ~0x02) | (Clock->xr02 & 0x02)));
	cPtr->writeXR(cPtr, 0x54, ((tmp54 & 0xF0) | (Clock->xr54 & ~0xF0)));
	hwp->writeFCR(hwp, (tmpfcr & ~0x03) & Clock->fcr);
	break;
    case NEW_STYLE:
	tmp33 = cPtr->readXR(cPtr, 0x33); /* get status of MCLK/VCLK select reg */
	tmp54 = cPtr->readXR(cPtr, 0x54);
	/* Only write to soft clock registers if we really need to */
	if ((Type & GET_TYPE) == TYPE_PROGRAMMABLE) {
	    /* select fixed clock 0  before tampering with VCLK select */
	    hwp->writeMiscOut(hwp, (tmpmsr & ~0x0D) |
			   cPtr->SuspendHack.vgaIOBaseFlag);
	    cPtr->writeXR(cPtr, 0x54, (tmp54 & 0xF3));
	    /* if user wants to set the memory clock, do it first */
	    if (cPtr->MemClock.Clk) {
		chipsCalcClock(pScrn, cPtr->MemClock.Clk, vclk);
		/* close eyes, hold breath ....*/
		cPtr->writeXR(cPtr, 0x33, tmp33 | 0x20);
		cPtr->writeXR(cPtr, 0x30, vclk[0]);
		cPtr->writeXR(cPtr, 0x31, vclk[1]);
		cPtr->writeXR(cPtr, 0x32, vclk[2]);
		usleep(10000);
	    }
	    chipsCalcClock(pScrn, Clock->Clock, vclk);
	    cPtr->writeXR(cPtr, 0x33, tmp33 & ~0x20);
	    cPtr->writeXR(cPtr, 0x30, vclk[0]);
	    cPtr->writeXR(cPtr, 0x31, vclk[1]);     /* restore VCLK regs.   */
	    cPtr->writeXR(cPtr, 0x32, vclk[2]);
	    /*  cPtr->writeXR(cPtr, 0x33, tmp33 & ~0x20);*/
	    usleep(10000);		         /* Let VCO stabilise    */
	}
	cPtr->writeXR(cPtr, 0x33, ((tmp33 & ~0x80) | (Clock->xr33 & 0x80))); 
	cPtr->writeXR(cPtr, 0x54, ((tmp54 & 0xF3) | (Clock->xr54 & ~0xF3)));
	break;
    }
    hwp->writeMiscOut(hwp, (Clock->msr & 0xFE) |
			   cPtr->SuspendHack.vgaIOBaseFlag);
#ifdef DEBUG
    ErrorF("restored\n");
#endif
}
   
/* 
 * This is Ken Raeburn's <raeburn@raeburn.org> clock
 * calculation code just modified a little bit to fit in here.
 */

static void
chipsCalcClock(ScrnInfoPtr pScrn, int Clock, unsigned char *vclk)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    int M, N, P = 0, PSN = 0, PSNx = 0;

    int bestM = 0, bestN = 0, bestP = 0, bestPSN = 0;
    double abest = 42;
#ifdef DEBUG
    double bestFout = 0;
#endif
    double target;

    double Fvco, Fout;
    double error, aerror;

    int M_min = 3;

    /* Hack to deal with problem of Toshiba 720CDT clock */
    int M_max = (IS_HiQV(cPtr) && cPtr->Chipset != CHIPS_CT69000 &&
				   cPtr->Chipset != CHIPS_CT69030) ? 63 : 127;

    /* @@@ < CHIPS_CT690x0 ?? */

    /* Other parameters available on the 65548 but not the 65545, and
     * not documented in the Clock Synthesizer doc in rev 1.0 of the
     * 65548 datasheet:
     * 
     * + XR30[4] = 0, VCO divider loop uses divide by 4 (same as 65545)
     * 1, VCO divider loop uses divide by 16
     * 
     * + XR30[5] = 1, reference clock is divided by 5
     * 
     * Other parameters available on the 65550 and not on the 65545
     * 
     * + XRCB[2] = 0, VCO divider loop uses divide by 4 (same as 65545)
     * 1, VCO divider loop uses divide by 16
     * 
     * + XRCB[1] = 1, reference clock is divided by 5
     * 
     * + XRCB[7] = Vclk = Mclk
     * 
     * + XRCA[0:1] = 2 MSB of a 10 bit M-Divisor
     * 
     * + XRCA[4:5] = 2 MSB of a 10 bit N-Divisor
     * 
     * I haven't put in any support for those here.  For simplicity,
     * they should be set to 0 on the 65548, and left untouched on
     * earlier chips.
     *
     * Other parameters available on the 690x0
     *
     * + The 690x0 has no reference clock divider, so PSN must
     *   always be 1.
     *   XRCB[0:1] are reserved according to the data book
     */


    target = Clock * 1000;

    /* @@@ >= CHIPS_CT690x0 ?? */
    for (PSNx = ((cPtr->Chipset == CHIPS_CT69000) || 
		 (cPtr->Chipset == CHIPS_CT69030)) ? 1 : 0; PSNx <= 1; PSNx++) {
	int low_N, high_N;
	double Fref4PSN;

	PSN = PSNx ? 1 : 4;

	low_N = 3;
	high_N = 127;

	while (Fref / (PSN * low_N) > (((cPtr->Chipset == CHIPS_CT69000) || 
					(cPtr->Chipset == CHIPS_CT69030)) ? 5.0e6 : 2.0e6))
	    low_N++;
	while (Fref / (PSN * high_N) < 150.0e3)
	    high_N--;

	Fref4PSN = Fref * 4 / PSN;
	for (N = low_N; N <= high_N; N++) {
	    double tmp = Fref4PSN / N;

	    /* @@@ < CHIPS_CT690x0 ?? */
	    for (P = (IS_HiQV(cPtr) && (cPtr->Chipset != CHIPS_CT69000) &&
				(cPtr->Chipset != CHIPS_CT69030)) ? 1 : 0;
		 P <= 5; P++) {	
	        /* to force post divisor on Toshiba 720CDT */
		double Fvco_desired = target * (1 << P);
		double M_desired = Fvco_desired / tmp;

		/* Which way will M_desired be rounded?  Do all three just to
		 * be safe.  */
		int M_low = M_desired - 1;
		int M_hi = M_desired + 1;

		if (M_hi < M_min || M_low > M_max)
		    continue;

		if (M_low < M_min)
		    M_low = M_min;
		if (M_hi > M_max)
		    M_hi = M_max;

		for (M = M_low; M <= M_hi; M++) {
		    Fvco = tmp * M;
		    /* @@@ >= CHIPS_CT690x0 ?? */
		    if (Fvco <= ((cPtr->Chipset == CHIPS_CT69000 ||
			cPtr->Chipset == CHIPS_CT69030) ? 100.0e6 : 48.0e6))
			continue;
		    if (Fvco > 220.0e6)
			break;

		    Fout = Fvco / (1 << P);

		    error = (target - Fout) / target;

		    aerror = (error < 0) ? -error : error;
		    if (aerror < abest) {
			abest = aerror;
			bestM = M;
			bestN = N;
			bestP = P;
			bestPSN = PSN;
#ifdef DEBUG
			bestFout = Fout;
#endif
		    }
		}
	    }
	}
    }
    /* @@@ >= CHIPS_CT690x0 ?? */
    vclk[0] = (bestP << (IS_HiQV(cPtr) ? 4 : 1)) +
	(((cPtr->Chipset == CHIPS_CT69000) || (cPtr->Chipset == CHIPS_CT69030)) 
	? 0 : (bestPSN == 1));
    vclk[1] = bestM - 2;
    vclk[2] = bestN - 2;
#ifdef DEBUG
    ErrorF("Freq. selected: %.2f MHz, vclk[0]=%X, vclk[1]=%X, vclk[2]=%X\n",
	(float)(Clock / 1000.), vclk[0], vclk[1], vclk[2]);
    ErrorF("Freq. set: %.2f MHz\n", bestFout / 1.0e6);
#endif
}

static void
chipsSave(ScrnInfoPtr pScrn, vgaRegPtr VgaSave, CHIPSRegPtr ChipsSave)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    int i;
    unsigned char tmp;
#ifdef DEBUG
    ErrorF("chipsSave\n");
#endif
    
    /* set registers that we can program the controller */
    /* bank 0 */
    if (IS_HiQV(cPtr)) {
	cPtr->writeXR(cPtr, 0x0E, 0x00);
    } else {
	cPtr->writeXR(cPtr, 0x10, 0x00);
	cPtr->writeXR(cPtr, 0x11, 0x00);
	tmp = cPtr->readXR(cPtr, 0x0C) & ~0x50; /* WINgine stores MSB here */
	cPtr->writeXR(cPtr, 0x0C, tmp);
    }
    chipsFixResume(pScrn);
    tmp = cPtr->readXR(cPtr, 0x02);
    cPtr->writeXR(cPtr, 0x02, tmp & ~0x18);
    /* get generic registers */

#ifdef AVOID_VGAHW
    vgaHWSave(pScrn, VgaSave, VGA_SR_CMAP | VGA_SR_MODE);
#else
    vgaHWSave(pScrn, VgaSave, VGA_SR_ALL);
#endif

    /* save clock */
    chipsClockSave(pScrn, &ChipsSave->Clock);

    /* save extended registers */
    if (IS_HiQV(cPtr)) {
	for (i = 0; i < 0xFF; i++) {
#ifdef SAR04
	    /* Save SAR04 multimedia register correctly */
	    if (i == 0x4F)
	        cPtr->writeXR(cPtr, 0x4E, 0x04);
#endif
	    ChipsSave->XR[i] = cPtr->readXR(cPtr,i);
#ifdef DEBUG
	    ErrorF("XS%X - %X\n", i, ChipsSave->XR[i]);
#endif
	}
	for (i = 0; i < 0x80; i++) {
	    ChipsSave->FR[i] = cPtr->readFR(cPtr, i);
#ifdef DEBUG
	    ErrorF("FS%X - %X\n", i, ChipsSave->FR[i]);
#endif
	}
	for (i = 0; i < 0x80; i++) {
		ChipsSave->MR[i] = cPtr->readMR(cPtr, i);
#ifdef DEBUG
	    ErrorF("MS%X - %X\n", i, ChipsSave->FR[i]);
#endif
	}
	/* Save CR0-CR40 even though we don't use them, so they can be 
	 *  printed */
	for (i = 0x0; i < 0x80; i++) {
	    ChipsSave->CR[i] = hwp->readCrtc(hwp, i);
#ifdef DEBUG
	    ErrorF("CS%X - %X\n", i, ChipsSave->CR[i]);
#endif
	}
    } else {
	for (i = 0; i < 0x7D; i++) { /* don't touch XR7D and XR7F on WINGINE */
	    ChipsSave->XR[i] = cPtr->readXR(cPtr, i);
#ifdef DEBUG
	    ErrorF("XS%X - %X\n", i, ChipsSave->XR[i]);
#endif
	}
    }
}

Bool
chipsModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
#ifdef DEBUG
    ErrorF("chipsModeInit\n");
#endif
#if 0
    *(int*)0xFFFFFF0 = 0;
    ErrorF("done\n");
#endif

    chipsUnlock(pScrn);
    chipsFixResume(pScrn);

    if (cPtr->Accel.UseHWCursor)
	cPtr->Flags |= ChipsHWCursor;
    else
	cPtr->Flags &= ~ChipsHWCursor;
    /*
     * We need to delay cursor loading after resetting the video mode
     * to give the engine a chance to recover.
     */
    cPtr->cursorDelay = TRUE;
    
    if (IS_HiQV(cPtr))
	return chipsModeInitHiQV(pScrn, mode);
    else if (IS_Wingine(cPtr))
        return chipsModeInitWingine(pScrn, mode);
    else
      return chipsModeInit655xx(pScrn, mode);
}

/*
 * The timing register of the C&T FP chipsets are organized
 * as follows:
 * The chipsets have two sets of timing registers:
 * the standard horizontal and vertical timing registers for
 * display size, blank start, sync start, sync end, blank end 
 * and total size at their default VGA locations and extensions
 * and the alternate horizontal and vertical timing registers for
 * display size, sync start, sync end and total size.
 * In LCD and mixed (LCD+CRT) mode the alternate timing registers
 * control the timing. The alternate horizontal and vertical display 
 * size registers are set to the physical pixel size of the display. 
 * Normally the alternalte registers are set by the BIOS to optimized 
 * values. 
 * While the horizontal an vertical refresh rates are fixed independent
 * of the visible display size to ensure optimal performance of both 
 * displays they can be adapted to the screen resolution and CRT
 * requirements in CRT mode by programming the standard timing registers
 * in the VGA fashion.
 * In LCD and mixed mode the _standard_ horizontal and vertical display
 * size registers control the size of the _visible_ part of the display
 * in contast to the _physical_ size of the display which is specified
 * by the _alternate_ horizontal and vertical display size registers.
 * The size of the visible should always be equal or less than the
 * physical size.
 * For the 69030 chipsets, the CRT and LCD display channels are separate
 * and so can be driven independently.
 */
static Bool
chipsModeInitHiQV(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    int i;
    int lcdHTotal, lcdHDisplay;
    int lcdVTotal, lcdVDisplay;
    int lcdHRetraceStart, lcdHRetraceEnd;
    int lcdVRetraceStart, lcdVRetraceEnd;
    int lcdHSyncStart;
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSRegPtr ChipsNew;
    vgaRegPtr ChipsStd;
    unsigned int tmp;
    
    ChipsNew = &cPtr->ModeReg;
    ChipsStd = &hwp->ModeReg;


    /*
     * Possibly fix up the panel size, if the manufacture is stupid
     * enough to set it incorrectly in text modes
     */
    if (xf86ReturnOptValBool(cPtr->Options, OPTION_PANEL_SIZE, FALSE)) {
	cPtr->PanelSize.HDisplay = mode->CrtcHDisplay;
	cPtr->PanelSize.VDisplay = mode->CrtcVDisplay;
    }

    /* generic init */
    if (!vgaHWInit(pScrn, mode)) {
	ErrorF("bomb 1\n");
	return (FALSE);
    }
    pScrn->vtSema = TRUE;

    /* init clock */
    if (!chipsClockFind(pScrn, mode, mode->ClockIndex, &ChipsNew->Clock)) {
	ErrorF("bomb 2\n");
	return (FALSE);
    }

    /* Give Warning if the dual display mode will cause problems */
    /* Note 64bit wide memory bus assumed (as in 69000 and 69030 */
    if (cPtr->UseDualChannel && ((cPtr->SecondCrtc == TRUE) ||
				 (cPtr->Flags & ChipsDualRefresh))) {
	if (((ChipsNew->Clock.FPClock + ChipsNew->Clock.Clock) * 
		(max(1, pScrn->bitsPerPixel >> 3) +
		((cPtr->FrameBufferSize && (cPtr->PanelType & ChipsLCD)) ?
		1 : 0)) / (8 * 0.7)) > cPtr->MemClock.Max) {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		"Memory bandwidth requirements exceeded by dual-channel\n");
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		       "   mode. Display might be corrupted!!!\n");
	}
    }

    /* get C&T Specific Registers */
    for (i = 0; i < 0xFF; i++) {
#ifdef SAR04
	/* Save SAR04 multimedia register correctly */
	if (i == 0x4F)
	    cPtr->writeXR(cPtr, 0x4E, 0x04);
#endif
	ChipsNew->XR[i] = cPtr->readXR(cPtr, i);
    }
    for (i = 0; i < 0x80; i++) {
	ChipsNew->FR[i] = cPtr->readFR(cPtr, i);
    }
    for (i = 0; i < 0x80; i++) {
	ChipsNew->MR[i] = cPtr->readMR(cPtr, i);
    }
    for (i = 0x30; i < 0x80; i++) {    /* These are the CT extended CRT regs */
	ChipsNew->CR[i] = hwp->readCrtc(hwp, i);
    }

    /*
     * Here all of the other fields of 'ChipsNew' get filled in, to
     * handle the SVGA extended registers.  It is also allowable
     * to override generic registers whenever necessary.
     */

    /* some generic settings */
    if (pScrn->depth == 1) {
	ChipsStd->Attribute[0x10] = 0x03;   /* mode */
    } else {
	ChipsStd->Attribute[0x10] = 0x01;   /* mode */
    }
    ChipsStd->Attribute[0x11] = 0x00;   /* overscan (border) color */
    ChipsStd->Attribute[0x12] = 0x0F;   /* enable all color planes */
    ChipsStd->Attribute[0x13] = 0x00;   /* horiz pixel panning 0 */

    ChipsStd->Graphics[0x05] = 0x00;    /* normal read/write mode */

    /* set virtual screen width */
    tmp = pScrn->displayWidth >> 3;
    if (pScrn->bitsPerPixel == 16) {
	tmp <<= 1;		       /* double the width of the buffer */
    } else if (pScrn->bitsPerPixel == 24) {
	tmp += tmp << 1;
    } else if (pScrn->bitsPerPixel == 32) {
	tmp <<= 2;
    } else if (pScrn->bitsPerPixel < 8) {
	tmp >>= 1;
    }
    ChipsStd->CRTC[0x13] = tmp & 0xFF;
    ChipsNew->CR[0x41] = (tmp >> 8) & 0x0F;

    /* Set paging mode on the HiQV32 architecture, if required */
    if (!(cPtr->Flags & ChipsLinearSupport) || (pScrn->bitsPerPixel < 8))
	ChipsNew->XR[0x0A] |= 0x1;

#if X_BYTE_ORDER == X_BIG_ENDIAN
    ChipsNew->XR[0x0A] &= 0xCF;
    if (pScrn->bitsPerPixel == 16) {
	if  (!cPtr->dualEndianAp)
	    ChipsNew->XR[0x0A] |= 0x10;
    }
#endif
    ChipsNew->XR[0x09] |= 0x1;	       /* Enable extended CRT registers */
    ChipsNew->XR[0x0E] = 0;           /* Single map */
    ChipsNew->XR[0x40] |= 0x2;	       /* Don't wrap at 256kb */
    ChipsNew->XR[0x81] &= 0xF8;
    if (pScrn->bitsPerPixel >= 8) {
        ChipsNew->XR[0x40] |= 0x1;    /* High Resolution. XR40[1] reserved? */
	ChipsNew->XR[0x81] |= 0x2;    /* 256 Color Video */
    }
    ChipsNew->XR[0x80] |= 0x10;       /* Enable cursor output on P0 and P1 */
    if (pScrn->depth > 1) {
	if (pScrn->rgbBits == 8)
	    ChipsNew->XR[0x80] |= 0x80;
	else
	    ChipsNew->XR[0x80] &= ~0x80;
    }

    if (abs(cPtr->MemClock.Clk - cPtr->MemClock.ProbedClk) > 50) {
	/* set mem clk */
	ChipsNew->XR[0xCC] = cPtr->MemClock.xrCC;
	ChipsNew->XR[0xCD] = cPtr->MemClock.xrCD;
	ChipsNew->XR[0xCE] = cPtr->MemClock.xrCE;
    }

    /* Set the 69030 dual channel settings */
    if (cPtr->Flags & ChipsDualChannelSupport) {
	ChipsNew->FR[0x01] &= 0xFC;
        if ((cPtr->SecondCrtc == FALSE) && (cPtr->PanelType & ChipsLCD))
	    ChipsNew->FR[0x01] |= 0x02;
	else
	    ChipsNew->FR[0x01] |= 0x01;
	ChipsNew->FR[0x02] &= 0xCC;
	if ((cPtr->SecondCrtc == TRUE) || (cPtr->Flags & ChipsDualRefresh))
	    ChipsNew->FR[0x02] |= 0x01;	/* Set DAC to pipe B */
	else
	    ChipsNew->FR[0x02] &= 0xFE;	/* Set DAC to pipe A */

        if (cPtr->PanelType & ChipsLCD)
	    ChipsNew->FR[0x02] |= 0x20;	/* Enable the LCD output */
        if (cPtr->PanelType & ChipsCRT)
	    ChipsNew->FR[0x02] |= 0x10;	/* Enable the CRT output */
    }
    
    /* linear specific */
    if (cPtr->Flags & ChipsLinearSupport) {
	ChipsNew->XR[0x0A] |= 0x02;   /* Linear Addressing Mode */
	ChipsNew->XR[0x20] = 0x0;     /*BitBLT Draw Mode for 8 */
	ChipsNew->XR[0x05] =
	    (unsigned char)((cPtr->FbAddress >> 16) & 0xFF);
	ChipsNew->XR[0x06] = 
	    (unsigned char)((cPtr->FbAddress >> 24) & 0xFF);
    }

    /* panel timing */
    /* By default don't set panel timings, but allow it as an option */
    if (xf86ReturnOptValBool(cPtr->Options, OPTION_USE_MODELINE, FALSE)) {
	lcdHTotal = (mode->CrtcHTotal >> 3) - 5;
	lcdHDisplay = (cPtr->PanelSize.HDisplay >> 3) - 1;
	lcdHRetraceStart = (mode->CrtcHSyncStart >> 3);
	lcdHRetraceEnd = (mode->CrtcHSyncEnd >> 3);
	lcdHSyncStart = lcdHRetraceStart - 2;

	lcdVTotal = mode->CrtcVTotal - 2;
	lcdVDisplay = cPtr->PanelSize.VDisplay - 1;
	lcdVRetraceStart = mode->CrtcVSyncStart;
	lcdVRetraceEnd = mode->CrtcVSyncEnd;

	ChipsNew->FR[0x20] = lcdHDisplay & 0xFF;
	ChipsNew->FR[0x21] = lcdHRetraceStart & 0xFF;
	ChipsNew->FR[0x25] = ((lcdHRetraceStart & 0xF00) >> 4) |
	    ((lcdHDisplay & 0xF00) >> 8);
	ChipsNew->FR[0x22] = lcdHRetraceEnd & 0x1F;
	ChipsNew->FR[0x23] = lcdHTotal & 0xFF;
	ChipsNew->FR[0x24] = (lcdHSyncStart >> 3) & 0xFF;
	ChipsNew->FR[0x26] = (ChipsNew->FR[0x26] & ~0x1F)
	    | ((lcdHTotal & 0xF00) >> 8)
	    | (((lcdHSyncStart >> 3) & 0x100) >> 4);
	ChipsNew->FR[0x27] &= 0x7F;

	ChipsNew->FR[0x30] = lcdVDisplay & 0xFF;
	ChipsNew->FR[0x31] = lcdVRetraceStart & 0xFF;
	ChipsNew->FR[0x35] = ((lcdVRetraceStart & 0xF00) >> 4)
	    | ((lcdVDisplay & 0xF00) >> 8);
	ChipsNew->FR[0x32] = lcdVRetraceEnd & 0x0F;
	ChipsNew->FR[0x33] = lcdVTotal & 0xFF;
	ChipsNew->FR[0x34] = (lcdVTotal - lcdVRetraceStart) & 0xFF;
	ChipsNew->FR[0x36] = ((lcdVTotal & 0xF00) >> 8) |
	    (((lcdVTotal - lcdVRetraceStart) & 0x700) >> 4);
	ChipsNew->FR[0x37] |= 0x80;
    }

    /* Set up the extended CRT registers of the HiQV32 chips */
    ChipsNew->CR[0x30] = ((mode->CrtcVTotal - 2) & 0xF00) >> 8;
    ChipsNew->CR[0x31] = ((mode->CrtcVDisplay - 1) & 0xF00) >> 8;
    ChipsNew->CR[0x32] = (mode->CrtcVSyncStart & 0xF00) >> 8;
    ChipsNew->CR[0x33] = (mode->CrtcVBlankStart & 0xF00) >> 8;
    if ((cPtr->Chipset == CHIPS_CT69000) || (cPtr->Chipset == CHIPS_CT69030)) {
	/* The 690xx has overflow bits for the horizontal values as well */
	ChipsNew->CR[0x38] = (((mode->CrtcHTotal >> 3) - 5) & 0x100) >> 8;
	ChipsNew->CR[0x3C] = vgaHWHBlankKGA(mode, ChipsStd, 8, 0) << 6;
    } else
      vgaHWHBlankKGA(mode, ChipsStd, 6, 0);
    vgaHWVBlankKGA(mode, ChipsStd, 8, 0);

    ChipsNew->CR[0x40] |= 0x80;

    /* centering/stretching */
    if (!xf86ReturnOptValBool(cPtr->Options, OPTION_SUSPEND_HACK, FALSE)) {
	if (!xf86ReturnOptValBool(cPtr->Options, OPTION_LCD_STRETCH, FALSE)) {
	    ChipsNew->FR[0x40] &= 0xDF;    /* Disable Horizontal stretching */
	    ChipsNew->FR[0x48] &= 0xFB;    /* Disable vertical stretching */
	    ChipsNew->XR[0xA0] = 0x10;     /* Disable cursor stretching */
	} else {
	    ChipsNew->FR[0x40] |= 0x21;    /* Enable Horizontal stretching */
	    ChipsNew->FR[0x48] |= 0x05;    /* Enable vertical stretching */
	    ChipsNew->XR[0xA0] = 0x70;     /* Enable cursor stretching */
	    if (cPtr->Accel.UseHWCursor 
		&& cPtr->PanelSize.HDisplay && cPtr->PanelSize.VDisplay
		&& (cPtr->PanelSize.HDisplay != mode->CrtcHDisplay)
		&& (cPtr->PanelSize.VDisplay != mode->CrtcVDisplay)) {
		if(cPtr->Accel.UseHWCursor)
		    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
			       "Disabling HW Cursor on stretched LCD\n");
		cPtr->Flags &= ~ChipsHWCursor;
	    }
	}
    }

    if (xf86ReturnOptValBool(cPtr->Options, OPTION_LCD_CENTER, TRUE)) {
	ChipsNew->FR[0x40] |= 0x3;    /* Enable Horizontal centering */
	ChipsNew->FR[0x48] |= 0x3;    /* Enable Vertical centering */
    } else {
	ChipsNew->FR[0x40] &= 0xFD;    /* Disable Horizontal centering */
	ChipsNew->FR[0x48] &= 0xFD;    /* Disable Vertical centering */
    }

    /* sync on green */
    if (xf86ReturnOptValBool(cPtr->Options, OPTION_SYNC_ON_GREEN, FALSE))
	ChipsNew->XR[0x82] |=0x02;

    /* software mode flag */
    ChipsNew->XR[0xE2] = chipsVideoMode((pScrn->depth), (cPtr->PanelType & ChipsLCD) ?
	min(mode->CrtcHDisplay, cPtr->PanelSize.HDisplay) :
	mode->CrtcHDisplay, mode->CrtcVDisplay);
#ifdef DEBUG
    ErrorF("VESA Mode: %Xh\n", ChipsNew->XR[0xE2]);
#endif

    /* sync. polarities */
    if ((mode->Flags & (V_PHSYNC | V_NHSYNC))
	&& (mode->Flags & (V_PVSYNC | V_NVSYNC))) {
	if (mode->Flags & (V_PHSYNC | V_NHSYNC)) {
	    if (mode->Flags & V_PHSYNC)
		ChipsNew->FR[0x08] &= 0xBF;	/* Alt. CRT Hsync positive */
	    else
		ChipsNew->FR[0x08] |= 0x40;	/* Alt. CRT Hsync negative */
	}
	if (mode->Flags & (V_PVSYNC | V_NVSYNC)) {
	    if (mode->Flags & V_PVSYNC)
		ChipsNew->FR[0x08] &= 0x7F;	/* Alt. CRT Vsync positive */
	    else
		ChipsNew->FR[0x08] |= 0x80;	/* Alt. CRT Vsync negative */
	}
    }
    if (mode->Flags & (V_PCSYNC | V_NCSYNC)) {
	ChipsNew->FR[0x0B] |= 0x20;
	if (mode->Flags & V_PCSYNC) {
	    ChipsNew->FR[0x08] &= 0x7F;	/* Alt. CRT Vsync positive */
	    ChipsNew->FR[0x08] &= 0xBF;	/* Alt. CRT Hsync positive */
	    ChipsStd->MiscOutReg &= 0x7F;
	    ChipsStd->MiscOutReg &= 0xBF;
	} else {
	    ChipsNew->FR[0x08] |= 0x80;	/* Alt. CRT Vsync negative */
	    ChipsNew->FR[0x08] |= 0x40;	/* Alt. CRT Hsync negative */
	    ChipsStd->MiscOutReg |= 0x40;
	    ChipsStd->MiscOutReg |= 0x80;
    	}
    }	    
    /* bpp depend */
    if (pScrn->bitsPerPixel == 16) {
	ChipsNew->XR[0x81] = (ChipsNew->XR[0x81] & 0xF0) | 0x4;
	if (cPtr->Flags & ChipsGammaSupport)
	    ChipsNew->XR[0x82] |= 0x0C;
	/* 16bpp = 5-5-5             */
	ChipsNew->FR[0x10] |= 0x0C;   /*Colour Panel               */
	ChipsNew->XR[0x20] = 0x10;    /*BitBLT Draw Mode for 16 bpp */
	if (pScrn->weight.green != 5)
	    ChipsNew->XR[0x81] |= 0x01;	/*16bpp */
    } else if (pScrn->bitsPerPixel == 24) {
	ChipsNew->XR[0x81] = (ChipsNew->XR[0x81] & 0xF0) | 0x6;
	if (cPtr->Flags & ChipsGammaSupport)
	    ChipsNew->XR[0x82] |= 0x0C;
	/* 24bpp colour              */
	ChipsNew->XR[0x20] = 0x20;    /*BitBLT Draw Mode for 24 bpp */
    } else if (pScrn->bitsPerPixel == 32) {
	ChipsNew->XR[0x81] = (ChipsNew->XR[0x81] & 0xF0) | 0x7;
	if (cPtr->Flags & ChipsGammaSupport)
	    ChipsNew->XR[0x82] |= 0x0C;
	/* 32bpp colour              */
	ChipsNew->XR[0x20] = 0x10;    /*BitBLT Mode for 16bpp used at 32bpp */
    }
    
    /*CRT only */
    if (!(cPtr->PanelType & ChipsLCD)) {
	if (mode->Flags & V_INTERLACE) {
	    ChipsNew->CR[0x70] = 0x80          /*   set interlace */
	      | (((((mode->CrtcHDisplay >> 3) - 1) >> 1) - 6) & 0x7F);
	    /* 
	     ** Double VDisplay to get back the full screen value, otherwise
	     ** you only see half the picture.
	     */
	    mode->CrtcVDisplay = mode->VDisplay;
	    tmp = ChipsStd->CRTC[7] & ~0x42;
	    ChipsStd->CRTC[7] = (tmp | 
				((((mode->CrtcVDisplay -1) & 0x100) >> 7 ) |
				 (((mode->CrtcVDisplay -1) & 0x200) >> 3 )));
	    ChipsStd->CRTC[0x12] = (mode->CrtcVDisplay -1) & 0xFF;
	    ChipsNew->CR[0x31] = ((mode->CrtcVDisplay - 1) & 0xF00) >> 8;
	} else {
	    ChipsNew->CR[0x70] &= ~0x80;	/* unset interlace */
	}
    }
    
#if defined(__arm__) && defined(__NetBSD__)
    if (cPtr->TVMode != XMODE_RGB) {
	/*
	 * Put the console into TV Out mode.
	 */
	xf86SetTVOut(cPtr->TVMode);
	
	ChipsNew->CR[0x72] = (mode->CrtcHTotal >> 1) >> 3;/* First horizontal
							   * serration pulse */
	ChipsNew->CR[0x73] = mode->CrtcHTotal >> 3; /* Second pulse */
	ChipsNew->CR[0x74] = (((mode->HSyncEnd - mode->HSyncStart) >> 3) - 1)
					& 0x1F; /* equalization pulse */
	
	if (cPtr->TVMode == XMODE_PAL || cPtr->TVMode == XMODE_SECAM) {
	    ChipsNew->CR[0x71] = 0xA0; /* PAL support with blanking delay */
	} else {
	    ChipsNew->CR[0x71] = 0x20; /* NTSC support with blanking delay */
	}
    } else {	/* XMODE_RGB */
	/*
	 * Put the console into RGB Out mode.
	 */
	xf86SetRGBOut();
    }
#endif

    /* STN specific */
    if (IS_STN(cPtr->PanelType)) {
	ChipsNew->FR[0x11] &= ~0x03;	/* FRC clear                    */
	ChipsNew->FR[0x11] &= ~0x8C;	/* Dither clear                 */
	ChipsNew->FR[0x11] |= 0x01;	/* 16 frame FRC                 */
	ChipsNew->FR[0x11] |= 0x84;	/* Dither                       */
	if ((cPtr->Flags & ChipsTMEDSupport) &&
		!xf86ReturnOptValBool(cPtr->Options, OPTION_NO_TMED, FALSE)) {
	    ChipsNew->FR[0x73] &= 0x4F; /* Clear TMED                   */
	    ChipsNew->FR[0x73] |= 0x80; /* Enable TMED                  */
	    ChipsNew->FR[0x73] |= 0x30; /* TMED 256 Shades of RGB       */
	}
	if (cPtr->PanelType & ChipsDD)	/* Shift Clock Mask. Use to get */
	    ChipsNew->FR[0x12] |= 0x4;	/* rid of line in DSTN screens  */
    }

    /*
     * The zero position of the overlay does not align with the zero
     * position of the display. The skew is dependent on the depth,
     * display type and refresh rate. Calculate the skew before setting
     * the X and Y dimensions of the overlay. These values are needed
     * both by the overlay and XvImages. So calculate and store them
     */
    if (cPtr->PanelType & ChipsLCD) {
	cPtr->OverlaySkewX = (((ChipsNew->FR[0x23] & 0xFF) 
			    - (ChipsNew->FR[0x20] & 0xFF) + 3) << 3) 
	    - 1;
	cPtr->OverlaySkewY = (ChipsNew->FR[0x33]
			    + ((ChipsNew->FR[0x36] & 0xF) << 8)
			    - (ChipsNew->FR[0x31] & 0xF0)
			    - (ChipsNew->FR[0x32] & 0x0F)
			    - ((ChipsNew->FR[0x35] & 0xF0) << 4));
	if (!xf86ReturnOptValBool(cPtr->Options, OPTION_LCD_STRETCH, FALSE)
	      && xf86ReturnOptValBool(cPtr->Options, OPTION_LCD_CENTER, TRUE))
	{
	    if (cPtr->PanelSize.HDisplay > mode->CrtcHDisplay) 
		cPtr->OverlaySkewX += (cPtr->PanelSize.HDisplay - 
				       mode->CrtcHDisplay) / 2;
	    if (cPtr->PanelSize.VDisplay > mode->CrtcVDisplay)
		cPtr->OverlaySkewY += (cPtr->PanelSize.VDisplay -
				       mode->CrtcVDisplay) / 2;
	}
    } else {
	cPtr->OverlaySkewX = mode->CrtcHTotal - mode->CrtcHBlankStart - 9;
	cPtr->OverlaySkewY = mode->CrtcVTotal - mode->CrtcVSyncEnd - 1;
	    
	if (mode->Flags & V_INTERLACE) {
	    /*
	     * This handles 1024 and 1280 interlaced modes only. Its 
	     * pretty arbitrary, but its what C&T recommends
	     */
#if 0
	    if (mode->CrtcHDisplay == 1024)
		cPtr->OverlaySkewY += 5;
	    else  if (mode->CrtcHDisplay == 1280)
#endif
		cPtr->OverlaySkewY *= 2;
	    
	}
    }

    /* mask for viewport granularity */

    switch (pScrn->bitsPerPixel) {
    case 8:
	cPtr->viewportMask = ~7U;
	break;
    case 16:
	cPtr->viewportMask = ~3U;
	break;
    case 24:
	cPtr->viewportMask = ~7U;
	break;
    case 32:
	cPtr->viewportMask = ~0U;
	break;
    default:
	cPtr->viewportMask = ~7U;
    }
    
    /* Turn off multimedia by default as it degrades performance */
    ChipsNew->XR[0xD0] &= 0x0f;	 
    
    if (cPtr->Flags & ChipsVideoSupport) {
#if 0   /* if we do this even though video isn't playing we kill performance */
	ChipsNew->XR[0xD0] |= 0x10;	/* Force the Multimedia engine on */
#endif
#ifdef SAR04
	ChipsNew->XR[0x4F] = 0x2A;	/* SAR04 >352 pixel overlay width */
#endif
	ChipsNew->MR[0x3C] &= 0x18;	/* Ensure that the overlay is off */
	cPtr->VideoZoomMax = 0x100;

	if (cPtr->Chipset == CHIPS_CT65550) {
	    tmp = cPtr->readXR(cPtr, 0x04);
	    if (tmp < 0x02)				/* 65550 ES0 has */ 
		cPtr->VideoZoomMax = 0x40;		/* 0x40 max zoom */
	}
    }

    /* Program the registers */
    /*vgaHWProtect(pScrn, TRUE);*/

    if (cPtr->Chipset <= CHIPS_CT69000) {
        ChipsNew->FR[0x01] &= ~0x03;
	if (cPtr->PanelType & ChipsLCD) 
	    ChipsNew->FR[0x01] |= 0x02;
	else
	    ChipsNew->FR[0x01] |= 0x01;
    }
    if ((cPtr->Flags & ChipsDualChannelSupport) &&
	(!xf86IsEntityShared(pScrn->entityList[0]))) {
	unsigned char IOSS, MSS, tmpfr01;


	IOSS = cPtr->readIOSS(cPtr);
	MSS = cPtr->readMSS(cPtr);
	cPtr->writeIOSS(cPtr, ((cPtr->storeIOSS & IOSS_MASK) |
			       IOSS_PIPE_A));
	cPtr->writeMSS(cPtr, hwp, ((cPtr->storeMSS & MSS_MASK) |
			      MSS_PIPE_A));
	chipsRestore(pScrn, ChipsStd, ChipsNew, FALSE);
	cPtr->writeIOSS(cPtr, ((cPtr->storeIOSS & IOSS_MASK) |
			       IOSS_PIPE_B));
	cPtr->writeMSS(cPtr, hwp, ((cPtr->storeMSS & MSS_MASK) |
			      MSS_PIPE_B));
	/* 
	 * Hack:: Force Pipe-B on for dual refresh, and off elsewise
	 */
	tmpfr01 = ChipsNew->FR[0x01];
	ChipsNew->FR[0x01] &= 0xFC;
	if (cPtr->UseDualChannel)
	    ChipsNew->FR[0x01] |= 0x01;
	chipsRestore(pScrn, ChipsStd, ChipsNew, FALSE);
	ChipsNew->FR[0x01] = tmpfr01;
	cPtr->writeIOSS(cPtr, IOSS);
	cPtr->writeMSS(cPtr, hwp, MSS);
    } else {
	chipsRestore(pScrn, ChipsStd, ChipsNew, FALSE);
    }

    /*vgaHWProtect(pScrn, FALSE);*/
    usleep(100000);  /* prevents cursor corruption seen on a TECRA 510 */
    
    return(TRUE);
}

static Bool
chipsModeInitWingine(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    int i, bytesPerPixel;
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSRegPtr ChipsNew;
    vgaRegPtr ChipsStd;
    unsigned int tmp;

    ChipsNew = &cPtr->ModeReg;
    ChipsStd = &hwp->ModeReg;

    bytesPerPixel = pScrn->bitsPerPixel >> 3;

    /* 
     * This chipset seems to have problems if 
     * HBlankEnd is chosen equals HTotal
     */
    if (!mode->CrtcHAdjusted)
      mode->CrtcHBlankEnd = min(mode->CrtcHSyncEnd, mode->CrtcHTotal - 2);

    /* correct the timings for 16/24 bpp */
    if (pScrn->bitsPerPixel == 16) {
	if (!mode->CrtcHAdjusted) {
	    mode->CrtcHDisplay++;
	    mode->CrtcHDisplay <<= 1;
	    mode->CrtcHDisplay--;
	    mode->CrtcHSyncStart <<= 1;
	    mode->CrtcHSyncEnd <<= 1;
	    mode->CrtcHBlankStart <<= 1;
	    mode->CrtcHBlankEnd <<= 1;
	    mode->CrtcHTotal <<= 1;
	    mode->CrtcHAdjusted = TRUE;
	}
    } else if (pScrn->bitsPerPixel == 24) {
	if (!mode->CrtcHAdjusted) {
	    mode->CrtcHDisplay++;
	    mode->CrtcHDisplay += ((mode->CrtcHDisplay) << 1);
	    mode->CrtcHDisplay--;
	    mode->CrtcHSyncStart += ((mode->CrtcHSyncStart) << 1);
	    mode->CrtcHSyncEnd += ((mode->CrtcHSyncEnd) << 1);
	    mode->CrtcHBlankStart += ((mode->CrtcHBlankStart) << 1);
	    mode->CrtcHBlankEnd += ((mode->CrtcHBlankEnd) << 1);
	    mode->CrtcHTotal += ((mode->CrtcHTotal) << 1);
	    mode->CrtcHAdjusted = TRUE;
	}
    }

    /* generic init */
    if (!vgaHWInit(pScrn, mode)) {
	ErrorF("bomb 3\n");
	return (FALSE);
    }
    pScrn->vtSema = TRUE;
    
    /* init clock */
    if (!chipsClockFind(pScrn, mode, mode->ClockIndex, &ChipsNew->Clock)) {
	ErrorF("bomb 4\n");
	return (FALSE);
    }

    /* get  C&T Specific Registers */
    for (i = 0; i < 0x7D; i++) {   /* don't touch XR7D and XR7F on WINGINE */
	ChipsNew->XR[i] = cPtr->readXR(cPtr, i);
    }

    /* some generic settings */
    if (pScrn->bitsPerPixel == 1) {
	ChipsStd->Attribute[0x10] = 0x03;   /* mode */
    } else {
	ChipsStd->Attribute[0x10] = 0x01;   /* mode */
    }
    ChipsStd->Attribute[0x11] = 0x00;   /* overscan (border) color */
    ChipsStd->Attribute[0x12] = 0x0F;   /* enable all color planes */
    ChipsStd->Attribute[0x13] = 0x00;   /* horiz pixel panning 0 */

    ChipsStd->Graphics[0x05] = 0x00;    /* normal read/write mode */


    /* set virtual screen width */
    if (pScrn->bitsPerPixel >= 8)
	ChipsStd->CRTC[0x13] = (pScrn->displayWidth * bytesPerPixel) >> 3;
    else
	ChipsStd->CRTC[0x13] = pScrn->displayWidth >> 4;

    
    /* set C&T Specific Registers */
    /* set virtual screen width */
    if (pScrn->bitsPerPixel >= 8)
	tmp = (pScrn->displayWidth >> 4) * bytesPerPixel;
    else
	tmp = (pScrn->displayWidth >> 5);
    ChipsNew->XR[0x0D] = (tmp & 0x80) >> 5; 

    ChipsNew->XR[0x04] |= 4;	       /* enable addr counter bits 16-17 */
    /* XR04: Memory control 1 */
    /* bit 2: Memory Wraparound */
    /*        Enable CRTC addr counter bits 16-17 if set */

    ChipsNew->XR[0x0B] |= 0x07;       /* extended mode, dual pages enabled */
    ChipsNew->XR[0x0B] &= ~0x10;      /* linear mode off */
    /* XR0B: CPU paging */
    /* bit 0: Memory mapping mode */
    /*        VGA compatible if 0 (default) */
    /*        Extended mode (mapping for > 256 kB mem) if 1 */
    /* bit 1: CPU single/dual mapping */
    /*        0, CPU uses only a single map to access (default) */
    /*        1, CPU uses two maps to access */
    /* bit 2: CPU address divide by 4 */

    ChipsNew->XR[0x10] = 0;	       /* XR10: Single/low map */
    ChipsNew->XR[0x11] = 0;	       /* XR11: High map      */
    ChipsNew->XR[0x0C] &= ~0x50;       /* MSB for XR10 & XR11  */ 
    if (pScrn->bitsPerPixel >= 8) {
	ChipsNew->XR[0x28] |= 0x10;       /* 256-color video     */
    } else {
	ChipsNew->XR[0x28] &= 0xEF;       /* 16-color video      */
    }
    /* set up extended display timings */
    /* in CRTonly mode this is simple: only set overflow for CR00-CR06 */
    ChipsNew->XR[0x17] = ((((mode->CrtcHTotal >> 3) - 5) & 0x100) >> 8)
	| ((((mode->CrtcHDisplay >> 3) - 1) & 0x100) >> 7)
	| ((((mode->CrtcHSyncStart >> 3) - 1) & 0x100) >> 6)
	| ((((mode->CrtcHSyncEnd >> 3)) & 0x20) >> 2)
	| ((((mode->CrtcHBlankStart >> 3) - 1) & 0x100) >> 4)
	| ((((mode->CrtcHBlankEnd >> 3) - 1) & 0x40) >> 1);


    ChipsNew->XR[0x16]  = (((mode->CrtcVTotal -2) & 0x400) >> 10 )
	| (((mode->CrtcVDisplay -1) & 0x400) >> 9 )
	| ((mode->CrtcVSyncStart & 0x400) >> 8 )
	| (((mode->CrtcVBlankStart) & 0x400) >> 6 );

    /* set video mode */
    ChipsNew->XR[0x2B] = chipsVideoMode(pScrn->depth, mode->CrtcHDisplay, mode->CrtcVDisplay);
#ifdef DEBUG
    ErrorF("VESA Mode: %Xh\n", ChipsNew->XR[0x2B]);
#endif

    /* set some linear specific registers */
    if (cPtr->Flags & ChipsLinearSupport) {
	/* enable linear addressing  */
	ChipsNew->XR[0x0B] &= 0xFD;   /* dual page clear                */
	ChipsNew->XR[0x0B] |= 0x10;   /* linear mode on                 */

	ChipsNew->XR[0x08] = 
	  (unsigned char)((cPtr->FbAddress >> 16) & 0xFF);    
	ChipsNew->XR[0x09] = 
	  (unsigned char)((cPtr->FbAddress >> 24) & 0xFF);    

	/* general setup */
	ChipsNew->XR[0x40] = 0x01;    /*BitBLT Draw Mode for 8 and 24 bpp */
    }

    /* common general setup */
    ChipsNew->XR[0x52] |= 0x01;       /* Refresh count                   */
    ChipsNew->XR[0x0F] &= 0xEF;       /* not Hi-/True-Colour             */
    ChipsNew->XR[0x02] &= 0xE7;       /* Attr. Cont. default access      */
                                       /* use ext. regs. for hor. in dual */
    ChipsNew->XR[0x06] &= 0xF3;       /* bpp clear                       */

    /* bpp depend */
    /*XR06: Palette control */
    /* bit 0: Pixel Data Pin Diag, 0 for flat panel pix. data (def)  */
    /* bit 1: Internal DAC disable                                   */
    /* bit 3-2: Colour depth, 0 for 4 or 8 bpp, 1 for 16(5-5-5) bpp, */
    /*          2 for 24 bpp, 3 for 16(5-6-5)bpp                     */
    /* bit 4:   Enable PC Video Overlay on colour key                */
    /* bit 5:   Bypass Internal VGA palette                          */
    /* bit 7-6: Colour reduction select, 0 for NTSC (default),       */
    /*          1 for Equivalent weighting, 2 for green only,        */
    /*          3 for Colour w/o reduction                           */
    /* XR50 Panel Format Register 1                                  */
    /* bit 1-0: Frame Rate Control; 00, No FRC;                      */
    /*          01, 16-frame FRC for colour STN and monochrome       */
    /*          10, 2-frame FRC for colour TFT or monochrome;        */
    /*          11, reserved                                         */
    /* bit 3-2: Dither Enable                                        */
    /*          00, disable dithering; 01, enable dithering          */
    /*          for 256 mode                                         */
    /*          10, enable dithering for all modes; 11, reserved     */
    /* bit6-4: Clock Divide (CD)                                     */
    /*          000, Shift Clock Freq = Dot Clock Freq;              */
    /*          001, SClk = DClk/2; 010 SClk = DClk/4;               */
    /*          011, SClk = DClk/8; 100 SClk = DClk/16;              */
    /* bit 7: TFT data width                                         */
    /*          0, 16 bit(565RGB); 1, 24bit (888RGB)                 */
    if (pScrn->bitsPerPixel == 16) {
	ChipsNew->XR[0x06] |= 0xC4;   /*15 or 16 bpp colour         */
	ChipsNew->XR[0x0F] |= 0x10;   /*Hi-/True-Colour             */
	ChipsNew->XR[0x40] = 0x02;    /*BitBLT Draw Mode for 16 bpp */
	if (pScrn->weight.green != 5)
	    ChipsNew->XR[0x06] |= 0x08;	/*16bpp              */
    } else if (pScrn->bitsPerPixel == 24) {
	ChipsNew->XR[0x06] |= 0xC8;   /*24 bpp colour               */
	ChipsNew->XR[0x0F] |= 0x10;   /*Hi-/True-Colour             */
    }

    /*CRT only: interlaced mode */
    if (mode->Flags & V_INTERLACE) {
	ChipsNew->XR[0x28] |= 0x20;    /* set interlace         */
	/* empirical value       */
	tmp = ((((mode->CrtcHDisplay >> 3) - 1) >> 1) 
	       - 6 * (pScrn->bitsPerPixel >= 8 ? bytesPerPixel : 1 ));
	ChipsNew->XR[0x19] = tmp & 0xFF;
	ChipsNew->XR[0x17] |= ((tmp & 0x100) >> 1); /* overflow */
	ChipsNew->XR[0x0F] &= ~0x40;   /* set SW-Flag           */
    } else {
	ChipsNew->XR[0x28] &= ~0x20;   /* unset interlace       */
	ChipsNew->XR[0x0F] |=  0x40;   /* set SW-Flag           */
    }

    /* Program the registers */
    /*vgaHWProtect(pScrn, TRUE);*/
    chipsRestore(pScrn, ChipsStd, ChipsNew, FALSE);
    /*vgaHWProtect(pScrn, FALSE);*/

    return (TRUE);
}

static Bool
chipsModeInit655xx(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    int i, bytesPerPixel;
    int lcdHTotal, lcdHDisplay;
    int lcdVTotal, lcdVDisplay;
    int lcdHRetraceStart, lcdHRetraceEnd;
    int lcdVRetraceStart, lcdVRetraceEnd;
    int HSyncStart, HDisplay;
    int CrtcHDisplay;
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSRegPtr ChipsNew;
    vgaRegPtr ChipsStd;
    unsigned int tmp;

    ChipsNew = &cPtr->ModeReg;
    ChipsStd = &hwp->ModeReg;

    bytesPerPixel = pScrn->bitsPerPixel >> 3;

    /*
     * Possibly fix up the panel size, if the manufacture is stupid
     * enough to set it incorrectly in text modes
     */
    if (xf86ReturnOptValBool(cPtr->Options, OPTION_PANEL_SIZE, FALSE)) {
	cPtr->PanelSize.HDisplay = mode->CrtcHDisplay;
	cPtr->PanelSize.VDisplay = mode->CrtcVDisplay;
    }
    
    /* 
     * This chipset seems to have problems if 
     * HBlankEnd is chosen equals HTotal
     */
    if (!mode->CrtcHAdjusted)
      mode->CrtcHBlankEnd = min(mode->CrtcHSyncEnd, mode->CrtcHTotal - 2);

    /* correct the timings for 16/24 bpp */
    if (pScrn->bitsPerPixel == 16) {
	if (!mode->CrtcHAdjusted) {
	    mode->CrtcHDisplay++;
	    mode->CrtcHDisplay <<= 1;
	    mode->CrtcHDisplay--;
	    mode->CrtcHSyncStart <<= 1;
	    mode->CrtcHSyncEnd <<= 1;
	    mode->CrtcHBlankStart <<= 1;
	    mode->CrtcHBlankEnd <<= 1;
	    mode->CrtcHTotal <<= 1;
	    mode->CrtcHAdjusted = TRUE;
	}
    } else if (pScrn->bitsPerPixel == 24) {
	if (!mode->CrtcHAdjusted) {
	    mode->CrtcHDisplay++;
	    mode->CrtcHDisplay += ((mode->CrtcHDisplay) << 1);
	    mode->CrtcHDisplay--;
	    mode->CrtcHSyncStart += ((mode->CrtcHSyncStart) << 1);
	    mode->CrtcHSyncEnd += ((mode->CrtcHSyncEnd) << 1);
	    mode->CrtcHBlankStart += ((mode->CrtcHBlankStart) << 1);
	    mode->CrtcHBlankEnd += ((mode->CrtcHBlankEnd) << 1);
	    mode->CrtcHTotal += ((mode->CrtcHTotal) << 1);
	    mode->CrtcHAdjusted = TRUE;
	}
    }
	   
    /* store orig. HSyncStart needed for flat panel mode */
    HSyncStart = mode->CrtcHSyncStart / (pScrn->bitsPerPixel >= 8 ? 
					 bytesPerPixel : 1 ) - 16;
    HDisplay = (mode->CrtcHDisplay + 1) /  (pScrn->bitsPerPixel >= 8 ? 
					 bytesPerPixel : 1 );
    
    /* generic init */
    if (!vgaHWInit(pScrn, mode)) {
	ErrorF("bomb 5\n");
	return (FALSE);
    }
    pScrn->vtSema = TRUE;
    
    /* init clock */
    if (!chipsClockFind(pScrn, mode, mode->ClockIndex, &ChipsNew->Clock)) {
	ErrorF("bomb 6\n");
	return (FALSE);
    }

    /* get  C&T Specific Registers */
    for (i = 0; i < 0x80; i++) {
	ChipsNew->XR[i] = cPtr->readXR(cPtr, i);
    }

    /* some generic settings */
    if (pScrn->bitsPerPixel == 1) {
	ChipsStd->Attribute[0x10] = 0x03;   /* mode */
    } else {
	ChipsStd->Attribute[0x10] = 0x01;   /* mode */
    }
    ChipsStd->Attribute[0x11] = 0x00;   /* overscan (border) color */
    ChipsStd->Attribute[0x12] = 0x0F;   /* enable all color planes */
    ChipsStd->Attribute[0x13] = 0x00;   /* horiz pixel panning 0 */

    ChipsStd->Graphics[0x05] = 0x00;    /* normal read/write mode */

    /* set virtual screen width */
    if (pScrn->bitsPerPixel >= 8)
	ChipsStd->CRTC[0x13] = (pScrn->displayWidth * bytesPerPixel) >> 3;
    else
	ChipsStd->CRTC[0x13] = pScrn->displayWidth >> 4;

    
    /* set C&T Specific Registers */
    /* set virtual screen width */
    ChipsNew->XR[0x1E] = ChipsStd->CRTC[0x13];	/* alternate offset */
    /*databook is not clear about 0x1E might be needed for 65520/30 */
    if (pScrn->bitsPerPixel >= 8)
	tmp = (pScrn->displayWidth * bytesPerPixel) >> 2;
    else
	tmp = pScrn->displayWidth >> 3;
    ChipsNew->XR[0x0D] = (tmp & 0x01) | ((tmp << 1) & 0x02)  ; 

    ChipsNew->XR[0x04] |= 4;	       /* enable addr counter bits 16-17 */
    /* XR04: Memory control 1 */
    /* bit 2: Memory Wraparound */
    /*        Enable CRTC addr counter bits 16-17 if set */

    ChipsNew->XR[0x0B] |= 0x07;       /* extended mode, dual pages enabled */
    ChipsNew->XR[0x0B] &= ~0x10;      /* linear mode off */
    /* XR0B: CPU paging */
    /* bit 0: Memory mapping mode */
    /*        VGA compatible if 0 (default) */
    /*        Extended mode (mapping for > 256 kB mem) if 1 */
    /* bit 1: CPU single/dual mapping */
    /*        0, CPU uses only a single map to access (default) */
    /*        1, CPU uses two maps to access */
    /* bit 2: CPU address divide by 4 */

    ChipsNew->XR[0x10] = 0;	       /* XR10: Single/low map */
    ChipsNew->XR[0x11] = 0;	       /* XR11: High map      */
    if (pScrn->bitsPerPixel >= 8) {
	ChipsNew->XR[0x28] |= 0x10;       /* 256-color video     */
    } else {
	ChipsNew->XR[0x28] &= 0xEF;       /* 16-color video      */
    }
    /* set up extended display timings */
    if (!(cPtr->PanelType & ChipsLCD)) {
	/* in CRTonly mode this is simple: only set overflow for CR00-CR06 */
	ChipsNew->XR[0x17] = ((((mode->CrtcHTotal >> 3) - 5) & 0x100) >> 8)
	    | ((((mode->CrtcHDisplay >> 3) - 1) & 0x100) >> 7)
	    | ((((mode->CrtcHSyncStart >> 3) - 1) & 0x100) >> 6)
	    | ((((mode->CrtcHSyncEnd >> 3)) & 0x20) >> 2)
	    | ((((mode->CrtcHBlankStart >> 3) - 1) & 0x100) >> 4)
	    | ((((mode->CrtcHBlankEnd >> 3) - 1) & 0x40) >> 1);

	ChipsNew->XR[0x16]  = (((mode->CrtcVTotal -2) & 0x400) >> 10 )
	    | (((mode->CrtcVDisplay -1) & 0x400) >> 9 )
	    | ((mode->CrtcVSyncStart & 0x400) >> 8 )
	    | (((mode->CrtcVBlankStart) & 0x400) >> 6 );
    } else {
	/* horizontal timing registers */
	/* in LCD/dual mode use saved bios values to derive timing values if
	 * not told otherwise */
	if (!xf86ReturnOptValBool(cPtr->Options, OPTION_USE_MODELINE, FALSE)) {
	    lcdHTotal = cPtr->PanelSize.HTotal;
	    lcdHRetraceStart = cPtr->PanelSize.HRetraceStart;
	    lcdHRetraceEnd = cPtr->PanelSize.HRetraceEnd;
	    if (pScrn->bitsPerPixel == 16) {
		lcdHRetraceStart <<= 1;
		lcdHRetraceEnd <<= 1;
		lcdHTotal <<= 1;
	    } else if (pScrn->bitsPerPixel == 24) {
		lcdHRetraceStart += (lcdHRetraceStart << 1);
		lcdHRetraceEnd += (lcdHRetraceEnd << 1);
		lcdHTotal += (lcdHTotal << 1);
	    }
 	    lcdHRetraceStart -=8;       /* HBlank =  HRetrace - 1: for */
 	    lcdHRetraceEnd   -=8;       /* compatibility with vgaHW.c  */
	} else {
	    /* use modeline values if bios values don't work */
	    lcdHTotal = mode->CrtcHTotal;
	    lcdHRetraceStart = mode->CrtcHSyncStart;
	    lcdHRetraceEnd = mode->CrtcHSyncEnd;
	}
	/* The chip takes the size of the visible display area from the
	 * CRTC values. We use bios screensize for LCD in LCD/dual mode
	 * wether or not we use modeline for LCD. This way we can specify
	 * always specify a smaller than default display size on LCD
	 * by writing it to the CRTC registers. */
	lcdHDisplay = cPtr->PanelSize.HDisplay;
	if (pScrn->bitsPerPixel == 16) {
	    lcdHDisplay++;
	    lcdHDisplay <<= 1;
	    lcdHDisplay--;
	} else if (pScrn->bitsPerPixel == 24) {
	    lcdHDisplay++;
	    lcdHDisplay += (lcdHDisplay << 1);
	    lcdHDisplay--;
	}
	lcdHTotal = (lcdHTotal >> 3) - 5;
	lcdHDisplay = (lcdHDisplay >> 3) - 1;
	lcdHRetraceStart = (lcdHRetraceStart >> 3);
	lcdHRetraceEnd = (lcdHRetraceEnd >> 3);
	/* This ugly hack is needed because CR01 and XR1C share the 8th bit!*/
	CrtcHDisplay = ((mode->CrtcHDisplay >> 3) - 1);
	if ((lcdHDisplay & 0x100) != (CrtcHDisplay & 0x100)) {
	    xf86ErrorF("This display configuration might cause problems !\n");
	    lcdHDisplay = 255;
	}

	/* now init register values */
	ChipsNew->XR[0x17] = (((lcdHTotal) & 0x100) >> 8)
	    | ((lcdHDisplay & 0x100) >> 7)
	    | ((lcdHRetraceStart & 0x100) >> 6)
	    | (((lcdHRetraceEnd) & 0x20) >> 2);

	ChipsNew->XR[0x19] = lcdHRetraceStart & 0xFF;
	ChipsNew->XR[0x1A] = lcdHRetraceEnd & 0x1F;

	/* XR1B: Alternate horizontal total */
	/* used in all flat panel mode with horiz. compression disabled, */
	/* CRT CGA text and graphic modes and Hercules graphics mode */
	/* similar to CR00, actual value - 5 */
	ChipsNew->XR[0x1B] = lcdHTotal & 0xFF;

	/*XR1C: Alternate horizontal blank start (CRT mode) */
	/*      /horizontal panel size (FP mode) */
	/* FP horizontal panel size (FP mode), */
	/* actual value - 1 (in characters unit) */
	/* CRT horizontal blank start (CRT mode) */
	/* similar to CR02, actual value - 1 */
	ChipsNew->XR[0x1C] = lcdHDisplay & 0xFF;

	if (xf86ReturnOptValBool(cPtr->Options, OPTION_USE_MODELINE, FALSE)) {
	    /* for ext. packed pixel mode on 64520/64530 */
	    /* no need to rescale: used only in 65530    */
	    ChipsNew->XR[0x21] = lcdHRetraceStart & 0xFF;
	    ChipsNew->XR[0x22] = lcdHRetraceEnd & 0x1F;
	    ChipsNew->XR[0x23] = lcdHTotal & 0xFF;

	    /* vertical timing registers */
	    lcdVTotal = mode->CrtcVTotal - 2;
	    lcdVDisplay = cPtr->PanelSize.VDisplay - 1;
	    lcdVRetraceStart = mode->CrtcVSyncStart;
	    lcdVRetraceEnd = mode->CrtcVSyncEnd;

	    ChipsNew->XR[0x64] = lcdVTotal & 0xFF;
	    ChipsNew->XR[0x66] = lcdVRetraceStart & 0xFF;
	    ChipsNew->XR[0x67] = lcdVRetraceEnd & 0x0F;
	    ChipsNew->XR[0x68] = lcdVDisplay & 0xFF;
	    ChipsNew->XR[0x65] = ((lcdVTotal & 0x100) >> 8)
		| ((lcdVDisplay & 0x100) >> 7)
		| ((lcdVRetraceStart & 0x100) >> 6)
		| ((lcdVRetraceStart & 0x400) >> 7)
		| ((lcdVTotal & 0x400) >> 6)
		| ((lcdVTotal & 0x200) >> 4)
		| ((lcdVDisplay & 0x200) >> 3)
		| ((lcdVRetraceStart & 0x200) >> 2);

	    /* 
	     * These are important: 0x2C specifies the numbers of lines 
	     * (hsync pulses) between vertical blank start and vertical 
	     * line total, 0x2D specifies the number of clock ticks? to
	     * horiz. blank start ( caution ! 16bpp/24bpp modes: that's
	     * why we need HSyncStart - can't use mode->CrtcHSyncStart) 
	     */
	    tmp = ((cPtr->PanelType & ChipsDD) && !(ChipsNew->XR[0x6F] & 0x02))
	      ? 1 : 0; /* double LP delay, FLM: 2 lines iff DD+no acc*/
	    /* Currently we support 2 FLM schemes: #1: FLM coincides with
	     * VTotal ie. the delay is programmed to the difference bet-
	     * ween lctVTotal and lcdVRetraceStart.    #2: FLM coincides
	     * lcdVRetraceStart - in this case FLM delay will be turned
	     * off. To decide which scheme to use we compare the value of
	     * XR2C set by the bios to the two schemes. The one that fits
	     * better will be used.
	     */

	    if (ChipsNew->XR[0x2C]  < abs((cPtr->PanelSize.VTotal -
		    cPtr->PanelSize.VRetraceStart - tmp - 1) -
		    ChipsNew->XR[0x2C]))
	        ChipsNew->XR[0x2F] |= 0x80;   /* turn FLM delay off */
	    ChipsNew->XR[0x2C] = lcdVTotal - lcdVRetraceStart - tmp;
	    /*ChipsNew->XR[0x2D] = (HSyncStart >> (3 - tmp)) & 0xFF;*/
	    ChipsNew->XR[0x2D] = (HDisplay >> (3 - tmp)) & 0xFF;
	    ChipsNew->XR[0x2F] = (ChipsNew->XR[0x2F] & 0xDF)
		| (((HSyncStart >> (3 - tmp)) & 0x100) >> 3);
	}

	/* set stretching/centering */	
	if (!xf86ReturnOptValBool(cPtr->Options, OPTION_SUSPEND_HACK, FALSE)) {
	    ChipsNew->XR[0x51] |= 0x40;   /* enable FP compensation          */
	    ChipsNew->XR[0x55] |= 0x01;   /* enable horiz. compensation      */
	    ChipsNew->XR[0x57] |= 0x01;   /* enable horiz. compensation      */
	    if (!xf86ReturnOptValBool(cPtr->Options, OPTION_LCD_STRETCH,
				     FALSE)) {
		if (mode->CrtcHDisplay < 1489)      /* HWBug                 */
		    ChipsNew->XR[0x55] |= 0x02;	/* enable auto h-centering   */
		else {
		    ChipsNew->XR[0x55] &= 0xFD;	/* disable auto h-centering  */
		    if (pScrn->bitsPerPixel == 24) /* ? */
			ChipsNew->XR[0x56] = (lcdHDisplay - CrtcHDisplay) >> 1;
		}
	    } else {
	      ChipsNew->XR[0x55] &= 0xFD;	/* disable h-centering    */
	      ChipsNew->XR[0x56] = 0;
	    }
	    ChipsNew->XR[0x57] = 0x03;    /* enable v-comp disable v-stretch */
	    if (!xf86ReturnOptValBool(cPtr->Options, OPTION_LCD_STRETCH,
				      FALSE)) {
		ChipsNew->XR[0x55] |= 0x20; /* enable h-comp disable h-double*/
		ChipsNew->XR[0x57] |= 0x60; /* Enable vertical stretching    */
		tmp = (mode->CrtcVDisplay / (cPtr->PanelSize.VDisplay -
		    mode->CrtcVDisplay + 1));
		if (tmp) {
			if (cPtr->PanelSize.HDisplay
			    && cPtr->PanelSize.VDisplay
			    && (cPtr->PanelSize.HDisplay != mode->CrtcHDisplay)
			 && (cPtr->PanelSize.VDisplay != mode->CrtcVDisplay)) {
			    /* Possible H/W bug? */
			    if(cPtr->Accel.UseHWCursor)
				xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
				    "Disabling HW Cursor on stretched LCD\n");
			    cPtr->Flags &= ~ChipsHWCursor;
			}
		    }
		if (cPtr->Flags & ChipsHWCursor)
		    tmp = (tmp == 0 ? 1 : tmp);  /* Bug when doubling */
		ChipsNew->XR[0x5A] = tmp > 0x0F ? 0 : (unsigned char)tmp;
	    } else {
		ChipsNew->XR[0x55] &= 0xDF; /* disable h-comp, h-double */
		ChipsNew->XR[0x57] &= 0x9F; /* disable vertical stretching  */
	    }
	}
    }

    /* set video mode */
    ChipsNew->XR[0x2B] = chipsVideoMode(pScrn->depth, (cPtr->PanelType & ChipsLCD) ?
	min(HDisplay, cPtr->PanelSize.HDisplay) : HDisplay,cPtr->PanelSize.VDisplay);
#ifdef DEBUG
    ErrorF("VESA Mode: %Xh\n", ChipsNew->XR[0x2B]);
#endif

    /* set some linear specific registers */
    if (cPtr->Flags & ChipsLinearSupport) {
	/* enable linear addressing  */
	ChipsNew->XR[0x0B] &= 0xFD;   /* dual page clear                */
	ChipsNew->XR[0x0B] |= 0x10;   /* linear mode on                 */
 	if (cPtr->Chipset == CHIPS_CT65535)
 	    ChipsNew->XR[0x08] = (unsigned char)(cPtr->FbAddress >> 17);
 	else if (cPtr->Chipset > CHIPS_CT65535)
 	    ChipsNew->XR[0x08] = (unsigned char)(cPtr->FbAddress >> 20);
	else {
            /* Its probably set correctly by BIOS anyway. Leave it alone    */
	    /* 65525 - 65530 require XR04[6] set for greater than 512k of   */
            /* ram. We only correct obvious bugs; VL probably uses MEMR/MEMW*/
	    if (cPtr->Bus == ChipsISA)
		ChipsNew->XR[0x04] &= ~0x40;  /* A19 sceme       */
	    if (pScrn->videoRam > 512)
		ChipsNew->XR[0x04] |= 0x40;   /* MEMR/MEMW sceme */
	}

	/* general setup */
	ChipsNew->XR[0x03] |= 0x08;   /* High bandwidth on 65548         */
	ChipsNew->XR[0x40] = 0x01;    /*BitBLT Draw Mode for 8 and 24 bpp */
    }

    /* common general setup */
    ChipsNew->XR[0x52] |= 0x01;       /* Refresh count                   */
    ChipsNew->XR[0x0F] &= 0xEF;       /* not Hi-/True-Colour             */
    ChipsNew->XR[0x02] |= 0x01;       /* 16bit CPU Memory Access         */
    ChipsNew->XR[0x02] &= 0xE3;       /* Attr. Cont. default access      */
                                      /* use ext. regs. for hor. in dual */
    ChipsNew->XR[0x06] &= 0xF3;       /* bpp clear                       */

    /* PCI */
    if (cPtr->Bus == ChipsPCI)
	ChipsNew->XR[0x03] |= 0x40;   /*PCI burst */
    
    /* sync. polarities */
    if ((mode->Flags & (V_PHSYNC | V_NHSYNC))
	&& (mode->Flags & (V_PVSYNC | V_NVSYNC))) {
	if (mode->Flags & (V_PHSYNC | V_NHSYNC)) {
	    if (mode->Flags & V_PHSYNC) {
		ChipsNew->XR[0x55] &= 0xBF;	/* CRT Hsync positive */
	    } else {
		ChipsNew->XR[0x55] |= 0x40;	/* CRT Hsync negative */
	    }
	}
	if (mode->Flags & (V_PVSYNC | V_NVSYNC)) {
	    if (mode->Flags & V_PVSYNC) {
		ChipsNew->XR[0x55] &= 0x7F;	/* CRT Vsync positive */
	    } else {
		ChipsNew->XR[0x55] |= 0x80;	/* CRT Vsync negative */
	    }
	}
    }

    /* bpp depend */
    /*XR06: Palette control */
    /* bit 0: Pixel Data Pin Diag, 0 for flat panel pix. data (def)  */
    /* bit 1: Internal DAC disable                                   */
    /* bit 3-2: Colour depth, 0 for 4 or 8 bpp, 1 for 16(5-5-5) bpp, */
    /*          2 for 24 bpp, 3 for 16(5-6-5)bpp                     */
    /* bit 4:   Enable PC Video Overlay on colour key                */
    /* bit 5:   Bypass Internal VGA palette                          */
    /* bit 7-6: Colour reduction select, 0 for NTSC (default),       */
    /*          1 for Equivalent weighting, 2 for green only,        */
    /*          3 for Colour w/o reduction                           */
    /* XR50 Panel Format Register 1                                  */
    /* bit 1-0: Frame Rate Control; 00, No FRC;                      */
    /*          01, 16-frame FRC for colour STN and monochrome       */
    /*          10, 2-frame FRC for colour TFT or monochrome;        */
    /*          11, reserved                                         */
    /* bit 3-2: Dither Enable                                        */
    /*          00, disable dithering; 01, enable dithering          */
    /*          for 256 mode                                         */
    /*          10, enable dithering for all modes; 11, reserved     */
    /* bit6-4: Clock Divide (CD)                                     */
    /*          000, Shift Clock Freq = Dot Clock Freq;              */
    /*          001, SClk = DClk/2; 010 SClk = DClk/4;               */
    /*          011, SClk = DClk/8; 100 SClk = DClk/16;              */
    /* bit 7: TFT data width                                         */
    /*          0, 16 bit(565RGB); 1, 24bit (888RGB)                 */
    if (pScrn->bitsPerPixel == 16) {
	ChipsNew->XR[0x06] |= 0xC4;   /*15 or 16 bpp colour         */
	ChipsNew->XR[0x0F] |= 0x10;   /*Hi-/True-Colour             */
	ChipsNew->XR[0x40] = 0x02;    /*BitBLT Draw Mode for 16 bpp */
	if (pScrn->weight.green != 5)
	    ChipsNew->XR[0x06] |= 0x08;	/*16bpp              */
    } else if (pScrn->bitsPerPixel == 24) {
	ChipsNew->XR[0x06] |= 0xC8;   /*24 bpp colour               */
	ChipsNew->XR[0x0F] |= 0x10;   /*Hi-/True-Colour             */
	if (xf86ReturnOptValBool(cPtr->Options, OPTION_18_BIT_BUS, FALSE)) {
	    ChipsNew->XR[0x50] &= 0x7F;   /*18 bit TFT data width   */
	} else {
	    ChipsNew->XR[0x50] |= 0x80;   /*24 bit TFT data width   */
	}
    }

    /*CRT only: interlaced mode */
    if (!(cPtr->PanelType & ChipsLCD)) {
	if (mode->Flags & V_INTERLACE){
	    ChipsNew->XR[0x28] |= 0x20;    /* set interlace         */
	    /* empirical value       */
	    tmp = ((((mode->CrtcHDisplay >> 3) - 1) >> 1) 
		- 6 * (pScrn->bitsPerPixel >= 8 ? bytesPerPixel : 1 ));
	    if(cPtr->Chipset < CHIPS_CT65535)
		ChipsNew->XR[0x19] = tmp & 0xFF;
	    else
		ChipsNew->XR[0x29] = tmp & 0xFF;
 	    ChipsNew->XR[0x0F] &= ~0x40;   /* set SW-Flag           */
	} else {
	    ChipsNew->XR[0x28] &= ~0x20;   /* unset interlace       */
 	    ChipsNew->XR[0x0F] |=  0x40;   /* set SW-Flag           */
	}
    }

    /* STN specific */
    if (IS_STN(cPtr->PanelType)) {
	ChipsNew->XR[0x50] &= ~0x03;  /* FRC clear                  */
	ChipsNew->XR[0x50] |= 0x01;   /* 16 frame FRC               */
	ChipsNew->XR[0x50] &= ~0x0C;  /* Dither clear               */
	ChipsNew->XR[0x50] |= 0x08;   /* Dither all modes           */
 	if (cPtr->Chipset == CHIPS_CT65548) {
	    ChipsNew->XR[0x03] |= 0x20; /* CRT I/F priority           */
	    ChipsNew->XR[0x04] |= 0x10; /* RAS precharge 65548        */
	}
    }

    /* This stuff was empirically derived several years ago. Not sure its 
     * still needed, and I'd love to get rid of it as its ugly
     */
    switch (cPtr->Chipset) {
    case CHIPS_CT65545:		  /*jet mini *//*DEC HighNote Ultra DSTN */
	ChipsNew->XR[0x03] |= 0x10;   /* do not hold off CPU for palette acc*/
	break;
    case CHIPS_CT65546:			       /*CT 65546, only for Toshiba */
	ChipsNew->XR[0x05] |= 0x80;   /* EDO RAM enable */
	break;
    }
    
    if (cPtr->PanelType & ChipsLCD) 
        ChipsNew->XR[0x51] |= 0x04;
    else 
        ChipsNew->XR[0x51] &= ~0x04;

    /* Program the registers */
    /*vgaHWProtect(pScrn, TRUE);*/
    chipsRestore(pScrn, ChipsStd, ChipsNew, FALSE);
    /*vgaHWProtect(pScrn, FALSE);*/

    return (TRUE);
}

static void 
chipsRestore(ScrnInfoPtr pScrn, vgaRegPtr VgaReg, CHIPSRegPtr ChipsReg,
	     Bool restoreFonts)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    unsigned char tmp = 0;

    /*vgaHWProtect(pScrn, TRUE);*/

    /* set registers so that we can program the controller */
    if (IS_HiQV(cPtr)) {
	cPtr->writeXR(cPtr, 0x0E, 0x00);
	if (cPtr->Flags & ChipsDualChannelSupport) {
	    tmp = cPtr->readFR(cPtr, 0x01);		/* Disable pipeline */
	    cPtr->writeFR(cPtr, 0x01, (tmp & 0xFC));
	    cPtr->writeFR(cPtr, 0x02, 0x00);		/* CRT/FP off */
	}
    } else {
	cPtr->writeXR(cPtr, 0x10, 0x00);
	cPtr->writeXR(cPtr, 0x11, 0x00);
	tmp = cPtr->readXR(cPtr, 0x0C) & ~0x50; /* WINgine stores MSB here */
	cPtr->writeXR(cPtr, 0x0C, tmp);
	cPtr->writeXR(cPtr, 0x15, 0x00); /* unprotect all registers */
	tmp = cPtr->readXR(cPtr, 0x14); 
	cPtr->writeXR(cPtr, 0x14, tmp & ~0x20);  /* enable vsync on ST01 */
    }

    chipsFixResume(pScrn);

    /* 
     * Wait for vsync if sequencer is running - stop sequencer.
     * Only do if sync reset is ignored. Dual pipeline capable 
     * chips have pipeline forced off here, so we don't care. 
     */
    if ((cPtr->SyncResetIgn)  && (!(cPtr->Flags & ChipsDualChannelSupport))) {
	while (((hwp->readST01(hwp)) & 0x08) == 0x08); /* VSync off */
	while (((hwp->readST01(hwp)) & 0x08) == 0x00); /* VSync on  */
	hwp->writeSeq(hwp, 0x07, 0x00); /* reset hsync - just in case...  */
    }

    /* set the clock */
    chipsClockLoad(pScrn, &ChipsReg->Clock);
    /* chipsClockLoad() sets this so we don't want vgaHWRestore() change it */
    VgaReg->MiscOutReg = inb(cPtr->PIOBase + 0x3CC);
	
    /* set extended regs */
    chipsRestoreExtendedRegs(pScrn, ChipsReg);
#if 0
    /* if people complain about lock ups or blank screens -- re-enable */
    /* set CRTC registers - do it before sequencer restarts */
    for (i=0; i<25; i++) 
	hwp->writeCrtc(hwp, i, VgaReg->CRTC[i]);
#endif
    /* set generic registers */
    /* 
     * Enabling writing to the colourmap causes 69030's to lock. 
     * Anyone care to explain to me why ????
     */
    if (cPtr->Flags & ChipsDualChannelSupport) {
        /* Enable pipeline if needed */
        cPtr->writeFR(cPtr, 0x01, ChipsReg->FR[0x01]);
	cPtr->writeFR(cPtr, 0x02, ChipsReg->FR[0x02]);
	vgaHWRestore(pScrn, VgaReg, VGA_SR_MODE |
		(restoreFonts ? VGA_SR_FONTS : 0));
    } else {
	vgaHWRestore(pScrn, VgaReg, VGA_SR_MODE | VGA_SR_CMAP | 
		     (restoreFonts ? VGA_SR_FONTS : 0));
    }

    /* set stretching registers */
    if (IS_HiQV(cPtr)) {
	chipsRestoreStretching(pScrn, (unsigned char)ChipsReg->FR[0x40],
			       (unsigned char)ChipsReg->FR[0x48]);
#if 0 
	/* if people report about stretching not working -- re-enable */
	/* why twice ? :
	 * sometimes the console is not well restored even if these registers 
	 * are good, re-write the registers works around it
	 */
	chipsRestoreStretching(pScrn, (unsigned char)ChipsReg->FR[0x40],
			       (unsigned char)ChipsReg->FR[0x48]);
#endif
    } else if (!IS_Wingine(cPtr))
	chipsRestoreStretching(pScrn, (unsigned char)ChipsReg->XR[0x55],
			       (unsigned char)ChipsReg->XR[0x57]);

    /* perform a synchronous reset */
    if (!cPtr->SyncResetIgn) {
	if (!IS_HiQV(cPtr)) {
	    /* enable synchronous reset on 655xx */
	    tmp = cPtr->readXR(cPtr, 0x0E);
	    cPtr->writeXR(cPtr, 0x0E, tmp & 0x7F);
	}
	hwp->writeSeq(hwp, 0x00, 0x01);
	usleep(10000);
	hwp->writeSeq(hwp, 0x00, 0x03);
	if (!IS_HiQV(cPtr))
	    cPtr->writeXR(cPtr, 0x0E, tmp);
    }
    /* Flag valid start address, if using CRT extensions */
    if (IS_HiQV(cPtr) && (ChipsReg->XR[0x09] & 0x1) == 0x1) {
	tmp = hwp->readCrtc(hwp, 0x40);
	hwp->writeCrtc(hwp, 0x40, tmp | 0x80);
    }

    /* Fix resume again here, as Nozomi seems to need it          */
     chipsFixResume(pScrn);
    /*vgaHWProtect(pScrn, FALSE);*/

#if 0
     /* Enable pipeline if needed */
     if (cPtr->Flags & ChipsDualChannelSupport) {
	cPtr->writeFR(cPtr, 0x01, ChipsReg->FR[0x01]);
	cPtr->writeFR(cPtr, 0x02, ChipsReg->FR[0x02]);
     }
#endif
}

static void
chipsRestoreExtendedRegs(ScrnInfoPtr pScrn, CHIPSRegPtr Regs)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    int i;
    unsigned char tmp;

    if (IS_HiQV(cPtr)) {
	/* set extended regs */
	for (i = 0; i < 0x43; i++) {
	    if ((cPtr->readXR(cPtr, i)) != Regs->XR[i])
		cPtr->writeXR(cPtr, i, Regs->XR[i]);
	}

	/* Set SAR04 multimedia register correctly */
	if ((cPtr->Flags & ChipsVideoSupport)) {
#ifdef SAR04
	    cPtr->writeXR(cPtr, 0x4E, 0x04);
	    if (cPtr->readXR(cPtr, 0x4F) != Regs->XR[0x4F])
		cPtr->writeXR(cPtr, 0x4F, Regs->XR[0x4F]);
#endif
	}

	/* Don't touch reserved memory control registers */
	for (i = 0x50; i < 0xBF; i++) {
	    if ((cPtr->readXR(cPtr, i)) != Regs->XR[i])
		cPtr->writeXR(cPtr, i, Regs->XR[i]);
	}
	/* Don't touch VCLK regs, but fix up MClk */
	
	/* set mem clock */
	tmp = cPtr->readXR(cPtr, 0xCE); /* Select Fixed MClk before */
	cPtr->writeXR(cPtr, 0xCE, tmp & 0x7F);
	if ((cPtr->readXR(cPtr, 0xCC)) != Regs->XR[0xCC])
	    cPtr->writeXR(cPtr, 0xCC, Regs->XR[0xCC]);
	if ((cPtr->readXR(cPtr, 0xCD)) != Regs->XR[0xCD])
	    cPtr->writeXR(cPtr, 0xCD, Regs->XR[0xCD]);
	if ((cPtr->readXR(cPtr, 0xCE)) != Regs->XR[0xCE])
	    cPtr->writeXR(cPtr, 0xCE, Regs->XR[0xCE]);

	/* set flat panel regs. */
	for (i = 0xD0; i < 0xFF; i++) {
	    if ((cPtr->readXR(cPtr, i)) != Regs->XR[i])
		cPtr->writeXR(cPtr, i, Regs->XR[i]);
	}

	for (i = 0; i < 0x80; i++) {
	    /* Don't touch alternate clock select reg. */
	    if ((i == 0x01) && (cPtr->Chipset == CHIPS_CT69030)) {
	    	/* restore the non clock bits */
		tmp = cPtr->readFR(cPtr, 0x01);
		cPtr->writeFR(cPtr, 0x01, ((Regs->FR[0x01] & 0xF0) |
				(tmp & ~0xF0)));
		continue;
	    }

	    if ((i == 0x02) && (cPtr->Chipset == CHIPS_CT69030))
	    	/* keep pipeline disabled till we are ready */
		continue;
	  
	    if ((i == 0x03) && (cPtr->Chipset != CHIPS_CT69030)) {
	    	/* restore the non clock bits */
		tmp = cPtr->readFR(cPtr, 0x03);
		cPtr->writeFR(cPtr, 0x03, ((Regs->FR[0x03] & 0xC3) |
				(tmp & ~0xC3)));
		continue;
	    }

	    if ((i > 0x03) && (cPtr->Chipset != CHIPS_CT69030) &&
				(cPtr->SecondCrtc == TRUE))
		continue;

	    if ( (i == 0x40) || (i==0x48)) {
	      /* !! set stretching but disable compensation   */
	      cPtr->writeFR(cPtr, i, Regs->FR[i] & 0xFE);
	      continue ;     /* some registers must be set before FR40/FR48 */
	    }
	    if ((cPtr->readFR(cPtr, i)) != Regs->FR[i]) {
		cPtr->writeFR(cPtr, i, Regs->FR[i]);
	    }
	}

	/* set the multimedia regs */
	for (i = 0x02; i < 0x80; i++) {
	    if ( (i == 0x43) || (i == 0x44))
		continue;
	    if ((cPtr->readMR(cPtr, i)) != Regs->MR[i])
		cPtr->writeMR(cPtr, i, Regs->MR[i]);
	}
	
	/* set extended crtc regs. */
	for (i = 0x30; i < 0x80; i++) {
	    if ((hwp->readCrtc(hwp, i)) != Regs->CR[i]) 
		hwp->writeCrtc(hwp, i, Regs->CR[i]);
	}
    } else {
	/* set extended regs. */
	for (i = 0; i < 0x30; i++) {
	    if ((cPtr->readXR(cPtr, i)) != Regs->XR[i])
		cPtr->writeXR(cPtr, i, Regs->XR[i]);
	}
	cPtr->writeXR(cPtr, 0x15, 0x00); /* unprotect just in case ... */
	/* Don't touch MCLK/VCLK regs. */
	for (i = 0x34; i < 0x54; i++) {
	    if ((cPtr->readXR(cPtr, i)) != Regs->XR[i])
		cPtr->writeXR(cPtr, i, Regs->XR[i]);
	}
	tmp = cPtr->readXR(cPtr, 0x54);	/*  restore the non clock bits     */
	cPtr->writeXR(cPtr, 0x54, ((Regs->XR[0x54] & 0xF3) | (tmp & ~0xF3)));
	cPtr->writeXR(cPtr, 0x55, Regs->XR[0x55] & 0xFE); /* h-comp off     */
	cPtr->writeXR(cPtr, 0x56, Regs->XR[0x56]);
	cPtr->writeXR(cPtr, 0x57, Regs->XR[0x57] & 0xFE); /* v-comp off     */
	for (i=0x58; i < 0x7D; i++) {/* don't touch XR7D and XR7F on WINGINE */
	    if ((cPtr->readXR(cPtr, i)) != Regs->XR[i])
		cPtr->writeXR(cPtr, i, Regs->XR[i]);
	}
    }
#ifdef DEBUG
    /* debug - dump out all the extended registers... */
    if (IS_HiQV(cPtr)) {
	for (i = 0; i < 0xFF; i++) {
	    ErrorF("XR%X - %X : %X\n", i, Regs->XR[i],
		   cPtr->readXR(cPtr, i));
	}
	for (i = 0; i < 0x80; i++) {
	    ErrorF("FR%X - %X : %X\n", i, Regs->FR[i],
		   cPtr->readFR(cPtr, i));
	}
    } else {
	for (i = 0; i < 0x80; i++) {
	    ErrorF("XR%X - %X : %X\n", i, Regs->XR[i],
		   cPtr->readXR(cPtr, i));
	}
    }
#endif
}

static void
chipsRestoreStretching(ScrnInfoPtr pScrn, unsigned char ctHorizontalStretch,
		       unsigned char ctVerticalStretch)
{
    unsigned char tmp;
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    
    /* write to regs. */
    if (IS_HiQV(cPtr)) {
	tmp = cPtr->readFR(cPtr, 0x48);
	cPtr->writeFR(cPtr, 0x48, (tmp & 0xFE) | (ctVerticalStretch & 0x01));
	tmp = cPtr->readFR(cPtr, 0x40);
	cPtr->writeFR(cPtr, 0x40, (tmp & 0xFE) | (ctHorizontalStretch & 0x01));
    } else {
	tmp = cPtr->readXR(cPtr, 0x55);
	cPtr->writeXR(cPtr, 0x55, (tmp & 0xFE) | (ctHorizontalStretch & 0x01));
	tmp = cPtr->readXR(cPtr, 0x57);
	cPtr->writeXR(cPtr, 0x57, (tmp & 0xFE) | (ctVerticalStretch & 0x01));
    }

    usleep(20000);			/* to be active */
}

static int
chipsVideoMode(int depth, int displayHSize,
	       int displayVSize)
{
    /*     4 bpp  8 bpp  16 bpp  18 bpp  24 bpp  32 bpp */
    /* 640  0x20   0x30    0x40    -      0x50     -    */
    /* 800  0x22   0x32    0x42    -      0x52     -    */
    /*1024  0x24   0x34    0x44    -      0x54     -    for 1024x768 */
    /*1024   -     0x36    0x47    -      0x56     -    for 1024x600 */
    /*1152  0x27   0x37    0x47    -      0x57     -    */
    /*1280  0x28   0x38    0x49    -        -      -    */
    /*1600  0x2C   0x3C    0x4C   0x5D      -      -    */
    /*This value is only for BIOS.... */

    int videoMode = 0;

    switch (depth) {
    case 1:
    case 4:
	videoMode = 0x20;
	break;
    case 8:
	videoMode = 0x30;
	break;
    case 15:
	videoMode = 0x40;
	break;
    case 16:
	videoMode = 0x41;
	break;
    default:
	videoMode = 0x50;
	break;
    }

    switch (displayHSize) {
    case 800:
	videoMode |= 0x02;
	break;
    case 1024:
	videoMode |= 0x04;
	if(displayVSize < 768)
	    videoMode |= 0x02;
	break;
    case 1152:
	videoMode |= 0x07;
	break;
    case 1280:
	videoMode |= 0x08;
	break;
    case 1600:
	videoMode |= 0x0C; /*0x0A??*/
	break;
    }

    return videoMode;
}


/*
 * Map the framebuffer and MMIO memory.
 */

static Bool
chipsMapMem(ScrnInfoPtr pScrn)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CHIPSEntPtr cPtrEnt;

    if (cPtr->Flags & ChipsLinearSupport) {
	if (cPtr->UseMMIO) {
	    if (IS_HiQV(cPtr)) {
#ifndef XSERVER_LIBPCIACCESS
		if (cPtr->pEnt->location.type == BUS_PCI)
		    cPtr->MMIOBase = xf86MapPciMem(pScrn->scrnIndex,
			   VIDMEM_MMIO_32BIT,cPtr->PciTag, cPtr->IOAddress,
			   0x20000L);
		 else 
		    cPtr->MMIOBase = xf86MapVidMem(pScrn->scrnIndex,
			   VIDMEM_MMIO_32BIT, cPtr->IOAddress, 0x20000L);
#else
		{
		  void** result = (void**)&cPtr->MMIOBase;
		  int err = pci_device_map_range(cPtr->PciInfo,
						 cPtr->IOAddress,
						 0x20000L,
						 PCI_DEV_MAP_FLAG_WRITABLE,
						 result);
		  if (err) 
		    return FALSE;
		}
#endif
	    } else {
#ifndef XSERVER_LIBPCIACCESS
		if (cPtr->pEnt->location.type == BUS_PCI)
		    cPtr->MMIOBase = xf86MapPciMem(pScrn->scrnIndex,
			  VIDMEM_MMIO_32BIT, cPtr->PciTag, cPtr->IOAddress,
			  0x10000L);
		else
		    cPtr->MMIOBase = xf86MapVidMem(pScrn->scrnIndex,
			  VIDMEM_MMIO_32BIT, cPtr->IOAddress, 0x10000L);
#else
		{
		  void** result = (void**)&cPtr->MMIOBase;
		  int err = pci_device_map_range(cPtr->PciInfo,
						 cPtr->IOAddress,
						 0x10000L,
						 PCI_DEV_MAP_FLAG_WRITABLE,
						 result);
		  if (err) 
		    return FALSE;
		}
#endif
	    }

	    if (cPtr->MMIOBase == NULL)
		return FALSE;
	}
	if (cPtr->FbMapSize) {
	  unsigned long Addr = (unsigned long)cPtr->FbAddress;
	  unsigned int Map =  cPtr->FbMapSize;
	  
	  if ((cPtr->Flags & ChipsDualChannelSupport) &&
	      (xf86IsEntityShared(pScrn->entityList[0]))) {
	      cPtrEnt = xf86GetEntityPrivate(pScrn->entityList[0],
					     CHIPSEntityIndex)->ptr;
	    if(cPtr->SecondCrtc == FALSE) {
	      Addr = cPtrEnt->masterFbAddress;
	      Map = cPtrEnt->masterFbMapSize;
	    } else {
	      Addr = cPtrEnt->slaveFbAddress;
	      Map = cPtrEnt->slaveFbMapSize;
	    }
	  }

#ifndef XSERVER_LIBPCIACCESS
	  if (cPtr->pEnt->location.type == BUS_PCI)
	      cPtr->FbBase = xf86MapPciMem(pScrn->scrnIndex,VIDMEM_FRAMEBUFFER,
 			          cPtr->PciTag, Addr, Map);

	  else
	      cPtr->FbBase = xf86MapVidMem(pScrn->scrnIndex,VIDMEM_FRAMEBUFFER,
					   Addr, Map);
#else
	  {
	    void** result = (void**)&cPtr->FbBase;
	    int err = pci_device_map_range(cPtr->PciInfo,
					   Addr,
					   Map,
					   PCI_DEV_MAP_FLAG_WRITABLE |
					   PCI_DEV_MAP_FLAG_WRITE_COMBINE,
					   result);
	    if (err) 
	      return FALSE;
	  }

#endif

	  if (cPtr->FbBase == NULL)
	      return FALSE;
	}
	if (cPtr->Flags & ChipsFullMMIOSupport) {
#ifndef XSERVER_LIBPCIACCESS
		cPtr->MMIOBaseVGA = xf86MapPciMem(pScrn->scrnIndex,
						  VIDMEM_MMIO,cPtr->PciTag,
						  cPtr->IOAddress, 0x2000L);
#else
		cPtr->MMIOBaseVGA = cPtr->MMIOBase;
#endif
	    /* 69030 MMIO Fix.
	     *
	     * The hardware lets us map the PipeB data registers
	     * into the MMIO address space normally occupied by PipeA,
	     * but it doesn't allow remapping of the index registers.
	     * So we're forced to map a separate MMIO space for each
	     * pipe and to toggle between them as necessary. -GHB
	     */
	    if (cPtr->Flags & ChipsDualChannelSupport)
#ifndef XSERVER_LIBPCIACCESS
	       	cPtr->MMIOBasePipeB = xf86MapPciMem(pScrn->scrnIndex,
				      VIDMEM_MMIO,cPtr->PciTag,
				      cPtr->IOAddress + 0x800000, 0x2000L);
#else
	    {
	      void** result = (void**)&cPtr->MMIOBasePipeB;
	      int err = pci_device_map_range(cPtr->PciInfo,
					     cPtr->IOAddress + 0x800000,
					     0x2000L,
					     PCI_DEV_MAP_FLAG_WRITABLE,
					     result);
	      if (err) 
		return FALSE;
	    }
#endif

	    cPtr->MMIOBasePipeA = cPtr->MMIOBaseVGA;
	}
    } else {
	/* In paged mode Base is the VGA window at 0xA0000 */
	cPtr->FbBase = hwp->Base;
    }
    
    return TRUE;
}


/*
 * Unmap the framebuffer and MMIO memory.
 */

static Bool
chipsUnmapMem(ScrnInfoPtr pScrn)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);

    if (cPtr->Flags & ChipsLinearSupport) {
	if (IS_HiQV(cPtr)) {
#ifndef XSERVER_LIBPCIACCESS
	    if (cPtr->MMIOBase)
		xf86UnMapVidMem(pScrn->scrnIndex, (pointer)cPtr->MMIOBase,
				0x20000);
	    if (cPtr->MMIOBasePipeB)
		xf86UnMapVidMem(pScrn->scrnIndex, (pointer)cPtr->MMIOBasePipeB,
				0x20000);
#else
	    if (cPtr->MMIOBase)
	      pci_device_unmap_range(cPtr->PciInfo, cPtr->MMIOBase, 0x20000);
	    
	    if (cPtr->MMIOBasePipeB)
	      pci_device_unmap_range(cPtr->PciInfo, cPtr->MMIOBasePipeB, 0x2000);
	      
#endif
	    cPtr->MMIOBasePipeB = NULL;
	} else {
#ifndef XSERVER_LIBPCIACCESS
	  if (cPtr->MMIOBase)
	      xf86UnMapVidMem(pScrn->scrnIndex, (pointer)cPtr->MMIOBase,
			      0x10000);
#else
	    if (cPtr->MMIOBase)
	      pci_device_unmap_range(cPtr->PciInfo, cPtr->MMIOBase, 0x10000);
#endif
	}
	cPtr->MMIOBase = NULL;
#ifndef XSERVER_LIBPCIACCESS
	xf86UnMapVidMem(pScrn->scrnIndex, (pointer)cPtr->FbBase, 
			cPtr->FbMapSize);
#else
	pci_device_unmap_range(cPtr->PciInfo, cPtr->FbBase, cPtr->FbMapSize);
#endif
    }
    cPtr->FbBase = NULL;
    
    return TRUE;
}

static void
chipsProtect(ScrnInfoPtr pScrn, Bool on)
{
    vgaHWProtect(pScrn, on);
}

static void
chipsBlankScreen(ScrnInfoPtr pScrn, Bool unblank)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    unsigned char scrn;
    CHIPSEntPtr cPtrEnt;    

    if (cPtr->UseDualChannel) {
        cPtrEnt = xf86GetEntityPrivate(pScrn->entityList[0],
					       CHIPSEntityIndex)->ptr;
	DUALREOPEN;
    }

    /* fix things that could be messed up by suspend/resume */
    if (!IS_HiQV(cPtr))
	cPtr->writeXR(cPtr, 0x15, 0x00);

    scrn = hwp->readSeq(hwp, 0x01);

    if (unblank) {
	scrn &= 0xDF;                       /* enable screen */
    } else {
	scrn |= 0x20;                       /* blank screen */
    }

    /* synchronous reset - stop counters */
    if (!cPtr->SyncResetIgn) {
	hwp->writeSeq(hwp, 0x00, 0x01);
    }

    hwp->writeSeq(hwp, 0x01, scrn); /* change mode */

    /* end reset - start counters */
    if (!cPtr->SyncResetIgn) {
	hwp->writeSeq(hwp, 0x00, 0x03);
    }

    if ((cPtr->UseDualChannel) &&
		(! xf86IsEntityShared(pScrn->entityList[0]))) {
	unsigned int IOSS, MSS;
	IOSS = cPtr->readIOSS(cPtr);
	MSS = cPtr->readMSS(cPtr);
	cPtr->writeIOSS(cPtr, ((cPtr->storeIOSS & IOSS_MASK) |
			       IOSS_PIPE_B));
	cPtr->writeMSS(cPtr, hwp, ((cPtr->storeMSS & MSS_MASK) | MSS_PIPE_B));

	/* fix things that could be messed up by suspend/resume */
	if (!IS_HiQV(cPtr))
	    cPtr->writeXR(cPtr, 0x15, 0x00);

	scrn = hwp->readSeq(hwp, 0x01);
	
	if (unblank) {
	    scrn &= 0xDF;                       /* enable screen */
	} else {
	    scrn |= 0x20;                       /* blank screen */
	}

	/* synchronous reset - stop counters */
	if (!cPtr->SyncResetIgn) {
	    hwp->writeSeq(hwp, 0x00, 0x01);
	}

	hwp->writeSeq(hwp, 0x01, scrn); /* change mode */

	/* end reset - start counters */
	if (!cPtr->SyncResetIgn) {
	    hwp->writeSeq(hwp, 0x00, 0x03);
	}

	cPtr->writeIOSS(cPtr, IOSS);
	cPtr->writeMSS(cPtr, hwp, MSS);
    }

}

static void
chipsLock(ScrnInfoPtr pScrn)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    unsigned char tmp;
    
    vgaHWLock(hwp);

    if (!IS_HiQV(cPtr)) {
	/* group protection attribute controller access */
	cPtr->writeXR(cPtr, 0x15, cPtr->SuspendHack.xr15);
	tmp = cPtr->readXR(cPtr, 0x02);
	cPtr->writeXR(cPtr, 0x02, (tmp & ~0x18) | cPtr->SuspendHack.xr02);
	tmp = cPtr->readXR(cPtr, 0x14);
	cPtr->writeXR(cPtr, 0x14, (tmp & ~0x20) | cPtr->SuspendHack.xr14);

	/* reset 32 bit register access */
	if (cPtr->Chipset > CHIPS_CT65540) {
	    tmp = cPtr->readXR(cPtr, 0x03);
	    cPtr->writeXR(cPtr, 0x03, (tmp & ~0x0A) | cPtr->SuspendHack.xr03);
	}
    }
}

static void
chipsUnlock(ScrnInfoPtr pScrn)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    unsigned char tmp;
    
    if (!IS_HiQV(cPtr)) {
	/* group protection attribute controller access */
	cPtr->writeXR(cPtr, 0x15, 0x00);
	tmp = cPtr->readXR(cPtr, 0x02);
	cPtr->writeXR(cPtr, 0x02, (tmp & ~0x18));
	tmp = cPtr->readXR(cPtr, 0x14);
	cPtr->writeXR(cPtr, 0x14, (tmp & ~0x20));
	/* enable 32 bit register access */
	if (cPtr->Chipset > CHIPS_CT65540) {
	    cPtr->writeXR(cPtr, 0x03, cPtr->SuspendHack.xr03 | 0x0A);
	}
    }
    vgaHWUnlock(hwp);
}

static void
chipsHWCursorOn(CHIPSPtr cPtr, ScrnInfoPtr pScrn)
{
    /* enable HW cursor */
    if (cPtr->HWCursorShown) {
	if (IS_HiQV(cPtr)) {
	    cPtr->writeXR(cPtr, 0xA0, cPtr->HWCursorContents & 0xFF);
	    if (cPtr->UseDualChannel && 
		(! xf86IsEntityShared(pScrn->entityList[0]))) {
		unsigned int IOSS, MSS;
		IOSS = cPtr->readIOSS(cPtr);
		MSS = cPtr->readMSS(cPtr);
		cPtr->writeIOSS(cPtr, ((cPtr->storeIOSS & IOSS_MASK) |
				       IOSS_PIPE_B));
		cPtr->writeMSS(cPtr, VGAHWPTR(pScrn), ((cPtr->storeMSS &
					  MSS_MASK) | MSS_PIPE_B));
		cPtr->writeXR(cPtr, 0xA0, cPtr->HWCursorContents & 0xFF);
		cPtr->writeIOSS(cPtr, IOSS);
		cPtr->writeMSS(cPtr, VGAHWPTR(pScrn), MSS);
	    }
	} else {
	    HW_DEBUG(0x8);	
	    if (cPtr->UseMMIO) {
		MMIOmeml(DR(0x8)) = cPtr->HWCursorContents;
	    } else {
		outl(cPtr->PIOBase + DR(0x8), cPtr->HWCursorContents);
	    }
	}
    }
}

static void
chipsHWCursorOff(CHIPSPtr cPtr, ScrnInfoPtr pScrn)
{
    /* disable HW cursor */
    if (cPtr->HWCursorShown) {
	if (IS_HiQV(cPtr)) {
	    cPtr->HWCursorContents = cPtr->readXR(cPtr, 0xA0);
	    cPtr->writeXR(cPtr, 0xA0, cPtr->HWCursorContents & 0xF8);
	} else {
	    HW_DEBUG(0x8);
	    if (cPtr->UseMMIO) {
		cPtr->HWCursorContents = MMIOmeml(DR(0x8));
		/* Below used to be MMIOmemw() change back if problem!!! */
		/* Also see ct_cursor.c */
		MMIOmeml(DR(0x8)) = cPtr->HWCursorContents & 0xFFFE;
	    } else {
		cPtr->HWCursorContents = inl(cPtr->PIOBase + DR(0x8));
		outw(cPtr->PIOBase + DR(0x8), cPtr->HWCursorContents & 0xFFFE);
	    }
	}
    }
}

void
chipsFixResume(ScrnInfoPtr pScrn)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    unsigned char tmp;
    
    /* fix things that could be messed up by suspend/resume */
    if (!IS_HiQV(cPtr))
	cPtr->writeXR(cPtr, 0x15, 0x00);
    tmp = hwp->readMiscOut(hwp);
    hwp->writeMiscOut(hwp, (tmp & 0xFE) | cPtr->SuspendHack.vgaIOBaseFlag);
    tmp = hwp->readCrtc(hwp, 0x11);
    hwp->writeCrtc(hwp, 0x11, (tmp & 0x7F));
}

static char
chipsTestDACComp(ScrnInfoPtr pScrn, unsigned char a, unsigned char b,
		 unsigned char c)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    unsigned char type;

    hwp->writeDacWriteAddr(hwp, 0x00);
    while ((hwp->readST01(hwp)) & 0x08){};    /* wait for vsync to end */
    while (!((hwp->readST01(hwp)) & 0x08)){}; /* wait for new vsync  */
    hwp->writeDacData(hwp, a);                /* set pattern */
    hwp->writeDacData(hwp, b);
    hwp->writeDacData(hwp, c);
    while (!(hwp->readST01(hwp)) & 0x01){};   /* wait for hsync to end  */
    while ((hwp->readST01(hwp)) & 0x01){};    /* wait for hsync to end  */
    type = hwp->readST00(hwp);                /* read comparator        */
    return (type & 0x10);
}

static int
chipsProbeMonitor(ScrnInfoPtr pScrn)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    unsigned char dacmask;
    unsigned char dacdata[3];
    unsigned char xr1, xr2;
    int type = 2;  /* no monitor */
    unsigned char IOSS=0, MSS=0, tmpfr02=0, tmpfr01a=0, tmpfr01b=0;

    /* Dual channel display, enable both pipelines */
    if (cPtr->Flags & ChipsDualChannelSupport) {
	IOSS = cPtr->readIOSS(cPtr);
	MSS = cPtr->readMSS(cPtr);
	tmpfr02 = cPtr->readFR(cPtr,0x02);
	cPtr->writeFR(cPtr, 0x02, (tmpfr02 & 0xCF)); /* CRT/FP off */
	usleep(1000);
	cPtr->writeIOSS(cPtr, ((IOSS & IOSS_MASK) | IOSS_PIPE_A));
	cPtr->writeMSS(cPtr, hwp, ((MSS & MSS_MASK) | MSS_PIPE_A));
	tmpfr01a = cPtr->readFR(cPtr,0x01);
	if ((tmpfr01a & 0x3) != 0x01)
	  cPtr->writeFR(cPtr, 0x01, ((tmpfr01a & 0xFC) | 0x1)); 
	cPtr->writeIOSS(cPtr, ((IOSS & IOSS_MASK) | IOSS_PIPE_B));
	cPtr->writeMSS(cPtr, hwp, ((MSS & MSS_MASK) | MSS_PIPE_B));
	tmpfr01b = cPtr->readFR(cPtr,0x01);
	if ((tmpfr01b & 0x3) != 0x01)
	  cPtr->writeFR(cPtr, 0x01, ((tmpfr01b & 0xFC) | 0x1)); 
	cPtr->writeIOSS(cPtr, IOSS);
	cPtr->writeMSS(cPtr, hwp, MSS);
	cPtr->writeFR(cPtr, 0x02, (tmpfr02 & 0xCF) | 0x10); /* CRT on/FP off*/ 
    }

    dacmask = hwp->readDacMask(hwp);    /* save registers */ 
    hwp->writeDacMask(hwp, 0x00);
    hwp->writeDacReadAddr(hwp, 0x00);

    dacdata[0]=hwp->readDacData(hwp);
    dacdata[1]=hwp->readDacData(hwp);
    dacdata[2]=hwp->readDacData(hwp);

    if (!IS_HiQV(cPtr)) {
	xr1 = cPtr->readXR(cPtr, 0x06);
	xr2 = cPtr->readXR(cPtr, 0x1F);
	cPtr->writeXR(cPtr, 0x06, xr1 & 0xF1);  /* turn on dac */
	cPtr->writeXR(cPtr, 0x1F, xr2 & 0x7F);  /* enable comp */
    } else {
	xr1 = cPtr->readXR(cPtr, 0x81);
	xr2 = cPtr->readXR(cPtr, 0xD0);
	cPtr->writeXR(cPtr, 0x81,(xr1 & 0xF0));
	cPtr->writeXR(cPtr, 0xD0,(xr2 | 0x03));
    }
    if (chipsTestDACComp(pScrn, 0x12,0x12,0x12)) {         /* test patterns */
	if (chipsTestDACComp(pScrn,0x14,0x14,0x14))        /* taken  from   */
	    if (!chipsTestDACComp(pScrn,0x2D,0x14,0x14))   /* BIOS          */
		if (!chipsTestDACComp(pScrn,0x14,0x2D,0x14))
		    if (!chipsTestDACComp(pScrn,0x14,0x14,0x2D))
			if (!chipsTestDACComp(pScrn,0x2D,0x2D,0x2D))
			    type = 0;    /* color monitor */
    } else {
	if (chipsTestDACComp(pScrn,0x04,0x12,0x04))
	    if (!chipsTestDACComp(pScrn,0x1E,0x12,0x04))
		if (!chipsTestDACComp(pScrn,0x04,0x2D,0x04))
		    if (!chipsTestDACComp(pScrn,0x1E,0x16,0x15))
			if (chipsTestDACComp(pScrn,0x00,0x00,0x00))
			    type = 1;    /* monochrome */
    }

    hwp->writeDacWriteAddr(hwp, 0x00);         /* restore registers */
    hwp->writeDacData(hwp, dacdata[0]);
    hwp->writeDacData(hwp, dacdata[1]);
    hwp->writeDacData(hwp, dacdata[2]);
    hwp->writeDacMask(hwp, dacmask);
    if (!IS_HiQV(cPtr)) {
	cPtr->writeXR(cPtr,0x06,xr1);
	cPtr->writeXR(cPtr,0x1F,xr2);
    } else {
	cPtr->writeXR(cPtr,0x81,xr1);
	cPtr->writeXR(cPtr,0xD0,xr2);
    }

    if (cPtr->Flags & ChipsDualChannelSupport) {
	cPtr->writeIOSS(cPtr, ((IOSS & IOSS_MASK) | IOSS_PIPE_A));
	cPtr->writeMSS(cPtr, hwp, ((MSS & MSS_MASK) | MSS_PIPE_A));
	cPtr->writeFR(cPtr, 0x01, tmpfr01a);
	cPtr->writeIOSS(cPtr, ((IOSS & IOSS_MASK) | IOSS_PIPE_B));
	cPtr->writeMSS(cPtr, hwp, ((MSS & MSS_MASK) | MSS_PIPE_B));
	cPtr->writeFR(cPtr, 0x01, tmpfr01b);
	usleep(1000);
	cPtr->writeIOSS(cPtr, IOSS);
	cPtr->writeMSS(cPtr, hwp, MSS);
	cPtr->writeFR(cPtr, 0x02, tmpfr02);
    }

    return type;
}

static int
chipsSetMonitor(ScrnInfoPtr pScrn)
{
    int tmp= chipsProbeMonitor(pScrn);

    switch (tmp) {
    case 0:
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Color monitor detected\n");
	break;
    case 1:
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Monochrome monitor detected\n");
	break;
    default:
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "No monitor detected\n");
    }
    return (tmp);
}

static void
chipsSetPanelType(CHIPSPtr cPtr)
{
    CARD8 tmp;
    
    if (IS_HiQV(cPtr)) {
	if (cPtr->Chipset == CHIPS_CT69030) {
	    tmp = cPtr->readFR(cPtr, 0x00);
	    if (tmp & 0x20) {
		/* FR02: DISPLAY TYPE REGISTER                         */
		/* FR02[4] = CRT, FR02[5] = FlatPanel                  */
		tmp = cPtr->readFR(cPtr, 0x02);
		if (tmp & 0x10)
		    cPtr->PanelType |= ChipsCRT;
		if (tmp & 0x20)
		    cPtr->PanelType |= ChipsLCD | ChipsLCDProbed;
	    } else {
		cPtr->PanelType |= ChipsCRT;
	    }
	} else {
	    /* test LCD */
	    /* FR01: DISPLAY TYPE REGISTER                         */
	    /* FR01[1:0]:   Display Type, 01 = CRT, 10 = FlatPanel */
	    /* LCD                                                 */
	    tmp = cPtr->readFR(cPtr, 0x01);
	    if ((tmp & 0x03) == 0x02) {
	        cPtr->PanelType |= ChipsLCD | ChipsLCDProbed;
	    }
	    tmp = cPtr->readXR(cPtr,0xD0);	
	    if (tmp & 0x01) {
	        cPtr->PanelType |= ChipsCRT;
	    }
	}
    } else {
	tmp = cPtr->readXR(cPtr, 0x51);
	/* test LCD */
	/* XR51: DISPLAY TYPE REGISTER                     */
	/* XR51[2]:   Display Type, 0 = CRT, 1 = FlatPanel */
	if (tmp & 0x04) {
	    cPtr->PanelType |= ChipsLCD | ChipsLCDProbed;
	} 
	if ((cPtr->readXR(cPtr, 0x06)) & 0x02) {
	    cPtr->PanelType |= ChipsCRT;
	}
    }
}

static void
chipsBlockHandler (BLOCKHANDLER_ARGS_DECL)
{
    SCREEN_PTR(arg);
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    CHIPSPtr    cPtr = CHIPSPTR(pScrn);
    
    pScreen->BlockHandler = cPtr->BlockHandler;
    (*pScreen->BlockHandler) (BLOCKHANDLER_ARGS);
    pScreen->BlockHandler = chipsBlockHandler;

    if(cPtr->VideoTimerCallback) {
	UpdateCurrentTime();
	(*cPtr->VideoTimerCallback)(pScrn, currentTime.milliseconds);
    }
}
