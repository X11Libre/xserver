/*
 * Copyright (c) 2007 Advanced Micro Devices, Inc.
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

/* We want to share as much code between GX and LX as we possibly can for obvious reasons */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>             /* memcmp() */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "xf86.h"
#include "geode.h"

#define move0(d,s,n) \
  __asm__ __volatile__( \
  "   rep; movsl\n" \
  : "=&c" (d0), "=&S" (d1), "=&D" (d2) \
  : "0" (n), "1" (s), "2" (d) \
  : "memory")

#define move1(d,s,n) \
  __asm__ __volatile__( \
  "   rep; movsl\n" \
  "   movsb\n" \
  : "=&c" (d0), "=&S" (d1), "=&D" (d2) \
  : "0" (n), "1" (s), "2" (d) \
  : "memory")

#define move2(d,s,n) \
  __asm__ __volatile__( \
  "   rep; movsl\n" \
  "   movsw\n" \
  : "=&c" (d0), "=&S" (d1), "=&D" (d2) \
  : "0" (n), "1" (s), "2" (d) \
  : "memory")

#define move3(d,s,n) \
  __asm__ __volatile__( \
  "   rep; movsl\n" \
  "   movsw\n" \
  "   movsb\n" \
  : "=&c" (d0), "=&S" (d1), "=&D" (d2) \
  : "0" (n), "1" (s), "2" (d) \
  : "memory")

void
geode_memory_to_screen_blt(unsigned long src, unsigned long dst,
                           unsigned long sp, unsigned long dp, long w, long h,
                           int bpp)
{
    int d0, d1, d2;
    int n = w * (bpp >> 3);
    int m = n >> 2;

    switch (n & 3) {
    case 0:
        while (--h >= 0) {
            move0(dst, src, m);
            src += sp;
            dst += dp;
        }
        break;
    case 1:
        while (--h >= 0) {
            move1(dst, src, m);
            src += sp;
            dst += dp;
        }
        break;
    case 2:
        while (--h >= 0) {
            move2(dst, src, m);
            src += sp;
            dst += dp;
        }
        break;
    case 3:
        while (--h >= 0) {
            move3(dst, src, m);
            src += sp;
            dst += dp;
        }
        break;
    }
}

/* I borrowed this function from the i830 driver - its much better
   then what we had before
*/

int
GeodeGetRefreshRate(DisplayModePtr pMode)
{
    if (pMode->VRefresh)
        return (int) (pMode->VRefresh + 0.5);

    return (int) (pMode->Clock * 1000.0 / pMode->HTotal / pMode->VTotal + 0.5);
}

/* This is used by both GX and LX.  It could be accelerated for LX, probably, but
   that would involve a two pass blt, the first to copy the data, and the second
   to copy the grey (using a pattern).  That seems like a bit of work for a
   very underused format - so we'll just use the slow version.
*/

void
GeodeCopyGreyscale(unsigned char *src, unsigned char *dst,
                   int dstPitch, int srcPitch, int h, int w)
{
    int i;
    unsigned char *src2 = src;
    unsigned char *dst2 = dst;
    unsigned char *dst3;
    unsigned char *src3;

    dstPitch <<= 1;

    while (h--) {
        dst3 = dst2;
        src3 = src2;
        for (i = 0; i < w; i++) {
            *dst3++ = *src3++;  /* Copy Y data */
            *dst3++ = 0x80;     /* Fill UV with 0x80 - greyscale */
        }

        src3 = src2;
        for (i = 0; i < w; i++) {
            *dst3++ = *src3++;  /* Copy Y data */
            *dst3++ = 0x80;     /* Fill UV with 0x80 - greyscale */
        }

        dst2 += dstPitch;
        src2 += srcPitch;
    }
}

#if defined(linux)

#include <linux/fb.h>

int
GeodeGetSizeFromFB(unsigned int *size)
{
    struct fb_fix_screeninfo fix;
    int ret;
    int fd = open("/dev/fb0", O_RDONLY);

    if (fd == -1)
        return -1;

    ret = ioctl(fd, FBIOGET_FSCREENINFO, &fix);
    close(fd);

    if (!ret) {
        if (!memcmp(fix.id, "Geode", 5)) {
            *size = fix.smem_len;
            return 0;
        }
    }

    return -1;
}

#else

int
GeodeGetSizeFromFB(unsigned int *size)
{
    return -1;
}

#endif
