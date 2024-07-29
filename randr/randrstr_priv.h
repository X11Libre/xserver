/*
 * Copyright © 2000 Compaq Computer Corporation
 * Copyright © 2002 Hewlett-Packard Company
 * Copyright © 2006 Intel Corporation
 * Copyright © 2008 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 *
 * Author:  Jim Gettys, Hewlett-Packard Company, Inc.
 *	    Keith Packard, Intel Corporation
 */

#ifndef _XSERVER_RANDRSTR_PRIV_H_
#define _XSERVER_RANDRSTR_PRIV_H_

#include "randrstr.h"

extern int RREventBase, RRErrorBase;

extern int (*ProcRandrVector[RRNumberRequests]) (ClientPtr);
extern int (*SProcRandrVector[RRNumberRequests]) (ClientPtr);

extern RESTYPE RRClientType, RREventType;     /* resource types for event masks */
extern DevPrivateKeyRec RRClientPrivateKeyRec;

#define RRClientPrivateKey (&RRClientPrivateKeyRec)

#define VERIFY_RR_OUTPUT(id, ptr, a)\
    {\
	int rc = dixLookupResourceByType((void **)&(ptr), id,\
	                                 RROutputType, client, a);\
	if (rc != Success) {\
	    client->errorValue = id;\
	    return rc;\
	}\
    }

#define VERIFY_RR_CRTC(id, ptr, a)\
    {\
	int rc = dixLookupResourceByType((void **)&(ptr), id,\
	                                 RRCrtcType, client, a);\
	if (rc != Success) {\
	    client->errorValue = id;\
	    return rc;\
	}\
    }

#define VERIFY_RR_MODE(id, ptr, a)\
    {\
	int rc = dixLookupResourceByType((void **)&(ptr), id,\
	                                 RRModeType, client, a);\
	if (rc != Success) {\
	    client->errorValue = id;\
	    return rc;\
	}\
    }

#define VERIFY_RR_PROVIDER(id, ptr, a)\
    {\
        int rc = dixLookupResourceByType((void **)&(ptr), id,\
                                         RRProviderType, client, a);\
        if (rc != Success) {\
            client->errorValue = id;\
            return rc;\
        }\
    }

#define VERIFY_RR_LEASE(id, ptr, a)\
    {\
        int rc = dixLookupResourceByType((void **)&(ptr), id,\
                                         RRLeaseType, client, a);\
        if (rc != Success) {\
            client->errorValue = id;\
            return rc;\
        }\
    }

#define GetRRClient(pClient)    ((RRClientPtr)dixLookupPrivate(&(pClient)->devPrivates, RRClientPrivateKey))
#define rrClientPriv(pClient)	RRClientPtr pRRClient = GetRRClient(pClient)

int ProcRRGetPanning(ClientPtr client);

int ProcRRSetPanning(ClientPtr client);

void RRConstrainCursorHarder(DeviceIntPtr, ScreenPtr, int, int *, int *);

/* rrlease.c */
void RRDeliverLeaseEvent(ClientPtr client, WindowPtr window);

void RRTerminateLease(RRLeasePtr lease);

Bool RRLeaseInit(void);

/* rrprovider.c */
#define PRIME_SYNC_PROP         "PRIME Synchronization"

void RRMonitorInit(ScreenPtr screen);

Bool RRMonitorMakeList(ScreenPtr screen, Bool get_active, RRMonitorPtr *monitors_ret, int *nmon_ret);

int RRMonitorCountList(ScreenPtr screen);

void RRMonitorFreeList(RRMonitorPtr monitors, int nmon);

void RRMonitorClose(ScreenPtr screen);

RRMonitorPtr RRMonitorAlloc(int noutput);

int RRMonitorAdd(ClientPtr client, ScreenPtr screen, RRMonitorPtr monitor);

void RRMonitorFree(RRMonitorPtr monitor);

int ProcRRGetMonitors(ClientPtr client);

int ProcRRSetMonitor(ClientPtr client);

int ProcRRDeleteMonitor(ClientPtr client);

int ProcRRCreateLease(ClientPtr client);

int ProcRRFreeLease(ClientPtr client);

int ProcRRQueryVersion(ClientPtr client);

int ProcRRSelectInput(ClientPtr client);

#endif /* _XSERVER_RANDRSTR_PRIV_H_ */
