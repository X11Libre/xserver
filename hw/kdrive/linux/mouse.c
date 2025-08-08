/*
 * Copyright © 2001 Keith Packard
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

#include <dix-config.h>
#include <errno.h>
#include <termios.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include "os/xserver_poll.h"
#include "inputstr.h"
#include "scrnintstr.h"
#include "kdrive.h"

#undef DEBUG
#undef DEBUG_BYTES
#define KBUFIO_SIZE 256
#define MOUSE_TIMEOUT	100

typedef struct _kbufio {
    int fd;
    unsigned char buf[KBUFIO_SIZE];
    int avail;
    int used;
} Kbufio;

static Bool
MouseWaitForReadable(int fd, int timeout)
{
    struct pollfd poll_fd;
    int n;
    CARD32 done;

    done = GetTimeInMillis() + timeout;
    poll_fd.fd = fd;
    poll_fd.events = POLLIN;
    for (;;) {
        n = xserver_poll(&poll_fd, 1, timeout);
        if (n > 0)
            return TRUE;
        if (n < 0 && (errno == EAGAIN || errno == EINTR)) {
            timeout = (int) (done - GetTimeInMillis());
            if (timeout > 0)
                continue;
        }
        break;
    }
    return FALSE;
}

static int
MouseReadByte(Kbufio * b, int timeout)
{
    int n;

    if (b->avail <= b->used) {
        if (timeout && !MouseWaitForReadable(b->fd, timeout)) {
#ifdef DEBUG_BYTES
            ErrorF("\tTimeout %d\n", timeout);
#endif
            return -1;
        }
        n = read(b->fd, b->buf, KBUFIO_SIZE);
        if (n <= 0)
            return -1;
        b->avail = n;
        b->used = 0;
    }
#ifdef DEBUG_BYTES
    ErrorF("\tget %02x\n", b->buf[b->used]);
#endif
    return b->buf[b->used++];
}

#if NOTUSED
static int
MouseFlush(Kbufio * b, char *buf, int size)
{
    CARD32 now = GetTimeInMillis();
    CARD32 done = now + 100;
    int c;
    int n = 0;

    while ((c = MouseReadByte(b, done - now)) != -1) {
        if (buf) {
            if (n == size) {
                memmove(buf, buf + 1, size - 1);
                n--;
            }
            buf[n++] = c;
        }
        now = GetTimeInMillis();
        if ((INT32) (now - done) >= 0)
            break;
    }
    return n;
}

static int
MousePeekByte(Kbufio * b, int timeout)
{
    int c;

    c = MouseReadByte(b, timeout);
    if (c != -1)
        --b->used;
    return c;
}
#endif                          /* NOTUSED */

static Bool
MouseWaitForWritable(int fd, int timeout)
{
    struct pollfd poll_fd;
    int n;

    poll_fd.fd = fd;
    poll_fd.events = POLLOUT;
    n = xserver_poll(&poll_fd, 1, timeout);
    if (n > 0)
        return TRUE;
    return FALSE;
}

static Bool
MouseWriteByte(int fd, unsigned char c, int timeout)
{
    int ret;

#ifdef DEBUG_BYTES
    ErrorF("\tput %02x\n", c);
#endif
    for (;;) {
        ret = write(fd, &c, 1);
        if (ret == 1)
            return TRUE;
        if (ret == 0)
            return FALSE;
        if (errno != EWOULDBLOCK)
            return FALSE;
        if (!MouseWaitForWritable(fd, timeout))
            return FALSE;
    }
}

static Bool
MouseWriteBytes(int fd, unsigned char *c, int n, int timeout)
{
    while (n--)
        if (!MouseWriteByte(fd, *c++, timeout))
            return FALSE;
    return TRUE;
}

#define MAX_MOUSE   10          /* maximum length of mouse protocol */
#define MAX_SKIP    16          /* number of error bytes before switching */
#define MAX_VALID   4           /* number of valid packets before accepting */

typedef struct _kmouseProt {
    const char *name;
    Bool (*Complete) (KdPointerInfo * pi, unsigned char *ev, int ne);
    int (*Valid) (KdPointerInfo * pi, unsigned char *ev, int ne);
    Bool (*Parse) (KdPointerInfo * pi, unsigned char *ev, int ne);
    Bool (*Init) (KdPointerInfo * pi);
    unsigned char headerMask, headerValid;
    unsigned char dataMask, dataValid;
    Bool tty;
    unsigned int c_iflag;
    unsigned int c_oflag;
    unsigned int c_lflag;
    unsigned int c_cflag;
    unsigned int speed;
    unsigned char *init;
    unsigned long state;
} KmouseProt;

typedef enum _kmouseStage {
    MouseBroken, MouseTesting, MouseWorking
} KmouseStage;

typedef struct _kmouse {
    Kbufio iob;
    const KmouseProt *prot;
    int i_prot;
    KmouseStage stage;          /* protocol verification stage */
    Bool tty;                   /* mouse device is a tty */
    int valid;                  /* sequential valid events */
    int tested;                 /* bytes scanned during Testing phase */
    int invalid;                /* total invalid bytes for this protocol */
    unsigned long state;        /* private per protocol, init to prot->state */
} Kmouse;

static int
mouseValid(KdPointerInfo * pi, unsigned char *ev, int ne)
{
    Kmouse *km = pi->driverPrivate;
    const KmouseProt *prot = km->prot;
    int i;

    for (i = 0; i < ne; i++)
        if ((ev[i] & prot->headerMask) == prot->headerValid)
            break;
    if (i != 0)
        return i;
    for (i = 1; i < ne; i++)
        if ((ev[i] & prot->dataMask) != prot->dataValid)
            return -1;
    return 0;
}

static Bool
threeComplete(KdPointerInfo * pi, unsigned char *ev, int ne)
{
    return ne == 3;
}

static Bool
fourComplete(KdPointerInfo * pi, unsigned char *ev, int ne)
{
    return ne == 4;
}

static Bool
fiveComplete(KdPointerInfo * pi, unsigned char *ev, int ne)
{
    return ne == 5;
}

static Bool
MouseReasonable(KdPointerInfo * pi, unsigned long flags, int dx, int dy)
{
    Kmouse *km = pi->driverPrivate;

    if (km->stage == MouseWorking)
        return TRUE;
    if (dx < -50 || dx > 50) {
#ifdef DEBUG
        ErrorF("Large X %d\n", dx);
#endif
        return FALSE;
    }
    if (dy < -50 || dy > 50) {
#ifdef DEBUG
        ErrorF("Large Y %d\n", dy);
#endif
        return FALSE;
    }
    return TRUE;
}

/*
 * Standard PS/2 mouse protocol
 */
static Bool
ps2Parse(KdPointerInfo * pi, unsigned char *ev, int ne)
{
    Kmouse *km = pi->driverPrivate;
    int dx, dy, dz;
    unsigned long flags;
    unsigned long flagsrelease = 0;

    flags = KD_MOUSE_DELTA;
    if (ev[0] & 4)
        flags |= KD_BUTTON_2;
    if (ev[0] & 2)
        flags |= KD_BUTTON_3;
    if (ev[0] & 1)
        flags |= KD_BUTTON_1;

    if (ne > 3) {
        dz = (int) (signed char) ev[3];
        if (dz < 0) {
            flags |= KD_BUTTON_4;
            flagsrelease = KD_BUTTON_4;
        }
        else if (dz > 0) {
            flags |= KD_BUTTON_5;
            flagsrelease = KD_BUTTON_5;
        }
    }

    dx = ev[1];
    if (ev[0] & 0x10)
        dx -= 256;
    dy = ev[2];
    if (ev[0] & 0x20)
        dy -= 256;
    dy = -dy;
    if (!MouseReasonable(pi, flags, dx, dy))
        return FALSE;
    if (km->stage == MouseWorking) {
        KdEnqueuePointerEvent(pi, flags, dx, dy, 0);
        if (flagsrelease) {
            flags &= ~flagsrelease;
            KdEnqueuePointerEvent(pi, flags, dx, dy, 0);
        }
    }
    return TRUE;
}

static Bool ps2Init(KdPointerInfo * pi);

static const KmouseProt ps2Prot = {
    "ps/2",
    threeComplete, mouseValid, ps2Parse, ps2Init,
    0x08, 0x08, 0x00, 0x00,
    FALSE
};

static const KmouseProt imps2Prot = {
    "imps/2",
    fourComplete, mouseValid, ps2Parse, ps2Init,
    0x08, 0x08, 0x00, 0x00,
    FALSE
};

static const KmouseProt exps2Prot = {
    "exps/2",
    fourComplete, mouseValid, ps2Parse, ps2Init,
    0x08, 0x08, 0x00, 0x00,
    FALSE
};

/*
 * Once the mouse is known to speak ps/2 protocol, go and find out
 * what advanced capabilities it has and turn them on
 */

/* these extracted from FreeBSD 4.3 sys/dev/kbd/atkbdcreg.h */

/* aux device commands (sent to KBD_DATA_PORT) */
#define PSMC_SET_SCALING11      0x00e6
#define PSMC_SET_SCALING21      0x00e7
#define PSMC_SET_RESOLUTION     0x00e8
#define PSMC_SEND_DEV_STATUS    0x00e9
#define PSMC_SET_STREAM_MODE    0x00ea
#define PSMC_SEND_DEV_DATA      0x00eb
#define PSMC_SET_REMOTE_MODE    0x00f0
#define PSMC_SEND_DEV_ID        0x00f2
#define PSMC_SET_SAMPLING_RATE  0x00f3
#define PSMC_ENABLE_DEV         0x00f4
#define PSMC_DISABLE_DEV        0x00f5
#define PSMC_SET_DEFAULTS       0x00f6
#define PSMC_RESET_DEV          0x00ff

/* PSMC_SET_RESOLUTION argument */
#define PSMD_RES_LOW            0       /* typically 25ppi */
#define PSMD_RES_MEDIUM_LOW     1       /* typically 50ppi */
#define PSMD_RES_MEDIUM_HIGH    2       /* typically 100ppi (default) */
#define PSMD_RES_HIGH           3       /* typically 200ppi */
#define PSMD_MAX_RESOLUTION     PSMD_RES_HIGH

/* PSMC_SET_SAMPLING_RATE */
#define PSMD_MAX_RATE           255     /* FIXME: not sure if it's possible */

/* aux device ID */
#define PSM_MOUSE_ID            0
#define PSM_BALLPOINT_ID        2
#define PSM_INTELLI_ID          3
#define PSM_EXPLORER_ID         4
#define PSM_4DMOUSE_ID          6
#define PSM_4DPLUS_ID           8

static unsigned char ps2_init[] = {
    PSMC_ENABLE_DEV
};

#define NINIT_PS2   1

static unsigned char wheel_3button_init[] = {
    PSMC_SET_SAMPLING_RATE, 200,
    PSMC_SET_SAMPLING_RATE, 100,
    PSMC_SET_SAMPLING_RATE, 80,
    PSMC_SEND_DEV_ID,
};

#define NINIT_IMPS2 4

static unsigned char wheel_5button_init[] = {
    PSMC_SET_SAMPLING_RATE, 200,
    PSMC_SET_SAMPLING_RATE, 100,
    PSMC_SET_SAMPLING_RATE, 80,
    PSMC_SET_SAMPLING_RATE, 200,
    PSMC_SET_SAMPLING_RATE, 200,
    PSMC_SET_SAMPLING_RATE, 80,
    PSMC_SEND_DEV_ID,
};

#define NINIT_EXPS2 7

static unsigned char intelli_init[] = {
    PSMC_SET_SAMPLING_RATE, 200,
    PSMC_SET_SAMPLING_RATE, 100,
    PSMC_SET_SAMPLING_RATE, 80,
};

#define NINIT_INTELLI	3

static int
ps2SkipInit(KdPointerInfo * pi, int ninit, Bool ret_next)
{
    Kmouse *km = pi->driverPrivate;
    int c = -1;
    Bool waiting;

    waiting = FALSE;
    while (ninit || ret_next) {
        c = MouseReadByte(&km->iob, 1); /* Minimum timeout like in xf86-input-mouse and tinyx */
        if (c == -1)
            break;
        /* look for ACK */
        if (c == 0xfa) {
            ninit--;
            if (ret_next)
                waiting = TRUE;
        }
        /* look for packet start -- not the response */
        else if ((c & 0x08) == 0x08)
            waiting = FALSE;
        else if (waiting)
            break;
    }
    return c;
}

static Bool
ps2Init(KdPointerInfo * pi)
{
    Kmouse *km = pi->driverPrivate;
    int id;
    unsigned char *init;
    int ninit;
    int len;

    /* Send Intellimouse initialization sequence */
    MouseWriteBytes(km->iob.fd, intelli_init, sizeof(intelli_init),
                    100);
    /*
     * Send ID command
     */
    if (!MouseWriteByte(km->iob.fd, PSMC_SEND_DEV_ID, 100))
        return FALSE;
    id = ps2SkipInit(pi, 0, TRUE);
    switch (id) {
    case 3:
        init = wheel_3button_init;
        ninit = NINIT_IMPS2;
        km->prot = &imps2Prot;
        len = sizeof(wheel_3button_init);
        break;
    case 4:
        init = wheel_5button_init;
        ninit = NINIT_EXPS2;
        km->prot = &exps2Prot;
        len = sizeof(wheel_5button_init);
        break;
    default:
        init = ps2_init;
        ninit = NINIT_PS2;
        km->prot = &ps2Prot;
        len = sizeof(ps2_init);
        break;
    }
    if (init)
        MouseWriteBytes(km->iob.fd, init, len, 100);
    /*
     * Flush out the available data to eliminate responses to the
     * initialization string.  Make sure any partial event is
     * skipped
     */
    (void) ps2SkipInit(pi, ninit, FALSE);
    return TRUE;
}

static Bool
busParse(KdPointerInfo * pi, unsigned char *ev, int ne)
{
    Kmouse *km = pi->driverPrivate;
    int dx, dy;
    unsigned long flags;

    flags = KD_MOUSE_DELTA;
    dx = (signed char) ev[1];
    dy = -(signed char) ev[2];
    if ((ev[0] & 4) == 0)
        flags |= KD_BUTTON_1;
    if ((ev[0] & 2) == 0)
        flags |= KD_BUTTON_2;
    if ((ev[0] & 1) == 0)
        flags |= KD_BUTTON_3;
    if (!MouseReasonable(pi, flags, dx, dy))
        return FALSE;
    if (km->stage == MouseWorking)
        KdEnqueuePointerEvent(pi, flags, dx, dy, 0);
    return TRUE;
}

static const KmouseProt busProt = {
    "bus",
    threeComplete, mouseValid, busParse, 0,
    0xf8, 0x00, 0x00, 0x00,
    FALSE
};

/*
 * Standard MS serial protocol, three bytes
 */

static Bool
msParse(KdPointerInfo * pi, unsigned char *ev, int ne)
{
    Kmouse *km = pi->driverPrivate;
    int dx, dy;
    unsigned long flags;

    flags = KD_MOUSE_DELTA;

    if (ev[0] & 0x20)
        flags |= KD_BUTTON_1;
    if (ev[0] & 0x10)
        flags |= KD_BUTTON_3;

    dx = (signed char) (((ev[0] & 0x03) << 6) | (ev[1] & 0x3F));
    dy = (signed char) (((ev[0] & 0x0C) << 4) | (ev[2] & 0x3F));
    if (!MouseReasonable(pi, flags, dx, dy))
        return FALSE;
    if (km->stage == MouseWorking)
        KdEnqueuePointerEvent(pi, flags, dx, dy, 0);
    return TRUE;
}

static const KmouseProt msProt = {
    "ms",
    threeComplete, mouseValid, msParse, 0,
    0xc0, 0x40, 0xc0, 0x00,
    TRUE,
    IGNPAR,
    0,
    0,
    CS7 | CSTOPB | CREAD | CLOCAL,
    B1200,
};

/*
 * Logitech mice send 3 or 4 bytes, the only way to tell is to look at the
 * first byte of a synchronized protocol stream and see if it's got
 * any bits turned on that can't occur in that fourth byte
 */
static Bool
logiComplete(KdPointerInfo * pi, unsigned char *ev, int ne)
{
    Kmouse *km = pi->driverPrivate;

    if ((ev[0] & 0x40) == 0x40)
        return ne == 3;
    if (km->stage != MouseBroken && (ev[0] & ~0x23) == 0)
        return ne == 1;
    return FALSE;
}

static int
logiValid(KdPointerInfo * pi, unsigned char *ev, int ne)
{
    Kmouse *km = pi->driverPrivate;
    const KmouseProt *prot = km->prot;
    int i;

    for (i = 0; i < ne; i++) {
        if ((ev[i] & 0x40) == 0x40)
            break;
        if (km->stage != MouseBroken && (ev[i] & ~0x23) == 0)
            break;
    }
    if (i != 0)
        return i;
    for (i = 1; i < ne; i++)
        if ((ev[i] & prot->dataMask) != prot->dataValid)
            return -1;
    return 0;
}

static Bool
logiParse(KdPointerInfo * pi, unsigned char *ev, int ne)
{
    Kmouse *km = pi->driverPrivate;
    int dx, dy;
    unsigned long flags;

    flags = KD_MOUSE_DELTA;

    if (ne == 3) {
        if (ev[0] & 0x20)
            flags |= KD_BUTTON_1;
        if (ev[0] & 0x10)
            flags |= KD_BUTTON_3;

        dx = (signed char) (((ev[0] & 0x03) << 6) | (ev[1] & 0x3F));
        dy = (signed char) (((ev[0] & 0x0C) << 4) | (ev[2] & 0x3F));
        flags |= km->state & KD_BUTTON_2;
    }
    else {
        if (ev[0] & 0x20)
            flags |= KD_BUTTON_2;
        dx = 0;
        dy = 0;
        flags |= km->state & (KD_BUTTON_1 | KD_BUTTON_3);
    }

    if (!MouseReasonable(pi, flags, dx, dy))
        return FALSE;
    if (km->stage == MouseWorking)
        KdEnqueuePointerEvent(pi, flags, dx, dy, 0);
    return TRUE;
}

static const KmouseProt logiProt = {
    "logitech",
    logiComplete, logiValid, logiParse, 0,
    0xc0, 0x40, 0xc0, 0x00,
    TRUE,
    IGNPAR,
    0,
    0,
    CS7 | CSTOPB | CREAD | CLOCAL,
    B1200,
};

/*
 * Mouse systems protocol, 5 bytes
 */
static Bool
mscParse(KdPointerInfo * pi, unsigned char *ev, int ne)
{
    Kmouse *km = pi->driverPrivate;
    int dx, dy;
    unsigned long flags;

    flags = KD_MOUSE_DELTA;

    if (!(ev[0] & 0x4))
        flags |= KD_BUTTON_1;
    if (!(ev[0] & 0x2))
        flags |= KD_BUTTON_2;
    if (!(ev[0] & 0x1))
        flags |= KD_BUTTON_3;
    dx = (signed char) (ev[1]) + (signed char) (ev[3]);
    dy = -((signed char) (ev[2]) + (signed char) (ev[4]));

    if (!MouseReasonable(pi, flags, dx, dy))
        return FALSE;
    if (km->stage == MouseWorking)
        KdEnqueuePointerEvent(pi, flags, dx, dy, 0);
    return TRUE;
}

static const KmouseProt mscProt = {
    "msc",
    fiveComplete, mouseValid, mscParse, 0,
    0xf8, 0x80, 0x00, 0x00,
    TRUE,
    IGNPAR,
    0,
    0,
    CS8 | CSTOPB | CREAD | CLOCAL,
    B1200,
};

/*
 * Use logitech before ms -- they're the same except that
 * logitech sometimes has a fourth byte
 */
static const KmouseProt *kmouseProts[] = {
    &ps2Prot, &imps2Prot, &exps2Prot, &busProt, &logiProt, &msProt, &mscProt,
};

#define NUM_PROT    (sizeof (kmouseProts) / sizeof (kmouseProts[0]))

static void
MouseInitProtocol(Kmouse * km)
{
    int ret;
    struct termios t;

    if (km->prot->tty) {
        ret = tcgetattr(km->iob.fd, &t);

        if (ret >= 0) {
            t.c_iflag = km->prot->c_iflag;
            t.c_oflag = km->prot->c_oflag;
            t.c_lflag = km->prot->c_lflag;
            t.c_cflag = km->prot->c_cflag;
            cfsetispeed(&t, km->prot->speed);
            cfsetospeed(&t, km->prot->speed);
            ret = tcsetattr(km->iob.fd, TCSANOW, &t);
        }
    }
    km->stage = MouseBroken;
    km->valid = 0;
    km->tested = 0;
    km->invalid = 0;
    km->state = km->prot->state;
}

static void
MouseFirstProtocol(Kmouse * km, const char *prot)
{
    if (prot) {
        for (km->i_prot = 0; km->i_prot < NUM_PROT; km->i_prot++)
            if (!strcmp(prot, kmouseProts[km->i_prot]->name))
                break;
        if (km->i_prot == NUM_PROT) {
            int i;

            ErrorF("Unknown mouse protocol \"%s\". Pick one of:", prot);
            for (i = 0; i < NUM_PROT; i++)
                ErrorF(" %s", kmouseProts[i]->name);
            ErrorF("\n");
            km->i_prot = 0;
            km->prot = kmouseProts[km->i_prot];
            ErrorF("Falling back to %s\n", km->prot->name);
        }
        else {
            km->prot = kmouseProts[km->i_prot];
            if (km->tty && !km->prot->tty)
                ErrorF
                    ("Mouse device is serial port, protocol %s is not serial protocol\n",
                     prot);
            else if (!km->tty && km->prot->tty)
                ErrorF
                    ("Mouse device is not serial port, protocol %s is serial protocol\n",
                     prot);
        }
    }
    if (!km->prot) {
        for (km->i_prot = 0; kmouseProts[km->i_prot]->tty != km->tty;
             km->i_prot++);
        km->prot = kmouseProts[km->i_prot];
    }
    MouseInitProtocol(km);
}

static void
MouseNextProtocol(Kmouse * km)
{
    do {
        if (!km->prot)
            km->i_prot = 0;
        else if (++km->i_prot >= NUM_PROT)
            km->i_prot = 0;
        km->prot = kmouseProts[km->i_prot];
    } while (km->prot->tty != km->tty);
    MouseInitProtocol(km);
    ErrorF("Switching to mouse protocol \"%s\"\n", km->prot->name);
}

static void
MouseRead(int mousePort, void *closure)
{
    KdPointerInfo *pi = closure;
    Kmouse *km = pi->driverPrivate;
    unsigned char event[MAX_MOUSE];
    int ne;
    int c;
    int i;
    int timeout;

    timeout = 0;
    ne = 0;
    for (;;) {
        c = MouseReadByte(&km->iob, timeout);
        if (c == -1) {
            if (ne) {
                km->invalid += ne + km->tested;
                km->valid = 0;
                km->tested = 0;
                km->stage = MouseBroken;
            }
            break;
        }
        event[ne++] = c;
        i = (*km->prot->Valid) (pi, event, ne);
        if (i != 0) {
#ifdef DEBUG
            ErrorF("Mouse protocol %s broken %d of %d bytes bad\n",
                   km->prot->name, i > 0 ? i : ne, ne);
#endif
            if (i > 0 && i < ne) {
                ne -= i;
                memmove(event, event + i, ne);
            }
            else {
                i = ne;
                ne = 0;
            }
            km->invalid += i + km->tested;
            km->valid = 0;
            km->tested = 0;
            if (km->stage == MouseWorking)
                km->i_prot--;
            km->stage = MouseBroken;
            if (km->invalid > MAX_SKIP) {
                MouseNextProtocol(km);
                ne = 0;
            }
            timeout = 0;
        }
        else {
            if ((*km->prot->Complete) (pi, event, ne)) {
                if ((*km->prot->Parse) (pi, event, ne)) {
                    switch (km->stage) {
                    case MouseBroken:
#ifdef DEBUG
                        ErrorF("Mouse protocol %s seems OK\n", km->prot->name);
#endif
                        /* do not zero invalid to accumulate invalid bytes */
                        km->valid = 0;
                        km->tested = 0;
                        km->stage = MouseTesting;
                        /* fall through ... */
                    case MouseTesting:
                        km->valid++;
                        km->tested += ne;
                        if (km->valid > MAX_VALID) {
#ifdef DEBUG
                            ErrorF("Mouse protocol %s working\n",
                                   km->prot->name);
#endif
                            km->stage = MouseWorking;
                            km->invalid = 0;
                            km->tested = 0;
                            km->valid = 0;
                            if (km->prot->Init && !(*km->prot->Init) (pi))
                                km->stage = MouseBroken;
                        }
                        break;
                    case MouseWorking:
                        break;
                    }
                }
                else {
                    km->invalid += ne + km->tested;
                    km->valid = 0;
                    km->tested = 0;
                    km->stage = MouseBroken;
                }
                ne = 0;
                timeout = 0;
            }
            else
                timeout = MOUSE_TIMEOUT;
        }
    }
}

const char *kdefaultMouse[] = {
    "/dev/input/mice",
    "/dev/mouse",
    "/dev/psaux",
    "/dev/adbmouse",
    "/dev/ttyS0",
    "/dev/ttyS1",
};

#define NUM_DEFAULT_MOUSE    (sizeof (kdefaultMouse) / sizeof (kdefaultMouse[0]))

static Status
MouseInit(KdPointerInfo * pi)
{
    int i;
    int fd;
    Kmouse *km;

    if (!pi)
        return BadImplementation;

    if (!pi->path || strcmp(pi->path, "auto") == 0) {
        for (i = 0; i < NUM_DEFAULT_MOUSE; i++) {
            fd = open(kdefaultMouse[i], 2);
            if (fd >= 0) {
                pi->path = strdup(kdefaultMouse[i]);
                break;
            }
        }
    }
    else {
        fd = open(pi->path, 2);
    }

    if (fd < 0)
        return BadMatch;

    km = (Kmouse *) malloc(sizeof(Kmouse));
    if (km) {
        km->iob.avail = km->iob.used = 0;
        MouseFirstProtocol(km, pi->protocol ? pi->protocol : "ps/2");
        /* MouseFirstProtocol sets state to MouseBroken for later protocol
         * checks. Skip these checks if a protocol was supplied */
        if (pi->protocol)
            km->state = MouseWorking;
        km->i_prot = 0;
        km->tty = isatty(fd);
        km->iob.fd = fd;
        pi->driverPrivate = km;
    }
    else {
        close(fd);
        return BadAlloc;
    }

    return Success;
}

static Status
MouseEnable(KdPointerInfo * pi)
{
    Kmouse *km;

    if (!pi || !pi->driverPrivate || !pi->path)
        return BadImplementation;

    km = pi->driverPrivate;

    km->iob.fd = open(pi->path, 2);
    if (km->iob.fd < 0)
        return BadMatch;

    if (!KdRegisterFd(km->iob.fd, MouseRead, pi)) {
        close(km->iob.fd);
        return BadAlloc;
    }

    return Success;
}

static void
MouseDisable(KdPointerInfo * pi)
{
    Kmouse *km;

    if (!pi || !pi->driverPrivate)
        return;

    km = pi->driverPrivate;
    KdUnregisterFd(pi, km->iob.fd, TRUE);
}

static void
MouseFini(KdPointerInfo * pi)
{
    free(pi->driverPrivate);
    pi->driverPrivate = NULL;
}

KdPointerDriver LinuxMouseDriver = {
    .name = "mouse",
    .Init = MouseInit,
    .Enable = MouseEnable,
    .Disable = MouseDisable,
    .Fini = MouseFini,
};
