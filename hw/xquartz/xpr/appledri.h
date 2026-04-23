/* $XFree86: xc/lib/GL/dri/xf86dri.h,v 1.7 2000/12/07 20:26:02 dawes Exp $ */
/**************************************************************************

   Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
   Copyright 2000 VA Linux Systems, Inc.
   Copyright (c) 2002-2012 Apple Computer, Inc.
   All Rights Reserved.

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sub license, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice (including the
   next paragraph) shall be included in all copies or substantial portions
   of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
   ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Jens Owen <jens@valinux.com>
 *   Rickard E. (Rik) Faith <faith@valinux.com>
 *   Jeremy Huddleston <jeremyhu@apple.com>
 *
 */

#ifndef _APPLEDRI_H_
#define _APPLEDRI_H_

#include <X11/Xfuncproto.h>

#define X_AppleDRIQueryVersion                0
#define X_AppleDRIQueryDirectRenderingCapable 1
#define X_AppleDRICreateSurface               2
#define X_AppleDRIDestroySurface              3
#define X_AppleDRIAuthConnection              4
#define X_AppleDRICreatePixmap                7
#define X_AppleDRIDestroyPixmap               8

/* Requests up to and including 18 were used in a previous version */

/* Events */
#define AppleDRIObsoleteEvent1 0
#define AppleDRISurfaceNotify  3
#define AppleDRINumberEvents   4

/* Errors */
#define AppleDRIClientNotLocal        0
#define AppleDRIOperationNotSupported 1
#define AppleDRINumberErrors          (AppleDRIOperationNotSupported + 1)

/* Kinds of SurfaceNotify events: */
#define AppleDRISurfaceNotifyChanged   0
#define AppleDRISurfaceNotifyDestroyed 1

#endif /* _APPLEDRI_H_ */
