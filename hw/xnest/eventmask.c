/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include "dix/screenint_priv.h"

#include "xnest-eventmask.h"
#include "xnest-screen_priv.h"
#include "xnest-xcb.h"

uint32_t xnestEventMask;

void xnestUpdateEventMask(void) {
    /* walk trough all screens and find ours by checking for devPrivate entry */
    DIX_FOR_EACH_SCREEN({
        XnestScreenPrivate *screenPriv = xnestGetScreenPrivate(walkScreen);
        if (screenPriv) {
            xcb_change_window_attributes(xnestUpstreamInfo.conn,
                                         screenPriv->defaultWindow,
                                         XCB_CW_EVENT_MASK,
                                         &xnestEventMask);
        }
    });
}
