#include <xorg-config.h>

#if defined(PCVT_SUPPORT)

#define CHECK_DRIVER_MSG \
  "Check your kernel's console driver configuration and /dev entries"

#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#if defined(__NetBSD__)
#include <dev/wscons/wsdisplay_usl_io.h>
#endif

#if defined(__FreeBSD__) || defined(__DragonFly__)
#include <sys/consio.h>
#include <sys/kbio.h>
#endif

#include "xf86Priv.h"
#include "xf86_priv.h"
#include "xf86_bsd_priv.h"
#include "xf86_console_priv.h"

#ifndef __OpenBSD__
#define PCVT_CONSOLE_DEV "/dev/ttyv0"
#else
#define PCVT_CONSOLE_DEV "/dev/ttyC0"
#endif
#define PCVT_CONSOLE_MODE O_RDWR|O_NDELAY

void xf86_console_pcvt_close(void)
{
    struct vt_mode VT = { 0 };
    ioctl(xf86Info.consoleFd, KDSETMODE, KD_TEXT);  /* Back to text mode */
    if (ioctl(xf86Info.consoleFd, VT_GETMODE, &VT) != -1) {
        VT.mode = VT_AUTO;
        ioctl(xf86Info.consoleFd, VT_SETMODE, &VT); /* dflt vt handling */
    }
#if !defined(__OpenBSD__) && !defined(USE_DEV_IO) && !defined(USE_I386_IOPL)
    if (ioctl(xf86Info.consoleFd, KDDISABIO, 0) < 0) {
        FatalError("xf86CloseConsole: KDDISABIO failed (%s)", strerror(errno));
    }
#endif
    if (xf86Info.autoVTSwitch && initialVT != -1)
        ioctl(xf86Info.consoleFd, VT_ACTIVATE, initialVT);

    close(xf86Info.consoleFd);
    xf86Info.consoleFd = -1;
}

void xf86_console_pcvt_reactivate(void)
{
    if (ioctl(xf86Info.consoleFd, VT_ACTIVATE, xf86Info.vtno) != 0)
        LogMessageVerb(X_WARNING, 1, "xf86_console_pcvt_reactivate: VT_ACTIVATE failed\n");
}

static void xf86_console_pcvt_bell(int loudness, int pitch, int duration)
{
    if (loudness && pitch) {
        ioctl(xf86Info.consoleFd, KDMKTONE,
              ((1193190 / pitch) & 0xffff) |
              (((unsigned long) duration * loudness / 50) << 16));
    }
}

bool xf86_console_pcvt_open(void)
{
    /* This looks much like syscons, since pcvt is API compatible */
    int fd = -1;
    vtmode_t vtmode;
    char vtname[12];
    const char *vtprefix;
#ifdef __NetBSD__
    struct pcvtid pcvt_version;
#endif

#ifndef __OpenBSD__
    vtprefix = "/dev/ttyv";
#else
    vtprefix = "/dev/ttyC";
#endif

    fd = open(PCVT_CONSOLE_DEV, PCVT_CONSOLE_MODE, 0);
#ifdef WSCONS_PCVT_COMPAT_CONSOLE_DEV
    if (fd < 0) {
        fd = open(WSCONS_PCVT_COMPAT_CONSOLE_DEV, PCVT_CONSOLE_MODE, 0);
        vtprefix = "/dev/ttyE";
    }
#endif
    if (fd >= 0) {
#ifdef __NetBSD__
        if (ioctl(fd, VGAPCVTID, &pcvt_version) >= 0) {
#endif
            if (ioctl(fd, VT_GETMODE, &vtmode) < 0) {
                FatalError("%s: VT_GETMODE failed\n%s%s\n%s",
                           "xf86OpenPcvt",
                           "Found pcvt driver but X11 seems to be",
                           " not supported.", CHECK_DRIVER_MSG);
            }

            xf86Info.vtno = xf86_console_requested_vt;

            if (ioctl(fd, VT_GETACTIVE, &initialVT) < 0)
                initialVT = -1;

            if (xf86Info.vtno == -1) {
                if (ioctl(fd, VT_OPENQRY, &xf86Info.vtno) < 0) {
                    /* No free VTs */
                    xf86Info.vtno = -1;
                }

                if (xf86Info.vtno == -1) {
                    /*
                     * All VTs are in use.  If initialVT was found, use it.
                     */
                    if (initialVT != -1) {
                        xf86Info.vtno = initialVT;
                    }
                    else {
                        FatalError("%s: Cannot find a free VT", "xf86OpenPcvt");
                    }
                }
            }

            close(fd);
            snprintf(vtname, sizeof(vtname), "%s%01x", vtprefix,
                     xf86Info.vtno - 1);
            if ((fd = open(vtname, PCVT_CONSOLE_MODE, 0)) < 0) {
                ErrorF("xf86OpenPcvt: Cannot open %s (%s)",
                       vtname, strerror(errno));
                xf86Info.vtno = initialVT;
                snprintf(vtname, sizeof(vtname), "%s%01x", vtprefix,
                         xf86Info.vtno - 1);
                if ((fd = open(vtname, PCVT_CONSOLE_MODE, 0)) < 0) {
                    FatalError("xf86OpenPcvt: Cannot open %s (%s)",
                               vtname, strerror(errno));
                }
            }
            if (ioctl(fd, VT_GETMODE, &vtmode) < 0) {
                FatalError("xf86OpenPcvt: VT_GETMODE failed");
            }
            xf86Info.consType = PCVT;
#ifdef WSCONS_SUPPORT
            LogMessageVerb(X_PROBED, 1,
                           "Using wscons driver on %s in pcvt compatibility mode ",
                           vtname);
#else
            LogMessageVerb(X_PROBED, 1, "Using pcvt driver\n");
#endif
#ifdef __NetBSD__
        }
        else {
            /* Not pcvt */
            close(fd);
            fd = -1;
        }
#endif
    }
    xf86Info.consoleFd = fd;

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
    goto out;
#endif
#if !(defined(__NetBSD__) && (__NetBSD_Version__ >= 200000000))
    /*
     * First activate the #1 VT.  This is a hack to allow a server
     * to be started while another one is active.  There should be
     * a better way.
     */
    if (initialVT != 1) {
        if (ioctl(xf86Info.consoleFd, VT_ACTIVATE, 1) != 0) {
            LogMessageVerb(X_WARNING, 1, "xf86OpenConsole: VT_ACTIVATE failed\n");
        }
        sleep(1);
    }
#endif
    goto out;
out:
    xf86_bsd_acquire_vt();
    xf86_console_proc_bell = xf86_console_pcvt_bell;
    xf86_console_proc_close = xf86_console_pcvt_close;
    xf86_console_proc_reactivate = xf86_console_pcvt_reactivate;
    return (fd > 0);
}

#endif /* PCVT_SUPPORT */
