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
#include <X11/Xatom.h>

#include "dix/atom_priv.h"
#include "dix/dix_priv.h"
#include "dix/input_priv.h"
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
#include "Xext/xtest/xtest_priv.h"

/**
 */

/* from Xext/xtest.c */
extern DeviceIntPtr xtestpointer, xtestkeyboard;

/* Needed for the screen setup, otherwise we crash during sprite initialization */
static Bool
device_cursor_init(DeviceIntPtr dev, ScreenPtr screen)
{
    return TRUE;
}

static void
device_cursor_cleanup(DeviceIntPtr dev, ScreenPtr screen)
{
}

static void
xtest_init(void)
{
    static ScreenRec screen = {0};
    static ClientRec server_client = {0};
    static WindowRec root = {{0}};
    static WindowOptRec optional = {0};

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
    InitAtoms();
    SyncExtensionInit();

    /* this also inits the xtest devices */
    InitCoreDevices();
}

static void
xtest_cleanup(void)
{
    CloseDownDevices();
}

static void
xtest_init_devices(void)
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
static void
xtest_properties(void)
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

static void
xtest_valuator_bounds(void)
{
    const int sentinel = 0x12345678;
    int valuators[MAX_VALUATORS + 1];
    deviceValuator dv = {
        .num_valuators = 1,
        .valuator0 = 0xdeadbeef,
    };
    int i;

    for (i = 0; i <= MAX_VALUATORS; i++)
        valuators[i] = sentinel;

    /*
     * This is the vulnerable equality boundary: firstValuator == numAxes
     * == MAX_VALUATORS. The old code accepted it and wrote valuators[36]
     * before rejecting the request. Keep a sentinel immediately after the
     * real array to make that write observable in this unit test.
     */
    assert(XTestCopyValuators(valuators, MAX_VALUATORS, MAX_VALUATORS,
                              0, &dv) == BadValue);
    assert(valuators[MAX_VALUATORS] == sentinel);

    /* The final six slots are valid, but the following slot is not. */
    dv.num_valuators = 6;
    dv.valuator0 = 10;
    dv.valuator1 = 11;
    dv.valuator2 = 12;
    dv.valuator3 = 13;
    dv.valuator4 = 14;
    dv.valuator5 = 15;
    assert(XTestCopyValuators(valuators, MAX_VALUATORS - 6,
                              MAX_VALUATORS, 0, &dv) == Success);
    for (i = 0; i < 6; i++)
        assert(valuators[MAX_VALUATORS - 6 + i] == 10 + i);
    assert(valuators[MAX_VALUATORS] == sentinel);

    /* One more value would cross both the device and stack-array bounds. */
    for (i = 0; i <= MAX_VALUATORS; i++)
        valuators[i] = sentinel;
    assert(XTestCopyValuators(valuators, MAX_VALUATORS - 5,
                              MAX_VALUATORS, 0, &dv) == BadValue);
    assert(valuators[MAX_VALUATORS - 5] == sentinel);
    assert(valuators[MAX_VALUATORS] == sentinel);
}

const testfunc_t*
xtest_test(void)
{
    static const testfunc_t testfuncs[] = {
        xtest_init_devices,
        xtest_properties,
        xtest_valuator_bounds,
        NULL,
    };
    return testfuncs;
}
