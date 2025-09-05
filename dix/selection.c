/************************************************************

Copyright 1987, 1989, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

Copyright 1987, 1989 by Digital Equipment Corporation, Maynard, Massachusetts.

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

********************************************************/

#include <dix-config.h>

#include "dix/dix_priv.h"
#include "dix/selection_priv.h"

#include "windowstr.h"
#include "dixstruct.h"
#include "dispatch.h"
#include "xace.h"

/*****************************************************************
 * Selection Stuff
 *
 *    dixLookupSelection
 *
 *   Selections are global to the server.  The list of selections should
 *   not be traversed directly.  Instead, use the functions listed above.
 *
 *****************************************************************/

Selection *CurrentSelections;
CallbackListPtr SelectionCallback;
CallbackListPtr SelectionFilterCallback = NULL;

int
dixLookupSelection(Selection ** result, Atom selectionName,
                   ClientPtr client, Mask access_mode)
{
    Selection *pSel;
    int rc = BadMatch;

    client->errorValue = selectionName;

    for (pSel = CurrentSelections; pSel; pSel = pSel->next)
        if (pSel->selection == selectionName)
            break;

    if (!pSel) {
        pSel = dixAllocateObjectWithPrivates(Selection, PRIVATE_SELECTION);
        if (!pSel)
            return BadAlloc;
        pSel->selection = selectionName;
        pSel->next = CurrentSelections;
        CurrentSelections = pSel;
    }

    /* security creation/labeling check */
    rc = XaceHookSelectionAccess(client, &pSel, access_mode | DixCreateAccess);
    if (rc != Success) {
        return rc;
    }

    *result = pSel;
    return rc;
}

void
InitSelections(void)
{
    Selection *pSel, *pNextSel;

    pSel = CurrentSelections;
    while (pSel) {
        pNextSel = pSel->next;
        dixFreeObjectWithPrivates(pSel, PRIVATE_SELECTION);
        pSel = pNextSel;
    }

    CurrentSelections = NULL;
}

static inline void
CallSelectionCallback(Selection * pSel, ClientPtr client,
                      SelectionCallbackKind kind)
{
    SelectionInfoRec info = { pSel, client, kind };
    CallCallbacks(&SelectionCallback, &info);
}

void
DeleteWindowFromAnySelections(WindowPtr pWin)
{
    for (Selection *pSel = CurrentSelections; pSel; pSel = pSel->next)
        if (pSel->pWin == pWin) {
            CallSelectionCallback(pSel, NULL, SelectionWindowDestroy);

            pSel->pWin = (WindowPtr) NULL;
            pSel->window = None;
            pSel->client = NULL;
        }
}

void
DeleteClientFromAnySelections(ClientPtr client)
{
    for (Selection *pSel = CurrentSelections; pSel; pSel = pSel->next)
        if (pSel->client == client) {
            CallSelectionCallback(pSel, NULL, SelectionClientClose);

            pSel->pWin = (WindowPtr) NULL;
            pSel->window = None;
            pSel->client = NULL;
        }
}

int
ProcSetSelectionOwner(ClientPtr client)
{
    WindowPtr pWin = NULL;
    TimeStamp time;
    Selection *pSel;
    int rc;

    REQUEST(xSetSelectionOwnerReq);
    REQUEST_SIZE_MATCH(xSetSelectionOwnerReq);

    UpdateCurrentTime();
    time = ClientTimeToServerTime(stuff->time);

    /* If the client's time stamp is in the future relative to the server's
       time stamp, do not set the selection, just return success. */
    if (CompareTimeStamps(time, currentTime) == LATER)
        return Success;

    /* allow extensions to intercept */
    SelectionFilterParamRec param = {
        .client = client,
        .selection = stuff->selection,
        .owner = stuff->window,
        .op = SELECTION_FILTER_SETOWNER,
    };
    CallCallbacks(&SelectionFilterCallback, &param);
    if (param.skip) {
        if (param.status != Success)
            client->errorValue = stuff->selection;
        return param.status;
    }

    if (param.owner != None) {
        rc = dixLookupWindow(&pWin, param.owner, client, DixSetAttrAccess);
        if (rc != Success)
            return rc;
    }

    if (!ValidAtom(param.selection)) {
        client->errorValue = stuff->selection;
        return BadAtom;
    }

    /*
     * First, see if the selection is already set...
     */
    rc = dixLookupSelection(&pSel, param.selection, client, DixSetAttrAccess);
    if (rc != Success) {
        client->errorValue = stuff->selection;
        return rc;
    }

    /* If the timestamp in client's request is in the past relative
       to the time stamp indicating the last time the owner of the
       selection was set, do not set the selection, just return
       success. */
    if (CompareTimeStamps(time, pSel->lastTimeChanged) == EARLIER)
        return Success;
    if (pSel->client && (!pWin || (pSel->client != client))) {
        SelectionFilterParamRec eventParam = {
            .client = client,
            .recvClient = pSel->client,
            .owner = pSel->window,
            .selection = stuff->selection,
            .op = SELECTION_FILTER_EV_CLEAR,
        };
        CallCallbacks(&SelectionFilterCallback, &eventParam);
        if (!param.skip) {
            xEvent event = {
                .u.selectionClear.time = time.milliseconds,
                .u.selectionClear.window = eventParam.owner,
                .u.selectionClear.atom = eventParam.selection,
            };
            event.u.u.type = SelectionClear;
            WriteEventsToClient(eventParam.recvClient, 1, &event);
        }
    }

    pSel->lastTimeChanged = time;
    pSel->window = param.owner;
    pSel->pWin = pWin;
    pSel->client = (pWin ? client : NULL);

    CallSelectionCallback(pSel, client, SelectionSetOwner);
    return Success;
}

int
ProcGetSelectionOwner(ClientPtr client)
{
    Selection *pSel;

    REQUEST(xResourceReq);
    REQUEST_SIZE_MATCH(xResourceReq);

    /* allow extensions to intercept */
    SelectionFilterParamRec param = {
        .client = client,
        .selection = stuff->id,
        .op = SELECTION_FILTER_GETOWNER,
    };
    CallCallbacks(&SelectionFilterCallback, &param);
    if (param.skip) {
        goto out;
    }

    if (!ValidAtom(param.selection)) {
        param.status = BadAtom;
        goto out;
    }

    xGetSelectionOwnerReply rep = { 0 };

    param.status = dixLookupSelection(&pSel, param.selection, param.client, DixGetAttrAccess);
    if (param.status == Success)
        rep.owner = pSel->window;
    else if (param.status == BadMatch)
        rep.owner = None;
    else
        goto out;

    if (client->swapped) {
        swapl(&rep.owner);
    }

    X_SEND_REPLY_SIMPLE(client, rep);
    return Success;

out:
    if (param.status != Success)
        client->errorValue = stuff->id;
    return param.status;
}

int
ProcConvertSelection(ClientPtr client)
{
    Bool paramsOkay;
    xEvent event;
    WindowPtr pWin;
    Selection *pSel;
    int rc;

    REQUEST(xConvertSelectionReq);
    REQUEST_SIZE_MATCH(xConvertSelectionReq);

    /* allow extensions to intercept */
    SelectionFilterParamRec param = {
        .client = client,
        .selection = stuff->selection,
        .op = SELECTION_FILTER_CONVERT,
        .requestor = stuff->requestor,
        .property = stuff->property,
        .target = stuff->target,
        .time = stuff->time,
    };
    CallCallbacks(&SelectionFilterCallback, &param);
    if (param.skip) {
        if (param.status != Success)
            client->errorValue = stuff->selection;
        return param.status;
    }

    rc = dixLookupWindow(&pWin, param.requestor, client, DixSetAttrAccess);
    if (rc != Success)
        return rc;

    paramsOkay = ValidAtom(param.selection) && ValidAtom(param.target);
    paramsOkay &= (param.property == None) || ValidAtom(param.property);
    if (!paramsOkay) {
        client->errorValue = stuff->property;
        return BadAtom;
    }

    if (stuff->time == CurrentTime)
        UpdateCurrentTime();

    rc = dixLookupSelection(&pSel, param.selection, client, DixReadAccess);

    memset(&event, 0, sizeof(xEvent));
    if (rc != Success && rc != BadMatch)
        return rc;

    /* If the specified selection has an owner, the X server sends
       SelectionRequest event to that owner */
    if (rc == Success && pSel->window != None && pSel->client &&
        pSel->client != serverClient && !pSel->client->clientGone)
    {
        SelectionFilterParamRec evParam = {
            .client = client,
            .selection = stuff->selection,
            .op = SELECTION_FILTER_EV_REQUEST,
            .owner = pSel->window,
            .requestor = stuff->requestor,
            .property = stuff->property,
            .target = stuff->target,
            .time = stuff->time,
            .recvClient = pSel->client,
        };

        CallCallbacks(&SelectionFilterCallback, &evParam);
        if (evParam.skip) {
            if (evParam.status != Success)
                client->errorValue = stuff->selection;
            return evParam.status;
        }

        event.u.u.type = SelectionRequest;
        event.u.selectionRequest.owner = evParam.owner;
        event.u.selectionRequest.time = evParam.time;
        event.u.selectionRequest.requestor = evParam.requestor;
        event.u.selectionRequest.selection = evParam.selection;
        event.u.selectionRequest.target = evParam.target;
        event.u.selectionRequest.property = evParam.property;
        WriteEventsToClient(evParam.recvClient, 1, &event);
        return Success;
    }

    /* If no owner for the specified selection exists, the X server generates
       a SelectionNotify event to the requestor with property None. */
    param.property = None;
    CallCallbacks(&SelectionFilterCallback, &param);
    if (param.skip) {
        if (param.status != Success)
            client->errorValue = stuff->selection;
        return param.status;
    }

    event.u.u.type = SelectionNotify;
    event.u.selectionNotify.time = param.time;
    event.u.selectionNotify.requestor = param.requestor;
    event.u.selectionNotify.selection = param.selection;
    event.u.selectionNotify.target = param.target;
    event.u.selectionNotify.property = param.property;
    WriteEventsToClient(client, 1, &event);
    return Success;
}
