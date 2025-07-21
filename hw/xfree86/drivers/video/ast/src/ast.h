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

#ifndef _AST_H_
# define _AST_H_

/* Compiler Options */
#define	Accel_2D
/* #define MMIO_2D */
#define HWC
#define AstVideo
/* #define PATCH_ABI_VERSION */
#define Support_ShadowFB

#include <string.h>
#include <stdlib.h>

#include "ast_pcirename.h"

#ifdef	Support_ShadowFB
#include "shadow.h"
#endif

/* Vendor & Device Info */
#ifndef PCI_VENDOR_AST
#define PCI_VENDOR_AST			0x1A03
#endif

#ifndef	PCI_CHIP_AST2000
#define PCI_CHIP_AST2000		0x2000
#endif

#ifndef	PCI_CHIP_AST2100
#define PCI_CHIP_AST2100		0x2010
#endif

#ifndef	PCI_CHIP_AST1180
#define PCI_CHIP_AST1180		0x1180
#endif

typedef enum _CHIP_ID {
    VGALegacy,
    AST2000,
    AST2100,
    AST1100,
    AST2200,
    AST2150,
    AST2300,
    AST2400,
    AST2500,
    AST1180
} CHIP_ID;

typedef enum _TX_CHIPTYPE {
    Tx_NONE,
    Tx_Sil164,
    Tx_Ite66121,
    Tx_DP501
} TX_CHIPTYPE;

/* AST REC Info */
#define AST_NAME 			"AST"
#define AST_DRIVER_NAME 		"ast"
#define AST_MAJOR_VERSION 		PACKAGE_VERSION_MAJOR
#define AST_MINOR_VERSION 		PACKAGE_VERSION_MINOR
#define AST_PATCH_VERSION		PACKAGE_VERSION_PATCHLEVEL
#define AST_VERSION	\
        ((AST_MAJOR_VERSION << 20) | (AST_MINOR_VERSION << 10) | AST_PATCH_VERSION)

/* Customized Info */
#define DEFAULT_VIDEOMEM_SIZE		0x00800000
#define DEFAULT_MMIO_SIZE		0x00020000
#define DEFAULT_CMDQ_SIZE		0x00100000
#define MIN_CMDQ_SIZE			0x00040000
#define CMD_QUEUE_GUARD_BAND    	0x00000020
#define DEFAULT_HWC_NUM			0x00000002

/* Customized Info. for DVO */
#define HDMI_I2C_CHANNEL		1
#define HDMI_TX_I2C_SLAVE_ADDR 		0x98

/* Patch Info */
#define ABI_VIDEODRV_VERSION_PATCH	SET_ABI_VERSION(0, 5)

/* Data Type Definition */
typedef INT32  		LONG;
typedef CARD8   	UCHAR;
typedef CARD16  	USHORT;
typedef CARD32  	ULONG;

/* Data Structure Definition */
typedef struct _ASTRegRec {

    UCHAR 	ExtCRTC[0x50];

    UCHAR 	MISC;
    UCHAR 	SEQ[4];
    UCHAR 	CRTC[25];
    UCHAR 	AR[20];
    UCHAR 	GR[9];
    UCHAR	DAC[256][3];

    ULONG	GFX[12];

    UCHAR	REGA4;
    ULONG	ENG8044;
} ASTRegRec, *ASTRegPtr;

typedef struct _VIDEOMODE {

    int			ScreenWidth;
    int			ScreenHeight;
    int			bitsPerPixel;
    int			ScreenPitch;

} VIDEOMODE, *PVIDEOMODE;

typedef struct {

    ULONG		ulCMDQSize;
    ULONG		ulCMDQType;

    ULONG		ulCMDQOffsetAddr;
    UCHAR       *pjCMDQVirtualAddr;

    UCHAR       *pjCmdQBasePort;
    UCHAR       *pjWritePort;
    UCHAR       *pjReadPort;
    UCHAR       *pjEngStatePort;

    ULONG		ulCMDQMask;
    ULONG		ulCurCMDQueueLen;

    ULONG		ulWritePointer;
    ULONG		ulReadPointer;

    ULONG		ulReadPointer_OK;		/* for Eng_DBGChk */

} CMDQINFO, *PCMDQINFO;

typedef struct {

    int			HWC_NUM;
    int			HWC_NUM_Next;

    ULONG		ulHWCOffsetAddr;
    UCHAR       *pjHWCVirtualAddr;

    USHORT		cursortype;
    USHORT		width;
    USHORT   	height;
    USHORT		offset_x;
    USHORT   	offset_y;
    ULONG		fg;
    ULONG		bg;

    UCHAR       cursorpattern[1024];

} HWCINFO, *PHWCINFO;

typedef struct _ASTPortPrivRec{
    FBLinearPtr     linear;
    FBAreaPtr       fbAreaPtr;
    int             fbSize;
    CARD32          bufAddr[2];

    unsigned char   currentBuf;

    short           drw_x, drw_y, drw_w, drw_h;
    short           src_x, src_y, src_w, src_h;
    int             id;
    short           srcPitch, height;

    INT32           brightness;
    INT32           contrast;
    INT32           saturation;
    INT32           hue;

    INT32           gammaR;
    INT32           gammaG;
    INT32           gammaB;

    RegionRec       clip;
    CARD32          colorKey;

    CARD32          videoStatus;
    Time            offTime;
    Time            freeTime;

    CARD32          displayMode;

    int             pitch;
    int             offset;
} ASTPortPrivRec, *ASTPortPrivPtr;

typedef struct _ASTRec {

    EntityInfoPtr 	pEnt;
#ifndef XSERVER_LIBPCIACCESS
	pciVideoPtr		PciInfo;
	PCITAG			PciTag;
#else
	struct pci_device       *PciInfo;
#endif

    OptionInfoPtr 	Options;
    DisplayModePtr  ModePtr;
    FBLinearPtr 	pCMDQPtr;
    xf86CursorInfoPtr   HWCInfoPtr;
    FBLinearPtr 	pHWCPtr;

    CloseScreenProcPtr CloseScreen;
    ScreenBlockHandlerProcPtr BlockHandler;

    UCHAR		jChipType;
    UCHAR		jDRAMType;
    ULONG		ulDRAMBusWidth  ;
    ULONG		ulDRAMSize;
    ULONG		ulVRAMSize;
    ULONG		ulVRAMBase;
    ULONG       ulMCLK;

    Bool 		noAccel;
    Bool 		noHWC;
    Bool 		MMIO2D;
    int			ENGCaps;
    int			DBGSelect;
    Bool		VGA2Clone;
    Bool		SupportWideScreen;

    ULONG     		FBPhysAddr;			/* Frame buffer physical address     */
    ULONG     		MMIOPhysAddr;     	/* MMIO region physical address      */
    ULONG     		BIOSPhysAddr;     	/* BIOS physical address             */

    UCHAR     		*FBVirtualAddr;   	/* Map of frame buffer               */
    UCHAR     		*MMIOVirtualAddr; 	/* Map of MMIO region                */

    unsigned long	FbMapSize;
    unsigned long	MMIOMapSize;

    int			RelocateIO;

    VIDEOMODE 		VideoModeInfo;
    ASTRegRec       SavedReg;
    CMDQINFO		CMDQInfo;
    HWCINFO    		HWCInfo;
    ULONG			ulCMDReg;
    Bool			EnableClip;

    int				clip_left;
    int				clip_top;
    int				clip_right;
    int				clip_bottom;

    int				mon_h_active;		/* Monitor Info. */
    int				mon_v_active;

    UCHAR			jTxChipType;		/* 3rd TX */
    UCHAR			DP501_MaxVCLK;
    UCHAR			*pDP501FWBufferVirtualAddress;

#ifdef AstVideo
    XF86VideoAdaptorPtr adaptor;
    Atom        	xvBrightness, xvContrast, xvColorKey, xvHue, xvSaturation;
    Atom			xvGammaRed, xvGammaGreen, xvGammaBlue;
#endif

#ifdef	Support_ShadowFB
    Bool			shadowFB;
    Bool			shadowFB_validation;
    void            *shadow;
    ShadowUpdateProc update;
    ShadowWindowProc window;
    CreateScreenResourcesProcPtr CreateScreenResources;
#endif

} ASTRec, *ASTRecPtr, *ASTPtr;

#define ASTPTR(p) ((ASTRecPtr)((p)->driverPrivate))

/* ast_vgatool.c */
extern Bool bASTIsVGAEnabled(ScrnInfoPtr pScrn);
extern Bool ASTGetVGA2EDID(ScrnInfoPtr pScrn, unsigned char *pEDIDBuffer);
extern void ASTDisplayPowerManagementSet(ScrnInfoPtr pScrn, int PowerManagementMode, int flags);
extern void vASTLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices, LOCO *colors, VisualPtr pVisual);
extern void ASTBlankScreen(ScrnInfoPtr pScreen, Bool unblack);
extern void vAST1000DisplayOn(ScrnInfoPtr pScrn);
extern void vAST1000DisplayOff(ScrnInfoPtr pScrn);
extern void vASTSetStartAddressCRT1(ASTRecPtr pAST, ULONG base);
extern void ASTGetScratchOptions(ScrnInfoPtr pScrn);
void ASTGetChipType(ScrnInfoPtr pScrn);
ULONG ASTGetMaxDCLK(ScrnInfoPtr pScrn);
ULONG ASTGetVRAMInfo(ScrnInfoPtr pScrn);
void ASTGetDRAMInfo(ScrnInfoPtr pScrn);
Bool bASTRegInit(ScrnInfoPtr pScrn);
void vASTOpenKey(ScrnInfoPtr pScrn);
Bool ASTReadEDID_M68K(ScrnInfoPtr pScrn, BYTE *pEDIDData);
UCHAR ASTGetLinkMaxCLK(ScrnInfoPtr pScrn);
Bool ASTGetVGAEDID(ScrnInfoPtr pScrn, unsigned char *pEDIDBuffer);
Bool bASTInitAST1180(ScrnInfoPtr pScrn);
void ASTGetAST1180DRAMInfo(ScrnInfoPtr pScrn);
void vASTEnableVGAMMIO(ScrnInfoPtr pScrn);
Bool ASTInitVGA(ScrnInfoPtr pScrn, ULONG Flags);

/* ast_2dtool.c */
void vASTWaitEngIdle(ScrnInfoPtr pScrn, ASTRecPtr pAST);
UCHAR *pASTjRequestCMDQ(ASTRecPtr pAST, ULONG ulDataLen);
Bool bASTEnable2D(ScrnInfoPtr pScrn, ASTRecPtr pAST);
void vASTDisable2D(ScrnInfoPtr pScrn, ASTRecPtr pAST);

/* ast_cursor.c */
#ifdef  HWC
Bool ASTCursorInit(ScreenPtr pScreen);
Bool bASTInitHWC(ScrnInfoPtr pScrn, ASTRecPtr pAST);
void ASTDisableHWC(ScrnInfoPtr pScrn);
#endif

/* ast_mode.c */
Bool ASTSetMode(ScrnInfoPtr pScrn, DisplayModePtr mode);

/* ast_accel.c */
void ASTDisplayVideo(ScrnInfoPtr pScrn, ASTPortPrivPtr pPriv, RegionPtr clipBoxes, int id);

/* ast_tool.c */
Bool ASTMapMem(ScrnInfoPtr pScrn);
Bool ASTUnmapMem(ScrnInfoPtr pScrn);
Bool ASTMapMMIO(ScrnInfoPtr pScrn);
void ASTUnmapMMIO(ScrnInfoPtr pScrn);

#endif /* _AST_H_ */
