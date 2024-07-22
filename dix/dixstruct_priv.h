/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIXSTRUCT_PRIV_H
#define _XSERVER_DIXSTRUCT_PRIV_H

#include "client.h"
#include "dix.h"
#include "resource.h"
#include "cursor.h"
#include "gc.h"
#include "pixmap.h"
#include "privates.h"
#include "dixstruct.h"
#include <X11/Xmd.h>

/*
 * Scheduling interface
 */
extern long SmartScheduleTime;
extern long SmartScheduleInterval;
extern long SmartScheduleSlice;
extern long SmartScheduleMaxSlice;
#ifdef HAVE_SETITIMER
extern Bool SmartScheduleSignalEnable;
#else
#define SmartScheduleSignalEnable FALSE
#endif
void SmartScheduleStartTimer(void);
void SmartScheduleStopTimer(void);

/* Client has requests queued or data on the network */
void mark_client_ready(ClientPtr client);

/*
 * Client has requests queued or data on the network, but awaits a
 * server grab release
 */
void mark_client_saved_ready(ClientPtr client);

/* Client has no requests queued and no data on network */
void mark_client_not_ready(ClientPtr client);

static inline Bool client_is_ready(ClientPtr client)
{
    return !xorg_list_is_empty(&client->ready);
}

Bool
clients_are_ready(void);

extern struct xorg_list output_pending_clients;

static inline void
output_pending_mark(ClientPtr client)
{
    if (!client->clientGone && xorg_list_is_empty(&client->output_pending))
        xorg_list_append(&client->output_pending, &output_pending_clients);
}

static inline void
output_pending_clear(ClientPtr client)
{
    xorg_list_del(&client->output_pending);
}

static inline Bool any_output_pending(void) {
    return !xorg_list_is_empty(&output_pending_clients);
}

#define SMART_MAX_PRIORITY  (20)
#define SMART_MIN_PRIORITY  (-20)

void SmartScheduleInit(void);

/* This prototype is used pervasively in Xext, dix */
#define DISPATCH_PROC(func) int func(ClientPtr /* client */)

/* proc vectors */

extern int (*InitialVector[3]) (ClientPtr /*client */ );

#endif /* _XSERVER_DIXSTRUCT_PRIV_H */
