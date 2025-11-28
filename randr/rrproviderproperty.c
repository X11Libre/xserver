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
#include "os/mathx_priv.h"
#include "randr/randrstr_priv.h"
#include "randr/rrdispatch_priv.h"

#include "propertyst.h"
#include "swaprep.h"

static int
DeliverPropertyEvent(WindowPtr pWin, void *value)
{
    xRRProviderPropertyNotifyEvent *event = value;
    RREventPtr *pHead, pRREvent;

    dixLookupResourceByType((void **) &pHead, pWin->drawable.id,
                            RREventType, serverClient, DixReadAccess);
    if (!pHead)
        return WT_WALKCHILDREN;

    for (pRREvent = *pHead; pRREvent; pRREvent = pRREvent->next) {
        if (!(pRREvent->mask & RRProviderPropertyNotifyMask))
            continue;

        event->window = pRREvent->window->drawable.id;
        WriteEventsToClient(pRREvent->client, 1, (xEvent *) event);
    }

    return WT_WALKCHILDREN;
}

static void
RRDeliverPropertyEvent(ScreenPtr pScreen, xEvent *event)
{
    if (!(dispatchException & (DE_TERMINATE)))
        WalkTree(pScreen, DeliverPropertyEvent, event);
}

static void
RRDestroyProviderProperty(RRPropertyPtr prop)
{
    free(prop->valid_values);
    free(prop->current.data);
    free(prop->pending.data);
    free(prop);
}

static void
RRDeleteProperty(RRProviderRec * provider, RRPropertyRec * prop)
{
    xRRProviderPropertyNotifyEvent event = {
        .type = RREventBase + RRNotify,
        .subCode = RRNotify_ProviderProperty,
        .provider = provider->id,
        .state = PropertyDelete,
        .atom = prop->propertyName,
        .timestamp = currentTime.milliseconds
    };

    RRDeliverPropertyEvent(provider->pScreen, (xEvent *) &event);

    RRDestroyProviderProperty(prop);
}

static void
RRInitProviderPropertyValue(RRPropertyValuePtr property_value)
{
    property_value->type = None;
    property_value->format = 0;
    property_value->size = 0;
    property_value->data = NULL;
}

static RRPropertyPtr
RRCreateProviderProperty(Atom property)
{
    RRPropertyPtr prop;

    prop = (RRPropertyPtr) calloc(1, sizeof(RRPropertyRec));
    if (!prop)
        return NULL;
    prop->propertyName = property;
    RRInitProviderPropertyValue(&prop->current);
    RRInitProviderPropertyValue(&prop->pending);
    return prop;
}

void
RRDeleteProviderProperty(RRProviderPtr provider, Atom property)
{
    RRPropertyRec *prop, **prev;

    for (prev = &provider->properties; (prop = *prev); prev = &(prop->next))
        if (prop->propertyName == property) {
            *prev = prop->next;
            RRDeleteProperty(provider, prop);
            return;
        }
}

/* shortcut for cleaning up property when failed to add */
static inline void cleanupProperty(RRPropertyPtr prop, Bool added) {
    if ((prop != NULL) && added)
        RRDestroyProviderProperty(prop);
}

int
RRChangeProviderProperty(RRProviderPtr provider, Atom property, Atom type,
                       int format, int mode, unsigned long len,
                       void *value, Bool sendevent, Bool pending)
{
    RRPropertyPtr prop;
    rrScrPrivPtr pScrPriv = rrGetScrPriv(provider->pScreen);
    int size_in_bytes;
    int total_size;
    unsigned long total_len;
    RRPropertyValuePtr prop_value;
    RRPropertyValueRec new_value;
    Bool add = FALSE;

    size_in_bytes = format >> 3;

    /* first see if property already exists */
    prop = RRQueryProviderProperty(provider, property);
    if (!prop) {                /* just add to list */
        prop = RRCreateProviderProperty(property);
        if (!prop)
            return BadAlloc;
        add = TRUE;
        mode = PropModeReplace;
    }
    if (pending && prop->is_pending)
        prop_value = &prop->pending;
    else
        prop_value = &prop->current;

    /* To append or prepend to a property the request format and type
       must match those of the already defined property.  The
       existing format and type are irrelevant when using the mode
       "PropModeReplace" since they will be written over. */

    if ((format != prop_value->format) && (mode != PropModeReplace))
        return BadMatch;
    if ((prop_value->type != type) && (mode != PropModeReplace))
        return BadMatch;
    new_value = *prop_value;
    if (mode == PropModeReplace)
        total_len = len;
    else
        total_len = prop_value->size + len;

    if (mode == PropModeReplace || len > 0) {
        void *new_data = NULL, *old_data = NULL;
        if (total_len > MAXINT / size_in_bytes) {
            cleanupProperty(prop, add);
            return BadValue;
        }
        total_size = total_len * size_in_bytes;
        new_value.data = calloc(1, total_size);
        if (!new_value.data && total_size) {
            cleanupProperty(prop, add);
            return BadAlloc;
        }
        new_value.size = len;
        new_value.type = type;
        new_value.format = format;

        switch (mode) {
        case PropModeReplace:
            new_data = new_value.data;
            old_data = NULL;
            break;
        case PropModeAppend:
            new_data = (void *) (((char *) new_value.data) +
                                  (prop_value->size * size_in_bytes));
            old_data = new_value.data;
            break;
        case PropModePrepend:
            new_data = new_value.data;
            old_data = (void *) (((char *) new_value.data) +
                                  (prop_value->size * size_in_bytes));
            break;
        }
        if (new_data)
            memcpy((char *) new_data, (char *) value, len * size_in_bytes);
        if (old_data)
            memcpy((char *) old_data, (char *) prop_value->data,
                   prop_value->size * size_in_bytes);

        if (pending && pScrPriv->rrProviderSetProperty &&
            !pScrPriv->rrProviderSetProperty(provider->pScreen, provider,
                                           prop->propertyName, &new_value)) {
            cleanupProperty(prop, add);
            free(new_value.data);
            return BadValue;
        }
        free(prop_value->data);
        *prop_value = new_value;
    }

    else if (len == 0) {
        /* do nothing */
    }

    if (add) {
        prop->next = provider->properties;
        provider->properties = prop;
    }

    if (pending && prop->is_pending)
        provider->pendingProperties = TRUE;

    if (sendevent) {
        xRRProviderPropertyNotifyEvent event = {
            .type = RREventBase + RRNotify,
            .subCode = RRNotify_ProviderProperty,
            .provider = provider->id,
            .state = PropertyNewValue,
            .atom = prop->propertyName,
            .timestamp = currentTime.milliseconds
        };
        RRDeliverPropertyEvent(provider->pScreen, (xEvent *) &event);
    }
    return Success;
}

RRPropertyPtr
RRQueryProviderProperty(RRProviderPtr provider, Atom property)
{
    RRPropertyPtr prop;

    for (prop = provider->properties; prop; prop = prop->next)
        if (prop->propertyName == property)
            return prop;
    return NULL;
}

RRPropertyValuePtr
RRGetProviderProperty(RRProviderPtr provider, Atom property, Bool pending)
{
    RRPropertyPtr prop = RRQueryProviderProperty(provider, property);
    rrScrPrivPtr pScrPriv = rrGetScrPriv(provider->pScreen);

    if (!prop)
        return NULL;
    if (pending && prop->is_pending)
        return &prop->pending;
    else {
#if RANDR_13_INTERFACE
        /* If we can, try to update the property value first */
        if (pScrPriv->rrProviderGetProperty)
            pScrPriv->rrProviderGetProperty(provider->pScreen, provider,
                                          prop->propertyName);
#endif
        return &prop->current;
    }
}

int
RRConfigureProviderProperty(RRProviderPtr provider, Atom property,
                          Bool pending, Bool range, Bool immutable,
                          int num_values, INT32 *values)
{
    RRPropertyPtr prop = RRQueryProviderProperty(provider, property);
    Bool add = FALSE;

    if (!prop) {
        prop = RRCreateProviderProperty(property);
        if (!prop)
            return BadAlloc;
        add = TRUE;
    }
    else if (prop->immutable && !immutable)
        return BadAccess;

    /*
     * ranges must have even number of values
     */
    if (range && (num_values & 1)) {
        cleanupProperty(prop, add);
        return BadMatch;
    }

    INT32 *new_values = NULL;
    if (num_values) {
        new_values = calloc(num_values, sizeof(INT32));
        if (!new_values) {
            cleanupProperty(prop, add);
            return BadAlloc;
        }
        memcpy(new_values, values, num_values * sizeof(INT32));
    }

    /*
     * Property moving from pending to non-pending
     * loses any pending values
     */
    if (prop->is_pending && !pending) {
        free(prop->pending.data);
        RRInitProviderPropertyValue(&prop->pending);
    }

    prop->is_pending = pending;
    prop->range = range;
    prop->immutable = immutable;
    prop->num_valid = num_values;
    free(prop->valid_values);
    prop->valid_values = new_values;

    if (add) {
        prop->next = provider->properties;
        provider->properties = prop;
    }

    return Success;
}

int
ProcRRListProviderProperties(ClientPtr client)
{
    REQUEST(xRRListProviderPropertiesReq);
    REQUEST_SIZE_MATCH(xRRListProviderPropertiesReq);

    if (client->swapped)
        swapl(&stuff->provider);

    int numProps = 0;
    RRProviderPtr provider;
    RRPropertyPtr prop;

    VERIFY_RR_PROVIDER(stuff->provider, provider, DixReadAccess);

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    for (prop = provider->properties; prop; prop = prop->next) {
        x_rpcbuf_write_CARD32(&rpcbuf, prop->propertyName);
        numProps++;
    }

    xRRListProviderPropertiesReply rep = {
        .nAtoms = numProps
    };

    if (client->swapped)
        swaps(&rep.nAtoms);

    return X_SEND_REPLY_WITH_RPCBUF(client, rep, rpcbuf);
}

int
ProcRRQueryProviderProperty(ClientPtr client)
{
    REQUEST(xRRQueryProviderPropertyReq);
    REQUEST_SIZE_MATCH(xRRQueryProviderPropertyReq);

    if (client->swapped) {
        swapl(&stuff->provider);
        swapl(&stuff->property);
    }

    RRProviderPtr provider;
    RRPropertyPtr prop;

    VERIFY_RR_PROVIDER(stuff->provider, provider, DixReadAccess);

    prop = RRQueryProviderProperty(provider, stuff->property);
    if (!prop)
        return BadName;

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };
    x_rpcbuf_write_INT32s(&rpcbuf, prop->valid_values, prop->num_valid);

    xRRQueryProviderPropertyReply rep = {
        .pending = prop->is_pending,
        .range = prop->range,
        .immutable = prop->immutable
    };

    return X_SEND_REPLY_WITH_RPCBUF(client, rep, rpcbuf);
}

int
ProcRRConfigureProviderProperty(ClientPtr client)
{
    REQUEST(xRRConfigureProviderPropertyReq);
    REQUEST_AT_LEAST_SIZE(xRRConfigureProviderPropertyReq);

    if (client->swapped) {
        swapl(&stuff->provider);
        swapl(&stuff->property);
        /* TODO: no way to specify format? */
        SwapRestL(stuff);
    }

    RRProviderPtr provider;
    int num_valid;

    VERIFY_RR_PROVIDER(stuff->provider, provider, DixReadAccess);

    num_valid =
        client->req_len - bytes_to_int32(sizeof(xRRConfigureProviderPropertyReq));
    return RRConfigureProviderProperty(provider, stuff->property, stuff->pending,
                                     stuff->range, FALSE, num_valid,
                                     (INT32 *) (stuff + 1));
}

int
ProcRRChangeProviderProperty(ClientPtr client)
{
    REQUEST(xRRChangeProviderPropertyReq);
    REQUEST_AT_LEAST_SIZE(xRRChangeProviderPropertyReq);

    if (client->swapped) {
        swapl(&stuff->provider);
        swapl(&stuff->property);
        swapl(&stuff->type);
        swapl(&stuff->nUnits);
        switch (stuff->format) {
            case 8:
                break;
            case 16:
                SwapRestS(stuff);
                break;
            case 32:
                SwapRestL(stuff);
                break;
        }
    }

    RRProviderPtr provider;
    char format, mode;
    unsigned long len;
    int sizeInBytes;
    uint64_t totalSize;
    int err;

    UpdateCurrentTime();
    format = stuff->format;
    mode = stuff->mode;
    if ((mode != PropModeReplace) && (mode != PropModeAppend) &&
        (mode != PropModePrepend)) {
        client->errorValue = mode;
        return BadValue;
    }
    if ((format != 8) && (format != 16) && (format != 32)) {
        client->errorValue = format;
        return BadValue;
    }
    len = stuff->nUnits;
    if (len > bytes_to_int32((0xffffffff - sizeof(xChangePropertyReq))))
        return BadLength;
    sizeInBytes = format >> 3;
    totalSize = len * sizeInBytes;
    REQUEST_FIXED_SIZE(xRRChangeProviderPropertyReq, totalSize);

    VERIFY_RR_PROVIDER(stuff->provider, provider, DixReadAccess);

    if (!ValidAtom(stuff->property)) {
        client->errorValue = stuff->property;
        return BadAtom;
    }
    if (!ValidAtom(stuff->type)) {
        client->errorValue = stuff->type;
        return BadAtom;
    }

    err = RRChangeProviderProperty(provider, stuff->property,
                                 stuff->type, (int) format,
                                 (int) mode, len, (void *) &stuff[1], TRUE,
                                 TRUE);
    if (err != Success)
        return err;
    else
        return Success;
}

int
ProcRRDeleteProviderProperty(ClientPtr client)
{
    REQUEST(xRRDeleteProviderPropertyReq);
    REQUEST_SIZE_MATCH(xRRDeleteProviderPropertyReq);

    if (client->swapped) {
        swapl(&stuff->provider);
        swapl(&stuff->property);
    }

    RRProviderPtr provider;
    RRPropertyPtr prop;

    UpdateCurrentTime();
    VERIFY_RR_PROVIDER(stuff->provider, provider, DixReadAccess);

    if (!ValidAtom(stuff->property)) {
        client->errorValue = stuff->property;
        return BadAtom;
    }

    prop = RRQueryProviderProperty(provider, stuff->property);
    if (!prop) {
        client->errorValue = stuff->property;
        return BadName;
    }

    if (prop->immutable) {
        client->errorValue = stuff->property;
        return BadAccess;
    }

    RRDeleteProviderProperty(provider, stuff->property);
    return Success;
}

int
ProcRRGetProviderProperty(ClientPtr client)
{
    REQUEST(xRRGetProviderPropertyReq);
    REQUEST_SIZE_MATCH(xRRGetProviderPropertyReq);

    if (client->swapped) {
        swapl(&stuff->provider);
        swapl(&stuff->property);
        swapl(&stuff->type);
        swapl(&stuff->longOffset);
        swapl(&stuff->longLength);
    }

    RRPropertyPtr prop, *prev;
    RRPropertyValuePtr prop_value;
    unsigned long n, len, ind;
    RRProviderPtr provider;

    if (stuff->delete)
        UpdateCurrentTime();
    VERIFY_RR_PROVIDER(stuff->provider, provider,
                     stuff->delete ? DixWriteAccess : DixReadAccess);

    if (!ValidAtom(stuff->property)) {
        client->errorValue = stuff->property;
        return BadAtom;
    }
    if ((stuff->delete != xTrue) && (stuff->delete != xFalse)) {
        client->errorValue = stuff->delete;
        return BadValue;
    }
    if ((stuff->type != AnyPropertyType) && !ValidAtom(stuff->type)) {
        client->errorValue = stuff->type;
        return BadAtom;
    }

    for (prev = &provider->properties; (prop = *prev); prev = &prop->next)
        if (prop->propertyName == stuff->property)
            break;

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    xRRGetProviderPropertyReply reply = { 0 };

    if (!prop)
        goto sendout;

    if (prop->immutable && stuff->delete)
        return BadAccess;

    prop_value = RRGetProviderProperty(provider, stuff->property, stuff->pending);
    if (!prop_value)
        return BadAtom;

    /* If the request type and actual type don't match. Return the
       property information, but not the data. */

    if (((stuff->type != prop_value->type) && (stuff->type != AnyPropertyType))
        ) {
        reply.bytesAfter = prop_value->size;
        reply.format = prop_value->format;
        reply.propertyType = prop_value->type;
        if (client->swapped) {
            swapl(&reply.propertyType);
            swapl(&reply.bytesAfter);
        }

        goto sendout;
    }

/*
 *  Return type, format, value to client
 */
    n = (prop_value->format / 8) * prop_value->size;    /* size (bytes) of prop */
    ind = stuff->longOffset << 2;

    /* If longOffset is invalid such that it causes "len" to
       be negative, it's a value error. */

    if (n < ind) {
        client->errorValue = stuff->longOffset;
        return BadValue;
    }

    len = MIN(n - ind, 4 * stuff->longLength);

    reply.bytesAfter = n - (ind + len);
    reply.format = prop_value->format;
    if (prop_value->format)
        reply.nItems = len / (prop_value->format / 8);
    reply.propertyType = prop_value->type;

    if (stuff->delete && (reply.bytesAfter == 0)) {
        xRRProviderPropertyNotifyEvent event = {
            .type = RREventBase + RRNotify,
            .subCode = RRNotify_ProviderProperty,
            .provider = provider->id,
            .state = PropertyDelete,
            .atom = prop->propertyName,
            .timestamp = currentTime.milliseconds
        };
        RRDeliverPropertyEvent(provider->pScreen, (xEvent *) &event);
    }

    if (client->swapped) {
        swapl(&reply.propertyType);
        swapl(&reply.bytesAfter);
        swapl(&reply.nItems);
    }

    if (len) {
        const char *dataptr = ((char*)prop_value->data) + ind;
        switch (prop_value->format) {
        case 32:
            x_rpcbuf_write_CARD32s(&rpcbuf, (CARD32*)dataptr, len/sizeof(CARD32));
            break;
        case 16:
            x_rpcbuf_write_CARD16s(&rpcbuf, (CARD16*)dataptr, len/sizeof(CARD16));
            break;
        default:
            x_rpcbuf_write_CARD8s(&rpcbuf, (CARD8*)dataptr, len);
            break;
        }
    }

    if (stuff->delete && (reply.bytesAfter == 0)) {     /* delete the Property */
        *prev = prop->next;
        RRDestroyProviderProperty(prop);
    }

sendout:
    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}
