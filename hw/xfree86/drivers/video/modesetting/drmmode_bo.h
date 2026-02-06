/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111@shitposting.expert>
 */

#ifndef DRMMODE_BO_H
#define DRMMODE_BO_H

#include <gbm.h>
#include "drmmode_display.h"

enum {
    DRMMODE_FRONT_BO = 1 << 0,
    DRMMODE_CURSOR_BO = 1 << 1,
    DRMMODE_SHADOW_BO = 1 << 2,
};

void*
gbm_bo_get_map(struct gbm_bo *bo);

Bool
gbm_bo_get_used_modifiers(struct gbm_bo *bo);

/* Create the best gbm bo of a given type */
struct gbm_bo*
gbm_create_best_bo(drmmode_ptr drmmode, Bool do_map,
                   uint32_t width, uint32_t height,
                   int type);

/* dmabuf import */
struct gbm_bo*
gbm_back_bo_from_fd(drmmode_ptr drmmode, Bool do_map,
                    int fd_handle, uint32_t pitch, uint32_t size);

/* A bit of a misnomer, this is a dmabuf export */
int
drmmode_bo_import(drmmode_ptr drmmode, struct gbm_bo *bo,
                  uint32_t *fb_id);

static inline uint32_t
drmmode_gbm_format_for_depth(int depth)
{
    switch (depth) {
    case 8:
        return GBM_FORMAT_R8;
    case 15:
        return GBM_FORMAT_ARGB1555;
    case 16:
        return GBM_FORMAT_RGB565;
    case 24:
        return GBM_FORMAT_XRGB8888;
    case 30:
        /* XXX Is this format right? https://github.com/X11Libre/xserver/pull/1396/files#r2523698616 XXX */
        return GBM_FORMAT_ARGB2101010;
    case 32:
        return GBM_FORMAT_ARGB8888;
    }

    /* Unsupported depth */
    return GBM_FORMAT_ARGB8888;
}

#endif /* DRMMODE_BO_H */
