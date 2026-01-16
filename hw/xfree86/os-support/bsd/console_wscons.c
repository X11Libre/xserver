#include <xorg-config.h>

#if defined(WSCONS_SUPPORT)

#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <dev/wscons/wsconsio.h>

#include "xf86Priv.h"
#include "xf86_console_priv.h"
#include "xf86_bsd_priv.h"

#define CHECK_DRIVER_MSG \
  "Check your kernel's console driver configuration and /dev entries"

void xf86_console_wscons_close(void)
{
    int mode = WSDISPLAYIO_MODE_EMUL;
    ioctl(xf86Info.consoleFd, WSDISPLAYIO_SMODE, &mode);

    close(xf86Info.consoleFd);
    xf86Info.consoleFd = -1;
}

static void xf86_console_wscons_bell(int loudness, int pitch, int duration)
{
    if (loudness && pitch) {
        struct wskbd_bell_data wsb = {
            .which = WSKBD_BELL_DOALL,
            .pitch = pitch,
            .period = duration,
            .volume = loudness,
        };
        ioctl(xf86Info.consoleFd, WSKBDIO_COMPLEXBELL, &wsb);
    }
}

bool xf86_console_wscons_open(void)
{
    int fd = -1;
    int mode = WSDISPLAYIO_MODE_MAPPED;
    int i;
    char ttyname[16];

    /* XXX Is this ok? */
    for (i = 0; i < 8; i++) {
#if defined(__NetBSD__)
        snprintf(ttyname, sizeof(ttyname), "/dev/ttyE%d", i);
#elif defined(__OpenBSD__)
        snprintf(ttyname, sizeof(ttyname), "/dev/ttyC%x", i);
#endif
        if ((fd = open(ttyname, 2)) != -1)
            break;
    }
    if (fd != -1) {
        if (ioctl(fd, WSDISPLAYIO_SMODE, &mode) < 0) {
            FatalError("%s: WSDISPLAYIO_MODE_MAPPED failed (%s)\n%s",
                       "xf86OpenConsole", strerror(errno), CHECK_DRIVER_MSG);
        }
        xf86Info.consType = WSCONS;
        LogMessageVerb(X_PROBED, 1, "Using wscons driver\n");
    }
    xf86Info.consoleFd = fd;

    xf86_console_proc_bell = xf86_console_wscons_bell;
    xf86_console_proc_close = xf86_console_wscons_close;

    /* nothing special to do for acquiring the VT */
    return (fd > 0);
}

#endif /* WSCONS_SUPPORT */
