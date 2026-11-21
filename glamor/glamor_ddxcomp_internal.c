/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 Tautvis <gtautvis@gmail.com>
 */

#include "dix-config.h"

#include "glamor_priv.h"
#include "glamor_glx_provider.h"


void
glamor_ddxcomp_egl_screen_init(ScreenPtr screen, struct glamor_context *glamor_ctx)
{

#ifdef GLXEXT
    static Bool vendor_initialized = FALSE;

    if (!vendor_initialized) {
        GlxPushProvider(&glamor_provider);
        vendor_initialized = TRUE;
    }
#endif

}
