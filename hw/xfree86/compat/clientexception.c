#include <dix-config.h>

#include <X11/Xfuncproto.h>

#include "dix/dix_priv.h"

#include "xf86_compat.h"

/*
 * this is specifically for NVidia proprietary driver: they're again lagging
 * behind a year, doing at least some minimal cleanup of their code base.
 * All attempts to get in direct contact with them have failed.
 */
_X_EXPORT void MarkClientException(ClientPtr pClient);

void MarkClientException(ClientPtr pClient)
{
    xf86NVidiaBugInternalFunc("MarkClientException()");

    dixMarkClientException(pClient);
}
