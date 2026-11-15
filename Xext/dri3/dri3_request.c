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
#include "include/syncsdk.h"
#include "os/client_priv.h"
#include "Xext/randr/randrstr_priv.h"

#include "dri3_priv.h"
#include "Xext/sync/syncsrv.h"
#include <xace.h>
#include <protocol-versions.h>
#include <drm_fourcc.h>
#include "dixstruct_priv.h"

static CARD32
dri3_screen_minor_version(ScreenPtr screen)
{
    dri3_screen_priv_ptr dri3 = dri3_screen_priv(screen);

    return dri3_info_minor_version(dri3 ? dri3->info : NULL);
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

    /* the version every screen and GPU screen supports */
    DIX_FOR_EACH_SCREEN({
        reply.minorVersion = min(reply.minorVersion,
                                 dri3_screen_minor_version(walkScreen));
    });

    DIX_FOR_EACH_GPU_SCREEN({
        reply.minorVersion = min(reply.minorVersion,
                                 dri3_screen_minor_version(walkScreen));
    });

    /* From DRI3 proto:
     * minorVersion of 0 means v1.0 which supports only pixmap_from_fds and
     * fd_from_pixmap.  minorVersion of 1 means v1.1 which adds fence support.
     * minorVersion of 2 means v1.2 which adds multiple planes and modifiers.
     * minorVersion of 3 means v1.3 which adds SetDRMDeviceInUse.
     * minorVersion of 4 means v1.4 which adds ImportSyncobj.
     */

    if (client->swapped) {
        swaps(&reply.majorVersion);
        swaps(&reply.minorVersion);
    }
    return X_SEND_REPLY_SIMPLE(client, reply);
}
