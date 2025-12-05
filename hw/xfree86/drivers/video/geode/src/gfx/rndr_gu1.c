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
 * This file contains routines to program the 2D acceleration hardware for
 * the first generation graphics unit (GXLV, SC1200).
 * 
 *    gfx_set_bpp            
 *    gfx_set_solid_pattern  
 *    gfx_set_mono_pattern
 *    gfx_set_color_pattern
 *    gfx_set_solid_source  
 *    gfx_set_mono_source
 *    gfx_set_raster_operation
 *    gfx_pattern_fill
 *    gfx_screen_to_screen_blt
 *    gfx_screen_to_screen_xblt
 *    gfx_color_bitmap_to_screen_blt
 *    gfx_color_bitmap_to_screen_xblt
 *    gfx_mono_bitmap_to_screen_blt
 *    gfx_bresenham_line 
 *    gfx_wait_until_idle   
 * */

#if GFX_NO_IO_IN_WAIT_MACROS
#define GFX_WAIT_PENDING		\
		while(READ_REG16(GP_BLIT_STATUS) & BS_BLIT_PENDING) { ; }
#define GFX_WAIT_BUSY 			\
		while(READ_REG16(GP_BLIT_STATUS) & BS_BLIT_BUSY) { ; }
#define GFX_WAIT_PIPELINE 		\
		while (READ_REG16(GP_BLIT_STATUS) & BS_PIPELINE_BUSY) { ; }
#else
#define GFX_WAIT_PENDING 		\
		while(READ_REG16(GP_BLIT_STATUS) & BS_BLIT_PENDING) { INB (0x80); }
#define GFX_WAIT_BUSY 			\
		while(READ_REG16(GP_BLIT_STATUS) & BS_BLIT_BUSY) { INB (0x80); }
#define GFX_WAIT_PIPELINE 		\
		while (READ_REG16(GP_BLIT_STATUS) & BS_PIPELINE_BUSY) { INB (0x80); }
#endif

void gu1_detect_blt_buffer_base(void);

/*---------------------------------------------------------------------------
 * GFX_SET_BPP
 *
 * This routine sets the bits per pixel value in the graphics engine.
 * It is also stored in a static variable to use in the future calls to 
 * the rendering routines.
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu1_set_bpp(unsigned short bpp)
#else
void
gfx_set_bpp(unsigned short bpp)
#endif
{
    int control = 0;
    unsigned short pitch = gfx_get_display_pitch();

    GFXbpp = bpp;

    /* DETECT BASE ADDRESSES FOR BLT BUFFERS */
    /* Different for 2K or 3K of scratchpad.  Also need to calculate */
    /* the number of pixels that can fit in a BLT buffer - need to */
    /* subtract 16 for alignment considerations.  The 2K case, for */
    /* example, is 816 bytes wide, allowing 800 pixels in 8 BPP, which */
    /* means rendering operations won't be split for 800x600. */

    gu1_detect_blt_buffer_base();
    GFXbufferWidthPixels = GFXbb1Base - GFXbb0Base - 16;
    if (bpp > 8) {
        /* If 16bpp, divide GFXbufferWidthPixels by 2 */
        GFXbufferWidthPixels >>= 1;
    }

    /* SET THE GRAPHICS CONTROLLER BPP AND PITCH */
    if (bpp > 8) {
        /* Set the 16bpp bit if necessary */
        control = BC_16BPP;
    }
    if ((gfx_cpu_version == GFX_CPU_PYRAMID) && (pitch > 2048)) {
        control |= BC_FB_WIDTH_4096;
    }
    else if (pitch > 1024) {
        control |= BC_FB_WIDTH_2048;
    }
    GFX_WAIT_BUSY;
    WRITE_REG32(GP_BLIT_STATUS, control);
}

/*
 *---------------------------------------------------------------------------
 * GFX_SET_SOLID_SOURCE
 *
 * This routine is used to specify a solid source color.  For the Xfree96
 * display driver, the source color is used to specify a planemask and the 
 * ROP is adjusted accordingly.
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu1_set_solid_source(unsigned long color)
#else
void
gfx_set_solid_source(unsigned long color)
#endif
{
    /* CLEAR TRANSPARENCY FLAG */

    GFXsourceFlags = 0;

    /* FORMAT 8 BPP COLOR */
    /* GX requires 8BPP color data be duplicated into bits [15:8]. */

    if (GFXbpp == 8) {
        color &= 0x00FF;
        color |= (color << 8);
    }

    /* POLL UNTIL ABLE TO WRITE THE SOURCE COLOR */

    GFX_WAIT_PENDING;
    WRITE_REG16(GP_SRC_COLOR_0, (unsigned short) color);
    WRITE_REG16(GP_SRC_COLOR_1, (unsigned short) color);
}

/*
 *---------------------------------------------------------------------------
 * GFX_SET_MONO_SOURCE
 *
 * This routine is used to specify the monochrome source colors.  
 * It must be called *after* loading any pattern data (those routines 
 * clear the source flags).
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu1_set_mono_source(unsigned long bgcolor, unsigned long fgcolor,
                    unsigned short transparent)
#else
void
gfx_set_mono_source(unsigned long bgcolor, unsigned long fgcolor,
                    unsigned short transparent)
#endif
{
    /* SET TRANSPARENCY FLAG */

    GFXsourceFlags = transparent ? RM_SRC_TRANSPARENT : 0;

    /* FORMAT 8 BPP COLOR */
    /* GX requires 8BPP color data be duplicated into bits [15:8]. */

    if (GFXbpp == 8) {
        bgcolor &= 0x00FF;
        bgcolor |= (bgcolor << 8);
        fgcolor &= 0x00FF;
        fgcolor |= (fgcolor << 8);
    }

    /* POLL UNTIL ABLE TO WRITE THE SOURCE COLOR */

    GFX_WAIT_PENDING;
    WRITE_REG16(GP_SRC_COLOR_0, (unsigned short) bgcolor);
    WRITE_REG16(GP_SRC_COLOR_1, (unsigned short) fgcolor);
}

/*
 *---------------------------------------------------------------------------
 * GFX_SET_SOLID_PATTERN
 *
 * This routine is used to specify a solid pattern color.  It is called 
 * before performing solid rectangle fills or more complicated BLTs that 
 * use a solid pattern color. 
 *
 * The driver should always call "gfx_load_raster_operation" after a call 
 * to this routine to make sure that the pattern flags are set appropriately.
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu1_set_solid_pattern(unsigned long color)
#else
void
gfx_set_solid_pattern(unsigned long color)
#endif
{
    /* CLEAR TRANSPARENCY FLAG */

    GFXsourceFlags = 0;

    /* SET PATTERN FLAGS */

    GFXpatternFlags = 0;

    /* FORMAT 8 BPP COLOR */
    /* GX requires 8BPP color data be duplicated into bits [15:8]. */

    if (GFXbpp == 8) {
        color &= 0x00FF;
        color |= (color << 8);
    }

    /* SAVE THE REFORMATTED COLOR FOR LATER */
    /* Used to call the "GFX_solid_fill" routine for special cases. */

    GFXsavedColor = color;

    /* POLL UNTIL ABLE TO WRITE THE PATTERN COLOR */

    GFX_WAIT_PENDING;
    WRITE_REG16(GP_PAT_COLOR_0, (unsigned short) color);
}

/*
 *---------------------------------------------------------------------------
 * GFX_SET_MONO_PATTERN
 *
 * This routine is used to specify a monochrome pattern. 
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu1_set_mono_pattern(unsigned long bgcolor, unsigned long fgcolor,
                     unsigned long data0, unsigned long data1,
                     unsigned char transparent)
#else
void
gfx_set_mono_pattern(unsigned long bgcolor, unsigned long fgcolor,
                     unsigned long data0, unsigned long data1,
                     unsigned char transparent)
#endif
{
    /* CLEAR TRANSPARENCY FLAG */

    GFXsourceFlags = 0;

    /* SET PATTERN FLAGS */

    GFXpatternFlags = transparent ? RM_PAT_MONO | RM_PAT_TRANSPARENT :
        RM_PAT_MONO;

    /* FORMAT 8 BPP COLOR */
    /* GXm requires 8BPP color data be duplicated into bits [15:8]. */

    if (GFXbpp == 8) {
        bgcolor &= 0x00FF;
        bgcolor |= (bgcolor << 8);
        fgcolor &= 0x00FF;
        fgcolor |= (fgcolor << 8);
    }

    /* POLL UNTIL ABLE TO WRITE THE PATTERN COLORS AND DATA */

    GFX_WAIT_PENDING;
    WRITE_REG16(GP_PAT_COLOR_0, (unsigned short) bgcolor);
    WRITE_REG16(GP_PAT_COLOR_1, (unsigned short) fgcolor);
    WRITE_REG32(GP_PAT_DATA_0, data0);
    WRITE_REG32(GP_PAT_DATA_1, data1);
}

/*
 *---------------------------------------------------------------------------
 * GFX_SET_COLOR_PATTERN
 *
 * This routine is used to specify a color pattern. 
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu1_set_color_pattern(unsigned long bgcolor, unsigned long fgcolor,
                      unsigned long data0, unsigned long data1,
                      unsigned long data2, unsigned long data3,
                      unsigned char transparent)
#else
void
gfx_set_color_pattern(unsigned long bgcolor, unsigned long fgcolor,
                      unsigned long data0, unsigned long data1,
                      unsigned long data2, unsigned long data3,
                      unsigned char transparent)
#endif
{
    /* CLEAR TRANSPARENCY FLAG */

    GFXsourceFlags = 0;

    /* SET PATTERN FLAGS */

    GFXpatternFlags = transparent ? RM_PAT_MONO | RM_PAT_TRANSPARENT :
        RM_PAT_MONO;

    GFXpatternFlags |= RM_PAT_COLOR;
    /* FORMAT 8 BPP COLOR */
    /* GXm requires 8BPP color data be duplicated into bits [15:8]. */

    if (GFXbpp == 8) {
        bgcolor &= 0x00FF;
        bgcolor |= (bgcolor << 8);
        fgcolor &= 0x00FF;
        fgcolor |= (fgcolor << 8);
    }

    /* POLL UNTIL ABLE TO WRITE THE PATTERN COLORS AND DATA */

    GFX_WAIT_PENDING;
    WRITE_REG16(GP_PAT_COLOR_0, (unsigned short) bgcolor);
    WRITE_REG16(GP_PAT_COLOR_1, (unsigned short) fgcolor);
    WRITE_REG32(GP_PAT_DATA_0, data0);
    WRITE_REG32(GP_PAT_DATA_1, data1);
    if (GFXbpp > 8) {
        WRITE_REG32(GP_PAT_DATA_2, data2);
        WRITE_REG32(GP_PAT_DATA_3, data3);
    }
}

/*
 *---------------------------------------------------------------------------
 * GFX_LOAD_COLOR_PATTERN_LINE
 *
 * This routine is used to load a single line of a 8x8 color pattern.   
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu1_load_color_pattern_line(short y, unsigned long *pattern_8x8)
#else
void
gfx_load_color_pattern_line(short y, unsigned long *pattern_8x8)
#endif
{
    /* CLEAR TRANSPARENCY FLAG */

    GFXsourceFlags = 0;

    /* SET PATTERN FLAGS */

    GFXpatternFlags = RM_PAT_COLOR;

    y &= 7;

    if (GFXbpp > 8)
        pattern_8x8 += (y << 2);
    else
        pattern_8x8 += (y << 1);

    /* POLL UNTIL ABLE TO WRITE THE PATTERN COLORS AND DATA */

    GFX_WAIT_PENDING;
    WRITE_REG32(GP_PAT_DATA_0, pattern_8x8[0]);
    WRITE_REG32(GP_PAT_DATA_1, pattern_8x8[1]);
    if (GFXbpp > 8) {
        WRITE_REG32(GP_PAT_DATA_2, pattern_8x8[2]);
        WRITE_REG32(GP_PAT_DATA_3, pattern_8x8[3]);
    }
}

/*
 *---------------------------------------------------------------------------
 * GFX_SET_RASTER_OPERATION
 *
 * This routine loads the specified raster operation.  It sets the pattern
 * flags appropriately.
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu1_set_raster_operation(unsigned char rop)
#else
void
gfx_set_raster_operation(unsigned char rop)
#endif
{
    unsigned short rop16;

    /* GENERATE 16-BIT VERSION OF ROP WITH PATTERN FLAGS */

    rop16 = (unsigned short) rop | GFXpatternFlags;
    if ((rop & 0x33) ^ ((rop >> 2) & 0x33))
        rop16 |= GFXsourceFlags;

    /* SAVE ROP FOR LATER COMPARISONS */
    /* Need to have the pattern flags included */

    GFXsavedRop = rop16;

    /* SET FLAG INDICATING ROP REQUIRES DESTINATION DATA */
    /* True if even bits (0:2:4:6) do not equal the corresponding */
    /* even bits (1:3:5:7). */

    GFXusesDstData = ((rop & 0x55) ^ ((rop >> 1) & 0x55));

    /* POLL UNTIL ABLE TO WRITE THE PATTERN COLOR */
    /* Only one operation can be pending at a time. */

    GFX_WAIT_PENDING;
    WRITE_REG16(GP_RASTER_MODE, rop16);
}

/*
 *---------------------------------------------------------------------------
 * GFX_SOLID_FILL
 *
 * This routine MUST be used when performing a solid rectangle fill with 
 * the ROPs of PATCOPY (0xF0), BLACKNESS (0x00), WHITENESS (0xFF), or 
 * PATINVERT (0x0F).  There is a bug in GXm for these cases that requires a 
 * workaround.  
 *
 * For BLACKNESS (ROP = 0x00), set the color to 0x0000.  
 * For WHITENESS (ROP = 0xFF), set the color to 0xFFFF.
 * For PATINVERT (ROP = 0x0F), invert the desired color.
 *
 *     X               screen X position (left)
 *     Y               screen Y position (top)
 *     WIDTH           width of rectangle, in pixels
 *     HEIGHT          height of rectangle, in scanlines
 *     COLOR           fill color
 *
 * THIS ROUTINE SHOULD NOT BE DIRECTLY CALLED FROM THE DRIVER.  The driver 
 * should always use GFX_pattern_fill and let that routine call this one
 * when approipriate.  This is to hide quirks specific to MediaGX hardware.
 *---------------------------------------------------------------------------
 */
void
gu1_solid_fill(unsigned short x, unsigned short y,
               unsigned short width, unsigned short height, unsigned long color)
{
    unsigned short section;

    /* POLL UNTIL ABLE TO WRITE TO THE REGISTERS */
    /* Only one operation can be pending at a time. */

    GFX_WAIT_PENDING;

    /* SET REGISTERS TO DRAW RECTANGLE */

    WRITE_REG16(GP_DST_XCOOR, x);
    WRITE_REG16(GP_DST_YCOOR, y);
    WRITE_REG16(GP_HEIGHT, height);
    WRITE_REG16(GP_RASTER_MODE, 0x00F0);        /* PATCOPY */
    WRITE_REG16(GP_PAT_COLOR_0, (unsigned short) color);

    /* CHECK WIDTH FOR GX BUG WORKAROUND */

    if (width <= 16) {
        /* OK TO DRAW SMALL RECTANGLE IN ONE PASS */

        WRITE_REG16(GP_WIDTH, width);
        WRITE_REG16(GP_BLIT_MODE, 0);
    }
    else {
        /* DRAW FIRST PART OF RECTANGLE */
        /* Get to a 16 pixel boundary. */

        section = 0x10 - (x & 0x0F);
        WRITE_REG16(GP_WIDTH, section);
        WRITE_REG16(GP_BLIT_MODE, 0);

        /* POLL UNTIL ABLE TO LOAD THE SECOND RECTANGLE */

        GFX_WAIT_PENDING;
        WRITE_REG16(GP_DST_XCOOR, x + section);
        WRITE_REG16(GP_DST_YCOOR, y);
        WRITE_REG16(GP_WIDTH, width - section);
        WRITE_REG16(GP_BLIT_MODE, 0);
    }
}

/*
 *----------------------------------------------------------------------------
 * GFX_PATTERN_FILL
 *
 * This routine is used to fill a rectangular region.  The pattern must 
 * be previously loaded using one of GFX_load_*_pattern routines.  Also, the 
 * raster operation must be previously specified using the 
 * "GFX_load_raster_operation" routine.
 *
 *      X               screen X position (left)
 *      Y               screen Y position (top)
 *      WIDTH           width of rectangle, in pixels
 *      HEIGHT          height of rectangle, in scanlines
 *----------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu1_pattern_fill(unsigned short x, unsigned short y,
                 unsigned short width, unsigned short height)
#else
void
gfx_pattern_fill(unsigned short x, unsigned short y,
                 unsigned short width, unsigned short height)
#endif
{
    unsigned short section, buffer_width, blit_mode;

    /* CHECK IF OPTIMIZED SOLID CASES */
    /* Check all 16 bits of the ROP to include solid pattern flags. */

    switch (GFXsavedRop) {
        /* CHECK FOR SPECIAL CASES WITHOUT DESTINATION DATA */
        /* Need hardware workaround for fast "burst write" cases. */

    case 0x00F0:
        gu1_solid_fill(x, y, width, height, (unsigned short) GFXsavedColor);
        break;
    case 0x000F:
        gu1_solid_fill(x, y, width, height, (unsigned short) ~GFXsavedColor);
        break;
    case 0x0000:
        gu1_solid_fill(x, y, width, height, 0x0000);
        break;
    case 0x00FF:
        gu1_solid_fill(x, y, width, height, 0xFFFF);
        break;

        /* REMAINING CASES REQUIRE DESTINATION DATA OR NOT SOLID COLOR */

    default:

        /* DETERMINE BLT MODE VALUE */
        /* Still here for non-solid patterns without destination data. */

        blit_mode = GFXusesDstData ? BM_READ_DST_FB0 : 0;

        /* SET SOURCE EXPANSION MODE */
        /* If the ROP requires source data, then the source data is all 1's */
        /* and then expanded into the desired color in GP_SRC_COLOR_1. */

        blit_mode |= BM_SOURCE_EXPAND;

        /* POLL UNTIL ABLE TO WRITE TO THE REGISTERS */
        /* Write the registers that do not change for each section. */

        GFX_WAIT_PENDING;
        WRITE_REG16(GP_HEIGHT, height);

        /* SINCE ONLY DESTINATION DATA, WE CAN USE BOTH BB0 AND BB1. */
        /* Therefore, width available = BLT buffer width * 2. */

        buffer_width = GFXbufferWidthPixels << 1;

        /* REPEAT UNTIL FINISHED WITH RECTANGLE */
        /* Perform BLT in vertical sections, as wide as the BLT buffer */
        /* allows.  Hardware does not split the operations, so */
        /* software must do it to avoid large scanlines that would */
        /* overflow the BLT buffers. */

        while (width > 0) {
            /* DETERMINE WIDTH OF SECTION */

            if (width > buffer_width)
                section = buffer_width;
            else
                section = width;

            /* POLL UNTIL ABLE TO WRITE TO THE REGISTERS */

            GFX_WAIT_PENDING;
            WRITE_REG16(GP_DST_XCOOR, x);
            WRITE_REG16(GP_DST_YCOOR, y);
            WRITE_REG16(GP_WIDTH, section);
            WRITE_REG16(GP_BLIT_MODE, blit_mode);

            /* ADJUST PARAMETERS FOR NEXT SECTION */

            width -= section;
            x += section;
        }
        break;
    }
}

/*
 *----------------------------------------------------------------------------
 * GFX_COLOR_PATTERN_FILL
 *
 * This routine is used to render a rectangle using the current raster 
 * operation and the specified color pattern.  It allows an 8x8 color 
 * pattern to be rendered without multiple calls to the gfx_set_color_pattern
 * and gfx_pattern_fill routines.
 *
 *      X               screen X position (left)
 *      Y               screen Y position (top)
 *      WIDTH           width of rectangle, in pixels
 *      HEIGHT          height of rectangle, in scanlines
 *      *PATTERN		pointer to 8x8 color pattern data
 *----------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu1_color_pattern_fill(unsigned short x, unsigned short y,
                       unsigned short width, unsigned short height,
                       unsigned long *pattern)
#else
void
gfx_color_pattern_fill(unsigned short x, unsigned short y,
                       unsigned short width, unsigned short height,
                       unsigned long *pattern)
#endif
{
    unsigned short blit_mode, passes, cur_y, pat_y, i;
    unsigned short buffer_width, line_width;
    unsigned short bpp_shift, section, cur_x;

    /* SET APPROPRIATE INCREMENT */

    bpp_shift = (GFXbpp > 8) ? 2 : 1;

    /* SET DESTINATION REQUIRED */

    blit_mode = GFXusesDstData ? BM_READ_DST_FB0 : 0;

    /* SET SOURCE EXPANSION */

    blit_mode |= BM_SOURCE_EXPAND;

    /* OVERRIDE RASTER MODE TO FORCE A COLOR PATTERN */

    GFX_WAIT_PENDING;
    WRITE_REG16(GP_RASTER_MODE,
                (GFXsavedRop & ~RM_PAT_MASK & ~RM_PAT_TRANSPARENT) |
                RM_PAT_COLOR);

    /* WRITE THE REGISTERS THAT DO NOT CHANGE         */
    /* If destination data is required, the width and */
    /* x position will be overwritten.                */

    WRITE_REG16(GP_HEIGHT, 1);
    WRITE_REG16(GP_WIDTH, width);
    WRITE_REG16(GP_DST_XCOOR, x);

    /* THE ENTIRE PATTERN WILL NOT BE DRAWN IF THE HEIGHT IS LESS THAN 8 */

    passes = (height < 8) ? height : 8;

    /* SINCE ONLY DESTINATION DATA, WE CAN USE BOTH BB0 AND BB1. */
    /* Therefore, width available = BLT buffer width * 2. */

    buffer_width = GFXbufferWidthPixels << 1;

    for (i = 0; i < passes; i++) {
        pat_y = ((y + i) & 7) << bpp_shift;
        cur_y = y + i;

        /* WRITE THE PATTERN DATA FOR THE ACTIVE LINE */

        GFX_WAIT_PENDING;
        WRITE_REG32(GP_PAT_DATA_0, pattern[pat_y]);
        WRITE_REG32(GP_PAT_DATA_1, pattern[pat_y + 1]);

        if (GFXbpp > 8) {
            WRITE_REG32(GP_PAT_DATA_2, pattern[pat_y + 2]);
            WRITE_REG32(GP_PAT_DATA_3, pattern[pat_y + 3]);
        }

        /* SPLIT BLT LINE INTO SECTIONS IF REQUIRED              */
        /* If no destination data is required, we can ignore     */
        /* the BLT buffers.  Otherwise, we must separate the BLT */
        /* so as not to overflow the buffers                     */

        if (blit_mode & BM_READ_DST_BB0) {
            line_width = width;
            cur_x = x;

            while (line_width) {
                section =
                    (line_width > buffer_width) ? buffer_width : line_width;
                cur_y = y + i;

                GFX_WAIT_PENDING;
                WRITE_REG16(GP_DST_XCOOR, cur_x);
                WRITE_REG16(GP_WIDTH, section);

                while (cur_y < y + height) {
                    GFX_WAIT_PENDING;
                    WRITE_REG16(GP_DST_YCOOR, cur_y);
                    WRITE_REG16(GP_BLIT_MODE, blit_mode);
                    cur_y += 8;
                }

                cur_x += section;
                line_width -= section;
            }

        }
        else {
            while (cur_y < y + height) {
                GFX_WAIT_PENDING;
                WRITE_REG16(GP_DST_YCOOR, cur_y);
                WRITE_REG16(GP_BLIT_MODE, blit_mode);
                cur_y += 8;
            }
        }

    }

    /* RESTORE ORIGINAL ROP AND FLAGS */

    GFX_WAIT_PENDING;
    WRITE_REG16(GP_RASTER_MODE, GFXsavedRop);

}

/*
 *----------------------------------------------------------------------------
 * SCREEN TO SCREEN BLT
 *
 * This routine should be used to perform a screen to screen BLT when the 
 * ROP does not require destination data.
 *
 *      SRCX            screen X position to copy from
 *      SRCY            screen Y position to copy from
 *      DSTX            screen X position to copy to
 *      DSTY            screen Y position to copy to
 *      WIDTH           width of rectangle, in pixels
 *      HEIGHT          height of rectangle, in scanlines
 *----------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu1_screen_to_screen_blt(unsigned short srcx, unsigned short srcy,
                         unsigned short dstx, unsigned short dsty,
                         unsigned short width, unsigned short height)
#else
void
gfx_screen_to_screen_blt(unsigned short srcx, unsigned short srcy,
                         unsigned short dstx, unsigned short dsty,
                         unsigned short width, unsigned short height)
#endif
{
    unsigned short section, buffer_width;
    unsigned short blit_mode;

    /* CHECK IF RASTER OPERATION REQUIRES DESTINATION DATA */

    blit_mode = GFXusesDstData ? BM_READ_DST_FB1 | BM_READ_SRC_FB :
        BM_READ_SRC_FB;

    /* CHECK Y DIRECTION */
    /* Hardware has support for negative Y direction. */

    if (dsty > srcy) {
        blit_mode |= BM_REVERSE_Y;
        srcy += height - 1;
        dsty += height - 1;
    }

    /* CHECK X DIRECTION */
    /* Hardware does not support negative X direction since at the time */
    /* of development all supported resolutions could fit a scanline of */
    /* data at once into the BLT buffers (using both BB0 and BB1).  This */
    /* code is more generic to allow for any size BLT buffer. */

    if (dstx > srcx) {
        srcx += width;
        dstx += width;
    }

    /* POLL UNTIL ABLE TO WRITE TO THE REGISTERS */
    /* Write the registers that do not change for each section. */

    GFX_WAIT_PENDING;
    WRITE_REG16(GP_HEIGHT, height);

    /* CHECK AVAILABLE BLT BUFFER SIZE */
    /* Can use both BLT buffers if no destination data is required. */

    buffer_width = GFXusesDstData ? GFXbufferWidthPixels :
        GFXbufferWidthPixels << 1;

    /* REPEAT UNTIL FINISHED WITH RECTANGLE */
    /* Perform BLT in vertical sections, as wide as the BLT buffer allows. */
    /* Hardware does not split the operations, so software must do it to */
    /* avoid large scanlines that would overflow the BLT buffers. */

    while (width > 0) {
        /* CHECK WIDTH OF CURRENT SECTION */

        if (width > buffer_width)
            section = buffer_width;
        else
            section = width;

        /* PROGRAM REGISTERS THAT ARE THE SAME FOR EITHER X DIRECTION */

        GFX_WAIT_PENDING;
        WRITE_REG16(GP_SRC_YCOOR, srcy);
        WRITE_REG16(GP_DST_YCOOR, dsty);
        WRITE_REG16(GP_WIDTH, section);

        /* CHECK X DIRECTION */

        if (dstx > srcx) {
            /* NEGATIVE X DIRECTION */
            /* Still positive X direction within the section. */

            srcx -= section;
            dstx -= section;
            WRITE_REG16(GP_SRC_XCOOR, srcx);
            WRITE_REG16(GP_DST_XCOOR, dstx);
            WRITE_REG16(GP_BLIT_MODE, blit_mode);
        }
        else {
            /* POSITIVE X DIRECTION */

            WRITE_REG16(GP_SRC_XCOOR, srcx);
            WRITE_REG16(GP_DST_XCOOR, dstx);
            WRITE_REG16(GP_BLIT_MODE, blit_mode);
            dstx += section;
            srcx += section;
        }
        width -= section;
    }
}

/*
 *----------------------------------------------------------------------------
 * SCREEN TO SCREEN TRANSPARENT BLT
 *
 * This routine should be used to perform a screen to screen BLT when a 
 * specified color should by transparent.  The only supported ROP is SRCCOPY.
 *
 *      SRCX            screen X position to copy from
 *      SRCY            screen Y position to copy from
 *      DSTX            screen X position to copy to
 *      DSTY            screen Y position to copy to
 *      WIDTH           width of rectangle, in pixels
 *      HEIGHT          height of rectangle, in scanlines
 *      COLOR           transparent color
 *----------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu1_screen_to_screen_xblt(unsigned short srcx, unsigned short srcy,
                          unsigned short dstx, unsigned short dsty,
                          unsigned short width, unsigned short height,
                          unsigned long color)
#else
void
gfx_screen_to_screen_xblt(unsigned short srcx, unsigned short srcy,
                          unsigned short dstx, unsigned short dsty,
                          unsigned short width, unsigned short height,
                          unsigned long color)
#endif
{
    unsigned short section, buffer_width;
    unsigned short blit_mode = BM_READ_SRC_FB;

    /* CHECK Y DIRECTION */
    /* Hardware has support for negative Y direction. */

    if (dsty > srcy) {
        blit_mode |= BM_REVERSE_Y;
        srcy += height - 1;
        dsty += height - 1;
    }

    /* CHECK X DIRECTION */
    /* Hardware does not support negative X direction since at the time */
    /* of development all supported resolutions could fit a scanline of */
    /* data at once into the BLT buffers (using both BB0 and BB1).  This */
    /* code is more generic to allow for any size BLT buffer. */

    if (dstx > srcx) {
        srcx += width;
        dstx += width;
    }

    /* CALCULATE BLT BUFFER SIZE */
    /* Need to use BB1 to store the BLT buffer data. */

    buffer_width = GFXbufferWidthPixels;

    /* WRITE TRANSPARENCY COLOR TO BLT BUFFER 1 */

    if (GFXbpp == 8) {
        color &= 0x00FF;
        color |= (color << 8);
    }
    color = (color & 0x0000FFFF) | (color << 16);

    /* WAIT UNTIL PIPELINE IS NOT BUSY BEFORE LOADING DATA INTO BB1 */
    /* Need to make sure any previous BLT using BB1 is complete. */
    /* Only need to load 32 bits of BB1 for the 1 pixel BLT that follows. */

    GFX_WAIT_BUSY;
    WRITE_SCRATCH32(GFXbb1Base, color);

    /* DO BOGUS BLT TO LATCH DATA FROM BB1 */
    /* Already know graphics pipeline is idle. */
    /* Only need to latch data into the holding registers for the current */
    /* data from BB1.  A 1 pixel wide BLT will suffice. */

    WRITE_REG32(GP_DST_XCOOR, 0);
    WRITE_REG32(GP_SRC_XCOOR, 0);
    WRITE_REG32(GP_WIDTH, 0x00010001);
    WRITE_REG16(GP_RASTER_MODE, 0x00CC);
    WRITE_REG16(GP_BLIT_MODE, BM_READ_SRC_FB | BM_READ_DST_BB1);

    /* WRITE REGISTERS FOR REAL SCREEN TO SCREEN BLT */

    GFX_WAIT_PENDING;
    WRITE_REG16(GP_HEIGHT, height);
    WRITE_REG16(GP_RASTER_MODE, 0x10C6);
    WRITE_REG32(GP_PAT_COLOR_0, 0xFFFFFFFF);

    /* REPEAT UNTIL FINISHED WITH RECTANGLE */
    /* Perform BLT in vertical sections, as wide as the BLT buffer allows. */
    /* Hardware does not split the operations, so software must do it to */
    /* avoid large scanlines that would overflow the BLT buffers. */

    while (width > 0) {
        /* CHECK WIDTH OF CURRENT SECTION */

        if (width > buffer_width)
            section = buffer_width;
        else
            section = width;

        /* PROGRAM REGISTERS THAT ARE THE SAME FOR EITHER X DIRECTION */

        GFX_WAIT_PENDING;
        WRITE_REG16(GP_SRC_YCOOR, srcy);
        WRITE_REG16(GP_DST_YCOOR, dsty);
        WRITE_REG16(GP_WIDTH, section);

        /* CHECK X DIRECTION */
        /* Again, this must be done in software, and can be removed if the */
        /* display driver knows that the BLT buffers will always be large  */
        /* enough to contain an entire scanline of a screen to screen BLT. */

        if (dstx > srcx) {
            /* NEGATIVE X DIRECTION */
            /* Still positive X direction within the section. */

            srcx -= section;
            dstx -= section;
            WRITE_REG16(GP_SRC_XCOOR, srcx);
            WRITE_REG16(GP_DST_XCOOR, dstx);
            WRITE_REG16(GP_BLIT_MODE, blit_mode);
        }
        else {
            /* POSITIVE X DIRECTION */

            WRITE_REG16(GP_SRC_XCOOR, srcx);
            WRITE_REG16(GP_DST_XCOOR, dstx);
            WRITE_REG16(GP_BLIT_MODE, blit_mode);
            dstx += section;
            srcx += section;
        }
        width -= section;
    }
}

/*
 *----------------------------------------------------------------------------
 * COLOR BITMAP TO SCREEN BLT
 *
 * This routine transfers color bitmap data to the screen.  For most cases,
 * when the ROP is SRCCOPY, it may be faster to write a separate routine that
 * copies the data to the frame buffer directly.  This routine should be 
 * used when the ROP requires destination data.
 *
 * Transparency is handled by another routine.
 *
 *      SRCX            X offset within source bitmap
 *      SRCY            Y offset within source bitmap
 *      DSTX            screen X position to render data
 *      DSTY            screen Y position to render data
 *      WIDTH           width of rectangle, in pixels
 *      HEIGHT          height of rectangle, in scanlines
 *      *DATA           pointer to bitmap data
 *      PITCH           pitch of bitmap data (bytes between scanlines)
 *----------------------------------------------------------------------------
 */

#if GFX_2DACCEL_DYNAMIC
void
gu1_color_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
                               unsigned short dstx, unsigned short dsty,
                               unsigned short width, unsigned short height,
                               unsigned char *data, long pitch)
#else
void
gfx_color_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
                               unsigned short dstx, unsigned short dsty,
                               unsigned short width, unsigned short height,
                               unsigned char *data, long pitch)
#endif
{
    unsigned short section, buffer_width;
    unsigned short blit_mode = BM_READ_SRC_BB0;
    unsigned short temp_height;
    unsigned long dword_bytes_needed, bytes_extra;
    unsigned long bpp_shift;
    long array_offset;

    /* CHECK SIZE OF BLT BUFFER */

    buffer_width = GFXbufferWidthPixels;

    /* CHECK IF RASTER OPERATION REQUIRES DESTINATION DATA */
    /* If no destination data, we have twice the room for  */
    /* source data.                                        */

    if (GFXusesDstData)
        blit_mode |= BM_READ_DST_FB1;
    else
        buffer_width <<= 1;

    /* SET THE SCRATCHPAD BASE */

    SET_SCRATCH_BASE(GFXbb0Base);

    /* POLL UNTIL ABLE TO WRITE TO THE REGISTERS                */
    /* Write the registers that do not change for each section. */

    GFX_WAIT_PENDING;
    WRITE_REG16(GP_HEIGHT, 1);

    bpp_shift = (GFXbpp + 7) >> 4;

    while (width > 0) {
        if (width > buffer_width)
            section = buffer_width;
        else
            section = width;

        dword_bytes_needed = (section << bpp_shift) & ~3l;
        bytes_extra = (section << bpp_shift) & 3l;

        temp_height = height;

        /* WRITE THE REGISTERS FOR EACH SECTION                          */
        /* The GX hardware will auto-increment the Y coordinate, meaning */
        /* that we don't have to.                                        */

        WRITE_REG16(GP_WIDTH, section);
        WRITE_REG16(GP_DST_XCOOR, dstx);
        WRITE_REG16(GP_DST_YCOOR, dsty);

        /* CALCULATE THE BITMAP OFFSET */

        array_offset =
            (unsigned long) srcy *(long) pitch + ((long) srcx << bpp_shift);

        while (temp_height--) {
            GFX_WAIT_PIPELINE;

            /* WRITE ALL DATA TO THE BLT BUFFERS */
            /* The WRITE_SCRATCH_STRING macro assumes that the data begins 
             * at the scratchpad offset set by the SET_SCRATCH_BASE macro.
             * */

            WRITE_SCRATCH_STRING(dword_bytes_needed, bytes_extra, data,
                                 array_offset);
            WRITE_REG16(GP_BLIT_MODE, blit_mode);

            array_offset += pitch;
        }

        width -= section;
        srcx += section;
        dstx += section;
    }
}

/*
 *----------------------------------------------------------------------------
 * COLOR BITMAP TO SCREEN TRANSPARENT BLT
 *
 * This routine transfers color bitmap data to the screen with transparency.
 * The transparent color is specified.  The only supported ROP is SRCCOPY, 
 * meaning that transparency cannot be applied if the ROP requires 
 * destination data (this is a hardware restriction).
 *
 *      SRCX            X offset within source bitmap
 *      SRCY            Y offset within source bitmap
 *      DSTX            screen X position to render data
 *      DSTY            screen Y position to render data
 *      WIDTH           width of rectangle, in pixels
 *      HEIGHT          height of rectangle, in scanlines
 *      *DATA           pointer to bitmap data
 *      PITCH           pitch of bitmap data (bytes between scanlines)
 *      COLOR           transparent color
 *----------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu1_color_bitmap_to_screen_xblt(unsigned short srcx, unsigned short srcy,
                                unsigned short dstx, unsigned short dsty,
                                unsigned short width, unsigned short height,
                                unsigned char *data, long pitch,
                                unsigned long color)
#else
void
gfx_color_bitmap_to_screen_xblt(unsigned short srcx, unsigned short srcy,
                                unsigned short dstx, unsigned short dsty,
                                unsigned short width, unsigned short height,
                                unsigned char *data, long pitch,
                                unsigned long color)
#endif
{
    unsigned short section, buffer_width;
    unsigned short temp_height;
    unsigned long dword_bytes_needed, bytes_extra;
    unsigned long bpp_shift;
    long array_offset;

    /* CHECK SIZE OF BLT BUFFER */

    buffer_width = GFXbufferWidthPixels;

    /* WRITE TRANSPARENCY COLOR TO BLT BUFFER 1 */

    if (GFXbpp == 8) {
        color &= 0x00FF;
        color |= (color << 8);
    }
    color = (color & 0x0000FFFF) | (color << 16);

    /* WAIT UNTIL PIPELINE IS NOT BUSY BEFORE LOADING DATA INTO BB1 */
    /* Need to make sure any previous BLT using BB1 is complete. */
    /* Only need to load 32 bits of BB1 for the 1 pixel BLT that follows. */

    GFX_WAIT_PIPELINE;
    GFX_WAIT_PENDING;
    WRITE_SCRATCH32(GFXbb1Base, color);

    /* DO BOGUS BLT TO LATCH DATA FROM BB1 */
    /* Already know graphics pipeline is idle. */
    /* Only need to latch data into the holding registers for the current */
    /* data from BB1.  A 1 pixel wide BLT will suffice. */

    WRITE_REG32(GP_DST_XCOOR, 0);
    WRITE_REG32(GP_SRC_XCOOR, 0);
    WRITE_REG32(GP_WIDTH, 0x00010001);
    WRITE_REG16(GP_RASTER_MODE, 0x00CC);
    WRITE_REG16(GP_BLIT_MODE, BM_READ_SRC_FB | BM_READ_DST_BB1);

    /* POLL UNTIL ABLE TO WRITE TO THE REGISTERS */
    /* Write the registers that do not change for each section. */

    GFX_WAIT_PENDING;
    WRITE_REG16(GP_HEIGHT, 1);
    WRITE_REG16(GP_RASTER_MODE, 0x10C6);
    WRITE_REG32(GP_PAT_COLOR_0, 0xFFFFFFFF);

    bpp_shift = (GFXbpp + 7) >> 4;

    /* SET THE SCRATCHPAD BASE */

    SET_SCRATCH_BASE(GFXbb0Base);

    while (width > 0) {
        if (width > buffer_width)
            section = buffer_width;
        else
            section = width;

        dword_bytes_needed = (section << bpp_shift) & ~3l;
        bytes_extra = (section << bpp_shift) & 3l;

        temp_height = height;

        /* WRITE THE REGISTERS FOR EACH SECTION                          */
        /* The GX hardware will auto-increment the Y coordinate, meaning */
        /* that we don't have to.                                        */

        WRITE_REG16(GP_WIDTH, section);
        WRITE_REG16(GP_DST_XCOOR, dstx);
        WRITE_REG16(GP_DST_YCOOR, dsty);

        /* CALCULATE THE BITMAP OFFSET */

        array_offset =
            (unsigned long) srcy *(long) pitch + ((long) srcx << bpp_shift);

        while (temp_height--) {
            GFX_WAIT_PIPELINE;

            /* WRITE ALL DATA TO THE BLT BUFFERS */
            /* The WRITE_SCRATCH_STRING macro assumes that the data begins at the */
            /* scratchpad offset set by the SET_SCRATCH_BASE macro.               */

            WRITE_SCRATCH_STRING(dword_bytes_needed, bytes_extra, data,
                                 array_offset);
            WRITE_REG16(GP_BLIT_MODE, BM_READ_SRC_BB0);

            array_offset += pitch;
        }

        width -= section;
        srcx += section;
        dstx += section;
    }
}

/*
 *----------------------------------------------------------------------------
 * MONOCHROME BITMAP TO SCREEN BLT
 *
 * This routine transfers monochrome bitmap data to the screen.  
 *
 *      SRCX            X offset within source bitmap
 *      SRCY            Y offset within source bitmap
 *      DSTX            screen X position to render data
 *      DSTY            screen Y position to render data
 *      WIDTH           width of rectangle, in pixels
 *      HEIGHT          height of rectangle, in scanlines
 *      *DATA           pointer to bitmap data
 *      PITCH           pitch of bitmap data (bytes between scanlines)
 *----------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu1_mono_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
                              unsigned short dstx, unsigned short dsty,
                              unsigned short width, unsigned short height,
                              unsigned char *data, short pitch)
#else
void
gfx_mono_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
                              unsigned short dstx, unsigned short dsty,
                              unsigned short width, unsigned short height,
                              unsigned char *data, short pitch)
#endif
{
    unsigned short section, buffer_width;
    unsigned short blit_mode = BM_READ_SRC_BB0 | BM_SOURCE_EXPAND;
    unsigned short temp_height;
    unsigned long dword_bytes_needed, bytes_extra;
    long array_offset;

    /* CHECK IF RASTER OPERATION REQUIRES DESTINATION DATA        */
    /* If no destination data, the source data will always fit.   */
    /* So, in that event we will set the buffer width to a        */
    /* fictitiously large value such that the BLT is never split. */

    if (GFXusesDstData) {
        buffer_width = GFXbufferWidthPixels;
        blit_mode |= BM_READ_DST_FB1;
    }
    else
        buffer_width = 3200;

    /* CHECK IF DATA ALREADY IN BLIT BUFFER */
    /* If the pointer is NULL, data for the full BLT is already there */
    /* WARNING: This could cause problems if destination data is */
    /* involved and it overflows the BLT buffer.  Need to remove */
    /* this option and change the drivers to use a temporary buffer. */

    if (!data) {
        GFX_WAIT_PENDING;
        WRITE_REG16(GP_SRC_XCOOR, srcx & 7);
        WRITE_REG16(GP_DST_XCOOR, dstx);
        WRITE_REG16(GP_DST_YCOOR, dsty);
        WRITE_REG16(GP_WIDTH, width);
        WRITE_REG16(GP_HEIGHT, height);
        WRITE_REG16(GP_BLIT_MODE, blit_mode);
        return;
    }

    /* SET THE SCRATCHPAD BASE */

    SET_SCRATCH_BASE(GFXbb0Base);

    /* POLL UNTIL ABLE TO WRITE TO THE REGISTERS */
    /* Write the registers that do not change for each section. */

    GFX_WAIT_PENDING;
    WRITE_REG16(GP_HEIGHT, 1);

    while (width > 0) {
        if (width > buffer_width)
            section = buffer_width;
        else
            section = width;

        /* CALCULATE BYTES NEEDED */
        /* Add 1 for possible alignment issues. */

        dword_bytes_needed = ((section + 7 + (srcx & 7)) >> 3) & ~3l;
        bytes_extra = ((section + 7 + (srcx & 7)) >> 3) & 3l;

        temp_height = height;

        /* WRITE THE REGISTERS FOR EACH SECTION                          */
        /* The GX hardware will auto-increment the Y coordinate, meaning */
        /* that we don't have to.                                        */

        WRITE_REG16(GP_WIDTH, section);
        WRITE_REG16(GP_DST_XCOOR, dstx);
        WRITE_REG16(GP_DST_YCOOR, dsty);
        WRITE_REG16(GP_SRC_XCOOR, srcx & 7);

        /* CALCULATE THE BITMAP OFFSET */

        array_offset = (unsigned long) srcy *(long) pitch + ((long) srcx >> 3);

        while (temp_height--) {
            GFX_WAIT_PIPELINE;

            /* WRITE ALL DATA TO THE BLT BUFFERS */
            /* The WRITE_SCRATCH_STRING macro assumes that the data begins at the */
            /* scratchpad offset set by the SET_SCRATCH_BASE macro.               */

            WRITE_SCRATCH_STRING(dword_bytes_needed, bytes_extra, data,
                                 array_offset);
            WRITE_REG16(GP_BLIT_MODE, blit_mode);

            array_offset += pitch;
        }

        width -= section;
        srcx += section;
        dstx += section;
    }
}

/*
 *----------------------------------------------------------------------------
 * MONOCHROME TEXT BLT
 *
 * This routine transfers contiguous monochrome text data to the screen.  
 *
 *      DSTX            screen X position to render data
 *      DSTY            screen Y position to render data
 *      WIDTH           width of rectangle, in pixels
 *      HEIGHT          height of rectangle, in scanlines
 *      *DATA           pointer to bitmap data
 *----------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu1_text_blt(unsigned short dstx, unsigned short dsty, unsigned short width,
             unsigned short height, unsigned char *data)
#else
void
gfx_text_blt(unsigned short dstx, unsigned short dsty, unsigned short width,
             unsigned short height, unsigned char *data)
#endif
{
    unsigned long dword_bytes_needed, bytes_extra;
    long pitch, buffer_bytes, data_bytes;

    /* CALCULATE DATA SIZE */

    pitch = (width + 7) >> 3;
    data_bytes = (long) height *pitch;

    /* CHECK FOR SIMPLE CASE */
    /* This routine is designed to render a source copy text glyph.  If 
     * destination data is required or the source data will not fit, we will 
     * punt the operation to the more versatile (and slow) mono bitmap routine
     * */

    if (GFXbpp > 8)
        buffer_bytes = GFXbufferWidthPixels << 1;
    else
        buffer_bytes = GFXbufferWidthPixels;

    if (GFXusesDstData || data_bytes > buffer_bytes) {
        gfx_mono_bitmap_to_screen_blt(0, 0, dstx, dsty, width, height, data,
                                      (short) pitch);
        return;
    }

    /* SET THE SCRATCHPAD BASE */

    SET_SCRATCH_BASE(GFXbb0Base);

    /* POLL UNTIL ABLE TO WRITE TO THE REGISTERS */

    dword_bytes_needed = data_bytes & ~3l;
    bytes_extra = data_bytes & 3l;

    GFX_WAIT_PENDING;
    WRITE_REG16(GP_HEIGHT, height);
    WRITE_REG16(GP_WIDTH, width);
    WRITE_REG16(GP_DST_XCOOR, dstx);
    WRITE_REG16(GP_DST_YCOOR, dsty);
    WRITE_REG16(GP_SRC_XCOOR, 0);

    /* WRITE ALL DATA TO THE BLT BUFFERS */
    /* The WRITE_SCRATCH_STRING macro assumes that the data begins at the */
    /* scratchpad offset set by the SET_SCRATCH_BASE macro.               */

    GFX_WAIT_PIPELINE;

    WRITE_SCRATCH_STRING(dword_bytes_needed, bytes_extra, data, 0);
    WRITE_REG16(GP_BLIT_MODE, BM_READ_SRC_BB0 | BM_SOURCE_TEXT);
}

/*
 *----------------------------------------------------------------------------
 * BRESENHAM LINE
 *
 * This routine draws a vector using the specified Bresenham parameters.  
 * Currently this file does not support a routine that accepts the two 
 * endpoints of a vector and calculates the Bresenham parameters.  If it 
 * ever does, this routine is still required for vectors that have been 
 * clipped.
 *
 *      X               screen X position to start vector
 *      Y               screen Y position to start vector
 *      LENGTH          length of the vector, in pixels
 *      INITERR         Bresenham initial error term
 *      AXIALERR        Bresenham axial error term
 *      DIAGERR         Bresenham diagonal error term
 *      FLAGS           VM_YMAJOR, VM_MAJOR_INC, VM_MINOR_INC
 *----------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu1_bresenham_line(unsigned short x, unsigned short y,
                   unsigned short length, unsigned short initerr,
                   unsigned short axialerr, unsigned short diagerr,
                   unsigned short flags)
#else
void
gfx_bresenham_line(unsigned short x, unsigned short y,
                   unsigned short length, unsigned short initerr,
                   unsigned short axialerr, unsigned short diagerr,
                   unsigned short flags)
#endif
{
    unsigned short vector_mode = flags;

    if (GFXusesDstData)
        vector_mode |= VM_READ_DST_FB;

    /* CHECK NULL LENGTH */

    if (!length)
        return;

    /* LOAD THE REGISTERS FOR THE VECTOR */

    GFX_WAIT_PENDING;
    WRITE_REG16(GP_DST_XCOOR, x);
    WRITE_REG16(GP_DST_YCOOR, y);
    WRITE_REG16(GP_VECTOR_LENGTH, length);
    WRITE_REG16(GP_INIT_ERROR, initerr);
    WRITE_REG16(GP_AXIAL_ERROR, axialerr);
    WRITE_REG16(GP_DIAG_ERROR, diagerr);
    WRITE_REG16(GP_VECTOR_MODE, vector_mode);
}

/*---------------------------------------------------------------------------
 * GFX_WAIT_UNTIL_IDLE
 *
 * This routine waits until the graphics engine is idle.  This is required
 * before allowing direct access to the frame buffer.
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu1_wait_until_idle(void)
#else
void
gfx_wait_until_idle(void)
#endif
{
    GFX_WAIT_BUSY;
}

/*---------------------------------------------------------------------------
 * GFX_TEST_BLT_PENDING
 *
 * This routine returns 1 if a BLT is pending, meaning that a call to 
 * perform a rendering operation would stall.  Otherwise it returns 0.
 * It is used by Darwin during random testing to only start a BLT 
 * operation when it knows the Durango routines won't spin on graphics
 * (so Darwin can continue to do frame buffer reads and writes).
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
int
gu1_test_blt_pending(void)
#else
int
gfx_test_blt_pending(void)
#endif
{
    if (READ_REG16(GP_BLIT_STATUS) & BS_BLIT_PENDING)
        return (1);
    else
        return (0);
}

/*---------------------------------------------------------------------------
 * BLT BUFFERS!!!!!
 *---------------------------------------------------------------------------
 */

/* THE BOOT CODE MUST SET THE BLT BUFFER BASES USING THE "CPU_WRITE" */
/* INSTRUCTION TO ONE OF THE FOLLOWING VALUES: */

#define BB0_BASE_2K		0x800
#define BB1_BASE_2K		0xB30
#define BB0_BASE_3K		0x400
#define BB1_BASE_3K		0x930

/*---------------------------------------------------------------------------
 * gu1_detect_blt_buffer_base
 * 
 * This detection is hidden from the driver by being called from the 
 * "gfx_set_bpp" routine.  
 *
 * This is fairly ugly for the following reasons:
 *
 * - It is the boot code that must set the BLT buffer bases to the 
 *   appropriate values and load the scratchpad tags.  
 * - The old drivers would also set the base address values to what they
 *   knew they should be for the 2K or 3K scratchpad configuration.
 * - Unfortunately, to set the base addresses requires the use of the 
 *   CPU_WRITE instruction, an instruction specific to GX.
 * - Using the CPU_WRITE instruction requires the use of assembly to 
 *   produce the appropriate op codes.
 * - Assembly is something that is avoided in Durango because it is not
 *   platform independent.  Some compilers do not support inline assembly.
 * - Therefore Durango cannot use the CPU_WRITE instruction.
 * - Therefore drivers using Durango must rely on the boot code to set
 *   the appropriate values.  Durango uses this routine to check where
 *   the base addresses have been set.
 * - Unfortunately, it is not as simple as using IO to check for 2K or 3K 
 *   scratchpad size.  In VSA1, even though the boot code may set it for
 *   3K, SoftVGA comes along and resets it to 2K for it's use in text
 *   redraws.  It used to be that the display driver would set it back
 *   to 3K.  
 * - So, the Durango code was changed to just always use 2K.
 * - But, the XpressROM code sets it for 3K, and the newer versions of 
 *   SoftVGA do not interfere with that, so then Durango needs to use
 *   the 3K values to work properly.
 * - Therefore, Durango does somewhat of a kludge by writing to directly
 *   to the scratchpad at both the 2K and 3K locations, then performing
 *   a unobtrusive BLT that loads data into BB0 (the graphics engine 
 *   always knows the true base).  After the BLT, Durango looks to see
 *   which location changed to know where the base address is.
 * - This is a relatively simple way to allow Durango to work on old
 *   and new platforms without using theCPU_WRITE instructions.
 *
 * To summarize, the BLT buffers are one of the most painful aspects of 
 * the GX graphics unit design, and have been removed from future designs
 * (the graphics unit has its own dedicated RAM).  Durango has tried to 
 * hide the BLT buffer use from the drivers.
 *---------------------------------------------------------------------------
 */
void
gu1_detect_blt_buffer_base(void)
{
    /* ASSUME 2K */

    GFXbb0Base = BB0_BASE_2K;
    GFXbb1Base = BB1_BASE_2K;

    /* CHECK IF SCRATCHPAD IS SET TO 3K OR 4K */
    /* Boot code should still set 3K values for 4K. */

    if (gfx_gxm_config_read(GXM_CONFIG_GCR) & 0x08) {
        /* WRITE DATA TO 3K LOCATION */

        GFX_WAIT_BUSY;
        WRITE_SCRATCH32(BB0_BASE_3K, 0xFEEDFACE);

        /* HAVE THE GRAPHICS UNIT STORE SOMETHING IN BB0 */

        WRITE_REG32(GP_DST_XCOOR, 0x00000000);  /* AT (0,0)     */
        WRITE_REG32(GP_WIDTH, 0x00010004);      /* 4x1 BLT              */
        WRITE_REG16(GP_RASTER_MODE, 0x00AA);    /* KEEP DST     */
        WRITE_REG16(GP_BLIT_MODE, BM_READ_DST_FB0);     /* STORE IN BB0 */

        /* CHECK 3K LOCATION */
        /* Breaks if data happened to be 0xFEEDFACE - unlikely. */

        GFX_WAIT_BUSY;
        if (READ_SCRATCH32(BB0_BASE_3K) != 0xFEEDFACE) {
            GFXbb0Base = BB0_BASE_3K;
            GFXbb1Base = BB1_BASE_3K;
        }
    }
}

/* END OF FILE */
