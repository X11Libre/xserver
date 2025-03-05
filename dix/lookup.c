/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * @brief DIX lookup functions
 */
#include <dix-config.h>

#include "dix/dix_priv.h"
#include "dix/resource_priv.h"
#include "include/input.h"
#include "include/inputstr.h"
#include "include/windowstr.h"

ClientPtr dixClientForWindow(WindowPtr pWin) {
    if (!pWin)
        return NullClient;

    return clients[dixClientIdForXID(pWin->drawable.id)];
}

ClientPtr dixClientForGrab(GrabPtr pGrab) {
    if (!pGrab)
        return NullClient;

    return clients[dixClientIdForXID(pGrab->resource)];
}

ClientPtr dixClientForInputClients(InputClientsPtr pInputClients) {
    if (!pInputClients)
        return NullClient;

    return clients[dixClientIdForXID(pInputClients->resource)];
}

ClientPtr dixClientForOtherClients(OtherClientsPtr pOtherClients) {
    if (!pOtherClients)
        return NullClient;

    return clients[dixClientIdForXID(pOtherClients->resource)];
}
