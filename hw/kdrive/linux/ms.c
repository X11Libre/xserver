/*
Copyright (c) 2001 by Juliusz Chroboczek
Copyright (c) 1999 by Keith Packard

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <dix-config.h>
#include <errno.h>
#include <termios.h>
#include "os/xserver_poll.h"
#include <X11/X.h>
#include <X11/Xproto.h>
#include "inputstr.h"
#include "scrnintstr.h"
#include "kdrive.h"

static int
MsReadBytes(int fd, char *buf, int len, int min)
{
    int n, tot;
    struct pollfd poll_fd;

    poll_fd.fd = fd;
    poll_fd.events = POLLIN;
    tot = 0;
    while (len) {
        n = read(fd, buf, len);
        if (n > 0) {
            tot += n;
            buf += n;
            len -= n;
        }
        if (tot % min == 0)
            break;
        n = xserver_poll(&poll_fd, 1, 100);
        if (n <= 0)
            break;
    }
    return tot;
}

static void
MsRead(int port, void *closure)
{
    unsigned char buf[3 * 200];
    unsigned char *b;
    int n;
    int dx, dy;
    unsigned long flags;

    while ((n = MsReadBytes(port, (char *) buf, sizeof(buf), 3)) > 0) {
        b = buf;
        while (n >= 3) {
            flags = KD_MOUSE_DELTA;

            if (b[0] & 0x20)
                flags |= KD_BUTTON_1;
            if (b[0] & 0x10)
                flags |= KD_BUTTON_3;

            dx = (char) (((b[0] & 0x03) << 6) | (b[1] & 0x3F));
            dy = (char) (((b[0] & 0x0C) << 4) | (b[2] & 0x3F));
            n -= 3;
            b += 3;
            KdEnqueuePointerEvent(closure, flags, dx, dy, 0);
        }
    }
}

static Status
MsInit(KdPointerInfo * pi)
{
    if (!pi)
        return BadImplementation;

    if (!pi->path || strcmp(pi->path, "auto"))
        pi->path = strdup("/dev/mouse");
    if (!pi->name)
        pi->name = strdup("Microsoft protocol mouse");

    return Success;
}

static Status
MsEnable(KdPointerInfo * pi)
{
    int port;
    struct termios t;
    int ret;

    port = open(pi->path, O_RDWR | O_NONBLOCK);
    if (port < 0) {
        ErrorF("Couldn't open %s (%d)\n", pi->path, (int) errno);
        return 0;
    }
    else if (port == 0) {
        ErrorF("Opening %s returned 0!  Please complain to Keith.\n", pi->path);
        goto bail;
    }

    if (!isatty(port)) {
        ErrorF("%s is not a tty\n", pi->path);
        goto bail;
    }

    ret = tcgetattr(port, &t);
    if (ret < 0) {
        ErrorF("Couldn't tcgetattr(%s): %d\n", pi->path, errno);
        goto bail;
    }
    t.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR |
                   IGNCR | ICRNL | IXON | IXOFF);
    t.c_oflag &= ~OPOST;
    t.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    t.c_cflag &= ~(CSIZE | PARENB);
    t.c_cflag |= CS8 | CLOCAL | CSTOPB;

    cfsetispeed(&t, B1200);
    cfsetospeed(&t, B1200);
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;
    ret = tcsetattr(port, TCSANOW, &t);
    if (ret < 0) {
        ErrorF("Couldn't tcsetattr(%s): %d\n", pi->path, errno);
        goto bail;
    }
    if (KdRegisterFd(port, MsRead, pi))
        return TRUE;
    pi->driverPrivate = (void *) (intptr_t) port;

    return Success;

 bail:
    close(port);
    return BadMatch;
}

static void
MsDisable(KdPointerInfo * pi)
{
    KdUnregisterFd(pi, (int) (intptr_t) pi->driverPrivate, TRUE);
}

static void
MsFini(KdPointerInfo * pi)
{
}

KdPointerDriver MsMouseDriver = {
    .name = "ms",
    .Init = MsInit,
    .Enable = MsEnable,
    .Disable = MsDisable,
    .Fini = MsFini,
};
