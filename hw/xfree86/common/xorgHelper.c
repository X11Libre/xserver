#include <xorg-config.h>

#include <X11/X.h>
#include "os.h"
#include "servermd.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "propertyst.h"
#include "gcstruct.h"
#include "loaderProcs.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xorgVersion.h"

CARD32
xorgGetVersion(void)
{
    return XORG_VERSION_CURRENT;
}
