#include <xorg-config.h>

#include "xf86Module.h"

static XF86ModuleVersionInfo VersRec = {
    .modname      = "shadowfb",
    .vendor       = MODULEVENDORSTRING,
    ._modinfo1_   = MODINFOSTRING1,
    ._modinfo2_   = MODINFOSTRING2,
    .xf86version  = XORG_VERSION_CURRENT,
    .majorversion = 1,
    .minorversion = 0,
    .patchlevel   = 0,
    .abiclass     = ABI_CLASS_ANSIC,
    .abiversion   = ABI_ANSIC_VERSION,
};

_X_EXPORT XF86ModuleData shadowfbModuleData = {
    .vers = &VersRec
};
