/************************************************************
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of Silicon Graphics not be
used in advertising or publicity pertaining to distribution
of the software without specific prior written permission.
Silicon Graphics makes no representation about the suitability
of this software for any purpose. It is provided "as is"
without any express or implied warranty.

SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

#include <dix-config.h>

#include "stdio.h"
#include <X11/X.h>
#include <X11/Xproto.h>

#include "xkb/xkbsrv_priv.h"

#include "misc.h"
#include "inputstr.h"
#include "xkbstr.h"
#include "extnsionst.h"
#include "xkb-procs.h"

        /*
         * REQUEST SWAPPING
         */
static int _X_COLD
SProcXkbSelectEvents(ClientPtr client)
{
    REQUEST(xkbSelectEventsReq);
    REQUEST_AT_LEAST_SIZE(xkbSelectEventsReq);
    swaps(&stuff->deviceSpec);
    swaps(&stuff->affectWhich);
    swaps(&stuff->clear);
    swaps(&stuff->selectAll);
    swaps(&stuff->affectMap);
    swaps(&stuff->map);
    if ((stuff->affectWhich & (~XkbMapNotifyMask)) != 0) {
        union {
            BOOL *b;
            CARD8 *c8;
            CARD16 *c16;
            CARD32 *c32;
        } from;
        register unsigned bit, ndx, maskLeft, dataLeft;

        from.c8 = (CARD8 *) &stuff[1];
        dataLeft = (client->req_len * 4) - sizeof(xkbSelectEventsReq);
        maskLeft = (stuff->affectWhich & (~XkbMapNotifyMask));
        for (ndx = 0, bit = 1; (maskLeft != 0); ndx++, bit <<= 1) {
            if (((bit & maskLeft) == 0) || (ndx == XkbMapNotify))
                continue;
            maskLeft &= ~bit;
            if ((stuff->selectAll & bit) || (stuff->clear & bit))
                continue;
            switch (ndx) {
            // CARD16
            case XkbNewKeyboardNotify:
            case XkbStateNotify:
            case XkbNamesNotify:
            case XkbAccessXNotify:
            case XkbExtensionDeviceNotify:
                if (dataLeft < sizeof(CARD16)*2)
                    return BadLength;
                swaps(&from.c16[0]);
                swaps(&from.c16[1]);
                from.c8 += sizeof(CARD16)*2;
                dataLeft -= sizeof(CARD16)*2;
                break;
            // CARD32
            case XkbControlsNotify:
            case XkbIndicatorStateNotify:
            case XkbIndicatorMapNotify:
                if (dataLeft < sizeof(CARD32)*2)
                    return BadLength;
                swapl(&from.c32[0]);
                swapl(&from.c32[1]);
                from.c8 += sizeof(CARD32)*2;
                dataLeft -= sizeof(CARD32)*2;
                break;
            // CARD8
            case XkbBellNotify:
            case XkbActionMessage:
            case XkbCompatMapNotify:
                if (dataLeft < 2)
                    return BadLength;
                from.c8 += 4;
                dataLeft -= 4;
                break;
            default:
                client->errorValue = _XkbErrCode2(0x1, bit);
                return BadValue;
            }
        }
        if (dataLeft > 2) {
            ErrorF("[xkb] Extra data (%d bytes) after SelectEvents\n",
                   dataLeft);
            return BadLength;
        }
    }
    return ProcXkbSelectEvents(client);
}

static int _X_COLD
SProcXkbGetState(ClientPtr client)
{
    REQUEST(xkbGetStateReq);
    REQUEST_SIZE_MATCH(xkbGetStateReq);
    swaps(&stuff->deviceSpec);
    return ProcXkbGetState(client);
}

static int _X_COLD
SProcXkbLatchLockState(ClientPtr client)
{
    REQUEST(xkbLatchLockStateReq);
    REQUEST_SIZE_MATCH(xkbLatchLockStateReq);
    swaps(&stuff->deviceSpec);
    swaps(&stuff->groupLatch);
    return ProcXkbLatchLockState(client);
}

static int _X_COLD
SProcXkbSetControls(ClientPtr client)
{
    REQUEST(xkbSetControlsReq);
    REQUEST_SIZE_MATCH(xkbSetControlsReq);
    swaps(&stuff->deviceSpec);
    swaps(&stuff->affectInternalVMods);
    swaps(&stuff->internalVMods);
    swaps(&stuff->affectIgnoreLockVMods);
    swaps(&stuff->ignoreLockVMods);
    swaps(&stuff->axOptions);
    swapl(&stuff->affectEnabledCtrls);
    swapl(&stuff->enabledCtrls);
    swapl(&stuff->changeCtrls);
    swaps(&stuff->repeatDelay);
    swaps(&stuff->repeatInterval);
    swaps(&stuff->slowKeysDelay);
    swaps(&stuff->debounceDelay);
    swaps(&stuff->mkDelay);
    swaps(&stuff->mkInterval);
    swaps(&stuff->mkTimeToMax);
    swaps(&stuff->mkMaxSpeed);
    swaps(&stuff->mkCurve);
    swaps(&stuff->axTimeout);
    swapl(&stuff->axtCtrlsMask);
    swapl(&stuff->axtCtrlsValues);
    swaps(&stuff->axtOptsMask);
    swaps(&stuff->axtOptsValues);
    return ProcXkbSetControls(client);
}

static int _X_COLD
SProcXkbSetMap(ClientPtr client)
{
    REQUEST(xkbSetMapReq);
    REQUEST_AT_LEAST_SIZE(xkbSetMapReq);
    swaps(&stuff->deviceSpec);
    swaps(&stuff->present);
    swaps(&stuff->flags);
    swaps(&stuff->totalSyms);
    swaps(&stuff->totalActs);
    swaps(&stuff->virtualMods);
    return ProcXkbSetMap(client);
}

static int _X_COLD
SProcXkbSetCompatMap(ClientPtr client)
{
    REQUEST(xkbSetCompatMapReq);
    REQUEST_AT_LEAST_SIZE(xkbSetCompatMapReq);
    swaps(&stuff->deviceSpec);
    swaps(&stuff->firstSI);
    swaps(&stuff->nSI);
    return ProcXkbSetCompatMap(client);
}

static int _X_COLD
SProcXkbSetIndicatorMap(ClientPtr client)
{
    REQUEST(xkbSetIndicatorMapReq);
    REQUEST_AT_LEAST_SIZE(xkbSetIndicatorMapReq);
    swaps(&stuff->deviceSpec);
    swapl(&stuff->which);
    return ProcXkbSetIndicatorMap(client);
}

static int _X_COLD
SProcXkbGetGeometry(ClientPtr client)
{
    REQUEST(xkbGetGeometryReq);
    REQUEST_SIZE_MATCH(xkbGetGeometryReq);
    swaps(&stuff->deviceSpec);
    swapl(&stuff->name);
    return ProcXkbGetGeometry(client);
}

static int _X_COLD
SProcXkbPerClientFlags(ClientPtr client)
{
    REQUEST(xkbPerClientFlagsReq);
    REQUEST_SIZE_MATCH(xkbPerClientFlagsReq);
    swaps(&stuff->deviceSpec);
    swapl(&stuff->change);
    swapl(&stuff->value);
    swapl(&stuff->ctrlsToChange);
    swapl(&stuff->autoCtrls);
    swapl(&stuff->autoCtrlValues);
    return ProcXkbPerClientFlags(client);
}

static int _X_COLD
SProcXkbGetKbdByName(ClientPtr client)
{
    REQUEST(xkbGetKbdByNameReq);
    REQUEST_AT_LEAST_SIZE(xkbGetKbdByNameReq);
    swaps(&stuff->deviceSpec);
    swaps(&stuff->want);
    swaps(&stuff->need);
    return ProcXkbGetKbdByName(client);
}

static int _X_COLD
SProcXkbSetDeviceInfo(ClientPtr client)
{
    REQUEST(xkbSetDeviceInfoReq);
    REQUEST_AT_LEAST_SIZE(xkbSetDeviceInfoReq);
    swaps(&stuff->deviceSpec);
    swaps(&stuff->change);
    swaps(&stuff->nDeviceLedFBs);
    return ProcXkbSetDeviceInfo(client);
}

static int _X_COLD
SProcXkbSetDebuggingFlags(ClientPtr client)
{
    REQUEST(xkbSetDebuggingFlagsReq);
    REQUEST_AT_LEAST_SIZE(xkbSetDebuggingFlagsReq);
    swapl(&stuff->affectFlags);
    swapl(&stuff->flags);
    swapl(&stuff->affectCtrls);
    swapl(&stuff->ctrls);
    swaps(&stuff->msgLength);
    return ProcXkbSetDebuggingFlags(client);
}

int _X_COLD
SProcXkbDispatch(ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data) {
    case X_kbUseExtension:
        return ProcXkbUseExtension(client);
    case X_kbSelectEvents:
        return SProcXkbSelectEvents(client);
    case X_kbBell:
        return ProcXkbBell(client);
    case X_kbGetState:
        return SProcXkbGetState(client);
    case X_kbLatchLockState:
        return SProcXkbLatchLockState(client);
    case X_kbGetControls:
        return ProcXkbGetControls(client);
    case X_kbSetControls:
        return SProcXkbSetControls(client);
    case X_kbGetMap:
        return ProcXkbGetMap(client);
    case X_kbSetMap:
        return SProcXkbSetMap(client);
    case X_kbGetCompatMap:
        return ProcXkbGetCompatMap(client);
    case X_kbSetCompatMap:
        return SProcXkbSetCompatMap(client);
    case X_kbGetIndicatorState:
        return ProcXkbGetIndicatorState(client);
    case X_kbGetIndicatorMap:
        return ProcXkbGetIndicatorMap(client);
    case X_kbSetIndicatorMap:
        return SProcXkbSetIndicatorMap(client);
    case X_kbGetNamedIndicator:
        return ProcXkbGetNamedIndicator(client);
    case X_kbSetNamedIndicator:
        return ProcXkbSetNamedIndicator(client);
    case X_kbGetNames:
        return ProcXkbGetNames(client);
    case X_kbSetNames:
        return ProcXkbSetNames(client);
    case X_kbGetGeometry:
        return SProcXkbGetGeometry(client);
    case X_kbSetGeometry:
        return ProcXkbSetGeometry(client);
    case X_kbPerClientFlags:
        return SProcXkbPerClientFlags(client);
    case X_kbListComponents:
        return ProcXkbListComponents(client);
    case X_kbGetKbdByName:
        return SProcXkbGetKbdByName(client);
    case X_kbGetDeviceInfo:
        return ProcXkbGetDeviceInfo(client);
    case X_kbSetDeviceInfo:
        return SProcXkbSetDeviceInfo(client);
    case X_kbSetDebuggingFlags:
        return SProcXkbSetDebuggingFlags(client);
    default:
        return BadRequest;
    }
}
