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
#include "miext/extinit_priv.h"
#include "os/screensaver.h"
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
    REQUEST(xDPMSSelectInputReq);
    DPMSEventPtr pEvent, pNewEvent, *pHead;
    XID clientResource;
    int i;

    REQUEST_SIZE_MATCH(xDPMSSelectInputReq);
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
    for (unsigned int walkScreenIdx = 0; walkScreenIdx < screenInfo.numScreens; walkScreenIdx++) {
        ScreenPtr walkScreen = screenInfo.screens[walkScreenIdx];
        if (walkScreen->DPMS != NULL)
            return TRUE;
    }

    for (unsigned int walkScreenIdx = 0; walkScreenIdx < screenInfo.numGPUScreens; walkScreenIdx++) {
        ScreenPtr walkScreen = screenInfo.gpuscreens[walkScreenIdx];
        if (walkScreen->DPMS != NULL)
            return TRUE;
    }

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

    for (unsigned int walkScreenIdx = 0; walkScreenIdx < screenInfo.numScreens; walkScreenIdx++) {
        ScreenPtr walkScreen = screenInfo.screens[walkScreenIdx];
        if (walkScreen->DPMS != NULL)
            walkScreen->DPMS(walkScreen, level);
    }

    for (unsigned int walkScreenIdx = 0; walkScreenIdx < screenInfo.numGPUScreens; walkScreenIdx++) {
        ScreenPtr walkScreen = screenInfo.gpuscreens[walkScreenIdx];
        if (walkScreen->DPMS != NULL)
            walkScreen->DPMS(walkScreen, level);
    }

    if (DPMSPowerLevel != old_level)
        SendDPMSInfoNotify();

    return Success;
}

static int
ProcDPMSGetVersion(ClientPtr client)
{
    /* REQUEST(xDPMSGetVersionReq); */
    xDPMSGetVersionReply rep = {
        .majorVersion = SERVER_DPMS_MAJOR_VERSION,
        .minorVersion = SERVER_DPMS_MINOR_VERSION
    };

    REQUEST_SIZE_MATCH(xDPMSGetVersionReq);

    if (client->swapped) {
        swaps(&rep.majorVersion);
        swaps(&rep.minorVersion);
    }
    X_SEND_REPLY_SIMPLE(client, rep);
    return Success;
}

static int
ProcDPMSCapable(ClientPtr client)
{
    /* REQUEST(xDPMSCapableReq); */
    xDPMSCapableReply rep = {
        .capable = TRUE
    };

    REQUEST_SIZE_MATCH(xDPMSCapableReq);

    X_SEND_REPLY_SIMPLE(client, rep);
    return Success;
}

static int
ProcDPMSGetTimeouts(ClientPtr client)
{
    /* REQUEST(xDPMSGetTimeoutsReq); */
    xDPMSGetTimeoutsReply rep = {
        .standby = DPMSStandbyTime / MILLI_PER_SECOND,
        .suspend = DPMSSuspendTime / MILLI_PER_SECOND,
        .off = DPMSOffTime / MILLI_PER_SECOND
    };

    REQUEST_SIZE_MATCH(xDPMSGetTimeoutsReq);

    if (client->swapped) {
        swaps(&rep.standby);
        swaps(&rep.suspend);
        swaps(&rep.off);
    }
    X_SEND_REPLY_SIMPLE(client, rep);
    return Success;
}

static int
ProcDPMSSetTimeouts(ClientPtr client)
{
    REQUEST(xDPMSSetTimeoutsReq);

    REQUEST_SIZE_MATCH(xDPMSSetTimeoutsReq);

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
    Bool was_enabled = DPMSEnabled;

    REQUEST_SIZE_MATCH(xDPMSEnableReq);

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
    Bool was_enabled = DPMSEnabled;

    /* REQUEST(xDPMSDisableReq); */

    REQUEST_SIZE_MATCH(xDPMSDisableReq);

    DPMSSet(client, DPMSModeOn);

    DPMSEnabled = FALSE;
    if (was_enabled)
        SendDPMSInfoNotify();

    return Success;
}

static int
ProcDPMSForceLevel(ClientPtr client)
{
    REQUEST(xDPMSForceLevelReq);

    REQUEST_SIZE_MATCH(xDPMSForceLevelReq);

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
    /* REQUEST(xDPMSInfoReq); */
    xDPMSInfoReply rep = {
        .power_level = DPMSPowerLevel,
        .state = DPMSEnabled
    };

    REQUEST_SIZE_MATCH(xDPMSInfoReq);

    if (client->swapped) {
        swaps(&rep.power_level);
    }
    X_SEND_REPLY_SIMPLE(client, rep);
    return Success;
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

static int _X_COLD
SProcDPMSGetVersion(ClientPtr client)
{
    REQUEST(xDPMSGetVersionReq);
    REQUEST_SIZE_MATCH(xDPMSGetVersionReq);
    swaps(&stuff->majorVersion);
    swaps(&stuff->minorVersion);
    return ProcDPMSGetVersion(client);
}

static int _X_COLD
SProcDPMSSetTimeouts(ClientPtr client)
{
    REQUEST(xDPMSSetTimeoutsReq);
    REQUEST_SIZE_MATCH(xDPMSSetTimeoutsReq);

    swaps(&stuff->standby);
    swaps(&stuff->suspend);
    swaps(&stuff->off);
    return ProcDPMSSetTimeouts(client);
}

static int _X_COLD
SProcDPMSForceLevel(ClientPtr client)
{
    REQUEST(xDPMSForceLevelReq);
    REQUEST_SIZE_MATCH(xDPMSForceLevelReq);

    swaps(&stuff->level);

    return ProcDPMSForceLevel(client);
}

static int _X_COLD
SProcDPMSSelectInput(ClientPtr client)
{
    REQUEST(xDPMSSelectInputReq);
    REQUEST_SIZE_MATCH(xDPMSSelectInputReq);
    swapl(&stuff->eventMask);
    return ProcDPMSSelectInput(client);
}



static int _X_COLD
SProcDPMSDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
    case X_DPMSGetVersion:
        return SProcDPMSGetVersion(client);
    case X_DPMSCapable:
        return ProcDPMSCapable(client);
    case X_DPMSGetTimeouts:
        return ProcDPMSGetTimeouts(client);
    case X_DPMSSetTimeouts:
        return SProcDPMSSetTimeouts(client);
    case X_DPMSEnable:
        return ProcDPMSEnable(client);
    case X_DPMSDisable:
        return ProcDPMSDisable(client);
    case X_DPMSForceLevel:
        return SProcDPMSForceLevel(client);
    case X_DPMSInfo:
        return ProcDPMSInfo(client);
    case X_DPMSSelectInput:
        return SProcDPMSSelectInput(client);
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
                                 ProcDPMSDispatch, SProcDPMSDispatch,
                                 DPMSCloseDownExtension, StandardMinorOpcode))) {
        DPMSReqCode = extEntry->base;
        GERegisterExtension(DPMSReqCode, SDPMSInfoNotifyEvent);
    }
}
