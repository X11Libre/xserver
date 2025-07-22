/* Copyright (c) 2003-2005 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPDIs2IED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Neither the name of the Advanced Micro Devices, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 * */

/*
 * File Contents:   This file consists of main Xfree video supported routines.
 *
 * Project:         Geode Xfree Frame buffer device driver.
 * */

/*
 * Fixes & Extensions to support Y800 greyscale modes
 * Alan Hourihane <alanh@fairlite.demon.co.uk>

 * code to allocate offscreen memory from EXA - is borrowed from Radeon
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "xf86.h"
#include "xf86_OSproc.h"
#include "compiler.h"
#include "xf86Pci.h"
#include "xf86fbman.h"
#include "regionstr.h"

#include "geode.h"
#include "xf86xv.h"
#include <X11/extensions/Xv.h>
#include "dixstruct.h"
#include "fourcc.h"
#include "geode_fourcc.h"

#define OFF_DELAY 		200     /* milliseconds */
#define FREE_DELAY 		60000

#define OFF_TIMER 		0x01
#define FREE_TIMER		0x02
#define CLIENT_VIDEO_ON	0x04

#define TIMER_MASK      (OFF_TIMER | FREE_TIMER)
#define XV_PROFILE 		0
#define REINIT  		1

#ifndef XvExtension
#error "It didn't work!"
void
GXInitVideo(ScreenPtr pScrn)
{
}

void
GXResetVideo(ScrnInfoPtr pScrni)
{
}

void
GXSetVideoPosition()
{
}
#else

#define DBUF 1
void GXResetVideo(ScrnInfoPtr pScrni);
static XF86VideoAdaptorPtr GXSetupImageVideo(ScreenPtr);
static void GXInitOffscreenImages(ScreenPtr);
static void GXStopVideo(ScrnInfoPtr, pointer, Bool);
static int GXSetPortAttribute(ScrnInfoPtr, Atom, INT32, pointer);
static int GXGetPortAttribute(ScrnInfoPtr, Atom, INT32 *, pointer);
static void GXQueryBestSize(ScrnInfoPtr, Bool,
                            short, short, short, short, unsigned int *,
                            unsigned int *, pointer);
static int GXPutImage(ScrnInfoPtr, short, short, short, short, short, short,
                      short, short, int, unsigned char *, short, short, Bool,
                      RegionPtr, pointer, DrawablePtr pDraw);

static void GXBlockHandler(BLOCKHANDLER_ARGS_DECL);
void GXSetVideoPosition(int x, int y, int width, int height,
                        short src_w, short src_h, short drw_w,
                        short drw_h, int id, int offset, ScrnInfoPtr pScrni);

extern int DeltaX, DeltaY;

unsigned long graphics_lut[256];
static int lutflag = 0;

#define MAKE_ATOM(a) MakeAtom(a, sizeof(a) - 1, TRUE)

static Atom xvColorKey, xvColorKeyMode, xvFilter
#if DBUF
, xvDoubleBuffer
#endif
;

#define PALETTE_ADDRESS   0x038
#define PALETTE_DATA      0x040
#define DISPLAY_CONFIG    0x008
#define MISC              0x050

static void
get_gamma_ram(unsigned long *lut)
{

    int i;

    gfx_write_vid32(PALETTE_ADDRESS, 0);

    for (i = 0; i < 256; i++)
        lut[i] = gfx_read_vid32(PALETTE_DATA);
}

/*----------------------------------------------------------------------------
 * GXInitVideo
 *
 * Description	:This is the initialization routine.It creates a new video
 * 				adapter and calls GXSetupImageVideo to initialize the adaptor
 * 				by filling XF86VideoAdaptorREc.Then it lists the existing
 * 				adaptors and adds the new one to it. Finally the list of
 * 				XF86VideoAdaptorPtr pointers are passed to the
 * 				xf86XVScreenInit().
 *
 * Parameters.
 *		pScrn	:Screen handler pointer having screen information.
 *
 * Returns		:none
 *
 * Comments		:none
 *----------------------------------------------------------------------------
 */
void
GXInitVideo(ScreenPtr pScrn)
{
    GeodeRec *pGeode;
    ScrnInfoPtr pScrni = xf86ScreenToScrn(pScrn);

    pGeode = GEODEPTR(pScrni);

    if (!pGeode->NoAccel) {
        XF86VideoAdaptorPtr *adaptors, *newAdaptors = NULL;
        XF86VideoAdaptorPtr newAdaptor = NULL;

        int num_adaptors;

        newAdaptor = GXSetupImageVideo(pScrn);
        GXInitOffscreenImages(pScrn);

        num_adaptors = xf86XVListGenericAdaptors(pScrni, &adaptors);

        if (newAdaptor) {
            if (!num_adaptors) {
                num_adaptors = 1;
                adaptors = &newAdaptor;
            }
            else {
                newAdaptors =   /* need to free this someplace */
                    malloc((num_adaptors + 1) * sizeof(XF86VideoAdaptorPtr *));
                if (newAdaptors) {
                    memcpy(newAdaptors, adaptors, num_adaptors *
                           sizeof(XF86VideoAdaptorPtr));
                    newAdaptors[num_adaptors] = newAdaptor;
                    adaptors = newAdaptors;
                    num_adaptors++;
                }
            }
        }

        if (num_adaptors)
            xf86XVScreenInit(pScrn, adaptors, num_adaptors);

        if (newAdaptors)
            free(newAdaptors);
    }
}

/* client libraries expect an encoding */
static XF86VideoEncodingRec DummyEncoding[1] = {
    {
     0,
     "XV_IMAGE",
     1024, 1024,
     {1, 1}
     }
};

#define NUM_FORMATS 4

static XF86VideoFormatRec Formats[NUM_FORMATS] = {
    {8, PseudoColor}, {15, TrueColor}, {16, TrueColor}, {24, TrueColor}
};

#if DBUF
#define NUM_ATTRIBUTES 4
#else
#define NUM_ATTRIBUTES 3
#endif

static XF86AttributeRec Attributes[NUM_ATTRIBUTES] = {
#if DBUF
    {XvSettable | XvGettable, 0, 1, (char *) "XV_DOUBLE_BUFFER"},
#endif
    {XvSettable | XvGettable, 0, (1 << 24) - 1, (char *) "XV_COLORKEY"},
    {XvSettable | XvGettable, 0, 1, (char *) "XV_FILTER"},
    {XvSettable | XvGettable, 0, 1, (char *) "XV_COLORKEYMODE"}
};

#define NUM_IMAGES 8

static XF86ImageRec Images[NUM_IMAGES] = {
    XVIMAGE_UYVY,
    XVIMAGE_YUY2,
    XVIMAGE_Y2YU,
    XVIMAGE_YVYU,
    XVIMAGE_Y800,
    XVIMAGE_I420,
    XVIMAGE_YV12,
    XVIMAGE_RGB565
};

typedef struct {
    void *area;
    int offset;
    RegionRec clip;
    CARD32 filter;
    CARD32 colorKey;
    CARD32 colorKeyMode;
    CARD32 videoStatus;
    Time offTime;
    Time freeTime;
#if DBUF
    Bool doubleBuffer;
    int currentBuffer;
#endif
} GeodePortPrivRec, *GeodePortPrivPtr;

#define GET_PORT_PRIVATE(pScrni) \
   (GeodePortPrivRec *)((GEODEPTR(pScrni))->adaptor->pPortPrivates[0].ptr)

/*----------------------------------------------------------------------------
 * GXSetColorKey
 *
 * Description	:This function reads the color key for the palette and
 *				  sets the video color key register.
 *
 * Parameters.
 * ScreenInfoPtr
 *		pScrni	:Screen  pointer having screen information.
 *		pPriv	:Video port private data
 *
 * Returns		:none
 *
 * Comments		:none
 *
 *----------------------------------------------------------------------------
 */
static INT32
GXSetColorkey(ScrnInfoPtr pScrni, GeodePortPrivRec *pPriv)
{
    int red, green, blue;
    unsigned long key;

    switch (pScrni->depth) {
    case 8:
        GFX(get_display_palette_entry(pPriv->colorKey & 0xFF, &key));
        red = ((key >> 16) & 0xFF);
        green = ((key >> 8) & 0xFF);
        blue = (key & 0xFF);
        break;
    case 16:
        red = (pPriv->colorKey & pScrni->mask.red) >>
            pScrni->offset.red << (8 - pScrni->weight.red);
        green = (pPriv->colorKey & pScrni->mask.green) >>
            pScrni->offset.green << (8 - pScrni->weight.green);
        blue = (pPriv->colorKey & pScrni->mask.blue) >>
            pScrni->offset.blue << (8 - pScrni->weight.blue);
        break;
    default:
        /* for > 16 bpp we send in the mask in xf86SetWeight. This
         * function is providing the offset by 1 more. So we take
         * this as a special case and subtract 1 for > 16
         */
        red = (pPriv->colorKey & pScrni->mask.red) >>
            (pScrni->offset.red - 1) << (8 - pScrni->weight.red);
        green = (pPriv->colorKey & pScrni->mask.green) >>
            (pScrni->offset.green - 1) << (8 - pScrni->weight.green);
        blue = (pPriv->colorKey & pScrni->mask.blue) >>
            (pScrni->offset.blue - 1) << (8 - pScrni->weight.blue);
        break;
    }

    GFX(set_video_color_key((blue | (green << 8) | (red << 16)), 0xFFFFFF,
                            (pPriv->colorKeyMode == 0)));
    REGION_EMPTY(pScrni->pScreen, &pPriv->clip);
    return 0;
}

/*----------------------------------------------------------------------------
 * GXResetVideo
 *
 * Description	: This function resets the video
 *
 * Parameters.
 *		pScrni	:Screen  pointer having screen information.
 *
 * Returns		:None
 *
 * Comments		:none
 *
 *----------------------------------------------------------------------------
 */
void
GXResetVideo(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    if (!pGeode->NoAccel) {
        GeodePortPrivRec *pPriv = pGeode->adaptor->pPortPrivates[0].ptr;

        GXAccelSync(pScrni);
        GXSetColorkey(pScrni, pPriv);
        GFX(set_video_filter(pPriv->filter, pPriv->filter));
    }
}

/*----------------------------------------------------------------------------
 * GXSetupImageVideo
 *
 * Description	: This function allocates space for a Videoadaptor and
 * 				initializes the XF86VideoAdaptorPtr record.
 *
 * Parameters.
 *		pScrn	:Screen handler pointer having screen information.
 *
 * Returns		:pointer to the initialized video adaptor record.
 *
 * Comments		:none
 *----------------------------------------------------------------------------
 */
static XF86VideoAdaptorPtr
GXSetupImageVideo(ScreenPtr pScrn)
{
    ScrnInfoPtr pScrni = xf86ScreenToScrn(pScrn);
    GeodeRec *pGeode = GEODEPTR(pScrni);
    XF86VideoAdaptorPtr adapt;
    GeodePortPrivRec *pPriv;

    if (!(adapt = calloc(1, sizeof(XF86VideoAdaptorRec) +
                         sizeof(GeodePortPrivRec) + sizeof(DevUnion))))
        return NULL;

    adapt->type = XvWindowMask | XvInputMask | XvImageMask;
    adapt->flags = VIDEO_OVERLAID_IMAGES | VIDEO_CLIP_TO_VIEWPORT;
    adapt->name = "Advanced Micro Devices";
    adapt->nEncodings = 1;
    adapt->pEncodings = DummyEncoding;
    adapt->nFormats = NUM_FORMATS;
    adapt->pFormats = Formats;
    adapt->nPorts = 1;
    adapt->pPortPrivates = (DevUnion *) (&adapt[1]);
    pPriv = (GeodePortPrivRec *) (&adapt->pPortPrivates[1]);
    adapt->pPortPrivates[0].ptr = (pointer) (pPriv);
    adapt->pAttributes = Attributes;
    adapt->nImages = NUM_IMAGES;
    adapt->nAttributes = NUM_ATTRIBUTES;
    adapt->pImages = Images;
    adapt->PutVideo = NULL;
    adapt->PutStill = NULL;
    adapt->GetVideo = NULL;
    adapt->GetStill = NULL;
    adapt->StopVideo = GXStopVideo;
    adapt->SetPortAttribute = GXSetPortAttribute;
    adapt->GetPortAttribute = GXGetPortAttribute;
    adapt->QueryBestSize = GXQueryBestSize;
    adapt->PutImage = GXPutImage;
    adapt->QueryImageAttributes = GeodeQueryImageAttributes;

    pPriv->filter = 0;
    pPriv->colorKey = 0;
    pPriv->colorKeyMode = 0;
    pPriv->videoStatus = 0;
#if DBUF
    pPriv->doubleBuffer = TRUE;
    pPriv->currentBuffer = 0;   /* init to first buffer */
#endif

    /* gotta uninit this someplace */
    REGION_NULL(pScrn, &pPriv->clip);

    pGeode->adaptor = adapt;

    pGeode->BlockHandler = pScrn->BlockHandler;
    pScrn->BlockHandler = GXBlockHandler;

    xvColorKey = MAKE_ATOM("XV_COLORKEY");
    xvColorKeyMode = MAKE_ATOM("XV_COLORKEYMODE");
    xvFilter = MAKE_ATOM("XV_FILTER");
#if DBUF
    xvDoubleBuffer = MAKE_ATOM("XV_DOUBLE_BUFFER");
#endif

    GXResetVideo(pScrni);

    return adapt;
}

/*----------------------------------------------------------------------------
 * GXStopVideo
 *
 * Description	:This function is used to stop input and output video
 *
 * Parameters.
 *		pScrni	:Screen handler pointer having screen information.
 *		data	:Pointer to the video port's private data
 *		exit	:Flag indicating whether the offscreen areas used for
 *				video to be deallocated or not.
 *
 * Returns		:none
 *
 * Comments		:none
 *----------------------------------------------------------------------------
 */
static void
GXStopVideo(ScrnInfoPtr pScrni, pointer data, Bool exit)
{
    GeodePortPrivRec *pPriv = (GeodePortPrivRec *) data;
    GeodeRec *pGeode = GEODEPTR(pScrni);

    REGION_EMPTY(pScrni->pScreen, &pPriv->clip);

    GXAccelSync(pScrni);
    if (exit) {
        if (pPriv->videoStatus & CLIENT_VIDEO_ON) {
            GFX(set_video_enable(0));

            /* If we have saved graphics LUT data - restore it */
            /* Otherwise, turn bypass on */

            if (lutflag)
                GFX(set_graphics_palette(graphics_lut));
            else
                GFX(set_video_palette_bypass(1));

            lutflag = 0;
        }

        if (pPriv->area) {
#ifdef XF86EXA
            if (pGeode->useEXA)
                exaOffscreenFree(pScrni->pScreen, pPriv->area);
#endif

            if (!pGeode->useEXA)
                xf86FreeOffscreenArea(pPriv->area);

            pPriv->area = NULL;
        }

        pPriv->videoStatus = 0;
        pGeode->OverlayON = FALSE;
    }
    else {
        if (pPriv->videoStatus & CLIENT_VIDEO_ON) {
            pPriv->videoStatus |= OFF_TIMER;
            pPriv->offTime = currentTime.milliseconds + OFF_DELAY;
        }
    }
}

/*----------------------------------------------------------------------------
 * GXSetPortAttribute
 *
 * Description		:This function is used to set the attributes of a port
 * 					like colorkeymode, double buffer support and filter.
 *
 * Parameters.
 *		pScrni		:Screen handler pointer having screen information.
 *		data		:Pointer to the video port's private data
 *		attribute	:The port attribute to be set
 *		value		:Value of the attribute to be set.
 *
 * Returns			:Success if the attribute is supported, else BadMatch
 *
 * Comments			:none
 *----------------------------------------------------------------------------
 */
static int
GXSetPortAttribute(ScrnInfoPtr pScrni,
                   Atom attribute, INT32 value, pointer data)
{
    GeodePortPrivRec *pPriv = (GeodePortPrivRec *) data;

    GXAccelSync(pScrni);
    if (attribute == xvColorKey) {
        pPriv->colorKey = value;
        GXSetColorkey(pScrni, pPriv);
    }
#if DBUF
    else if (attribute == xvDoubleBuffer) {
        if ((value < 0) || (value > 1))
            return BadValue;
        pPriv->doubleBuffer = value;
    }
#endif
    else if (attribute == xvColorKeyMode) {
        pPriv->colorKeyMode = value;
        GXSetColorkey(pScrni, pPriv);
    }
    else if (attribute == xvFilter) {
        if ((value < 0) || (value > 1))
            return BadValue;
        pPriv->filter = value;
    }
    else
        return BadMatch;

    return Success;
}

/*----------------------------------------------------------------------------
 * GXGetPortAttribute
 *
 * Description		:This function is used to get the attributes of a port
 * 					like hue, saturation,brightness or contrast.
 *
 * Parameters.
 *		pScrni		:Screen handler pointer having screen information.
 *		data		:Pointer to the video port's private data
 *		attribute	:The port attribute to be read
 *		value		:Pointer to the value of the attribute to be read.
 *
 * Returns			:Success if the attribute is supported, else BadMatch
 *
 * Comments			:none
 *----------------------------------------------------------------------------
 */
static int
GXGetPortAttribute(ScrnInfoPtr pScrni,
                   Atom attribute, INT32 *value, pointer data)
{
    GeodePortPrivRec *pPriv = (GeodePortPrivRec *) data;

    if (attribute == xvColorKey) {
        *value = pPriv->colorKey;
    }
#if DBUF
    else if (attribute == xvDoubleBuffer) {
        *value = (pPriv->doubleBuffer) ? 1 : 0;
    }
#endif
    else if (attribute == xvColorKeyMode) {
        *value = pPriv->colorKeyMode;
    }
    else if (attribute == xvFilter) {
        *value = pPriv->filter;
    }
    else
        return BadMatch;

    return Success;
}

/*----------------------------------------------------------------------------
 * GXQueryBestSize
 *
 * Description		:This function provides a way to query what the
 * 					destination dimensions would end up being if they were to
 * 					request that an area vid_w by vid_h from the video stream
 * 					be scaled to rectangle of drw_w by drw_h on the screen.
 *
 * Parameters.
 *		pScrni		:Screen handler pointer having screen information.
 *		data		:Pointer to the video port's private data
 *      vid_w,vid_h	:Width and height of the video data.
 *		drw_w,drw_h :Width and height of the scaled rectangle.
 *		p_w,p_h		:Width and height of the destination rectangle.
 *
 * Returns			:None
 *
 * Comments			:None
 *----------------------------------------------------------------------------
 */
static void
GXQueryBestSize(ScrnInfoPtr pScrni,
                Bool motion,
                short vid_w, short vid_h,
                short drw_w, short drw_h,
                unsigned int *p_w, unsigned int *p_h, pointer data)
{
    *p_w = drw_w;
    *p_h = drw_h;

    if (*p_w > 16384)
        *p_w = 16384;
}

/*----------------------------------------------------------------------------
 * GXCopyData420
 *
 * Description		: Copies data from src to destination
 *
 * Parameters.
 *		src			: pointer to the source data
 *		dst			: pointer to destination data
 *		srcPitch	: pitch of the srcdata
 *		dstPitch	: pitch of the destination data
 *		h & w		: height and width of source data
 *
 * Returns			:None
 *
 * Comments			:None
 *----------------------------------------------------------------------------
 */
static void
GXCopyData420(unsigned char *src, unsigned char *dst,
              int srcPitch, int dstPitch, int h, int w)
{
    while (h--) {
        memcpy(dst, src, w);
        src += srcPitch;
        dst += dstPitch;
    }
}

/*----------------------------------------------------------------------------
 * GXCopyData422
 *
 * Description		: Copies data from src to destination
 *
 * Parameters.
 *		src			: pointer to the source data
 *		dst			: pointer to destination data
 *		srcPitch	: pitch of the srcdata
 *		dstPitch	: pitch of the destination data
 *		h & w		: height and width of source data
 *
 * Returns			:None
 *
 * Comments			:None
 *----------------------------------------------------------------------------
 */
static void
GXCopyData422(unsigned char *src, unsigned char *dst,
              int srcPitch, int dstPitch, int h, int w)
{
    w <<= 1;
    while (h--) {
        memcpy(dst, src, w);
        src += srcPitch;
        dst += dstPitch;
    }
}

#ifdef XF86EXA
static void
GXVideoSave(ScreenPtr pScreen, ExaOffscreenArea *area)
{
    ScrnInfoPtr pScrni = xf86ScreenToScrn(pScreen);
    GeodePortPrivRec *pPriv = GET_PORT_PRIVATE(pScrni);

    if (area == pPriv->area)
        pPriv->area = NULL;
}
#endif

static int
GXAllocateMemory(ScrnInfoPtr pScrni, void **memp, int numlines)
{
    ScreenPtr pScrn = xf86ScrnToScreen(pScrni);
    GeodeRec *pGeode = GEODEPTR(pScrni);

    //long displayWidth = pGeode->Pitch / ((pScrni->bitsPerPixel + 7) / 8);
    int size = numlines * pGeode->displayWidth;

#if XF86EXA
    if (pGeode->useEXA) {
        ExaOffscreenArea *area = *memp;

        if (area != NULL) {
            if (area->size >= size)
                return area->offset;

            exaOffscreenFree(pScrni->pScreen, area);
        }

        area = exaOffscreenAlloc(pScrni->pScreen, size, 16,
                                 TRUE, GXVideoSave, NULL);
        *memp = area;

        return area == NULL ? 0 : area->offset;
    }
#endif

    if (!pGeode->useEXA) {
        FBAreaPtr area = *memp;
        FBAreaPtr new_area;

        if (area) {
            if ((area->box.y2 - area->box.y1) >= numlines)
                return (area->box.y1 * pGeode->Pitch);

            if (xf86ResizeOffscreenArea(area, pGeode->displayWidth, numlines))
                return (area->box.y1 * pGeode->Pitch);

            xf86FreeOffscreenArea(area);
        }

        new_area = xf86AllocateOffscreenArea(pScrn, pGeode->displayWidth,
                                             numlines, 0, NULL, NULL, NULL);

        if (!new_area) {
            int max_w, max_h;

            xf86QueryLargestOffscreenArea(pScrn, &max_w, &max_h, 0,
                                          FAVOR_WIDTH_THEN_AREA,
                                          PRIORITY_EXTREME);

            if ((max_w < pGeode->displayWidth) || (max_h < numlines)) {
                xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                           "No room - how sad %x, %x, %x, %x\n", max_w,
                           pGeode->displayWidth, max_h, numlines);
                return 0;
            }

            xf86PurgeUnlockedOffscreenAreas(pScrn);
            new_area = xf86AllocateOffscreenArea(pScrn, pGeode->displayWidth,
                                                 numlines, 0, NULL, NULL, NULL);
        }

        return (new_area->box.y1 * pGeode->Pitch);
    }

    return 0;
}

static BoxRec global_dstBox;
static int srcPitch = 0, srcPitch2 = 0, dstPitch = 0, dstPitch2 = 0;
static INT32 Bx1, Bx2, By1, By2;
static int top, left, npixels, nlines;
static int global_offset, s1offset = 0, s2offset = 0, s3offset = 0;
static unsigned char *dst_start;
static int d2offset = 0, d3offset = 0;

#if 0
static Bool
RegionsIntersect(BoxPtr pRcl1, BoxPtr pRcl2, BoxPtr pRclResult)
{
    pRclResult->x1 = max(pRcl1->x1, pRcl2->x1);
    pRclResult->x2 = min(pRcl1->x2, pRcl2->x2);

    if (pRclResult->x1 <= pRclResult->x2) {
        pRclResult->y1 = max(pRcl1->y1, pRcl2->y1);
        pRclResult->y2 = min(pRcl1->y2, pRcl2->y2);

        if (pRclResult->y1 <= pRclResult->y2) {
            return (TRUE);
        }
    }

    return (FALSE);
}
#endif

void
GXSetVideoPosition(int x, int y, int width, int height,
                   short src_w, short src_h, short drw_w, short drw_h,
                   int id, int offset, ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    long ystart, xend, yend;
    unsigned long lines = 0;
    unsigned long y_extra, uv_extra = 0;
    unsigned long startAddress;

#if 0
    BoxRec ovly, display, result;
#endif

    xend = x + drw_w;
    yend = y + drw_h;

    /* Take care of panning when panel is present */

    startAddress = gfx_get_display_offset();
    DeltaY = startAddress / pGeode->Pitch;
    DeltaX = startAddress & (pGeode->Pitch - 1);
    DeltaX /= (pScrni->bitsPerPixel >> 3);

#if 0
    /* Thhis code is pretty dang broken - comment it out for now */

    if (pGeode->Panel) {
        ovly.x1 = x;
        ovly.x2 = x + pGeode->video_dstw;
        ovly.y1 = y;
        ovly.y2 = y + pGeode->video_dsth;

        display.x1 = DeltaX;
        display.x2 = DeltaX + pGeode->FPBX;
        display.y1 = DeltaY;
        display.y2 = DeltaY + pGeode->FPBY;
        x = xend = 0;
        if (RegionsIntersect(&display, &ovly, &result)) {
            x = ovly.x1 - DeltaX;
            xend = ovly.x2 - DeltaX;
            y = ovly.y1 - DeltaY;
            yend = ovly.y2 - DeltaY;
        }
    }
#endif

    /*  TOP CLIPPING */

    if (y < 0) {
        if (src_h < drw_h)
            lines = (-y) * src_h / drw_h;
        else
            lines = (-y);
        ystart = 0;
        drw_h += y;
        y_extra = lines * dstPitch;
        uv_extra = (lines >> 1) * (dstPitch2);
    }
    else {
        ystart = y;
        lines = 0;
        y_extra = 0;
    }

    GFX(set_video_window(x, ystart, xend - x, yend - ystart));

    if ((id == FOURCC_Y800) || (id == FOURCC_I420) || (id == FOURCC_YV12)) {
        GFX(set_video_yuv_offsets(offset + y_extra,
                                  offset + d3offset + uv_extra,
                                  offset + d2offset + uv_extra));
    }
    else {
        GFX(set_video_offset(offset + y_extra));
    }
}

/*----------------------------------------------------------------------------
 * GXDisplayVideo
 *
 * Description	:This function sets up the video registers for playing video
 * 				It sets up the video format,width, height & position of the
 *		  		video window ,video offsets( y,u,v) and video pitches(y,u,v)
 *
 * Parameters
 *
 * Returns		:None
 *
 * Comments		:None
 *----------------------------------------------------------------------------
 */

static void
GXDisplayVideo(ScrnInfoPtr pScrni,
               int id,
               int offset,
               short width, short height,
               int pitch,
               int x1, int y1, int x2, int y2,
               BoxPtr dstBox, short src_w, short src_h, short drw_w,
               short drw_h)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    unsigned long dcfg, misc;

    GXAccelSync(pScrni);

    /* If the gamma LUT is already loaded with graphics data, then save it
     * off
     */

    if (id != FOURCC_RGB565) {
        dcfg = gfx_read_vid32(DISPLAY_CONFIG);
        misc = gfx_read_vid32(MISC);

        lutflag = (!(misc & 1) && (dcfg & (1 << 21)));

        if (lutflag)
            get_gamma_ram(graphics_lut);

        /* Set the video gamma ram */
        GFX(set_video_palette(NULL));
    }

    GFX(set_video_enable(1));

    switch (id) {
    case FOURCC_UYVY:          /* UYVY */
        GFX(set_video_format(VIDEO_FORMAT_UYVY));
        GFX(set_video_size(width, height));
        break;
    case FOURCC_Y800:          /* Y800 - greyscale - we munge it! */
    case FOURCC_YV12:          /* YV12 */
    case FOURCC_I420:          /* I420 */
        GFX(set_video_format(VIDEO_FORMAT_Y0Y1Y2Y3));
        GFX(set_video_size(width, height));
        GFX(set_video_yuv_pitch(dstPitch, dstPitch2));
        break;
    case FOURCC_YUY2:          /* YUY2 */
        GFX(set_video_format(VIDEO_FORMAT_YUYV));
        GFX(set_video_size(width, height));
        break;
    case FOURCC_Y2YU:          /* Y2YU */
        GFX(set_video_format(VIDEO_FORMAT_Y2YU));
        GFX(set_video_size(width, height));
        break;
    case FOURCC_YVYU:          /* YVYU */
        GFX(set_video_format(VIDEO_FORMAT_YVYU));
        GFX(set_video_size(width, height));
        break;
    case FOURCC_RGB565:
        GFX(set_video_format(VIDEO_FORMAT_RGB));
        GFX(set_video_size(width, height));
        break;

    }

    if (pGeode->Panel) {
        pGeode->video_x = dstBox->x1;
        pGeode->video_y = dstBox->y1;
        pGeode->video_w = width;
        pGeode->video_h = height;
        pGeode->video_srcw = src_w;
        pGeode->video_srch = src_h;
        pGeode->video_dstw = drw_w;
        pGeode->video_dsth = drw_h;
        pGeode->video_offset = offset;
        pGeode->video_id = id;
        pGeode->video_scrnptr = pScrni;
    }

    if ((drw_w >= src_w) && (drw_h >= src_h))
        GFX(set_video_scale(width, height, drw_w, drw_h));
    else if (drw_w < src_w)
        GFX(set_video_scale(drw_w, height, drw_w, drw_h));
    else if (drw_h < src_h)
        GFX(set_video_scale(width, drw_h, drw_w, drw_h));

    GXSetVideoPosition(dstBox->x1, dstBox->y1, width, height, src_w,
                       src_h, drw_w, drw_h, id, offset, pScrni);
}

/* Used by LX as well */

Bool
RegionsEqual(RegionPtr A, RegionPtr B)
{
    int *dataA, *dataB;
    int num;

    num = REGION_NUM_RECTS(A);
    if (num != REGION_NUM_RECTS(B)) {
        return FALSE;
    }

    if ((A->extents.x1 != B->extents.x1) ||
        (A->extents.x2 != B->extents.x2) ||
        (A->extents.y1 != B->extents.y1) || (A->extents.y2 != B->extents.y2))
        return FALSE;

    dataA = (int *) REGION_RECTS(A);
    dataB = (int *) REGION_RECTS(B);

    while (num--) {
        if ((dataA[0] != dataB[0]) || (dataA[1] != dataB[1]))
            return FALSE;

        dataA += 2;
        dataB += 2;
    }

    return TRUE;
}

/*----------------------------------------------------------------------------
 * GXPutImage	:This function writes a single frame of video into a
 * 				drawable. The position and size of the source rectangle is
 * 				specified by src_x,src_y, src_w and src_h. This data is
 * 				stored in a system memory buffer at buf. The position and
 * 				size of the destination rectangle is specified by drw_x,
 * 				drw_y,drw_w,drw_h.The data is in the format indicated by the
 * 				image descriptor and represents a source of size width by
 * 				height.  If sync is TRUE the driver should not return from
 * 				this function until it is through reading the data from buf.
 * 				Returning when sync is TRUE indicates that it is safe for the
 * 				data at buf to be replaced,freed, or modified.
 *
 * Parameters.
 *
 * Returns		:None
 *
 * Comments		:None
 *----------------------------------------------------------------------------
 */

static int
GXPutImage(ScrnInfoPtr pScrni,
           short src_x, short src_y,
           short drw_x, short drw_y,
           short src_w, short src_h,
           short drw_w, short drw_h,
           int id, unsigned char *buf,
           short width, short height, Bool sync, RegionPtr clipBoxes,
           pointer data, DrawablePtr pDraw)
{
    GeodePortPrivRec *pPriv = (GeodePortPrivRec *) data;
    GeodeRec *pGeode = GEODEPTR(pScrni);
    int new_h;

#if REINIT
    BOOL ReInitVideo = FALSE;
    static BOOL DoReinitAgain = 0;
#endif

#if XV_PROFILE
    long oldtime, newtime;

    UpdateCurrentTime();
    oldtime = currentTime.milliseconds;
#endif

#if REINIT
/* update cliplist */
    if (!RegionsEqual(&pPriv->clip, clipBoxes)) {
        ReInitVideo = TRUE;
    }

    if (DoReinitAgain)
        ReInitVideo = TRUE;

    if (ReInitVideo) {
        DEBUGMSG(1, (0, X_NONE, "Regional Not Equal - Init\n"));
#endif
        DoReinitAgain = ~DoReinitAgain;
        if (drw_w > 16384)
            drw_w = 16384;

        /* Clip */
        Bx1 = src_x;
        Bx2 = src_x + src_w;
        By1 = src_y;
        By2 = src_y + src_h;

        if ((Bx1 >= Bx2) || (By1 >= By2))
            return Success;

        global_dstBox.x1 = drw_x;
        global_dstBox.x2 = drw_x + drw_w;
        global_dstBox.y1 = drw_y;
        global_dstBox.y2 = drw_y + drw_h;

        global_dstBox.x1 -= pScrni->frameX0;
        global_dstBox.x2 -= pScrni->frameX0;
        global_dstBox.y1 -= pScrni->frameY0;
        global_dstBox.y2 -= pScrni->frameY0;

        switch (id) {
        case FOURCC_YV12:
        case FOURCC_I420:
            srcPitch = (width + 3) & ~3;        /* of luma */
            dstPitch = (width + 31) & ~31;

            s2offset = srcPitch * height;
            d2offset = dstPitch * height;

            srcPitch2 = ((width >> 1) + 3) & ~3;
            dstPitch2 = ((width >> 1) + 15) & ~15;

            s3offset = (srcPitch2 * (height >> 1)) + s2offset;
            d3offset = (dstPitch2 * (height >> 1)) + d2offset;

            new_h = dstPitch * height;  /* Y */
            new_h += (dstPitch2 * height);      /* U+V */
            new_h += pGeode->Pitch - 1;
            new_h /= pGeode->Pitch;
            break;
        case FOURCC_UYVY:
        case FOURCC_YUY2:
        case FOURCC_Y800:
        case FOURCC_RGB565:
        default:
            dstPitch = ((width << 1) + 3) & ~3;
            srcPitch = (width << 1);
            new_h = ((dstPitch * height) + pGeode->Pitch - 1) / pGeode->Pitch;
            break;
        }
#if DBUF
        if (pPriv->doubleBuffer)
            new_h <<= 1;
#endif

        if (!(pPriv->offset = GXAllocateMemory(pScrni, &pPriv->area, new_h))) {
            xf86DrvMsg(pScrni->scrnIndex, X_ERROR,
                       "Could not allocate area of size %d\n", new_h);
            return BadAlloc;
        }

        /* copy data */
        top = By1;
        left = Bx1 & ~1;
        npixels = ((Bx2 + 1) & ~1) - left;

        switch (id) {
        case FOURCC_YV12:
        case FOURCC_I420:
        {
            int tmp;

            top &= ~1;

            global_offset = pPriv->offset + (top * dstPitch);

#if DBUF
            if (pPriv->doubleBuffer && pPriv->currentBuffer)
                global_offset += (new_h >> 1) * pGeode->Pitch;
#endif
            dst_start = pGeode->FBBase + global_offset + left;
            tmp = ((top >> 1) * srcPitch2) + (left >> 1);
            s2offset += tmp;
            s3offset += tmp;
            if (id == FOURCC_I420) {
                tmp = s2offset;
                s2offset = s3offset;
                s3offset = tmp;
            }
            nlines = ((By2 + 1) & ~1) - top;
        }
            break;
        case FOURCC_UYVY:
        case FOURCC_YUY2:
        case FOURCC_Y800:
        case FOURCC_RGB565:
        default:
            left <<= 1;
            buf += (top * srcPitch) + left;
            nlines = By2 - top;
            global_offset = (pPriv->offset) + (top * dstPitch);

#if DBUF
            if (pPriv->doubleBuffer && pPriv->currentBuffer)
                global_offset += (new_h >> 1) * pGeode->Pitch;
#endif
            dst_start = pGeode->FBBase + global_offset + left;
            break;
        }
        s1offset = (top * srcPitch) + left;
#if REINIT
        /* update cliplist */
        REGION_COPY(pScrni->pScreen, &pPriv->clip, clipBoxes);

        if (pPriv->colorKeyMode == 0) {
            xf86XVFillKeyHelper(pScrni->pScreen, pPriv->colorKey, clipBoxes);
        }

        GXDisplayVideo(pScrni, id, global_offset, width, height, dstPitch,
                       Bx1, By1, Bx2, By2, &global_dstBox, src_w, src_h, drw_w,
                       drw_h);
    }
#endif
    switch (id) {
    case FOURCC_Y800:
        /* This is shared between LX and GX, so it lives in amd_common.c */
        GeodeCopyGreyscale(buf, dst_start, srcPitch, dstPitch, nlines, npixels);
        break;
    case FOURCC_YV12:
    case FOURCC_I420:
        GXCopyData420(buf + s1offset, dst_start, srcPitch, dstPitch, nlines,
                      npixels);
        GXCopyData420(buf + s2offset, dst_start + d2offset, srcPitch2,
                      dstPitch2, nlines >> 1, npixels >> 1);
        GXCopyData420(buf + s3offset, dst_start + d3offset, srcPitch2,
                      dstPitch2, nlines >> 1, npixels >> 1);
        break;
    case FOURCC_UYVY:
    case FOURCC_YUY2:
    case FOURCC_RGB565:
    default:
        GXCopyData422(buf, dst_start, srcPitch, dstPitch, nlines, npixels);
        break;
    }
#if !REINIT
    /* update cliplist */
    REGION_COPY(pScrni->pScreen, &pPriv->clip, clipBoxes);
    if (pPriv->colorKeyMode == 0) {
        /* draw these */
        XAAFillSolidRects(pScrni, pPriv->colorKey, GXcopy, ~0,
                          REGION_NUM_RECTS(clipBoxes), REGION_RECTS(clipBoxes));
    }

    GXDisplayVideo(pScrni, id, global_offset, width, height, dstPitch,
                   Bx1, By1, Bx2, By2, &global_dstBox, src_w, src_h, drw_w,
                   drw_h);
#endif

#if XV_PROFILE
    UpdateCurrentTime();
    newtime = currentTime.milliseconds;
    DEBUGMSG(1, (0, X_NONE, "PI %d\n", newtime - oldtime));
#endif

#if DBUF
    pPriv->currentBuffer ^= 1;
#endif

    pPriv->videoStatus = CLIENT_VIDEO_ON;
    pGeode->OverlayON = TRUE;
    return Success;
}

/*----------------------------------------------------------------------------
 * GXQueryImageAttributes
 *
 * Description	:This function is called to let the driver specify how data
 *				 for a particular image of size width by height should be
 *				 stored.
 *
 * Parameters.
 *		pScrni	:Screen handler pointer having screen information.
 *		id		:Id for the video format
 *		width	:width  of the image (can be modified by the driver)
 *		height	:height of the image (can be modified by the driver)
 * Returns		: Size of the memory required for storing this image
 *
 * Comments		:None
 *
 *----------------------------------------------------------------------------
 */

int
GeodeQueryImageAttributes(ScrnInfoPtr pScrni,
                          int id, unsigned short *w, unsigned short *h,
                          int *pitches, int *offsets)
{
    int size;
    int tmp;

    DEBUGMSG(0, (0, X_NONE, "QueryImageAttributes %X\n", id));

    if (*w > 1024)
        *w = 1024;
    if (*h > 1024)
        *h = 1024;

    *w = (*w + 1) & ~1;
    if (offsets)
        offsets[0] = 0;

    switch (id) {
    case FOURCC_YV12:
    case FOURCC_I420:
        *h = (*h + 1) & ~1;
        size = (*w + 3) & ~3;
        if (pitches)
            pitches[0] = size;

        size *= *h;
        if (offsets)
            offsets[1] = size;

        tmp = ((*w >> 1) + 3) & ~3;
        if (pitches)
            pitches[1] = pitches[2] = tmp;

        tmp *= (*h >> 1);
        size += tmp;
        if (offsets)
            offsets[2] = size;

        size += tmp;
        break;
    case FOURCC_UYVY:
    case FOURCC_YUY2:
    case FOURCC_Y800:
    default:
        size = *w << 1;
        if (pitches)
            pitches[0] = size;

        size *= *h;
        break;
    }
    return size;
}

static void
GXBlockHandler(BLOCKHANDLER_ARGS_DECL)
{
    ScrnInfoPtr pScrni = xf86ScreenToScrn(pScreen);
    GeodeRec *pGeode = GEODEPTR(pScrni);
    GeodePortPrivRec *pPriv = GET_PORT_PRIVATE(pScrni);

    pScreen->BlockHandler = pGeode->BlockHandler;
    (*pScreen->BlockHandler) (BLOCKHANDLER_ARGS);
    pScreen->BlockHandler = GXBlockHandler;

    if (pPriv->videoStatus & TIMER_MASK) {
        GXAccelSync(pScrni);
        UpdateCurrentTime();
        if (pPriv->videoStatus & OFF_TIMER) {
            if (pPriv->offTime < currentTime.milliseconds) {
                GFX(set_video_enable(0));

                /* If we have saved graphics LUT data - restore it */
                /* Otherwise, turn bypass on */

                if (lutflag)
                    GFX(set_graphics_palette(graphics_lut));
                else
                    GFX(set_video_palette_bypass(1));

                lutflag = 0;

                pPriv->videoStatus = FREE_TIMER;
                pPriv->freeTime = currentTime.milliseconds + FREE_DELAY;
            }
        }
        else {                  /* FREE_TIMER */
            if (pPriv->freeTime < currentTime.milliseconds) {

                if (pPriv->area) {
#ifdef XF86EXA
                    if (pGeode->useEXA)
                        exaOffscreenFree(pScrni->pScreen, pPriv->area);
#endif
                    if (!pGeode->useEXA)
                        xf86FreeOffscreenArea(pPriv->area);

                    pPriv->area = NULL;
                }

                pPriv->videoStatus = 0;
            }
        }
    }
}

/****************** Offscreen stuff ***************/

typedef struct {
    void *area;
    int offset;
    Bool isOn;
} OffscreenPrivRec, *OffscreenPrivPtr;

/*----------------------------------------------------------------------------
 * GXAllocateSurface
 *
 * Description	:This function allocates an area of w by h in the offscreen
 *
 * Parameters.
 *		pScrni	:Screen handler pointer having screen information.
 *
 * Returns		:None
 *
 * Comments		:None
 *----------------------------------------------------------------------------
 */
static int
GXAllocateSurface(ScrnInfoPtr pScrni,
                  int id, unsigned short w, unsigned short h,
                  XF86SurfacePtr surface)
{
    void *area = NULL;
    int pitch, fbpitch, numlines;
    OffscreenPrivRec *pPriv;

    if ((w > 1024) || (h > 1024))
        return BadAlloc;

    w = (w + 1) & ~1;
    pitch = ((w << 1) + 15) & ~15;
    fbpitch = pScrni->bitsPerPixel * pScrni->displayWidth >> 3;
    numlines = ((pitch * h) + fbpitch - 1) / fbpitch;

    if (!(global_offset = GXAllocateMemory(pScrni, &area, numlines)))
        return BadAlloc;

    surface->width = w;
    surface->height = h;

    if (!(surface->pitches = malloc(sizeof(int))))
        return BadAlloc;

    if (!(surface->offsets = malloc(sizeof(int)))) {
        free(surface->pitches);
        return BadAlloc;
    }

    if (!(pPriv = malloc(sizeof(OffscreenPrivRec)))) {
        free(surface->pitches);
        free(surface->offsets);
        return BadAlloc;
    }

    pPriv->area = area;
    pPriv->offset = global_offset;

    pPriv->isOn = FALSE;

    surface->pScrn = pScrni;
    surface->id = id;
    surface->pitches[0] = pitch;
    surface->offsets[0] = global_offset;
    surface->devPrivate.ptr = (pointer) pPriv;

    return Success;
}

static int
GXStopSurface(XF86SurfacePtr surface)
{
    OffscreenPrivRec *pPriv = (OffscreenPrivRec *) surface->devPrivate.ptr;

    if (pPriv->isOn) {
        pPriv->isOn = FALSE;
    }

    return Success;
}

static int
GXFreeSurface(XF86SurfacePtr surface)
{
    OffscreenPrivRec *pPriv = (OffscreenPrivRec *) surface->devPrivate.ptr;

    if (pPriv->isOn)
        GXStopSurface(surface);

    xf86FreeOffscreenArea(pPriv->area);
    free(surface->pitches);
    free(surface->offsets);
    free(surface->devPrivate.ptr);

    return Success;
}

static int
GXGetSurfaceAttribute(ScrnInfoPtr pScrni, Atom attribute, INT32 *value)
{
    return GXGetPortAttribute(pScrni, attribute, value,
                              (pointer) (GET_PORT_PRIVATE(pScrni)));
}

static int
GXSetSurfaceAttribute(ScrnInfoPtr pScrni, Atom attribute, INT32 value)
{
    return GXSetPortAttribute(pScrni, attribute, value,
                              (pointer) (GET_PORT_PRIVATE(pScrni)));
}

static int
GXDisplaySurface(XF86SurfacePtr surface,
                 short src_x, short src_y,
                 short drw_x, short drw_y,
                 short src_w, short src_h, short drw_w, short drw_h,
                 RegionPtr clipBoxes)
{
    OffscreenPrivRec *pPriv = (OffscreenPrivRec *) surface->devPrivate.ptr;
    ScrnInfoPtr pScrni = surface->pScrn;
    GeodePortPrivRec *portPriv = GET_PORT_PRIVATE(pScrni);
    INT32 x1, y1, x2, y2;
    BoxRec dstBox;

    DEBUGMSG(0, (0, X_NONE, "DisplaySuface\n"));
    x1 = src_x;
    x2 = src_x + src_w;
    y1 = src_y;
    y2 = src_y + src_h;

    dstBox.x1 = drw_x;
    dstBox.x2 = drw_x + drw_w;
    dstBox.y1 = drw_y;
    dstBox.y2 = drw_y + drw_h;

    if ((x1 >= x2) || (y1 >= y2))
        return Success;

    dstBox.x1 -= pScrni->frameX0;
    dstBox.x2 -= pScrni->frameX0;
    dstBox.y1 -= pScrni->frameY0;
    dstBox.y2 -= pScrni->frameY0;

    xf86XVFillKeyHelper(pScrni->pScreen, portPriv->colorKey, clipBoxes);

    GXDisplayVideo(pScrni, surface->id, surface->offsets[0],
                   surface->width, surface->height, surface->pitches[0],
                   x1, y1, x2, y2, &dstBox, src_w, src_h, drw_w, drw_h);

    pPriv->isOn = TRUE;
    if (portPriv->videoStatus & CLIENT_VIDEO_ON) {
        REGION_EMPTY(pScrni->pScreen, &portPriv->clip);
        UpdateCurrentTime();
        portPriv->videoStatus = FREE_TIMER;
        portPriv->freeTime = currentTime.milliseconds + FREE_DELAY;
    }

    return Success;
}

/*----------------------------------------------------------------------------
 * GXInitOffscreenImages
 *
 * Description	:This function sets up the offscreen memory management. It
 * 				fills in the XF86OffscreenImagePtr structure with functions to
 * 				handle offscreen memory operations.
 *
 * Parameters.
 *		pScrn	:Screen handler pointer having screen information.
 *
 * Returns		: None
 *
 * Comments		:None
 *----------------------------------------------------------------------------
 */
static void
GXInitOffscreenImages(ScreenPtr pScrn)
{
    XF86OffscreenImagePtr offscreenImages;

    /* need to free this someplace */
    if (!(offscreenImages = malloc(sizeof(XF86OffscreenImageRec))))
        return;

    offscreenImages[0].image = &Images[0];
    offscreenImages[0].flags = VIDEO_OVERLAID_IMAGES | VIDEO_CLIP_TO_VIEWPORT;
    offscreenImages[0].alloc_surface = GXAllocateSurface;
    offscreenImages[0].free_surface = GXFreeSurface;
    offscreenImages[0].display = GXDisplaySurface;
    offscreenImages[0].stop = GXStopSurface;
    offscreenImages[0].setAttribute = GXSetSurfaceAttribute;
    offscreenImages[0].getAttribute = GXGetSurfaceAttribute;
    offscreenImages[0].max_width = 1024;
    offscreenImages[0].max_height = 1024;
    offscreenImages[0].num_attributes = NUM_ATTRIBUTES;
    offscreenImages[0].attributes = Attributes;

    xf86XVRegisterOffscreenImages(pScrn, offscreenImages, 1);
}

#endif                          /* !XvExtension */
