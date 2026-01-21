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

#include "os/cmdline.h"
#include "os/ddx_priv.h"

void
InitCard(char *name)
{
    KdCardInfoAdd(&fbdevFuncs, 0);
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
        ("-fb path         Framebuffer device to use. Defaults to /dev/fb0\n");
    ErrorF
        ("-noshadow        Disable the ShadowFB layer if possible\n");
    ErrorF
        ("-glvendor        Suggest what glvnd vendor library should be used\n");
    ErrorF
        ("-force-gl        Force glamor to only use GL contexts\n");
    ErrorF
        ("-force-es        Force glamor to only use GLES contexts\n");
    ErrorF("\n");
}

int
ddxProcessArgument(int argc, char **argv, int i)
{
    if (!strcmp(argv[i], "-fb")) {
        if (i + 1 < argc) {
            fbdevDevicePath = argv[i + 1];
            return 2;
        }
        UseMsg();
        exit(1);
    }

    if (!strcmp(argv[i], "-noshadow")) {
        fbDisableShadow = TRUE;
        return 1;
    }

#ifdef GLAMOR
    if (!strcmp(argv[i], "-glvendor")) {
        if (i + 1 < argc) {
            fbdev_glvnd_provider = argv[i + 1];
            return 2;
        }
        UseMsg();
        exit(1);
    }

    if (!strcmp(argv[i], "-force-gl")) {
        es_allowed = FALSE;
        return 1;
    }

    if (!strcmp(argv[i], "-force-es")) {
        force_es = TRUE;
        return 1;
    }
#endif

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
