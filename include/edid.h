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

#include <stdbool.h>
#include <stdint.h>
#include <X11/Xmd.h>
#include <X11/Xfuncproto.h>

#define STD_TIMINGS 8
#define DET_TIMINGS 4

/* input type */
#define DIGITAL(x) (x)

/* Msc stuff EDID Ver > 1.1 */
#define PREFERRED_TIMING_MODE(x) ((x) & 0x2)
#define GTF_SUPPORTED(x) ((x) & 0x1)

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

/**
 * @brief The monitor_hdr_data hdr metadata struct
 * This struct mirrors Table 92 - HDR Static Metadata Data Block (HDR SMDB) structure
 */
struct monitor_hdr_data {
    bool hdr_valid : 1;          /* static metadata v1 found, doesn't mean all fields are set */
    bool colorimetry_valid : 1;  /* if colorimetry tag found */

    bool tf_traditional_sdr : 1,
        tf_traditional_hdr : 1,
        tf_pq : 1,
        tf_hlg : 1,
        tf_4 : 1,
        tf_5 : 1;

    unsigned int resv : 2;  /* reserved bits */

    float desired_content_min_luminance;            /* in cd/m2, can be 0 */
    float desired_content_max_luminance;            /* in cd/m2, can be 0 */
    float desired_content_max_frame_avg_luminance;  /* in cd/m2, if unset 0 */

    bool sm_static_metadata_type1 : 1;

    unsigned int sm_1_7 : 7;  /* reserved bits */

    int reserved_1[4]; /* for future */

    unsigned int colorimetry_profiles : 16;

    int reserved_2[4]; /* for future */
};

#define CTA_HDR_SMDB_TF_SDR     1
#define CTA_HDR_SMDB_TF_HDR     2
#define CTA_HDR_SMDB_TF_ST2084  4
#define CTA_HDR_SMDB_TF_HLG     8

#define CTA_HDR_SMDB_SM_TYPE1   1

/* these constants are from Colorimetry Data Block (CDB) byte 3 and 4, converted into natural representation (big endian) uint16 */

#define is_CTA_CDB_BT2020_RGB(x)   (((x) & 0x8000) == 0x8000)
#define is_CTA_CDB_BT2020_YCC(x)   (((x) & 0x4000) == 0x4000)
#define is_CTA_CDB_BT2020_cYCC(x)  (((x) & 0x2000) == 0x2000)
#define is_CTA_CDB_opRGB(x)        (((x) & 0x1000) == 0x1000)
#define is_CTA_CDB_opYCC601(x)     (((x) & 0x0800) == 0x0800)
#define is_CTA_CDB_sYCC601(x)      (((x) & 0x0400) == 0x0400)
#define is_CTA_CDB_xvYCC709(x)     (((x) & 0x0200) == 0x0200)
#define is_CTA_CDB_xvYCC601(x)     (((x) & 0x0100) == 0x0100)
#define is_CTA_CDB_ST2113RGB(x)    (((x) & 0x0080) == 0x0080)
#define is_CTA_CDB_iCtCp(x)        (((x) & 0x0040) == 0x0040)

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
    struct monitor_hdr_data hdr;
} xf86Monitor, *xf86MonPtr;

extern _X_EXPORT xf86MonPtr ConfiguredMonitor;

/*
 * check whether monitor supports Generalized Timing Formula
 *
 * @param  monitor the monitor information structure to check
 * @return true if GTF is supported by the monitor
 */
_X_EXPORT bool xf86Monitor_gtf_supported(xf86MonPtr monitor);

#endif                          /* _EDID_H_ */
