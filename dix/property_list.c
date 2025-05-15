/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include "dix/property_priv.h"
#include "include/privates.h"
#include "include/propertyst.h"

void dixPropertyFree(PropertyPtr pr)
{
    if (pr) {
        free(pr->data);
        dixFreeObjectWithPrivates(pr, PRIVATE_PROPERTY);
    }
}
