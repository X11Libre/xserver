/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111github@gmail.com>
 */

#include <kdrive-config.h>

#include "cursorstr.h"

#include "kdrive.h"

#include <stdint.h>

/*
 * Given a screen coordinate, rotate back to a cursor source coordinate
 */
void
KdRotateCoord(Rotation rotation,
              int width, int height,
              int x_dst, int y_dst,
              int *x_src, int *y_src)
{
    int t;

    switch (rotation & 0xf) {
    case RR_Rotate_0:
        break;
    case RR_Rotate_90:
        t = x_dst;
        x_dst = width - y_dst - 1;
        y_dst = t;
        break;
    case RR_Rotate_180:
        x_dst = width - x_dst - 1;
        y_dst = height - y_dst - 1;
        break;
    case RR_Rotate_270:
        t = x_dst;
        x_dst = y_dst;
        y_dst = height - t - 1;
        break;
    }
    if (rotation & RR_Reflect_X)
        x_dst = width - x_dst - 1;
    if (rotation & RR_Reflect_Y)
        y_dst = height - y_dst - 1;

    if (x_dst < 0) {
        x_dst = 0;
    }

    if (y_dst < 0) {
        y_dst = 0;
    }

    *x_src = x_dst;
    *y_src = y_dst;
}

/*
 * Given a cursor source  coordinate, rotate to a screen coordinate
 */
void
KdRotateCoordBack(Rotation rotation,
                  int width, int height,
                  int x_dst, int y_dst,
                  int *x_src, int *y_src)
{
    int t;

    if (rotation & RR_Reflect_X)
        x_dst = width - x_dst - 1;
    if (rotation & RR_Reflect_Y)
        y_dst = height - y_dst - 1;

    switch (rotation & 0xf) {
    case RR_Rotate_0:
        break;
    case RR_Rotate_90:
        t = x_dst;
        x_dst = y_dst;
        y_dst = width - t - 1;
        break;
    case RR_Rotate_180:
        x_dst = width - x_dst - 1;
        y_dst = height - y_dst - 1;
        break;
    case RR_Rotate_270:
        t = x_dst;
        x_dst = height - y_dst - 1;
        y_dst = t;
        break;
    }

    *x_src = x_dst;
    *y_src = y_dst;
}

/* Adapted from kdrive ati_cursor.c RadeonLoadCursor */
void
KdLoadCursor(uint32_t *ram, CursorPtr pCursor,
             int width, int height, int stride)
{
    CursorBitsPtr bits = pCursor->bits;
    uint32_t *msk, *mskLine, *src, *srcLine;

    int x, y;

    int w = bits->width;
    int h = bits->height;

    if (w > width) {
        w = width;
    }
    if (h > height) {
        h = height;
    }

    if (bits->argb) {
        srcLine = (uint32_t*)bits->argb;
        for (y = 0; y < h; y++) {
            src = srcLine;
            srcLine += bits->width;
            for (x = 0; x < w; x++) {
                ram[y * stride + x] = *src++;
            }
            memset(ram + y * stride + x, 0, (width - x) * sizeof(*ram));
        }
        for (; y < height; y++) {
            memset(ram + y * stride, 0, width * sizeof(*ram));
        }
    } else {
        uint32_t colors[4];
        colors[0] = 0;
        colors[1] = 0;
        colors[2] = (((pCursor->backRed   >> 8) << 16) |
                     ((pCursor->backGreen >> 8) <<  8) |
                     ((pCursor->backBlue  >> 8) <<  0) |
                     0xff000000);
        colors[3] = (((pCursor->foreRed   >> 8) << 16) |
                     ((pCursor->foreGreen >> 8) <<  8) |
                     ((pCursor->foreBlue  >> 8) <<  0) |
                     0xff000000);
        mskLine = (uint32_t*)bits->mask;
        srcLine = (uint32_t*)bits->source;

        /* words per line */
        int lwsrc = BitmapBytePad(bits->width) / sizeof(uint32_t);

        for (y = 0; y < height; y++) {
            uint32_t m, s;

            msk = mskLine;
            src = srcLine;
            mskLine += lwsrc;
            srcLine += lwsrc;

            for (x = 0; x < width / 32; x++) {
                if (y < h && x < lwsrc) {
                    m = *msk++;
                    s = *src++;
                } else {
                    m = 0;
                    s = 0;
                }

                for (int k = 0; k < 32; k++) {
                    uint32_t val = (s & 1) | ((m & 1) << 1);
                    ram[y * stride + x * 32 + k] = colors[val];
                    s >>= 1;
                    m >>= 1;
                }
            }
        }
    }
}

void
KdLoadCursorRandR(uint32_t *ram, CursorPtr pCursor,
                  int width, int height, int stride,
                  Rotation randr, uint32_t **shadow,
                  int *xhot, int *yhot)
{
    uint32_t *map = ram;
    int t;

    if (!xhot) {
        xhot = &t;
    }

    if (!yhot) {
        yhot = &t;
    }

    if (!shadow) {
        shadow = &ram;
    }

    if (randr != RR_Rotate_0) {
        if (*shadow == NULL) {
            *shadow = calloc(width * stride, sizeof(*ram));
        }
        if (*shadow) {
            ram = *shadow;
        }
    }

    KdLoadCursor(ram, pCursor, width, height, stride);

    if (randr != RR_Rotate_0) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int x_in, y_in;
                KdRotateCoord(randr,
                              width, height,
                              x, y,
                              &x_in, &y_in);
                map[y * stride + x] = ram[y_in * stride + x_in];
            }
        }
    }

    /* Save the cursor glyph hotspot */
    KdRotateCoordBack(randr,
                      width, height,
                      pCursor->bits->xhot, pCursor->bits->yhot,
                      xhot, yhot);
}

void
KdGetCursorPosition(ScreenPtr pScreen, Rotation randr,
                    int xhot, int yhot,
                    int *x, int *y)
{
    KdRotateCoordBack(randr,
                      pScreen->width, pScreen->height,
                      *x, *y,
                      x, y);

    /* Offset the cursor position, so that the cursor hotspot looks right */
    *x -= xhot;
    *y -= yhot;
}
