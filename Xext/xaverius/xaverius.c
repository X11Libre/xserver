#include "xaverius.h"
#include "xaverius_device.h"
#include "xaverius_proto.h"
#include "extnsionst.h"
#include "xf86.h"
#include "dixstruct.h"

static int XAveriusEventBase = 0;
int XAveriusExtensionMajorOpcode = 0;

static void
XAveriusExtensionInit(ExtensionEntry *extEntry)
{
    XAveriusExtensionMajorOpcode = extEntry->base;
    XAveriusEventBase = extEntry->eventBase;

    extEntry->base = XAveriusExtensionMajorOpcode;
    extEntry->errorBase = extEntry->errorBase;
    extEntry->eventBase = XAveriusEventBase;

    xaverius_device_init();
}

static void
XAveriusExtensionFini(ExtensionEntry *extEntry)
{
    xaverius_device_fini();
}

void
xaverius_extension_init(void)
{
    ExtensionEntry *extEntry = AddExtension(XAVERIUS_EXTENSION_NAME, 0, 0,
                                            ProcXAveriusDispatch,
                                            SProcXAveriusDispatch,
                                            XAveriusExtensionFini,
                                            StandardMinorOpcode);
    if (!extEntry)
        return;

    XAveriusExtensionInit(extEntry);
}

void
xaverius_extension_fini(void)
{
}