/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111github@gmail.com>
 */

#include <kdrive-config.h>

#ifdef KDRIVE_LINUX
#include "klinux.h"
#else
#include "kstub.h"
#endif

#include "modesetting.h"

#include "dix/dix_priv.h"
#include "os/cmdline.h"
#include "os/ddx_priv.h"
#include "os/log_priv.h"

#include <string.h>

static MsScreenConf *msCurrScreen = NULL;

static const MsScreenConf msDefaultConfig = {
                                             .dev_path = NULL,
                                             .shadow = TRUE,
                                             .glamor_info = {.use_gbm = TRUE,},
                                            };

static void msLogScreenInfo(const MsScreenConf *config, int screen_num);

static void
msLogInit(void)
{
    KdCardInfo *curr_card = kdCardInfo;
    char *log_file = NULL;
    const char *display_name = display ? display : "";
    if (asprintf(&log_file, DEFAULT_LOGDIR "/Xmodesetting.%s.log", display_name) < 0) {
        LogInit(DEFAULT_LOGDIR "/Xkdrive.log", ".old");
    } else {
        LogInit(log_file, ".old");
        free(log_file);
    }

    LogMessage(X_INFO, "Xmodesetting: X11 server for KMS devices\n");
    LogMessage(X_INFO, "\n");
    LogMessage(X_INFO, "Xmodesetting: Configured screens info:\n");
    LogMessage(X_INFO, "\n");

    if (curr_card) {
        while(curr_card) {
            msLogScreenInfo(curr_card->closure, curr_card->mynum);
            curr_card = curr_card->next;
        }
    } else {
        MsScreenConf msDummyConfig = msDefaultConfig;
        msLogScreenInfo(&msDummyConfig, 0);
    }
}

void
InitCard(char *name)
{
    msCurrScreen = XNFalloc(sizeof(*msCurrScreen));
    *msCurrScreen = msDefaultConfig;
    KdCardInfoAdd(&msFuncs, msCurrScreen);
}

static void
msLogScreenInfo(const MsScreenConf *config, int screen_num)
{
    LogMessage(X_INFO, "Xmodesetting(%d): Screen %d:\n", screen_num, screen_num);

    LogMessage(X_INFO, "Xmodesetting(%d): kms device: %s\n", screen_num,
               config->dev_path ? config->dev_path : "not passed");
    LogMessage(X_INFO, "Xmodesetting(%d): ShadowFB %s\n", screen_num,
               config->shadow ? "enabled" : "disabled");

    LogMessage(X_INFO, "Xmodesetting(%d): glvnd library: %s\n", screen_num,
               config->glamor_info.glvnd ? config->glamor_info.glvnd : "not passed");

    LogMessage(X_INFO, "Xmodesetting(%d): dri device: %s\n", screen_num,
               config->glamor_info.dri_path ? config->glamor_info.dri_path : "none");

    LogMessage(X_INFO, "Xmodesetting(%d): glamor OpenGL contexts %s\n", screen_num,
               !config->glamor_info.force_es ? "allowed" : "forbidden");
    LogMessage(X_INFO, "Xmodesetting(%d): glamor GLES contexts %s\n", screen_num,
               !config->glamor_info.force_gl ? "allowed" : "forbidden");

    LogMessage(X_INFO, "Xmodesetting(%d): glamor render acceleration %s\n", screen_num,
               !config->glamor_info.no_render_accel ? "enabled" : "disabled");
    LogMessage(X_INFO, "Xmodesetting(%d): glamor render acceleration %s on software renderers\n", screen_num,
               config->glamor_info.force_render_accel ? "allowed" : "forbidden");
    LogMessage(X_INFO, "Xmodesetting(%d): glamor is %s libgbm \n", screen_num,
               config->glamor_info.use_gbm ? "allowed to use" : "forbidden from using");

    LogMessage(X_INFO, "Xmodesetting(%d): glamor X-Video support %s\n", screen_num,
               config->glamor_info.no_xv ? "allowed" : "forbidden");
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
#ifdef KDRIVE_LINUX
    LinuxAddInputDrivers();
#else
    StubAddInputDrivers();
#endif
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
    ErrorF("\nXmodesetting Device Usage:\n");
    ErrorF
        ("-dev <path>          KMS device to use. Defaults to /dev/dri/card0\n");
    ErrorF
        ("-noshadow            Disable the ShadowFB layer if possible\n");
    ErrorF("\n");
}

int
ddxProcessArgument(int argc, char **argv, int i)
{
    int glamor_arg;

    KdEnsureCard(argc, argv, i, !msCurrScreen);

    if (!strcmp(argv[i], "-dev")) {
        if ((i + 1 < argc) && (argv[i + 1][0] != '-')) {
            msCurrScreen->dev_path = argv[i + 1];
            if (!msCurrScreen->glamor_info.dri_path) {
                msCurrScreen->glamor_info.dri_path = msCurrScreen->dev_path;
            }
            return 2;
        }
        UseMsg();
        exit(1);
    }

    if (!strcmp(argv[i], "-noshadow")) {
        msCurrScreen->shadow = FALSE;
        return 1;
    }

    glamor_arg = KdGlamorParse(&msCurrScreen->glamor_info, argc, argv, i);
    if (glamor_arg) {
        return glamor_arg;
    }

    return KdProcessArgument(argc, argv, i);
}

void ddxInit(void)
{
    msLogInit();
#ifdef KDRIVE_LINUX
    KdOsInit(&LinuxFuncs);
#else
    KdOsInit(&StubOsFuncs);
#endif
}

KdCardFuncs msFuncs = {
    .cardinit         = msCardInit,
    .scrinit          = msScreenInit,
    .initScreen       = msInitScreen,
    .finishInitScreen = msFinishInitScreen,
    .createRes        = msCreateResources,
    .preserve         = msPreserve,
    .enable           = msEnable,
    .dpms             = NULL, /* TODO: implement */
    .disable          = msDisable,
    .restore          = msRestore,
    .scrfini          = msScreenFini,
    .cardfini         = msCardFini,

    /* no cursor funcs */

#ifdef GLAMOR
    .initAccel        = msInitAccel,
    .enableAccel      = msEnableAccel,
    .disableAccel     = msDisableAccel,
    .finiAccel        = msFiniAccel,
#endif

#if 0
    .getColors        = msGetColors,
    .putColors        = msPutColors,
#endif

    .closeScreen      = msCloseScreen,
};
