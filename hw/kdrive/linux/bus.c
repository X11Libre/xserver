/* SPDX-License-Identifier: HPND-sell-variant */
/*
 * Copyright  2000 Keith Packard
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
#include <X11/X.h>
#include <X11/Xproto.h>
#include "inputstr.h"
#include "scrnintstr.h"
#include "kdrive.h"

/* /dev/adbmouse is a busmouse */

static void
BusRead (int adbPort, void *closure)
{
    unsigned char   buf[3];
    int		    n;
    int		    dx, dy;
    unsigned long   flags;

    n = read (adbPort, buf, 3);
    if (n == 3)
    {
	flags = KD_MOUSE_DELTA;
	dx = (char) buf[1];
	dy = -(char) buf[2];
	if ((buf[0] & 4) == 0)
	    flags |= KD_BUTTON_1;
	if ((buf[0] & 2) == 0)
	    flags |= KD_BUTTON_2;
	if ((buf[0] & 1) == 0)
	    flags |= KD_BUTTON_3;
        KdEnqueuePointerEvent (closure, flags, dx, dy, 0);
    }
}

const char *BusNames[] = {
    "/dev/adbmouse",
    "/dev/mouse",
};

#define NUM_BUS_NAMES	(sizeof (BusNames) / sizeof (BusNames[0]))

static int
BusInit (KdPointerInfo *pi)
{
    int	    i, fd = 0;

    if (!pi->path || (strcmp(pi->path, "auto") == 0))
    {
        for (i = 0; i < NUM_BUS_NAMES; i++)
        {
            if ((fd = open (BusNames[i], 0)) >= 0)
            {
                close(fd);
                free(pi->path);
                pi->path = strdup(BusNames[i]);
                return Success;
            }
        }
    }
    else
    {
        if ((fd = open(pi->path, 0)) >= 0)
        {
            close(fd);
            return Success;
        }
    }

    return !Success;
}

static int
BusEnable (KdPointerInfo *pi)
{
    int fd = open(pi->path, 0);

    if (fd >= 0)
    {
        KdRegisterFd(fd, BusRead, pi);
        pi->driverPrivate = (void *)(intptr_t)fd;
        return Success;
    }
    else
    {
        return !Success;
    }
}

static void
BusDisable (KdPointerInfo *pi)
{
    KdUnregisterFd(pi, (int)(intptr_t)pi->driverPrivate, TRUE);
}

static void
BusFini (KdPointerInfo *pi)
{
    return;
}

KdPointerDriver BusMouseDriver = {
    .name = "bus",
    .Init = BusInit,
    .Enable = BusEnable,
    .Disable = BusDisable,
    .Fini = BusFini,
};
