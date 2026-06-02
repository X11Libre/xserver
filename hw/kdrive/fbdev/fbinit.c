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
#include "fbdev.h"

#include "dix/dix_priv.h"
#include "os/cmdline.h"
#include "os/ddx_priv.h"
#include "os/log_priv.h"

#include <string.h>

static FbScreenConf *fbCurrScreen = NULL;

static const FbScreenConf fbDefaultConfig = {
                                             .fbdevDevicePath = NULL,
                                             .fbDisableShadow = FALSE,

                                             .fbdev_glvnd_provider = NULL,

                                             .fbdev_dri_path = NULL,
                                             .fbdev_auto_dri3 = FALSE,
                                             .fbdev_drm_master = FALSE,
                                             .partial_dri_allowed = FALSE,

                                             .es_allowed = TRUE,
                                             .force_es = FALSE,

                                             .fbGlamorAllowed = TRUE,
                                             .fbForceGlamor = FALSE,
                                             .gbm_allowed = FALSE,

                                             .fbXVAllowed = TRUE,
                                            };

static void fbdevLogScreenInfo(const FbScreenConf *config, int screen_num);

void LinuxLogInit(void);

void
LinuxLogInit(void)
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
        fbdevLogScreenInfo(&fbDefaultConfig, 0);
    }
}

void
InitCard(char *name)
{
    fbCurrScreen = XNFalloc(sizeof(*fbCurrScreen));
    *fbCurrScreen = fbDefaultConfig;
    KdCardInfoAdd(&fbdevFuncs, fbCurrScreen);
}

static void
fbdevLogScreenInfo(const FbScreenConf *config, int screen_num)
{
    LogMessage(X_INFO, "Xfbdev(%d): Screen %d:\n", screen_num, screen_num);

    LogMessage(X_INFO, "Xfbdev(%d): framebuffer device: %s\n", screen_num,
               config->fbdevDevicePath ? config->fbdevDevicePath : "not passed");
    LogMessage(X_INFO, "Xfbdev(%d): ShadowFB %s\n", screen_num,
               config->fbDisableShadow ? "disabled" : "enabled");
    LogMessage(X_INFO, "Xfbdev(%d): HW Acceleration %s\n", screen_num,
               config->fbNoAccel ? "disabled" : "enabled");

    LogMessage(X_INFO, "Xfbdev(%d): glvnd library: %s\n", screen_num,
               config->fbdev_glvnd_provider ? config->fbdev_glvnd_provider : "not passed");

    LogMessage(X_INFO, "Xfbdev(%d): dri device: %s\n", screen_num,
               config->fbdev_dri_path ? config->fbdev_dri_path : "none");
    LogMessage(X_INFO, "Xfbdev(%d): automatic DRI3 %s\n", screen_num,
               config->fbdev_auto_dri3 ? "enabled" : "disabled");
    LogMessage(X_INFO, "Xfbdev(%d): drm master %s\n", screen_num,
               config->fbdev_drm_master ? "enabled" : "disabled");
    LogMessage(X_INFO, "Xfbdev(%d): partial DRI3 %s\n", screen_num,
               config->partial_dri_allowed ? "allowed" : "forbidden");


    LogMessage(X_INFO, "Xfbdev(%d): glamor OpenGL contexts %s\n", screen_num,
               !config->force_es ? "allowed" : "forbidden");
    LogMessage(X_INFO, "Xfbdev(%d): glamor GLES contexts %s\n", screen_num,
               config->es_allowed ? "allowed" : "forbidden");

    LogMessage(X_INFO, "Xfbdev(%d): glamor render acceleration %s\n", screen_num,
               config->fbGlamorAllowed ? "enabled" : "disabled");
    LogMessage(X_INFO, "Xfbdev(%d): glamor render acceleration %s on software renderers\n", screen_num,
               config->fbForceGlamor ? "allowed" : "forbidden");
    LogMessage(X_INFO, "Xfbdev(%d): glamor is %s libgbm \n", screen_num,
               config->gbm_allowed ? "allowed to use" : "forbidden from using");

    LogMessage(X_INFO, "Xfbdev(%d): glamor X-Video support %s\n", screen_num,
               config->fbXVAllowed ? "allowed" : "forbidden");
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
    KdOsAddInputDrivers();
    KdAddConfigInputDrivers();
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
    KdUseMsg();
    ErrorF("\nXfbdev Device Usage:\n");
    ErrorF
        ("-fb <path>           Framebuffer device to use. Defaults to /dev/fb0\n");
    ErrorF
        ("-dri [path|auto]     Optional drm device path to use\n");
    ErrorF
        ("-partial-dri         Allow glamor to initialize DRI3 only partially\n");
    ErrorF
        ("-drm-master          Enable master permissions on the fd used for dri\n");
    ErrorF
        ("-noshadow            Disable the ShadowFB layer if possible\n");
    ErrorF
        ("-noaccel             Disable hw acceleration (per screen)\n");
    ErrorF
        ("-glamor              Force enable glamor render acceleration if possible\n");
    ErrorF
        ("-noglamor            Force disable glamor render acceleration\n");
    ErrorF
        ("-gbm                 Allow glamor to use libgbm\n");
    ErrorF
        ("-glvendor <string>   Suggest what glvnd vendor library should be used\n");
    ErrorF
        ("-force-gl            Force glamor to only use GL contexts\n");
    ErrorF
        ("-force-es            Force glamor to only use GLES contexts\n");
    ErrorF
        ("-noxv                Disable X-Video support\n");
    ErrorF("\n");
}

int
ddxProcessArgument(int argc, char **argv, int i)
{
    if (!fbCurrScreen || !strcmp(argv[i], "-screen")) {
        /* xinit adds an implicit :0 arg */
        int implicit_first_screen = !fbCurrScreen && strcmp(argv[i], "-screen") && (argv[i][0] != ':');

        /* Put each screen on a separate card */
        if (argv[i][0] != ':') {
            InitCard(NULL);
        }
        if (implicit_first_screen) {
            /* This is what KdInitOutput would have done */
            KdCardInfo *card = KdCardInfoLast();
            KdScreenInfo *screen = KdScreenInfoAdd(card);
            KdParseScreen(screen, NULL);
        }
    }

    if (!strcmp(argv[i], "-fb")) {
        if (i + 1 < argc) {
            fbCurrScreen->fbdevDevicePath = argv[i + 1];
            return 2;
        }
        UseMsg();
        exit(1);
    }

    if (!strcmp(argv[i], "-noshadow")) {
        fbCurrScreen->fbDisableShadow = TRUE;
        return 1;
    }

    if (!strcmp(argv[i], "-noaccel")) {
        fbCurrScreen->fbNoAccel = TRUE;
        return 1;
    }

    if (!strcmp(argv[i], "-glamor")) {
        fbCurrScreen->fbForceGlamor = TRUE;
        return 1;
    }

    if (!strcmp(argv[i], "-noglamor")) {
        fbCurrScreen->fbGlamorAllowed = FALSE;
        return 1;
    }

    if (!strcmp(argv[i], "-gbm")) {
        fbCurrScreen->gbm_allowed = TRUE;
        return 1;
    }

    if (!strcmp(argv[i], "-glvendor")) {
        if (i + 1 < argc) {
            fbCurrScreen->fbdev_glvnd_provider = argv[i + 1];
            return 2;
        }
        UseMsg();
        exit(1);
    }

    if (!strcmp(argv[i], "-dri")) {
        if (i + 1 < argc) {
            if (argv[i + 1][0] == '-' || !strcmp(argv[i + 1], "auto")) {
                fbCurrScreen->fbdev_auto_dri3 = TRUE;
            } else {
                fbCurrScreen->fbdev_dri_path = argv[i + 1];
            }
            return 2;
        } else {
            fbCurrScreen->fbdev_auto_dri3 = TRUE;
            return 1;
        }
    }

    if (!strcmp(argv[i], "-partial-dri")) {
        fbCurrScreen->partial_dri_allowed = TRUE;
        return 1;
    }

    if (!strcmp(argv[i], "-drm-master")) {
        fbCurrScreen->fbdev_drm_master = TRUE;
        return 1;
    }

    if (!strcmp(argv[i], "-force-gl")) {
        fbCurrScreen->es_allowed = FALSE;
        return 1;
    }

    if (!strcmp(argv[i], "-force-es")) {
        fbCurrScreen->force_es = TRUE;
        return 1;
    }

    if (!strcmp(argv[i], "-noxv")) {
        fbCurrScreen->fbXVAllowed = FALSE;
        return 1;
    }

    return KdProcessArgument(argc, argv, i);
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
