/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111@shitposting.expert>
 */

#ifndef DRMMODE_BO_H
#define DRMMODE_BO_H

/* Forward declarations so we don't have to include gbm.h*/
struct gbm_bo;
struct gbm_device;

#include "drmmode_display.h"
#include "dri3_util_priv.h"

enum {
    DRMMODE_FRONT_BO = 1 << 0,
    DRMMODE_CURSOR_BO = 1 << 1,
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
    /* For modesetting, we usually want alpha only if the depth is explicitly 32 */
    return dri3_gbm_format_for_depth(depth, (depth + 7) & ~7, depth == 32 || depth == 30);
}

#endif /* DRMMODE_BO_H */
