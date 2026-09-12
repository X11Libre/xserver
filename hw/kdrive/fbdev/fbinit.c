/*
 * Copyright © 1999 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <kdrive-config.h>
#include "klinux.h"
#include "fbdev.h"

#include "dix/dix_priv.h"
#include "os/cmdline.h"
#include "os/ddx_priv.h"
#include "os/log_priv.h"

#include <string.h>

static FbScreenConf *fbCurrScreen = NULL;

static const FbScreenConf fbDefaultConfig = {
                                             .fb_path = NULL,
                                             .shadow = TRUE,
                                            };

static void fbdevLogScreenInfo(const FbScreenConf *config, int screen_num);

static void
FbdevLogInit(void)
{
    KdCardInfo *curr_card = kdCardInfo;
    char *log_file = NULL;
    const char *display_name = display ? display : "";
    if (asprintf(&log_file, DEFAULT_LOGDIR "/Xfbdev.%s.log", display_name) < 0) {
        LogInit(DEFAULT_LOGDIR "/Xkdrive.log", ".old");
    } else {
        LogInit(log_file, ".old");
        free(log_file);
    }

    LogMessage(X_INFO, "Xfbdev: X11 server for linux framebuffer devices\n");
    LogMessage(X_INFO, "\n");
    LogMessage(X_INFO, "Xfbdev: Configured screens info:\n");
    LogMessage(X_INFO, "\n");

    if (curr_card) {
        while(curr_card) {
            fbdevLogScreenInfo(curr_card->closure, curr_card->mynum);
            curr_card = curr_card->next;
        }
    } else {
        FbScreenConf fbDummyConfig = fbDefaultConfig;
        fbDummyConfig.glamor_info = kdGlamorDefault;
        fbdevLogScreenInfo(&fbDummyConfig, 0);
    }
}

void
InitCard(char *name)
{
    fbCurrScreen = XNFalloc(sizeof(*fbCurrScreen));
    *fbCurrScreen = fbDefaultConfig;
    fbCurrScreen->glamor_info = kdGlamorDefault;
    KdCardInfoAdd(&fbdevFuncs, fbCurrScreen);
}

static void
fbdevLogScreenInfo(const FbScreenConf *config, int screen_num)
{
    LogMessage(X_INFO, "Xfbdev(%d): Screen %d:\n", screen_num, screen_num);

    LogMessage(X_INFO, "Xfbdev(%d): framebuffer device: %s\n", screen_num,
               config->fb_path ? config->fb_path : "not passed");
    LogMessage(X_INFO, "Xfbdev(%d): ShadowFB %s\n", screen_num,
               config->shadow ? "enabled" : "disabled");

    LogMessage(X_INFO, "Xfbdev(%d): glvnd library: %s\n", screen_num,
               config->glamor_info.glvnd ? config->glamor_info.glvnd : "not passed");

    LogMessage(X_INFO, "Xfbdev(%d): dri device: %s\n", screen_num,
               config->dri_path ? config->dri_path : "none");

    LogMessage(X_INFO, "Xfbdev(%d): glamor OpenGL contexts %s\n", screen_num,
               !config->glamor_info.force_es ? "allowed" : "forbidden");
    LogMessage(X_INFO, "Xfbdev(%d): glamor GLES contexts %s\n", screen_num,
               !config->glamor_info.force_gl ? "allowed" : "forbidden");

    LogMessage(X_INFO, "Xfbdev(%d): glamor render acceleration %s\n", screen_num,
               !config->glamor_info.no_render_accel ? "enabled" : "disabled");
    LogMessage(X_INFO, "Xfbdev(%d): glamor render acceleration %s on software renderers\n", screen_num,
               config->glamor_info.force_render_accel ? "allowed" : "forbidden");
    LogMessage(X_INFO, "Xfbdev(%d): glamor is %s libgbm \n", screen_num,
               config->glamor_info.use_gbm ? "allowed to use" : "forbidden from using");

    LogMessage(X_INFO, "Xfbdev(%d): glamor X-Video support %s\n", screen_num,
               config->glamor_info.use_xv ? "allowed" : "forbidden");
    LogMessage(X_INFO, "\n");
}

#if INPUTTHREAD
/** This function is called in Xserver/os/inputthread.c when starting
    the input thread. */
void
ddxInputThreadInit(void)
{
}
#endif

void
InitOutput(int argc, char **argv)
{
    KdInitOutput(argc, argv);
}

void
InitInput(int argc, char **argv)
{
    LinuxAddInputDrivers();
    KdInitInput();
}

void
CloseInput(void)
{
    KdCloseInput();
}

void
ddxUseMsg(void)
{
    KdGlamorUseMsg();
    ErrorF("\nXfbdev Device Usage:\n");
    ErrorF
        ("-fb <path>           Framebuffer device to use. Defaults to /dev/fb0\n");
    ErrorF
        ("-noshadow            Disable the ShadowFB layer if possible\n");
    ErrorF("\n");
}

int
ddxProcessArgument(int argc, char **argv, int i)
{
    int glamor_arg;

    KdEnsureCard(argc, argv, i, !fbCurrScreen);

    if (!strcmp(argv[i], "-fb")) {
        if ((i + 1 < argc) && (argv[i + 1][0] != '-')) {
            fbCurrScreen->fb_path = argv[i + 1];
            return 2;
        }
        UseMsg();
        exit(1);
    }

    if (!strcmp(argv[i], "-noshadow")) {
        fbCurrScreen->shadow = FALSE;
        return 1;
    }

    glamor_arg = KdGlamorParse(&fbCurrScreen->glamor_info, &fbCurrScreen->dri_path, argc, argv, i);
    if (glamor_arg) {
        return glamor_arg;
    }

    return KdProcessArgument(argc, argv, i);
}

void ddxInit(void)
{
    FbdevLogInit();
    KdOsInit(&LinuxFuncs);
}

KdCardFuncs fbdevFuncs = {
    .cardinit         = fbdevCardInit,
    .scrinit          = fbdevScreenInit,
    .initScreen       = fbdevInitScreen,
    .finishInitScreen = fbdevFinishInitScreen,
    .createRes        = fbdevCreateResources,
    .preserve         = fbdevPreserve,
    .enable           = fbdevEnable,
    .dpms             = fbdevDPMS,
    .disable          = fbdevDisable,
    .restore          = fbdevRestore,
    .scrfini          = fbdevScreenFini,
    .cardfini         = fbdevCardFini,

    /* no cursor funcs */

#ifdef GLAMOR
    .initAccel        = fbdevInitAccel,
    .enableAccel      = fbdevEnableAccel,
    .disableAccel     = fbdevDisableAccel,
    .finiAccel        = fbdevFiniAccel,
#endif

    .getColors        = fbdevGetColors,
    .putColors        = fbdevPutColors,

    /* no closescreen func */
};
