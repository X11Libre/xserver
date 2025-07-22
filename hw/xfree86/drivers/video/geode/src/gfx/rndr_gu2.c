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
 * the second generation graphics unit.
 * 
 * Basic rendering routines (common to all Geode processors):
 *    gfx_set_bpp           
 *    gfx_set_solid_pattern  
 *    gfx_set_mono_pattern
 *    gfx_set_color_pattern
 *    gfx_set_solid_source  
 *    gfx_set_mono_source
 *    gfx_set_raster_operation
 *    gfx_pattern_fill
 *    gfx_color_pattern_fill
 *    gfx_screen_to_screen_blt
 *    gfx_screen_to_screen_xblt
 *    gfx_color_bitmap_to_screen_blt
 *    gfx_color_bitmap_to_screen_xblt
 *    gfx_mono_bitmap_to_screen_blt
 *    gfx_bresenham_line 
 *    gfx_wait_until_idle   
 *
 * Extended rendering routines for second generation functionality:
 *    gfx2_set_source_stride
 *    gfx2_set_destination_stride
 *    gfx2_set_pattern_origins
 *    gfx2_set_source_transparency
 *    gfx2_set_alpha_mode
 *    gfx2_set_alpha_value
 *    gfx2_pattern_fill
 *    gfx2_color_pattern_fill
 *    gfx2_screen_to_screen_blt
 *    gfx2_mono_expand_blt
 *    gfx2_color_bitmap_to_screen_blt
 *    gfx2_mono_bitmap_to_screen_blt
 *    gfx2_bresenham_line
 *    gfx2_sync_to_vblank
 * */

#define GU2_WAIT_PENDING		\
		while(READ_GP32(MGP_BLT_STATUS) & MGP_BS_BLT_PENDING)
#define GU2_WAIT_BUSY			\
		while(READ_GP32(MGP_BLT_STATUS) & MGP_BS_BLT_BUSY)
#define GU2_WAIT_HALF_EMPTY		\
		while(!(READ_GP32(MGP_BLT_STATUS) & MGP_BS_HALF_EMPTY))

/* PATTERN SWIZZLES */

#define WORD_SWIZZLE(x) (((x) << 16) | ((x) >> 16))
#define BYTE_SWIZZLE(x) (((x) << 24) | ((x) >> 24) | (((x) << 8) & 	\
						0x00FF0000) | (((x) >> 8) & 0x0000FF00))

/* GLOBAL VARIABLES USED BY THE RENDERING ROUTINES */

unsigned long gu2_bpp;
unsigned long gu2_pitch = 1280;
unsigned long gu2_src_pitch = 1280;
unsigned long gu2_dst_pitch = 1280;
unsigned long gu2_xshift = 1;
unsigned long gu2_pattern_origin = 0;
unsigned long gu2_rop32;
unsigned long gu2_alpha32 = 0;
unsigned long gu2_alpha_value = 0;
unsigned long gu2_alpha_mode = 0;
unsigned long gu2_alpha_active = 0;
unsigned short gu2_alpha_blt_mode = 0;
unsigned short gu2_alpha_vec_mode = 0;
unsigned short gu2_blt_mode = 0;
unsigned short gu2_vector_mode = 0;
unsigned short gu2_bm_throttle = 0;
unsigned short gu2_vm_throttle = 0;
int gu2_current_line = 0;

/* Kill a warning */
void gfx_reset_pitch(unsigned short pitch);

/*---------------------------------------------------------------------------
 * GFX_RESET_PITCH (PRIVATE ROUTINE - NOT PART OF API)
 *
 * This routine resets all pitches in the graphics engine to one value.
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu2_reset_pitch(unsigned short pitch)
#else
void
gfx_reset_pitch(unsigned short pitch)
#endif
{
    gu2_pitch = pitch;
    gu2_dst_pitch = pitch;
    gu2_src_pitch = pitch;
}

/*---------------------------------------------------------------------------
 * GFX_SET_BPP
 *
 * This routine sets the bits per pixel value in the graphics engine.
 * It is also stored in the static variable "gu2_bpp" to use in the future 
 * calls to the rendering routines.  That variable contains the hardware 
 * specific value to load into the MGP_RASTER_MODE register.
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu2_set_bpp(unsigned short bpp)
#else
void
gfx_set_bpp(unsigned short bpp)
#endif
{
    GFXbpp = bpp;

    /* CONVERT TO BPP/FORMAT VALUE */
    /* Save in global to combine with ROP later. */
    /* Could write register here and then use byte access for */
    /* the ROP, but would need to set other 24 bits to make */
    /* sure all are set to their appropriate values. */

    switch (bpp) {
    case 8:
        gu2_bpp = MGP_RM_BPPFMT_332;
        gu2_xshift = 0;
        break;
    case 12:
        gu2_bpp = MGP_RM_BPPFMT_4444;
        gu2_xshift = 1;
        break;
    case 15:
        gu2_bpp = MGP_RM_BPPFMT_1555;
        gu2_xshift = 1;
        break;
    case 16:
        gu2_bpp = MGP_RM_BPPFMT_565;
        gu2_xshift = 1;
        break;
    case 32:
        gu2_bpp = MGP_RM_BPPFMT_8888;
        gu2_xshift = 2;
        break;
    }

    /* SET INITIAL ROP BASED ONLY ON BPP */
    /* Needs to be set before loading any pattern or source colors. */
    /* We must wait for BUSY because these bits are not pipelined   */
    /* in the hardware.                                             */

    GU2_WAIT_BUSY;
    WRITE_GP32(MGP_RASTER_MODE, gu2_bpp);
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
gu2_set_solid_source(unsigned long color)
#else
void
gfx_set_solid_source(unsigned long color)
#endif
{
    /* CLEAR TRANSPARENCY FLAG */

    GFXsourceFlags = 0;

    /* WRITE REGISTERS TO SPECIFY SOURCE COLOR */

    GU2_WAIT_PENDING;
    WRITE_GP32(MGP_SRC_COLOR_FG, color);
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
gu2_set_mono_source(unsigned long bgcolor, unsigned long fgcolor,
                    unsigned short transparent)
#else
void
gfx_set_mono_source(unsigned long bgcolor, unsigned long fgcolor,
                    unsigned short transparent)
#endif
{
    /* SET TRANSPARENCY FLAG */

    GFXsourceFlags = transparent ? MGP_RM_SRC_TRANS : 0;

    /* WRITE COLOR VALUES */

    GU2_WAIT_PENDING;
    WRITE_GP32(MGP_SRC_COLOR_FG, fgcolor);
    WRITE_GP32(MGP_SRC_COLOR_BG, bgcolor);
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
gu2_set_solid_pattern(unsigned long color)
#else
void
gfx_set_solid_pattern(unsigned long color)
#endif
{
    /* CLEAR TRANSPARENCY FLAG */

    GFXsourceFlags = 0;

    /* SET PATTERN FLAGS */

    GFXpatternFlags = 0;

    /* POLL UNTIL ABLE TO WRITE THE PATTERN COLOR */

    GU2_WAIT_PENDING;
    WRITE_GP32(MGP_RASTER_MODE, gu2_bpp);
    WRITE_GP32(MGP_PAT_COLOR_0, color);
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
gu2_set_mono_pattern(unsigned long bgcolor, unsigned long fgcolor,
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

    if (transparent)
        GFXpatternFlags = MGP_RM_PAT_MONO | MGP_RM_PAT_TRANS;
    else
        GFXpatternFlags = MGP_RM_PAT_MONO;

    /* POLL UNTIL ABLE TO WRITE THE PATTERN COLOR */

    GU2_WAIT_PENDING;
    WRITE_GP32(MGP_RASTER_MODE, gu2_bpp | GFXpatternFlags);
    WRITE_GP32(MGP_PAT_COLOR_0, bgcolor);
    WRITE_GP32(MGP_PAT_COLOR_1, fgcolor);
    WRITE_GP32(MGP_PAT_DATA_0, data0);
    WRITE_GP32(MGP_PAT_DATA_1, data1);
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
gu2_set_color_pattern(unsigned long bgcolor, unsigned long fgcolor,
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
    /* REMOVE */
}

/*
 *---------------------------------------------------------------------------
 * GFX_LOAD_COLOR_PATTERN_LINE
 *
 * This routine is used to load a single line of a 8x8 color pattern.   
 *---------------------------------------------------------------------------  */
#if GFX_2DACCEL_DYNAMIC
void
gu2_load_color_pattern_line(short y, unsigned long *pattern_8x8)
#else
void
gfx_load_color_pattern_line(short y, unsigned long *pattern_8x8)
#endif
{
    unsigned long temp1, temp2, temp3, temp4;

    /* CLEAR TRANSPARENCY FLAG */

    GFXsourceFlags = 0;

    /* SET PATTERN FLAGS */

    GFXpatternFlags = MGP_RM_PAT_COLOR;

    /* OVERRIDE THE RASTER MODE REGISTER */
    /* If the pattern format is set to anything but color  */
    /* before loading the registers, some of the data will */
    /* be duplicated according to the current mode.        */

    GU2_WAIT_PENDING;
    WRITE_GP32(MGP_RASTER_MODE,
               (gu2_rop32 & ~MGP_RM_PAT_FLAGS) | MGP_RM_PAT_COLOR);

    /* LOAD THE PATTERN DATA */
    /* This routine is designed to work in tandem with gfx_pattern_fill.  */
    /* It is used for cases when multiple BLTs with color pattern data    */
    /* are desired on the same line.  It would be inefficient to          */
    /* repeatedly call gfx_color_pattern_fill for each single-line BLT.   */
    /* So, we will simply replicate the pattern data across all available */
    /* lines such that the pattern y origin plays no part in the BLT.     */

    /* 8 BPP */

    if (gu2_xshift == 0) {
        pattern_8x8 += (y & 7) << 1;
        temp1 = BYTE_SWIZZLE(pattern_8x8[0]);
        temp2 = BYTE_SWIZZLE(pattern_8x8[1]);
        WRITE_GP32(MGP_PAT_DATA_1, temp1);
        WRITE_GP32(MGP_PAT_DATA_0, temp2);
        WRITE_GP32(MGP_PAT_COLOR_1, temp1);
        WRITE_GP32(MGP_PAT_COLOR_0, temp2);

        GU2_WAIT_BUSY;
        WRITE_GP32(MGP_PAT_COLOR_3, temp1);
        WRITE_GP32(MGP_PAT_COLOR_2, temp2);
        WRITE_GP32(MGP_PAT_COLOR_5, temp1);
        WRITE_GP32(MGP_PAT_COLOR_4, temp2);
    }
    else if (gu2_xshift == 1) {
        pattern_8x8 += (y & 7) << 2;
        temp1 = WORD_SWIZZLE(pattern_8x8[0]);
        temp2 = WORD_SWIZZLE(pattern_8x8[1]);
        temp3 = WORD_SWIZZLE(pattern_8x8[2]);
        temp4 = WORD_SWIZZLE(pattern_8x8[3]);

        WRITE_GP32(MGP_PAT_COLOR_1, temp1);
        WRITE_GP32(MGP_PAT_COLOR_0, temp2);
        WRITE_GP32(MGP_PAT_DATA_1, temp3);
        WRITE_GP32(MGP_PAT_DATA_0, temp4);

        GU2_WAIT_BUSY;
        WRITE_GP32(MGP_PAT_COLOR_5, temp1);
        WRITE_GP32(MGP_PAT_COLOR_4, temp2);
        WRITE_GP32(MGP_PAT_COLOR_3, temp3);
        WRITE_GP32(MGP_PAT_COLOR_2, temp4);
    }
    else {
        pattern_8x8 += (y & 7) << 3;

        WRITE_GP32(MGP_PAT_COLOR_1, pattern_8x8[4]);
        WRITE_GP32(MGP_PAT_COLOR_0, pattern_8x8[5]);
        WRITE_GP32(MGP_PAT_DATA_1, pattern_8x8[6]);
        WRITE_GP32(MGP_PAT_DATA_0, pattern_8x8[7]);

        GU2_WAIT_BUSY;
        WRITE_GP32(MGP_PAT_COLOR_5, pattern_8x8[0]);
        WRITE_GP32(MGP_PAT_COLOR_4, pattern_8x8[1]);
        WRITE_GP32(MGP_PAT_COLOR_3, pattern_8x8[2]);
        WRITE_GP32(MGP_PAT_COLOR_2, pattern_8x8[3]);
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
gu2_set_raster_operation(unsigned char rop)
#else
void
gfx_set_raster_operation(unsigned char rop)
#endif
{
    gu2_blt_mode = 0;

    /* DISABLE ALPHA BLENDING */

    gu2_alpha_active = 0;

    /* GENERATE 32-BIT VERSION OF ROP WITH PATTERN FLAGS */

    gu2_rop32 = (unsigned long) rop | GFXpatternFlags | gu2_bpp;

    /* CHECK IF SOURCE FLAGS SHOULD BE MERGED */

    if ((rop & 0x33) ^ ((rop >> 2) & 0x33))
        gu2_rop32 |= GFXsourceFlags;
    else
        gu2_blt_mode = 0x40;

    /* SET FLAG INDICATING ROP REQUIRES DESTINATION DATA */
    /* True if even bits (0:2:4:6) do not equal the corresponding */
    /* even bits (1:3:5:7). */

    if ((rop & 0x55) ^ ((rop >> 1) & 0x55)) {
        gu2_blt_mode |= MGP_BM_DST_REQ;
        gu2_vector_mode = MGP_VM_DST_REQ;
    }
    else {
        gu2_vector_mode = 0;
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
gu2_pattern_fill(unsigned short x, unsigned short y,
                 unsigned short width, unsigned short height)
#else
void
gfx_pattern_fill(unsigned short x, unsigned short y,
                 unsigned short width, unsigned short height)
#endif
{
    unsigned long offset = 0, size;

    size = (((unsigned long) width) << 16) | height;

    /* CALCULATE STARTING OFFSET */

    offset = (unsigned long) y *gu2_pitch + (((unsigned long) x) << gu2_xshift);

    /* CHECK IF PATTERN ORIGINS NEED TO BE SET */

    if (GFXpatternFlags) {
        /* COMBINE X AND Y PATTERN ORIGINS WITH OFFSET */

        offset |= ((unsigned long) (x & 7)) << 26;
        offset |= ((unsigned long) (y & 7)) << 29;
    }

    /* POLL UNTIL ABLE TO WRITE TO THE REGISTERS */
    /* Put off poll for as long as possible (do most calculations first). */

    GU2_WAIT_PENDING;
    WRITE_GP32(MGP_RASTER_MODE, gu2_rop32);
    WRITE_GP32(MGP_DST_OFFSET, offset);
    WRITE_GP32(MGP_WID_HEIGHT, size);
    WRITE_GP32(MGP_STRIDE, gu2_pitch);
    WRITE_GP32(MGP_BLT_MODE, gu2_blt_mode);
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
gu2_color_pattern_fill(unsigned short x, unsigned short y,
                       unsigned short width, unsigned short height,
                       unsigned long *pattern)
#else
void
gfx_color_pattern_fill(unsigned short x, unsigned short y,
                       unsigned short width, unsigned short height,
                       unsigned long *pattern)
#endif
{
    /* CALL GFX2 ROUTINE TO AVOID DUPLICATION OF CODE */

    unsigned long offset = (unsigned long) y * gu2_pitch +
        (((unsigned long) x) << gu2_xshift);
    unsigned long origin = gu2_pattern_origin;
    unsigned long pitch = gu2_dst_pitch;

    gfx2_set_pattern_origin(x, y);
    gfx2_set_destination_stride((unsigned short) gu2_pitch);
    gfx2_color_pattern_fill(offset, width, height, pattern);

    /* RESTORE GFX2 VALUES */

    gu2_pattern_origin = origin;
    gu2_dst_pitch = pitch;
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
gu2_screen_to_screen_blt(unsigned short srcx, unsigned short srcy,
                         unsigned short dstx, unsigned short dsty,
                         unsigned short width, unsigned short height)
#else
void
gfx_screen_to_screen_blt(unsigned short srcx, unsigned short srcy,
                         unsigned short dstx, unsigned short dsty,
                         unsigned short width, unsigned short height)
#endif
{
    unsigned long srcoffset, dstoffset, size;
    unsigned short blt_mode;

    size = (((unsigned long) width) << 16) | height;

    /* CALCULATE THE DIRECTION OF THE BLT */

    blt_mode = (gu2_blt_mode & ~MGP_BM_SRC_TYPE_MASK) | MGP_BM_SRC_FB;
    if (dstx > srcx) {
        blt_mode |= MGP_BM_NEG_XDIR;
        srcx += width - 1;
        dstx += width - 1;
    }
    if (dsty > srcy) {
        blt_mode |= MGP_BM_NEG_YDIR;
        srcy += height - 1;
        dsty += height - 1;
    }

    /* CALCULATE STARTING OFFSETS */

    srcoffset = (unsigned long) srcy *gu2_pitch +
        (((unsigned long) srcx) << gu2_xshift);
    dstoffset = ((unsigned long) dsty * gu2_pitch +
                 (((unsigned long) dstx) << gu2_xshift)) & 0xFFFFFF;

    /* MERGE PATTERN INFORMATION */
    /* This must be done after the x and y coordinates have been updated,  */
    /* as the x and y pattern origins correspond to the first ROPed pixel. */

    if (GFXpatternFlags) {
        /* COMBINE X AND Y PATTERN ORIGINS WITH OFFSET */

        dstoffset |= ((unsigned long) (dstx & 7)) << 26;
        dstoffset |= ((unsigned long) (dsty & 7)) << 29;
    }

    /* TURN INTO BYTE ADDRESS IF NEGATIVE X DIRECTION */
    /* This is a quirk of the hardware. */

    if (blt_mode & MGP_BM_NEG_XDIR) {
        srcoffset += (1 << gu2_xshift) - 1;
        dstoffset += (1 << gu2_xshift) - 1;
    }

    /* POLL UNTIL ABLE TO WRITE TO THE REGISTERS */
    /* Put off poll for as long as possible (do most calculations first). */

    GU2_WAIT_PENDING;
    WRITE_GP32(MGP_RASTER_MODE, gu2_rop32 | GFXsourceFlags);
    WRITE_GP32(MGP_SRC_OFFSET, srcoffset);
    WRITE_GP32(MGP_DST_OFFSET, dstoffset);
    WRITE_GP32(MGP_WID_HEIGHT, size);
    WRITE_GP32(MGP_STRIDE, gu2_pitch | (gu2_pitch << 16));
    WRITE_GP16(MGP_BLT_MODE, blt_mode);
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
gu2_screen_to_screen_xblt(unsigned short srcx, unsigned short srcy,
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
    unsigned long rop32;

    /* SAVE ORIGINAL RASTER MODE SETTINGS */

    rop32 = gu2_rop32;

    /* WRITE REGISTERS TO SPECIFY COLOR TRANSPARENCY */
    /* Match GU1 implementation that only allows SRCCOPY for the ROP. */

    GU2_WAIT_PENDING;
    WRITE_GP32(MGP_SRC_COLOR_FG, color);
    WRITE_GP32(MGP_SRC_COLOR_BG, 0xFFFFFFFF);

    /* SET GLOBAL RASTER SETTINGS */
    /* This is needed, as the screen-to-screen BLT      */
    /* routine will overwrite the raster mode register. */

    gu2_rop32 = gu2_bpp | MGP_RM_SRC_TRANS | 0xCC;

    /* CALL NORMAL SCREEN TO SCREEN BLT ROUTINE */

    gfx_screen_to_screen_blt(srcx, srcy, dstx, dsty, width, height);

    /* RESTORE GLOBAL RASTER SETTINGS */

    gu2_rop32 = rop32;
}

/*
 *----------------------------------------------------------------------------
 * COLOR BITMAP TO SCREEN BLT
 *
 * This routine transfers color bitmap data to the screen.
 *
 *      SRCX            X offset within source bitmap
 *      SRCY            Y offset within source bitmap
 *      DSTX            screen X position to render data
 *      DSTY            screen Y position to render data
 *      WIDTH           width of rectangle, in pixels
 *      HEIGHT          height of rectangle, in scanlines
 *      *DATA           pointer to bitmap data
 *      PITCH           pitch of bitmap data (bytes between scanlines)
 *
 * Transparency is handled by another routine.
 *----------------------------------------------------------------------------
 */

#if GFX_2DACCEL_DYNAMIC
void
gu2_color_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
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
    unsigned long dstoffset, srcoffset, size, bytes;
    unsigned long offset, temp_offset;
    unsigned long dword_bytes, bytes_extra;
    unsigned short blt_mode;

    blt_mode = (gu2_blt_mode & ~MGP_BM_SRC_TYPE_MASK) | MGP_BM_SRC_FB;
    size = (((unsigned long) width) << 16) | 1;

    /* CALCULATE STARTING OFFSETS */

    offset = (unsigned long) srcy *pitch + ((unsigned long) srcx << gu2_xshift);

    dstoffset = (unsigned long) dsty *gu2_pitch +
        (((unsigned long) dstx) << gu2_xshift);

    /* CHECK IF PATTERN ORIGINS NEED TO BE SET */

    if (GFXpatternFlags) {
        /* COMBINE X AND Y PATTERN ORIGINS WITH OFFSET */

        dstoffset |= ((unsigned long) (dstx & 7)) << 26;
        dstoffset |= ((unsigned long) (dsty & 7)) << 29;
    }

    bytes = width << gu2_xshift;
    dword_bytes = bytes & ~0x3L;
    bytes_extra = bytes & 0x3L;

    /* POLL UNTIL ABLE TO WRITE TO THE REGISTERS */
    /* Put off poll for as long as possible (do most calculations first).   */
    /* The source offset is always 0 since we allow misaligned dword reads. */
    /* We must wait for BLT busy because the GP may be executing a screen   */
    /* to screen BLT from the scratchpad area.                              */

    GU2_WAIT_BUSY;
    WRITE_GP32(MGP_RASTER_MODE, gu2_rop32 | GFXsourceFlags);
    WRITE_GP32(MGP_WID_HEIGHT, size);
    WRITE_GP32(MGP_STRIDE, gu2_pitch);

    /* WRITE DATA ONE LINE AT A TIME */
    /* For speed reasons, data is written to an offscreen scratch area and then        */
    /* BLTed using a screen to screen BLT. This is similar to the GX1 BLT buffers, but */
    /* slightly more efficient in that we can queue up data while the GP is rendering  */
    /* a line.                                                                         */

    while (height--) {
        temp_offset = offset;
        srcoffset = gfx_gx2_scratch_base;
        if (gu2_current_line)
            srcoffset += 8192;

        GU2_WAIT_PENDING;
        WRITE_GP32(MGP_SRC_OFFSET, srcoffset);
        WRITE_GP32(MGP_DST_OFFSET, dstoffset);
        dstoffset += gu2_pitch;
        dstoffset += 0x20000000;

        WRITE_FRAME_BUFFER_STRING32(srcoffset, dword_bytes, data, temp_offset);
        if (bytes_extra) {
            temp_offset += dword_bytes;
            srcoffset += dword_bytes;
            WRITE_FRAME_BUFFER_STRING8(srcoffset, bytes_extra, data,
                                       temp_offset);
        }
        WRITE_GP16(MGP_BLT_MODE, blt_mode);
        offset += pitch;
        gu2_current_line = 1 - gu2_current_line;
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
gu2_color_bitmap_to_screen_xblt(unsigned short srcx, unsigned short srcy,
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
    unsigned long rop32;

    /* SAVE EXISTING RASTER MODE SETTINGS */

    rop32 = gu2_rop32;

    /* WRITE REGISTERS TO SPECIFY COLOR TRANSPARENCY */
    /* Match GU1 implementation that only allows SRCCOPY for the ROP. */

    GU2_WAIT_PENDING;
    WRITE_GP32(MGP_SRC_COLOR_FG, color);
    WRITE_GP32(MGP_SRC_COLOR_BG, 0xFFFFFFFF);

    /* SET GLOBAL RASTER SETTINGS */
    /* This is needed, as the screen-to-screen BLT      */
    /* routine will overwrite the raster mode register. */

    gu2_rop32 = gu2_bpp | MGP_RM_SRC_TRANS | 0xCC;

    /* CALL NORMAL COLOR BITMAP TO SCREEN BLT ROUTINE */

    gfx_color_bitmap_to_screen_blt(srcx, srcy, dstx, dsty, width, height,
                                   data, pitch);

    /* RESTORE RASTER SETTINGS */

    gu2_rop32 = rop32;
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
gu2_mono_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
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
    unsigned long dstoffset, size, bytes;
    unsigned long offset, temp_offset, temp1 = 0, temp2 = 0;
    unsigned long i, j = 0, fifo_lines, dwords_extra, bytes_extra;
    unsigned long shift = 0;

    size = (((unsigned long) width) << 16) | height;

    /* CALCULATE STARTING OFFSETS */

    offset = (unsigned long) srcy *pitch + ((unsigned long) srcx >> 3);

    dstoffset = (unsigned long) dsty *gu2_pitch +
        (((unsigned long) dstx) << gu2_xshift);

    /* CHECK IF PATTERN ORIGINS NEED TO BE SET */

    if (GFXpatternFlags) {
        /* COMBINE X AND Y PATTERN ORIGINS WITH OFFSET */

        dstoffset |= ((unsigned long) (dstx & 7)) << 26;
        dstoffset |= ((unsigned long) (dsty & 7)) << 29;
    }

    bytes = ((srcx & 7) + width + 7) >> 3;
    fifo_lines = bytes >> 5;
    dwords_extra = (bytes & 0x0000001Cl) >> 2;
    bytes_extra = bytes & 0x00000003l;

    /* POLL UNTIL ABLE TO WRITE TO THE REGISTERS */
    /* Put off poll for as long as possible (do most calculations first).   */
    /* The source offset is always 0 since we allow misaligned dword reads. */
    /* Need to wait for busy instead of pending, since hardware clears      */
    /* the host data FIFO at the beginning of a BLT.                        */

    GU2_WAIT_PENDING;
    WRITE_GP32(MGP_RASTER_MODE, gu2_rop32 | GFXsourceFlags);
    WRITE_GP32(MGP_SRC_OFFSET, ((unsigned long) srcx & 7) << 26);
    WRITE_GP32(MGP_DST_OFFSET, dstoffset);
    WRITE_GP32(MGP_WID_HEIGHT, size);
    WRITE_GP32(MGP_STRIDE, gu2_pitch);
    WRITE_GP16(MGP_BLT_MODE,
               (gu2_blt_mode & ~MGP_BM_SRC_TYPE_MASK) | MGP_BM_SRC_HOST |
               MGP_BM_SRC_MONO);

    /* WAIT FOR BLT TO BE LATCHED */

    GU2_WAIT_PENDING;

    /* WRITE ALL OF THE DATA TO THE HOST SOURCE REGISTER */

    while (height--) {
        temp_offset = offset;

        /* WRITE ALL FULL FIFO LINES */

        for (i = 0; i < fifo_lines; i++) {
            GU2_WAIT_HALF_EMPTY;
            WRITE_GPREG_STRING32(MGP_HST_SOURCE, 8, j, data, temp_offset,
                                 temp1);
            temp_offset += 32;
        }

        /* WRITE ALL FULL DWORDS */

        GU2_WAIT_HALF_EMPTY;
        if (dwords_extra) {
            WRITE_GPREG_STRING32(MGP_HST_SOURCE, dwords_extra, i, data,
                                 temp_offset, temp1);
            temp_offset += (dwords_extra << 2);
        }

        /* WRITE REMAINING BYTES */

        shift = 0;
        if (bytes_extra)
            WRITE_GPREG_STRING8(MGP_HST_SOURCE, bytes_extra, shift, i, data,
                                temp_offset, temp1, temp2);

        offset += pitch;
    }
}

/*---------------------------------------------------------------------------
 * GFX_TEXT_BLT	
 *
 * This routine is similar to the gfx_mono_bitmap_to_screen_blt routine 
 * but assumes that source data is byte-packed.
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu2_text_blt(unsigned short dstx, unsigned short dsty, unsigned short width,
             unsigned short height, unsigned char *data)
#else
void
gfx_text_blt(unsigned short dstx, unsigned short dsty, unsigned short width,
             unsigned short height, unsigned char *data)
#endif
{
    unsigned long size, bytes;
    unsigned long dstoffset, temp1 = 0, temp2 = 0, temp_offset = 0;
    unsigned long i, j = 0, fifo_lines, dwords_extra, bytes_extra;
    unsigned long shift;

    size = (((unsigned long) width) << 16) | height;

    dstoffset = (unsigned long) dsty *gu2_pitch +
        (((unsigned long) dstx) << gu2_xshift);

    /* CHECK IF PATTERN ORIGINS NEED TO BE SET */

    if (GFXpatternFlags) {
        /* COMBINE X AND Y PATTERN ORIGINS WITH OFFSET */

        dstoffset |= ((unsigned long) (dstx & 7)) << 26;
        dstoffset |= ((unsigned long) (dsty & 7)) << 29;
    }

    /* CALCULATE STARTING OFFSETS */

    bytes = ((width + 7) >> 3) * height;
    fifo_lines = bytes >> 5;
    dwords_extra = (bytes & 0x0000001Cl) >> 2;
    bytes_extra = bytes & 0x00000003l;

    /* POLL UNTIL ABLE TO WRITE TO THE REGISTERS */

    GU2_WAIT_PENDING;
    WRITE_GP32(MGP_RASTER_MODE, gu2_rop32 | GFXsourceFlags);
    WRITE_GP32(MGP_SRC_OFFSET, 0);
    WRITE_GP32(MGP_DST_OFFSET, dstoffset);
    WRITE_GP32(MGP_WID_HEIGHT, size);
    WRITE_GP32(MGP_STRIDE, gu2_pitch);
    WRITE_GP16(MGP_BLT_MODE,
               (gu2_blt_mode & ~MGP_BM_SRC_TYPE_MASK) | MGP_BM_SRC_HOST |
               MGP_BM_SRC_BP_MONO);

    /* WAIT FOR BLT TO BE LATCHED */

    GU2_WAIT_PENDING;

    /* WRITE ALL FULL FIFO LINES */

    for (i = 0; i < fifo_lines; i++) {
        GU2_WAIT_HALF_EMPTY;
        WRITE_GPREG_STRING32(MGP_HST_SOURCE, 8, j, data, temp_offset, temp1);
        temp_offset += 32;
    }

    /* WRITE ALL FULL DWORDS */

    if (dwords_extra || bytes_extra) {
        GU2_WAIT_HALF_EMPTY;
        if (dwords_extra) {
            WRITE_GPREG_STRING32(MGP_HST_SOURCE, dwords_extra, i, data,
                                 temp_offset, temp1);
            temp_offset += (dwords_extra << 2);
        }
        if (bytes_extra) {
            shift = 0;
            WRITE_GPREG_STRING8(MGP_HST_SOURCE, bytes_extra, shift, i, data,
                                temp_offset, temp1, temp2);
        }
    }
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
gu2_bresenham_line(unsigned short x, unsigned short y,
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
    unsigned long offset;
    unsigned long data1 = (((unsigned long) axialerr) << 16) | diagerr;
    unsigned long data2 = (((unsigned long) length) << 16) | initerr;
    unsigned short vector_mode = gu2_vector_mode | flags;

    /* CALCULATE STARTING OFFSET */

    offset = (unsigned long) y *gu2_pitch + (((unsigned long) x) << gu2_xshift);

    /* CHECK NULL LENGTH */

    if (!length)
        return;

    /* POLL UNTIL ABLE TO WRITE TO THE REGISTERS */
    /* Put off poll for as long as possible (do most calculations first). */

    GU2_WAIT_PENDING;
    WRITE_GP32(MGP_RASTER_MODE, gu2_rop32);
    WRITE_GP32(MGP_DST_OFFSET, offset);
    WRITE_GP32(MGP_VEC_ERR, data1);
    WRITE_GP32(MGP_VEC_LEN, data2);
    WRITE_GP32(MGP_STRIDE, gu2_pitch);
    WRITE_GP32(MGP_VECTOR_MODE, vector_mode);
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
gu2_wait_until_idle(void)
#else
void
gfx_wait_until_idle(void)
#endif
{
    while (READ_GP32(MGP_BLT_STATUS) & MGP_BS_BLT_BUSY);
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
gu2_test_blt_pending(void)
#else
int
gfx_test_blt_pending(void)
#endif
{
    if (READ_GP32(MGP_BLT_STATUS) & MGP_BS_BLT_PENDING)
        return (1);
    return (0);
}

/*---------------------------------------------------------------------------
 * NEW ROUTINES FOR REDCLOUD
 *---------------------------------------------------------------------------
 */

/*---------------------------------------------------------------------------
 * GFX2_SET_SOURCE_STRIDE
 *
 * This routine sets the stride to be used in successive screen to screen 
 * BLTs (used by gfx2_screen_to_screen_blt and gfx2_mono_expand_blt).
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu22_set_source_stride(unsigned short stride)
#else
void
gfx2_set_source_stride(unsigned short stride)
#endif
{
    /* SAVE STRIDE TO BE USED LATER */

    gu2_src_pitch = (unsigned long) stride;
}

/*---------------------------------------------------------------------------
 * GFX2_SET_DESTINATION_STRIDE
 *
 * This routine sets the stride used when rendering to the screen.
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu22_set_destination_stride(unsigned short stride)
#else
void
gfx2_set_destination_stride(unsigned short stride)
#endif
{
    /* SAVE STRIDE TO BE USED LATER */

    gu2_dst_pitch = (unsigned long) stride;
}

/*---------------------------------------------------------------------------
 * GFX2_SET_PATTERN_ORIGIN
 *
 * This routine sets the origin within an 8x8 pattern.  It is needed if 
 * using a monochrome or color pattern (not used for a solid pattern).
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu22_set_pattern_origin(int x, int y)
#else
void
gfx2_set_pattern_origin(int x, int y)
#endif
{
    /* STORE IN FORMAT THAT CAN BE COMBINED WITH THE DESTINATION OFFSET */

    gu2_pattern_origin = (((unsigned long) (x & 7)) << 26) |
        (((unsigned long) (y & 7)) << 29);
}

/*---------------------------------------------------------------------------
 * GFX2_SET_SOURCE_TRANSPARENCY
 *
 * This routine sets the source transparency color and mask to be used
 * in future rendering operations.  If both the color and mask are set 
 * to zero (normally completely transparent), those values indicate that
 * transparency should be disabled.
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu22_set_source_transparency(unsigned long color, unsigned long mask)
#else
void
gfx2_set_source_transparency(unsigned long color, unsigned long mask)
#endif
{
    /* WRITE REGISTERS TO SPECIFY COLOR TRANSPARENCY */

    GU2_WAIT_PENDING;
    WRITE_GP32(MGP_SRC_COLOR_FG, color);
    WRITE_GP32(MGP_SRC_COLOR_BG, mask);

    /* SET TRANSPARENCY FLAG */

    GFXsourceFlags = (color || mask) ? MGP_RM_SRC_TRANS : 0;
}

/*---------------------------------------------------------------------------
 * GFX2_SET_ALPHA_MODE
 *
 * This routine sets the alpha blending mode to be used in successive 
 * rendering operations.
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu22_set_alpha_mode(int mode)
#else
void
gfx2_set_alpha_mode(int mode)
#endif
{
    /* SAVE ALPHA MODE FOR LATER */

    gu2_alpha_mode = mode;
}

/*---------------------------------------------------------------------------
 * GFX2_SET_ALPHA_VALUE
 *
 * This routine sets the alpha value to be used with certain alpha blending
 * modes (ALPHA_MODE_BLEND).
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu22_set_alpha_value(unsigned char value)
#else
void
gfx2_set_alpha_value(unsigned char value)
#endif
{
    /* SAVE ALPHA VALUE TO BE USED LATER */

    gu2_alpha_value = (unsigned long) value;

    /* SET GLOBAL FLAG */
    /* gfx2_* routines will use this flag to program alpha values */
    /* appropriately.  Normal gfx_* routines will always write    */
    /* the current ROP settings.  In this way, the alpha mode     */
    /* affects only second generation routines.                   */

    gu2_alpha_active = 1;

    switch (gu2_alpha_mode) {
    case ALPHA_MODE_BLEND:

        /* GENERATE 32-BIT VERSION OF RASTER MODE REGISTER */
        /* Pattern data is not involved in the operation.  */

        gu2_alpha32 = gu2_alpha_value | gu2_bpp;

        /* HANDLE SPECIAL CASES FOR ENDPOINTS              */
        /* The 8-bit hardware alpha value is always        */
        /* interpreted as a fraction.  Consequently, there */
        /* is no way to use values of 255 or 0 to exclude  */
        /* one of the inputs.                              */

        switch (gu2_alpha_value) {
            /* DESTINATION ONLY                               */
            /* Operation is alpha * A, where A is destination */
            /* and alpha is 1.                                */

        case 0:

            gu2_alpha32 |= MGP_RM_SELECT_ALPHA_1 |
                MGP_RM_ALPHA_TIMES_A |
                MGP_RM_ALPHA_TO_RGB | MGP_RM_DEST_FROM_CHAN_A;
            break;

            /* SOURCE ONLY                                    */
            /* Operation is alpha * A, where A is source and  */
            /* alpha is 1.                                    */

        case 255:

            gu2_alpha32 |= MGP_RM_SELECT_ALPHA_1 |
                MGP_RM_ALPHA_TO_RGB | MGP_RM_ALPHA_TIMES_A;
            break;

            /* DEFAULT                                        */
            /* Operation is alpha * A + (1 - alpha) * B;      */
            /* A is source, B is destination and alpha is the */
            /* programmed 8-bit value.                        */

        default:

            gu2_alpha32 |= MGP_RM_SELECT_ALPHA_R |
                MGP_RM_ALPHA_TO_RGB | MGP_RM_ALPHA_A_PLUS_BETA_B;

        }

        /* CHECK IF SOURCE INFORMATION SHOULD BE MERGED    */
        /* Alpha value of 0 indicates destination only.    */

        if (gu2_alpha_value != 0)
            gu2_alpha32 |= GFXsourceFlags;

        /* SET FLAG FOR DESTINATION DATA IF NECESSARY      */
        /* Alpha value of 255 indicates no destination     */

        if (gu2_alpha_value != 255) {
            gu2_alpha_blt_mode = MGP_BM_DST_REQ;
            gu2_alpha_vec_mode = MGP_VM_DST_REQ;
        }

        break;
    }
}

/*---------------------------------------------------------------------------
 * GFX2_PATTERN_FILL
 *
 * This routine is similar to the gfx_pattern_fill routine, but allows the
 * use of an arbitrary destination stride.  The rendering position is 
 * also specified as an offset instead of an (x,y) position.
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu22_pattern_fill(unsigned long dstoffset, unsigned short width,
                  unsigned short height)
#else
void
gfx2_pattern_fill(unsigned long dstoffset, unsigned short width,
                  unsigned short height)
#endif
{
    unsigned long size;

    size = (((unsigned long) width) << 16) | height;

    /* POLL UNTIL ABLE TO WRITE TO THE REGISTERS */
    /* Put off poll for as long as possible (do most calculations first). */

    GU2_WAIT_PENDING;
    WRITE_GP32(MGP_RASTER_MODE, gu2_rop32);
    WRITE_GP32(MGP_DST_OFFSET, dstoffset | gu2_pattern_origin);
    WRITE_GP32(MGP_WID_HEIGHT, size);
    WRITE_GP32(MGP_STRIDE, gu2_dst_pitch);
    WRITE_GP32(MGP_BLT_MODE, gu2_blt_mode | gu2_bm_throttle);
    gu2_bm_throttle = 0;
    gu2_vm_throttle = 0;
}

/*---------------------------------------------------------------------------
 * GFX2_COLOR_PATTERN_FILL
 *
 * This routine is used to render a rectangle using the current raster 
 * operation and the specified color pattern.  It allows an 8x8 color 
 * pattern to be rendered without multiple calls to the gfx_set_color_pattern
 * and gfx_pattern_fill routines.
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu22_color_pattern_fill(unsigned long dstoffset, unsigned short width,
                        unsigned short height, unsigned long *pattern)
#else
void
gfx2_color_pattern_fill(unsigned long dstoffset, unsigned short width,
                        unsigned short height, unsigned long *pattern)
#endif
{
    int pass;
    unsigned long lines, size, patxorigin, patoffset;

    /* ONLY USE HW PATTERN ORIGIN FOR THE X DIRECTION */
    /* Y direction handled by referencing proper location in pattern data. */

    patxorigin = (gu2_pattern_origin) & 0x1C000000;

    /* OVERRIDE PATTERN FLAGS IN ROP TO FORCE COLOR PATTERN */

    GU2_WAIT_PENDING;
    WRITE_GP32(MGP_RASTER_MODE,
               (gu2_rop32 & ~MGP_RM_PAT_FLAGS) | MGP_RM_PAT_COLOR);

    /* ATTEMPT TO OPTIMIZE */
    /* If possible, we can perform the pattern fill in only a few passes    */
    /* This is performed by multiplying the pitch by an appropriate amount. */
    /* Consequently, if the multiplied pitch exceeds 16 bits, this          */
    /* optimization is impossible.                                          */

    if ((gu2_dst_pitch << (gu2_xshift + 1)) <= 0xFFFF) {
        /* HANDLE VARIOUS COLOR DEPTHS DIFFERENTLY */

        switch (gu2_xshift) {
        case 0:                /* 8 BPP */

            /* TWO PASSES FOR 8 BPP */
            /* Render every other line per pass by doubling the pitch. */

            patoffset = (gu2_pattern_origin >> 28) & 0x0E;
            for (pass = 0; pass < 2; pass++) {
                /* CAN WRITE SOME PATTERN REGISTERS WHILE "PENDING" */

                GU2_WAIT_PENDING;
                WRITE_GP32(MGP_DST_OFFSET, dstoffset | patxorigin);
                lines = (height + 1 - pass) >> 1;
                if (!lines)
                    break;
                size = (((unsigned long) width) << 16) | lines;
                WRITE_GP32(MGP_WID_HEIGHT, size);
                WRITE_GP32(MGP_STRIDE, gu2_dst_pitch << 1);
                WRITE_GP32(MGP_PAT_DATA_1, BYTE_SWIZZLE(pattern[patoffset]));
                WRITE_GP32(MGP_PAT_DATA_0,
                           BYTE_SWIZZLE(pattern[patoffset + 1]));
                patoffset = (patoffset + 4) & 0x0E;
                WRITE_GP32(MGP_PAT_COLOR_1, BYTE_SWIZZLE(pattern[patoffset]));
                WRITE_GP32(MGP_PAT_COLOR_0,
                           BYTE_SWIZZLE(pattern[patoffset + 1]));
                patoffset = (patoffset + 4) & 0x0E;

                /* NEED TO WAIT UNTIL IDLE FOR COLORS 2 THROUGH 5 */
                /* Those registers are not pipelined. */

                GU2_WAIT_BUSY;
                WRITE_GP32(MGP_PAT_COLOR_3, BYTE_SWIZZLE(pattern[patoffset]));
                WRITE_GP32(MGP_PAT_COLOR_2,
                           BYTE_SWIZZLE(pattern[patoffset + 1]));
                patoffset = (patoffset + 4) & 0x0E;
                WRITE_GP32(MGP_PAT_COLOR_5, BYTE_SWIZZLE(pattern[patoffset]));
                WRITE_GP32(MGP_PAT_COLOR_4,
                           BYTE_SWIZZLE(pattern[patoffset + 1]));
                WRITE_GP16(MGP_BLT_MODE, gu2_blt_mode | gu2_bm_throttle);
                gu2_bm_throttle = 0;
                gu2_vm_throttle = 0;

                /* ADJUST FOR NEXT PASS */

                dstoffset += gu2_dst_pitch;
                patoffset = (patoffset + 6) & 0x0E;
            }
            break;

        case 1:                /* 12, 15, OR 16 BPP */

            /* FOUR PASSES FOR 16 BPP */
            /* Render every 4th line per pass by quadrupling the pitch. */

            patoffset = (gu2_pattern_origin >> 27) & 0x1C;
            for (pass = 0; pass < 4; pass++) {
                /* CAN WRITE SOME PATTERN REGISTERS WHILE "PENDING" */

                GU2_WAIT_PENDING;
                WRITE_GP32(MGP_DST_OFFSET, dstoffset | patxorigin);
                lines = (height + 3 - pass) >> 2;
                if (!lines)
                    break;
                size = (((unsigned long) width) << 16) | lines;
                WRITE_GP32(MGP_WID_HEIGHT, size);
                WRITE_GP32(MGP_STRIDE, gu2_dst_pitch << 2);
                WRITE_GP32(MGP_PAT_COLOR_1, WORD_SWIZZLE(pattern[patoffset]));
                WRITE_GP32(MGP_PAT_COLOR_0,
                           WORD_SWIZZLE(pattern[patoffset + 1]));
                WRITE_GP32(MGP_PAT_DATA_1,
                           WORD_SWIZZLE(pattern[patoffset + 2]));
                WRITE_GP32(MGP_PAT_DATA_0,
                           WORD_SWIZZLE(pattern[patoffset + 3]));
                patoffset = (patoffset + 16) & 0x1C;

                /* NEED TO WAIT UNTIL IDLE FOR COLORS 2 THROUGH 5 */
                /* Those registers are not pipelined. */

                GU2_WAIT_BUSY;
                WRITE_GP32(MGP_PAT_COLOR_5, WORD_SWIZZLE(pattern[patoffset]));
                WRITE_GP32(MGP_PAT_COLOR_4,
                           WORD_SWIZZLE(pattern[patoffset + 1]));
                WRITE_GP32(MGP_PAT_COLOR_3,
                           WORD_SWIZZLE(pattern[patoffset + 2]));
                WRITE_GP32(MGP_PAT_COLOR_2,
                           WORD_SWIZZLE(pattern[patoffset + 3]));
                WRITE_GP16(MGP_BLT_MODE, gu2_blt_mode | gu2_bm_throttle);
                gu2_bm_throttle = 0;
                gu2_vm_throttle = 0;

                /* ADJUST FOR NEXT PASS */

                dstoffset += gu2_dst_pitch;
                patoffset = (patoffset + 20) & 0x1C;
            }
            break;

        case 2:                /* 32 BPP */

            /* EIGHT PASSES FOR 32 BPP */
            /* Render every 8th line per pass by setting pitch * 8. */

            patoffset = (gu2_pattern_origin >> 26) & 0x38;
            for (pass = 0; pass < 8; pass++) {
                /* CAN WRITE SOME PATTERN REGISTERS WHILE "PENDING" */

                GU2_WAIT_PENDING;
                WRITE_GP32(MGP_DST_OFFSET, dstoffset | patxorigin);
                lines = (height + 7 - pass) >> 3;
                if (!lines)
                    break;
                size = (((unsigned long) width) << 16) | lines;
                WRITE_GP32(MGP_WID_HEIGHT, size);
                WRITE_GP32(MGP_STRIDE, gu2_dst_pitch << 3);
                WRITE_GP32(MGP_PAT_COLOR_1, pattern[patoffset + 4]);
                WRITE_GP32(MGP_PAT_COLOR_0, pattern[patoffset + 5]);
                WRITE_GP32(MGP_PAT_DATA_1, pattern[patoffset + 6]);
                WRITE_GP32(MGP_PAT_DATA_0, pattern[patoffset + 7]);

                /* NEED TO WAIT UNTIL IDLE FOR COLORS 2 THROUGH 5 */
                /* Those registers are not pipelined. */

                GU2_WAIT_BUSY;
                WRITE_GP32(MGP_PAT_COLOR_5, pattern[patoffset]);
                WRITE_GP32(MGP_PAT_COLOR_4, pattern[patoffset + 1]);
                WRITE_GP32(MGP_PAT_COLOR_3, pattern[patoffset + 2]);
                WRITE_GP32(MGP_PAT_COLOR_2, pattern[patoffset + 3]);
                WRITE_GP16(MGP_BLT_MODE, gu2_blt_mode | gu2_bm_throttle);
                gu2_bm_throttle = 0;
                gu2_vm_throttle = 0;

                /* ADJUST FOR NEXT PASS */

                dstoffset += gu2_dst_pitch;
                patoffset = (patoffset + 8) & 0x38;
            }
            break;
        }
    }

    else {
        WRITE_GP32(MGP_STRIDE, gu2_dst_pitch);

        switch (gu2_xshift) {
        case 0:                /* 8 BPP - 4 LINES PER PASS */

            patoffset = (gu2_pattern_origin >> 28) & 0x0E;
            while (height) {
                lines = height > 4 ? 4 : height;

                /* CAN WRITE SOME REGISTERS WHILE PENDING */

                WRITE_GP32(MGP_DST_OFFSET, dstoffset | patxorigin);
                WRITE_GP32(MGP_WID_HEIGHT,
                           (((unsigned long) width) << 16) | lines);
                WRITE_GP32(MGP_PAT_DATA_1, BYTE_SWIZZLE(pattern[patoffset]));
                WRITE_GP32(MGP_PAT_DATA_0,
                           BYTE_SWIZZLE(pattern[patoffset + 1]));
                patoffset = (patoffset + 2) & 0x0E;
                WRITE_GP32(MGP_PAT_COLOR_1, BYTE_SWIZZLE(pattern[patoffset]));
                WRITE_GP32(MGP_PAT_COLOR_0,
                           BYTE_SWIZZLE(pattern[patoffset + 1]));
                patoffset = (patoffset + 2) & 0x0E;

                /* NEED TO WAIT UNTIL IDLE FOR COLORS 2 THROUGH 5 */
                /* Those registers are not pipelined.             */

                GU2_WAIT_BUSY;
                WRITE_GP32(MGP_PAT_COLOR_3, BYTE_SWIZZLE(pattern[patoffset]));
                WRITE_GP32(MGP_PAT_COLOR_2,
                           BYTE_SWIZZLE(pattern[patoffset + 1]));
                patoffset = (patoffset + 2) & 0x0E;
                WRITE_GP32(MGP_PAT_COLOR_5, BYTE_SWIZZLE(pattern[patoffset]));
                WRITE_GP32(MGP_PAT_COLOR_4,
                           BYTE_SWIZZLE(pattern[patoffset + 1]));
                patoffset = (patoffset + 2) & 0x0E;
                WRITE_GP16(MGP_BLT_MODE, gu2_blt_mode | gu2_bm_throttle);

                /* ADJUST FOR NEXT PASS */

                dstoffset += gu2_dst_pitch << 2;
                height -= (unsigned short) lines;
            }
            break;

        case 1:                /* 12, 15 AND 16 BPP - 2 LINES PER PASS */

            patoffset = (gu2_pattern_origin >> 27) & 0x1C;
            while (height) {
                lines = height > 2 ? 2 : height;

                /* CAN WRITE SOME REGISTERS WHILE PENDING */

                WRITE_GP32(MGP_DST_OFFSET, dstoffset | patxorigin);
                WRITE_GP32(MGP_WID_HEIGHT,
                           (((unsigned long) width) << 16) | lines);
                WRITE_GP32(MGP_PAT_COLOR_1, WORD_SWIZZLE(pattern[patoffset]));
                WRITE_GP32(MGP_PAT_COLOR_0,
                           WORD_SWIZZLE(pattern[patoffset + 1]));
                WRITE_GP32(MGP_PAT_DATA_1,
                           WORD_SWIZZLE(pattern[patoffset + 2]));
                WRITE_GP32(MGP_PAT_DATA_0,
                           WORD_SWIZZLE(pattern[patoffset + 3]));
                patoffset = (patoffset + 4) & 0x1C;

                /* NEED TO WAIT UNTIL IDLE FOR COLORS 2 THROUGH 5 */
                /* Those registers are not pipelined.             */

                GU2_WAIT_BUSY;
                WRITE_GP32(MGP_PAT_COLOR_5, WORD_SWIZZLE(pattern[patoffset]));
                WRITE_GP32(MGP_PAT_COLOR_4,
                           WORD_SWIZZLE(pattern[patoffset + 1]));
                WRITE_GP32(MGP_PAT_COLOR_3,
                           WORD_SWIZZLE(pattern[patoffset + 2]));
                WRITE_GP32(MGP_PAT_COLOR_2,
                           WORD_SWIZZLE(pattern[patoffset + 3]));
                patoffset = (patoffset + 4) & 0x1C;
                WRITE_GP16(MGP_BLT_MODE, gu2_blt_mode | gu2_bm_throttle);

                /* ADJUST FOR NEXT PASS */

                dstoffset += gu2_dst_pitch << 1;
                height -= (unsigned short) lines;
            }
            break;

        case 2:                /* 32 BPP - 1 LINE PER PASS */

            patoffset = (gu2_pattern_origin >> 26) & 0x38;
            while (height) {
                /* CAN WRITE SOME REGISTERS WHILE PENDING */

                WRITE_GP32(MGP_DST_OFFSET, dstoffset | patxorigin);
                WRITE_GP32(MGP_WID_HEIGHT,
                           (((unsigned long) width) << 16) | 1l);
                WRITE_GP32(MGP_PAT_COLOR_1, pattern[patoffset + 4]);
                WRITE_GP32(MGP_PAT_COLOR_0, pattern[patoffset + 5]);
                WRITE_GP32(MGP_PAT_DATA_1, pattern[patoffset + 6]);
                WRITE_GP32(MGP_PAT_DATA_0, pattern[patoffset + 7]);

                /* NEED TO WAIT UNTIL IDLE FOR COLORS 2 THROUGH 5 */
                /* Those registers are not pipelined.             */

                GU2_WAIT_BUSY;
                WRITE_GP32(MGP_PAT_COLOR_5, pattern[patoffset]);
                WRITE_GP32(MGP_PAT_COLOR_4, pattern[patoffset + 1]);
                WRITE_GP32(MGP_PAT_COLOR_3, pattern[patoffset + 2]);
                WRITE_GP32(MGP_PAT_COLOR_2, pattern[patoffset + 3]);
                patoffset = (patoffset + 8) & 0x38;
                WRITE_GP16(MGP_BLT_MODE, gu2_blt_mode | gu2_bm_throttle);

                /* ADJUST FOR NEXT PASS */

                dstoffset += gu2_dst_pitch;
                height--;
            }
            break;
        }

    }

}

/*---------------------------------------------------------------------------
 * GFX2_SCREEN_TO_SCREEN_BLT
 *
 * This routine is similar to the gfx_screen_to_screen_blt routine but
 * allows the use of arbitrary source and destination strides and alpha
 * blending.  It also allows the use of an arbitrary ROP with transparency.
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu22_screen_to_screen_blt(unsigned long srcoffset, unsigned long dstoffset,
                          unsigned short width, unsigned short height,
                          int flags)
#else
void
gfx2_screen_to_screen_blt(unsigned long srcoffset, unsigned long dstoffset,
                          unsigned short width, unsigned short height,
                          int flags)
#endif
{
    unsigned long size, xbytes;
    unsigned short blt_mode;

    size = (((unsigned long) width) << 16) | height;

    /* USE ALPHA SETTINGS, IF REQUESTED */

    if (gu2_alpha_active)
        blt_mode = gu2_alpha_blt_mode | MGP_BM_SRC_FB;

    else
        blt_mode = (gu2_blt_mode & ~MGP_BM_SRC_TYPE_MASK) | MGP_BM_SRC_FB;

    /* CALCULATE THE DIRECTION OF THE BLT */
    /* Using offsets, so flags from the calling routine are needed. */

    if (flags & 1) {
        xbytes = (width - 1) << gu2_xshift;
        srcoffset += xbytes;
        dstoffset += xbytes;
        blt_mode |= MGP_BM_NEG_XDIR;
    }
    if (flags & 2) {
        srcoffset += (height - 1) * gu2_src_pitch;
        dstoffset += (height - 1) * gu2_dst_pitch;
        blt_mode |= MGP_BM_NEG_YDIR;
    }

    /* TURN INTO BYTE ADDRESS IF NEGATIVE X DIRECTION */
    /* This is a quirk of the hardware. */

    if (blt_mode & MGP_BM_NEG_XDIR) {
        srcoffset += (1 << gu2_xshift) - 1;
        dstoffset += (1 << gu2_xshift) - 1;
    }

    /* POLL UNTIL ABLE TO WRITE TO THE REGISTERS */
    /* Put off poll for as long as possible (do most calculations first). */

    GU2_WAIT_PENDING;

    if (gu2_alpha_active) {
        WRITE_GP32(MGP_RASTER_MODE, gu2_alpha32);
    }
    else {
        WRITE_GP32(MGP_RASTER_MODE, gu2_rop32 | GFXsourceFlags);
    }

    WRITE_GP32(MGP_SRC_OFFSET, srcoffset);
    WRITE_GP32(MGP_DST_OFFSET, dstoffset | gu2_pattern_origin);
    WRITE_GP32(MGP_WID_HEIGHT, size);
    WRITE_GP32(MGP_STRIDE, gu2_dst_pitch | (gu2_src_pitch << 16));
    WRITE_GP16(MGP_BLT_MODE, blt_mode | gu2_bm_throttle);
    gu2_bm_throttle = 0;
    gu2_vm_throttle = 0;
}

/*---------------------------------------------------------------------------
 * GFX2_MONO_EXPAND_BLT
 *
 * This routine is similar to the gfx2_screen_to_screen_blt routine but
 * expands monochrome data stored in graphics memory.  
 * WARNING: This routine assumes that the regions in graphics memory 
 * will not overlap, and therefore does not check the BLT direction.
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu22_mono_expand_blt(unsigned long srcbase, unsigned short srcx,
                     unsigned short srcy, unsigned long dstoffset,
                     unsigned short width, unsigned short height,
                     int byte_packed)
#else
void
gfx2_mono_expand_blt(unsigned long srcbase, unsigned short srcx,
                     unsigned short srcy, unsigned long dstoffset,
                     unsigned short width, unsigned short height,
                     int byte_packed)
#endif
{
    unsigned long size, srcoffset;
    unsigned short blt_mode;

    size = (((unsigned long) width) << 16) | height;

    /* CALCULATE SOURCE OFFSET */

    srcoffset = srcbase + (unsigned long) srcy *gu2_src_pitch;

    srcoffset += srcx >> 3;
    srcoffset |= ((unsigned long) srcx & 7) << 26;

    /* POLL UNTIL ABLE TO WRITE TO THE REGISTERS */
    /* Put off poll for as long as possible (do most calculations first). */

    GU2_WAIT_PENDING;

    if (gu2_alpha_active) {
        blt_mode = gu2_alpha_blt_mode;

        WRITE_GP32(MGP_RASTER_MODE, gu2_alpha32);
    }
    else {
        blt_mode = (gu2_blt_mode & ~MGP_BM_SRC_TYPE_MASK);

        WRITE_GP32(MGP_RASTER_MODE, gu2_rop32 | GFXsourceFlags);
    }

    if (byte_packed)
        blt_mode |= MGP_BM_SRC_FB | MGP_BM_SRC_BP_MONO | gu2_bm_throttle;
    else
        blt_mode |= MGP_BM_SRC_FB | MGP_BM_SRC_MONO | gu2_bm_throttle;

    WRITE_GP32(MGP_SRC_OFFSET, srcoffset);
    WRITE_GP32(MGP_DST_OFFSET, dstoffset | gu2_pattern_origin);
    WRITE_GP32(MGP_WID_HEIGHT, size);
    WRITE_GP32(MGP_STRIDE, gu2_dst_pitch | (gu2_src_pitch << 16));
    WRITE_GP16(MGP_BLT_MODE, blt_mode);
    gu2_bm_throttle = 0;
    gu2_vm_throttle = 0;
}

/*---------------------------------------------------------------------------
 * GFX2_COLOR_BITMAP_TO_SCREEN_BLT	
 *
 * This routine is similar to the gfx_color_bitmap_to_screen_blt routine 
 * but allows the use of an arbitrary destination stride and alpha blending.
 * It also allows the use of an arbitrary ROP with transparency.
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu22_color_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
                                unsigned long dstoffset, unsigned short width,
                                unsigned short height, unsigned char *data,
                                short pitch)
#else
void
gfx2_color_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
                                unsigned long dstoffset, unsigned short width,
                                unsigned short height, unsigned char *data,
                                short pitch)
#endif
{
    unsigned long size, bytes;
    unsigned long offset, temp_offset;
    unsigned long srcoffset, dword_bytes, bytes_extra;
    unsigned short blt_mode;

    size = (((unsigned long) width) << 16) | 1;

    /* CALCULATE STARTING OFFSETS */

    offset = (unsigned long) srcy *pitch + ((unsigned long) srcx << gu2_xshift);

    dstoffset |= gu2_pattern_origin;

    bytes = width << gu2_xshift;
    dword_bytes = bytes & ~0x3L;
    bytes_extra = bytes & 0x3L;

    /* POLL UNTIL ABLE TO WRITE TO THE REGISTERS */
    /* Put off poll for as long as possible (do most calculations first).   */
    /* The source offset is always 0 since we allow misaligned dword reads. */
    /* We must wait for BLT busy because the GP may be executing a screen   */
    /* to screen BLT from the scratchpad area.                              */

    GU2_WAIT_BUSY;

    if (gu2_alpha_active) {
        blt_mode = gu2_alpha_blt_mode;

        WRITE_GP32(MGP_RASTER_MODE, gu2_alpha32);
    }
    else {
        blt_mode = (gu2_blt_mode & ~MGP_BM_SRC_TYPE_MASK);

        WRITE_GP32(MGP_RASTER_MODE, gu2_rop32 | GFXsourceFlags);
    }
    blt_mode |= MGP_BM_SRC_FB | gu2_bm_throttle;
    gu2_bm_throttle = 0;
    gu2_vm_throttle = 0;

    WRITE_GP32(MGP_WID_HEIGHT, size);

    /* WRITE DATA ONE LINE AT A TIME */
    /* For speed reasons, data is written to an offscreen scratch area and 
     * then BLTed using a screen to screen BLT. This is similar to the GX1 BLT
     * buffers, but slightly more efficient in that we can queue up data while
     * the GP is rendering a line.
     * */

    while (height--) {
        temp_offset = offset;
        srcoffset = gfx_gx2_scratch_base;
        if (gu2_current_line)
            srcoffset += 8192;

        GU2_WAIT_PENDING;
        WRITE_GP32(MGP_SRC_OFFSET, srcoffset);
        WRITE_GP32(MGP_DST_OFFSET, dstoffset);
        dstoffset += gu2_dst_pitch;
        dstoffset += 0x20000000;

        WRITE_FRAME_BUFFER_STRING32(srcoffset, dword_bytes, data, temp_offset);
        if (bytes_extra) {
            temp_offset += dword_bytes;
            srcoffset += dword_bytes;
            WRITE_FRAME_BUFFER_STRING8(srcoffset, bytes_extra, data,
                                       temp_offset);
        }
        WRITE_GP16(MGP_BLT_MODE, blt_mode);
        offset += pitch;
        gu2_current_line = 1 - gu2_current_line;
    }
}

/*---------------------------------------------------------------------------
 * GFX2_TEXT_BLT	
 *
 * This routine is similar to the gfx2_mono_bitmap_to_screen_blt routine 
 * but assumes that source data is byte-packed.
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu22_text_blt(unsigned long dstoffset, unsigned short width,
              unsigned short height, unsigned char *data)
#else
void
gfx2_text_blt(unsigned long dstoffset, unsigned short width,
              unsigned short height, unsigned char *data)
#endif
{
    unsigned long size, bytes;
    unsigned long temp1 = 0, temp2 = 0, temp_offset = 0;
    unsigned long i, j = 0, fifo_lines, dwords_extra, bytes_extra;
    unsigned long shift;
    unsigned short blt_mode;

    size = (((unsigned long) width) << 16) | height;

    /* CALCULATE STARTING OFFSETS */

    bytes = ((width + 7) >> 3) * height;
    fifo_lines = bytes >> 5;
    dwords_extra = (bytes & 0x0000001Cl) >> 2;
    bytes_extra = bytes & 0x00000003l;

    /* POLL UNTIL ABLE TO WRITE TO THE REGISTERS */

    GU2_WAIT_PENDING;

    if (gu2_alpha_active) {
        blt_mode = gu2_alpha_blt_mode;

        WRITE_GP32(MGP_RASTER_MODE, gu2_alpha32);
    }
    else {
        blt_mode = (gu2_blt_mode & ~MGP_BM_SRC_TYPE_MASK);

        WRITE_GP32(MGP_RASTER_MODE, gu2_rop32 | GFXsourceFlags);
    }

    WRITE_GP32(MGP_SRC_OFFSET, 0);
    WRITE_GP32(MGP_DST_OFFSET, dstoffset | gu2_pattern_origin);
    WRITE_GP32(MGP_WID_HEIGHT, size);
    WRITE_GP32(MGP_STRIDE, gu2_dst_pitch);
    WRITE_GP16(MGP_BLT_MODE, blt_mode | MGP_BM_SRC_HOST |
               MGP_BM_SRC_BP_MONO | gu2_bm_throttle);
    gu2_bm_throttle = 0;
    gu2_vm_throttle = 0;

    /* WAIT FOR BLT TO BE LATCHED */

    GU2_WAIT_PENDING;

    /* WRITE ALL FULL FIFO LINES */

    for (i = 0; i < fifo_lines; i++) {
        GU2_WAIT_HALF_EMPTY;
        WRITE_GPREG_STRING32(MGP_HST_SOURCE, 8, j, data, temp_offset, temp1);
        temp_offset += 32;
    }

    /* WRITE ALL FULL DWORDS */

    if (dwords_extra || bytes_extra) {
        GU2_WAIT_HALF_EMPTY;
        if (dwords_extra) {
            WRITE_GPREG_STRING32(MGP_HST_SOURCE, dwords_extra, i, data,
                                 temp_offset, temp1);
            temp_offset += (dwords_extra << 2);
        }
        if (bytes_extra) {
            shift = 0;
            WRITE_GPREG_STRING8(MGP_HST_SOURCE, bytes_extra, shift, i, data,
                                temp_offset, temp1, temp2);
        }
    }
}

/*---------------------------------------------------------------------------
 * GFX2_MONO_BITMAP_TO_SCREEN_BLT	
 *
 * This routine is similar to the gfx_mono_bitmap_to_screen_blt routine 
 * but allows the use of an arbitrary destination stride and alpha blending.
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu22_mono_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
                               unsigned long dstoffset, unsigned short width,
                               unsigned short height, unsigned char *data,
                               short pitch)
#else
void
gfx2_mono_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
                               unsigned long dstoffset, unsigned short width,
                               unsigned short height, unsigned char *data,
                               short pitch)
#endif
{
    unsigned long size, bytes;
    unsigned long offset, temp_offset, temp1 = 0, temp2 = 0;
    unsigned long i, j = 0, fifo_lines, dwords_extra, bytes_extra;
    unsigned long shift = 0;
    unsigned short blt_mode;

    size = (((unsigned long) width) << 16) | height;

    /* CALCULATE STARTING OFFSETS */

    offset = (unsigned long) srcy *pitch + ((unsigned long) srcx >> 3);

    bytes = ((srcx & 7) + width + 7) >> 3;
    fifo_lines = bytes >> 5;
    dwords_extra = (bytes & 0x0000001Cl) >> 2;
    bytes_extra = bytes & 0x00000003l;

    /* POLL UNTIL ABLE TO WRITE TO THE REGISTERS */

    GU2_WAIT_PENDING;

    if (gu2_alpha_active) {
        blt_mode = gu2_alpha_blt_mode;

        WRITE_GP32(MGP_RASTER_MODE, gu2_alpha32);
    }
    else {
        blt_mode = (gu2_blt_mode & ~MGP_BM_SRC_TYPE_MASK);

        WRITE_GP32(MGP_RASTER_MODE, gu2_rop32 | GFXsourceFlags);
    }

    WRITE_GP32(MGP_SRC_OFFSET, ((unsigned long) srcx & 7) << 26);
    WRITE_GP32(MGP_DST_OFFSET, dstoffset | gu2_pattern_origin);
    WRITE_GP32(MGP_WID_HEIGHT, size);
    WRITE_GP32(MGP_STRIDE, gu2_dst_pitch);
    WRITE_GP16(MGP_BLT_MODE, blt_mode | MGP_BM_SRC_HOST |
               MGP_BM_SRC_MONO | gu2_bm_throttle);
    gu2_bm_throttle = 0;
    gu2_vm_throttle = 0;

    /* WAIT FOR BLT TO BE LATCHED */

    GU2_WAIT_PENDING;

    /* WRITE ALL OF THE DATA TO THE HOST SOURCE REGISTER */

    while (height--) {
        temp_offset = offset;

        /* WRITE ALL FULL FIFO LINES */

        for (i = 0; i < fifo_lines; i++) {
            GU2_WAIT_HALF_EMPTY;
            WRITE_GPREG_STRING32(MGP_HST_SOURCE, 8, j, data, temp_offset,
                                 temp1);
            temp_offset += 32;
        }

        /* WRITE ALL FULL DWORDS */

        GU2_WAIT_HALF_EMPTY;
        if (dwords_extra)
            WRITE_GPREG_STRING32(MGP_HST_SOURCE, dwords_extra, i, data,
                                 temp_offset, temp1);
        temp_offset += (dwords_extra << 2);

        /* WRITE REMAINING BYTES */

        shift = 0;
        if (bytes_extra)
            WRITE_GPREG_STRING8(MGP_HST_SOURCE, bytes_extra, shift, i, data,
                                temp_offset, temp1, temp2);

        offset += pitch;
    }
}

/*---------------------------------------------------------------------------
 * GFX2_BRESENHAM_LINE
 *
 * This routine is similar to the gfx_bresenam_line routine but allows 
 * the use of an arbitrary destination stride.
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu22_bresenham_line(unsigned long dstoffset,
                    unsigned short length, unsigned short initerr,
                    unsigned short axialerr, unsigned short diagerr,
                    unsigned short flags)
#else
void
gfx2_bresenham_line(unsigned long dstoffset,
                    unsigned short length, unsigned short initerr,
                    unsigned short axialerr, unsigned short diagerr,
                    unsigned short flags)
#endif
{
    unsigned long vector_mode = gu2_vector_mode | flags;
    unsigned long data1 = (((unsigned long) axialerr) << 16) | diagerr;
    unsigned long data2 = (((unsigned long) length) << 16) | initerr;

    /* CHECK NULL LENGTH */

    if (!length)
        return;

    /* POLL UNTIL ABLE TO WRITE TO THE REGISTERS */
    /* Put off poll for as long as possible (do most calculations first). */

    GU2_WAIT_PENDING;

    if (gu2_alpha_active) {
        vector_mode = gu2_alpha_vec_mode | flags;

        WRITE_GP32(MGP_RASTER_MODE, gu2_alpha32);
    }
    else
        WRITE_GP32(MGP_RASTER_MODE, gu2_rop32);

    WRITE_GP32(MGP_DST_OFFSET, dstoffset | gu2_pattern_origin);
    WRITE_GP32(MGP_VEC_ERR, data1);
    WRITE_GP32(MGP_VEC_LEN, data2);
    WRITE_GP32(MGP_STRIDE, gu2_dst_pitch);
    WRITE_GP32(MGP_VECTOR_MODE, vector_mode | gu2_vm_throttle);
    gu2_bm_throttle = 0;
    gu2_vm_throttle = 0;
}

/*---------------------------------------------------------------------------
 * GFX2_SYNC_TO_VBLANK
 *
 * This routine sets a flag to synchronize the next rendering routine to 
 * VBLANK.  The flag is cleared by the rendering routine.
 *---------------------------------------------------------------------------
 */
#if GFX_2DACCEL_DYNAMIC
void
gu22_sync_to_vblank(void)
#else
void
gfx2_sync_to_vblank(void)
#endif
{
    /* SET FLAGS TO THROTTLE NEXT RENDERING ROUTINE */

    gu2_bm_throttle = MGP_BM_THROTTLE;
    gu2_vm_throttle = MGP_VM_THROTTLE;
}

/* END OF FILE */
