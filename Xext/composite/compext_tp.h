/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 cepelinas9000, gtautvis@gmail.com
 */

#undef LTTNG_UST_TRACEPOINT_PROVIDER
#define LTTNG_UST_TRACEPOINT_PROVIDER xlibre_xext_composite

#undef LTTNG_UST_TRACEPOINT_INCLUDE
#define LTTNG_UST_TRACEPOINT_INCLUDE "./compext_tp.h"

#if !defined(_TP_H) || \
defined(LTTNG_UST_TRACEPOINT_HEADER_MULTI_READ)
#define _TP_H

#include <lttng/tracepoint.h>

    LTTNG_UST_TRACEPOINT_ENUM(
        xlibre_xext_composite,
        composite_req,
        LTTNG_UST_TP_ENUM_VALUES(
            lttng_ust_field_enum_value("X_CompositeQueryVersion", 0)
            lttng_ust_field_enum_value("X_CompositeRedirectWindow", 1)
            lttng_ust_field_enum_value("X_CompositeRedirectSubwindows", 2)
            lttng_ust_field_enum_value("X_CompositeUnredirectWindow", 3)
            lttng_ust_field_enum_value("X_CompositeUnredirectSubwindows", 4)
            lttng_ust_field_enum_value("X_CompositeCreateRegionFromBorderClip", 5)
            lttng_ust_field_enum_value("X_CompositeNameWindowPixmap", 6)
            lttng_ust_field_enum_value("X_CompositeGetOverlayWindow", 7)
            lttng_ust_field_enum_value("X_CompositeReleaseOverlayWindow", 8)
            )
        )

    LTTNG_UST_TRACEPOINT_EVENT(
        /* Tracepoint provider name */
        xlibre_xext_composite,

        /* Tracepoint/event name */
        generic_req,

        /* List of tracepoint arguments (input) */
        LTTNG_UST_TP_ARGS(
            int, client_id,
            uint8_t, _req,
            int, size
            ),

        /* List of fields of eventual event (output) */
        LTTNG_UST_TP_FIELDS(
            lttng_ust_field_integer(int,client_id,client_id)
            lttng_ust_field_enum(xlibre_xext_composite, composite_req, uint8_t, req, _req)
            lttng_ust_field_integer(int,req_size,size)
            )
        )

    LTTNG_UST_TRACEPOINT_EVENT(
        /* Tracepoint provider name */
        xlibre_xext_composite,

        /* Tracepoint/event name */
        redirect_req,

        /* List of tracepoint arguments (input) */
        LTTNG_UST_TP_ARGS(
            int, client_id,
            uint8_t, _req,
            uint32_t, window,
            uint8_t, update
            ),

        /* List of fields of eventual event (output) */
        LTTNG_UST_TP_FIELDS(
            lttng_ust_field_integer(int,client_id,client_id)
            lttng_ust_field_enum(xlibre_xext_composite, composite_req, uint8_t, req, _req)
            lttng_ust_field_integer_hex(uint32_t,window,window)
            lttng_ust_field_integer_hex(uint8_t,update,update)
            )
        )

    LTTNG_UST_TRACEPOINT_EVENT(
        /* Tracepoint provider name */
        xlibre_xext_composite,

        /* Tracepoint/event name */
        redirect_req_simple,

        /* List of tracepoint arguments (input) */
        LTTNG_UST_TP_ARGS(
            int, client_id,
            uint8_t, _req,
            uint32_t, window
            ),

        /* List of fields of eventual event (output) */
        LTTNG_UST_TP_FIELDS(
            lttng_ust_field_integer(int,client_id,client_id)
            lttng_ust_field_enum(xlibre_xext_composite, composite_req, uint8_t, req, _req)
            lttng_ust_field_integer_hex(uint32_t,window,window)
            )
        )


    LTTNG_UST_TRACEPOINT_EVENT(
        /* Tracepoint provider name */
        xlibre_xext_composite,

        /* Tracepoint/event name */
        window_pixmap_redirect_req,

        /* List of tracepoint arguments (input) */
        LTTNG_UST_TP_ARGS(
            int, client_id,
            uint8_t, _req,
            uint32_t, window,
            uint32_t, pixmap
            ),

        /* List of fields of eventual event (output) */
        LTTNG_UST_TP_FIELDS(
            lttng_ust_field_integer(int,client_id,client_id)
            lttng_ust_field_enum(xlibre_xext_composite, composite_req, uint8_t, req, _req)
            lttng_ust_field_integer_hex(uint32_t,window,window)
            lttng_ust_field_integer_hex(uint32_t,pixmap,pixmap)
            )
        )
#endif


/*
 * Add this after defining the tracepoint events to expand the macros.
 */
#include <lttng/tracepoint-event.h>
