/*
 * Copyright (c) 1997-2002 by The XFree86 Project, Inc.
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
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

/*
 * This file contains declarations for private XFree86 functions and variables,
 * and definitions of private macros.
 *
 * "private" means not available to video drivers.
 */

#ifndef _XF86PRIV_H
#define _XF86PRIV_H

#include "xf86Privstr.h"
#include "input.h"

extern _X_EXPORT int xf86FbBpp;
extern _X_EXPORT int xf86Depth;

/* Other parameters */

extern _X_EXPORT xf86InfoRec xf86Info;
extern _X_EXPORT serverLayoutRec xf86ConfigLayout;

extern _X_EXPORT DriverPtr *xf86DriverList;
extern _X_EXPORT int xf86NumScreens;

extern _X_EXPORT ScrnInfoPtr *xf86GPUScreens;      /* List of pointers to ScrnInfoRecs */
extern _X_EXPORT int xf86NumGPUScreens;
extern _X_EXPORT int xf86DRMMasterFd;              /* Command line argument for DRM master file descriptor */
#ifndef DEFAULT_DPI
#define DEFAULT_DPI		96
#endif

/* xf86Bus.c */
extern _X_EXPORT void xf86BusProbe(void);
extern _X_EXPORT void xf86AddDevToEntity(int entityIndex, GDevPtr dev);

#endif                          /* _XF86PRIV_H */
