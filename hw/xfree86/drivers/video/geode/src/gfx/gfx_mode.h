/* Copyright (c) 2005 Advanced Micro Devices, Inc.
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
 * */

/* 
 * This header file contains the mode tables.  It is used by the "gfx_disp.c" 
 * file to set a display mode.
 * */

#ifndef _gfx_mode_h
#define _gfx_mode_h

/* MODE FLAGS (BITWISE-OR) */

#define GFX_MODE_8BPP		  0x00000001
#define GFX_MODE_12BPP        0x00000002
#define GFX_MODE_15BPP        0x00000004
#define GFX_MODE_16BPP		  0x00000008
#define GFX_MODE_24BPP        0x00000010
#define GFX_MODE_56HZ         0x00000020
#define GFX_MODE_60HZ		  0x00000040
#define GFX_MODE_70HZ		  0x00000080
#define GFX_MODE_72HZ		  0x00000100
#define GFX_MODE_75HZ		  0x00000200
#define GFX_MODE_85HZ		  0x00000400
#define GFX_MODE_90HZ         0x00000800
#define GFX_MODE_100HZ        0x00001000
#define GFX_MODE_NEG_HSYNC	  0x00002000
#define GFX_MODE_NEG_VSYNC	  0x00004000
#define GFX_MODE_PIXEL_DOUBLE 0x00008000
#define GFX_MODE_LINE_DOUBLE  0x00010000
#define GFX_MODE_TV_NTSC      0x00020000
#define GFX_MODE_TV_PAL       0x00040000
#define GFX_MODE_EXCLUDE_PLL  0x00080000
#define GFX_MODE_LOCK_TIMING  0x10000000

#define gfx_mode_hz_conversion		\
	switch (hz) {					\
	case 56:						\
		hz_flag = GFX_MODE_56HZ;	\
		break;						\
	case 60:						\
       	hz_flag = GFX_MODE_60HZ;	\
		break;						\
	case 70:						\
       	hz_flag = GFX_MODE_70HZ;	\
		break;						\
	case 72:						\
       	hz_flag = GFX_MODE_72HZ;	\
		break;						\
	case 75:						\
     	hz_flag = GFX_MODE_75HZ;	\
		break;						\
	case 85:						\
      	hz_flag = GFX_MODE_85HZ;	\
		break;						\
	case 90:						\
      	hz_flag = GFX_MODE_90HZ;	\
		break;						\
	case 100:						\
      	hz_flag = GFX_MODE_100HZ;	\
		break;						\
	}

#define gfx_mode_bpp_conversion			\
	switch (bpp) {						\
    case 8:								\
        bpp_flag = GFX_MODE_8BPP;		\
		break;							\
    case 12:							\
		bpp_flag = GFX_MODE_12BPP;		\
		break;							\
    case 15:							\
		bpp_flag = GFX_MODE_15BPP;		\
		break;							\
    case 16:							\
		bpp_flag = GFX_MODE_16BPP;		\
		break;							\
    case 32:							\
		bpp_flag = GFX_MODE_24BPP;		\
		break;							\
    default:							\
        return -1;						\
    }

#define gfx_mode_bpp_conversion_def(bpp) \
	switch (bpp) {						\
    case 8:								\
        bpp_flag = GFX_MODE_8BPP;		\
		break;							\
    case 12:							\
		bpp_flag = GFX_MODE_12BPP;		\
		break;							\
    case 15:							\
		bpp_flag = GFX_MODE_15BPP;		\
		break;							\
    case 16:							\
		bpp_flag = GFX_MODE_16BPP;		\
		break;							\
    case 32:							\
		bpp_flag = GFX_MODE_24BPP;		\
		break;							\
    default:							\
        bpp_flag = GFX_MODE_8BPP;		\
    }

/* STRUCTURE DEFINITION */

typedef struct tagDISPLAYMODE {
    /* DISPLAY MODE FLAGS */
    /* Specify valid color depths and the refresh rate. */

    unsigned long flags;

    /* TIMINGS */

    unsigned short hactive;
    unsigned short hblankstart;
    unsigned short hsyncstart;
    unsigned short hsyncend;
    unsigned short hblankend;
    unsigned short htotal;

    unsigned short vactive;
    unsigned short vblankstart;
    unsigned short vsyncstart;
    unsigned short vsyncend;
    unsigned short vblankend;
    unsigned short vtotal;

    /* CLOCK FREQUENCY */

    unsigned long frequency;

} DISPLAYMODE;

/* For Fixed timings */
typedef struct tagFIXEDTIMINGS {
    /* DISPLAY MODE FLAGS */
    /* Specify valid color depths and the refresh rate. */

    int panelresx;
    int panelresy;
    unsigned short xres;
    unsigned short yres;

    /* TIMINGS */

    unsigned short hactive;
    unsigned short hblankstart;
    unsigned short hsyncstart;
    unsigned short hsyncend;
    unsigned short hblankend;
    unsigned short htotal;

    unsigned short vactive;
    unsigned short vblankstart;
    unsigned short vsyncstart;
    unsigned short vsyncend;
    unsigned short vblankend;
    unsigned short vtotal;

    /* CLOCK FREQUENCY */

    unsigned long frequency;

} FIXEDTIMINGS;

#endif                          /* !_gfx_mode_h */

/* END OF FILE */
