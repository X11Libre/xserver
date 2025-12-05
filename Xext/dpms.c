/*****************************************************************

Copyright (c) 1996 Digital Equipment Corporation, Maynard, Massachusetts.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR ANY CLAIM, DAMAGES, INCLUDING,
BUT NOT LIMITED TO CONSEQUENTIAL OR INCIDENTAL DAMAGES, OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Digital Equipment Corporation
shall not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from Digital
Equipment Corporation.

******************************************************************/

#include <dix-config.h>

#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/extensions/dpmsproto.h>

#include "dix/dix_priv.h"
#include "dix/request_priv.h"
#include "dix/screenint_priv.h"
#include "dix/screensaver_priv.h"
#include "miext/extinit_priv.h"
#include "os/screensaver.h"
#include "os/osdep.h"
#include "Xext/geext_priv.h"

#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "opaque.h"
#include "dpmsproc.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "protocol-versions.h"

Bool noDPMSExtension = FALSE;

CARD16 DPMSPowerLevel = 0;
Bool DPMSDisabledSwitch = FALSE;
CARD32 DPMSStandbyTime = -1;
CARD32 DPMSSuspendTime = -1;
CARD32 DPMSOffTime = -1;
Bool DPMSEnabled;

static int DPMSReqCode = 0;
static RESTYPE ClientType, DPMSEventType;  /* resource types for event masks */
static XID eventResource;

typedef struct _DPMSEvent *DPMSEventPtr;
typedef struct _DPMSEvent {
    DPMSEventPtr next;
    ClientPtr client;
    XID clientResource;
    unsigned int mask;
} DPMSEventRec;

 /*ARGSUSED*/ static int
DPMSFreeClient(void *data, XID id)
{
    DPMSEventPtr pEvent;
    DPMSEventPtr *pHead, pCur, pPrev;

    pEvent = (DPMSEventPtr) data;
    dixLookupResourceByType((void *) &pHead, eventResource, DPMSEventType,
                            NULL, DixUnknownAccess);
    if (pHead) {
        pPrev = 0;
        for (pCur = *pHead; pCur && pCur != pEvent; pCur = pCur->next)
            pPrev = pCur;
        if (pCur) {
            if (pPrev)
                pPrev->next = pEvent->next;
            else
                *pHead = pEvent->next;
        }
    }
    free((void *) pEvent);
    return 1;
}

 /*ARGSUSED*/ static int
DPMSFreeEvents(void *data, XID id)
{
    DPMSEventPtr *pHead, pCur, pNext;

    pHead = (DPMSEventPtr *) data;
    for (pCur = *pHead; pCur; pCur = pNext) {
        pNext = pCur->next;
        FreeResource(pCur->clientResource, ClientType);
        free((void *) pCur);
    }
    free((void *) pHead);
    return 1;
}

static void
SDPMSInfoNotifyEvent(xGenericEvent * from,
                     xGenericEvent * to)
{
    *to = *from;
    swaps(&to->sequenceNumber);
    swapl(&to->length);
    swaps(&to->evtype);
    if (from->evtype == DPMSInfoNotify) {
        xDPMSInfoNotifyEvent *c = (xDPMSInfoNotifyEvent *) to;
        swapl(&c->timestamp);
        swaps(&c->power_level);
    }
}

static int
ProcDPMSSelectInput(register ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDPMSSelectInputReq);
    X_REQUEST_FIELD_CARD32(eventMask);

    DPMSEventPtr pEvent, pNewEvent, *pHead;
    XID clientResource;
    int i;

    i = dixLookupResourceByType((void **)&pHead, eventResource, DPMSEventType,
                                client,
                                DixWriteAccess);
    if (stuff->eventMask == DPMSInfoNotifyMask) {
        if (i == Success && pHead) {
            /* check for existing entry. */
            for (pEvent = *pHead; pEvent; pEvent = pEvent->next) {
                if (pEvent->client == client) {
                    pEvent->mask = stuff->eventMask;
                    return Success;
                }
            }
        }

        /* build the entry */
        pNewEvent = calloc(1, sizeof(DPMSEventRec));
        if (!pNewEvent)
            return BadAlloc;
        pNewEvent->client = client;
        pNewEvent->mask = stuff->eventMask;
        /*
         * add a resource that will be deleted when
         * the client goes away
         */
        clientResource = FakeClientID(client->index);
        pNewEvent->clientResource = clientResource;
        if (!AddResource(clientResource, ClientType, (void *)pNewEvent))
            return BadAlloc;
        /*
         * create a resource to contain a pointer to the list
         * of clients selecting input
         */
        if (i != Success || !pHead) {
            pHead = calloc(1, sizeof(DPMSEventPtr));
            if (!pHead ||
                    !AddResource(eventResource, DPMSEventType, (void *)pHead)) {
                FreeResource(clientResource, X11_RESTYPE_NONE);
                return BadAlloc;
            }
            *pHead = 0;
        }
        pNewEvent->next = *pHead;
        *pHead = pNewEvent;
    }
    else if (stuff->eventMask == 0) {
        /* delete the interest */
        if (i == Success && pHead) {
            pNewEvent = 0;
            for (pEvent = *pHead; pEvent; pEvent = pEvent->next) {
                if (pEvent->client == client)
                    break;
                pNewEvent = pEvent;
            }
            if (pEvent) {
                FreeResource(pEvent->clientResource, ClientType);
                if (pNewEvent)
                    pNewEvent->next = pEvent->next;
                else
                    *pHead = pEvent->next;
                free(pEvent);
            }
        }
    }
    else {
        client->errorValue = stuff->eventMask;
        return BadValue;
    }
    return Success;
}

static void
SendDPMSInfoNotify(void)
{
    DPMSEventPtr *pHead, pEvent;
    xDPMSInfoNotifyEvent se;
    int i;

    i = dixLookupResourceByType((void **)&pHead, eventResource, DPMSEventType,
                                serverClient,
                                DixReadAccess);
    if (i != Success || !pHead)
        return;
    for (pEvent = *pHead; pEvent; pEvent = pEvent->next) {
        if ((pEvent->mask & DPMSInfoNotifyMask) == 0)
            continue;
        se.type = GenericEvent;
        se.extension = DPMSReqCode;
        se.length = (sizeof(xDPMSInfoNotifyEvent) - 32) >> 2;
        se.evtype = DPMSInfoNotify;
        se.timestamp = currentTime.milliseconds;
        se.power_level = DPMSPowerLevel;
        se.state = DPMSEnabled;
        WriteEventsToClient(pEvent->client, 1, (xEvent *)&se);
    }
}

Bool
DPMSSupported(void)
{
    /* For each screen, check if DPMS is supported */
    DIX_FOR_EACH_SCREEN({
        if (walkScreen->DPMS != NULL)
            return TRUE;
    });

    DIX_FOR_EACH_GPU_SCREEN({
        if (walkScreen->DPMS != NULL)
            return TRUE;
    });

    return FALSE;
}

static Bool
isUnblank(int mode)
{
    switch (mode) {
    case SCREEN_SAVER_OFF:
    case SCREEN_SAVER_FORCER:
        return TRUE;
    case SCREEN_SAVER_ON:
    case SCREEN_SAVER_CYCLE:
        return FALSE;
    default:
        return TRUE;
    }
}

int
DPMSSet(ClientPtr client, int level)
{
    int rc;
    int old_level = DPMSPowerLevel;

    DPMSPowerLevel = level;

    if (level != DPMSModeOn) {
        if (isUnblank(screenIsSaved)) {
            rc = dixSaveScreens(client, SCREEN_SAVER_FORCER, ScreenSaverActive);
            if (rc != Success)
                return rc;
        }
    } else if (!isUnblank(screenIsSaved)) {
        rc = dixSaveScreens(client, SCREEN_SAVER_OFF, ScreenSaverReset);
        if (rc != Success)
            return rc;
    }

    DIX_FOR_EACH_SCREEN({
        if (walkScreen->DPMS != NULL)
            walkScreen->DPMS(walkScreen, level);
    });

    DIX_FOR_EACH_GPU_SCREEN({
        if (walkScreen->DPMS != NULL)
            walkScreen->DPMS(walkScreen, level);
    });

    if (DPMSPowerLevel != old_level)
        SendDPMSInfoNotify();

    return Success;
}

static int
ProcDPMSGetVersion(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDPMSGetVersionReq);

    xDPMSGetVersionReply reply = {
        .majorVersion = SERVER_DPMS_MAJOR_VERSION,
        .minorVersion = SERVER_DPMS_MINOR_VERSION
    };

    if (client->swapped) {
        swaps(&reply.majorVersion);
        swaps(&reply.minorVersion);
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
ProcDPMSCapable(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDPMSCapableReq);

    xDPMSCapableReply reply = {
        .capable = TRUE
    };

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
ProcDPMSGetTimeouts(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDPMSGetTimeoutsReq);

    xDPMSGetTimeoutsReply reply = {
        .standby = DPMSStandbyTime / MILLI_PER_SECOND,
        .suspend = DPMSSuspendTime / MILLI_PER_SECOND,
        .off = DPMSOffTime / MILLI_PER_SECOND
    };

    if (client->swapped) {
        swaps(&reply.standby);
        swaps(&reply.suspend);
        swaps(&reply.off);
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
ProcDPMSSetTimeouts(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDPMSSetTimeoutsReq);
    X_REQUEST_FIELD_CARD16(standby);
    X_REQUEST_FIELD_CARD16(suspend);
    X_REQUEST_FIELD_CARD16(off);

    if ((stuff->off != 0) && (stuff->off < stuff->suspend)) {
        client->errorValue = stuff->off;
        return BadValue;
    }
    if ((stuff->suspend != 0) && (stuff->suspend < stuff->standby)) {
        client->errorValue = stuff->suspend;
        return BadValue;
    }

    DPMSStandbyTime = stuff->standby * MILLI_PER_SECOND;
    DPMSSuspendTime = stuff->suspend * MILLI_PER_SECOND;
    DPMSOffTime = stuff->off * MILLI_PER_SECOND;
    SetScreenSaverTimer();

    return Success;
}

static int
ProcDPMSEnable(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDPMSEnableReq);

    Bool was_enabled = DPMSEnabled;

    DPMSEnabled = TRUE;
    if (!was_enabled) {
        SetScreenSaverTimer();
        SendDPMSInfoNotify();
    }

    return Success;
}

static int
ProcDPMSDisable(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDPMSDisableReq);

    Bool was_enabled = DPMSEnabled;

    DPMSSet(client, DPMSModeOn);

    DPMSEnabled = FALSE;
    if (was_enabled)
        SendDPMSInfoNotify();

    return Success;
}

static int
ProcDPMSForceLevel(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDPMSForceLevelReq);
    X_REQUEST_FIELD_CARD16(level);

    if (!DPMSEnabled)
        return BadMatch;

    if (stuff->level != DPMSModeOn &&
        stuff->level != DPMSModeStandby &&
        stuff->level != DPMSModeSuspend && stuff->level != DPMSModeOff) {
        client->errorValue = stuff->level;
        return BadValue;
    }

    DPMSSet(client, stuff->level);

    return Success;
}

static int
ProcDPMSInfo(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDPMSInfoReq);

    xDPMSInfoReply reply = {
        .power_level = DPMSPowerLevel,
        .state = DPMSEnabled
    };

    if (client->swapped) {
        swaps(&reply.power_level);
    }
    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
ProcDPMSDispatch(ClientPtr client)
{
    REQUEST(xReq);

    switch (stuff->data) {
    case X_DPMSGetVersion:
        return ProcDPMSGetVersion(client);
    case X_DPMSCapable:
        return ProcDPMSCapable(client);
    case X_DPMSGetTimeouts:
        return ProcDPMSGetTimeouts(client);
    case X_DPMSSetTimeouts:
        return ProcDPMSSetTimeouts(client);
    case X_DPMSEnable:
        return ProcDPMSEnable(client);
    case X_DPMSDisable:
        return ProcDPMSDisable(client);
    case X_DPMSForceLevel:
        return ProcDPMSForceLevel(client);
    case X_DPMSInfo:
        return ProcDPMSInfo(client);
    case X_DPMSSelectInput:
        return ProcDPMSSelectInput(client);
    default:
        return BadRequest;
    }
}

static void
DPMSCloseDownExtension(ExtensionEntry *e)
{
    DPMSSet(serverClient, DPMSModeOn);
}

void
DPMSExtensionInit(void)
{
    ExtensionEntry *extEntry;

#define CONDITIONALLY_SET_DPMS_TIMEOUT(_timeout_value_)         \
    if (_timeout_value_ == -1) { /* not yet set from config */  \
        _timeout_value_ = ScreenSaverTime;                      \
    }

    CONDITIONALLY_SET_DPMS_TIMEOUT(DPMSStandbyTime)
    CONDITIONALLY_SET_DPMS_TIMEOUT(DPMSSuspendTime)
    CONDITIONALLY_SET_DPMS_TIMEOUT(DPMSOffTime)

    DPMSPowerLevel = DPMSModeOn;
    DPMSEnabled = DPMSSupported();

    ClientType = CreateNewResourceType(DPMSFreeClient, "DPMSClient");
    DPMSEventType = CreateNewResourceType(DPMSFreeEvents, "DPMSEvent");
    eventResource = dixAllocServerXID();

    if (DPMSEnabled && ClientType && DPMSEventType &&
        (extEntry = AddExtension(DPMSExtensionName, 0, 0,
                                 ProcDPMSDispatch, ProcDPMSDispatch,
                                 DPMSCloseDownExtension, StandardMinorOpcode))) {
        DPMSReqCode = extEntry->base;
        GERegisterExtension(DPMSReqCode, SDPMSInfoNotifyEvent);
    }
}
