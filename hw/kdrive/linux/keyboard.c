/*
 * Copyright © 1999 Keith Packard
 * XKB integration © 2006 Nokia Corporation, author: Tomas Frydrych <tf@o-hand.com>
 *
 * LinuxKeyboardRead() XKB code based on xf86KbdLnx.c:
 * Copyright © 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 * Copyright © 1994-2001 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#include <kdrive-config.h>
#include "kdrive.h"
#include <linux/keyboard.h>
#include <linux/kd.h>
#define XK_PUBLISHING
#include <X11/keysym.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "os/xserver_poll.h"

extern int LinuxConsoleFd;

/*
 * We need these to handle extended scancodes correctly (I could just use the
 * numbers below, but this makes the code more readable
 */

/* The prefix codes */
#define KEY_Prefix0      /* special               0x60  */   96
#define KEY_Prefix1      /* special               0x61  */   97

/* The raw scancodes */
#define KEY_Enter        /* Enter                 0x1c  */   28
#define KEY_LCtrl        /* Ctrl(left)            0x1d  */   29
#define KEY_Slash        /* / (Slash)   ?         0x35  */   53
#define KEY_KP_Multiply  /* *                     0x37  */   55
#define KEY_Alt          /* Alt(left)             0x38  */   56
#define KEY_F3           /* F3                    0x3d  */   61
#define KEY_F4           /* F4                    0x3e  */   62
#define KEY_F5           /* F5                    0x3f  */   63
#define KEY_F6           /* F6                    0x40  */   64
#define KEY_F7           /* F7                    0x41  */   65
#define KEY_ScrollLock   /* ScrollLock            0x46  */   70
#define KEY_KP_7         /* 7           Home      0x47  */   71
#define KEY_KP_8         /* 8           Up        0x48  */   72
#define KEY_KP_9         /* 9           PgUp      0x49  */   73
#define KEY_KP_Minus     /* - (Minus)             0x4a  */   74
#define KEY_KP_4         /* 4           Left      0x4b  */   75
#define KEY_KP_5         /* 5                     0x4c  */   76
#define KEY_KP_6         /* 6           Right     0x4d  */   77
#define KEY_KP_Plus      /* + (Plus)              0x4e  */   78
#define KEY_KP_1         /* 1           End       0x4f  */   79
#define KEY_KP_2         /* 2           Down      0x50  */   80
#define KEY_KP_3         /* 3           PgDown    0x51  */   81
#define KEY_KP_0         /* 0           Insert    0x52  */   82
#define KEY_KP_Decimal   /* . (Decimal) Delete    0x53  */   83
#define KEY_Home         /* Home                  0x59  */   89
#define KEY_Up           /* Up                    0x5a  */   90
#define KEY_PgUp         /* PgUp                  0x5b  */   91
#define KEY_Left         /* Left                  0x5c  */   92
#define KEY_Begin        /* Begin                 0x5d  */   93
#define KEY_Right        /* Right                 0x5e  */   94
#define KEY_End          /* End                   0x5f  */   95
#define KEY_Down         /* Down                  0x60  */   96
#define KEY_PgDown       /* PgDown                0x61  */   97
#define KEY_Insert       /* Insert                0x62  */   98
#define KEY_Delete       /* Delete                0x63  */   99
#define KEY_KP_Enter     /* Enter                 0x64  */  100
#define KEY_RCtrl        /* Ctrl(right)           0x65  */  101
#define KEY_Pause        /* Pause                 0x66  */  102
#define KEY_Print        /* Print                 0x67  */  103
#define KEY_KP_Divide    /* Divide                0x68  */  104
#define KEY_AltLang      /* AtlLang(right)        0x69  */  105
#define KEY_Break        /* Break                 0x6a  */  106
#define KEY_LMeta        /* Left Meta             0x6b  */  107
#define KEY_RMeta        /* Right Meta            0x6c  */  108
#define KEY_Menu         /* Menu                  0x6d  */  109
#define KEY_F13          /* F13                   0x6e  */  110
#define KEY_F14          /* F14                   0x6f  */  111
#define KEY_F15          /* F15                   0x70  */  112
#define KEY_F16          /* F16                   0x71  */  113
#define KEY_F17          /* F17                   0x72  */  114
#define KEY_KP_DEC       /* KP_DEC                0x73  */  115

static void
LinuxKeyboardRead(int fd, void *closure)
{
    unsigned char buf[256], *b;
    int n;
    unsigned char prefix = 0, scancode = 0;

    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        b = buf;
        while (n--) {
            /*
             * With xkb we use RAW mode for reading the console, which allows us
             * process extended scancodes.
             *
             * See if this is a prefix extending the following keycode
             */
            if (!prefix && ((b[0] & 0x7f) == KEY_Prefix0)) {
                prefix = KEY_Prefix0;
                /* swallow this up */
                b++;
                continue;
            }
            else if (!prefix && ((b[0] & 0x7f) == KEY_Prefix1)) {
                prefix = KEY_Prefix1;
                /* swallow this up */
                b++;
                continue;
            }
            scancode = b[0] & 0x7f;

            switch (prefix) {
                /* from xf86Events.c */
            case KEY_Prefix0:
            {
                switch (scancode) {
                case KEY_KP_7:
                    scancode = KEY_Home;
                    break;      /* curs home */
                case KEY_KP_8:
                    scancode = KEY_Up;
                    break;      /* curs up */
                case KEY_KP_9:
                    scancode = KEY_PgUp;
                    break;      /* curs pgup */
                case KEY_KP_4:
                    scancode = KEY_Left;
                    break;      /* curs left */
                case KEY_KP_5:
                    scancode = KEY_Begin;
                    break;      /* curs begin */
                case KEY_KP_6:
                    scancode = KEY_Right;
                    break;      /* curs right */
                case KEY_KP_1:
                    scancode = KEY_End;
                    break;      /* curs end */
                case KEY_KP_2:
                    scancode = KEY_Down;
                    break;      /* curs down */
                case KEY_KP_3:
                    scancode = KEY_PgDown;
                    break;      /* curs pgdown */
                case KEY_KP_0:
                    scancode = KEY_Insert;
                    break;      /* curs insert */
                case KEY_KP_Decimal:
                    scancode = KEY_Delete;
                    break;      /* curs delete */
                case KEY_Enter:
                    scancode = KEY_KP_Enter;
                    break;      /* keypad enter */
                case KEY_LCtrl:
                    scancode = KEY_RCtrl;
                    break;      /* right ctrl */
                case KEY_KP_Multiply:
                    scancode = KEY_Print;
                    break;      /* print */
                case KEY_Slash:
                    scancode = KEY_KP_Divide;
                    break;      /* keyp divide */
                case KEY_Alt:
                    scancode = KEY_AltLang;
                    break;      /* right alt */
                case KEY_ScrollLock:
                    scancode = KEY_Break;
                    break;      /* curs break */
                case 0x5b:
                    scancode = KEY_LMeta;
                    break;
                case 0x5c:
                    scancode = KEY_RMeta;
                    break;
                case 0x5d:
                    scancode = KEY_Menu;
                    break;
                case KEY_F3:
                    scancode = KEY_F13;
                    break;
                case KEY_F4:
                    scancode = KEY_F14;
                    break;
                case KEY_F5:
                    scancode = KEY_F15;
                    break;
                case KEY_F6:
                    scancode = KEY_F16;
                    break;
                case KEY_F7:
                    scancode = KEY_F17;
                    break;
                case KEY_KP_Plus:
                    scancode = KEY_KP_DEC;
                    break;
                    /* Ignore virtual shifts (E0 2A, E0 AA, E0 36, E0 B6) */
                case 0x2A:
                case 0x36:
                    b++;
                    prefix = 0;
                    continue;
                default:
                    /*
                     * "Internet" keyboards are generating lots of new
                     * codes.  Let them pass.  There is little consistency
                     * between them, so don't bother with symbolic names at
                     * this level.
                     */
                    scancode += 0x78;
                }
                break;
            }

            case KEY_Prefix1:
            {
                /* we do no handle these */
                b++;
                prefix = 0;
                continue;
            }

            default:           /* should not happen */
            case 0:            /* do nothing */
                ;
            }

            prefix = 0;
            KdEnqueueKeyboardEvent(closure, scancode, b[0] & 0x80);
            b++;
        }
    }
}

static int LinuxKbdTrans;
static struct termios LinuxTermios;

static void
LinuxKeyboardFlush(int fd)
{
    struct pollfd poll_fd;
    unsigned char buf[256];

    if (tcflush(fd, TCIFLUSH) == 0) {
        return;
    }

    poll_fd.fd = fd;
    poll_fd.events = POLLIN;
    while (xserver_poll(&poll_fd, 1, 0) > 0) {
        if (read(fd, &buf, sizeof(buf)) < 1) {
            return;
        }
    }
}

static Status
LinuxKeyboardEnable(KdKeyboardInfo * ki)
{
    struct termios nTty;
    int fd;

    if (!ki)
        return !Success;

    fd = LinuxConsoleFd;
    ki->driverPrivate = (void *) (intptr_t) fd;

    ioctl(fd, KDGKBMODE, &LinuxKbdTrans);
    tcgetattr(fd, &LinuxTermios);
    ioctl(fd, KDSKBMODE, K_RAW);
    nTty = LinuxTermios;
    nTty.c_iflag = (IGNPAR | IGNBRK) & (~PARMRK) & (~ISTRIP);
    nTty.c_oflag = 0;
    nTty.c_cflag = CREAD | CS8;
    nTty.c_lflag = 0;
    nTty.c_cc[VTIME] = 0;
    nTty.c_cc[VMIN] = 1;
    cfsetispeed(&nTty, 9600);
    cfsetospeed(&nTty, 9600);
    tcsetattr(fd, TCSANOW, &nTty);
    /*
     * Flush any pending keystrokes
     */
    LinuxKeyboardFlush(fd);
    KdRegisterFd(fd, LinuxKeyboardRead, ki);
    return Success;
}

static void
LinuxKeyboardDisable(KdKeyboardInfo * ki)
{
    int fd;

    if (!ki)
        return;

    fd = (int) (intptr_t) ki->driverPrivate;

    KdUnregisterFd(ki, fd, FALSE);
    ioctl(fd, KDSKBMODE, LinuxKbdTrans);
    tcsetattr(fd, TCSANOW, &LinuxTermios);
}

static Status
LinuxKeyboardPreInit(KdKeyboardInfo * ki)
{
    ki->xkbRules = strdup("base");
    return Success;
}

static Status
LinuxKeyboardInit(KdKeyboardInfo * ki)
{
    if (!ki)
        return !Success;

    free(ki->path);
    ki->path = strdup("console");
    free(ki->name);
    ki->name = strdup("Linux console keyboard");

    ki->minScanCode = 0;
    ki->maxScanCode = 255;
    return Success;
}

static void
LinuxKeyboardLeds(KdKeyboardInfo * ki, int leds)
{
    if (!ki)
        return;

    ioctl((int) (intptr_t) ki->driverPrivate, KDSETLED, leds & 7);
}

KdKeyboardDriver LinuxKeyboardDriver = {
    .name = "keyboard",
    .PreInit = LinuxKeyboardPreInit,
    .Init = LinuxKeyboardInit,
    .Enable = LinuxKeyboardEnable,
    .Leds = LinuxKeyboardLeds,
    .Disable = LinuxKeyboardDisable,
};
