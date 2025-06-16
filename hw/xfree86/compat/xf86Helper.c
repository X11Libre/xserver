#include <dix-config.h>

#include <X11/Xfuncproto.h>


#include "xf86Priv.h"
#include "xf86Bus.h"


/*
 * this is specifically for NVidia proprietary driver: they're again lagging
 * behind a year, doing at least some minimal cleanup of their code base.
 * All attempts to get in direct contact with them have failed.
 */

/*
 * this is only needed for the 570.x nvidia drivers
 */

_X_EXPORT Bool xf86IsScreenPrimary(ScrnInfoPtr pScrn);

Bool
xf86IsScreenPrimary(ScrnInfoPtr pScrn)
{
    int i;

    for (i = 0; i < pScrn->numEntities; i++) {
        if (xf86IsEntityPrimary(i))
            return TRUE;
    }
    return FALSE;
}
