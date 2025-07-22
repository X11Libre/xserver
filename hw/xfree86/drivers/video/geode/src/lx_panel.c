/* Copyright (c) 2008 Advanced Micro Devices, Inc.
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
 */

/* Reference: Video Graphics Suite Specification:
 * VG Config Register (0x00) page 16
 * VG FP Register (0x02) page 18
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include "xf86.h"
#include "compiler.h"
#include "xf86Modes.h"
#include "geode.h"

#define LX_READ_VG(reg) \
                (outw(0xAC1C,0xFC53), outw(0xAC1C,0x0200|(reg)), inw(0xAC1E))

/* This is borrowed from xerver/hw/xfree86/modes */

#define MODEPREFIX NULL, NULL, NULL, 0, M_T_DRIVER
#define MODESUFFIX 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,FALSE,FALSE,0,NULL,0,0.0,0.0

DisplayModeRec lx_panel_modes[] = {
    {MODEPREFIX, 31200, 320, 354, 384, 400, 0, 240, 249, 253, 260, 0,
     V_NHSYNC | V_NVSYNC, MODESUFFIX}
    ,                           /* 320x200@75 */
    {MODEPREFIX, 25175, 640, 656, 744, 800, 0, 480, 490, 492, 525, 0,
     V_NHSYNC | V_NVSYNC, MODESUFFIX}
    ,                           /* 640x480@60 */
    {MODEPREFIX, 40000, 800, 840, 968, 1056, 0, 600, 601, 605, 628, 0,
     V_NHSYNC | V_NVSYNC, MODESUFFIX}
    ,                           /* 880x600@60 */
    {MODEPREFIX, 65000, 1024, 1048, 1184, 1344, 0, 768, 771, 777, 806, 0,
     V_NHSYNC | V_NVSYNC, MODESUFFIX}
    ,                           /* 1024x768@60 */
    {MODEPREFIX, 81600, 1152, 1216, 1336, 1520, 0, 864, 865, 868, 895, 0,
     V_NHSYNC | V_NVSYNC, MODESUFFIX}
    ,                           /* 1152x864@60 */
    {MODEPREFIX, 108000, 1280, 1328, 1440, 1688, 0, 1024, 1025, 1028, 1066, 0,
     V_NHSYNC | V_NVSYNC, MODESUFFIX}
    ,                           /* 1280x1024@60 */
    {MODEPREFIX, 162000, 1600, 1664, 1856, 2160, 0, 1200, 1201, 1204, 1250, 0,
     V_NHSYNC | V_NVSYNC, MODESUFFIX}
    ,                           /* 1600x1200@60 */
    {MODEPREFIX, 48960, 1024, 1064, 1168, 1312, 0, 600, 601, 604, 622, 0,
     V_NHSYNC | V_NVSYNC, MODESUFFIX}
    ,                           /* 1024x600@60 wide panels */
};

/* Get the legacy panel size from VSA, and return the associated mode rec */

DisplayModePtr
LXGetLegacyPanelMode(ScrnInfoPtr pScrni)
{
    unsigned short reg = LX_READ_VG(0x00);
    unsigned char ret = (reg >> 8) & 0x07;

    if ((ret == 1 || ret == 5)) {

        reg = LX_READ_VG(0x02);
        ret = (reg >> 3) & 0x07;

        /* FIXME: 7 is reserved in default. We use this value to support
         * wide screen resolution 1024x600@80 now for panel. If you want to use
         * that resolution, please assign ret to 7 manually here:
         * "reg = 7"
         * The user can use this entry for other wide screen resolutions.
         */

        if (ret < 8) {
            xf86DrvMsg(pScrni->scrnIndex, X_INFO,
                       " VSA Panel Mode is: %dx%d, pixel clock freq(kHz) is %d\n",
                       lx_panel_modes[ret].HDisplay,
                       lx_panel_modes[ret].VDisplay, lx_panel_modes[ret].Clock);
            return &lx_panel_modes[ret];
        }

    }

    return NULL;
}

/* Construct a moderec from the specified panel mode */

DisplayModePtr
LXGetManualPanelMode(const char *modestr)
{
    int clock;
    int hactive, hsstart, hsend, htotal;
    int vactive, vsstart, vsend, vtotal;
    DisplayModePtr mode;
    char sname[32];

    int ret = sscanf(modestr, "%d %d %d %d %d %d %d %d %d",
                     &clock,
                     &hactive, &hsstart, &hsend, &htotal,
                     &vactive, &vsstart, &vsend, &vtotal);

    if (ret != 9)
        return NULL;

    mode = XNFcallocarray(1, sizeof(DisplayModeRec));

    if (mode == NULL)
        return NULL;

    sprintf(sname, "%dx%d", hactive, vactive);

    mode->name = (char *) XNFalloc(strlen(sname) + 1);
    strcpy((char *) mode->name, sname);

    mode->type = M_T_DRIVER | M_T_PREFERRED;
    mode->Clock = clock;
    mode->HDisplay = hactive;
    mode->HSyncStart = hsstart;
    mode->HSyncEnd = hsend;
    mode->HTotal = htotal;
    mode->VDisplay = vactive;
    mode->VSyncStart = vsstart;
    mode->VSyncEnd = vsend;
    mode->VTotal = vtotal;

    mode->prev = mode->next = NULL;

    return mode;
}
