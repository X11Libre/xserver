/*
 * Copyright © 2014 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of the
 * copyright holders not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#ifndef XWAYLAND_PIXMAP_H
#define XWAYLAND_PIXMAP_H

#include <xwayland-config.h>
#include <wayland-client.h>

#include "dix/resource_priv.h"

#include "pixmapstr.h"

/* This is an opaque structure implemented in the different backends */
struct xwl_pixmap;

typedef void (*xwl_buffer_release_cb) (void *data);

void xwl_pixmap_set_private(PixmapPtr pixmap, struct xwl_pixmap *xwl_pixmap);
struct xwl_pixmap *xwl_pixmap_get(PixmapPtr pixmap);
struct wl_buffer *xwl_pixmap_get_wl_buffer(PixmapPtr pixmap);
Bool xwl_pixmap_set_buffer_release_cb(PixmapPtr pixmap,
                                      xwl_buffer_release_cb func, void *data);
void xwl_pixmap_del_buffer_release_cb(PixmapPtr pixmap);
void xwl_pixmap_buffer_release_cb(void *data, struct wl_buffer *wl_buffer);
Bool xwl_pixmap_init(void);

static inline Bool
xwl_is_client_pixmap(PixmapPtr pixmap)
{
    return (!dixResouceIsServerOwned(pixmap->drawable.id));
}

#endif /* XWAYLAND_PIXMAP_H */
