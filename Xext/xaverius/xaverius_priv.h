/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * Xaverius per-screen private API for DDX drivers.
 * This header is installed and can be included by DDX drivers (e.g. modesetting)
 * to hook into Xaverius per-screen functionality.
 *
 * Xaverius allocates the per-screen structure and calls into the DDX driver
 * through the ops callbacks.
 */

#ifndef _XLIBRE_XAVERIUS_PRIV_H
#define _XLIBRE_XAVERIUS_PRIV_H

#include <stdbool.h>

#include "include/xlibre_ptrtypes.h"

/* Forward declaration for callback signatures */
typedef struct _XaveriusScreen XaveriusScreenRec, *XaveriusScreenPtr;

/* Called by DDX during screen initialization to register its callbacks.
 * Xaverius allocates the per-screen structure, DDX fills in the ops.
 */
typedef struct _XaveriusDDXOps
{
    bool (*ScreenInit)(XaveriusScreenPtr xav_screen);
    void (*ScreenFini)(XaveriusScreenPtr xav_screen);
} XaveriusDDXOps;

/* Per-screen state (internal, not exposed to DDX beyond the pointer) */
struct _XaveriusScreen
{
    ScreenPtr pScreen;
    const XaveriusDDXOps *ops;   /* callbacks provided by DDX, called by Xaverius */
    void *driver_private;        /* DDX-owned private data */
    bool initialized;
};

/* Called by DDX during screen initialization to register its callbacks.
 * Xaverius allocates the per-screen structure, DDX fills in the ops.
 * Screen close cleanup is handled automatically via the screen close hook.
 */
_X_EXPORT bool XaveriusScreenInit(ScreenPtr pScreen, const XaveriusDDXOps *ops);

_X_EXPORT XaveriusScreenPtr XaveriusGetScreen(ScreenPtr pScreen);

#endif /* _XLIBRE_XAVERIUS_PRIV_H */
