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
 * File Contents:   This file contains the panel library files to the 
 *                  GX2 platforms with 9211 support.
 * 
 * SubModule:       Geode FlatPanel library
 * */

#include "92xx.h"
#include "gx2_9211.h"
#include "pnl_defs.h"

#if defined(_WIN32)             /* windows */
#include "gfx_defs.h"

extern DEV_STATUS gfx_msr_read(unsigned int device, unsigned int msrRegister,
                               Q_WORD * msrValue);
extern DEV_STATUS gfx_msr_write(unsigned int device, unsigned int msrRegister,
                                Q_WORD * msrValue);
#endif

static unsigned long FPBaseAddr;

void
SetFPBaseAddr(unsigned long addr)
{

    FPBaseAddr = addr;
}

/****************************************************************************
 * protected_mode_access( unsigned long mode, unsigned long width, 
 * 		unsigned long addr, unsigned char* pdata )
 * This function provides access to physical memory at the requested address.
 *
 * mode is: 
 *        GX2_READ or GX2_WRITE (accesses a single byte, word or double word
 *        	depending on the value of "width".  Only 1, 2 or 4 supported).
 *        READ_BYTES, WRITE_BYTES accesses "width" number of bytes (8 bits)
 *        READ_WORDS, WRITE_WORDS accesses "width" number of words (16 bits)
 *        READ_DWORDS, WRITE_DWORDS accesses "width" number of dwords (32
 *        	bits)
 *        	
 * width is: 
 * 		  The size of the access.  For READ or WRITE, only 1, 2 and 4 are
 *        	supported.  For other modes, width is not limited but will cause 
 *        	paging if the block traverses page boundaries.  
 * 
 * addr is: 
 * 		  The physical address being accessed
 * 
 * pdata is: 
 * 		  A pointer to the data to be read or written into.
 *
 * NOTE! WORD or DWORD accesses can only be made on WORD or DWORD boundaries!
 ****************************************************************************/
void
protected_mode_access(unsigned long mode,
                      unsigned long width, unsigned long addr, char *pdata)
{
    void *ptr = (void *) (FPBaseAddr + addr);

    /* type specific buffer pointers */
    char *byte_data = (char *) pdata;
    unsigned long *word_data = (unsigned long *) pdata;
    unsigned long *dword_data = (unsigned long *) pdata;

    if (mode == GX2_READ) {
        switch (width) {
        case FOUR_BYTES:
            *(dword_data) = (unsigned long) (*(unsigned long *) ptr);
            break;
        case TWO_BYTES:
            *(word_data) = (unsigned long) (*(unsigned long *) ptr);
            break;
        default:
            *(byte_data) = (char) (*(char *) ptr);
            break;
        }
    }                           /* end  GX2_READ */
    else if (mode == GX2_WRITE) {
        switch (width) {
        case FOUR_BYTES:
            *(unsigned long *) ptr = *dword_data;
            break;
        case TWO_BYTES:
            *(unsigned long *) ptr = *word_data;
            break;
        default:
            *(char *) ptr = *byte_data;
            break;
        }                       /* end switch(mode) */
    }
    /* end case GX2_WRITE */
    return;

}                               /* End of protected_mode_access. */

/*************************************************************************
 * void write_video_reg64_low( unsigned long offset, unsigned long value )
 *
 * Writes value to the low 32 bits of the 64 bit memory mapped video 
 * register indicated by offset.
 * This function uses Sys_info.video_reg_base as the base address, so
 * the value of offset should be with respect to this base.
 *************************************************************************/
void
write_video_reg64_low(unsigned long offset, unsigned long value)
{
    protected_mode_access(GX2_WRITE, FOUR_BYTES,
                          FPBaseAddr + offset, (char *) &value);
}                               /*end write_video_reg64_low() */

/*************************************************************************
 * unsigned long read_video_reg64_low( unsigned long offset )
 *
 * Returns the contents of the low 32 bits of the 64 bit memory mapped
 * video register indicated by offset.
 * This function uses Sys_info.video_reg_base as the base address, so
 * the value of offset should be with respect to this base.
 *************************************************************************/
unsigned long
read_video_reg64_low(unsigned long offset)
{
    unsigned long data;

    protected_mode_access(GX2_READ, FOUR_BYTES,
                          FPBaseAddr + offset, (char *) &data);
    return (data);
}                               /*end read_video_reg64_low() */

/*****************************************************************************
 * void Redcloud_fp_reg(int mode, unsigned long address, unsigned long *data)
 *
 * Writes and reads dwords to the Redcloud flat panel registers in the 
 * Redcloud Display Filter.  There's no clock control, chip select or timing 
 * to deal with.
 * This routine expects the actual GX2 macro definitions for the address.
 *
 * Parameters:
 *			mode:		An integer value for a GX2_READ or GX2_WRITE operation
 *						0 = GX2_Read and 1 = GX2_Write
 *			address:	A dword value representing the offset of the register.
 *			data:		A pointer to a dword value that is to be written in to
 *						the required register.  In case of a Read operation 
 *						this will point to the result of the Read operation.
 ****************************************************************************/
void
Redcloud_fp_reg(int mode, unsigned long address, unsigned long *data)
{
    if (mode == GX2_READ) {
        *data = read_video_reg64_low(address);
    }
    else {
        write_video_reg64_low(address, *data);
    }

}                               /* End of Redcloud_fp_reg() */

/*-------------------------------------------------------------------
 *
 * SET_92XX_MODE_PARAMS
 * This routine sets the 9211 mode parameters.  
 *
 *-------------------------------------------------------------------*/

void
set_Redcloud_92xx_mode_params(int mode)
{
    CS92xx_MODE *pMode = &FPModeParams[mode];
    unsigned long temp_data = 0;
    unsigned long base_data;
    QQ_WORD msrValue;

    /* on a Redcloud, we need to set up the DF pad select MSR */
    if (gfx_msr_read(RC_ID_DF, GX2_VP_MSR_PAD_SELECT, &msrValue) == FOUND) {
        msrValue.low &= ~GX2_VP_PAD_SELECT_MASK;
        if (pMode->panel_type == PNL_TFT || pMode->panel_type == PNL_TWOP) {
            msrValue.low = GX2_VP_PAD_SELECT_TFT;
        }
        else {
            msrValue.low = GX2_VP_PAD_SELECT_DSTN;
        }
        gfx_msr_write(RC_ID_DF, GX2_VP_MSR_PAD_SELECT, &msrValue);
    }

    /* Turn the 92xx power off before setting any new parameters. */
    temp_data = pMode->power_management & ~GX2_FP_PM_PWR_ON;
    Redcloud_fp_reg(GX2_WRITE, GX2_FP_PWR_MAN, (unsigned long *) &temp_data);

    /* Set 9211 registers using the desired panel settings */

    Redcloud_fp_reg(GX2_WRITE, GX2_FP_PAN_TIMING1,
                    (unsigned long *) &pMode->panel_timing1);

    /* On Redcloud, bit 31 is now reserved. */
    temp_data = pMode->panel_timing2 & 0x7FFFFFFF;
    Redcloud_fp_reg(GX2_WRITE, GX2_FP_PAN_TIMING2,
                    (unsigned long *) &temp_data);

    /* On Redcloud TFT parts, set this to 0x70 so all 8 bits per color run 
     * thru fp crc but only non-TFT parts.  Otherwise, set it to be 0x50. 
     * (source: Larry G.). 
     */
    if (pMode->panel_type == PNL_TFT || pMode->panel_type == PNL_TWOP) {
        temp_data = GX2_FP_CRC_PASS_THRU_MASK;
    }
    else {
        temp_data = pMode->rev_C_dither_frc;
    }
    Redcloud_fp_reg(GX2_WRITE, GX2_FP_DITH_FR_CNTRL,
                    (unsigned long *) &temp_data);
    Redcloud_fp_reg(GX2_WRITE, GX2_FP_BLFSR,
                    (unsigned long *) &pMode->blue_lsfr_seed);
    Redcloud_fp_reg(GX2_WRITE, GX2_FP_RLFSR,
                    (unsigned long *) &pMode->red_green_lsfr_seed);

    /* Set the memory information, then the power register last. 
     * This will turn the panel on at the 9211.
     */

    Redcloud_fp_reg(GX2_READ, GX2_FP_FBB, (unsigned long *) &base_data);
    if (base_data != 0x41780000) {
        base_data = 0x41780000;
        Redcloud_fp_reg(GX2_WRITE, GX2_FP_FBB, (unsigned long *) &base_data);
    }

    Redcloud_fp_reg(GX2_WRITE, GX2_FP_PWR_MAN,
                    (unsigned long *) &pMode->power_management);

}                               /*end set_92xx_mode_params() */

/* -----------------------------------------------------------------------
 * SET_FLAT_PANEL_MODE
 *
 * This routine sets the specified flat panel moden parameters in 
 * the 9211.
 * Returns PASS if successful, FAIL if the mode parameters could 
 * not be set.
 *------------------------------------------------------------------------*/

unsigned char
set_Redcloud_92xx_mode(Pnl_PanelStat *pstat)
{
    int mode;

    /* LOOP THROUGH THE AVAILABLE MODES TO FIND A MATCH */

    for (mode = 0; mode < NUM_92XX_MODES; mode++) {
        if ((FPModeParams[mode].xres == pstat->XRes) &&
            (FPModeParams[mode].yres == pstat->YRes) &&
            (FPModeParams[mode].bpp == pstat->Depth) &&
            (FPModeParams[mode].panel_type == pstat->Type) &&
            (FPModeParams[mode].color_type == pstat->MonoColor)) {

            /* SET THE 92xx FOR THE SELECTED MODE */
            set_Redcloud_92xx_mode_params(mode);
            return TRUE;
        }                       /* end if() */
    }                           /* end for() */
    return FALSE;

}                               /* end set_Centaurus_92xx_mode() */

void
Redcloud_9211init(Pnl_PanelStat *pstat)
{

    set_Redcloud_92xx_mode(pstat);

}
