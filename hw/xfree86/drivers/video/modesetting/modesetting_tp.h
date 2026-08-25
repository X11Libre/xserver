/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 cepelinas9000, gtautvis@gmail.com
 */
#undef LTTNG_UST_TRACEPOINT_PROVIDER
#define LTTNG_UST_TRACEPOINT_PROVIDER xlibre_modesetting

#undef LTTNG_UST_TRACEPOINT_INCLUDE
#define LTTNG_UST_TRACEPOINT_INCLUDE "modesetting_tp.h"

#if !defined(_TP_H) || \
defined(LTTNG_UST_TRACEPOINT_HEADER_MULTI_READ)
#define _TP_H

#include <lttng/tracepoint.h>


LTTNG_UST_TRACEPOINT_EVENT(
    /* Tracepoint provider name */
    xlibre_modesetting,


    vblank_screen_init,
        LTTNG_UST_TP_ARGS(
            int, screen_num,
            int, has_queue_sequence
        ),

    LTTNG_UST_TP_FIELDS(
            lttng_ust_field_integer(int,screen_num,screen_num)
            lttng_ust_field_integer(int,has_queue_sequence,has_queue_sequence)
        )
    )



LTTNG_UST_TRACEPOINT_EVENT(
    /* Tracepoint provider name */
    xlibre_modesetting,

    /* has queue sequence */
    ms_queue_vblank_has_qs,

    LTTNG_UST_TP_ARGS(
        int, screen_num,
        int, crtc_id,
        uint32_t, drm_flags,
        uint64_t, msc,
        uint32_t, seq,
        int, drmCrtc_ret,
        bool, msc_queued_valid,
        uint64_t, msc_queued
        ),

    LTTNG_UST_TP_FIELDS(
        lttng_ust_field_integer(int,screen_num,screen_num)
        lttng_ust_field_integer(int,crtc_id,crtc_id)
        lttng_ust_field_integer_hex(uint32_t,drm_flags,drm_flags)
        lttng_ust_field_integer(uint64_t,msc,msc)
        lttng_ust_field_integer(uint32_t,seq,seq)
        lttng_ust_field_integer(int,drmCrtc_ret,drmCrtc_ret)
        lttng_ust_field_integer(int,msc_queued_valid,msc_queued_valid)
        lttng_ust_field_integer(uint64_t,msc_queued,msc_queued)
        )
    )

LTTNG_UST_TRACEPOINT_EVENT(
    /* Tracepoint provider name */
    xlibre_modesetting,

    /* has queue sequence */
    ms_queue_vblank_no_qs,

    LTTNG_UST_TP_ARGS(
        int, screen_num,
        int, crtc_id,
        uint32_t, drm_flags,
        uint64_t, msc,
        uint32_t, seq,
        int, drmWaitVB_ret,
        bool, msc_queued_valid,
        uint64_t, msc_queued
        ),

    LTTNG_UST_TP_FIELDS(
        lttng_ust_field_integer(int,screen_num,screen_num)
        lttng_ust_field_integer(int,crtc_id,crtc_id)
        lttng_ust_field_integer_hex(uint32_t,drm_flags,drm_flags)
        lttng_ust_field_integer(uint64_t,msc,msc)
        lttng_ust_field_integer(uint32_t,seq,seq)
        lttng_ust_field_integer(int,drmWaitVB_ret,drmWaitVB_ret)
        lttng_ust_field_integer(int,msc_queued_valid,msc_queued_valid)
        lttng_ust_field_integer(uint64_t,msc_queued,msc_queued)
        )
    )

LTTNG_UST_TRACEPOINT_EVENT(
    /* Tracepoint provider name */
    xlibre_modesetting,

    /* has queue sequence */
    ms_queue_vblank_abort,

    LTTNG_UST_TP_ARGS(
        int, screen_num,
        int, crtc_id,
        uint32_t, queue_flags,
        uint64_t, msc,
        uint32_t, seq
        ),

    LTTNG_UST_TP_FIELDS(
        lttng_ust_field_integer(int,screen_num,screen_num)
        lttng_ust_field_integer(int,crtc_id,crtc_id)
        lttng_ust_field_integer_hex(uint32_t,queue_flags,queue_flags)
        lttng_ust_field_integer(uint64_t,msc,msc)
        lttng_ust_field_integer(uint32_t,seq,seq)
        )
    )

LTTNG_UST_TRACEPOINT_EVENT(
    /* Tracepoint provider name */
    xlibre_modesetting,

    /* has queue sequence */
    ms_get_kernel_ust_msc_has_qs,

    LTTNG_UST_TP_ARGS(
        int, screen_num,
        int, crtc_id,
        int, drmCrtcGet_ret,
        uint64_t, msc,
        uint64_t, ust

        ),

    LTTNG_UST_TP_FIELDS(
        lttng_ust_field_integer(int,screen_num,screen_num)
        lttng_ust_field_integer(int,crtc_id,crtc_id)
        lttng_ust_field_integer(int,drmCrtcGet_ret,drmCrtcGet_ret)
        lttng_ust_field_integer(uint64_t,msc,msc)
        lttng_ust_field_integer(uint32_t,ust,ust)
        )
    )

LTTNG_UST_TRACEPOINT_EVENT(
    /* Tracepoint provider name */
    xlibre_modesetting,

    /* has queue sequence */
    ms_get_kernel_ust_msc_no_qs,

    LTTNG_UST_TP_ARGS(
        int, screen_num,
        int, crtc_id,
        int, drmWaitVB_ret,
        uint64_t, msc,
        uint64_t, ust

        ),

    LTTNG_UST_TP_FIELDS(
        lttng_ust_field_integer(int,screen_num,screen_num)
        lttng_ust_field_integer(int,crtc_id,crtc_id)
        lttng_ust_field_integer(int,drmWaitVB_ret,drmWaitVB_ret)
        lttng_ust_field_integer_hex(uint64_t,msc,msc)
        lttng_ust_field_integer_hex(uint32_t,ust,ust)
        )
    )


#endif

/*
 * Add this after defining the tracepoint events to expand the macros.
 */
#include <lttng/tracepoint-event.h>
