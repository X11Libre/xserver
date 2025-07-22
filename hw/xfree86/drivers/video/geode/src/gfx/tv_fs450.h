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
 * This file defines the common FS450 API.
 * */

#ifndef __FS450_H__
#define __FS450_H__

#ifdef __cplusplus
extern "C" {
#endif

/*==========================================================================*/
/*	Init and cleanup functions*/

    int FS450_init(void);
    void FS450_cleanup(void);
    /* call FS450_init at startup to probe for and initialize FS450. */
    /* returns 0 if successful. */

/*==========================================================================*/
/*	TV output on or off*/

    int FS450_get_tv_enable(unsigned int *p_on);
    int FS450_set_tv_enable(unsigned int on);
    /* on is 1 for TV on, 0 for off */

/*==========================================================================*/
/*	TV standard*/

    int FS450_get_tv_standard(unsigned long *p_standard);
    int FS450_get_available_tv_standards(unsigned long *p_standards);
    int FS450_set_tv_standard(unsigned long standard);
    /* standard is one of the FS450_TV_STANDARD constants */
    /* standards is a bitmask of zero or more FS450_TV_STANDARD constants */

/* FS450 TV Standard flags*/
#define FS450_TV_STANDARD_NTSC_M 0x0001
#define FS450_TV_STANDARD_NTSC_M_J 0x0002
#define FS450_TV_STANDARD_PAL_B 0x0004
#define FS450_TV_STANDARD_PAL_D 0x0008
#define FS450_TV_STANDARD_PAL_H 0x0010
#define FS450_TV_STANDARD_PAL_I 0x0020
#define FS450_TV_STANDARD_PAL_M 0x0040
#define FS450_TV_STANDARD_PAL_N 0x0080
#define FS450_TV_STANDARD_PAL_G 0x0100

/*==========================================================================*/
/*	VGA mode assumed by FS450*/

    int FS450_get_vga_mode(unsigned long *p_vga_mode);
    int FS450_get_available_vga_modes(unsigned long *p_vga_modes);
    int FS450_set_vga_mode(unsigned long vga_mode);
    /* vga_mode is one of the FS450_VGA_MODE constants */
    /* vga_modes is a bitmask of zero or more FS450_VGA_MODE constants */

/* FS450 VGA Mode flags*/
#define FS450_VGA_MODE_UNKNOWN 0
#define FS450_VGA_MODE_640X480 0x0001
#define FS450_VGA_MODE_720X487 0x0002
#define FS450_VGA_MODE_720X576 0x0004
#define FS450_VGA_MODE_800X600 0x0008
#define FS450_VGA_MODE_1024X768 0x0010

/*==========================================================================*/
/*	TVout mode*/

    int FS450_get_tvout_mode(unsigned long *p_tvout_mode);
    int FS450_set_tvout_mode(unsigned long tvout_mode);
    /* tvout_mode is a bitmask of FS450_TVOUT_MODE constants */

/* FS450 TVout mode flags*/
#define FS450_TVOUT_MODE_CVBS 0x0001
#define FS450_TVOUT_MODE_YC 0x0002
#define FS450_TVOUT_MODE_RGB 0x0004
#define FS450_TVOUT_MODE_CVBS_YC (FS450_TVOUT_MODE_CVBS | FS450_TVOUT_MODE_YC)

/*==========================================================================*/
/*	Flicker control*/

    int FS450_get_sharpness(int *p_sharpness);
    int FS450_set_sharpness(int sharpness);
    /* sharpness is a percentage in tenths of a percent, 0 to 1000 */

    int FS450_get_flicker_filter(int *p_flicker);
    int FS450_set_flicker_filter(int flicker);
    /* flicker_filter is a percentage in tenths of a percent, 0 to 1000 */

/*==========================================================================*/
/*	Size and Position*/

    int FS450_get_overscan(int *p_x, int *p_y);
    int FS450_set_overscan(int x, int y);
    int FS450_get_position(int *p_x, int *p_y);
    int FS450_set_position(int x, int y);
    /* x and y are horizontal and vertical adjustments, -1000 to +1000 */

/*==========================================================================*/
/*	Visual adjustments*/

    int FS450_get_color(int *p_color);
    int FS450_set_color(int color);
    /* color is a percentage, 0 to 100 */

    int FS450_get_brightness(int *p_brightness);
    int FS450_set_brightness(int brightness);
    /* brightness is a percentage, 0 to 100 */

    int FS450_get_contrast(int *p_contrast);
    int FS450_set_contrast(int contrast);
    /* contrast is a percentage, 0 to 100 */

/*==========================================================================*/
/*	Luma and Chroma filter*/

    int FS450_get_yc_filter(unsigned int *p_yc_filter);
    int FS450_set_yc_filter(unsigned int yc_filter);
    /* yc_filter is a bitmask of FS450_LUMA_FILTER and/or FS450_CHROMA_FILTER */

/* FS450 Luma and Chroma Filters*/
#define FS450_LUMA_FILTER 0x0001
#define FS450_CHROMA_FILTER 0x0002

/*==========================================================================*/
/*	Macrovision*/

    int FS450_get_aps_trigger_bits(unsigned int *p_trigger_bits);
    int FS450_set_aps_trigger_bits(unsigned int trigger_bits);
    /* trigger_bits is one of the FS450_APS_TRIGGER constants */

/* APS Trigger Bits*/
#define FS450_APS_TRIGGER_OFF 			0
#define FS450_APS_TRIGGER_AGC_ONLY 		1
#define FS450_APS_TRIGGER_AGC_2_LINE 	2
#define FS450_APS_TRIGGER_AGC_4_LINE 	3

/*==========================================================================*/
/*	direct access to Houston and platform registers (debug builds only)
 *	The two functions FS450_ReadRegister and FS450_WriteRegister allow access
 *	to device registers.  These functions are intended for debugging purposes
 *	only and should not be included in a shipping product.
 *	*/

#ifdef FS450_DIRECTREG

#define SOURCE_HOUSTON 0
#define SOURCE_GCC 1

    typedef struct _S_REG_INFO {
        int source;
        unsigned int size;
        unsigned long offset;
        unsigned long value;
    } S_REG_INFO;

    int FS450_ReadRegister(S_REG_INFO * p_reg);
    int FS450_WriteRegister(S_REG_INFO * p_reg);

#endif

/*==========================================================================*/
/* Error Codes*/

#define ERR_INVALID_PARAMETER			0x1000
#define ERR_NOT_SUPPORTED				0x1001
#define ERR_CANNOT_CHANGE_WHILE_TV_ON	0x1002

#define ERR_DRIVER_NOT_FOUND			0x1100
#define ERR_DRIVER_ERROR				0x1101
#define ERR_DEVICE_NOT_FOUND			0x1120

#define ERR_I2C_MISSING_DEVICE			0x1200
#define ERR_I2C_WRITE_FAILED			0x1201
#define ERR_I2C_READ_FAILED				0x1202

#ifdef __cplusplus
}
#endif
#endif
