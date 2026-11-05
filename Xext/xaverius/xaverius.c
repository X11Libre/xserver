/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * Xaverius extension entry point.
 */

#include "xaverius.h"
#include "xaverius_device.h"
#include "xaverius_registry.h"
#include "xaverius_proto.h"
#include "extnsionst.h"
#include "xf86.h"
#include "dixstruct.h"

static int XAveriusEventBase = 0;
int XAveriusExtensionMajorOpcode = 0;
Bool noXaveriusExtension = TRUE;

static void
XAveriusExtensionFini(ExtensionEntry *extEntry)
{
    XaveriusRegistryFini();
}

void
XaveriusExtensionInit(void)
{
    ExtensionEntry *extEntry = AddExtension(XAVERIUS_EXTENSION_NAME, 0, 0,
                                            ProcXAveriusDispatch,
                                            SProcXAveriusDispatch,
                                            XAveriusExtensionFini,
                                            StandardMinorOpcode);
    if (!extEntry)
        return;

    XAveriusExtensionMajorOpcode = extEntry->base;
    XAveriusEventBase = extEntry->eventBase;

    XaveriusRegistryInit();
}

void
xaverius_extension_init(void)
{
    XaveriusExtensionInit();
}

void
xaverius_extension_fini(void)
{
    XaveriusDevicesFini();
    XaveriusRegistryFini();
}
