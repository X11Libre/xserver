#include <xorg-config.h>

#if defined(SYSCONS_SUPPORT)

#if defined(__FreeBSD__) || defined(__DragonFly__)
#include <sys/consio.h>
#include <sys/kbio.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>

#include "xf86Priv.h"
#include "xf86_console_priv.h"
#include "xf86_bsd_priv.h"

void xf86_console_syscons_close(void)
{
    struct vt_mode VT = { 0 };
    ioctl(xf86Info.consoleFd, KDSETMODE, KD_TEXT);  /* Back to text mode */
    if (ioctl(xf86Info.consoleFd, VT_GETMODE, &VT) != -1) {
        VT.mode = VT_AUTO;
        ioctl(xf86Info.consoleFd, VT_SETMODE, &VT); /* dflt vt handling */
    }
#if !defined(__OpenBSD__) && !defined(USE_DEV_IO) && !defined(USE_I386_IOPL)
    if (ioctl(xf86Info.consoleFd, KDDISABIO, 0) < 0) {
        xf86FatalError("xf86CloseConsole: KDDISABIO failed (%s)",
                       strerror(errno));
    }
#endif
    if (xf86Info.autoVTSwitch && initialVT != -1)
        ioctl(xf86Info.consoleFd, VT_ACTIVATE, initialVT);

    close(xf86Info.consoleFd);
    xf86Info.consoleFd = -1;
}

void xf86_console_syscons_reactivate(void)
{
    if (ioctl(xf86Info.consoleFd, VT_ACTIVATE, xf86Info.vtno) != 0)
        LogMessageVerb(X_WARNING, 1, "xf86_console_syscons_reactivate: VT_ACTIVATE failed\n");
}

/* The FreeBSD 1.1 version syscons driver uses /dev/ttyv0 */
#define SYSCONS_CONSOLE_DEV1 "/dev/ttyv0"
#define SYSCONS_CONSOLE_DEV2 "/dev/vga"
#define SYSCONS_CONSOLE_MODE O_RDWR|O_NDELAY

bool xf86_console_syscons_open(void)
{
    int fd = -1;
    vtmode_t vtmode;
    char vtname[12];
    long syscons_version;
    MessageType from;

    /* Check for syscons */
    if ((fd = open(SYSCONS_CONSOLE_DEV1, SYSCONS_CONSOLE_MODE, 0)) >= 0
        || (fd = open(SYSCONS_CONSOLE_DEV2, SYSCONS_CONSOLE_MODE, 0)) >= 0) {
        if (ioctl(fd, VT_GETMODE, &vtmode) >= 0) {
            /* Get syscons version */
            if (ioctl(fd, CONS_GETVERS, &syscons_version) < 0) {
                syscons_version = 0;
            }

            xf86Info.vtno = xf86_console_requested_vt;
            from = X_CMDLINE;

#ifdef VT_GETACTIVE
            if (ioctl(fd, VT_GETACTIVE, &initialVT) < 0)
                initialVT = -1;
#endif
            if (xf86Info.ShareVTs)
                xf86Info.vtno = initialVT;

            if (xf86Info.vtno == -1) {
                /*
                 * For old syscons versions (<0x100), VT_OPENQRY returns
                 * the current VT rather than the next free VT.  In this
                 * case, the server gets started on the current VT instead
                 * of the next free VT.
                 */

#if 0
                /* check for the fixed VT_OPENQRY */
                if (syscons_version >= 0x100) {
#endif
                    if (ioctl(fd, VT_OPENQRY, &xf86Info.vtno) < 0) {
                        /* No free VTs */
                        xf86Info.vtno = -1;
                    }
#if 0
                }
#endif

                if (xf86Info.vtno == -1) {
                    /*
                     * All VTs are in use.  If initialVT was found, use it.
                     */
                    if (initialVT != -1) {
                        xf86Info.vtno = initialVT;
                    }
                    else {
                        if (syscons_version >= 0x100)
                            FatalError("xf86_console_syscons_open: Cannot find a free VT");

                        /* Should no longer reach here */
                        FatalError(
                            "xf86_console_syscons_open: syscons versions prior to 1.0 require either\n"
                            "the server's stdin be a VT or the use of the vtxx server option");
                    }
                }
                from = X_PROBED;
            }

            close(fd);
            snprintf(vtname, sizeof(vtname), "/dev/ttyv%01x",
                     xf86Info.vtno - 1);
            if ((fd = open(vtname, SYSCONS_CONSOLE_MODE, 0)) < 0) {
                FatalError("xf86OpenSyscons: Cannot open %s (%s)",
                           vtname, strerror(errno));
            }
            if (ioctl(fd, VT_GETMODE, &vtmode) < 0) {
                FatalError("xf86OpenSyscons: VT_GETMODE failed");
            }
            xf86Info.consType = SYSCONS;
            LogMessageVerb(X_PROBED, 1, "Using syscons driver with X support");
            if (syscons_version >= 0x100) {
                LogMessageVerb(X_PROBED, 1, " (version %ld.%ld)\n", syscons_version >> 8,
                           syscons_version & 0xFF);
            }
            else {
                LogMessageVerb(X_PROBED, 1, " (version 0.x)\n");
            }
            LogMessageVerb(from, 1, "using VT number %d\n\n", xf86Info.vtno);
        }
        else {
            /* VT_GETMODE failed, probably not syscons */
            close(fd);
            fd = -1;
        }
    }
    xf86Info.consoleFd = fd;
    xf86_bsd_acquire_vt();
    xf86_console_proc_close = xf86_console_syscons_close;
    xf86_console_proc_reactivate = xf86_console_syscons_reactivate;
    return (fd > 0);
}

#endif /* SYSCONS_SUPPORT */
