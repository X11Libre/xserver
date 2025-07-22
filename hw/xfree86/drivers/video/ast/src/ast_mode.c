/*
 * Copyright (c) 2005 ASPEED Technology Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the authors not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The authors makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THE AUTHORS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
#include <dix-config.h>

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86cmap.h"
#include "compiler.h"
#include "vgaHW.h"
#include "mipointer.h"
#include "micmap.h"

#include "fb.h"
#include "regionstr.h"
#include "xf86xv.h"
#include <X11/extensions/Xv.h>

#include "xf86Pci.h"

/* framebuffer offscreen manager */
#include "xf86fbman.h"

/* include xaa includes */
#include "xaarop.h"

/* H/W cursor support */
#include "xf86Cursor.h"

/* usleep() */
#include <unistd.h>

/* Driver specific headers */
#include "ast.h"
#include "ast_mode.h"
#include "ast_vgatool.h"

static VBIOS_STDTABLE_STRUCT StdTable[] = {
    /* MD_2_3_400 */
    {
        0x67,
        {0x00,0x03,0x00,0x02},
        {0x5f,0x4f,0x50,0x82,0x55,0x81,0xbf,0x1f,
         0x00,0x4f,0x0d,0x0e,0x00,0x00,0x00,0x00,
         0x9c,0x8e,0x8f,0x28,0x1f,0x96,0xb9,0xa3,
         0xff},
        {0x00,0x01,0x02,0x03,0x04,0x05,0x14,0x07,
         0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
         0x0c,0x00,0x0f,0x08},
        {0x00,0x00,0x00,0x00,0x00,0x10,0x0e,0x00,
         0xff}
    },
    /* Mode12/ExtEGATable */
    {
        0xe3,
        {0x01,0x0f,0x00,0x06},
        {0x5f,0x4f,0x50,0x82,0x55,0x81,0x0b,0x3e,
         0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,
         0xe9,0x8b,0xdf,0x28,0x00,0xe7,0x04,0xe3,
         0xff},
        {0x00,0x01,0x02,0x03,0x04,0x05,0x14,0x07,
         0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
         0x01,0x00,0x0f,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x0f,
         0xff}
    },
    /* ExtVGATable */
    {
        0x2f,
        {0x01,0x0f,0x00,0x0e},
        {0x5f,0x4f,0x50,0x82,0x54,0x80,0x0b,0x3e,
         0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,
         0xea,0x8c,0xdf,0x28,0x40,0xe7,0x04,0xa3,
         0xff},
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
         0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
         0x01,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0f,
         0xff}
    },
    /* ExtHiCTable */
    {
        0x2f,
        {0x01,0x0f,0x00,0x0e},
        {0x5f,0x4f,0x50,0x82,0x54,0x80,0x0b,0x3e,
         0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,
         0xea,0x8c,0xdf,0x28,0x40,0xe7,0x04,0xa3,
         0xff},
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
         0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
         0x01,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x0f,
         0xff}
    },
    /* ExtTrueCTable */
    {
        0x2f,
        {0x01,0x0f,0x00,0x0e},
        {0x5f,0x4f,0x50,0x82,0x54,0x80,0x0b,0x3e,
         0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,
         0xea,0x8c,0xdf,0x28,0x40,0xe7,0x04,0xa3,
         0xff},
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
         0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
         0x01,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x0f,
         0xff}
    },
};

static VBIOS_ENHTABLE_STRUCT  Res640x480Table[] = {
    { 800, 640, 8, 96, 525, 480, 2, 2, VCLK25_175,	/* 60Hz */
      (SyncNN | HBorder | VBorder | Charx8Dot), 60, 1, 0x2E },
    { 832, 640, 16, 40, 520, 480, 1, 3, VCLK31_5,	/* 72Hz */
      (SyncNN | HBorder | VBorder | Charx8Dot), 72, 2, 0x2E  },
    { 840, 640, 16, 64, 500, 480, 1, 3, VCLK31_5,	/* 75Hz */
      (SyncNN | Charx8Dot) , 75, 3, 0x2E },
    { 832, 640, 56, 56, 509, 480, 1, 3, VCLK36,		/* 85Hz */
      (SyncNN | Charx8Dot) , 85, 4, 0x2E },
    { 832, 640, 56, 56, 509, 480, 1, 3, VCLK36,		/* end */
      (SyncNN | Charx8Dot) , 0xFF, 4, 0x2E },
};


static VBIOS_ENHTABLE_STRUCT  Res800x600Table[] = {
    {1024, 800, 24, 72, 625, 600, 1, 2, VCLK36,		/* 56Hz */
      (SyncPP | Charx8Dot), 56, 1, 0x30 },
    {1056, 800, 40, 128, 628, 600, 1, 4, VCLK40,	/* 60Hz */
      (SyncPP | Charx8Dot), 60, 2, 0x30 },
    {1040, 800, 56, 120, 666, 600, 37, 6, VCLK50,	/* 72Hz */
      (SyncPP | Charx8Dot), 72, 3, 0x30 },
    {1056, 800, 16, 80, 625, 600, 1, 3, VCLK49_5,	/* 75Hz */
      (SyncPP | Charx8Dot), 75, 4, 0x30 },
    {1048, 800, 32, 64, 631, 600, 1, 3, VCLK56_25,	/* 85Hz */
      (SyncPP | Charx8Dot), 84, 5, 0x30 },
    {1048, 800, 32, 64, 631, 600, 1, 3, VCLK56_25,	/* end */
      (SyncPP | Charx8Dot), 0xFF, 5, 0x30 },
};

static VBIOS_ENHTABLE_STRUCT  Res1024x768Table[] = {
    {1344, 1024, 24, 136, 806, 768, 3, 6, VCLK65,	/* 60Hz */
      (SyncNN | Charx8Dot), 60, 1, 0x31 },
    {1328, 1024, 24, 136, 806, 768, 3, 6, VCLK75,	/* 70Hz */
      (SyncNN | Charx8Dot), 70, 2, 0x31 },
    {1312, 1024, 16, 96, 800, 768, 1, 3, VCLK78_75,	/* 75Hz */
      (SyncPP | Charx8Dot), 75, 3, 0x31 },
    {1376, 1024, 48, 96, 808, 768, 1, 3, VCLK94_5,	/* 85Hz */
      (SyncPP | Charx8Dot), 84, 4, 0x31 },
    {1376, 1024, 48, 96, 808, 768, 1, 3, VCLK94_5,	/* end */
      (SyncPP | Charx8Dot), 0xFF, 4, 0x31 },
};

static VBIOS_ENHTABLE_STRUCT  Res1280x1024Table[] = {
    {1688, 1280, 48, 112, 1066, 1024, 1, 3, VCLK108,	/* 60Hz */
      (SyncPP | Charx8Dot), 60, 1, 0x32 },
    {1688, 1280, 16, 144, 1066, 1024, 1, 3, VCLK135,	/* 75Hz */
      (SyncPP | Charx8Dot), 75, 2, 0x32 },
    {1728, 1280, 64, 160, 1072, 1024, 1, 3, VCLK157_5,	/* 85Hz */
      (SyncPP | Charx8Dot), 85, 3, 0x32 },
    {1728, 1280, 64, 160, 1072, 1024, 1, 3, VCLK157_5,	/* end */
      (SyncPP | Charx8Dot), 0xFF, 3, 0x32 },
};

static VBIOS_ENHTABLE_STRUCT  Res1600x1200Table[] = {
    {2160, 1600, 64, 192, 1250, 1200, 1, 3, VCLK162,	/* 60Hz */
      (SyncPP | Charx8Dot), 60, 1, 0x33 },
    {2160, 1600, 64, 192, 1250, 1200, 1, 3, VCLK162,	/* end */
      (SyncPP | Charx8Dot), 0xFF, 1, 0x33 },
};

/* 16:9 */
static VBIOS_ENHTABLE_STRUCT  Res1360x768Table[] = {
    {1792, 1360, 64,112, 795,  768, 3, 6, VCLK85_5,	/* 60Hz */
      (SyncPP | Charx8Dot | LineCompareOff | WideScreenMode | NewModeInfo), 60, 1, 0x39 },
    {1792, 1360, 64,112, 795,  768, 3, 6, VCLK85_5,	/* end */
      (SyncPP | Charx8Dot | LineCompareOff | WideScreenMode | NewModeInfo), 0xFF, 1, 0x39 },
};

static VBIOS_ENHTABLE_STRUCT  Res1600x900Table[] = {
    {1760, 1600, 48, 32, 926,  900, 3, 5, VCLK97_75,	/* 60Hz CVT RB */
      (SyncNP | Charx8Dot | LineCompareOff | WideScreenMode | NewModeInfo | AST2500PreCatchCRT), 60, 2, 0x3A },
    {2112, 1600, 88,168, 934,  900, 3, 5, VCLK118_25,	/* 60Hz CVT */
      (SyncPN | Charx8Dot | LineCompareOff | WideScreenMode | NewModeInfo), 60, 1, 0x3A },
    {2112, 1600, 88,168, 934,  900, 3, 5, VCLK118_25,	/* end */
      (SyncPN | Charx8Dot | LineCompareOff | WideScreenMode | NewModeInfo), 0xFF, 1, 0x3A },
};

static VBIOS_ENHTABLE_STRUCT  Res1920x1080Table[] = {
    {2200, 1920, 88, 44, 1125, 1080, 4, 5, VCLK148_5,	/* HDTV 60Hz */
      (SyncPP | Charx8Dot | LineCompareOff | WideScreenMode | NewModeInfo | AST2500PreCatchCRT), 60, 1, 0x38 },
    {2200, 1920, 88, 44, 1125, 1080, 4, 5, VCLK148_5,	/* end */
      (SyncPP | Charx8Dot | LineCompareOff | WideScreenMode | NewModeInfo | AST2500PreCatchCRT), 0xFF, 1, 0x38 },
};

/* 16:10 */
static VBIOS_ENHTABLE_STRUCT  Res1280x800Table[] = {
    {1440, 1280, 48, 32,  823,  800, 3, 6, VCLK71,		/* 60Hz CVT RB */
      (SyncNP | Charx8Dot | LineCompareOff | WideScreenMode | NewModeInfo | AST2500PreCatchCRT), 60, 2, 0x35 },
    {1680, 1280, 72,128,  831,  800, 3, 6, VCLK83_5,	/* 60Hz CVT */
      (SyncPN | Charx8Dot | LineCompareOff | WideScreenMode | NewModeInfo), 60, 1, 0x35 },
    {1680, 1280, 72,128,  831,  800, 3, 6, VCLK83_5,	/* end */
      (SyncPN | Charx8Dot | LineCompareOff | WideScreenMode | NewModeInfo), 0xFF, 1, 0x35 },
};

static VBIOS_ENHTABLE_STRUCT  Res1440x900Table[] = {
    {1600, 1440, 48, 32,  926,  900, 3, 6, VCLK88_75,	/* 60Hz CVT RB */
      (SyncNP | Charx8Dot | LineCompareOff | WideScreenMode | NewModeInfo | AST2500PreCatchCRT), 60, 2, 0x36 },
    {1904, 1440, 80,152,  934,  900, 3, 6, VCLK106_5,	/* 60Hz CVT */
      (SyncPN | Charx8Dot | LineCompareOff | WideScreenMode | NewModeInfo), 60, 1, 0x36 },
    {1904, 1440, 80,152,  934,  900, 3, 6, VCLK106_5,	/* end */
      (SyncPN | Charx8Dot | LineCompareOff | WideScreenMode | NewModeInfo), 0xFF, 1, 0x36 },
};

static VBIOS_ENHTABLE_STRUCT  Res1680x1050Table[] = {
    {1840, 1680, 48, 32, 1080, 1050, 3, 6, VCLK119,		/* 60Hz CVT RB */
      (SyncNP | Charx8Dot | LineCompareOff | WideScreenMode | NewModeInfo | AST2500PreCatchCRT), 60, 2, 0x37 },
    {2240, 1680,104,176, 1089, 1050, 3, 6, VCLK146_25,	/* 60Hz CVT */
      (SyncPN | Charx8Dot | LineCompareOff | WideScreenMode | NewModeInfo), 60, 1, 0x37 },
    {2240, 1680,104,176, 1089, 1050, 3, 6, VCLK146_25,	/* end */
      (SyncPN | Charx8Dot | LineCompareOff | WideScreenMode | NewModeInfo), 0xFF, 1, 0x37 },
};

static VBIOS_ENHTABLE_STRUCT  Res1920x1200Table[] = {
    {2080, 1920, 48, 32, 1235, 1200, 3, 6, VCLK154,		/* 60Hz CVT RB */
      (SyncNP | Charx8Dot | LineCompareOff | WideScreenMode | NewModeInfo | AST2500PreCatchCRT), 60, 1, 0x34 },
    {2080, 1920, 48, 32, 1235, 1200, 3, 6, VCLK154,		/* end */
      (SyncNP | Charx8Dot | LineCompareOff | WideScreenMode | NewModeInfo), 0xFF, 1, 0x34 },
};

static VBIOS_DCLK_INFO DCLKTable [] = {
    {0x2C, 0xE7, 0x03},					/* 00: VCLK25_175	*/
    {0x95, 0x62, 0x03},					/* 01: VCLK28_322	*/
    {0x67, 0x63, 0x01},					/* 02: VCLK31_5     */
    {0x76, 0x63, 0x01},					/* 03: VCLK36		*/
    {0xEE, 0x67, 0x01},					/* 04: VCLK40		*/
    {0x82, 0x62, 0x01},					/* 05: VCLK49_5		*/
    {0xC6, 0x64, 0x01},					/* 06: VCLK50		*/
    {0x94, 0x62, 0x01},					/* 07: VCLK56_25	*/
    {0x80, 0x64, 0x00},					/* 08: VCLK65		*/
    {0x7B, 0x63, 0x00},					/* 09: VCLK75		*/
    {0x67, 0x62, 0x00},					/* 0A: VCLK78_75	*/
    {0x7C, 0x62, 0x00},					/* 0B: VCLK94_5		*/
    {0x8E, 0x62, 0x00},					/* 0C: VCLK108		*/
    {0x85, 0x24, 0x00},					/* 0D: VCLK135		*/
    {0x67, 0x22, 0x00},					/* 0E: VCLK157_5	*/
    {0x6A, 0x22, 0x00},					/* 0F: VCLK162		*/
    {0x4d, 0x4c, 0x80},				    /* 10: VCLK154      */
    {0xa7, 0x78, 0x80},					/* 11: VCLK83.5     */
    {0x28, 0x49, 0x80},					/* 12: VCLK106.5    */
    {0x37, 0x49, 0x80},					/* 13: VCLK146.25   */
    {0x1f, 0x45, 0x80},					/* 14: VCLK148.5    */
    {0x47, 0x6c, 0x80},					/* 15: VCLK71       */
    {0x25, 0x65, 0x80},					/* 16: VCLK88.75    */
    {0x77, 0x58, 0x80},					/* 17: VCLK119      */
    {0x32, 0x67, 0x80},				    /* 18: VCLK85_5     */
    {0x6a, 0x6d, 0x80},					/* 19: VCLK97_75	*/
    {0x3b, 0x2c, 0x81},					/* 1A: VCLK118_25	*/
};

static VBIOS_DCLK_INFO DCLKTable_AST2100 [] = {
    {0x2C, 0xE7, 0x03},					/* 00: VCLK25_175	*/
    {0x95, 0x62, 0x03},					/* 01: VCLK28_322	*/
    {0x67, 0x63, 0x01},					/* 02: VCLK31_5     */
    {0x76, 0x63, 0x01},					/* 03: VCLK36		*/
    {0xEE, 0x67, 0x01},					/* 04: VCLK40		*/
    {0x82, 0x62, 0x01},					/* 05: VCLK49_5		*/
    {0xC6, 0x64, 0x01},					/* 06: VCLK50		*/
    {0x94, 0x62, 0x01},					/* 07: VCLK56_25	*/
    {0x80, 0x64, 0x00},					/* 08: VCLK65		*/
    {0x7B, 0x63, 0x00},					/* 09: VCLK75		*/
    {0x67, 0x62, 0x00},					/* 0A: VCLK78_75	*/
    {0x7C, 0x62, 0x00},					/* 0B: VCLK94_5		*/
    {0x8E, 0x62, 0x00},					/* 0C: VCLK108		*/
    {0x85, 0x24, 0x00},					/* 0D: VCLK135		*/
    {0x67, 0x22, 0x00},					/* 0E: VCLK157_5	*/
    {0x6A, 0x22, 0x00},					/* 0F: VCLK162		*/
    {0x4d, 0x4c, 0x80},				    /* 10: VCLK154      */
    {0x68, 0x6f, 0x80},					/* 11: VCLK83.5     */
    {0x28, 0x49, 0x80},					/* 12: VCLK106.5    */
    {0x37, 0x49, 0x80},					/* 13: VCLK146.25   */
    {0x1f, 0x45, 0x80},					/* 14: VCLK148.5    */
    {0x47, 0x6c, 0x80},					/* 15: VCLK71       */
    {0x25, 0x65, 0x80},					/* 16: VCLK88.75    */
    {0x77, 0x58, 0x80},					/* 17: VCLK119      */
    {0x32, 0x67, 0x80},				    /* 18: VCLK85_5     */
    {0x6a, 0x6d, 0x80},					/* 19: VCLK97_75	*/
    {0x3b, 0x2c, 0x81},					/* 1A: VCLK118_25	*/
};

static VBIOS_DCLK_INFO DCLKTable_AST2500 [] = {
    {0x40, 0x38, 0x73},					/* 00: VCLK25_175	*/
    {0x3A, 0x38, 0x43},					/* 01: VCLK28_322	*/
    {0x3E, 0x70, 0x73},					/* 02: VCLK31_5         */
    {0x35, 0x70, 0x43},					/* 03: VCLK36		*/
    {0x31, 0x28, 0x73},					/* 04: VCLK40		*/
    {0x41, 0x68, 0x73},					/* 05: VCLK49_5		*/
    {0x31, 0x68, 0x53},					/* 06: VCLK50		*/
    {0x4A, 0x68, 0x73},					/* 07: VCLK56_25	*/
    {0x40, 0x68, 0x53},					/* 08: VCLK65		*/
    {0x31, 0x60, 0x73},					/* 09: VCLK75		*/
    {0x3A, 0x28, 0x43},					/* 0A: VCLK78_75	*/
    {0x3E, 0x60, 0x73},					/* 0B: VCLK94_5		*/
    {0x35, 0x60, 0x63},					/* 0C: VCLK108		*/
    {0x3D, 0x40, 0x63},					/* 0D: VCLK135		*/
    {0x4E, 0x60, 0x63},					/* 0E: VCLK157_5	*/
    {0x35, 0x60, 0x53},					/* 0F: VCLK162		*/
    {0x4C, 0x60, 0x63},				    /* 10: VCLK154      	*/
    {0x4F, 0x48, 0x53},					/* 11: VCLK83.5         */
    {0x46, 0x60, 0x73},					/* 12: VCLK106.5        */
    {0x3C, 0x20, 0x63},					/* 13: VCLK146.25       */
    {0x3D, 0x20, 0x63},					/* 14: VCLK148.5        */
    {0x46, 0x68, 0x53},					/* 15: VCLK71           */
    {0x49, 0x68, 0x43},					/* 16: VCLK88.75        */
    {0x4e, 0x60, 0x73},					/* 17: VCLK119          */
    {0x38, 0x60, 0x73},				    /* 18: VCLK85_5         */
    {0x38, 0x20, 0x73},					/* 19: VCLK97_75 */
    {0x4e, 0x60, 0x73},					/* 1A: VCLK118_25 */
};

static VBIOS_DCLK_INFO DCLKTable_AST2500A1 [] = {
    {0x2C, 0xE7, 0x03},					/* 00: VCLK25_175	*/
    {0x95, 0x62, 0x03},					/* 01: VCLK28_322	*/
    {0x67, 0x63, 0x01},					/* 02: VCLK31_5         */
    {0x76, 0x63, 0x01},					/* 03: VCLK36		*/
    {0xEE, 0x67, 0x01},					/* 04: VCLK40		*/
    {0x82, 0x62, 0x01},					/* 05: VCLK49_5		*/
    {0xC6, 0x64, 0x01},					/* 06: VCLK50		*/
    {0x94, 0x62, 0x01},					/* 07: VCLK56_25	*/
    {0x80, 0x64, 0x00},					/* 08: VCLK65		*/
    {0x7B, 0x63, 0x00},					/* 09: VCLK75		*/
    {0x67, 0x62, 0x00},					/* 0A: VCLK78_75	*/
    {0x7C, 0x62, 0x00},					/* 0B: VCLK94_5		*/
    {0x8E, 0x62, 0x00},					/* 0C: VCLK108		*/
    {0x85, 0x24, 0x00},					/* 0D: VCLK135		*/
    {0x67, 0x22, 0x00},					/* 0E: VCLK157_5	*/
    {0x6A, 0x22, 0x00},					/* 0F: VCLK162		*/
    {0x4d, 0x4c, 0x80},				        /* 10: VCLK154      	*/
    {0x68, 0x6f, 0x80},					/* 11: VCLK83.5         */
    {0x28, 0x49, 0x80},					/* 12: VCLK106.5        */
    {0x37, 0x49, 0x80},					/* 13: VCLK146.25       */
    {0x1f, 0x45, 0x80},					/* 14: VCLK148.5        */
    {0x47, 0x6c, 0x80},					/* 15: VCLK71           */
    {0x25, 0x65, 0x80},					/* 16: VCLK88.75        */
    {0x58, 0x01, 0x42},					/* 17: VCLK119          */
    {0x32, 0x67, 0x80},				        /* 18: VCLK85_5         */
    {0x6a, 0x6d, 0x80},					/* 19: VCLK97_75 */
    {0x44, 0x20, 0x43},					/* 1A: VCLK118_25 */
};

#if 0
static VBIOS_DAC_INFO DAC_TEXT[] = {
 { 0x00, 0x00, 0x00 },  { 0x00, 0x00, 0x2a },  { 0x00, 0x2a, 0x00 },  { 0x00, 0x2a, 0x2a },
 { 0x2a, 0x00, 0x00 },  { 0x2a, 0x00, 0x2a },  { 0x2a, 0x2a, 0x00 },  { 0x2a, 0x2a, 0x2a },
 { 0x00, 0x00, 0x15 },  { 0x00, 0x00, 0x3f },  { 0x00, 0x2a, 0x15 },  { 0x00, 0x2a, 0x3f },
 { 0x2a, 0x00, 0x15 },  { 0x2a, 0x00, 0x3f },  { 0x2a, 0x2a, 0x15 },  { 0x2a, 0x2a, 0x3f },
 { 0x00, 0x15, 0x00 },  { 0x00, 0x15, 0x2a },  { 0x00, 0x3f, 0x00 },  { 0x00, 0x3f, 0x2a },
 { 0x2a, 0x15, 0x00 },  { 0x2a, 0x15, 0x2a },  { 0x2a, 0x3f, 0x00 },  { 0x2a, 0x3f, 0x2a },
 { 0x00, 0x15, 0x15 },  { 0x00, 0x15, 0x3f },  { 0x00, 0x3f, 0x15 },  { 0x00, 0x3f, 0x3f },
 { 0x2a, 0x15, 0x15 },  { 0x2a, 0x15, 0x3f },  { 0x2a, 0x3f, 0x15 },  { 0x2a, 0x3f, 0x3f },
 { 0x15, 0x00, 0x00 },  { 0x15, 0x00, 0x2a },  { 0x15, 0x2a, 0x00 },  { 0x15, 0x2a, 0x2a },
 { 0x3f, 0x00, 0x00 },  { 0x3f, 0x00, 0x2a },  { 0x3f, 0x2a, 0x00 },  { 0x3f, 0x2a, 0x2a },
 { 0x15, 0x00, 0x15 },  { 0x15, 0x00, 0x3f },  { 0x15, 0x2a, 0x15 },  { 0x15, 0x2a, 0x3f },
 { 0x3f, 0x00, 0x15 },  { 0x3f, 0x00, 0x3f },  { 0x3f, 0x2a, 0x15 },  { 0x3f, 0x2a, 0x3f },
 { 0x15, 0x15, 0x00 },  { 0x15, 0x15, 0x2a },  { 0x15, 0x3f, 0x00 },  { 0x15, 0x3f, 0x2a },
 { 0x3f, 0x15, 0x00 },  { 0x3f, 0x15, 0x2a },  { 0x3f, 0x3f, 0x00 },  { 0x3f, 0x3f, 0x2a },
 { 0x15, 0x15, 0x15 },  { 0x15, 0x15, 0x3f },  { 0x15, 0x3f, 0x15 },  { 0x15, 0x3f, 0x3f },
 { 0x3f, 0x15, 0x15 },  { 0x3f, 0x15, 0x3f },  { 0x3f, 0x3f, 0x15 },  { 0x3f, 0x3f, 0x3f },
};

static VBIOS_DAC_INFO DAC_EGA[] = {
 { 0x00, 0x00, 0x00 },  { 0x00, 0x00, 0x2a },  { 0x00, 0x2a, 0x00 },  { 0x00, 0x2a, 0x2a },
 { 0x2a, 0x00, 0x00 },  { 0x2a, 0x00, 0x2a },  { 0x2a, 0x2a, 0x00 },  { 0x2a, 0x2a, 0x2a },
 { 0x00, 0x00, 0x15 },  { 0x00, 0x00, 0x3f },  { 0x00, 0x2a, 0x15 },  { 0x00, 0x2a, 0x3f },
 { 0x2a, 0x00, 0x15 },  { 0x2a, 0x00, 0x3f },  { 0x2a, 0x2a, 0x15 },  { 0x2a, 0x2a, 0x3f },
 { 0x00, 0x15, 0x00 },  { 0x00, 0x15, 0x2a },  { 0x00, 0x3f, 0x00 },  { 0x00, 0x3f, 0x2a },
 { 0x2a, 0x15, 0x00 },  { 0x2a, 0x15, 0x2a },  { 0x2a, 0x3f, 0x00 },  { 0x2a, 0x3f, 0x2a },
 { 0x00, 0x15, 0x15 },  { 0x00, 0x15, 0x3f },  { 0x00, 0x3f, 0x15 },  { 0x00, 0x3f, 0x3f },
 { 0x2a, 0x15, 0x15 },  { 0x2a, 0x15, 0x3f },  { 0x2a, 0x3f, 0x15 },  { 0x2a, 0x3f, 0x3f },
 { 0x15, 0x00, 0x00 },  { 0x15, 0x00, 0x2a },  { 0x15, 0x2a, 0x00 },  { 0x15, 0x2a, 0x2a },
 { 0x3f, 0x00, 0x00 },  { 0x3f, 0x00, 0x2a },  { 0x3f, 0x2a, 0x00 },  { 0x3f, 0x2a, 0x2a },
 { 0x15, 0x00, 0x15 },  { 0x15, 0x00, 0x3f },  { 0x15, 0x2a, 0x15 },  { 0x15, 0x2a, 0x3f },
 { 0x3f, 0x00, 0x15 },  { 0x3f, 0x00, 0x3f },  { 0x3f, 0x2a, 0x15 },  { 0x3f, 0x2a, 0x3f },
 { 0x15, 0x15, 0x00 },  { 0x15, 0x15, 0x2a },  { 0x15, 0x3f, 0x00 },  { 0x15, 0x3f, 0x2a },
 { 0x3f, 0x15, 0x00 },  { 0x3f, 0x15, 0x2a },  { 0x3f, 0x3f, 0x00 },  { 0x3f, 0x3f, 0x2a },
 { 0x15, 0x15, 0x15 },  { 0x15, 0x15, 0x3f },  { 0x15, 0x3f, 0x15 },  { 0x15, 0x3f, 0x3f },
 { 0x3f, 0x15, 0x15 },  { 0x3f, 0x15, 0x3f },  { 0x3f, 0x3f, 0x15 },  { 0x3f, 0x3f, 0x3f },
};
#endif

static VBIOS_DAC_INFO DAC_VGA[] = {
 { 0x00, 0x00, 0x00 },  { 0x00, 0x00, 0x2a },  { 0x00, 0x2a, 0x00 },  { 0x00, 0x2a, 0x2a },
 { 0x2a, 0x00, 0x00 },  { 0x2a, 0x00, 0x2a },  { 0x2a, 0x15, 0x00 },  { 0x2a, 0x2a, 0x2a },
 { 0x15, 0x15, 0x15 },  { 0x15, 0x15, 0x3f },  { 0x15, 0x3f, 0x15 },  { 0x15, 0x3f, 0x3f },
 { 0x3f, 0x15, 0x15 },  { 0x3f, 0x15, 0x3f },  { 0x3f, 0x3f, 0x15 },  { 0x3f, 0x3f, 0x3f },
 { 0x00, 0x00, 0x00 },  { 0x05, 0x05, 0x05 },  { 0x08, 0x08, 0x08 },  { 0x0b, 0x0b, 0x0b },
 { 0x0e, 0x0e, 0x0e },  { 0x11, 0x11, 0x11 },  { 0x14, 0x14, 0x14 },  { 0x18, 0x18, 0x18 },
 { 0x1c, 0x1c, 0x1c },  { 0x20, 0x20, 0x20 },  { 0x24, 0x24, 0x24 },  { 0x28, 0x28, 0x28 },
 { 0x2d, 0x2d, 0x2d },  { 0x32, 0x32, 0x32 },  { 0x38, 0x38, 0x38 },  { 0x3f, 0x3f, 0x3f },
 { 0x00, 0x00, 0x3f },  { 0x10, 0x00, 0x3f },  { 0x1f, 0x00, 0x3f },  { 0x2f, 0x00, 0x3f },
 { 0x3f, 0x00, 0x3f },  { 0x3f, 0x00, 0x2f },  { 0x3f, 0x00, 0x1f },  { 0x3f, 0x00, 0x10 },
 { 0x3f, 0x00, 0x00 },  { 0x3f, 0x10, 0x00 },  { 0x3f, 0x1f, 0x00 },  { 0x3f, 0x2f, 0x00 },
 { 0x3f, 0x3f, 0x00 },  { 0x2f, 0x3f, 0x00 },  { 0x1f, 0x3f, 0x00 },  { 0x10, 0x3f, 0x00 },
 { 0x00, 0x3f, 0x00 },  { 0x00, 0x3f, 0x10 },  { 0x00, 0x3f, 0x1f },  { 0x00, 0x3f, 0x2f },
 { 0x00, 0x3f, 0x3f },  { 0x00, 0x2f, 0x3f },  { 0x00, 0x1f, 0x3f },  { 0x00, 0x10, 0x3f },
 { 0x1f, 0x1f, 0x3f },  { 0x27, 0x1f, 0x3f },  { 0x2f, 0x1f, 0x3f },  { 0x37, 0x1f, 0x3f },
 { 0x3f, 0x1f, 0x3f },  { 0x3f, 0x1f, 0x37 },  { 0x3f, 0x1f, 0x2f },  { 0x3f, 0x1f, 0x27 },
 { 0x3f, 0x1f, 0x1f },  { 0x3f, 0x27, 0x1f },  { 0x3f, 0x2f, 0x1f },  { 0x3f, 0x37, 0x1f },
 { 0x3f, 0x3f, 0x1f },  { 0x37, 0x3f, 0x1f },  { 0x2f, 0x3f, 0x1f },  { 0x27, 0x3f, 0x1f },
 { 0x1f, 0x3f, 0x1f },  { 0x1f, 0x3f, 0x27 },  { 0x1f, 0x3f, 0x2f },  { 0x1f, 0x3f, 0x37 },
 { 0x1f, 0x3f, 0x3f },  { 0x1f, 0x37, 0x3f },  { 0x1f, 0x2f, 0x3f },  { 0x1f, 0x27, 0x3f },
 { 0x2d, 0x2d, 0x3f },  { 0x31, 0x2d, 0x3f },  { 0x36, 0x2d, 0x3f },  { 0x3a, 0x2d, 0x3f },
 { 0x3f, 0x2d, 0x3f },  { 0x3f, 0x2d, 0x3a },  { 0x3f, 0x2d, 0x36 },  { 0x3f, 0x2d, 0x31 },
 { 0x3f, 0x2d, 0x2d },  { 0x3f, 0x31, 0x2d },  { 0x3f, 0x36, 0x2d },  { 0x3f, 0x3a, 0x2d },
 { 0x3f, 0x3f, 0x2d },  { 0x3a, 0x3f, 0x2d },  { 0x36, 0x3f, 0x2d },  { 0x31, 0x3f, 0x2d },
 { 0x2d, 0x3f, 0x2d },  { 0x2d, 0x3f, 0x31 },  { 0x2d, 0x3f, 0x36 },  { 0x2d, 0x3f, 0x3a },
 { 0x2d, 0x3f, 0x3f },  { 0x2d, 0x3a, 0x3f },  { 0x2d, 0x36, 0x3f },  { 0x2d, 0x31, 0x3f },
 { 0x00, 0x00, 0x1c },  { 0x07, 0x00, 0x1c },  { 0x0e, 0x00, 0x1c },  { 0x15, 0x00, 0x1c },
 { 0x1c, 0x00, 0x1c },  { 0x1c, 0x00, 0x15 },  { 0x1c, 0x00, 0x0e },  { 0x1c, 0x00, 0x07 },
 { 0x1c, 0x00, 0x00 },  { 0x1c, 0x07, 0x00 },  { 0x1c, 0x0e, 0x00 },  { 0x1c, 0x15, 0x00 },
 { 0x1c, 0x1c, 0x00 },  { 0x15, 0x1c, 0x00 },  { 0x0e, 0x1c, 0x00 },  { 0x07, 0x1c, 0x00 },
 { 0x00, 0x1c, 0x00 },  { 0x00, 0x1c, 0x07 },  { 0x00, 0x1c, 0x0e },  { 0x00, 0x1c, 0x15 },
 { 0x00, 0x1c, 0x1c },  { 0x00, 0x15, 0x1c },  { 0x00, 0x0e, 0x1c },  { 0x00, 0x07, 0x1c },
 { 0x0e, 0x0e, 0x1c },  { 0x11, 0x0e, 0x1c },  { 0x15, 0x0e, 0x1c },  { 0x18, 0x0e, 0x1c },
 { 0x1c, 0x0e, 0x1c },  { 0x1c, 0x0e, 0x18 },  { 0x1c, 0x0e, 0x15 },  { 0x1c, 0x0e, 0x11 },
 { 0x1c, 0x0e, 0x0e },  { 0x1c, 0x11, 0x0e },  { 0x1c, 0x15, 0x0e },  { 0x1c, 0x18, 0x0e },
 { 0x1c, 0x1c, 0x0e },  { 0x18, 0x1c, 0x0e },  { 0x15, 0x1c, 0x0e },  { 0x11, 0x1c, 0x0e },
 { 0x0e, 0x1c, 0x0e },  { 0x0e, 0x1c, 0x11 },  { 0x0e, 0x1c, 0x15 },  { 0x0e, 0x1c, 0x18 },
 { 0x0e, 0x1c, 0x1c },  { 0x0e, 0x18, 0x1c },  { 0x0e, 0x15, 0x1c },  { 0x0e, 0x11, 0x1c },
 { 0x14, 0x14, 0x1c },  { 0x16, 0x14, 0x1c },  { 0x18, 0x14, 0x1c },  { 0x1a, 0x14, 0x1c },
 { 0x1c, 0x14, 0x1c },  { 0x1c, 0x14, 0x1a },  { 0x1c, 0x14, 0x18 },  { 0x1c, 0x14, 0x16 },
 { 0x1c, 0x14, 0x14 },  { 0x1c, 0x16, 0x14 },  { 0x1c, 0x18, 0x14 },  { 0x1c, 0x1a, 0x14 },
 { 0x1c, 0x1c, 0x14 },  { 0x1a, 0x1c, 0x14 },  { 0x18, 0x1c, 0x14 },  { 0x16, 0x1c, 0x14 },
 { 0x14, 0x1c, 0x14 },  { 0x14, 0x1c, 0x16 },  { 0x14, 0x1c, 0x18 },  { 0x14, 0x1c, 0x1a },
 { 0x14, 0x1c, 0x1c },  { 0x14, 0x1a, 0x1c },  { 0x14, 0x18, 0x1c },  { 0x14, 0x16, 0x1c },
 { 0x00, 0x00, 0x10 },  { 0x04, 0x00, 0x10 },  { 0x08, 0x00, 0x10 },  { 0x0c, 0x00, 0x10 },
 { 0x10, 0x00, 0x10 },  { 0x10, 0x00, 0x0c },  { 0x10, 0x00, 0x08 },  { 0x10, 0x00, 0x04 },
 { 0x10, 0x00, 0x00 },  { 0x10, 0x04, 0x00 },  { 0x10, 0x08, 0x00 },  { 0x10, 0x0c, 0x00 },
 { 0x10, 0x10, 0x00 },  { 0x0c, 0x10, 0x00 },  { 0x08, 0x10, 0x00 },  { 0x04, 0x10, 0x00 },
 { 0x00, 0x10, 0x00 },  { 0x00, 0x10, 0x04 },  { 0x00, 0x10, 0x08 },  { 0x00, 0x10, 0x0c },
 { 0x00, 0x10, 0x10 },  { 0x00, 0x0c, 0x10 },  { 0x00, 0x08, 0x10 },  { 0x00, 0x04, 0x10 },
 { 0x08, 0x08, 0x10 },  { 0x0a, 0x08, 0x10 },  { 0x0c, 0x08, 0x10 },  { 0x0e, 0x08, 0x10 },
 { 0x10, 0x08, 0x10 },  { 0x10, 0x08, 0x0e },  { 0x10, 0x08, 0x0c },  { 0x10, 0x08, 0x0a },
 { 0x10, 0x08, 0x08 },  { 0x10, 0x0a, 0x08 },  { 0x10, 0x0c, 0x08 },  { 0x10, 0x0e, 0x08 },
 { 0x10, 0x10, 0x08 },  { 0x0e, 0x10, 0x08 },  { 0x0c, 0x10, 0x08 },  { 0x0a, 0x10, 0x08 },
 { 0x08, 0x10, 0x08 },  { 0x08, 0x10, 0x0a },  { 0x08, 0x10, 0x0c },  { 0x08, 0x10, 0x0e },
 { 0x08, 0x10, 0x10 },  { 0x08, 0x0e, 0x10 },  { 0x08, 0x0c, 0x10 },  { 0x08, 0x0a, 0x10 },
 { 0x0b, 0x0b, 0x10 },  { 0x0c, 0x0b, 0x10 },  { 0x0d, 0x0b, 0x10 },  { 0x0f, 0x0b, 0x10 },
 { 0x10, 0x0b, 0x10 },  { 0x10, 0x0b, 0x0f },  { 0x10, 0x0b, 0x0d },  { 0x10, 0x0b, 0x0c },
 { 0x10, 0x0b, 0x0b },  { 0x10, 0x0c, 0x0b },  { 0x10, 0x0d, 0x0b },  { 0x10, 0x0f, 0x0b },
 { 0x10, 0x10, 0x0b },  { 0x0f, 0x10, 0x0b },  { 0x0d, 0x10, 0x0b },  { 0x0c, 0x10, 0x0b },
 { 0x0b, 0x10, 0x0b },  { 0x0b, 0x10, 0x0c },  { 0x0b, 0x10, 0x0d },  { 0x0b, 0x10, 0x0f },
 { 0x0b, 0x10, 0x10 },  { 0x0b, 0x0f, 0x10 },  { 0x0b, 0x0d, 0x10 },  { 0x0b, 0x0c, 0x10 },
 { 0x00, 0x00, 0x00 },  { 0x00, 0x00, 0x00 },  { 0x00, 0x00, 0x00 },  { 0x00, 0x00, 0x00 },
 { 0x00, 0x00, 0x00 },  { 0x00, 0x00, 0x00 },  { 0x00, 0x00, 0x00 },  { 0x00, 0x00, 0x00 },
};

/* Prototype type declaration*/
static Bool bGetAST1000VGAModeInfo(ScrnInfoPtr pScrn, DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo);
static void vSetStdReg(ScrnInfoPtr pScrn,  DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo);
static void vSetCRTCReg(ScrnInfoPtr pScrn, DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo);
static void vSetOffsetReg(ScrnInfoPtr pScrn, DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo);
static void vSetDCLKReg(ScrnInfoPtr pScrn, DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo);
static void vSetExtReg(ScrnInfoPtr pScrn, DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo);
static void vSetSyncReg(ScrnInfoPtr pScrn, DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo);
static Bool bSetDACReg(ScrnInfoPtr pScrn, DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo);
static BOOL bSetAST1180CRTCReg(ScrnInfoPtr pScrn,  DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo);
static BOOL bSetAST1180OffsetReg(ScrnInfoPtr pScrn,  DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo);
static BOOL bSetAST1180DCLKReg(ScrnInfoPtr pScrn,  DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo);
static BOOL bSetAST1180ExtReg(ScrnInfoPtr pScrn,  DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo);
static void vInitChrontelReg(ScrnInfoPtr pScrn, DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo);

Bool
ASTSetMode(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    ASTRecPtr pAST;
    VBIOS_MODE_INFO vgamodeinfo = { 0 };

    pAST = ASTPTR(pScrn);

    /* pre set mode */
    bGetAST1000VGAModeInfo(pScrn, mode, &vgamodeinfo);

    /* set mode */
    if (pAST->jChipType == AST1180)
    {
        bASTInitAST1180(pScrn);

        bSetAST1180CRTCReg(pScrn, mode, &vgamodeinfo);
        bSetAST1180OffsetReg(pScrn, mode, &vgamodeinfo);
        bSetAST1180DCLKReg(pScrn, mode, &vgamodeinfo);
        bSetAST1180ExtReg(pScrn, mode, &vgamodeinfo);

        vInitChrontelReg(pScrn, mode, &vgamodeinfo);
    }
    else
    {
        vASTOpenKey(pScrn);
        bASTRegInit(pScrn);

        vAST1000DisplayOff(pScrn);

        vSetStdReg(pScrn, mode, &vgamodeinfo);
        vSetCRTCReg(pScrn, mode, &vgamodeinfo);
        vSetOffsetReg(pScrn, mode, &vgamodeinfo);
        vSetDCLKReg(pScrn, mode, &vgamodeinfo);
        vSetExtReg(pScrn, mode, &vgamodeinfo);
        vSetSyncReg(pScrn, mode, &vgamodeinfo);
        bSetDACReg(pScrn, mode, &vgamodeinfo);

        /* clear video buffer to avoid display noise */
        memset(pAST->FBVirtualAddr, 0x00, pAST->VideoModeInfo.ScreenPitch*pAST->VideoModeInfo.ScreenHeight);

        vAST1000DisplayOn(pScrn);
    }

    /* post set mode */
#ifdef	Accel_2D
   if (!pAST->noAccel) {
       if (!bASTEnable2D(pScrn, pAST)) {
           xf86DrvMsg(pScrn->scrnIndex, X_ERROR,"Enable 2D failed\n");
           pAST->noAccel = TRUE;
       }
   }
#endif
#ifdef	HWC
   if (!pAST->noHWC) {
       if (!bASTInitHWC(pScrn, pAST)) {
           xf86DrvMsg(pScrn->scrnIndex, X_ERROR,"Init HWC failed\n");
           pAST->noHWC = TRUE;
       }
   }
#endif

    return (TRUE);
}


static Bool bGetAST1000VGAModeInfo(ScrnInfoPtr pScrn, DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo)
{
    ASTRecPtr pAST;
    ULONG ulModeID, ulColorIndex, ulRefreshRate, ulRefreshRateIndex = 0;
    ULONG ulHBorder, ulVBorder;
    Bool check_sync;
    PVBIOS_ENHTABLE_STRUCT loop, best = NULL;

    pAST = ASTPTR(pScrn);

    switch (pScrn->bitsPerPixel)
    {
    case 8:
         pVGAModeInfo->pStdTableEntry = (PVBIOS_STDTABLE_STRUCT) &StdTable[VGAModeIndex];
	 ulColorIndex = VGAModeIndex-1;
         break;
    case 16:
         pVGAModeInfo->pStdTableEntry = (PVBIOS_STDTABLE_STRUCT) &StdTable[HiCModeIndex];
	 ulColorIndex = HiCModeIndex;
         break;
    case 24:
    case 32:
         pVGAModeInfo->pStdTableEntry = (PVBIOS_STDTABLE_STRUCT) &StdTable[TrueCModeIndex];
	 ulColorIndex = TrueCModeIndex;
	 break;
    default:
         return (FALSE);
    }

    switch (mode->CrtcHDisplay)
    {
    case 640:
	 pVGAModeInfo->pEnhTableEntry = (PVBIOS_ENHTABLE_STRUCT) &Res640x480Table[ulRefreshRateIndex];
	 break;
    case 800:
	 pVGAModeInfo->pEnhTableEntry = (PVBIOS_ENHTABLE_STRUCT) &Res800x600Table[ulRefreshRateIndex];
	 break;
    case 1024:
	 pVGAModeInfo->pEnhTableEntry = (PVBIOS_ENHTABLE_STRUCT) &Res1024x768Table[ulRefreshRateIndex];
	 break;
    case 1280:
         if (mode->CrtcVDisplay == 800)
             pVGAModeInfo->pEnhTableEntry = (PVBIOS_ENHTABLE_STRUCT) &Res1280x800Table[ulRefreshRateIndex];
         else
             pVGAModeInfo->pEnhTableEntry = (PVBIOS_ENHTABLE_STRUCT) &Res1280x1024Table[ulRefreshRateIndex];
	 break;
    case 1360:
         pVGAModeInfo->pEnhTableEntry = (PVBIOS_ENHTABLE_STRUCT) &Res1360x768Table[ulRefreshRateIndex];
         break;
    case 1440:
         pVGAModeInfo->pEnhTableEntry = (PVBIOS_ENHTABLE_STRUCT) &Res1440x900Table[ulRefreshRateIndex];
         break;
    case 1600:
         if (mode->CrtcVDisplay == 900)
	     pVGAModeInfo->pEnhTableEntry = (PVBIOS_ENHTABLE_STRUCT) &Res1600x900Table[ulRefreshRateIndex];
         else
	     pVGAModeInfo->pEnhTableEntry = (PVBIOS_ENHTABLE_STRUCT) &Res1600x1200Table[ulRefreshRateIndex];
	 break;
    case 1680:
         pVGAModeInfo->pEnhTableEntry = (PVBIOS_ENHTABLE_STRUCT) &Res1680x1050Table[ulRefreshRateIndex];
         break;
    case 1920:
         if (mode->CrtcVDisplay == 1080)
             pVGAModeInfo->pEnhTableEntry = (PVBIOS_ENHTABLE_STRUCT) &Res1920x1080Table[ulRefreshRateIndex];
         else
             pVGAModeInfo->pEnhTableEntry = (PVBIOS_ENHTABLE_STRUCT) &Res1920x1200Table[ulRefreshRateIndex];
         break;
    default:
	 return (FALSE);
    }

    /* Get Proper Mode Index */
    ulRefreshRate = (mode->Clock * 1000) / (mode->HTotal * mode->VTotal) + 1;
    loop = pVGAModeInfo->pEnhTableEntry;
    check_sync = loop->Flags & WideScreenMode;
    do {
	while (loop->ulRefreshRate != 0xff) {
		if ((check_sync) &&
		    (((mode->Flags & V_NVSYNC)  &&
		      (loop->Flags & PVSync))  ||
		     ((mode->Flags & V_PVSYNC)  &&
		      (loop->Flags & NVSync))  ||
		     ((mode->Flags & V_NHSYNC)  &&
		      (loop->Flags & PHSync))  ||
		     ((mode->Flags & V_PHSYNC)  &&
		      (loop->Flags & NHSync)))) {
			loop++;
			continue;
		}
	if (loop->ulRefreshRate <= ulRefreshRate
		    && (!best || loop->ulRefreshRate > best->ulRefreshRate))
			best = loop;
			loop++;
	}
	if (best || !check_sync)
		break;
	check_sync = 0;
    } while (1);
    if (best)
	pVGAModeInfo->pEnhTableEntry = best;

    /* Update mode CRTC info */
    ulHBorder = (pVGAModeInfo->pEnhTableEntry->Flags & HBorder) ? 8:0;
    ulVBorder = (pVGAModeInfo->pEnhTableEntry->Flags & VBorder) ? 8:0;

    mode->CrtcHTotal      = (int) pVGAModeInfo->pEnhTableEntry->HT;
    mode->CrtcHBlankStart = (int) (pVGAModeInfo->pEnhTableEntry->HDE + ulHBorder);
    mode->CrtcHBlankEnd   = (int) (pVGAModeInfo->pEnhTableEntry->HT - ulHBorder);
    mode->CrtcHSyncStart  = (int) (pVGAModeInfo->pEnhTableEntry->HDE + ulHBorder
                                   + pVGAModeInfo->pEnhTableEntry->HFP);
    mode->CrtcHSyncEnd    = (int) (pVGAModeInfo->pEnhTableEntry->HDE + ulHBorder
                                   + pVGAModeInfo->pEnhTableEntry->HFP
                                   + pVGAModeInfo->pEnhTableEntry->HSYNC);

    mode->CrtcVTotal      = (int) pVGAModeInfo->pEnhTableEntry->VT;
    mode->CrtcVBlankStart = (int) (pVGAModeInfo->pEnhTableEntry->VDE + ulVBorder);
    mode->CrtcVBlankEnd   = (int) (pVGAModeInfo->pEnhTableEntry->VT - ulVBorder);
    mode->CrtcVSyncStart  = (int) (pVGAModeInfo->pEnhTableEntry->VDE + ulVBorder
                                   + pVGAModeInfo->pEnhTableEntry->VFP);
    mode->CrtcVSyncEnd    = (int) (pVGAModeInfo->pEnhTableEntry->VDE + ulVBorder
                                   + pVGAModeInfo->pEnhTableEntry->VFP
                                   + pVGAModeInfo->pEnhTableEntry->VSYNC);

    /* Write mode info to scratch */
    ulRefreshRateIndex = pVGAModeInfo->pEnhTableEntry->ulRefreshRateIndex;
    ulModeID = pVGAModeInfo->pEnhTableEntry->ulModeID;

    if (pAST->jChipType == AST1180)
    {
        /* TODO */
    }
    else
    {
        SetIndexReg(CRTC_PORT, 0x8C, (UCHAR) ((ulColorIndex & 0x0F) << 4));
        SetIndexReg(CRTC_PORT, 0x8D, (UCHAR) (ulRefreshRateIndex & 0xFF));
        SetIndexReg(CRTC_PORT, 0x8E, (UCHAR) (ulModeID & 0xFF));

        /* NewModeInfo */
        SetIndexReg(CRTC_PORT, 0x91, 0x00);	/* clear signature */
        if (pVGAModeInfo->pEnhTableEntry->Flags & NewModeInfo)
        {
            SetIndexReg(CRTC_PORT, 0x91, 0xA8);	/* signature */
            SetIndexReg(CRTC_PORT, 0x92, (UCHAR) (pScrn->bitsPerPixel) );
            SetIndexReg(CRTC_PORT, 0x93, (UCHAR) (mode->Clock / 1000) );
            SetIndexReg(CRTC_PORT, 0x94, (UCHAR) (mode->CrtcHDisplay) );
            SetIndexReg(CRTC_PORT, 0x95, (UCHAR) (mode->CrtcHDisplay >> 8) );	/* color depth */
            SetIndexReg(CRTC_PORT, 0x96, (UCHAR) (mode->CrtcVDisplay) );
            SetIndexReg(CRTC_PORT, 0x97, (UCHAR) (mode->CrtcVDisplay >> 8) );	/* color depth */
        }
    }

    return (TRUE);
}

static void vSetStdReg(ScrnInfoPtr pScrn, DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo)
{

    PVBIOS_STDTABLE_STRUCT pStdModePtr;
    ASTRecPtr pAST;
    ULONG i;
    UCHAR jReg;

    pStdModePtr = pVGAModeInfo->pStdTableEntry;
    pAST = ASTPTR(pScrn);

    /* Set Misc */
    jReg = pStdModePtr->MISC;
    SetReg(MISC_PORT_WRITE,jReg);

    /* Set Seq */
    SetIndexReg(SEQ_PORT,0x00, 0x03);
    for (i=0; i<4; i++)
    {
        jReg = pStdModePtr->SEQ[i];
    	if (!i) (jReg |= 0x20);			/* display off */
        SetIndexReg(SEQ_PORT,(UCHAR) (i+1), jReg);
    }

    /* Set CRTC */
    SetIndexRegMask(CRTC_PORT,0x11, 0x7F, 0x00);
    for (i=0; i<25; i++)
    {
        jReg = pStdModePtr->CRTC[i];
        SetIndexReg(CRTC_PORT,(UCHAR) i, jReg);
    }

    /* Set AR */
    jReg = GetReg(INPUT_STATUS1_READ);
    for (i=0; i<20; i++)
    {
        jReg = pStdModePtr->AR[i];
        SetReg(AR_PORT_WRITE, (UCHAR) i);
        SetReg(AR_PORT_WRITE, jReg);
    }
    SetReg(AR_PORT_WRITE, 0x14);
    SetReg(AR_PORT_WRITE, 0x00);

    jReg = GetReg(INPUT_STATUS1_READ);
    SetReg (AR_PORT_WRITE, 0x20);		/* set POS */

    /* Set GR */
    for (i=0; i<9; i++)
    {
        jReg = pStdModePtr->GR[i];
        SetIndexReg(GR_PORT,(UCHAR) i, jReg);

    }


}

static void
vSetCRTCReg(ScrnInfoPtr pScrn, DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo)
{
    ASTRecPtr pAST;
    USHORT usTemp, ulPreCache = 0;
    UCHAR jReg05, jReg07, jReg09, jRegAC, jRegAD, jRegAE;

    pAST = ASTPTR(pScrn);
    jReg05 = jReg07 = jReg09 = jRegAC = jRegAD = jRegAE = 0;

    /* init value */
    if ((pAST->jChipType == AST2500) && (pVGAModeInfo->pEnhTableEntry->Flags & AST2500PreCatchCRT))
    	ulPreCache = 40;

    /* unlock CRTC */
    SetIndexRegMask(CRTC_PORT,0x11, 0x7F, 0x00);

    /* Horizontal Timing Programming */
    usTemp = (mode->CrtcHTotal >> 3) - 5;
    if (usTemp & 0x100) jRegAC |= 0x01;			/* HT D[8] */
    SetIndexRegMask(CRTC_PORT,0x00, 0x00, (UCHAR) usTemp);
    usTemp = (mode->CrtcHDisplay >> 3) - 1;
    if (usTemp & 0x100) jRegAC |= 0x04;			/* HDE D[8] */
    SetIndexRegMask(CRTC_PORT,0x01, 0x00, (UCHAR) usTemp);
    usTemp = (mode->CrtcHBlankStart >> 3) - 1;
    if (usTemp & 0x100) jRegAC |= 0x10;			/* HBS D[8] */
    SetIndexRegMask(CRTC_PORT,0x02, 0x00, (UCHAR) usTemp);
    usTemp = ((mode->CrtcHBlankEnd >> 3) - 1) & 0x7F;
    if (usTemp & 0x20) jReg05 |= 0x80;			/* HBE D[5] */
    if (usTemp & 0x40) jRegAD |= 0x01;			/* HBE D[6] */
    SetIndexRegMask(CRTC_PORT,0x03, 0xE0, (UCHAR) (usTemp & 0x1F));
    usTemp = ((mode->CrtcHSyncStart - ulPreCache) >> 3 ) - 1;
    if (usTemp & 0x100) jRegAC |= 0x40;			/* HRS D[5] */
    SetIndexRegMask(CRTC_PORT,0x04, 0x00, (UCHAR) (usTemp));
    usTemp = (((mode->CrtcHSyncEnd - ulPreCache) >> 3 ) - 1) & 0x3F;
    if (usTemp & 0x20) jRegAD |= 0x04;			/* HRE D[5] */
    SetIndexRegMask(CRTC_PORT,0x05, 0x60, (UCHAR) ((usTemp & 0x1F) | jReg05));

    SetIndexRegMask(CRTC_PORT,0xAC, 0x00, (UCHAR) jRegAC);
    SetIndexRegMask(CRTC_PORT,0xAD, 0x00, (UCHAR) jRegAD);

    /* Vetical Timing Programming */
    usTemp = (mode->CrtcVTotal) - 2;
    if (usTemp & 0x100) jReg07 |= 0x01;			/* VT D[8] */
    if (usTemp & 0x200) jReg07 |= 0x20;
    if (usTemp & 0x400) jRegAE |= 0x01;			/* VT D[10] */
    SetIndexRegMask(CRTC_PORT,0x06, 0x00, (UCHAR) usTemp);
    usTemp = (mode->CrtcVSyncStart) - 1;
    if (usTemp & 0x100) jReg07 |= 0x04;			/* VRS D[8] */
    if (usTemp & 0x200) jReg07 |= 0x80;			/* VRS D[9] */
    if (usTemp & 0x400) jRegAE |= 0x08;			/* VRS D[10] */
    SetIndexRegMask(CRTC_PORT,0x10, 0x00, (UCHAR) usTemp);
    usTemp = ((mode->CrtcVSyncEnd) - 1) & 0x3F;
    if (usTemp & 0x10) jRegAE |= 0x20;			/* VRE D[4] */
    if (usTemp & 0x20) jRegAE |= 0x40;			/* VRE D[5] */
    SetIndexRegMask(CRTC_PORT,0x11, 0x70, (UCHAR) (usTemp & 0x0F));
    usTemp = (mode->CrtcVDisplay) - 1;
    if (usTemp & 0x100) jReg07 |= 0x02;			/* VDE D[8] */
    if (usTemp & 0x200) jReg07 |= 0x40;			/* VDE D[9] */
    if (usTemp & 0x400) jRegAE |= 0x02;			/* VDE D[10] */
    SetIndexRegMask(CRTC_PORT,0x12, 0x00, (UCHAR) usTemp);
    usTemp = (mode->CrtcVBlankStart) - 1;
    if (usTemp & 0x100) jReg07 |= 0x08;			/* VBS D[8] */
    if (usTemp & 0x200) jReg09 |= 0x20;			/* VBS D[9] */
    if (usTemp & 0x400) jRegAE |= 0x04;			/* VBS D[10] */
    SetIndexRegMask(CRTC_PORT,0x15, 0x00, (UCHAR) usTemp);
    usTemp = (mode->CrtcVBlankEnd) - 1 ;
    if (usTemp & 0x100) jRegAE |= 0x10;			/* VBE D[8] */
    SetIndexRegMask(CRTC_PORT,0x16, 0x00, (UCHAR) usTemp);

    SetIndexRegMask(CRTC_PORT,0x07, 0x00, (UCHAR) jReg07);
    SetIndexRegMask(CRTC_PORT,0x09, 0xDF, (UCHAR) jReg09);
    SetIndexRegMask(CRTC_PORT,0xAE, 0x00, (UCHAR) (jRegAE | 0x80));	/* disable line compare */

    if ((pAST->jChipType == AST2500) && (pVGAModeInfo->pEnhTableEntry->Flags & AST2500PreCatchCRT))
    {
    	SetIndexRegMask(CRTC_PORT,0xB6, 0x3F, 0x80);
	}
	else
	{
    	SetIndexRegMask(CRTC_PORT,0xB6, 0x3F, 0x00);
	}

    /* lock CRTC */
    SetIndexRegMask(CRTC_PORT,0x11, 0x7F, 0x80);

}

static void vSetOffsetReg(ScrnInfoPtr pScrn, DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo)
{
    ASTRecPtr pAST;
    USHORT usOffset;

    pAST = ASTPTR(pScrn);

    usOffset = 	pAST->VideoModeInfo.ScreenPitch >> 3;		/* Unit: char */

    SetIndexReg(CRTC_PORT,0x13, (UCHAR) (usOffset & 0xFF));
    SetIndexReg(CRTC_PORT,0xB0, (UCHAR) ((usOffset >> 8) & 0x3F));

}

static void vSetDCLKReg(ScrnInfoPtr pScrn, DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo)
{
    PVBIOS_ENHTABLE_STRUCT pEnhModePtr;
    PVBIOS_DCLK_INFO pDCLKPtr;
    ASTRecPtr pAST;

    pAST = ASTPTR(pScrn);

    pEnhModePtr = pVGAModeInfo->pEnhTableEntry;
    if ((pAST->jChipType == AST2500) && (PCI_DEV_REVISION(pAST->PciInfo) > 0x40))
        pDCLKPtr = &DCLKTable_AST2500A1[pEnhModePtr->DCLKIndex];
    else if ((pAST->jChipType == AST2500) && (PCI_DEV_REVISION(pAST->PciInfo) == 0x40))
        pDCLKPtr = &DCLKTable_AST2500[pEnhModePtr->DCLKIndex];
    else if ((pAST->jChipType == AST2100) || (pAST->jChipType == AST1100) || (pAST->jChipType == AST2200) || (pAST->jChipType == AST2150) || (pAST->jChipType == AST2300) || (pAST->jChipType == AST2400))
        pDCLKPtr = &DCLKTable_AST2100[pEnhModePtr->DCLKIndex];
    else
        pDCLKPtr = &DCLKTable[pEnhModePtr->DCLKIndex];

    SetIndexRegMask(CRTC_PORT,0xC0, 0x00,  pDCLKPtr->Param1);
    SetIndexRegMask(CRTC_PORT,0xC1, 0x00,  pDCLKPtr->Param2);
    if ((pAST->jChipType == AST2500) && (PCI_DEV_REVISION(pAST->PciInfo) == 0x40))
    {
        SetIndexRegMask(CRTC_PORT,0xBB, 0x0F, (pDCLKPtr->Param3 & 0xF0));
    }
    else
    {
        SetIndexRegMask(CRTC_PORT,0xBB, 0x0F, (pDCLKPtr->Param3 & 0xC0) | ((pDCLKPtr->Param3 & 0x03) << 4) );
    }

}


static void vSetExtReg(ScrnInfoPtr pScrn, DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo)
{

    ASTRecPtr pAST;
    UCHAR jRegA0, jRegA3, jRegA8;

    pAST = ASTPTR(pScrn);

    jRegA0=jRegA3=jRegA8=0;
    /* Mode Type Setting */
    switch (pScrn->bitsPerPixel) {
    case 8:
        jRegA0 = 0x70;
        jRegA3 = 0x01;
        jRegA8 = 0x00;
        break;
    case 15:
    case 16:
        jRegA0 = 0x70;
        jRegA3 = 0x04;
        jRegA8 = 0x02;
        break;
    case 32:
        jRegA0 = 0x70;
        jRegA3 = 0x08;
        jRegA8 = 0x02;
        break;
    }
    SetIndexRegMask(CRTC_PORT,0xA0, 0x8F, (UCHAR) jRegA0);
    SetIndexRegMask(CRTC_PORT,0xA3, 0xF0, (UCHAR) jRegA3);
    SetIndexRegMask(CRTC_PORT,0xA8, 0xFD, (UCHAR) jRegA8);

#if	defined(__sparc__)
    UCHAR jRegA2 = 0x80;
    if ((pScrn->bitsPerPixel == 15) || (pScrn->bitsPerPixel == 16) )
        jRegA2 |= 0x40;
    SetIndexRegMask(CRTC_PORT,0xA2, 0x3F, (UCHAR) jRegA2);
#endif

    /* Set Threshold */
    if ((pAST->jChipType == AST2300) || (pAST->jChipType == AST2400) || (pAST->jChipType == AST2500))
    {
        SetIndexReg(CRTC_PORT,0xA7, 0x78);
        SetIndexReg(CRTC_PORT,0xA6, 0x60);
    }
    else if ((pAST->jChipType == AST2100) || (pAST->jChipType == AST1100) || (pAST->jChipType == AST2200) || (pAST->jChipType == AST2150) )
    {
        SetIndexReg(CRTC_PORT,0xA7, 0x3F);
        SetIndexReg(CRTC_PORT,0xA6, 0x2F);
    }
    else
    {
        SetIndexReg(CRTC_PORT,0xA7, 0x2F);
        SetIndexReg(CRTC_PORT,0xA6, 0x1F);
    }

}

static void vSetSyncReg(ScrnInfoPtr pScrn, DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo)
{
    PVBIOS_ENHTABLE_STRUCT pEnhModePtr;
    ASTRecPtr pAST;
    UCHAR jReg;

    pAST = ASTPTR(pScrn);
    pEnhModePtr = pVGAModeInfo->pEnhTableEntry;

    jReg  = GetReg(MISC_PORT_READ);
    jReg &= ~0xC0;
	if (pEnhModePtr->Flags & NVSync) jReg |= 0x80;
	if (pEnhModePtr->Flags & NHSync) jReg |= 0x40;
    SetReg(MISC_PORT_WRITE,jReg);
}

static Bool bSetDACReg(ScrnInfoPtr pScrn, DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo)
{
    PVBIOS_DAC_INFO pDACPtr;
    ASTRecPtr pAST;
    ULONG i, ulDACNumber;
    UCHAR DACR, DACG, DACB;

    pAST = ASTPTR(pScrn);

    switch (pScrn->bitsPerPixel)
    {
    case 8:
         ulDACNumber = DAC_NUM_VGA;
         pDACPtr = (PVBIOS_DAC_INFO) &DAC_VGA[0];
         break;
    default:
         return (FALSE);
    }

    for (i=0; i<ulDACNumber; i++)
    {
    	DACR = pDACPtr->DACR;
    	DACG = pDACPtr->DACG;
    	DACB = pDACPtr->DACB;

        VGA_LOAD_PALETTE_INDEX (i, DACR, DACG, DACB);

        pDACPtr++;
    }

    return (TRUE);

}

static ULONG AST1180DCLKTable [] = {
    0x0008676b,						/* 00: VCLK25_175	*/
    0x00086342,				        	/* 01: VCLK28_322	*/
    0x00086568,				        	/* 02: VCLK31_5         */
    0x00082118,				        	/* 03: VCLK36         	*/
    0x0008232e,				        	/* 04: VCLK40          	*/
    0x000c256d, 		        		/* 05: VCLK49_5        	*/
    0x00082016,                        	        	/* 06: VCLK50          	*/
    0x000c0010,                        	        	/* 07: VCLK56_25       	*/
    0x000c0332,                        	        	/* 08: VCLK65		*/
    0x00080010,                        	        	/* 09: VCLK75	        */
    0x000c033d,				        	/* 0A: VCLK78_75       	*/
    0x000c0568,                        	        	/* 0B: VCLK94_5        	*/
    0x00040118,                        	        	/* 0C: VCLK108         	*/
    0x00040334,                        	        	/* 0D: VCLK135         	*/
    0x0004033d,                        	        	/* 0E: VCLK157_5       	*/
    0x00040018,				        	/* 0F: VCLK162         	*/
    0x00040123,						/* 10: VCLK154          */
    0x000c0669,						/* 11: VCLK83_5         */
    0x0004074b,						/* 12: VCLK106_5        */
    0x0004022d,						/* 13: VCLK146_25       */
    0x00040769,						/* 14: VCLK148_5        */
};

static BOOL bSetAST1180CRTCReg(ScrnInfoPtr pScrn,  DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo)
{
    ASTRecPtr pAST = ASTPTR(pScrn);

    ULONG HTIndex, HRIndex, VTIndex, VRIndex;
    ULONG HT, HDE, HRS, HRE;
    ULONG VT, VDE, VRS, VRE;
    ULONG HT2, HDE2, HRS2, HRE2;
    ULONG VT2, VDE2, VRS2, VRE2;

    /* Reg. Index Select */
    {
        HTIndex =  AST1180_VGA1_HTREG;
        HRIndex =  AST1180_VGA1_HRREG;
        VTIndex =  AST1180_VGA1_VTREG;
        VRIndex =  AST1180_VGA1_VRREG;
    }

    /* Get CRTC Info */
    HT = mode->CrtcHTotal;
    HDE= mode->CrtcHDisplay;
    HRS= mode->CrtcHSyncStart;
    HRE= mode->CrtcHSyncEnd;
    VT = mode->CrtcVTotal;
    VDE= mode->CrtcVDisplay;
    VRS= mode->CrtcVSyncStart;
    VRE= mode->CrtcVSyncEnd;

    /* Calculate CRTC Reg Setting */
    HT2  = HT - 1;
    HDE2 = HDE - 1;
    HRS2 = HRS - 1;
    HRE2 = HRE - 1;
    VT2  = VT  - 1;
    VDE2 = VDE - 1;
    VRS2 = VRS - 1;
    VRE2 = VRE - 1;

    /* Write Reg */
    WriteAST1180SOC(AST1180_GFX_BASE + HTIndex, (ULONG)(HDE2 << 16) | (ULONG) (HT2));
    WriteAST1180SOC(AST1180_GFX_BASE + HRIndex, (ULONG)(HRE2 << 16) | (ULONG) (HRS2));
    WriteAST1180SOC(AST1180_GFX_BASE + VTIndex, (ULONG)(VDE2 << 16) | (ULONG) (VT2));
    WriteAST1180SOC(AST1180_GFX_BASE + VRIndex, (ULONG)(VRE2 << 16) | (ULONG) (VRS2));

    return (TRUE);

} /* bSetAST1180CRTCReg */

static BOOL bSetAST1180OffsetReg(ScrnInfoPtr pScrn,  DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo)
{
    ASTRecPtr pAST = ASTPTR(pScrn);
    ULONG ulOffset, ulTermalCount;

    ulOffset      = pAST->VideoModeInfo.ScreenPitch;
    ulTermalCount = (pAST->VideoModeInfo.ScreenPitch + 7) >> 3;

    /* Write Reg */
    WriteAST1180SOC(AST1180_GFX_BASE + AST1180_VGA1_OFFSET, (ULONG) (ulTermalCount << 16) | (ULONG) (ulOffset));

    return (TRUE);

} /* bSetAST1180OffsetReg */

static BOOL bSetAST1180DCLKReg(ScrnInfoPtr pScrn,  DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo)
{
    PVBIOS_ENHTABLE_STRUCT pEnhModePtr;
    ASTRecPtr pAST = ASTPTR(pScrn);
    ULONG ulDCLK;

    pEnhModePtr = pVGAModeInfo->pEnhTableEntry;
    ulDCLK = AST1180DCLKTable[pEnhModePtr->DCLKIndex];
    if (pEnhModePtr->Flags & HalfDCLK)
        ulDCLK |= 0x00400000;		/* D[22]: div by 2 */
    WriteAST1180SOC(AST1180_GFX_BASE + AST1180_VGA1_PLL, ulDCLK);

    return (TRUE);
}

static BOOL bSetAST1180ExtReg(ScrnInfoPtr pScrn, DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo)
{
    PVBIOS_ENHTABLE_STRUCT pEnhModePtr;
    ASTRecPtr pAST = ASTPTR(pScrn);

    ULONG ulCtlRegIndex, ulCtlReg;			/* enable display */
    ULONG ulCtlReg2Index, ulCtlReg2 = 0x80;		/* single edge */
    ULONG ulThresholdRegIndex ;				/* Threshold */
    ULONG ulStartAddressIndex;				/* ulStartAddress */
    ULONG ulStartAddress = pAST->ulVRAMBase;

    /* Reg. Index Select */
    {
        ulCtlRegIndex       = AST1180_VGA1_CTRL;
        ulCtlReg2Index      = AST1180_VGA1_CTRL2;
        ulThresholdRegIndex = AST1180_VGA1_THRESHOLD;
        ulStartAddressIndex = AST1180_VGA1_STARTADDR;
    }

    /* Mode Type Setting */
    ulCtlReg = 0x30000000;
    {
        switch (pScrn->bitsPerPixel) {
        case 15:
        case 16:
            ulCtlReg |= 0x100001;            	/* RGB565, SCREEN OFF, ENABLE */
            break;
        case 32:
            ulCtlReg |= 0x100101;            	/* XRGB8888, SCREEN OFF, ENABLE */
            break;
        }
    }

    /* Polarity */
    pEnhModePtr = pVGAModeInfo->pEnhTableEntry;
    ulCtlReg   |= (ULONG) (pEnhModePtr->Flags & SyncNN) << 10;

    /* Single/Dual Edge */
    ulCtlReg2 |= 0x40;				/* dual-edge */

    /* Write Reg */
    WriteAST1180SOC(AST1180_GFX_BASE + ulStartAddressIndex, ulStartAddress);
    WriteAST1180SOC(AST1180_GFX_BASE + ulThresholdRegIndex, ((ULONG) CRT_HIGH_THRESHOLD_VALUE << 8) | (ULONG) (CRT_LOW_THRESHOLD_VALUE));
    WriteAST1180SOC(AST1180_GFX_BASE + ulCtlReg2Index, ulCtlReg2);
    WriteAST1180SOC(AST1180_GFX_BASE + ulCtlRegIndex, ulCtlReg);

    return (TRUE);

} /* bSetAST1180ExtReg */

#define I2C_BASE_AST1180	0x80fcb000
#define I2C_DEVICEADDR_AST1180	0x0EC			/* slave addr */

static void SetChrontelReg(ASTRecPtr pAST, UCHAR jChannel, UCHAR jIndex, UCHAR jData )
{
    ULONG ulData, ulI2CAddr, ulI2CPortBase;
    ULONG retry;

    {
        ulI2CPortBase = I2C_BASE_AST1180 + 0x40 * jChannel;
        ulI2CAddr = I2C_DEVICEADDR_AST1180;
    }

    WriteAST1180SOC(ulI2CPortBase + 0x00, 0x00);
    WriteAST1180SOC(ulI2CPortBase + 0x04, 0x77743355);
    WriteAST1180SOC(ulI2CPortBase + 0x08, 0x0);
    WriteAST1180SOC(ulI2CPortBase + 0x10, 0xffffffff);
    WriteAST1180SOC(ulI2CPortBase + 0x00, 0x1);
    WriteAST1180SOC(ulI2CPortBase + 0x0C, 0xAF);
    WriteAST1180SOC(ulI2CPortBase + 0x20, ulI2CAddr);
    WriteAST1180SOC(ulI2CPortBase + 0x14, 0x03);
    retry = 0;
    do {
        ReadAST1180SOC(ulI2CPortBase + 0x10, ulData);
        usleep(10);
        if (retry++ > 1000)
            goto Exit_SetChrontelReg;
    } while (!(ulData & 0x01));

    WriteAST1180SOC(ulI2CPortBase + 0x10, 0xffffffff);
    WriteAST1180SOC(ulI2CPortBase + 0x20, (ULONG) jIndex);
    WriteAST1180SOC(ulI2CPortBase + 0x14, 0x02);
    do {
        ReadAST1180SOC(ulI2CPortBase + 0x10, ulData);
     } while (!(ulData & 0x01));

    WriteAST1180SOC(ulI2CPortBase + 0x10, 0xffffffff);
    WriteAST1180SOC(ulI2CPortBase + 0x20, (ULONG) jData);
    WriteAST1180SOC(ulI2CPortBase + 0x14, 0x02);
    do {
        ReadAST1180SOC(ulI2CPortBase + 0x10, ulData);
     } while (!(ulData & 0x01));

    WriteAST1180SOC(ulI2CPortBase + 0x10, 0xffffffff);
    WriteAST1180SOC(ulI2CPortBase + 0x0C, 0xBF);
    WriteAST1180SOC(ulI2CPortBase + 0x14, 0x20);
    do {
        ReadAST1180SOC(ulI2CPortBase + 0x10, ulData);
    } while (!(ulData & 0x10));

    ReadAST1180SOC(ulI2CPortBase + 0x0C, ulData);
    ulData &= 0xffffffef;
    WriteAST1180SOC(ulI2CPortBase + 0x0C, ulData);
    WriteAST1180SOC(ulI2CPortBase + 0x10, 0xffffffff);

Exit_SetChrontelReg:
    ;
}

static UCHAR GetChrontelReg(ASTRecPtr pAST, UCHAR jChannel, UCHAR jIndex)
{
    ULONG ulData, ulI2CAddr, ulI2CPortBase;
    UCHAR jData;
    ULONG retry;

    {
        ulI2CPortBase = I2C_BASE_AST1180 + 0x40 * jChannel;
        ulI2CAddr = I2C_DEVICEADDR_AST1180;
    }

    WriteAST1180SOC(ulI2CPortBase + 0x00, 0x00);
    WriteAST1180SOC(ulI2CPortBase + 0x04, 0x77743355);
    WriteAST1180SOC(ulI2CPortBase + 0x08, 0x0);
    WriteAST1180SOC(ulI2CPortBase + 0x10, 0xffffffff);
    WriteAST1180SOC(ulI2CPortBase + 0x00, 0x1);
    WriteAST1180SOC(ulI2CPortBase + 0x0C, 0xAF);
    WriteAST1180SOC(ulI2CPortBase + 0x20, ulI2CAddr);
    WriteAST1180SOC(ulI2CPortBase + 0x14, 0x03);
    retry = 0;
    do {
        ReadAST1180SOC(ulI2CPortBase + 0x10, ulData);
        usleep(10);
        if (retry++ > 1000)
            return 0;
    } while (!(ulData & 0x01));

    WriteAST1180SOC(ulI2CPortBase + 0x10, 0xffffffff);
    WriteAST1180SOC(ulI2CPortBase + 0x20, (ULONG) jIndex);
    WriteAST1180SOC(ulI2CPortBase + 0x14, 0x02);
    do {
        ReadAST1180SOC(ulI2CPortBase + 0x10, ulData);
     } while (!(ulData & 0x01));

    WriteAST1180SOC(ulI2CPortBase + 0x10, 0xffffffff);
    WriteAST1180SOC(ulI2CPortBase + 0x20, (ULONG) (ulI2CAddr + 1) );
    WriteAST1180SOC(ulI2CPortBase + 0x14, 0x1B);
    do {
        ReadAST1180SOC(ulI2CPortBase + 0x10, ulData);
     } while (!(ulData & 0x04));

    WriteAST1180SOC(ulI2CPortBase + 0x10, 0xffffffff);
    WriteAST1180SOC(ulI2CPortBase + 0x0C, 0xBF);
    WriteAST1180SOC(ulI2CPortBase + 0x14, 0x20);
    do {
        ReadAST1180SOC(ulI2CPortBase + 0x10, ulData);
    } while (!(ulData & 0x10));

    ReadAST1180SOC(ulI2CPortBase + 0x0C, ulData);
    ulData &= 0xffffffef;
    WriteAST1180SOC(ulI2CPortBase + 0x0C, ulData);
    WriteAST1180SOC(ulI2CPortBase + 0x10, 0xffffffff);

    ReadAST1180SOC(ulI2CPortBase + 0x20, ulData);
    jData = (UCHAR) ((ulData & 0xFF00) >> 8);

    return (jData);
}

static void vInitChrontelReg(ScrnInfoPtr pScrn, DisplayModePtr mode, PVBIOS_MODE_INFO pVGAModeInfo)
{
    ASTRecPtr pAST = ASTPTR(pScrn);
    ULONG ulDCLK = 65;					/* todo */
    UCHAR jReg;

    jReg = GetChrontelReg(pAST, 1, 0x4A);		/* get vendor id */
    if (jReg == 0x95)
    {
        jReg = GetChrontelReg(pAST, 1, 0x20);		/* DVI/D-Sub */
        if (jReg & 0x20)			        /* DVI */
        {

            /* DVI PLL Filter */
            if (ulDCLK > 65)
            {
                SetChrontelReg(pAST, 1, 0x33, 0x06);
                SetChrontelReg(pAST, 1, 0x34, 0x26);
                SetChrontelReg(pAST, 1, 0x36, 0xA0);
            }
            else
        	{
                SetChrontelReg(pAST, 1, 0x33, 0x08);
                SetChrontelReg(pAST, 1, 0x34, 0x16);
                SetChrontelReg(pAST, 1, 0x36, 0x60);
            }

            SetChrontelReg(pAST, 1, 0x49, 0xc0);
        }
        else						/* D-Sub */
        {

            SetChrontelReg(pAST, 1, 0x21, 0x09);
            SetChrontelReg(pAST, 1, 0x49, 0x00);
            SetChrontelReg(pAST, 1, 0x56, 0x00);
        }
    }

}

