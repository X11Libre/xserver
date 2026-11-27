/**
 * Copyright © 2009 Red Hat, Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice (including the next
 *  paragraph) shall be included in all copies or substantial portions of the
 *  Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 */

/* Test relies on assert() */
#undef NDEBUG

#include <assert.h>
#include <dix-config.h>

#include <stdint.h>
#include <stdlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/XIproto.h>
#include <X11/extensions/xtestproto.h>

#include "dix/atom_priv.h"
#include "dix/dix_priv.h"
#include "dix/input_priv.h"
#include "include/dixstruct.h"
#include "include/extnsionst.h"
#include "Xext/xinput/exglobals.h"
#include "miext/extinit_priv.h"

#include "input.h"
#include "inputstr.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "exevents.h"
#include "xkbsrv.h"
#include "xserver-properties.h"
#include "Xext/sync/syncsrv.h"
#include "tests-common.h"

/**
 */

/* from Xext/xtest.c */
extern DeviceIntPtr xtestpointer, xtestkeyboard;

/* Needed for the screen setup, otherwise we crash during sprite initialization */
static Bool device_cursor_init(DeviceIntPtr dev, ScreenPtr screen)
{
    return TRUE;
}

static void device_cursor_cleanup(DeviceIntPtr dev, ScreenPtr screen)
{
}

static void xtest_init(void)
{
    static ScreenRec screen = {0};
    static ClientRec server_client = {0};
    static WindowRec root = {{0}};
    static WindowOptRec optional = {0};
    /* This unit fixture has no DDX input queue. Keep stable storage for the
     * pointers retained by SetInputCheck(), and report that no input is
     * pending so UpdateCurrentTime() never calls an uninitialized
     * ProcessInputEvents() hook. */
    static HWEventQueueType noInput[2] = { 0, 0 };

    /* random stuff that needs initialization */
    root.drawable.id = 0xab;
    root.optional = &optional;
    screen.root = &root;
    screenInfo.numScreens = 1;
    screenInfo.screens[0] = &screen;
    screen.myNum = 0;
    screen.id = 100;
    screen.width = 640;
    screen.height = 480;
    screen.DeviceCursorInitialize = device_cursor_init;
    screen.DeviceCursorCleanup = device_cursor_cleanup;
    dixResetPrivates();
    serverClient = &server_client;
    InitClient(serverClient, 0, (void *) NULL);
    if (!InitClientResources(serverClient)) /* for root resources */
        FatalError("couldn't init server resources");
    SetInputCheck(&noInput[0], &noInput[1]);
    InitAtoms();
    SyncExtensionInit();

    /* this also inits the xtest devices */
    InitCoreDevices();
}

static void xtest_cleanup(void)
{
    CloseDownDevices();
}

static void xtest_init_devices(void)
{
    xtest_init();

    assert(xtestpointer);
    assert(xtestkeyboard);
    assert(IsXTestDevice(xtestpointer, NULL));
    assert(IsXTestDevice(xtestkeyboard, NULL));
    assert(IsXTestDevice(xtestpointer, inputInfo.pointer));

    assert(IsXTestDevice(xtestkeyboard, inputInfo.keyboard));
    assert(GetXTestDevice(inputInfo.pointer) == xtestpointer);

    assert(GetXTestDevice(inputInfo.keyboard) == xtestkeyboard);

    xtest_cleanup();
}

/**
 * Each xtest devices has a property attached marking it. This property
 * cannot be changed.
 */
static void xtest_properties(void)
{
    int rc;
    char value = 1;
    XIPropertyValuePtr prop;
    Atom xtest_prop;

    xtest_init();

    xtest_prop = XIGetKnownProperty(XI_PROP_XTEST_DEVICE);
    rc = XIGetDeviceProperty(xtestpointer, xtest_prop, &prop);
    assert(rc == Success);
    assert(prop);

    rc = XIGetDeviceProperty(xtestkeyboard, xtest_prop, &prop);
    assert(rc == Success);
    assert(prop != NULL);

    rc = XIChangeDeviceProperty(xtestpointer, xtest_prop,
                                XA_INTEGER, 8, PropModeReplace, 1, &value,
                                FALSE);
    assert(rc == BadAccess);
    rc = XIChangeDeviceProperty(xtestkeyboard, xtest_prop,
                                XA_INTEGER, 8, PropModeReplace, 1, &value,
                                FALSE);
    assert(rc == BadAccess);

    xtest_cleanup();
}

/**
 * Exercise the XTestFakeInput request handler at the valuator-array boundary.
 *
 * A real device may have MAX_VALUATORS axes. The request below starts at
 * first_valuator == numAxes == MAX_VALUATORS and supplies one valuator. The
 * request must be rejected before ProcXTestFakeInput writes to its local
 * valuators[MAX_VALUATORS] array. AddressSanitizer makes the historical
 * write-before-reject regression observable.
 */
static void xtest_valuator_request_bounds(void)
{
    Atom labels[MAX_VALUATORS] = { 0 };
    struct {
        xXTestFakeInputReq header;
        deviceValuator valuator;
    } request = { 0 };
    ExtensionEntry *xtest_ext;
    int rc;

    xtest_init();

    /* Register the real XI event numbers and XTEST request dispatcher. */
    XInputExtensionInit();
    XTestExtensionInit();

    assert(xtestpointer);
    assert(xtestpointer->valuator);

    /* Reuse the registered XTEST device, but give it the maximum valid axis
     * count. This keeps dixLookupDevice() on the real device lookup path. */
    FreeDeviceClass(ValuatorClass, (void **) &xtestpointer->valuator);
    free(xtestpointer->last.scroll);
    xtestpointer->last.scroll = NULL;
    assert(InitValuatorClassDeviceStruct(xtestpointer, MAX_VALUATORS,
                                         labels, 0, Absolute));
    assert(xtestpointer->valuator->numAxes == MAX_VALUATORS);

    xtest_ext = CheckExtension("XTEST");
    assert(xtest_ext);

    /* xXTestFakeInputReq contains the first 32-byte event after its xReq
     * header; the following deviceValuator is the second event. */
    assert(sizeof(request) % 4 == 0);
    assert((char *) &request.valuator ==
           (char *) &request + sizeof(request.header));
    request.header.reqType = xtest_ext->base;
    request.header.xtReqType = X_XTestFakeInput;
    request.header.length = sizeof(request) / 4;
    request.header.type = DeviceMotionNotify;
    request.header.detail = xFalse;
    request.header.deviceid = xtestpointer->id;
    request.valuator.type = DeviceValuator;
    request.valuator.deviceid = xtestpointer->id;
    request.valuator.num_valuators = 1;
    request.valuator.first_valuator = MAX_VALUATORS;
    request.valuator.valuator0 = 0x12345678;

    /* Enter through the extension's registered request dispatch function;
     * ProcXTestFakeInput remains static and no production test hook is used. */
    serverClient->requestVector = ProcVector;
    serverClient->requestBuffer = (char *) &request;
    serverClient->req_len = request.header.length;
    serverClient->majorOp = xtest_ext->base;
    serverClient->minorOp = X_XTestFakeInput;
    serverClient->swapped = FALSE;
    serverClient->errorValue = 0;

    rc = serverClient->requestVector[xtest_ext->base](serverClient);
    assert(rc == BadValue);

    serverClient->requestBuffer = NULL;
    xtest_cleanup();
}

const testfunc_t* xtest_test(void)
{
    static const testfunc_t testfuncs[] = {
        xtest_init_devices,
        xtest_properties,
        xtest_valuator_request_bounds,
        NULL,
    };
    return testfuncs;
}
