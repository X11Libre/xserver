/*
 * Copyright (c) 2000 by Conectiva S.A. (http://www.conectiva.com)
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
 */

#ifndef _VESA_H_
#define _VESA_H_

/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"

/* All drivers need this */

#include "compiler.h"

#include "vgaHW.h"

/* Drivers that need to access the PCI config space directly need this */
#include "xf86Pci.h"

/* VBE/DDC support */
#include "vbe.h"
#include "vbeModes.h"
#include "xf86DDC.h"

/* ShadowFB support */
#include "shadow.h"

/* Int 10 support */
#include "xf86int10.h"

/* Dga definitions */
#include "dgaproc.h"

#include "fb.h"

#include <pciaccess.h>

#define VESA_VERSION            4000
#define VESA_NAME               "VESA"
#define VESA_DRIVER_NAME        "vesa"
#define VESA_MAJOR_VERSION      PACKAGE_VERSION_MAJOR
#define VESA_MINOR_VERSION      PACKAGE_VERSION_MINOR
#define VESA_PATCHLEVEL         PACKAGE_VERSION_PATCHLEVEL

/*XXX*/

typedef struct _VESARec
{
    vbeInfoPtr pVbe;
    EntityInfoPtr pEnt;
    CARD16 major, minor;
    VbeInfoBlock *vbeInfo;
    struct pci_device *pciInfo;
    int curBank, bankSwitchWindowB;
    CARD16 maxBytesPerScanline;
    unsigned long mapPhys, mapOff, mapSize; /* video memory */
    void *base, *VGAbase;
    CARD8 *state, *pstate; /* SVGA state */
    int statePage, stateSize, stateMode;
    int page;
    CARD32 *pal, *savedPal;
    CARD8 *fonts;
    xf86MonPtr monitor;
    Bool shadowFB, strict_validation;
    CARD32 windowAoffset;
    /* Don't override the default refresh rate. */
    Bool defaultRefresh;
    /* DGA info */
    DGAModePtr pDGAMode;
    int nDGAMode;
    CloseScreenProcPtr CloseScreen;
    CreateScreenResourcesProcPtr CreateScreenResources;
    xf86EnableDisableFBAccessProc *EnableDisableFBAccess;
    Bool accessEnabled;
    OptionInfoPtr Options;
    unsigned long ioBase;
    Bool ModeSetClearScreen;
    void *shadow;
    ShadowUpdateProc update;
    ShadowWindowProc window;
} VESARec, *VESAPtr;


#endif /* _VESA_H_ */
