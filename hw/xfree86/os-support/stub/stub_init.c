#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86_os_support.h"
#include "xf86_OSlib.h"

void
xf86OpenConsole(void)
{
}

void
xf86CloseConsole(void)
{
}

Bool
xf86VTKeepTtyIsSet(void)
{
     return FALSE;
}


int
xf86ProcessArgument(int argc, char *argv[], int i)
{
    return 0;
}

void
xf86UseMsg(void)
{
}

void
xf86OSInputThreadInit(void)
{
    return;
}
