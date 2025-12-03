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
#include "dix/request_priv.h"
#include "randr/randrstr_priv.h"
#include "randr/rrdispatch_priv.h"
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
    REQUEST(xRRQueryVersionReq);
    REQUEST_SIZE_MATCH(xRRQueryVersionReq);

    if (client->swapped) {
        swapl(&stuff->majorVersion);
        swapl(&stuff->minorVersion);
    }

    rrClientPriv(client);

    pRRClient->major_version = stuff->majorVersion;
    pRRClient->minor_version = stuff->minorVersion;

    xRRQueryVersionReply reply = {
        .majorVersion = SERVER_RANDR_MAJOR_VERSION,
        .minorVersion = SERVER_RANDR_MINOR_VERSION
    };

    if (version_compare(stuff->majorVersion, stuff->minorVersion,
                        SERVER_RANDR_MAJOR_VERSION,
                        SERVER_RANDR_MINOR_VERSION) < 0) {
        reply.majorVersion = stuff->majorVersion;
        reply.minorVersion = stuff->minorVersion;
    }

    if (client->swapped) {
        swapl(&reply.majorVersion);
        swapl(&reply.minorVersion);
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

int
ProcRRSelectInput(ClientPtr client)
{
    REQUEST(xRRSelectInputReq);
    REQUEST_SIZE_MATCH(xRRSelectInputReq);

    if (client->swapped) {
        swapl(&stuff->window);
        swaps(&stuff->enable);
    }

    rrClientPriv(client);
    RRTimesPtr pTimes;
    WindowPtr pWin;
    RREventPtr pRREvent, *pHead;
    XID clientResource;
    int rc;

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
            pRREvent = calloc(1, sizeof(RREventRec));
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
                pHead = calloc(1, sizeof(RREventPtr));
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

int
ProcRRDispatch(ClientPtr client)
{
    REQUEST(xReq);
    UpdateCurrentTimeIf();

    switch (stuff->data) {
        case X_RRQueryVersion:              return ProcRRQueryVersion(client);
        case X_RRSetScreenConfig:           return ProcRRSetScreenConfig(client);
        case X_RRSelectInput:               return ProcRRSelectInput(client);
        case X_RRGetScreenInfo:             return ProcRRGetScreenInfo(client);

        /* V1.2 additions */
        case X_RRGetScreenSizeRange:        return ProcRRGetScreenSizeRange(client);
        case X_RRSetScreenSize:             return ProcRRSetScreenSize(client);
        case X_RRGetScreenResources:        return ProcRRGetScreenResources(client);
        case X_RRGetOutputInfo:             return ProcRRGetOutputInfo(client);
        case X_RRListOutputProperties:      return ProcRRListOutputProperties(client);
        case X_RRQueryOutputProperty:       return ProcRRQueryOutputProperty(client);
        case X_RRConfigureOutputProperty:   return ProcRRConfigureOutputProperty(client);
        case X_RRChangeOutputProperty:      return ProcRRChangeOutputProperty(client);
        case X_RRDeleteOutputProperty:      return ProcRRDeleteOutputProperty(client);
        case X_RRGetOutputProperty:         return ProcRRGetOutputProperty(client);
        case X_RRCreateMode:                return ProcRRCreateMode(client);
        case X_RRDestroyMode:               return ProcRRDestroyMode(client);
        case X_RRAddOutputMode:             return ProcRRAddOutputMode(client);
        case X_RRDeleteOutputMode:          return ProcRRDeleteOutputMode(client);
        case X_RRGetCrtcInfo:               return ProcRRGetCrtcInfo(client);
        case X_RRSetCrtcConfig:             return ProcRRSetCrtcConfig(client);
        case X_RRGetCrtcGammaSize:          return ProcRRGetCrtcGammaSize(client);
        case X_RRGetCrtcGamma:              return ProcRRGetCrtcGamma(client);
        case X_RRSetCrtcGamma:              return ProcRRSetCrtcGamma(client);

        /* V1.3 additions */
        case X_RRGetScreenResourcesCurrent: return ProcRRGetScreenResourcesCurrent(client);
        case X_RRSetCrtcTransform:          return ProcRRSetCrtcTransform(client);
        case X_RRGetCrtcTransform:          return ProcRRGetCrtcTransform(client);
        case X_RRGetPanning:                return ProcRRGetPanning(client);
        case X_RRSetPanning:                return ProcRRSetPanning(client);
        case X_RRSetOutputPrimary:          return ProcRRSetOutputPrimary(client);
        case X_RRGetOutputPrimary:          return ProcRRGetOutputPrimary(client);

        /* V1.4 additions */
        case X_RRGetProviders:              return ProcRRGetProviders(client);
        case X_RRGetProviderInfo:           return ProcRRGetProviderInfo(client);
        case X_RRSetProviderOffloadSink:    return ProcRRSetProviderOffloadSink(client);
        case X_RRSetProviderOutputSource:   return ProcRRSetProviderOutputSource(client);
        case X_RRListProviderProperties:    return ProcRRListProviderProperties(client);
        case X_RRQueryProviderProperty:     return ProcRRQueryProviderProperty(client);
        case X_RRConfigureProviderProperty: return ProcRRConfigureProviderProperty(client);
        case X_RRChangeProviderProperty:    return ProcRRChangeProviderProperty(client);
        case X_RRDeleteProviderProperty:    return ProcRRDeleteProviderProperty(client);
        case X_RRGetProviderProperty:       return ProcRRGetProviderProperty(client);

        /* V1.5 additions */
        case X_RRGetMonitors:               return ProcRRGetMonitors(client);
        case X_RRSetMonitor:                return ProcRRSetMonitor(client);
        case X_RRDeleteMonitor:             return ProcRRDeleteMonitor(client);

        /* V1.6 additions */
        case X_RRCreateLease:               return ProcRRCreateLease(client);
        case X_RRFreeLease:                 return ProcRRFreeLease(client);
    }

    return BadRequest;
}
