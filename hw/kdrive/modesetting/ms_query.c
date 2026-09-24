/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2026 stefan11111 <stefan11111github@gmail.com>
 */

#include <kdrive-config.h>

#include "modesetting.h"

#include <errno.h> /* ENOENT */

/* Various modesetting query functions */

static int
modesetting_grade_mode(drmModeModeInfo *mode, uint32_t req_w, uint32_t req_h, uint32_t req_rate)
{
    int score = 1;

    if (req_w && (req_w == mode->hdisplay)) {
        score += 10;
    }

    if (req_h && (req_h == mode->vdisplay)) {
        score += 10;
    }

    if (req_rate && (req_rate == mode->vrefresh)) {
        score += 5;
    }

    if (mode->type & DRM_MODE_TYPE_PREFERRED) {
        score++;
    }

    return score;
}

drmModeModeInfo*
modesetting_find_mode(drmModeConnector *conn, uint32_t req_w, uint32_t req_h, uint32_t req_rate)
{
    drmModeModeInfo *best_mode = NULL;
    int best_score = 0;

    for (int i = 0; i < conn->count_modes; i++) {
        drmModeModeInfo *mode = &conn->modes[i];
        int score;

        score = modesetting_grade_mode(mode, req_w, req_h, req_rate);
        if (score <= best_score) {
            continue;
        }

        best_mode = mode;
        best_score = score;
    }

    return best_mode;
}

static int
modesetting_grade_connector(msPriv *priv, drmModeConnector *conn, uint32_t conn_id)
{
    int score = 1;
    Bool in_use = FALSE;

    if (conn->modes && conn->count_modes) {
        score += 5;
    }

    switch(conn->connection) {
    case DRM_MODE_CONNECTED:
        score++;
    case DRM_MODE_UNKNOWNCONNECTION:
        score++;
    case DRM_MODE_DISCONNECTED:
        score++;
    }

    for(int i = 0; i < priv->num_used_connectors; i++) {
        if (priv->used_connectors[i] == conn_id) {
            in_use = TRUE;
            break;
        }
    }

    if (!in_use) {
        score += 10;
    }

    return score;
}

drmModeConnector*
modesetting_find_connector(msPriv *priv, int fd, uint32_t *conn_id)
{
    drmModeConnector *best_connector = NULL;
    int best_score = 0;

    drmModeRes *res = priv->resources;

    for (int i = 0; i < res->count_connectors; i++) {
        drmModeConnector *conn;
        int id;
        int score;
        id = res->connectors[i];
        conn = drmModeGetConnector(fd, id);
        if (!conn) {
            continue;
        }

        score = modesetting_grade_connector(priv, conn, id);
        if (score <= best_score) {
            drmModeFreeConnector(conn);
            continue;
        }

        if (best_connector) {
            drmModeFreeConnector(best_connector);
        }

        best_connector = conn;
        *conn_id = id;
        best_score = score;
    }

    return best_connector;
}

static Bool
modesetting_crtc_is_used(msPriv *priv, int crtc)
{
    for (int i = 0; i < priv->num_used_crtcs; i++) {
        if (priv->used_crtcs[i] == crtc) {
            return TRUE;
        }
    }

    return FALSE;
}

/* Slightly modified rom man drm-kms */
int
modeseting_find_crtc(msPriv *priv, int fd, drmModeConnector *conn)
{
     drmModeRes *res = priv->resources;
    drmModeEncoder *enc;
    unsigned int i, j;
    int crtc = -ENOENT;

    /* iterate all encoders of this connector */
    for (i = 0; i < conn->count_encoders; ++i) {
        enc = drmModeGetEncoder(fd, conn->encoders[i]);
        if (!enc) {
            /* cannot retrieve encoder, ignoring... */
            continue;
        }

        /* iterate all global CRTCs */
        for (j = 0; j < res->count_crtcs; ++j) {
            /* check whether this CRTC works with the encoder */
            if (!(enc->possible_crtcs & (1 << j)))
                continue;

            /* Here you need to check that no other connector
             * currently uses the CRTC with id "crtc". If you intend
             * to drive one connector only, then you can skip this
             * step. Otherwise, simply scan your list of configured
             * connectors and CRTCs whether this CRTC is already
             * used. If it is, then simply continue the search here. */
            if (!modesetting_crtc_is_used(priv, res->crtcs[j])) {
                drmModeFreeEncoder(enc);
                return res->crtcs[j];
            }

            /* Allow reusing crtcs for testing */
            if (crtc < 0) {
                crtc = res->crtcs[j];
            }
        }

        drmModeFreeEncoder(enc);
    }

    /* cannot find a suitable CRTC */
    return crtc;
}
