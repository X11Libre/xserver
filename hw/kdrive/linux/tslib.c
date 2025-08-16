/*
 * TSLIB based touchscreen driver for KDrive
 * Porting to new input API and event queueing by Daniel Stone.
 * Derived from ts.c by Keith Packard
 * Derived from ps2.c by Jim Gettys
 *
 * Copyright © 1999 Keith Packard
 * Copyright © 2000 Compaq Computer Corporation
 * Copyright © 2002 MontaVista Software Inc.
 * Copyright © 2005 OpenedHand Ltd.
 * Copyright © 2006 Nokia Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the authors and/or copyright holders
 * not be used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  The authors and/or
 * copyright holders make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * THE AUTHORS AND/OR COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE AUTHORS AND/OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <dix-config.h>

#include <X11/X.h>
#include <X11/Xproto.h>
#include "inputstr.h"
#include "scrnintstr.h"
#include "kdrive.h"
#include <sys/ioctl.h>
#include <tslib.h>
#include <dirent.h>
#include <linux/input.h>

struct TslibPrivate {
    int fd;
    int lastx, lasty;
    struct tsdev *tsDev;
    void (*raw_event_hook) (int x, int y, int pressure, void *closure);
    void *raw_event_closure;
    int phys_screen;
};

static void
TsRead(int fd, void *closure)
{
    KdPointerInfo *pi = closure;
    struct TslibPrivate *private = pi->driverPrivate;
    struct ts_sample event;
    long x = 0, y = 0;
    unsigned long flags;

    if (private->raw_event_hook) {
        while (ts_read_raw(private->tsDev, &event, 1) == 1)
            private->raw_event_hook(event.x, event.y, event.pressure,
                                    private->raw_event_closure);
        return;
    }

    while (ts_read(private->tsDev, &event, 1) == 1) {
        if (event.pressure) {
            flags = KD_BUTTON_1;

            /*
             * Here we test for the touch screen driver actually being on the
             * touch screen, if it is we send absolute coordinates. If not,
             * then we send delta's so that we can track the entire vga screen.
             */
            if (KdCurScreen == private->phys_screen) {
                x = event.x;
                y = event.y;
            }
            else {
                flags |= KD_MOUSE_DELTA;
                if ((private->lastx == 0) || (private->lasty == 0)) {
                    x = event.x;
                    y = event.y;
                }
                else {
                    x = event.x - private->lastx;
                    y = event.y - private->lasty;
                }
            }
            private->lastx = event.x;
            private->lasty = event.y;
        }
        else {
            flags = 0;
            x = private->lastx;
            y = private->lasty;
        }

        KdEnqueuePointerEvent(pi, flags, x, y, event.pressure);
    }
}

static Status
TslibEnable(KdPointerInfo * pi)
{
    struct TslibPrivate *private = pi->driverPrivate;

    private->raw_event_hook = NULL;
    private->raw_event_closure = NULL;
    if (!pi->path) {
        pi->path = strdup("/dev/input/touchscreen0");
        ErrorF("[tslib/TslibEnable] no device path given, trying %s\n",
               pi->path);
    }

    private->tsDev = ts_open(pi->path, 0);
    if (!private->tsDev) {
        ErrorF("[tslib/TslibEnable] failed to open %s\n", pi->path);
        return BadAlloc;
    }

    if (ts_config(private->tsDev)) {
        ErrorF("[tslib/TslibEnable] failed to load configuration\n");
        ts_close(private->tsDev);
        private->tsDev = NULL;
        return BadValue;
    }

    private->fd = ts_fd(private->tsDev);

    KdRegisterFd(private->fd, TsRead, pi);

    return Success;
}

static void
TslibDisable(KdPointerInfo * pi)
{
    struct TslibPrivate *private = pi->driverPrivate;

    if (private->fd)
        KdUnregisterFd(pi, private->fd, TRUE);

    if (private->tsDev)
        ts_close(private->tsDev);

    private->fd = 0;
    private->tsDev = NULL;
}

static Status
TslibInit(KdPointerInfo * pi)
{
    struct TslibPrivate *private = NULL;

    if (!pi || !pi->dixdev)
        return !Success;

    pi->driverPrivate = (struct TslibPrivate *)
        calloc(sizeof(struct TslibPrivate), 1);
    if (!pi->driverPrivate)
        return !Success;

    private = pi->driverPrivate;
    /* hacktastic */
    private->phys_screen = 0;
    pi->nAxes = 3;
    pi->name = strdup("Touchscreen");
    pi->inputClass = KD_TOUCHSCREEN;

    return Success;
}

static void
TslibFini(KdPointerInfo * pi)
{
    free(pi->driverPrivate);
    pi->driverPrivate = NULL;
}

KdPointerDriver TsDriver = {
    .name = "tslib",
    .Init = TslibInit,
    .Enable = TslibEnable,
    .Disable = TslibDisable,
    .Fini = TslibFini,
};
