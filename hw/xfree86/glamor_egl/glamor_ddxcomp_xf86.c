/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 Tautvis <gtautvis@gmail.com>
 */
#include "dix-config.h"

#include "glamor_priv.h"
#include "glamor_glx_provider.h"

#include "xf86.h"

#define GLAMOR_LOG_STR(idx, type, str) \
do { \
        if ((idx) != -1) { \
            LogMessage((type), "glamor(%d): " str, (idx)); \
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
        ScrnInfoPtr pScrn = xf86ScreenToScrn(screen);

        void *mod = xf86LoadSubModule(pScrn, "glx");

        if (mod != NULL){
            void (*glx_GlxPushProvider)(__GLXprovider * provider) =  LoaderSymbolFromModule(mod, "GlxPushProvider");

            if (glx_GlxPushProvider){
                GlxPushProvider(&glamor_provider);
                vendor_initialized = TRUE;
            } else {
#ifdef CONFIG_LEGACY_NVIDIA_PADDING
                GLAMOR_LOG_STR(screen->myNum, X_ERROR,
                               "glx extension doens't contain GlxPushProvider symbol and  XLibre compiled with -Dlegacy_nvidia_padding=true - do not load glx for this screen\n");
#else
                GLAMOR_LOG_STR(screen->myNum, X_ERROR,
                               "glx extension doens't contain GlxPushProvider - glx glvnd vendor won't work and glx won't function correctly (if you load old nvidia driver - please compile with `-Dlegacy_nvidia_padding=true` \n");
#endif
            }
        } else {
            GLAMOR_LOG_STR(screen->myNum, X_ERROR,
                           "Failed to load glx extension, missing? glamor won't have glx\n");

        }

    }
#endif

}