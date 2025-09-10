#include <dix-config.h>

#include <X11/Xfuncproto.h>
#include <X11/Xproto.h>

#include "os/osdep.h"

#include "xf86_compat.h"

/*
 * needed for NVidia proprietary driver 570.x and 575.x version
 *
 * they really need special functions for trivial struct initialization :p
 *
 * this function had been obsolete and removed long ago, but NVidia folks
 * still didn't do basic maintenance and fixed their driver
 */

_X_EXPORT void GEInitEvent(xGenericEvent *ev, int extension);

void GEInitEvent(xGenericEvent *ev, int extension)
{
    xf86NVidiaBugObsoleteFunc("GEInitEvent()");

    ev->type = GenericEvent;
    ev->extension = extension;
    ev->length = 0;
}
