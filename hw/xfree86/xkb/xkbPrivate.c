
#include <dix-config.h>

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <stdio.h>
#include <X11/X.h>
#include "windowstr.h"
#include <xkbsrv.h>

#include "os.h"
#include "xf86.h"

int
XkbDDXPrivate(DeviceIntPtr dev, KeyCode key, XkbAction *act)
{
    XkbAnyAction *xf86act = &(act->any);
    char msgbuf[XkbAnyActionDataSize + 1];

    if (xf86act->type == XkbSA_XFree86Private) {
        memcpy(msgbuf, xf86act->data, XkbAnyActionDataSize);
        msgbuf[XkbAnyActionDataSize] = '\0';
        if (strcasecmp(msgbuf, "-vmode") == 0)
            xf86ProcessActionEvent(ACTION_PREV_MODE, NULL);
        else if (strcasecmp(msgbuf, "+vmode") == 0)
            xf86ProcessActionEvent(ACTION_NEXT_MODE, NULL);
    }

    return 0;
}
