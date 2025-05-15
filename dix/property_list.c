/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include "dix/property_priv.h"
#include "include/privates.h"
#include "include/propertyst.h"
#include "os/bug_priv.h"

void dixPropertyFree(PropertyPtr pr)
{
    if (pr) {
        free(pr->data);
        dixFreeObjectWithPrivates(pr, PRIVATE_PROPERTY);
    }
}

PropertyPtr dixPropertyCreate(Atom type, Atom name, int format, size_t len,
                              const void *value)
{
    const int totalSize = (format >> 3) * len;

    void *data = calloc(1, totalSize);
    if (!data)
        return NULL;

    PropertyPtr pProp = dixAllocateObjectWithPrivates(PropertyRec, PRIVATE_PROPERTY);
    if (!pProp) {
        free(data);
        return NULL;
    }

    memcpy(data, value, totalSize);

    pProp->data = data;
    pProp->propertyName = name;
    pProp->format = format;
    pProp->size = len;
    pProp->type = type;

    return pProp;
}

PropertyPtr dixPropertyUnlinkPtr(PropertyPtr *list, PropertyPtr prop)
{
    BUG_RETURN_VAL(!list, NULL);

    if ((!prop) || (!(*list))) // nothing to do
        return NULL;

    PropertyPtr walk = *list;

    // remove from head
    if (walk == prop) {
        *list = prop->next;
        walk->next = NULL;
        return prop;
    }

    // walk the list to find it
    while (walk->next && walk->next != prop) {
        walk = walk->next;
    }

    // didn't find it
    if (!walk->next)
        return NULL;

    // unlink the element
    walk->next = walk->next->next;
    prop->next = NULL;

    return prop;
}
