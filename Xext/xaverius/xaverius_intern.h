/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * Xaverius internal definitions shared between source files within the extension.
 */
#ifndef _XLIBRE_XAVERIUS_INTERN_H
#define _XLIBRE_XAVERIUS_INTERN_H

#include <stdbool.h>
#include <X11/Xdefs.h>

#include "include/list.h"
#include "include/privates.h"
#include "include/xlibre_ptrtypes.h"

#include "xaverius_priv.h"

extern int XaveriusEventBase;

extern DevPrivateKeyRec XaveriusScreenPrivateKeyRec;

/* Global property atoms (initialized in init.c) */
extern Atom xaverius_atom_device_uuid;
extern Atom xaverius_atom_driver_uuid;
extern Atom xaverius_atom_max_work_group_invocations;
extern Atom xaverius_atom_max_work_group_size;
extern Atom xaverius_atom_timeline_semaphore;

int ProcXaveriusDispatch(ClientPtr client);

/* Registry functions */
void XaveriusRegistryInit(void);
void XaveriusRegistryFini(void);

XaveriusDevicePtr XaveriusRegisterDevice(XaveriusDeviceIdentity identity,
                                         const XaveriusDeviceCapabilities *caps);
XaveriusDevicePtr XaveriusFindDeviceByXid(XID xid);
XaveriusDevicePtr XaveriusFindDeviceByIdentity(XaveriusDeviceIdentity identity);
void XaveriusSetDeviceStatus(XaveriusDevicePtr device, XaveriusDeviceStatus status);
XaveriusDevicePtr XaveriusFirstDevice(void);
XaveriusDevicePtr XaveriusNextDevice(XaveriusDevicePtr device);
uint32_t XaveriusDeviceCount(void);

/* Internal device record (defined here, opaque to the DDX) */
typedef struct _XaveriusDevice {
    XID xid;
    XaveriusDeviceIdentity identity;
    XaveriusDeviceStatus status;
    uint64_t generation;
    const XaveriusDeviceCapabilities *caps;
    struct xorg_list list;
} XaveriusDeviceRec, *XaveriusDevicePtr;

#endif /* _XLIBRE_XAVERIUS_INTERN_H */
