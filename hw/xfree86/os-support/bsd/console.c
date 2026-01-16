/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <xorg-config.h>

#include "xf86_console_priv.h"
#include "xf86_os_support.h"

void xf86OSRingBell(int loudness, int pitch, int duration)
{
    if (xf86_console_proc_bell)
        xf86_console_proc_bell(loudness, pitch, duration);
}

Bool xf86VTSwitchAway(void)
{
    if (xf86_console_proc_switch_away)
        if (xf86_console_proc_switch_away())
            return TRUE;
    return FALSE;
}
