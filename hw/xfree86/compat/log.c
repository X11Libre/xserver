#include <dix-config.h>

#include <X11/Xfuncproto.h>

#include "include/os.h"

#undef xf86Msg
#undef xf86MsgVerb

/*
 * this is specifically for NVidia proprietary driver: they're again lagging
 * behind a year, doing at least some minimal cleanup of their code base.
 * All attempts to get in direct contact with them have failed.
 */
_X_EXPORT void xf86Msg(MessageType type, const char *format, ...)
    _X_ATTRIBUTE_PRINTF(2, 3);

void xf86Msg(MessageType type, const char *format, ...)
{
    LogMessageVerb(X_WARNING, 0, "Outdated driver still using xf86Msg() !\n");
    LogMessageVerb(X_WARNING, 0, "File a bug report to driver vendor or use a FOSS driver.\n");
    LogMessageVerb(X_WARNING, 0, "Proprietary drivers are inherently unstable, they just can't be done right.\n");

    va_list ap;

    va_start(ap, format);
    LogVMessageVerb(type, 1, format, ap);
    va_end(ap);
}


/*
 * this is only needed for the 570.x nvidia drivers
 */

_X_EXPORT void xf86MsgVerb(MessageType type, int verb, const char *format, ...)
    _X_ATTRIBUTE_PRINTF(3, 4);

void
xf86MsgVerb(MessageType type, int verb, const char *format, ...)
{
    LogMessageVerb(X_WARNING, 0, "Outdated driver still using xf86MsgVerb() !\n");
    LogMessageVerb(X_WARNING, 0, "File a bug report to driver vendor or use a FOSS driver.\n");
    LogMessageVerb(X_WARNING, 0, "Proprietary drivers are inherently unstable, they just can't be done right.\n");

    va_list ap;
    va_start(ap, format);
    LogVMessageVerb(type, verb, format, ap);
    va_end(ap);
}
