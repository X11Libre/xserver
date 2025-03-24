/*
 * Copyright © 2013 Keith Packard
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

#include "present_priv.h"
#include "randrstr_priv.h"
#include <protocol-versions.h>

static int
proc_present_query_version(ClientPtr client)
{
    REQUEST(xPresentQueryVersionReq);
    xPresentQueryVersionReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .majorVersion = SERVER_PRESENT_MAJOR_VERSION,
        .minorVersion = SERVER_PRESENT_MINOR_VERSION
    };

    REQUEST_SIZE_MATCH(xPresentQueryVersionReq);
    /* From presentproto:
     *
     * The client sends the highest supported version to the server
     * and the server sends the highest version it supports, but no
     * higher than the requested version.
     */

    if (rep.majorVersion > stuff->majorVersion ||
        rep.minorVersion > stuff->minorVersion) {
        rep.majorVersion = stuff->majorVersion;
        rep.minorVersion = stuff->minorVersion;
    }

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.majorVersion);
        swapl(&rep.minorVersion);
    }
    WriteToClient(client, sizeof(rep), &rep);
    return Success;
}

#define VERIFY_FENCE_OR_NONE(fence_ptr, fence_id, client, access) do {  \
        if ((fence_id) == None)                                         \
            (fence_ptr) = NULL;                                         \
        else {                                                          \
            int __rc__ = SyncVerifyFence(&fence_ptr, fence_id, client, access); \
            if (__rc__ != Success)                                      \
                return __rc__;                                          \
        }                                                               \
    } while (0)

#define VERIFY_CRTC_OR_NONE(crtc_ptr, crtc_id, client, access) do {     \
        if ((crtc_id) == None)                                          \
            (crtc_ptr) = NULL;                                          \
        else {                                                          \
            VERIFY_RR_CRTC(crtc_id, crtc_ptr, access);                  \
        }                                                               \
    } while (0)

static int
proc_present_pixmap_common(ClientPtr client,
                           Window req_window,
                           Pixmap req_pixmap,
                           CARD32 req_serial,
                           CARD32 req_valid,
                           CARD32 req_update,
                           INT16 req_x_off,
                           INT16 req_y_off,
                           CARD32 req_target_crtc,
                           XSyncFence req_wait_fence,
                           XSyncFence req_idle_fence,
#ifdef DRI3
                           struct dri3_syncobj *acquire_syncobj,
                           struct dri3_syncobj *release_syncobj,
                           CARD64 req_acquire_point,
                           CARD64 req_release_point,
#endif /* DRI3 */
                           CARD32 req_options,
                           CARD64 req_target_msc,
                           CARD64 req_divisor,
                           CARD64 req_remainder,
                           size_t base_req_size,
                           xPresentNotify *req_notifies)
{
    WindowPtr window;
    PixmapPtr pixmap;
    RegionPtr valid = NULL;
    RegionPtr update = NULL;
    RRCrtcPtr target_crtc;
    SyncFence *wait_fence;
    SyncFence *idle_fence;
    int nnotifies;
    present_notify_ptr notifies = NULL;
    int ret;

    ret = dixLookupWindow(&window, req_window, client, DixWriteAccess);
    if (ret != Success)
        return ret;
    ret = dixLookupResourceByType((void **) &pixmap, req_pixmap, X11_RESTYPE_PIXMAP, client, DixReadAccess);
    if (ret != Success)
        return ret;

    if (window->drawable.depth != pixmap->drawable.depth)
        return BadMatch;

    VERIFY_REGION_OR_NONE(valid, req_valid, client, DixReadAccess);
    VERIFY_REGION_OR_NONE(update, req_update, client, DixReadAccess);

    VERIFY_CRTC_OR_NONE(target_crtc, req_target_crtc, client, DixReadAccess);

    VERIFY_FENCE_OR_NONE(wait_fence, req_wait_fence, client, DixReadAccess);
    VERIFY_FENCE_OR_NONE(idle_fence, req_idle_fence, client, DixWriteAccess);

    if (req_options & ~(PresentAllOptions)) {
        client->errorValue = req_options;
        return BadValue;
    }

    /*
     * Check to see if remainder is sane
     */
    if (req_divisor == 0) {
        if (req_remainder != 0) {
            client->errorValue = (CARD32)req_remainder;
            return BadValue;
        }
    } else {
        if (req_remainder >= req_divisor) {
            client->errorValue = (CARD32)req_remainder;
            return BadValue;
        }
    }

    nnotifies = (client->req_len << 2) - base_req_size;
    if (nnotifies % sizeof (xPresentNotify))
        return BadLength;

    nnotifies /= sizeof (xPresentNotify);
    if (nnotifies) {
        ret = present_create_notifies(client, nnotifies, req_notifies, &notifies);
        if (ret != Success)
            return ret;
    }

    ret = present_pixmap(window, pixmap, req_serial,
                         valid, update, req_x_off, req_y_off, target_crtc,
                         wait_fence, idle_fence,
#ifdef DRI3
                         acquire_syncobj, release_syncobj,
                         req_acquire_point, req_release_point,
#endif /* DRI3 */
                         req_options, req_target_msc, req_divisor, req_remainder,
                         notifies, nnotifies);

    if (ret != Success)
        present_destroy_notifies(notifies, nnotifies);
    return ret;
}

static int
proc_present_pixmap(ClientPtr client)
{
    REQUEST(xPresentPixmapReq);
    REQUEST_AT_LEAST_SIZE(xPresentPixmapReq);
    return proc_present_pixmap_common(client, stuff->window, stuff->pixmap, stuff->serial,
                                      stuff->valid, stuff->update, stuff->x_off, stuff->y_off,
                                      stuff->target_crtc,
                                      stuff->wait_fence, stuff->idle_fence,
#ifdef DRI3
                                      None, None, 0, 0,
#endif /* DRI3 */
                                      stuff->options, stuff->target_msc,
                                      stuff->divisor, stuff->remainder,
                                      sizeof (xPresentPixmapReq),
                                      (xPresentNotify *)(stuff + 1));
}

static int
proc_present_notify_msc(ClientPtr client)
{
    REQUEST(xPresentNotifyMSCReq);
    WindowPtr   window;
    int         rc;

    REQUEST_SIZE_MATCH(xPresentNotifyMSCReq);
    rc = dixLookupWindow(&window, stuff->window, client, DixReadAccess);
    if (rc != Success)
        return rc;

    /*
     * Check to see if remainder is sane
     */
    if (stuff->divisor == 0) {
        if (stuff->remainder != 0) {
            client->errorValue = (CARD32) stuff->remainder;
            return BadValue;
        }
    } else {
        if (stuff->remainder >= stuff->divisor) {
            client->errorValue = (CARD32) stuff->remainder;
            return BadValue;
        }
    }

    return present_notify_msc(window, stuff->serial,
                              stuff->target_msc, stuff->divisor, stuff->remainder);
}

static int
proc_present_select_input (ClientPtr client)
{
    REQUEST(xPresentSelectInputReq);
    WindowPtr window;
    int rc;

    REQUEST_SIZE_MATCH(xPresentSelectInputReq);

    rc = dixLookupWindow(&window, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    if (stuff->eventMask & ~PresentAllEvents) {
        client->errorValue = stuff->eventMask;
        return BadValue;
    }
    return present_select_input(client, stuff->eid, window, stuff->eventMask);
}

static int
proc_present_query_capabilities (ClientPtr client)
{
    REQUEST(xPresentQueryCapabilitiesReq);
    xPresentQueryCapabilitiesReply rep = {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
    };
    WindowPtr   window;
    RRCrtcPtr   crtc = NULL;
    int         r;

    REQUEST_SIZE_MATCH(xPresentQueryCapabilitiesReq);
    r = dixLookupWindow(&window, stuff->target, client, DixGetAttrAccess);
    switch (r) {
    case Success:
        crtc = present_get_crtc(window);
        break;
    case BadWindow:
        VERIFY_RR_CRTC(stuff->target, crtc, DixGetAttrAccess);
        break;
    default:
        return r;
    }

    rep.capabilities = present_query_capabilities(crtc);

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.capabilities);
    }
    WriteToClient(client, sizeof(rep), &rep);
    return Success;
}

#ifdef DRI3
static int
proc_present_pixmap_synced (ClientPtr client)
{
    REQUEST(xPresentPixmapSyncedReq);
    struct dri3_syncobj *acquire_syncobj;
    struct dri3_syncobj *release_syncobj;

    REQUEST_AT_LEAST_SIZE(xPresentPixmapSyncedReq);
    VERIFY_DRI3_SYNCOBJ(stuff->acquire_syncobj, acquire_syncobj, DixWriteAccess);
    VERIFY_DRI3_SYNCOBJ(stuff->release_syncobj, release_syncobj, DixWriteAccess);

    if (stuff->acquire_point == 0 || stuff->release_point == 0 ||
        (stuff->acquire_syncobj == stuff->release_syncobj &&
         stuff->acquire_point >= stuff->release_point))
        return BadValue;

    return proc_present_pixmap_common(client, stuff->window, stuff->pixmap, stuff->serial,
                                      stuff->valid, stuff->update, stuff->x_off, stuff->y_off,
                                      stuff->target_crtc,
                                      None, None,
                                      acquire_syncobj, release_syncobj,
                                      stuff->acquire_point, stuff->release_point,
                                      stuff->options, stuff->target_msc,
                                      stuff->divisor, stuff->remainder,
                                      sizeof (xPresentPixmapSyncedReq),
                                      (xPresentNotify *)(stuff + 1));
}
#endif /* DRI3 */

int
proc_present_dispatch(ClientPtr client)
{
    REQUEST(xReq);

    switch (stuff->data) {
        case X_PresentQueryVersion:
            return proc_present_query_version(client);
        case X_PresentPixmap:
            return proc_present_pixmap(client);
        case X_PresentNotifyMSC:
            return proc_present_notify_msc(client);
        case X_PresentSelectInput:
            return proc_present_select_input(client);
        case X_PresentQueryCapabilities:
            return proc_present_query_capabilities(client);
#ifdef DRI3
        case X_PresentPixmapSynced:
            return proc_present_pixmap_synced(client);
#endif
    }

    return BadRequest;
}

static int _X_COLD
sproc_present_query_version(ClientPtr client)
{
    REQUEST(xPresentQueryVersionReq);
    REQUEST_SIZE_MATCH(xPresentQueryVersionReq);

    swapl(&stuff->majorVersion);
    swapl(&stuff->minorVersion);
    return proc_present_query_version(client);
}

static int _X_COLD
sproc_present_pixmap(ClientPtr client)
{
    REQUEST(xPresentPixmapReq);
    REQUEST_AT_LEAST_SIZE(xPresentPixmapReq);

    swapl(&stuff->window);
    swapl(&stuff->pixmap);
    swapl(&stuff->valid);
    swapl(&stuff->update);
    swaps(&stuff->x_off);
    swaps(&stuff->y_off);
    swapll(&stuff->target_msc);
    swapll(&stuff->divisor);
    swapll(&stuff->remainder);
    swapl(&stuff->idle_fence);
    return proc_present_pixmap(client);
}

static int _X_COLD
sproc_present_notify_msc(ClientPtr client)
{
    REQUEST(xPresentNotifyMSCReq);
    REQUEST_SIZE_MATCH(xPresentNotifyMSCReq);

    swapl(&stuff->window);
    swapll(&stuff->target_msc);
    swapll(&stuff->divisor);
    swapll(&stuff->remainder);
    return proc_present_notify_msc(client);
}

static int _X_COLD
sproc_present_select_input (ClientPtr client)
{
    REQUEST(xPresentSelectInputReq);
    REQUEST_SIZE_MATCH(xPresentSelectInputReq);

    swapl(&stuff->window);
    swapl(&stuff->eventMask);
    return proc_present_select_input(client);
}

static int _X_COLD
sproc_present_query_capabilities (ClientPtr client)
{
    REQUEST(xPresentQueryCapabilitiesReq);
    REQUEST_SIZE_MATCH(xPresentQueryCapabilitiesReq);
    swapl(&stuff->target);
    return proc_present_query_capabilities(client);
}


#ifdef DRI3
static int _X_COLD
sproc_present_pixmap_synced(ClientPtr client)
{
    REQUEST(xPresentPixmapSyncedReq);
    REQUEST_AT_LEAST_SIZE(xPresentPixmapSyncedReq);

    swapl(&stuff->window);

    swapl(&stuff->pixmap);
    swapl(&stuff->serial);

    swapl(&stuff->valid);
    swapl(&stuff->update);

    swaps(&stuff->x_off);
    swaps(&stuff->y_off);
    swapl(&stuff->target_crtc);

    swapl(&stuff->acquire_syncobj);
    swapl(&stuff->release_syncobj);
    swapll(&stuff->acquire_point);
    swapll(&stuff->release_point);

    swapl(&stuff->options);

    swapll(&stuff->target_msc);
    swapll(&stuff->divisor);
    swapll(&stuff->remainder);
    return proc_present_pixmap_synced(client);
}
#endif /* DRI3 */

int _X_COLD
sproc_present_dispatch(ClientPtr client)
{
    REQUEST(xReq);

    switch (stuff->data) {
        case X_PresentQueryVersion:
            return sproc_present_query_version(client);
        case X_PresentPixmap:
            return sproc_present_pixmap(client);
        case X_PresentNotifyMSC:
            return sproc_present_notify_msc(client);
        case X_PresentSelectInput:
            return sproc_present_select_input(client);
        case X_PresentQueryCapabilities:
            return sproc_present_query_capabilities(client);
#ifdef DRI3
        case X_PresentPixmapSynced:
            return sproc_present_pixmap_synced(client);
#endif
    }

    return BadRequest;
}
