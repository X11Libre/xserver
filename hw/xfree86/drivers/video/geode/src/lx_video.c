/* Copyright (c) 2007-2008 Advanced Micro Devices, Inc.
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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Neither the name of the Advanced Micro Devices, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 */

/* TODO:
   Add rotation
   Add back in double buffering?

*/
#include <xorg-config.h>

#include <stdlib.h>
#include <string.h>

#include "xf86.h"
#include "xf86_OSproc.h"
#include "compiler.h"
#include "xf86Pci.h"
#include "xf86fbman.h"
#include "regionstr.h"
#include "dixstruct.h"

#include "geode.h"
#include "xf86xv.h"
#include <X11/extensions/Xv.h>
#include "fourcc.h"
#include "geode_fourcc.h"
#include "cim/cim_defs.h"
#include "cim/cim_regs.h"

#define OFF_DELAY 		200
#define FREE_DELAY 		60000
#define OFF_TIMER 		0x01
#define FREE_TIMER		0x02
#define CLIENT_VIDEO_ON	0x04
#define TIMER_MASK      (OFF_TIMER | FREE_TIMER)

#define MAKE_ATOM(a) MakeAtom(a, sizeof(a) - 1, TRUE)
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof((a)) / (sizeof(*(a))))
#endif

/* Local function prototypes */
static void LXStopVideo(ScrnInfoPtr pScrni, void *data, Bool exit);

static void

LXDisplayVideo(ScrnInfoPtr pScrni, int id, short width, short height,
               BoxPtr dstBox, short srcW, short srcH, short drawW, short drawH);

static void LXResetVideo(ScrnInfoPtr pScrni);

static XF86VideoEncodingRec DummyEncoding[1] = {
    {0, "XV_IMAGE", 1024, 1024, {1, 1}}
};

static XF86VideoFormatRec Formats[] = {
    {8, PseudoColor}, {15, TrueColor}, {16, TrueColor}, {24, TrueColor}
};

static XF86AttributeRec Attributes[] = {
    {XvSettable | XvGettable, 0, (1 << 24) - 1, (char *) "XV_COLORKEY"},
    {XvSettable | XvGettable, 0, 1, (char *) "XV_FILTER"},
    {XvSettable | XvGettable, 0, 1, (char *) "XV_COLORKEYMODE"}
};

static XF86ImageRec Images[] = {
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
    ExaOffscreenArea *vidmem;
    RegionRec clip;
    CARD32 filter;
    CARD32 colorKey;
    CARD32 colorKeyMode;
    CARD32 videoStatus;
    Time offTime;
    Time freeTime;
    short pwidth, pheight;
} GeodePortPrivRec, *GeodePortPrivPtr;

#define GET_PORT_PRIVATE(pScrni) \
   (GeodePortPrivRec *)((GEODEPTR(pScrni))->adaptor->pPortPrivates[0].ptr)

static void
LXCopyFromSys(GeodeRec *pGeode, unsigned char *src, unsigned int dst,
              int dstPitch, int srcPitch, int h, int w)
{

    gp_declare_blt(0);
    gp_set_bpp((srcPitch / w) << 3);

    gp_set_raster_operation(0xCC);
    gp_set_strides(dstPitch, srcPitch);
    gp_set_solid_pattern(0);

    gp_color_bitmap_to_screen_blt(dst, 0, w, h, src, srcPitch);
}

static void
LXSetColorkey(ScrnInfoPtr pScrni, GeodePortPrivRec *pPriv)
{
    int red, green, blue;
    unsigned long key;

    switch (pScrni->depth) {
    case 8:
        vg_get_display_palette_entry(pPriv->colorKey & 0xFF, &key);
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

    df_set_video_color_key((blue | (green << 8) | (red << 16)),
                           0xFFFFFF, (pPriv->colorKeyMode == 0));

    REGION_EMPTY(pScrni->pScreen, &pPriv->clip);
}

/* A structure full of the scratch information that originates in the copy routines,
   but is needed for the video display - maybe we should figure out a way to attach
   this to structures?  I hate to put it in pGeode since it will increase the size of
   the structure, and possibly cause us cache issues.
*/

struct {
    unsigned int dstOffset;
    unsigned int dstPitch;
    unsigned int UVPitch;
    unsigned int UDstOffset;
    unsigned int VDstOffset;
} videoScratch;

/* Copy planar YUV data */

static Bool
LXAllocateVidMem(ScrnInfoPtr pScrni, GeodePortPrivRec *pPriv, int size)
{
    if (!pPriv->vidmem || pPriv->vidmem->size < size) {
        if (pPriv->vidmem) {
            exaOffscreenFree(pScrni->pScreen, pPriv->vidmem);
            pPriv->vidmem = NULL;
        }

        pPriv->vidmem = exaOffscreenAlloc(pScrni->pScreen, size, 4,
                                          TRUE, NULL, NULL);

        if (pPriv->vidmem == NULL) {
            ErrorF("Could not allocate memory for the video\n");
            return FALSE;
        }
    }

    return TRUE;
}

static Bool
LXCopyPlanar(ScrnInfoPtr pScrni, int id, unsigned char *buf,
             short x1, short y1, short x2, short y2,
             int width, int height, void *data)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    GeodePortPrivRec *pPriv = (GeodePortPrivRec *) data;

    unsigned int YSrcPitch, YDstPitch;
    unsigned int UVSrcPitch, UVDstPitch;
    unsigned int YSrcOffset, YDstOffset;
    unsigned int USrcOffset, UDstOffset;
    unsigned int VSrcOffset, VDstOffset;

    unsigned int size, lines, top, left, pixels;

    YSrcPitch = (width + 3) & ~3;
    YDstPitch = (width + 31) & ~31;

    UVSrcPitch = ((width >> 1) + 3) & ~3;
    UVDstPitch = ((width >> 1) + 15) & ~15;

    USrcOffset = YSrcPitch * height;
    VSrcOffset = USrcOffset + (UVSrcPitch * (height >> 1));

    UDstOffset = YDstPitch * height;
    VDstOffset = UDstOffset + (UVDstPitch * (height >> 1));

    size = YDstPitch * height;
    size += UVDstPitch * height;

    if (LXAllocateVidMem(pScrni, pPriv, size) == FALSE) {
        ErrorF("Error allocating an offscreen Planar region.\n");
        return FALSE;
    }

    /* The top of the source region we want to copy */
    top = y1 & ~1;

    /* The left hand side of the source region, aligned on a word */
    left = x1 & ~1;

    /* Number of bytes to copy, also word aligned */
    pixels = ((x2 + 1) & ~1) - left;

    /* Calculate the source offset */
    YSrcOffset = (top * YSrcPitch) + left;
    USrcOffset += ((top >> 1) * UVSrcPitch) + (left >> 1);
    VSrcOffset += ((top >> 1) * UVSrcPitch) + (left >> 1);

    /* Calculate the destination offset */
    YDstOffset = (top * YDstPitch) + left;
    UDstOffset += ((top >> 1) * UVDstPitch) + (left >> 1);
    VDstOffset += ((top >> 1) * UVDstPitch) + (left >> 1);

    lines = ((y2 + 1) & ~1) - top;

    /* Copy Y */

    LXCopyFromSys(pGeode, buf + YSrcOffset,
                  pPriv->vidmem->offset + YDstOffset, YDstPitch, YSrcPitch,
                  lines, pixels);

    /* Copy U + V at the same time */

    LXCopyFromSys(pGeode, buf + USrcOffset,
                  pPriv->vidmem->offset + UDstOffset, UVDstPitch, UVSrcPitch,
                  lines, pixels >> 1);

    videoScratch.dstOffset = pPriv->vidmem->offset + YDstOffset;
    videoScratch.dstPitch = YDstPitch;
    videoScratch.UVPitch = UVDstPitch;
    videoScratch.UDstOffset = pPriv->vidmem->offset + UDstOffset;
    videoScratch.VDstOffset = pPriv->vidmem->offset + VDstOffset;

    return TRUE;
}

static Bool
LXCopyPacked(ScrnInfoPtr pScrni, int id, unsigned char *buf,
             short x1, short y1, short x2, short y2,
             int width, int height, void *data)
{
    GeodePortPrivRec *pPriv = (GeodePortPrivRec *) data;
    GeodeRec *pGeode = GEODEPTR(pScrni);
    unsigned int dstPitch, srcPitch;
    unsigned int srcOffset, dstOffset;
    unsigned int top, left, pixels;

    dstPitch = ((width << 1) + 3) & ~3;
    srcPitch = (width << 1);

    /* lines = ((dstPitch * height) + pGeode->Pitch - 1) / pGeode->Pitch; */

    if (LXAllocateVidMem(pScrni, pPriv, dstPitch * height) == FALSE) {
        ErrorF("Error allocating an offscreen Packed region.\n");
        return FALSE;
    }

    /* The top of the source region we want to copy */
    top = y1;

    /* The left hand side of the source region, aligned on a word */
    left = x1 & ~1;

    /* Number of bytes to copy, also word aligned */
    pixels = ((x2 + 1) & ~1) - left;

    /* Adjust the incoming buffer */
    srcOffset = (top * srcPitch) + left;

    /* Calculate the destination offset */
    dstOffset = pPriv->vidmem->offset + (top * dstPitch) + left;

    /* Make the copy happen */

    if (id == FOURCC_Y800) {

        /* Use the shared (unaccelerated) greyscale copy - you could probably
         * accelerate it using a 2 pass blit and patterns, but it doesn't really
         * seem worth it
         */

        GeodeCopyGreyscale(buf + srcOffset, pGeode->FBBase + dstOffset,
                           srcPitch, dstPitch, height, pixels >> 1);
    }
    else
        /* FIXME: should lines be used here instead of height? */
        LXCopyFromSys(pGeode, buf + srcOffset, dstOffset, dstPitch, srcPitch,
                      height, pixels);

    videoScratch.dstOffset = dstOffset;
    videoScratch.dstPitch = dstPitch;

    return TRUE;
}

static void
LXDisplayVideo(ScrnInfoPtr pScrni, int id, short width, short height,
               BoxPtr dstBox, short srcW, short srcH, short drawW, short drawH)
{
    long ystart, xend, yend;
    unsigned long lines = 0;
    unsigned long yExtra, uvExtra = 0;
    DF_VIDEO_POSITION vidPos;
    DF_VIDEO_SOURCE_PARAMS vSrcParams;
    int err;

    memset(&vSrcParams, 0, sizeof(vSrcParams));

    gp_wait_until_idle();

    switch (id) {
    case FOURCC_UYVY:
        vSrcParams.video_format = DF_VIDFMT_UYVY;
        break;

    case FOURCC_Y800:
    case FOURCC_YV12:
    case FOURCC_I420:
        vSrcParams.video_format = DF_VIDFMT_Y0Y1Y2Y3;
        break;
    case FOURCC_YUY2:
        vSrcParams.video_format = DF_VIDFMT_YUYV;
        break;
    case FOURCC_Y2YU:
        vSrcParams.video_format = DF_VIDFMT_Y2YU;
        break;
    case FOURCC_YVYU:
        vSrcParams.video_format = DF_VIDFMT_YVYU;
        break;
    case FOURCC_RGB565:
        vSrcParams.video_format = DF_VIDFMT_RGB;
        break;
    }

    vSrcParams.width = width;
    vSrcParams.height = height;
    vSrcParams.y_pitch = videoScratch.dstPitch;
    vSrcParams.uv_pitch = videoScratch.UVPitch;

    /* Set up scaling */
    df_set_video_filter_coefficients(NULL, 1);

    err = df_set_video_scale(width, height, drawW, drawH,
                             DF_SCALEFLAG_CHANGEX | DF_SCALEFLAG_CHANGEY);
    if (err != CIM_STATUS_OK) {
        /* Note the problem, but do nothing for now. */
        ErrorF("Video scale factor too large: %dx%d -> %dx%d\n",
               width, height, drawW, drawH);
    }

    /* Figure out clipping */

    xend = dstBox->x2;
    yend = dstBox->y2;

    if (dstBox->y1 < 0) {
        if (srcH < drawH)
            lines = ((-dstBox->y1) * srcH) / drawH;
        else
            lines = (-dstBox->y1);

        ystart = 0;
        drawH += dstBox->y1;
    }
    else {
        ystart = dstBox->y1;
        lines = 0;
    }

    yExtra = lines * videoScratch.dstPitch;
    uvExtra = (lines >> 1) * videoScratch.UVPitch;

    memset(&vidPos, 0, sizeof(vidPos));

    vidPos.x = dstBox->x1;
    vidPos.y = ystart;
    vidPos.width = xend - dstBox->x1;
    vidPos.height = yend - ystart;

    df_set_video_position(&vidPos);

    vSrcParams.y_offset = videoScratch.dstOffset + yExtra;

    switch (id) {
    case FOURCC_Y800:
    case FOURCC_I420:
        vSrcParams.u_offset = videoScratch.UDstOffset + uvExtra;
        vSrcParams.v_offset = videoScratch.VDstOffset + uvExtra;
        break;
    case FOURCC_YV12:
        vSrcParams.v_offset = videoScratch.UDstOffset + uvExtra;
        vSrcParams.u_offset = videoScratch.VDstOffset + uvExtra;
        break;

    default:
        vSrcParams.u_offset = vSrcParams.v_offset = 0;
        break;
    }

    vSrcParams.flags = DF_SOURCEFLAG_IMPLICITSCALING;
    df_configure_video_source(&vSrcParams, &vSrcParams);

    /* Turn on the video palette */
    df_set_video_palette(NULL);
    df_set_video_enable(1, 0);
}

static int
LXPutImage(ScrnInfoPtr pScrni,
           short srcX, short srcY, short drawX, short drawY,
           short srcW, short srcH, short drawW, short drawH,
           int id, unsigned char *buf,
           short width, short height, Bool sync, RegionPtr clipBoxes,
           void *data, DrawablePtr pDraw)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    GeodePortPrivRec *pPriv = (GeodePortPrivRec *) data;
    INT32 x1, x2, y1, y2;
    BoxRec dstBox;
    Bool ret;

    if (pGeode->rotation != RR_Rotate_0)
        return Success;

    if (srcW <= 0 || srcH <= 0) {
        return Success;
    }

    if (drawW <= 0 || drawH <= 0) {
        return Success;
    }

    if (drawW > 16384)
        drawW = 16384;

    memset(&videoScratch, 0, sizeof(videoScratch));

    x1 = srcX;
    x2 = srcX + srcW;
    y1 = srcY;
    y2 = srcY + srcH;

    dstBox.x1 = drawX;
    dstBox.x2 = drawX + drawW;
    dstBox.y1 = drawY;
    dstBox.y2 = drawY + drawH;

    dstBox.x1 -= pScrni->frameX0;
    dstBox.x2 -= pScrni->frameX0;
    dstBox.y1 -= pScrni->frameY0;
    dstBox.y2 -= pScrni->frameY0;

    if (id == FOURCC_YV12 || id == FOURCC_I420)
        ret = LXCopyPlanar(pScrni, id, buf, x1, y1, x2, y2, width,
                           height, data);
    else
        ret = LXCopyPacked(pScrni, id, buf, x1, y1, x2, y2, width,
                           height, data);

    if (ret == FALSE)
        return BadAlloc;

    if (!RegionsEqual(&pPriv->clip, clipBoxes) ||
        (drawW != pPriv->pwidth || drawH != pPriv->pheight)) {
        REGION_COPY(pScrni->pScreen, &pPriv->clip, clipBoxes);

        if (pPriv->colorKeyMode == 0) {
            xf86XVFillKeyHelper(pScrni->pScreen, pPriv->colorKey, clipBoxes);
        }

        LXDisplayVideo(pScrni, id, width, height, &dstBox,
                       srcW, srcH, drawW, drawH);
        pPriv->pwidth = drawW;
        pPriv->pheight = drawH;
    }

    pPriv->videoStatus = CLIENT_VIDEO_ON;

    return Success;
}

static void
LXQueryBestSize(ScrnInfoPtr pScrni, Bool motion,
                short vidW, short vidH, short drawW, short drawH,
                unsigned int *retW, unsigned int *retH, void *data)
{
    *retW = drawW > 16384 ? 16384 : drawW;
    *retH = drawH;
}

static Atom xvColorKey, xvColorKeyMode, xvFilter;

static int
LXGetPortAttribute(ScrnInfoPtr pScrni,
                   Atom attribute, INT32 *value, void *data)
{
    GeodePortPrivRec *pPriv = (GeodePortPrivRec *) data;

    if (attribute == xvColorKey)
        *value = pPriv->colorKey;
    else if (attribute == xvColorKeyMode)
        *value = pPriv->colorKeyMode;
    else if (attribute == xvFilter)
        *value = pPriv->filter;
    else
        return BadMatch;

    return Success;
}

static int
LXSetPortAttribute(ScrnInfoPtr pScrni,
                   Atom attribute, INT32 value, void *data)
{
    GeodePortPrivRec *pPriv = (GeodePortPrivRec *) data;

    gp_wait_until_idle();

    if (attribute == xvColorKey) {
        pPriv->colorKey = value;
        LXSetColorkey(pScrni, pPriv);
    }
    else if (attribute == xvColorKeyMode) {
        pPriv->colorKeyMode = value;
        LXSetColorkey(pScrni, pPriv);
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

static void
LXStopVideo(ScrnInfoPtr pScrni, void *data, Bool exit)
{
    GeodePortPrivRec *pPriv = (GeodePortPrivRec *) data;

    if (pPriv->videoStatus == 0)
        return;

    REGION_EMPTY(pScrni->pScreen, &pPriv->clip);
    gp_wait_until_idle();

    if (exit) {
        if (pPriv->videoStatus & CLIENT_VIDEO_ON) {
            unsigned int val;

            df_set_video_enable(0, 0);
            /* Put the LUT back in bypass */
            val = READ_VID32(DF_VID_MISC);
            WRITE_VID32(DF_VID_MISC, val | DF_GAMMA_BYPASS_BOTH);
        }

        if (pPriv->vidmem) {
            exaOffscreenFree(pScrni->pScreen, pPriv->vidmem);
            pPriv->vidmem = NULL;
        }

        pPriv->videoStatus = 0;

        /* Eh? */
    }
    else if (pPriv->videoStatus & CLIENT_VIDEO_ON) {
        pPriv->videoStatus |= OFF_TIMER;
        pPriv->offTime = currentTime.milliseconds + OFF_DELAY;
    }
}

static void
LXResetVideo(ScrnInfoPtr pScrni)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);

    if (!pGeode->NoAccel) {
        GeodePortPrivRec *pPriv = pGeode->adaptor->pPortPrivates[0].ptr;

        gp_wait_until_idle();
        df_set_video_palette(NULL);

        LXSetColorkey(pScrni, pPriv);
    }
}

static void
LXVidBlockHandler(BLOCKHANDLER_ARGS_DECL)
{
    ScrnInfoPtr pScrni = xf86ScreenToScrn(pScreen);
    GeodeRec *pGeode = GEODEPTR(pScrni);
    GeodePortPrivRec *pPriv = GET_PORT_PRIVATE(pScrni);

    pScreen->BlockHandler = pGeode->BlockHandler;
    (*pScreen->BlockHandler) (BLOCKHANDLER_ARGS);
    pScreen->BlockHandler = LXVidBlockHandler;

    if (pPriv->videoStatus & TIMER_MASK) {
        Time now = currentTime.milliseconds;

        if (pPriv->videoStatus & OFF_TIMER) {
            gp_wait_until_idle();

            if (pPriv->offTime < now) {
                unsigned int val;

                df_set_video_enable(0, 0);
                pPriv->videoStatus = FREE_TIMER;
                pPriv->freeTime = now + FREE_DELAY;

                /* Turn off the video palette */
                val = READ_VID32(DF_VID_MISC);
                WRITE_VID32(DF_VID_MISC, val | DF_GAMMA_BYPASS_BOTH);
            }
        }
        else {
            if (pPriv->freeTime < now) {

                if (pPriv->vidmem) {
                    exaOffscreenFree(pScrni->pScreen, pPriv->vidmem);
                    pPriv->vidmem = NULL;
                }

                pPriv->videoStatus = 0;
            }
        }
    }
}

static XF86VideoAdaptorPtr
LXSetupImageVideo(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrni = xf86ScreenToScrn(pScreen);
    GeodeRec *pGeode = GEODEPTR(pScrni);
    XF86VideoAdaptorPtr adapt;
    GeodePortPrivRec *pPriv;

    adapt = calloc(1, sizeof(XF86VideoAdaptorRec) +
                   sizeof(GeodePortPrivRec) + sizeof(DevUnion));

    if (adapt == NULL) {
        ErrorF("Couldn't create the rec\n");
        return NULL;
    }

    adapt->type = XvWindowMask | XvInputMask | XvImageMask;
    adapt->flags = VIDEO_OVERLAID_IMAGES | VIDEO_CLIP_TO_VIEWPORT;

    adapt->name = "AMD Geode LX";
    adapt->nEncodings = 1;
    adapt->pEncodings = DummyEncoding;
    adapt->nFormats = ARRAY_SIZE(Formats);
    adapt->pFormats = Formats;
    adapt->nPorts = 1;
    adapt->pPortPrivates = (DevUnion *) (&adapt[1]);
    pPriv = (GeodePortPrivRec *) (&adapt->pPortPrivates[1]);
    adapt->pPortPrivates[0].ptr = pPriv;
    adapt->pAttributes = Attributes;
    adapt->nImages = ARRAY_SIZE(Images);
    adapt->nAttributes = ARRAY_SIZE(Attributes);
    adapt->pImages = Images;
    adapt->PutVideo = NULL;
    adapt->PutStill = NULL;
    adapt->GetVideo = NULL;
    adapt->GetStill = NULL;
    adapt->StopVideo = LXStopVideo;
    adapt->SetPortAttribute = LXSetPortAttribute;
    adapt->GetPortAttribute = LXGetPortAttribute;
    adapt->QueryBestSize = LXQueryBestSize;
    adapt->PutImage = LXPutImage;

    /* Use the common function */
    adapt->QueryImageAttributes = GeodeQueryImageAttributes;

    pPriv->vidmem = NULL;
    pPriv->filter = 0;
    pPriv->colorKey = 0;
    pPriv->colorKeyMode = 0;
    pPriv->videoStatus = 0;
    pPriv->pwidth = 0;
    pPriv->pheight = 0;

    REGION_NULL(pScrn, &pPriv->clip);

    pGeode->adaptor = adapt;

    pGeode->BlockHandler = pScreen->BlockHandler;
    pScreen->BlockHandler = LXVidBlockHandler;

    xvColorKey = MAKE_ATOM("XV_COLORKEY");
    xvColorKeyMode = MAKE_ATOM("XV_COLORKEYMODE");
    xvFilter = MAKE_ATOM("XV_FILTER");

    LXResetVideo(pScrni);

    return adapt;
}

/* Offscreen surface allocation */

struct OffscreenPrivRec {
    ExaOffscreenArea *vidmem;
    Bool isOn;
};

static int
LXDisplaySurface(XF86SurfacePtr surface,
                 short srcX, short srcY, short drawX, short drawY,
                 short srcW, short srcH, short drawW, short drawH,
                 RegionPtr clipBoxes)
{
    struct OffscreenPrivRec *pPriv =
        (struct OffscreenPrivRec *) surface->devPrivate.ptr;

    ScrnInfoPtr pScrni = surface->pScrn;
    GeodePortPrivRec *portPriv = GET_PORT_PRIVATE(pScrni);

    BoxRec dstBox;

    dstBox.x1 = drawX;
    dstBox.x2 = drawX + drawW;
    dstBox.y1 = drawY;
    dstBox.y2 = drawY + drawH;

    if ((drawW <= 0) | (drawH <= 0))
        return Success;

    /* Is this still valid? */

    dstBox.x1 -= pScrni->frameX0;
    dstBox.x2 -= pScrni->frameX0;
    dstBox.y1 -= pScrni->frameY0;
    dstBox.y2 -= pScrni->frameY0;

    xf86XVFillKeyHelper(pScrni->pScreen, portPriv->colorKey, clipBoxes);

    videoScratch.dstOffset = surface->offsets[0];
    videoScratch.dstPitch = surface->pitches[0];

    LXDisplayVideo(pScrni, surface->id, surface->width, surface->height,
                   &dstBox, srcW, srcH, drawW, drawH);

    pPriv->isOn = TRUE;

    if (portPriv->videoStatus & CLIENT_VIDEO_ON) {
        REGION_EMPTY(pScrni->pScreen, &portPriv->clip);
        UpdateCurrentTime();
        portPriv->videoStatus = FREE_TIMER;
        portPriv->freeTime = currentTime.milliseconds + FREE_DELAY;
    }

    return Success;
}

static int
LXAllocateSurface(ScrnInfoPtr pScrni, int id, unsigned short w,
                  unsigned short h, XF86SurfacePtr surface)
{
    GeodeRec *pGeode = GEODEPTR(pScrni);
    int pitch, lines;
    ExaOffscreenArea *vidmem;
    struct OffscreenPrivRec *pPriv;

    if (w > 1024 || h > 1024)
        return BadAlloc;

    /* The width needs to be word aligned */
    w = (w + 1) & ~1;

    pitch = ((w << 1) + 15) & ~15;
    lines = ((pitch * h) + (pGeode->Pitch - 1)) / pGeode->Pitch;

    /* FIXME: is lines the right parameter to use here,
     * or should it be height * pitch? */
    vidmem = exaOffscreenAlloc(pScrni->pScreen, lines, 4, TRUE, NULL, NULL);

    if (vidmem == NULL) {
        ErrorF("Error while allocating an offscreen region.\n");
        return BadAlloc;
    }

    surface->width = w;
    surface->height = h;

    surface->pitches = malloc(sizeof(int));

    surface->offsets = malloc(sizeof(int));

    pPriv = malloc(sizeof(struct OffscreenPrivRec));

    if (pPriv && surface->pitches && surface->offsets) {

        pPriv->vidmem = vidmem;

        pPriv->isOn = FALSE;

        surface->pScrn = pScrni;
        surface->id = id;
        surface->pitches[0] = pitch;
        surface->offsets[0] = vidmem->offset;
        surface->devPrivate.ptr = pPriv;

        return Success;
    }

    if (surface->offsets)
        free(surface->offsets);

    if (surface->pitches)
        free(surface->pitches);

    if (vidmem) {
        exaOffscreenFree(pScrni->pScreen, vidmem);
        vidmem = NULL;
    }

    return BadAlloc;
}

static int
LXStopSurface(XF86SurfacePtr surface)
{
    struct OffscreenPrivRec *pPriv = (struct OffscreenPrivRec *)
        surface->devPrivate.ptr;

    pPriv->isOn = FALSE;
    return Success;
}

static int
LXFreeSurface(XF86SurfacePtr surface)
{
    struct OffscreenPrivRec *pPriv = (struct OffscreenPrivRec *)
        surface->devPrivate.ptr;
    ScrnInfoPtr pScrni = surface->pScrn;

    if (pPriv->isOn)
        LXStopSurface(surface);

    if (pPriv->vidmem) {
        exaOffscreenFree(pScrni->pScreen, pPriv->vidmem);
        pPriv->vidmem = NULL;
    }

    free(surface->pitches);
    free(surface->offsets);
    free(surface->devPrivate.ptr);

    return Success;
}

static int
LXGetSurfaceAttribute(ScrnInfoPtr pScrni, Atom attribute, INT32 *value)
{
    return LXGetPortAttribute(pScrni, attribute, value,
                              (GET_PORT_PRIVATE(pScrni)));
}

static int
LXSetSurfaceAttribute(ScrnInfoPtr pScrni, Atom attribute, INT32 value)
{
    return LXSetPortAttribute(pScrni, attribute, value,
                              (GET_PORT_PRIVATE(pScrni)));
}

static void
LXInitOffscreenImages(ScreenPtr pScrn)
{
    XF86OffscreenImagePtr offscreenImages;

    /* need to free this someplace */
    if (!(offscreenImages = malloc(sizeof(XF86OffscreenImageRec))))
        return;

    offscreenImages[0].image = &Images[0];
    offscreenImages[0].flags = VIDEO_OVERLAID_IMAGES | VIDEO_CLIP_TO_VIEWPORT;
    offscreenImages[0].alloc_surface = LXAllocateSurface;
    offscreenImages[0].free_surface = LXFreeSurface;
    offscreenImages[0].display = LXDisplaySurface;
    offscreenImages[0].stop = LXStopSurface;
    offscreenImages[0].setAttribute = LXSetSurfaceAttribute;
    offscreenImages[0].getAttribute = LXGetSurfaceAttribute;
    offscreenImages[0].max_width = 1024;
    offscreenImages[0].max_height = 1024;
    offscreenImages[0].num_attributes = ARRAY_SIZE(Attributes);
    offscreenImages[0].attributes = Attributes;

    xf86XVRegisterOffscreenImages(pScrn, offscreenImages, 1);
}

void
LXInitVideo(ScreenPtr pScrn)
{
    GeodeRec *pGeode;
    ScrnInfoPtr pScrni = xf86ScreenToScrn(pScrn);
    XF86VideoAdaptorPtr *adaptors, *newAdaptors = NULL;
    XF86VideoAdaptorPtr newAdaptor = NULL;
    int num_adaptors;

    pGeode = GEODEPTR(pScrni);

    if (pGeode->NoAccel) {
        ErrorF("Cannot run Xv without accelerations!\n");
        return;
    }

    if (!(newAdaptor = LXSetupImageVideo(pScrn))) {
        ErrorF("Error while setting up the adaptor.\n");
        return;
    }

    LXInitOffscreenImages(pScrn);

    num_adaptors = xf86XVListGenericAdaptors(pScrni, &adaptors);

    if (!num_adaptors) {
        num_adaptors = 1;
        adaptors = &newAdaptor;
    }
    else {
        newAdaptors =
            malloc((num_adaptors + 1) * sizeof(XF86VideoAdaptorPtr *));

        if (newAdaptors) {
            memcpy(newAdaptors, adaptors, num_adaptors *
                   sizeof(XF86VideoAdaptorPtr));
            newAdaptors[num_adaptors] = newAdaptor;
            adaptors = newAdaptors;
            num_adaptors++;
        }
        else
            ErrorF("Memory error while setting up the adaptor\n");
    }

    if (num_adaptors)
        xf86XVScreenInit(pScrn, adaptors, num_adaptors);

    if (newAdaptors)
        free(newAdaptors);
}
