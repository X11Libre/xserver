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
 * This header file defines the Durango routines and variables used
 * to access the memory mapped regions.
 * */

#ifndef _gfx_rtns_h
#define _gfx_rtns_h

/* INCLUDE DURANGO DEFINITIONS */
/* These definitions are placed in another file to allow their inclusion
 * in a user application.  Such applications generally work through driver
 * shell routines that simply pass their parameters to Durango routines.
 * An external file provides an easy way to provide the definitions for these
 * parameters without the applications gaining any Durango visisbility.      
 * */

#include "gfx_type.h"

/* COMPILER OPTION FOR C++ PROGRAMS */

#ifdef __cplusplus
extern "C" {
#endif

/* DURANGO MEMORY POINTERS */

    extern unsigned char *gfx_virt_regptr;
    extern unsigned char *gfx_virt_fbptr;
    extern unsigned char *gfx_virt_vidptr;
    extern unsigned char *gfx_virt_vipptr;
    extern unsigned char *gfx_virt_spptr;
    extern unsigned char *gfx_virt_gpptr;

    extern unsigned char *gfx_phys_regptr;
    extern unsigned char *gfx_phys_fbptr;
    extern unsigned char *gfx_phys_vidptr;
    extern unsigned char *gfx_phys_vipptr;
    extern unsigned char *gfx_phys_spptr;
    extern unsigned char *gfx_phys_gpptr;

/* DURANGO VARIABLES FOR RUNTIME SELECTION AND POSSIBLE VALUES */

    extern int gfx_display_type;
#define GFX_DISPLAY_TYPE_GU1		0x0001
#define GFX_DISPLAY_TYPE_GU2		0x0002

    extern int gfx_init_type;
#define GFX_INIT_TYPE_GU1           0x0001
#define GFX_INIT_TYPE_GU2           0x0002

    extern int gfx_msr_type;
#define GFX_MSR_TYPE_REDCLOUD       0x0001

    extern int gfx_2daccel_type;
#define GFX_2DACCEL_TYPE_GU1		0x0001
#define GFX_2DACCEL_TYPE_GU2		0x0002

    extern int gfx_video_type;
#define GFX_VIDEO_TYPE_CS5530		0x0001
#define GFX_VIDEO_TYPE_SC1200		0x0002
#define GFX_VIDEO_TYPE_REDCLOUD     0x0004

    extern int gfx_vip_type;
#define GFX_VIP_TYPE_SC1200			0x0001

    extern int gfx_decoder_type;
#define GFX_DECODER_TYPE_SAA7114	0x0001

    extern int gfx_tv_type;
#define GFX_TV_TYPE_SC1200			0x0001
#define GFX_TV_TYPE_FS451			0x0002

    extern int gfx_i2c_type;
#define GFX_I2C_TYPE_ACCESS			0x0001
#define GFX_I2C_TYPE_GPIO			0x0002

/* GLOBAL CPU INFORMATION */

    extern unsigned long gfx_cpu_version;
    extern unsigned long gfx_cpu_frequency;
    extern unsigned long gfx_vid_version;
    extern ChipType gfx_chip_revision;

/* ROUTINES IN GFX_INIT.C */

    unsigned long gfx_pci_config_read(unsigned long address);
    void gfx_pci_config_write(unsigned long address, unsigned long data);
    unsigned long gfx_get_core_freq(void);
    unsigned long gfx_detect_cpu(void);
    unsigned long gfx_detect_video(void);
    unsigned long gfx_get_cpu_register_base(void);
    unsigned long gfx_get_graphics_register_base(void);
    unsigned long gfx_get_frame_buffer_base(void);
    unsigned long gfx_get_frame_buffer_size(void);
    unsigned long gfx_get_vid_register_base(void);
    unsigned long gfx_get_vip_register_base(void);

/* ROUTINES IN GFX_MSR.C */

    int gfx_msr_init(void);
    DEV_STATUS gfx_id_msr_device(MSR * pDev, unsigned long address);
    DEV_STATUS gfx_get_msr_dev_address(unsigned int device,
                                       unsigned long *address);
    DEV_STATUS gfx_get_glink_id_at_address(unsigned int *device,
                                           unsigned long address);
    DEV_STATUS gfx_msr_read(unsigned int device, unsigned int msrRegister,
                            Q_WORD * msrValue);
    DEV_STATUS gfx_msr_write(unsigned int device, unsigned int msrRegister,
                             Q_WORD * msrValue);

/* ROUTINES IN GFX_DISP.C */

    int gfx_set_display_bpp(unsigned short bpp);
    int gfx_is_display_mode_supported(int xres, int yres, int bpp, int hz);
    int gfx_set_display_mode(int xres, int yres, int bpp, int hz);
    int gfx_set_display_timings(unsigned short bpp, unsigned short flags,
                                unsigned short hactive,
                                unsigned short hblank_start,
                                unsigned short hsync_start,
                                unsigned short hsync_end,
                                unsigned short hblank_end,
                                unsigned short htotal, unsigned short vactive,
                                unsigned short vblank_start,
                                unsigned short vsync_start,
                                unsigned short vsync_end,
                                unsigned short vblank_end,
                                unsigned short vtotal, unsigned long frequency);
    int gfx_set_vtotal(unsigned short vtotal);
    void gfx_set_display_pitch(unsigned short pitch);
    void gfx_set_display_offset(unsigned long offset);
    int gfx_set_display_palette_entry(unsigned long index,
                                      unsigned long palette);
    int gfx_set_display_palette(unsigned long *palette);
    void gfx_video_shutdown(void);
    void gfx_set_clock_frequency(unsigned long frequency);
    int gfx_set_crt_enable(int enable);
    void gfx_set_cursor_enable(int enable);
    void gfx_set_cursor_colors(unsigned long bkcolor, unsigned long fgcolor);
    void gfx_set_cursor_position(unsigned long memoffset,
                                 unsigned short xpos, unsigned short ypos,
                                 unsigned short xhotspot,
                                 unsigned short yhotspot);
    void gfx_set_cursor_shape32(unsigned long memoffset, unsigned long *andmask,
                                unsigned long *xormask);
    void gfx_set_cursor_shape64(unsigned long memoffset, unsigned long *andmask,
                                unsigned long *xormask);
    void gfx_set_icon_enable(int enable);
    void gfx_set_icon_colors(unsigned long color0, unsigned long color1,
                             unsigned long color2);
    void gfx_set_icon_position(unsigned long memoffset, unsigned short xpos);
    void gfx_set_icon_shape64(unsigned long memoffset, unsigned long *andmask,
                              unsigned long *xormask, unsigned int lines);

    int gfx_set_compression_enable(int enable);
    int gfx_set_compression_offset(unsigned long offset);
    int gfx_set_compression_pitch(unsigned short pitch);
    int gfx_set_compression_size(unsigned short size);
    void gfx_set_display_priority_high(int enable);
    int gfx_test_timing_active(void);
    int gfx_test_vertical_active(void);
    int gfx_wait_vertical_blank(void);
    void gfx_delay_milliseconds(unsigned long milliseconds);
    void gfx_delay_microseconds(unsigned long microseconds);
    void gfx_enable_panning(int x, int y);
    int gfx_is_panel_mode_supported(int panelResX, int panelResY,
                                    unsigned short width, unsigned short height,
                                    unsigned short bpp);
    int gfx_set_fixed_timings(int panelResX, int panelResY,
                              unsigned short width, unsigned short height,
                              unsigned short bpp);
    int gfx_set_panel_present(int panelResX, int panelResY,
                              unsigned short width, unsigned short height,
                              unsigned short bpp);
    void gfx_reset_timing_lock(void);

/* "READ" ROUTINES IN GFX_DISP.C */

    int gfx_get_display_details(unsigned int mode, int *xres, int *yres,
                                int *hz);
    unsigned short gfx_get_display_pitch(void);
    int gfx_get_vsa2_softvga_enable(void);
    int gfx_get_sync_polarities(void);
    unsigned long gfx_get_clock_frequency(void);
    unsigned long gfx_get_max_supported_pixel_clock(void);
    int gfx_mode_frequency_supported(int xres, int yres, int bpp,
                                     unsigned long frequency);
    int gfx_get_refreshrate_from_frequency(int xres, int yres, int bpp,
                                           int *hz, unsigned long frequency);
    int gfx_get_refreshrate_from_mode(int xres, int yres, int bpp, int *hz,
                                      unsigned long frequency);
    int gfx_get_frequency_from_refreshrate(int xres, int yres, int bpp,
                                           int hz, int *frequency);
    int gfx_get_display_mode_count(void);
    int gfx_get_display_mode(int *xres, int *yres, int *bpp, int *hz);
    unsigned long gfx_get_frame_buffer_line_size(void);
    unsigned short gfx_get_hactive(void);
    unsigned short gfx_get_hblank_start(void);
    unsigned short gfx_get_hsync_start(void);
    unsigned short gfx_get_hsync_end(void);
    unsigned short gfx_get_hblank_end(void);
    unsigned short gfx_get_htotal(void);
    unsigned short gfx_get_vactive(void);
    unsigned short gfx_get_vline(void);
    unsigned short gfx_get_vblank_start(void);
    unsigned short gfx_get_vsync_start(void);
    unsigned short gfx_get_vsync_end(void);
    unsigned short gfx_get_vblank_end(void);
    unsigned short gfx_get_vtotal(void);
    unsigned short gfx_get_display_bpp(void);
    unsigned long gfx_get_display_offset(void);
    int gfx_get_display_palette_entry(unsigned long index,
                                      unsigned long *palette);
    void gfx_get_display_palette(unsigned long *palette);
    unsigned long gfx_get_cursor_enable(void);
    unsigned long gfx_get_cursor_offset(void);
    unsigned long gfx_get_cursor_position(void);
    unsigned long gfx_get_cursor_clip(void);
    unsigned long gfx_get_cursor_color(int color);
    unsigned long gfx_get_icon_enable(void);
    unsigned long gfx_get_icon_offset(void);
    unsigned long gfx_get_icon_position(void);
    unsigned long gfx_get_icon_color(int color);
    int gfx_get_compression_enable(void);
    unsigned long gfx_get_compression_offset(void);
    unsigned short gfx_get_compression_pitch(void);
    unsigned short gfx_get_compression_size(void);
    int gfx_get_display_priority_high(void);
    int gfx_get_valid_bit(int line);

/* ROUTINES IN GFX_RNDR.C */

    void gfx_set_bpp(unsigned short bpp);
    void gfx_set_solid_pattern(unsigned long color);
    void gfx_set_mono_pattern(unsigned long bgcolor, unsigned long fgcolor,
                              unsigned long data0, unsigned long data1,
                              unsigned char transparency);
    void gfx_set_color_pattern(unsigned long bgcolor, unsigned long fgcolor,
                               unsigned long data0, unsigned long data1,
                               unsigned long data2, unsigned long data3,
                               unsigned char transparency);
    void gfx_load_color_pattern_line(short y, unsigned long *pattern_8x8);
    void gfx_set_solid_source(unsigned long color);
    void gfx_set_mono_source(unsigned long bgcolor, unsigned long fgcolor,
                             unsigned short transparent);
    void gfx_set_pattern_flags(unsigned short flags);
    void gfx_set_raster_operation(unsigned char rop);
    void gfx_pattern_fill(unsigned short x, unsigned short y,
                          unsigned short width, unsigned short height);
    void gfx_color_pattern_fill(unsigned short x, unsigned short y,
                                unsigned short width, unsigned short height,
                                unsigned long *pattern);
    void gfx_screen_to_screen_blt(unsigned short srcx, unsigned short srcy,
                                  unsigned short dstx, unsigned short dsty,
                                  unsigned short width, unsigned short height);
    void gfx_screen_to_screen_xblt(unsigned short srcx, unsigned short srcy,
                                   unsigned short dstx, unsigned short dsty,
                                   unsigned short width, unsigned short height,
                                   unsigned long color);
    void gfx_color_bitmap_to_screen_blt(unsigned short srcx,
                                        unsigned short srcy,
                                        unsigned short dstx,
                                        unsigned short dsty,
                                        unsigned short width,
                                        unsigned short height,
                                        unsigned char *data, long pitch);
    void gfx_color_bitmap_to_screen_xblt(unsigned short srcx,
                                         unsigned short srcy,
                                         unsigned short dstx,
                                         unsigned short dsty,
                                         unsigned short width,
                                         unsigned short height,
                                         unsigned char *data, long pitch,
                                         unsigned long color);
    void gfx_mono_bitmap_to_screen_blt(unsigned short srcx, unsigned short srcy,
                                       unsigned short dstx, unsigned short dsty,
                                       unsigned short width,
                                       unsigned short height,
                                       unsigned char *data, short pitch);
    void gfx_text_blt(unsigned short dstx, unsigned short dsty,
                      unsigned short width, unsigned short height,
                      unsigned char *data);
    void gfx_bresenham_line(unsigned short x, unsigned short y,
                            unsigned short length, unsigned short initerr,
                            unsigned short axialerr, unsigned short diagerr,
                            unsigned short flags);
    void gfx_wait_until_idle(void);
    int gfx_test_blt_pending(void);

/* SECOND GENERATION RENDERING ROUTINES */

    void gfx2_set_source_stride(unsigned short stride);
    void gfx2_set_destination_stride(unsigned short stride);
    void gfx2_set_pattern_origin(int x, int y);
    void gfx2_set_source_transparency(unsigned long color, unsigned long mask);
    void gfx2_set_alpha_mode(int mode);
    void gfx2_set_alpha_value(unsigned char value);
    void gfx2_pattern_fill(unsigned long dstoffset, unsigned short width,
                           unsigned short height);
    void gfx2_color_pattern_fill(unsigned long dstoffset,
                                 unsigned short width, unsigned short height,
                                 unsigned long *pattern);
    void gfx2_screen_to_screen_blt(unsigned long srcoffset,
                                   unsigned long dstoffset,
                                   unsigned short width, unsigned short height,
                                   int flags);
    void gfx2_mono_expand_blt(unsigned long srcbase, unsigned short srcx,
                              unsigned short srcy, unsigned long dstoffset,
                              unsigned short width, unsigned short height,
                              int byte_packed);
    void gfx2_color_bitmap_to_screen_blt(unsigned short srcx,
                                         unsigned short srcy,
                                         unsigned long dstoffset,
                                         unsigned short width,
                                         unsigned short height,
                                         unsigned char *data, short pitch);
    void gfx2_mono_bitmap_to_screen_blt(unsigned short srcx,
                                        unsigned short srcy,
                                        unsigned long dstoffset,
                                        unsigned short width,
                                        unsigned short height,
                                        unsigned char *data, short pitch);
    void gfx2_text_blt(unsigned long dstoffset, unsigned short width,
                       unsigned short height, unsigned char *data);
    void gfx2_bresenham_line(unsigned long dstoffset, unsigned short length,
                             unsigned short initerr, unsigned short axialerr,
                             unsigned short diagerr, unsigned short flags);
    void gfx2_sync_to_vblank(void);

/* ROUTINES IN GFX_VID.C */

    int gfx_set_video_enable(int enable);
    int gfx_set_video_format(unsigned long format);
    int gfx_set_video_size(unsigned short width, unsigned short height);
    int gfx_set_video_yuv_pitch(unsigned long ypitch, unsigned long uvpitch);
    int gfx_set_video_offset(unsigned long offset);
    int gfx_set_video_yuv_offsets(unsigned long yoffset,
                                  unsigned long uoffset, unsigned long voffset);
    int gfx_set_video_window(short x, short y, unsigned short w,
                             unsigned short h);
    int gfx_set_video_left_crop(unsigned short x);
    int gfx_set_video_upscale(unsigned short srcw, unsigned short srch,
                              unsigned short dstw, unsigned short dsth);
    int gfx_set_video_scale(unsigned short srcw, unsigned short srch,
                            unsigned short dstw, unsigned short dsth);
    int gfx_set_video_vertical_downscale(unsigned short srch,
                                         unsigned short dsth);
    void gfx_set_video_vertical_downscale_enable(int enable);
    int gfx_set_video_downscale_config(unsigned short type, unsigned short m);
    int gfx_set_video_color_key(unsigned long key, unsigned long mask,
                                int bluescreen);
    int gfx_set_video_filter(int xfilter, int yfilter);
    int gfx_set_video_palette(unsigned long *palette);
    int gfx_set_graphics_palette(unsigned long *palette);
    int gfx_set_video_palette_bypass(int enable);
    int gfx_set_video_palette_entry(unsigned long index, unsigned long color);
    int gfx_set_graphics_palette_entry(unsigned long index,
                                       unsigned long color);
    int gfx_set_video_downscale_coefficients(unsigned short coef1,
                                             unsigned short coef2,
                                             unsigned short coef3,
                                             unsigned short coef4);
    int gfx_set_video_downscale_enable(int enable);
    int gfx_set_video_source(VideoSourceType source);
    int gfx_set_vbi_source(VbiSourceType source);
    int gfx_set_vbi_lines(unsigned long even, unsigned long odd);
    int gfx_set_vbi_total(unsigned long even, unsigned long odd);
    int gfx_set_video_interlaced(int enable);
    int gfx_set_color_space_YUV(int enable);
    int gfx_set_vertical_scaler_offset(char offset);
    int gfx_set_top_line_in_odd(int enable);
    int gfx_set_genlock_delay(unsigned long delay);
    int gfx_set_genlock_enable(int flags);
    int gfx_set_video_cursor(unsigned long key, unsigned long mask,
                             unsigned short select_color2, unsigned long color1,
                             unsigned long color2);
    int gfx_set_video_cursor_enable(int enable);
    int gfx_set_video_request(short x, short y);

    int gfx_select_alpha_region(int region);
    int gfx_set_alpha_enable(int enable);
    int gfx_set_alpha_window(short x, short y,
                             unsigned short width, unsigned short height);
    int gfx_set_alpha_value(unsigned char alpha, char delta);
    int gfx_set_alpha_priority(int priority);
    int gfx_set_alpha_color(unsigned long color);
    int gfx_set_alpha_color_enable(int enable);
    int gfx_set_no_ck_outside_alpha(int enable);
    int gfx_disable_softvga(void);
    int gfx_enable_softvga(void);
    int gfx_set_macrovision_enable(int enable);
    unsigned long gfx_get_max_video_width(void);

/* READ ROUTINES IN GFX_VID.C */

    int gfx_get_video_enable(void);
    int gfx_get_video_format(void);
    unsigned long gfx_get_video_src_size(void);
    unsigned long gfx_get_video_line_size(void);
    unsigned long gfx_get_video_xclip(void);
    unsigned long gfx_get_video_offset(void);
    void gfx_get_video_yuv_offsets(unsigned long *yoffset,
                                   unsigned long *uoffset,
                                   unsigned long *voffset);
    void gfx_get_video_yuv_pitch(unsigned long *ypitch, unsigned long *uvpitch);
    unsigned long gfx_get_video_upscale(void);
    unsigned long gfx_get_video_scale(void);
    unsigned long gfx_get_video_downscale_delta(void);
    int gfx_get_video_vertical_downscale_enable(void);
    int gfx_get_video_downscale_config(unsigned short *type, unsigned short *m);
    void gfx_get_video_downscale_coefficients(unsigned short *coef1,
                                              unsigned short *coef2,
                                              unsigned short *coef3,
                                              unsigned short *coef4);
    void gfx_get_video_downscale_enable(int *enable);
    unsigned long gfx_get_video_dst_size(void);
    unsigned long gfx_get_video_position(void);
    unsigned long gfx_get_video_color_key(void);
    unsigned long gfx_get_video_color_key_mask(void);
    int gfx_get_video_palette_entry(unsigned long index,
                                    unsigned long *palette);
    int gfx_get_video_color_key_src(void);
    int gfx_get_video_filter(void);
    int gfx_get_video_request(short *x, short *y);
    int gfx_get_video_source(VideoSourceType * source);
    int gfx_get_vbi_source(VbiSourceType * source);
    unsigned long gfx_get_vbi_lines(int odd);
    unsigned long gfx_get_vbi_total(int odd);
    int gfx_get_video_interlaced(void);
    int gfx_get_color_space_YUV(void);
    int gfx_get_vertical_scaler_offset(char *offset);
    unsigned long gfx_get_genlock_delay(void);
    int gfx_get_genlock_enable(void);
    int gfx_get_video_cursor(unsigned long *key, unsigned long *mask,
                             unsigned short *select_color2,
                             unsigned long *color1, unsigned short *color2);
    unsigned long gfx_read_crc(void);
    unsigned long gfx_read_crc32(void);
    unsigned long gfx_read_window_crc(int source, unsigned short x,
                                      unsigned short y, unsigned short width,
                                      unsigned short height, int crc32);
    int gfx_get_macrovision_enable(void);

    void gfx_get_alpha_enable(int *enable);
    void gfx_get_alpha_size(unsigned short *x, unsigned short *y,
                            unsigned short *width, unsigned short *height);
    void gfx_get_alpha_value(unsigned char *alpha, char *delta);
    void gfx_get_alpha_priority(int *priority);
    void gfx_get_alpha_color(unsigned long *color);

/* ROUTINES IN GFX_VIP.C */

    int gfx_set_vip_enable(int enable);
    int gfx_set_vip_capture_run_mode(int mode);
    int gfx_set_vip_base(unsigned long even, unsigned long odd);
    int gfx_set_vip_pitch(unsigned long pitch);
    int gfx_set_vip_mode(int mode);
    int gfx_set_vbi_enable(int enable);
    int gfx_set_vbi_mode(int mode);
    int gfx_set_vbi_base(unsigned long even, unsigned long odd);
    int gfx_set_vbi_pitch(unsigned long pitch);
    int gfx_set_vbi_direct(unsigned long even_lines, unsigned long odd_lines);
    int gfx_set_vbi_interrupt(int enable);
    int gfx_set_vip_bus_request_threshold_high(int enable);
    int gfx_set_vip_last_line(int last_line);
    int gfx_test_vip_odd_field(void);
    int gfx_test_vip_bases_updated(void);
    int gfx_test_vip_fifo_overflow(void);
    int gfx_get_vip_line(void);

/* READ ROUTINES IN GFX_VIP.C */

    int gfx_get_vip_enable(void);
    unsigned long gfx_get_vip_base(int odd);
    unsigned long gfx_get_vip_pitch(void);
    int gfx_get_vip_mode(void);
    int gfx_get_vbi_enable(void);
    int gfx_get_vbi_mode(void);
    unsigned long gfx_get_vbi_base(int odd);
    unsigned long gfx_get_vbi_pitch(void);
    unsigned long gfx_get_vbi_direct(int odd);
    int gfx_get_vbi_interrupt(void);
    int gfx_get_vip_bus_request_threshold_high(void);

/* ROUTINES IN GFX_DCDR.C */

    int gfx_set_decoder_defaults(void);
    int gfx_set_decoder_analog_input(unsigned char input);
    int gfx_set_decoder_brightness(unsigned char brightness);
    int gfx_set_decoder_contrast(unsigned char contrast);
    int gfx_set_decoder_hue(char hue);
    int gfx_set_decoder_saturation(unsigned char saturation);
    int gfx_set_decoder_input_offset(unsigned short x, unsigned short y);
    int gfx_set_decoder_input_size(unsigned short width, unsigned short height);
    int gfx_set_decoder_output_size(unsigned short width,
                                    unsigned short height);
    int gfx_set_decoder_scale(unsigned short srcw, unsigned short srch,
                              unsigned short dstw, unsigned short dsth);
    int gfx_set_decoder_vbi_format(int start, int end, int format);
    int gfx_set_decoder_vbi_enable(int enable);
    int gfx_set_decoder_vbi_upscale(void);
    int gfx_set_decoder_TV_standard(TVStandardType TVStandard);
    int gfx_set_decoder_luminance_filter(unsigned char lufi);
    int gfx_decoder_software_reset(void);
    int gfx_decoder_detect_macrovision(void);
    int gfx_decoder_detect_video(void);

/* READ ROUTINES IN GFX_DCDR.C */

    unsigned char gfx_get_decoder_brightness(void);
    unsigned char gfx_get_decoder_contrast(void);
    char gfx_get_decoder_hue(void);
    unsigned char gfx_get_decoder_saturation(void);
    unsigned long gfx_get_decoder_input_offset(void);
    unsigned long gfx_get_decoder_input_size(void);
    unsigned long gfx_get_decoder_output_size(void);
    int gfx_get_decoder_vbi_format(int line);

/* ROUTINES IN GFX_I2C.C */

    int gfx_i2c_reset(unsigned char busnum, short adr, char freq);
    int gfx_i2c_write(unsigned char busnum, unsigned char chipadr,
                      unsigned char subadr, unsigned char bytes,
                      unsigned char *data);
    int gfx_i2c_read(unsigned char busnum, unsigned char chipadr,
                     unsigned char subadr, unsigned char bytes,
                     unsigned char *data);
    int gfx_i2c_select_gpio(int clock, int data);
    int gfx_i2c_init(void);
    void gfx_i2c_cleanup(void);

/* ROUTINES IN GFX_TV.C */

    int gfx_set_tv_format(TVStandardType format, GfxOnTVType resolution);
    int gfx_set_tv_output(int output);
    int gfx_set_tv_enable(int enable);
    int gfx_set_tv_flicker_filter(int ff);
    int gfx_set_tv_sub_carrier_reset(int screset);
    int gfx_set_tv_vphase(int vphase);
    int gfx_set_tv_YC_delay(int delay);
    int gfx_set_tvenc_reset_interval(int interval);
    int gfx_set_tv_cc_enable(int enable);
    int gfx_set_tv_cc_data(unsigned char data1, unsigned char data2);
    int gfx_set_tv_display(int width, int height);
    int gfx_test_tvout_odd_field(void);
    int gfx_test_tvenc_odd_field(void);
    int gfx_set_tv_field_status_invert(int enable);
    int gfx_get_tv_vphase(void);
    int gfx_get_tv_enable(unsigned int *p_on);
    int gfx_get_tv_output(void);
    int gfx_get_tv_mode_count(TVStandardType format);
    int gfx_get_tv_display_mode(int *width, int *height, int *bpp, int *hz);
    int gfx_get_tv_display_mode_frequency(unsigned short width,
                                          unsigned short height,
                                          TVStandardType format,
                                          int *frequency);
    int gfx_is_tv_display_mode_supported(unsigned short width,
                                         unsigned short height,
                                         TVStandardType format);

    int gfx_get_tv_standard(unsigned long *p_standard);
    int gfx_get_available_tv_standards(unsigned long *p_standards);
    int gfx_set_tv_standard(unsigned long standard);
    int gfx_get_tv_vga_mode(unsigned long *p_vga_mode);
    int gfx_get_available_tv_vga_modes(unsigned long *p_vga_modes);
    int gfx_set_tv_vga_mode(unsigned long vga_mode);
    int gfx_get_tvout_mode(unsigned long *p_tvout_mode);
    int gfx_set_tvout_mode(unsigned long tvout_mode);
    int gfx_get_sharpness(int *p_sharpness);
    int gfx_set_sharpness(int sharpness);
    int gfx_get_flicker_filter(int *p_flicker);
    int gfx_set_flicker_filter(int flicker);
    int gfx_get_overscan(int *p_x, int *p_y);
    int gfx_set_overscan(int x, int y);
    int gfx_get_position(int *p_x, int *p_y);
    int gfx_set_position(int x, int y);
    int gfx_get_color(int *p_color);
    int gfx_set_color(int color);
    int gfx_get_brightness(int *p_brightness);
    int gfx_set_brightness(int brightness);
    int gfx_get_contrast(int *p_contrast);
    int gfx_set_contrast(int contrast);
    int gfx_get_yc_filter(unsigned int *p_yc_filter);
    int gfx_set_yc_filter(unsigned int yc_filter);
    int gfx_get_aps_trigger_bits(unsigned int *p_trigger_bits);
    int gfx_set_aps_trigger_bits(unsigned int trigger_bits);

/* ROUTINES IN GFX_VGA.C */

    int gfx_get_softvga_active(void);
    int gfx_vga_test_pci(void);
    unsigned char gfx_vga_get_pci_command(void);
    int gfx_vga_set_pci_command(unsigned char command);
    int gfx_vga_seq_reset(int reset);
    int gfx_vga_set_graphics_bits(void);
    int gfx_vga_mode(gfx_vga_struct * vga, int xres, int yres, int bpp, int hz);
    int gfx_vga_pitch(gfx_vga_struct * vga, unsigned short pitch);
    int gfx_vga_save(gfx_vga_struct * vga, int flags);
    int gfx_vga_restore(gfx_vga_struct * vga, int flags);
    int gfx_vga_mode_switch(int active);
    void gfx_vga_clear_extended(void);

/* CLOSE BRACKET FOR C++ COMPLILATION */

#ifdef __cplusplus
}
#endif
#endif                          /* !_gfx_rtns_h */
/* END OF FILE */
