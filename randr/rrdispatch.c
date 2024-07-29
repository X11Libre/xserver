/*
 * Copyright © 2006 Keith Packard
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
 */
#include <dix-config.h>

#include "dix/dix_priv.h"
#include "randr/randrstr_priv.h"
#include "os/fmt.h"

#include "protocol-versions.h"

Bool
RRClientKnowsRates(ClientPtr pClient)
{
    rrClientPriv(pClient);

    return version_compare(pRRClient->major_version, pRRClient->minor_version,
                           1, 1) >= 0;
}

int
ProcRRQueryVersion(ClientPtr client)
{
    xRRQueryVersionReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0
    };
    REQUEST(xRRQueryVersionReq);
    rrClientPriv(client);

    REQUEST_SIZE_MATCH(xRRQueryVersionReq);
    pRRClient->major_version = stuff->majorVersion;
    pRRClient->minor_version = stuff->minorVersion;

    if (version_compare(stuff->majorVersion, stuff->minorVersion,
                        SERVER_RANDR_MAJOR_VERSION,
                        SERVER_RANDR_MINOR_VERSION) < 0) {
        rep.majorVersion = stuff->majorVersion;
        rep.minorVersion = stuff->minorVersion;
    }
    else {
        rep.majorVersion = SERVER_RANDR_MAJOR_VERSION;
        rep.minorVersion = SERVER_RANDR_MINOR_VERSION;
    }

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.majorVersion);
        swapl(&rep.minorVersion);
    }
    WriteToClient(client, sizeof(xRRQueryVersionReply), &rep);
    return Success;
}

int
ProcRRSelectInput(ClientPtr client)
{
    REQUEST(xRRSelectInputReq);
    rrClientPriv(client);
    RRTimesPtr pTimes;
    WindowPtr pWin;
    RREventPtr pRREvent, *pHead;
    XID clientResource;
    int rc;

    REQUEST_SIZE_MATCH(xRRSelectInputReq);
    rc = dixLookupWindow(&pWin, stuff->window, client, DixReceiveAccess);
    if (rc != Success)
        return rc;
    rc = dixLookupResourceByType((void **) &pHead, pWin->drawable.id,
                                 RREventType, client, DixWriteAccess);
    if (rc != Success && rc != BadValue)
        return rc;

    if (stuff->enable & (RRScreenChangeNotifyMask |
                         RRCrtcChangeNotifyMask |
                         RROutputChangeNotifyMask |
                         RROutputPropertyNotifyMask |
                         RRProviderChangeNotifyMask |
                         RRProviderPropertyNotifyMask |
                         RRResourceChangeNotifyMask)) {
        ScreenPtr pScreen = pWin->drawable.pScreen;

        rrScrPriv(pScreen);

        pRREvent = NULL;
        if (pHead) {
            /* check for existing entry. */
            for (pRREvent = *pHead; pRREvent; pRREvent = pRREvent->next)
                if (pRREvent->client == client)
                    break;
        }

        if (!pRREvent) {
            /* build the entry */
            pRREvent = (RREventPtr) malloc(sizeof(RREventRec));
            if (!pRREvent)
                return BadAlloc;
            pRREvent->next = 0;
            pRREvent->client = client;
            pRREvent->window = pWin;
            pRREvent->mask = stuff->enable;
            /*
             * add a resource that will be deleted when
             * the client goes away
             */
            clientResource = FakeClientID(client->index);
            pRREvent->clientResource = clientResource;
            if (!AddResource(clientResource, RRClientType, (void *) pRREvent))
                return BadAlloc;
            /*
             * create a resource to contain a pointer to the list
             * of clients selecting input.  This must be indirect as
             * the list may be arbitrarily rearranged which cannot be
             * done through the resource database.
             */
            if (!pHead) {
                pHead = (RREventPtr *) malloc(sizeof(RREventPtr));
                if (!pHead ||
                    !AddResource(pWin->drawable.id, RREventType,
                                 (void *) pHead)) {
                    FreeResource(clientResource, X11_RESTYPE_NONE);
                    return BadAlloc;
                }
                *pHead = 0;
            }
            pRREvent->next = *pHead;
            *pHead = pRREvent;
        }
        /*
         * Now see if the client needs an event
         */
        if (pScrPriv) {
            pTimes = &((RRTimesPtr) (pRRClient + 1))[pScreen->myNum];
            if (CompareTimeStamps(pTimes->setTime,
                                  pScrPriv->lastSetTime) != 0 ||
                CompareTimeStamps(pTimes->configTime,
                                  pScrPriv->lastConfigTime) != 0) {
                if (pRREvent->mask & RRScreenChangeNotifyMask) {
                    RRDeliverScreenEvent(client, pWin, pScreen);
                }

                if (pRREvent->mask & RRCrtcChangeNotifyMask) {
                    int i;

                    for (i = 0; i < pScrPriv->numCrtcs; i++) {
                        RRDeliverCrtcEvent(client, pWin, pScrPriv->crtcs[i]);
                    }
                }

                if (pRREvent->mask & RROutputChangeNotifyMask) {
                    int i;

                    for (i = 0; i < pScrPriv->numOutputs; i++) {
                        RRDeliverOutputEvent(client, pWin,
                                             pScrPriv->outputs[i]);
                    }
                }

                /* We don't check for RROutputPropertyNotifyMask, as randrproto.txt doesn't
                 * say if there ought to be notifications of changes to output properties
                 * if those changes occurred before the time RRSelectInput is called.
                 */
            }
        }
    }
    else if (stuff->enable == 0) {
        /* delete the interest */
        if (pHead) {
            RREventPtr pNewRREvent = 0;

            for (pRREvent = *pHead; pRREvent; pRREvent = pRREvent->next) {
                if (pRREvent->client == client)
                    break;
                pNewRREvent = pRREvent;
            }
            if (pRREvent) {
                FreeResource(pRREvent->clientResource, RRClientType);
                if (pNewRREvent)
                    pNewRREvent->next = pRREvent->next;
                else
                    *pHead = pRREvent->next;
                free(pRREvent);
            }
        }
    }
    else {
        client->errorValue = stuff->enable;
        return BadValue;
    }
    return Success;
}

#define HANDLER(name) case X_##name: return Proc##name(client)

int
ProcRRDispatch(ClientPtr client)
{
    REQUEST(xReq);
    UpdateCurrentTimeIf();

    switch (stuff->data) {
        HANDLER(RRQueryVersion);
        HANDLER(RRSetScreenConfig);
        HANDLER(RRSelectInput);
        HANDLER(RRGetScreenInfo);

        /* V1.2 additions */
        HANDLER(RRGetScreenSizeRange);
        HANDLER(RRSetScreenSize);
        HANDLER(RRGetScreenResources);
        HANDLER(RRGetOutputInfo);
        HANDLER(RRListOutputProperties);
        HANDLER(RRQueryOutputProperty);
        HANDLER(RRConfigureOutputProperty);
        HANDLER(RRChangeOutputProperty);
        HANDLER(RRDeleteOutputProperty);
        HANDLER(RRGetOutputProperty);
        HANDLER(RRCreateMode);
        HANDLER(RRDestroyMode);
        HANDLER(RRAddOutputMode);
        HANDLER(RRDeleteOutputMode);
        HANDLER(RRGetCrtcInfo);
        HANDLER(RRSetCrtcConfig);
        HANDLER(RRGetCrtcGammaSize);
        HANDLER(RRGetCrtcGamma);
        HANDLER(RRSetCrtcGamma);

        /* V1.3 additions */
        HANDLER(RRGetScreenResourcesCurrent);
        HANDLER(RRSetCrtcTransform);
        HANDLER(RRGetCrtcTransform);
        HANDLER(RRGetPanning);
        HANDLER(RRSetPanning);
        HANDLER(RRSetOutputPrimary);
        HANDLER(RRGetOutputPrimary);

        /* V1.4 additions */
        HANDLER(RRGetProviders);
        HANDLER(RRGetProviderInfo);
        HANDLER(RRSetProviderOffloadSink);
        HANDLER(RRSetProviderOutputSource);
        HANDLER(RRListProviderProperties);
        HANDLER(RRQueryProviderProperty);
        HANDLER(RRConfigureProviderProperty);
        HANDLER(RRChangeProviderProperty);
        HANDLER(RRDeleteProviderProperty);
        HANDLER(RRGetProviderProperty);

        /* V1.5 additions */
        HANDLER(RRGetMonitors);
        HANDLER(RRSetMonitor);
        HANDLER(RRDeleteMonitor);

        /* V1.6 additions */
        HANDLER(RRCreateLease);
        HANDLER(RRFreeLease);
    }

    return BadRequest;
}
