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

#ifndef _XFREE86_EDID_PRIV_H_
#define _XFREE86_EDID_PRIV_H_

#include "edid.h"

/* read complete EDID record */
#define EDID1_LEN 128

/* header: 0x00 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0x00  */
#define HEADER_SECTION 0
#define HEADER_LENGTH 8

/* vendor section */
#define VENDOR_SECTION (HEADER_SECTION + HEADER_LENGTH)
#define V_MANUFACTURER 0
#define V_PROD_ID (V_MANUFACTURER + 2)
#define V_SERIAL (V_PROD_ID + 2)
#define V_WEEK (V_SERIAL + 4)
#define V_YEAR (V_WEEK + 1)
#define VENDOR_LENGTH (V_YEAR + 1)

/* EDID version */
#define VERSION_SECTION (VENDOR_SECTION + VENDOR_LENGTH)
#define V_VERSION 0
#define V_REVISION (V_VERSION + 1)
#define VERSION_LENGTH (V_REVISION + 1)

/* display information */
#define DISPLAY_SECTION (VERSION_SECTION + VERSION_LENGTH)
#define D_INPUT 0
#define D_HSIZE (D_INPUT + 1)
#define D_VSIZE (D_HSIZE + 1)
#define D_GAMMA (D_VSIZE + 1)
#define FEAT_S (D_GAMMA + 1)
#define D_RG_LOW (FEAT_S + 1)
#define D_BW_LOW (D_RG_LOW + 1)
#define D_REDX (D_BW_LOW + 1)
#define D_REDY (D_REDX + 1)
#define D_GREENX (D_REDY + 1)
#define D_GREENY (D_GREENX + 1)
#define D_BLUEX (D_GREENY + 1)
#define D_BLUEY (D_BLUEX + 1)
#define D_WHITEX (D_BLUEY + 1)
#define D_WHITEY (D_WHITEX + 1)
#define DISPLAY_LENGTH (D_WHITEY + 1)

/* supported VESA and other standard timings */
#define ESTABLISHED_TIMING_SECTION (DISPLAY_SECTION + DISPLAY_LENGTH)
#define E_T1 0
#define E_T2 (E_T1 + 1)
#define E_TMANU (E_T2 + 1)
#define E_TIMING_LENGTH (E_TMANU + 1)

/* non predefined standard timings supported by display */
#define STD_TIMING_SECTION (ESTABLISHED_TIMING_SECTION + E_TIMING_LENGTH)
#define STD_TIMING_INFO_LEN 2
#define STD_TIMING_INFO_NUM STD_TIMINGS
#define STD_TIMING_LENGTH (STD_TIMING_INFO_LEN * STD_TIMING_INFO_NUM)

/* detailed timing info of non standard timings */
#define DET_TIMING_SECTION (STD_TIMING_SECTION + STD_TIMING_LENGTH)
#define DET_TIMING_INFO_LEN 18
#define MONITOR_DESC_LEN DET_TIMING_INFO_LEN
#define DET_TIMING_INFO_NUM DET_TIMINGS
#define DET_TIMING_LENGTH (DET_TIMING_INFO_LEN * DET_TIMING_INFO_NUM)

/* number of EDID sections to follow */
#define NO_EDID (DET_TIMING_SECTION + DET_TIMING_LENGTH)
/* one byte checksum */
#define CHECKSUM (NO_EDID + 1)

#if (CHECKSUM != (EDID1_LEN - 1))
#error "EDID1 length != 128!"
#endif

#define SECTION(x,y) (uint8_t *)(x + y)
#define GET_ARRAY(y) ((uint8_t *)(c + y))
#define GET(y) *(uint8_t *)(c + y)

/* extract information from vendor section */
#define _PROD_ID(x) x[0] + (x[1] << 8);
#define _SERIAL_NO(x) x[0] + (x[1] << 8) + (x[2] << 16) + (x[3] << 24)
#define _YEAR(x) (x & 0xFF) + 1990
#define _L1(x) ((x[0] & 0x7C) >> 2) + '@'
#define _L2(x) ((x[0] & 0x03) << 3) + ((x[1] & 0xE0) >> 5) + '@'
#define _L3(x) (x[1] & 0x1F) + '@';

/* extract information from display section */
#define _INPUT_TYPE(x) ((x & 0x80) >> 7)
#define _INPUT_VOLTAGE(x) ((x & 0x60) >> 5)
#define _SETUP(x) ((x & 0x10) >> 4)
#define _SYNC(x) (x  & 0x0F)
#define _DFP(x) (x & 0x01)
#define _BPC(x) ((x & 0x70) >> 4)
#define _DIGITAL_INTERFACE(x) (x & 0x0F)
#define _GAMMA(x) (x == 0xff ? 0.0 : ((x + 100.0)/100.0))
#define _DPMS(x) ((x & 0xE0) >> 5)
#define _DISPLAY_TYPE(x) ((x & 0x18) >> 3)
#define _MSC(x) (x & 0x7)

/* color characteristics */
#define CC_L(x,y) ((x & (0x03 << y)) >> y)
#define CC_H(x) (x << 2)
#define I_CC(x,y,z) CC_H(y) | CC_L(x,z)
#define F_CC(x) ((x)/1024.0)

/* extract information from established timing section */
#define _VALID_TIMING(x) !(((x[0] == 0x01) && (x[1] == 0x01)) \
                        || ((x[0] == 0x00) && (x[1] == 0x00)) \
                        || ((x[0] == 0x20) && (x[1] == 0x20)) )
#define _HSIZE1(x) ((x[0] + 31) * 8)
#define RATIO(x) ((x[1] & 0xC0) >> 6)
#define RATIO1_1 0
/* EDID Ver. 1.3 redefined this */
#define RATIO16_10 RATIO1_1
#define RATIO4_3 1
#define RATIO5_4 2
#define RATIO16_9 3
#define _VSIZE1(x,y,r) switch(RATIO(x)){ \
  case RATIO1_1: y =  ((v->version > 1 || v->revision > 2) \
		       ? (_HSIZE1(x) * 10) / 16 : _HSIZE1(x)); break; \
  case RATIO4_3: y = _HSIZE1(x) * 3 / 4; break; \
  case RATIO5_4: y = _HSIZE1(x) * 4 / 5; break; \
  case RATIO16_9: y = _HSIZE1(x) * 9 / 16; break; \
  }
#define _REFRESH_R(x) (x[1] & 0x3F) + 60
#define _ID_LOW(x) x[0]
#define ID_LOW _ID_LOW(c)
#define _ID_HIGH(x) (x[1] << 8)
#define ID_HIGH _ID_HIGH(c)
#define STD_TIMING_ID (ID_LOW | ID_HIGH)
#define _NEXT_STD_TIMING(x)  (x = (x + STD_TIMING_INFO_LEN))

/* EDID Ver. >= 1.2 */
/**
 * Returns true if the pointer is the start of a monitor descriptor block
 * instead of a detailed timing descriptor.
 *
 * Checking the reserved pad fields for zeroes fails on some monitors with
 * broken empty ASCII strings.  Only the first two bytes are reliable.
 */
#define _IS_MONITOR_DESC(x) (x[0] == 0 && x[1] == 0)
#define _PIXEL_CLOCK(x) (x[0] + (x[1] << 8)) * 10000
#define PIXEL_CLOCK _PIXEL_CLOCK(c)
#define _H_ACTIVE(x) (x[2] + ((x[4] & 0xF0) << 4))
#define H_ACTIVE _H_ACTIVE(c)
#define _H_BLANK(x) (x[3] + ((x[4] & 0x0F) << 8))
#define H_BLANK _H_BLANK(c)
#define _V_ACTIVE(x) (x[5] + ((x[7] & 0xF0) << 4))
#define V_ACTIVE _V_ACTIVE(c)
#define _V_BLANK(x) (x[6] + ((x[7] & 0x0F) << 8))
#define V_BLANK _V_BLANK(c)
#define _H_SYNC_OFF(x) (x[8] + ((x[11] & 0xC0) << 2))
#define H_SYNC_OFF _H_SYNC_OFF(c)
#define _H_SYNC_WIDTH(x) (x[9] + ((x[11] & 0x30) << 4))
#define H_SYNC_WIDTH _H_SYNC_WIDTH(c)
#define _V_SYNC_OFF(x) ((x[10] >> 4) + ((x[11] & 0x0C) << 2))
#define V_SYNC_OFF _V_SYNC_OFF(c)
#define _V_SYNC_WIDTH(x) ((x[10] & 0x0F) + ((x[11] & 0x03) << 4))
#define V_SYNC_WIDTH _V_SYNC_WIDTH(c)
#define _H_SIZE(x) (x[12] + ((x[14] & 0xF0) << 4))
#define H_SIZE _H_SIZE(c)
#define _V_SIZE(x) (x[13] + ((x[14] & 0x0F) << 8))
#define V_SIZE _V_SIZE(c)
#define _H_BORDER(x) (x[15])
#define H_BORDER _H_BORDER(c)
#define _V_BORDER(x) (x[16])
#define V_BORDER _V_BORDER(c)
#define _INTERLACED(x) ((x[17] & 0x80) >> 7)
#define INTERLACED _INTERLACED(c)
#define _STEREO(x) ((x[17] & 0x60) >> 5)
#define STEREO _STEREO(c)
#define _STEREO1(x) (x[17] & 0x1)
#define STEREO1 _STEREO(c)
#define _SYNC_T(x) ((x[17] & 0x18) >> 3)
#define SYNC_T _SYNC_T(c)
#define _MISC(x) ((x[17] & 0x06) >> 1)
#define MISC _MISC(c)

#define _MONITOR_DESC_TYPE(x) x[3]
#define SERIAL_NUMBER 0xFF
#define ASCII_STR 0xFE
#define MONITOR_RANGES 0xFD
#define _MIN_V_OFFSET(x) ((!!(x[4] & 0x01)) * 255)
#define _MAX_V_OFFSET(x) ((!!(x[4] & 0x02)) * 255)
#define _MIN_H_OFFSET(x) ((!!(x[4] & 0x04)) * 255)
#define _MAX_H_OFFSET(x) ((!!(x[4] & 0x08)) * 255)
#define _MIN_V(x) x[5]
#define MIN_V (_MIN_V(c) + _MIN_V_OFFSET(c))
#define _MAX_V(x) x[6]
#define MAX_V (_MAX_V(c) + _MAX_V_OFFSET(c))
#define _MIN_H(x) x[7]
#define MIN_H (_MIN_H(c) + _MIN_H_OFFSET(c))
#define _MAX_H(x) x[8]
#define MAX_H (_MAX_H(c) + _MAX_H_OFFSET(c))
#define _MAX_CLOCK(x) x[9]
#define MAX_CLOCK _MAX_CLOCK(c)
#define _HAVE_2ND_GTF(x) (x[10] == 0x02)
#define HAVE_2ND_GTF _HAVE_2ND_GTF(c)
#define _F_2ND_GTF(x) (x[12] * 2)
#define F_2ND_GTF _F_2ND_GTF(c)
#define _C_2ND_GTF(x) (x[13] / 2)
#define C_2ND_GTF _C_2ND_GTF(c)
#define _M_2ND_GTF(x) (x[14] + (x[15] << 8))
#define M_2ND_GTF _M_2ND_GTF(c)
#define _K_2ND_GTF(x) (x[16])
#define K_2ND_GTF _K_2ND_GTF(c)
#define _J_2ND_GTF(x) (x[17] / 2)
#define J_2ND_GTF _J_2ND_GTF(c)
#define _HAVE_CVT(x) (x[10] == 0x04)
#define HAVE_CVT _HAVE_CVT(c)
#define _MAX_CLOCK_KHZ(x) (x[12] >> 2)
#define MAX_CLOCK_KHZ (MAX_CLOCK * 10000) - (_MAX_CLOCK_KHZ(c) * 250)
#define _MAXWIDTH(x) ((x[13] == 0 ? 0 : x[13] + ((x[12] & 0x03) << 8)) * 8)
#define MAXWIDTH _MAXWIDTH(c)
#define _SUPPORTED_ASPECT(x) x[14]
#define SUPPORTED_ASPECT _SUPPORTED_ASPECT(c)
#define  SUPPORTED_ASPECT_4_3   0x80
#define  SUPPORTED_ASPECT_16_9  0x40
#define  SUPPORTED_ASPECT_16_10 0x20
#define  SUPPORTED_ASPECT_5_4   0x10
#define  SUPPORTED_ASPECT_15_9  0x08
#define _PREFERRED_ASPECT(x) ((x[15] & 0xe0) >> 5)
#define PREFERRED_ASPECT _PREFERRED_ASPECT(c)
#define  PREFERRED_ASPECT_4_3   0
#define  PREFERRED_ASPECT_16_9  1
#define  PREFERRED_ASPECT_16_10 2
#define  PREFERRED_ASPECT_5_4   3
#define  PREFERRED_ASPECT_15_9  4
#define _SUPPORTED_BLANKING(x) ((x[15] & 0x18) >> 3)
#define SUPPORTED_BLANKING _SUPPORTED_BLANKING(c)
#define  CVT_STANDARD 0x01
#define  CVT_REDUCED  0x02
#define _SUPPORTED_SCALING(x) ((x[16] & 0xf0) >> 4)
#define SUPPORTED_SCALING _SUPPORTED_SCALING(c)
#define  SCALING_HSHRINK  0x08
#define  SCALING_HSTRETCH 0x04
#define  SCALING_VSHRINK  0x02
#define  SCALING_VSTRETCH 0x01
#define _PREFERRED_REFRESH(x) x[17]
#define PREFERRED_REFRESH _PREFERRED_REFRESH(c)

#define MONITOR_NAME 0xFC
#define ADD_COLOR_POINT 0xFB
#define WHITEX F_CC(I_CC((GET(D_BW_LOW)),(GET(D_WHITEX)),2))
#define WHITEY F_CC(I_CC((GET(D_BW_LOW)),(GET(D_WHITEY)),0))
#define _WHITEX_ADD(x,y) F_CC(I_CC(((*(x + y))),(*(x + y + 1)),2))
#define _WHITEY_ADD(x,y) F_CC(I_CC(((*(x + y))),(*(x + y + 2)),0))
#define _WHITE_INDEX1(x) x[5]
#define WHITE_INDEX1 _WHITE_INDEX1(c)
#define _WHITE_INDEX2(x) x[10]
#define WHITE_INDEX2 _WHITE_INDEX2(c)
#define WHITEX1 _WHITEX_ADD(c,6)
#define WHITEY1 _WHITEY_ADD(c,6)
#define WHITEX2 _WHITEX_ADD(c,12)
#define WHITEY2 _WHITEY_ADD(c,12)
#define _WHITE_GAMMA1(x) _GAMMA(x[9])
#define WHITE_GAMMA1 _WHITE_GAMMA1(c)
#define _WHITE_GAMMA2(x) _GAMMA(x[14])
#define WHITE_GAMMA2 _WHITE_GAMMA2(c)
#define ADD_STD_TIMINGS 0xFA
#define COLOR_MANAGEMENT_DATA 0xF9
#define CVT_3BYTE_DATA 0xF8
#define ADD_EST_TIMINGS 0xF7
#define ADD_DUMMY 0x10

#define _NEXT_DT_MD_SECTION(x) (x = (x + DET_TIMING_INFO_LEN))

#endif /* _XFREE86_EDID_PRIV_H_ */
