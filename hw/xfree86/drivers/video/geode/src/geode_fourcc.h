/* Xfree video macro definitions.
 *
 * Copyright (c) 2003-2005 Advanced Micro Devices, Inc.
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

#ifndef AMD_FOURCC_H_
#define AMD_FOURCC_H_
#define FOURCC_Y2YU 0x55593259
#define XVIMAGE_Y2YU \
   { \
	FOURCC_Y2YU, \
        XvYUV, \
	LSBFirst, \
	{'Y','2','Y','U', \
	  0x00,0x00,0x00,0x10,0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71}, \
	16, \
	XvPacked, \
	1, \
	0, 0, 0, 0, \
	8, 8, 8, \
	1, 2, 2, \
	1, 1, 1, \
	{'Y','V','Y','U', \
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, \
	XvTopToBottom \
   }

#define FOURCC_YVYU 0x55595659
#define XVIMAGE_YVYU \
   { \
	FOURCC_YVYU, \
        XvYUV, \
	LSBFirst, \
	{'Y','V','Y','U', \
	  0x00,0x00,0x00,0x10,0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71}, \
	16, \
	XvPacked, \
	1, \
	0, 0, 0, 0, \
	8, 8, 8, \
	1, 2, 2, \
	1, 1, 1, \
	{'Y','V','Y','U', \
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, \
	XvTopToBottom \
   }

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

/* Borrowed from Trident */

#ifndef FOURCC_RGB565
#define FOURCC_RGB565 0x36315652
#endif
#ifndef XVIMAGE_RGB565
#define XVIMAGE_RGB565 \
   { \
        FOURCC_RGB565, \
        XvRGB,\
        LSBFirst,\
       {'R','V','1','6',\
       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},\
       16,\
       XvPacked,\
       1,\
       16, 0xF800, 0x07E0, 0x001F,\
       0, 0, 0,\
       0, 0, 0,\
       0, 0, 0,\
       {'R','V','B',0,\
       0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},\
       XvTopToBottom\
    }
#endif
#endif
