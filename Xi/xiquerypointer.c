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
 * Request to query the pointer location of an extension input device.
 *
 */

#include <dix-config.h>

#include <X11/X.h>              /* for inputstr.h    */
#include <X11/Xproto.h>         /* Request macro     */
#include <X11/extensions/XI.h>
#include <X11/extensions/XI2proto.h>

#include "dix/dix_priv.h"
#include "dix/eventconvert.h"
#include "dix/exevents_priv.h"
#include "dix/input_priv.h"
#include "dix/inpututils_priv.h"
#include "dix/request_priv.h"
#include "dix/rpcbuf_priv.h"
#include "dix/screenint_priv.h"
#include "include/extinit.h"
#include "os/fmt.h"
#include "Xext/panoramiXsrv.h"
#include "Xi/handlers.h"

#include "inputstr.h"           /* DeviceIntPtr      */
#include "windowstr.h"          /* window structure  */
#include "extnsionst.h"
#include "exglobals.h"
#include "scrnintstr.h"
#include "xkbsrv.h"

/***********************************************************************
 *
 * This procedure allows a client to query the pointer of a device.
 *
 */

int
ProcXIQueryPointer(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXIQueryPointerReq);
    X_REQUEST_FIELD_CARD16(deviceid);
    X_REQUEST_FIELD_CARD32(win);

    int rc;
    DeviceIntPtr pDev, kbd;
    WindowPtr pWin, t;
    SpritePtr pSprite;
    XkbStatePtr state;
    Bool have_xi22 = FALSE;

    /* Check if client is compliant with XInput 2.2 or later. Earlier clients
     * do not know about touches, so we must report emulated button presses. 2.2
     * and later clients are aware of touches, so we don't include emulated
     * button presses in the reply. */
    XIClientPtr xi_client = XIClientPriv(client);

    if (version_compare(xi_client->major_version,
                        xi_client->minor_version, 2, 2) >= 0)
        have_xi22 = TRUE;

    rc = dixLookupDevice(&pDev, stuff->deviceid, client, DixReadAccess);
    if (rc != Success) {
        client->errorValue = stuff->deviceid;
        return rc;
    }

    if (pDev->valuator == NULL || IsKeyboardDevice(pDev) ||
        (!InputDevIsMaster(pDev) && !InputDevIsFloating(pDev))) {   /* no attached devices */
        client->errorValue = stuff->deviceid;
        return BadDevice;
    }

    rc = dixLookupWindow(&pWin, stuff->win, client, DixGetAttrAccess);
    if (rc != Success) {
        client->errorValue = stuff->win;
        return rc;
    }

    if (pDev->valuator->motionHintWindow)
        MaybeStopHint(pDev, client);

    if (InputDevIsMaster(pDev))
        kbd = GetMaster(pDev, MASTER_KEYBOARD);
    else
        kbd = (pDev->key) ? pDev : NULL;

    pSprite = pDev->spriteInfo->sprite;

    xXIQueryPointerReply reply = {
        .RepType = X_XIQueryPointer,
        .root = (InputDevCurrentRootWindow(pDev))->drawable.id,
        .root_x = double_to_fp1616(pSprite->hot.x),
        .root_y = double_to_fp1616(pSprite->hot.y),
    };

    if (kbd) {
        state = &kbd->key->xkbInfo->state;
        reply.mods.base_mods = state->base_mods;
        reply.mods.latched_mods = state->latched_mods;
        reply.mods.locked_mods = state->locked_mods;

        reply.group.base_group = state->base_group;
        reply.group.latched_group = state->latched_group;
        reply.group.locked_group = state->locked_group;
    }

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    if (pDev->button) {
        int i;

        const int buttons_size = bits_to_bytes(256); /* button map up to 255 */
        reply.buttons_len = bytes_to_int32(buttons_size);
        char *buttons = x_rpcbuf_reserve(&rpcbuf, buttons_size);
        if (!buttons)
            return BadAlloc;

        for (i = 1; i < pDev->button->numButtons; i++)
            if (BitIsOn(pDev->button->down, i))
                SetBit(buttons, pDev->button->map[i]);

        if (!have_xi22 && pDev->touch && pDev->touch->buttonsDown > 0)
            SetBit(buttons, pDev->button->map[1]);
    }

    if (pSprite->hot.pScreen == pWin->drawable.pScreen) {
        reply.same_screen = xTrue;
        reply.win_x = double_to_fp1616(pSprite->hot.x - pWin->drawable.x);
        reply.win_y = double_to_fp1616(pSprite->hot.y - pWin->drawable.y);
        for (t = pSprite->win; t; t = t->parent)
            if (t->parent == pWin) {
                reply.child = t->drawable.id;
                break;
            }
    }

#ifdef XINERAMA
    if (!noPanoramiXExtension) {
        ScreenPtr masterScreen = dixGetMasterScreen();
        reply.root_x += double_to_fp1616(masterScreen->x);
        reply.root_y += double_to_fp1616(masterScreen->y);
        if (stuff->win == reply.root) {
            reply.win_x += double_to_fp1616(masterScreen->x);
            reply.win_y += double_to_fp1616(masterScreen->y);
        }
    }
#endif /* XINERAMA */

    REPLY_FIELD_CARD32(root);
    REPLY_FIELD_CARD32(child);
    REPLY_FIELD_CARD32(root_x);
    REPLY_FIELD_CARD32(root_y);
    REPLY_FIELD_CARD32(win_x);
    REPLY_FIELD_CARD32(win_y);
    REPLY_FIELD_CARD16(buttons_len);

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}
