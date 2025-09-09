
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86.h"

CARD32
xorgGetVersion(void)
{
    return XORG_VERSION_CURRENT;
}
