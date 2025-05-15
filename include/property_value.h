/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * This header is part of the SDK / public driver API
 */
#ifndef _XORG_SDK_PROPERTY_VALUE_H
#define _XORG_SDK_PROPERTY_VALUE_H

#include <stdint.h>

#include "misc.h" // ATOM type

typedef struct _PropertyValue {
    ATOM type;                  /* ignored by server */
    uint32_t format;            /* format of data for swapping - 8,16,32 */
    uint32_t size;              /* size of data in (format/8) bytes */
    void *data;                 /* private to client */
} PropertyValueRec, *PropertyValuePtr;

#endif /* _XORG_SDK_PROPERTY_VALUE_H */
