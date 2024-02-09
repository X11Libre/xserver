
/* xf86DDC.h
 *
 * This file contains all information to interpret a standard EDIC block
 * transmitted by a display device via DDC (Display Data Channel). So far
 * there is no information to deal with optional EDID blocks.
 * DDC is a Trademark of VESA (Video Electronics Standard Association).
 *
 * Copyright 1998 by Egbert Eich <Egbert.Eich@Physik.TU-Darmstadt.DE>
 */

#ifndef XF86_DDC_H
#define XF86_DDC_H

#include "edid.h"
#include "xf86i2c.h"
#include "xf86str.h"

/* speed up / slow down */
typedef enum {
    DDC_SLOW,
    DDC_FAST
} xf86ddcSpeed;

typedef void (*DDC1SetSpeedProc) (ScrnInfoPtr, xf86ddcSpeed);

extern _X_EXPORT xf86MonPtr xf86DoEDID_DDC1(ScrnInfoPtr pScrn,
                                            DDC1SetSpeedProc DDC1SetSpeed,
                                            unsigned
                                            int (*DDC1Read) (ScrnInfoPtr)
    );

extern _X_EXPORT xf86MonPtr xf86DoEDID_DDC2(ScrnInfoPtr pScrn, I2CBusPtr pBus);

extern _X_EXPORT xf86MonPtr xf86DoEEDID(ScrnInfoPtr pScrn, I2CBusPtr pBus, Bool);

extern _X_EXPORT xf86MonPtr xf86PrintEDID(xf86MonPtr monPtr);

extern _X_EXPORT xf86MonPtr xf86InterpretEDID(int screenIndex, Uchar * block);

extern _X_EXPORT xf86MonPtr xf86InterpretEEDID(int screenIndex, Uchar * block);

extern _X_EXPORT void
 xf86EdidMonitorSet(int scrnIndex, MonPtr Monitor, xf86MonPtr DDC);

extern _X_EXPORT Bool xf86SetDDCproperties(ScrnInfoPtr pScreen, xf86MonPtr DDC);

extern _X_EXPORT Bool
 xf86MonitorIsHDMI(xf86MonPtr mon);

extern _X_EXPORT Bool
gtf_supported(xf86MonPtr mon);

extern _X_EXPORT DisplayModePtr
FindDMTMode(int hsize, int vsize, int refresh, Bool rb);

extern _X_EXPORT const DisplayModeRec DMTModes[];

#endif
