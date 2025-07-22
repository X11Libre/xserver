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
 * This file contains routines to program the 2D acceleration hardware:
 * 
 *    gfx_set_bpp
 *    gfx_set_solid_pattern
 *    gfx_set_mono_pattern
 *    gfx_set_color_pattern
 *    gfx_load_color_pattern_line
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
 * */

/* STATIC VARIABLES */

unsigned short GFXbpp = 16;
unsigned short GFXbb0Base = 0x400;
unsigned short GFXbb1Base = 0x930;
unsigned short GFXbufferWidthPixels = 400;

unsigned short GFXpatternFlags = 0;
unsigned short GFXsourceFlags = 0;
unsigned long GFXsavedColor = 0;
unsigned short GFXsavedRop = 0;
unsigned short GFXusesDstData = 0;

/* INCLUDE SUPPORT FOR FIRST GENERATION, IF SPECIFIED. */

#if GFX_2DACCEL_GU1
#include "rndr_gu1.c"
#endif

/* INCLUDE SUPPORT FOR SECOND GENERATION, IF SPECIFIED. */

#if GFX_2DACCEL_GU2
#include "rndr_gu2.c"
#endif

/* WRAPPERS IF DYNAMIC SELECTION */
/* Extra layer to call either first or second generation routines. */

#if GFX_2DACCEL_DYNAMIC

/*---------------------------------------------------------------------------
 * gfx_reset_pitch (PRIVATE ROUTINE - NOT PART OF API)
 *---------------------------------------------------------------------------
 */
void
gfx_reset_pitch(unsigned short pitch)
{
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu2_reset_pitch(pitch);
#endif
}

/*---------------------------------------------------------------------------
 * gfx_set_bpp
 *---------------------------------------------------------------------------
 */
void
gfx_set_bpp(unsigned short bpp)
{
#if GFX_2DACCEL_GU1
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
        gu1_set_bpp(bpp);
#endif
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu2_set_bpp(bpp);
#endif
}

/*---------------------------------------------------------------------------
 * gfx_set_solid_source
 *---------------------------------------------------------------------------
 */
void
gfx_set_solid_source(unsigned long color)
{
#if GFX_2DACCEL_GU1
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
        gu1_set_solid_source(color);
#endif
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu2_set_solid_source(color);
#endif
}

/*---------------------------------------------------------------------------
 * gfx_set_mono_source
 *---------------------------------------------------------------------------
 */
void
gfx_set_mono_source(unsigned long bgcolor, unsigned long fgcolor,
                    unsigned short transparent)
{
#if GFX_2DACCEL_GU1
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
        gu1_set_mono_source(bgcolor, fgcolor, transparent);
#endif
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu2_set_mono_source(bgcolor, fgcolor, transparent);
#endif
}

void
gfx_set_pattern_flags(unsigned short flags)
{
    GFXpatternFlags |= flags;
}

/*---------------------------------------------------------------------------
 * gfx_set_solid_pattern
 *---------------------------------------------------------------------------
 */
void
gfx_set_solid_pattern(unsigned long color)
{
#if GFX_2DACCEL_GU1
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
        gu1_set_solid_pattern(color);
#endif
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu2_set_solid_pattern(color);
#endif
}

/*---------------------------------------------------------------------------
 * gfx_set_mono_pattern
 *---------------------------------------------------------------------------
 */
void
gfx_set_mono_pattern(unsigned long bgcolor, unsigned long fgcolor,
                     unsigned long data0, unsigned long data1,
                     unsigned char transparent)
{
#if GFX_2DACCEL_GU1
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
        gu1_set_mono_pattern(bgcolor, fgcolor, data0, data1, transparent);
#endif
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu2_set_mono_pattern(bgcolor, fgcolor, data0, data1, transparent);
#endif
}

/*---------------------------------------------------------------------------
 * gfx_set_color_pattern
 *---------------------------------------------------------------------------
 */
void
gfx_set_color_pattern(unsigned long bgcolor, unsigned long fgcolor,
                      unsigned long data0, unsigned long data1,
                      unsigned long data2, unsigned long data3,
                      unsigned char transparent)
{
#if GFX_2DACCEL_GU1
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
        gu1_set_color_pattern(bgcolor, fgcolor, data0, data1, data2, data3,
                              transparent);
#endif
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu2_set_color_pattern(bgcolor, fgcolor, data0, data1, data2, data3,
                              transparent);
#endif
}

/*---------------------------------------------------------------------------
 * gfx_load_color_pattern_line
 *---------------------------------------------------------------------------
 */
void
gfx_load_color_pattern_line(short y, unsigned long *pattern_8x8)
{
#if GFX_2DACCEL_GU1
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
        gu1_load_color_pattern_line(y, pattern_8x8);
#endif
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu2_load_color_pattern_line(y, pattern_8x8);
#endif
}

/*---------------------------------------------------------------------------
 * gfx_set_raster_operation
 *---------------------------------------------------------------------------
 */
void
gfx_set_raster_operation(unsigned char rop)
{
#if GFX_2DACCEL_GU1
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
        gu1_set_raster_operation(rop);
#endif
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu2_set_raster_operation(rop);
#endif
}

/*---------------------------------------------------------------------------
 * gfx_pattern_fill
 *---------------------------------------------------------------------------
 */
void
gfx_pattern_fill(unsigned short x, unsigned short y,
                 unsigned short width, unsigned short height)
{
#if GFX_2DACCEL_GU1
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
        gu1_pattern_fill(x, y, width, height);
#endif
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu2_pattern_fill(x, y, width, height);
#endif
}

/*---------------------------------------------------------------------------
 * gfx_color_pattern_fill
 *---------------------------------------------------------------------------
 */
void
gfx_color_pattern_fill(unsigned short x, unsigned short y,
                       unsigned short width, unsigned short height,
                       unsigned long *pattern)
{
#if GFX_2DACCEL_GU1
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
        gu1_color_pattern_fill(x, y, width, height, pattern);
#endif
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu2_color_pattern_fill(x, y, width, height, pattern);
#endif
}

/*---------------------------------------------------------------------------
 * gfx_screen_to_screen_blt
 *---------------------------------------------------------------------------
 */
void
gfx_screen_to_screen_blt(unsigned short srcx, unsigned short srcy,
                         unsigned short dstx, unsigned short dsty,
                         unsigned short width, unsigned short height)
{
#if GFX_2DACCEL_GU1
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
        gu1_screen_to_screen_blt(srcx, srcy, dstx, dsty, width, height);
#endif
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu2_screen_to_screen_blt(srcx, srcy, dstx, dsty, width, height);
#endif
}

/*---------------------------------------------------------------------------
 * gfx_screen_to_screen_xblt
 *---------------------------------------------------------------------------
 */
void
gfx_screen_to_screen_xblt(unsigned short srcx, unsigned short srcy,
                          unsigned short dstx, unsigned short dsty,
                          unsigned short width, unsigned short height,
                          unsigned long color)
{
#if GFX_2DACCEL_GU1
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
        gu1_screen_to_screen_xblt(srcx, srcy, dstx, dsty, width, height, color);
#endif
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu2_screen_to_screen_xblt(srcx, srcy, dstx, dsty, width, height, color);
#endif
}

/*---------------------------------------------------------------------------
 * gfx_color_bitmap_to_screen_blt
 *---------------------------------------------------------------------------
 */
void
gfx_color_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
                               unsigned short dstx, unsigned short dsty,
                               unsigned short width, unsigned short height,
                               unsigned char *data, long pitch)
{
#if GFX_2DACCEL_GU1
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
        gu1_color_bitmap_to_screen_blt(srcx, srcy, dstx, dsty, width, height,
                                       data, pitch);
#endif
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu2_color_bitmap_to_screen_blt(srcx, srcy, dstx, dsty, width, height,
                                       data, pitch);
#endif
}

/*---------------------------------------------------------------------------
 * gfx_color_bitmap_to_screen_xblt
 *---------------------------------------------------------------------------
 */
void
gfx_color_bitmap_to_screen_xblt(unsigned short srcx, unsigned short srcy,
                                unsigned short dstx, unsigned short dsty,
                                unsigned short width, unsigned short height,
                                unsigned char *data, long pitch,
                                unsigned long color)
{
#if GFX_2DACCEL_GU1
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
        gu1_color_bitmap_to_screen_xblt(srcx, srcy, dstx, dsty, width, height,
                                        data, pitch, color);
#endif
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu2_color_bitmap_to_screen_xblt(srcx, srcy, dstx, dsty, width, height,
                                        data, pitch, color);
#endif
}

/*---------------------------------------------------------------------------
 * gfx_mono_bitmap_to_screen_blt
 *---------------------------------------------------------------------------
 */
void
gfx_mono_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
                              unsigned short dstx, unsigned short dsty,
                              unsigned short width, unsigned short height,
                              unsigned char *data, short pitch)
{
#if GFX_2DACCEL_GU1
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
        gu1_mono_bitmap_to_screen_blt(srcx, srcy, dstx, dsty, width, height,
                                      data, pitch);
#endif
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu2_mono_bitmap_to_screen_blt(srcx, srcy, dstx, dsty, width, height,
                                      data, pitch);
#endif
}

/*---------------------------------------------------------------------------
 * gfx_text_blt
 *---------------------------------------------------------------------------
 */
void
gfx_text_blt(unsigned short dstx, unsigned short dsty, unsigned short width,
             unsigned short height, unsigned char *data)
{
#if GFX_2DACCEL_GU1
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
        gu1_text_blt(dstx, dsty, width, height, data);
#endif
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu2_text_blt(dstx, dsty, width, height, data);
#endif
}

/*---------------------------------------------------------------------------
 * gfx_bresenham_line
 *---------------------------------------------------------------------------
 */
void
gfx_bresenham_line(unsigned short x, unsigned short y,
                   unsigned short length, unsigned short initerr,
                   unsigned short axialerr, unsigned short diagerr,
                   unsigned short flags)
{
#if GFX_2DACCEL_GU1
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
        gu1_bresenham_line(x, y, length, initerr, axialerr, diagerr, flags);
#endif
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu2_bresenham_line(x, y, length, initerr, axialerr, diagerr, flags);
#endif
}

/*---------------------------------------------------------------------------
 * gfx_wait_until_idle
 *---------------------------------------------------------------------------
 */
void
gfx_wait_until_idle(void)
{
#if GFX_2DACCEL_GU1
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
        gu1_wait_until_idle();
#endif
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu2_wait_until_idle();
#endif
}

/*---------------------------------------------------------------------------
 * gfx_test_blt_pending
 *---------------------------------------------------------------------------
 */
int
gfx_test_blt_pending(void)
{
    int retval = 0;

#if GFX_2DACCEL_GU1
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU1)
        retval = gu1_test_blt_pending();
#endif
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        retval = gu2_test_blt_pending();
#endif
    return (retval);
}

/*---------------------------------------------------------------------------
 * NEW ROUTINES FOR REDCLOUD
 *---------------------------------------------------------------------------
 */

/*---------------------------------------------------------------------------
 * gfx2_set_source_stride
 *---------------------------------------------------------------------------
 */
void
gfx2_set_source_stride(unsigned short stride)
{
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu22_set_source_stride(stride);
#endif
}

/*---------------------------------------------------------------------------
 * gfx2_set_destination_stride
 *---------------------------------------------------------------------------
 */
void
gfx2_set_destination_stride(unsigned short stride)
{
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu22_set_destination_stride(stride);
#endif
}

/*---------------------------------------------------------------------------
 * gfx2_set_pattern_origin
 *---------------------------------------------------------------------------
 */
void
gfx2_set_pattern_origin(int x, int y)
{
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu22_set_pattern_origin(x, y);
#endif
}

/*---------------------------------------------------------------------------
 * gfx2_set_source_transparency
 *---------------------------------------------------------------------------
 */
void
gfx2_set_source_transparency(unsigned long color, unsigned long mask)
{
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu22_set_source_transparency(color, mask);
#endif
}

/*---------------------------------------------------------------------------
 * gfx2_set_alpha_mode
 *---------------------------------------------------------------------------
 */
void
gfx2_set_alpha_mode(int mode)
{
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu22_set_alpha_mode(mode);
#endif
}

/*---------------------------------------------------------------------------
 * gfx2_set_alpha_value
 *---------------------------------------------------------------------------
 */
void
gfx2_set_alpha_value(unsigned char value)
{
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu22_set_alpha_value(value);
#endif
}

/*---------------------------------------------------------------------------
 * gfx2_pattern_fill
 *---------------------------------------------------------------------------
 */
void
gfx2_pattern_fill(unsigned long dstoffset, unsigned short width,
                  unsigned short height)
{
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu22_pattern_fill(dstoffset, width, height);
#endif
}

/*---------------------------------------------------------------------------
 * gfx2_color_pattern_fill
 *---------------------------------------------------------------------------
 */
void
gfx2_color_pattern_fill(unsigned long dstoffset, unsigned short width,
                        unsigned short height, unsigned long *pattern)
{
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu22_color_pattern_fill(dstoffset, width, height, pattern);
#endif
}

/*---------------------------------------------------------------------------
 * gfx2_screen_to_screen_blt
 *---------------------------------------------------------------------------
 */
void
gfx2_screen_to_screen_blt(unsigned long srcoffset, unsigned long dstoffset,
                          unsigned short width, unsigned short height,
                          int flags)
{
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu22_screen_to_screen_blt(srcoffset, dstoffset, width, height, flags);
#endif
}

/*---------------------------------------------------------------------------
 * gfx2_mono_expand_blt
 *---------------------------------------------------------------------------
 */
void
gfx2_mono_expand_blt(unsigned long srcbase, unsigned short srcx,
                     unsigned short srcy, unsigned long dstoffset,
                     unsigned short width, unsigned short height,
                     int byte_packed)
{
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu22_mono_expand_blt(srcbase, srcx, srcy, dstoffset, width, height,
                             byte_packed);
#endif
}

/*---------------------------------------------------------------------------
 * gfx2_color_bitmap_to_screen_blt	
 *---------------------------------------------------------------------------
 */
void
gfx2_color_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
                                unsigned long dstoffset, unsigned short width,
                                unsigned short height, unsigned char *data,
                                short pitch)
{
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu22_color_bitmap_to_screen_blt(srcx, srcy, dstoffset, width, height,
                                        data, pitch);
#endif
}

/*---------------------------------------------------------------------------
 * gfx2_text_blt	
 *---------------------------------------------------------------------------
 */
void
gfx2_text_blt(unsigned long dstoffset, unsigned short width,
              unsigned short height, unsigned char *data)
{
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu22_text_blt(dstoffset, width, height, data);
#endif
}

/*---------------------------------------------------------------------------
 * gfx2_mono_bitmap_to_screen_blt	
 *---------------------------------------------------------------------------
 */
void
gfx2_mono_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
                               unsigned long dstoffset, unsigned short width,
                               unsigned short height, unsigned char *data,
                               short pitch)
{
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu22_mono_bitmap_to_screen_blt(srcx, srcy, dstoffset, width, height,
                                       data, pitch);
#endif
}

/*---------------------------------------------------------------------------
 * gfx2_bresenham_line
 *---------------------------------------------------------------------------
 */
void
gfx2_bresenham_line(unsigned long dstoffset,
                    unsigned short length, unsigned short initerr,
                    unsigned short axialerr, unsigned short diagerr,
                    unsigned short flags)
{
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu22_bresenham_line(dstoffset, length, initerr, axialerr, diagerr,
                            flags);
#endif
}

/*---------------------------------------------------------------------------
 * gfx2_sync_to_vblank
 *---------------------------------------------------------------------------
 */
void
gfx2_sync_to_vblank(void)
{
#if GFX_2DACCEL_GU2
    if (gfx_2daccel_type & GFX_2DACCEL_TYPE_GU2)
        gu22_sync_to_vblank();
#endif
}

#endif                          /* GFX_2DACCEL_DYNAMIC */

/* END OF FILE */
