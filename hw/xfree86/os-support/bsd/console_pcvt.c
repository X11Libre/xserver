#include <xorg-config.h>

#if defined(PCVT_SUPPORT)

#include <sys/ioctl.h>
#include <errno.h>
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
#include "xf86_bsd_priv.h"
#include "xf86_console_priv.h"

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

#endif /* PCVT_SUPPORT */
