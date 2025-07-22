/*
 * Copyright (c) 2006 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Neither the name of the Advanced Micro Devices, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 */

/* prototype Xv interface for lxv4l2 driver */
#include <xorg-config.h>

#include <sys/time.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "xf86.h"
#include <X11/extensions/Xv.h>
#include "xf86_OSproc.h"
#include "compiler.h"
#include "xf86xv.h"
#include "fourcc.h"

#include "compat-api.h"

#define __s64 __s_64
typedef long long __s64;

#define __u64 __u_64
typedef unsigned long long __u64;

#define __user
#include "linux/videodev2.h"

typedef signed long long s64;
typedef unsigned long long u64;

int debuglvl = 0;

#define NONBLK_IO
#undef HAVE_SELECT

#define DEBUG 1
#ifdef DEBUG
#define DBLOG(n,s...) do { \
						if ( debuglvl >= (n) ) \
							xf86Msg(X_INFO, "z4l: " s); \
					  } while(0)
#else
#define DBLOG(n,s...) do {} while(0)
#endif

#define DEFAULT_COLORKEY 0x010203
#define DEFAULT_KEYMODE 0

#define MAX_BUFFERS 4
#define MAX_OVLY_WIDTH  2048
#define MAX_OVLY_HEIGHT 2048

static const char *z4l_dev_paths[] = {
    "/dev/videox", NULL
};

#define ATTR_ENCODING "encoding"
#define ATTR_ENCODING_ID -1
#define ATTR_KEYMODE "keymode"
#define ATTR_KEYMODE_ID -2
#define ATTR_COLORKEY "colorkey"
#define ATTR_COLORKEY_ID -3
#define ATTR_MAX_ID 3

#ifdef XvExtension

static XF86VideoFormatRec Formats[] = {
    {8, PseudoColor},
    {15, TrueColor},
    {16, TrueColor},
    {24, TrueColor}
};

#define NUM_FORMATS (sizeof(Formats)/sizeof(Formats[0]))

#define FOURCC_Y800 0x30303859
#define XVIMAGE_Y800 \
   { \
        FOURCC_Y800, \
        XvYUV, \
        LSBFirst, \
        {'Y','8','0','0', \
          0x00,0x00,0x00,0x10,0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71}, \
        8, \
        XvPacked, \
        1, \
        0, 0, 0, 0, \
        8, 0, 0, \
        1, 0, 0, \
        1, 0, 0, \
        {'Y','8','0','0', \
          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, \
        XvTopToBottom \
   }

static XF86ImageRec pixfmts[] = {
    XVIMAGE_UYVY, XVIMAGE_YUY2,
    XVIMAGE_I420, XVIMAGE_YV12,
    XVIMAGE_Y800,
};

#define NUM_PIXFMTS (sizeof(pixfmts)/sizeof(pixfmts[0]))

typedef struct s_std_data {
    int inp;
    v4l2_std_id std;
    unsigned int fmt;
} t_std_data;

typedef struct s_ovly_bfrs {
    void *start;
    unsigned long offset;
    size_t length;
} t_ovly_bfrs;

typedef struct {
    int fd;
    int run;
    int dir;
    int nbfrs;
    int bufno;
    int bufsz;
    int last;
    int width, height;
    int keymode, colorkey;
    int src_is_set, src_x, src_y, src_w, src_h;
    int drw_is_set, drw_x, drw_y, drw_w, drw_h;
    unsigned int pixfmt;
    char dev_path[32];
    t_ovly_bfrs bfrs[MAX_BUFFERS];
    XF86VideoAdaptorPtr adpt;
    XF86VideoEncodingPtr enc;
    RegionRec clips;
    int attrIds[1];
} Z4lPortPrivRec;

static int z4l_x_offset = 0;
static int z4l_y_offset = 0;
static int Z4l_nAdaptors = 0;
static XF86VideoAdaptorPtr *Z4l_pAdaptors = NULL;

static int
IoCtl(int fd, unsigned int fn, void *arg, int flag)
{
    int ret;

    errno = 0;
    ret = ioctl(fd, fn, arg);
    if (ret != 0 && flag != 0)
        DBLOG(0, "ioctl(%08x)=%d\n", fn, ret);
    return ret;
}

static void
z4l_ovly_unmap(Z4lPortPrivRec *pPriv)
{
    int i, nbfrs;

    nbfrs = pPriv->nbfrs;
    for (i = 0; i < nbfrs; ++i) {
        if (pPriv->bfrs[i].start != NULL) {
            munmap(pPriv->bfrs[i].start, pPriv->bfrs[i].length);
            pPriv->bfrs[i].start = NULL;
        }
    }
    pPriv->nbfrs = -1;
    pPriv->bufsz = -1;
    pPriv->last = -1;
}

static void
z4l_ovly_map(Z4lPortPrivRec *pPriv, int dir)
{
    long offset, bsz;
    int i, fd;
    struct v4l2_buffer bfr;
    struct v4l2_requestbuffers req;
    int type = dir >= 0 ?
        V4L2_BUF_TYPE_VIDEO_CAPTURE : V4L2_BUF_TYPE_VIDEO_OVERLAY;
    if (pPriv->run > 0) {
        DBLOG(1, "busy\n");
        return;
    }
    fd = pPriv->fd;
    memset(&req, 0, sizeof(req));
    req.type = type;
    req.memory = V4L2_MEMORY_MMAP;
    req.count = MAX_BUFFERS;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0)
        goto xit;
    pPriv->nbfrs = req.count;
    if (pPriv->nbfrs <= 0) {
        DBLOG(1, "no vidmem\n");
        return;
    }
    memset(&pPriv->bfrs, 0, sizeof(pPriv->bfrs));

    for (i = 0; i < pPriv->nbfrs; ++i) {
        memset(&bfr, 0, sizeof(bfr));
        bfr.type = type;
        bfr.index = i;
        if (ioctl(fd, VIDIOC_QUERYBUF, &bfr) < 0)
            goto xit;
        offset = bfr.m.offset;
        pPriv->bfrs[i].offset = offset;
        pPriv->bfrs[i].length = bfr.length;
        bsz = offset + bfr.length;
        if (pPriv->bufsz < bsz)
            pPriv->bufsz = bsz;
    }

    for (i = 0; i < pPriv->nbfrs; ++i) {
        pPriv->bfrs[i].start = mmap(NULL, bfr.length, PROT_READ | PROT_WRITE,
                                    MAP_SHARED, fd, pPriv->bfrs[i].offset);
        if (pPriv->bfrs[i].start == MAP_FAILED)
            goto xit;
    }

    for (i = 0; i < pPriv->nbfrs; ++i) {
        DBLOG(3, "bfr %d ofs %#lx adr %p sz %lu\n", i, pPriv->bfrs[i].offset,
              pPriv->bfrs[i].start, (unsigned long) pPriv->bfrs[i].length);
        memset(pPriv->bfrs[i].start, 0x80, pPriv->bfrs[i].length);
    }

    pPriv->last = 0;
    while (pPriv->last < pPriv->nbfrs - 1) {
        bfr.index = pPriv->last++;
        bfr.type = type;
        if (ioctl(fd, VIDIOC_QBUF, &bfr) < 0)
            goto xit;
    }
    return;

 xit:
    z4l_ovly_unmap(pPriv);
}

static int
z4l_ovly_dqbuf(Z4lPortPrivRec *pPriv)
{
    int stat;
    struct v4l2_buffer bfr;
    int fd = pPriv->fd;

#ifdef HAVE_SELECT
    struct timeval tmo;
    fd_set dqset;

    FD_ZERO(&dqset);
    FD_SET(pPriv->fd, &dqset);
    tmo.tv_sec = 0;
    tmo.tv_usec = 0;
    if (select(fd + 1, &dqset, NULL, NULL, &tmo) <= 0)
        return -1;
#endif
    memset(&bfr, 0, sizeof(bfr));
    bfr.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
    stat = ioctl(fd, VIDIOC_DQBUF, &bfr);
    DBLOG(3, "dqbuf %d,%d,%d,%d\n", stat, bfr.index, pPriv->last, errno);

    return stat == 0 ? bfr.index : -1;
}

static int
z4l_open_device(Z4lPortPrivRec *pPriv)
{
    int enable;

    if (pPriv->fd < 0) {
        pPriv->fd = open(&pPriv->dev_path[0], O_RDWR, 0);
        DBLOG(1, "open(%s)=%d\n", &pPriv->dev_path[0], pPriv->fd);
        enable = 1;
#ifdef NONBLK_IO
        if (IoCtl(pPriv->fd, FIONBIO, &enable, 1) != 0) {
            DBLOG(1, "open can't enable nonblocking\n");
            close(pPriv->fd);
            pPriv->fd = -1;
        }
#endif
    }
    return pPriv->fd;
}

static int
z4l_close_device(Z4lPortPrivRec *pPriv)
{
    int ret = 0;

    if (pPriv->fd >= 0) {
        ret = close(pPriv->fd);
        pPriv->fd = -1;
        DBLOG(1, "close()=%d\n", ret);
    }
    if (pPriv->run > 0) {
        z4l_ovly_unmap(pPriv);
        pPriv->run = -1;
    }

    return ret;
}

static int
z4l_ovly_reset(Z4lPortPrivRec *pPriv)
{
    int ret = 0;

    if (pPriv->run > 0) {
        z4l_close_device(pPriv);
        ret = z4l_open_device(pPriv);
    }

    return ret;
}

static unsigned int
z4l_fourcc_pixfmt(int fourcc)
{
    unsigned int pixfmt = -1;

    switch (fourcc) {
    case FOURCC_UYVY:
        pixfmt = V4L2_PIX_FMT_UYVY;
        break;
    case FOURCC_YV12:
        pixfmt = V4L2_PIX_FMT_YVU420;
        break;
    case FOURCC_Y800:
    case FOURCC_I420:
        pixfmt = V4L2_PIX_FMT_YUV420;
        break;
    case FOURCC_YUY2:
        pixfmt = V4L2_PIX_FMT_YUYV;
        break;
    }

    return pixfmt;
}

static void
z4l_ovly_pixfmt(Z4lPortPrivRec *pPriv, unsigned int pixfmt)
{
    struct v4l2_framebuffer fbuf;

    DBLOG(1, "pixfmt %4.4s %4.4s\n", (char *) &pPriv->pixfmt, (char *) &pixfmt);
    memset(&fbuf, 0, sizeof(fbuf));
    IoCtl(pPriv->fd, VIDIOC_G_FBUF, &fbuf, 1);
    fbuf.fmt.pixelformat = pixfmt;
    fbuf.base = NULL;
    IoCtl(pPriv->fd, VIDIOC_S_FBUF, &fbuf, 1);
    pPriv->pixfmt = pixfmt;
}

static void
z4l_ovly_bfr(Z4lPortPrivRec *pPriv, int width, int height)
{
    struct v4l2_format fmt;

    DBLOG(1, "sfmt ovly %dx%d\n", width, height);
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = 0x102;
    IoCtl(pPriv->fd, VIDIOC_G_FMT, &fmt, 1);
    fmt.fmt.win.field = V4L2_FIELD_NONE;
    fmt.fmt.win.w.width = pPriv->width = width;
    fmt.fmt.win.w.height = pPriv->height = height;
    IoCtl(pPriv->fd, VIDIOC_S_FMT, &fmt, 1);
}

static void
z4l_ovly_rect(Z4lPortPrivRec *pPriv,
              int src_x, int src_y, int src_w, int src_h,
              int drw_x, int drw_y, int drw_w, int drw_h)
{
    int x, dx, w, y, dy, h;
    struct v4l2_format fmt;

    pPriv->src_x = src_x;
    pPriv->src_y = src_y;
    pPriv->src_w = src_w;
    pPriv->src_h = src_h;
    pPriv->drw_x = drw_x;
    pPriv->drw_y = drw_y;
    pPriv->drw_w = drw_w;
    pPriv->drw_h = drw_h;

    if ((drw_x -= z4l_x_offset) < 0) {
        if ((w = pPriv->drw_w) <= 0)
            w = 1;
        x = -drw_x;
        dx = x * pPriv->src_w / w;
        src_x = pPriv->src_x + dx;
        src_w = pPriv->src_w - dx;
        drw_w = pPriv->drw_w - x;
        drw_x = 0;
    }

    if ((drw_y -= z4l_y_offset) < 0) {
        if ((h = pPriv->drw_h) <= 0)
            h = 1;
        y = -drw_y;
        dy = y * pPriv->src_h / h;
        src_y = pPriv->src_y + dy;
        src_h = pPriv->src_h - dy;
        drw_h = pPriv->drw_h - y;
        drw_y = 0;
    }

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = 0x100;
    IoCtl(pPriv->fd, VIDIOC_G_FMT, &fmt, 1);
    if (pPriv->src_is_set != 0) {
        if (src_x != fmt.fmt.win.w.left || src_y != fmt.fmt.win.w.top ||
            src_w != fmt.fmt.win.w.width || src_h != fmt.fmt.win.w.height)
            pPriv->src_is_set = 0;
    }
    if (pPriv->src_is_set == 0) {
        pPriv->src_is_set = 1;
        fmt.fmt.win.w.left = src_x;
        fmt.fmt.win.w.top = src_y;
        fmt.fmt.win.w.width = src_w;
        fmt.fmt.win.w.height = src_h;
        IoCtl(pPriv->fd, VIDIOC_S_FMT, &fmt, 1);
        DBLOG(3, "  set src %d,%d %dx%d\n", src_x, src_y, src_w, src_h);
    }
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = 0x101;
    IoCtl(pPriv->fd, VIDIOC_G_FMT, &fmt, 1);
    if (pPriv->drw_is_set != 0) {
        if (drw_x != fmt.fmt.win.w.left || drw_y != fmt.fmt.win.w.top ||
            drw_w != fmt.fmt.win.w.width || drw_h != fmt.fmt.win.w.height)
            pPriv->drw_is_set = 0;
    }
    if (pPriv->drw_is_set == 0) {
        pPriv->drw_is_set = 1;
        fmt.fmt.win.w.left = drw_x;
        fmt.fmt.win.w.top = drw_y;
        fmt.fmt.win.w.width = drw_w;
        fmt.fmt.win.w.height = drw_h;
        IoCtl(pPriv->fd, VIDIOC_S_FMT, &fmt, 1);
        DBLOG(3, "  set drw %d,%d %dx%d\n", drw_x, drw_y, drw_w, drw_h);
    }
}

static void
z4l_ovly_pitch(unsigned int pixfmt, int w, int h, int *py_pitch,
               int *puv_pitch, int *poffset1, int *poffset2, int *psize)
{
    int y_pitch, uv_pitch;
    int offset1, offset2;
    int size, is_420;

    switch (pixfmt) {
    case V4L2_PIX_FMT_YVU420:
    case V4L2_PIX_FMT_YUV420:
        is_420 = 1;
        y_pitch = ((w + 1) / 2) * 2;
        uv_pitch = (w + 1) / 2;
        break;
    default:
        is_420 = 0;
        y_pitch = ((w + 1) / 2) * 4;
        uv_pitch = 0;
        break;
    }

    offset1 = y_pitch * h;
    offset2 = uv_pitch * h;

    if (is_420 != 0)
        offset2 /= 2;

    size = offset1 + 2 * offset2;
    *py_pitch = y_pitch;
    *puv_pitch = uv_pitch;
    *poffset1 = offset1;
    *poffset2 = offset2;
    *psize = size;
}

static int
z4l_ovly_set_colorkey(Z4lPortPrivRec *pPriv, int key)
{
    struct v4l2_format fmt;

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
    if (IoCtl(pPriv->fd, VIDIOC_G_FMT, &fmt, 1) < 0)
        return 0;
    fmt.fmt.win.chromakey = key;
    if (IoCtl(pPriv->fd, VIDIOC_S_FMT, &fmt, 1) < 0)
        return 0;
    pPriv->colorkey = key;

    return 1;
}

static int
z4l_ovly_get_colorkey(Z4lPortPrivRec *pPriv, int *key)
{
    struct v4l2_format fmt;

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
    if (IoCtl(pPriv->fd, VIDIOC_G_FMT, &fmt, 1) < 0)
        return 0;
    *key = fmt.fmt.win.chromakey;

    return 1;
}

static int
z4l_ovly_set_keymode(Z4lPortPrivRec *pPriv, int enable)
{
    struct v4l2_framebuffer fbuf;

    memset(&fbuf, 0, sizeof(fbuf));
    if (IoCtl(pPriv->fd, VIDIOC_G_FBUF, &fbuf, 1) < 0)
        return 0;

    if (enable != 0)
        fbuf.flags |= V4L2_FBUF_FLAG_CHROMAKEY;
    else
        fbuf.flags &= ~V4L2_FBUF_FLAG_CHROMAKEY;

    fbuf.base = NULL;
    if (IoCtl(pPriv->fd, VIDIOC_S_FBUF, &fbuf, 1) < 0)
        return 0;
    pPriv->keymode = enable;

    return 1;
}

static int
z4l_ovly_get_keymode(Z4lPortPrivRec *pPriv, int *enable)
{
    struct v4l2_framebuffer fbuf;

    memset(&fbuf, 0, sizeof(fbuf));
    if (IoCtl(pPriv->fd, VIDIOC_G_FBUF, &fbuf, 1) < 0)
        return 0;
    *enable = (fbuf.flags & V4L2_FBUF_FLAG_CHROMAKEY) != 0 ? 1 : 0;

    return 1;
}

static int
z4l_ovly_set_encoding(Z4lPortPrivRec *pPriv, int id)
{
    int l, n, inp;
    const char *cp;
    t_std_data *sp;
    XF86VideoEncodingPtr enc;
    XF86VideoAdaptorPtr adpt;
    v4l2_std_id std;
    struct v4l2_format fmt;
    struct v4l2_framebuffer fbuf;

    adpt = pPriv->adpt;
    DBLOG(1, "z4l_ovly_set_encoding(%d)\n", id);
    if (id < 0 || id >= adpt->nEncodings)
        return 0;
    enc = &adpt->pEncodings[id];
    cp = &enc->name[0];
    n = sizeof(int) - 1;
    l = strlen(cp) + 1;
    l = (l + n) & ~n;
    sp = (t_std_data *) (cp + l);
    inp = sp->inp;

    DBLOG(1, " nm %s fmt %4.4s inp %d std %llx\n",
          cp, (char *) &sp->fmt, sp->inp, sp->std);

    if (IoCtl(pPriv->fd, VIDIOC_S_INPUT, &inp, 1) < 0)
        return 0;

    std = sp->std;
    if (IoCtl(pPriv->fd, VIDIOC_S_STD, &std, 1) < 0)
        return 0;

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (IoCtl(pPriv->fd, VIDIOC_G_FMT, &fmt, 1) < 0)
        return 0;

    fmt.fmt.pix.pixelformat = sp->fmt;
    if (IoCtl(pPriv->fd, VIDIOC_S_FMT, &fmt, 1) < 0)
        return 0;
    memset(&fbuf, 0, sizeof(fbuf));

    if (IoCtl(pPriv->fd, VIDIOC_G_FBUF, &fbuf, 1) < 0)
        return 0;

    fbuf.fmt.pixelformat = sp->fmt;
    fbuf.base = NULL;
    if (IoCtl(pPriv->fd, VIDIOC_S_FBUF, &fbuf, 1) < 0)
        return 0;
    pPriv->pixfmt = sp->fmt;
    pPriv->enc = enc;
    pPriv->src_is_set = pPriv->drw_is_set = 0;

    return 1;
}

static int
z4l_ovly_get_encoding(Z4lPortPrivRec *pPriv, int *id)
{
    XF86VideoEncodingPtr enc = pPriv->enc;

    *id = enc->id;
    return 1;
}

static void
z4l_ovly_stop(Z4lPortPrivRec *pPriv)
{
    int type, enable, fd;

    if (pPriv->run < 0)
        return;

    fd = pPriv->fd;
    if (pPriv->dir > 0) {
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(fd, VIDIOC_STREAMOFF, &type);
    }

    if (pPriv->dir <= 0) {
        enable = 0;
        ioctl(fd, VIDIOC_OVERLAY, &enable);
    }

    if (pPriv->dir != 0)
        z4l_ovly_unmap(pPriv);

    pPriv->run = -1;
    z4l_close_device(pPriv);
}

static void
z4l_ovly_start(Z4lPortPrivRec *pPriv, int dir)
{
    int enable;

    if (pPriv->run >= 0)
        return;

    if ((pPriv->dir = dir) != 0)
        z4l_ovly_map(pPriv, dir);

    enable = 1;

    if (IoCtl(pPriv->fd, VIDIOC_OVERLAY, &enable, 1) != 0) {
        z4l_ovly_stop(pPriv);
        return;
    }

    pPriv->run = 1;
}

static int
z4l_region_equal(RegionPtr ap, RegionPtr bp)
{
    int nboxes;
    BoxPtr abox, bbox;

    if (ap == NULL && bp == NULL)
        return 1;
    if (ap == NULL || bp == NULL)
        return 0;

    if ((nboxes = REGION_NUM_RECTS(ap)) != REGION_NUM_RECTS(bp) ||
        ap->extents.x1 != bp->extents.x1 ||
        ap->extents.x2 != bp->extents.x2
        || ap->extents.y1 != bp->extents.y1 || ap->extents.y2 != bp->extents.y2)
        return 0;

    abox = REGION_RECTS(ap);
    bbox = REGION_RECTS(bp);

    while (--nboxes >= 0) {
        if (abox->x1 != bbox->x1 || abox->y1 != bbox->y1 ||
            abox->x2 != bbox->x2 || abox->y2 != bbox->y2)
            return 0;
        ++abox;
        ++bbox;
    }

    return 1;
}

static void
z4l_setup_colorkey(Z4lPortPrivRec *pPriv, ScreenPtr pScrn, RegionPtr clipBoxes)
{
    if (pPriv->run > 0 && pPriv->dir <= 0 && pPriv->keymode != 0 &&
        z4l_region_equal(&pPriv->clips, clipBoxes) == 0) {
        xf86XVFillKeyHelper(pScrn, pPriv->colorkey, clipBoxes);
        REGION_COPY(pScrn, &pPriv->clips, clipBoxes);
    }
}

static void
Z4lStopVideo(ScrnInfoPtr pScrni, pointer data, Bool exit)
{
    Z4lPortPrivRec *pPriv = (Z4lPortPrivRec *) data;

    DBLOG(1, "Z4lStopVideo()\n");

    if (exit != 0)
        z4l_ovly_stop(pPriv);
    else
        pPriv->src_is_set = pPriv->drw_is_set = 0;

    REGION_EMPTY(pScrni->pScreen, &pPriv->clips);
}

static void
Z4lQueryBestSize(ScrnInfoPtr pScrni, Bool motion,
                 short vid_w, short vid_h, short drw_w, short drw_h,
                 unsigned int *p_w, unsigned int *p_h, pointer data)
{
    if (drw_w > MAX_OVLY_WIDTH)
        drw_w = MAX_OVLY_WIDTH;
    if (drw_h > MAX_OVLY_HEIGHT)
        drw_h = MAX_OVLY_HEIGHT;

    *p_w = drw_w;
    *p_h = drw_h;
    DBLOG(1, "Z4lQueryBestSize(%d, src %dx%d dst %dx%d)\n", motion, vid_w,
          vid_h, drw_w, drw_h);
}

static int
Z4lPutImage(ScrnInfoPtr pScrni, short src_x, short src_y, short drw_x,
            short drw_y, short src_w, short src_h, short drw_w, short drw_h,
            int id, unsigned char *buf, short width, short height,
            Bool sync, RegionPtr clipBoxes, pointer data, DrawablePtr pDraw)
{
    int fd, size;
    int y_pitch, uv_pitch, offset1, offset2;
    unsigned char *src, *dst;
    unsigned int pixfmt;
    struct v4l2_buffer bfr;
    Z4lPortPrivRec *pPriv = (Z4lPortPrivRec *) data;

    if (pPriv->run > 0 && pPriv->dir >= 0)
        return BadMatch;
    if (pPriv->fd < 0) {
        z4l_open_device(pPriv);
        if (pPriv->fd < 0)
            return BadValue;
    }

    fd = pPriv->fd;
    if (pPriv->run < 0) {
        DBLOG(2, "PutImg id %#x src %d,%d %dx%d drw %d,%d %dx%d bfr %p "
              "%dx%d data %p\n", id, src_x, src_y, src_w, src_h, drw_x,
              drw_y, drw_w, drw_h, buf, width, height, data);
        pPriv->pixfmt = pPriv->height = -1;
        pPriv->src_is_set = pPriv->drw_is_set = 0;
    }

    pixfmt = z4l_fourcc_pixfmt(id);
    if (pixfmt != pPriv->pixfmt) {
        z4l_ovly_reset(pPriv);
        z4l_ovly_pixfmt(pPriv, pixfmt);
    }
    if (pPriv->width != width || pPriv->height != height) {
        z4l_ovly_reset(pPriv);
        z4l_ovly_bfr(pPriv, width, height);
    }

    if (pPriv->src_is_set == 0 || pPriv->drw_is_set == 0 ||
        pPriv->src_x != src_x || pPriv->src_y != src_y ||
        pPriv->src_w != src_w || pPriv->src_h != src_h ||
        pPriv->drw_x != drw_x || pPriv->drw_y != drw_y ||
        pPriv->drw_w != drw_w || pPriv->drw_h != drw_h)
        z4l_ovly_rect(pPriv, src_x, src_y, src_w, src_h, drw_x, drw_y, drw_w,
                      drw_h);

    if (pPriv->run < 0) {
        z4l_ovly_start(pPriv, -1);
        if (pPriv->run < 0)
            return BadValue;
    }

    if (pPriv->last < 0 && (pPriv->last = z4l_ovly_dqbuf(pPriv)) < 0)
        return BadAlloc;

    z4l_ovly_pitch(pixfmt, width, height, &y_pitch, &uv_pitch,
                   &offset1, &offset2, &size);
    src = buf;
    dst = (unsigned char *) pPriv->bfrs[pPriv->last].start;
    DBLOG(3, "cpy %4.4s src %p dst %p yp %d uvp %d o1 %d o2 %d sz %d\n",
          (char *) &id, src, dst, y_pitch, uv_pitch, offset1, offset2, size);

    if (id == FOURCC_Y800) {
        memcpy(dst, src, offset1);
        src += offset1;
        dst += offset1;
        memset(dst, 0x80, 2 * offset2);
    }
    else
        memcpy(dst, src, size);

    memset(&bfr, 0, sizeof(bfr));
    bfr.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
    bfr.index = pPriv->last;
    bfr.timestamp.tv_sec = 0;
    bfr.timestamp.tv_usec = 0;
    bfr.flags |= V4L2_BUF_FLAG_TIMECODE;
    if (IoCtl(fd, VIDIOC_QBUF, &bfr, 1) != 0)
        return BadAccess;

    pPriv->last = z4l_ovly_dqbuf(pPriv);
    z4l_setup_colorkey(pPriv, pScrni->pScreen, clipBoxes);

    return Success;
}

static int
Z4lQueryImageAttributes(ScrnInfoPtr pScrni, int id, unsigned short *width,
                        unsigned short *height, int *pitches, int *offsets)
{
    int w, h, size;
    int y_pitch, uv_pitch, offset1, offset2;
    unsigned int pixfmt = z4l_fourcc_pixfmt(id);

    w = *width;
    h = *height;
    if (w > MAX_OVLY_WIDTH)
        w = MAX_OVLY_WIDTH;
    if (h > MAX_OVLY_HEIGHT)
        h = MAX_OVLY_HEIGHT;

    z4l_ovly_pitch(pixfmt, w, h, &y_pitch, &uv_pitch,
                   &offset1, &offset2, &size);

    if (offsets != NULL)
        offsets[0] = 0;
    if (pitches != NULL)
        pitches[0] = y_pitch;

    switch (pixfmt) {
    case V4L2_PIX_FMT_YVU420:
    case V4L2_PIX_FMT_YUV420:
        if (offsets != NULL) {
            offsets[1] = offset1;
            offsets[2] = offset1 + offset2;
        }
        if (pitches != NULL)
            pitches[1] = pitches[2] = uv_pitch;
        h = (h + 1) & ~1;
        break;
    }

    w = (w + 1) & ~1;
    *width = w;
    *height = h;
    DBLOG(1, "Z4lQueryImageAttributes(%4.4s) = %d, %dx%d %d/%d %d/%d\n",
          (char *) &id, size, w, h, y_pitch, uv_pitch, offset1, offset2);

    return size;
}

static int
Z4lPutVideo(ScrnInfoPtr pScrni, short src_x, short src_y, short drw_x,
            short drw_y, short src_w, short src_h, short drw_w, short drw_h,
            RegionPtr clipBoxes, pointer data, DrawablePtr pDraw)
{
    int id;
    Z4lPortPrivRec *pPriv = (Z4lPortPrivRec *) data;

    DBLOG(2, "PutVid src %d,%d %dx%d drw %d,%d %dx%d data %p\n",
          src_x, src_y, src_w, src_h, drw_x, drw_y, drw_w, drw_h, data);

    if (z4l_open_device(pPriv) >= 0) {
        if (pPriv->run < 0) {
            DBLOG(2, "PutVid start\n");
            z4l_ovly_get_encoding(pPriv, &id);
            z4l_ovly_set_encoding(pPriv, id);
        }
        DBLOG(2, "PutVid priv %d,%d %dx%d drw %d,%d %dx%d\n",
              pPriv->src_x, pPriv->src_y, pPriv->src_w, pPriv->src_h,
              pPriv->drw_x, pPriv->drw_y, pPriv->drw_w, pPriv->drw_h);
        if (pPriv->src_is_set == 0 || pPriv->drw_is_set == 0 ||
            pPriv->src_w != src_w || pPriv->src_h != src_h ||
            pPriv->drw_x != drw_x || pPriv->drw_y != drw_y ||
            pPriv->drw_w != drw_w || pPriv->drw_h != drw_h)
            z4l_ovly_rect(pPriv, src_x, src_y, src_w, src_h, drw_x, drw_y,
                          drw_w, drw_h);
        if (pPriv->run < 0)
            z4l_ovly_start(pPriv, 0);

        z4l_setup_colorkey(pPriv, pScrni->pScreen, clipBoxes);
    }

    return Success;
}

static XF86VideoEncodingPtr
Z4lNewEncoding(XF86VideoEncodingPtr *encs, int *nencs)
{
    XF86VideoEncodingPtr enc;
    XF86VideoEncodingPtr tencs =
        (XF86VideoEncodingPtr) realloc(*encs, sizeof(*tencs) * (*nencs + 1));

    if (tencs == NULL)
        return NULL;

    *encs = tencs;
    enc = &tencs[*nencs];
    ++*nencs;
    memset(enc, 0, sizeof(*enc));

    return enc;
}

static void
Z4lEncodingName(char *ename, int l, char *inp_name, char *std_name, char *fmt)
{
    int i, ch;

    while ((ch = *inp_name++) != 0) {
        if (isalnum(ch) == 0)
            continue;
        if (--l <= 0)
            goto xit;
        *ename++ = ch;
    }

    if (--l <= 0)
        goto xit;

    *ename++ = '-';

    while ((ch = *std_name++) != 0) {
        if (isalnum(ch) == 0)
            continue;
        if (--l <= 0)
            goto xit;
        *ename++ = ch;
    }

    if (--l <= 0)
        goto xit;

    *ename++ = '-';
    i = 4;

    while (--i >= 0 && (ch = *fmt++) != 0) {
        if (isalnum(ch) == 0)
            continue;
        if (--l <= 0)
            goto xit;
        *ename++ = ch;
    }

 xit:
    *ename = 0;
}

static int
Z4lAddEncoding(XF86VideoEncodingPtr enc, const char *name, int id, int width,
               int height, int numer, int denom, int inp, v4l2_std_id std,
               unsigned int fmt)
{
    int l, n;
    t_std_data *sp;
    char *cp;

    n = sizeof(int) - 1;
    l = strlen(&name[0]) + 1;
    l = (l + n) & ~n;
    n = l + sizeof(*sp);
    cp = (char *) malloc(n);

    if (cp == NULL)
        return 0;

    sp = (t_std_data *) (cp + l);
    enc->id = id;
    strcpy(cp, &name[0]);
    enc->name = cp;
    enc->width = width;
    enc->height = height;
    enc->rate.numerator = numer;
    enc->rate.denominator = denom;
    sp->inp = inp;
    sp->std = std;
    sp->fmt = fmt;
    DBLOG(1, "enc %s\n", &name[0]);

    return 1;
}

static XF86ImagePtr
Z4lNewImage(XF86ImagePtr *imgs, int *nimgs)
{
    XF86ImagePtr img;
    XF86ImagePtr timgs =
        (XF86ImagePtr) realloc(*imgs, sizeof(*timgs) * (*nimgs + 1));

    if (timgs == NULL)
        return NULL;

    *imgs = timgs;
    img = &timgs[*nimgs];
    ++*nimgs;
    memset(img, 0, sizeof(*img));

    return img;
}

static int
Z4lAddImage(XF86ImagePtr img, XF86ImagePtr ip)
{
    *img = *ip;
    DBLOG(1, "img %4.4s\n", (char *) &img->id);
    return 1;
}

static XF86AttributePtr
Z4lNewAttribute(XF86AttributePtr *attrs, int *nattrs)
{
    XF86AttributePtr attr;
    XF86AttributePtr tattrs =
        (XF86AttributePtr) realloc(*attrs, sizeof(*tattrs) * (*nattrs + 1));

    if (tattrs == NULL)
        return NULL;

    *attrs = tattrs;
    attr = &tattrs[*nattrs];
    ++*nattrs;
    memset(attr, 0, sizeof(*attr));

    return attr;
}

static void
Z4lAttributeName(char *bp, int l, char *cp)
{
    int ch;
    char *atomNm = bp;

    if (l > 0) {
        *bp++ = 'X';
        --l;
    }
    if (l > 0) {
        *bp++ = 'V';
        --l;
    }
    if (l > 0) {
        *bp++ = '_';
        --l;
    }

    while (l > 0 && (ch = *cp++) != 0) {
        if (isalnum(ch) == 0)
            continue;
        *bp++ = toupper(ch);
    }

    *bp = 0;
    MakeAtom(&atomNm[0], strlen(&atomNm[0]), TRUE);
}

static int
Z4lAddAttribute(XF86AttributePtr attr, char *name, int min, int max, int flags)
{
    char *cp = (char *) malloc(strlen((char *) &name[0]) + 1);

    if (cp == NULL)
        return 0;

    attr->name = cp;
    strcpy(&attr->name[0], name);
    attr->min_value = min;
    attr->max_value = max;
    attr->flags = flags;
    DBLOG(1, "attr %s\n", attr->name);

    return 1;
}

static XF86VideoAdaptorPtr
Z4lNewAdaptor(XF86VideoAdaptorPtr **adpts, int *nadpts, int nattrs)
{
    int n;
    Z4lPortPrivRec *pPriv;
    XF86VideoAdaptorPtr adpt, *tadpts;

    tadpts = (XF86VideoAdaptorPtr *) realloc(*adpts,
                                             sizeof(*tadpts) * (*nadpts + 1));

    if (tadpts == NULL)
        return NULL;

    *adpts = tadpts;
    n = sizeof(*adpt) + sizeof(*pPriv) + 1 * sizeof(*adpt->pPortPrivates);
    n += (nattrs - 1) * sizeof(pPriv->attrIds[0]);
    adpt = (XF86VideoAdaptorPtr) malloc(n);

    if (adpt == NULL)
        return NULL;

    memset(adpt, 0, n);
    tadpts[*nadpts] = adpt;
    ++*nadpts;
    adpt->pPortPrivates = (DevUnion *) &adpt[1];
    pPriv = (Z4lPortPrivRec *) & adpt->pPortPrivates[1];
    adpt->pPortPrivates[0].ptr = (pointer) pPriv;
    pPriv->adpt = adpt;
    adpt->nPorts = 1;

    return adpt;
}

static int
Z4lSetPortAttribute(ScrnInfoPtr pScrni, Atom attribute, INT32 value,
                    pointer data)
{
    Z4lPortPrivRec *pPriv = (Z4lPortPrivRec *) data;
    XF86VideoAdaptorPtr adpt;
    XF86AttributePtr attr;
    struct v4l2_control ctrl;
    int i, nattrs, attrId, val;
    const char *name = NameForAtom(attribute);
    int old_fd = pPriv->fd;

    DBLOG(1, "Z4lSetPortAttribute(%#lx,%d) '%s'\n", (unsigned long) attribute,
          (int) value, name != NULL ? name : "_null_");

    if (name == NULL)
        return BadImplementation;
    if (old_fd < 0 && z4l_open_device(pPriv) < 0)
        return BadAccess;

    adpt = pPriv->adpt;
    attr = adpt->pAttributes;
    nattrs = adpt->nAttributes;

    for (i = 0; i < nattrs; ++i, ++attr)
        if (strcmp(attr->name, name) == 0)
            break;

    if (i >= nattrs)
        return BadMatch;

    attrId = pPriv->attrIds[i];
    val = value;

    switch (attrId) {
    case ATTR_ENCODING_ID:
        z4l_ovly_set_encoding(pPriv, val);
        break;
    case ATTR_KEYMODE_ID:
        z4l_ovly_set_keymode(pPriv, val);
        REGION_EMPTY(pScrni->pScreen, &pPriv->clips);
        z4l_setup_colorkey(pPriv, pScrni->pScreen, &pPriv->clips);
        break;
    case ATTR_COLORKEY_ID:
        z4l_ovly_set_colorkey(pPriv, val);
        break;
    default:
        memset(&ctrl, 0, sizeof(ctrl));
        ctrl.id = attrId + V4L2_CID_BASE;
        ctrl.value = val;
        if (IoCtl(pPriv->fd, VIDIOC_S_CTRL, &ctrl, 1) != 0)
            return BadMatch;
        break;
    }

    if (old_fd < 0)
        z4l_close_device(pPriv);

    return Success;
}

static int
Z4lGetPortAttribute(ScrnInfoPtr pScrni, Atom attribute, INT32 *value,
                    pointer data)
{
    Z4lPortPrivRec *pPriv = (Z4lPortPrivRec *) data;
    XF86VideoAdaptorPtr adpt;
    XF86AttributePtr attr;
    struct v4l2_control ctrl;
    int i, nattrs, attrId, val;
    const char *name = NameForAtom(attribute);
    int old_fd = pPriv->fd;

    DBLOG(1, "Z4lGetPortAttribute(%#lx) '%s'\n",
          (unsigned long) attribute, name != NULL ? name : "_null_");

    if (name == NULL)
        return BadImplementation;
    if (old_fd < 0 && z4l_open_device(pPriv) < 0)
        return BadAccess;

    adpt = pPriv->adpt;
    attr = adpt->pAttributes;
    nattrs = adpt->nAttributes;

    for (i = 0; i < nattrs; ++i, ++attr)
        if (strcmp(attr->name, name) == 0)
            break;

    if (i >= nattrs)
        return BadMatch;

    attrId = pPriv->attrIds[i];
    val = 0;

    switch (attrId) {
    case ATTR_ENCODING_ID:
        z4l_ovly_get_encoding(pPriv, &val);
        *value = val;
        break;
    case ATTR_KEYMODE_ID:
        z4l_ovly_get_keymode(pPriv, &val);
        *value = val;
        break;
    case ATTR_COLORKEY_ID:
        z4l_ovly_get_colorkey(pPriv, &val);
        break;
    default:
        memset(&ctrl, 0, sizeof(ctrl));
        ctrl.id = attrId + V4L2_CID_BASE;
        if (IoCtl(pPriv->fd, VIDIOC_G_CTRL, &ctrl, 1) != 0)
            return BadMatch;
        val = ctrl.value;
        break;
    }

    if (old_fd < 0)
        z4l_close_device(pPriv);

    *value = val;

    return Success;
}

static void (*oldAdjustFrame)(ScrnInfoPtr pScrni, int x, int y) = NULL;

static void
Z4lAdjustFrame(ScrnInfoPtr pScrni, int x, int y)
{
    int i;
    XF86VideoAdaptorPtr adpt;
    Z4lPortPrivRec *pPriv;

    DBLOG(3, "Z4lAdjustFrame(%d,%d)\n", x, y);
    z4l_x_offset = x;
    z4l_y_offset = y;
    oldAdjustFrame(pScrni, x, y);

    /* xv adjust does not handle putvideo case */
    for (i = 0; i < Z4l_nAdaptors; ++i) {
        adpt = Z4l_pAdaptors[i];
        pPriv = (Z4lPortPrivRec *) adpt->pPortPrivates[0].ptr;
        if (pPriv->run > 0) {
            pPriv->drw_is_set = 0;
            z4l_ovly_rect(pPriv,
                          pPriv->src_x, pPriv->src_y, pPriv->src_w,
                          pPriv->src_h, pPriv->drw_x, pPriv->drw_y,
                          pPriv->drw_w, pPriv->drw_h);
        }
    }
}

static int
Z4lInit(ScrnInfoPtr pScrni, XF86VideoAdaptorPtr **adaptors)
{
    int i, id, fd, dev, enable, has_video, has_colorkey;
    int ctl, cinp, inp, std, fmt, has_image;
    int nadpts, nattrs, nencs, cenc, nimgs;
    int numer, denom, width, height;
    unsigned int pixfmt, cpixfmt, opixfmt;
    XF86VideoAdaptorPtr *adpts, adpt;
    XF86AttributePtr attrs, attr;
    XF86VideoEncodingPtr encs, enc;
    XF86ImagePtr ip, img, imgs;
    Z4lPortPrivRec *pPriv;
    char *dp;
    const char *msg;
    char enc_name[256], attr_name[256];
    int attrIds[V4L2_CID_LASTP1 - V4L2_CID_BASE + ATTR_MAX_ID];
    struct v4l2_capability capability;
    v4l2_std_id cstd_id, std_id;
    struct v4l2_standard standard;
    struct v4l2_format format, cfmt;
    struct v4l2_input input;
    struct v4l2_fmtdesc fmtdesc;
    struct v4l2_queryctrl queryctrl;
    struct v4l2_framebuffer fbuf;

    DBLOG(1, "Init\n");
    if (oldAdjustFrame == NULL) {
        oldAdjustFrame = pScrni->AdjustFrame;
        pScrni->AdjustFrame = Z4lAdjustFrame;
    }

    fd = -1;
    enc = NULL;
    encs = NULL;
    nencs = 0;
    img = NULL;
    imgs = NULL;
    nimgs = 0;
    attr = NULL;
    attrs = NULL;
    nattrs = 0;
    adpt = NULL;
    adpts = NULL;
    nadpts = 0;
    has_video = has_image = has_colorkey = 0;

    for (dev = 0; z4l_dev_paths[dev] != NULL; ++dev) {
        fd = open(z4l_dev_paths[dev], O_RDWR, 0);
        if (fd < 0)
            continue;
        DBLOG(1, "%s open ok\n", z4l_dev_paths[dev]);
        msg = NULL;
        enable = 1;
        if (IoCtl(fd, VIDIOC_QUERYCAP, &capability, 1) < 0)
            msg = "bad querycap";
        else if ((capability.capabilities & V4L2_CAP_VIDEO_OVERLAY) == 0)
            msg = "no overlay";
        else if ((capability.capabilities & V4L2_CAP_STREAMING) == 0)
            msg = "no streaming";
#ifdef NONBLK_IO
        else if (IoCtl(fd, FIONBIO, &enable, 1) != 0)
            msg = "can't enable non-blocking io";
#endif
        if (msg == NULL) {
            memset(&format, 0, sizeof(format));
            format.type = 0x100;
            if (IoCtl(fd, VIDIOC_G_FMT, &format, 1) != 0)
                msg = "no src/dst ovly fmt";
        }
        if (msg != NULL) {
            DBLOG(0, "%s %s\n", z4l_dev_paths[dev], msg);
            close(fd);
            continue;
        }

        memset(&cfmt, 0, sizeof(cfmt));
        cfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (IoCtl(fd, VIDIOC_G_FMT, &cfmt, 1) < 0)
            goto fail;
        if (IoCtl(fd, VIDIOC_G_STD, &cstd_id, 1) < 0)
            goto fail;
        if (IoCtl(fd, VIDIOC_G_INPUT, &cinp, 1) < 0)
            goto fail;
        cpixfmt = cfmt.fmt.pix.pixelformat;
        cenc = 0;
        for (inp = 0;; ++inp) {
            memset(&input, 0, sizeof(input));
            input.index = inp;
            if (IoCtl(fd, VIDIOC_ENUMINPUT, &input, 0) < 0)
                break;
            id = inp;
            if (IoCtl(fd, VIDIOC_S_INPUT, &id, 1) < 0)
                goto fail;
            for (std = 0;; ++std) {
                memset(&standard, 0, sizeof(standard));
                standard.index = std;
                if (IoCtl(fd, VIDIOC_ENUMSTD, &standard, 0) < 0)
                    break;
                std_id = standard.id;
                denom = standard.frameperiod.denominator;
                numer = standard.frameperiod.numerator;
                if (IoCtl(fd, VIDIOC_S_STD, &std_id, 1) < 0)
                    continue;
                memset(&format, 0, sizeof(format));
                format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if (IoCtl(fd, VIDIOC_G_FMT, &format, 1) < 0)
                    continue;
                width = format.fmt.pix.width;
                height = format.fmt.pix.height;
                for (fmt = 0;; ++fmt) {
                    memset(&fmtdesc, 0, sizeof(fmtdesc));
                    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    fmtdesc.index = fmt;
                    if (IoCtl(fd, VIDIOC_ENUM_FMT, &fmtdesc, 0) < 0)
                        break;
                    pixfmt = fmtdesc.pixelformat;
                    ip = &pixfmts[0];
                    for (i = sizeof(pixfmts) / sizeof(pixfmts[0]); --i >= 0;
                         ++ip)
                        if (z4l_fourcc_pixfmt(ip->id) == pixfmt)
                            break;

                    if (i >= 0) {
                        id = nencs;
                        has_video = 1;
                        if ((enc = Z4lNewEncoding(&encs, &nencs)) == NULL)
                            goto fail;
                        Z4lEncodingName(&enc_name[0], sizeof(enc_name),
                                        (char *) &input.name[0],
                                        (char *) &standard.name[0],
                                        (char *) &pixfmt);
                        if (Z4lAddEncoding
                            (enc, &enc_name[0], id, width, height, denom, numer,
                             inp, std_id, pixfmt) == 0)
                            goto fail;
                        if (std_id == cstd_id && inp == cinp
                            && pixfmt == cpixfmt)
                            cenc = id;
                    }
                }
            }
        }

        if (IoCtl(fd, VIDIOC_S_INPUT, &cinp, 1) < 0)
            goto fail;
        if (IoCtl(fd, VIDIOC_S_STD, &cstd_id, 1) < 0)
            goto fail;
        if (IoCtl(fd, VIDIOC_S_FMT, &cfmt, 1) < 0)
            goto fail;

        if (encs == NULL) {
            DBLOG(0, "no encodings\n");
            goto fail;
        }

        for (fmt = 0;; ++fmt) {
            memset(&fmtdesc, 0, sizeof(fmtdesc));
            fmtdesc.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
            fmtdesc.index = fmt;
            if (IoCtl(fd, VIDIOC_ENUM_FMT, &fmtdesc, 0) < 0)
                break;
            pixfmt = fmtdesc.pixelformat;
            ip = &pixfmts[0];
            for (i = sizeof(pixfmts) / sizeof(pixfmts[0]); --i >= 0; ++ip)
                if (z4l_fourcc_pixfmt(ip->id) == pixfmt)
                    break;

            if (i >= 0) {
                has_image = 1;
                if ((img = Z4lNewImage(&imgs, &nimgs)) == NULL)
                    goto fail;
                if (Z4lAddImage(img, ip) == 0)
                    goto fail;
            }
        }

        if (nimgs > 0) {
            id = nencs;
            if ((enc = Z4lNewEncoding(&encs, &nencs)) == NULL)
                goto fail;
            if (Z4lAddEncoding(enc, "XV_IMAGE", id, MAX_OVLY_WIDTH,
                               MAX_OVLY_HEIGHT, 0, 0, 0, 0, pixfmt) == 0)
                goto fail;
        }

        ctl = 0;
        for (ctl = 0; ctl < (V4L2_CID_LASTP1 - V4L2_CID_BASE); ++ctl) {
            memset(&queryctrl, 0, sizeof(queryctrl));
            queryctrl.id = V4L2_CID_BASE + ctl;
            if (IoCtl(fd, VIDIOC_QUERYCTRL, &queryctrl, 0) < 0)
                continue;
            if (queryctrl.type != V4L2_CTRL_TYPE_INTEGER &&
                queryctrl.type != V4L2_CTRL_TYPE_BOOLEAN)
                continue;
            attrIds[nattrs] = ctl;
            if ((attr = Z4lNewAttribute(&attrs, &nattrs)) == NULL)
                goto fail;
            Z4lAttributeName(&attr_name[0], sizeof(attr_name),
                             (char *) &queryctrl.name[0]);
            if (Z4lAddAttribute(attr, &attr_name[0],
                                queryctrl.minimum, queryctrl.maximum,
                                XvSettable | XvGettable) == 0)
                goto fail;
        }
        attrIds[nattrs] = ATTR_ENCODING_ID;
        if ((attr = Z4lNewAttribute(&attrs, &nattrs)) == NULL)
            goto fail;
        Z4lAttributeName(&attr_name[0], sizeof(attr_name),
                         (char *) ATTR_ENCODING);
        if (Z4lAddAttribute
            (attr, &attr_name[0], 0, nencs - 1, XvSettable | XvGettable) == 0)
            goto fail;
        memset(&fbuf, 0, sizeof(fbuf));
        if (IoCtl(fd, VIDIOC_G_FBUF, &fbuf, 1) < 0)
            goto fail;
        opixfmt = fbuf.fmt.pixelformat;

        if ((fbuf.capability & V4L2_FBUF_CAP_CHROMAKEY) != 0) {
            attrIds[nattrs] = ATTR_KEYMODE_ID;
            if ((attr = Z4lNewAttribute(&attrs, &nattrs)) == NULL)
                goto fail;
            Z4lAttributeName(&attr_name[0], sizeof(attr_name),
                             (char *) ATTR_KEYMODE);
            if (Z4lAddAttribute
                (attr, &attr_name[0], 0, 1, XvSettable | XvGettable) == 0)
                goto fail;
            attrIds[nattrs] = ATTR_COLORKEY_ID;
            if ((attr = Z4lNewAttribute(&attrs, &nattrs)) == NULL)
                goto fail;
            Z4lAttributeName(&attr_name[0], sizeof(attr_name),
                             (char *) ATTR_COLORKEY);
            if (Z4lAddAttribute
                (attr, &attr_name[0], 0, 0xffffff,
                 XvSettable | XvGettable) == 0)
                goto fail;
            has_colorkey = 1;
        }

        dp = malloc(strlen((char *) &capability.card[0]) + 1);
        if (dp == NULL)
            goto fail;
        strcpy(dp, (char *) &capability.card[0]);
        if ((adpt = Z4lNewAdaptor(&adpts, &nadpts, nattrs)) == NULL)
            goto fail;
        adpt->type = XvWindowMask | XvInputMask;
        if (has_video != 0)
            adpt->type |= XvVideoMask;
        if (has_image != 0)
            adpt->type |= XvImageMask;
        adpt->flags = VIDEO_OVERLAID_IMAGES | VIDEO_CLIP_TO_VIEWPORT;
        adpt->name = dp;
        adpt->type = XvInputMask | XvWindowMask | XvVideoMask | XvImageMask;
        adpt->pEncodings = encs;
        adpt->nEncodings = nencs;
        adpt->pFormats = &Formats[0];
        adpt->nFormats = sizeof(Formats) / sizeof(Formats[0]);
        adpt->pAttributes = attrs;
        adpt->nAttributes = nattrs;
        attrs = NULL;
        nattrs = 0;
        adpt->pImages = imgs;
        adpt->nImages = nimgs;
        imgs = NULL;
        nimgs = 0;
        adpt->PutVideo = Z4lPutVideo;
        adpt->StopVideo = Z4lStopVideo;
        adpt->SetPortAttribute = Z4lSetPortAttribute;
        adpt->GetPortAttribute = Z4lGetPortAttribute;
        adpt->QueryBestSize = Z4lQueryBestSize;
        adpt->PutImage = Z4lPutImage;
        adpt->QueryImageAttributes = Z4lQueryImageAttributes;
        pPriv = (Z4lPortPrivRec *) adpt->pPortPrivates[0].ptr;
        pPriv->fd = fd;
        pPriv->run = -1;
        pPriv->dir = 0;
        pPriv->nbfrs = -1;
        pPriv->bufsz = -1;
        pPriv->last = -1;
        pPriv->pixfmt = opixfmt;
        REGION_NULL(pScrni->pScreen, &pPriv->clips);
        strncpy(&pPriv->dev_path[0], z4l_dev_paths[dev],
                sizeof(pPriv->dev_path));
        pPriv->enc = &encs[cenc];
        for (i = 0; i < adpt->nAttributes; ++i)
            pPriv->attrIds[i] = attrIds[i];
        DBLOG(1, "adpt %s\n", dp);
        if (has_colorkey != 0) {
            z4l_ovly_set_colorkey(pPriv, DEFAULT_COLORKEY);
            z4l_ovly_set_keymode(pPriv, DEFAULT_KEYMODE);
        }
        close(fd);
        pPriv->fd = -1;
        adpt = NULL;
        cenc = 0;
        encs = NULL;
        nencs = 0;
    }

    DBLOG(0, "init done, %d device(s) found\n", nadpts);
    Z4l_nAdaptors = nadpts;
    Z4l_pAdaptors = adpts;
    *adaptors = adpts;

    return nadpts;

 fail:
    if (attrs != NULL) {
        for (i = 0; i < nattrs; ++i)
            if (attrs[i].name != NULL)
                free(attrs[i].name);
        free(attrs);
    }

    if (encs != NULL) {
        for (i = 0; i < nencs; ++i) {
            if (encs[i].name != NULL)
                free((char *) encs[i].name);
        }
        free(encs);
    }

    if (imgs != NULL)
        free(imgs);

    if (adpts != NULL) {
        for (i = 0; i < nadpts; ++i) {
            if ((adpt = adpts[i]) != NULL) {
                if (adpt->name != NULL)
                    free((char *) adpt->name);
                if ((attrs = adpt->pAttributes) != NULL) {
                    for (i = 0; i < adpt->nAttributes; ++i)
                        if (attrs[i].name != NULL)
                            free(attrs[i].name);
                    free(attrs);
                }
                if ((encs = adpt->pEncodings) != NULL) {
                    for (i = 0; i < adpt->nEncodings; ++i, ++enc)
                        if (encs[i].name != NULL)
                            free((char *) encs[i].name);
                    free(encs);
                }
                if ((imgs = adpt->pImages) != NULL)
                    free(imgs);
                free(adpt);
            }
        }
        free(adpts);
    }

    if (fd >= 0)
        close(fd);

    return 0;
}

static Bool
Z4lProbe(DriverPtr drv, int flags)
{
    DBLOG(1, "Probe\n");
    if (flags & PROBE_DETECT)
        return TRUE;

    xf86XVRegisterGenericAdaptorDriver(Z4lInit);
    drv->refCount++;

    return TRUE;
}

static const OptionInfoRec *
Z4lAvailableOptions(int chipid, int busid)
{
    return NULL;
}

static void
Z4lIdentify(int flags)
{
    xf86Msg(X_INFO, "z4l driver for Video4Linux\n");
}

_X_EXPORT DriverRec Z4l = {
    40001,
    "z4l",
    Z4lIdentify,
    Z4lProbe,
    Z4lAvailableOptions,
    NULL,
    0
};

#ifdef XFree86LOADER

static MODULESETUPPROTO(z4lSetup);

static XF86ModuleVersionInfo z4lVersionRec = {
    "ztv",
    MODULEVENDORSTRING, MODINFOSTRING1, MODINFOSTRING2,
    XORG_VERSION_CURRENT, 0, 0, 1,
    ABI_CLASS_VIDEODRV, ABI_VIDEODRV_VERSION, MOD_CLASS_NONE,
    {0, 0, 0, 0}
};

_X_EXPORT XF86ModuleData ztvModuleData = { &z4lVersionRec, z4lSetup, NULL };

static pointer
z4lSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;

    if (setupDone != FALSE) {
        if (errmaj != NULL)
            *errmaj = LDR_ONCEONLY;
        return NULL;
    }

    setupDone = TRUE;
    xf86AddDriver(&Z4l, module, 0);
    return (pointer) 1;
}

#endif
#endif                          /* !XvExtension */
