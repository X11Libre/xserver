/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 cepelinas9000, gtautvis@gmail.com
 */
#undef LTTNG_UST_TRACEPOINT_PROVIDER
#define LTTNG_UST_TRACEPOINT_PROVIDER xlibre_xext_damage

#undef LTTNG_UST_TRACEPOINT_INCLUDE
#define LTTNG_UST_TRACEPOINT_INCLUDE "./damageext_tp.h"

#if !defined(_TP_H) || \
defined(LTTNG_UST_TRACEPOINT_HEADER_MULTI_READ)
#define _TP_H

#include <lttng/tracepoint.h>

    LTTNG_UST_TRACEPOINT_ENUM(
        xlibre_xext_damage,
        damage_req,
        LTTNG_UST_TP_ENUM_VALUES(
            lttng_ust_field_enum_value("X_DamageQueryVersion", 0)
            lttng_ust_field_enum_value("X_DamageCreate", 1)
            lttng_ust_field_enum_value("X_DamageDestroy", 2)
            lttng_ust_field_enum_value("X_DamageSubtract", 3)
            lttng_ust_field_enum_value("X_DamageAdd", 4)

            )
        )


    LTTNG_UST_TRACEPOINT_EVENT(
        /* Tracepoint provider name */
        xlibre_xext_damage,

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
            lttng_ust_field_enum(xlibre_xext_damage, damage_req, uint8_t, req, _req)
            lttng_ust_field_integer(int,req_size,size)
            )
        )

    LTTNG_UST_TRACEPOINT_EVENT(
        /* Tracepoint provider name */
        xlibre_xext_damage,

        /* Tracepoint/event name */
        generic_req2,

        /* List of tracepoint arguments (input) */
        LTTNG_UST_TP_ARGS(
            int, client_id,
            uint8_t, _req,
            uint32_t, damage,
            uint32_t, drawable
            ),

        /* List of fields of eventual event (output) */
        LTTNG_UST_TP_FIELDS(
            lttng_ust_field_integer(int,client_id,client_id)
            lttng_ust_field_enum(xlibre_xext_damage, damage_req, uint8_t, req, _req)
            lttng_ust_field_integer_hex(uint32_t,drawable,drawable)
            lttng_ust_field_integer_hex(uint32_t,damage,damage)
            )
        )

#endif


/*
 * Add this after defining the tracepoint events to expand the macros.
 */
#include <lttng/tracepoint-event.h>
