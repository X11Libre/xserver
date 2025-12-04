/*
 * Copyright 2007-2008 Peter Hutterer
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
 *
 * Author: Peter Hutterer, University of South Australia, NICTA
 */

/***********************************************************************
 *
 * Request to Warp the pointer location of an extension input device.
 *
 */

#include <dix-config.h>

#include <X11/X.h>              /* for inputstr.h    */
#include <X11/Xproto.h>         /* Request macro     */
#include <X11/extensions/XI.h>
#include <X11/extensions/XI2proto.h>

#include "dix/cursor_priv.h"
#include "dix/dix_priv.h"
#include "dix/input_priv.h"
#include "dix/request_priv.h"
#include "mi/mipointer_priv.h"
#include "Xi/handlers.h"

#include "inputstr.h"           /* DeviceIntPtr      */
#include "windowstr.h"          /* window structure  */
#include "scrnintstr.h"         /* screen structure  */
#include "extnsionst.h"
#include "exevents.h"
#include "exglobals.h"
#include "mipointer.h"          /* for miPointerUpdateSprite */

/***********************************************************************
 *
 * This procedure allows a client to warp the pointer of a device.
 *
 */

int
ProcXIWarpPointer(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXIWarpPointerReq);
    X_REQUEST_FIELD_CARD32(src_win);
    X_REQUEST_FIELD_CARD32(dst_win);
    X_REQUEST_FIELD_CARD32(src_x);
    X_REQUEST_FIELD_CARD32(src_y);
    X_REQUEST_FIELD_CARD16(src_width);
    X_REQUEST_FIELD_CARD16(src_height);
    X_REQUEST_FIELD_CARD32(dst_x);
    X_REQUEST_FIELD_CARD32(dst_y);
    X_REQUEST_FIELD_CARD16(deviceid);

    int rc;
    int x, y;
    WindowPtr dest = NULL;
    DeviceIntPtr pDev;
    SpritePtr pSprite;
    ScreenPtr newScreen;
    int src_x, src_y;
    int dst_x, dst_y;

    /* FIXME: panoramix stuff is missing, look at ProcWarpPointer */

    rc = dixLookupDevice(&pDev, stuff->deviceid, client, DixWriteAccess);

    if (rc != Success) {
        client->errorValue = stuff->deviceid;
        return rc;
    }

    if ((!InputDevIsMaster(pDev) && !InputDevIsFloating(pDev)) ||
        (InputDevIsMaster(pDev) && !IsPointerDevice(pDev))) {
        client->errorValue = stuff->deviceid;
        return BadDevice;
    }

    if (stuff->dst_win != None) {
        rc = dixLookupWindow(&dest, stuff->dst_win, client, DixGetAttrAccess);
        if (rc != Success) {
            client->errorValue = stuff->dst_win;
            return rc;
        }
    }

    pSprite = pDev->spriteInfo->sprite;
    x = pSprite->hotPhys.x;
    y = pSprite->hotPhys.y;

    src_x = stuff->src_x / (double) (1 << 16);
    src_y = stuff->src_y / (double) (1 << 16);
    dst_x = stuff->dst_x / (double) (1 << 16);
    dst_y = stuff->dst_y / (double) (1 << 16);

    if (stuff->src_win != None) {
        int winX, winY;
        WindowPtr src;

        rc = dixLookupWindow(&src, stuff->src_win, client, DixGetAttrAccess);
        if (rc != Success) {
            client->errorValue = stuff->src_win;
            return rc;
        }

        winX = src->drawable.x;
        winY = src->drawable.y;
        if (src->drawable.pScreen != pSprite->hotPhys.pScreen ||
            x < winX + src_x ||
            y < winY + src_y ||
            (stuff->src_width != 0 &&
             winX + src_x + (int) stuff->src_width < 0) ||
            (stuff->src_height != 0 &&
             winY + src_y + (int) stuff->src_height < y) ||
            !PointInWindowIsVisible(src, x, y))
            return Success;
    }

    if (dest) {
        x = dest->drawable.x;
        y = dest->drawable.y;
        newScreen = dest->drawable.pScreen;
    }
    else
        newScreen = pSprite->hotPhys.pScreen;

    x += dst_x;
    y += dst_y;

    if (x < 0)
        x = 0;
    else if (x > newScreen->width)
        x = newScreen->width - 1;

    if (y < 0)
        y = 0;
    else if (y > newScreen->height)
        y = newScreen->height - 1;

    if (newScreen == pSprite->hotPhys.pScreen) {
        if (x < pSprite->physLimits.x1)
            x = pSprite->physLimits.x1;
        else if (x >= pSprite->physLimits.x2)
            x = pSprite->physLimits.x2 - 1;

        if (y < pSprite->physLimits.y1)
            y = pSprite->physLimits.y1;
        else if (y >= pSprite->physLimits.y2)
            y = pSprite->physLimits.y2 - 1;

        if (pSprite->hotShape)
            ConfineToShape(pSprite->hotShape, &x, &y);
        if (newScreen->SetCursorPosition)
            newScreen->SetCursorPosition(pDev, newScreen, x, y, TRUE);
    }
    else if (!PointerConfinedToScreen(pDev)) {
        NewCurrentScreen(pDev, newScreen, x, y);
    }

    /* if we don't update the device, we get a jump next time it moves */
    pDev->last.valuators[0] = x;
    pDev->last.valuators[1] = y;
    miPointerUpdateSprite(pDev);

    if (*newScreen->CursorWarpedTo)
        (*newScreen->CursorWarpedTo) (pDev, newScreen, client,
                                      dest, pSprite, x, y);

    /* FIXME: XWarpPointer is supposed to generate an event. It doesn't do it
       here though. */
    return Success;
}
