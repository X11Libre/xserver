/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111github@gmail.com>
 */

#include <kdrive-config.h>
#include "kstub.h"

static int
StubInit(void)
{
    return 1;
}

static void
StubEnable(void)
{
}

static Bool
StubSpecialKey(KeySym sym)
{
    return FALSE;
}

static void
StubDisable(void)
{
}

static void
StubFini(void)
{
}

static void
StubBell(int volume, int pitch, int duration)
{
}

void
StubAddInputDrivers(void)
{
    KdAddPointerDriver(&StubMouseDriver);
    KdAddKeyboardDriver(&StubKeyboardDriver);

    KdAddDefaultKeyboard("stub");
    KdAddDefaultPointer("stub");
}

KdOsFuncs StubOsFuncs = {
    .Init = StubInit,
    .Enable = StubEnable,
    .SpecialKey = StubSpecialKey,
    .Disable = StubDisable,
    .Fini = StubFini,
    .Bell = StubBell,
};
