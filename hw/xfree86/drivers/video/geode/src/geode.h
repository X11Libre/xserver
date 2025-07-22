 /*
  * (c) 2006-2008 Advanced Micro Devices, Inc.
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
  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  * DEALINGS IN THE SOFTWARE.
  *
  * Neither the name of the Advanced Micro Devices, Inc. nor the names of its
  * contributors may be used to endorse or promote products derived from this
  * software without specific prior written permission.
  */

#ifndef _GEODE_H_
#define _GEODE_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "geode_pcirename.h"

#include "exa.h"
#include "xf86Cursor.h"

#include "vgaHW.h"
#include "xf86int10.h"
#include <X11/extensions/randr.h>
#include "randrstr.h"

#include "xf86xv.h"

#include "compat-api.h"

/* We only support EXA version >=2 */

#if (EXA_VERSION_MAJOR >= 2)
#ifndef XF86EXA
#define XF86EXA 1
#endif
#else
#undef XF86EXA
#endif

#define CFB 0
#define INT10_SUPPORT 1

/* Existing Processor Models */
#define GX1     0x01
#define GX      0x10
#define LX      0x20

#define PCI_VENDOR_ID_CYRIX  0x1078
#define PCI_VENDOR_ID_NS     0x100b
#define PCI_VENDOR_ID_AMD    0x1022

#define PCI_CHIP_5530     0x0104
#define PCI_CHIP_SC1200   0x0504
#define PCI_CHIP_SC1400   0x0104
#define PCI_CHIP_GEODEGX  0x0030
#define PCI_CHIP_GEODELX  0x2081

#define GFX_CPU_GEODELX   4

#define GFX(func) gfx_##func
#define GFX2(func) gfx2_##func

#define GEODEPTR(p) ((GeodeRec *)((p)->driverPrivate))

#define DEFAULT_EXA_SCRATCH_BFRSZ 0x40000

/* the LX hardware (and Cimarron) require a cursor memory size of 48x64.. */
#define LX_CURSOR_HW_WIDTH 48
#define LX_CURSOR_HW_HEIGHT 64

/* ...but xorg requires the cursor width to be a multiple of 32.  This is
the size of the *actual* cursor. */
#define LX_CURSOR_MAX_WIDTH 32
#define LX_CURSOR_MAX_HEIGHT 32

/* FIXME: Get rid of this from GX */

#define DEBUGMSG(cond, drv_msg)

#ifdef HAVE_LX
extern unsigned char cim_inb(unsigned short port);
extern void cim_outb(unsigned short port, unsigned char data);
extern unsigned short cim_inw(unsigned short port);
extern void cim_outw(unsigned short port, unsigned short data);
extern void cim_outd(unsigned short port, unsigned long data);

#include "cim_rtns.h"

typedef struct _CIM_DISPLAYTIMING {
    unsigned short wPitch;
    unsigned short wBpp;
    VG_DISPLAY_MODE vgDisplayMode;
} CIM_DISPLAYTIMING;

#endif

extern void gfx_write_reg32(int offset, int data);
extern void gfx_write_reg16(int offset, short data);
extern void gfx_write_reg8(int offset, char data);
extern int gfx_read_reg32(int offset);
extern short gfx_read_reg16(int offset);
extern void gfx_write_vid32(int offset, int data);
extern int gfx_read_vid32(int offset);
extern unsigned char gfx_inb(unsigned short port);
extern void gfx_outb(unsigned short port, unsigned char data);
extern unsigned short gfx_inw(unsigned short port);
extern void gfx_outw(unsigned short port, unsigned short data);
extern unsigned long gfx_ind(unsigned short port);
extern void gfx_outd(unsigned short port, unsigned long data);

#define Q_WORD QQ_WORD
#include "gfx_rtns.h"
#undef Q_WORD
#include "panel.h"

typedef struct tag_GFX_DISPLAYTIMING {
    unsigned int dwDotClock;
    unsigned short wPitch;
    unsigned short wBpp;
    unsigned short wHTotal;
    unsigned short wHActive;
    unsigned short wHSyncStart;
    unsigned short wHSyncEnd;
    unsigned short wHBlankStart;
    unsigned short wHBlankEnd;
    unsigned short wVTotal;
    unsigned short wVActive;
    unsigned short wVSyncStart;
    unsigned short wVSyncEnd;
    unsigned short wVBlankStart;
    unsigned short wVBlankEnd;
    unsigned short wPolarity;
} GFX_DISPLAYTIMING;

typedef struct _VESARec {
    xf86Int10InfoPtr pInt;
} VESARec;

typedef struct _GeodeMemRec {
    struct _GeodeMemRec *next;
    struct _GeodeMemRec *prev;
    unsigned int offset;
    int size;
} GeodeMemRec, *GeodeMemPtr;

#define OUTPUT_PANEL 0x01
#define OUTPUT_CRT   0x02
#define OUTPUT_TV    0x04
#define OUTPUT_VOP   0x08
#define OUTPUT_DCON  0x10

typedef struct _geodeRec {
    /* Common for both GX and LX */
    int Output;                 /* Bitmask indicating the valid output options */
    Bool HWCursor;
    Bool NoAccel;
    Bool useVGA;
    Bool VGAActive;             /* Flag indicating if LX VGA is active */
    Bool Compression;
    Bool useEXA;

    int rotation;
    int displayWidth;
    Bool starting;
    Bool tryCompression;
    Bool tryHWCursor;

    int mm_width, mm_height;    /* physical display size */

    unsigned long CursorStartOffset;

    int Pitch;                  /* display FB pitch */
    int displaySize;            /* The size of the visible area */

    ExaOffscreenArea *shadowArea;

    /* Framebuffer memory */

    unsigned char *FBBase;
    unsigned int FBAvail;

    /* EXA structures */

    ExaDriverPtr pExa;
    unsigned int exaBfrOffset;
    unsigned int exaBfrSz;

    /* Other structures */

    EntityInfoPtr pEnt;
    ScreenBlockHandlerProcPtr BlockHandler;     /* needed for video */
    XF86VideoAdaptorPtr adaptor;

    /* State save structures */
    unsigned long FBDisplayOffset;
    unsigned long PrevDisplayOffset;

    VESARec *vesa;

    int FBCompressionEnable;

    VG_CURSOR_DATA FBCursor;

    /* Save the Cursor offset of the FB */
    unsigned long FBCursorOffset;
    unsigned char FBBIOSMode;

    /* Hooks */

    void (*PointerMoved)(ScrnInfoPtr pScrn, int x, int y);
    CloseScreenProcPtr CloseScreen;
    Bool (*CreateScreenResources)(ScreenPtr);

    /* ===== LX specific items ===== */

    /* Flags */
    Bool Scale;

    DisplayModePtr panelMode;   /* The mode for the panel (if attached) */

    /* Command buffer information */
    unsigned long CmdBfrOffset;
    unsigned long CmdBfrSize;

    /* Memory Management */
    GeodeMemPtr offscreenList;
    unsigned int offscreenStart;
    unsigned int offscreenSize;

    /* Save state */
    VG_COMPRESSION_DATA CBData;
    VG_COMPRESSION_DATA FBCBData;
    CIM_DISPLAYTIMING FBcimdisplaytiming;

    /* ===== GX specific items ===== */

#ifdef HAVE_GX
    /* Flags */
    Bool Panel;

    int PanelX;
    int PanelY;

    int displayPitch;           /* The pitch of the visible area */
    int displayOffset;          /* The offset of the visible area */

    DisplayModePtr curMode;

    unsigned int shadowSize;
    unsigned int shadowOffset;

    int FBVGAActive;

    xf86CursorInfoPtr CursorInfo;

    unsigned int cpySrcOffset;
    int cpySrcPitch, cpySrcBpp;
    int cpyDx, cpyDy;
    unsigned int cmpSrcOffset;
    int cmpSrcPitch, cmpSrcBpp;
    unsigned int cmpSrcFmt, cmpDstFmt;
    int cmpOp;

    /* Save state */
    unsigned long FBCompressionOffset;
    unsigned short FBCompressionPitch;
    unsigned short FBCompressionSize;
    GFX_DISPLAYTIMING FBgfxdisplaytiming;

    /* Hooks */
    void (*WritePixmap)(ScrnInfoPtr pScrni, int x, int y, int w, int h,
                        unsigned char *src, int srcwidth, int rop,
                        unsigned int planemask, int trans, int bpp, int depth);

    /* Video information */
    int video_x;
    int video_y;
    short video_w;
    short video_h;
    short video_srcw;
    short video_srch;
    short video_dstw;
    short video_dsth;
    int video_id;
    int video_offset;
    ScrnInfoPtr video_scrnptr;
    BOOL OverlayON;
#endif
} GeodeRec, *GeodePtr;

/* option flags are self-explanatory */
#ifdef HAVE_LX
enum LX_GeodeOpts {
    LX_OPTION_SW_CURSOR,
    LX_OPTION_HW_CURSOR,
    LX_OPTION_NOCOMPRESSION,
    LX_OPTION_NOACCEL,
    LX_OPTION_EXA_SCRATCH_BFRSZ,
    LX_OPTION_TV_SUPPORT,
    LX_OPTION_TV_OUTPUT,
    LX_OPTION_TV_OVERSCAN,
    LX_OPTION_ROTATE,
    LX_OPTION_NOPANEL,
    LX_OPTION_FBSIZE,
    LX_OPTION_PANEL_MODE,
    LX_OPTION_DONT_PROGRAM
};
#endif

#ifdef HAVE_GX
enum GX_GeodeOpts {
    GX_OPTION_SW_CURSOR,
    GX_OPTION_HW_CURSOR,
    GX_OPTION_NOCOMPRESSION,
    GX_OPTION_NOACCEL,
    GX_OPTION_ACCEL_METHOD,
    GX_OPTION_EXA_SCRATCH_BFRSZ,
    GX_OPTION_TV_SUPPORT,
    GX_OPTION_TV_OUTPUT,
    GX_OPTION_TV_OVERSCAN,
    GX_OPTION_ROTATE,
    GX_OPTION_NOPANEL,
    GX_OPTION_OSM_IMG_BUFS,
    GX_OPTION_OSM_CLR_BUFS,
    GX_OPTION_FBSIZE,
    GX_OPTION_PANEL_GEOMETRY,
    GX_OPTION_DONT_PROGRAM
};
#endif

/* geode_dcon.c */
extern Bool dcon_init(ScrnInfoPtr pScrni);
extern int DCONDPMSSet(ScrnInfoPtr pScrni, int mode);

/* geode_common.c */

void geode_memory_to_screen_blt(unsigned long, unsigned long,
                                unsigned long, unsigned long, long, long, int);
int GeodeGetRefreshRate(DisplayModePtr);
void GeodeCopyGreyscale(unsigned char *, unsigned char *, int, int, int, int);
int GeodeGetSizeFromFB(unsigned int *);

/* gx_video.c */

int

GeodeQueryImageAttributes(ScrnInfoPtr, int id, unsigned short *w,
                          unsigned short *h, int *pitches, int *offsets);

Bool RegionsEqual(RegionPtr A, RegionPtr B);

/* gx_driver.c */

void GeodeProbeDDC(ScrnInfoPtr pScrni, int index);
xf86MonPtr GeodeDoDDC(ScrnInfoPtr pScrni, int index);
Bool GeodeI2CInit(ScrnInfoPtr pScrni, I2CBusPtr * ptr, const char *name);

int GeodeGetFPGeometry(const char *str, int *width, int *height);
void GeodePointerMoved(ScrnInfoPtr pScrn, int x, int y);
void GeodeFreeScreen(ScrnInfoPtr pScrn);
int GeodeCalculatePitchBytes(unsigned int width, unsigned int bpp);
void GXSetupChipsetFPtr(ScrnInfoPtr pScrn);

/* geode_msr.c */
int GeodeReadMSR(uint32_t addr, uint32_t * lo, uint32_t * hi);
int GeodeWriteMSR(uint32_t addr, uint32_t lo, uint32_t hi);

/* gx_cursor.c */
Bool GXHWCursorInit(ScreenPtr pScrn);
void GXLoadCursorImage(ScrnInfoPtr pScrni, unsigned char *src);
void GXHideCursor(ScrnInfoPtr pScrni);
void GXShowCursor(ScrnInfoPtr pScrni);

/* gx_randr.c */
Rotation GXGetRotation(ScreenPtr pScreen);
Bool GXRandRInit(ScreenPtr pScreen, int rotation);
extern _X_EXPORT Bool GXRandRSetConfig(ScreenPtr pScreen, Rotation rotation,
                                       int rate, RRScreenSizePtr pSize);

/* gx_rotate.c */
Bool GXRotate(ScrnInfoPtr pScrni, DisplayModePtr mode);

/* gx_exa.c */
Bool GXAccelInit(ScreenPtr pScrn);
void GXAccelSync(ScrnInfoPtr pScrni);

/* gx_video.c */
void GXInitVideo(ScreenPtr pScrn);

/* lx_display.c */
void lx_enable_dac_power(ScrnInfoPtr pScrni, int option);
void lx_disable_dac_power(ScrnInfoPtr pScrni, int option);

/* lx_driver.c */
void LXSetupChipsetFPtr(ScrnInfoPtr pScrn);

/* lx_cursor.c */
Bool LXHWCursorInit(ScreenPtr pScrn);
void LXLoadARGBCursorImage(ScrnInfoPtr pScrni, unsigned char *src);
void LXHideCursor(ScrnInfoPtr pScrni);
void LXShowCursor(ScrnInfoPtr pScrni);

/* lx_exa.c */
Bool LXExaInit(ScreenPtr pScreen);

/* lx_video.c */
void LXInitVideo(ScreenPtr pScrn);

/* lx_output.c */
void LXSetupOutput(ScrnInfoPtr);

/* lx_panel.c */
DisplayModePtr LXGetLegacyPanelMode(ScrnInfoPtr pScrni);
DisplayModePtr LXGetManualPanelMode(const char *modestr);

void LXAdjustFrame(ScrnInfoPtr pScrni, int x, int y);

/* lx_display.c */
void LXSetupCrtc(ScrnInfoPtr pScrni);

/* lx_memory.c */
GeodeMemPtr GeodeAllocOffscreen(GeodeRec * pGeode, int size, int align);
void GeodeFreeOffscreen(GeodeRec * pGeode, GeodeMemPtr ptr);
void LXInitOffscreen(ScrnInfoPtr pScrni);
void GeodeCloseOffscreen(ScrnInfoPtr pScrni);
unsigned int GeodeOffscreenFreeSize(GeodeRec * pGeode);

/* lx_cursor.c */
Bool LXCursorInit(ScreenPtr pScrn);

#endif
