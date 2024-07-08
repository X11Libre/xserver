/***********************************************************
Copyright 1991 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.
******************************************************************/

#include <dix-config.h>

#include <string.h>

#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvproto.h>

#include "dix/dix_priv.h"
#include "dix/rpcbuf_priv.h"
#include "dix/request_priv.h"
#include "dix/screenint_priv.h"
#include "Xext/panoramiX.h"
#include "Xext/panoramiXsrv.h"
#include "Xext/shm_priv.h"
#include "Xext/xvdix_priv.h"

#include "misc.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "gcstruct.h"
#include "dixstruct.h"
#include "resource.h"
#include "opaque.h"
#ifdef CONFIG_MITSHM
#include <X11/extensions/shmproto.h>
#include "shmint.h"
#endif

#include "xvdisp.h"

#ifdef XINERAMA
unsigned long XvXRTPort;
#endif /* XINERAMA */

static int
ProcXvQueryExtension(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xvQueryExtensionReq);

    xvQueryExtensionReply reply = {
        .version = XvVersion,
        .revision = XvRevision
    };

    if (client->swapped) {
        swaps(&reply.version);
        swaps(&reply.revision);
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
ProcXvQueryAdaptors(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xvQueryAdaptorsReq);
    X_REQUEST_FIELD_CARD32(window);

    int na, nf, rc;
    XvAdaptorPtr pa;
    XvFormatPtr pf;
    WindowPtr pWin;
    ScreenPtr pScreen;
    XvScreenPtr pxvs;

    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    pScreen = pWin->drawable.pScreen;
    pxvs = (XvScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                          XvGetScreenKey());

    size_t numAdaptors = 0;
    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    if (pxvs) {
        numAdaptors = pxvs->nAdaptors;
        na = pxvs->nAdaptors;
        pa = pxvs->pAdaptors;
        while (na--) {
            /* xvAdaptorInfo */
            x_rpcbuf_write_CARD32(&rpcbuf, pa->base_id);
            x_rpcbuf_write_CARD16(&rpcbuf, strlen(pa->name));
            x_rpcbuf_write_CARD16(&rpcbuf, pa->nPorts);
            x_rpcbuf_write_CARD16(&rpcbuf, pa->nFormats);
            x_rpcbuf_write_CARD8(&rpcbuf, pa->type);
            x_rpcbuf_write_CARD8(&rpcbuf, 0); /* padding */
            x_rpcbuf_write_string_pad(&rpcbuf, pa->name);

            nf = pa->nFormats;
            pf = pa->pFormats;
            while (nf--) {
                /* xvFormat */
                x_rpcbuf_write_CARD32(&rpcbuf, pf->visual);
                x_rpcbuf_write_CARD8(&rpcbuf, pf->depth);
                x_rpcbuf_write_CARD8(&rpcbuf, 0); /* padding */
                x_rpcbuf_write_CARD16(&rpcbuf, 0); /* padding */
                pf++;
            }
            pa++;
        }
    }

    xvQueryAdaptorsReply reply = {
        .num_adaptors = numAdaptors,
    };

    if (client->swapped) {
        swaps(&reply.num_adaptors);
    }

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

static int
ProcXvQueryEncodings(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xvQueryEncodingsReq);
    X_REQUEST_FIELD_CARD32(port);

    XvPortPtr pPort;
    VALIDATE_XV_PORT(stuff->port, pPort, DixReadAccess);

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    size_t ne = pPort->pAdaptor->nEncodings;
    XvEncodingPtr pe = pPort->pAdaptor->pEncodings;
    while (ne--) {
        size_t nameSize = strlen(pe->name);

        x_rpcbuf_write_CARD32(&rpcbuf, pe->id);
        x_rpcbuf_write_CARD16(&rpcbuf, nameSize);
        x_rpcbuf_write_CARD16(&rpcbuf, pe->width);
        x_rpcbuf_write_CARD16(&rpcbuf, pe->height);
        x_rpcbuf_write_CARD32(&rpcbuf, pe->rate.numerator);
        x_rpcbuf_write_CARD32(&rpcbuf, pe->rate.denominator);
        x_rpcbuf_write_string_pad(&rpcbuf, pe->name);

        pe++;
    }

    xvQueryEncodingsReply reply = {
        .num_encodings = pPort->pAdaptor->nEncodings,
    };

    if (client->swapped) {
        swaps(&reply.num_encodings);
    }

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

static int
SingleXvPutVideo(ClientPtr client)
{
    DrawablePtr pDraw;
    XvPortPtr pPort;
    GCPtr pGC;
    int status;

    REQUEST(xvPutVideoReq);

    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, DixWriteAccess);
    VALIDATE_XV_PORT(stuff->port, pPort, DixReadAccess);

    if (!(pPort->pAdaptor->type & XvInputMask) ||
        !(pPort->pAdaptor->type & XvVideoMask)) {
        client->errorValue = stuff->port;
        return BadMatch;
    }

    status = XvdiMatchPort(pPort, pDraw);
    if (status != Success) {
        return status;
    }

    return XvdiPutVideo(client, pDraw, pPort, pGC, stuff->vid_x, stuff->vid_y,
                        stuff->vid_w, stuff->vid_h, stuff->drw_x, stuff->drw_y,
                        stuff->drw_w, stuff->drw_h);
}

#ifdef XINERAMA
static int XineramaXvPutVideo(ClientPtr client);
#endif

static int
ProcXvPutVideo(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xvPutVideoReq);
    X_REQUEST_FIELD_CARD32(port);
    X_REQUEST_FIELD_CARD32(drawable);
    X_REQUEST_FIELD_CARD32(gc);
    X_REQUEST_FIELD_CARD16(vid_x);
    X_REQUEST_FIELD_CARD16(vid_y);
    X_REQUEST_FIELD_CARD16(vid_w);
    X_REQUEST_FIELD_CARD16(vid_h);
    X_REQUEST_FIELD_CARD16(drw_x);
    X_REQUEST_FIELD_CARD16(drw_y);
    X_REQUEST_FIELD_CARD16(drw_w);
    X_REQUEST_FIELD_CARD16(drw_h);

#ifdef XINERAMA
    if (xvUseXinerama)
        return XineramaXvPutVideo(client);
#endif
    return SingleXvPutVideo(client);
}

static int
SingleXvPutStill(ClientPtr client)
{
    DrawablePtr pDraw;
    XvPortPtr pPort;
    GCPtr pGC;
    int status;

    REQUEST(xvPutStillReq);

    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, DixWriteAccess);
    VALIDATE_XV_PORT(stuff->port, pPort, DixReadAccess);

    if (!(pPort->pAdaptor->type & XvInputMask) ||
        !(pPort->pAdaptor->type & XvStillMask)) {
        client->errorValue = stuff->port;
        return BadMatch;
    }

    status = XvdiMatchPort(pPort, pDraw);
    if (status != Success) {
        return status;
    }

    return XvdiPutStill(client, pDraw, pPort, pGC, stuff->vid_x, stuff->vid_y,
                        stuff->vid_w, stuff->vid_h, stuff->drw_x, stuff->drw_y,
                        stuff->drw_w, stuff->drw_h);
}

#ifdef XINERAMA
static int XineramaXvPutStill(ClientPtr client);
#endif

static int
ProcXvPutStill(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xvPutStillReq);
    X_REQUEST_FIELD_CARD32(port);
    X_REQUEST_FIELD_CARD32(drawable);
    X_REQUEST_FIELD_CARD32(gc);
    X_REQUEST_FIELD_CARD16(vid_x);
    X_REQUEST_FIELD_CARD16(vid_y);
    X_REQUEST_FIELD_CARD16(vid_w);
    X_REQUEST_FIELD_CARD16(vid_h);
    X_REQUEST_FIELD_CARD16(drw_x);
    X_REQUEST_FIELD_CARD16(drw_y);
    X_REQUEST_FIELD_CARD16(drw_w);
    X_REQUEST_FIELD_CARD16(drw_h);

#ifdef XINERAMA
    if (xvUseXinerama)
        return XineramaXvPutStill(client);
#endif
    return SingleXvPutStill(client);
}

static int
ProcXvGetVideo(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xvGetVideoReq);
    X_REQUEST_FIELD_CARD32(port);
    X_REQUEST_FIELD_CARD32(drawable);
    X_REQUEST_FIELD_CARD32(gc);
    X_REQUEST_FIELD_CARD16(vid_x);
    X_REQUEST_FIELD_CARD16(vid_y);
    X_REQUEST_FIELD_CARD16(vid_w);
    X_REQUEST_FIELD_CARD16(vid_h);
    X_REQUEST_FIELD_CARD16(drw_x);
    X_REQUEST_FIELD_CARD16(drw_y);
    X_REQUEST_FIELD_CARD16(drw_w);
    X_REQUEST_FIELD_CARD16(drw_h);

    DrawablePtr pDraw;
    XvPortPtr pPort;
    GCPtr pGC;
    int status;

    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, DixReadAccess);
    VALIDATE_XV_PORT(stuff->port, pPort, DixReadAccess);

    if (!(pPort->pAdaptor->type & XvOutputMask) ||
        !(pPort->pAdaptor->type & XvVideoMask)) {
        client->errorValue = stuff->port;
        return BadMatch;
    }

    status = XvdiMatchPort(pPort, pDraw);
    if (status != Success) {
        return status;
    }

    return XvdiGetVideo(client, pDraw, pPort, pGC, stuff->vid_x, stuff->vid_y,
                        stuff->vid_w, stuff->vid_h, stuff->drw_x, stuff->drw_y,
                        stuff->drw_w, stuff->drw_h);
}

static int
ProcXvGetStill(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xvGetStillReq);
    X_REQUEST_FIELD_CARD32(port);
    X_REQUEST_FIELD_CARD32(drawable);
    X_REQUEST_FIELD_CARD32(gc);
    X_REQUEST_FIELD_CARD16(vid_x);
    X_REQUEST_FIELD_CARD16(vid_y);
    X_REQUEST_FIELD_CARD16(vid_w);
    X_REQUEST_FIELD_CARD16(vid_h);
    X_REQUEST_FIELD_CARD16(drw_x);
    X_REQUEST_FIELD_CARD16(drw_y);
    X_REQUEST_FIELD_CARD16(drw_w);
    X_REQUEST_FIELD_CARD16(drw_h);

    DrawablePtr pDraw;
    XvPortPtr pPort;
    GCPtr pGC;
    int status;

    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, DixReadAccess);
    VALIDATE_XV_PORT(stuff->port, pPort, DixReadAccess);

    if (!(pPort->pAdaptor->type & XvOutputMask) ||
        !(pPort->pAdaptor->type & XvStillMask)) {
        client->errorValue = stuff->port;
        return BadMatch;
    }

    status = XvdiMatchPort(pPort, pDraw);
    if (status != Success) {
        return status;
    }

    return XvdiGetStill(client, pDraw, pPort, pGC, stuff->vid_x, stuff->vid_y,
                        stuff->vid_w, stuff->vid_h, stuff->drw_x, stuff->drw_y,
                        stuff->drw_w, stuff->drw_h);
}

static int
ProcXvSelectVideoNotify(ClientPtr client)
{
    DrawablePtr pDraw;
    int rc;

    X_REQUEST_HEAD_STRUCT(xvSelectVideoNotifyReq);
    X_REQUEST_FIELD_CARD32(drawable);

    rc = dixLookupDrawable(&pDraw, stuff->drawable, client, 0,
                           DixReceiveAccess);
    if (rc != Success)
        return rc;

    return XvdiSelectVideoNotify(client, pDraw, stuff->onoff);
}

static int
ProcXvSelectPortNotify(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xvSelectPortNotifyReq);
    X_REQUEST_FIELD_CARD32(port);

    XvPortPtr pPort;
    VALIDATE_XV_PORT(stuff->port, pPort, DixReadAccess);

    return XvdiSelectPortNotify(client, pPort, stuff->onoff);
}

static int
ProcXvGrabPort(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xvGrabPortReq);
    X_REQUEST_FIELD_CARD32(port);
    X_REQUEST_FIELD_CARD32(time);

    int result, status;
    XvPortPtr pPort;

    VALIDATE_XV_PORT(stuff->port, pPort, DixReadAccess);

    status = XvdiGrabPort(client, pPort, stuff->time, &result);

    if (status != Success) {
        return status;
    }
    xvGrabPortReply reply = {
        .result = result
    };

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
ProcXvUngrabPort(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xvGrabPortReq);
    X_REQUEST_FIELD_CARD32(port);
    X_REQUEST_FIELD_CARD32(time);

    XvPortPtr pPort;
    VALIDATE_XV_PORT(stuff->port, pPort, DixReadAccess);

    return XvdiUngrabPort(client, pPort, stuff->time);
}

static int
SingleXvStopVideo(ClientPtr client)
{
    int ret;
    DrawablePtr pDraw;
    XvPortPtr pPort;

    REQUEST(xvStopVideoReq);

    VALIDATE_XV_PORT(stuff->port, pPort, DixReadAccess);

    ret = dixLookupDrawable(&pDraw, stuff->drawable, client, 0, DixWriteAccess);
    if (ret != Success)
        return ret;

    return XvdiStopVideo(client, pPort, pDraw);
}

#ifdef XINERAMA
static int XineramaXvStopVideo(ClientPtr client);
#endif

static int
ProcXvStopVideo(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xvStopVideoReq);
    X_REQUEST_FIELD_CARD32(port);
    X_REQUEST_FIELD_CARD32(drawable);

#ifdef XINERAMA
    if (xvUseXinerama)
        return XineramaXvStopVideo(client);
#endif
    return SingleXvStopVideo(client);
}

static int
SingleXvSetPortAttribute(ClientPtr client)
{
    int status;
    XvPortPtr pPort;

    REQUEST(xvSetPortAttributeReq);

    VALIDATE_XV_PORT(stuff->port, pPort, DixSetAttrAccess);

    if (!ValidAtom(stuff->attribute)) {
        client->errorValue = stuff->attribute;
        return BadAtom;
    }

    status =
        XvdiSetPortAttribute(client, pPort, stuff->attribute, stuff->value);

    if (status == BadMatch)
        client->errorValue = stuff->attribute;
    else
        client->errorValue = stuff->value;

    return status;
}

#ifdef XINERAMA
static int XineramaXvSetPortAttribute(ClientPtr client);
#endif

static int
ProcXvSetPortAttribute(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xvSetPortAttributeReq);
    X_REQUEST_FIELD_CARD32(port);
    X_REQUEST_FIELD_CARD32(attribute);
    X_REQUEST_FIELD_CARD32(value);

#ifdef XINERAMA
    if (xvUseXinerama)
        return XineramaXvSetPortAttribute(client);
#endif
    return SingleXvSetPortAttribute(client);
}

static int
ProcXvGetPortAttribute(ClientPtr client)
{
    INT32 value;
    int status;
    XvPortPtr pPort;

    X_REQUEST_HEAD_STRUCT(xvGetPortAttributeReq);
    X_REQUEST_FIELD_CARD32(port);
    X_REQUEST_FIELD_CARD32(attribute);

    VALIDATE_XV_PORT(stuff->port, pPort, DixGetAttrAccess);

    if (!ValidAtom(stuff->attribute)) {
        client->errorValue = stuff->attribute;
        return BadAtom;
    }

    status = XvdiGetPortAttribute(client, pPort, stuff->attribute, &value);
    if (status != Success) {
        client->errorValue = stuff->attribute;
        return status;
    }

    xvGetPortAttributeReply reply = {
        .value = value
    };

    if (client->swapped) {
        swapl(&reply.value);
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
ProcXvQueryBestSize(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xvQueryBestSizeReq);
    X_REQUEST_FIELD_CARD32(port);
    X_REQUEST_FIELD_CARD16(vid_w);
    X_REQUEST_FIELD_CARD16(vid_h);
    X_REQUEST_FIELD_CARD16(drw_w);
    X_REQUEST_FIELD_CARD16(drw_h);

    unsigned int actual_width, actual_height;
    XvPortPtr pPort;

    VALIDATE_XV_PORT(stuff->port, pPort, DixReadAccess);

    (*pPort->pAdaptor->ddQueryBestSize) (pPort, stuff->motion,
                                         stuff->vid_w, stuff->vid_h,
                                         stuff->drw_w, stuff->drw_h,
                                         &actual_width, &actual_height);

    xvQueryBestSizeReply reply = {
        .actual_width = actual_width,
        .actual_height = actual_height
    };

    if (client->swapped) {
        swaps(&reply.actual_width);
        swaps(&reply.actual_height);
    }

    return X_SEND_REPLY_SIMPLE(client, reply);
}

static int
ProcXvQueryPortAttributes(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xvQueryPortAttributesReq);
    X_REQUEST_FIELD_CARD32(port);

    int i;
    XvPortPtr pPort;
    XvAttributePtr pAtt;

    VALIDATE_XV_PORT(stuff->port, pPort, DixGetAttrAccess);

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    size_t textSize = 0;
    for (i = 0, pAtt = pPort->pAdaptor->pAttributes;
         i < pPort->pAdaptor->nAttributes; i++, pAtt++) {
        textSize += pad_to_int32(strlen(pAtt->name) + 1);
        x_rpcbuf_write_CARD32(&rpcbuf, pAtt->flags);
        x_rpcbuf_write_CARD32(&rpcbuf, pAtt->min_value);
        x_rpcbuf_write_CARD32(&rpcbuf, pAtt->max_value);
        x_rpcbuf_write_CARD32(&rpcbuf, pad_to_int32(strlen(pAtt->name)+1)); /* pass the NULL */
        x_rpcbuf_write_string_0t_pad(&rpcbuf, pAtt->name);
    }

    xvQueryPortAttributesReply reply = {
        .num_attributes = pPort->pAdaptor->nAttributes,
        .text_size = textSize,
    };

    if (client->swapped) {
        swapl(&reply.num_attributes);
        swapl(&reply.text_size);
    }

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

static int
SingleXvPutImage(ClientPtr client)
{
    DrawablePtr pDraw;
    XvPortPtr pPort;
    XvImagePtr pImage = NULL;
    GCPtr pGC;
    int status, i, size;
    CARD16 width, height;

    REQUEST(xvPutImageReq);

    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, DixWriteAccess);
    VALIDATE_XV_PORT(stuff->port, pPort, DixReadAccess);

    if (!(pPort->pAdaptor->type & XvImageMask) ||
        !(pPort->pAdaptor->type & XvInputMask)) {
        client->errorValue = stuff->port;
        return BadMatch;
    }

    status = XvdiMatchPort(pPort, pDraw);
    if (status != Success) {
        return status;
    }

    for (i = 0; i < pPort->pAdaptor->nImages; i++) {
        if (pPort->pAdaptor->pImages[i].id == stuff->id) {
            pImage = &(pPort->pAdaptor->pImages[i]);
            break;
        }
    }

    if (!pImage)
        return BadMatch;

    width = stuff->width;
    height = stuff->height;
    size = (*pPort->pAdaptor->ddQueryImageAttributes) (pPort, pImage, &width,
                                                       &height, NULL, NULL);
    size += sizeof(xvPutImageReq);
    size = bytes_to_int32(size);

    if ((width < stuff->width) || (height < stuff->height))
        return BadValue;

    if (client->req_len < size)
        return BadLength;

    return XvdiPutImage(client, pDraw, pPort, pGC, stuff->src_x, stuff->src_y,
                        stuff->src_w, stuff->src_h, stuff->drw_x, stuff->drw_y,
                        stuff->drw_w, stuff->drw_h, pImage,
                        (unsigned char *) (&stuff[1]), FALSE,
                        stuff->width, stuff->height);
}

#ifdef XINERAMA
static int
XineramaXvPutImage(ClientPtr client);
#endif

static int
ProcXvPutImage(ClientPtr client)
{
    X_REQUEST_HEAD_AT_LEAST(xvPutImageReq);
    X_REQUEST_FIELD_CARD32(port);
    X_REQUEST_FIELD_CARD32(drawable);
    X_REQUEST_FIELD_CARD32(gc);
    X_REQUEST_FIELD_CARD32(id);
    X_REQUEST_FIELD_CARD16(src_x);
    X_REQUEST_FIELD_CARD16(src_y);
    X_REQUEST_FIELD_CARD16(src_w);
    X_REQUEST_FIELD_CARD16(src_h);
    X_REQUEST_FIELD_CARD16(drw_x);
    X_REQUEST_FIELD_CARD16(drw_y);
    X_REQUEST_FIELD_CARD16(drw_w);
    X_REQUEST_FIELD_CARD16(drw_h);
    X_REQUEST_FIELD_CARD16(width);
    X_REQUEST_FIELD_CARD16(height);

#ifdef XINERAMA
    if (xvUseXinerama)
        return XineramaXvPutImage(client);
#endif
    return SingleXvPutImage(client);
}

#ifdef CONFIG_MITSHM

static int
SingleXvShmPutImage(ClientPtr client)
{
    ShmDescPtr shmdesc;
    DrawablePtr pDraw;
    XvPortPtr pPort;
    XvImagePtr pImage = NULL;
    GCPtr pGC;
    int status, size_needed, i;
    CARD16 width, height;

    REQUEST(xvShmPutImageReq);

    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, DixWriteAccess);
    VALIDATE_XV_PORT(stuff->port, pPort, DixReadAccess);

    if (!(pPort->pAdaptor->type & XvImageMask) ||
        !(pPort->pAdaptor->type & XvInputMask)) {
        client->errorValue = stuff->port;
        return BadMatch;
    }

    status = XvdiMatchPort(pPort, pDraw);
    if (status != Success) {
        return status;
    }

    for (i = 0; i < pPort->pAdaptor->nImages; i++) {
        if (pPort->pAdaptor->pImages[i].id == stuff->id) {
            pImage = &(pPort->pAdaptor->pImages[i]);
            break;
        }
    }

    if (!pImage)
        return BadMatch;

    status = dixLookupResourceByType((void **) &shmdesc, stuff->shmseg,
                                     ShmSegType, serverClient, DixReadAccess);
    if (status != Success)
        return status;

    width = stuff->width;
    height = stuff->height;
    size_needed = (*pPort->pAdaptor->ddQueryImageAttributes) (pPort, pImage,
                                                              &width, &height,
                                                              NULL, NULL);
    if ((size_needed + stuff->offset) > shmdesc->size)
        return BadAccess;

    if ((width < stuff->width) || (height < stuff->height))
        return BadValue;

    status = XvdiPutImage(client, pDraw, pPort, pGC, stuff->src_x, stuff->src_y,
                          stuff->src_w, stuff->src_h, stuff->drw_x,
                          stuff->drw_y, stuff->drw_w, stuff->drw_h, pImage,
                          (unsigned char *) shmdesc->addr + stuff->offset,
                          stuff->send_event, stuff->width, stuff->height);

    if ((status == Success) && stuff->send_event) {
        xShmCompletionEvent ev = {
            .type = ShmCompletionCode,
            .drawable = stuff->drawable,
            .minorEvent = xv_ShmPutImage,
            .majorEvent = XvReqCode,
            .shmseg = stuff->shmseg,
            .offset = stuff->offset
        };
        WriteEventsToClient(client, 1, (xEvent *) &ev);
    }

    return status;
}

#ifdef XINERAMA
static int XineramaXvShmPutImage(ClientPtr client);
#endif

#endif /* CONFIG_MITSHM */

static int
ProcXvShmPutImage(ClientPtr client)
{
#ifdef CONFIG_MITSHM
    X_REQUEST_HEAD_STRUCT(xvShmPutImageReq);
    X_REQUEST_FIELD_CARD32(port);
    X_REQUEST_FIELD_CARD32(drawable);
    X_REQUEST_FIELD_CARD32(gc);
    X_REQUEST_FIELD_CARD32(shmseg);
    X_REQUEST_FIELD_CARD32(id);
    X_REQUEST_FIELD_CARD32(offset);
    X_REQUEST_FIELD_CARD16(src_x);
    X_REQUEST_FIELD_CARD16(src_y);
    X_REQUEST_FIELD_CARD16(src_w);
    X_REQUEST_FIELD_CARD16(src_h);
    X_REQUEST_FIELD_CARD16(drw_x);
    X_REQUEST_FIELD_CARD16(drw_y);
    X_REQUEST_FIELD_CARD16(drw_w);
    X_REQUEST_FIELD_CARD16(drw_h);
    X_REQUEST_FIELD_CARD16(width);
    X_REQUEST_FIELD_CARD16(height);

#ifdef XINERAMA
    if (xvUseXinerama)
        return XineramaXvShmPutImage(client);
#endif
    return SingleXvShmPutImage(client);
#else
    return BadImplementation;
#endif /* CONFIG_MITSHM */
}

#ifdef XvMCExtension
#include "xvmcext.h"
#endif

__size_assert(int, sizeof(INT32));

static int
ProcXvQueryImageAttributes(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xvQueryImageAttributesReq);
    X_REQUEST_FIELD_CARD32(port);
    X_REQUEST_FIELD_CARD32(id);
    X_REQUEST_FIELD_CARD16(width);
    X_REQUEST_FIELD_CARD16(height);

    int size, num_planes, i;
    CARD16 width, height;
    XvImagePtr pImage = NULL;
    XvPortPtr pPort;

    VALIDATE_XV_PORT(stuff->port, pPort, DixReadAccess);

    for (i = 0; i < pPort->pAdaptor->nImages; i++) {
        if (pPort->pAdaptor->pImages[i].id == stuff->id) {
            pImage = &(pPort->pAdaptor->pImages[i]);
            break;
        }
    }

#ifdef XvMCExtension
    if (!pImage)
        pImage = XvMCFindXvImage(pPort, stuff->id);
#endif

    if (!pImage)
        return BadMatch;

    num_planes = pImage->num_planes;

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    /* allocating for `offsets` as well as `pitches` in one block */
    /* both having CARD32 * num_planes (actually int32_t put into CARD32) */
    int *offsets = x_rpcbuf_reserve(&rpcbuf, 2 * num_planes * sizeof(int));
    if (!offsets)
        return BadAlloc;
    int *pitches = offsets + num_planes;

    width = stuff->width;
    height = stuff->height;

    size = (*pPort->pAdaptor->ddQueryImageAttributes) (pPort, pImage,
                                                       &width, &height, offsets,
                                                       pitches);

    xvQueryImageAttributesReply reply = {
        .num_planes = num_planes,
        .width = width,
        .height = height,
        .data_size = size
    };

    if (client->swapped) {
        swapl(&reply.num_planes);
        swapl(&reply.data_size);
        swaps(&reply.width);
        swaps(&reply.height);
        /* needed here, because ddQueryImageAttributes() directly wrote into
           our rpcbuf area */
        SwapLongs((CARD32 *) offsets, x_rpcbuf_wsize_units(&rpcbuf));
    }

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

static int
ProcXvListImageFormats(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xvListImageFormatsReq);
    X_REQUEST_FIELD_CARD32(port);

    XvPortPtr pPort;
    XvImagePtr pImage;
    int i;

    VALIDATE_XV_PORT(stuff->port, pPort, DixReadAccess);

    pImage = pPort->pAdaptor->pImages;

    x_rpcbuf_t rpcbuf = { .swapped = client->swapped, .err_clear = TRUE };

    for (i = 0; i < pPort->pAdaptor->nImages; i++, pImage++) {
        /* xvImageFormatInfo */
        x_rpcbuf_write_CARD32(&rpcbuf, pImage->id);
        x_rpcbuf_write_CARD8(&rpcbuf, pImage->type);
        x_rpcbuf_write_CARD8(&rpcbuf, pImage->byte_order);
        x_rpcbuf_reserve(&rpcbuf, sizeof(CARD16)); /* pad1; */
        x_rpcbuf_write_binary_pad(&rpcbuf, pImage->guid, 16);
        x_rpcbuf_write_CARD8(&rpcbuf, pImage->bits_per_pixel);
        x_rpcbuf_write_CARD8(&rpcbuf, pImage->num_planes);
        x_rpcbuf_reserve(&rpcbuf, sizeof(CARD16)); /* pad2; */
        x_rpcbuf_write_CARD8(&rpcbuf, pImage->depth);
        x_rpcbuf_reserve(&rpcbuf, sizeof(CARD8)+sizeof(CARD16)); /* pad3, pad4 */
        x_rpcbuf_write_CARD32(&rpcbuf, pImage->red_mask);
        x_rpcbuf_write_CARD32(&rpcbuf, pImage->green_mask);
        x_rpcbuf_write_CARD32(&rpcbuf, pImage->blue_mask);
        x_rpcbuf_write_CARD8(&rpcbuf, pImage->format);
        x_rpcbuf_reserve(&rpcbuf, sizeof(CARD8)+sizeof(CARD16)); /* pad5, pad6 */
        x_rpcbuf_write_CARD32(&rpcbuf, pImage->y_sample_bits);
        x_rpcbuf_write_CARD32(&rpcbuf, pImage->u_sample_bits);
        x_rpcbuf_write_CARD32(&rpcbuf, pImage->v_sample_bits);
        x_rpcbuf_write_CARD32(&rpcbuf, pImage->horz_y_period);
        x_rpcbuf_write_CARD32(&rpcbuf, pImage->horz_u_period);
        x_rpcbuf_write_CARD32(&rpcbuf, pImage->horz_v_period);
        x_rpcbuf_write_CARD32(&rpcbuf, pImage->vert_y_period);
        x_rpcbuf_write_CARD32(&rpcbuf, pImage->vert_u_period);
        x_rpcbuf_write_CARD32(&rpcbuf, pImage->vert_v_period);
        x_rpcbuf_write_binary_pad(&rpcbuf, pImage->component_order, 32);
        x_rpcbuf_write_CARD8(&rpcbuf, pImage->scanline_order);
        x_rpcbuf_reserve(&rpcbuf, sizeof(CARD8)+sizeof(CARD16)+(sizeof(CARD32)*2)); /* pad7, pad8, pad9, pad10 */
    }

    /* use rpc.wpos here, in order to get how much we've really written */
    if (rpcbuf.wpos != (pPort->pAdaptor->nImages*sz_xvImageFormatInfo))
        LogMessage(X_WARNING, "ProcXvListImageFormats() payload_len mismatch: %ld but shoud be %d\n",
                   rpcbuf.wpos, (pPort->pAdaptor->nImages*sz_xvImageFormatInfo));

    xvListImageFormatsReply reply = {
        .num_formats = pPort->pAdaptor->nImages,
    };

    if (client->swapped) {
        swapl(&reply.num_formats);
    }

    return X_SEND_REPLY_WITH_RPCBUF(client, reply, rpcbuf);
}

int
ProcXvDispatch(ClientPtr client)
{
    REQUEST(xReq);

    UpdateCurrentTime();

    switch (stuff->data) {
        case xv_QueryExtension:
            return ProcXvQueryExtension(client);
        case xv_QueryAdaptors:
            return ProcXvQueryAdaptors(client);
        case xv_QueryEncodings:
            return ProcXvQueryEncodings(client);
        case xv_GrabPort:
            return ProcXvGrabPort(client);
        case xv_UngrabPort:
            return ProcXvUngrabPort(client);
        case xv_PutVideo:
            return ProcXvPutVideo(client);
        case xv_PutStill:
            return ProcXvPutStill(client);
        case xv_GetVideo:
            return ProcXvGetVideo(client);
        case xv_GetStill:
            return ProcXvGetStill(client);
        case xv_StopVideo:
            return ProcXvStopVideo(client);
        case xv_SelectVideoNotify:
            return ProcXvSelectVideoNotify(client);
        case xv_SelectPortNotify:
            return ProcXvSelectPortNotify(client);
        case xv_QueryBestSize:
            return ProcXvQueryBestSize(client);
        case xv_SetPortAttribute:
            return ProcXvSetPortAttribute(client);
        case xv_GetPortAttribute:
            return ProcXvGetPortAttribute(client);
        case xv_QueryPortAttributes:
            return ProcXvQueryPortAttributes(client);
        case xv_ListImageFormats:
            return ProcXvListImageFormats(client);
        case xv_QueryImageAttributes:
            return ProcXvQueryImageAttributes(client);
        case xv_PutImage:
            return ProcXvPutImage(client);
        case xv_ShmPutImage:
            return ProcXvShmPutImage(client);
        default:
            return BadRequest;
    }
}

#ifdef XINERAMA
static int
XineramaXvStopVideo(ClientPtr client)
{
    int result;
    PanoramiXRes *draw, *port;

    REQUEST(xvStopVideoReq);

    result = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                      XRC_DRAWABLE, client, DixWriteAccess);
    if (result != Success)
        return (result == BadValue) ? BadDrawable : result;

    result = dixLookupResourceByType((void **) &port, stuff->port,
                                     XvXRTPort, client, DixReadAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        if (port->info[walkScreenIdx].id) {
            stuff->drawable = draw->info[walkScreenIdx].id;
            stuff->port = port->info[walkScreenIdx].id;
            result = SingleXvStopVideo(client);
        }
    });

    return result;
}

static int
XineramaXvSetPortAttribute(ClientPtr client)
{
    REQUEST(xvSetPortAttributeReq);
    PanoramiXRes *port;
    int result;

    result = dixLookupResourceByType((void **) &port, stuff->port,
                                     XvXRTPort, client, DixReadAccess);
    if (result != Success)
        return result;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        if (port->info[walkScreenIdx].id) {
            stuff->port = port->info[walkScreenIdx].id;
            result = SingleXvSetPortAttribute(client);
        }
    });

    return result;
}

#ifdef CONFIG_MITSHM
static int
XineramaXvShmPutImage(ClientPtr client)
{
    REQUEST(xvShmPutImageReq);
    PanoramiXRes *draw, *gc, *port;
    Bool send_event;
    Bool isRoot;
    int result, x, y;

    send_event = stuff->send_event;

    result = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                      XRC_DRAWABLE, client, DixWriteAccess);
    if (result != Success)
        return (result == BadValue) ? BadDrawable : result;

    result = dixLookupResourceByType((void **) &gc, stuff->gc,
                                     XRT_GC, client, DixReadAccess);
    if (result != Success)
        return result;

    result = dixLookupResourceByType((void **) &port, stuff->port,
                                     XvXRTPort, client, DixReadAccess);
    if (result != Success)
        return result;

    isRoot = (draw->type == XRT_WINDOW) && draw->u.win.root;

    x = stuff->drw_x;
    y = stuff->drw_y;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        if (port->info[walkScreenIdx].id) {
            stuff->drawable = draw->info[walkScreenIdx].id;
            stuff->port = port->info[walkScreenIdx].id;
            stuff->gc = gc->info[walkScreenIdx].id;
            stuff->drw_x = x;
            stuff->drw_y = y;
            if (isRoot) {
                stuff->drw_x -= walkScreen->x;
                stuff->drw_y -= walkScreen->y;
            }
            stuff->send_event = (send_event && !walkScreenIdx) ? 1 : 0;

            result = SingleXvShmPutImage(client);
        }
    });

    return result;
}
#else /* CONFIG_MITSHM */
#define XineramaXvShmPutImage ProcXvShmPutImage
#endif /* CONFIG_MITSHM */

static int
XineramaXvPutImage(ClientPtr client)
{
    REQUEST(xvPutImageReq);
    PanoramiXRes *draw, *gc, *port;
    Bool isRoot;
    int result, x, y;

    result = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                      XRC_DRAWABLE, client, DixWriteAccess);
    if (result != Success)
        return (result == BadValue) ? BadDrawable : result;

    result = dixLookupResourceByType((void **) &gc, stuff->gc,
                                     XRT_GC, client, DixReadAccess);
    if (result != Success)
        return result;

    result = dixLookupResourceByType((void **) &port, stuff->port,
                                     XvXRTPort, client, DixReadAccess);
    if (result != Success)
        return result;

    isRoot = (draw->type == XRT_WINDOW) && draw->u.win.root;

    x = stuff->drw_x;
    y = stuff->drw_y;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        if (port->info[walkScreenIdx].id) {
            stuff->drawable = draw->info[walkScreenIdx].id;
            stuff->port = port->info[walkScreenIdx].id;
            stuff->gc = gc->info[walkScreenIdx].id;
            stuff->drw_x = x;
            stuff->drw_y = y;
            if (isRoot) {
                stuff->drw_x -= walkScreen->x;
                stuff->drw_y -= walkScreen->y;
            }

            result = SingleXvPutImage(client);
        }
    });

    return result;
}

static int
XineramaXvPutVideo(ClientPtr client)
{
    REQUEST(xvPutImageReq);
    PanoramiXRes *draw, *gc, *port;
    Bool isRoot;
    int result, x, y;

    result = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                      XRC_DRAWABLE, client, DixWriteAccess);
    if (result != Success)
        return (result == BadValue) ? BadDrawable : result;

    result = dixLookupResourceByType((void **) &gc, stuff->gc,
                                     XRT_GC, client, DixReadAccess);
    if (result != Success)
        return result;

    result = dixLookupResourceByType((void **) &port, stuff->port,
                                     XvXRTPort, client, DixReadAccess);
    if (result != Success)
        return result;

    isRoot = (draw->type == XRT_WINDOW) && draw->u.win.root;

    x = stuff->drw_x;
    y = stuff->drw_y;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        if (port->info[walkScreenIdx].id) {
            stuff->drawable = draw->info[walkScreenIdx].id;
            stuff->port = port->info[walkScreenIdx].id;
            stuff->gc = gc->info[walkScreenIdx].id;
            stuff->drw_x = x;
            stuff->drw_y = y;
            if (isRoot) {
                stuff->drw_x -= walkScreen->x;
                stuff->drw_y -= walkScreen->y;
            }

            result = SingleXvPutVideo(client);
        }
    });

    return result;
}

static int
XineramaXvPutStill(ClientPtr client)
{
    REQUEST(xvPutImageReq);
    PanoramiXRes *draw, *gc, *port;
    Bool isRoot;
    int result, x, y;

    result = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                      XRC_DRAWABLE, client, DixWriteAccess);
    if (result != Success)
        return (result == BadValue) ? BadDrawable : result;

    result = dixLookupResourceByType((void **) &gc, stuff->gc,
                                     XRT_GC, client, DixReadAccess);
    if (result != Success)
        return result;

    result = dixLookupResourceByType((void **) &port, stuff->port,
                                     XvXRTPort, client, DixReadAccess);
    if (result != Success)
        return result;

    isRoot = (draw->type == XRT_WINDOW) && draw->u.win.root;

    x = stuff->drw_x;
    y = stuff->drw_y;

    XINERAMA_FOR_EACH_SCREEN_BACKWARD({
        if (port->info[walkScreenIdx].id) {
            stuff->drawable = draw->info[walkScreenIdx].id;
            stuff->port = port->info[walkScreenIdx].id;
            stuff->gc = gc->info[walkScreenIdx].id;
            stuff->drw_x = x;
            stuff->drw_y = y;
            if (isRoot) {
                stuff->drw_x -= walkScreen->x;
                stuff->drw_y -= walkScreen->y;
            }
            result = SingleXvPutStill(client);
        }
    });

    return result;
}

static Bool
isImageAdaptor(XvAdaptorPtr pAdapt)
{
    return (pAdapt->type & XvImageMask) && (pAdapt->nImages > 0);
}

static Bool
hasOverlay(XvAdaptorPtr pAdapt)
{
    int i;

    for (i = 0; i < pAdapt->nAttributes; i++)
        if (!strcmp(pAdapt->pAttributes[i].name, "XV_COLORKEY"))
            return TRUE;
    return FALSE;
}

static XvAdaptorPtr
matchAdaptor(ScreenPtr pScreen, XvAdaptorPtr refAdapt, Bool isOverlay)
{
    int i;
    XvScreenPtr xvsp =
        dixLookupPrivate(&pScreen->devPrivates, XvGetScreenKey());
    /* Do not try to go on if xv is not supported on this screen */
    if (xvsp == NULL)
        return NULL;

    /* if the adaptor has the same name it's a perfect match */
    for (i = 0; i < xvsp->nAdaptors; i++) {
        XvAdaptorPtr pAdapt = xvsp->pAdaptors + i;

        if (!strcmp(refAdapt->name, pAdapt->name))
            return pAdapt;
    }

    /* otherwise we only look for XvImage adaptors */
    if (!isImageAdaptor(refAdapt))
        return NULL;

    /* prefer overlay/overlay non-overlay/non-overlay pairing */
    for (i = 0; i < xvsp->nAdaptors; i++) {
        XvAdaptorPtr pAdapt = xvsp->pAdaptors + i;

        if (isImageAdaptor(pAdapt) && isOverlay == hasOverlay(pAdapt))
            return pAdapt;
    }

    /* but we'll take any XvImage pairing if we can get it */
    for (i = 0; i < xvsp->nAdaptors; i++) {
        XvAdaptorPtr pAdapt = xvsp->pAdaptors + i;

        if (isImageAdaptor(pAdapt))
            return pAdapt;
    }
    return NULL;
}

void
XineramifyXv(void)
{
    XvScreenPtr xvsp0 =
        dixLookupPrivate(&(dixGetMasterScreen()->devPrivates), XvGetScreenKey());
    XvAdaptorPtr MatchingAdaptors[MAXSCREENS];
    int i;

    XvXRTPort = CreateNewResourceType(XineramaDeleteResource, "XvXRTPort");

    if (!xvsp0 || !XvXRTPort)
        return;
    SetResourceTypeErrorValue(XvXRTPort, _XvBadPort);

    for (i = 0; i < xvsp0->nAdaptors; i++) {
        Bool isOverlay;
        XvAdaptorPtr refAdapt = xvsp0->pAdaptors + i;

        if (!(refAdapt->type & XvInputMask))
            continue;

        MatchingAdaptors[0] = refAdapt;
        isOverlay = hasOverlay(refAdapt);

        XINERAMA_FOR_EACH_SCREEN_FORWARD_SKIP0({
            MatchingAdaptors[walkScreenIdx] = matchAdaptor(walkScreen, refAdapt, isOverlay);
        });

        /* now create a resource for each port */
        for (int j = 0; j < refAdapt->nPorts; j++) {
            PanoramiXRes *port = calloc(1, sizeof(PanoramiXRes));

            if (!port)
                break;

            XINERAMA_FOR_EACH_SCREEN_BACKWARD({
                if (MatchingAdaptors[walkScreenIdx] && (MatchingAdaptors[walkScreenIdx]->nPorts > j))
                    port->info[walkScreenIdx].id = MatchingAdaptors[walkScreenIdx]->base_id + j;
                else
                    port->info[walkScreenIdx].id = 0;
            });

            AddResource(port->info[0].id, XvXRTPort, port);
        }
    }

    xvUseXinerama = 1;
}
#endif /* XINERAMA */
