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

static int ac = 0;
static char **av = NULL;

static MsScreenConf *msCurrScreen = NULL;

static const MsScreenConf msDefaultConfig = {
                                             .shadow = TRUE,
                                             .glamor_info = {.fake_rate = -1, .use_gbm = TRUE,},
                                            };

static const char* FindDevPath(int argc, char **argv, int i);

static void msLogScreenInfo(const MsScreenConf *config, const char* dev_path, int screen_num);

static void
msLogInit(void)
{
    KdCardInfo *curr_card = kdCardInfo;
    char *log_file = NULL;
    const char *display_name = display ? display : "";
    int idx = 0;
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
            for (KdScreenInfo *screen = curr_card->screenList; screen; screen = screen->next) {
                msLogScreenInfo(screen->closure, curr_card->closure, idx++);
            }
            curr_card = curr_card->next;
        }
    } else {
        msLogScreenInfo(msCurrScreen ? msCurrScreen : &msDefaultConfig, FindDevPath(ac, av, 0), 0);
    }
}

void
InitCard(char *name)
{
    KdCardInfoAdd(&msFuncs, name);
}

/* Find the dev_path argument for this screen */
static const char*
FindDevPath(int argc, char **argv, int i)
{
    const char *dev_path = NULL;

    /* If this is the last screen and we find a -dev argument, return it */
    for (int j = i + 1; j < argc; j++) {
        if (!strcmp(argv[j], "-dev")) {
            if ((j + 1 < argc) && (argv[j + 1][0] != '-')) {
                dev_path = argv[j + 1];
            }
        }

        /* This was not the last screen */
        if (!strcmp(argv[j], "-screen")) {
            dev_path = NULL;
            break;
        }
    }

    if (dev_path) {
        return dev_path;
    }

    /* Now go backwards to find the -dev argument */
    for (int j = i - 1; j >= 0; j--) {
        if (!strcmp(argv[j], "-dev")) {
            if ((j + 1 < argc) && (argv[j + 1][0] != '-')) {
                return argv[j + 1];
            }
        }

        /* This screen had no -dev argument */
        if (!strcmp(argv[j], "-screen")) {
            return NULL;
        }
    }

    /* This screen had no -dev argument */
    return NULL;
}

static int
InitScreen(int argc, char **argv, int i)
{
    KdCardInfo *card;
    KdScreenInfo *screen;
    const char *screen_arg;
    const char *dev_path;

    /* We need at least one screen config */
    if (!msCurrScreen) {
        msCurrScreen = XNFalloc(sizeof(*msCurrScreen));
        *msCurrScreen = msDefaultConfig;
    }

    /* Not a -screen argument */
    if (strcmp(argv[i], "-screen")) {
        return 0;
    }

    screen_arg = ((i + 1) < argc && argv[i + 1][0] != '-') ? argv[i + 1] : NULL;
    dev_path = FindDevPath(argc, argv, i);

    card = msFindMatchingCard(dev_path);
    if (!card) {
        InitCard((char*)dev_path);
        card = KdCardInfoLast();
    }

    if (!card) {
        FatalError("Xmodesetting: No matching card found for device: %s!\n", dev_path);
    }

    screen = KdScreenInfoAdd(card, msCurrScreen);
    KdParseScreen(screen, screen_arg);

    /* Check if there are any more screens */
    for (int j = i + 1; j < argc; j++) {
        /* If yes, allocate one more screen config */
        if (!strcmp(argv[j], "-screen")) {
            msCurrScreen = XNFalloc(sizeof(*msCurrScreen));
            *msCurrScreen = msDefaultConfig;
            break;
        }
    }

    return screen_arg ? 2 : 1;
}

static void
msLogScreenInfo(const MsScreenConf *config, const char *dev_path, int screen_num)
{
    LogMessage(X_INFO, "Xmodesetting(%d): Screen %d:\n", screen_num, screen_num);

    LogMessage(X_INFO, "Xmodesetting(%d): KMS device: %s\n", screen_num,
               dev_path ? dev_path : "not passed");
    LogMessage(X_INFO, "Xmodesetting(%d): ShadowFB %s\n", screen_num,
               config->shadow ? "enabled" : "disabled");
    LogMessage(X_INFO, "Xmodesetting(%d): Preferred format color ordering %s\n", screen_num,
               config->format_swap ? "BGR" : "RGB");
    KdGlamorLogScreenInfo(&config->glamor_info, screen_num);
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
    KdCardInfo *card;
    KdScreenInfo *screen;

    if (!kdCardInfo) {
        InitCard((char*)FindDevPath(argc, argv, 0));
    }

    if (!(card = KdCardInfoLast()))
        FatalError("No matching cards found!\n");

    /* Add at least one screen */
    if (!card->screenList) {
        if (!msCurrScreen) {
            msCurrScreen = XNFalloc(sizeof(*msCurrScreen));
            *msCurrScreen = msDefaultConfig;
        }
        screen = KdScreenInfoAdd(card, msCurrScreen);
        KdParseScreen(screen, NULL);
    }

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
    ErrorF
        ("-swap                Prefer BGR format color ordering instead of RGB\n");
    ErrorF
        ("-notile              Don't use a tiled front buffer\n");
    ErrorF
        ("-planar              Allow planar modifiers for the front bo\n");
    ErrorF("\n");
}

int
ddxProcessArgument(int argc, char **argv, int i)
{
    int glamor_arg;
    int screen_arg;

    ac = argc;
    av = argv;

    screen_arg = InitScreen(argc, argv, i);
    if (screen_arg) {
        return screen_arg;
    }

    if (!strcmp(argv[i], "-dev")) {
        if ((i + 1 < argc) && (argv[i + 1][0] != '-')) {
            /* Handled by InitScreen */
            return 2;
        }
        UseMsg();
        exit(1);
    }

    if (!strcmp(argv[i], "-noshadow")) {
        msCurrScreen->shadow = FALSE;
        return 1;
    }

    if (!strcmp(argv[i], "-swap")) {
        msCurrScreen->format_swap = TRUE;
        return 1;
    }

    if (!strcmp(argv[i], "-notile")) {
        msCurrScreen->no_tile = TRUE;
        return 1;
    }

    if (!strcmp(argv[i], "-planar")) {
        msCurrScreen->planar = TRUE;
        return 1;
    }

    glamor_arg = KdGlamorParse(&msCurrScreen->glamor_info, argc, argv, i);
    if (glamor_arg) {
        return glamor_arg;
    }

    return KdProcessArgument(argc, argv, i);
}

void
ddxInit(void)
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
    .dpms             = msDPMS,
    .disable          = msDisable,
    .restore          = msRestore,
    .scrfini          = msScreenFini,
    .cardfini         = msCardFini,

    .initCursor       = msCursorInit,
    .enableCursor     = msCursorEnable,
    .disableCursor    = msCursorDisable,
    .finiCursor       = msCursorFini,
    .recolorCursor    = msRecolorCursor,

#ifdef GLAMOR
    .initAccel        = msGlamorInit,
    .enableAccel      = msGlamorEnable,
    .disableAccel     = msGlamorDisable,
    .finiAccel        = msGlamorFini,
#endif

    .getColors        = msGetColors,
    .putColors        = msPutColors,

    .closeScreen      = msDamageCloseScreen,
};
