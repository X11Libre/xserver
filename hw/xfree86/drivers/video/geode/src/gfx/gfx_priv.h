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
 * This header file contains the prototypes for local and private routines.
 * */

/*---------------------------------*/
/* FIRST GENERATION INITIALIZATION */
/*---------------------------------*/

#if GFX_INIT_GU1

/* PRIVATE ROUTINES */

unsigned char gfx_gxm_config_read(unsigned char index);

/* DYNAMIC ROUTINES */

#if GFX_INIT_DYNAMIC
unsigned long gu1_get_core_freq(void);
unsigned long gu1_get_cpu_register_base(void);
unsigned long gu1_get_graphics_register_base(void);
unsigned long gu1_get_frame_buffer_base(void);
unsigned long gu1_get_frame_buffer_size(void);
unsigned long gu1_get_vid_register_base(void);
unsigned long gu1_get_vip_register_base(void);
#endif

#endif

/*----------------------------------*/
/* SECOND GENERATION INITIALIZATION */
/*----------------------------------*/

#if GFX_INIT_GU2

/* DYNAMIC ROUTINES */

#if GFX_INIT_DYNAMIC
unsigned long gu2_get_core_freq(void);
unsigned long gu2_get_cpu_register_base(void);
unsigned long gu2_get_graphics_register_base(void);
unsigned long gu2_get_frame_buffer_base(void);
unsigned long gu2_get_frame_buffer_size(void);
unsigned long gu2_get_vid_register_base(void);
unsigned long gu2_get_vip_register_base(void);
#endif

#endif

/*----------------------------------*/
/* MSR ROUTINES                     */
/*----------------------------------*/

#if GFX_MSR_REDCLOUD

/* DYNAMIC ROUTINES */

#if GFX_MSR_DYNAMIC
int redcloud_msr_init(void);
DEV_STATUS redcloud_id_msr_device(MSR * pDev, unsigned long address);
DEV_STATUS redcloud_get_msr_dev_address(unsigned int device,
                                        unsigned long *address);
DEV_STATUS redcloud_get_glink_id_at_address(unsigned int *device,
                                            unsigned long address);
DEV_STATUS redcloud_msr_read(unsigned int device, unsigned int msrRegister,
                             Q_WORD * msrValue);
DEV_STATUS redcloud_msr_write(unsigned int device, unsigned int msrRegister,
                              Q_WORD * msrValue);
#endif

#endif

void gfx_set_display_video_enable(int enable);
void gfx_set_display_video_size(unsigned short width, unsigned short height);
void gfx_set_display_video_offset(unsigned long offset);
unsigned long gfx_get_display_video_offset(void);
unsigned long gfx_get_display_video_size(void);

/*----------------------------------*/
/* FIRST GENERATION DISPLAY         */
/*----------------------------------*/

#if GFX_DISPLAY_GU1

/* PRIVATE ROUTINES */

void gu1_enable_compression(void);
void gu1_disable_compression(void);
void gu1_delay_approximate(unsigned long milliseconds);
void gu1_delay_precise(unsigned long milliseconds);

/* DYNAMIC ROUTINES */

#if GFX_DISPLAY_DYNAMIC
void gu1_set_display_video_enable(int enable);
void gu1_set_display_video_size(unsigned short width, unsigned short height);
void gu1_set_display_video_offset(unsigned long offset);
unsigned long gu1_get_display_video_offset(void);
unsigned long gu1_get_display_video_size(void);
int gu1_set_display_bpp(unsigned short bpp);
int gu1_is_display_mode_supported(int xres, int yres, int bpp, int hz);
int gu1_set_display_mode(int xres, int yres, int bpp, int hz);
int gu1_set_display_timings(unsigned short bpp, unsigned short flags,
                            unsigned short hactive, unsigned short hblank_start,
                            unsigned short hsync_start,
                            unsigned short hsync_end, unsigned short hblank_end,
                            unsigned short htotal, unsigned short vactive,
                            unsigned short vblank_start,
                            unsigned short vsync_start,
                            unsigned short vsync_end, unsigned short vblank_end,
                            unsigned short vtotal, unsigned long frequency);
int gu1_set_vtotal(unsigned short vtotal);
void gu1_set_display_pitch(unsigned short pitch);
void gu1_set_display_offset(unsigned long offset);
int gu1_set_display_palette_entry(unsigned long index, unsigned long palette);
int gu1_set_display_palette(unsigned long *palette);
void gu1_video_shutdown(void);
void gu1_set_cursor_enable(int enable);
void gu1_set_cursor_colors(unsigned long bkcolor, unsigned long fgcolor);
void gu1_set_cursor_position(unsigned long memoffset,
                             unsigned short xpos, unsigned short ypos,
                             unsigned short xhotspot, unsigned short yhotspot);
void gu1_set_cursor_shape32(unsigned long memoffset,
                            unsigned long *andmask, unsigned long *xormask);
int gu1_set_compression_enable(int enable);
int gu1_set_compression_offset(unsigned long offset);
int gu1_set_compression_pitch(unsigned short pitch);
int gu1_set_compression_size(unsigned short size);
void gu1_set_display_priority_high(int enable);
int gu1_test_timing_active(void);
int gu1_test_vertical_active(void);
int gu1_wait_vertical_blank(void);
void gu1_delay_milliseconds(unsigned long milliseconds);
void gu1_delay_microseconds(unsigned long microseconds);
void gu1_enable_panning(int x, int y);
int gu1_set_fixed_timings(int panelResX, int panelResY, unsigned short width,
                          unsigned short height, unsigned short bpp);
int gu1_set_panel_present(int panelResX, int panelResY, unsigned short width,
                          unsigned short height, unsigned short bpp);
void gu1_reset_timing_lock(void);
int gu1_get_display_details(unsigned int mode, int *xres, int *yres, int *hz);
unsigned short gu1_get_display_pitch(void);
unsigned long gu1_get_max_supported_pixel_clock(void);
int gu1_mode_frequency_supported(int xres, int yres, int bpp,
                                 unsigned long frequency);
int gu1_get_refreshrate_from_frequency(int xres, int yres, int bpp, int *hz,
                                       unsigned long frequency);
int gu1_get_refreshrate_from_mode(int xres, int yres, int bpp, int *hz,
                                  unsigned long frequency);
int gu1_get_frequency_from_refreshrate(int xres, int yres, int bpp, int hz,
                                       int *frequency);
int gu1_get_display_mode_count(void);
int gu1_get_display_mode(int *xres, int *yres, int *bpp, int *hz);
unsigned long gu1_get_frame_buffer_line_size(void);
unsigned short gu1_get_hactive(void);
unsigned short gu1_get_hblank_start(void);
unsigned short gu1_get_hsync_start(void);
unsigned short gu1_get_hsync_end(void);
unsigned short gu1_get_hblank_end(void);
unsigned short gu1_get_htotal(void);
unsigned short gu1_get_vactive(void);
unsigned short gu1_get_vline(void);
unsigned short gu1_get_vblank_start(void);
unsigned short gu1_get_vsync_start(void);
unsigned short gu1_get_vsync_end(void);
unsigned short gu1_get_vblank_end(void);
unsigned short gu1_get_vtotal(void);
unsigned short gu1_get_display_bpp(void);
unsigned long gu1_get_display_offset(void);
int gu1_get_display_palette_entry(unsigned long index, unsigned long *palette);
void gu1_get_display_palette(unsigned long *palette);
unsigned long gu1_get_cursor_enable(void);
unsigned long gu1_get_cursor_offset(void);
unsigned long gu1_get_cursor_position(void);
unsigned long gu1_get_cursor_clip(void);
unsigned long gu1_get_cursor_color(int color);
unsigned long gu1_get_icon_enable(void);
unsigned long gu1_get_icon_offset(void);
unsigned long gu1_get_icon_position(void);
unsigned long gu1_get_icon_color(int color);
int gu1_get_compression_enable(void);
unsigned long gu1_get_compression_offset(void);
unsigned short gu1_get_compression_pitch(void);
unsigned short gu1_get_compression_size(void);
int gu1_get_display_priority_high(void);
int gu1_get_valid_bit(int line);
#endif

#endif

void gfx_set_display_video_format(unsigned long format);
void gfx_set_display_video_yuv_offsets(unsigned long yoffset,
                                       unsigned long uoffset,
                                       unsigned long voffset);
void gfx_set_display_video_yuv_pitch(unsigned long ypitch,
                                     unsigned long uvpitch);
void gfx_set_display_video_downscale(unsigned short srch, unsigned short dsth);
void gfx_set_display_video_vertical_downscale_enable(int enable);
void gfx_get_display_video_yuv_offsets(unsigned long *yoffset,
                                       unsigned long *uoffset,
                                       unsigned long *voffset);
void gfx_get_display_video_yuv_pitch(unsigned long *ypitch,
                                     unsigned long *uvpitch);
unsigned long gfx_get_display_video_downscale_delta(void);
int gfx_get_display_video_downscale_enable(void);

/*----------------------------------*/
/* SECOND GENERATION DISPLAY        */
/*----------------------------------*/

#if GFX_DISPLAY_GU2

/* DYNAMIC ROUTINES */

#if GFX_DISPLAY_DYNAMIC
void gu2_get_display_video_yuv_offsets(unsigned long *yoffset,
                                       unsigned long *uoffset,
                                       unsigned long *voffset);
void gu2_get_display_video_yuv_pitch(unsigned long *ypitch,
                                     unsigned long *uvpitch);
unsigned long gu2_get_display_video_downscale_delta(void);
int gu2_get_display_video_downscale_enable(void);
void gu2_set_display_video_yuv_offsets(unsigned long yoffset,
                                       unsigned long uoffset,
                                       unsigned long voffset);
void gu2_set_display_video_format(unsigned long format);
void gu2_set_display_video_yuv_pitch(unsigned long ypitch,
                                     unsigned long uvpitch);
void gu2_set_display_video_downscale(unsigned short srch, unsigned short dsth);
void gu2_set_display_video_vertical_downscale_enable(int enable);
void gu2_set_display_video_enable(int enable);
void gu2_set_display_video_size(unsigned short width, unsigned short height);
void gu2_set_display_video_offset(unsigned long offset);
unsigned long gu2_get_display_video_offset(void);
unsigned long gu2_get_display_video_size(void);
int gu2_set_display_bpp(unsigned short bpp);
int gu2_is_display_mode_supported(int xres, int yres, int bpp, int hz);
int gu2_set_display_mode(int xres, int yres, int bpp, int hz);
int gu2_set_display_timings(unsigned short bpp, unsigned short flags,
                            unsigned short hactive, unsigned short hblank_start,
                            unsigned short hsync_start,
                            unsigned short hsync_end, unsigned short hblank_end,
                            unsigned short htotal, unsigned short vactive,
                            unsigned short vblank_start,
                            unsigned short vsync_start,
                            unsigned short vsync_end, unsigned short vblank_end,
                            unsigned short vtotal, unsigned long frequency);
int gu2_set_vtotal(unsigned short vtotal);
void gu2_set_display_pitch(unsigned short pitch);
void gu2_set_display_offset(unsigned long offset);
int gu2_set_display_palette_entry(unsigned long index, unsigned long palette);
int gu2_set_display_palette(unsigned long *palette);
void gu2_set_cursor_enable(int enable);
void gu2_set_cursor_colors(unsigned long bkcolor, unsigned long fgcolor);
void gu2_set_cursor_position(unsigned long memoffset,
                             unsigned short xpos, unsigned short ypos,
                             unsigned short xhotspot, unsigned short yhotspot);
void gu2_set_cursor_shape32(unsigned long memoffset,
                            unsigned long *andmask, unsigned long *xormask);
void gu2_set_cursor_shape64(unsigned long memoffset,
                            unsigned long *andmask, unsigned long *xormask);
void gu2_set_icon_enable(int enable);
void gu2_set_icon_colors(unsigned long color0, unsigned long color1,
                         unsigned long color2);
void gu2_set_icon_position(unsigned long memoffset, unsigned short xpos);
void gu2_set_icon_shape64(unsigned long memoffset, unsigned long *andmask,
                          unsigned long *xormask, unsigned int lines);
int gu2_set_compression_enable(int enable);
int gu2_set_compression_offset(unsigned long offset);
int gu2_set_compression_pitch(unsigned short pitch);
int gu2_set_compression_size(unsigned short size);
void gu2_set_display_priority_high(int enable);
int gu2_test_timing_active(void);
int gu2_test_vertical_active(void);
int gu2_wait_vertical_blank(void);
void gu2_delay_milliseconds(unsigned long milliseconds);
void gu2_delay_microseconds(unsigned long microseconds);
void gu2_enable_panning(int x, int y);
int gu2_is_panel_mode_supported(int panelResX, int panelResY,
                                unsigned short width, unsigned short height,
                                unsigned short bpp);
int gu2_set_fixed_timings(int panelResX, int panelResY, unsigned short width,
                          unsigned short height, unsigned short bpp);
int gu2_set_panel_present(int panelResX, int panelResY, unsigned short width,
                          unsigned short height, unsigned short bpp);
void gu2_reset_timing_lock(void);
int gu2_get_display_details(unsigned int mode, int *xres, int *yres, int *hz);
unsigned short gu2_get_display_pitch(void);
unsigned long gu2_get_max_supported_pixel_clock(void);
int gu2_mode_frequency_supported(int xres, int yres, int bpp,
                                 unsigned long frequency);
int gu2_get_refreshrate_from_frequency(int xres, int yres, int bpp, int *hz,
                                       unsigned long frequency);
int gu2_get_refreshrate_from_mode(int xres, int yres, int bpp, int *hz,
                                  unsigned long frequency);
int gu2_get_frequency_from_refreshrate(int xres, int yres, int bpp, int hz,
                                       int *frequency);
int gu2_get_display_mode_count(void);
int gu2_get_display_mode(int *xres, int *yres, int *bpp, int *hz);
unsigned long gu2_get_frame_buffer_line_size(void);
unsigned short gu2_get_hactive(void);
unsigned short gu2_get_hblank_start(void);
unsigned short gu2_get_hsync_start(void);
unsigned short gu2_get_hsync_end(void);
unsigned short gu2_get_hblank_end(void);
unsigned short gu2_get_htotal(void);
unsigned short gu2_get_vactive(void);
unsigned short gu2_get_vline(void);
unsigned short gu2_get_vblank_start(void);
unsigned short gu2_get_vsync_start(void);
unsigned short gu2_get_vsync_end(void);
unsigned short gu2_get_vblank_end(void);
unsigned short gu2_get_vtotal(void);
unsigned short gu2_get_display_bpp(void);
unsigned long gu2_get_display_offset(void);
int gu2_get_display_palette_entry(unsigned long index, unsigned long *palette);
void gu2_get_display_palette(unsigned long *palette);
unsigned long gu2_get_cursor_enable(void);
unsigned long gu2_get_cursor_offset(void);
unsigned long gu2_get_cursor_position(void);
unsigned long gu2_get_cursor_clip(void);
unsigned long gu2_get_cursor_color(int color);
unsigned long gu2_get_icon_enable(void);
unsigned long gu2_get_icon_offset(void);
unsigned long gu2_get_icon_position(void);
unsigned long gu2_get_icon_color(int color);
int gu2_get_compression_enable(void);
unsigned long gu2_get_compression_offset(void);
unsigned short gu2_get_compression_pitch(void);
unsigned short gu2_get_compression_size(void);
int gu2_get_valid_bit(int line);
#endif

#endif

/*----------------------------------*/
/* FIRST GENERATION 2D ACCELERATION */
/*----------------------------------*/

#if GFX_2DACCEL_GU1

/* PRIVATE ROUTINES */

void gu1_solid_fill(unsigned short x, unsigned short y,
                    unsigned short width, unsigned short height,
                    unsigned long color);
void gu1_detect_blt_buffer_base(void);

/* DYNAMIC ROUTINES */

#if GFX_2DACCEL_DYNAMIC
void gu1_set_bpp(unsigned short bpp);
void gu1_set_solid_pattern(unsigned long color);
void gu1_set_mono_pattern(unsigned long bgcolor, unsigned long fgcolor,
                          unsigned long data0, unsigned long data1,
                          unsigned char transparency);
void gu1_set_color_pattern(unsigned long bgcolor, unsigned long fgcolor,
                           unsigned long data0, unsigned long data1,
                           unsigned long data2, unsigned long data3,
                           unsigned char transparency);
void gu1_load_color_pattern_line(short y, unsigned long *pattern_8x8);
void gu1_set_solid_source(unsigned long color);
void gu1_set_mono_source(unsigned long bgcolor, unsigned long fgcolor,
                         unsigned short transparent);
void gu1_set_pattern_flags(unsigned short flags);
void gu1_set_raster_operation(unsigned char rop);
void gu1_pattern_fill(unsigned short x, unsigned short y,
                      unsigned short width, unsigned short height);
void gu1_color_pattern_fill(unsigned short x, unsigned short y,
                            unsigned short width, unsigned short height,
                            unsigned long *pattern);
void gu1_screen_to_screen_blt(unsigned short srcx, unsigned short srcy,
                              unsigned short dstx, unsigned short dsty,
                              unsigned short width, unsigned short height);
void gu1_screen_to_screen_xblt(unsigned short srcx, unsigned short srcy,
                               unsigned short dstx, unsigned short dsty,
                               unsigned short width, unsigned short height,
                               unsigned long color);
void gu1_color_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
                                    unsigned short dstx, unsigned short dsty,
                                    unsigned short width, unsigned short height,
                                    unsigned char *data, long pitch);
void gu1_color_bitmap_to_screen_xblt(unsigned short srcx, unsigned short srcy,
                                     unsigned short dstx, unsigned short dsty,
                                     unsigned short width,
                                     unsigned short height, unsigned char *data,
                                     long pitch, unsigned long color);
void gu1_mono_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
                                   unsigned short dstx, unsigned short dsty,
                                   unsigned short width, unsigned short height,
                                   unsigned char *data, short pitch);
void gu1_text_blt(unsigned short dstx, unsigned short dsty,
                  unsigned short width, unsigned short height,
                  unsigned char *data);
void gu1_bresenham_line(unsigned short x, unsigned short y,
                        unsigned short length, unsigned short initerr,
                        unsigned short axialerr, unsigned short diagerr,
                        unsigned short flags);
void gu1_wait_until_idle(void);
int gu1_test_blt_pending(void);
#endif

#endif

/*-----------------------------------*/
/* SECOND GENERATION 2D ACCELERATION */
/*-----------------------------------*/

#if GFX_2DACCEL_GU2

/* DYNAMIC ROUTINES */

#if GFX_2DACCEL_DYNAMIC
void gfx_reset_pitch(unsigned short pitch);
void gu2_reset_pitch(unsigned short pitch);
void gu2_set_bpp(unsigned short bpp);
void gu2_set_solid_pattern(unsigned long color);
void gu2_set_mono_pattern(unsigned long bgcolor, unsigned long fgcolor,
                          unsigned long data0, unsigned long data1,
                          unsigned char transparency);
void gu2_set_color_pattern(unsigned long bgcolor, unsigned long fgcolor,
                           unsigned long data0, unsigned long data1,
                           unsigned long data2, unsigned long data3,
                           unsigned char transparency);
void gu2_load_color_pattern_line(short y, unsigned long *pattern_8x8);
void gu2_set_solid_source(unsigned long color);
void gu2_set_mono_source(unsigned long bgcolor, unsigned long fgcolor,
                         unsigned short transparent);
void gu2_set_pattern_flags(unsigned short flags);
void gu2_set_raster_operation(unsigned char rop);
void gu2_pattern_fill(unsigned short x, unsigned short y,
                      unsigned short width, unsigned short height);
void gu2_color_pattern_fill(unsigned short x, unsigned short y,
                            unsigned short width, unsigned short height,
                            unsigned long *pattern);
void gu2_screen_to_screen_blt(unsigned short srcx, unsigned short srcy,
                              unsigned short dstx, unsigned short dsty,
                              unsigned short width, unsigned short height);
void gu2_screen_to_screen_xblt(unsigned short srcx, unsigned short srcy,
                               unsigned short dstx, unsigned short dsty,
                               unsigned short width, unsigned short height,
                               unsigned long color);
void gu2_color_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
                                    unsigned short dstx, unsigned short dsty,
                                    unsigned short width, unsigned short height,
                                    unsigned char *data, long pitch);
void gu2_color_bitmap_to_screen_xblt(unsigned short srcx, unsigned short srcy,
                                     unsigned short dstx, unsigned short dsty,
                                     unsigned short width,
                                     unsigned short height, unsigned char *data,
                                     long pitch, unsigned long color);
void gu2_mono_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
                                   unsigned short dstx, unsigned short dsty,
                                   unsigned short width, unsigned short height,
                                   unsigned char *data, short pitch);
void gu2_text_blt(unsigned short dstx, unsigned short dsty,
                  unsigned short width, unsigned short height,
                  unsigned char *data);
void gu2_bresenham_line(unsigned short x, unsigned short y,
                        unsigned short length, unsigned short initerr,
                        unsigned short axialerr, unsigned short diagerr,
                        unsigned short flags);
void gu2_wait_until_idle(void);
int gu2_test_blt_pending(void);

void gu22_set_source_stride(unsigned short stride);
void gu22_set_destination_stride(unsigned short stride);
void gu22_set_pattern_origin(int x, int y);
void gu22_set_source_transparency(unsigned long color, unsigned long mask);
void gu22_set_alpha_mode(int mode);
void gu22_set_alpha_value(unsigned char value);
void gu22_pattern_fill(unsigned long dstoffset, unsigned short width,
                       unsigned short height);
void gu22_color_pattern_fill(unsigned long dstoffset, unsigned short width,
                             unsigned short height, unsigned long *pattern);
void gu22_screen_to_screen_blt(unsigned long srcoffset,
                               unsigned long dstoffset, unsigned short width,
                               unsigned short height, int flags);
void gu22_mono_expand_blt(unsigned long srcbase, unsigned short srcx,
                          unsigned short srcy, unsigned long dstoffset,
                          unsigned short width, unsigned short height,
                          int byte_packed);
void gu22_color_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
                                     unsigned long dstoffset,
                                     unsigned short width,
                                     unsigned short height, unsigned char *data,
                                     short pitch);
void gu22_mono_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
                                    unsigned long dstoffset,
                                    unsigned short width, unsigned short height,
                                    unsigned char *data, short pitch);
void gu22_text_blt(unsigned long dstoffset, unsigned short width,
                   unsigned short height, unsigned char *data);
void gu22_bresenham_line(unsigned long dstoffset, unsigned short length,
                         unsigned short initerr, unsigned short axialerr,
                         unsigned short diagerr, unsigned short flags);
void gu22_sync_to_vblank(void);
#endif

#endif

void gfx_reset_video(void);
int gfx_set_display_control(int sync_polarities);
int gfx_set_screen_enable(int enable);

/*-----------------------*/
/* CS5530 VIDEO ROUTINES */
/*-----------------------*/

#if GFX_VIDEO_CS5530

/* PRIVATE ROUTINES */

/* DYNAMIC ROUTINES */

#if GFX_VIDEO_DYNAMIC
void cs5530_reset_video(void);
int cs5530_set_display_control(int sync_polarities);
void cs5530_set_clock_frequency(unsigned long frequency);
unsigned long cs5530_get_clock_frequency(void);
int cs5530_set_crt_enable(int enable);
int cs5530_get_sync_polarities(void);
int cs5530_get_vsa2_softvga_enable(void);
int cs5530_set_video_enable(int enable);
int cs5530_set_video_format(unsigned long format);
int cs5530_set_video_size(unsigned short width, unsigned short height);
int cs5530_set_video_offset(unsigned long offset);
int cs5530_set_video_window(short x, short y, unsigned short w,
                            unsigned short h);
int cs5530_set_video_left_crop(unsigned short x);
int cs5530_set_video_scale(unsigned short srcw, unsigned short srch,
                           unsigned short dstw, unsigned short dsth);
int cs5530_set_video_color_key(unsigned long key, unsigned long mask,
                               int bluescreen);
int cs5530_set_video_filter(int xfilter, int yfilter);
int cs5530_set_video_palette(unsigned long *palette);
int cs5530_set_video_palette_entry(unsigned long index, unsigned long color);
int cs5530_disable_softvga(void);
int cs5530_enable_softvga(void);
unsigned long cs5530_get_max_video_width(void);

/* READ ROUTINES IN GFX_VID.C */

int cs5530_get_video_enable(void);
int cs5530_get_video_format(void);
unsigned long cs5530_get_video_src_size(void);
unsigned long cs5530_get_video_line_size(void);
unsigned long cs5530_get_video_xclip(void);
unsigned long cs5530_get_video_offset(void);
unsigned long cs5530_get_video_upscale(void);
unsigned long cs5530_get_video_scale(void);
unsigned long cs5530_get_video_dst_size(void);
unsigned long cs5530_get_video_position(void);
unsigned long cs5530_get_video_color_key(void);
unsigned long cs5530_get_video_color_key_mask(void);
int cs5530_get_video_palette_entry(unsigned long index, unsigned long *palette);
int cs5530_get_video_color_key_src(void);
int cs5530_get_video_filter(void);
unsigned long cs5530_read_crc(void);
#endif
#endif

/*-----------------------*/
/* SC1200 VIDEO ROUTINES */
/*-----------------------*/

#if GFX_VIDEO_SC1200

/* PRIVATE ROUTINES */

/* DYNAMIC ROUTINES */

#if GFX_VIDEO_DYNAMIC
void sc1200_reset_video(void);
int sc1200_set_display_control(int sync_polarities);
int sc1200_set_screen_enable(int enable);
void sc1200_set_clock_frequency(unsigned long frequency);
unsigned long sc1200_get_clock_frequency(void);
int sc1200_set_crt_enable(int enable);
int sc1200_get_sync_polarities(void);
int sc1200_get_vsa2_softvga_enable(void);
int sc1200_set_video_enable(int enable);
int sc1200_set_video_format(unsigned long format);
int sc1200_set_video_size(unsigned short width, unsigned short height);
int sc1200_set_video_offset(unsigned long offset);
int sc1200_set_video_window(short x, short y, unsigned short w,
                            unsigned short h);
int sc1200_set_video_left_crop(unsigned short x);
int sc1200_set_video_upscale(unsigned short srcw, unsigned short srch,
                             unsigned short dstw, unsigned short dsth);
int sc1200_set_video_scale(unsigned short srcw, unsigned short srch,
                           unsigned short dstw, unsigned short dsth);
int sc1200_set_video_downscale_config(unsigned short type, unsigned short m);
int sc1200_set_video_color_key(unsigned long key, unsigned long mask,
                               int bluescreen);
int sc1200_set_video_filter(int xfilter, int yfilter);
int sc1200_set_video_palette(unsigned long *palette);
int sc1200_set_video_palette_bypass(int enable);
int sc1200_set_video_palette_entry(unsigned long index, unsigned long color);
int sc1200_set_video_downscale_coefficients(unsigned short coef1,
                                            unsigned short coef2,
                                            unsigned short coef3,
                                            unsigned short coef4);
int sc1200_set_video_downscale_enable(int enable);
int sc1200_set_video_source(VideoSourceType source);
int sc1200_set_vbi_source(VbiSourceType source);
int sc1200_set_vbi_lines(unsigned long even, unsigned long odd);
int sc1200_set_vbi_total(unsigned long even, unsigned long odd);
int sc1200_set_video_interlaced(int enable);
int sc1200_set_color_space_YUV(int enable);
int sc1200_set_vertical_scaler_offset(char offset);
int sc1200_set_top_line_in_odd(int enable);
int sc1200_set_genlock_delay(unsigned long delay);
int sc1200_set_genlock_enable(int flags);
int sc1200_set_video_cursor(unsigned long key, unsigned long mask,
                            unsigned short select_color2, unsigned long color1,
                            unsigned long color2);
int sc1200_set_video_request(short x, short y);

int sc1200_select_alpha_region(int region);
int sc1200_set_alpha_enable(int enable);
int sc1200_set_alpha_window(short x, short y,
                            unsigned short width, unsigned short height);
int sc1200_set_alpha_value(unsigned char alpha, char delta);
int sc1200_set_alpha_priority(int priority);
int sc1200_set_alpha_color(unsigned long color);
int sc1200_set_alpha_color_enable(int enable);
int sc1200_set_no_ck_outside_alpha(int enable);
int sc1200_disable_softvga(void);
int sc1200_enable_softvga(void);
int sc1200_set_macrovision_enable(int enable);
unsigned long sc1200_get_max_video_width(void);

/* READ ROUTINES IN GFX_VID.C */

int sc1200_get_video_enable(void);
int sc1200_get_video_format(void);
unsigned long sc1200_get_video_src_size(void);
unsigned long sc1200_get_video_line_size(void);
unsigned long sc1200_get_video_xclip(void);
unsigned long sc1200_get_video_offset(void);
unsigned long sc1200_get_video_upscale(void);
unsigned long sc1200_get_video_scale(void);
int sc1200_get_video_downscale_config(unsigned short *type, unsigned short *m);
void sc1200_get_video_downscale_coefficients(unsigned short *coef1,
                                             unsigned short *coef2,
                                             unsigned short *coef3,
                                             unsigned short *coef4);
void sc1200_get_video_downscale_enable(int *enable);
unsigned long sc1200_get_video_dst_size(void);
unsigned long sc1200_get_video_position(void);
unsigned long sc1200_get_video_color_key(void);
unsigned long sc1200_get_video_color_key_mask(void);
int sc1200_get_video_palette_entry(unsigned long index, unsigned long *palette);
int sc1200_get_video_color_key_src(void);
int sc1200_get_video_filter(void);
int sc1200_get_video_request(short *x, short *y);
int sc1200_get_video_source(VideoSourceType * source);
int sc1200_get_vbi_source(VbiSourceType * source);
unsigned long sc1200_get_vbi_lines(int odd);
unsigned long sc1200_get_vbi_total(int odd);
int sc1200_get_video_interlaced(void);
int sc1200_get_color_space_YUV(void);
int sc1200_get_vertical_scaler_offset(char *offset);
unsigned long sc1200_get_genlock_delay(void);
int sc1200_get_genlock_enable(void);
int sc1200_get_video_cursor(unsigned long *key, unsigned long *mask,
                            unsigned short *select_color2,
                            unsigned long *color1, unsigned short *color2);
unsigned long sc1200_read_crc(void);
int sc1200_get_macrovision_enable(void);

void sc1200_get_alpha_enable(int *enable);
void sc1200_get_alpha_size(unsigned short *x, unsigned short *y,
                           unsigned short *width, unsigned short *height);
void sc1200_get_alpha_value(unsigned char *alpha, char *delta);
void sc1200_get_alpha_priority(int *priority);
void sc1200_get_alpha_color(unsigned long *color);
#endif
#endif

/*-------------------------*/
/* REDCLOUD VIDEO ROUTINES */
/*-------------------------*/

#if GFX_VIDEO_REDCLOUD

/* PRIVATE ROUTINES */

/* DYNAMIC ROUTINES */

#if GFX_VIDEO_DYNAMIC
void redcloud_reset_video(void);
int redcloud_set_display_control(int sync_polarities);
void redcloud_set_clock_frequency(unsigned long frequency);
unsigned long redcloud_get_clock_frequency(void);
int redcloud_set_crt_enable(int enable);
int redcloud_get_sync_polarities(void);
int redcloud_set_video_enable(int enable);
int redcloud_set_video_format(unsigned long format);
int redcloud_set_video_size(unsigned short width, unsigned short height);
int redcloud_set_video_yuv_pitch(unsigned long ypitch, unsigned long uvpitch);
int redcloud_set_video_offset(unsigned long offset);
int redcloud_set_video_yuv_offsets(unsigned long yoffset,
                                   unsigned long uoffset,
                                   unsigned long voffset);
int redcloud_set_video_window(short x, short y, unsigned short w,
                              unsigned short h);
int redcloud_set_video_left_crop(unsigned short x);
int redcloud_set_video_scale(unsigned short srcw, unsigned short srch,
                             unsigned short dstw, unsigned short dsth);
int redcloud_set_video_vertical_downscale(unsigned short srch,
                                          unsigned short dsth);
void redcloud_set_video_vertical_downscale_enable(int enable);
int redcloud_set_video_downscale_config(unsigned short type, unsigned short m);
int redcloud_set_video_color_key(unsigned long key, unsigned long mask,
                                 int bluescreen);
int redcloud_set_video_filter(int xfilter, int yfilter);
int redcloud_set_video_palette(unsigned long *palette);
int redcloud_set_graphics_palette(unsigned long *palette);
int redcloud_set_video_palette_bypass(int enable);
int redcloud_set_video_palette_entry(unsigned long index, unsigned long color);
int redcloud_set_graphics_palette_entry(unsigned long index,
                                        unsigned long color);
int redcloud_set_video_downscale_coefficients(unsigned short coef1,
                                              unsigned short coef2,
                                              unsigned short coef3,
                                              unsigned short coef4);
int redcloud_set_video_downscale_enable(int enable);
int redcloud_set_video_cursor(unsigned long key, unsigned long mask,
                              unsigned short select_color2,
                              unsigned long color1, unsigned long color2);
int redcloud_set_video_cursor_enable(int enable);

int redcloud_select_alpha_region(int region);
int redcloud_set_alpha_enable(int enable);
int redcloud_set_alpha_window(short x, short y,
                              unsigned short width, unsigned short height);
int redcloud_set_alpha_value(unsigned char alpha, char delta);
int redcloud_set_alpha_priority(int priority);
int redcloud_set_alpha_color(unsigned long color);
int redcloud_set_alpha_color_enable(int enable);
int redcloud_set_no_ck_outside_alpha(int enable);
int redcloud_set_video_request(short x, short y);

/* READ ROUTINES IN GFX_VID.C */

int redcloud_get_video_enable(void);
int redcloud_get_video_format(void);
unsigned long redcloud_get_video_src_size(void);
unsigned long redcloud_get_video_line_size(void);
unsigned long redcloud_get_video_xclip(void);
unsigned long redcloud_get_video_offset(void);
void redcloud_get_video_yuv_offsets(unsigned long *yoffset,
                                    unsigned long *uoffset,
                                    unsigned long *voffset);
void redcloud_get_video_yuv_pitch(unsigned long *ypitch,
                                  unsigned long *uvpitch);
unsigned long redcloud_get_video_scale(void);
unsigned long redcloud_get_video_downscale_delta(void);
int redcloud_get_video_vertical_downscale_enable(void);
int redcloud_get_video_downscale_config(unsigned short *type,
                                        unsigned short *m);
void redcloud_get_video_downscale_coefficients(unsigned short *coef1,
                                               unsigned short *coef2,
                                               unsigned short *coef3,
                                               unsigned short *coef4);
void redcloud_get_video_downscale_enable(int *enable);
unsigned long redcloud_get_video_dst_size(void);
unsigned long redcloud_get_video_position(void);
unsigned long redcloud_get_video_color_key(void);
unsigned long redcloud_get_video_color_key_mask(void);
int redcloud_get_video_palette_entry(unsigned long index,
                                     unsigned long *palette);
int redcloud_get_video_color_key_src(void);
int redcloud_get_video_filter(void);
int redcloud_get_video_cursor(unsigned long *key, unsigned long *mask,
                              unsigned short *select_color2,
                              unsigned long *color1, unsigned short *color2);
unsigned long redcloud_read_crc(void);
unsigned long redcloud_read_crc32(void);
unsigned long redcloud_read_window_crc(int source, unsigned short x,
                                       unsigned short y, unsigned short width,
                                       unsigned short height, int crc32);

void redcloud_get_alpha_enable(int *enable);
void redcloud_get_alpha_size(unsigned short *x, unsigned short *y,
                             unsigned short *width, unsigned short *height);
void redcloud_get_alpha_value(unsigned char *alpha, char *delta);
void redcloud_get_alpha_priority(int *priority);
void redcloud_get_alpha_color(unsigned long *color);
int redcloud_get_video_request(short *x, short *y);
#endif
#endif

/*--------------*/
/* VIP ROUTINES */
/*--------------*/

#if GFX_VIP_SC1200

/* DYNAMIC ROUTINES */

#if GFX_VIP_DYNAMIC
int sc1200_set_vip_enable(int enable);
int sc1200_set_vip_capture_run_mode(int mode);
int sc1200_set_vip_base(unsigned long even, unsigned long odd);
int sc1200_set_vip_pitch(unsigned long pitch);
int sc1200_set_vip_mode(int mode);
int sc1200_set_vbi_enable(int enable);
int sc1200_set_vbi_mode(int mode);
int sc1200_set_vbi_base(unsigned long even, unsigned long odd);
int sc1200_set_vbi_pitch(unsigned long pitch);
int sc1200_set_vbi_direct(unsigned long even_lines, unsigned long odd_lines);
int sc1200_set_vbi_interrupt(int enable);
int sc1200_set_vip_bus_request_threshold_high(int enable);
int sc1200_set_vip_last_line(int last_line);
int sc1200_test_vip_odd_field(void);
int sc1200_test_vip_bases_updated(void);
int sc1200_test_vip_fifo_overflow(void);
int sc1200_get_vip_line(void);
int sc1200_get_vip_enable(void);
unsigned long sc1200_get_vip_base(int odd);
unsigned long sc1200_get_vip_pitch(void);
int sc1200_get_vip_mode(void);
int sc1200_get_vbi_enable(void);
int sc1200_get_vbi_mode(void);
unsigned long sc1200_get_vbi_base(int odd);
unsigned long sc1200_get_vbi_pitch(void);
unsigned long sc1200_get_vbi_direct(int odd);
int sc1200_get_vbi_interrupt(void);
int sc1200_get_vip_bus_request_threshold_high(void);
#endif
#endif

/* DECODER ROUTINES */

#if GFX_DECODER_SAA7114

/* PRIVATE ROUTINES */

int saa7114_write_reg(unsigned char reg, unsigned char val);
int saa7114_read_reg(unsigned char reg, unsigned char *val);

/* DYNAMIC ROUTINES */

#if GFX_DECODER_DYNAMIC
int saa7114_set_decoder_defaults(void);
int saa7114_set_decoder_analog_input(unsigned char input);
int saa7114_set_decoder_brightness(unsigned char brightness);
int saa7114_set_decoder_contrast(unsigned char contrast);
int saa7114_set_decoder_hue(char hue);
int saa7114_set_decoder_saturation(unsigned char saturation);
int saa7114_set_decoder_input_offset(unsigned short x, unsigned short y);
int saa7114_set_decoder_input_size(unsigned short width, unsigned short height);
int saa7114_set_decoder_output_size(unsigned short width,
                                    unsigned short height);
int saa7114_set_decoder_scale(unsigned short srcw, unsigned short srch,
                              unsigned short dstw, unsigned short dsth);
int saa7114_set_decoder_vbi_format(int start, int end, int format);
int saa7114_set_decoder_vbi_enable(int enable);
int saa7114_set_decoder_vbi_upscale(void);
int saa7114_set_decoder_TV_standard(TVStandardType TVStandard);
int saa7114_set_decoder_luminance_filter(unsigned char lufi);
int saa7114_decoder_software_reset(void);
int saa7114_decoder_detect_macrovision(void);
int saa7114_decoder_detect_video(void);
unsigned char saa7114_get_decoder_brightness(void);
unsigned char saa7114_get_decoder_contrast(void);
char saa7114_get_decoder_hue(void);
unsigned char saa7114_get_decoder_saturation(void);
unsigned long saa7114_get_decoder_input_offset(void);
unsigned long saa7114_get_decoder_input_size(void);
unsigned long saa7114_get_decoder_output_size(void);
int saa7114_get_decoder_vbi_format(int line);
#endif
#endif

/* ACCESS I2C ROUTINES */

#if GFX_I2C_ACCESS

#if GFX_I2C_DYNAMIC
int acc_i2c_reset(unsigned char busnum, short adr, char freq);
int acc_i2c_write(unsigned char busnum, unsigned char chipadr,
                  unsigned char subadr, unsigned char bytes,
                  unsigned char *data);
int acc_i2c_read(unsigned char busnum, unsigned char chipadr,
                 unsigned char subadr, unsigned char bytes,
                 unsigned char *data);
int acc_i2c_select_gpio(int clock, int data);
int acc_i2c_init(void);
void acc_i2c_cleanup(void);
#endif
#endif

/* GPIO I2C ROUTINES */

#if GFX_I2C_GPIO

#if GFX_I2C_DYNAMIC
int gpio_i2c_reset(unsigned char busnum, short adr, char freq);
int gpio_i2c_write(unsigned char busnum, unsigned char chipadr,
                   unsigned char subadr, unsigned char bytes,
                   unsigned char *data);
int gpio_i2c_read(unsigned char busnum, unsigned char chipadr,
                  unsigned char subadr, unsigned char bytes,
                  unsigned char *data);
int gpio_i2c_select_gpio(int clock, int data);
int gpio_i2c_init(void);
void gpio_i2c_cleanup(void);
#endif
#endif

/* TV ROUTINES */

#if GFX_TV_SC1200

#if GFX_TV_DYNAMIC
int sc1200_set_tv_format(TVStandardType format, GfxOnTVType resolution);
int sc1200_set_tv_output(int output);
int sc1200_set_tv_enable(int enable);
int sc1200_set_tv_flicker_filter(int ff);
int sc1200_set_tv_sub_carrier_reset(int screset);
int sc1200_set_tv_vphase(int vphase);
int sc1200_set_tv_YC_delay(int delay);
int sc1200_set_tvenc_reset_interval(int interval);
int sc1200_set_tv_cc_enable(int enable);
int sc1200_set_tv_cc_data(unsigned char data1, unsigned char data2);
int sc1200_set_tv_display(int width, int height);
int sc1200_test_tvout_odd_field(void);
int sc1200_test_tvenc_odd_field(void);
int sc1200_set_tv_field_status_invert(int enable);
int sc1200_get_tv_vphase(void);
int sc1200_get_tv_enable(unsigned int *p_on);
int sc1200_get_tv_output(void);
int sc1200_get_tv_mode_count(TVStandardType format);
int sc1200_get_tv_display_mode(int *width, int *height, int *bpp, int *hz);
int sc1200_get_tv_display_mode_frequency(unsigned short width,
                                         unsigned short height,
                                         TVStandardType format, int *frequency);
int sc1200_is_tv_display_mode_supported(unsigned short width,
                                        unsigned short height,
                                        TVStandardType format);
unsigned char cc_add_parity_bit(unsigned char data);

#endif
#endif

/* FS450 ROUTINES */

#if GFX_TV_FS451

#if GFX_TV_DYNAMIC
int fs450_set_tv_format(TVStandardType format, GfxOnTVType resolution);
int fs450_set_tv_output(int output);
int fs450_set_tv_enable(int enable);
int fs450_get_tv_standard(unsigned long *p_standard);
int fs450_get_available_tv_standards(unsigned long *p_standards);
int fs450_set_tv_standard(unsigned long standard);
int fs450_get_tv_vga_mode(unsigned long *p_vga_mode);
int fs450_get_available_tv_vga_modes(unsigned long *p_vga_modes);
int fs450_set_tv_vga_mode(unsigned long vga_mode);
int fs450_get_tvout_mode(unsigned long *p_tvout_mode);
int fs450_set_tvout_mode(unsigned long tvout_mode);
int fs450_get_sharpness(int *p_sharpness);
int fs450_set_sharpness(int sharpness);
int fs450_get_flicker_filter(int *p_flicker);
int fs450_set_flicker_filter(int flicker);
int fs450_get_overscan(int *p_x, int *p_y);
int fs450_set_overscan(int x, int y);
int fs450_get_position(int *p_x, int *p_y);
int fs450_set_position(int x, int y);
int fs450_get_color(int *p_color);
int fs450_set_color(int color);
int fs450_get_brightness(int *p_brightness);
int fs450_set_brightness(int brightness);
int fs450_get_contrast(int *p_contrast);
int fs450_set_contrast(int contrast);
int fs450_get_yc_filter(unsigned int *p_yc_filter);
int fs450_set_yc_filter(unsigned int yc_filter);
int fs450_get_aps_trigger_bits(unsigned int *p_trigger_bits);
int fs450_set_aps_trigger_bits(unsigned int trigger_bits);
#endif
#endif
