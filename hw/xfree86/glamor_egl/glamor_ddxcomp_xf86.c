/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 Tautvis <gtautvis@gmail.com>
 */

/**
 * \brief xf86 glamor bits
 */
#include "dix-config.h"

#include "glamor_priv.h"
#include "glamor_glx_provider.h"

#include "xf86.h"

#define GLAMOR_LOG_STR(idx, type, str) \
do { \
        if ((idx) != -1) { \
            if ((idx) >= 256) {\
                LogMessage((type), "glamor(G%d): " str, (idx-256)); \
             }  else { \
                LogMessage((type), "glamor(%d): " str, (idx)); \
             } \
    } else { \
            LogMessage((type), "glamor: " str); \
    } \
} while(0)

void
glamor_ddxcomp_egl_screen_init(ScreenPtr screen, struct glamor_context *glamor_ctx)
{

#ifdef GLXEXT
    static Bool vendor_initialized = FALSE;

    if (!vendor_initialized) {

        /* anything smarter than using LoaderSymbol requires X server to gain API functions to lookup for already loaded module */
        void (*glx_GlxPushProvider)(__GLXprovider * provider) = LoaderSymbol("GlxPushProvider");

            if (glx_GlxPushProvider){
                glx_GlxPushProvider(&glamor_provider);
                vendor_initialized = TRUE;
            } else {
#ifdef CONFIG_LEGACY_NVIDIA_PADDING
                GLAMOR_LOG_STR(screen->myNum, X_ERROR,
                               "loaded glx module symbol GlxPushProvider not found (or glx module not loaded) and XLibre compiled with -Dlegacy_nvidia_padding=true - glamor with continoue without glx provider and limited glx functionality\n");
#else
                GLAMOR_LOG_STR(screen->myNum, X_ERROR,
                               "loaded glx symbol GlxPushProvider not found (or glx module not loaded) - glx won't function correctly (if you loaded old nvidia driver - please compile with `-Dlegacy_nvidia_padding=true` and follow your distribution instructions\n");
#endif
            }
        }

#endif

}