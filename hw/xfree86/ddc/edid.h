/*
 * edid.h: defines to parse an EDID block
 *
 * This file contains all information to interpret a standard EDIC block
 * transmitted by a display device via DDC (Display Data Channel). So far
 * there is no information to deal with optional EDID blocks.
 * DDC is a Trademark of VESA (Video Electronics Standard Association).
 *
 * Copyright 1998 by Egbert Eich <Egbert.Eich@Physik.TU-Darmstadt.DE>
 */

#ifndef _EDID_H_
#define _EDID_H_

#include <stdint.h>
#include <X11/Xmd.h>
#include <X11/Xfuncproto.h>

/* read complete EDID record */
#define EDID1_LEN 128

#define STD_TIMINGS 8
#define DET_TIMINGS 4

/* input type */
#define DIGITAL(x) x

/* sync characteristics */
#define SEP_SYNC(x) (x & 0x08)
#define COMP_SYNC(x) (x & 0x04)
#define SYNC_O_GREEN(x) (x & 0x02)
#define SYNC_SERR(x) (x & 0x01)

/* Msc stuff EDID Ver > 1.1 */
#define PREFERRED_TIMING_MODE(x) (x & 0x2)
#define GTF_SUPPORTED(x) (x & 0x1)
#define CVT_SUPPORTED(x) (x & 0x1)

struct vendor {
    char name[4];
    int prod_id;
    unsigned int serial;
    int week;
    int year;
};

struct edid_version {
    int version;
    int revision;
};

struct disp_features {
    unsigned int input_type:1;
    unsigned int input_voltage:2;
    unsigned int input_setup:1;
    unsigned int input_sync:5;
    unsigned int input_dfp:1;
    unsigned int input_bpc:3;
    unsigned int input_interface:4;
    /* 15 bit hole */
    int hsize;
    int vsize;
    float gamma;
    unsigned int dpms:3;
    unsigned int display_type:2;
    unsigned int msc:3;
    float redx;
    float redy;
    float greenx;
    float greeny;
    float bluex;
    float bluey;
    float whitex;
    float whitey;
};

struct established_timings {
    uint8_t t1;
    uint8_t t2;
    uint8_t t_manu;
};

struct std_timings {
    int hsize;
    int vsize;
    int refresh;
    CARD16 id;
};

struct detailed_timings {
    int clock;
    int h_active;
    int h_blanking;
    int v_active;
    int v_blanking;
    int h_sync_off;
    int h_sync_width;
    int v_sync_off;
    int v_sync_width;
    int h_size;
    int v_size;
    int h_border;
    int v_border;
    unsigned int interlaced:1;
    unsigned int stereo:2;
    unsigned int sync:2;
    unsigned int misc:2;
    unsigned int stereo_1:1;
};

#define DT 0
#define DS_SERIAL 0xFF
#define DS_ASCII_STR 0xFE
#define DS_NAME 0xFC
#define DS_RANGES 0xFD
#define DS_WHITE_P 0xFB
#define DS_STD_TIMINGS 0xFA
#define DS_CMD 0xF9
#define DS_CVT 0xF8
#define DS_EST_III 0xF7
#define DS_DUMMY 0x10
#define DS_UNKOWN 0x100         /* type is an int */
#define DS_VENDOR 0x101
#define DS_VENDOR_MAX 0x110

/*
 * Display range limit Descriptor of EDID version1, reversion 4
 */
typedef enum {
	DR_DEFAULT_GTF,
	DR_LIMITS_ONLY,
	DR_SECONDARY_GTF,
	DR_CVT_SUPPORTED = 4,
} DR_timing_flags;

struct monitor_ranges {
    int min_v;
    int max_v;
    int min_h;
    int max_h;
    int max_clock;              /* in mhz */
    int gtf_2nd_f;
    int gtf_2nd_c;
    int gtf_2nd_m;
    int gtf_2nd_k;
    int gtf_2nd_j;
    int max_clock_khz;
    int maxwidth;               /* in pixels */
    char supported_aspect;
    char preferred_aspect;
    char supported_blanking;
    char supported_scaling;
    int preferred_refresh;      /* in hz */
    DR_timing_flags display_range_timing_flags;
};

struct whitePoints {
    int index;
    float white_x;
    float white_y;
    float white_gamma;
};

struct cvt_timings {
    int width;
    int height;
    int rate;
    int rates;
};

/*
 * Be careful when adding new sections; this structure can't grow, it's
 * embedded in the middle of xf86Monitor which is ABI.  Sizes below are
 * in bytes, for ILP32 systems.  If all else fails just copy the section
 * literally like serial and friends.
 */
struct detailed_monitor_section {
    int type;
    union {
        struct detailed_timings d_timings;      /* 56 */
        uint8_t serial[13];
        uint8_t ascii_data[13];
        uint8_t name[13];
        struct monitor_ranges ranges;   /* 60 */
        struct std_timings std_t[5];    /* 80 */
        struct whitePoints wp[2];       /* 32 */
        /* color management data */
        struct cvt_timings cvt[4];      /* 64 */
        uint8_t est_iii[6];       /* 6 */
    } section;                  /* max: 80 */
};

/* flags */
#define MONITOR_EDID_COMPLETE_RAWDATA	0x01
/* old, don't use */
#define EDID_COMPLETE_RAWDATA		0x01

/*
 * For DisplayID devices, only the scrnIndex, flags, and rawData fields
 * are meaningful.  For EDID, they all are.
 */
typedef struct {
    int scrnIndex;
    struct vendor vendor;
    struct edid_version ver;
    struct disp_features features;
    struct established_timings timings1;
    struct std_timings timings2[8];
    struct detailed_monitor_section det_mon[4];
    unsigned long flags;
    int no_sections;
    uint8_t *rawData;
} xf86Monitor, *xf86MonPtr;

extern _X_EXPORT xf86MonPtr ConfiguredMonitor;

#define EXT_TAG 0
#define EXT_REV 1
#define CEA_EXT   0x02
#define VTB_EXT   0x10
#define DI_EXT    0x40
#define LS_EXT    0x50
#define MI_EXT    0x60

#define CEA_EXT_MIN_DATA_OFFSET 4
#define CEA_EXT_MAX_DATA_OFFSET 127
#define CEA_EXT_DET_TIMING_NUM 6

#define IEEE_ID_HDMI    0x000C03
#define CEA_AUDIO_BLK   1
#define CEA_VIDEO_BLK   2
#define CEA_VENDOR_BLK  3
#define CEA_SPEAKER_ALLOC_BLK 4
#define CEA_VESA_DTC_BLK 5
#define VENDOR_LATENCY_PRESENT(x)     ( (x) >> 7)
#define VENDOR_LATENCY_PRESENT_I(x) ( ( (x) >> 6) & 0x01)
#define HDMI_MAX_TMDS_UNIT   (5000)

struct cea_video_block {
    uint8_t video_code;
};

struct cea_audio_block_descriptor {
    uint8_t audio_code[3];
};

struct cea_audio_block {
    struct cea_audio_block_descriptor descriptor[10];
};

struct cea_vendor_block_hdmi {
    uint8_t portB:4;
    uint8_t portA:4;
    uint8_t portD:4;
    uint8_t portC:4;
    uint8_t support_flags;
    uint8_t max_tmds_clock;
    uint8_t latency_present;
    uint8_t video_latency;
    uint8_t audio_latency;
    uint8_t interlaced_video_latency;
    uint8_t interlaced_audio_latency;
};

struct cea_vendor_block {
    unsigned char ieee_id[3];
    union {
        struct cea_vendor_block_hdmi hdmi;
        /* any other vendor blocks we know about */
    };
};

struct cea_speaker_block {
    uint8_t FLR:1;
    uint8_t LFE:1;
    uint8_t FC:1;
    uint8_t RLR:1;
    uint8_t RC:1;
    uint8_t FLRC:1;
    uint8_t RLRC:1;
    uint8_t FLRW:1;
    uint8_t FLRH:1;
    uint8_t TC:1;
    uint8_t FCH:1;
    uint8_t Resv:5;
    uint8_t ResvByte;
};

struct cea_data_block {
    uint8_t len:5;
    uint8_t tag:3;
    union {
        struct cea_video_block video;
        struct cea_audio_block audio;
        struct cea_vendor_block vendor;
        struct cea_speaker_block speaker;
    } u;
};

#endif                          /* _EDID_H_ */
