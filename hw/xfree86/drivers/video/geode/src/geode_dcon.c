/* Copyright (c) 2006 Advanced Micro Devices, Inc.
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
#include <xorg-config.h>

/* Includes that are used by all drivers */
#include <xf86.h>
#include <xf86Modes.h>
#include <xf86_OSproc.h>
#include <compiler.h>

#include "geode.h"
#include <unistd.h>
#include <fcntl.h>

#define DCON_SLEEP_FILE "/sys/devices/platform/dcon/sleep"
#define DCON_FREEZE_FILE "/sys/devices/platform/dcon/freeze"

static Bool
dcon_present(void)
{
    static int _dval = -1;

    if (_dval == -1)
        _dval = (access("/sys/class/power_supply/olpc-ac", F_OK) == 0);

    return (Bool) _dval;
}

int
DCONDPMSSet(ScrnInfoPtr pScrni, int mode)
{
    static int failed = -1;
    ssize_t ret;
    int fd;
    char value[1];

    if (failed == -1)
        failed = !dcon_present();

    if (failed)
        return 0;

    /* If the DCON is frozen, don't power it down, it was probably frozen
     * for a reason and powering it down would corrupt the display.
     * This is needed to avoid losing OLPC's frozen boot image during X
     * startup, where DPMS is used to power down and up the display.
     * When geode uses KMS this will not be needed as the system realises
     * that no mode change is needed and the display power is untouched. */
    fd = open(DCON_FREEZE_FILE, O_RDONLY);
    if (fd < 0) {
        failed = 1;
        return 0;
    }

    ret = read(fd, value, 1);
    close(fd);
    if (ret == 1) {
        if (value[0] == '1')
            return 0;
    }

    fd = open(DCON_SLEEP_FILE, O_WRONLY);

    if (fd < 0) {
        failed = 1;
        return 0;
    }

    switch (mode) {
    case DPMSModeOn:
        value[0] = '0';
        break;
    case DPMSModeStandby:
    case DPMSModeSuspend:
    case DPMSModeOff:
        value[0] = '1';
        break;
    }

    ret = write(fd, value, sizeof(value));
    close(fd);

    if (ret < 0) {
        failed = 1;
        return 0;
    }

    return 1;
}

Bool
dcon_init(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    pGeode->mm_width = 0;
    pGeode->mm_height = 0;

    if (!dcon_present()) {
        xf86DrvMsg(pScrni->scrnIndex, X_DEFAULT, "No DCON is present\n");
        return FALSE;
    }

    pGeode->panelMode = XNFcallocarray(1, sizeof(DisplayModeRec));
    if (pGeode->panelMode == NULL)
        return FALSE;

    /* Set up the panel mode structure automagically */

    pGeode->panelMode->type = M_T_DRIVER | M_T_PREFERRED;
    pGeode->panelMode->Clock = 57275;
    pGeode->panelMode->HDisplay = 1200;
    pGeode->panelMode->HSyncStart = 1208;
    pGeode->panelMode->HSyncEnd = 1216;
    pGeode->panelMode->HTotal = 1240;
    pGeode->panelMode->VDisplay = 900;
    pGeode->panelMode->VSyncStart = 905;
    pGeode->panelMode->VSyncEnd = 908;
    pGeode->panelMode->VTotal = 912;
    pGeode->panelMode->Flags = V_NHSYNC | V_NVSYNC;

    pGeode->mm_width = 152;
    pGeode->mm_height = 114;

    xf86SetModeDefaultName(pGeode->panelMode);

    /* TODO: Print board revision once sysfs exports it. */
    xf86DrvMsg(pScrni->scrnIndex, X_DEFAULT, "DCON detected.\n");

    return TRUE;
}
