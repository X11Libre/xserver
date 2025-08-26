/*
 * Copyright © 2014 Keith Packard
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

#include <assert.h>

#include "os/bug_priv.h"

#include "glamor_priv.h"
#include "glamor_transfer.h"

/*
 * Write a region of bits into a drawable's backing pixmap
 */
void
glamor_upload_boxes(DrawablePtr drawable, BoxPtr in_boxes, int in_nbox,
                    int dx_src, int dy_src,
                    int dx_dst, int dy_dst,
                    uint8_t *bits, uint32_t byte_stride)
{
    ScreenPtr                   screen = drawable->pScreen;
    glamor_screen_private       *glamor_priv = glamor_get_screen_private(screen);
    PixmapPtr                   pixmap = glamor_get_drawable_pixmap(drawable);
    glamor_pixmap_private       *priv = glamor_get_pixmap_private(pixmap);
    int                         box_index;
    const struct glamor_format *f = glamor_format_for_pixmap(pixmap);
    int                         bytes_per_pixel = PIXMAN_FORMAT_BPP(f->render_format) >> 3;
    char *tmp_bits = NULL;

    if (glamor_drawable_effective_depth(drawable) == 24 && pixmap->drawable.depth == 32)
        tmp_bits = XNFalloc(byte_stride * pixmap->drawable.height);

    glamor_make_current(glamor_priv);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    if (glamor_priv->has_unpack_subimage)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, byte_stride / bytes_per_pixel);

    BUG_RETURN(!priv);

    glamor_pixmap_loop(priv, box_index) {
        BoxPtr                  box = glamor_pixmap_box_at(priv, box_index);
        glamor_pixmap_fbo       *fbo = glamor_pixmap_fbo_at(priv, box_index);
        BoxPtr                  boxes = in_boxes;
        int                     nbox = in_nbox;

        glamor_bind_texture(glamor_priv, GL_TEXTURE0, fbo, TRUE);

        while (nbox--) {

            /* compute drawable coordinates */
            int x1 = MAX(boxes->x1 + dx_dst, box->x1);
            int x2 = MIN(boxes->x2 + dx_dst, box->x2);
            int y1 = MAX(boxes->y1 + dy_dst, box->y1);
            int y2 = MIN(boxes->y2 + dy_dst, box->y2);

            uint32_t *src_line;
            size_t ofs = (y1 - dy_dst + dy_src) * byte_stride;
            ofs += (x1 - dx_dst + dx_src) * bytes_per_pixel;

            boxes++;

            if (x2 <= x1 || y2 <= y1)
                continue;

            src_line = (uint32_t *)(bits + ofs);

            if (tmp_bits) {
                uint32_t *tmp_line = (uint32_t *)(tmp_bits + ofs);
                int x, y;

                /* Make sure any sampling of the alpha channel will return 1.0 */
                for (y = y1; y < y2;
                     y++, src_line += byte_stride / 4, tmp_line += byte_stride / 4) {
                    for (x = 0; x < x2 - x1; x++)
                        tmp_line[x] = src_line[x] | 0xff000000;
                }

                src_line = (uint32_t *)(tmp_bits + ofs);
            }

            if (glamor_priv->has_unpack_subimage ||
                x2 - x1 == byte_stride / bytes_per_pixel) {
                glTexSubImage2D(GL_TEXTURE_2D, 0,
                                x1 - box->x1, y1 - box->y1,
                                x2 - x1, y2 - y1,
                                f->format, f->type,
                                src_line);
            } else {
                for (; y1 < y2; y1++, src_line += byte_stride / bytes_per_pixel)
                    glTexSubImage2D(GL_TEXTURE_2D, 0,
                                    x1 - box->x1, y1 - box->y1,
                                    x2 - x1, 1,
                                    f->format, f->type,
                                    src_line);
            }
        }
    }

    free(tmp_bits);

    if (glamor_priv->has_unpack_subimage)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

/*
 * Upload a region of data
 */

void
glamor_upload_region(DrawablePtr drawable, RegionPtr region,
                     int region_x, int region_y,
                     uint8_t *bits, uint32_t byte_stride)
{
    glamor_upload_boxes(drawable, RegionRects(region), RegionNumRects(region),
                        -region_x, -region_y,
                        0, 0,
                        bits, byte_stride);
}

/*
 * Read stuff from the drawable's backing pixmap FBOs and write to memory
 */
void
glamor_download_boxes(DrawablePtr drawable, BoxPtr in_boxes, int in_nbox,
                      int dx_src, int dy_src,
                      int dx_dst, int dy_dst,
                      uint8_t *bits, uint32_t byte_stride)
{
    ScreenPtr screen = drawable->pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
    glamor_pixmap_private *priv = glamor_get_pixmap_private(pixmap);
    int box_index;
    const struct glamor_format *f = glamor_format_for_pixmap(pixmap);
    int bytes_per_pixel = PIXMAN_FORMAT_BPP(f->render_format) >> 3;

    glamor_make_current(glamor_priv);

    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    if (glamor_priv->has_pack_subimage)
        glPixelStorei(GL_PACK_ROW_LENGTH, byte_stride / bytes_per_pixel);

    BUG_RETURN(!priv);

    glamor_pixmap_loop(priv, box_index) {
        BoxPtr                  box = glamor_pixmap_box_at(priv, box_index);
        glamor_pixmap_fbo       *fbo = glamor_pixmap_fbo_at(priv, box_index);
        BoxPtr                  boxes = in_boxes;
        int                     nbox = in_nbox;

        /* This should not be called on GLAMOR_FBO_NO_FBO-allocated pixmaps. */
        assert(fbo->fb);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo->fb);

        while (nbox--) {

            /* compute drawable coordinates */
            int                     x1 = MAX(boxes->x1 + dx_src, box->x1);
            int                     x2 = MIN(boxes->x2 + dx_src, box->x2);
            int                     y1 = MAX(boxes->y1 + dy_src, box->y1);
            int                     y2 = MIN(boxes->y2 + dy_src, box->y2);
            size_t ofs = (y1 - dy_src + dy_dst) * byte_stride;
            ofs += (x1 - dx_src + dx_dst) * bytes_per_pixel;

            boxes++;

            if (x2 <= x1 || y2 <= y1)
                continue;

            if (glamor_priv->has_pack_subimage ||
                x2 - x1 == byte_stride / bytes_per_pixel) {
                glReadPixels(x1 - box->x1, y1 - box->y1, x2 - x1, y2 - y1, f->format, f->type, bits + ofs);
            } else {
                for (; y1 < y2; y1++, ofs += byte_stride)
                    glReadPixels(x1 - box->x1, y1 - box->y1, x2 - x1, 1, f->format, f->type, bits + ofs);
            }
        }
    }
    if (glamor_priv->has_pack_subimage)
        glPixelStorei(GL_PACK_ROW_LENGTH, 0);
}
