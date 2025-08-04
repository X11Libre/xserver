/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */

#ifndef _XORG_XVDIX_PRIV_H
#define _XORG_XVDIX_PRIV_H

#include <X11/Xdefs.h>

#include "include/pixmap.h"
#include "include/regionstr.h"
#include "Xext/xvdix.h"

#define VALIDATE_XV_PORT(portID, pPort, mode)\
    {\
        int rc = dixLookupResourceByType((void **)&(pPort), portID,\
                                         XvRTPort, client, mode);\
        if (rc != Success)\
            return rc;\
    }

/* Errors */

#define _XvBadPort (XvBadPort+XvErrorBase)

typedef struct _XvPortNotifyRec {
    struct _XvPortNotifyRec *next;
    ClientPtr client;
    unsigned long id;
} XvPortNotifyRec;

extern int XvReqCode;
extern int XvErrorBase;

extern RESTYPE XvRTPort;

/* dispatch functions */
int ProcXvDispatch(ClientPtr);
int SProcXvDispatch(ClientPtr);

void XvFreeAdaptor(XvAdaptorPtr pAdaptor);

void XvFillColorKey(DrawablePtr pDraw, CARD32 key, RegionPtr region);

int XvdiSelectVideoNotify(ClientPtr client, DrawablePtr pDraw, BOOL onoff);
int XvdiSelectPortNotify(ClientPtr client, XvPortPtr pPort, BOOL onoff);

int XvdiPutVideo(ClientPtr client, DrawablePtr pDraw, XvPortPtr pPort,
                 GCPtr pGC, INT16 vid_x, INT16 vid_y, CARD16 vid_w,
                 CARD16 wid_h, INT16 drw_x, INT16 drw_y, CARD16 drw_w,
                 CARD16 drw_h);
int XvdiPutStill(ClientPtr client, DrawablePtr pDraw, XvPortPtr pPort,
                 GCPtr pGC, INT16 vid_x, INT16 vid_y, CARD16 vid_w,
                 CARD16 vid_h, INT16 drw_x, INT16 drw_y, CARD16 drw_w,
                 CARD16 drw_h);
int XvdiPutImage(ClientPtr client, DrawablePtr pDraw, XvPortPtr pPort,
                 GCPtr pGC, INT16 src_x, INT16 src_y, CARD16 src_w,
                 CARD16 src_h, INT16 drw_x, INT16 drw_y, CARD16 drw_w,
                 CARD16 drw_h, XvImagePtr image, unsigned char *data,
                 Bool sync, CARD16 width, CARD16 height);

int XvdiGetVideo(ClientPtr client, DrawablePtr pDraw, XvPortPtr pPort,
                 GCPtr pGC, INT16 vid_x, INT16 vid_y, CARD16 vid_w,
                 CARD16 vid_h, INT16 drw_x, INT16 drw_y, CARD16 drw_w,
                 CARD16 drw_h);
int XvdiGetStill(ClientPtr client, DrawablePtr pDraw, XvPortPtr pPort,
                 GCPtr pGC, INT16 vid_x, INT16 vid_y, CARD16 vid_w,
                 CARD16 vid_h, INT16 drw_x, INT16 drw_y, CARD16 drw_w,
                 CARD16 drw_h);

int XvdiSetPortAttribute(ClientPtr client, XvPortPtr pPort, Atom attribute,
                         INT32 value);
int XvdiGetPortAttribute(ClientPtr client, XvPortPtr pPort, Atom attribute,
                         INT32 *p_value);

int XvdiStopVideo(ClientPtr client, XvPortPtr pPort, DrawablePtr pDraw);

int XvdiMatchPort(XvPortPtr pPort, DrawablePtr pDraw);

int XvdiGrabPort(ClientPtr client, XvPortPtr pPort, Time ctime, int *p_result);
int XvdiUngrabPort(ClientPtr client, XvPortPtr pPort, Time ctime);

XvImagePtr XvMCFindXvImage(XvPortPtr pPort, CARD32 id);

#endif /* _XORG_XVDIX_PRIV_H */
