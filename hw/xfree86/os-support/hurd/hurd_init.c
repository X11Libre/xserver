/*
 * Copyright 1997,1998 by UCHIYAMA Yasushi
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of UCHIYAMA Yasushi not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  UCHIYAMA Yasushi makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * UCHIYAMA YASUSHI DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL UCHIYAMA YASUSHI BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
#include <xorg-config.h>

#include <X11/X.h>

#include "input.h"
#include "scrnintstr.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_os_support.h"
#include "xf86_OSlib.h"

#include <errno.h>
#include <mach.h>
#include <hurd.h>

int
xf86ProcessArgument(int argc, char **argv, int i)
{
    return 0;
}

void
xf86UseMsg()
{
    return;
}

Bool
xf86VTKeepTtyIsSet(void)
{
     return FALSE;
}

void
xf86OpenConsole()
{
    if (serverGeneration == 1) {
        kern_return_t err;
        mach_port_t device;

        err = get_privileged_ports(NULL, &device);
        if (err) {
            errno = err;
            FatalError("xf86OpenConsole: can't get_privileged_ports. (%s)\n",
                       strerror(errno));
        }
        mach_port_deallocate(mach_task_self(), device);

        /*
         * Hurd has no VT concept, so leave consoleFd = -1.
         * Opening /dev/kbd here would clash with the external
         * kbd input driver which opens it for reading events.
         */
    }
    return;
}

void
xf86CloseConsole()
{
    if (xf86Info.consoleFd >= 0)
        close(xf86Info.consoleFd);
    return;
}

void
xf86OSInputThreadInit(void)
{
    return;
}
