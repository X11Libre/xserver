/* SPDX-License-Identifier: MIT OR X11 OR AGPL-3.0-or-later
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_REGISTRY_H
#define _XSERVER_DIX_REGISTRY_H

#include "include/extnsionst.h"
#include "include/resource.h"

/*
 * Result returned from any unsuccessful lookup
 */
#define XREGISTRY_UNKNOWN "<unknown>"

/*
 * Setup and teardown
 */
void dixResetRegistry(void);
void dixFreeRegistry(void);
void dixCloseRegistry(void);

/* Functions used by the X-Resource extension */
void RegisterResourceName(RESTYPE type, const char *name);
const char *LookupResourceName(RESTYPE rtype);

void RegisterExtensionNames(ExtensionEntry * ext);

/*
 * Lookup functions.  The returned string must not be modified or freed.
 */
const char *LookupMajorName(int major);
const char *LookupRequestName(int major, int minor);
const char *LookupEventName(int event);
const char *LookupErrorName(int error);

void LookupDixAccessName(Mask acc, char *buf, int sz);

#endif /* _XSERVER_DIX_REGISTRY_H */
