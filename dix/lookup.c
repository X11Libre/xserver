/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * @brief DIX lookup functions
 */
#include <dix-config.h>

#include "dix/dix_priv.h"
#include "include/windowstr.h"

struct window_xid_match {
    WindowPtr pWin;
    Window id;
};

static int dix_match_window_xid(WindowPtr pWin, void *ptr)
{
    struct window_xid_match *walk = (struct window_xid_match*) ptr;

    if (walk->id == pWin->drawable.id) {
        walk->pWin = pWin;
        return WT_STOPWALKING;
    }
    else
        return WT_WALKCHILDREN;
}

WindowPtr dixLookupWindowByXID(Window window)
{
    struct window_xid_match walk = {
        .id = window,
    };

    for (int i = 0; i < screenInfo.numScreens; i++) {
        WalkTree(screenInfo.screens[i], dix_match_window_xid, &walk);
        if (walk.pWin)
            break;
    }

    return walk.pWin;
}
