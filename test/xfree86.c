/**
 * Copyright © 2011 Red Hat, Inc.
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

#include <dix-config.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "xf86.h"
#include "xf86Parser.h"
#include "xf86DDC_priv.h"
#include "edid.h"

#ifdef XSERVER_PLATFORM_BUS
#include "xf86platformBus_priv.h"
#include "config/hotplug_priv.h"
#endif
#if defined(XSERVER_PLATFORM_BUS) && defined(XSERVER_LIBPCIACCESS)
#include "xf86Bus.h"
#include "xf86Priv.h"
#endif

#include "tests-common.h"

static void
edid_set_checksum(unsigned char *block)
{
    int i, sum = 0;

    for (i = 0; i < 127; i++)
        sum += block[i];
    block[127] = (unsigned char) ((256 - (sum & 0xff)) & 0xff);
}

static void
fill_range_descriptor(unsigned char *d, int min_h, int max_h, int min_v, int max_v)
{
    memset(d, 0, 18);
    d[3] = 0xFD;                /* DS_RANGES */
    d[5] = (unsigned char) min_v;
    d[6] = (unsigned char) max_v;
    d[7] = (unsigned char) min_h;
    d[8] = (unsigned char) max_h;
    d[9] = 20;                  /* max pixel clock ~205 MHz */
    d[11] = 0x0A;               /* padded with spaces per EDID */
    memset(d + 12, 0x20, 6);
}

/*
 * Craft an EDID with more Display-Range (DS_RANGES) descriptors than
 * MonRec.hsync[] / vrefresh[] can hold: 4 in the base block plus 6 in a
 * CEA-861 extension (CEA_EXT_DET_TIMING_NUM).
 */
static void
build_overlong_ranges_edid(unsigned char *edid)
{
    int i;

    memset(edid, 0, 256);

    /* Header */
    edid[0] = 0x00;
    memset(edid + 1, 0xFF, 6);
    edid[7] = 0x00;

    /* EDID 1.3 */
    edid[18] = 1;
    edid[19] = 3;

    /* Digital input, 30x20 cm */
    edid[20] = 0x80;
    edid[21] = 30;
    edid[22] = 20;
    edid[23] = 0x78;

    /* Four range descriptors in the base detailed-timing slots (offset 54) */
    for (i = 0; i < 4; i++)
        fill_range_descriptor(edid + 54 + i * 18, 30 + i, 80 + i, 50 + i, 75 + i);

    edid[126] = 1;              /* one extension block */
    edid_set_checksum(edid);

    /* CEA-861 extension: no data blocks, six detailed descriptors */
    edid[128] = 0x02;           /* CEA_EXT */
    edid[129] = 0x03;
    edid[130] = 4;              /* dt_offset: descriptors start immediately */
    edid[131] = 0;
    for (i = 0; i < 6; i++)
        fill_range_descriptor(edid + 132 + i * 18,
                              40 + i, 90 + i, 60 + i, 85 + i);
    edid_set_checksum(edid + 128);
}

static void
xfree86_edid_ds_ranges_array_bound(void)
{
    unsigned char edid[256];
    xf86MonPtr ddc;
    MonRec monitor = { 0 };
    DisplayModePtr mode;

    build_overlong_ranges_edid(edid);

    ddc = xf86InterpretEEDID(0, edid);
    assert(ddc);
    assert(ddc->no_sections == 1);

    xf86EdidMonitorSet(0, &monitor, ddc);

    assert(monitor.nHsync <= MAX_HSYNC);
    assert(monitor.nVrefresh <= MAX_VREFRESH);
    assert(monitor.nHsync == MAX_HSYNC);
    assert(monitor.nVrefresh == MAX_VREFRESH);

    while (monitor.Modes) {
        mode = monitor.Modes;
        xf86DeleteMode(&monitor.Modes, mode);
    }
    free(ddc);
}

static void
xfree86_option_list_duplicate(void)
{
    XF86OptionPtr options;
    XF86OptionPtr duplicate;
    const char *o1 = "foo", *o2 = "bar", *v1 = "one", *v2 = "two";
    const char *o_null = "NULL";
    char *val1, *val2;
    XF86OptionPtr a, b;

    duplicate = xf86OptionListDuplicate(NULL);
    assert(!duplicate);

    options = xf86AddNewOption(NULL, o1, v1);
    assert(options);
    options = xf86AddNewOption(options, o2, v2);
    assert(options);
    options = xf86AddNewOption(options, o_null, NULL);
    assert(options);

    duplicate = xf86OptionListDuplicate(options);
    assert(duplicate);

    val1 = xf86CheckStrOption(options, o1, "1");
    val2 = xf86CheckStrOption(duplicate, o1, "2");

    assert(strcmp(val1, v1) == 0);
    assert(strcmp(val1, val2) == 0);
    free(val1);
    free(val2);

    val1 = xf86CheckStrOption(options, o2, "1");
    val2 = xf86CheckStrOption(duplicate, o2, "2");

    assert(strcmp(val1, v2) == 0);
    assert(strcmp(val1, val2) == 0);
    free(val1);
    free(val2);

    a = xf86FindOption(options, o_null);
    b = xf86FindOption(duplicate, o_null);
    assert(a);
    assert(b);

    xf86OptionListFree(duplicate);
    xf86OptionListFree(options);
}

static void
xfree86_add_comment(void)
{
    char *current = NULL;
    const char *comment;
    char compare[1024] = { 0 };

    comment = "# foo";
    current = xf86addComment(current, comment);
    strcpy(compare, comment);
    strcat(compare, "\n");

    assert(!strcmp(current, compare));

    /* this used to overflow */
    strcpy(current, "\n");
    comment = "foobar\n";
    current = xf86addComment(current, comment);
    strcpy(compare, "\n#");
    strcat(compare, comment);
    assert(!strcmp(current, compare));

    free(current);
}

/*
 * A platform device without a bus (e.g. vkms) has no busid. Scanning the
 * PCI devices, as GPU hot-plug does, must skip it and not crash.
 */
static void
xfree86_platform_scan_no_busid(void)
{
#ifdef XSERVER_PLATFORM_BUS
    struct OdevAttributes *attribs = config_odev_allocate_attributes();
    int n = xf86_num_platform_devices;

    attribs->path = strdup("/dev/dri/card9");
    assert(attribs->path);
    assert(attribs->busid == NULL);
    xf86_add_platform_device(attribs, FALSE);

    xf86PlatformScanPciDev();
    assert(xf86_platform_devices[n]->pdev == NULL);

    xf86_remove_platform_device(n);
    assert(xf86_num_platform_devices == n);
#endif
}

/*
 * A GPU screen claims its card without a Device section (numInstances 0).
 * Its slot must still count as claimed. Entities stay in xf86Entities after
 * the test, so everything they point to is static and released at the end.
 */
static void
xfree86_check_slot_gpu_screen(void)
{
#if defined(XSERVER_PLATFORM_BUS) && defined(XSERVER_LIBPCIACCESS)
    static DriverRec testdrv = { .driverName = "testdrv" };
    static DriverRec msdrv = { .driverName = "modesetting" };
    static struct pci_device pci = { .domain = 0x7fff, .bus = 1, .dev = 2, .func = 0 };
    static struct pci_device pci2 = { .domain = 0x7fff, .bus = 1, .dev = 2, .func = 0 };
    static char path7[] = "/dev/dri/card7";
    static char path9[] = "/dev/dri/card9";
    static struct OdevAttributes attr7 = { .path = path7 };
    static struct OdevAttributes attr9 = { .path = path9 };
    static struct xf86_platform_device plat = { .attribs = &attr7, .pdev = &pci };
    static struct xf86_platform_device plat2 = { .attribs = &attr9, .pdev = &pci2 };
    static GDevRec fbms = { .identifier = "fb-ms", .driver = "modesetting" };
    GDevRec ms = { .identifier = "ms", .driver = "modesetting" };
    GDevRec ms8 = { .identifier = "ms8", .driver = "modesetting" };
    GDevRec fbdev = { .identifier = "fbdev", .driver = "fbdev" };
    int e, f;

    ms.options = xf86AddNewOption(NULL, "kmsdev", "/dev/dri/card7");
    ms8.options = xf86AddNewOption(NULL, "kmsdev", "/dev/dri/card8");
    fbms.options = xf86AddNewOption(NULL, "kmsdev", "/dev/dri/card7");

    /* A GPU screen: platform claim, no Device section */
    e = xf86AllocateEntity();
    xf86Entities[e]->driver = &testdrv;
    xf86Entities[e]->bus.type = BUS_PLATFORM;
    xf86Entities[e]->bus.id.plat = &plat;
    assert(xf86Entities[e]->numInstances == 0);

    /* Same PCI address through another node, and as a PCI device */
    assert(!xf86CheckSlot(&plat2, BUS_PLATFORM));
    assert(!xf86CheckPciSlot(&pci));
    /* modesetting fb claim of the same node */
    assert(!xf86CheckSlot(&ms, BUS_NONE));
    /* autoconfigured fbdev must be alone */
    assert(!xf86CheckSlot(&fbdev, BUS_NONE));
    /* another node is free */
    assert(xf86CheckSlot(&ms8, BUS_NONE));

    /* Released: the slot is free again */
    xf86Entities[e]->bus.type = BUS_NONE;
    assert(xf86CheckSlot(&plat2, BUS_PLATFORM));
    assert(xf86CheckSlot(&ms, BUS_NONE));

    /* An fb claim of a node blocks a platform claim of it (#1505) */
    f = xf86AllocateEntity();
    xf86Entities[f]->driver = &msdrv;
    xf86Entities[f]->bus.type = BUS_NONE;
    xf86AddDevToEntity(f, &fbms);
    assert(!xf86CheckSlot(&plat, BUS_PLATFORM));
    xf86RemoveDevFromEntity(f, &fbms);
    assert(xf86Entities[f]->numInstances == 0);
    assert(xf86CheckSlot(&plat, BUS_PLATFORM));

    xf86OptionListFree(fbms.options);
    xf86OptionListFree(ms8.options);
    xf86OptionListFree(ms.options);
#endif
}

/*
 * Entities and drivers keep pointers to platform devices, so a device must
 * keep its address while other devices are added (the list grows) and
 * removed (the list shrinks).
 */
static void
xfree86_platform_device_stable(void)
{
#ifdef XSERVER_PLATFORM_BUS
    enum { NDEV = 64 };
    struct xf86_platform_device *devs[NDEV];
    int base = xf86_num_platform_devices;
    int i;

    for (i = 0; i < NDEV; i++) {
        struct OdevAttributes *attribs = config_odev_allocate_attributes();

        attribs->major = 226;
        attribs->minor = 100 + i;
        xf86_add_platform_device(attribs, FALSE);
        devs[i] = xf86_platform_devices[base + i];
    }

    /* adding grew the list many times; earlier devices didn't move */
    for (i = 0; i < NDEV; i++) {
        assert(xf86_platform_devices[base + i] == devs[i]);
        assert(devs[i]->attribs->minor == 100 + i);
    }

    /* removing one from the middle doesn't move the others either */
    xf86_remove_platform_device(base + NDEV / 2);
    for (i = 0; i < NDEV; i++) {
        if (i == NDEV / 2)
            continue;
        assert(xf86_find_platform_device_by_devnum(226, 100 + i) == devs[i]);
        assert(devs[i]->attribs->minor == 100 + i);
    }

    while (xf86_num_platform_devices > base)
        xf86_remove_platform_device(base);
#endif
}

const testfunc_t*
xfree86_test(void)
{
    static const testfunc_t testfuncs[] = {
        xfree86_option_list_duplicate,
        xfree86_add_comment,
        xfree86_edid_ds_ranges_array_bound,
        xfree86_platform_scan_no_busid,
        xfree86_check_slot_gpu_screen,
        xfree86_platform_device_stable,
        NULL,
    };
    return testfuncs;
}
