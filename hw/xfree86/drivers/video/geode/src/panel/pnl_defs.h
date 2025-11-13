/* Copyright (c) 2005 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Neither the name of the Advanced Micro Devices, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 * */

/* 
 * File Contents:   This file contains definitions of the Geode frame buffer
 *                  panel data structures.
 *
 * SubModule:       Geode FlatPanel library
 * */

#ifndef _pnl_defs_h
#define _pnl_defs_h

typedef enum {
    MARMOT_PLATFORM = 0,
    UNICORN_PLATFORM,
    CENTAURUS_PLATFORM,
    ARIES_PLATFORM,
    CARMEL_PLATFORM,
    HYDRA_PLATFORM,
    DORADO_PLATFORM,
    DRACO_PLATFORM,
    REDCLOUD_PLATFORM,
    OTHER_PLATFORM
} SYS_BOARD;

#define PNL_9210             0x01
#define PNL_9211_A           0x02
#define PNL_9211_C           0x04
#define PNL_UNKNOWN_CHIP     0x08

#define PNL_TFT              0x01
#define PNL_SSTN             0x02
#define PNL_DSTN             0x04
#define PNL_TWOP             0x08
#define PNL_UNKNOWN_PANEL    0x10

#define PNL_MONO_PANEL       0x01
#define PNL_COLOR_PANEL      0x02
#define PNL_UNKNOWN_COLOR    0x08

#define PNL_PANELPRESENT     0x01
#define PNL_PLATFORM         0x02
#define PNL_PANELCHIP        0x04
#define PNL_PANELSTAT        0x08
#define PNL_OVERRIDE_STAT    0x10
#define PNL_OVERRIDE_ALL     0x1F

typedef struct _Pnl_PanelStat_ {
    int Type;
    int XRes;
    int YRes;
    int Depth;
    int MonoColor;
} Pnl_PanelStat;

typedef struct _Pnl_Params_ {
    unsigned long Flags;
    int PanelPresent;
    int Platform;
    int PanelChip;
    Pnl_PanelStat PanelStat;
} Pnl_PanelParams, *PPnl_PanelParams;

#endif                          /* _pnl_defs_h */

/* END OF FILE */
