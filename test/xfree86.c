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
#include "xf86DDC.h"
#include "edid.h"

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

const testfunc_t*
xfree86_test(void)
{
    static const testfunc_t testfuncs[] = {
        xfree86_option_list_duplicate,
        xfree86_add_comment,
        xfree86_edid_ds_ranges_array_bound,
        NULL,
    };
    return testfuncs;
}
