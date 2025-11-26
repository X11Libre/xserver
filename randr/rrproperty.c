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

#include <X11/Xatom.h>

#include "dix/dix_priv.h"
#include "dix/request_priv.h"
#include "os/mathx_priv.h"
#include "randr/rrdispatch_priv.h"

#include "randrstr_priv.h"
#include "propertyst.h"
#include "swaprep.h"

static int
DeliverPropertyEvent(WindowPtr pWin, void *value)
{
    xRROutputPropertyNotifyEvent *event = value;
    RREventPtr *pHead, pRREvent;

    dixLookupResourceByType((void **) &pHead, pWin->drawable.id,
                            RREventType, serverClient, DixReadAccess);
    if (!pHead)
        return WT_WALKCHILDREN;

    for (pRREvent = *pHead; pRREvent; pRREvent = pRREvent->next) {
        if (!(pRREvent->mask & RROutputPropertyNotifyMask))
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
RRDestroyOutputProperty(RRPropertyPtr prop)
{
    free(prop->valid_values);
    free(prop->current.data);
    free(prop->pending.data);
    free(prop);
}

static void
RRDeleteProperty(RROutputRec * output, RRPropertyRec * prop)
{
    xRROutputPropertyNotifyEvent event = {
        .type = RREventBase + RRNotify,
        .subCode = RRNotify_OutputProperty,
        .output = output->id,
        .state = PropertyDelete,
        .atom = prop->propertyName,
        .timestamp = currentTime.milliseconds
    };

    RRDeliverPropertyEvent(output->pScreen, (xEvent *) &event);

    RRDestroyOutputProperty(prop);
}

void
RRDeleteAllOutputProperties(RROutputPtr output)
{
    RRPropertyPtr prop, next;

    for (prop = output->properties; prop; prop = next) {
        next = prop->next;
        RRDeleteProperty(output, prop);
    }
}

static void
RRInitOutputPropertyValue(RRPropertyValuePtr property_value)
{
    property_value->type = None;
    property_value->format = 0;
    property_value->size = 0;
    property_value->data = NULL;
}

static RRPropertyPtr
RRCreateOutputProperty(Atom property)
{
    RRPropertyPtr prop = calloc(1, sizeof(RRPropertyRec));
    if (!prop)
        return NULL;
    prop->next = NULL;
    prop->propertyName = property;
    prop->is_pending = FALSE;
    prop->range = FALSE;
    prop->immutable = FALSE;
    prop->num_valid = 0;
    prop->valid_values = NULL;
    RRInitOutputPropertyValue(&prop->current);
    RRInitOutputPropertyValue(&prop->pending);
    return prop;
}

void
RRDeleteOutputProperty(RROutputPtr output, Atom property)
{
    RRPropertyRec *prop, **prev;

    for (prev = &output->properties; (prop = *prev); prev = &(prop->next))
        if (prop->propertyName == property) {
            *prev = prop->next;
            RRDeleteProperty(output, prop);
            return;
        }
}

static void
RRNoticePropertyChange(RROutputPtr output, Atom property, RRPropertyValuePtr value)
{
    const char *non_desktop_str = RR_PROPERTY_NON_DESKTOP;
    Atom non_desktop_prop = dixGetAtomID(non_desktop_str);

    if (property == non_desktop_prop) {
        if (value->type == XA_INTEGER && value->format == 32 && value->size >= 1) {
            uint32_t     nonDesktopData;
            Bool        nonDesktop;

            memcpy(&nonDesktopData, value->data, sizeof (nonDesktopData));
            nonDesktop = nonDesktopData != 0;

            if (nonDesktop != output->nonDesktop) {
                output->nonDesktop = nonDesktop;
                RROutputChanged(output, 0);
                RRTellChanged(output->pScreen);
            }
        }
    }
}

int
RRChangeOutputProperty(RROutputPtr output, Atom property, Atom type,
                       int format, int mode, unsigned long len,
                       const void *value, Bool sendevent, Bool pending)
{
    RRPropertyPtr prop;
    rrScrPrivPtr pScrPriv = rrGetScrPriv(output->pScreen);
    int size_in_bytes;
    unsigned long total_len;
    RRPropertyValuePtr prop_value;
    RRPropertyValueRec new_value;
    Bool add = FALSE;

    size_in_bytes = format >> 3;

    /* first see if property already exists */
    prop = RRQueryOutputProperty(output, property);
    if (!prop) {                /* just add to list */
        prop = RRCreateOutputProperty(property);
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

        new_value.data = calloc(total_len, size_in_bytes);
        if (!new_value.data && total_len && size_in_bytes) {
            if (add)
                RRDestroyOutputProperty(prop);
            return BadAlloc;
        }
        new_value.size = total_len;
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
                                  (len * size_in_bytes));
            break;
        }
        if (new_data)
            memcpy((char *) new_data, (char *) value, len * size_in_bytes);
        if (old_data)
            memcpy((char *) old_data, (char *) prop_value->data,
                   prop_value->size * size_in_bytes);

        if (pending && pScrPriv->rrOutputSetProperty &&
            !pScrPriv->rrOutputSetProperty(output->pScreen, output,
                                           prop->propertyName, &new_value)) {
            free(new_value.data);
            if (add)
                RRDestroyOutputProperty(prop);
            return BadValue;
        }
        free(prop_value->data);
        *prop_value = new_value;
    }

    else if (len == 0) {
        /* do nothing */
    }

    if (add) {
        prop->next = output->properties;
        output->properties = prop;
    }

    if (pending && prop->is_pending)
        output->pendingProperties = TRUE;

    if (!(pending && prop->is_pending))
        RRNoticePropertyChange(output, prop->propertyName, prop_value);

    if (sendevent) {
        xRROutputPropertyNotifyEvent event = {
            .type = RREventBase + RRNotify,
            .subCode = RRNotify_OutputProperty,
            .output = output->id,
            .state = PropertyNewValue,
            .atom = prop->propertyName,
            .timestamp = currentTime.milliseconds
        };
        RRDeliverPropertyEvent(output->pScreen, (xEvent *) &event);
    }
    return Success;
}

Bool
RRPostPendingProperties(RROutputPtr output)
{
    RRPropertyValuePtr pending_value;
    RRPropertyValuePtr current_value;
    RRPropertyPtr property;
    Bool ret = TRUE;

    if (!output->pendingProperties)
        return TRUE;

    output->pendingProperties = FALSE;
    for (property = output->properties; property; property = property->next) {
        /* Skip non-pending properties */
        if (!property->is_pending)
            continue;

        pending_value = &property->pending;
        current_value = &property->current;

        /*
         * If the pending and current values are equal, don't mark it
         * as changed (which would deliver an event)
         */
        if (pending_value->type == current_value->type &&
            pending_value->format == current_value->format &&
            pending_value->size == current_value->size &&
            !memcmp(pending_value->data, current_value->data,
                    pending_value->size * (pending_value->format / 8)))
            continue;

        if (RRChangeOutputProperty(output, property->propertyName,
                                   pending_value->type, pending_value->format,
                                   PropModeReplace, pending_value->size,
                                   pending_value->data, TRUE, FALSE) != Success)
            ret = FALSE;
    }
    return ret;
}

RRPropertyPtr
RRQueryOutputProperty(RROutputPtr output, Atom property)
{
    RRPropertyPtr prop;

    for (prop = output->properties; prop; prop = prop->next)
        if (prop->propertyName == property)
            return prop;
    return NULL;
}

RRPropertyValuePtr
RRGetOutputProperty(RROutputPtr output, Atom property, Bool pending)
{
    RRPropertyPtr prop = RRQueryOutputProperty(output, property);
    rrScrPrivPtr pScrPriv = rrGetScrPriv(output->pScreen);

    if (!prop)
        return NULL;
    if (pending && prop->is_pending)
        return &prop->pending;
    else {
#if RANDR_13_INTERFACE
        /* If we can, try to update the property value first */
        if (pScrPriv->rrOutputGetProperty)
            pScrPriv->rrOutputGetProperty(output->pScreen, output,
                                          prop->propertyName);
#endif
        return &prop->current;
    }
}

int
RRConfigureOutputProperty(RROutputPtr output, Atom property,
                          Bool pending, Bool range, Bool immutable,
                          int num_values, const INT32 *values)
{
    RRPropertyPtr prop = RRQueryOutputProperty(output, property);
    Bool add = FALSE;

    if (!prop) {
        prop = RRCreateOutputProperty(property);
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
        if (add)
            RRDestroyOutputProperty(prop);
        return BadMatch;
    }

    INT32 *new_values = NULL;

    if (num_values) {
        new_values = calloc(num_values, sizeof(INT32));
        if (!new_values) {
            if (add)
                RRDestroyOutputProperty(prop);
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
        RRInitOutputPropertyValue(&prop->pending);
    }

    prop->is_pending = pending;
    prop->range = range;
    prop->immutable = immutable;
    prop->num_valid = num_values;
    free(prop->valid_values);
    prop->valid_values = new_values;

    if (add) {
        prop->next = output->properties;
        output->properties = prop;
    }

    return Success;
}

int
ProcRRListOutputProperties(ClientPtr client)
{
    REQUEST(xRRListOutputPropertiesReq);
    REQUEST_SIZE_MATCH(xRRListOutputPropertiesReq);

    if (client->swapped)
        swapl(&stuff->output);

    RROutputPtr output;
    VERIFY_RR_OUTPUT(stuff->output, output, DixReadAccess);

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    size_t numProps = 0;
    for (RRPropertyPtr prop = output->properties; prop; prop = prop->next) {
        numProps++;
        x_rpcbuf_write_CARD32(&rpcbuf, prop->propertyName);
    }

    xRRListOutputPropertiesReply rep = {
        .nAtoms = numProps
    };

    if (client->swapped) {
        swaps(&rep.nAtoms);
    }

    return X_SEND_REPLY_WITH_RPCBUF(client, rep, rpcbuf);
}

int
ProcRRQueryOutputProperty(ClientPtr client)
{
    REQUEST(xRRQueryOutputPropertyReq);
    REQUEST_SIZE_MATCH(xRRQueryOutputPropertyReq);

    if (client->swapped) {
        swapl(&stuff->output);
        swapl(&stuff->property);
    }

    RROutputPtr output;
    RRPropertyPtr prop;

    VERIFY_RR_OUTPUT(stuff->output, output, DixReadAccess);

    prop = RRQueryOutputProperty(output, stuff->property);
    if (!prop)
        return BadName;

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };
    x_rpcbuf_write_CARD32s(&rpcbuf, (CARD32*)prop->valid_values, prop->num_valid);

    xRRQueryOutputPropertyReply rep = {
        .pending = prop->is_pending,
        .range = prop->range,
        .immutable = prop->immutable
    };

    return X_SEND_REPLY_WITH_RPCBUF(client, rep, rpcbuf);
}

int
ProcRRConfigureOutputProperty(ClientPtr client)
{
    REQUEST(xRRConfigureOutputPropertyReq);
    REQUEST_AT_LEAST_SIZE(xRRConfigureOutputPropertyReq);

    if (client->swapped) {
        swapl(&stuff->output);
        swapl(&stuff->property);
        SwapRestL(stuff);
    }

    RROutputPtr output;
    int num_valid;

    VERIFY_RR_OUTPUT(stuff->output, output, DixReadAccess);

    if (RROutputIsLeased(output))
        return BadAccess;

    num_valid =
        client->req_len - bytes_to_int32(sizeof(xRRConfigureOutputPropertyReq));
    return RRConfigureOutputProperty(output, stuff->property, stuff->pending,
                                     stuff->range, FALSE, num_valid,
                                     (INT32 *) (stuff + 1));
}

int
ProcRRChangeOutputProperty(ClientPtr client)
{
    REQUEST(xRRChangeOutputPropertyReq);
    REQUEST_AT_LEAST_SIZE(xRRChangeOutputPropertyReq);

    if (client->swapped) {
        swapl(&stuff->output);
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
            default:
                client->errorValue = stuff->format;
                return BadValue;
        }
    }

    RROutputPtr output;
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
    REQUEST_FIXED_SIZE(xRRChangeOutputPropertyReq, totalSize);

    VERIFY_RR_OUTPUT(stuff->output, output, DixReadAccess);

    if (!ValidAtom(stuff->property)) {
        client->errorValue = stuff->property;
        return BadAtom;
    }
    if (!ValidAtom(stuff->type)) {
        client->errorValue = stuff->type;
        return BadAtom;
    }

    err = RRChangeOutputProperty(output, stuff->property,
                                 stuff->type, (int) format,
                                 (int) mode, len, (void *) &stuff[1], TRUE,
                                 TRUE);
    if (err != Success)
        return err;
    else
        return Success;
}

int
ProcRRDeleteOutputProperty(ClientPtr client)
{
    REQUEST(xRRDeleteOutputPropertyReq);
    REQUEST_SIZE_MATCH(xRRDeleteOutputPropertyReq);

    if (client->swapped) {
        swapl(&stuff->output);
        swapl(&stuff->property);
    }

    RROutputPtr output;
    RRPropertyPtr prop;

    UpdateCurrentTime();
    VERIFY_RR_OUTPUT(stuff->output, output, DixReadAccess);

    if (RROutputIsLeased(output))
        return BadAccess;

    if (!ValidAtom(stuff->property)) {
        client->errorValue = stuff->property;
        return BadAtom;
    }

    prop = RRQueryOutputProperty(output, stuff->property);
    if (!prop) {
        client->errorValue = stuff->property;
        return BadName;
    }

    if (prop->immutable) {
        client->errorValue = stuff->property;
        return BadAccess;
    }

    RRDeleteOutputProperty(output, stuff->property);
    return Success;
}

int
ProcRRGetOutputProperty(ClientPtr client)
{
    REQUEST(xRRGetOutputPropertyReq);
    REQUEST_SIZE_MATCH(xRRGetOutputPropertyReq);

    if (client->swapped) {
        swapl(&stuff->output);
        swapl(&stuff->property);
        swapl(&stuff->type);
        swapl(&stuff->longOffset);
        swapl(&stuff->longLength);
    }

    RRPropertyPtr prop, *prev;
    RRPropertyValuePtr prop_value;
    unsigned long n, ind;
    RROutputPtr output;

    if (stuff->delete)
        UpdateCurrentTime();
    VERIFY_RR_OUTPUT(stuff->output, output,
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

    for (prev = &output->properties; (prop = *prev); prev = &prop->next)
        if (prop->propertyName == stuff->property)
            break;

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    xRRGetOutputPropertyReply rep = { 0 };

    if (!prop)
        goto sendout;

    if (prop->immutable && stuff->delete)
        return BadAccess;

    prop_value = RRGetOutputProperty(output, stuff->property, stuff->pending);
    if (!prop_value)
        return BadAtom;

    /* If the request type and actual type don't match. Return the
       property information, but not the data. */

    if (((stuff->type != prop_value->type) && (stuff->type != AnyPropertyType))
        ) {
        rep.bytesAfter = prop_value->size;
        rep.format = prop_value->format;
        rep.propertyType = prop_value->type;
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

    size_t len = MIN(n - ind, 4 * stuff->longLength);

    rep.bytesAfter = n - (ind + len);
    rep.format = prop_value->format;
    if (prop_value->format)
        rep.nItems = len / (prop_value->format / 8);
    rep.propertyType = prop_value->type;

    if (stuff->delete && (rep.bytesAfter == 0)) {
        xRROutputPropertyNotifyEvent event = {
            .type = RREventBase + RRNotify,
            .subCode = RRNotify_OutputProperty,
            .output = output->id,
            .state = PropertyDelete,
            .atom = prop->propertyName,
            .timestamp = currentTime.milliseconds
        };
        RRDeliverPropertyEvent(output->pScreen, (xEvent *) &event);
    }

    if (len) {
        const char *src = (char*) prop_value->data + ind;
        switch (rep.format) {
        case 32:
            x_rpcbuf_write_CARD32s(&rpcbuf, (CARD32*)src, len / sizeof(CARD32));
            break;
        case 16:
            x_rpcbuf_write_CARD16s(&rpcbuf, (CARD16*)src, len / sizeof(CARD16));
            break;
        default:
            x_rpcbuf_write_binary_pad(&rpcbuf, src, len);
            break;
        }
    }

sendout:
    if (rpcbuf.error)
        return BadAlloc;

    if (client->swapped) {
        swapl(&rep.propertyType);
        swapl(&rep.bytesAfter);
        swapl(&rep.nItems);
    }

    if (prop && stuff->delete && (rep.bytesAfter == 0)) {     /* delete the Property */
        *prev = prop->next;
        RRDestroyOutputProperty(prop);
    }

    return X_SEND_REPLY_WITH_RPCBUF(client, rep, rpcbuf);
}
