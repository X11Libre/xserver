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

/* Registry lifecycle */
void XaveriusRegistryInit(void);
void XaveriusRegistryFini(void);

/* Device registration and lookup */
bool XaveriusRegisterDeviceDirect(const XaveriusDeviceIdentity *identity,
                                  const XaveriusDeviceCapabilities *caps);
XaveriusDevicePtr XaveriusFindDeviceByXid(XID xid);
void XaveriusSetDeviceStatus(XaveriusDevicePtr device, XaveriusDeviceStatus status);
XaveriusDevicePtr XaveriusFirstDevice(void);
XaveriusDevicePtr XaveriusNextDevice(XaveriusDevicePtr device);
uint32_t XaveriusDeviceCount(void);

#endif /* XAVERIUS_REGISTRY_H */