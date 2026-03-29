/*
 * Copyright (C) 1998 The XFree86 Project, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 *
 * Authors:
 *    Zhigang Gong <zhigang.gong@gmail.com>
 */
#include <dix-config.h>

#include <xf86.h>
#define GLAMOR_FOR_XORG
#include <xf86Module.h>
#include "glamor.h"

static XF86ModuleVersionInfo VersRec = {
    .modname      = GLAMOR_EGL_MODULE_NAME,
    .vendor       = MODULEVENDORSTRING,
    ._modinfo1_   = MODINFOSTRING1,
    ._modinfo2_   = MODINFOSTRING2,
    .xf86version  = XORG_VERSION_CURRENT,
    .majorversion = 1,
    .minorversion = 0,
    .patchlevel   = 1,
    .abiclass     = ABI_CLASS_ANSIC,
    .abiversion   = ABI_ANSIC_VERSION,
};

_X_EXPORT XF86ModuleData glamoreglModuleData = {
    .vers = &VersRec
};
