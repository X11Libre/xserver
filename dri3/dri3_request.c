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

#include <unistd.h>

#include "dix/dix_priv.h"
#include "dix/request_priv.h"
#include "dix/screenint_priv.h"
#include "os/client_priv.h"

#include "dri3_priv.h"
#include <syncsrv.h>
#include <xace.h>
#include "../Xext/syncsdk.h"
#include <protocol-versions.h>
#include <drm_fourcc.h>
#include "randrstr_priv.h"
#include "dixstruct_priv.h"

static Bool
dri3_screen_can_one_point_four(ScreenPtr screen)
{
    dri3_screen_priv_ptr dri3 = dri3_screen_priv(screen);

    return dri3 &&
        dri3->info &&
        dri3->info->version >= 4 &&
        dri3->info->import_syncobj;
}

static Bool
dri3_screen_can_one_point_two(ScreenPtr screen)
{
    dri3_screen_priv_ptr dri3 = dri3_screen_priv(screen);

    if (dri3 && dri3->info && dri3->info->version >= 2 &&
        dri3->info->pixmap_from_fds && dri3->info->fds_from_pixmap &&
        dri3->info->get_formats && dri3->info->get_modifiers &&
        dri3->info->get_drawable_modifiers)
        return TRUE;

    return FALSE;
}

static int
proc_dri3_query_version(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDRI3QueryVersionReq);
    X_REQUEST_FIELD_CARD32(majorVersion);
    X_REQUEST_FIELD_CARD32(minorVersion);

    xDRI3QueryVersionReply reply = {
        .majorVersion = SERVER_DRI3_MAJOR_VERSION,
        .minorVersion = SERVER_DRI3_MINOR_VERSION
    };

    DIX_FOR_EACH_SCREEN({
        if (!dri3_screen_can_one_point_two(walkScreen)) {
            reply.minorVersion = 0;
            break;
        }
        if (!dri3_screen_can_one_point_four(walkScreen)) {
            reply.minorVersion = 2;
            break;
        }
    });

    DIX_FOR_EACH_GPU_SCREEN({
        if (!dri3_screen_can_one_point_two(walkScreen)) {
            reply.minorVersion = 0;
            break;
        }
        if (!dri3_screen_can_one_point_four(walkScreen)) {
            reply.minorVersion = 2;
            break;
        }
    });

    /* From DRI3 proto:
     *
     * The client sends the highest supported version to the server
     * and the server sends the highest version it supports, but no
     * higher than the requested version.
     */

    if (reply.majorVersion > stuff->majorVersion ||
        (reply.majorVersion == stuff->majorVersion &&
         reply.minorVersion > stuff->minorVersion)) {
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
dri3_send_open_reply(ClientPtr client, int fd)
{
    xDRI3OpenReply reply = {
        .nfd = 1,
    };

    if (WriteFdToClient(client, fd, TRUE) < 0) {
        close(fd);
        return BadAlloc;
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
proc_dri3_open(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDRI3OpenReq);
    X_REQUEST_FIELD_CARD32(drawable);
    X_REQUEST_FIELD_CARD32(provider);

    RRProviderPtr provider;
    DrawablePtr drawable;
    ScreenPtr screen;
    int fd;
    int status;

    status = dixLookupDrawable(&drawable, stuff->drawable, client, 0, DixGetAttrAccess);
    if (status != Success)
        return status;

    if (stuff->provider == None)
        provider = NULL;
    else if (!RRProviderType) {
        return BadMatch;
    } else {
        VERIFY_RR_PROVIDER(stuff->provider, provider, DixReadAccess);
        if (drawable->pScreen != provider->pScreen)
            return BadMatch;
    }
    screen = drawable->pScreen;

    status = dri3_open(client, screen, provider, &fd);
    if (status != Success)
        return status;

    if (client->ignoreCount == 0)
        return dri3_send_open_reply(client, fd);

    return Success;
}

static int
proc_dri3_pixmap_from_buffer(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDRI3PixmapFromBufferReq);
    X_REQUEST_FIELD_CARD32(pixmap);
    X_REQUEST_FIELD_CARD32(drawable);
    X_REQUEST_FIELD_CARD32(size);
    X_REQUEST_FIELD_CARD16(width);
    X_REQUEST_FIELD_CARD16(height);
    X_REQUEST_FIELD_CARD16(stride);

    int fd;
    DrawablePtr drawable;
    PixmapPtr pixmap;
    CARD32 stride, offset;
    int rc;

    SetReqFds(client, 1);
    LEGAL_NEW_RESOURCE(stuff->pixmap, client);
    rc = dixLookupDrawable(&drawable, stuff->drawable, client, M_ANY, DixGetAttrAccess);
    if (rc != Success) {
        client->errorValue = stuff->drawable;
        return rc;
    }

    if (!stuff->width || !stuff->height) {
        client->errorValue = 0;
        return BadValue;
    }

    if (stuff->width > 32767 || stuff->height > 32767)
        return BadAlloc;

    if (stuff->depth != 1) {
        DepthPtr depth = drawable->pScreen->allowedDepths;
        int i;
        for (i = 0; i < drawable->pScreen->numDepths; i++, depth++)
            if (depth->depth == stuff->depth)
                break;
        if (i == drawable->pScreen->numDepths) {
            client->errorValue = stuff->depth;
            return BadValue;
        }
    }

    fd = ReadFdFromClient(client);
    if (fd < 0)
        return BadValue;

    offset = 0;
    stride = stuff->stride;
    rc = dri3_pixmap_from_fds(&pixmap,
                              drawable->pScreen, 1, &fd,
                              stuff->width, stuff->height,
                              &stride, &offset,
                              stuff->depth, stuff->bpp,
                              DRM_FORMAT_MOD_INVALID);
    close (fd);
    if (rc != Success)
        return rc;

    pixmap->drawable.id = stuff->pixmap;

    /* security creation/labeling check */
    rc = XaceHookResourceAccess(client, stuff->pixmap, X11_RESTYPE_PIXMAP,
                  pixmap, X11_RESTYPE_NONE, NULL, DixCreateAccess);

    if (rc != Success) {
        dixDestroyPixmap(pixmap, 0);
        return rc;
    }
    if (!AddResource(stuff->pixmap, X11_RESTYPE_PIXMAP, (void *) pixmap))
        return BadAlloc;

    return Success;
}

static int
proc_dri3_buffer_from_pixmap(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDRI3BufferFromPixmapReq);
    X_REQUEST_FIELD_CARD32(pixmap);

    int rc;
    int fd;
    PixmapPtr pixmap;

    rc = dixLookupResourceByType((void **) &pixmap, stuff->pixmap, X11_RESTYPE_PIXMAP,
                                 client, DixWriteAccess);
    if (rc != Success) {
        client->errorValue = stuff->pixmap;
        return rc;
    }

    xDRI3BufferFromPixmapReply reply = {
        .nfd = 1,
        .width = pixmap->drawable.width,
        .height = pixmap->drawable.height,
        .depth = pixmap->drawable.depth,
        .bpp = pixmap->drawable.bitsPerPixel,
    };

    fd = dri3_fd_from_pixmap(pixmap, &reply.stride, &reply.size);
    if (fd < 0)
        return BadPixmap;

    if (client->swapped) {
        swapl(&reply.size);
        swaps(&reply.width);
        swaps(&reply.height);
        swaps(&reply.stride);
    }
    if (WriteFdToClient(client, fd, TRUE) < 0) {
        close(fd);
        return BadAlloc;
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
proc_dri3_fence_from_fd(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDRI3FenceFromFDReq);
    X_REQUEST_FIELD_CARD32(drawable);
    X_REQUEST_FIELD_CARD32(fence);

    DrawablePtr drawable;
    int fd;
    int status;

    SetReqFds(client, 1);
    LEGAL_NEW_RESOURCE(stuff->fence, client);

    status = dixLookupDrawable(&drawable, stuff->drawable, client, M_ANY, DixGetAttrAccess);
    if (status != Success)
        return status;

    fd = ReadFdFromClient(client);
    if (fd < 0)
        return BadValue;

    status = SyncCreateFenceFromFD(client, drawable, stuff->fence,
                                   fd, stuff->initially_triggered);

    return status;
}

static int
proc_dri3_fd_from_fence(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDRI3FDFromFenceReq);
    X_REQUEST_FIELD_CARD32(drawable);
    X_REQUEST_FIELD_CARD32(fence);

    xDRI3FDFromFenceReply reply = {
        .nfd = 1,
    };
    DrawablePtr drawable;
    int fd;
    int status;
    SyncFence *fence;

    status = dixLookupDrawable(&drawable, stuff->drawable, client, M_ANY, DixGetAttrAccess);
    if (status != Success)
        return status;
    status = SyncVerifyFence(&fence, stuff->fence, client, DixWriteAccess);
    if (status != Success)
        return status;

    fd = SyncFDFromFence(client, drawable, fence);
    if (fd < 0)
        return BadMatch;

    if (WriteFdToClient(client, fd, FALSE) < 0)
        return BadAlloc;

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
proc_dri3_get_supported_modifiers(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDRI3GetSupportedModifiersReq);
    X_REQUEST_FIELD_CARD32(window);

    WindowPtr window;
    ScreenPtr pScreen;
    CARD64 *window_modifiers = NULL;
    CARD64 *screen_modifiers = NULL;
    CARD32 nwindowmodifiers = 0;
    CARD32 nscreenmodifiers = 0;
    int status;

    status = dixLookupWindow(&window, stuff->window, client, DixGetAttrAccess);
    if (status != Success)
        return status;
    pScreen = window->drawable.pScreen;

    dri3_get_supported_modifiers(pScreen, &window->drawable,
                                 stuff->depth, stuff->bpp,
                                 &nwindowmodifiers, &window_modifiers,
                                 &nscreenmodifiers, &screen_modifiers);

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };
    x_rpcbuf_write_CARD64s(&rpcbuf, window_modifiers, nwindowmodifiers);
    x_rpcbuf_write_CARD64s(&rpcbuf, screen_modifiers, nscreenmodifiers);

    free(window_modifiers);
    free(screen_modifiers);

    xDRI3GetSupportedModifiersReply reply = {
        .numWindowModifiers = nwindowmodifiers,
        .numScreenModifiers = nscreenmodifiers,
    };

    if (client->swapped) {
        swapl(&reply.numWindowModifiers);
        swapl(&reply.numScreenModifiers);
    }

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

static int
proc_dri3_pixmap_from_buffers(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDRI3PixmapFromBuffersReq);
    X_REQUEST_FIELD_CARD32(pixmap);
    X_REQUEST_FIELD_CARD32(window);
    X_REQUEST_FIELD_CARD16(width);
    X_REQUEST_FIELD_CARD16(height);
    X_REQUEST_FIELD_CARD32(stride0);
    X_REQUEST_FIELD_CARD32(offset0);
    X_REQUEST_FIELD_CARD32(stride1);
    X_REQUEST_FIELD_CARD32(offset1);
    X_REQUEST_FIELD_CARD32(stride2);
    X_REQUEST_FIELD_CARD32(offset2);
    X_REQUEST_FIELD_CARD32(stride3);
    X_REQUEST_FIELD_CARD32(offset3);
    X_REQUEST_FIELD_CARD64(modifier);

    int fds[4];
    CARD32 strides[4], offsets[4];
    ScreenPtr screen;
    WindowPtr window;
    PixmapPtr pixmap;
    int rc;
    int i;

    SetReqFds(client, stuff->num_buffers);
    LEGAL_NEW_RESOURCE(stuff->pixmap, client);
    rc = dixLookupWindow(&window, stuff->window, client, DixGetAttrAccess);
    if (rc != Success) {
        client->errorValue = stuff->window;
        return rc;
    }
    screen = window->drawable.pScreen;

    if (!stuff->width || !stuff->height || !stuff->bpp || !stuff->depth) {
        client->errorValue = 0;
        return BadValue;
    }

    if (stuff->width > 32767 || stuff->height > 32767)
        return BadAlloc;

    if (stuff->depth != 1) {
        DepthPtr depth = screen->allowedDepths;
        int j;
        for (j = 0; j < screen->numDepths; j++, depth++)
            if (depth->depth == stuff->depth)
                break;
        if (j == screen->numDepths) {
            client->errorValue = stuff->depth;
            return BadValue;
        }
    }

    if (!stuff->num_buffers || stuff->num_buffers > 4) {
        client->errorValue = stuff->num_buffers;
        return BadValue;
    }

    for (i = 0; i < stuff->num_buffers; i++) {
        fds[i] = ReadFdFromClient(client);
        if (fds[i] < 0) {
            while (--i >= 0)
                close(fds[i]);
            return BadValue;
        }
    }

    strides[0] = stuff->stride0;
    strides[1] = stuff->stride1;
    strides[2] = stuff->stride2;
    strides[3] = stuff->stride3;
    offsets[0] = stuff->offset0;
    offsets[1] = stuff->offset1;
    offsets[2] = stuff->offset2;
    offsets[3] = stuff->offset3;

    rc = dri3_pixmap_from_fds(&pixmap, screen,
                              stuff->num_buffers, fds,
                              stuff->width, stuff->height,
                              strides, offsets,
                              stuff->depth, stuff->bpp,
                              stuff->modifier);

    for (i = 0; i < stuff->num_buffers; i++)
        close (fds[i]);

    if (rc != Success)
        return rc;

    pixmap->drawable.id = stuff->pixmap;

    /* security creation/labeling check */
    rc = XaceHookResourceAccess(client, stuff->pixmap, X11_RESTYPE_PIXMAP,
                  pixmap, X11_RESTYPE_NONE, NULL, DixCreateAccess);

    if (rc != Success) {
        dixDestroyPixmap(pixmap, 0);
        return rc;
    }
    if (!AddResource(stuff->pixmap, X11_RESTYPE_PIXMAP, (void *) pixmap))
        return BadAlloc;

    return Success;
}

static int
proc_dri3_buffers_from_pixmap(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDRI3BuffersFromPixmapReq);
    X_REQUEST_FIELD_CARD32(pixmap);

    int rc;
    int fds[4];
    int num_fds;
    uint32_t strides[4], offsets[4];
    uint64_t modifier;
    int i;
    PixmapPtr pixmap;

    rc = dixLookupResourceByType((void **) &pixmap, stuff->pixmap, X11_RESTYPE_PIXMAP,
                                 client, DixWriteAccess);
    if (rc != Success) {
        client->errorValue = stuff->pixmap;
        return rc;
    }

    num_fds = dri3_fds_from_pixmap(pixmap, fds, strides, offsets, &modifier);
    if (num_fds == 0)
        return BadPixmap;

    for (i = 0; i < num_fds; i++) {
        if (WriteFdToClient(client, fds[i], TRUE) < 0) {
            while (i--)
                close(fds[i]);
            return BadAlloc;
        }
    }

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };
    x_rpcbuf_write_CARD32s(&rpcbuf, (CARD32*)strides, num_fds);
    x_rpcbuf_write_CARD32s(&rpcbuf, (CARD32*)offsets, num_fds);

    xDRI3BuffersFromPixmapReply reply = {
        .nfd = num_fds,
        .width = pixmap->drawable.width,
        .height = pixmap->drawable.height,
        .depth = pixmap->drawable.depth,
        .bpp = pixmap->drawable.bitsPerPixel,
        .modifier = modifier,
    };

    if (client->swapped) {
        swaps(&reply.width);
        swaps(&reply.height);
        swapll(&reply.modifier);
    }

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

static int
proc_dri3_set_drm_device_in_use(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDRI3SetDRMDeviceInUseReq);
    X_REQUEST_FIELD_CARD32(window);
    X_REQUEST_FIELD_CARD32(drmMajor);
    X_REQUEST_FIELD_CARD32(drmMinor);

    WindowPtr window;
    int status;

    status = dixLookupWindow(&window, stuff->window, client,
                             DixGetAttrAccess);
    if (status != Success)
        return status;

    /* TODO Eventually we should use this information to have
     * DRI3GetSupportedModifiers return device-specific modifiers, but for now
     * we will ignore it until multi-device support is more complete.
     * Otherwise we can't advertise support for DRI3 1.4.
     */
    return Success;
}

static int
proc_dri3_import_syncobj(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDRI3ImportSyncobjReq);
    X_REQUEST_FIELD_CARD32(syncobj);
    X_REQUEST_FIELD_CARD32(drawable);

    DrawablePtr drawable;
    ScreenPtr screen;
    int fd;
    int status;

    SetReqFds(client, 1);
    LEGAL_NEW_RESOURCE(stuff->syncobj, client);

    status = dixLookupDrawable(&drawable, stuff->drawable, client,
                               M_ANY, DixGetAttrAccess);
    if (status != Success)
        return status;

    screen = drawable->pScreen;

    fd = ReadFdFromClient(client);
    if (fd < 0)
        return BadValue;

    return dri3_import_syncobj(client, screen, stuff->syncobj, fd);
}

static int
proc_dri3_free_syncobj(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xDRI3FreeSyncobjReq);
    X_REQUEST_FIELD_CARD32(syncobj);

    struct dri3_syncobj *syncobj;
    int status;

    status = dixLookupResourceByType((void **) &syncobj, stuff->syncobj,
                                     dri3_syncobj_type, client, DixWriteAccess);
    if (status != Success)
        return status;

    FreeResource(stuff->syncobj, RT_NONE);
    return Success;
}

int
proc_dri3_dispatch(ClientPtr client)
{
    REQUEST(xReq);
    if (!client->local)
        return BadMatch;

    switch (stuff->data) {
        case X_DRI3QueryVersion:
            return proc_dri3_query_version(client);
        case X_DRI3Open:
            return proc_dri3_open(client);
        case X_DRI3PixmapFromBuffer:
            return proc_dri3_pixmap_from_buffer(client);
        case X_DRI3BufferFromPixmap:
            return proc_dri3_buffer_from_pixmap(client);
        case X_DRI3FenceFromFD:
            return proc_dri3_fence_from_fd(client);
        case X_DRI3FDFromFence:
            return proc_dri3_fd_from_fence(client);

        /* v1.2 */
        case xDRI3GetSupportedModifiers:
            return proc_dri3_get_supported_modifiers(client);
        case xDRI3PixmapFromBuffers:
            return proc_dri3_pixmap_from_buffers(client);
        case xDRI3BuffersFromPixmap:
            return proc_dri3_buffers_from_pixmap(client);

        /* v1.3 */
        case xDRI3SetDRMDeviceInUse:
            return proc_dri3_set_drm_device_in_use(client);

        /* v1.4 */
        case xDRI3ImportSyncobj:
            return proc_dri3_import_syncobj(client);
        case xDRI3FreeSyncobj:
            return proc_dri3_free_syncobj(client);
        default:
            return BadRequest;
    }
}
