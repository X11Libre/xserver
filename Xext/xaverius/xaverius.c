/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include "xaverius.h"
#include "xaverius_compute.h"
#include "extnsionst.h"

int XAveriusExtensionMajorOpcode = 0;
static int XAveriusEventBase = 0;

static void
XAveriusExtensionFini(ExtensionEntry *extEntry)
{
    xaverius_compute_fini();
}

int
xaverius_extension_init(void)
{
    ExtensionEntry *extEntry = AddExtension(XAVERIUS_EXTENSION_NAME, 0, 0,
                                            ProcXAveriusDispatch,
                                            SProcXAveriusDispatch,
                                            XAveriusExtensionFini,
                                            StandardMinorOpcode);
    if (!extEntry)
        return -1;

    XAveriusExtensionMajorOpcode = extEntry->base;
    XAveriusEventBase = extEntry->eventBase;

    extEntry->base = XAveriusExtensionMajorOpcode;
    extEntry->errorBase = extEntry->errorBase;
    extEntry->eventBase = XAveriusEventBase;

    xaverius_compute_init();

    return 0;
}

void
xaverius_extension_fini(void)
{
    XAveriusExtensionFini(NULL);
}