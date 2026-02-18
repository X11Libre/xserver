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
#include <xorg-config.h>

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/X.h>
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"

#include "xf86Bus.h"
#include "xf86_OSproc.h"

static Bool
xf86_check_fb_slot(GDevPtr dev, const char* driver_name)
{
#define AUTOCONFIGURED_STRING "Autoconfigured Video Device "
    /* Apparently asan complains is memcmp is used */
    if (!dev || !dev->identifier ||
        !strncmp(dev->identifier, AUTOCONFIGURED_STRING, sizeof(AUTOCONFIGURED_STRING) - 1)) {
        Bool reject_device = FALSE;

        /**
         * We have to walk all devices and check for potential collisions.
         * Sadly, when autoconfiguring, we don't have any information about drivers running in framebuffer mode.
         * This is both because we don't have any api for sending this information, and because the drivers
         * often find out what device they will ultimately use way later in the initialization process.
         * Just checking/counting how many times xf86Claim*Slot gets called is not enough.
         * There are also drivers like xf86-video-vesa, which don't use the xf86Claim*Slot api to check
         * for slot conflicts. Instead, vesa just allocates an entity and relies on the fact that is's usually
         * the last driver in the initialization queue to avoid conflicts.
         * We even have to forbid multiple autoconfigured drivers running in framebuffer mode, as that can
         * lead to conflicts too, including black screens.
         */
        for (int i = 0; i < xf86NumEntities; i++) {
            const EntityPtr pent = xf86Entities[i];

            if (pent->numInstances > 0) {
                reject_device = TRUE;
                break;
            }
        }

        if (reject_device) {
            LogMessageVerb(X_ERROR, 1, "Refusing to use autoconfigured driver: %s in framebuffer mode.\n", driver_name);
            LogMessageVerb(X_ERROR, 1, "Please write explicit configuration Screen sections for all framebuffer devices.\n");
            return FALSE;
        }
    }
#undef AUTOCONFIGURED_STRING

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

        return num;
    } else {
        return -1;
    }
}
