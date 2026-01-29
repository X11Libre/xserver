#include <xorg-config.h>

#include "xf86_os_support.h"
#include "xf86_OSlib.h"

void
xf86OSInitVidMem(VidMemInfoPtr pVidMem)
{
    pVidMem->initialised = TRUE;
    return;
}
