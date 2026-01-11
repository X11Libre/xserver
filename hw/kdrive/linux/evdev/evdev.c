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
#include <errno.h>
#include <linux/input.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include "inputstr.h"
#include "scrnintstr.h"
#include "kdrive.h"
#include "evdev.h"

#define NUM_EVENTS  128
#define ABS_UNSET   -65535

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define ISBITSET(x,y) ((x)[LONG(y)] & BIT(y))
#define OFF(x)   ((x)%BITS_PER_LONG)
#define LONG(x)  ((x)/BITS_PER_LONG)
#define BIT(x)         (1 << OFF(x))

typedef struct _kevdev {
    /* current device state */
    int rel[REL_MAX + 1];
    int abs[ABS_MAX + 1];
    int prevabs[ABS_MAX + 1];
    long key[NBITS(KEY_MAX + 1)];

    /* supported device info */
    long relbits[NBITS(REL_MAX + 1)];
    long absbits[NBITS(ABS_MAX + 1)];
    long keybits[NBITS(KEY_MAX + 1)];
    struct input_absinfo absinfo[ABS_MAX + 1];
    int max_rel;
    int max_abs;

    int fd;
} Kevdev;

static void
EvdevPtrBtn(KdPointerInfo * pi, struct input_event *ev)
{
    int flags = KD_MOUSE_DELTA | pi->buttonState;

    if (ev->code >= BTN_MOUSE && ev->code < BTN_JOYSTICK) {
        switch (ev->code) {
        case BTN_LEFT:
            if (ev->value == 1)
                flags |= KD_BUTTON_1;
            else
                flags &= ~KD_BUTTON_1;
            break;
        case BTN_MIDDLE:
            if (ev->value == 1)
                flags |= KD_BUTTON_2;
            else
                flags &= ~KD_BUTTON_2;
            break;
        case BTN_RIGHT:
            if (ev->value == 1)
                flags |= KD_BUTTON_3;
            else
                flags &= ~KD_BUTTON_3;
            break;
        default:
            /* Unknow button */
            break;
        }

        KdEnqueuePointerEvent(pi, flags, 0, 0, 0);
    }
}

static void
EvdevPtrMotion(KdPointerInfo * pi, struct input_event *ev)
{
    Kevdev *ke = pi->driverPrivate;
    int i;
    int flags = KD_MOUSE_DELTA | pi->buttonState;

    for (i = 0; i <= ke->max_rel; i++)
        if (ke->rel[i]) {
            int a;

            for (a = 0; a <= ke->max_rel; a++) {
                if (ISBITSET(ke->relbits, a)) {
                    if (a == 0)
                        KdEnqueuePointerEvent(pi, flags, ke->rel[a], 0, 0);
                    else if (a == 1)
                        KdEnqueuePointerEvent(pi, flags, 0, ke->rel[a], 0);
                }
                ke->rel[a] = 0;
            }
            break;
        }
    for (i = 0; i < ke->max_abs; i++)
        if (ke->abs[i] != ke->prevabs[i]) {
            int a;

            ErrorF("abs");
            for (a = 0; a <= ke->max_abs; a++) {
                if (ISBITSET(ke->absbits, a))
                    ErrorF(" %d=%d", a, ke->abs[a]);
                ke->prevabs[a] = ke->abs[a];
            }
            ErrorF("\n");
            break;
        }

    if (ev->code == REL_WHEEL) {
        for (i = 0; i < abs(ev->value); i++) {
            if (ev->value > 0)
                flags |= KD_BUTTON_4;
            else
                flags |= KD_BUTTON_5;

            KdEnqueuePointerEvent(pi, flags, 0, 0, 0);

            if (ev->value > 0)
                flags &= ~KD_BUTTON_4;
            else
                flags &= ~KD_BUTTON_5;

            KdEnqueuePointerEvent(pi, flags, 0, 0, 0);
        }
    }

}

static void
EvdevPtrRead(int evdevPort, void *closure)
{
    KdPointerInfo *pi = closure;
    Kevdev *ke = pi->driverPrivate;
    int i;
    struct input_event events[NUM_EVENTS];
    int n;

    n = read(evdevPort, &events, NUM_EVENTS * sizeof(struct input_event));
    if (n <= 0) {
        if (errno == ENODEV)
            DeleteInputDeviceRequest(pi->dixdev);
        return;
    }

    n /= sizeof(struct input_event);
    for (i = 0; i < n; i++) {
        switch (events[i].type) {
        case EV_SYN:
            break;
        case EV_KEY:
            EvdevPtrBtn(pi, &events[i]);
            break;
        case EV_REL:
            ke->rel[events[i].code] += events[i].value;
            EvdevPtrMotion(pi, &events[i]);
            break;
        case EV_ABS:
            ke->abs[events[i].code] = events[i].value;
            EvdevPtrMotion(pi, &events[i]);
            break;
        }
    }
}

static Status
EvdevPtrInit(KdPointerInfo * pi)
{
    if (!pi->path) {
        pi->path = EvdevDefaultPtr();
    }
    else {
        int fd = open(pi->path, O_RDWR);
        if (fd < 0) {
            ErrorF("Failed to open evdev device %s\n", pi->path);
            return BadMatch;
        }
        close(fd);
    }

    if (!pi->name)
        pi->name = strdup("Evdev mouse");

    return Success;
}

static Status
EvdevPtrEnable(KdPointerInfo * pi)
{
    int fd;
    unsigned long ev[NBITS(EV_MAX)];
    Kevdev *ke;

    if (!pi || !pi->path)
        return BadImplementation;

    fd = open(pi->path, 2);
    if (fd < 0)
        return BadMatch;

    if (ioctl(fd, EVIOCGRAB, 1) < 0)
        perror("Grabbing evdev mouse device failed");

    if (ioctl(fd, EVIOCGBIT(0 /*EV*/, sizeof(ev)), ev) < 0) {
        perror("EVIOCGBIT 0");
        close(fd);
        return BadMatch;
    }
    ke = calloc(1, sizeof(Kevdev));
    if (!ke) {
        close(fd);
        return BadAlloc;
    }
    if (ISBITSET(ev, EV_KEY)) {
        if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(ke->keybits)), ke->keybits) < 0) {
            perror("EVIOCGBIT EV_KEY");
            free(ke);
            close(fd);
            return BadMatch;
        }
    }
    if (ISBITSET(ev, EV_REL)) {
        if (ioctl(fd, EVIOCGBIT(EV_REL, sizeof(ke->relbits)), ke->relbits) < 0) {
            perror("EVIOCGBIT EV_REL");
            free(ke);
            close(fd);
            return BadMatch;
        }
        for (ke->max_rel = REL_MAX; ke->max_rel >= 0; ke->max_rel--)
            if (ISBITSET(ke->relbits, ke->max_rel))
                break;
    }
    if (ISBITSET(ev, EV_ABS)) {
        int i;

        if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(ke->absbits)), ke->absbits) < 0) {
            perror("EVIOCGBIT EV_ABS");
            free(ke);
            close(fd);
            return BadMatch;
        }
        for (ke->max_abs = ABS_MAX; ke->max_abs >= 0; ke->max_abs--)
            if (ISBITSET(ke->absbits, ke->max_abs))
                break;
        for (i = 0; i <= ke->max_abs; i++) {
            if (ISBITSET(ke->absbits, i))
                if (ioctl(fd, EVIOCGABS(i), &ke->absinfo[i]) < 0) {
                    perror("EVIOCGABS");
                    break;
                }
            ke->prevabs[i] = ABS_UNSET;
        }
        if (i <= ke->max_abs) {
            free(ke);
            close(fd);
            return BadValue;
        }
    }
    if (!KdRegisterFd(fd, EvdevPtrRead, pi)) {
        free(ke);
        close(fd);
        return BadAlloc;
    }
    pi->driverPrivate = ke;
    ke->fd = fd;

    return Success;
}

static void
EvdevPtrDisable(KdPointerInfo * pi)
{
    Kevdev *ke;

    ke = pi->driverPrivate;

    if (!pi || !pi->driverPrivate)
        return;

    KdUnregisterFd(pi, ke->fd, TRUE);

    if (ioctl(ke->fd, EVIOCGRAB, 0) < 0)
        perror("Ungrabbing evdev mouse device failed");

    free(ke);
    pi->driverPrivate = 0;
}

static void
EvdevPtrFini(KdPointerInfo * pi)
{
}

/*
 * Evdev keyboard functions
 */

static void
readMapping(KdKeyboardInfo * ki)
{
    if (!ki)
        return;

    ki->minScanCode = 0;
    ki->maxScanCode = 247;
}

static void
EvdevKbdRead(int evdevPort, void *closure)
{
    KdKeyboardInfo *ki = closure;
    struct input_event events[NUM_EVENTS];
    int i, n;

    n = read(evdevPort, &events, NUM_EVENTS * sizeof(struct input_event));
    if (n <= 0) {
        if (errno == ENODEV)
            DeleteInputDeviceRequest(ki->dixdev);
        return;
    }

    n /= sizeof(struct input_event);
    for (i = 0; i < n; i++) {
        if (events[i].type == EV_KEY)
            KdEnqueueKeyboardEvent(ki, events[i].code, !events[i].value);
/* FIXME: must implement other types of events
        else
            ErrorF("Event type (%d) not delivered\n", events[i].type);
*/
    }
}

static Status
EvdevKbdInit(KdKeyboardInfo * ki)
{
    if (!ki->path) {
        ki->path = EvdevDefaultKbd();
    }
    else {
        int fd = open(ki->path, O_RDWR);
        if (fd < 0) {
            ErrorF("Failed to open evdev device %s\n", ki->path);
            return BadMatch;
        }
        close(fd);
    }

    if (!ki->name)
        ki->name = strdup("Evdev keyboard");

    readMapping(ki);

    return Success;
}

static Status
EvdevKbdEnable(KdKeyboardInfo * ki)
{
    unsigned long ev[NBITS(EV_MAX)];
    Kevdev *ke;
    int fd;

    if (!ki || !ki->path)
        return BadImplementation;

    fd = open(ki->path, O_RDWR);
    if (fd < 0)
        return BadMatch;

    if (ioctl(fd, EVIOCGRAB, 1) < 0)
        perror("Grabbing evdev keyboard device failed");

    if (ioctl(fd, EVIOCGBIT(0 /*EV*/, sizeof(ev)), ev) < 0) {
        perror("EVIOCGBIT 0");
        close(fd);
        return BadMatch;
    }

    ke = calloc(1, sizeof(Kevdev));
    if (!ke) {
        close(fd);
        return BadAlloc;
    }

    if (!KdRegisterFd(fd, EvdevKbdRead, ki)) {
        free(ke);
        close(fd);
        return BadAlloc;
    }
    ki->driverPrivate = ke;
    ke->fd = fd;

    return Success;
}

static void
EvdevKbdLeds(KdKeyboardInfo * ki, int leds)
{
    struct input_event event;
    Kevdev             *ke;
    int                i;

    if (!ki)
        return;

    ke = ki->driverPrivate;

    if (!ke)
        return;

    memset(&event, 0, sizeof(event));

    event.type = EV_LED;
    event.code = LED_CAPSL;
    event.value = leds & (1 << 0) ? 1 : 0;
    i = write(ke->fd, (char *) &event, sizeof(event));
    (void) i;

    event.type = EV_LED;
    event.code = LED_NUML;
    event.value = leds & (1 << 1) ? 1 : 0;
    i = write(ke->fd, (char *) &event, sizeof(event));
    (void) i;

    event.type = EV_LED;
    event.code = LED_SCROLLL;
    event.value = leds & (1 << 2) ? 1 : 0;
    i = write(ke->fd, (char *) &event, sizeof(event));
    (void) i;

    event.type = EV_LED;
    event.code = LED_COMPOSE;
    event.value = leds & (1 << 3) ? 1 : 0;
    i = write(ke->fd, (char *) &event, sizeof(event));
    (void) i;
}

static void
EvdevKbdBell(KdKeyboardInfo * ki, int volume, int frequency, int duration)
{
}

static void
EvdevKbdDisable(KdKeyboardInfo * ki)
{
    Kevdev *ke;

    ke = ki->driverPrivate;

    if (!ki || !ki->driverPrivate)
        return;

    KdUnregisterFd(ki, ke->fd, TRUE);

    if (ioctl(ke->fd, EVIOCGRAB, 0) < 0)
        perror("Ungrabbing evdev keyboard device failed");

    free(ke);
    ki->driverPrivate = 0;
}

static void
EvdevKbdFini(KdKeyboardInfo * ki)
{
}

KdPointerDriver LinuxEvdevMouseDriver = {
    .name = "evdev",
    .Init = EvdevPtrInit,
    .Enable = EvdevPtrEnable,
    .Disable = EvdevPtrDisable,
    .Fini = EvdevPtrFini,
};

KdKeyboardDriver LinuxEvdevKeyboardDriver = {
    .name = "evdev",
    .Init = EvdevKbdInit,
    .Enable = EvdevKbdEnable,
    .Leds = EvdevKbdLeds,
    .Bell = EvdevKbdBell,
    .Disable = EvdevKbdDisable,
    .Fini = EvdevKbdFini,
};
