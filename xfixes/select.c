/*
 * Copyright © 2002 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <dix-config.h>

#include "dix/dix_priv.h"
#include "dix/request_priv.h"
#include "dix/selection_priv.h"

#include "xfixesint.h"
#include "xace.h"

static RESTYPE SelectionClientType, SelectionWindowType;
static Bool SelectionCallbackRegistered = FALSE;

/*
 * There is a global list of windows selecting for selection events
 * on every selection.  This should be plenty efficient for the
 * expected usage, if it does become a problem, it should be easily
 * replaced with a hash table of some kind keyed off the selection atom
 */

typedef struct _SelectionEvent *SelectionEventPtr;

typedef struct _SelectionEvent {
    SelectionEventPtr next;
    Selection *selection;
    CARD32 eventMask;
    ClientPtr pClient;
    WindowPtr pWindow;
    XID clientResource;
} SelectionEventRec;

static SelectionEventPtr selectionEvents;

static void
XFixesSelectionCallback(CallbackListPtr *callbacks, void *data, void *args)
{
    SelectionEventPtr e;
    SelectionInfoRec *info = (SelectionInfoRec *) args;
    Selection *selection = info->selection;
    int subtype;
    CARD32 eventMask;

    switch (info->kind) {
    case SelectionSetOwner:
        subtype = XFixesSetSelectionOwnerNotify;
        eventMask = XFixesSetSelectionOwnerNotifyMask;
        break;
    case SelectionWindowDestroy:
        subtype = XFixesSelectionWindowDestroyNotify;
        eventMask = XFixesSelectionWindowDestroyNotifyMask;
        break;
    case SelectionClientClose:
        subtype = XFixesSelectionClientCloseNotify;
        eventMask = XFixesSelectionClientCloseNotifyMask;
        break;
    default:
        return;
    }
    UpdateCurrentTimeIf();
    for (e = selectionEvents; e; e = e->next) {
        if (e->selection == selection && (e->eventMask & eventMask)) {

            /* allow extensions to intercept */
            SelectionFilterParamRec param = {
                .client = e->pClient,
                .selection = selection->selection,
                .owner = (subtype == XFixesSetSelectionOwnerNotify) ?
                            selection->window : 0,
                .op = SELECTION_FILTER_NOTIFY,
            };
            CallCallbacks(&SelectionFilterCallback, &param);
            if (param.skip)
                continue;

            xXFixesSelectionNotifyEvent ev = {
                .type = XFixesEventBase + XFixesSelectionNotify,
                .subtype = subtype,
                .window = e->pWindow->drawable.id,
                .owner = param.owner,
                .selection = param.selection,
                .timestamp = currentTime.milliseconds,
                .selectionTimestamp = selection->lastTimeChanged.milliseconds
            };
            WriteEventsToClient(e->pClient, 1, (xEvent *) &ev);
        }
    }
}

static Bool
CheckSelectionCallback(void)
{
    if (selectionEvents) {
        if (!SelectionCallbackRegistered) {
            if (!AddCallback(&SelectionCallback, XFixesSelectionCallback, NULL))
                return FALSE;
            SelectionCallbackRegistered = TRUE;
        }
    }
    else {
        if (SelectionCallbackRegistered) {
            DeleteCallback(&SelectionCallback, XFixesSelectionCallback, NULL);
            SelectionCallbackRegistered = FALSE;
        }
    }
    return TRUE;
}

#define SelectionAllEvents (XFixesSetSelectionOwnerNotifyMask |\
			    XFixesSelectionWindowDestroyNotifyMask |\
			    XFixesSelectionClientCloseNotifyMask)

int
ProcXFixesSelectSelectionInput(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXFixesSelectSelectionInputReq);
    X_REQUEST_FIELD_CARD32(window);
    X_REQUEST_FIELD_CARD32(selection);
    X_REQUEST_FIELD_CARD32(eventMask);

    /* allow extensions to intercept */
    SelectionFilterParamRec param = {
        .client = client,
        .selection = stuff->selection,
        .owner = stuff->window,
        .op = SELECTION_FILTER_LISTEN,
    };
    CallCallbacks(&SelectionFilterCallback, &param);
    if (param.skip) {
        if (param.status != Success)
            client->errorValue = param.selection;
        return param.status;
    }

    WindowPtr pWindow;
    int rc = dixLookupWindow(&pWindow, param.owner, param.client, DixGetAttrAccess);
    if (rc != Success)
        return rc;
    if (stuff->eventMask & ~SelectionAllEvents) {
        client->errorValue = stuff->eventMask;
        return BadValue;
    }

    void *val;
    SelectionEventPtr *prev, e;
    Selection *selection;

    rc = dixLookupSelection(&selection, param.selection, param.client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    for (prev = &selectionEvents; (e = *prev); prev = &e->next) {
        if (e->selection == selection &&
            e->pClient == param.client && e->pWindow == pWindow) {
            break;
        }
    }
    if (!stuff->eventMask) {
        if (e) {
            FreeResource(e->clientResource, 0);
        }
        return Success;
    }
    if (!e) {
        e = calloc(1, sizeof(SelectionEventRec));
        if (!e)
            return BadAlloc;

        e->next = 0;
        e->selection = selection;
        e->pClient = param.client;
        e->pWindow = pWindow;
        e->clientResource = FakeClientID(param.client->index);

        /*
         * Add a resource hanging from the window to
         * catch window destroy
         */
        rc = dixLookupResourceByType(&val, pWindow->drawable.id,
                                     SelectionWindowType, serverClient,
                                     DixGetAttrAccess);
        if (rc != Success)
            if (!AddResource(pWindow->drawable.id, SelectionWindowType,
                             (void *) pWindow)) {
                free(e);
                return BadAlloc;
            }

        if (!AddResource(e->clientResource, SelectionClientType, (void *) e))
            return BadAlloc;

        *prev = e;
        if (!CheckSelectionCallback()) {
            FreeResource(e->clientResource, 0);
            return BadAlloc;
        }
    }
    e->eventMask = stuff->eventMask;
    return Success;
}

void _X_COLD
SXFixesSelectionNotifyEvent(xXFixesSelectionNotifyEvent * from,
                            xXFixesSelectionNotifyEvent * to)
{
    to->type = from->type;
    cpswaps(from->sequenceNumber, to->sequenceNumber);
    cpswapl(from->window, to->window);
    cpswapl(from->owner, to->owner);
    cpswapl(from->selection, to->selection);
    cpswapl(from->timestamp, to->timestamp);
    cpswapl(from->selectionTimestamp, to->selectionTimestamp);
}

static int
SelectionFreeClient(void *data, XID id)
{
    SelectionEventPtr old = (SelectionEventPtr) data;
    SelectionEventPtr *prev, e;

    for (prev = &selectionEvents; (e = *prev); prev = &e->next) {
        if (e == old) {
            *prev = e->next;
            free(e);
            CheckSelectionCallback();
            break;
        }
    }
    return 1;
}

static int
SelectionFreeWindow(void *data, XID id)
{
    WindowPtr pWindow = (WindowPtr) data;
    SelectionEventPtr e, next;

    for (e = selectionEvents; e; e = next) {
        next = e->next;
        if (e->pWindow == pWindow) {
            FreeResource(e->clientResource, 0);
        }
    }
    return 1;
}

Bool
XFixesSelectionInit(void)
{
    SelectionClientType = CreateNewResourceType(SelectionFreeClient,
                                                "XFixesSelectionClient");
    SelectionWindowType = CreateNewResourceType(SelectionFreeWindow,
                                                "XFixesSelectionWindow");
    return SelectionClientType && SelectionWindowType;
}
