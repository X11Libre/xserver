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

#include "dix/screen_hooks_priv.h"
#include "dix/screenint_priv.h"
#include "miext/extinit_priv.h"

#include "dri3_priv.h"
#include <drm_fourcc.h>

static int dri3_request;
DevPrivateKeyRec dri3_screen_private_key;

static x_server_generation_t dri3_screen_generation;

static void dri3_screen_close(CallbackListPtr *pcbl, ScreenPtr screen, void *unused)
{
    dri3_screen_priv_ptr screen_priv = dri3_screen_priv(screen);
    dixScreenUnhookClose(screen, dri3_screen_close);
    free(screen_priv->info);
    free(screen_priv);
}

Bool
dri3_screen_init(ScreenPtr screen, const dri3_screen_info_rec *info)
{
    dri3_screen_priv_ptr screen_priv;
    dri3_screen_generation = serverGeneration;

    if (!dixRegisterPrivateKey(&dri3_screen_private_key, PRIVATE_SCREEN, 0))
        return FALSE;

    screen_priv = dri3_screen_priv(screen);

    if (!screen_priv) {
        screen_priv = calloc(1, sizeof (dri3_screen_priv_rec));
        if (!screen_priv)
            return FALSE;

        dixScreenHookClose(screen, dri3_screen_close);

        dixSetPrivate(&screen->devPrivates, &dri3_screen_private_key, screen_priv);
    }

    if (info) {
        void *ptr = realloc(screen_priv->info, sizeof(*info));
        if (!ptr) {
            return FALSE;
        }
        screen_priv->info = ptr;
        *screen_priv->info = *info;
    }

    return TRUE;
}

RESTYPE dri3_syncobj_type;

static int dri3_syncobj_free(void *data, XID id)
{
    struct dri3_syncobj *syncobj = data;
    if (--syncobj->refcount == 0)
        syncobj->free(syncobj);
    return 0;
}

void
