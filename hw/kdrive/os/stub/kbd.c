/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111github@gmail.com>
 */

#include <kdrive-config.h>
#include "kstub.h"

static int
StubKeyboardInit(KdKeyboardInfo * ki)
{
    ki->minScanCode = 8;
    ki->maxScanCode = 255;
    return Success;
}

static Status
StubKeyboardEnable(KdKeyboardInfo * ki)
{
    return Success;
}

static void
StubKeyboardDisable(KdKeyboardInfo * ki)
{
}

static void
StubKeyboardFini(KdKeyboardInfo * ki)
{
}

static void
StubKeyboardLeds(KdKeyboardInfo * ki, int leds)
{
}

static void
StubKeyboardBell(KdKeyboardInfo * ki, int volume, int frequency, int duration)
{
}

KdKeyboardDriver StubKeyboardDriver = {
    .name ="stub",
    .PreInit = NULL,
    .Init = StubKeyboardInit,
    .Enable = StubKeyboardEnable,
    .Leds = StubKeyboardLeds,
    .Bell = StubKeyboardBell,
    .Disable = StubKeyboardDisable,
    .Fini = StubKeyboardFini,
};
