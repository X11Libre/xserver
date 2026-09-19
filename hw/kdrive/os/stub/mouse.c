/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111github@gmail.com>
 */

#include <kdrive-config.h>
#include "kstub.h"

static Status
StubMouseInit(KdPointerInfo * pi)
{
    char *tmp;

    if (!pi)
        return BadImplementation;

    tmp = strdup("Stub Mouse");
    if (tmp) {
        free(pi->name);
        pi->name = tmp;
    }

    return Success;
}

static Status
StubMouseEnable(KdPointerInfo * pi)
{
    return Success;
}

static void
StubMouseDisable(KdPointerInfo * pi)
{
    return;
}

static void
StubMouseFini(KdPointerInfo * pi)
{
    return;
}

KdPointerDriver StubMouseDriver = {
    .name = "stub",
    .Init = StubMouseInit,
    .Enable = StubMouseEnable,
    .Disable = StubMouseDisable,
    .Fini = StubMouseFini,
};
