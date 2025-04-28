/*
 * Copyright (c) 1998-2003 by The XFree86 Project, Inc.
 * Copyright © 2013 Red Hat
 * Copyright © 2014 Intel Corporation
 * Copyright © 2016 Red Hat
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *      Olivier Fourdan <ofourdan@redhat.com>
 *
 * Derived from the glamor_xf86_xv, ephyr_glamor_xv and xf86xv
 * implementations
 */

#include <xwayland-config.h>

#include <X11/extensions/Xv.h>

#include "dix/screen_hooks_priv.h"
#include "Xext/xvdix_priv.h"

#include "glamor_priv.h"
#include "xwayland-glamor.h"

#define NUM_FORMATS    3
#define NUM_PORTS      16
#define ADAPTOR_NAME   "glamor textured video"
#define ENCODER_NAME   "XV_IMAGE"

static DevPrivateKeyRec xwlXvScreenPrivateKeyRec;
#define xwlXvScreenPrivateKey (&xwlXvScreenPrivateKeyRec)

typedef struct {
    XvAdaptorPtr glxv_adaptor; /* We have only one adaptor, glamor Xv */
    glamor_port_private *port_privates;
} xwlXvScreenRec, *xwlXvScreenPtr;

typedef struct {
    char depth;
    short class;
} xwlVideoFormatRec, *xwlVideoFormatPtr;

static xwlVideoFormatRec Formats[NUM_FORMATS] = {
    {15, TrueColor},
    {16, TrueColor},
    {24, TrueColor}
};

static int
xwl_glamor_xv_stop_video(XvPortPtr   pPort,
                         DrawablePtr pDraw)
{
    glamor_port_private *gpp = (glamor_port_private *) (pPort->devPriv.ptr);

    if (pDraw->type != DRAWABLE_WINDOW)
        return BadAlloc;

    glamor_xv_stop_video(gpp);

    return Success;
}

static int
xwl_glamor_xv_set_port_attribute(XvPortPtr pPort,
                                 Atom      attribute,
                                 INT32     value)
{
    glamor_port_private *gpp = (glamor_port_private *) (pPort->devPriv.ptr);

    return glamor_xv_set_port_attribute(gpp, attribute, value);
}

static int
xwl_glamor_xv_get_port_attribute(XvPortPtr pPort,
                                 Atom      attribute,
                                 INT32    *pValue)
{
    glamor_port_private *gpp = (glamor_port_private *) (pPort->devPriv.ptr);

    return glamor_xv_get_port_attribute(gpp, attribute, pValue);
}

static int
xwl_glamor_xv_query_best_size(XvPortPtr     pPort,
                              CARD8         motion,
                              CARD16        vid_w,
                              CARD16        vid_h,
                              CARD16        drw_w,
                              CARD16        drw_h,
                              unsigned int *p_w,
                              unsigned int *p_h)
{
    *p_w = drw_w;
    *p_h = drw_h;

    return Success;
}

static int
xwl_glamor_xv_query_image_attributes(XvPortPtr  pPort,
                                     XvImagePtr format,
                                     CARD16    *width,
                                     CARD16    *height,
                                     int       *pitches,
                                     int       *offsets)
{
    return glamor_xv_query_image_attributes(format->id,
                                            width,
                                            height,
                                            pitches,
                                            offsets);
}

static int
xwl_glamor_xv_put_image(DrawablePtr    pDrawable,
                        XvPortPtr      pPort,
                        GCPtr          pGC,
                        INT16          src_x,
                        INT16          src_y,
                        CARD16         src_w,
                        CARD16         src_h,
                        INT16          drw_x,
                        INT16          drw_y,
                        CARD16         drw_w,
                        CARD16         drw_h,
                        XvImagePtr     format,
                        unsigned char *data,
                        Bool           sync,
                        CARD16         width,
                        CARD16         height)
{
    glamor_port_private *gpp = (glamor_port_private *) (pPort->devPriv.ptr);

    RegionRec WinRegion;
    RegionRec ClipRegion;
    BoxRec WinBox;
    int ret = Success;

    if (pDrawable->type != DRAWABLE_WINDOW)
        return BadWindow;

    WinBox.x1 = pDrawable->x + drw_x;
    WinBox.y1 = pDrawable->y + drw_y;
    WinBox.x2 = WinBox.x1 + drw_w;
    WinBox.y2 = WinBox.y1 + drw_h;

    RegionInit(&WinRegion, &WinBox, 1);
    RegionInit(&ClipRegion, NullBox, 1);
    RegionIntersect(&ClipRegion, &WinRegion, pGC->pCompositeClip);

    if (RegionNotEmpty(&ClipRegion))
        ret = glamor_xv_put_image(gpp,
                                  pDrawable,
                                  src_x,
                                  src_y,
                                  pDrawable->x + drw_x,
                                  pDrawable->y + drw_y,
                                  src_w,
                                  src_h,
                                  drw_w,
                                  drw_h,
                                  format->id,
                                  data,
                                  width,
                                  height,
                                  sync,
                                  &ClipRegion);

     RegionUninit(&WinRegion);
     RegionUninit(&ClipRegion);

     return ret;

}

static Bool
xwl_glamor_xv_add_formats(XvAdaptorPtr pa)
{
    ScreenPtr pScreen;
    XvFormatPtr pFormat, pf;
    VisualPtr pVisual;
    int numFormat;
    int totFormat;
    int numVisuals;
    int i;

    totFormat = NUM_FORMATS;
    pFormat = XNFcallocarray(totFormat, sizeof(XvFormatRec));
    pScreen = pa->pScreen;
    for (pf = pFormat, i = 0, numFormat = 0; i < NUM_FORMATS; i++) {
        numVisuals = pScreen->numVisuals;
        pVisual = pScreen->visuals;

        while (numVisuals--) {
           if ((pVisual->class == Formats[i].class) &&
               (pVisual->nplanes == Formats[i].depth)) {
                    if (numFormat >= totFormat) {
                        void *moreSpace;

                        totFormat *= 2;
                        moreSpace = XNFreallocarray(pFormat, totFormat,
                                                    sizeof(XvFormatRec));
                        pFormat = moreSpace;
                        pf = pFormat + numFormat;
                    }

                    pf->visual = pVisual->vid;
                    pf->depth = Formats[i].depth;

                    pf++;
                    numFormat++;
                }
            pVisual++;
        }
    }
    pa->nFormats = numFormat;
    pa->pFormats = pFormat;

    return numFormat != 0;
}

static Bool
xwl_glamor_xv_add_ports(XvAdaptorPtr pa)
{
    XvPortPtr pPorts, pp;
    xwlXvScreenPtr xwlXvScreen;
    unsigned long PortResource = 0;
    int nPorts;
    int i;

    pPorts = XNFcallocarray(NUM_PORTS, sizeof(XvPortRec));
    xwlXvScreen = dixLookupPrivate(&(pa->pScreen)->devPrivates,
                                   xwlXvScreenPrivateKey);
    xwlXvScreen->port_privates = XNFcallocarray(NUM_PORTS,
                                           sizeof(glamor_port_private));

    PortResource = XvGetRTPort();
    for (pp = pPorts, i = 0, nPorts = 0; i < NUM_PORTS; i++) {
        if (!(pp->id = FakeClientID(0)))
            continue;

        pp->pAdaptor = pa;

        glamor_xv_init_port(&xwlXvScreen->port_privates[i]);
        pp->devPriv.ptr = &xwlXvScreen->port_privates[i];

        if (AddResource(pp->id, PortResource, pp)) {
            pp++;
            nPorts++;
        }
    }

    pa->base_id = pPorts->id;
    pa->nPorts = nPorts;
    pa->pPorts = pPorts;

    return nPorts != 0;
}

static void
xwl_glamor_xv_add_attributes(XvAdaptorPtr pa)
{
    int i;

    pa->pAttributes = XNFcallocarray(glamor_xv_num_attributes, sizeof(XvAttributeRec));
    memcpy(pa->pAttributes, glamor_xv_attributes,
           glamor_xv_num_attributes * sizeof(XvAttributeRec));

    for (i = 0; i < glamor_xv_num_attributes; i++)
        pa->pAttributes[i].name = strdup(glamor_xv_attributes[i].name);

    pa->nAttributes = glamor_xv_num_attributes;
}

static void
xwl_glamor_xv_add_images(XvAdaptorPtr pa)
{
    pa->pImages = XNFcallocarray(glamor_xv_num_images, sizeof(XvImageRec));
    memcpy(pa->pImages, glamor_xv_images, glamor_xv_num_images * sizeof(XvImageRec));

    pa->nImages = glamor_xv_num_images;
}

static void
xwl_glamor_xv_add_encodings(XvAdaptorPtr pa)
{
    XvEncodingPtr pe;
    GLint texsize;

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texsize);

    pe = XNFcallocarray(1, sizeof(XvEncodingRec));
    pe->id = 0;
    pe->pScreen = pa->pScreen;
    pe->name = strdup(ENCODER_NAME);
    pe->width = texsize;
    pe->height = texsize;
    pe->rate.numerator = 1;
    pe->rate.denominator = 1;

    pa->pEncodings = pe;
    pa->nEncodings = 1;
}

static Bool
xwl_glamor_xv_add_adaptors(ScreenPtr pScreen)
{
    DevPrivateKey XvScreenKey;
    XvScreenPtr XvScreen;
    xwlXvScreenPtr xwlXvScreen;
    XvAdaptorPtr pa;

    if (XvScreenInit(pScreen) != Success)
        return FALSE;

    XvScreenKey = XvGetScreenKey();
    XvScreen = dixLookupPrivate(&(pScreen)->devPrivates, XvScreenKey);

    XvScreen->nAdaptors = 0;
    XvScreen->pAdaptors = NULL;

    pa = XNFcallocarray(1, sizeof(XvAdaptorRec));
    pa->pScreen = pScreen;
    pa->type = (unsigned char) (XvInputMask | XvImageMask);
    pa->ddStopVideo = xwl_glamor_xv_stop_video;
    pa->ddPutImage = xwl_glamor_xv_put_image;
    pa->ddSetPortAttribute = xwl_glamor_xv_set_port_attribute;
    pa->ddGetPortAttribute = xwl_glamor_xv_get_port_attribute;
    pa->ddQueryBestSize = xwl_glamor_xv_query_best_size;
    pa->ddQueryImageAttributes = xwl_glamor_xv_query_image_attributes;
    pa->name = strdup(ADAPTOR_NAME);

    xwl_glamor_xv_add_encodings(pa);
    xwl_glamor_xv_add_images(pa);
    xwl_glamor_xv_add_attributes(pa);
    if (!xwl_glamor_xv_add_formats(pa))
        goto failed;
    if (!xwl_glamor_xv_add_ports(pa))
        goto failed;

    /* We're good now with out Xv adaptor */
    XvScreen->nAdaptors = 1;
    XvScreen->pAdaptors = pa;

    xwlXvScreen = dixLookupPrivate(&(pa->pScreen)->devPrivates,
                                   xwlXvScreenPrivateKey);
    xwlXvScreen->glxv_adaptor = pa;

    return TRUE;

failed:
    XvFreeAdaptor(pa);
    free(pa);

    return FALSE;
}

static void xwl_glamor_xv_close_screen(CallbackListPtr *pcbl,
                                       ScreenPtr pScreen, void *unused)
{
    dixScreenUnhookClose(pScreen, xwl_glamor_xv_close_screen);

    xwlXvScreenPtr xwlXvScreen;
    xwlXvScreen = dixLookupPrivate(&(pScreen)->devPrivates,
                                   xwlXvScreenPrivateKey);

    if (!xwlXvScreen)
        return;

    if (xwlXvScreen->glxv_adaptor) {
        XvFreeAdaptor(xwlXvScreen->glxv_adaptor);
        free(xwlXvScreen->glxv_adaptor);
    }
    free(xwlXvScreen->port_privates);
    dixSetPrivate(&(pScreen)->devPrivates, xwlXvScreenPrivateKey, NULL);
}

Bool
xwl_glamor_xv_init(ScreenPtr pScreen)
{
    xwlXvScreenPtr xwlXvScreen;

    if (!dixRegisterPrivateKey(xwlXvScreenPrivateKey, PRIVATE_SCREEN,
                               sizeof(xwlXvScreenRec)))
        return FALSE;

    xwlXvScreen = dixLookupPrivate(&(pScreen)->devPrivates,
                                    xwlXvScreenPrivateKey);

    xwlXvScreen->port_privates = NULL;
    xwlXvScreen->glxv_adaptor = NULL;

    dixScreenHookClose(pScreen, xwl_glamor_xv_close_screen);

    glamor_xv_core_init(pScreen);

    return xwl_glamor_xv_add_adaptors(pScreen);
}
