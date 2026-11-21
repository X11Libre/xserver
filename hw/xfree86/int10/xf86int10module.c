/*
 *                   XFree86 int10 module
 *   execute BIOS int 10h calls in x86 real mode environment
 *                 Copyright 1999 Egbert Eich
 */
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86Module.h"

static XF86ModuleVersionInfo VersRec = {
    .modname      = "int10",
    .vendor       = MODULEVENDORSTRING,
    ._modinfo1_   = MODINFOSTRING1,
    ._modinfo2_   = MODINFOSTRING2,
    .xf86version  = XORG_VERSION_CURRENT,
    .majorversion = 1,
    .minorversion = 0,
    .patchlevel   = 0,
    .abiclass     = ABI_CLASS_VIDEODRV,
    .abiversion   = ABI_VIDEODRV_VERSION,
};

_X_EXPORT XF86ModuleData int10ModuleData = {
    .vers = &VersRec
};

XF86_MODULE_VERSION_SUFFIX(int10, PACKAGE_VERSION_SUFFIX);
