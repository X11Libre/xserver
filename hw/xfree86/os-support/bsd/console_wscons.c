#include <xorg-config.h>

#if defined(WSCONS_SUPPORT)

#include <unistd.h>
#include <sys/ioctl.h>
#include <dev/wscons/wsconsio.h>

#include "xf86Priv.h"
#include "xf86_bsd_priv.h"

void xf86_console_wscons_close(void)
{
    int mode = WSDISPLAYIO_MODE_EMUL;
    ioctl(xf86Info.consoleFd, WSDISPLAYIO_SMODE, &mode);

    close(xf86Info.consoleFd);
    xf86Info.consoleFd = -1;
}

#endif /* WSCONS_SUPPORT */
