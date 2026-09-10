/*
 * Copyright © 2004 Keith Packard
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

#include "fake.h"
#include "kstub.h"

#include "os/cmdline.h"
#include "os/ddx_priv.h"

void
InitCard(char *name)
{
    KdCardInfoAdd(&fakeFuncs, 0);
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
    StubAddInputDrivers();
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
}

int
ddxProcessArgument(int argc, char **argv, int i)
{
    return KdProcessArgument(argc, argv, i);
}

void
ddxInit(void)
{
    KdOsInit(&StubOsFuncs);
}

KdCardFuncs fakeFuncs = {
    .cardinit         = fakeCardInit,
    .scrinit          = fakeScreenInit,
    .initScreen       = fakeInitScreen,
    .finishInitScreen = fakeFinishInitScreen,
    .createRes        = fakeCreateResources,
    .preserve         = fakePreserve,
    .enable           = fakeEnable,
    .dpms             = fakeDPMS,
    .disable          = fakeDisable,
    .restore          = fakeRestore,
    .scrfini          = fakeScreenFini,
    .cardfini         = fakeCardFini,

    /* no cursor funcs */
    /* XXX We could add sw-emulated versions of the hw cursor funcs for testing/reference purposes XXX */

    /* no accel funcs, for now */

    .getColors        = fakeGetColors,
    .putColors        = fakePutColors,

    /* no closescreen func */
};
