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

#ifndef _DRI3PRIV_H_
#define _DRI3PRIV_H_

#include <X11/X.h>
#include "scrnintstr.h"
#include "misc.h"
#include "list.h"
#include "windowstr.h"
#include "dixstruct.h"
#include <randrstr.h>
#include "dri3.h"

extern DevPrivateKeyRec dri3_screen_private_key;

extern RESTYPE dri3_syncobj_type;

typedef struct dri3_dmabuf_format {
    uint32_t                    format;
    uint32_t                    num_modifiers;
    uint64_t                   *modifiers;
} dri3_dmabuf_format_rec, *dri3_dmabuf_format_ptr;

typedef struct dri3_screen_priv {
    ConfigNotifyProcPtr         ConfigNotify;

    Bool                        formats_cached;
    CARD32                      num_formats;
    dri3_dmabuf_format_ptr      formats;

    const dri3_screen_info_rec *info;
} dri3_screen_priv_rec, *dri3_screen_priv_ptr;

#define wrap(priv,real,mem,func) {\
    priv->mem = real->mem; \
    real->mem = func; \
}

#define unwrap(priv,real,mem) {\
    real->mem = priv->mem; \
}

#define VERIFY_DRI3_SYNCOBJ(id, ptr, a)\
    do {\
        int rc = dixLookupResourceByType((void **)&(ptr), id,\
                                         dri3_syncobj_type, client, a);\
        if (rc != Success) {\
            client->errorValue = id;\
            return rc;\
        }\
    } while (0);

static inline dri3_screen_priv_ptr
dri3_screen_priv(ScreenPtr screen)
{
    return (dri3_screen_priv_ptr)dixLookupPrivate(&(screen)->devPrivates, &dri3_screen_private_key);
}

int
proc_dri3_dispatch(ClientPtr client);

/* DDX interface */

int
dri3_open(ClientPtr client, ScreenPtr screen, RRProviderPtr provider, int *fd);

int
dri3_pixmap_from_fds(PixmapPtr *ppixmap, ScreenPtr screen,
                     CARD8 num_fds, const int *fds,
                     CARD16 width, CARD16 height,
                     const CARD32 *strides, const CARD32 *offsets,
                     CARD8 depth, CARD8 bpp, CARD64 modifier);

int
dri3_fd_from_pixmap(PixmapPtr pixmap, CARD16 *stride, CARD32 *size);

int
dri3_fds_from_pixmap(PixmapPtr pixmap, int *fds,
                     uint32_t *strides, uint32_t *offsets,
                     uint64_t *modifier);

int
dri3_get_supported_modifiers(ScreenPtr screen, DrawablePtr drawable,
                             CARD8 depth, CARD8 bpp,
                             CARD32 *num_drawable_modifiers,
                             CARD64 **drawable_modifiers,
                             CARD32 *num_screen_modifiers,
                             CARD64 **screen_modifiers);

int
dri3_import_syncobj(ClientPtr client, ScreenPtr screen, XID id, int fd);

int
dri3_send_open_reply(ClientPtr client, int fd);

uint32_t
drm_format_for_depth(uint32_t depth, uint32_t bpp);

#endif /* _DRI3PRIV_H_ */
