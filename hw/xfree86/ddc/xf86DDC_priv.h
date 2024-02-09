/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 * Copyright © 1998 by Egbert Eich <Egbert.Eich@Physik.TU-Darmstadt.DE>
 */
#ifndef _XSERVER_XF86_DDC_PRIV_H
#define _XSERVER_XF86_DDC_PRIV_H

#include "xf86DDC.h"

/*
 * Quirks to work around broken EDID data from various monitors.
 */
typedef enum {
    DDC_QUIRK_NONE = 0,
    /* First detailed mode is bogus, prefer largest mode at 60hz */
    DDC_QUIRK_PREFER_LARGE_60 = 1 << 0,
    /* 135MHz clock is too high, drop a bit */
    DDC_QUIRK_135_CLOCK_TOO_HIGH = 1 << 1,
    /* Prefer the largest mode at 75 Hz */
    DDC_QUIRK_PREFER_LARGE_75 = 1 << 2,
    /* Convert detailed timing's horizontal from units of cm to mm */
    DDC_QUIRK_DETAILED_H_IN_CM = 1 << 3,
    /* Convert detailed timing's vertical from units of cm to mm */
    DDC_QUIRK_DETAILED_V_IN_CM = 1 << 4,
    /* Detailed timing descriptors have bogus size values, so just take the
     * maximum size and use that.
     */
    DDC_QUIRK_DETAILED_USE_MAXIMUM_SIZE = 1 << 5,
    /* Monitor forgot to set the first detailed is preferred bit. */
    DDC_QUIRK_FIRST_DETAILED_PREFERRED = 1 << 6,
    /* use +hsync +vsync for detailed mode */
    DDC_QUIRK_DETAILED_SYNC_PP = 1 << 7,
    /* Force single-link DVI bandwidth limit */
    DDC_QUIRK_DVI_SINGLE_LINK = 1 << 8,
} ddc_quirk_t;

typedef void (*handle_detailed_fn) (struct detailed_monitor_section *, void *);

void xf86ForEachDetailedBlock(xf86MonPtr mon, handle_detailed_fn, void *data);

ddc_quirk_t xf86DDCDetectQuirks(int scrnIndex, xf86MonPtr DDC, Bool verbose);

void xf86DetTimingApplyQuirks(struct detailed_monitor_section *det_mon,
                              ddc_quirk_t quirks, int hsize, int vsize);

typedef void (*handle_video_fn) (struct cea_video_block *, void *);

void xf86ForEachVideoBlock(xf86MonPtr, handle_video_fn, void *);

struct cea_data_block *xf86MonitorFindHDMIBlock(xf86MonPtr mon);

#endif /* _XSERVER_XF86_DDC_PRIV_H */
