/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 cepelinas9000, gtautvis@gmail.com
 */

/* Generic lttng-ust diz tracing  */


#undef LTTNG_UST_TRACEPOINT_PROVIDER
#define LTTNG_UST_TRACEPOINT_PROVIDER xlibre_dix

#undef LTTNG_UST_TRACEPOINT_INCLUDE
#define LTTNG_UST_TRACEPOINT_INCLUDE "./xlibre_gentracing_tp.h"

#if !defined(_TP_H) || \
defined(LTTNG_UST_TRACEPOINT_HEADER_MULTI_READ)
#define _TP_H

#include <lttng/tracepoint.h>


    /* generic string print event (simpler and prefixed version of https://lttng.org/man/3/lttng_ust_vtracef/v2.13/ ) */
    LTTNG_UST_TRACEPOINT_EVENT(
        /* Tracepoint provider name */
        xlibre_dix,

        /* Tracepoint/event name */
        print,

        /* List of tracepoint arguments (input) */
        LTTNG_UST_TP_ARGS(
            int, client_id,
            const char*,str
            ),

        /* List of fields of eventual event (output) */
        LTTNG_UST_TP_FIELDS(
            lttng_ust_field_integer(int,client_id,client_id)
            lttng_ust_field_string(msg,str)
            )
        )

LTTNG_UST_TRACEPOINT_EVENT(
    /* Tracepoint provider name */
    xlibre_dix,

    /* Tracepoint/event name */
    CopyArea,

    /* List of tracepoint arguments (input) */
    LTTNG_UST_TP_ARGS(
        int, client_id,
        uint32_t, src_drawable,
        uint32_t, dst_drawable,
        uint32_t, srcX,
        uint32_t, srcY,
        uint32_t, dstX,
        uint32_t,  dstY,
        uint32_t, width,
        uint32_t, height

        ),

    /* List of fields of eventual event (output) */
    LTTNG_UST_TP_FIELDS(
        lttng_ust_field_integer(int,client_id,client_id)
        lttng_ust_field_integer_hex(uint32_t, src, src_drawable)
        lttng_ust_field_integer_hex(uint32_t, dst, dst_drawable)

        lttng_ust_field_integer(uint32_t, srcX, srcX)
        lttng_ust_field_integer(uint32_t, srcY, srcY)

        lttng_ust_field_integer(uint32_t, dstX, dstX)
        lttng_ust_field_integer(uint32_t, dstY, dstY)

        lttng_ust_field_integer(uint32_t, width, width)
        lttng_ust_field_integer(uint32_t, height, height)

        )
    )
#endif


/*
 * Add this after defining the tracepoint events to expand the macros.
 */
#include <lttng/tracepoint-event.h>
