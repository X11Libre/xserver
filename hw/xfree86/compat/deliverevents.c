#include <dix-config.h>

#include <X11/Xfuncproto.h>

#include "dix/dix_priv.h"

/*
 * this is specifically for NVidia proprietary driver: they're again lagging
 * behind a year, doing at least some minimal cleanup of their code base.
 * All attempts to get in direct contact with them have failed.
 */

_X_EXPORT enum EventDeliveryState DeliverEvents(WindowPtr pWindow,
                                               xEventPtr events,
                                               size_t nEvents,
                                               WindowPtr otherParent);

enum EventDeliveryState DeliverEvents(WindowPtr pWindow,
                                      xEventPtr events,
                                      size_t nEvents,
                                      WindowPtr otherParent)
{
    LogMessageVerb(X_WARNING, 0, "Bogus driver calling DIX-internal function DeliverEvents() !\n");
    LogMessageVerb(X_WARNING, 0, "External drivers really should never ever call this function.\n");
    LogMessageVerb(X_WARNING, 0, "File a bug report to driver vendor or use a FOSS driver.\n");
    LogMessageVerb(X_WARNING, 0, "Proprietary drivers are inherently unstable, they just can't be done right.\n");
    LogMessageVerb(X_WARNING, 0, "And just don't buy Nvidia hardware, ever.\n");

    return dixDeliverEvents(pWindow, events, nEvents, otherParent);
}
