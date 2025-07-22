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
  * Cimarron user definitions.
  */

#ifndef _cim_parm_h
#define _cim_parm_h

/*===================================================*/
/*       CIMARRON RETURN VALUE DEFINITIONS           */
/*===================================================*/

#define CIM_STATUS_OK                     0x00000000
#define CIM_STATUS_ERROR                  0x00000001
#define CIM_STATUS_INVALIDPARAMS          0x00000002
#define CIM_STATUS_DEVNOTFOUND            0x00000004
#define CIM_STATUS_INVALIDSCALE           0x00000008
#define CIM_STATUS_INEXACTMATCH           0x00000010
#define CIM_STATUS_NOLOCK                 0x00000020
#define CIM_STATUS_CPUNOTFOUND            0x00000040
#define CIM_STATUS_DISPLAYUNAVAILABLE     0x00000080
#define CIM_STATUS_NOTFOUND               0x00000100

/*===================================================*/
/*       CIMARRON CPU DEFINITIONS                    */
/*===================================================*/

#define CIM_CPU_GEODEGX                   0x00000001
#define CIM_CPU_GEODELX                   0x00000002

#define CIM_SB_5535                       0x00000001
#define CIM_SB_5536                       0x00000002

/*===================================================*/
/* MSR PARAMETERS                                    */
/*===================================================*/

/*-------------------------------------------------------------*/
/* GEODELINK DEVICE IDS                                        */
/* These values uniquely identify all known GeodeLink devices  */
/* in GeodeLX and its companion, 5535/6.  For multiple devices */
/* of the same class (GLIU, USB, etc.) the table order is used */
/* to to identify the expected device order, in terms of on    */
/* which GLIU the device is found, and on which port.          */
/*-------------------------------------------------------------*/

#define MSR_DEVICE_GEODELX_GLIU0          0x00
#define MSR_DEVICE_GEODELX_GLIU1          0x01
#define MSR_DEVICE_5535_GLIU              0x02
#define MSR_DEVICE_GEODELX_GLCP           0x03
#define MSR_DEVICE_5535_GLCP              0x04
#define MSR_DEVICE_GEODELX_MPCI           0x05
#define MSR_DEVICE_5535_MPCI              0x06
#define MSR_DEVICE_GEODELX_MC             0x07
#define MSR_DEVICE_GEODELX_GP             0x08
#define MSR_DEVICE_GEODELX_VG             0x09
#define MSR_DEVICE_GEODELX_VIP            0x0A
#define MSR_DEVICE_GEODELX_AES            0x0B
#define MSR_DEVICE_GEODELX_DF             0x0C
#define MSR_DEVICE_GEODELX_FG             0x0D
#define MSR_DEVICE_GEODELX_VAIL           0x0E
#define MSR_DEVICE_5536_USB_2_0           0x0F
#define MSR_DEVICE_5535_USB2              0x10
#define MSR_DEVICE_5535_USB1              0x11
#define MSR_DEVICE_5535_ATAC              0x12
#define MSR_DEVICE_5535_MDD               0x13
#define MSR_DEVICE_5535_ACC               0x14
#define MSR_DEVICE_EMPTY                  0x15
#define MSR_DEVICE_REFLECTIVE             0x16
#define MSR_DEVICE_PRESENT                0x17
#define MSR_DEVICE_NOTFOUND               0x18

/*---------------------------------------------*/
/* GEODELINK TABLE ENTRY                       */
/* The following structure represents one port */
/* on a GeodeLink Interface Unit (GLIU)        */
/*---------------------------------------------*/

typedef struct tagGeodeLinkNode {
    unsigned long address_from_cpu;
    unsigned long device_id;

} GEODELINK_NODE;

/*---------------------------------------------*/
/* QWORD DATA STRUCTURE                        */
/* 64-bit data structure for MSR access.       */
/*---------------------------------------------*/

typedef struct tagQ_WORD {
    uint32_t high;
    uint32_t low;
} Q_WORD;

/*===================================================*/
/*       INITIALIZATION USER PARAMETERS              */
/*===================================================*/

typedef struct tagInitBaseAddresses {
    unsigned long framebuffer_base;
    unsigned long gp_register_base;
    unsigned long vg_register_base;
    unsigned long df_register_base;
    unsigned long vip_register_base;
    unsigned long framebuffer_size;

} INIT_BASE_ADDRESSES;

/*===================================================*/
/*          GP USER PARAMETER DEFINITIONS            */
/*===================================================*/

/*---------------------------*/
/* GP_DECLARE_BLT PARAMETERS */
/*---------------------------*/

#define CIMGP_BLTFLAGS_PRES_LUT          0x0001
#define CIMGP_BLTFLAGS_PRES_COLOR_PAT    0x0002
#define CIMGP_ENABLE_PREFETCH            0x0004
#define CIMGP_BLTFLAGS_HAZARD            0x0008
#define CIMGP_BLTFLAGS_INVERTMONO        0x0010
#define CIMGP_BLTFLAGS_LIMITBUFFER       0x0020

/*-----------------------------------*/
/* GP_SET_ALPHA_OPERATION PARAMETERS */
/*-----------------------------------*/

#define CIMGP_APPLY_BLEND_TO_RGB         1
#define CIMGP_APPLY_BLEND_TO_ALPHA       2
#define CIMGP_APPLY_BLEND_TO_ALL         3

#define CIMGP_ALPHA_TIMES_A              0
#define CIMGP_BETA_TIMES_B               1
#define CIMGP_A_PLUS_BETA_B              2
#define CIMGP_ALPHA_A_PLUS_BETA_B        3

#define CIMGP_CHANNEL_A_ALPHA            0
#define CIMGP_CHANNEL_B_ALPHA            1
#define CIMGP_CONSTANT_ALPHA             2
#define CIMGP_ALPHA_EQUALS_ONE           3
#define CIMGP_ALPHA_FROM_RGB_A           4
#define CIMGP_ALPHA_FROM_RGB_B           5
#define CIMGP_CONVERTED_ALPHA            6

#define CIMGP_CHANNEL_A_SOURCE           0
#define CIMGP_CHANNEL_A_DEST             1

/*---------------------------------*/
/* GP_SET_SOURCE_FORMAT PARAMETERS */
/*---------------------------------*/

#define CIMGP_SOURCE_FMT_3_3_2           0x00
#define CIMGP_SOURCE_FMT_8BPP_INDEXED    0x01
#define CIMGP_SOURCE_FMT_4_4_4_4         0x04
#define CIMGP_SOURCE_FMT_12BPP_BGR       0x14
#define CIMGP_SOURCE_FMT_1_5_5_5         0x05
#define CIMGP_SOURCE_FMT_15BPP_BGR       0x15
#define CIMGP_SOURCE_FMT_0_5_6_5         0x06
#define CIMGP_SOURCE_FMT_16BPP_BGR       0x16
#define CIMGP_SOURCE_FMT_YUYV            0x07
#define CIMGP_SOURCE_FMT_UYVY            0x17
#define CIMGP_SOURCE_FMT_8_8_8_8         0x08
#define CIMGP_SOURCE_FMT_32BPP_BGR       0x18
#define CIMGP_SOURCE_FMT_24BPP           0x0B
#define CIMGP_SOURCE_FMT_4BPP_INDEXED    0x0D

/*------------------------------------*/
/* GP_SCREEN_TO_SCREEN_BLT PARAMETERS */
/*------------------------------------*/

#define CIMGP_NEGXDIR                    1
#define CIMGP_NEGYDIR                    2

/*------------------------------------*/
/* GP_BRESENHAM_LINE PARAMETERS       */
/*------------------------------------*/

#define CIMGP_YMAJOR                     1
#define CIMGP_POSMAJOR                   2
#define CIMGP_POSMINOR                   4

/*----------------------------------------------*/
/* USER STRUCTURE FOR SAVING/RESTORING GP STATE */
/*----------------------------------------------*/

typedef struct tagGPSaveRestore {
    unsigned long base_offset;
    unsigned long cmd_top;
    unsigned long cmd_bottom;
    unsigned long cmd_base;
    unsigned long cmd_read;

} GP_SAVE_RESTORE;

/*===================================================*/
/*          VG USER PARAMETER DEFINITIONS            */
/*===================================================*/

/*-------------------------------------------*/
/*       SUPPORTED TV ENCODERS               */
/*-------------------------------------------*/

#define VG_ENCODER_ADV7171                0x0001
#define VG_ENCODER_SAA7127                0x0002
#define VG_ENCODER_FS454                  0x0003
#define VG_ENCODER_ADV7300                0x0004

/*-------------------------------------------*/
/*       SUPPORTED TV RESOLUTIONS            */
/*-------------------------------------------*/

#define VG_TVMODE_NTSC                    0x00000000
#define VG_TVMODE_PAL                     0x00000001
#define VG_TVMODE_480P                    0x00000002
#define VG_TVMODE_720P                    0x00000003
#define VG_TVMODE_1080I                   0x00000004
#define VG_TVMODE_6X4_NTSC                0x00000005
#define VG_TVMODE_8X6_NTSC                0x00000006
#define VG_TVMODE_10X7_NTSC               0x00000007
#define VG_TVMODE_6X4_PAL                 0x00000008
#define VG_TVMODE_8X6_PAL                 0x00000009
#define VG_TVMODE_10X7_PAL                0x0000000A

/*-------------------------------------------*/
/* USER STRUCTURE FOR SETTING A DISPLAY MODE */
/*-------------------------------------------*/

#define VG_SUPPORTFLAG_8BPP               0x00000001
#define VG_SUPPORTFLAG_12BPP              0x00000002
#define VG_SUPPORTFLAG_15BPP              0x00000004
#define VG_SUPPORTFLAG_16BPP              0x00000008
#define VG_SUPPORTFLAG_24BPP              0x00000010
#define VG_SUPPORTFLAG_32BPP              0x00000020
#define VG_SUPPORTFLAG_56HZ               0x00000040
#define VG_SUPPORTFLAG_60HZ               0x00000080
#define VG_SUPPORTFLAG_70HZ               0x00000100
#define VG_SUPPORTFLAG_72HZ               0x00000200
#define VG_SUPPORTFLAG_75HZ               0x00000400
#define VG_SUPPORTFLAG_85HZ               0x00000800
#define VG_SUPPORTFLAG_90HZ               0x00001000
#define VG_SUPPORTFLAG_100HZ              0x00002000
#define VG_SUPPORTFLAG_HZMASK             0x00003FC0
#define VG_SUPPORTFLAG_ADV7171            0x00004000
#define VG_SUPPORTFLAG_SAA7127            0x00008000
#define VG_SUPPORTFLAG_FS454              0x00010000
#define VG_SUPPORTFLAG_ADV7300            0x00020000
#define VG_SUPPORTFLAG_ENCODERMASK        0x0003C000
#define VG_SUPPORTFLAG_PANEL              0x00040000
#define VG_SUPPORTFLAG_TVOUT              0x00080000
#define VG_SUPPORTFLAG_NTSC               0x00000000
#define VG_SUPPORTFLAG_PAL                0x00100000
#define VG_SUPPORTFLAG_480P               0x00200000
#define VG_SUPPORTFLAG_720P               0x00300000
#define VG_SUPPORTFLAG_1080I              0x00400000
#define VG_SUPPORTFLAG_6X4_NTSC           0x00500000
#define VG_SUPPORTFLAG_8X6_NTSC           0x00600000
#define VG_SUPPORTFLAG_10X7_NTSC          0x00700000
#define VG_SUPPORTFLAG_6X4_PAL            0x00800000
#define VG_SUPPORTFLAG_8X6_PAL            0x00900000
#define VG_SUPPORTFLAG_10X7_PAL           0x00A00000
#define VG_SUPPORTFLAG_TVMODEMASK         0x00F00000

#define VG_MODEFLAG_NEG_HSYNC             0x00000001
#define VG_MODEFLAG_NEG_VSYNC             0x00000002
#define VG_MODEFLAG_INTERLACED            0x00000004
#define VG_MODEFLAG_PANELOUT              0x00000008
#define VG_MODEFLAG_CENTERED              0x00000010
#define VG_MODEFLAG_LINEARPITCH           0x00000020
#define VG_MODEFLAG_TVOUT                 0x00000040
#define VG_MODEFLAG_HALFCLOCK             0x00000080
#define VG_MODEFLAG_QVGA                  0x00000100
#define VG_MODEFLAG_EXCLUDEPLL            0x00000200
#define VG_MODEFLAG_NOPANELTIMINGS        0x00000400
#define VG_MODEFLAG_XVGA_TFT              0x00000800
#define VG_MODEFLAG_CUSTOM_PANEL          0x00001000
#define VG_MODEFLAG_CRT_AND_FP            0x00002000
#define VG_MODEFLAG_LOW_BAND              0x00000000
#define VG_MODEFLAG_AVG_BAND              0x00004000
#define VG_MODEFLAG_HIGH_BAND             0x00008000
#define VG_MODEFLAG_LEGACY_BAND           0x0000C000
#define VG_MODEFLAG_BANDWIDTHMASK         0x0000C000
#define VG_MODEFLAG_OVERRIDE_BAND         0x00010000
#define VG_MODEFLAG_INT_ADDRESS           0x00000000
#define VG_MODEFLAG_INT_LINEDOUBLE        0x00020000
#define VG_MODEFLAG_INT_FLICKER           0x00040000
#define VG_MODEFLAG_INT_MASK              0x00060000
#define VG_MODEFLAG_INT_OVERRIDE          0x00080000
#define VG_MODEFLAG_INVERT_SHFCLK         0x00100000
#define VG_MODEFLAG_MANUAL_FREQUENCY      0x00200000
#define VG_MODEFLAG_PLL_BYPASS            0x00400000
#define VG_MODEFLAG_VIP_TO_DOT_CLOCK      0x00800000

#define VG_MODEFLAG_VALIDUSERFLAGS        (VG_MODEFLAG_CRT_AND_FP     | \
                                           VG_MODEFLAG_XVGA_TFT       | \
                                           VG_MODEFLAG_NOPANELTIMINGS | \
                                           VG_MODEFLAG_EXCLUDEPLL     | \
                                           VG_MODEFLAG_LINEARPITCH)

typedef struct tagVGDisplayMode {
    /* DISPLAY MODE FLAGS */
    /* Includes BPP, refresh rate information, interlacing, etc. */

    unsigned long internal_flags;
    unsigned long flags;

    /* SOURCE RESOLUTION */
    /* The following values reflect the resolution of the data in the frame */
    /* buffer.  These values are used to enable scaling and filtering.      */

    unsigned long src_width;
    unsigned long src_height;

    /* PANEL SETTINGS
     * These allow a user to set a panel mode through the vg_set_custom_mode
     * routine.  These values are only relevant if the VG_MODEFLAG_PANEL is
     * also set.
     */

    unsigned long mode_width;
    unsigned long mode_height;
    unsigned long panel_width;
    unsigned long panel_height;
    unsigned long panel_tim1;
    unsigned long panel_tim2;
    unsigned long panel_dither_ctl;
    unsigned long panel_pad_sel_low;
    unsigned long panel_pad_sel_high;

    /* OUTPUT TIMINGS */
    /* If the active width and height do not match the source */
    /* dimensions the graphics data will be scaled.           */

    unsigned long hactive;
    unsigned long hblankstart;
    unsigned long hsyncstart;
    unsigned long hsyncend;
    unsigned long hblankend;
    unsigned long htotal;

    unsigned long vactive;
    unsigned long vblankstart;
    unsigned long vsyncstart;
    unsigned long vsyncend;
    unsigned long vblankend;
    unsigned long vtotal;

    unsigned long vactive_even;
    unsigned long vblankstart_even;
    unsigned long vsyncstart_even;
    unsigned long vsyncend_even;
    unsigned long vblankend_even;
    unsigned long vtotal_even;

    /* CLOCK FREQUENCY */

    unsigned long frequency;

} VG_DISPLAY_MODE;

/*-------------------------------------------*/
/*                PLL FLAGS                  */
/*-------------------------------------------*/

#define VG_PLL_DIVIDE_BY_2                0x00000001
#define VG_PLL_DIVIDE_BY_4                0x00000002
#define VG_PLL_BYPASS                     0x00000004
#define VG_PLL_MANUAL                     0x00000008
#define VG_PLL_VIP_CLOCK                  0x00000010

/*-------------------------------------------*/
/* USER STRUCTURE FOR QUERYING DISPLAY MODES */
/*-------------------------------------------*/

typedef struct tagQueryDisplayMode {
    int interlaced;
    int halfclock;
    unsigned long active_width;
    unsigned long active_height;
    unsigned long panel_width;
    unsigned long panel_height;
    unsigned long total_width;
    unsigned long total_height;
    unsigned long bpp;
    unsigned long hz;
    unsigned long frequency;
    unsigned long query_flags;
    unsigned long encoder;
    unsigned long tvmode;

} VG_QUERY_MODE;

/*-------------------------------------------*/
/* USER STRUCTURE FOR QUERYING CURSOR DATA   */
/*-------------------------------------------*/

typedef struct tagCursorData {
    int enable;
    int color_cursor;
    unsigned long cursor_offset;
    unsigned long cursor_x;
    unsigned long cursor_y;
    unsigned long clipx;
    unsigned long clipy;
    unsigned long mono_color0;
    unsigned long mono_color1;
    unsigned long flags;

} VG_CURSOR_DATA;

/*------------------------------------------------*/
/*          VG INTERRUPT STATUS SOURCES           */
/*------------------------------------------------*/

#define VG_INT_LINE_MATCH                 0x00010000
#define VG_INT_VSYNC_LOSS                 0x00020000

/*------------------------------------------------*/
/* USER STRUCTURE FOR SETTING COMPRESSION DATA    */
/*------------------------------------------------*/

typedef struct tagCompressionData {
    unsigned long compression_offset;
    unsigned long pitch;
    unsigned long size;
    unsigned long flags;

} VG_COMPRESSION_DATA;

/*-------------------------------------------------*/
/* USER STRUCTURE FOR CONFIGURING LINE INTERRUPTS  */
/*-------------------------------------------------*/

typedef struct tagInterruptInfo {
    unsigned long line;
    unsigned long flags;
    int enable;

} VG_INTERRUPT_PARAMS;

/*-------------------------------------------------*/
/* USER STRUCTURE FOR PANNING THE DESKTOP          */
/*-------------------------------------------------*/

typedef struct tagPanningInfo {
    unsigned long start_x;
    unsigned long start_y;
    int start_updated;

} VG_PANNING_COORDINATES;

/*--------------------------------------------------*/
/* USER STRUCTURE FOR SAVING/RESTORING THE VG STATE */
/*--------------------------------------------------*/

typedef struct tagVGSaveRestore {
    /* VG REGISTERS */

    unsigned long unlock;
    unsigned long gcfg;
    unsigned long dcfg;
    unsigned long arb_cfg;
    unsigned long fb_offset;
    unsigned long cb_offset;
    unsigned long cursor_offset;
    unsigned long video_y_offset;
    unsigned long video_u_offset;
    unsigned long video_v_offset;
    unsigned long dv_top;
    unsigned long line_size;
    unsigned long gfx_pitch;
    unsigned long video_yuv_pitch;
    unsigned long h_active;
    unsigned long h_blank;
    unsigned long h_sync;
    unsigned long v_active;
    unsigned long v_blank;
    unsigned long v_sync;
    unsigned long fb_active;
    unsigned long cursor_x;
    unsigned long cursor_y;
    unsigned long vid_ds_delta;
    unsigned long fb_base;
    unsigned long dv_ctl;
    unsigned long gfx_scale;
    unsigned long irq_ctl;
    unsigned long vbi_even_ctl;
    unsigned long vbi_odd_ctl;
    unsigned long vbi_hor_ctl;
    unsigned long vbi_odd_line_enable;
    unsigned long vbi_even_line_enable;
    unsigned long vbi_pitch;
    unsigned long color_key;
    unsigned long color_key_mask;
    unsigned long color_key_x;
    unsigned long color_key_y;
    unsigned long irq;
    unsigned long genlk_ctl;
    unsigned long vid_y_even_offset;
    unsigned long vid_u_even_offset;
    unsigned long vid_v_even_offset;
    unsigned long vactive_even;
    unsigned long vblank_even;
    unsigned long vsync_even;
    unsigned long h_coeff[512];
    unsigned long v_coeff[256];
    unsigned long palette[261];
    unsigned long cursor_data[3072];
    unsigned long dot_pll;
    unsigned long pll_flags;

    /* VG MSRS */

    Q_WORD msr_cap;
    Q_WORD msr_config;
    Q_WORD msr_smi;
    Q_WORD msr_error;
    Q_WORD msr_pm;
    Q_WORD msr_diag;
    Q_WORD msr_spare;
    Q_WORD msr_ram_ctl;

} VG_SAVE_RESTORE;

/*-------------------------------------------*/
/* VG_GET_DISPLAY_MODE_INDEX PARAMETERS      */
/*-------------------------------------------*/

#define VG_QUERYFLAG_ACTIVEWIDTH          0x00000001
#define VG_QUERYFLAG_ACTIVEHEIGHT         0x00000002
#define VG_QUERYFLAG_TOTALWIDTH           0x00000004
#define VG_QUERYFLAG_TOTALHEIGHT          0x00000008
#define VG_QUERYFLAG_BPP                  0x00000010
#define VG_QUERYFLAG_REFRESH              0x00000020
#define VG_QUERYFLAG_PIXELCLOCK           0x00000040
#define VG_QUERYFLAG_PIXELCLOCK_APPROX    0x00000080
#define VG_QUERYFLAG_PANEL                0x00000100
#define VG_QUERYFLAG_PANELWIDTH           0x00000200
#define VG_QUERYFLAG_PANELHEIGHT          0x00000400
#define VG_QUERYFLAG_TVOUT                0x00000800
#define VG_QUERYFLAG_INTERLACED           0x00001000
#define VG_QUERYFLAG_HALFCLOCK            0x00002000
#define VG_QUERYFLAG_ENCODER              0x00004000
#define VG_QUERYFLAG_TVMODE               0x00008000

/*-----------------------------------------------*/
/*         VG FLICKER FILTER SETTINGS            */
/*-----------------------------------------------*/

#define VG_FLICKER_FILTER_NONE            0x00000000
#define VG_FLICKER_FILTER_1_16            0x10000000
#define VG_FLICKER_FILTER_1_8             0x20000000
#define VG_FLICKER_FILTER_1_4             0x40000000
#define VG_FLICKER_FILTER_5_16            0x50000000
#define VG_FLICKER_FILTER_MASK            0xF0000000

/*-----------------------------------------------*/
/*            VG CRC SOURCES                     */
/*-----------------------------------------------*/

#define VG_CRC_SOURCE_PREFILTER           0x00000000
#define VG_CRC_SOURCE_PREFLICKER          0x00000001
#define VG_CRC_SOURCE_POSTFLICKER         0x00000002
#define VG_CRC_SOURCE_PREFILTER_EVEN      0x00000010
#define VG_CRC_SOURCE_PREFLICKER_EVEN     0x00000011
#define VG_CRC_SOURCE_POSTFLICKER_EVEN    0x00000012
#define VG_CRC_SOURCE_EVEN                0x00000010

/*===================================================*/
/* DISPLAY FILTER PARAMETERS                         */
/*===================================================*/

/*-----------------------------------------------*/
/*         VIDEO FORMAT DEFINITIONS              */
/*-----------------------------------------------*/

#define DF_VIDFMT_UYVY                    0x0000
#define DF_VIDFMT_Y2YU                    0x0001
#define DF_VIDFMT_YUYV                    0x0002
#define DF_VIDFMT_YVYU                    0x0003
#define DF_VIDFMT_Y0Y1Y2Y3                0x0004
#define DF_VIDFMT_Y3Y2Y1Y0                0x0005
#define DF_VIDFMT_Y1Y0Y3Y2                0x0006
#define DF_VIDFMT_Y1Y2Y3Y0                0x0007
#define DF_VIDFMT_RGB                     0x0008
#define DF_VIDFMT_P2M_P2L_P1M_P1L         0x0009
#define DF_VIDFMT_P1M_P1L_P2M_P2L         0x000A
#define DF_VIDFMT_P1M_P2L_P2M_P1L         0x000B

/*-----------------------------------------------*/
/*             CRT ENABLE STATES                 */
/*-----------------------------------------------*/

#define DF_CRT_DISABLE                    0x0000
#define DF_CRT_ENABLE                     0x0001
#define DF_CRT_STANDBY                    0x0002
#define DF_CRT_SUSPEND                    0x0003

/*-----------------------------------------------*/
/*             VIDEO SCALING FLAGS               */
/*-----------------------------------------------*/

#define DF_SCALEFLAG_CHANGEX              0x0001
#define DF_SCALEFLAG_CHANGEY              0x0002

/*-----------------------------------------------*/
/*       DISPLAY FILTER COLOR SPACES             */
/*-----------------------------------------------*/

#define DF_OUTPUT_RGB                     0x0001
#define DF_OUTPUT_ARGB                    0x0002
#define DF_OUTPUT_SDTV                    0x0003
#define DF_OUTPUT_HDTV                    0x0004

/*-----------------------------------------------*/
/*       DISPLAY FILTER OUTPUT PATHS             */
/*-----------------------------------------------*/

#define DF_DISPLAY_CRT                    0x0001
#define DF_DISPLAY_FP                     0x0002
#define DF_DISPLAY_CRT_FP                 0x0003
#define DF_DISPLAY_VOP                    0x0004
#define DF_DISPLAY_DRGB                   0x0005
#define DF_DISPLAY_CRT_DRGB               0x0006

/*-----------------------------------------------*/
/*       WINDOWED CRC DATA SOURCES               */
/*-----------------------------------------------*/

#define DF_CRC_SOURCE_GFX_DATA            0x0000
#define DF_CRC_SOURCE_CRT_RGB             0x0001
#define DF_CRC_SOURCE_FP_DATA             0x0002

/*-----------------------------------------------*/
/*          VIDEO ENABLE FLAGS                   */
/*-----------------------------------------------*/

#define DF_ENABLEFLAG_NOCOLORKEY          0x0001

/*-----------------------------------------------*/
/* USER STRUCTURE FOR CONFIGURING A VIDEO SOURCE */
/*-----------------------------------------------*/

#define DF_SOURCEFLAG_HDTVSOURCE          0x0001
#define DF_SOURCEFLAG_IMPLICITSCALING     0x0002

typedef struct tagVideoSourceInfo {
    unsigned long video_format;
    unsigned long y_offset;
    unsigned long u_offset;
    unsigned long v_offset;
    unsigned long y_pitch;
    unsigned long uv_pitch;
    unsigned long width;
    unsigned long height;
    unsigned long flags;

} DF_VIDEO_SOURCE_PARAMS;

/*---------------------------------------------------*/
/* USER STRUCTURE FOR CONFIGURING THE VIDEO POSITION */
/*---------------------------------------------------*/

#define DF_POSFLAG_DIRECTCLIP             0x0001
#define DF_POSFLAG_INCLUDEBORDER          0x0002

typedef struct tagVideoPosition {
    long x;
    long y;
    unsigned long width;
    unsigned long height;
    unsigned long left_clip;
    unsigned long dst_clip;
    unsigned long flags;

} DF_VIDEO_POSITION;

/*-------------------------------------------------*/
/* USER STRUCTURE FOR CONFIGURING THE VIDEO CURSOR */
/*-------------------------------------------------*/

typedef struct tagVideoCursorInfo {
    unsigned long key;
    unsigned long mask;
    unsigned long color1;
    unsigned long color2;
    unsigned long select_color2;
    unsigned long flags;

} DF_VIDEO_CURSOR_PARAMS;

/*-------------------------------------------------*/
/* USER STRUCTURE FOR CONFIGURING AN ALPHA REGION  */
/*-------------------------------------------------*/

#define DF_ALPHAFLAG_COLORENABLED         0x0001
#define DF_ALPHAFLAG_PERPIXELENABLED      0x0002

typedef struct tagAlphaRegionInfo {
    unsigned long x;
    unsigned long y;
    unsigned long width;
    unsigned long height;
    unsigned long alpha_value;
    unsigned long priority;
    unsigned long color;
    unsigned long flags;
    long delta;

} DF_ALPHA_REGION_PARAMS;

/*-------------------------------------------------*/
/* USER STRUCTURE FOR SAVING/RESTORING DF DATA     */
/*-------------------------------------------------*/

typedef struct tagDFSaveRestore {
    unsigned long vcfg;
    unsigned long dcfg;
    unsigned long video_x;
    unsigned long video_y;
    unsigned long video_scaler;
    unsigned long video_color_key;
    unsigned long video_color_mask;
    unsigned long sat_limit;
    unsigned long vid_misc;
    unsigned long video_yscale;
    unsigned long video_xscale;
    unsigned long vid_alpha_control;
    unsigned long cursor_key;
    unsigned long cursor_mask;
    unsigned long cursor_color1;
    unsigned long cursor_color2;
    unsigned long alpha_xpos1;
    unsigned long alpha_ypos1;
    unsigned long alpha_color1;
    unsigned long alpha_control1;
    unsigned long alpha_xpos2;
    unsigned long alpha_ypos2;
    unsigned long alpha_color2;
    unsigned long alpha_control2;
    unsigned long alpha_xpos3;
    unsigned long alpha_ypos3;
    unsigned long alpha_color3;
    unsigned long alpha_control3;
    unsigned long vid_request;
    unsigned long vid_ypos_even;
    unsigned long alpha_ypos_even1;
    unsigned long alpha_ypos_even2;
    unsigned long alpha_ypos_even3;
    unsigned long panel_tim1;
    unsigned long panel_tim2;
    unsigned long panel_pm;
    unsigned long panel_dither;

    unsigned long palette[256];
    unsigned long coefficients[512];

    /* DF MSRS */

    Q_WORD msr_cap;
    Q_WORD msr_config;
    Q_WORD msr_smi;
    Q_WORD msr_error;
    Q_WORD msr_pm;
    Q_WORD msr_diag;
    Q_WORD msr_df_diag;
    Q_WORD msr_pad_sel;

} DF_SAVE_RESTORE;

/*-----------------------------------------------*/
/*            DF CRC SOURCES                     */
/*-----------------------------------------------*/

#define DF_CRC_SOURCE_ODD_FIELD           0x00000100
#define DF_CRC_SOURCE_EVEN_FIELD          0x00001000
#define DF_CRC_SOURCE_EVEN                0x00001000

/*===================================================*/
/*          VIP USER PARAMETER DEFINITIONS           */
/*===================================================*/

#define VIP_MODEFLAG_VSYNCACTIVEHIGH      0x00000001
#define VIP_MODEFLAG_HSYNCACTIVEHIGH      0x00000002

/*---------------------------------------------*/
/* USER STRUCTURE FOR CONFIGURING 601 SETTINGS */
/*---------------------------------------------*/

typedef struct _TAG_VIP601PARAMS {
    unsigned long flags;
    unsigned long horz_start;
    unsigned long width;
    unsigned long vert_start_even;
    unsigned long even_height;
    unsigned long vert_start_odd;
    unsigned long odd_height;
    unsigned long vbi_start;
    unsigned long vbi_height;
    unsigned long odd_detect_start;
    unsigned long odd_detect_end;

} VIP_601PARAMS;

/*-------------------------------------------*/
/* USER STRUCTURE FOR CONFIGURING A VIP MODE */
/*-------------------------------------------*/

/* VIP MODE FLAGS */

#define VIP_MODEFLAG_PLANARCAPTURE        0x00000001
#define VIP_MODEFLAG_INVERTPOLARITY       0x00000002
#define VIP_MODEFLAG_PROGRESSIVE          0x00000004
#define VIP_MODEFLAG_DISABLEZERODETECT    0x00000008
#define VIP_MODEFLAG_ENABLEREPEATFLAG     0x00000010
#define VIP_MODEFLAG_10BITANCILLARY       0x00000020
#define VIP_MODEFLAG_TOGGLEEACHFIELD      0x00000040
#define VIP_MODEFLAG_INVERTTASKPOLARITY   0x00000080
#define VIP_MODEFLAG_FLIPMESSAGEWHENFULL  0x00000100

/* VIP CAPTURE ENABLE FLAGS */

#define VIP_ENABLE_TASKA                  0x00000100
#define VIP_ENABLE_TASKA_VBI              0x00000200
#define VIP_ENABLE_TASKB                  0x00000400
#define VIP_ENABLE_TASKB_VBI              0x00000800
#define VIP_ENABLE_ANCILLARY              0x00001000
#define VIP_ENABLE_ALL                    0x00001F00

/* VIP CAPTURE MODE FLAGS */

#define VIP_MODE_IDLE                     0x00000000
#define VIP_MODE_VIP2_8BIT                0x00000002
#define VIP_MODE_VIP2_16BIT               0x00000004
#define VIP_MODE_VIP1_8BIT                0x00000006
#define VIP_MODE_MSG                      0x00000008
#define VIP_MODE_DATA                     0x0000000A
#define VIP_MODE_8BIT601                  0x0000000C
#define VIP_MODE_16BIT601                 0x0000000E

/* 4:2:0 PLANAR CAPTURE METHODS */

#define VIP_420CAPTURE_EVERYLINE          0x00000001
#define VIP_420CAPTURE_ALTERNATINGLINES   0x00000002
#define VIP_420CAPTURE_ALTERNATINGFIELDS  0x00000003

typedef struct _TAG_SETMODEBUFFER {
    unsigned long flags;
    unsigned long stream_enables;
    unsigned long operating_mode;
    unsigned long planar_capture;
    VIP_601PARAMS vip601_settings;

} VIPSETMODEBUFFER;

/*-----------------------------------------------*/
/* USER STRUCTURE FOR CONFIGURING VG/VIP GENLOCK */
/*-----------------------------------------------*/

/* LOSS OF VIDEO DETECTION FLAGS */

#define VIP_VDE_RUNAWAY_LINE              0x00800000
#define VIP_VDE_VERTICAL_TIMING           0x00400000
#define VIP_VDE_CLOCKS_PER_LINE           0x00200000
#define VIP_VDE_LOST_CLOCK                0x00100000

/* VIP VSYNC SELECT FOR THE VG */

#define VIP_VGSYNC_NONE                  0x00000000
#define VIP_VGSYNC_START_FRAME           0x00000001
#define VIP_VGSYNC_FALLING_EDGE_VBLANK   0x00000002
#define VIP_VGSYNC_RISING_EDGE_VBLANK    0x00000003
#define VIP_VGSYNC_FALLING_EDGE_FIELD    0x00000004
#define VIP_VGSYNC_RISING_EDGE_FIELD     0x00000005
#define VIP_VGSYNC_VIP_CURRENT_LINE      0x00000006
#define VIP_VGSYNC_MSG_INT               0x00000007

/* VIP FIELD SELECT FOR THE VG */

#define VIP_VGFIELD_INPUT                0x00000000
#define VIP_VGFIELD_INPUT_INV            0x00000008
#define VIP_VGFIELD_ACTIVE_PAGE          0x00000010
#define VIP_VGFIELD_ACTIVE_PAGE_IN       0x00000018

/*--------------------------------------------------------*/
/* USER STRUCTURE FOR CONFIGURING THE VG VSYNC GENLOCK    */
/*--------------------------------------------------------*/

typedef struct _TAG_GENLOCKBUFFER {
    unsigned long vip_signal_loss;
    unsigned long vsync_to_vg;
    unsigned long field_to_vg;
    unsigned long genlock_skew;
    int enable_timeout;

} VIPGENLOCKBUFFER;

/*------------------------------------------------------*/
/* USER STRUCTURE FOR CONFIGURING VIP ANCILLARY CAPTURE */
/*------------------------------------------------------*/

typedef struct _TAG_ANCILLARYBUFFER {
    unsigned long msg1_base;
    unsigned long msg2_base;
    unsigned long msg_size;

} VIPANCILLARYBUFFER;

/*----------------------------------------------------*/
/* USER STRUCTURE FOR CONFIGURING VIP CAPTURE BUFFERS */
/*----------------------------------------------------*/

#define VIP_INPUTFLAG_VBI                 0x00000001
#define VIP_INPUTFLAG_INVERTPOLARITY      0x00000002
#define VIP_INPUTFLAG_PLANAR              0x00000004

#define VIP_MAX_BUFFERS                   10

#define VIP_BUFFER_TASK_A                 0x0000
#define VIP_BUFFER_TASK_B                 0x0001
#define VIP_BUFFER_MAX_TASKS              0x0002

#define	VIP_BUFFER_A                      0x0000
#define	VIP_BUFFER_B                      0x0001
#define	VIP_BUFFER_ANC                    0x0002
#define VIP_BUFFER_MSG                    0x0003
#define VIP_BUFFER_601                    0x0004
#define VIP_BUFFER_A_ODD                  0x0005
#define VIP_BUFFER_A_EVEN                 0x0006
#define VIP_BUFFER_B_ODD                  0x0007
#define VIP_BUFFER_B_EVEN                 0x0008

typedef struct _TAG_INPUTBUFFER_ADDR {
    unsigned long even_base[VIP_MAX_BUFFERS];
    unsigned long odd_base[VIP_MAX_BUFFERS];
    unsigned long y_pitch;
    unsigned long uv_pitch;
    unsigned long odd_uoffset;
    unsigned long odd_voffset;
    unsigned long even_uoffset;
    unsigned long even_voffset;
    unsigned long vbi_even_base;
    unsigned long vbi_odd_base;

} VIPINPUTBUFFER_ADDR;

typedef struct _TAG_SETINPUTBUFFER {
    unsigned long flags;
    VIPINPUTBUFFER_ADDR offsets[VIP_BUFFER_MAX_TASKS];
    unsigned long current_buffer;

    VIPANCILLARYBUFFER ancillaryData;

} VIPINPUTBUFFER;

/*------------------------------------------------------*/
/* USER STRUCTURE FOR CONFIGURING VIP SUBWINDOW CAPTURE */
/*------------------------------------------------------*/

typedef struct _TAG_SUBWINDOWBUFFER {
    int enable;
    unsigned long start;
    unsigned long stop;

} VIPSUBWINDOWBUFFER;

/*--------------------------------------------------------*/
/* USER STRUCTURE FOR SAVING/RESTORING VIP REGISTERS      */
/*--------------------------------------------------------*/

typedef struct _TAG_VIPSTATEBUFFER {
    unsigned long control1;
    unsigned long control2;
    unsigned long vip_int;
    unsigned long current_target;
    unsigned long max_address;
    unsigned long taska_evenbase;
    unsigned long taska_oddbase;
    unsigned long taska_vbi_evenbase;
    unsigned long taska_vbi_oddbase;
    unsigned long taska_data_pitch;
    unsigned long control3;
    unsigned long taska_v_oddoffset;
    unsigned long taska_u_oddoffset;
    unsigned long taskb_evenbase;
    unsigned long taskb_oddbase;
    unsigned long taskb_vbi_evenbase;
    unsigned long taskb_vbi_oddbase;
    unsigned long taskb_pitch;
    unsigned long taskb_voffset;
    unsigned long taskb_uoffset;
    unsigned long msg1_base;
    unsigned long msg2_base;
    unsigned long msg_size;
    unsigned long page_offset;
    unsigned long vert_start_stop;
    unsigned long vsync_err_count;
    unsigned long taska_u_evenoffset;
    unsigned long taska_v_evenoffset;

    Q_WORD msr_config;
    Q_WORD msr_smi;
    Q_WORD msr_pm;
    Q_WORD msr_diag;

} VIPSTATEBUFFER;

/*--------------------------------------------------------*/
/* VIP_SET_CAPTURE_STATE USER PARAMETERS                  */
/*--------------------------------------------------------*/

#define VIP_STOPCAPTURE                   0x0000
#define VIP_STOPCAPTUREATLINEEND          0x0001
#define VIP_STOPCAPTUREATFIELDEND         0x0002
#define VIP_STOPCAPTUREATFRAMEEND         0x0003
#define VIP_STARTCAPTUREATNEXTLINE        0x0004
#define VIP_STARTCAPTUREATNEXTFIELD       0x0005
#define VIP_STARTCAPTUREATNEXTFRAME       0x0006
#define VIP_STARTCAPTURE                  0x0007

/*--------------------------------------------------------*/
/* VIP_CONFIGURE_FIFO USER PARAMETERS                     */
/*--------------------------------------------------------*/

#define VIP_VIDEOTHRESHOLD                0x3000
#define	VIP_ANCILLARYTHRESHOLD            0x3001
#define	VIP_VIDEOFLUSH                    0x3002
#define VIP_ANCILLARYFLUSH                0x3003

/*--------------------------------------------------------*/
/* VIP_SET_INTERRUPT_ENABLE USER DEFINITIONS              */
/*--------------------------------------------------------*/

#define VIP_INT_FIFO_ERROR                0x80000000
#define VIP_INT_FIFO_WRAP                 0x40000000
#define VIP_INT_FIFO_OVERFLOW             0x20000000
#define VIP_INT_FIFO_THRESHOLD            0x10000000
#define VIP_INT_LONGLINE                  0x08000000
#define VIP_INT_VERTICAL_TIMING           0x04000000
#define VIP_INT_ACTIVE_PIXELS             0x02000000
#define VIP_INT_CLOCK_INPUT	              0x01000000
#define VIP_INT_ANC_CHECKSUM_PARITY       0x00800000
#define VIP_INT_MSG_BUFFER_FULL           0x00400000
#define VIP_INT_END_VBLANK                0x00200000
#define VIP_INT_START_VBLANK              0x00100000
#define VIP_INT_START_EVEN                0x00080000
#define VIP_INT_START_ODD                 0x00040000
#define VIP_INT_LINE_MATCH_TARGET         0x00020000
#define VIP_ALL_INTERRUPTS                0xFFFE0000

/*--------------------------------------------------------*/
/* VIP_GET_CURRENT_FIELD RETURN VALUES                    */
/*--------------------------------------------------------*/

#define VIP_ODD_FIELD                     1
#define VIP_EVEN_FIELD                    0

/*-------------------------------------------------*/
/* USER STRUCTURE FOR QUERYING VIP CAPABILITIES    */
/*-------------------------------------------------*/

typedef struct _TAG_CAPABILITIESBUFFER {
    unsigned long revision_id;
    unsigned long device_id;
    unsigned long n_clock_domains;
    unsigned long n_smi_registers;

} VIPCAPABILITIESBUFFER;

/*-------------------------------------------------*/
/* USER STRUCTURE FOR CONFIGURING VIP POWER        */
/*-------------------------------------------------*/

typedef struct _TAG_POWERBUFFER {
    int glink_clock_mode;
    int vip_clock_mode;

} VIPPOWERBUFFER;

/*-------------------------------------------------*/
/* USER STRUCTURE FOR CONFIGURING VIP PRIORITIES   */
/*-------------------------------------------------*/

typedef struct _TAG_PRIORITYBUFFER {
    unsigned long secondary;
    unsigned long primary;
    unsigned long pid;

} VIPPRIORITYBUFFER;

/*--------------------------------------------------*/
/* USER STRUCTURE FOR CONFIGURING VIP DEBUG OUTPUTS */
/*--------------------------------------------------*/

#define VIP_DIAG_UPPER_GLINK_MASTER       0x00010000
#define VIP_DIAG_UPPER_GLINK_SLAVE        0x00020000
#define VIP_DIAG_UPPER_GLINK_SLAVE_MMREG  0x00040000
#define VIP_DIAG_UPPER_Y_BUFFER           0x00080000
#define VIP_DIAG_UPPER_A_BUFFER           0x00100000
#define VIP_DIAG_UPPER_FIFO_OUTPUT        0x00200000
#define VIP_DIAG_UPPER_FIFO_INPUT         0x01000000
#define VIP_DIAG_UPPER_FORMATTER          0x02000000
#define VIP_DIAG_UPPER_INPUT_CONTROL      0x04000000

#define VIP_DIAG_LOWER_GLINK_MASTER       0x00000001
#define VIP_DIAG_LOWER_GLINK_SLAVE        0x00000002
#define VIP_DIAG_LOWER_GLINK_SLAVE_MMREG  0x00000004
#define VIP_DIAG_LOWER_Y_BUFFER           0x00000008
#define VIP_DIAG_LOWER_A_BUFFER           0x00000010
#define VIP_DIAG_LOWER_FIFO_OUTPUT        0x00000020
#define VIP_DIAG_LOWER_FIFO_INPUT         0x00000100
#define VIP_DIAG_LOWER_FORMATTER          0x00000200
#define VIP_DIAG_LOWER_INPUT_CONTROL      0x00000400

typedef struct _TAG_DEBUGBUFFER {
    unsigned long bist;
    unsigned long enable_upper;
    unsigned long select_upper;
    unsigned long enable_lower;
    unsigned long select_lower;

} VIPDEBUGBUFFER;

/*===================================================*/
/*          VOP USER PARAMETER DEFINITIONS           */
/*===================================================*/

/*------------------------------------------------------*/
/* USER STRUCTURE FOR CONFIGURING VBI CAPTURE           */
/*------------------------------------------------------*/

typedef struct _TAG_VBIWINDOWBUFFER {
    long horz_start;
    unsigned long vbi_width;
    unsigned long odd_line_capture_mask;
    unsigned long even_line_capture_mask;
    unsigned long odd_line_offset;
    unsigned long even_line_offset;
    unsigned long even_address_offset;
    unsigned long odd_address_offset;
    unsigned long data_size;
    unsigned long data_pitch;
    int enable_upscale;
    int horz_from_hsync;

} VOPVBIWINDOWBUFFER;

/*------------------------------------------------------*/
/* USER STRUCTURE FOR CONFIGURING 601 FOR VOP           */
/*------------------------------------------------------*/

#define VOP_601_INVERT_DISPE              0x00080000
#define VOP_601_INVERT_VSYNC              0x00040000
#define VOP_601_INVERT_HSYNC              0x00020000

#define VOP_VSYNC_EARLIER_BY4             0x00000000
#define VOP_VSYNC_EARLIER_BY2             0x00004000
#define VOP_VSYNC_NOSHIFT                 0x00008000
#define VOP_VSYNC_LATER_BY_X              0x0000C000

#define VOP_601_YUV_8BIT                  0x00000000
#define VOP_601_YUV_16BIT                 0x00000001
#define VOP_601_RGB_8_8_8                 0x00000002
#define VOP_601_YUV_4_4_4                 0x00000003

typedef struct _TAG_VOP601 {
    unsigned long flags;
    unsigned long vsync_shift;
    unsigned long vsync_shift_count;
    unsigned long output_mode;

} VOP_601DATA;

/*------------------------------------------------------*/
/* USER STRUCTURE FOR CONFIGURING VOP OUTPUT            */
/*------------------------------------------------------*/

/* VOP FLAGS */

#define VOP_FLAG_SINGLECHIPCOMPAT         0x00000001
#define VOP_FLAG_EXTENDEDSAV              0x00000002
#define VOP_FLAG_VBI                      0x00000008
#define VOP_FLAG_TASK                     0x00000010
#define VOP_FLAG_SWAP_UV                  0x00000020
#define VOP_FLAG_SWAP_VBI                 0x00000040

/* 4:4:2 TO 4:2:2 DECIMATION */

#define VOP_422MODE_COSITED               0x00000000
#define VOP_422MODE_INTERSPERSED          0x00000010
#define VOP_422MODE_ALTERNATING           0x00000020

/* VOP OPERATING MODES */

#define VOP_MODE_DISABLED                 0x00000000
#define VOP_MODE_VIP11                    0x00000001
#define VOP_MODE_CCIR656                  0x00000002
#define VOP_MODE_VIP20_8BIT               0x00000003
#define VOP_MODE_VIP20_16BIT              0x00000004
#define VOP_MODE_601                      0x00000005

/* VSYNC OUT SELECT FLAGS */

#define VOP_MB_SYNCSEL_DISABLED           0x00000000
#define VOP_MB_SYNCSEL_VG                 0x00000020
#define VOP_MB_SYNCSEL_VG_INV             0x00000040
#define VOP_MB_SYNCSEL_STATREG17          0x00000060
#define VOP_MB_SYNCSEL_STATREG17_INV      0x00000080

typedef struct _TAG_VOPMODECONFIGURATIONBUFFER {
    unsigned long flags;
    unsigned long mode;
    unsigned long conversion_mode;
    unsigned long vsync_out;
    VOP_601DATA vop601;

} VOPCONFIGURATIONBUFFER;

/*--------------------------------------------------------*/
/* USER STRUCTURE FOR SAVING/RESTORING VOP REGISTERS      */
/*--------------------------------------------------------*/

typedef struct _TAG_VOPSTATEBUFFER {
    unsigned long config;
} VOPSTATEBUFFER;

#endif
