/* (c) Itai Nahshon */

#ifndef CIR_H
#define CIR_H

#include "xf86Cursor.h"
#include "xf86i2c.h"
#include <string.h>
#include <stdlib.h>

#include "cir_pcirename.h"

/* Card-specific driver information */
#define CIRPTR(p) ((CirPtr)((p)->driverPrivate))
struct lgRec;
struct alpRec;

typedef struct {
    ScrnInfoPtr pScrn;
    CARD32 properties;
    pciVideoPtr PciInfo;
#ifndef XSERVER_LIBPCIACCESS
    PCITAG PciTag;
#endif
    unsigned long PIOReg;
    union {
        struct lgRec *lg;
        struct alpRec *alp;
    } chip;
    EntityInfoPtr pEnt;
    int Chipset;
    int ChipRev;
    int Rounding;
    int BppShift;
    Bool HasFBitBlt;
    CARD32 IOAddress;
    CARD32 FbAddress;
    unsigned char *IOBase;
    unsigned char *FbBase;
    long FbMapSize;
    long IoMapSize;
    int MinClock;
    int MaxClock;
    Bool NoAccel;
    Bool HWCursor;
    Bool UseMMIO;
    xf86CursorInfoPtr CursorInfoRec;
    int DGAnumModes;
    DGAModePtr DGAModes;
    Bool DGAactive;
    Bool (*DGAModeInit)(ScrnInfoPtr, DisplayModePtr);
    int DGAViewportStatus;
    I2CBusPtr I2CPtr1;
    I2CBusPtr I2CPtr2;
    CloseScreenProcPtr CloseScreen;

    Bool CursorIsSkewed;
    Bool shadowFB;
    int rotate;
    int ShadowPitch;
    unsigned char * ShadowPtr;
    void (*PointerMoved)(ScrnInfoPtr arg, int x, int y);
    int pitch;

    unsigned char ** ScanlineColorExpandBuffers;
    void (*InitAccel)(ScrnInfoPtr);
    int offscreen_size;
    int offscreen_offset;
    OptionInfoPtr Options;
} CirRec, *CirPtr;

/* CirrusClk.c */
extern Bool
CirrusFindClock(int *rfreq, int max_clock, int *num_out, int *den_out);

/* cir_driver.c */
extern SymTabRec CIRChipsets[];
extern PciChipsets CIRPciChipsets[];

extern Bool CirMapMem(CirPtr pCir, int scrnIndex);
extern Bool CirUnmapMem(CirPtr pCir, int scrnIndex);
extern void cirProbeDDC(ScrnInfoPtr pScrn, int index);

/* cir_dga.c */
extern Bool CirDGAInit(ScreenPtr pScreen);

/* in cir_shadow.c */
void cirPointerMoved(ScrnInfoPtr arg, int x, int y);
void cirRefreshArea(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void cirRefreshArea8(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void cirRefreshArea16(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void cirRefreshArea24(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void cirRefreshArea32(ScrnInfoPtr pScrn, int num, BoxPtr pbox);

/* properties */
#define HWCUR64 0x1
#define ACCEL_AUTOSTART 0x2

#endif /* CIR_H */
