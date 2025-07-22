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

 /*
  * Cimarron function prototypes.
  */

#ifndef _cim_rtns_h
#define _cim_rtns_h

#include <stdint.h>

/* INCLUDE USER PARAMETER DEFINITIONS */

#include "cim_parm.h"

/* COMPILER OPTION FOR C++ PROGRAMS */

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------*/
/* CIMARRON MEMORY POINTERS */
/*--------------------------*/

    extern unsigned char *cim_gp_ptr;
    extern unsigned char *cim_fb_ptr;
    extern unsigned char *cim_cmd_base_ptr;
    extern unsigned char *cim_cmd_ptr;
    extern unsigned char *cim_vid_ptr;
    extern unsigned char *cim_vip_ptr;
    extern unsigned char *cim_vg_ptr;

/*----------------------------------------*/
/* INITIALIZATION ROUTINE DEFINITIONS     */
/*----------------------------------------*/

    int init_detect_cpu(unsigned long *cpu_revision,
                        unsigned long *companion_revision);
    unsigned long init_read_pci(unsigned long address);
    int init_read_base_addresses(INIT_BASE_ADDRESSES * base_addresses);
    int init_read_cpu_frequency(unsigned long *cpu_frequency);

/*----------------------------------------*/
/* GRAPHICS PROCESSOR ROUTINE DEFINITIONS */
/*----------------------------------------*/

    void gp_set_limit_on_buffer_lead(unsigned long lead);
    void gp_set_command_buffer_base(unsigned long address,
                                    unsigned long start, unsigned long stop);
    void gp_set_frame_buffer_base(unsigned long address, unsigned long size);
    void gp_set_bpp(int bpp);
    void gp_declare_blt(unsigned long flags);
    void gp_declare_vector(unsigned long flags);
    void gp_write_parameters(void);
    void gp_set_raster_operation(unsigned char ROP);
    void gp_set_alpha_operation(int alpha_operation, int alpha_type,
                                int channel, int apply_alpha,
                                unsigned char alpha);
    void gp_set_solid_pattern(unsigned long color);
    void gp_set_mono_pattern(unsigned long bgcolor, unsigned long fgcolor,
                             unsigned long data0, unsigned long data1,
                             int transparent, int x, int y);
    void gp_set_pattern_origin(int x, int y);
    void gp_set_color_pattern(unsigned long *pattern, int format, int x, int y);
    void gp_set_mono_source(unsigned long bgcolor, unsigned long fgcolor,
                            int transparent);
    void gp_set_solid_source(unsigned long color);
    void gp_set_source_transparency(unsigned long color, unsigned long mask);
    void gp_program_lut(unsigned long *colors, int full_lut);
    void gp_set_vector_pattern(unsigned long pattern, unsigned long color,
                               int length);
    void gp_set_strides(unsigned long dst_stride, unsigned long src_stride);
    void gp_set_source_format(int format);
    void gp_pattern_fill(unsigned long dstoffset, unsigned long width,
                         unsigned long height);
    void gp_screen_to_screen_blt(unsigned long dstoffset,
                                 unsigned long srcoffset, unsigned long width,
                                 unsigned long height, int flags);
    void gp_screen_to_screen_convert(unsigned long dstoffset,
                                     unsigned long srcoffset,
                                     unsigned long width, unsigned long height,
                                     int nibble);
    void gp_color_bitmap_to_screen_blt(unsigned long dstoffset,
                                       unsigned long srcx, unsigned long width,
                                       unsigned long height,
                                       unsigned char *data, long pitch);
    void gp_color_convert_blt(unsigned long dstoffset, unsigned long srcx,
                              unsigned long width, unsigned long height,
                              unsigned char *data, long pitch);
    void gp_custom_convert_blt(unsigned long dstoffset, unsigned long srcx,
                               unsigned long width, unsigned long height,
                               unsigned char *data, long pitch);
    void gp_rotate_blt(unsigned long dstoffset, unsigned long srcoffset,
                       unsigned long width, unsigned long height, int degrees);
    void gp_mono_bitmap_to_screen_blt(unsigned long dstoffset,
                                      unsigned long srcx, unsigned long width,
                                      unsigned long height, unsigned char *data,
                                      long stride);
    void gp_text_blt(unsigned long dstoffset, unsigned long width,
                     unsigned long height, unsigned char *data);
    void gp_mono_expand_blt(unsigned long dstoffset, unsigned long srcoffset,
                            unsigned long srcx, unsigned long width,
                            unsigned long height, int byte_packed);
    void gp_antialiased_text(unsigned long dstoffset, unsigned long srcx,
                             unsigned long width, unsigned long height,
                             unsigned char *data, long stride, int fourbpp);
    void gp_blend_mask_blt(unsigned long dstoffset, unsigned long srcx,
                           unsigned long width, unsigned long height,
                           unsigned long, long stride, int operation,
                           int fourbpp);
    void gp_masked_blt(unsigned long dstoffset, unsigned long width,
                       unsigned long height, unsigned long mono_srcx,
                       unsigned long color_srcx, unsigned char *mono_mask,
                       unsigned char *color_data, long mono_pitch,
                       long color_pitch);
    void gp_screen_to_screen_masked(unsigned long dstoffset,
                                    unsigned long srcoffset,
                                    unsigned long width, unsigned long height,
                                    unsigned long mono_srcx,
                                    unsigned char *mono_mask, long mono_pitch);
    void gp_bresenham_line(unsigned long dstoffset, unsigned short length,
                           unsigned short initerr, unsigned short axialerr,
                           unsigned short diagerr, unsigned long flags);
    void gp_line_from_endpoints(unsigned long dstoffset, unsigned long x0,
                                unsigned long y0, unsigned long x1,
                                unsigned long y1, int inclusive);

    int gp_test_blt_pending(void);
    void gp_wait_blt_pending(void);
    void gp_wait_until_idle(void);
    int gp_test_blt_busy(void);
    void gp_save_state(GP_SAVE_RESTORE * gp_state);
    void gp_restore_state(GP_SAVE_RESTORE * gp_state);

/*----------------------------------------*/
/* VIDEO GENERATOR ROUTINE DEFINITIONS    */
/*----------------------------------------*/

    int vg_delay_milliseconds(unsigned long ms);
    int vg_set_display_mode(unsigned long src_width, unsigned long src_height,
                            unsigned long dst_width, unsigned long dst_height,
                            int bpp, int hz, unsigned long flags);
    int vg_set_panel_mode(unsigned long src_width, unsigned long src_height,
                          unsigned long dst_width, unsigned long dst_height,
                          unsigned long panel_width, unsigned long panel_height,
                          int bpp, unsigned long flags);
    int vg_set_tv_mode(unsigned long *src_width, unsigned long *src_height,
                       unsigned long encoder, unsigned long tvres, int bpp,
                       unsigned long flags, unsigned long h_overscan,
                       unsigned long v_overscan);
    int vg_set_custom_mode(VG_DISPLAY_MODE * mode_params, int bpp);
    int vg_set_display_bpp(int bpp);
    int vg_get_display_mode_index(VG_QUERY_MODE * query);
    int vg_get_display_mode_information(unsigned int index,
                                        VG_DISPLAY_MODE * vg_mode);
    int vg_get_display_mode_count(void);
    int vg_get_current_display_mode(VG_DISPLAY_MODE * current_display,
                                    int *bpp);
    int vg_set_scaler_filter_coefficients(long h_taps[][5], long v_taps[][3]);
    int vg_configure_flicker_filter(unsigned long flicker_strength,
                                    int flicker_alpha);
    int vg_set_clock_frequency(unsigned long frequency,
                               unsigned long pll_flags);
    int vg_set_border_color(unsigned long border_color);
    int vg_set_cursor_enable(int enable);
    int vg_set_mono_cursor_colors(unsigned long bkcolor, unsigned long fgcolor);
    int vg_set_cursor_position(long xpos, long ypos,
                               VG_PANNING_COORDINATES * panning);
    int vg_set_mono_cursor_shape32(unsigned long memoffset,
                                   unsigned long *andmask,
                                   unsigned long *xormask,
                                   unsigned long x_hotspot,
                                   unsigned long y_hotspot);
    int vg_set_mono_cursor_shape64(unsigned long memoffset,
                                   unsigned long *andmask,
                                   unsigned long *xormask,
                                   unsigned long x_hotspot,
                                   unsigned long y_hotspot);
    int vg_set_color_cursor_shape(unsigned long memoffset, unsigned char *data,
                                  unsigned long width, unsigned long height,
                                  long pitch, unsigned long x_hotspot,
                                  unsigned long y_hotspot);
    int vg_pan_desktop(unsigned long x, unsigned long y,
                       VG_PANNING_COORDINATES * panning);
    int vg_set_display_offset(unsigned long address);
    int vg_set_display_pitch(unsigned long pitch);
    int vg_set_display_palette_entry(unsigned long index,
                                     unsigned long palette);
    int vg_set_display_palette(unsigned long *palette);
    int vg_set_compression_enable(int enable);
    int vg_configure_compression(VG_COMPRESSION_DATA * comp_data);
    int vg_test_timing_active(void);
    int vg_test_vertical_active(void);
    int vg_wait_vertical_blank(void);
    int vg_test_even_field(void);
    int vg_configure_line_interrupt(VG_INTERRUPT_PARAMS * interrupt_info);
    unsigned long vg_test_and_clear_interrupt(void);
    unsigned long vg_test_flip_status(void);
    int vg_save_state(VG_SAVE_RESTORE * vg_state);
    int vg_restore_state(VG_SAVE_RESTORE * vg_state);

/*----------------------------------------*/
/* VIDEO GENERATOR READ ROUTINES          */
/*----------------------------------------*/

    unsigned long vg_read_graphics_crc(int crc_source);
    unsigned long vg_read_window_crc(int crc_source, unsigned long x,
                                     unsigned long y, unsigned long width,
                                     unsigned long height);
    int vg_get_scaler_filter_coefficients(long h_taps[][5], long v_taps[][3]);
    int vg_get_flicker_filter_configuration(unsigned long *strength,
                                            int *flicker_alpha);
    unsigned long vg_get_display_pitch(void);
    unsigned long vg_get_frame_buffer_line_size(void);
    unsigned long vg_get_current_vline(void);
    unsigned long vg_get_display_offset(void);
    int vg_get_cursor_info(VG_CURSOR_DATA * cursor_data);
    int vg_get_display_palette_entry(unsigned long index, unsigned long *entry);
    unsigned long vg_get_border_color(void);
    int vg_get_display_palette(unsigned long *palette);
    int vg_get_compression_info(VG_COMPRESSION_DATA * comp_data);
    int vg_get_compression_enable(void);
    int vg_get_valid_bit(int line);

/*----------------------------------------*/
/* DISPLAY FILTER ROUTINE DEFINITIONS     */
/*----------------------------------------*/

    int df_set_crt_enable(int crt_output);
    int df_set_panel_enable(int panel_output);
    int df_configure_video_source(DF_VIDEO_SOURCE_PARAMS * video_source_odd,
                                  DF_VIDEO_SOURCE_PARAMS * video_source_even);
    int df_set_video_offsets(int even, unsigned long y_offset,
                             unsigned long u_offset, unsigned long v_offset);
    int df_set_video_scale(unsigned long src_width, unsigned long src_height,
                           unsigned long dst_width, unsigned long dst_height,
                           unsigned long flags);
    int df_set_video_position(DF_VIDEO_POSITION * video_window);
    int df_set_video_filter_coefficients(long taps[][4], int phase256);
    int df_set_video_enable(int enable, unsigned long flags);
    int df_set_video_color_key(unsigned long key, unsigned long mask,
                               int graphics);
    int df_set_video_palette(unsigned long *palette);
    int df_set_video_palette_entry(unsigned long index, unsigned long palette);
    int df_configure_video_cursor_color_key(DF_VIDEO_CURSOR_PARAMS *
                                            cursor_color_key);
    int df_set_video_cursor_color_key_enable(int enable);
    int df_configure_alpha_window(int window,
                                  DF_ALPHA_REGION_PARAMS * alpha_data);
    int df_set_alpha_window_enable(int window, int enable);
    int df_set_no_ck_outside_alpha(int enable);
    int df_set_video_request(unsigned long x, unsigned long y);
    int df_set_output_color_space(int color_space);
    int df_set_output_path(int format);
    unsigned long df_test_video_flip_status(void);
    int df_save_state(DF_SAVE_RESTORE * gp_state);
    int df_restore_state(DF_SAVE_RESTORE * gp_state);

/*----------------------------------------*/
/*    DISPLAY FILTER READ ROUTINES        */
/*----------------------------------------*/

    unsigned long df_read_composite_crc(int crc_source);
    unsigned long df_read_composite_window_crc(unsigned long x,
                                               unsigned long y,
                                               unsigned long width,
                                               unsigned long height,
                                               int source);
    unsigned long df_read_panel_crc(void);
    int df_get_video_enable(int *enable, unsigned long *flags);
    int df_get_video_source_configuration(DF_VIDEO_SOURCE_PARAMS *
                                          video_source_odd,
                                          DF_VIDEO_SOURCE_PARAMS *
                                          video_source_even);
    int df_get_video_position(DF_VIDEO_POSITION * video_window);
    int df_get_video_scale(unsigned long *x_scale, unsigned long *y_scale);
    int df_get_video_filter_coefficients(long taps[][4], int *phase256);
    int df_get_video_color_key(unsigned long *key, unsigned long *mask,
                               int *graphics);
    int df_get_video_palette_entry(unsigned long index, unsigned long *palette);
    int df_get_video_palette(unsigned long *palette);
    int df_get_video_cursor_color_key(DF_VIDEO_CURSOR_PARAMS *
                                      cursor_color_key);
    int df_get_video_cursor_color_key_enable(void);
    int df_get_alpha_window_configuration(int window,
                                          DF_ALPHA_REGION_PARAMS * alpha_data);
    int df_get_alpha_window_enable(int window);
    int df_get_video_request(unsigned long *x, unsigned long *y);
    int df_get_output_color_space(int *color_space);

/*----------------------------------------*/
/*        MSR ROUTINE DEFINITIONS         */
/*----------------------------------------*/

    int msr_init_table(void);
    int msr_create_geodelink_table(GEODELINK_NODE * gliu_nodes);
    int msr_create_device_list(GEODELINK_NODE * gliu_nodes, int max_devices);
    int msr_read64(unsigned long device, unsigned long msr_register,
                   Q_WORD * msr_value);
    int msr_write64(unsigned long device, unsigned long msr_register,
                    Q_WORD * msr_value);

/*----------------------------------------*/
/*        VIP ROUTINE DEFINITIONS         */
/*----------------------------------------*/

    int vip_initialize(VIPSETMODEBUFFER * buffer);
    int vip_update_601_params(VIP_601PARAMS * buffer);
    int vip_terminate(void);
    int vip_configure_capture_buffers(int buffer_type, VIPINPUTBUFFER * buffer);
    int vip_toggle_video_offsets(int buffer_type, VIPINPUTBUFFER * buffer);
    int vip_max_address_enable(unsigned long max_address, int enable);
    int vip_set_interrupt_enable(unsigned long mask, int enable);
    unsigned long vip_get_interrupt_state(void);
    int vip_set_capture_state(unsigned long state);
    int vip_set_vsync_error(unsigned long vertical_count,
                            unsigned long window_before,
                            unsigned long window_after, int enable);
    int vip_configure_fifo(unsigned long fifo_type, unsigned long fifo_size);
    int vip_set_loopback_enable(int bEnable);
    int vip_configure_genlock(VIPGENLOCKBUFFER * buffer);
    int vip_set_genlock_enable(int bEnable);
    int vip_configure_pages(int page_count, unsigned long page_offset);
    int vip_set_interrupt_line(int line);
    int vip_reset(void);
    int vip_set_subwindow_enable(VIPSUBWINDOWBUFFER * buffer);
    int vip_reset_interrupt_state(unsigned long interrupt_mask);

    int vip_save_state(VIPSTATEBUFFER * save_buffer);
    int vip_restore_state(VIPSTATEBUFFER * restore_buffer);
    int vip_set_power_characteristics(VIPPOWERBUFFER * buffer);
    int vip_set_priority_characteristics(VIPPRIORITYBUFFER * buffer);
    int vip_set_debug_characteristics(VIPDEBUGBUFFER * buffer);
    int vip_test_genlock_active(void);
    int vip_test_signal_status(void);
    unsigned long vip_get_current_field(void);

/*----------------------------------------*/
/*        VIP READ ROUTINES               */
/*----------------------------------------*/

    int vip_get_current_mode(VIPSETMODEBUFFER * buffer);
    int vip_get_601_configuration(VIP_601PARAMS * buffer);
    int vip_get_buffer_configuration(int buffer_type, VIPINPUTBUFFER * buffer);
    int vip_get_genlock_configuration(VIPGENLOCKBUFFER * buffer);
    int vip_get_genlock_enable(void);
    int vip_is_buffer_update_latched(void);
    unsigned long vip_get_capture_state(void);
    unsigned long vip_get_current_line(void);
    unsigned long vip_read_fifo(unsigned long fifo_address);
    int vip_write_fifo(unsigned long fifo_address, unsigned long fifo_data);
    int vip_enable_fifo_access(int enable);
    int vip_get_capability_characteristics(VIPCAPABILITIESBUFFER * buffer);
    int vip_get_power_characteristics(VIPPOWERBUFFER * buffer);
    int vip_get_priority_characteristics(VIPPRIORITYBUFFER * buffer);

/*----------------------------------------*/
/*        VOP ROUTINE DEFINITIONS         */
/*----------------------------------------*/

    int vop_set_vbi_window(VOPVBIWINDOWBUFFER * buffer);
    int vop_enable_vbi_output(int enable);
    int vop_set_configuration(VOPCONFIGURATIONBUFFER * config);
    int vop_save_state(VOPSTATEBUFFER * save_buffer);
    int vop_restore_state(VOPSTATEBUFFER * save_buffer);

/*----------------------------------------*/
/*        VOP READ ROUTINES               */
/*----------------------------------------*/

    int vop_get_current_mode(VOPCONFIGURATIONBUFFER * config);
    int vop_get_vbi_configuration(VOPVBIWINDOWBUFFER * buffer);
    int vop_get_vbi_enable(void);
    unsigned long vop_get_crc(void);
    unsigned long vop_read_vbi_crc(void);

/* CLOSE BRACKET FOR C++ COMPLILATION */

#ifdef __cplusplus
}
#endif
#endif
