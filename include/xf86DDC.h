
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

/*
 * Legacy pointer-only parsers: the caller must provide storage for all
 * extension blocks advertised by the base block. Prefer xf86ParseEDID() when
 * the actual buffer length is available.
 */
extern _X_EXPORT xf86MonPtr xf86InterpretEDID(int screenIndex, uint8_t * block);

extern _X_EXPORT xf86MonPtr xf86InterpretEEDID(int screenIndex, uint8_t * block);

extern _X_EXPORT Bool xf86SetDDCproperties(ScrnInfoPtr pScreen, xf86MonPtr DDC);

/**
 * @brief xf86DoInterpretHDRMetadata fill hdr structure using raw edid data
 * This function is necessary because xf86InterpretEDID have no indication if this whole EDID or not
 * @param monPtr
 * @return true if colorimetry & hdr static metadata is found
 */
_X_EXPORT bool xf86DoInterpretHDRMetadata(xf86MonPtr monPtr);

/*
 * parse EDID block and return a newly allocated xf86Monitor
 *
 * the data block will be copied into the structure (actually right after the struct)
 * and thus automatically be freed when the returned struct is freed.
 *
 * @param pScreen       screen whose index will be recorded in the xf86Monitor
 * @param block         the EDID block to parse
 * @param size          number of available bytes (128 to 32768); only complete
 *                       extension blocks are copied, missing advertised blocks
 *                       are zero-filled, and completeness requires all blocks
 * @return              newly allocated xf86MonRec or NULL on failure
 */
_X_EXPORT xf86MonPtr xf86ParseEDID(ScrnInfoPtr pScreen, uint8_t *block, size_t size);

#endif
