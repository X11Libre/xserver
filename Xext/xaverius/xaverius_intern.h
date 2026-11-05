/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 *
 * Xaverius internal definitions shared between source files within the extension.
 */
#ifndef _XLIBRE_XAVERIUS_INTERN_H
#define _XLIBRE_XAVERIUS_INTERN_H

#include <stdbool.h>
#include <stdint.h>

#include <X11/Xdefs.h>

#include "include/list.h"
#include "include/privates.h"
#include "include/xlibre_ptrtypes.h"

#include "xaverius_priv.h"
#include "xaverius_proto.h"

/* Forward declarations for internal types */
struct _XaveriusDevice;
typedef struct _XaveriusDevice XaveriusDeviceRec, *XaveriusDevicePtr;

extern int XaveriusEventBase;

extern DevPrivateKeyRec XaveriusScreenPrivateKeyRec;

/* Global property atoms (initialized in init.c) */
extern Atom xaverius_atom_device_uuid;
extern Atom xaverius_atom_driver_uuid;
extern Atom xaverius_atom_max_work_group_invocations;
extern Atom xaverius_atom_max_work_group_size;
extern Atom xaverius_atom_timeline_semaphore;

/*
 * Device type values (xserver-internal, no separate xaverius_device.h header).
 */
#define XAVERIUS_DEVICE_TYPE_PHYSICAL    1
#define XAVERIUS_DEVICE_TYPE_LOGICAL     2

extern struct xorg_list xaverius_physical_devices;
extern uint32_t xaverius_physical_device_count;

int ProcXaveriusDispatch(ClientPtr client);

bool XaveriusRegistryInit(void);
void XaveriusRegistryFini(void);

XaveriusDevicePtr XaveriusFindDeviceByXid(XID xid);
XaveriusDevicePtr XaveriusFindDeviceByIdentity(XaveriusDeviceIdentity identity);
void XaveriusSetDeviceStatus(XaveriusDevicePtr device, XaveriusDeviceStatus status);
XaveriusDevicePtr XaveriusFirstDevice(void);
XaveriusDevicePtr XaveriusNextDevice(XaveriusDevicePtr device);
uint32_t XaveriusDeviceCount(void);

struct _XaveriusDevice {
    XID xid;
    XaveriusDeviceIdentity identity;
    XaveriusDeviceStatus status;
    uint64_t generation;
    XaveriusDeviceCapabilities capabilities;
    int native_handle;
    void *context;
    struct xorg_list list;
};

#endif /* _XLIBRE_XAVERIUS_INTERN_H */
