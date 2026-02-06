/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifndef DIXSTRUCT_H
#define DIXSTRUCT_H

#include <X11/Xmd.h>

#include "callback.h"
#include "dix.h"
#include "resource.h"
#include "cursor.h"
#include "gc.h"
#include "pixmap.h"
#include "privates.h"

struct _Client;
typedef struct _ClientId *ClientIdPtr;

/*
 * 	direct-mapped hash table, used by resource manager to store
 *      translation from client ids to server addresses.
 */

extern _X_EXPORT CallbackListPtr ClientStateCallback;

typedef struct {
    ClientPtr client;
    xConnSetupPrefix *prefix;
    xConnSetup *setup;
} NewClientInfoRec;

typedef void (*ReplySwapPtr) (ClientPtr /* pClient */ ,
                              int /* size */ ,
                              void * /* pbuf */ );

typedef enum { ClientStateInitial,
    ClientStateRunning,
    ClientStateRetained,
    ClientStateGone
} ClientState;

struct _ClientId;

typedef struct _Client {
    void *requestBuffer;
    void *osPrivate;             /* for OS layer, including scheduler */
    struct xorg_list ready;      /* List of clients ready to run */
    struct xorg_list output_pending; /* List of clients with output queued */
    Mask clientAsMask;
    unsigned short index;
    unsigned char majorOp, minorOp;
    unsigned int swapped:1;
    unsigned int local:1;
    unsigned int big_requests:1; /* supports large requests */
    unsigned int clientGone:1;
    unsigned int closeDownMode:2;
    unsigned int clientState:2;
    signed char smart_priority;
    short noClientException;      /* this client died or needs to be killed */
    int priority;
    ReplySwapPtr pSwapReplyFunc;
    XID errorValue;
    int sequence;
    int ignoreCount;            /* count for Attend/IgnoreClient */
    int __dummy0;                       /* used to be numSave */
    void *__dummy1;                     /* used to be saveSet */
    int (**requestVector) (ClientPtr /* pClient */ );
    CARD32 req_len;             /* length of current request */
    unsigned int replyBytesRemaining;
    PrivateRec *devPrivates;
    unsigned short mapNotifyMask;
    unsigned short newKeyboardNotifyMask;
    unsigned char xkbClientFlags;
    KeyCode minKC, maxKC;

    int smart_start_tick;
    int smart_stop_tick;

    DeviceIntPtr clientPtr;
    struct _ClientId *clientIds;
    int req_fds;

    /* driver should never ever touch anything beyond here */

    struct xorg_list saveSets;
} ClientRec;

extern _X_EXPORT TimeStamp currentTime;

extern _X_EXPORT int
CompareTimeStamps(TimeStamp /*a */ ,
                  TimeStamp /*b */ );

extern _X_EXPORT TimeStamp
ClientTimeToServerTime(CARD32 /*c */ );

/* proc vectors */

extern _X_EXPORT int (*ProcVector[256]) (ClientPtr /*client */ );

extern _X_EXPORT int (*SwappedProcVector[256]) (ClientPtr /*client */ );

/* fixme: still needed by (public) dix.h */
extern ReplySwapPtr ReplySwapVector[256];

#endif                          /* DIXSTRUCT_H */
