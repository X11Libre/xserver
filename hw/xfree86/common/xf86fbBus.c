/*
 * Copyright (c) 2000-2001 by The XFree86 Project, Inc.
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
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

/*
 * This file contains the interfaces to the bus-specific code
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/X.h>
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"

#include "xf86Bus.h"
#include "xf86_OSproc.h"

int fbSlotClaimed = 0;

static Bool
xf86_check_fb_slot(GDevPtr dev, const char* driver_name)
{
#ifdef XSERVER_LIBPCIACCESS

#define AUTOCONFIGURED_STRING "Autoconfigured Video Device "
    /* Apparently asan complains is memcmp is used */
    if ((!dev || !dev->identifier ||
        !strncmp(dev->identifier, AUTOCONFIGURED_STRING, sizeof(AUTOCONFIGURED_STRING) - 1)) &&
        (pciSlotClaimed
#ifdef XSERVER_PLATFORM_BUS
         || platformSlotClaimed
#endif
#if defined(__sparc__) || defined(__sparc)
         || sbusSlotClaimed
#endif
        )) {
        LogMessageVerb(X_ERROR, 1, "Refusing to use autoconfigured driver: %s in framebuffer mode.\n", driver_name);
        LogMessageVerb(X_ERROR, 1, "Please write explicit configuration Screen sections for all framebuffer devices.\n");
        return FALSE;
    }
#undef AUTOCONFIGURED_STRING
#endif
    return TRUE;
}

int
xf86ClaimFbSlot(DriverPtr drvp, int chipset, GDevPtr dev, Bool active)
{
    EntityPtr p;
    int num;

    if (xf86_check_fb_slot(dev, drvp->driverName)) {
        num = xf86AllocateEntity();
        p = xf86Entities[num];
        p->driver = drvp;
        p->chipset = 0;
        p->bus.type = BUS_NONE;
        p->active = active;
        p->inUse = FALSE;
        xf86AddDevToEntity(num, dev);

        fbSlotClaimed++;
        return num;
    } else {
        return -1;
    }
}
