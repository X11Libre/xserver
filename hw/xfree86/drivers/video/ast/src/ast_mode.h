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

/* Mode Limitation */
#define MAX_HResolution		1920
#define MAX_VResolution		1200

/* Std. Table Index Definition */
#define TextModeIndex 		0
#define EGAModeIndex 		1
#define VGAModeIndex 		2
#define HiCModeIndex 		3
#define TrueCModeIndex 		4

/* DCLK Index */
#define VCLK25_175     		0x00
#define VCLK28_322     		0x01
#define VCLK31_5       		0x02
#define VCLK36         		0x03
#define VCLK40         		0x04
#define VCLK49_5       		0x05
#define VCLK50         		0x06
#define VCLK56_25      		0x07
#define VCLK65		 	0x08
#define VCLK75	        	0x09
#define VCLK78_75      		0x0A
#define VCLK94_5       		0x0B
#define VCLK108        		0x0C
#define VCLK135        		0x0D
#define VCLK157_5      		0x0E
#define VCLK162        		0x0F
/* #define VCLK193_25     		0x10 */
#define VCLK154     		0x10
#define VCLK83_5    		0x11
#define VCLK106_5   		0x12
#define VCLK146_25  		0x13
#define VCLK148_5   		0x14
#define VCLK71      		0x15
#define VCLK88_75   		0x16
#define VCLK119     		0x17
#define VCLK85_5     		0x18
#define VCLK97_75     		0x19
#define VCLK118_25     		0x1A

/* Flags Definition */
#define Charx8Dot               0x00000001
#define HalfDCLK                0x00000002
#define DoubleScanMode          0x00000004
#define LineCompareOff          0x00000008
#define HBorder                 0x00000020
#define VBorder                 0x00000010
#define WideScreenMode		0x00000100
#define NewModeInfo		0x00000200
#define NHSync			0x00000400
#define PHSync			0x00000800
#define NVSync			0x00001000
#define PVSync			0x00002000
#define	SyncPP			(PVSync | PHSync)
#define	SyncPN			(PVSync | NHSync)
#define	SyncNP			(NVSync | PHSync)
#define	SyncNN			(NVSync | NHSync)
#define AST2500PreCatchCRT		0x00004000

/* DAC Definition */
#define DAC_NUM_TEXT		64
#define DAC_NUM_EGA		64
#define DAC_NUM_VGA		256

/* Data Structure declaration for internal use */
typedef struct {

    UCHAR MISC;
    UCHAR SEQ[4];
    UCHAR CRTC[25];
    UCHAR AR[20];
    UCHAR GR[9];

} VBIOS_STDTABLE_STRUCT, *PVBIOS_STDTABLE_STRUCT;

typedef struct {

    ULONG HT;
    ULONG HDE;
    ULONG HFP;
    ULONG HSYNC;
    ULONG VT;
    ULONG VDE;
    ULONG VFP;
    ULONG VSYNC;
    ULONG DCLKIndex;
    ULONG Flags;

    ULONG ulRefreshRate;
    ULONG ulRefreshRateIndex;
    ULONG ulModeID;

} VBIOS_ENHTABLE_STRUCT, *PVBIOS_ENHTABLE_STRUCT;

typedef struct {
    UCHAR Param1;
    UCHAR Param2;
    UCHAR Param3;
} VBIOS_DCLK_INFO, *PVBIOS_DCLK_INFO;

typedef struct {
    UCHAR DACR;
    UCHAR DACG;
    UCHAR DACB;
} VBIOS_DAC_INFO, *PVBIOS_DAC_INFO;

typedef struct {
    PVBIOS_STDTABLE_STRUCT pStdTableEntry;
    PVBIOS_ENHTABLE_STRUCT pEnhTableEntry;

} VBIOS_MODE_INFO, *PVBIOS_MODE_INFO;
