/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 AbdulRehman, 2LazyDevs <ardev1.deverson@proton.me>
 */

#ifndef DRI3_UTIL_PRIV_H
#define DRI3_UTIL_PRIV_H

#include <stdint.h>
#include <stdbool.h>

#ifdef WITH_LIBDRM
#include <xf86drm.h>
#include <drm_fourcc.h>
#else
/* Manual fallbacks for FourCC values if LibDRM is missing */
#define DRM_FORMAT_C8            0x20203843
#define DRM_FORMAT_R8            0x20203852
#define DRM_FORMAT_G8            0x20203847
#define DRM_FORMAT_RGB565        0x36314752
#define DRM_FORMAT_BGR565        0x36314742
#define DRM_FORMAT_XRGB1555      0x35315258
#define DRM_FORMAT_XBGR1555      0x35314258
#define DRM_FORMAT_ARGB1555      0x35315241
#define DRM_FORMAT_ABGR1555      0x35314241
#define DRM_FORMAT_GR88          0x38385247
#define DRM_FORMAT_RGB888        0x34324752
#define DRM_FORMAT_BGR888        0x34324742
#define DRM_FORMAT_XRGB8888      0x34325258
#define DRM_FORMAT_XBGR8888      0x34324258
#define DRM_FORMAT_RGBX8888      0x34325852
#define DRM_FORMAT_BGRX8888      0x34325842
#define DRM_FORMAT_ARGB8888      0x34325241
#define DRM_FORMAT_ABGR8888      0x34324241
#define DRM_FORMAT_RGBA8888      0x34324152
#define DRM_FORMAT_BGRA8888      0x34324142
#define DRM_FORMAT_XRGB2101010   0x30335258
#define DRM_FORMAT_XBGR2101010   0x30334258
#define DRM_FORMAT_ARGB2101010   0x30335241
#define DRM_FORMAT_ABGR2101010   0x30334241

/* 64-bit HDR Formats */
#define DRM_FORMAT_XRGB16161616F 0x48345258
#define DRM_FORMAT_XBGR16161616F 0x48344258
#define DRM_FORMAT_ARGB16161616F 0x48345241
#define DRM_FORMAT_ABGR16161616F 0x48344241
#define DRM_FORMAT_XRGB16161616  0x38345258
#define DRM_FORMAT_XBGR16161616  0x38344258
#define DRM_FORMAT_ARGB16161616  0x38345241
#define DRM_FORMAT_ABGR16161616  0x38344241
#endif

static inline uint32_t 
dri3_fourcc_for_depth(int depth, int bpp, bool explicit_alpha)
{
    switch (bpp) {
    case 8:
        return DRM_FORMAT_R8;
    case 16:
        if (depth == 15)
            return explicit_alpha ? DRM_FORMAT_ARGB1555 : DRM_FORMAT_XRGB1555;
        return DRM_FORMAT_RGB565;
    case 32:
        switch (depth) {
        case 30:
            return explicit_alpha ? DRM_FORMAT_ARGB2101010 : DRM_FORMAT_XRGB2101010;
        case 24:
            return explicit_alpha ? DRM_FORMAT_ARGB8888 : DRM_FORMAT_XRGB8888;
        case 32:
            return DRM_FORMAT_ARGB8888;
        default:
            return 0;
        }
    default:
        return 0;
    }
}

static inline uint32_t 
dri3_gbm_format_for_depth(int depth, int bpp, bool alpha) 
{
    return dri3_fourcc_for_depth(depth, bpp, alpha);
}

static inline int 
dri3_bpp_for_fourcc(uint32_t format)
{
    switch (format) {
    case DRM_FORMAT_C8:
    case DRM_FORMAT_R8:
    case DRM_FORMAT_G8:
        return 8;

    case DRM_FORMAT_RGB565:
    case DRM_FORMAT_BGR565:
    case DRM_FORMAT_XRGB1555:
    case DRM_FORMAT_XBGR1555:
    case DRM_FORMAT_ARGB1555:
    case DRM_FORMAT_ABGR1555:
    case DRM_FORMAT_GR88:
        return 16;

    case DRM_FORMAT_RGB888:
    case DRM_FORMAT_BGR888:
        return 24;

    case DRM_FORMAT_XRGB8888:
    case DRM_FORMAT_XBGR8888:
    case DRM_FORMAT_RGBX8888:
    case DRM_FORMAT_BGRX8888:
    case DRM_FORMAT_ARGB8888:
    case DRM_FORMAT_ABGR8888:
    case DRM_FORMAT_RGBA8888:
    case DRM_FORMAT_BGRA8888:
    case DRM_FORMAT_XRGB2101010:
    case DRM_FORMAT_XBGR2101010:
    case DRM_FORMAT_ARGB2101010:
    case DRM_FORMAT_ABGR2101010:
        return 32;
    
    case DRM_FORMAT_XRGB16161616F:
    case DRM_FORMAT_XBGR16161616F:
    case DRM_FORMAT_ARGB16161616F:
    case DRM_FORMAT_ABGR16161616F:
    case DRM_FORMAT_XRGB16161616:
    case DRM_FORMAT_XBGR16161616:
    case DRM_FORMAT_ARGB16161616:
    case DRM_FORMAT_ABGR16161616:
        return 64;

    default:
        return 0;
    }
}

#endif /* DRI3_UTIL_PRIV_H */   