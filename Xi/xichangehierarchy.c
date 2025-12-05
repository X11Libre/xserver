/*
 * Copyright 2007-2008 Peter Hutterer
 * Copyright 2009 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Peter Hutterer, University of South Australia, NICTA
 */

/***********************************************************************
 *
 * Request change in the device hierarchy.
 *
 */

#include <dix-config.h>

#include <X11/X.h>              /* for inputstr.h    */
#include <X11/Xproto.h>         /* Request macro     */
#include <X11/extensions/XI.h>
#include <X11/extensions/XI2proto.h>
#include <X11/extensions/geproto.h>

#include "dix/dix_priv.h"
#include "dix/exevents_priv.h"
#include "dix/extension_priv.h"
#include "dix/input_priv.h"
#include "dix/request_priv.h"
#include "os/bug_priv.h"
#include "Xi/handlers.h"

#include "inputstr.h"           /* DeviceIntPtr      */
#include "windowstr.h"          /* window structure  */
#include "scrnintstr.h"         /* screen structure  */
#include "extnsionst.h"
#include "exglobals.h"
#include "misc.h"
#include "xace.h"
#include "xiquerydevice.h"      /* for GetDeviceUse */
#include "xkbsrv.h"
#include "xichangehierarchy.h"
#include "xibarriers.h"

/**
 * Send the current state of the device hierarchy to all clients.
 */
void
XISendDeviceHierarchyEvent(int flags[MAXDEVICES])
{
    xXIHierarchyEvent *ev;
    xXIHierarchyInfo *info;
    DeviceIntRec dummyDev;
    DeviceIntPtr dev;
    int i;

    if (!flags)
        return;

    ev = calloc(1, sizeof(xXIHierarchyEvent) +
                MAXDEVICES * sizeof(xXIHierarchyInfo));
    if (!ev)
        return;
    ev->type = GenericEvent;
    ev->extension = EXTENSION_MAJOR_XINPUT;
    ev->evtype = XI_HierarchyChanged;
    ev->time = GetTimeInMillis();
    ev->flags = 0;
    ev->num_info = inputInfo.numDevices;

    info = (xXIHierarchyInfo *) &ev[1];
    for (dev = inputInfo.devices; dev; dev = dev->next) {
        info->deviceid = dev->id;
        info->enabled = dev->enabled;
        info->use = GetDeviceUse(dev, &info->attachment);
        info->flags = flags[dev->id];
        ev->flags |= info->flags;
        info++;
    }
    for (dev = inputInfo.off_devices; dev; dev = dev->next) {
        info->deviceid = dev->id;
        info->enabled = dev->enabled;
        info->use = GetDeviceUse(dev, &info->attachment);
        info->flags = flags[dev->id];
        ev->flags |= info->flags;
        info++;
    }

    for (i = 0; i < MAXDEVICES; i++) {
        if (flags[i] & (XIMasterRemoved | XISlaveRemoved)) {
            info->deviceid = i;
            info->enabled = FALSE;
            info->flags = flags[i];
            info->use = 0;
            ev->flags |= info->flags;
            ev->num_info++;
            info++;
        }
    }

    ev->length = bytes_to_int32(ev->num_info * sizeof(xXIHierarchyInfo));

    memset(&dummyDev, 0, sizeof(dummyDev));
    dummyDev.id = XIAllDevices;
    dummyDev.type = SLAVE;
    SendEventToAllWindows(&dummyDev, (XI_HierarchyChangedMask >> 8),
                          (xEvent *) ev, 1);
    free(ev);
}

/***********************************************************************
 *
 * This procedure allows a client to change the device hierarchy through
 * adding new master devices, removing them, etc.
 *
 */

static int
add_master(ClientPtr client, xXIAddMasterInfo * c, int flags[MAXDEVICES])
{
    DeviceIntPtr ptr, keybd, XTestptr, XTestkeybd;
    char *name;
    int rc;

    name = calloc(c->name_len + 1, sizeof(char));
    if (name == NULL) {
        rc = BadAlloc;
        goto unwind;
    }
    strncpy(name, (char *) &c[1], c->name_len);

    rc = AllocDevicePair(client, name, &ptr, &keybd,
                         CorePointerProc, CoreKeyboardProc, TRUE);
    if (rc != Success)
        goto unwind;

    if (!c->send_core)
        ptr->coreEvents = keybd->coreEvents = FALSE;

    /* Allocate virtual slave devices for xtest events */
    rc = AllocXTestDevice(client, name, &XTestptr, &XTestkeybd, ptr, keybd);
    if (rc != Success) {
        DeleteInputDeviceRequest(ptr);
        DeleteInputDeviceRequest(keybd);
        goto unwind;
    }

    ActivateDevice(ptr, FALSE);
    ActivateDevice(keybd, FALSE);
    flags[ptr->id] |= XIMasterAdded;
    flags[keybd->id] |= XIMasterAdded;

    ActivateDevice(XTestptr, FALSE);
    ActivateDevice(XTestkeybd, FALSE);
    flags[XTestptr->id] |= XISlaveAdded;
    flags[XTestkeybd->id] |= XISlaveAdded;

    if (c->enable) {
        EnableDevice(ptr, FALSE);
        EnableDevice(keybd, FALSE);
        flags[ptr->id] |= XIDeviceEnabled;
        flags[keybd->id] |= XIDeviceEnabled;

        EnableDevice(XTestptr, FALSE);
        EnableDevice(XTestkeybd, FALSE);
        flags[XTestptr->id] |= XIDeviceEnabled;
        flags[XTestkeybd->id] |= XIDeviceEnabled;
    }

    /* Attach the XTest virtual devices to the newly
       created master device */
    AttachDevice(NULL, XTestptr, ptr);
    AttachDevice(NULL, XTestkeybd, keybd);
    flags[XTestptr->id] |= XISlaveAttached;
    flags[XTestkeybd->id] |= XISlaveAttached;

    for (int i = 0; i < currentMaxClients; i++)
        XIBarrierNewMasterDevice(clients[i], ptr->id);

 unwind:
    free(name);
    return rc;
}

static void
disable_clientpointer(DeviceIntPtr dev)
{
    int i;

    for (i = 0; i < currentMaxClients; i++) {
        ClientPtr client = clients[i];

        if (client && client->clientPtr == dev)
            client->clientPtr = NULL;
    }
}

static DeviceIntPtr
find_disabled_master(int type)
{
    DeviceIntPtr dev;

    /* Once a master device is disabled it loses the pairing, so returning the first
     * match is good enough */
    for (dev = inputInfo.off_devices; dev; dev = dev->next) {
        if (dev->type == type)
            return dev;
    }

    return NULL;
}

static int
remove_master(ClientPtr client, xXIRemoveMasterInfo * r, int flags[MAXDEVICES])
{
    DeviceIntPtr dev, ptr, keybd, XTestptr, XTestkeybd;
    int rc = Success;

    if (r->return_mode != XIAttachToMaster && r->return_mode != XIFloating)
        return BadValue;

    rc = dixLookupDevice(&dev, r->deviceid, client, DixDestroyAccess);
    if (rc != Success)
        goto unwind;

    if (!InputDevIsMaster(dev)) {
        client->errorValue = r->deviceid;
        rc = BadDevice;
        goto unwind;
    }

    /* XXX: For now, don't allow removal of VCP, VCK */
    if (dev == inputInfo.pointer || dev == inputInfo.keyboard) {
        rc = BadDevice;
        goto unwind;
    }

    if ((ptr = GetMaster(dev, MASTER_POINTER)) == NULL)
        ptr = find_disabled_master(MASTER_POINTER);
    BUG_RETURN_VAL(ptr == NULL, BadDevice);
    rc = dixLookupDevice(&ptr, ptr->id, client, DixDestroyAccess);
    if (rc != Success)
        goto unwind;

    if ((keybd = GetMaster(dev, MASTER_KEYBOARD)) == NULL)
        keybd = find_disabled_master(MASTER_KEYBOARD);
    BUG_RETURN_VAL(keybd == NULL, BadDevice);
    rc = dixLookupDevice(&keybd, keybd->id, client, DixDestroyAccess);
    if (rc != Success)
        goto unwind;

    XTestptr = GetXTestDevice(ptr);
    BUG_RETURN_VAL(XTestptr == NULL, BadDevice);
    rc = dixLookupDevice(&XTestptr, XTestptr->id, client, DixDestroyAccess);
    if (rc != Success)
        goto unwind;

    XTestkeybd = GetXTestDevice(keybd);
    BUG_RETURN_VAL(XTestkeybd == NULL, BadDevice);
    rc = dixLookupDevice(&XTestkeybd, XTestkeybd->id, client, DixDestroyAccess);
    if (rc != Success)
        goto unwind;

    disable_clientpointer(ptr);

    /* Disabling sends the devices floating, reattach them if
     * desired. */
    if (r->return_mode == XIAttachToMaster) {
        DeviceIntPtr attached, newptr, newkeybd;

        rc = dixLookupDevice(&newptr, r->return_pointer, client, DixAddAccess);
        if (rc != Success)
            goto unwind;

        if (!InputDevIsMaster(newptr) || !IsPointerDevice(newptr)) {
            client->errorValue = r->return_pointer;
            rc = BadDevice;
            goto unwind;
        }

        rc = dixLookupDevice(&newkeybd, r->return_keyboard,
                             client, DixAddAccess);
        if (rc != Success)
            goto unwind;

        if (!InputDevIsMaster(newkeybd) || !IsKeyboardDevice(newkeybd)) {
            client->errorValue = r->return_keyboard;
            rc = BadDevice;
            goto unwind;
        }

        for (attached = inputInfo.devices; attached; attached = attached->next) {
            if (!InputDevIsMaster(attached)) {
                if (GetMaster(attached, MASTER_ATTACHED) == ptr) {
                    AttachDevice(client, attached, newptr);
                    flags[attached->id] |= XISlaveAttached;
                }
                if (GetMaster(attached, MASTER_ATTACHED) == keybd) {
                    AttachDevice(client, attached, newkeybd);
                    flags[attached->id] |= XISlaveAttached;
                }
            }
        }
    }

    for (int i = 0; i < currentMaxClients; i++)
        XIBarrierRemoveMasterDevice(clients[i], ptr->id);

    /* disable the remove the devices, XTest devices must be done first
       else the sprites they rely on will be destroyed  */
    DisableDevice(XTestptr, FALSE);
    DisableDevice(XTestkeybd, FALSE);
    DisableDevice(keybd, FALSE);
    DisableDevice(ptr, FALSE);
    flags[XTestptr->id] |= XIDeviceDisabled | XISlaveDetached;
    flags[XTestkeybd->id] |= XIDeviceDisabled | XISlaveDetached;
    flags[keybd->id] |= XIDeviceDisabled;
    flags[ptr->id] |= XIDeviceDisabled;

    flags[XTestptr->id] |= XISlaveRemoved;
    flags[XTestkeybd->id] |= XISlaveRemoved;
    flags[keybd->id] |= XIMasterRemoved;
    flags[ptr->id] |= XIMasterRemoved;

    RemoveDevice(XTestptr, FALSE);
    RemoveDevice(XTestkeybd, FALSE);
    RemoveDevice(keybd, FALSE);
    RemoveDevice(ptr, FALSE);

 unwind:
    return rc;
}

static int
detach_slave(ClientPtr client, xXIDetachSlaveInfo * c, int flags[MAXDEVICES])
{
    DeviceIntPtr dev;
    int rc;

    rc = dixLookupDevice(&dev, c->deviceid, client, DixManageAccess);
    if (rc != Success)
        goto unwind;

    if (InputDevIsMaster(dev)) {
        client->errorValue = c->deviceid;
        rc = BadDevice;
        goto unwind;
    }

    /* Don't allow changes to XTest Devices, these are fixed */
    if (IsXTestDevice(dev, NULL)) {
        client->errorValue = c->deviceid;
        rc = BadDevice;
        goto unwind;
    }

    ReleaseButtonsAndKeys(dev);
    AttachDevice(client, dev, NULL);
    flags[dev->id] |= XISlaveDetached;

 unwind:
    return rc;
}

static int
attach_slave(ClientPtr client, xXIAttachSlaveInfo * c, int flags[MAXDEVICES])
{
    DeviceIntPtr dev;
    DeviceIntPtr newmaster;
    int rc;

    rc = dixLookupDevice(&dev, c->deviceid, client, DixManageAccess);
    if (rc != Success)
        goto unwind;

    if (InputDevIsMaster(dev)) {
        client->errorValue = c->deviceid;
        rc = BadDevice;
        goto unwind;
    }

    /* Don't allow changes to XTest Devices, these are fixed */
    if (IsXTestDevice(dev, NULL)) {
        client->errorValue = c->deviceid;
        rc = BadDevice;
        goto unwind;
    }

    rc = dixLookupDevice(&newmaster, c->new_master, client, DixAddAccess);
    if (rc != Success)
        goto unwind;
    if (!InputDevIsMaster(newmaster)) {
        client->errorValue = c->new_master;
        rc = BadDevice;
        goto unwind;
    }

    if (!((IsPointerDevice(newmaster) && IsPointerDevice(dev)) ||
          (IsKeyboardDevice(newmaster) && IsKeyboardDevice(dev)))) {
        rc = BadDevice;
        goto unwind;
    }

    ReleaseButtonsAndKeys(dev);
    AttachDevice(client, dev, newmaster);
    flags[dev->id] |= XISlaveAttached;

 unwind:
    return rc;
}

int
ProcXIChangeHierarchy(ClientPtr client)
{
    xXIAnyHierarchyChangeInfo *any;
    size_t len;			/* length of data remaining in request */
    int rc = Success;
    int flags[MAXDEVICES] = { 0 };
    enum {
        NO_CHANGE,
        FLUSH,
        CHANGED,
    } changes = NO_CHANGE;

    X_REQUEST_HEAD_AT_LEAST(xXIChangeHierarchyReq);

    if (!stuff->num_changes)
        return rc;

    len = ((size_t)client->req_len << 2) - sizeof(xXIChangeHierarchyReq);

    any = (xXIAnyHierarchyChangeInfo *) &stuff[1];
    while (stuff->num_changes--) {
        if (len < sizeof(xXIAnyHierarchyChangeInfo)) {
            rc = BadLength;
            goto unwind;
        }

        if (client->swapped) {
            swaps(&any->type);
            swaps(&any->length);
        }

        if (len < ((size_t)any->length << 2))
            return BadLength;

#define CHANGE_SIZE_MATCH(type) \
    do { \
        if ((len < sizeof(type)) || (any->length != (sizeof(type) >> 2))) { \
            rc = BadLength; \
            goto unwind; \
        } \
    } while(0)

        switch (any->type) {
        case XIAddMaster:
        {
            xXIAddMasterInfo *c = (xXIAddMasterInfo *) any;

            /* Variable length, due to appended name string */
            if (len < sizeof(xXIAddMasterInfo)) {
                rc = BadLength;
                goto unwind;
            }

            if (client->swapped)
                swaps(&c->name_len);

            if (c->name_len > (len - sizeof(xXIAddMasterInfo))) {
                rc = BadLength;
                goto unwind;
            }

            rc = add_master(client, c, flags);
            if (rc != Success)
                goto unwind;
            changes = FLUSH;
            break;
        }
        case XIRemoveMaster:
        {
            xXIRemoveMasterInfo *r = (xXIRemoveMasterInfo *) any;

            CHANGE_SIZE_MATCH(xXIRemoveMasterInfo);
            rc = remove_master(client, r, flags);
            if (rc != Success)
                goto unwind;
            changes = FLUSH;
            break;
        }
        case XIDetachSlave:
        {
            xXIDetachSlaveInfo *c = (xXIDetachSlaveInfo *) any;

            CHANGE_SIZE_MATCH(xXIDetachSlaveInfo);
            rc = detach_slave(client, c, flags);
            if (rc != Success)
                goto unwind;
            changes = CHANGED;
            break;
        }
        case XIAttachSlave:
        {
            xXIAttachSlaveInfo *c = (xXIAttachSlaveInfo *) any;

            CHANGE_SIZE_MATCH(xXIAttachSlaveInfo);
            rc = attach_slave(client, c, flags);
            if (rc != Success)
                goto unwind;
            changes = CHANGED;
            break;
        }
        default:
            break;
        }

        if (changes == FLUSH) {
            XISendDeviceHierarchyEvent(flags);
            memset(flags, 0, sizeof(flags));
            changes = NO_CHANGE;
        }

        len -= any->length * 4;
        any = (xXIAnyHierarchyChangeInfo *) ((char *) any + any->length * 4);
    }

 unwind:
    if (changes != NO_CHANGE)
        XISendDeviceHierarchyEvent(flags);
    return rc;
}
