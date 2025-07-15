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
#include "os/fmt.h"
#include "Xext/panoramiXsrv.h"

#include "inputstr.h"           /* DeviceIntPtr      */
#include "windowstr.h"          /* window structure  */
#include "extnsionst.h"
#include "exglobals.h"
#include "scrnintstr.h"
#include "xkbsrv.h"

#include "xiquerypointer.h"

/***********************************************************************
 *
 * This procedure allows a client to query the pointer of a device.
 *
 */

int _X_COLD
SProcXIQueryPointer(ClientPtr client)
{
    REQUEST(xXIQueryPointerReq);
    REQUEST_SIZE_MATCH(xXIQueryPointerReq);

    swaps(&stuff->deviceid);
    swapl(&stuff->win);
    return (ProcXIQueryPointer(client));
}

int
ProcXIQueryPointer(ClientPtr client)
{
    int rc;
    DeviceIntPtr pDev, kbd;
    WindowPtr pWin, t;
    SpritePtr pSprite;
    XkbStatePtr state;
    char *buttons = NULL;
    int buttons_size = 0;       /* size of buttons array */
    XIClientPtr xi_client;
    Bool have_xi22 = FALSE;

    REQUEST(xXIQueryPointerReq);
    REQUEST_SIZE_MATCH(xXIQueryPointerReq);

    /* Check if client is compliant with XInput 2.2 or later. Earlier clients
     * do not know about touches, so we must report emulated button presses. 2.2
     * and later clients are aware of touches, so we don't include emulated
     * button presses in the reply. */
    xi_client = dixLookupPrivate(&client->devPrivates, XIClientPrivateKey);
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

    xXIQueryPointerReply rep = {
        .repType = X_Reply,
        .RepType = X_XIQueryPointer,
        .sequenceNumber = client->sequence,
        .length = 6,
        .root = (InputDevCurrentRootWindow(pDev))->drawable.id,
        .root_x = double_to_fp1616(pSprite->hot.x),
        .root_y = double_to_fp1616(pSprite->hot.y),
        .child = None
    };

    if (kbd) {
        state = &kbd->key->xkbInfo->state;
        rep.mods.base_mods = state->base_mods;
        rep.mods.latched_mods = state->latched_mods;
        rep.mods.locked_mods = state->locked_mods;

        rep.group.base_group = state->base_group;
        rep.group.latched_group = state->latched_group;
        rep.group.locked_group = state->locked_group;
    }

    if (pDev->button) {
        int i;

        rep.buttons_len = bytes_to_int32(bits_to_bytes(256)); /* button map up to 255 */
        rep.length += rep.buttons_len;
        buttons = calloc(rep.buttons_len, 4);
        if (!buttons)
            return BadAlloc;
        buttons_size = rep.buttons_len * 4;

        for (i = 1; i < pDev->button->numButtons; i++)
            if (BitIsOn(pDev->button->down, i))
                SetBit(buttons, pDev->button->map[i]);

        if (!have_xi22 && pDev->touch && pDev->touch->buttonsDown > 0)
            SetBit(buttons, pDev->button->map[1]);
    }
    else
        rep.buttons_len = 0;

    if (pSprite->hot.pScreen == pWin->drawable.pScreen) {
        rep.same_screen = xTrue;
        rep.win_x = double_to_fp1616(pSprite->hot.x - pWin->drawable.x);
        rep.win_y = double_to_fp1616(pSprite->hot.y - pWin->drawable.y);
        for (t = pSprite->win; t; t = t->parent)
            if (t->parent == pWin) {
                rep.child = t->drawable.id;
                break;
            }
    }
    else {
        rep.same_screen = xFalse;
        rep.win_x = 0;
        rep.win_y = 0;
    }

#ifdef XINERAMA
    if (!noPanoramiXExtension) {
        rep.root_x += double_to_fp1616(screenInfo.screens[0]->x);
        rep.root_y += double_to_fp1616(screenInfo.screens[0]->y);
        if (stuff->win == rep.root) {
            rep.win_x += double_to_fp1616(screenInfo.screens[0]->x);
            rep.win_y += double_to_fp1616(screenInfo.screens[0]->y);
        }
    }
#endif /* XINERAMA */

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.root);
        swapl(&rep.child);
        swapl(&rep.root_x);
        swapl(&rep.root_y);
        swapl(&rep.win_x);
        swapl(&rep.win_y);
        swaps(&rep.buttons_len);
    }
    WriteToClient(client, sizeof(xXIQueryPointerReply), &rep);
    WriteToClient(client, buttons_size, buttons);

    free(buttons);

    return Success;
}
