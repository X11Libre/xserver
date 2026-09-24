/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include <stdbool.h>
#include <stddef.h>
#include <version-config.h>

#include <X11/Xproto.h>

#include "include/misc.h"
#include "dix/client_priv.h"
#include "dix/server_priv.h"
#include "dix/dix_priv.h"
#include "Xext/panoramiX/panoramiXsrv.h"
#include "Xext/panoramiX/panoramiX_priv.h"

size_t ConnectionInfoSize = 0;

void dixSendConnAbort(ClientPtr pClient, const char *reason)
{
    xConnSetupPrefix csp = {
        .success = xFalse,
        .lengthReason = strlen(reason),
        .length = bytes_to_int32(csp.lengthReason),
        .majorVersion = X_PROTOCOL,
        .minorVersion = X_PROTOCOL_REVISION,
    };

    if (pClient->swapped) {
        swaps(&csp.majorVersion);
        swaps(&csp.minorVersion);
        swaps(&csp.length);
    }

    dixWriteToClient(pClient, sizeof(csp), &csp);
    dixWriteToClient(pClient, (int) csp.lengthReason, reason);
    pClient->noClientException = -1;
}

void dixInitConnectionBlock(void)
{
    free(ConnectionInfo);
    ConnectionInfo = NULL;
    ConnectionInfoSize = 0;

#ifdef XINERAMA
    if (PanoramiXIsEnabled()) {
        if (!PanoramiXCreateConnectionBlock()) {
            FatalError("could not create panoramix connection block info");
        }
    }
    else
#endif /* XINERAMA */
    {
        if (!CreateConnectionBlock(0)) {
            FatalError("could not create connection block info");
        }
    }
}
