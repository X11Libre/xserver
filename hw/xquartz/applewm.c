/*
 * Copyright (c) 2003 Torrey T. Lyons. All Rights Reserved.
 * Copyright (c) 2002-2012 Apple Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT
 * HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above
 * copyright holders shall not be used in advertising or otherwise to
 * promote the sale, use or other dealings in this Software without
 * prior written authorization.
 */

#include "sanitizedCarbon.h"

#include <dix-config.h>

#include <errno.h>

#include "dix/dix_priv.h"
#include "dix/property_priv.h"
#include "dix/request_priv.h"

#include "quartz.h"

#include "misc.h"
#include "dixstruct.h"
#include "globals.h"
#include "extnsionst.h"
#include "cursorstr.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "servermd.h"
#include "swaprep.h"
#include "propertyst.h"
#include <X11/Xatom.h>
#include "darwin.h"
#define _APPLEWM_SERVER_
#include <X11/extensions/applewmproto.h>
#include "applewmExt.h"
#include "X11Application.h"
#include "protocol-versions.h"

#define DEFINE_ATOM_HELPER(func, atom_name)                      \
    static Atom func(void) {                                       \
        return dixAddAtom(atom_name);                           \
    }

DEFINE_ATOM_HELPER(xa_native_screen_origin, "_NATIVE_SCREEN_ORIGIN")
DEFINE_ATOM_HELPER(xa_apple_no_order_in, "_APPLE_NO_ORDER_IN")

static AppleWMProcsPtr appleWMProcs;

static int WMErrorBase;

static unsigned char WMReqCode = 0;
static int WMEventBase = 0;

static RESTYPE ClientType, EventType; /* resource types for event masks */
static XID eventResource;

/* Currently selected events */
static unsigned int eventMask = 0;

static int
WMFreeClient(void *data, XID id);
static int
WMFreeEvents(void *data, XID id);
static void
SNotifyEvent(xAppleWMNotifyEvent *from, xAppleWMNotifyEvent *to);

typedef struct _WMEvent *WMEventPtr;
typedef struct _WMEvent {
    WMEventPtr next;
    ClientPtr client;
    XID clientResource;
    unsigned int mask;
} WMEventRec;

static inline BoxRec
make_box(int x, int y, int w, int h)
{
    BoxRec r;
    r.x1 = x;
    r.y1 = y;
    r.x2 = x + w;
    r.y2 = y + h;
    return r;
}

/* Updates the _NATIVE_SCREEN_ORIGIN property on the given root window. */
void
AppleWMSetScreenOrigin(WindowPtr pWin)
{
    int32_t data[2];

    data[0] = pWin->drawable.pScreen->x + darwinMainScreenX;
    data[1] = pWin->drawable.pScreen->y + darwinMainScreenY;

    dixChangeWindowProperty(serverClient, pWin, xa_native_screen_origin(),
                            XA_INTEGER, 32, PropModeReplace, 2, data, TRUE);
}

/* Window managers can set the _APPLE_NO_ORDER_IN property on windows
   that are being genie-restored from the Dock. We want them to
   be mapped but remain ordered-out until the animation
   completes (when the Dock will order them in). */
Bool
AppleWMDoReorderWindow(WindowPtr pWin)
{
    Atom atom;
    PropertyPtr prop;
    int rc;

    atom = xa_apple_no_order_in();
    rc = dixLookupProperty(&prop, pWin, atom, serverClient, DixReadAccess);

    if (Success == rc && prop->type == atom)
        return 0;

    return 1;
}

static int
ProcAppleWMQueryVersion(register ClientPtr client)
{
    REQUEST_SIZE_MATCH(xAppleWMQueryVersionReq);

    xAppleWMQueryVersionReply reply = {
        .majorVersion = SERVER_APPLEWM_MAJOR_VERSION,
        .minorVersion = SERVER_APPLEWM_MINOR_VERSION,
        .patchVersion = SERVER_APPLEWM_PATCH_VERSION,
    };

    return X_SEND_REPLY_SIMPLE(client, reply);
}

/* events */

static inline void
updateEventMask(WMEventPtr *pHead)
{
    WMEventPtr pCur;

    eventMask = 0;
    for (pCur = *pHead; pCur != NULL; pCur = pCur->next)
        eventMask |= pCur->mask;
}

/*ARGSUSED*/
static int
WMFreeClient(void *data, XID id)
{
    WMEventPtr pEvent;
    WMEventPtr   *pHead, pCur, pPrev;
    int i;

    pEvent = (WMEventPtr)data;
    i = dixLookupResourceByType(
        (void **)&pHead, eventResource, EventType, serverClient,
        DixReadAccess |
        DixWriteAccess | DixDestroyAccess);
    if (i == Success && pHead) {
        pPrev = 0;
        for (pCur = *pHead; pCur && pCur != pEvent; pCur = pCur->next)
            pPrev = pCur;
        if (pCur) {
            if (pPrev)
                pPrev->next = pEvent->next;
            else
                *pHead = pEvent->next;
        }
        updateEventMask(pHead);
    }
    free((void *)pEvent);
    return 1;
}

/*ARGSUSED*/
static int
WMFreeEvents(void *data, XID id)
{
    WMEventPtr   *pHead, pCur, pNext;

    pHead = (WMEventPtr *)data;
    for (pCur = *pHead; pCur; pCur = pNext) {
        pNext = pCur->next;
        FreeResource(pCur->clientResource, ClientType);
        free((void *)pCur);
    }
    free((void *)pHead);
    eventMask = 0;
    return 1;
}

static int
ProcAppleWMSelectInput(register ClientPtr client)
{
    REQUEST(xAppleWMSelectInputReq);
    WMEventPtr pEvent, pNewEvent, *pHead;
    XID clientResource;
    int i;

    REQUEST_SIZE_MATCH(xAppleWMSelectInputReq);
    i =
        dixLookupResourceByType((void **)&pHead, eventResource, EventType,
                                client,
                                DixWriteAccess);
    if (stuff->mask != 0) {
        if (i == Success && pHead) {
            /* check for existing entry. */
            for (pEvent = *pHead; pEvent; pEvent = pEvent->next) {
                if (pEvent->client == client) {
                    pEvent->mask = stuff->mask;
                    updateEventMask(pHead);
                    return Success;
                }
            }
        }

        /* build the entry */
        pNewEvent = calloc(1, sizeof(WMEventRec));
        if (!pNewEvent)
            return BadAlloc;
        pNewEvent->next = 0;
        pNewEvent->client = client;
        pNewEvent->mask = stuff->mask;
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
         * of clients selecting input.  This must be indirect as
         * the list may be arbitrarily rearranged which cannot be
         * done through the resource database.
         */
        if (i != Success || !pHead) {
            pHead = calloc(1, sizeof(WMEventPtr));
            if (!pHead ||
                !AddResource(eventResource, EventType, (void *)pHead)) {
                FreeResource(clientResource, X11_RESTYPE_NONE);
                return BadAlloc;
            }
            *pHead = 0;
        }
        pNewEvent->next = *pHead;
        *pHead = pNewEvent;
        updateEventMask(pHead);
    }
    else if (stuff->mask == 0) {
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
                updateEventMask(pHead);
            }
        }
    }
    else {
        client->errorValue = stuff->mask;
        return BadValue;
    }
    return Success;
}

/*
 * deliver the event
 */

void
AppleWMSendEvent(int type, unsigned int mask, int which, int arg)
{
    WMEventPtr      *pHead, pEvent;
    xAppleWMNotifyEvent se;
    int i;

    i =
        dixLookupResourceByType((void **)&pHead, eventResource, EventType,
                                serverClient,
                                DixReadAccess);
    if (i != Success || !pHead)
        return;
    for (pEvent = *pHead; pEvent; pEvent = pEvent->next) {
        if ((pEvent->mask & mask) == 0)
            continue;
        se.type = type + WMEventBase;
        se.kind = which;
        se.arg = arg;
        se.time = currentTime.milliseconds;
        WriteEventsToClient(pEvent->client, 1, (xEvent *)&se);
    }
}

/* Safe to call from any thread. */
unsigned int
AppleWMSelectedEvents(void)
{
    return eventMask;
}

/* general utility functions */

static int
ProcAppleWMDisableUpdate(register ClientPtr client)
{
    REQUEST_SIZE_MATCH(xAppleWMDisableUpdateReq);

    appleWMProcs->DisableUpdate();

    return Success;
}

static int
ProcAppleWMReenableUpdate(register ClientPtr client)
{
    REQUEST_SIZE_MATCH(xAppleWMReenableUpdateReq);

    appleWMProcs->EnableUpdate();

    return Success;
}

/* window functions */

static int
ProcAppleWMSetWindowMenu(register ClientPtr client)
{
    const char *bytes;
    int max_len, nitems, i, j;
    REQUEST(xAppleWMSetWindowMenuReq);

    REQUEST_AT_LEAST_SIZE(xAppleWMSetWindowMenuReq);

    nitems = stuff->nitems;
    char **items = calloc(nitems, sizeof(char *));
    char *shortcuts = calloc(nitems, sizeof(char));

    if (!items || !shortcuts) {
        free(items);
        free(shortcuts);

        return BadAlloc;
    }

    max_len = (client->req_len << 2) - sizeof(xAppleWMSetWindowMenuReq);
    bytes = (char *)&stuff[1];

    for (i = j = 0; i < max_len && j < nitems;) {
        shortcuts[j] = bytes[i++];
        items[j++] = bytes + i;

        while (i < max_len)
        {
            if (bytes[i++] == 0)
                break;
        }
    }

    /* Check if we bailed out of the above loop due to a request that was too long */
    if (j < nitems) {
        free(items);
        free(shortcuts);

        return BadRequest;
    }

    X11ApplicationSetWindowMenu(nitems, items, shortcuts);
    free(items);
    free(shortcuts);

    return Success;
}

static int
ProcAppleWMSetWindowMenuCheck(register ClientPtr client)
{
    REQUEST(xAppleWMSetWindowMenuCheckReq);

    REQUEST_SIZE_MATCH(xAppleWMSetWindowMenuCheckReq);
    X11ApplicationSetWindowMenuCheck(stuff->index);
    return Success;
}

static int
ProcAppleWMSetFrontProcess(register ClientPtr client)
{
    REQUEST_SIZE_MATCH(xAppleWMSetFrontProcessReq);

    X11ApplicationSetFrontProcess();
    return Success;
}

static int
ProcAppleWMSetWindowLevel(register ClientPtr client)
{
    REQUEST(xAppleWMSetWindowLevelReq);
    WindowPtr pWin;
    int err;

    REQUEST_SIZE_MATCH(xAppleWMSetWindowLevelReq);

    if (Success != dixLookupWindow(&pWin, stuff->window, client,
                                   DixReadAccess))
        return BadValue;

    if (stuff->level >= AppleWMNumWindowLevels) {
        return BadValue;
    }

    err = appleWMProcs->SetWindowLevel(pWin, stuff->level);
    if (err != Success) {
        return err;
    }

    return Success;
}

static int
ProcAppleWMSendPSN(register ClientPtr client)
{
    REQUEST(xAppleWMSendPSNReq);
    int err;

    REQUEST_SIZE_MATCH(xAppleWMSendPSNReq);

    if (!appleWMProcs->SendPSN)
        return BadRequest;

    err = appleWMProcs->SendPSN(stuff->psn_hi, stuff->psn_lo);
    if (err != Success) {
        return err;
    }

    return Success;
}

static int
ProcAppleWMAttachTransient(register ClientPtr client)
{
    WindowPtr pWinChild, pWinParent;
    REQUEST(xAppleWMAttachTransientReq);
    int err;

    REQUEST_SIZE_MATCH(xAppleWMAttachTransientReq);

    if (!appleWMProcs->AttachTransient)
        return BadRequest;

    if (Success !=
        dixLookupWindow(&pWinChild, stuff->child, client, DixReadAccess))
        return BadValue;

    if (stuff->parent) {
        if (Success !=
            dixLookupWindow(&pWinParent, stuff->parent, client, DixReadAccess))
            return BadValue;
    }
    else {
        pWinParent = NULL;
    }

    err = appleWMProcs->AttachTransient(pWinChild, pWinParent);
    if (err != Success) {
        return err;
    }

    return Success;
}

static int
ProcAppleWMSetCanQuit(register ClientPtr client)
{
    REQUEST(xAppleWMSetCanQuitReq);

    REQUEST_SIZE_MATCH(xAppleWMSetCanQuitReq);

    X11ApplicationSetCanQuit(stuff->state);
    return Success;
}

/* frame functions */

static int
ProcAppleWMFrameGetRect(register ClientPtr client)
{
    BoxRec ir, or, rr;
    REQUEST(xAppleWMFrameGetRectReq);

    REQUEST_SIZE_MATCH(xAppleWMFrameGetRectReq);

    ir = make_box(stuff->ix, stuff->iy, stuff->iw, stuff->ih);
    or = make_box(stuff->ox, stuff->oy, stuff->ow, stuff->oh);

    if (appleWMProcs->FrameGetRect(stuff->frame_rect,
                                   stuff->frame_class,
                                   &or, &ir, &rr) != Success) {
        return BadValue;
    }

    xAppleWMFrameGetRectReply reply = {
        .x = rr.x1,
        .y = rr.y1,
        .w = rr.x2 - rr.x1,
        .h = rr.y2 - rr.y1,
    };

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
ProcAppleWMFrameHitTest(register ClientPtr client)
{
    BoxRec ir, or;
    int ret;
    REQUEST(xAppleWMFrameHitTestReq);

    REQUEST_SIZE_MATCH(xAppleWMFrameHitTestReq);

    ir = make_box(stuff->ix, stuff->iy, stuff->iw, stuff->ih);
    or = make_box(stuff->ox, stuff->oy, stuff->ow, stuff->oh);

    if (appleWMProcs->FrameHitTest(stuff->frame_class, stuff->px,
                                   stuff->py, &or, &ir, &ret) != Success) {
        return BadValue;
    }

    xAppleWMFrameHitTestReply reply = {
        .ret = ret
    };

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
ProcAppleWMFrameDraw(register ClientPtr client)
{
    BoxRec ir, or;
    unsigned int title_length, title_max;
    unsigned char *title_bytes;
    REQUEST(xAppleWMFrameDrawReq);
    WindowPtr pWin;

    REQUEST_AT_LEAST_SIZE(xAppleWMFrameDrawReq);

    if (Success != dixLookupWindow(&pWin, stuff->window, client,
                                   DixReadAccess))
        return BadValue;

    ir = make_box(stuff->ix, stuff->iy, stuff->iw, stuff->ih);
    or = make_box(stuff->ox, stuff->oy, stuff->ow, stuff->oh);

    title_length = stuff->title_length;
    title_max = (client->req_len << 2) - sizeof(xAppleWMFrameDrawReq);

    if (title_max < title_length)
        return BadValue;

    title_bytes = (unsigned char *)&stuff[1];

    errno = appleWMProcs->FrameDraw(pWin, stuff->frame_class,
                                    stuff->frame_attr, &or, &ir,
                                    title_length, title_bytes);
    if (errno != Success) {
        return errno;
    }

    return Success;
}

/* dispatch */

static int
ProcAppleWMDispatch(register ClientPtr client)
{
    REQUEST(xReq);

    switch (stuff->data) {
    case X_AppleWMQueryVersion:
        return ProcAppleWMQueryVersion(client);
    }

    if (!client->local)
        return WMErrorBase + AppleWMClientNotLocal;

    switch (stuff->data) {
    case X_AppleWMSelectInput:
        return ProcAppleWMSelectInput(client);

    case X_AppleWMDisableUpdate:
        return ProcAppleWMDisableUpdate(client);

    case X_AppleWMReenableUpdate:
        return ProcAppleWMReenableUpdate(client);

    case X_AppleWMSetWindowMenu:
        return ProcAppleWMSetWindowMenu(client);

    case X_AppleWMSetWindowMenuCheck:
        return ProcAppleWMSetWindowMenuCheck(client);

    case X_AppleWMSetFrontProcess:
        return ProcAppleWMSetFrontProcess(client);

    case X_AppleWMSetWindowLevel:
        return ProcAppleWMSetWindowLevel(client);

    case X_AppleWMSetCanQuit:
        return ProcAppleWMSetCanQuit(client);

    case X_AppleWMFrameGetRect:
        return ProcAppleWMFrameGetRect(client);

    case X_AppleWMFrameHitTest:
        return ProcAppleWMFrameHitTest(client);

    case X_AppleWMFrameDraw:
        return ProcAppleWMFrameDraw(client);

    case X_AppleWMSendPSN:
        return ProcAppleWMSendPSN(client);

    case X_AppleWMAttachTransient:
        return ProcAppleWMAttachTransient(client);

    default:
        return BadRequest;
    }
}

static void
SNotifyEvent(xAppleWMNotifyEvent *from, xAppleWMNotifyEvent *to)
{
    to->type = from->type;
    to->kind = from->kind;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->time, to->time);
    cpswapl(from->arg, to->arg);
}

void
AppleWMExtensionInit(AppleWMProcsPtr procsPtr)
{
    ExtensionEntry* extEntry;

    ClientType = CreateNewResourceType(WMFreeClient, "WMClient");
    EventType = CreateNewResourceType(WMFreeEvents, "WMEvent");
    eventResource = dixAllocServerXID();

    if (ClientType && EventType &&
        (extEntry = AddExtension(APPLEWMNAME,
                                 AppleWMNumberEvents,
                                 AppleWMNumberErrors,
                                 ProcAppleWMDispatch,
                                 ProcAppleWMDispatch,
                                 NULL,
                                 StandardMinorOpcode))) {
        size_t i;
        WMReqCode = (unsigned char)extEntry->base;
        WMErrorBase = extEntry->errorBase;
        WMEventBase = extEntry->eventBase;
        for (i = 0; i < AppleWMNumberEvents; i++)
            EventSwapVector[WMEventBase + i] = (EventSwapPtr)SNotifyEvent;
        appleWMProcs = procsPtr;
    }
}
