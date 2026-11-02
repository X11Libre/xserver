/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include "include/extnsionst.h"
#include "miext/extinit_priv.h"

#include "xaverius_intern.h"
#include "xaverius_proto.h"

Bool noXaveriusExtension = TRUE;

int XaveriusEventBase = 0;

static void XaveriusExtensionFini(ExtensionEntry *extEntry)
{
}

void XaveriusExtensionInit(void)
{
    ExtensionEntry *extEntry = AddExtension(XAVERIUS_EXTENSION_NAME, 0, 0,
                                            ProcXaveriusDispatch,
                                            ProcXaveriusDispatch,
                                            XaveriusExtensionFini,
                                            StandardMinorOpcode);
    if (!extEntry) {
        FatalError("failed adding Xaverius extension entry. OOM ?\n");
    }

    XaveriusEventBase = extEntry->eventBase;
    LogMessage(X_INFO, "Xaverius extension initialized\n");
}
