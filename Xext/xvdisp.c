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
#include "dix/request_priv.h"
#include "Xext/xvdix_priv.h"
#include "Xext/panoramiX.h"
#include "Xext/panoramiXsrv.h"

#include "misc.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "gcstruct.h"
#include "dixstruct.h"
#include "resource.h"
#include "opaque.h"
#ifdef MITSHM
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
    REQUEST_HEAD_STRUCT(xvQueryExtensionReq);

    xvQueryExtensionReply rep = {
        .length = 0,
        .version = XvVersion,
        .revision = XvRevision
    };

    REPLY_FIELD_CARD16(version);
    REPLY_FIELD_CARD16(revision);
    REPLY_SEND();
    return Success;
}

static int
ProcXvQueryAdaptors(ClientPtr client)
{
    int totalSize, na, nf, rc;
    int nameSize;
    XvAdaptorPtr pa;
    XvFormatPtr pf;
    WindowPtr pWin;
    ScreenPtr pScreen;
    XvScreenPtr pxvs;

    REQUEST_HEAD_STRUCT(xvQueryAdaptorsReq);
    REQUEST_FIELD_CARD32(window);

    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    pScreen = pWin->drawable.pScreen;
    pxvs = (XvScreenPtr) dixLookupPrivate(&pScreen->devPrivates,
                                          XvGetScreenKey());
    if (!pxvs) {
        xvQueryAdaptorsReply rep = { 0 };
        REPLY_SEND();
        return Success;
    }

    /* CALCULATE THE TOTAL SIZE OF THE REPLY IN BYTES */

    totalSize = pxvs->nAdaptors * sz_xvAdaptorInfo;

    /* FOR EACH ADPATOR ADD UP THE BYTES FOR ENCODINGS AND FORMATS */

    na = pxvs->nAdaptors;
    pa = pxvs->pAdaptors;
    while (na--) {
        totalSize += pad_to_int32(strlen(pa->name));
        totalSize += pa->nFormats * sz_xvFormat;
        pa++;
    }

    char *payload = calloc(1, totalSize);
    if (!payload)
        return BadAlloc;
    char *walk = payload;

    na = pxvs->nAdaptors;
    pa = pxvs->pAdaptors;
    while (na--) {
        xvAdaptorInfo *ainfo = (xvAdaptorInfo*)walk;

        ainfo->base_id = pa->base_id;
        ainfo->num_ports = pa->nPorts;
        ainfo->type = pa->type;
        ainfo->name_size = nameSize = strlen(pa->name);
        ainfo->num_formats = pa->nFormats;

        CLIENT_STRUCT_CARD32_1(ainfo, base_id);
        CLIENT_STRUCT_CARD16_3(ainfo, name_size, num_ports, num_formats);

        walk += sizeof(ainfo);
        memcpy(walk, pa->name, nameSize);
        walk += pad_to_int32(nameSize);

        nf = pa->nFormats;
        pf = pa->pFormats;
        while (nf--) {
            xvFormat *format = (xvFormat *)walk;
            format->depth = pf->depth;
            format->visual = pf->visual;
            CLIENT_STRUCT_CARD32_1(format, visual);
            pf++;
        }

        pa++;
    }

    xvQueryAdaptorsReply rep = {
        rep.num_adaptors = pxvs->nAdaptors,
    };

    REPLY_FIELD_CARD16(num_adaptors);
    REPLY_SEND_EXTRA(payload, totalSize);
    free(payload);
    return Success;
}

static int
ProcXvQueryEncodings(ClientPtr client)
{
    int totalSize;
    XvPortPtr pPort;
    int ne;
    XvEncodingPtr pe;

    REQUEST_HEAD_STRUCT(xvQueryEncodingsReq);
    REQUEST_FIELD_CARD32(port);

    VALIDATE_XV_PORT(stuff->port, pPort, DixReadAccess);

    /* FOR EACH ENCODING ADD UP THE BYTES FOR ENCODING NAMES */

    ne = pPort->pAdaptor->nEncodings;
    pe = pPort->pAdaptor->pEncodings;
    totalSize = ne * sz_xvEncodingInfo;
    while (ne--) {
        totalSize += pad_to_int32(strlen(pe->name));
        pe++;
    }

    char *buf = calloc(1, totalSize);
    if (!buf)
        return BadAlloc;
    char *walk = buf;

    xvQueryEncodingsReply rep = {
        .num_encodings = pPort->pAdaptor->nEncodings,
        .length = bytes_to_int32(totalSize),
    };

    ne = pPort->pAdaptor->nEncodings;
    pe = pPort->pAdaptor->pEncodings;
    while (ne--) {
        int nameSize;
        xvEncodingInfo *einfo = (xvEncodingInfo*)walk;

        einfo->encoding = pe->id;
        einfo->name_size = nameSize = strlen(pe->name);
        einfo->width = pe->width;
        einfo->height = pe->height;
        einfo->rate.numerator = pe->rate.numerator;
        einfo->rate.denominator = pe->rate.denominator;

        CLIENT_STRUCT_CARD32_3(einfo, encoding, rate.numerator, rate.denominator);
        CLIENT_STRUCT_CARD16_3(einfo, name_size, width, height);

        walk += sizeof(xvEncodingInfo);
        memcpy(walk, pe->name, nameSize);
        walk += pad_to_int32(nameSize);

        pe++;
    }

    REPLY_FIELD_CARD16(num_encodings);
    REPLY_SEND_EXTRA(buf, totalSize);
    return Success;
}

static int
SingleXvPutVideo(ClientPtr client)
{
    DrawablePtr pDraw;
    XvPortPtr pPort;
    GCPtr pGC;
    int status;

    REQUEST_HEAD_STRUCT(xvPutVideoReq);

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

static int XineramaXvPutVideo(ClientPtr client);

static int
ProcXvPutVideo(ClientPtr client)
{
    REQUEST_HEAD_STRUCT(xvPutVideoReq);
    REQUEST_FIELD_CARD32(port);
    REQUEST_FIELD_CARD32(drawable);
    REQUEST_FIELD_CARD32(gc);
    REQUEST_FIELD_CARD16(vid_x);
    REQUEST_FIELD_CARD16(vid_y);
    REQUEST_FIELD_CARD16(vid_w);
    REQUEST_FIELD_CARD16(vid_h);
    REQUEST_FIELD_CARD16(drw_x);
    REQUEST_FIELD_CARD16(drw_y);
    REQUEST_FIELD_CARD16(drw_w);
    REQUEST_FIELD_CARD16(drw_h);

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

    REQUEST_HEAD_STRUCT(xvPutStillReq);

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

static int XineramaXvPutStill(ClientPtr client);

static int
ProcXvPutStill(ClientPtr client)
{
    REQUEST_HEAD_STRUCT(xvPutStillReq);
    REQUEST_FIELD_CARD32(port);
    REQUEST_FIELD_CARD32(drawable);
    REQUEST_FIELD_CARD32(gc);
    REQUEST_FIELD_CARD16(vid_x);
    REQUEST_FIELD_CARD16(vid_y);
    REQUEST_FIELD_CARD16(vid_w);
    REQUEST_FIELD_CARD16(vid_h);
    REQUEST_FIELD_CARD16(drw_x);
    REQUEST_FIELD_CARD16(drw_y);
    REQUEST_FIELD_CARD16(drw_w);
    REQUEST_FIELD_CARD16(drw_h);

#ifdef XINERAMA
    if (xvUseXinerama)
        return XineramaXvPutStill(client);
#endif
    return SingleXvPutStill(client);
}

static int
ProcXvGetVideo(ClientPtr client)
{
    DrawablePtr pDraw;
    XvPortPtr pPort;
    GCPtr pGC;
    int status;

    REQUEST_HEAD_STRUCT(xvGetVideoReq);
    REQUEST_FIELD_CARD32(port);
    REQUEST_FIELD_CARD32(drawable);
    REQUEST_FIELD_CARD32(gc);
    REQUEST_FIELD_CARD16(vid_x);
    REQUEST_FIELD_CARD16(vid_y);
    REQUEST_FIELD_CARD16(vid_w);
    REQUEST_FIELD_CARD16(vid_h);
    REQUEST_FIELD_CARD16(drw_x);
    REQUEST_FIELD_CARD16(drw_y);
    REQUEST_FIELD_CARD16(drw_w);
    REQUEST_FIELD_CARD16(drw_h);

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
    DrawablePtr pDraw;
    XvPortPtr pPort;
    GCPtr pGC;
    int status;

    REQUEST_HEAD_STRUCT(xvGetStillReq);
    REQUEST_FIELD_CARD32(port);
    REQUEST_FIELD_CARD32(drawable);
    REQUEST_FIELD_CARD32(gc);
    REQUEST_FIELD_CARD16(vid_x);
    REQUEST_FIELD_CARD16(vid_y);
    REQUEST_FIELD_CARD16(vid_w);
    REQUEST_FIELD_CARD16(vid_h);
    REQUEST_FIELD_CARD16(drw_x);
    REQUEST_FIELD_CARD16(drw_y);
    REQUEST_FIELD_CARD16(drw_w);
    REQUEST_FIELD_CARD16(drw_h);

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

    REQUEST_HEAD_STRUCT(xvSelectVideoNotifyReq);
    REQUEST_FIELD_CARD32(drawable);

    rc = dixLookupDrawable(&pDraw, stuff->drawable, client, 0,
                           DixReceiveAccess);
    if (rc != Success)
        return rc;

    return XvdiSelectVideoNotify(client, pDraw, stuff->onoff);
}

static int
ProcXvSelectPortNotify(ClientPtr client)
{
    XvPortPtr pPort;

    REQUEST_HEAD_STRUCT(xvSelectPortNotifyReq);
    REQUEST_FIELD_CARD32(port);

    VALIDATE_XV_PORT(stuff->port, pPort, DixReadAccess);

    return XvdiSelectPortNotify(client, pPort, stuff->onoff);
}

static int
ProcXvGrabPort(ClientPtr client)
{
    int result, status;
    XvPortPtr pPort;

    REQUEST_HEAD_STRUCT(xvGrabPortReq);
    REQUEST_FIELD_CARD32(port);
    REQUEST_FIELD_CARD32(time);

    VALIDATE_XV_PORT(stuff->port, pPort, DixReadAccess);

    status = XvdiGrabPort(client, pPort, stuff->time, &result);

    if (status != Success) {
        return status;
    }
    xvGrabPortReply rep = {
        .result = result
    };

    REPLY_SEND();
    return Success;
}

static int
ProcXvUngrabPort(ClientPtr client)
{
    XvPortPtr pPort;

    REQUEST_HEAD_STRUCT(xvGrabPortReq);
    REQUEST_FIELD_CARD32(port);
    REQUEST_FIELD_CARD32(time);

    VALIDATE_XV_PORT(stuff->port, pPort, DixReadAccess);

    return XvdiUngrabPort(client, pPort, stuff->time);
}

static int
SingleXvStopVideo(ClientPtr client)
{
    int ret;
    DrawablePtr pDraw;
    XvPortPtr pPort;

    REQUEST_HEAD_STRUCT(xvStopVideoReq);
    REQUEST_FIELD_CARD32(port);
    REQUEST_FIELD_CARD32(drawable);

    VALIDATE_XV_PORT(stuff->port, pPort, DixReadAccess);

    ret = dixLookupDrawable(&pDraw, stuff->drawable, client, 0, DixWriteAccess);
    if (ret != Success)
        return ret;

    return XvdiStopVideo(client, pPort, pDraw);
}

static int XineramaXvStopVideo(ClientPtr client);

static int
ProcXvStopVideo(ClientPtr client)
{
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

    REQUEST_HEAD_STRUCT(xvSetPortAttributeReq);
    REQUEST_FIELD_CARD32(port);
    REQUEST_FIELD_CARD32(attribute);
    REQUEST_FIELD_CARD32(value);

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

static int XineramaXvSetPortAttribute(ClientPtr client);

static int
ProcXvSetPortAttribute(ClientPtr client)
{
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

    REQUEST_HEAD_STRUCT(xvGetPortAttributeReq);
    REQUEST_FIELD_CARD32(port);
    REQUEST_FIELD_CARD32(attribute);

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

    xvGetPortAttributeReply rep = {
        .value = value
    };

    REPLY_FIELD_CARD32(value);
    REPLY_SEND();
    return Success;
}

static int
ProcXvQueryBestSize(ClientPtr client)
{
    unsigned int actual_width, actual_height;
    XvPortPtr pPort;

    REQUEST_HEAD_STRUCT(xvQueryBestSizeReq);
    REQUEST_FIELD_CARD32(port);
    REQUEST_FIELD_CARD16(vid_w);
    REQUEST_FIELD_CARD16(vid_h);
    REQUEST_FIELD_CARD16(drw_w);
    REQUEST_FIELD_CARD16(drw_h);

    VALIDATE_XV_PORT(stuff->port, pPort, DixReadAccess);

    (*pPort->pAdaptor->ddQueryBestSize) (pPort, stuff->motion,
                                         stuff->vid_w, stuff->vid_h,
                                         stuff->drw_w, stuff->drw_h,
                                         &actual_width, &actual_height);

    xvQueryBestSizeReply rep = {
        .actual_width = actual_width,
        .actual_height = actual_height
    };

    REPLY_FIELD_CARD16(actual_width);
    REPLY_FIELD_CARD16(actual_height);
    REPLY_SEND();
    return Success;
}

static int
ProcXvQueryPortAttributes(ClientPtr client)
{
    int size, i;
    XvPortPtr pPort;
    XvAttributePtr pAtt;

    REQUEST_HEAD_STRUCT(xvQueryPortAttributesReq);
    REQUEST_FIELD_CARD32(port);

    VALIDATE_XV_PORT(stuff->port, pPort, DixGetAttrAccess);

    int text_size = 0;
    for (i = 0, pAtt = pPort->pAdaptor->pAttributes;
         i < pPort->pAdaptor->nAttributes; i++, pAtt++) {
        text_size += pad_to_int32(strlen(pAtt->name) + 1);
    }

    int length = (pPort->pAdaptor->nAttributes * sz_xvAttributeInfo)
               + text_size;

    xvQueryPortAttributesReply rep = {
        .num_attributes = pPort->pAdaptor->nAttributes,
        .length = bytes_to_int32(length),
        .text_size = text_size,
    };

    char * buf = calloc(1, length);
    if (!buf)
        return BadAlloc;
    char * walk = buf;

    for (i = 0, pAtt = pPort->pAdaptor->pAttributes;
         i < pPort->pAdaptor->nAttributes; i++, pAtt++) {
        size = strlen(pAtt->name) + 1;  /* pass the NULL */

        xvAttributeInfo *Info = (xvAttributeInfo*)walk;
        Info->flags = pAtt->flags;
        Info->min = pAtt->min_value;
        Info->max = pAtt->max_value;
        Info->size = pad_to_int32(size);

        CLIENT_STRUCT_CARD32_4(Info, flags, size, min, max);

        walk += sizeof(xvAttributeInfo);

        memcpy(walk, pAtt->name, size);
        walk += pad_to_int32(size);
    }

    REPLY_FIELD_CARD32(num_attributes);
    REPLY_FIELD_CARD32(text_size);
    REPLY_SEND_EXTRA(buf, length);
    free(buf);
    return Success;
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

    REQUEST_HEAD_AT_LEAST(xvPutImageReq);

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

static int
XineramaXvPutImage(ClientPtr client);

static int
ProcXvPutImage(ClientPtr client)
{
    REQUEST_HEAD_AT_LEAST(xvPutImageReq);
    REQUEST_FIELD_CARD32(port);
    REQUEST_FIELD_CARD32(drawable);
    REQUEST_FIELD_CARD32(gc);
    REQUEST_FIELD_CARD32(id);
    REQUEST_FIELD_CARD16(src_x);
    REQUEST_FIELD_CARD16(src_y);
    REQUEST_FIELD_CARD16(src_w);
    REQUEST_FIELD_CARD16(src_h);
    REQUEST_FIELD_CARD16(drw_x);
    REQUEST_FIELD_CARD16(drw_y);
    REQUEST_FIELD_CARD16(drw_w);
    REQUEST_FIELD_CARD16(drw_h);
    REQUEST_FIELD_CARD16(width);
    REQUEST_FIELD_CARD16(height);

#ifdef XINERAMA
    if (xvUseXinerama)
        return XineramaXvPutImage(client);
#endif
    return SingleXvPutImage(client);
}

#ifdef MITSHM

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

    REQUEST_HEAD_STRUCT(xvShmPutImageReq);

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

static int XineramaXvShmPutImage(ClientPtr client);
#endif /* MITSHM */

static int
ProcXvShmPutImage(ClientPtr client)
{
#ifdef MITSHM
    REQUEST_HEAD_STRUCT(xvShmPutImageReq);
    REQUEST_FIELD_CARD32(port);
    REQUEST_FIELD_CARD32(drawable);
    REQUEST_FIELD_CARD32(gc);
    REQUEST_FIELD_CARD32(shmseg);
    REQUEST_FIELD_CARD32(id);
    REQUEST_FIELD_CARD32(offset);
    REQUEST_FIELD_CARD16(src_x);
    REQUEST_FIELD_CARD16(src_y);
    REQUEST_FIELD_CARD16(src_w);
    REQUEST_FIELD_CARD16(src_h);
    REQUEST_FIELD_CARD16(drw_x);
    REQUEST_FIELD_CARD16(drw_y);
    REQUEST_FIELD_CARD16(drw_w);
    REQUEST_FIELD_CARD16(drw_h);
    REQUEST_FIELD_CARD16(width);
    REQUEST_FIELD_CARD16(height);

#ifdef XINERAMA
    if (xvUseXinerama)
        return XineramaXvShmPutImage(client);
#endif
    return SingleXvShmPutImage(client);
#else
    return BadImplementation;
#endif
}

#ifdef XvMCExtension
#include "xvmcext.h"
#endif

static int
ProcXvQueryImageAttributes(ClientPtr client)
{
    int size, num_planes, i;
    CARD16 width, height;
    XvImagePtr pImage = NULL;
    XvPortPtr pPort;
    int32_t *offsets;
    int32_t *pitches;

    REQUEST_HEAD_STRUCT(xvQueryImageAttributesReq);
    REQUEST_FIELD_CARD32(port);
    REQUEST_FIELD_CARD32(id);
    REQUEST_FIELD_CARD16(width);
    REQUEST_FIELD_CARD16(height);

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

    // allocating for `offsets` as well as `pitches` in one block
    // both having CARD32 * num_planes (actually int32_t put into CARD32)
    if (!(offsets = calloc(num_planes*2, sizeof(CARD32))))
        return BadAlloc;
    pitches = offsets + num_planes;

    width = stuff->width;
    height = stuff->height;

    size = (*pPort->pAdaptor->ddQueryImageAttributes) (pPort, pImage,
                                                       &width, &height, offsets,
                                                       pitches);

    xvQueryImageAttributesReply rep = {
        .num_planes = num_planes,
        .width = width,
        .height = height,
        .data_size = size
    };

    REPLY_FIELD_CARD32(num_planes);
    REPLY_FIELD_CARD32(data_size);
    REPLY_FIELD_CARD16(width);
    REPLY_FIELD_CARD16(height);
    REPLY_BUF_CARD32(offsets, num_planes * 2);
    REPLY_SEND_EXTRA(offsets, num_planes * sizeof(CARD32) * 2);
    free(offsets);
    return Success;
}

static int
ProcXvListImageFormats(ClientPtr client)
{
    XvPortPtr pPort;
    XvImagePtr pImage;
    int i;

    REQUEST_HEAD_STRUCT(xvListImageFormatsReq);
    REQUEST_FIELD_CARD32(port);

    VALIDATE_XV_PORT(stuff->port, pPort, DixReadAccess);

    int payload_size = pPort->pAdaptor->nImages * sz_xvImageFormatInfo;

    xvListImageFormatsReply rep = {
        .num_formats = pPort->pAdaptor->nImages,
        .length = bytes_to_int32(payload_size)
    };

    REPLY_FIELD_CARD32(num_formats);

    pImage = pPort->pAdaptor->pImages;

    xvImageFormatInfo *info = calloc(1, payload_size);
    if (!info)
        return BadAlloc;

    for (i = 0; i < pPort->pAdaptor->nImages; i++, pImage++) {
        info[i].id = pImage->id;
        info[i].type = pImage->type;
        info[i].byte_order = pImage->byte_order;
        memcpy(&info[i].guid, pImage->guid, 16);
        info[i].bpp = pImage->bits_per_pixel;
        info[i].num_planes = pImage->num_planes;
        info[i].depth = pImage->depth;
        info[i].red_mask = pImage->red_mask;
        info[i].green_mask = pImage->green_mask;
        info[i].blue_mask = pImage->blue_mask;
        info[i].format = pImage->format;
        info[i].y_sample_bits = pImage->y_sample_bits;
        info[i].u_sample_bits = pImage->u_sample_bits;
        info[i].v_sample_bits = pImage->v_sample_bits;
        info[i].horz_y_period = pImage->horz_y_period;
        info[i].horz_u_period = pImage->horz_u_period;
        info[i].horz_v_period = pImage->horz_v_period;
        info[i].vert_y_period = pImage->vert_y_period;
        info[i].vert_u_period = pImage->vert_u_period;
        info[i].vert_v_period = pImage->vert_v_period;
        memcpy(&info[i].comp_order, pImage->component_order, 32);
        info[i].scanline_order = pImage->scanline_order;

        CLIENT_STRUCT_CARD32_5(&info[i], id, red_mask, green_mask, blue_mask, y_sample_bits);
        CLIENT_STRUCT_CARD32_4(&info[i], u_sample_bits, v_sample_bits, horz_y_period, horz_u_period);
        CLIENT_STRUCT_CARD32_4(&info[i], horz_v_period, vert_y_period, vert_u_period, vert_v_period);
    }

    REPLY_SEND_EXTRA(info, payload_size);
    free(info);
    return Success;
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
    int result, i;
    PanoramiXRes *draw, *port;

    REQUEST(xvStopVideoReq);
    REQUEST_SIZE_MATCH(xvStopVideoReq);

    result = dixLookupResourceByClass((void **) &draw, stuff->drawable,
                                      XRC_DRAWABLE, client, DixWriteAccess);
    if (result != Success)
        return (result == BadValue) ? BadDrawable : result;

    result = dixLookupResourceByType((void **) &port, stuff->port,
                                     XvXRTPort, client, DixReadAccess);
    if (result != Success)
        return result;

    FOR_NSCREENS_BACKWARD(i) {
        if (port->info[i].id) {
            stuff->drawable = draw->info[i].id;
            stuff->port = port->info[i].id;
            result = SingleXvStopVideo(client);
        }
    }

    return result;
}

static int
XineramaXvSetPortAttribute(ClientPtr client)
{
    REQUEST(xvSetPortAttributeReq);
    PanoramiXRes *port;
    int result, i;

    REQUEST_SIZE_MATCH(xvSetPortAttributeReq);

    result = dixLookupResourceByType((void **) &port, stuff->port,
                                     XvXRTPort, client, DixReadAccess);
    if (result != Success)
        return result;

    FOR_NSCREENS_BACKWARD(i) {
        if (port->info[i].id) {
            stuff->port = port->info[i].id;
            result = SingleXvSetPortAttribute(client);
        }
    }
    return result;
}

#ifdef MITSHM
static int
XineramaXvShmPutImage(ClientPtr client)
{
    REQUEST(xvShmPutImageReq);
    PanoramiXRes *draw, *gc, *port;
    Bool send_event;
    Bool isRoot;
    int result, i, x, y;

    REQUEST_SIZE_MATCH(xvShmPutImageReq);

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

    FOR_NSCREENS_BACKWARD(i) {
        if (port->info[i].id) {
            stuff->drawable = draw->info[i].id;
            stuff->port = port->info[i].id;
            stuff->gc = gc->info[i].id;
            stuff->drw_x = x;
            stuff->drw_y = y;
            if (isRoot) {
                stuff->drw_x -= screenInfo.screens[i]->x;
                stuff->drw_y -= screenInfo.screens[i]->y;
            }
            stuff->send_event = (send_event && !i) ? 1 : 0;

            result = SingleXvShmPutImage(client);
        }
    }
    return result;
}
#endif

static int
XineramaXvPutImage(ClientPtr client)
{
    REQUEST(xvPutImageReq);
    PanoramiXRes *draw, *gc, *port;
    Bool isRoot;
    int result, i, x, y;

    REQUEST_AT_LEAST_SIZE(xvPutImageReq);

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

    FOR_NSCREENS_BACKWARD(i) {
        if (port->info[i].id) {
            stuff->drawable = draw->info[i].id;
            stuff->port = port->info[i].id;
            stuff->gc = gc->info[i].id;
            stuff->drw_x = x;
            stuff->drw_y = y;
            if (isRoot) {
                stuff->drw_x -= screenInfo.screens[i]->x;
                stuff->drw_y -= screenInfo.screens[i]->y;
            }

            result = SingleXvPutImage(client);
        }
    }
    return result;
}

static int
XineramaXvPutVideo(ClientPtr client)
{
    REQUEST(xvPutImageReq);
    PanoramiXRes *draw, *gc, *port;
    Bool isRoot;
    int result, i, x, y;

    REQUEST_AT_LEAST_SIZE(xvPutVideoReq);

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

    FOR_NSCREENS_BACKWARD(i) {
        if (port->info[i].id) {
            stuff->drawable = draw->info[i].id;
            stuff->port = port->info[i].id;
            stuff->gc = gc->info[i].id;
            stuff->drw_x = x;
            stuff->drw_y = y;
            if (isRoot) {
                stuff->drw_x -= screenInfo.screens[i]->x;
                stuff->drw_y -= screenInfo.screens[i]->y;
            }

            result = SingleXvPutVideo(client);
        }
    }
    return result;
}

static int
XineramaXvPutStill(ClientPtr client)
{
    REQUEST(xvPutImageReq);
    PanoramiXRes *draw, *gc, *port;
    Bool isRoot;
    int result, i, x, y;

    REQUEST_AT_LEAST_SIZE(xvPutImageReq);

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

    FOR_NSCREENS_BACKWARD(i) {
        if (port->info[i].id) {
            stuff->drawable = draw->info[i].id;
            stuff->port = port->info[i].id;
            stuff->gc = gc->info[i].id;
            stuff->drw_x = x;
            stuff->drw_y = y;
            if (isRoot) {
                stuff->drw_x -= screenInfo.screens[i]->x;
                stuff->drw_y -= screenInfo.screens[i]->y;
            }

            result = SingleXvPutStill(client);
        }
    }
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
        dixLookupPrivate(&screenInfo.screens[0]->devPrivates, XvGetScreenKey());
    XvAdaptorPtr MatchingAdaptors[MAXSCREENS];
    int i, j, k;

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
        FOR_NSCREENS_FORWARD_SKIP(j)
            MatchingAdaptors[j] =
            matchAdaptor(screenInfo.screens[j], refAdapt, isOverlay);

        /* now create a resource for each port */
        for (j = 0; j < refAdapt->nPorts; j++) {
            PanoramiXRes *port = calloc(1, sizeof(PanoramiXRes));

            if (!port)
                break;

            FOR_NSCREENS_BACKWARD(k) {
                if (MatchingAdaptors[k] && (MatchingAdaptors[k]->nPorts > j))
                    port->info[k].id = MatchingAdaptors[k]->base_id + j;
                else
                    port->info[k].id = 0;
            }
            AddResource(port->info[0].id, XvXRTPort, port);
        }
    }

    xvUseXinerama = 1;
}
#endif /* XINERAMA */
