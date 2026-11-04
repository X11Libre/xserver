/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * Xaverius Physical Device Registry — public API declarations.
 * Included by consumers of the registry (extension init, request handlers).
 */

#ifndef XAVERIUS_REGISTRY_H
#define XAVERIUS_REGISTRY_H

#include "xaverius_intern.h"

/* Native handle and context accessors for the device record. */
int XaveriusDeviceNativeHandle(XaveriusDevicePtr device);
void *XaveriusDeviceContext(XaveriusDevicePtr device);

#endif /* XAVERIUS_REGISTRY_H */
