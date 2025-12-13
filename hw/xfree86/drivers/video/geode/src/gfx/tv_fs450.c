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

#define	FS450_DIRECTREG	0

#include "tv_fs450.h"

/*==========================================================================
 *	Macros
 *==========================================================================
 */
#undef		fsmax
#undef		fsmin
#define		fsmax(a, b)		((a) > (b) ? (a) : (b))
#define		fsmin(a, b)		((a) < (b) ? (a) : (b))

#undef range_limit
#define range_limit(val,min_val,max_val) (fsmax((min_val),fsmin((val),(max_val))))

/*==========================================================================
 *	Registers
 *==========================================================================
 */

#define MAX_REGISTERS	32
#define MAX_BITS		32

#define READ	1
#define WRITE	2
#define READ_WRITE (READ | WRITE)

typedef struct {
    char *name;
    unsigned long offset;
    unsigned char bit_length;
    unsigned char valid_bits;
    unsigned char read_write;
    char *bitfield_names[MAX_BITS];
} S_REGISTER_DESCRIP;

typedef struct {
    int source;
    char *name;
    S_REGISTER_DESCRIP registers[MAX_REGISTERS];
} S_SET_DESCRIP;

const S_SET_DESCRIP *houston_regs(void);
const S_SET_DESCRIP *encoder_regs(void);
const S_SET_DESCRIP *macrovision_regs(void);
const S_SET_DESCRIP *gcc_regs(void);

/*==========================================================================
 *	Houston Register Addresses & Bit Definitions
 *==========================================================================
 */
#define	HOUSTON_IHO				0x00    /* Input Horizontal Offset                      */
#define	HOUSTON_IVO				0x02    /* Input Vertical Offset                        */
#define	HOUSTON_IHA				0x04    /* Input Horizontal Active Width        */
#define	HOUSTON_VSC				0x06    /* Vertical Scaling Coefficient          */
#define	HOUSTON_HSC				0x08    /* Horizontal Scaling Coefficient        */
#define	HOUSTON_BYP				0x0A    /* Bypass Register                                      */
#define	HOUSTON_CR				0x0C    /* Control Register                             */
#define	HOUSTON_SP				0x0E    /* Status                                                       */
#define	HOUSTON_NCONL			0x10    /* NCO numerator low word                       */
#define	HOUSTON_NCONH			0x12    /* NCO numerator high word                      */
#define	HOUSTON_NCODL			0x14    /* NCO denominator low word             */
#define	HOUSTON_NCODH			0x16    /* NCO denominator high word            */
#define	HOUSTON_APO				0x18
#define	HOUSTON_ALO				0x1A
#define	HOUSTON_AFO				0x1C
#define	HOUSTON_HSOUTWID		0x1E
#define	HOUSTON_HSOUTST			0x20
#define	HOUSTON_HSOUTEND		0x22
#define	HOUSTON_SHP				0x24    /* Sharpness                                            */
#define	HOUSTON_FLK				0x26    /* Flicker Filter                                       */
#define	HOUSTON_BCONTL			0x28
#define	HOUSTON_BCONTH			0x2A
#define	HOUSTON_BDONE			0x2C
#define	HOUSTON_BDIAGL			0x2E
#define	HOUSTON_BDIAGH			0x30
#define	HOUSTON_REV				0x32
#define	HOUSTON_MISC			0x34
#define	HOUSTON_FFO				0x36
#define	HOUSTON_FFO_LAT			0x38
#define HOUSTON_VSOUTWID		0x3A
#define HOUSTON_VSOUTST			0x3C
#define HOUSTON_VSOUTEND		0x3E
/*		BYP Register Bits		*/
#define	BYP_RGB_BYPASS			0x0001
#define	BYP_HDS_BYPASS			0x0002
#define	BYP_HDS_TBYPASS			0x0004
#define	BYP_CAC_BYPASS			0x0008
#define	BYP_R2V_SBYPASS			0x0010
#define	BYP_R2V_BYPASS			0x0020
#define	BYP_VDS_BYPASS			0x0040
#define	BYP_FFT_BYPASS			0x0080
#define	BYP_FIF_BYPASS			0x0100
#define	BYP_FIF_TBYPASS			0x0200
#define	BYP_HUS_BYPASS			0x0400
#define	BYP_HUS_TBYPASS			0x0800
#define	BYP_CCR_BYPASS			0x1000
#define	BYP_PLL_BYPASS			0x2000
#define	BYP_NCO_BYPASS			0x4000
#define	BYP_ENC_BYPASS			0x8000
/*		CR Register Bits		*/
#define	CR_RESET				0x0001
#define	CR_CLKOFF				0x0002
#define	CR_NCO_EN				0x0004
#define	CR_COMPOFF				0x0008
#define	CR_YCOFF				0x0010
#define	CR_LP_EN				0x0020
#define	CR_CACQ_CLR				0x0040
#define	CR_FFO_CLR				0x0080
#define	CR_656_PAL_NTSC			0x0100
#define	CR_656_STD_VMI			0x0200
#define	CR_OFMT					0x0400
#define	CR_UIM_CLK				0x0800
#define	CR_UIM_DEC				0x1000
#define	CR_BIPGEN_EN1			0x2000
#define	CR_UIM_MOD0				0x4000
#define	CR_UIM_MOD1				0x8000
/*		Status Register Bits	*/
#define	SP_CACQ_ST				0x0001
#define	SP_FFO_ST				0x0002
#define	SP_REVID_MASK			0x7FFC
#define	SP_MV_EN				0x8000
/*		BDONE Register Bits		*/
#define	BDONE_BIST_DONE_A		0x0001
#define	BDONE_BIST_DONE_B		0x0002
#define	BDONE_BIST_DONE_C		0x0004
#define	BDONE_BIST_DONE_D		0x0008
#define	BDONE_BIST_DONE_E		0x0010
#define	BDONE_BIST_DONE_F		0x0020
#define	BDONE_BIST_DONE_G		0x0040
/*		BDIAGL Register Bits	*/
#define	BDIAGL_BIST_DIAG_A		0x000F
#define	BDIAGL_BIST_DIAG_B		0x00F0
#define	BDIAGL_BIST_DIAG_C		0x0F00
#define	BDIAGL_BIST_DIAG_D		0xF000
/*		BDIAGH Register Bits	*/
#define	BDIAGH_BIST_DIAG_E		0x000F
#define	BDIAGH_BIST_DIAG_F		0x000F
#define	BDIAGH_BIST_DIAG_G		0x000F
/*		MISC Register Bits		*/
#define	MISC_TV_SHORT_FLD		0x0001
#define	MISC_ENC_TEST			0x0002
#define	MISC_DAC_TEST			0x0004
#define	MISC_MV_SOFT_EN			0x0008
#define	MISC_NCO_LOAD0			0x0010
#define	MISC_NCO_LOAD1			0x0020
#define	MISC_VGACKDIV			0x0200
#define	MISC_BRIDGE_SYNC		0x0400
#define	MISC_GTLIO_PD			0x8000
/*==========================================================================
 *	Encoder Registers & Bit Definitions
 *==========================================================================
 */
#define	ENC_CHROMA_FREQ		0x40
#define	ENC_CHROMA_PHASE	0x44
#define	ENC_REG05			0x45
#define	ENC_REG06			0x46
#define	ENC_REG07			0x47
#define	ENC_HSYNC_WIDTH		0x48
#define	ENC_BURST_WIDTH		0x49
#define	ENC_BACK_PORCH		0x4A
#define	ENC_CB_BURST_LEVEL	0x4B
#define	ENC_CR_BURST_LEVEL	0x4C
#define	ENC_SLAVE_MODE		0x4D
#define	ENC_BLACK_LEVEL		0x4e
#define	ENC_BLANK_LEVEL		0x50
#define	ENC_NUM_LINES		0x57
#define	ENC_WHITE_LEVEL		0x5e
#define	ENC_CB_GAIN			0x60
#define	ENC_CR_GAIN			0x62
#define	ENC_TINT			0x65
#define	ENC_BREEZE_WAY		0x69
#define	ENC_FRONT_PORCH		0x6C
#define	ENC_ACTIVELINE		0x71
#define	ENC_FIRST_LINE		0x73
#define	ENC_REG34			0x74
#define	ENC_SYNC_LEVEL		0x75
#define	ENC_VBI_BLANK_LEVEL	0x7C
#define	ENC_RESET			0x7e
#define	ENC_NOTCH_FILTER	0x8d
/*==========================================================================
 * 	Macrovision Registers & Bit Definitions
 *==========================================================================
 */
#define		MV_N0			0x59
#define		MV_N1			0x52
#define		MV_N2			0x7b
#define		MV_N3			0x53
#define		MV_N4			0x79
#define		MV_N5			0x5d
#define		MV_N6			0x7a
#define		MV_N7			0x64
#define		MV_N8			0x54
#define		MV_N9			0x55
#define		MV_N10			0x56
#define		MV_N11			0x6d
#define		MV_N12			0x6f
#define		MV_N13			0x5a
#define		MV_N14			0x5b
#define		MV_N15			0x5c
#define		MV_N16			0x63
#define		MV_N17			0x66
#define		MV_N18			0x68
#define		MV_N19			0x67
#define		MV_N20			0x61
#define		MV_N21			0x6a
#define		MV_N22			0x76
#define		MV_AGC_PULSE_LEVEL	0x77
#define		MV_BP_PULSE_LEVEL	0x78
/*==========================================================================
 *	The TRACE macro can be used to display debug information.  It can display
 *	one or more parameters in a formatted string like printf.  No code will be
 *	generated for a release build.  Use double parentheses for compatibility
 *	with C #define statements.  Newline characters are not added
 *	automatically.  Usage example:
 *
 *	TRACE(("Number is %d, Name is %s.\n",iNumber,lpszName))
 *==========================================================================
 */
#define TRACE(parameters) {}
/*		GCC timing structure		*/
typedef struct _S_TIMING_SPECS {
    int vga_width;
    int vga_lines;
    int tv_width;
    int tv_lines;
    int h_total;
    int h_sync;
    int v_total;
    int v_sync;
} S_TIMING_SPECS;

/*		Revision of Houston chip	*/
#define HOUSTON_REV_A 0
#define HOUSTON_REV_B 1
static int houston_Rev(void);

/*==========================================================================
 *	Functions
 *==========================================================================
 */
static int houston_init(void);

static unsigned char PLAL_FS450_i2c_address(void);
static int PLAL_FS450_UIM_mode(void);
static int PLAL_ReadRegister(S_REG_INFO * p_reg);
static int PLAL_WriteRegister(const S_REG_INFO * p_reg);
static int PLAL_IsTVOn(void);
static int PLAL_EnableVga(void);
static int PLAL_PrepForTVout(void);
static int PLAL_SetTVTimingRegisters(const S_TIMING_SPECS * p_specs);
static int PLAL_FinalEnableTVout(unsigned long vga_mode);

/* Direct Memory Access Functions	*/
/* NOTE: Cx5530 is assumed hardcoded at 0x10000 offset from MediaGX base. 
 * F4Bar is bogus as described in the Cx5530 datasheet (actually points to GX
 * frame buffer).					*/
static int
DMAL_ReadUInt32(unsigned long phys_addr, unsigned long *p_data)
{
    *p_data = READ_REG32(phys_addr);
    return 0;
}

static int
DMAL_WriteUInt32(unsigned long phys_addr, unsigned long data)
{
    WRITE_REG32(phys_addr, data);
    return 0;
}

/* Houston register access functions.	*/
static int
houston_ReadReg(unsigned int reg, unsigned long *p_value, unsigned int bytes)
{
    return gfx_i2c_read(1, PLAL_FS450_i2c_address(), (unsigned char) reg,
                        (unsigned char) bytes, (unsigned char *) p_value);
}

static int
houston_WriteReg(unsigned int reg, unsigned long value, unsigned int bytes)
{
    return gfx_i2c_write(1, PLAL_FS450_i2c_address(), (unsigned char) reg,
                         (unsigned char) bytes, (unsigned char *) &value);
}

/* TV configuration functions.			*/
static int config_init(void);
static const S_TIMING_SPECS *p_specs(void);
static void config_power(int on);
static void config_vga_mode(unsigned long vga_mode);
static void config_tv_std(unsigned long tv_std, unsigned int trigger_bits);
static void conget_tv_std(unsigned long *p_tv_std);
static unsigned long supported_standards(void);
static void config_tvout_mode(unsigned long tvout_mode);
static void conget_tvout_mode(unsigned long *p_tvout_mode);
static void config_overscan_xy(unsigned long tv_std, unsigned long vga_mode,
                               int overscan_x, int overscan_y, int pos_x,
                               int pos_y);
static void config_nco(unsigned long tv_std, unsigned long vga_mode);
static void config_sharpness(int sharpness);
static void conget_sharpness(int *p_sharpness);
static void config_flicker(int flicker);
static void conget_flicker(int *p_flicker);
static void config_color(int color);
static void conget_color(int *p_color);
static void config_brightness_contrast(unsigned long tv_std,
                                       unsigned int trigger_bits,
                                       int brightness, int contrast);
static void conget_brightness_contrast(unsigned long tv_std,
                                       unsigned int trigger_bits,
                                       int *p_brightness, int *p_contrast);
static void config_yc_filter(unsigned long tv_std, int luma_filter,
                             int chroma_filter);
static void conget_yc_filter(int *p_luma_filter, int *p_chroma_filter);
static void config_macrovision(unsigned long tv_std,
                               unsigned int cp_trigger_bits);
static void conget_macrovision(unsigned long tv_std,
                               unsigned int *p_cp_trigger_bits);

/* Device settings.			*/
typedef struct _S_DEVICE_SETTINGS {
    int tv_on;
    unsigned long vga_mode;
    unsigned long tv_std;
    unsigned long tvout_mode;
    int overscan_x;
    int overscan_y;
    int position_x;
    int position_y;
    int sharpness;
    int flicker;
    int color;
    int brightness;
    int contrast;
    unsigned char yc_filter;
    unsigned int aps_trigger_bits;
    int last_overscan_y;
} S_DEVICE_SETTINGS;

static S_DEVICE_SETTINGS d;

/*==========================================================================
 * TV Setup Parameters
 *==========================================================================
 * */

static const struct {
    unsigned long chroma_freq[5];
    unsigned short chroma_phase[5];
    unsigned short cphase_rst[5];
    unsigned short color[5];
    unsigned short cr_burst_level[5];
    unsigned short cb_burst_level[5];
    unsigned short sys625_50[5];
    unsigned short vsync5[5];
    unsigned short pal_mode[5];
    unsigned short hsync_width[5];
    unsigned short burst_width[5];
    unsigned short back_porch[5];
    unsigned short front_porch[5];
    unsigned short breeze_way[5];
    unsigned short activeline[5];
    unsigned short blank_level[5];
    unsigned short vbi_blank_level[5];
    unsigned short black_level[5];
    unsigned short white_level[5];
    unsigned short hamp_offset[5];
    unsigned short sync_level[5];
    unsigned short tv_lines[5];
    unsigned short tv_width[5];
    unsigned short tv_active_lines[5];
    unsigned short tv_active_width[5];
    unsigned char notch_filter[5];
    unsigned short houston_cr[5];
    unsigned short houston_ncodl[5];
    unsigned short houston_ncodh[5];
} tvsetup = {
    /*     ntsc,    pal,                ntsc-eij,   pal-m,      pal-n */
    {
     0x1f7cf021, 0xcb8a092a, 0x1f7cf021, 0xe3efe621, 0xcb8a092a},
    /* chroma_freq                  */
    {
     0, 0, 0, 0, 0},
    /* chroma_phase                 */
    {
     2, 0, 2, 0, 0},
    /* cphase_rst                   */
    {
     54, 43, 54, 43, 43},
    /* color                                */
    {
     0, 31, 0, 29, 29},
    /* cr_burst_level               */
    {
     59, 44, 59, 41, 41},
    /* cb_burst_level               */
    {
     0, 1, 0, 0, 1},
    /* sys625_50                    */
    {
     0, 1, 0, 0, 0},
    /* vsync5                               */
    {
     0, 1, 0, 1, 1},
    /* pal_mode                     */
    {
     0x7a, 0x7a, 0x7a, 0x7a, 0x7a},
    /* hsync_width                  */
    {
     0x40, 0x3c, 0x40, 0x40, 0x3c},
    /* burst_width                  */
    {
     0x80, 0x9a, 0x80, 0x80, 0x9a},
    /* back_porch                   */
    {
     0x24, 0x1e, 0x24, 0x24, 0x1e},
    /* front_porch                  */
    {
     0x19, 0x1a, 0x19, 0x12, 0x1a},
    /* breeze_way                   */
    {
     0xb4, 0xb4, 0xb4, 0xb4, 0xb4},
    /* active_line                  */
    {
     240, 251, 240, 240, 240},
    /* blank_level                  */
    {
     240, 251, 240, 240, 240},
    /* vbi_blank_level              */
    {
     284, 252, 240, 252, 252},
    /* black_level                  */
    {
     823, 821, 823, 821, 821},
    /* white_level                  */
    {
     60, 48, 60, 48, 48},
    /* hamp_offset                  */
    {
     0x08, 0x08, 0x08, 0x08, 0x08},
    /* sync_level                   */
    {
     525, 625, 525, 525, 625},
    /* tv_lines                     */
    {
     858, 864, 858, 858, 864},
    /* tv_width                     */
    {
     487, 576, 487, 487, 576},
    /* tv_active_lines              */
    {
     800, 800, 800, 800, 800},
    /* tv_active_width              */
    {
     0x1a, 0x1d, 0x1a, 0x1d, 0x1d},
    /* notch filter enabled */
    {
     0x0000, 0x0100, 0x0000, 0x0000, 0x0100},
    /* houston cr pal               */
    {
     0x7e48, 0xf580, 0x7e48, 0x7e48, 0xf580},
    /* houston ncodl                */
    {
     0x001b, 0x0020, 0x001b, 0x001b, 0x0020}
    /* houston ncodh                */
};

/* MediaGX default underscan and centered position setups. */
#define	SCANTABLE_ENTRIES	5
struct _scantable {
    unsigned long mode;
    unsigned short v_total[5];
    unsigned short v_sync[5];
    unsigned short iha[5];
    signed short iho[5];
    signed short hsc[5];
};

static struct _scantable scantable[SCANTABLE_ENTRIES] = {
    {
     GFX_VGA_MODE_640X480,
     {617, 624, 617, 624, 624}, /* v_total      */
     {69, 88, 69, 88, 88},      /* v_sync       */
     {720, 720, 720, 720, 720}, /* iha          */
     {0, 0, 0, 0, 0},           /* iho          */
     {-12, 0, -6, 0, 0}         /* hsc          */
     },
    {
     GFX_VGA_MODE_800X600,
     {740, 740, 740, 740, 740}, /* v_total      */
     {90, 88, 90, 88, 88},      /* v_sync       */
     {720, 720, 508, 720, 720}, /* iha          */
     {-8, 11, -8, -8, 11},      /* iho          */
     {-27, -27, -27, -27, -27}  /* hsc          */
     },
    {
     GFX_VGA_MODE_720X487,
     {525, 720, 525, 720, 720}, /* v_total      */
     {23, 230, 23, 230, 230},   /* v_sync       */
     {720, 720, 720, 720, 720}, /* iha          */
     {0xa2, 0xa2, 0xa2, 0xa2, 0xa2},    /* iho          */
     {0, 0, 0, 0, 0}            /* hsc          */
     },
    {
     GFX_VGA_MODE_720X576,
     {720, 625, 720, 625, 625}, /* v_total      */
     {129, 25, 129, 25, 25},    /* v_sync       */
     {720, 720, 720, 720, 720}, /* iha          */
     {0xaa, 0xaa, 0xaa, 0xaa, 0xaa},    /* iho              */
     {0, 0, 0, 0, 0}            /* hsc          */
     },
    {
     GFX_VGA_MODE_1024X768,
     {933, 942, 933, 806, 806}, /* v_total      */
     {121, 112, 121, 88, 88},   /* v_sync       */
     {600, 600, 600, 600, 600}, /* iha          */
     {0x3c, 0x23, 0x3c, 0x65, 0x65},    /* iho              */
     {35, 26, 35, 26, 26}       /* hsc          */
     },
};

/* Houston fifo configuration constants. */
struct _ffolat {
    int v_total;
    unsigned short ffolat;
};

struct _ffolativo {
    int v_total;
    unsigned short ivo;
    unsigned short ffolat;
};

/* h_total=832, ivo=40, tv_width=858, tv_lines=525, vga_lines=480 */
#define SIZE6X4NTSC		66
static struct _ffolat ffo6x4ntsc[SIZE6X4NTSC + 1] = {
    {541, 0x40}, {545, 0x40}, {549, 0x40}, {553, 0x40},
    {557, 0x58}, {561, 0x40}, {565, 0x40}, {569, 0x40},
    {573, 0x48}, {577, 0x40}, {581, 0x40}, {585, 0x40},
    {589, 0x40}, {593, 0x48}, {597, 0x40}, {601, 0x40},
    {605, 0x40}, {609, 0x40}, {613, 0x5b}, {617, 0x48},
    {621, 0x60}, {625, 0x48}, {629, 0x48}, {633, 0x40},
    {637, 0x5e}, {641, 0x40}, {645, 0x50}, {649, 0x56},
    {653, 0x58}, {657, 0x6c}, {661, 0x40}, {665, 0x40},
    {669, 0x40}, {673, 0x40}, {677, 0x40}, {681, 0x40},
    {685, 0x40}, {689, 0x40}, {693, 0x40}, {697, 0x40},
    {701, 0x40}, {705, 0x40}, {709, 0x40}, {713, 0x40},
    {717, 0x40}, {721, 0x40}, {725, 0x40}, {729, 0x40},
    {733, 0x40}, {737, 0x40}, {741, 0x40}, {745, 0x40},
    {749, 0x40}, {753, 0x40}, {757, 0x40}, {761, 0x40},
    {765, 0x40}, {769, 0x40}, {773, 0x40}, {777, 0x40},
    {781, 0x40}, {785, 0x40}, {789, 0x40}, {793, 0x40},
    {797, 0x30}, {801, 0x40},
    {-1, 0}
};

#define SIZE6X4PAL		45
static struct _ffolat ffo6x4pal[SIZE6X4PAL + 1] = {
    {625, 0x60}, {629, 0x60}, {633, 0x60}, {637, 0x60},
    {641, 0x50}, {645, 0x60}, {649, 0x60}, {653, 0x60},
    {657, 0x60}, {661, 0x60}, {665, 0x60}, {669, 0x60},
    {673, 0x60}, {677, 0x60}, {681, 0x60}, {685, 0x60},
    {689, 0x60}, {693, 0x60}, {697, 0x60}, {701, 0x60},
    {705, 0x60}, {709, 0x60}, {713, 0x60}, {717, 0x60},
    {721, 0x60}, {725, 0x60}, {729, 0x60}, {733, 0x60},
    {737, 0x60}, {741, 0x60}, {745, 0x60}, {749, 0x60},
    {753, 0x60}, {757, 0x60}, {761, 0x60}, {765, 0x60},
    {769, 0x60}, {773, 0x60}, {777, 0x60}, {781, 0x60},
    {785, 0x60}, {789, 0x60}, {793, 0x60}, {797, 0x60},
    {801, 0x60},
    {-1, 0}
};

#define SIZE7X4NTSC		40
static struct _ffolat ffo7x4ntsc[SIZE7X4NTSC + 1] = {
    {525, 0x52}, {529, 0x52}, {533, 0x52}, {537, 0x52},
    {541, 0x52}, {545, 0x40}, {549, 0x40}, {553, 0x40},
    {557, 0x58}, {561, 0x40}, {565, 0x58}, {569, 0x40},
    {573, 0x48}, {577, 0x40}, {581, 0x40}, {585, 0x40},
    {589, 0x40}, {593, 0x48}, {597, 0x40}, {601, 0x40},
    {605, 0x40}, {609, 0x40}, {613, 0x5b}, {617, 0x48},
    {621, 0x60}, {625, 0x48}, {629, 0x48}, {633, 0x40},
    {637, 0x5e}, {641, 0x40}, {645, 0x50}, {649, 0x56},
    {653, 0x58}, {657, 0x6c}, {661, 0x40}, {665, 0x40},
    {669, 0x40}, {673, 0x40}, {677, 0x40}, {681, 0x40},
    {-1, 0}
};

#define SIZE7X4PAL		24
static struct _ffolat ffo7x4pal[SIZE7X4PAL + 1] = {
    {625, 0x60}, {629, 0x60}, {633, 0x60}, {637, 0x60},
    {641, 0x50}, {645, 0x60}, {649, 0x60}, {653, 0x60},
    {657, 0x60}, {661, 0x60}, {665, 0x60}, {669, 0x60},
    {673, 0x60}, {677, 0x60}, {681, 0x60}, {685, 0x60},
    {689, 0x60}, {693, 0x60}, {697, 0x60}, {701, 0x60},
    {705, 0x60}, {709, 0x60}, {713, 0x60}, {717, 0x60},
    {-1, 0}
};

#define SIZE7X5NTSC		54
static struct _ffolat ffo7x5ntsc[SIZE7X5NTSC + 1] = {
    {590, 0x40}, {594, 0x48}, {598, 0x40}, {602, 0x40},
    {606, 0x40}, {610, 0x40}, {614, 0x5b}, {618, 0x48},
    {622, 0x60}, {626, 0x48}, {630, 0x48}, {634, 0x40},
    {638, 0x5e}, {642, 0x40}, {646, 0x50}, {650, 0x56},
    {654, 0x58}, {658, 0x6c}, {662, 0x40}, {666, 0x40},
    {670, 0x40}, {674, 0x40}, {678, 0x40}, {682, 0x40},
    {686, 0x40}, {690, 0x40}, {694, 0x40}, {698, 0x40},
    {702, 0x40}, {706, 0x40}, {710, 0x40}, {714, 0x40},
    {718, 0x40}, {722, 0x40}, {726, 0x40}, {730, 0x40},
    {734, 0x40}, {738, 0x40}, {742, 0x40}, {746, 0x40},
    {750, 0x40}, {754, 0x40}, {758, 0x40}, {762, 0x40},
    {766, 0x40}, {770, 0x40}, {774, 0x40}, {778, 0x40},
    {782, 0x40}, {786, 0x40}, {790, 0x40}, {794, 0x40},
    {798, 0x30}, {802, 0x40},
    {-1, 0}
};

#define SIZE7X5PAL		45
static struct _ffolat ffo7x5pal[SIZE7X5PAL + 1] = {
    {625, 0x60}, {629, 0x60}, {633, 0x60}, {637, 0x60},
    {641, 0x50}, {645, 0x60}, {649, 0x60}, {653, 0x60},
    {657, 0x60}, {661, 0x60}, {665, 0x60}, {669, 0x60},
    {673, 0x60}, {677, 0x60}, {681, 0x60}, {685, 0x60},
    {689, 0x60}, {693, 0x60}, {697, 0x60}, {701, 0x60},
    {705, 0x60}, {709, 0x60}, {713, 0x60}, {717, 0x60},
    {721, 0x60}, {725, 0x60}, {729, 0x60}, {733, 0x60},
    {737, 0x60}, {741, 0x60}, {745, 0x60}, {749, 0x60},
    {753, 0x60}, {757, 0x60}, {761, 0x60}, {765, 0x60},
    {769, 0x60}, {773, 0x60}, {777, 0x60}, {781, 0x60},
    {785, 0x60}, {789, 0x60}, {793, 0x60}, {797, 0x60},
    {801, 0x60},
    {-1, 0}
};

/* h_total=1056, vga_lines=600 */
#define	SIZE8X6NTSC		37
static struct _ffolat ffo8x6ntsc[SIZE8X6NTSC + 1] = {
    {620, 0x40},                /* v_total_min >= vsync+10 >= vga_lines+10 = 610 */
    {625, 0x58}, {630, 0x40}, {635, 0x40}, {640, 0x40},
    {645, 0x46}, {650, 0x46}, {655, 0x4f}, {660, 0x4c},
    {665, 0x4a}, {670, 0x50}, {675, 0x2f}, {680, 0x48},
    {685, 0x38}, {690, 0x31}, {695, 0x40}, {700, 0x21},
    {705, 0x25}, {710, 0x40}, {715, 0x48}, {720, 0x50},
    {725, 0x30}, {730, 0x50}, {735, 0x50}, {740, 0x50},
    {745, 0x40}, {750, 0x38}, {755, 0x50}, {760, 0x50},
    {765, 0x40}, {770, 0x38}, {775, 0x40}, {780, 0x40},
    {785, 0x40}, {790, 0x38}, {795, 0x50}, {800, 0x50},
    {-1, 0}
};

/* h_total=1056, vga_lines=600 */
#define	SIZE8X6PAL		36
static struct _ffolat ffo8x6pal[SIZE8X6PAL + 1] = {
    {625, 0x80}, {630, 0x80}, {635, 0x5a}, {640, 0x55},
    {645, 0x48}, {650, 0x65}, {655, 0x65}, {660, 0x50},
    {665, 0x80}, {670, 0x70}, {675, 0x56}, {680, 0x80},
    {685, 0x58}, {690, 0x31}, {695, 0x80}, {700, 0x60},
    {705, 0x45}, {710, 0x4a}, {715, 0x50}, {720, 0x50},
    {725, 0x50}, {730, 0x45}, {735, 0x50}, {740, 0x50},
    {745, 0x50}, {750, 0x50}, {755, 0x50}, {760, 0x50},
    {765, 0x50}, {770, 0x50}, {775, 0x50}, {780, 0x50},
    {785, 0x50}, {790, 0x50}, {795, 0x50}, {800, 0x50},
    {-1, 0}
};

/* h_total=1344, vga_lines=768 */
#define	SIZE10X7NTSC		45
static struct _ffolativo ffo10x7ntsc[SIZE10X7NTSC] = {
    {783, 0x4d, 0x40},
    {789, 0x47, 0x14},
    {795, 0x47, 0x7f},
    {801, 0x47, 0x53},
    {807, 0x47, 0x11},
    {813, 0x47, 0x78},
    {819, 0x47, 0x54},
    {825, 0x47, 0x40},
    {831, 0x47, 0x0f},
    {837, 0x4d, 0x40},
    {843, 0x47, 0x5a},
    {849, 0x4d, 0x40},
    {855, 0x47, 0x4b},
    {861, 0x4d, 0x40},
    {867, 0x47, 0x4b},
    {873, 0x4d, 0x40},
    {879, 0x47, 0x07},
    {885, 0x48, 0x20},
    {891, 0x47, 0x82},
    {897, 0x47, 0x60},
    {903, 0x47, 0x7f},
    {909, 0x4d, 0x40},
    {915, 0x48, 0x40},
    {921, 0x4c, 0x40},
    {927, 0x49, 0x40},
    {933, 0x48, 0x40},
    {939, 0x4a, 0x40},
    {945, 0x46, 0x40},
    {951, 0x4a, 0x40},
    {957, 0x4a, 0x40},
    {963, 0x4b, 0x40},
    {969, 0x4b, 0x40},
    {975, 0x48, 0x40},
    {981, 0x47, 0x40},
    {987, 0x47, 0x40},
    {993, 0x47, 0x40},
    {999, 0x48, 0x40},
    {1005, 0x48, 0x40},
    {1011, 0x47, 0x40},
    {1017, 0x47, 0x40},
    {1023, 0x48, 0x40},
    {1029, 0x48, 0x40},
    {1035, 0x46, 0x40},
    {1041, 0x47, 0x40},
    {1047, 0x47, 0x40}
};

/* h_total=1344, vga_lines=768 */
#define	SIZE10X7PAL		46
static struct _ffolativo ffo10x7pal[SIZE10X7PAL] = {
    {781, 0x49, 0x40},
    {787, 0x46, 0x40},
    {793, 0x48, 0x40},
    {799, 0x46, 0x40},
    {805, 0x49, 0x40},
    {811, 0x47, 0x40},
    {817, 0x46, 0x40},
    {823, 0x46, 0x56},
    {829, 0x46, 0x2d},
    {835, 0x46, 0x40},
    {841, 0x46, 0x2d},
    {847, 0x46, 0x3f},
    {853, 0x46, 0x10},
    {859, 0x46, 0x86},
    {865, 0x46, 0xc9},
    {871, 0x46, 0x83},
    {877, 0x46, 0xa8},
    {883, 0x46, 0x81},
    {889, 0x46, 0xa5},
    {895, 0x46, 0xa9},
    {901, 0x46, 0x81},
    {907, 0x46, 0xa4},
    {913, 0x46, 0xa5},
    {919, 0x46, 0x7f},
    {925, 0x46, 0xa2},
    {931, 0x46, 0x9d},
    {937, 0x46, 0xc1},
    {943, 0x46, 0x96},
    {949, 0x46, 0xb7},
    {955, 0x46, 0xb1},
    {961, 0x46, 0x8a},
    {967, 0x46, 0xa9},
    {973, 0x46, 0xa0},
    {979, 0x46, 0x40},
    {985, 0x46, 0x97},
    {991, 0x46, 0xb5},
    {997, 0x46, 0xaa},
    {1003, 0x46, 0x83},
    {1009, 0x46, 0x9f},
    {1015, 0x47, 0x40},
    {1021, 0x46, 0xad},
    {1027, 0x46, 0x87},
    {1033, 0x46, 0xa2},
    {1039, 0x47, 0x40},
    {1045, 0x46, 0xac},
    {1051, 0x46, 0x86}
};

/*==========================================================================*/
/*FS450 API Functions.														*/
/*==========================================================================*/

/* Initialize device settings */
static void
initialize_houston_static_registers(void)
{
    houston_WriteReg(HOUSTON_BYP, 0, 2);
    houston_WriteReg(HOUSTON_APO, 0, 2);
    houston_WriteReg(HOUSTON_ALO, 0, 2);
    houston_WriteReg(HOUSTON_AFO, 0, 2);
    houston_WriteReg(HOUSTON_BCONTL, 0, 2);
    houston_WriteReg(HOUSTON_BCONTH, 0, 2);
    houston_WriteReg(HOUSTON_BDONE, 0, 2);
    houston_WriteReg(HOUSTON_BDIAGL, 0, 2);
    houston_WriteReg(HOUSTON_BDIAGH, 0, 2);
    houston_WriteReg(HOUSTON_MISC, 0, 2);
}

int
FS450_init(void)
{
    int err;

    TRACE(("FS450_Init()\n"))
        err = houston_init();
    if (err)
        return err;

    initialize_houston_static_registers();

    d.tv_on = PLAL_IsTVOn()? 1 : 0;

    /* get the current tv standard */
    conget_tv_std(&d.tv_std);

    d.vga_mode = 0;

    /* default to VP_TVOUT_MODE_CVBS_YC */
    d.tvout_mode = GFX_TVOUT_MODE_CVBS_YC;

    /* default to 1000 out of 1000 */
    d.sharpness = 1000;
    config_sharpness(d.sharpness);

    /* default to 800 out of 1000 */
    d.flicker = 800;
    config_flicker(d.flicker);

    /* default to zeros */
    d.overscan_x = 0;
    d.overscan_y = 0;
    d.position_x = 0;
    d.position_y = 0;

    d.color = 50;
    /* d.color = tvsetup.color[k]; */
    config_color(d.color);

    /* default */
    d.brightness = 50;
    d.contrast = 60;
    config_brightness_contrast(d.tv_std, d.aps_trigger_bits, d.brightness,
                               d.contrast);

    /* get the current yc filtering */
    {
        int luma_filter, chroma_filter;

        conget_yc_filter(&luma_filter, &chroma_filter);
        d.yc_filter = 0;
        if (luma_filter)
            d.yc_filter |= GFX_LUMA_FILTER;
        if (chroma_filter)
            d.yc_filter |= GFX_CHROMA_FILTER;
    }

    d.aps_trigger_bits = 0;
    config_macrovision(d.tv_std, d.aps_trigger_bits);

    d.last_overscan_y = -10000;

    return 0;
}

void
FS450_cleanup(void)
{
}

/*==========================================================================*/
/* Required configuration calls to write new settings to the device			*/
/*==========================================================================*/

#define REQ_TV_STANDARD_BIT			0x0002
#define REQ_VGA_MODE_BIT			0x0004
#define REQ_TVOUT_MODE_BIT			0x0008
#define REQ_SHARPNESS_BIT			0x0010
#define REQ_FLICKER_BIT				0x0020
#define REQ_OVERSCAN_POSITION_BIT	0x0040
#define REQ_COLOR_BIT				0x0080
#define REQ_BRIGHTNESS_CONTRAST_BIT 0x0100
#define REQ_YC_FILTER_BIT			0x0200
#define REQ_MACROVISION_BIT			0x0400
#define REQ_NCO_BIT					0x1000

#define REQ_TV_STANDARD			(REQ_TV_STANDARD_BIT | REQ_OVERSCAN_POSITION \
									| REQ_BRIGHTNESS_CONTRAST 				 \
									| REQ_MACROVISION_BIT | REQ_YC_FILTER)
#define REQ_VGA_MODE			(REQ_VGA_MODE_BIT | REQ_OVERSCAN_POSITION)
#define REQ_TVOUT_MODE			(REQ_TVOUT_MODE_BIT)
#define REQ_SHARPNESS			(REQ_SHARPNESS_BIT)
#define REQ_FLICKER				(REQ_FLICKER_BIT)
#define REQ_OVERSCAN_POSITION	(REQ_OVERSCAN_POSITION_BIT | REQ_NCO)
#define REQ_COLOR				(REQ_COLOR_BIT)
#define REQ_BRIGHTNESS_CONTRAST	(REQ_BRIGHTNESS_CONTRAST_BIT)
#define REQ_YC_FILTER			(REQ_YC_FILTER_BIT)
#define REQ_MACROVISION			(REQ_TV_STANDARD_BIT | 						 \
									REQ_BRIGHTNESS_CONTRAST_BIT | 			 \
									REQ_MACROVISION_BIT)
#define REQ_NCO					(REQ_NCO_BIT)
#define REQ_ENCODER				(REQ_TV_STANDARD | REQ_COLOR | 				 \
									REQ_BRIGHTNESS_CONTRAST | REQ_YC_FILTER)

static int
write_config(int req)
{
    unsigned long reg, reg_encoder_reset = 0;
    int reset;

    /*if we're changing the nco, and the vertical scaling has changed... */
    reset = ((REQ_NCO_BIT & req) && (d.overscan_y != d.last_overscan_y));
    if (reset) {
        /*put the encoder into reset while making changes */
        houston_ReadReg(ENC_RESET, &reg, 1);
        houston_WriteReg(ENC_RESET, reg | 0x01, 1);
        reg_encoder_reset = reg & 0x01;
    }

    if (REQ_TV_STANDARD_BIT & req)
        config_tv_std(d.tv_std, d.aps_trigger_bits);

    if (REQ_VGA_MODE_BIT & req)
        config_vga_mode(d.vga_mode);

    if (REQ_TVOUT_MODE_BIT & req)
        config_tvout_mode(d.tvout_mode);

    if (REQ_OVERSCAN_POSITION_BIT & req) {
        config_overscan_xy(d.tv_std,
                           d.vga_mode,
                           d.overscan_x, d.overscan_y, d.position_x,
                           d.position_y);

        /*h_timing and v_timing and syncs. */
        if (PLAL_IsTVOn())
            PLAL_SetTVTimingRegisters(p_specs());
    }

    if (REQ_NCO_BIT & req)
        config_nco(d.tv_std, d.vga_mode);

    if (REQ_SHARPNESS_BIT & req)
        config_sharpness(d.sharpness);

    if (REQ_FLICKER_BIT & req)
        config_flicker(d.flicker);

    if (REQ_COLOR_BIT & req)
        config_color(d.color);

    if (REQ_BRIGHTNESS_CONTRAST_BIT & req) {
        config_brightness_contrast(d.tv_std,
                                   d.aps_trigger_bits, d.brightness,
                                   d.contrast);
    }

    if (REQ_YC_FILTER_BIT & req) {
        config_yc_filter(d.tv_std,
                         (d.yc_filter & GFX_LUMA_FILTER),
                         (d.yc_filter & GFX_CHROMA_FILTER));
    }

    if (REQ_MACROVISION_BIT & req)
        config_macrovision(d.tv_std, d.aps_trigger_bits);

    /*if we decided to put the encoder into reset, put it back */
    if (reset) {
        houston_ReadReg(ENC_RESET, &reg, 1);
        houston_WriteReg(ENC_RESET, reg_encoder_reset | (reg & ~0x01), 1);

        d.last_overscan_y = d.overscan_y;
    }
    return 0;
}

/*==========================================================================*/
/* TV On																	*/
/*==========================================================================*/

#if GFX_TV_DYNAMIC
int
fs450_get_tv_enable(unsigned int *p_on)
#else
int
gfx_get_tv_enable(unsigned int *p_on)
#endif
{
    if (!p_on)
        return ERR_INVALID_PARAMETER;

    *p_on = d.tv_on;

    return 0;
}

/*//int FS450_set_tv_on(unsigned int on)*/
#if GFX_TV_DYNAMIC
int
fs450_set_tv_enable(int on)
#else
int
gfx_set_tv_enable(int on)
#endif
{
    unsigned long reg;

    /*if not mode change, just return */
    if ((d.tv_on && on) || (!d.tv_on && !on))
        return 0;

    /*if turning off... */
    if (!on) {
        /*re-enable vga. */
        PLAL_EnableVga();

        /*power down houston */
        config_power(0);

        d.tv_on = 0;

        return 0;
    }

    /*turning on... */

    /*power up houston      */
    config_power(1);

    /*assert encoder reset. */
    houston_WriteReg(ENC_RESET, 0x01, 1);

    /*initial platform preparation */
    PLAL_PrepForTVout();

    /*configure encoder and nco. */
    write_config(REQ_VGA_MODE |
                 REQ_TV_STANDARD |
                 REQ_TVOUT_MODE |
                 REQ_OVERSCAN_POSITION | REQ_YC_FILTER | REQ_MACROVISION);

    /*set LP_EN and UIM */
    houston_ReadReg(HOUSTON_CR, &reg, 2);
    reg |= CR_LP_EN;
    reg &= ~(CR_UIM_MOD0 | CR_UIM_MOD1);
    reg |= (PLAL_FS450_UIM_mode() << 14);
    houston_WriteReg(HOUSTON_CR, reg, 2);

    /*set platform timing registers */
    PLAL_SetTVTimingRegisters(p_specs());

    PLAL_FinalEnableTVout(d.vga_mode);

    /*sync bridge */
    {
        int retry_count = 0;

        /*sync 50 times */
        while (retry_count++ < 50) {
            /*sync bridge. */
            houston_ReadReg(HOUSTON_MISC, &reg, 2);
            reg |= MISC_BRIDGE_SYNC;
            houston_WriteReg(HOUSTON_MISC, reg, 2);
            reg &= ~MISC_BRIDGE_SYNC;
            houston_WriteReg(HOUSTON_MISC, reg, 2);
        }
    }

    /*deassert encoder reset. */
    houston_WriteReg(ENC_RESET, 0x00, 1);

    d.tv_on = 1;

    return 0;
}

#if GFX_TV_DYNAMIC
int
fs450_set_tv_defaults(int format)
#else
int
gfx_set_tv_defaults(int format)
#endif
{
    return 0;
}

/*==========================================================================*/
/* TV standard																*/
/*==========================================================================*/

#if GFX_TV_DYNAMIC
int
fs450_get_tv_standard(unsigned long *p_standard)
#else
int
gfx_get_tv_standard(unsigned long *p_standard)
#endif
{
    if (!p_standard)
        return ERR_INVALID_PARAMETER;

    *p_standard = d.tv_std;

    return 0;
}

#if GFX_TV_DYNAMIC
int
fs450_get_available_tv_standards(unsigned long *p_standards)
#else
int
gfx_get_available_tv_standards(unsigned long *p_standards)
#endif
{
    if (!p_standards)
        return ERR_INVALID_PARAMETER;

    *p_standards = supported_standards();

    return 0;
}

#if GFX_TV_DYNAMIC
int
fs450_set_tv_standard(unsigned long standard)
#else
int
gfx_set_tv_standard(unsigned long standard)
#endif
{
    /* verify supported standard. */
    if (!(standard & supported_standards()))
        return ERR_INVALID_PARAMETER;

    /* disallow if tv is on */
    if (d.tv_on)
        return ERR_CANNOT_CHANGE_WHILE_TV_ON;

    d.tv_std = standard;
    /* d.color = tvsetup.color[k]; */

    return write_config(REQ_TV_STANDARD);
}

/*==========================================================================*/
/* vga mode as known by the driver											*/
/*==========================================================================*/

#if GFX_TV_DYNAMIC
int
fs450_get_tv_vga_mode(unsigned long *p_vga_mode)
#else
int
gfx_get_tv_vga_mode(unsigned long *p_vga_mode)
#endif
{
    if (!p_vga_mode)
        return ERR_INVALID_PARAMETER;

    *p_vga_mode = d.vga_mode;

    return 0;
}

#if GFX_TV_DYNAMIC
int
fs450_get_available_tv_vga_modes(unsigned long *p_vga_modes)
#else
int
gfx_get_available_tv_vga_modes(unsigned long *p_vga_modes)
#endif
{
    if (!p_vga_modes)
        return ERR_INVALID_PARAMETER;

    *p_vga_modes =
        GFX_VGA_MODE_640X480 |
        GFX_VGA_MODE_720X487 | GFX_VGA_MODE_720X576 | GFX_VGA_MODE_800X600;
    if (houston_Rev() >= HOUSTON_REV_B)
        *p_vga_modes |= GFX_VGA_MODE_1024X768;

    return 0;
}

#if GFX_TV_DYNAMIC
int
fs450_set_tv_vga_mode(unsigned long vga_mode)
#else
int
gfx_set_tv_vga_mode(unsigned long vga_mode)
#endif
{
    /*reject if not a single valid VGA mode */
    switch (vga_mode) {
    default:
        return ERR_INVALID_PARAMETER;
    case GFX_VGA_MODE_640X480:
    case GFX_VGA_MODE_720X487:
    case GFX_VGA_MODE_720X576:
    case GFX_VGA_MODE_800X600:
        break;
    case GFX_VGA_MODE_1024X768:
        if (houston_Rev() >= HOUSTON_REV_B)
            break;
        return ERR_INVALID_PARAMETER;
    }

    /*if the mode has changed... */
    if (vga_mode != d.vga_mode) {
        d.vga_mode = vga_mode;

        return write_config(REQ_VGA_MODE);
    }

    return 0;
}

/*==========================================================================*/
/* tvout mode																*/
/*==========================================================================*/

#if GFX_TV_DYNAMIC
int
fs450_get_tvout_mode(unsigned long *p_tvout_mode)
#else
int
gfx_get_tvout_mode(unsigned long *p_tvout_mode)
#endif
{
    if (!p_tvout_mode)
        return ERR_INVALID_PARAMETER;

    *p_tvout_mode = d.tvout_mode;

    return 0;
}

#if GFX_TV_DYNAMIC
int
fs450_set_tvout_mode(unsigned long tvout_mode)
#else
int
gfx_set_tvout_mode(unsigned long tvout_mode)
#endif
{
    d.tvout_mode = tvout_mode;

    return write_config(REQ_TVOUT_MODE);
}

/*==========================================================================*/
/* Sharpness																*/
/*==========================================================================*/

#if GFX_TV_DYNAMIC
int
fs450_get_sharpness(int *p_sharpness)
#else
int
gfx_get_sharpness(int *p_sharpness)
#endif
{
    if (!p_sharpness)
        return ERR_INVALID_PARAMETER;

    *p_sharpness = d.sharpness;

    return 0;
}

#if GFX_TV_DYNAMIC
int
fs450_set_sharpness(int sharpness)
#else
int
gfx_set_sharpness(int sharpness)
#endif
{
    d.sharpness = range_limit(sharpness, 0, 1000);

    return write_config(REQ_SHARPNESS);
}

/*==========================================================================*/
/* flicker filter control.													*/
/*==========================================================================*/

#if GFX_TV_DYNAMIC
int
fs450_get_flicker_filter(int *p_flicker)
#else
int
gfx_get_flicker_filter(int *p_flicker)
#endif
{
    if (!p_flicker)
        return ERR_INVALID_PARAMETER;

    *p_flicker = d.flicker;

    return 0;
}

#if GFX_TV_DYNAMIC
int
fs450_set_flicker_filter(int flicker)
#else
int
gfx_set_flicker_filter(int flicker)
#endif
{
    d.flicker = range_limit(flicker, 0, 1000);

    return write_config(REQ_FLICKER);
}

/*==========================================================================*/
/* Overscan and Position													*/
/*==========================================================================*/

#if GFX_TV_DYNAMIC
int
fs450_get_overscan(int *p_x, int *p_y)
#else
int
gfx_get_overscan(int *p_x, int *p_y)
#endif
{
    if (!p_x || !p_y)
        return ERR_INVALID_PARAMETER;

    *p_x = d.overscan_x;
    *p_y = d.overscan_y;

    return 0;
}

#if GFX_TV_DYNAMIC
int
fs450_set_overscan(int x, int y)
#else
int
gfx_set_overscan(int x, int y)
#endif
{
    d.overscan_x = range_limit(x, -1000, 1000);
    d.overscan_y = range_limit(y, -1000, 1000);

    return write_config(REQ_OVERSCAN_POSITION);
}

#if GFX_TV_DYNAMIC
int
fs450_get_position(int *p_x, int *p_y)
#else
int
gfx_get_position(int *p_x, int *p_y)
#endif
{
    if (!p_x || !p_y)
        return ERR_INVALID_PARAMETER;

    *p_x = d.position_x;
    *p_y = d.position_y;

    return 0;
}

#if GFX_TV_DYNAMIC
int
fs450_set_position(int x, int y)
#else
int
gfx_set_position(int x, int y)
#endif
{
    d.position_x = range_limit(x, -1000, 1000);
    d.position_y = range_limit(y, -1000, 1000);

    return write_config(REQ_OVERSCAN_POSITION);
}

/*==========================================================================*/
/* Color, Brightness, and Contrast											*/
/*==========================================================================*/

#if GFX_TV_DYNAMIC
int
fs450_get_color(int *p_color)
#else
int
gfx_get_color(int *p_color)
#endif
{
    if (!p_color)
        return ERR_INVALID_PARAMETER;

    *p_color = d.color;

    return 0;
}

#if GFX_TV_DYNAMIC
int
fs450_set_color(int color)
#else
int
gfx_set_color(int color)
#endif
{
    d.color = range_limit(color, 0, 100);

    return write_config(REQ_COLOR);
}

#if GFX_TV_DYNAMIC
int
fs450_get_brightness(int *p_brightness)
#else
int
gfx_get_brightness(int *p_brightness)
#endif
{
    if (!p_brightness)
        return ERR_INVALID_PARAMETER;

    *p_brightness = d.brightness;

    return 0;
}

#if GFX_TV_DYNAMIC
int
fs450_set_brightness(int brightness)
#else
int
gfx_set_brightness(int brightness)
#endif
{
    d.brightness = range_limit(brightness, 0, 100);

    return write_config(REQ_BRIGHTNESS_CONTRAST);
}

#if GFX_TV_DYNAMIC
int
fs450_get_contrast(int *p_contrast)
#else
int
gfx_get_contrast(int *p_contrast)
#endif
{
    if (!p_contrast)
        return ERR_INVALID_PARAMETER;

    *p_contrast = d.contrast;

    return 0;
}

#if GFX_TV_DYNAMIC
int
fs450_set_contrast(int contrast)
#else
int
gfx_set_contrast(int contrast)
#endif
{
    d.contrast = range_limit(contrast, 0, 100);

    return write_config(REQ_BRIGHTNESS_CONTRAST);
}

/*==========================================================================*/
/* YC filters																*/
/*==========================================================================*/

#if GFX_TV_DYNAMIC
int
fs450_get_yc_filter(unsigned int *p_yc_filter)
#else
int
gfx_get_yc_filter(unsigned int *p_yc_filter)
#endif
{
    if (!p_yc_filter)
        return ERR_INVALID_PARAMETER;

    if (houston_Rev() < HOUSTON_REV_B)
        return ERR_NOT_SUPPORTED;

    *p_yc_filter = d.yc_filter;

    return 0;
}

#if GFX_TV_DYNAMIC
int
fs450_set_yc_filter(unsigned int yc_filter)
#else
int
gfx_set_yc_filter(unsigned int yc_filter)
#endif
{
    if (houston_Rev() < HOUSTON_REV_B)
        return ERR_NOT_SUPPORTED;

    /*luma filter. */
    if (yc_filter & GFX_LUMA_FILTER)
        d.yc_filter |= GFX_LUMA_FILTER;
    else
        d.yc_filter &= ~GFX_LUMA_FILTER;

    /*chroma filter. */
    if (yc_filter & GFX_CHROMA_FILTER)
        d.yc_filter |= GFX_CHROMA_FILTER;
    else
        d.yc_filter &= ~GFX_CHROMA_FILTER;

    return write_config(REQ_YC_FILTER);
}

#if GFX_TV_DYNAMIC
int
fs450_get_aps_trigger_bits(unsigned int *p_trigger_bits)
#else
int
gfx_get_aps_trigger_bits(unsigned int *p_trigger_bits)
#endif
{
    if (!p_trigger_bits)
        return ERR_INVALID_PARAMETER;

    *p_trigger_bits = d.aps_trigger_bits;

    return 0;
}

#if GFX_TV_DYNAMIC
int
fs450_set_aps_trigger_bits(unsigned int trigger_bits)
#else
int
gfx_set_aps_trigger_bits(unsigned int trigger_bits)
#endif
{
    d.aps_trigger_bits = trigger_bits;

    return write_config(REQ_MACROVISION);
}

/*----------------------------------------------------------------------------
 * gfx_set_tv_format
 *
 * This routine sets the TV encoder registers to the specified format
 * and resolution.
 * Currently only NTSC 640x480 is supported.
 *----------------------------------------------------------------------------
 */
#if GFX_TV_DYNAMIC
int
fs450_set_tv_format(TVStandardType format, GfxOnTVType resolution)
#else
int
gfx_set_tv_format(TVStandardType format, GfxOnTVType resolution)
#endif
{
    /* ### ADD ### IMPLEMENTATION */
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_tv_output
 *
 * This routine sets the TV encoder registers to the specified output type.
 * Supported output types are : S-VIDEO and Composite.
 *----------------------------------------------------------------------------
 */
#if GFX_TV_DYNAMIC
int
fs450_set_tv_output(int output)
#else
int
gfx_set_tv_output(int output)
#endif
{
    /* ### ADD ### IMPLEMENTATION */
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_tv_cc_enable
 *
 * This routine enables or disables the use of the hardware CC registers 
 * in the TV encoder.
 *----------------------------------------------------------------------------
 */
#if GFX_TV_DYNAMIC
int
fs450_set_tv_cc_enable(int enable)
#else
int
gfx_set_tv_cc_enable(int enable)
#endif
{
    /* ### ADD ### IMPLEMENTATION */
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_tv_cc_data
 *
 * This routine writes the two specified characters to the CC data register 
 * of the TV encoder.
 *----------------------------------------------------------------------------
 */
#if GFX_TV_DYNAMIC
int
fs450_set_tv_cc_data(unsigned char data1, unsigned char data2)
#else
int
gfx_set_tv_cc_data(unsigned char data1, unsigned char data2)
#endif
{
    /* ### ADD ### IMPLEMENTATION */
    return (0);
}

#ifdef FS450_DIRECTREG

/*==========================================================================*/
/* Direct Read and Write registers											*/
/*==========================================================================*/

int
FS450_ReadRegister(S_REG_INFO *p_reg)
{
    unsigned long tmp;

    if (PLAL_ReadRegister(p_reg))
        return 0;

    if (SOURCE_HOUSTON == p_reg->source) {
        switch (p_reg->size) {
        case 1:
        case 2:
        {
            houston_ReadReg((int) p_reg->offset, &tmp, (int) p_reg->size);
            p_reg->value = tmp;
        }
            return 0;

        case 4:
        {
            houston_ReadReg((unsigned int) p_reg->offset, &tmp, 2);
            p_reg->value = (tmp << 16);
            houston_ReadReg((unsigned int) (p_reg->offset + 2), &tmp, 2);
            p_reg->value |= tmp;
        }
            return 0;
        }
    }

    return ERR_INVALID_PARAMETER;
}

int
FS450_WriteRegister(S_REG_INFO *p_reg)
{
    if (PLAL_WriteRegister(p_reg))
        return 0;

    if (SOURCE_HOUSTON == p_reg->source) {
        houston_WriteReg((unsigned int) p_reg->offset, p_reg->value,
                         p_reg->size);

        return 0;
    }

    return ERR_INVALID_PARAMETER;
}

#endif

/* Houston initialization function. */
static int g_houston_rev = -1;

static int
houston_init(void)
{
    /*//int errc; */
    unsigned long write, read;

    TRACE(("houston_init()\n"))
        /*Before we begin, we must enable power to the TFT */
        read = READ_VID32(CS5530_DISPLAY_CONFIG);
    read |= CS5530_DCFG_FP_PWR_EN | CS5530_DCFG_FP_DATA_EN;
    WRITE_VID32(CS5530_DISPLAY_CONFIG, read);

    /*simple w/r test. */
    write = 0x0055;
    read = 0;

    houston_WriteReg(HOUSTON_IHO, write, 2);
    houston_ReadReg(HOUSTON_IHO, &read, 2);
    if (read != write) {
        houston_WriteReg(HOUSTON_IHO, write, 2);
        houston_ReadReg(HOUSTON_IHO, &read, 2);
        if (read != write) {
            /*chip is not there, do something appropriate? */
            TRACE(("wrote HOUSTON_IHO=0x0055, read 0x%04x\n", read))
                return ERR_DEVICE_NOT_FOUND;
        }
    }

    /*read chip revision. */
    houston_ReadReg(HOUSTON_REV, &read, 2);
    g_houston_rev = (int) read;

    /*ok. */
    return 0;
}

static int
houston_Rev(void)
{
    return g_houston_rev;
}

static S_TIMING_SPECS g_specs;

static const S_TIMING_SPECS *
p_specs(void)
{
    return &g_specs;
}

/*==========================================================================*/
/* FS450 configuration functions.											*/
/*==========================================================================*/
static int
config_init(void)
{
    int err;

    TRACE(("config_init()\n"))
        err = houston_init();
    if (err)
        return err;

    return 0;
}

/*==========================================================================*/
/* convert word to encoder 10 bit value.									*/
/*==========================================================================*/

static unsigned short
w10bit2z(unsigned short w)
{
    return (w >> 2) | ((w & 0x03) << 8);
}

static unsigned short
z2w10bit(unsigned short z)
{
    return (0x03 & (z >> 8)) | ((0xFF & z) << 2);
}

/*==========================================================================*/
/* TV Standards																*/
/*==========================================================================*/

static const struct {
    unsigned long standard;
    int tvsetup_index;
} g_tv_standards[] = {
    {
     GFX_TV_STANDARD_NTSC_M, 0}, {
                                  GFX_TV_STANDARD_NTSC_M_J, 2}, {
                                                                 GFX_TV_STANDARD_PAL_B,
                                                                 1}, {
                                                                      GFX_TV_STANDARD_PAL_D,
                                                                      1}, {
                                                                           GFX_TV_STANDARD_PAL_H,
                                                                           1}, {
                                                                                GFX_TV_STANDARD_PAL_I,
                                                                                1},
    {
     GFX_TV_STANDARD_PAL_M, 3}, {
                                 GFX_TV_STANDARD_PAL_N, 4}, {
                                                             GFX_TV_STANDARD_PAL_G,
                                                             1},
};

static int
map_tvstd_to_index(unsigned long tv_std)
{
    unsigned int i;

    for (i = 0; i < sizeof(g_tv_standards) / sizeof(*g_tv_standards); i++) {
        if (tv_std == g_tv_standards[i].standard)
            return g_tv_standards[i].tvsetup_index;
    }

    return -1;
}

static unsigned long
supported_standards(void)
{
    unsigned long standards = 0;
    unsigned int i;

    for (i = 0; i < sizeof(g_tv_standards) / sizeof(*g_tv_standards); i++) {
        if (g_tv_standards[i].tvsetup_index >= 0)
            standards |= g_tv_standards[i].standard;
    }

    return standards;
}

/*==========================================================================*/

static void
config_power(int on)
{
    unsigned long reg;

    if (houston_Rev() < HOUSTON_REV_B) {
        /* no power down supported, but still turn of clock in off mode */
        if (on) {
            houston_ReadReg(HOUSTON_CR, &reg, 2);
            reg &= ~(CR_CLKOFF | CR_RESET);
            houston_WriteReg(HOUSTON_CR, reg, 2);
            reg |= CR_RESET;
            houston_WriteReg(HOUSTON_CR, reg, 2);
            reg &= ~CR_RESET;
            houston_WriteReg(HOUSTON_CR, reg, 2);
        }
        else {
            houston_ReadReg(HOUSTON_CR, &reg, 2);
            reg |= CR_CLKOFF;
            houston_WriteReg(HOUSTON_CR, reg, 2);
        }

        return;
    }

    if (on) {
        /* !CLKOFF, !COMPOFF, !YCOFF */
        /* and reset Houston */
        houston_ReadReg(HOUSTON_CR, &reg, 2);
        reg &= ~(CR_CLKOFF | CR_RESET | CR_COMPOFF | CR_YCOFF);
        houston_WriteReg(HOUSTON_CR, reg, 2);
        reg |= CR_RESET;
        houston_WriteReg(HOUSTON_CR, reg, 2);
        reg &= ~CR_RESET;
        houston_WriteReg(HOUSTON_CR, reg, 2);

        /* !GTLIO_PD */
        houston_ReadReg(HOUSTON_MISC, &reg, 2);
        reg &= ~MISC_GTLIO_PD;
        houston_WriteReg(HOUSTON_MISC, reg, 2);
    }
    else {
        /* CLKOFF, COMPOFF, YCOFF */
        houston_ReadReg(HOUSTON_CR, &reg, 2);
        reg |= (CR_CLKOFF | CR_COMPOFF | CR_YCOFF);
        houston_WriteReg(HOUSTON_CR, reg, 2);

        /* GTLIO_PD */
        houston_ReadReg(HOUSTON_MISC, &reg, 2);
        reg |= MISC_GTLIO_PD;
        houston_WriteReg(HOUSTON_MISC, reg, 2);
    }
}

/*==========================================================================*/
/* VGA mode																	*/
/*==========================================================================*/

static void
config_vga_mode(unsigned long vga_mode)
{
    /*h_total must be evenly divisible by 32? */

    static struct {
        unsigned long mode;
        int width;
        int lines;
        int h_total;
    } vgaparams[] = {
        {
         GFX_VGA_MODE_640X480, 640, 480, 1056}, {
                                                 GFX_VGA_MODE_720X487, 720, 487,
                                                 1056}, {
                                                         GFX_VGA_MODE_720X576,
                                                         720, 576, 1056}, {
                                                                           GFX_VGA_MODE_800X600,
                                                                           800,
                                                                           600,
                                                                           1056},
        {
         GFX_VGA_MODE_1024X768, 1024, 768, 1344},
    };

    unsigned long cr, misc, byp;
    unsigned int i;

    g_specs.vga_width = 0;
    g_specs.vga_lines = 0;
    g_specs.h_total = 0;

    for (i = 0; i < sizeof(vgaparams) / sizeof(*vgaparams); i++) {
        if (vga_mode == vgaparams[i].mode) {
            g_specs.vga_width = vgaparams[i].width;
            g_specs.vga_lines = vgaparams[i].lines;
            g_specs.h_total = vgaparams[i].h_total;
            break;
        }
    }
    if (!g_specs.h_total)
        return;

    /*clock mux decimator and vga dual. */
    houston_ReadReg(HOUSTON_CR, &cr, 2);
    houston_ReadReg(HOUSTON_MISC, &misc, 2);
    houston_ReadReg(HOUSTON_BYP, &byp, 2);

    if (vga_mode == GFX_VGA_MODE_1024X768) {
         /*XGA*/ cr |= CR_UIM_DEC;
        misc |= MISC_VGACKDIV;
        byp |= (BYP_HDS_BYPASS | BYP_CAC_BYPASS);
    }
    else {
        /*VGA,SVGA */
        cr &= ~CR_UIM_DEC;
        misc &= ~MISC_VGACKDIV;
        byp &= ~(BYP_HDS_BYPASS | BYP_CAC_BYPASS);
    }

    houston_WriteReg(HOUSTON_CR, cr, 2);
    houston_WriteReg(HOUSTON_MISC, misc, 2);
    houston_WriteReg(HOUSTON_BYP, byp, 2);
}

/*==========================================================================*/
/* Write settings for TV standard to device									*/
/*==========================================================================*/

static void
config_tv_std(unsigned long tv_std, unsigned int trigger_bits)
{
    int k;
    unsigned short reg34;
    unsigned long cr, w;
    unsigned long l;

    /*verify supported standard. */
    k = map_tvstd_to_index(tv_std);
    if (k < 0)
        return;

    /*store tv width and lines */
    g_specs.tv_width = tvsetup.tv_width[k];
    g_specs.tv_lines = tvsetup.tv_lines[k];

    /*houston CR register. */
    houston_ReadReg(HOUSTON_CR, &cr, 2);
    cr &= ~CR_656_PAL_NTSC;
    cr |= tvsetup.houston_cr[k];
    houston_WriteReg(HOUSTON_CR, cr, 2);

    /*setup the encoder. */
    l = tvsetup.chroma_freq[k];
    houston_WriteReg(ENC_CHROMA_FREQ, (int) (l & 0x00ff), 1);
    houston_WriteReg(ENC_CHROMA_FREQ + 1, (int) ((l >> 8) & 0x00ff), 1);
    houston_WriteReg(ENC_CHROMA_FREQ + 2, (int) ((l >> 16) & 0x00ff), 1);
    houston_WriteReg(ENC_CHROMA_FREQ + 3, (int) ((l >> 24) & 0x00ff), 1);

    houston_WriteReg(ENC_CHROMA_PHASE, tvsetup.chroma_phase[k], 1);
    houston_WriteReg(ENC_REG05, 0x00, 1);       /*reg 0x05 */
    houston_WriteReg(ENC_REG06, 0x89, 1);       /*reg 0x06 */
    houston_WriteReg(ENC_REG07, 0x00, 1);       /*reg 0x07 */
    houston_WriteReg(ENC_HSYNC_WIDTH, tvsetup.hsync_width[k], 1);
    houston_WriteReg(ENC_BURST_WIDTH, tvsetup.burst_width[k], 1);
    houston_WriteReg(ENC_BACK_PORCH, tvsetup.back_porch[k], 1);
    houston_WriteReg(ENC_CB_BURST_LEVEL, tvsetup.cb_burst_level[k], 1);
    houston_WriteReg(ENC_CR_BURST_LEVEL, tvsetup.cr_burst_level[k], 1);
    houston_WriteReg(ENC_SLAVE_MODE, 0x01, 1);  /*slave mode */
    if (trigger_bits == 0)
        w = w10bit2z(tvsetup.blank_level[k]);   /*blank level */
    else
        w = w10bit2z((unsigned short) (tvsetup.blank_level[k] -
                                       tvsetup.hamp_offset[k]));
    houston_WriteReg(ENC_BLANK_LEVEL, w & 0x00ff, 1);
    houston_WriteReg(ENC_BLANK_LEVEL + 1, w >> 8, 1);
    w = w10bit2z(tvsetup.tv_lines[k]);  /*num_lines */
    houston_WriteReg(ENC_NUM_LINES, w & 0x00ff, 1);
    houston_WriteReg(ENC_NUM_LINES + 1, w >> 8, 1);

    houston_WriteReg(ENC_TINT, 0x00, 1);        /*tint */
    houston_WriteReg(ENC_BREEZE_WAY, tvsetup.breeze_way[k], 1);
    houston_WriteReg(ENC_FRONT_PORCH, tvsetup.front_porch[k], 1);
    houston_WriteReg(ENC_ACTIVELINE, tvsetup.activeline[k], 1);
    houston_WriteReg(ENC_FIRST_LINE, 0x15, 1);  /*firstvideoline */
    reg34 =
        0x80 |
        (tvsetup.pal_mode[k] << 6) |
        (tvsetup.sys625_50[k] << 3) |
        (tvsetup.cphase_rst[k] << 1) | (tvsetup.vsync5[k]);
    houston_WriteReg(ENC_REG34, reg34, 1);      /*reg 0x34 */
    houston_WriteReg(ENC_SYNC_LEVEL, tvsetup.sync_level[k], 1);
    if (trigger_bits == 0)
        w = w10bit2z(tvsetup.vbi_blank_level[k]);       /*blank level */
    else
        w = w10bit2z((unsigned short) (tvsetup.vbi_blank_level[k] - 1));
    houston_WriteReg(ENC_VBI_BLANK_LEVEL, w & 0x00ff, 1);
    houston_WriteReg(ENC_VBI_BLANK_LEVEL + 1, w >> 8, 1);
}

static void
conget_tv_std(unsigned long *p_tv_standard)
{
    unsigned long cr;

    if (!p_tv_standard)
        return;

    /*just pick between NTSC and PAL */
    houston_ReadReg(HOUSTON_CR, &cr, 2);
    if (CR_656_PAL_NTSC & cr)
        *p_tv_standard = GFX_TV_STANDARD_PAL_B;
    else
        *p_tv_standard = GFX_TV_STANDARD_NTSC_M;
}

/*==========================================================================*/
/* TVout mode																*/
/*==========================================================================*/

static void
config_tvout_mode(unsigned long tvout_mode)
{
    unsigned long cr;

    houston_ReadReg(HOUSTON_CR, &cr, 2);

    /*all dacs off */
    cr |= (CR_COMPOFF | CR_YCOFF);
    /*not rgb */
    cr &= ~CR_OFMT;

    /*turn on requested output */
    if (GFX_TVOUT_MODE_CVBS & tvout_mode)
        cr &= ~CR_COMPOFF;
    if (GFX_TVOUT_MODE_YC & tvout_mode)
        cr &= ~CR_YCOFF;
    if (GFX_TVOUT_MODE_RGB & tvout_mode) {
        cr &= ~(CR_COMPOFF | CR_YCOFF);
        cr |= CR_OFMT;
    }

    houston_WriteReg(HOUSTON_CR, cr, 2);
}

static void
conget_tvout_mode(unsigned long *p_tvout_mode)
{
    unsigned long cr;

    if (!p_tvout_mode)
        return;

    houston_ReadReg(HOUSTON_CR, &cr, 2);

    if (CR_OFMT & cr)
        *p_tvout_mode = GFX_TVOUT_MODE_RGB;
    else {
        *p_tvout_mode = 0;
        if (!(CR_YCOFF & cr))
            *p_tvout_mode |= GFX_TVOUT_MODE_YC;
        if (!(CR_COMPOFF & cr))
            *p_tvout_mode |= GFX_TVOUT_MODE_CVBS;
    }
}

/*==========================================================================*/
/* Size & Position															*/
/*==========================================================================*/

#define IS_NTSC(tv_std) 		\
	(tv_std & ( 				\
	GFX_TV_STANDARD_NTSC_M |   	\
	GFX_TV_STANDARD_NTSC_M_J | 	\
	GFX_TV_STANDARD_PAL_M))
#define IS_PAL(tv_std) 			\
	(tv_std & ( 				\
	GFX_TV_STANDARD_PAL_B | 	\
	GFX_TV_STANDARD_PAL_D | 	\
	GFX_TV_STANDARD_PAL_H | 	\
	GFX_TV_STANDARD_PAL_I | 	\
	GFX_TV_STANDARD_PAL_N | 	\
	GFX_TV_STANDARD_PAL_G))

/*return fifo delay setting for mode, std, and total lines.*/

static void
get_ffolat_ivo(unsigned long vga_mode,
               unsigned long tv_std, long i, unsigned short *ffolat,
               unsigned short *ivo)
{
    switch (vga_mode) {
    case GFX_VGA_MODE_640X480:
        if (IS_NTSC(tv_std)) {
            if (i > SIZE6X4NTSC - 1)
                i = SIZE6X4NTSC - 1;
            *ffolat = ffo6x4ntsc[i].ffolat;
            *ivo = 0x20;
        }
        else {
            if (i > SIZE6X4PAL - 1)
                i = SIZE6X4PAL - 1;
            *ffolat = ffo6x4pal[i].ffolat;
            *ivo = 0x28;
        }
        break;

    case GFX_VGA_MODE_800X600:
        if (IS_NTSC(tv_std)) {
            if (i > SIZE8X6NTSC - 1)
                i = SIZE8X6NTSC - 1;
            *ffolat = ffo8x6ntsc[i].ffolat;
            *ivo = 0x3a;
        }
        else {
            if (i > SIZE8X6PAL - 1)
                i = SIZE8X6PAL - 1;
            *ffolat = ffo8x6pal[i].ffolat;
            *ivo = 0x39;
        }
        break;

    case GFX_VGA_MODE_720X487:
        *ffolat = 0x40;         /*FFO7x4; */
        *ivo = 0x1a;
        break;

    case GFX_VGA_MODE_720X576:
        *ffolat = 0x40;         /*FFO7x5; */
        *ivo = 0x1a;
        break;

    case GFX_VGA_MODE_1024X768:
    default:
        if (IS_NTSC(tv_std)) {
            if (i > SIZE10X7NTSC - 1)
                i = SIZE10X7NTSC - 1;
            *ffolat = ffo10x7ntsc[i].ffolat;
            *ivo = ffo10x7ntsc[i].ivo;
        }
        else {
            if (i > SIZE10X7PAL - 1)
                i = SIZE10X7PAL - 1;
            *ffolat = ffo10x7pal[i].ffolat;
            *ivo = ffo10x7pal[i].ivo;
        }
        break;
    }
}

/*get vertical line min and max for mode and std.*/

static void
get_vtotal_min_max(unsigned long vga_mode,
                   unsigned long tv_std, int *v_total_min, int *v_total_max,
                   int *v_step)
{
    int k = map_tvstd_to_index(tv_std);

    switch (vga_mode) {
    case GFX_VGA_MODE_640X480:
        if (IS_NTSC(tv_std)) {
            *v_total_min = ffo6x4ntsc[0].v_total;
            *v_total_max = ffo6x4ntsc[SIZE6X4NTSC - 1].v_total;
        }
        else {
            *v_total_min = ffo6x4pal[0].v_total;
            *v_total_max = ffo6x4pal[SIZE6X4PAL - 1].v_total;
        }
        *v_step = 4;
        break;

    case GFX_VGA_MODE_800X600:
        if (IS_NTSC(tv_std)) {
            *v_total_min = ffo8x6ntsc[0].v_total;
            *v_total_max = ffo8x6ntsc[SIZE8X6NTSC - 1].v_total;
        }
        else {
            *v_total_min = ffo8x6pal[0].v_total;
            *v_total_max = ffo8x6pal[SIZE8X6PAL - 1].v_total;
        }
        *v_step = 5;
        break;

    case GFX_VGA_MODE_720X487:
    case GFX_VGA_MODE_720X576:
        *v_total_min = tvsetup.tv_lines[k];
        *v_total_max = tvsetup.tv_lines[k];
        *v_step = 4;
        break;

    case GFX_VGA_MODE_1024X768:
        if (IS_NTSC(tv_std)) {
            *v_total_min = ffo10x7ntsc[0].v_total;
            *v_total_max = ffo10x7ntsc[SIZE10X7NTSC - 1].v_total;
        }
        else {
            *v_total_min = ffo10x7pal[0].v_total;
            *v_total_max = ffo10x7pal[SIZE10X7PAL - 1].v_total;
        }
        *v_step = 6;
        break;
    }
}

static void
config_overscan_xy(unsigned long tv_std,
                   unsigned long vga_mode,
                   int overscan_x, int overscan_y, int pos_x, int pos_y)
{
    unsigned int vga_index;
    unsigned long reg;
    double vsc;
    int k;
    unsigned short ffolat, ivo;
    int base_v_total, range, v_offset;
    int v_total_min, v_total_max, v_step;
    float r, f;
    int vga_pixels, pre_pixels;
    float hscale, hscale_min, hscale_max;
    int hsc;
    int iho, iho_max, ihw;

    /*tv_std is valid. */
    k = map_tvstd_to_index(tv_std);

    /*store tv width and lines */
    g_specs.tv_width = tvsetup.tv_width[k];
    g_specs.tv_lines = tvsetup.tv_lines[k];

    /*determine vga mode index */
    for (vga_index = 0; vga_index < SCANTABLE_ENTRIES; vga_index++) {
        if (scantable[vga_index].mode == vga_mode)
            break;
    }
    if (vga_index >= SCANTABLE_ENTRIES)
        return;

    /*vertical scaling (v_total setup). */
    /*calculate vertical range. */
    get_vtotal_min_max(vga_mode, tv_std, &v_total_min, &v_total_max, &v_step);
    TRACE(("v_total min=%d, max=%d\n", v_total_min, v_total_max))
        base_v_total = scantable[vga_index].v_total[k];
    range = fsmax(base_v_total - v_total_min, v_total_max - base_v_total);
    TRACE(("v_total range = %d\n", range))
        /*map +/-1000 overscan y into +/-range. */
        v_offset = (int) ((((float) overscan_y * range) / 1000.f) + .5f);
    TRACE(("v_offset = %d\n", v_offset))
        /*range limit v_total. */
        g_specs.v_total =
        range_limit(base_v_total + v_offset, v_total_min, v_total_max);

    /*round to calibrated value. */
    v_offset = (g_specs.v_total - v_total_min + (v_step / 2)) / v_step;
    g_specs.v_total = v_total_min + v_offset * v_step;
    TRACE(("desired v_total=%d\n", g_specs.v_total))
        /*vertical positioning (vsync setup). */
        get_ffolat_ivo(vga_mode, tv_std, v_offset, &ffolat, &ivo);
    houston_WriteReg(HOUSTON_IVO, ivo, 2);

    /*scale base sync offset by scaling ratio. */
    r = (float) g_specs.v_total / (float) base_v_total;
    v_offset = (int) (r * (float) scantable[vga_index].v_sync[k]);

    /*scale ivo. */
    f = (float) ivo;
    v_offset -= (int) (f - f / r);

    /*compensate for center screen. */
    f = (float) tvsetup.tv_active_lines[k] / 2.f;
    v_offset += (int) (f * r - f);

    /*calculate vsync. */
    g_specs.v_sync = g_specs.v_total - v_offset + pos_y;
    TRACE(("desired v_total=%d, desired v_sync=%d\n", g_specs.v_total,
           g_specs.v_sync))
        if (g_specs.v_sync < g_specs.vga_lines + 10) {
        TRACE(("vsync too low\n"))
            /*d.v_total += d.vga_lines+10-d.v_sync; */
            g_specs.v_sync = g_specs.vga_lines + 10;
    }
    else if (g_specs.v_sync > g_specs.v_total - 10) {
        TRACE(("vsync too high\n"))
            g_specs.v_sync = g_specs.v_total - 10;
    }
    TRACE(("v_total=%d v_sync=%d\n", g_specs.v_total, g_specs.v_sync))
        /* FFOLAT. */
        houston_WriteReg(HOUSTON_FFO_LAT, ffolat, 2);

    /* VSC. */
    vsc =
        (65536.0f * (1.0f -
                     (double) g_specs.tv_lines / (double) g_specs.v_total)) +
        0.5f;
    reg = ((unsigned long) -vsc) & 0xffff;
    TRACE(("vsc=%04x, tv_lines=%d, v_total=%d\n", reg, g_specs.tv_lines,
           g_specs.v_total))
        houston_WriteReg(HOUSTON_VSC, (int) reg, 2);

    /* horizontal scaling. */

    /* vga pixels is vga width, except in 1024x768, where it's half that. */
    vga_pixels = g_specs.vga_width;
    if (1024 == vga_pixels)
        vga_pixels /= 2;

    /* maximum scaling coefficient is tv_width / vga_pixels */
    /* minimum is about 1/2, but that is quite small.  arbitrarily set 
     * minimum at 75% maximum. */
    hscale_max = (720.0f / vga_pixels);
    hscale_min = fsmax((0.75f * hscale_max), (1.0f - (63.0f / 128.0f)));
    TRACE(("hscale_min = %u.%u, hscale_max = %u.%u\n",
           (int) hscale_min,
           (int) ((hscale_min - (int) hscale_min) * 1000),
           (int) hscale_max, (int) ((hscale_max - (int) hscale_max) * 1000)))
        /* map overscan_x into min to max. */
        hscale =
        hscale_min + ((overscan_x + 1000.0f) / 2000.0f) * (hscale_max -
                                                           hscale_min);
    TRACE(("hscale = %u.%u\n", (int) hscale,
           (int) ((hscale - (int) hscale) * 1000)))
        /* determine hsc where hscale = (1 + hsc/128) */
        if (hscale >= 1.0f)
        hsc = (int) (128.f * (hscale - 1.0f) + .5f);
    else
        hsc = (int) (128.f * (hscale - 1.0f) - .5f);

    TRACE(("hsc = %d\n", hsc))
        if (hsc >= 0)
        houston_WriteReg(HOUSTON_HSC, hsc << 8, 2);
    else
        houston_WriteReg(HOUSTON_HSC, hsc & 0xFF, 2);

    /* recalculate hscale for future formulas */
    hscale = 1.0f + (hsc / 128.0f);
    TRACE(("recalculated hscale = %u.%u\n", (int) hscale,
           (int) ((hscale - (int) hscale) * 1000)))
        /* horizontal offset. */
        /* place hsync 40 before halfway from vga_width to htotal */
        /* but not less than vga_width + 10 */
        g_specs.h_sync =
        fsmax((g_specs.h_total + g_specs.vga_width) / 2 - 40,
              g_specs.vga_width + 10);
    /* also, make it even */
    g_specs.h_sync &= ~1;
    TRACE(("hsync = %u\n", g_specs.h_sync))
        /* iho range is 0 to iho_max. */
        /* iho_max is 2 * iho_center. */
        /* iho_center is pre_pixels - (tvwidth / hscale - vga pixels) / 2. */
        /* pre_pixels = (htotal - hsync) * (vga_pixels / vga_width) */
        /* note that the range is inverted also, because it specifies the number 
         * of pixels */
        /* to skip, or subtract.  iho=0 maps to farthest right. */
        /* map -pos_x = +/-1000 into (0 to iho_max) */
        pre_pixels =
        (int) ((long) (g_specs.h_total -
                       g_specs.h_sync) * vga_pixels / g_specs.vga_width);
    iho_max = (2 * pre_pixels) - ((int) (720.0f / hscale + 0.5f) - vga_pixels);
    TRACE(("iho_max = %u\n", iho_max))
        iho =
        (int) range_limit(((long) (1000 - pos_x) * iho_max / 2000) +
                          scantable[vga_index].iho[k], 0, iho_max);
    TRACE(("iho = %u\n", iho))
        houston_WriteReg(HOUSTON_IHO, iho, 2);

    /* input horizontal width. */

    /* input horizontal width is vga pixels + pre_pixels - iho */
    /* additionally, ihw cannot exceed tv width / hscale */
    /* and if hsc is negative, (ihw)(-hsc/128) cannot exceed ~250. */
    /* and ihw should be even. */
    ihw = fsmin(vga_pixels + pre_pixels - iho, (int) (720.0f / hscale));
    if (hsc < 0)
        ihw = (int) fsmin(ihw, 253L * 128 / (-hsc));
    ihw &= ~1;
    TRACE(("ihw = %u\n", ihw))
        houston_WriteReg(HOUSTON_IHA, ihw, 2);

    f = (((float) g_specs.h_total * g_specs.v_total) * 27.f) /
        ((float) g_specs.tv_width * g_specs.tv_lines);

    TRACE(("freq=%u.%uMHz\n", (int) f, (int) ((f - (int) f) * 1000)))
}

/*==========================================================================*/
/* configure houston nco.													*/
/*==========================================================================*/

static void
config_nco(unsigned long tv_std, unsigned long vga_mode)
{
    unsigned long cr, misc;
    unsigned long reg;
    int k = map_tvstd_to_index(tv_std);

    /*read and store CR. */
    houston_ReadReg(HOUSTON_CR, &cr, 2);

    /*make sure NCO_EN (enable latch) bit is clear */
    cr &= ~CR_NCO_EN;
    houston_WriteReg(HOUSTON_CR, cr, 2);

    /*clear NCO_LOADX. */
    houston_ReadReg(HOUSTON_MISC, &misc, 2);
    misc &= ~(MISC_NCO_LOAD1 + MISC_NCO_LOAD0);
    houston_WriteReg(HOUSTON_MISC, misc, 2);

    if (vga_mode == GFX_VGA_MODE_1024X768) {
        /*setup for M and N load (Nco_load=1). */
        misc |= (MISC_NCO_LOAD0);
        houston_WriteReg(HOUSTON_MISC, misc, 2);

        /*M and N. */
        houston_WriteReg(HOUSTON_NCONL, 1024 - 2, 2);
        houston_WriteReg(HOUSTON_NCODL, 128 - 1, 2);

        /*latch M/N in. */
        cr |= CR_NCO_EN;
        houston_WriteReg(HOUSTON_CR, cr, 2);
        cr &= ~CR_NCO_EN;
        houston_WriteReg(HOUSTON_CR, cr, 2);

        /*setup ncon and ncod load (Nco_load=0). */
        misc &= ~(MISC_NCO_LOAD1 + MISC_NCO_LOAD0);
        houston_WriteReg(HOUSTON_MISC, misc, 2);

        /*NCON. */
        reg = ((unsigned long) g_specs.v_total * g_specs.h_total) / 2;
        houston_WriteReg(HOUSTON_NCONH, reg >> 16, 2);
        houston_WriteReg(HOUSTON_NCONL, reg & 0xffff, 2);

        /*NCOD. */
        houston_WriteReg(HOUSTON_NCODL, tvsetup.houston_ncodl[k], 2);
        houston_WriteReg(HOUSTON_NCODH, tvsetup.houston_ncodh[k], 2);
    }
    else {
        /*setup for M and N load (Nco_load=2). */
        misc |= (MISC_NCO_LOAD1);
        houston_WriteReg(HOUSTON_MISC, misc, 2);

        /*NCON. */
        reg = (unsigned long) g_specs.v_total * g_specs.h_total;
        houston_WriteReg(HOUSTON_NCONH, reg >> 16, 2);
        houston_WriteReg(HOUSTON_NCONL, reg & 0xffff, 2);

        /*NCOD. */
        houston_WriteReg(HOUSTON_NCODL, tvsetup.houston_ncodl[k], 2);
        houston_WriteReg(HOUSTON_NCODH, tvsetup.houston_ncodh[k], 2);

        TRACE(("NCON = %lu (0x%08lx), NCOD = %lu (0x%08lx)\n",
               reg,
               reg,
               ((unsigned long) tvsetup.houston_ncodh[k] << 16) +
               tvsetup.houston_ncodl[k],
               ((unsigned long) tvsetup.houston_ncodh[k] << 16) +
               tvsetup.houston_ncodl[k]))
    }

    /*latch M/N and NCON/NCOD in. */
    cr |= CR_NCO_EN;
    houston_WriteReg(HOUSTON_CR, cr, 2);
    cr &= ~CR_NCO_EN;
    houston_WriteReg(HOUSTON_CR, cr, 2);
}

/*==========================================================================*/
/* Write sharpness settings to device										*/
/*==========================================================================*/

static void
config_sharpness(int sharpness)
{
    unsigned int shp;

    /*map 0-1000 to 0-20. */
    shp = (unsigned int) (0.5f + ((float) sharpness * 20.0f / 1000.0f));
    shp = range_limit(shp, 0, 20);

    houston_WriteReg(HOUSTON_SHP, shp, 2);
}

static void
conget_sharpness(int *p_sharpness)
{
    unsigned long shp;

    if (!p_sharpness)
        return;

    houston_ReadReg(HOUSTON_SHP, &shp, 2);

    /*map 0-20 to 0-1000. */
    *p_sharpness = (int) (0.5f + ((float) shp * 1000.0f / 20.0f));
}

/*==========================================================================*/
/* Write flicker settings to device											*/
/*==========================================================================*/

static void
config_flicker(int flicker)
{
    unsigned int flk;

    /*map 0-1000 to 0-16. */
    flk = (unsigned int) (0.5f + ((float) flicker * 16.0f / 1000.0f));
    flk = range_limit(flk, 0, 16);

    houston_WriteReg(HOUSTON_FLK, flk, 2);
}

static void
conget_flicker(int *p_flicker)
{
    unsigned long flk;

    if (!p_flicker)
        return;

    houston_ReadReg(HOUSTON_FLK, &flk, 2);

    /*map 0-16 to 0-1000. */
    *p_flicker = (int) (0.5f + ((float) flk * 1000.0f / 16.0f));
}

/*==========================================================================*/
/* Write color settings to device											*/
/*==========================================================================*/

static void
config_color(int color)
{
    unsigned long clr;

    /*map 0-100 to 0-255. */
    /*montreal production test needs 169 to be mappable, so */
    /*use .8 rounding factor, 169=(int)(66.*2.55+.8). */
    clr = (unsigned long) (0.8f + ((float) color * 255.0f / 100.0f));
    clr = range_limit(clr, 0, 255);

    houston_WriteReg(ENC_CR_GAIN, clr, 1);
    houston_WriteReg(ENC_CB_GAIN, clr, 1);
}

static void
conget_color(int *p_color)
{
    unsigned long cr_gain;

    if (!p_color)
        return;

    /*just get CR GAIN, CB GAIN should match. */
    houston_ReadReg(ENC_CR_GAIN, &cr_gain, 1);

    /*map 0-255 to 0-100. */
    *p_color = (int) (0.5f + ((float) cr_gain * 100.0f / 255.0f));
}

/*==========================================================================*/
/* Write brightness and contrast settings to device							*/
/*==========================================================================*/

#define	NTSC_BLANK_LEVEL	240

static const int min_black_level = NTSC_BLANK_LEVEL + 1;
static const int max_white_level = 1023;

static void
config_brightness_contrast(unsigned long tv_std, unsigned int trigger_bits,
                           int brightness, int contrast)
{
    int brightness_off;
    float contrast_mult;
    int black, white;
    unsigned short w;
    int k = map_tvstd_to_index(tv_std);

    /*0-100 maps to +/-220. */
    brightness_off =
        (int) (0.5f + ((float) brightness * 440.0f / 100.0f)) - 220;

    /*0-100 maps to .75-1.25. */
    contrast_mult = ((float) contrast * 0.5f / 100.0f) + 0.75f;

    black = tvsetup.black_level[k];
    if (trigger_bits != 0)
        black -= tvsetup.hamp_offset[k];

    white = tvsetup.white_level[k];
    if (trigger_bits != 0)
        white -= tvsetup.hamp_offset[k];

    black = (int) ((float) (black + brightness_off) * contrast_mult);
    white = (int) ((float) (white + brightness_off) * contrast_mult);
    if (black < min_black_level)
        black = min_black_level;
    if (white > max_white_level)
        white = max_white_level;

    w = w10bit2z((unsigned short) black);
    houston_WriteReg(ENC_BLACK_LEVEL, w & 0x00ff, 1);
    houston_WriteReg(ENC_BLACK_LEVEL + 1, w >> 8, 1);
    w = w10bit2z((unsigned short) white);
    houston_WriteReg(ENC_WHITE_LEVEL, w & 0x00ff, 1);
    houston_WriteReg(ENC_WHITE_LEVEL + 1, w >> 8, 1);
}

static void
conget_brightness_contrast(unsigned long tv_std, unsigned int trigger_bits,
                           int *p_brightness, int *p_contrast)
{
    int brightness_off;
    float contrast_mult;
    unsigned short black, white;
    unsigned long zh, zl;
    int k;

    if (!p_brightness || !p_contrast)
        return;

    k = map_tvstd_to_index(tv_std);

    houston_ReadReg(ENC_BLACK_LEVEL, &zl, 1);
    houston_ReadReg(ENC_BLACK_LEVEL + 1, &zh, 1);
    black = z2w10bit((unsigned short) (zl + (zh << 8)));
    if (trigger_bits != 0)
        black += tvsetup.hamp_offset[k];
    houston_ReadReg(ENC_WHITE_LEVEL, &zl, 1);
    houston_ReadReg(ENC_WHITE_LEVEL + 1, &zh, 1);
    white = z2w10bit((unsigned short) (zl + (zh << 8)));
    if (trigger_bits != 0)
        white += tvsetup.hamp_offset[k];

    /*this reverse computation does not account for clipping, but should */
    /*provide somewhat reasonable numbers */
    contrast_mult =
        ((float) white - (float) black) / ((float) tvsetup.white_level[k] -
                                           (float) tvsetup.black_level[k]);
    brightness_off =
        (int) (((float) black / contrast_mult) - tvsetup.black_level[k]);

    /*+/-220 maps to 0-100. */
    *p_brightness =
        range_limit((int) (0.5f + ((float) (brightness_off +
                                            220) * 100.0f / 440.0f)), 0, 100);

    /*.75-1.25 maps to 0-100. */
    *p_contrast =
        range_limit((int) (0.5f + ((float) (contrast_mult -
                                            0.75f) * 100.0f / 0.5f)), 0, 100);
}

/*==========================================================================*/
/* configure luma/chroma filters.											*/
/*==========================================================================*/

static void
config_yc_filter(unsigned long tv_std, int luma_filter, int chroma_filter)
{
    unsigned long reg, reg07, reg34;

    if (houston_Rev() < HOUSTON_REV_B)
        return;

    /*luma filter. */
    if (luma_filter)
        reg = tvsetup.notch_filter[map_tvstd_to_index(tv_std)];
    else
        reg = 0;
    houston_WriteReg(ENC_NOTCH_FILTER, reg, 1);

    /*chroma filter. */
    houston_ReadReg(ENC_REG07, &reg07, 1);
    houston_ReadReg(ENC_REG34, &reg34, 1);
    if (chroma_filter) {
        reg07 &= ~0x08;
        reg34 &= ~0x20;
    }
    else {
        reg07 |= 0x08;
        reg34 |= 0x20;
    }
    houston_WriteReg(ENC_REG07, reg07, 1);
    houston_WriteReg(ENC_REG34, reg34, 1);
}

static void
conget_yc_filter(int *p_luma_filter, int *p_chroma_filter)
{
    unsigned long reg, reg07, reg34;

    if (!p_luma_filter || !p_chroma_filter)
        return;

    if (houston_Rev() < HOUSTON_REV_B) {
        *p_luma_filter = 0;
        *p_chroma_filter = 0;
        return;
    }

    /*luma filter. */
    houston_ReadReg(ENC_NOTCH_FILTER, &reg, 1);
    *p_luma_filter = (reg ? 1 : 0);

    /*chroma filter. */
    houston_ReadReg(ENC_REG07, &reg07, 1);
    houston_ReadReg(ENC_REG34, &reg34, 1);
    *p_chroma_filter = !((0x08 & reg07) || (0x20 & reg34));
}

/*==========================================================================*/
/* Macrovision																*/
/*==========================================================================*/

static void
config_macrovision(unsigned long tv_std, unsigned int trigger_bits)
{
/*Constants to index into mvsetup columns.*/
#define	nNTSC_APS00		0       /*ntsc mv off.                   */
#define	nNTSC_APS01		1       /*ntsc AGC only.                 */
#define	nNTSC_APS10		2       /*ntsc AGC + 2-line CS.  */
#define	nNTSC_APS11		3       /*ntsc AGC + 4-line CS.  */
#define	nPAL_APS00		4       /*pal mv off.                    */
#define	nPAL_APSXX		5       /*pal mv on.                     */
#define	nMVModes		6

    /*Macrovision setup table. */
    static const struct mvparms {
        unsigned short n0[nMVModes];
        unsigned short n1[nMVModes];
        unsigned short n2[nMVModes];
        unsigned short n3[nMVModes];
        unsigned short n4[nMVModes];
        unsigned short n5[nMVModes];
        unsigned short n6[nMVModes];
        unsigned short n7[nMVModes];
        unsigned short n8[nMVModes];
        unsigned short n9[nMVModes];
        unsigned short n10[nMVModes];
        unsigned short n11[nMVModes];
        unsigned short n12[nMVModes];
        unsigned short n13[nMVModes];
        unsigned short n14[nMVModes];
        unsigned short n15[nMVModes];
        unsigned short n16[nMVModes];
        unsigned short n17[nMVModes];
        unsigned short n18[nMVModes];
        unsigned short n19[nMVModes];
        unsigned short n20[nMVModes];
        unsigned short n21[nMVModes];
        unsigned short n22[nMVModes];
        unsigned short agc_pulse_level[nMVModes];
        unsigned short bp_pulse_level[nMVModes];
    } mvsetup = {
        /*    ntsc    ntsc    ntsc    ntsc    pal             pal */
        /*    MV      AGC     AGC +   AGC +   MV              MV */
        /*    off.    only    2-line  4-line  off.    on. */
        /*    CS.             CS. */
        {
         0x00, 0x36, 0x3e, 0x3e, 0x00, 0x3e},   /*n0 */
        {
         0x1d, 0x1d, 0x1d, 0x17, 0x1a, 0x1a},   /*n1 */
        {
         0x11, 0x11, 0x11, 0x15, 0x22, 0x22},   /*n2 */
        {
         0x25, 0x25, 0x25, 0x21, 0x2a, 0x2a},   /*n3 */
        {
         0x11, 0x11, 0x11, 0x15, 0x22, 0x22},   /*n4 */
        {
         0x01, 0x01, 0x01, 0x05, 0x05, 0x05},   /*n5 */
        {
         0x07, 0x07, 0x07, 0x05, 0x02, 0x02},   /*n6 */
        {
         0x00, 0x00, 0x00, 0x02, 0x00, 0x00},   /*n7 */
        {
         0x1b, 0x1b, 0x1b, 0x1b, 0x1c, 0x1c},   /*n8 */
        {
         0x1b, 0x1b, 0x1b, 0x1b, 0x3d, 0x3d},   /*n9 */
        {
         0x24, 0x24, 0x24, 0x24, 0x14, 0x14},   /*n10 */
        {
         0x780f, 0x780f, 0x780f, 0x780f, 0x7e07, 0x7e07},       /*n11 */
        {
         0x0000, 0x0000, 0x0000, 0x0000, 0x5402, 0x5402},       /*n12 */
        {
         0x0f, 0x0f, 0x0f, 0x0f, 0xfe, 0xfe},   /*n13 */
        {
         0x0f, 0x0f, 0x0f, 0x0f, 0x7e, 0x7e},   /*n14 */
        {
         0x60, 0x60, 0x60, 0x60, 0x60, 0x60},   /*n15 */
        {
         0x01, 0x01, 0x01, 0x01, 0x00, 0x00},   /*n16 */
        {
         0x0a, 0x0a, 0x0a, 0x0a, 0x08, 0x08},   /*n17 */
        {
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   /*n18 */
        {
         0x05, 0x05, 0x05, 0x05, 0x04, 0x04},   /*n19 */
        {
         0x04, 0x04, 0x04, 0x04, 0x07, 0x07},   /*n20 */
        {
         0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x0155, 0x0155},       /*n21 */
        {
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   /*n22 */
        {
         0xa3, 0xa3, 0xa3, 0xa3, 0xa3, 0xa3},   /*agc_pulse_level */
        {
         0xc8, 0xc8, 0xc8, 0xc8, 0xc8, 0xc8},   /*bp_pulse_level */
    };

    int nMode;
    unsigned long misc;
    unsigned short n0;

    trigger_bits &= 0x3;

    /*Determine the OEM Macrovision Program Mode and Register 0 Data. */
    if (IS_NTSC(tv_std)) {
        /*NTSC TV Standard. */
        if (trigger_bits == 0) {
            /*turn Macrovision OFF. */
            nMode = nNTSC_APS00;
        }
        else if (trigger_bits == 1) {
            /*AGC Only. */
            nMode = nNTSC_APS01;
        }
        else if (trigger_bits == 2) {
            /*AGC + 2-line CS. */
            nMode = nNTSC_APS10;
        }
        else {
            /*AGC + 4-line CS. */
            nMode = nNTSC_APS11;
        }
    }
    else {
        /*PAL TV Standard. */
        if (trigger_bits == 0) {
            /*turn Macrovision OFF. */
            nMode = nPAL_APS00;
        }
        else {
            /*APS 01, 10, or 11. */
            nMode = nPAL_APSXX;
        }
    }

    /*Retrieve the Macrovision Program Mode Data */
    if (tv_std != GFX_TV_STANDARD_PAL_M)
        n0 = mvsetup.n0[nMode];
    else {
        /*PAL-M sets up like NTSC except for n0. */
        if ((trigger_bits & 0x03) == 0)
            n0 = mvsetup.n0[nPAL_APS00];
        else
            n0 = mvsetup.n0[nPAL_APSXX];
    }

    /*download settings now. */
    houston_WriteReg(MV_N0, n0, 1);
    houston_WriteReg(MV_N1, mvsetup.n1[nMode], 1);
    houston_WriteReg(MV_N2, mvsetup.n2[nMode], 1);
    houston_WriteReg(MV_N3, mvsetup.n3[nMode], 1);
    houston_WriteReg(MV_N4, mvsetup.n4[nMode], 1);
    houston_WriteReg(MV_N5, mvsetup.n5[nMode], 1);
    houston_WriteReg(MV_N6, mvsetup.n6[nMode], 1);
    houston_WriteReg(MV_N7, mvsetup.n7[nMode], 1);
    houston_WriteReg(MV_N8, mvsetup.n8[nMode], 1);
    houston_WriteReg(MV_N9, mvsetup.n9[nMode], 1);
    houston_WriteReg(MV_N10, mvsetup.n10[nMode], 1);
    houston_WriteReg(MV_N11, mvsetup.n11[nMode] & 0xff, 1);
    houston_WriteReg(MV_N11 + 1, mvsetup.n11[nMode] >> 8, 1);
    houston_WriteReg(MV_N12, mvsetup.n12[nMode] & 0xff, 1);
    houston_WriteReg(MV_N12 + 1, mvsetup.n12[nMode] >> 8, 1);
    houston_WriteReg(MV_N13, mvsetup.n13[nMode], 1);
    houston_WriteReg(MV_N14, mvsetup.n14[nMode], 1);
    houston_WriteReg(MV_N15, mvsetup.n15[nMode], 1);
    houston_WriteReg(MV_N16, mvsetup.n16[nMode], 1);
    houston_WriteReg(MV_N17, mvsetup.n17[nMode], 1);
    houston_WriteReg(MV_N18, mvsetup.n18[nMode], 1);
    houston_WriteReg(MV_N19, mvsetup.n19[nMode], 1);
    houston_WriteReg(MV_N20, mvsetup.n20[nMode], 1);
    houston_WriteReg(MV_N21, mvsetup.n21[nMode] & 0xff, 1);
    houston_WriteReg(MV_N21 + 1, mvsetup.n21[nMode] >> 8, 1);
    houston_WriteReg(MV_N22, mvsetup.n22[nMode], 1);
    houston_WriteReg(MV_AGC_PULSE_LEVEL, mvsetup.agc_pulse_level[nMode], 1);
    houston_WriteReg(MV_BP_PULSE_LEVEL, mvsetup.bp_pulse_level[nMode], 1);

    houston_ReadReg(HOUSTON_MISC, &misc, 2);
    if (trigger_bits == 0)
        misc &= ~MISC_MV_SOFT_EN;
    else
        misc |= MISC_MV_SOFT_EN;
    houston_WriteReg(HOUSTON_MISC, misc, 2);
}

static void
conget_macrovision(unsigned long tv_std, unsigned int *p_cp_trigger_bits)
{
    unsigned long n0, n1;

    if (!p_cp_trigger_bits)
        return;

    houston_ReadReg(MV_N0, &n0, 1);
    houston_ReadReg(MV_N1, &n1, 1);

    *p_cp_trigger_bits = 0;

    if (IS_NTSC(tv_std)) {
        switch (n0) {
        case 0:
            *p_cp_trigger_bits = 0;
            break;

        case 0x36:
            *p_cp_trigger_bits = 1;
            break;

        case 0x3E:
        {
            if (0x1D == n1)
                *p_cp_trigger_bits = 2;
            else
                *p_cp_trigger_bits = 3;
        }
            break;
        }
    }
    else if (IS_PAL(tv_std)) {
        if (0 == n0)
            *p_cp_trigger_bits = 0;
        else {
            /*don't know here what the non-zero trigger bits were */
            *p_cp_trigger_bits = 1;
        }
    }
}

/* PLAL_MediaGX.cpp															*/
/*==========================================================================*/
/* These functions provides implementation of platform-specific functions	*/
/* MediaGX platform.														*/
/*==========================================================================*/

/*MediaGX control registers.*/
#define CCR3	0xC3
#define	GCR		0xb8

/*Media GX general config register.*/
#define	GX_DCLK_MUL			0x00c0
#define	GX_DCLKx1			0x0040
#define	GX_DCLKx2			0x0080
#define GX_DCLKx4			0x00c0

/*Media GX timing config register.*/
#define	GX_TGEN				0x0020

/*Cx5530 register offsets (from GX_BASE).*/
#define	CX_DISPLAY_CONFIG	0x10004
#define	CX_DOT_CLK			0x10024
#define	CX_TV_CONFIG		0x10028

/*Cx5530 display configuration register.*/
#define	CX_FPVSYNC_POL		0x0800
#define	CX_FPHSYNC_POL		0x0400
#define	CX_FPDATA_ENB		0x0080
#define	CX_FPPOWER_ENB		0x0040
#define CX_CRTVSYNC_POL		0x0200
#define CX_CRTHSYNC_POL		0x0100

/*Cx5530 dot clock configuration register.*/
#define	CX_TVCLK_SELECT		0x0400

/*Cx5530 tv configuration register*/
#define CX_INVERT_FPCLK (1 << 6)

/*==========================================================================
 *	FS450 I2C Address
 *	There are two possible 7-bit addresses, 0x4A and 0x6A.
 *	The address if selectable via pins on the FS450.
 *	There are also two possible 10-bit addresses, 0x224 and 0x276, but this
 *	source is not designed to use them.
 *==========================================================================*/

#define	FS450_I2C_ADDRESS (0x4A)

static unsigned char
PLAL_FS450_i2c_address(void)
{
    return FS450_I2C_ADDRESS;
}

/*==========================================================================
 *	FS450 UIM mode
 *	This mode is programmed in the FS450 command register when enabling TV
 *	out.
 *==========================================================================
 */
static int
PLAL_FS450_UIM_mode(void)
{
    return 3;
}

/*==========================================================================*/
/* Read and Write MediaGX registers											*/
/*==========================================================================*/
static unsigned long
ReadGx(unsigned long inRegAddr)
{
    unsigned long data;

    DMAL_ReadUInt32(inRegAddr, &data);

    return data;
}

static void
WriteGx(unsigned long inRegAddr, unsigned long inData)
{
    int is_timing_register;
    unsigned long reg_timing_cfg;

    /*because the unlock register for the MediaGx video registers may not */
    /*persist, we will write the unlock code before every write. */
    DMAL_WriteUInt32(DC_UNLOCK, 0x4758);

    /*see if register is a timing register */
    is_timing_register =
        (DC_H_TIMING_1 == inRegAddr) ||
        (DC_H_TIMING_2 == inRegAddr) ||
        (DC_H_TIMING_3 == inRegAddr) ||
        (DC_FP_H_TIMING == inRegAddr) ||
        (DC_V_TIMING_1 == inRegAddr) ||
        (DC_V_TIMING_2 == inRegAddr) ||
        (DC_V_TIMING_3 == inRegAddr) || (DC_FP_V_TIMING == inRegAddr);

    /*if the register is a timing register, clear the TGEN bit to allow 
     * modification */
    if (is_timing_register) {
        DMAL_ReadUInt32(DC_TIMING_CFG, &reg_timing_cfg);
        DMAL_WriteUInt32(DC_TIMING_CFG, reg_timing_cfg & ~GX_TGEN);
    }

    /*write the requested register */
    DMAL_WriteUInt32(inRegAddr, inData);

    /*reset the TGEN bit to previous state */
    if (is_timing_register) {
        DMAL_WriteUInt32(DC_TIMING_CFG, reg_timing_cfg);
    }
}

#ifdef FS450_DIRECTREG

/*==========================================================================*/
/*	Platform-specific processing for a Read or Write Register calls.		*/
/*	The functions should return true if the specified register belongs to	*/
/*	this platform.															*/
/*==========================================================================*/

static int
PLAL_ReadRegister(S_REG_INFO *p_reg)
{
    if (!p_reg)
        return 0;

    if (SOURCE_GCC == p_reg->source) {
        p_reg->value = ReadGx(p_reg->offset);

        return 1;
    }

    return 0;
}

static int
PLAL_WriteRegister(const S_REG_INFO *p_reg)
{
    if (!p_reg)
        return 0;

    if (SOURCE_GCC == p_reg->source) {
        WriteGx(p_reg->offset, p_reg->value);

        return 1;
    }

    return 0;
}

#endif

/*==========================================================================
 *	Determine if TV is on
 *==========================================================================*/
static int
PLAL_IsTVOn(void)
{
    unsigned long reg;

    /*check Cx5530 dot clock */
    reg = ReadGx(CX_DOT_CLK);
    return (reg & CX_TVCLK_SELECT) ? 1 : 0;
}

/*==========================================================================
 * Platform-specific actions to reset to VGA mode
 *==========================================================================*/

static int
PLAL_EnableVga(void)
{
    unsigned long reg;

    /*2 x dclk */
    reg = ReadGx(DC_GENERAL_CFG);
    reg &= ~GX_DCLK_MUL;
    reg |= GX_DCLKx2;
    WriteGx(DC_GENERAL_CFG, reg);

    /*select pll dot clock. */
    reg = ReadGx(CX_DOT_CLK);
    reg &= ~CX_TVCLK_SELECT;
    WriteGx(CX_DOT_CLK, reg);

    /*timing config, reset everything on dclk. */
    reg = ReadGx(DC_TIMING_CFG);
    reg &= ~GX_TGEN;
    WriteGx(DC_TIMING_CFG, reg);
    reg |= GX_TGEN;
    WriteGx(DC_TIMING_CFG, reg);

    /*un-invert FP clock */
    reg = ReadGx(CX_TV_CONFIG);
    reg &= ~CX_INVERT_FPCLK;
    WriteGx(CX_TV_CONFIG, reg);

    return 0;
}

/*==========================================================================*/
/*Platform-specific actions to enter TVout mode								*/
/*==========================================================================*/

static int
PLAL_PrepForTVout(void)
{
    unsigned int reg;

    /*Cx5530 tv config. */
    reg = 0;
    WriteGx(CX_TV_CONFIG, reg);

    /*invert FP clock */
    reg = (int) ReadGx(CX_TV_CONFIG);
    reg |= CX_INVERT_FPCLK;
    WriteGx(CX_TV_CONFIG, reg);

    return 0;
}

static int
PLAL_SetTVTimingRegisters(const S_TIMING_SPECS *p_specs)
{
    unsigned long reg;

    /*timing config, reset everything on dclk. */
    reg = ReadGx(DC_TIMING_CFG);
    reg &= ~GX_TGEN;
    WriteGx(DC_TIMING_CFG, reg);

    /*htotal and hactive. */
    reg = ((p_specs->h_total - 1) << 16) | (p_specs->vga_width - 1);
    WriteGx(DC_H_TIMING_1, reg);

    /*hblank. */
    reg = ((p_specs->h_total - 1) << 16) | (p_specs->vga_width - 1);
    WriteGx(DC_H_TIMING_2, reg);

    /*hsync. */
    reg = ((p_specs->h_sync + 63) << 16) | p_specs->h_sync;
    WriteGx(DC_H_TIMING_3, reg);

    /*fp hsync. */
    WriteGx(DC_FP_H_TIMING, reg);

    /*vtotal and vactive. */
    reg = ((p_specs->v_total - 1) << 16) | (p_specs->vga_lines - 1);
    WriteGx(DC_V_TIMING_1, reg);

    /*vblank. */
    reg = ((p_specs->v_total - 1) << 16) | (p_specs->vga_lines - 1);
    WriteGx(DC_V_TIMING_2, reg);

    /*vsync. */
    reg = ((p_specs->v_sync) << 16) | (p_specs->v_sync - 1);
    WriteGx(DC_V_TIMING_3, reg);

    /*fp vsync. */
    reg = ((p_specs->v_sync - 1) << 16) | (p_specs->v_sync - 2);
    WriteGx(DC_FP_V_TIMING, reg);

    /*timing config, re-enable all dclk stuff. */
    reg = ReadGx(DC_TIMING_CFG);
    reg |= GX_TGEN;
    WriteGx(DC_TIMING_CFG, reg);

    return 0;
}

static int
PLAL_FinalEnableTVout(unsigned long vga_mode)
{
    unsigned int reg;

    /*Cx5530 select tv dot clock. */
    reg = (int) ReadGx(CX_DOT_CLK);
    reg |= CX_TVCLK_SELECT;
    WriteGx(CX_DOT_CLK, reg);

    /*2 x dclk (actually 1x) */
    reg = (int) ReadGx(DC_GENERAL_CFG);
    reg &= ~GX_DCLK_MUL;
    WriteGx(DC_GENERAL_CFG, reg);

    reg |= GX_DCLKx2;
    WriteGx(DC_GENERAL_CFG, reg);

    /*Cx5530 display configuration register. */
    reg = (int) ReadGx(CX_DISPLAY_CONFIG);
    reg |= (CX_FPVSYNC_POL | CX_FPHSYNC_POL | CX_FPDATA_ENB | CX_FPPOWER_ENB);
    WriteGx(CX_DISPLAY_CONFIG, reg);

    return 0;
}
