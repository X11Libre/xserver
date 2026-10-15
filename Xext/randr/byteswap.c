/* SPDX-License-Identifier: MIT OR X11 OR AGPLv3
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include <X11/Xproto.h>
#include <X11/extensions/randrproto.h>

#include "include/misc.h"

#include "randr_intern.h"

#define __CONV_HDR(_TYPE_) \
    _TYPE_ *from = (_TYPE_*)_from; \
    _TYPE_ *to = (_TYPE_*)_to; \
    *to = *from; \
    swaps(&to->sequenceNumber); \
    swapl(&to->timestamp);

#define __CONV_S(_NAME_) swaps(&to->_NAME_)
#define __CONV_L(_NAME_) swapl(&to->_NAME_)

void RRSwapScreenChangeNotifyEvent(xEvent *_from, xEvent *_to)
{
    __CONV_HDR(xRRScreenChangeNotifyEvent);
    __CONV_L(configTimestamp);
    __CONV_L(root);
    __CONV_L(window);
    __CONV_S(sizeID);
    __CONV_S(subpixelOrder);
    __CONV_S(widthInPixels);
    __CONV_S(heightInPixels);
    __CONV_S(widthInMillimeters);
    __CONV_S(heightInMillimeters);
}

static void RRSwapCrtcChangeNotifyEvent(xEvent *_from, xEvent *_to)
{
    __CONV_HDR(xRRCrtcChangeNotifyEvent);
    __CONV_L(window);
    __CONV_L(crtc);
    __CONV_L(mode);
    __CONV_S(rotation);
    __CONV_S(x);
    __CONV_S(y);
    __CONV_S(width);
    __CONV_S(height);
}

static void RRSwapOutputChangeNotifyEvent(xEvent *_from, xEvent *_to)
{
    __CONV_HDR(xRROutputChangeNotifyEvent);
    __CONV_L(configTimestamp);
    __CONV_L(window);
    __CONV_L(output);
    __CONV_L(crtc);
    __CONV_L(mode);
    __CONV_S(rotation);
}

static void RRSwapOutputPropertyNotifyEvent(xEvent *_from, xEvent *_to)
{
    __CONV_HDR(xRROutputPropertyNotifyEvent);
    __CONV_L(window);
    __CONV_L(output);
    __CONV_L(atom);
}

static void RRSwapProviderChangeNotifyEvent(xEvent *_from, xEvent *_to)
{
    __CONV_HDR(xRRProviderChangeNotifyEvent);
    __CONV_L(window);
    __CONV_L(provider);
}

static void RRSwapProviderPropertyNotifyEvent(xEvent *_from, xEvent *_to)
{
    __CONV_HDR(xRRProviderPropertyNotifyEvent);
    __CONV_L(window);
    __CONV_L(provider);
    __CONV_L(atom);
}

static void RRSwapResourceChangeNotifyEvent(xEvent *_from, xEvent *_to)
{
    __CONV_HDR(xRRResourceChangeNotifyEvent);
    __CONV_L(window);
}

static void RRSwapLeaseNotifyEvent(xEvent *_from, xEvent *_to)
{
    __CONV_HDR(xRRLeaseNotifyEvent);
    __CONV_L(window);
    __CONV_L(lease);
}

void RRSwapNotifyEvent(xEvent *from, xEvent *to)
{
    switch (from->u.u.detail) {
    case RRNotify_CrtcChange:
        RRSwapCrtcChangeNotifyEvent(from, to);
        break;
    case RRNotify_OutputChange:
        RRSwapOutputChangeNotifyEvent(from, to);
        break;
    case RRNotify_OutputProperty:
        RRSwapOutputPropertyNotifyEvent(from, to);
        break;
    case RRNotify_ProviderChange:
        RRSwapProviderChangeNotifyEvent(from, to);
        break;
    case RRNotify_ProviderProperty:
        RRSwapProviderPropertyNotifyEvent(from, to);
        break;
    case RRNotify_ResourceChange:
        RRSwapResourceChangeNotifyEvent(from, to);
        break;
    case RRNotify_Lease:
        RRSwapLeaseNotifyEvent(from, to);
        break;
    default:
        break;
    }
}
