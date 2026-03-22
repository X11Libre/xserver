/*
 * Copyright 1998 by The XFree86 Project, Inc
 */
#include <xorg-config.h>

#include "xf86Module.h"

static XF86ModuleVersionInfo VersRec = {
    .modname      = "vgahw",
    .vendor       = MODULEVENDORSTRING,
    ._modinfo1_   = MODINFOSTRING1,
    ._modinfo2_   = MODINFOSTRING2,
    .xf86version  = XORG_VERSION_CURRENT,
    .majorversion = 0,
    .minorversion = 1,
    .patchlevel   = 0,
    .abiclass     = ABI_CLASS_VIDEODRV,
    .abiversion   = ABI_VIDEODRV_VERSION,
};

_X_EXPORT XF86ModuleData vgahwModuleData = {
    .vers = &VersRec
};
