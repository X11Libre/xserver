/* Based on hw/xfree86/os-support/bsd/bsd_apm.c which bore no explicit
 * copyright notice, so is covered by the following notice:
 *
 * Copyright (C) 1994-2003 The XFree86 Project, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 */

/* Copyright (c) 2005, Oracle and/or its affiliates.
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
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <errno.h>
#include <sys/srn.h>
#include <X11/X.h>

#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_os_support.h"
#include "xf86_OSproc.h"
#include "xf86_OSlib.h"

typedef struct apm_event_info {
    int type;
} apm_event_info;

#define APM_DEVICE "/dev/srn"

static void *APMihPtr = NULL;
static void sunCloseAPM(void);

static struct {
    u_int apmBsd;
    pmEvent xf86;
} sunToXF86Array[] = {
    {SRN_STANDBY_REQ, XF86_APM_SYS_STANDBY},
    {SRN_SUSPEND_REQ, XF86_APM_SYS_SUSPEND},
    {SRN_NORMAL_RESUME, XF86_APM_NORMAL_RESUME},
    {SRN_CRIT_RESUME, XF86_APM_CRITICAL_RESUME},
    {SRN_BATTERY_LOW, XF86_APM_LOW_BATTERY},
    {SRN_POWER_CHANGE, XF86_APM_POWER_STATUS_CHANGE},
    {SRN_UPDATE_TIME, XF86_APM_UPDATE_TIME},
    {SRN_CRIT_SUSPEND_REQ, XF86_APM_CRITICAL_SUSPEND},
    {SRN_USER_STANDBY_REQ, XF86_APM_USER_STANDBY},
    {SRN_USER_SUSPEND_REQ, XF86_APM_USER_SUSPEND},
    {SRN_SYS_STANDBY_RESUME, XF86_APM_STANDBY_RESUME},
};

static pmEvent
sunToXF86(int type)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(sunToXF86Array); i++) {
        if (type == sunToXF86Array[i].apmBsd) {
            return sunToXF86Array[i].xf86;
        }
    }
    return XF86_APM_UNKNOWN;
}

/*
 * APM events can be requested directly from /dev/apm
 */
static int
sunPMGetEventFromOS(int fd, pmEvent * events, int num)
{
    struct apm_event_info sunEvent;
    int i;

    for (i = 0; i < num; i++) {

        if (ioctl(fd, SRN_IOC_NEXTEVENT, &sunEvent) < 0) {
            if (errno != EAGAIN) {
                LogMessageVerb(X_WARNING, 1, "sunPMGetEventFromOS: SRN_IOC_NEXTEVENT"
                               " %s\n", strerror(errno));
            }
            break;
        }
        events[i] = sunToXF86(sunEvent.type);
    }
    LogMessageVerb(X_WARNING, 1, "Got some events\n");
    return i;
}

static pmWait
sunPMConfirmEventToOs(int fd, pmEvent event)
{
    switch (event) {
/* XXX: NOT CURRENTLY RETURNED FROM OS */
    case XF86_APM_SYS_STANDBY:
    case XF86_APM_USER_STANDBY:
        if (ioctl(fd, SRN_IOC_STANDBY, NULL) == 0)
            return PM_WAIT;     /* should we stop the Xserver in standby, too? */
        else
            return PM_NONE;
    case XF86_APM_SYS_SUSPEND:
    case XF86_APM_CRITICAL_SUSPEND:
    case XF86_APM_USER_SUSPEND:
        LogMessageVerb(X_WARNING, 1, "Got SUSPENDED\n");
        if (ioctl(fd, SRN_IOC_SUSPEND, NULL) == 0)
            return PM_CONTINUE;
        else {
            LogMessageVerb(X_WARNING, 1, "sunPMConfirmEventToOs: SRN_IOC_SUSPEND"
                    " %s\n", strerror(errno));
            return PM_FAILED;
        }
    case XF86_APM_STANDBY_RESUME:
    case XF86_APM_NORMAL_RESUME:
    case XF86_APM_CRITICAL_RESUME:
    case XF86_APM_STANDBY_FAILED:
    case XF86_APM_SUSPEND_FAILED:
        LogMessageVerb(X_WARNING, 1, "Got RESUME\n");
        if (ioctl(fd, SRN_IOC_RESUME, NULL) == 0)
            return PM_CONTINUE;
        else {
            LogMessageVerb(X_WARNING, 1, "sunPMConfirmEventToOs: SRN_IOC_RESUME"
                    " %s\n", strerror(errno));
            return PM_FAILED;
        }
    default:
        return PM_NONE;
    }
}

PMClose
xf86OSPMOpen(void)
{
    int fd;

    if (APMihPtr || !xf86Info.pmFlag) {
        return NULL;
    }

    if ((fd = open(APM_DEVICE, O_RDWR)) == -1) {
        return NULL;
    }
    xf86PMGetEventFromOs = sunPMGetEventFromOS;
    xf86PMConfirmEventToOs = sunPMConfirmEventToOs;
    APMihPtr = xf86AddGeneralHandler(fd, xf86HandlePMEvents, NULL);
    return sunCloseAPM;
}

static void
sunCloseAPM(void)
{
    int fd;

    if (APMihPtr) {
        fd = xf86RemoveGeneralHandler(APMihPtr);
        close(fd);
        APMihPtr = NULL;
    }
}
