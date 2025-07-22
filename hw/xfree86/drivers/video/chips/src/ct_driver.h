/*
 * Modified 1996 by Egbert Eich <eich@xfree86.org>
 * Modified 1996 by David Bateman <dbateman@club-internet.fr>
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
#ifndef _CT_DRIVER_H_
#define _CT_DRIVER_H_

#include <xorg-config.h>

#include "ct_pcirename.h"
#include "exa.h"
#include "vbe.h"
#include "xf86Cursor.h"
#include "xf86i2c.h"
#include "xf86DDC.h"
#include "xf86xv.h"
#include "vgaHW.h"
#include <string.h>
#include <unistd.h>

#include "compat-api.h"

/* Supported chipsets */
typedef enum {
    CHIPS_CT65520,
    CHIPS_CT65525,
    CHIPS_CT65530,
    CHIPS_CT65535,
    CHIPS_CT65540,
    CHIPS_CT65545,
    CHIPS_CT65546,
    CHIPS_CT65548,
    CHIPS_CT65550,
    CHIPS_CT65554,
    CHIPS_CT65555,
    CHIPS_CT68554,
    CHIPS_CT69000,
    CHIPS_CT69030,
    CHIPS_CT64200,
    CHIPS_CT64300
} CHIPSType;

/* Clock related */
typedef struct {
    unsigned char msr;		/* Dot Clock Related */
    unsigned char fcr;
    unsigned char xr02;
    unsigned char xr03;
    unsigned char xr33;
    unsigned char xr54;
    unsigned char fr03;
    int Clock;
    int FPClock;
} CHIPSClockReg, *CHIPSClockPtr;

typedef struct {
    unsigned int ProbedClk;
    unsigned int Max;		/* Memory Clock Related */
    unsigned int Clk;
    unsigned char M;
    unsigned char N;
    unsigned char P;
    unsigned char PSN;
    unsigned char xrCC;
    unsigned char xrCD;
    unsigned char xrCE;
} CHIPSMemClockReg, *CHIPSMemClockPtr;

#define TYPE_HW 0x01
#define TYPE_PROGRAMMABLE 0x02
#define GET_TYPE 0x0F
#define OLD_STYLE 0x10
#define NEW_STYLE 0x20
#define HiQV_STYLE 0x30
#define WINGINE_1_STYLE 0x40        /* 64300: external clock; 4 clocks    */
#define WINGINE_2_STYLE 0x50        /* 64300: internal clock; 2 hw-clocks */
#define GET_STYLE 0xF0
#define LCD_TEXT_CLK_FREQ 25000	    /* lcd textclock if TYPE_PROGRAMMABLE */
#define CRT_TEXT_CLK_FREQ 28322     /* crt textclock if TYPE_PROGRAMMABLE */
#define Fref 14318180               /* The reference clock in Hertz       */

/* The capability flags for the C&T chipsets */
#define ChipsLinearSupport	0x00000001
#define ChipsAccelSupport	0x00000002
#define ChipsFullMMIOSupport	0x00000004
#define ChipsMMIOSupport	0x00000008
#define ChipsHDepthSupport	0x00000010
#define ChipsDPMSSupport	0x00000020
#define ChipsTMEDSupport	0x00000040
#define ChipsGammaSupport	0x00000080
#define ChipsVideoSupport	0x00000100
#define ChipsDualChannelSupport	0x00000200
#define ChipsDualRefresh	0x00000400
#define Chips64BitMemory	0x00000800

/* Options flags for the C&T chipsets */
#define ChipsHWCursor		0x00001000
#define ChipsShadowFB		0x00002000
#define ChipsUseNewFB		0x00008000

/* Architecture type flags */
#define ChipsHiQV		0x00010000
#define ChipsWingine		0x00020000
#define IS_Wingine(x)		((x->Flags) & ChipsWingine)
#define IS_HiQV(x)		((x->Flags) & ChipsHiQV)

/* Acceleration flags for the C&T chipsets */
#define ChipsColorTransparency	0x0100000
#define ChipsImageReadSupport	0x0200000

/* Overlay Transparency Key */
#define TRANSPARENCY_KEY 255

/* Flag Bus Types */
#define ChipsUnknown	0
#define ChipsISA	1
#define ChipsVLB	2
#define ChipsPCI	3
#define ChipsCPUDirect	4
#define ChipsPIB	5
#define ChipsMCB	6

/* Macro's to select the 32 bit acceleration registers */
#define DR(x) cPtr->Regs32[x]	/* For CT655xx naming scheme  */
#define MR(x) cPtr->Regs32[x]	/* CT655xx MMIO naming scheme */
#define BR(x) cPtr->Regs32[x]	/* For HiQV naming scheme     */
#define MMIOmeml(x) *(CARD32 *)(cPtr->MMIOBase + (x))
#if 0
#define MMIOmemw(x) *(CARD16 *)(cPtr->MMIOBase + (x))
#endif
/* Monitor or flat panel type flags */
#define ChipsCRT	0x0010
#define ChipsLCD	0x1000
#define ChipsLCDProbed	0x2000
#define ChipsTFT	0x0100
#define ChipsDS		0x0200
#define ChipsDD		0x0400
#define ChipsSS		0x0800
#define IS_STN(x)	((x) & 0xE00)

/* Dual channel register enable masks */
#define IOSS_MASK	0xE0
#define IOSS_BOTH	0x13
#define IOSS_PIPE_A	0x11
#define IOSS_PIPE_B	0x1E
#define MSS_MASK	0xF0
#define MSS_BOTH	0x0B
#define MSS_PIPE_A	0x02
#define MSS_PIPE_B	0x05
/* Aggregate value of MSS shadow bits -GHB */
#define MSS_SHADOW  0x07

/* Storage for the registers of the C&T chipsets */
typedef struct {
	unsigned char XR[0xFF];
	unsigned char CR[0x80];
	unsigned char FR[0x80];
	unsigned char MR[0x80];
	CHIPSClockReg Clock;
} CHIPSRegRec, *CHIPSRegPtr;

/* Storage for the flat panel size */
typedef struct {
    int HDisplay;
    int HRetraceStart;
    int HRetraceEnd;
    int HTotal;
    int VDisplay;
    int VRetraceStart;
    int VTotal;
} CHIPSPanelSizeRec, *CHIPSPanelSizePtr;

/* Some variables needed in the XAA acceleration */
typedef struct {
    /* General variable */ 
    unsigned int CommandFlags;
    unsigned int BytesPerPixel;
    unsigned int BitsPerPixel;
    unsigned int FbOffset;
    unsigned int PitchInBytes;
    unsigned int ScratchAddress;
    /* 64k for color expansion and imagewrites */
    unsigned char * BltDataWindow;
    /* Hardware cursor address */
    unsigned int CursorAddress;
    Bool UseHWCursor;
    /* Boundaries of the pixmap cache */
    unsigned int CacheStart;
    unsigned int CacheEnd;
    /* Storage for pattern mask */
    int planemask;
    int srcpitch, srcoffset, xdir, ydir;
    /* Storage for foreground and background color */
    int fgColor;
    int bgColor;
    /* For the 8x8 pattern fills */
    int patternyrot;
    /* For cached stipple fills */
    int SlotWidth;
    /* Variables for the 24bpp fill */
    unsigned char fgpixel;
    unsigned char bgpixel;
    unsigned char xorpixel;
    Bool fastfill;
    Bool rgb24equal;
    int fillindex;
    unsigned int width24bpp;
    unsigned int color24bpp;
    unsigned int rop24bpp;
} CHIPSACLRec, *CHIPSACLPtr;
#define CHIPSACLPTR(p)	&((CHIPSPtr)((p)->driverPrivate))->Accel

/* Storage for some register values that are messed up by suspend/resumes */
typedef struct {
    unsigned char xr02;
    unsigned char xr03;
    unsigned char xr14;
    unsigned char xr15;
    unsigned char vgaIOBaseFlag;
} CHIPSSuspendHackRec, *CHIPSSuspendHackPtr;

/* The functions to access the C&T extended registers */
typedef struct _CHIPSRec *CHIPSPtr;
typedef CARD8 (*chipsReadXRPtr)(CHIPSPtr cPtr, CARD8 index);
typedef void (*chipsWriteXRPtr)(CHIPSPtr cPtr, CARD8 index, CARD8 value);
typedef CARD8 (*chipsReadFRPtr)(CHIPSPtr cPtr, CARD8 index);
typedef void (*chipsWriteFRPtr)(CHIPSPtr cPtr, CARD8 index, CARD8 value);
typedef CARD8 (*chipsReadMRPtr)(CHIPSPtr cPtr, CARD8 index);
typedef void (*chipsWriteMRPtr)(CHIPSPtr cPtr, CARD8 index, CARD8 value);
typedef CARD8 (*chipsReadMSSPtr)(CHIPSPtr cPtr);
typedef void (*chipsWriteMSSPtr)(CHIPSPtr cPtr, vgaHWPtr hwp, CARD8 value);
typedef CARD8 (*chipsReadIOSSPtr)(CHIPSPtr cPtr);
typedef void (*chipsWriteIOSSPtr)(CHIPSPtr cPtr, CARD8 value);

/* The privates of the C&T driver */
#define CHIPSPTR(p)	((CHIPSPtr)((p)->driverPrivate))


typedef struct {
    int			lastInstance;
    int			refCount;
    CARD32		masterFbAddress;
    long		masterFbMapSize;
    CARD32		slaveFbAddress;
    long		slaveFbMapSize;
    int			mastervideoRam;
    int			slavevideoRam;
    Bool		masterOpen;
    Bool		slaveOpen;
    Bool		masterActive;
    Bool		slaveActive;
} CHIPSEntRec, *CHIPSEntPtr;


typedef struct _CHIPSRec {
    pciVideoPtr		PciInfo;
#ifndef XSERVER_LIBPCIACCESS
    PCITAG		PciTag;
#endif
    int			Chipset;
    EntityInfoPtr       pEnt;
    unsigned long	PIOBase;
    unsigned long	IOAddress;
    unsigned long	FbAddress;
    unsigned int	IOBase;
    unsigned char *	FbBase;
    unsigned char *	MMIOBase;
    unsigned char *	MMIOBaseVGA;
    unsigned char *	MMIOBasePipeA;
    unsigned char *	MMIOBasePipeB;
    long		FbMapSize;
    unsigned char *	ShadowPtr;
    int			ShadowPitch;
    int                 Rotate;
    void		(*PointerMoved)(SCRN_ARG_TYPE arg, int x, int y);
    int                 FbOffset16;
    int                 FbSize16;  
    OptionInfoPtr	Options;
    CHIPSPanelSizeRec	PanelSize;
    int			FrameBufferSize;
    Bool		SyncResetIgn;
    Bool		UseMMIO;
    Bool		UseFullMMIO;
    Bool		UseDualChannel;
    int			Monitor;
    int			MinClock;
    int			MaxClock;
    CHIPSClockReg	SaveClock;		/* Storage for ClockSelect */
    CHIPSMemClockReg	MemClock;
    unsigned char	ClockType;
    unsigned char	CRTClk[4];
    unsigned char       FPClk[4];
    int                 FPclock;
    int                 FPclkInx;
    int                 CRTclkInx;
    Bool                FPClkModified;
    int			ClockMulFactor;
    int			Rounding;
    CHIPSSuspendHackRec	SuspendHack;
    CARD32		PanelType;
    CHIPSRegRec		ModeReg;
    CHIPSRegRec		SavedReg;
    CHIPSRegRec		SavedReg2;
    vgaRegRec		VgaSavedReg2;
    unsigned int *	Regs32;
    unsigned int	Flags;
    CARD32		Bus;
    ExaDriverPtr 	pExa;
    xf86CursorInfoPtr	CursorInfoRec;
    CHIPSACLRec		Accel;
    unsigned int	HWCursorContents;
    Bool		HWCursorShown;
    DGAModePtr		DGAModes;
    int			numDGAModes;
    Bool		DGAactive;
    int			DGAViewportStatus;
    CloseScreenProcPtr	CloseScreen;
    ScreenBlockHandlerProcPtr BlockHandler;
    void		(*VideoTimerCallback)(ScrnInfoPtr, Time);
    int			videoKey;
    XF86VideoAdaptorPtr	adaptor;
    int			OverlaySkewX;
    int			OverlaySkewY;
    int			VideoZoomMax;
    Bool		SecondCrtc;
    CHIPSEntPtr		entityPrivate;
    unsigned char	storeMSS;
    unsigned char	storeIOSS;
#ifdef __arm__
#ifdef __NetBSD__
    int			TVMode;
#endif
    int			Bank;
#endif
    unsigned char       ddc_mask;
    I2CBusPtr           I2C;
    vbeInfoPtr          pVbe;
    chipsReadXRPtr	readXR;
    chipsWriteXRPtr	writeXR;
    chipsReadFRPtr	readFR;
    chipsWriteFRPtr	writeFR;
    chipsReadMRPtr	readMR;
    chipsWriteMRPtr	writeMR;
    chipsReadMSSPtr	readMSS;
    chipsWriteMSSPtr	writeMSS;
    chipsReadIOSSPtr	readIOSS;
    chipsWriteIOSSPtr	writeIOSS;
    Bool cursorDelay;
    unsigned int viewportMask;
    Bool dualEndianAp;
} CHIPSRec;

typedef struct _CHIPSi2c {
  unsigned char i2cClockBit;
  unsigned char i2cDataBit;
  CHIPSPtr cPtr;
} CHIPSI2CRec, *CHIPSI2CPtr;

/* External variables */
extern int ChipsAluConv[];
extern int ChipsAluConv2[];
extern int ChipsAluConv3[];
extern unsigned int ChipsReg32[];
extern unsigned int ChipsReg32HiQV[];

/* Prototypes */

void CHIPSAdjustFrame(ADJUST_FRAME_ARGS_DECL);
Bool CHIPSSwitchMode(SWITCH_MODE_ARGS_DECL);

/* video */
void CHIPSInitVideo(ScreenPtr pScreen);
void CHIPSResetVideo(ScrnInfoPtr pScrn);

/* banking */
int CHIPSSetRead(ScreenPtr pScreen, int bank);
int CHIPSSetWrite(ScreenPtr pScreen, int bank);
int CHIPSSetReadWrite(ScreenPtr pScreen, int bank);
int CHIPSSetReadPlanar(ScreenPtr pScreen, int bank);
int CHIPSSetWritePlanar(ScreenPtr pScreen, int bank);
int CHIPSSetReadWritePlanar(ScreenPtr pScreen, int bank);
int CHIPSWINSetRead(ScreenPtr pScreen, int bank);
int CHIPSWINSetWrite(ScreenPtr pScreen, int bank);
int CHIPSWINSetReadWrite(ScreenPtr pScreen, int bank);
int CHIPSWINSetReadPlanar(ScreenPtr pScreen, int bank);
int CHIPSWINSetWritePlanar(ScreenPtr pScreen, int bank);
int CHIPSWINSetReadWritePlanar(ScreenPtr pScreen, int bank);
int CHIPSHiQVSetReadWrite(ScreenPtr pScreen, int bank);
int CHIPSHiQVSetReadWritePlanar(ScreenPtr pScreen, int bank);

/* acceleration */
Bool CHIPSAccelInit(ScreenPtr pScreen);
void CHIPSSync(ScrnInfoPtr pScrn);
Bool CHIPSMMIOAccelInit(ScreenPtr pScreen);
void CHIPSMMIOSync(ScrnInfoPtr pScrn);
Bool CHIPSHiQVAccelInit(ScreenPtr pScreen);
void CHIPSHiQVSync(ScrnInfoPtr pScrn);
Bool CHIPSCursorInit(ScreenPtr pScreen);
Bool CHIPSInitEXA(ScreenPtr pScreen);

/* register access functions */
void CHIPSSetStdExtFuncs(CHIPSPtr cPtr);
void CHIPSSetMmioExtFuncs(CHIPSPtr cPtr);
void CHIPSHWSetMmioFuncs(ScrnInfoPtr pScrn, CARD8 *base, int offset);

/* ddc */
extern void chips_ddc1(ScrnInfoPtr pScrn);
extern Bool chips_i2cInit(ScrnInfoPtr pScrn);

/* dga */
Bool CHIPSDGAInit(ScreenPtr pScreen);

/* shadow fb */
void     chipsRefreshArea(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void     chipsRefreshArea8(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void     chipsRefreshArea16(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void     chipsRefreshArea24(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void     chipsRefreshArea32(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void     chipsPointerMoved(SCRN_ARG_TYPE arg, int x, int y);

#if X_BYTE_ORDER == X_BIG_ENDIAN
# define BE_SWAP_APRETURE(pScrn,cPtr) \
           ((pScrn->bitsPerPixel == 16) && cPtr->dualEndianAp)
#endif

/*
 * Some macros for switching display channels. NOTE... It appears that we
 * can't write to both display channels at the same time, and so the options
 * MSS_BOTH and IOSS_BOTH should not be used. Need to get around this by set
 * dual channel mode to pipe A by default and handling multiple channel writes
 * in ModeInit..
 */

#define DUALOPEN \
    {									      \
        /* Set the IOSS/MSS registers to point to the right register set */   \
	if (xf86IsEntityShared(pScrn->entityList[0])) { 		      \
	    if (cPtr->SecondCrtc == TRUE) {				      \
		cPtr->writeIOSS(cPtr, ((cPtr->storeIOSS & IOSS_MASK) |	      \
					IOSS_PIPE_B));			      \
		cPtr->writeMSS(cPtr, VGAHWPTR(pScrn), ((cPtr->storeMSS &      \
					MSS_MASK) | MSS_PIPE_B));	      \
		cPtrEnt->slaveOpen = TRUE;				      \
		cPtrEnt->slaveActive = TRUE;				      \
		cPtrEnt->masterActive = FALSE;				      \
	    } else {							      \
		cPtr->writeIOSS(cPtr, ((cPtr->storeIOSS & IOSS_MASK) |	      \
					IOSS_PIPE_A));			      \
		cPtr->writeMSS(cPtr, VGAHWPTR(pScrn), ((cPtr->storeMSS &      \
					MSS_MASK) | MSS_PIPE_A));	      \
		cPtrEnt->masterOpen = TRUE;				      \
		cPtrEnt->masterActive = TRUE;				      \
		cPtrEnt->slaveActive = FALSE;				      \
	    }								      \
	} else {							      \
	    cPtr->writeIOSS(cPtr, ((cPtr->storeIOSS & IOSS_MASK) | 	      \
					IOSS_PIPE_A));			      \
	    cPtr->writeMSS(cPtr, VGAHWPTR(pScrn), ((cPtr->storeMSS &	      \
					MSS_MASK) | MSS_PIPE_A));	      \
	}								      \
    }

#define DUALREOPEN							      \
    {									      \
	if (xf86IsEntityShared(pScrn->entityList[0])) { 		      \
	    if (cPtr->SecondCrtc == TRUE) {				      \
		if (! cPtrEnt->slaveActive) {				      \
		    cPtr->writeIOSS(cPtr, ((cPtr->storeIOSS & IOSS_MASK) |    \
					IOSS_PIPE_B));			      \
		    cPtr->writeMSS(cPtr, VGAHWPTR(pScrn), ((cPtr->storeMSS &  \
					MSS_MASK) | MSS_PIPE_B));	      \
		    cPtrEnt->slaveOpen = TRUE;				      \
		    cPtrEnt->slaveActive = TRUE;			      \
		    cPtrEnt->masterActive = FALSE;			      \
		}							      \
	    } else {							      \
		if (! cPtrEnt->masterActive) {				      \
		    cPtr->writeIOSS(cPtr, ((cPtr->storeIOSS & IOSS_MASK) |    \
					IOSS_PIPE_A));			      \
		    cPtr->writeMSS(cPtr, VGAHWPTR(pScrn), ((cPtr->storeMSS &  \
					MSS_MASK) | MSS_PIPE_A));	      \
		    cPtrEnt->masterOpen = TRUE;				      \
		    cPtrEnt->masterActive = TRUE;			      \
		    cPtrEnt->slaveActive = FALSE;			      \
		}							      \
	    }								      \
	}								      \
    }

#define DUALCLOSE							      \
    {									      \
	if (! xf86IsEntityShared(pScrn->entityList[0])) {		      \
	    cPtr->writeIOSS(cPtr, ((cPtr->storeIOSS & IOSS_MASK) |	      \
			       IOSS_PIPE_A));				      \
	    cPtr->writeMSS(cPtr, VGAHWPTR(pScrn), ((cPtr->storeMSS &	      \
				MSS_MASK) | MSS_PIPE_A));		      \
	    chipsHWCursorOff(cPtr, pScrn);				      \
	    chipsRestore(pScrn, &(VGAHWPTR(pScrn))->SavedReg,		      \
				&cPtr->SavedReg, TRUE);			      \
	    chipsLock(pScrn);						      \
	    cPtr->writeIOSS(cPtr, ((cPtr->storeIOSS & IOSS_MASK) |	      \
			       IOSS_PIPE_B));				      \
	    cPtr->writeMSS(cPtr, VGAHWPTR(pScrn), ((cPtr->storeMSS &	      \
				MSS_MASK) | MSS_PIPE_B)); 		      \
	    chipsHWCursorOff(cPtr, pScrn);				      \
	    chipsRestore(pScrn, &cPtr->VgaSavedReg2, &cPtr->SavedReg2, TRUE); \
	    cPtr->writeIOSS(cPtr, cPtr->storeIOSS);			      \
	    cPtr->writeMSS(cPtr, VGAHWPTR(pScrn), cPtr->storeMSS);	      \
	    chipsLock(pScrn);						      \
	} else {							      \
	    chipsHWCursorOff(cPtr, pScrn);				      \
	    chipsRestore(pScrn, &(VGAHWPTR(pScrn))->SavedReg, &cPtr->SavedReg,\
					TRUE);				      \
	    if (cPtr->SecondCrtc == TRUE) {				      \
		cPtrEnt->slaveActive = FALSE;				      \
		cPtrEnt->slaveOpen = FALSE;				      \
		if (! cPtrEnt->masterActive) {				      \
		    cPtr->writeIOSS(cPtr, cPtr->storeIOSS);		      \
		    cPtr->writeMSS(cPtr, VGAHWPTR(pScrn), cPtr->storeMSS);    \
		    chipsLock(pScrn);					      \
		}							      \
	    } else {							      \
		cPtrEnt->masterActive = FALSE;				      \
		cPtrEnt->masterOpen = FALSE;				      \
		if (! cPtrEnt->slaveActive) {				      \
		    cPtr->writeIOSS(cPtr, cPtr->storeIOSS);		      \
		    cPtr->writeMSS(cPtr, VGAHWPTR(pScrn), cPtr->storeMSS);    \
		    chipsLock(pScrn);					      \
		}							      \
	    }								      \
	}								      \
    }


/* To aid debugging of 32 bit register access we make the following defines */
/*
#define DEBUG
#define CT_HW_DEBUG 
*/
#if defined(DEBUG) & defined(CT_HW_DEBUG)
#define HW_DEBUG(x) {usleep(500000); ErrorF("Register/Address: 0x%X\n",x);}
#else
#define HW_DEBUG(x) 
#endif
#endif
