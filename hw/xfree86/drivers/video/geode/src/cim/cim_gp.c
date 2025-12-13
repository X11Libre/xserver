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
  * Cimarron graphics processor routines.  These routines program the graphics
  * hardware using the graphics command buffer.
  */

/*---------------------*/
/* CIMARRON GP GLOBALS */
/*---------------------*/

CIMARRON_STATIC unsigned long gp3_bpp = 0;
CIMARRON_STATIC unsigned long gp3_ch3_bpp = 0;
CIMARRON_STATIC unsigned long gp3_pat_origin = 0;
CIMARRON_STATIC unsigned long gp3_buffer_lead = 0;
CIMARRON_STATIC unsigned long gp3_cmd_header;
CIMARRON_STATIC unsigned long gp3_cmd_top;
CIMARRON_STATIC unsigned long gp3_cmd_bottom;
CIMARRON_STATIC unsigned long gp3_cmd_current;
CIMARRON_STATIC unsigned long gp3_cmd_next;
CIMARRON_STATIC unsigned long gp3_blt_mode;
CIMARRON_STATIC unsigned long gp3_vec_mode;
CIMARRON_STATIC unsigned long gp3_raster_mode;
CIMARRON_STATIC unsigned long gp3_pix_shift;
CIMARRON_STATIC unsigned long gp3_ch3_pat;
CIMARRON_STATIC unsigned long gp3_blt;
CIMARRON_STATIC unsigned long gp3_blt_flags;
CIMARRON_STATIC unsigned long gp3_src_stride;
CIMARRON_STATIC unsigned long gp3_dst_stride;
CIMARRON_STATIC unsigned long gp3_src_format;
CIMARRON_STATIC unsigned long gp3_src_pix_shift;
CIMARRON_STATIC unsigned long gp3_pat_format;
CIMARRON_STATIC unsigned long gp3_pat_pix_shift;
CIMARRON_STATIC unsigned long gp3_fb_base;
CIMARRON_STATIC unsigned long gp3_vector_pattern_color;
CIMARRON_STATIC unsigned long gp3_scratch_base;
CIMARRON_STATIC unsigned long gp3_base_register;
CIMARRON_STATIC unsigned long gp3_vec_pat;

/*---------------------------------------------------------------------------
 * gp_set_limit_on_buffer_lead
 *
 * This routine is used to specify the maximum number of bytes in the command
 * buffer by which software can lead the graphics processor.  When declaring
 * a BLT with the CIMGP_BLTFLAGS_LIMITBUFFER flag set, Cimarron will wait
 * until the command buffer read and write pointers differ by no more than
 * 'lead' bytes.  This can be useful to limit the time lag possible when
 * creating a command buffer full of simple BLT commands.
 *-------------------------------------------------------------------------*/

void
gp_set_limit_on_buffer_lead(unsigned long lead)
{
    gp3_buffer_lead = lead;
}

/*---------------------------------------------------------------------------
 * gp_set_command_buffer_base
 *
 * This routine is used to program the command buffer region in physical
 * memory.  The command buffer start address must be 1MB aligned. start and
 * stop refer to endpoints within the associated 16MB region.  Command buffers
 * larger than 16MB are not supported.
 *-------------------------------------------------------------------------*/

void
gp_set_command_buffer_base(unsigned long address, unsigned long start,
                           unsigned long stop)
{
    Q_WORD msr_value = { 0, 0 };

    /* WAIT FOR IDLE */
    /* Obviously, we cannot change the command buffer pointer while the GP */
    /* is currently fetching commands.                                     */

    gp_wait_until_idle();

    /* WRITE THE COMMAND BUFFER BASE */

    msr_read64(MSR_DEVICE_GEODELX_GP, MSR_GEODELINK_CONFIG, &msr_value);
    msr_value.low &= 0xF000FFFF;
    msr_value.low |= (address >> 4) & 0x0FFF0000;
    msr_write64(MSR_DEVICE_GEODELX_GP, MSR_GEODELINK_CONFIG, &msr_value);

    /* WRITE THE BASE OFFSETS */
    /* We also reset the write and read pointers.  The hardware will */
    /* automatically update the write pointer when the read pointer  */
    /* is updated to prevent the hardware from getting confused when */
    /* initializing a new command buffer.                            */

    WRITE_GP32(GP3_CMD_TOP, start);
    WRITE_GP32(GP3_CMD_BOT, stop);
    WRITE_GP32(GP3_CMD_READ, start);

    /* SAVE THE BASE ADDRESSES */
    /* These are used to determine the appropriate wrap point. */

    gp3_cmd_current = gp3_cmd_top = start;
    gp3_cmd_bottom = stop;
}

/*---------------------------------------------------------------------------
 * gp_set_frame_buffer_base
 *
 * This routine is used to program the base address of the frame buffer in
 * physical memory.  The frame buffer address must be 16MB aligned.  Cimarron
 * tracks the base address because the maximum frame buffer size may exceed
 * 16MB. Any primitive will thus program the corresponding 16MB region into
 * all base offset registers as well as program the offset into the 16MB
 * region.  The size parameter is provided to allow Cimarron to claim the
 * last 1MB of space to be used as a scratch area for workarounds or
 * expanded functionality.
 *-------------------------------------------------------------------------*/

void
gp_set_frame_buffer_base(unsigned long address, unsigned long size)
{
    gp3_scratch_base = size - GP3_SCRATCH_BUFFER_SIZE;
    gp3_fb_base = address >> 24;
    gp3_base_register =
        (gp3_fb_base << 24) | (gp3_fb_base << 14) | (gp3_fb_base << 4);
    WRITE_GP32(GP3_BASE_OFFSET, gp3_base_register);
}

/*---------------------------------------------------------------------------
 * gp_set_bpp
 *
 * This routine sets the output BPP of the GP.  The BPP used by the GP does
 * not have to match the display BPP, but that is usually the case.  The
 * supported BPP values are as follows:
 *
 *    8  - palettized 8BPP
 *    12 - 4:4:4:4
 *    15 - 1:5:5:5
 *    16 - 0:5:6:5
 *    32 - 8:8:8:8
 *-------------------------------------------------------------------------*/

void
gp_set_bpp(int bpp)
{
    /* STORE BPP */
    /* The bpp is not updated until the next call to gp_set_raster_mode. */
    /* This allows the gp_set_bpp call to happen outside of a BLT.  It   */
    /* also implies that no registers need be written in this routine.   */

    switch (bpp) {
    case 8:
        gp3_bpp = GP3_RM_BPPFMT_332;
        gp3_ch3_bpp = GP3_CH3_SRC_3_3_2;
        gp3_pix_shift = 0;
        break;
    case 12:
        gp3_bpp = GP3_RM_BPPFMT_4444;
        gp3_ch3_bpp = GP3_CH3_SRC_4_4_4_4;
        gp3_pix_shift = 1;
        break;
    case 15:
        gp3_bpp = GP3_RM_BPPFMT_1555;
        gp3_ch3_bpp = GP3_CH3_SRC_1_5_5_5;
        gp3_pix_shift = 1;
        break;
    case 16:
        gp3_bpp = GP3_RM_BPPFMT_565;
        gp3_ch3_bpp = GP3_CH3_SRC_0_5_6_5;
        gp3_pix_shift = 1;
        break;
    case 24:
    case 32:
        gp3_bpp = GP3_RM_BPPFMT_8888;
        gp3_ch3_bpp = GP3_CH3_SRC_8_8_8_8;
        gp3_pix_shift = 2;
        break;
    default:
        gp3_bpp = GP3_RM_BPPFMT_332;
        gp3_ch3_bpp = GP3_CH3_SRC_3_3_2;
        gp3_pix_shift = 0;
        break;
    }
}

/*---------------------------------------------------------------------------
 * gp_declare_blt
 *
 * This routine is used to prepare for a 2D BLT.  Its primary function
 * is to verify that enough room is available in the command buffer
 * to hold a BLT command.  This command can be called multiple times if
 * necessary.  For example, if a function calls this routine on entry, but
 * later realizes that a LUT load command must be executed before the BLT,
 * the application could call gp_set_color_pattern and then call
 * gp_declare_blt to declare the BLT.  This is possible because the hardware
 * buffer pointer is not updated until a new BLT is actually executed.  An
 * application must take care not to call any routines that perform a buffer
 * command, (such as gp_set_color_pattern) between gp_declare_blt and the
 * routines used to program the BLT parameters.  In addition to checking for
 * available space, this routine also performs the following actions:
 *    - Sets the wrap bit if this BLT will pass close to the end of the
 *		buffer.
 *    - Writes the command header.
 *
 * The available flags are defined as follows:
 *   0x01 - Preserve the LUT
 *   0x02 - Preserve the color pattern.
 *   0x04 - Enable prefetch.
 *-------------------------------------------------------------------------*/

void
gp_declare_blt(unsigned long flags)
{
    unsigned long temp;

    gp3_blt = 1;
    gp3_blt_flags = flags;

    /* SET ADDRESS OF NEXT COMMAND */
    /* A summary of the command buffer logic is as follows:           */
    /*  - If after a basic BLT we will not have room for the largest  */
    /*    command (a full line of host source data), we set the wrap  */
    /*    bit.  This will waste up to a whopping 8K of command buffer */
    /*    space, but it simplifies the logic for all commands.        */
    /* -  If we are wrapping, we have extra logic to ensure that we   */
    /*    don't skip over the current GP read pointer.                */

    gp3_cmd_next = gp3_cmd_current + GP3_BLT_COMMAND_SIZE;

    /* CHECK WRAP CONDITION */

    if ((gp3_cmd_bottom - gp3_cmd_next) <= GP3_MAX_COMMAND_SIZE) {
        gp3_cmd_next = gp3_cmd_top;
        gp3_cmd_header = GP3_BLT_HDR_TYPE | GP3_BLT_HDR_WRAP;

        /* WAIT FOR HARDWARE */
        /* When wrapping, we must take steps to ensure that we do not    */
        /* wrap over the current hardware read pointer.  We do this by   */
        /* verifying that the hardware is not between us and the end of  */
        /* the command buffer.  We also have a special case to make sure */
        /* that the hardware is not currently reading the top of the     */
        /* command buffer.                                               */

        GP3_WAIT_WRAP(temp);
    }
    else {
        gp3_cmd_header = GP3_BLT_HDR_TYPE;

        /* WAIT FOR AVAILABLE SPACE */

        GP3_WAIT_PRIMITIVE(temp);
    }

    if (flags & CIMGP_BLTFLAGS_LIMITBUFFER) {
        while (1) {
            temp = READ_GP32(GP3_CMD_READ);
            if (((gp3_cmd_current >= temp)
                 && ((gp3_cmd_current - temp) <= gp3_buffer_lead))
                || ((gp3_cmd_current < temp)
                    && ((gp3_cmd_current + (gp3_cmd_bottom - temp)) <=
                        gp3_buffer_lead))) {
                break;
            }
        }
    }

    /* SET THE CURRENT BUFFER POINTER */
    /* We initialize a pointer to the current buffer base to avoid an */
    /* extra addition for every buffer write.                         */

    cim_cmd_ptr = cim_cmd_base_ptr + gp3_cmd_current;

    /* SET THE HAZARD BIT */

    if (flags & CIMGP_BLTFLAGS_HAZARD)
        gp3_cmd_header |= GP3_BLT_HDR_HAZARD_ENABLE;
}

/*---------------------------------------------------------------------------
 * gp_declare_vector
 *
 * This routine is used to prepare for a 2D vector.  It has no other function
 * except to verify that enough room is available in the command buffer
 * to hold a vector command.  The same rules that apply to BLTs apply to
 * vectors. (See the documentation for gp_declare_blt).
 *-------------------------------------------------------------------------*/

void
gp_declare_vector(unsigned long flags)
{
    unsigned long temp;

    gp3_blt = 0;
    gp3_blt_flags = flags;

    /* SET ADDRESS OF NEXT COMMAND                            */
    /* The logic to force a wrap during a vector is identical */
    /* to the BLT logic.                                      */

    /* ALLOCATE SPACE FOR AN ADDITIONAL VECTOR TO CLEAR THE BYTE ENABLES */

    gp3_cmd_next = gp3_cmd_current + GP3_VECTOR_COMMAND_SIZE +
        GP3_VECTOR_COMMAND_SIZE + 32;

    /* CHECK WRAP CONDITION */

    if ((gp3_cmd_bottom - gp3_cmd_next) <= GP3_MAX_COMMAND_SIZE) {
        gp3_cmd_next = gp3_cmd_top;
        gp3_cmd_header = GP3_VEC_HDR_TYPE | GP3_VEC_HDR_WRAP;

        /* CHECK WRAP CONDITION */

        GP3_WAIT_WRAP(temp);
    }
    else {
        gp3_cmd_header = GP3_VEC_HDR_TYPE;

        /* WAIT FOR AVAILABLE SPACE */

        GP3_WAIT_PRIMITIVE(temp);

        gp3_cmd_next -= GP3_VECTOR_COMMAND_SIZE + 32;
    }

    if (flags & CIMGP_BLTFLAGS_LIMITBUFFER) {
        while (1) {
            temp = READ_GP32(GP3_CMD_READ);
            if (((gp3_cmd_current >= temp)
                 && ((gp3_cmd_current - temp) <= gp3_buffer_lead))
                || ((gp3_cmd_current < temp)
                    && ((gp3_cmd_current + (gp3_cmd_bottom - temp)) <=
                        gp3_buffer_lead))) {
                break;
            }
        }
    }

    cim_cmd_ptr = cim_cmd_base_ptr + gp3_cmd_current;

    /* SET THE HAZARD BIT */

    if (flags & CIMGP_BLTFLAGS_HAZARD)
        gp3_cmd_header |= GP3_VEC_HDR_HAZARD_ENABLE;
}

/*---------------------------------------------------------------------------
 * gp_write_parameters
 *
 * This routine is called to write all recent parameters to the hardware.
 * This routine is necessary for any implementation that performs the setup
 * for a BLT separate from the actual BLT.  An example would be a driver
 * that prepares for multiple pattern fills by programming the ROP,
 * pattern color and destination stride.  The driver might then perform
 * repeated pattern fills with minimal effort.
 *-------------------------------------------------------------------------*/

void
gp_write_parameters(void)
{
    /* WRITE THE COMMAND HEADER */
    /* Command header is at offset 0 for BLTs and vectors */

    WRITE_COMMAND32(GP3_BLT_CMD_HEADER, gp3_cmd_header);

    /* INCREMENT THE CURRENT WRITE POINTER */

    gp3_cmd_current = gp3_cmd_next;

    /* UPDATE THE GP WRITE POINTER */

    WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_current);
}

/*---------------------------------------------------------------------------
 * gp_set_raster_operation
 *
 * This is generally the first routine called when programming a BLT.  This
 * routine performs the following functions:
 *   - Sets the initial value of the GP3_RASTER_MODE register in the buffer.
 *   - Clears any 8x8 pattern if the ROP does not involve pattern data.
 *-------------------------------------------------------------------------*/

void
gp_set_raster_operation(unsigned char ROP)
{
    gp3_cmd_header |= GP3_BLT_HDR_RASTER_ENABLE;

    /* WRITE THE RASTER MODE REGISTER                                   */
    /* This register is in the same location in BLT and vector commands */

    gp3_raster_mode = gp3_bpp | (unsigned long) ROP;
    WRITE_COMMAND32(GP3_BLT_RASTER_MODE, gp3_raster_mode);

    /* CHECK IF DESTINATION IS REQUIRED */

    if ((ROP & 0x55) ^ ((ROP >> 1) & 0x55)) {
        gp3_blt_mode = GP3_BM_DST_REQ;
        gp3_vec_mode = GP3_VM_DST_REQ;
    }
    else {
        gp3_blt_mode = gp3_vec_mode = 0;
    }
}

/*----------------------------------------------------------------------------
 * gp_set_alpha_operation
 *
 * BLTs are generally one of two types, a ROPed BLT or a BLT composited using
 * alpha blending.  For the latter, this routine is used to configure the
 * mathematical function used to create the blended output.  This routine
 * should generally be called first when programming a BLT.  The available
 * parameters mirror the hardware and are described as follows:
 *
 * alpha_operation =
 *   0 - alpha * A
 *   1 - (1 - alpha) * B
 *   2 - A + (1 - alpha)*B
 *   3 - alpha*A + (1 - alpha)*B
 *
 * alpha_type =
 *   0 - alpha component of channel A
 *   1 - alpha component of channel B
 *   2 - Constant alpha
 *   3 - Constant 1
 *   4 - The color components of channel A
 *   5 - The color components of channel B
 *   6 - Alpha comes from the alpha channel of the source before the source
 *		 undergoes color conversion.
 *
 * channel =
 *   0 - Channel A = source, channel B = destination
 *   1 - Channel B = source, channel A = destination
 *
 * apply_alpha =
 *   1 - Apply alpha blend to only the RGB portion of the pixel.  This must be
 *       set when the source or destination format do not include an alpha
 *		 channel.
 *   2 - Apply alpha blend only to the alpha portion of the pixel.  This
 *		 implies that both destination and source include an alpha channel.
 *   3 - Apply alpha blend to both the RGB and alpha portions of the pixel.
 *
 * Alpha-blended vectors are not currently supported.
 *-------------------------------------------------------------------------*/

void
gp_set_alpha_operation(int alpha_operation, int alpha_type, int channel,
                       int apply_alpha, unsigned char alpha)
{
    gp3_cmd_header |= GP3_BLT_HDR_RASTER_ENABLE;

    /* THE AVAILABLE ALPHA DEFINITIONS FOLLOW THE HARDWARE       */
    /* This allows us to avoid giant switch structures, but it   */
    /* also implies that there is no mechanism to detect invalid */
    /* parameters.                                               */

    gp3_raster_mode = gp3_bpp | (unsigned long) alpha |
        ((unsigned long) apply_alpha << 22) |
        ((unsigned long) alpha_operation << 20) |
        ((unsigned long) alpha_type << 17) | ((unsigned long) channel << 16);

    WRITE_COMMAND32(GP3_BLT_RASTER_MODE, gp3_raster_mode);

    /* CHECK IF DESTINATION IS REQUIRED */

    if ((alpha_operation == CIMGP_ALPHA_TIMES_A &&
         channel == CIMGP_CHANNEL_A_SOURCE &&
         alpha_type != CIMGP_CHANNEL_B_ALPHA &&
         alpha_type != CIMGP_ALPHA_FROM_RGB_B) ||
        (alpha_operation == CIMGP_BETA_TIMES_B &&
         channel == CIMGP_CHANNEL_A_DEST &&
         alpha_type != CIMGP_CHANNEL_A_ALPHA &&
         alpha_type != CIMGP_ALPHA_FROM_RGB_A)) {
        gp3_blt_mode = 0;
    }
    else
        gp3_blt_mode = GP3_BM_DST_REQ;
}

/*---------------------------------------------------------------------------
 * gp_set_solid_pattern
 *
 * This routine is called to program the hardware for a solid pattern.  It
 * need not be called for any other reason.  As a side effect, this routine
 * will clear any 8x8 pattern data.
 *-------------------------------------------------------------------------*/

void
gp_set_solid_pattern(unsigned long color)
{
    /* CHANNEL 3 IS NOT NEEDED FOR SOLID PATTERNS */

    gp3_ch3_pat = 0;

    /* SET SOLID PATTERN IN COMMAND BUFFER */
    /* We are assuming that only one pattern type is ever set for a */
    /* BLT.  We are also assuming that gp_set_raster_operation will */
    /* be called before this routine.  With these assumptions, we   */
    /* will thus never have to change the raster mode register for  */
    /* solid patterns.                                              */

    if (gp3_blt) {
        gp3_cmd_header |= GP3_BLT_HDR_PAT_CLR0_ENABLE;

        WRITE_COMMAND32(GP3_BLT_PAT_COLOR_0, color);
    }
    else {
        gp3_cmd_header |= GP3_VEC_HDR_PAT_CLR0_ENABLE;

        WRITE_COMMAND32(GP3_VECTOR_PAT_COLOR_0, color);
    }
}

/*---------------------------------------------------------------------------
 * gp_set_mono_pattern
 *
 * This routine is called to program the hardware for a monochrome pattern.
 * As a side effect, this routine will clear any 8x8 pattern data.
 *-------------------------------------------------------------------------*/

void
gp_set_mono_pattern(unsigned long bgcolor, unsigned long fgcolor,
                    unsigned long data0, unsigned long data1, int transparent,
                    int x, int y)
{
    /* CHANNEL 3 IS NOT NEEDED FOR MONOCHROME PATTERNS */

    gp3_ch3_pat = 0;

    /* UPDATE RASTER MODE REGISTER */

    if (transparent)
        gp3_raster_mode |= GP3_RM_PAT_MONO | GP3_RM_PAT_TRANS;
    else
        gp3_raster_mode |= GP3_RM_PAT_MONO;

    gp3_cmd_header |= GP3_BLT_HDR_RASTER_ENABLE;

    WRITE_COMMAND32(GP3_BLT_RASTER_MODE, gp3_raster_mode);

    /* SET MONOCHROME PATTERN DATA AND COLORS */

    if (gp3_blt) {
        gp3_cmd_header |=
            (GP3_BLT_HDR_PAT_CLR0_ENABLE | GP3_BLT_HDR_PAT_CLR1_ENABLE |
             GP3_BLT_HDR_PAT_DATA0_ENABLE | GP3_BLT_HDR_PAT_DATA1_ENABLE);

        WRITE_COMMAND32(GP3_BLT_PAT_COLOR_0, bgcolor);
        WRITE_COMMAND32(GP3_BLT_PAT_COLOR_1, fgcolor);
        WRITE_COMMAND32(GP3_BLT_PAT_DATA_0, data0);
        WRITE_COMMAND32(GP3_BLT_PAT_DATA_1, data1);
    }
    else {
        gp3_cmd_header |=
            (GP3_VEC_HDR_PAT_CLR0_ENABLE | GP3_VEC_HDR_PAT_CLR1_ENABLE |
             GP3_VEC_HDR_PAT_DATA0_ENABLE | GP3_VEC_HDR_PAT_DATA1_ENABLE);

        WRITE_COMMAND32(GP3_VECTOR_PAT_COLOR_0, bgcolor);
        WRITE_COMMAND32(GP3_VECTOR_PAT_COLOR_1, fgcolor);
        WRITE_COMMAND32(GP3_VECTOR_PAT_DATA_0, data0);
        WRITE_COMMAND32(GP3_VECTOR_PAT_DATA_1, data1);
    }

    /* SAVE PATTERN ORIGIN */

    gp3_pat_origin = ((unsigned long) y << 29) |
        (((unsigned long) x & 7) << 26);
}

/*---------------------------------------------------------------------------
 * gp_set_pattern_origin
 *
 * This routine overrides the pattern origins set in gp_set_mono_pattern or
 * gp_set_color_pattern.  It is generally used to override the original
 * pattern origin due to a change in clipping.
 *-------------------------------------------------------------------------*/

void
gp_set_pattern_origin(int x, int y)
{
    /* SAVE PATTERN ORIGIN */

    gp3_pat_origin = ((unsigned long) y << 29) |
        (((unsigned long) x & 7) << 26);
}

/*---------------------------------------------------------------------------
 * gp_set_color_pattern
 *
 * This routine is called to program a 8x8 color pattern into the LUT
 * hardware.  Unlike the other pattern routines, this routine must be called
 * before any gp_declare_xxxx routines.  The pattern that is programmed into
 * the hardware will stay persistent for all subsequent primitives until one
 * of the following conditions happens.
 *   - Another pattern type is programmed.
 *   - A color-conversion BLT rotation BLT.
 *-------------------------------------------------------------------------*/

void
gp_set_color_pattern(unsigned long *pattern, int format, int x, int y)
{
    unsigned long size_dwords, temp;

    gp3_ch3_pat = 1;

    /* SAVE COLOR PATTERN SOURCE INFO
     * Color patterns can be in a format different than the primary display.
     * 4BPP patterns are not supported.
     */

    gp3_pat_pix_shift = (unsigned long) ((format >> 2) & 3);
    gp3_pat_format = (((unsigned long) format & 0xF) << 24) |
        (((unsigned long) format & 0x10) << 17) |
        GP3_CH3_COLOR_PAT_ENABLE | GP3_CH3_C3EN;

    size_dwords = (64 << gp3_pat_pix_shift) >> 2;

    /* CHECK FOR WRAP AFTER LUT LOAD                 */
    /* Primitive size is 12 plus the amount of data. */

    gp3_cmd_next = gp3_cmd_current + (size_dwords << 2) + 12;

    if ((gp3_cmd_bottom - gp3_cmd_next) <= GP3_MAX_COMMAND_SIZE) {
        gp3_cmd_next = gp3_cmd_top;
        gp3_cmd_header = GP3_LUT_HDR_TYPE | GP3_LUT_HDR_WRAP |
            GP3_LUT_HDR_DATA_ENABLE;

        /* WAIT FOR HARDWARE           */
        /* Same logic as BLT wrapping. */

        GP3_WAIT_WRAP(temp);
    }
    else {
        gp3_cmd_header = GP3_LUT_HDR_TYPE | GP3_LUT_HDR_DATA_ENABLE;

        /* WAIT FOR AVAILABLE SPACE */

        GP3_WAIT_PRIMITIVE(temp);
    }

    /* SAVE CURRENT BUFFER POINTER */

    cim_cmd_ptr = cim_cmd_base_ptr + gp3_cmd_current;

    /* PREPARE FOR COMMAND BUFFER DATA WRITES                 */
    /* Pattern data is contiguous DWORDs at LUT address 0x100 */

    WRITE_COMMAND32(0, gp3_cmd_header);
    WRITE_COMMAND32(4, 0x100);
    WRITE_COMMAND32(8, size_dwords | GP3_LUT_DATA_TYPE);

    /* WRITE ALL DATA */

    WRITE_COMMAND_STRING32(12, pattern, 0, size_dwords);

    /* START OPERATION */

    WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
    gp3_cmd_current = gp3_cmd_next;

    /* SAVE PATTERN ORIGIN */

    gp3_pat_origin = ((unsigned long) y << 29) |
        (((unsigned long) x & 7) << 26);
}

/*---------------------------------------------------------------------------
 * gp_set_mono_source
 *
 * This routine is called to program the colors for monochrome source data.
 *-------------------------------------------------------------------------*/

void
gp_set_mono_source(unsigned long bgcolor, unsigned long fgcolor,
                   int transparent)
{
    /* UPDATE RASTER MODE REGISTER IF TRANSPARENT */

    if (transparent) {
        gp3_cmd_header |= GP3_BLT_HDR_RASTER_ENABLE;
        gp3_raster_mode |= GP3_RM_SRC_TRANS;

        WRITE_COMMAND32(GP3_BLT_RASTER_MODE, gp3_raster_mode);
    }

    /* SET MONOCHROME SOURCE COLORS */
    /* Note that this routine only sets the colors.  The actual */
    /* source type is determined by the final output routine    */
    /* (gp_mono_bitmap_xxx, gp_color_bitmap_xxx, etc.)          */

    gp3_cmd_header |= GP3_BLT_HDR_SRC_FG_ENABLE | GP3_BLT_HDR_SRC_BG_ENABLE;

    WRITE_COMMAND32(GP3_BLT_SRC_COLOR_FG, fgcolor);
    WRITE_COMMAND32(GP3_BLT_SRC_COLOR_BG, bgcolor);
}

/*---------------------------------------------------------------------------
 * gp_set_solid_source
 *
 * This routine is called to program a solid source color.  A solid source
 * color is used primarily for vectors or antialiased text.
 *-------------------------------------------------------------------------*/

void
gp_set_solid_source(unsigned long color)
{
    /* SET SOLID SOURCE COLOR */
    /* The solid source register is in the same place for both BLTs and */
    /* vectors.                                                         */

    gp3_cmd_header |= GP3_BLT_HDR_SRC_FG_ENABLE;

    WRITE_COMMAND32(GP3_BLT_SRC_COLOR_FG, color);
}

/*---------------------------------------------------------------------------
 * gp_set_source_transparency
 *
 * This routine sets the source transparency and mask to be used in future
 * rendering operations.  Transparency is cleared by gp_set_raster_operation,
 * so this routine should never be called first.
 *-------------------------------------------------------------------------*/

void
gp_set_source_transparency(unsigned long color, unsigned long mask)
{
    gp3_raster_mode |= GP3_RM_SRC_TRANS;
    gp3_cmd_header |= GP3_BLT_HDR_RASTER_ENABLE | GP3_BLT_HDR_SRC_FG_ENABLE |
        GP3_BLT_HDR_SRC_BG_ENABLE;

    WRITE_COMMAND32(GP3_BLT_RASTER_MODE, gp3_raster_mode);
    WRITE_COMMAND32(GP3_BLT_SRC_COLOR_FG, color);
    WRITE_COMMAND32(GP3_BLT_SRC_COLOR_BG, mask);
}

/*---------------------------------------------------------------------------
 * gp_program_lut
 *
 * This routine is called to program the hardware LUT with color-conversion
 * information.  This routine should be called before any gp_declare_xxxx
 * routines.
 *
 * colors   - Pointer to an array of DWORDs for color expansion.
 *
 * full_lut - Selector between 4BPP and 8BPP expansion. The hardware is
 *            initialized with 16 dwords for 4BPP expansion and 256 dwords
 *            for 8BPP expansion.
 *-------------------------------------------------------------------------*/

void
gp_program_lut(unsigned long *colors, int full_lut)
{
    unsigned long size_dwords, temp;

    /* SIZE IS EITHER 16 DWORDS (4BPP) or 256 DWORDS (8BPP) */

    if (full_lut)
        size_dwords = 256;
    else
        size_dwords = 16;

    /* CHECK FOR WRAP AFTER LUT LOAD                 */
    /* Primitive size is 12 plus the amount of data. */

    gp3_cmd_next = gp3_cmd_current + (size_dwords << 2) + 12;

    if ((gp3_cmd_bottom - gp3_cmd_next) <= GP3_MAX_COMMAND_SIZE) {
        gp3_cmd_next = gp3_cmd_top;
        gp3_cmd_header = GP3_LUT_HDR_TYPE | GP3_LUT_HDR_WRAP |
            GP3_LUT_HDR_DATA_ENABLE;

        /* WAIT FOR HARDWARE           */
        /* Same logic as BLT wrapping. */

        GP3_WAIT_WRAP(temp);
    }
    else {
        gp3_cmd_header = GP3_LUT_HDR_TYPE | GP3_LUT_HDR_DATA_ENABLE;

        /* WAIT FOR AVAILABLE SPACE */

        GP3_WAIT_PRIMITIVE(temp);
    }

    /* SAVE CURRENT BUFFER POINTER */

    cim_cmd_ptr = cim_cmd_base_ptr + gp3_cmd_current;

    /* PREPARE FOR COMMAND BUFFER DATA WRITES             */
    /* Pattern data is contiguous DWORDs at LUT address 0 */

    WRITE_COMMAND32(0, gp3_cmd_header);
    WRITE_COMMAND32(4, 0);
    WRITE_COMMAND32(8, (size_dwords | GP3_LUT_DATA_TYPE));

    /* WRITE ALL DATA */

    WRITE_COMMAND_STRING32(12, colors, 0, size_dwords);

    /* START OPERATION */

    WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
    gp3_cmd_current = gp3_cmd_next;
}

/*---------------------------------------------------------------------------
 * gp_set_vector_pattern
 *
 * This routine is called to program the hardware LUT with a vector pattern.
 * A vector pattern is programmed as a 32-bit mask that specifies a
 * transparency pattern.  A length parameter is used to specify patterns
 * smaller than 32.  Note that vectors in Geode LX do not continue across
 * corners.  The beginning of each vector will always begin with bit 0 of the
 * vector pattern.  It is the responsibility of the caller to update the
 * pattern if an alternate behavior is desired.
 *
 * This routine faces the same restrictions of all routines that program
 * the LUT, in that it must be called before any gp_declare_xxxx routines,
 * it cannot be combined with an 8x8 color pattern, color conversion or
 * rotation.
 *-------------------------------------------------------------------------*/

void
gp_set_vector_pattern(unsigned long pattern, unsigned long color, int length)
{
    unsigned long temp, mask;

    gp3_ch3_pat = 1;
    gp3_vector_pattern_color = color;

    /* CREATE SUITABLE PATTERN MASK */
    /* The GP requires a minimum of four pixels in a vector pattern.  We */
    /* can get around this restriction by doubling any patterns smaller  */
    /* than 4 pixels.                                                    */

    while (length < 4) {
        mask = 0xFFFFFFFF >> (32 - length);
        pattern = (pattern << length) | (pattern & mask);
        length <<= 1;
    }
    mask = 0xFFFFFFFF >> (32 - length);

    gp3_vec_pat = pattern;

    /* CHECK FOR WRAP AFTER LUT LOAD */

    gp3_cmd_next = gp3_cmd_current + GP3_VECTOR_PATTERN_COMMAND_SIZE;

    if ((gp3_cmd_bottom - gp3_cmd_next) <= GP3_MAX_COMMAND_SIZE) {
        gp3_cmd_next = gp3_cmd_top;
        gp3_cmd_header = GP3_LUT_HDR_TYPE | GP3_LUT_HDR_WRAP |
            GP3_LUT_HDR_DATA_ENABLE;

        /* WAIT FOR HARDWARE           */
        /* Same logic as BLT wrapping. */

        GP3_WAIT_WRAP(temp);
    }
    else {
        gp3_cmd_header = GP3_LUT_HDR_TYPE | GP3_LUT_HDR_DATA_ENABLE;

        /* WAIT FOR AVAILABLE SPACE */

        GP3_WAIT_PRIMITIVE(temp);
    }

    /* SAVE CURRENT BUFFER POINTER */

    cim_cmd_ptr = cim_cmd_base_ptr + gp3_cmd_current;

    /* PREPARE FOR COMMAND BUFFER DATA WRITES      */
    /* Pattern data is 2 DWORDs at 0x100 and 0x101 */

    WRITE_COMMAND32(0, gp3_cmd_header);
    WRITE_COMMAND32(4, 0x100);
    WRITE_COMMAND32(8, (2 | GP3_LUT_DATA_TYPE));
    WRITE_COMMAND32(12, pattern);
    WRITE_COMMAND32(16, mask);

    /* START OPERATION */

    WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
    gp3_cmd_current = gp3_cmd_next;
}

/*---------------------------------------------------------------------------
 * gp_set_strides
 *
 * This routine is called to program the pitch between successive lines of
 * data in the frame buffer.  The strides should be DWORD aligned and less
 * than 64K.  These restrictions are not checked by the API.
 *-------------------------------------------------------------------------*/

void
gp_set_strides(unsigned long dst_stride, unsigned long src_stride)
{
    /* SAVE STRIDES */
    /* The source stride may be needed later for channel 3 source data and */
    /* we may need to use these strides in calculations.                   */

    gp3_src_stride = src_stride;
    gp3_dst_stride = dst_stride;

    /* ENABLE STRIDES */
    /* The stride register is in the same place for BLTs and vectors */

    gp3_cmd_header |= GP3_BLT_HDR_STRIDE_ENABLE;

    WRITE_COMMAND32(GP3_BLT_STRIDE, ((src_stride << 16) | dst_stride));
}

/*---------------------------------------------------------------------------
 * gp_set_source_format
 *
 * This routine is used to program the format of source data used in
 * subsequent color-conversion or rotation operations.  Note that 4BPP
 * indexed and 8BPP indexed source formats cannot be used when rotating, as
 * the LUT will be destroyed.  These formats also cannot be combined with an
 * 8x8 color pattern.  The supported formats mirror the hardware and are
 * described as follows:
 *
 * 0  - 8BPP 3:3:2
 * 1  - 8BPP indexed
 * 4  - 16BPP 4:4:4:4
 * 5  - 16BPP 1:5:5:5
 * 6  - 16BPP 5:6:5
 * 7  - 16BPP YUV
 * 8  - 32BPP 8:8:8:8
 * 13 - 4BPP indexed
 * 20 - 16BPP 4:4:4:4 BGR
 * 21 - 16BPP 1:5:5:5 BGR
 * 22 - 16BPP 0:5:6:5 BGR
 * 24 - 32BPP 8:8:8:8 BGR
 *-------------------------------------------------------------------------*/

void
gp_set_source_format(int format)
{
    /* SAVE FORMAT */
    /* We will combine the source format later when doing color conversion. */
    /* We also save the pixel size for host source calculations.            */
    /* Conveniently, the source formats are organized such that the upper   */
    /* two bits of the nibble represent the pixel shift, with a pixel shift */
    /* of 3 being a special case for 4BPP data.  Clever, yes?  Even more    */
    /* clever, bit 4 indicates BGR ordering.                                */

    gp3_src_pix_shift = (unsigned long) ((format >> 2) & 3);
    gp3_src_format = (((unsigned long) format & 0xF) << 24) |
        (((unsigned long) format & 0x10) << 18);
}

/*---------------------------------------------------------------------------
 * gp_pattern_fill
 *
 * This routine is called to perform a simple pattern fill.  The pattern
 * can be solid, monochrome or a preprogrammed 8x8 color pattern.  If
 * the ROP involves source data, that source data will be constant.
 *-------------------------------------------------------------------------*/

void
gp_pattern_fill(unsigned long dstoffset, unsigned long width,
                unsigned long height)
{
    unsigned long base_register;

    base_register = (gp3_base_register & ~GP3_BASE_OFFSET_DSTMASK) |
        ((gp3_fb_base << 24) + (dstoffset & 0xFFC00000));

    /* ENABLE RELEVANT REGISTERS */
    /* Note that we always enable and write the channel 3 mode, if only */
    /* to turn it off.   Cimarron also always writes the base offset    */
    /* register to allow operation with frame buffers larger than 16MB. */

    gp3_cmd_header |= GP3_BLT_HDR_DST_OFF_ENABLE |
        GP3_BLT_HDR_WIDHI_ENABLE |
        GP3_BLT_HDR_BASE_OFFSET_ENABLE |
        GP3_BLT_HDR_CH3_STR_ENABLE |
        GP3_BLT_HDR_CH3_WIDHI_ENABLE | GP3_BLT_HDR_BLT_MODE_ENABLE;

    /* WRITE THE REGISTERS COMMON TO ALL PATTERN TYPES */
    /* The destination base is the frame buffer base plus whatever */
    /* 4MB segment we happen to be BLTing to.                      */

    WRITE_COMMAND32(GP3_BLT_WID_HEIGHT, ((width << 16) | height));
    WRITE_COMMAND32(GP3_BLT_BASE_OFFSET, base_register);

    /* CHECK 8X8 COLOR PATTERN CASE */

    if (gp3_ch3_pat) {
        /* SET CHANNEL 3 PATTERN ORIGINS */

        gp3_cmd_header |= GP3_BLT_HDR_CH3_OFF_ENABLE;

        /* INITIALIZE CHANNEL 3 PARAMETERS */

        WRITE_COMMAND32(GP3_BLT_CH3_WIDHI, ((width << 16) | height));
        WRITE_COMMAND32(GP3_BLT_CH3_OFFSET, gp3_pat_origin);
        WRITE_COMMAND32(GP3_BLT_DST_OFFSET, dstoffset & 0x3FFFFF);
        WRITE_COMMAND32(GP3_BLT_CH3_MODE_STR, gp3_pat_format);
    }
    else {
        /* DISABLE CHANNEL 3 AND USE NORMAL PATTERN ORIGINS */

        WRITE_COMMAND32(GP3_BLT_CH3_MODE_STR, 0);
        WRITE_COMMAND32(GP3_BLT_DST_OFFSET,
                        ((dstoffset & 0x3FFFFF) | gp3_pat_origin));
    }

    /* START THE BLT */

    WRITE_COMMAND32(GP3_BLT_CMD_HEADER, gp3_cmd_header);
    WRITE_COMMAND32(GP3_BLT_MODE, gp3_blt_mode);
    WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
    gp3_cmd_current = gp3_cmd_next;
}

/*---------------------------------------------------------------------------
 * gp_screen_to_screen_blt
 *
 * This routine is called to perform a BLT from one location inside video
 * memory to another location inside video memory.  The source and destination
 * formats are assumed to be the current BPP.  Whenever possible, this routine
 * tries to use channel 3 to fetch source data.  The BLT flags can affect this
 * behavior in the following ways:
 *   CIMGP_BLTFLAGS_PRES_COLOR_PAT
 *      A color pattern is being stored in the channel 3 buffer.  It is either
 *      being stored for a later BLT or being combined with the current source
 *      data.  Channel 3 cannot be used to fetch source data or the pattern
 *      will be overwritten.
 *   CIMGP_BLTFLAGS_PRES_LUT
 *      If the first flag is not set, this flag will limit the use of the
 *      channel 3 buffer to 1K.
 *-------------------------------------------------------------------------*/

void
gp_screen_to_screen_blt(unsigned long dstoffset, unsigned long srcoffset,
                        unsigned long width, unsigned long height, int flags)
{
    unsigned long base;
    unsigned long ch3_flags = 0;
    unsigned long blt_mode = gp3_blt_mode;
    unsigned long size = ((width << 16) | height);
    unsigned long dstbase, srcbase;

    /* CALCULATE BASE OFFSETS                                             */
    /* We need to set the 4MB aligned base offsets before we add offsets  */
    /* for negative BLTs.                                                 */

    srcbase = srcoffset & 0xFFC00000;
    dstbase = dstoffset & 0xFFC00000;
    srcoffset &= 0x3FFFFF;
    dstoffset &= 0x3FFFFF;

    /* ADJUST OFFSETS BASED ON FLAGS */
    /* We adjust the source and destination offsets to point to the first */
    /* byte of the first pixel of the BLT.  This routine assumes that the */
    /* source and destination regions do not wrap past the end of a 16MB  */
    /* region.                                                            */

    if (flags & CIMGP_NEGXDIR) {
        srcoffset += (width << gp3_pix_shift) - 1;
        dstoffset += (width << gp3_pix_shift) - 1;
        blt_mode |= GP3_BM_NEG_XDIR;
        ch3_flags |= GP3_CH3_NEG_XDIR;
    }
    if (flags & CIMGP_NEGYDIR) {
        srcoffset += (height - 1) * gp3_src_stride;
        dstoffset += (height - 1) * gp3_dst_stride;
        blt_mode |= GP3_BM_NEG_YDIR;
        ch3_flags |= GP3_CH3_NEG_YDIR;
    }

    /* BRANCH BASED ON CHANNEL 3                                     */
    /* If a color pattern is not being saved or used, channel 3 will */
    /* be used to fetch source for maximum throughput.   Channel 3   */
    /* is not used if transparency or alpha blending is enabled.     */

    if (!(gp3_blt_flags & CIMGP_BLTFLAGS_PRES_COLOR_PAT) &&
        !(gp3_raster_mode & GP3_RM_SRC_TRANS) && !(flags & CIMGP_NEGYDIR)) {
        base = ((gp3_fb_base << 24) + dstbase) |
            ((gp3_fb_base << 4) + (srcbase >> 20)) |
            (gp3_base_register & GP3_BASE_OFFSET_SRCMASK);

        gp3_cmd_header |= GP3_BLT_HDR_DST_OFF_ENABLE |
            GP3_BLT_HDR_WIDHI_ENABLE |
            GP3_BLT_HDR_CH3_STR_ENABLE |
            GP3_BLT_HDR_CH3_WIDHI_ENABLE |
            GP3_BLT_HDR_CH3_OFF_ENABLE |
            GP3_BLT_HDR_BASE_OFFSET_ENABLE | GP3_BLT_HDR_BLT_MODE_ENABLE;

        WRITE_COMMAND32(GP3_BLT_DST_OFFSET, (dstoffset | gp3_pat_origin));
        WRITE_COMMAND32(GP3_BLT_CH3_OFFSET, srcoffset);
        WRITE_COMMAND32(GP3_BLT_WID_HEIGHT, size);
        WRITE_COMMAND32(GP3_BLT_CH3_WIDHI, size);
        WRITE_COMMAND32(GP3_BLT_BASE_OFFSET, base);
        WRITE_COMMAND32(GP3_BLT_CH3_MODE_STR,
                        GP3_CH3_C3EN | GP3_CH3_REPLACE_SOURCE |
                        gp3_ch3_bpp | gp3_src_stride | ch3_flags |
                        ((gp3_blt_flags & CIMGP_ENABLE_PREFETCH) << 17) |
                        ((gp3_blt_flags & CIMGP_BLTFLAGS_PRES_LUT) << 20));
    }
    else {
        /* CALCULATE BASE OFFSET REGISTER */

        base = ((gp3_fb_base << 24) + dstbase) |
            ((gp3_fb_base << 14) + (srcbase >> 10)) |
            (gp3_base_register & GP3_BASE_OFFSET_CH3MASK);

        /* PROGRAM THE NORMAL SOURCE CHANNEL REGISTERS                */
        /* We assume that a color pattern is being ROPed with source  */
        /* data if the pattern type is color and the preserve pattern */
        /* was set.                                                   */

        blt_mode |= GP3_BM_SRC_FB;

        gp3_cmd_header |= GP3_BLT_HDR_SRC_OFF_ENABLE |
            GP3_BLT_HDR_DST_OFF_ENABLE |
            GP3_BLT_HDR_WIDHI_ENABLE |
            GP3_BLT_HDR_CH3_WIDHI_ENABLE |
            GP3_BLT_HDR_CH3_STR_ENABLE |
            GP3_BLT_HDR_CH3_OFF_ENABLE |
            GP3_BLT_HDR_BASE_OFFSET_ENABLE | GP3_BLT_HDR_BLT_MODE_ENABLE;

        if (gp3_ch3_pat) {
            WRITE_COMMAND32(GP3_BLT_CH3_OFFSET, gp3_pat_origin);
            WRITE_COMMAND32(GP3_BLT_DST_OFFSET, dstoffset);
            WRITE_COMMAND32(GP3_BLT_CH3_MODE_STR, gp3_pat_format | ch3_flags);
            WRITE_COMMAND32(GP3_BLT_CH3_WIDHI, size);
        }
        else {
            WRITE_COMMAND32(GP3_BLT_DST_OFFSET, dstoffset | gp3_pat_origin);
            WRITE_COMMAND32(GP3_BLT_CH3_MODE_STR, 0);
        }

        WRITE_COMMAND32(GP3_BLT_SRC_OFFSET, srcoffset);
        WRITE_COMMAND32(GP3_BLT_WID_HEIGHT, size);
        WRITE_COMMAND32(GP3_BLT_BASE_OFFSET, base);
    }

    /* START THE BLT */

    WRITE_COMMAND32(GP3_BLT_CMD_HEADER, gp3_cmd_header);
    WRITE_COMMAND32(GP3_BLT_MODE, blt_mode);
    WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
    gp3_cmd_current = gp3_cmd_next;
}

/*---------------------------------------------------------------------------
 * gp_screen_to_screen_convert
 *
 * This routine is called to color-convert a rectangular region of the frame
 * buffer into the current BPP. The format of the source region is programmed
 * by gp_set_source_format.
 *-------------------------------------------------------------------------*/

void
gp_screen_to_screen_convert(unsigned long dstoffset, unsigned long srcoffset,
                            unsigned long width, unsigned long height,
                            int nibble)
{
    unsigned long size = ((width << 16) | height);
    unsigned long ch3_offset = srcoffset & 0x3FFFFF;
    unsigned long ch3_size, base;

    base = ((gp3_fb_base << 24) + (dstoffset & 0xFFC00000)) |
        ((gp3_fb_base << 4) + ((srcoffset & 0xFFC00000) >> 20)) |
        (gp3_base_register & GP3_BASE_OFFSET_SRCMASK);

    /* SET NIBBLE FOR 4BPP */
    /* 4BPP is a special case in that it requires subpixel addressing.  The */
    /* user must supply this information via the nibble parameter.  This    */
    /* parameter is ignored for every other source format.                  */

    ch3_size = size;
    if (gp3_src_pix_shift == 3)
        ch3_offset |= ((nibble & 1) << 25);
    else if ((gp3_src_format & GP3_CH3_SRC_MASK) == GP3_CH3_SRC_24BPP_PACKED)
        ch3_size = ((((width * 3) + 3) >> 2) << 16) | height;

    /* SET APPROPRIATE ENABLES */

    gp3_cmd_header |= GP3_BLT_HDR_DST_OFF_ENABLE |
        GP3_BLT_HDR_WIDHI_ENABLE |
        GP3_BLT_HDR_CH3_OFF_ENABLE |
        GP3_BLT_HDR_CH3_STR_ENABLE |
        GP3_BLT_HDR_CH3_WIDHI_ENABLE |
        GP3_BLT_HDR_BASE_OFFSET_ENABLE | GP3_BLT_HDR_BLT_MODE_ENABLE;

    /* WRITE ALL BLT REGISTERS */

    WRITE_COMMAND32(GP3_BLT_DST_OFFSET,
                    (dstoffset & 0x3FFFFF) | gp3_pat_origin);
    WRITE_COMMAND32(GP3_BLT_WID_HEIGHT, size);
    WRITE_COMMAND32(GP3_BLT_CH3_WIDHI, ch3_size);
    WRITE_COMMAND32(GP3_BLT_CH3_OFFSET, ch3_offset);
    WRITE_COMMAND32(GP3_BLT_CH3_MODE_STR,
                    GP3_CH3_C3EN | GP3_CH3_REPLACE_SOURCE | gp3_src_format |
                    ((gp3_blt_flags & CIMGP_BLTFLAGS_PRES_LUT) << 20) |
                    ((gp3_blt_flags & CIMGP_ENABLE_PREFETCH) << 17) |
                    gp3_src_stride);
    WRITE_COMMAND32(GP3_BLT_BASE_OFFSET, base);

    /* START THE BLT */

    WRITE_COMMAND32(GP3_BLT_CMD_HEADER, gp3_cmd_header);
    WRITE_COMMAND32(GP3_BLT_MODE, gp3_blt_mode);
    WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
    gp3_cmd_current = gp3_cmd_next;
}

/*---------------------------------------------------------------------------
 * gp_color_bitmap_to_screen_blt
 *
 * This routine is called to BLT data from system memory into the frame
 * buffer.  'srcy' is deliberately omitted to prevent extra calculations for
 * simple applications that have no source indexes.
 *-------------------------------------------------------------------------*/

void
gp_color_bitmap_to_screen_blt(unsigned long dstoffset, unsigned long srcx,
                              unsigned long width, unsigned long height,
                              unsigned char *data, long pitch)
{
    unsigned long indent, temp;
    unsigned long total_dwords, size_dwords;
    unsigned long dword_count, byte_count;
    unsigned long size = ((width << 16) | height);
    unsigned long srcoffset;

    /* ASSUME BITMAPS ARE DWORD ALIGNED */
    /* We will offset into the source data in DWORD increments.  We */
    /* set the source index to the remaining byte offset and        */
    /* increment the size of each line to account for the dont-care */
    /* pixel(s).                                                    */

    indent = srcx << gp3_pix_shift;
    srcoffset = (indent & ~3L);
    indent &= 3;

    /* PROGRAM THE NORMAL SOURCE CHANNEL REGISTERS                */
    /* We assume that a color pattern is being ROPed with source  */
    /* data if the pattern type is color and the preserve pattern */
    /* was set.                                                   */

    gp3_cmd_header |= GP3_BLT_HDR_SRC_OFF_ENABLE |
        GP3_BLT_HDR_DST_OFF_ENABLE |
        GP3_BLT_HDR_WIDHI_ENABLE |
        GP3_BLT_HDR_CH3_STR_ENABLE |
        GP3_BLT_HDR_BASE_OFFSET_ENABLE | GP3_BLT_HDR_BLT_MODE_ENABLE;

    if (gp3_ch3_pat) {
        gp3_cmd_header |= GP3_BLT_HDR_CH3_OFF_ENABLE |
            GP3_BLT_HDR_CH3_WIDHI_ENABLE;

        WRITE_COMMAND32(GP3_BLT_CH3_OFFSET, gp3_pat_origin);
        WRITE_COMMAND32(GP3_BLT_DST_OFFSET, (dstoffset & 0x3FFFFF));
        WRITE_COMMAND32(GP3_BLT_CH3_MODE_STR, gp3_pat_format);
        WRITE_COMMAND32(GP3_BLT_CH3_WIDHI, size);
    }
    else {
        WRITE_COMMAND32(GP3_BLT_DST_OFFSET,
                        ((dstoffset & 0x3FFFFF) | gp3_pat_origin));
        WRITE_COMMAND32(GP3_BLT_CH3_MODE_STR, 0);
    }

    WRITE_COMMAND32(GP3_BLT_SRC_OFFSET, indent);
    WRITE_COMMAND32(GP3_BLT_WID_HEIGHT, size);
    WRITE_COMMAND32(GP3_BLT_BASE_OFFSET,
                    ((gp3_fb_base << 24) + (dstoffset & 0xFFC00000)));
    WRITE_COMMAND32(GP3_BLT_MODE, gp3_blt_mode | GP3_BM_SRC_HOST);

    /* START THE BLT */

    WRITE_COMMAND32(GP3_BLT_CMD_HEADER, gp3_cmd_header);
    WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
    gp3_cmd_current = gp3_cmd_next;

    /* CALCULATE THE SIZE OF ONE LINE */

    size = (width << gp3_pix_shift) + indent;
    total_dwords = (size + 3) >> 2;
    size_dwords = (total_dwords << 2) + 8;
    dword_count = (size >> 2);
    byte_count = (size & 3);

    /* CHECK FOR SMALL BLT CASE */

    if (((total_dwords << 2) * height) <= GP3_BLT_1PASS_SIZE &&
        (gp3_cmd_bottom - gp3_cmd_current) > (GP3_BLT_1PASS_SIZE + 72)) {
        /* UPDATE THE COMMAND POINTER */

        cim_cmd_ptr = cim_cmd_base_ptr + gp3_cmd_current;

        /* CHECK IF A WRAP WILL BE NEEDED */

        gp3_cmd_next = gp3_cmd_current + ((total_dwords << 2) * height) + 8;

        if ((gp3_cmd_bottom - gp3_cmd_next) <= GP3_MAX_COMMAND_SIZE) {
            gp3_cmd_next = gp3_cmd_top;

            GP3_WAIT_WRAP(temp);
            WRITE_COMMAND32(0,
                            GP3_DATA_LOAD_HDR_TYPE | GP3_DATA_LOAD_HDR_WRAP |
                            GP3_DATA_LOAD_HDR_ENABLE);
        }
        else {
            GP3_WAIT_PRIMITIVE(temp);
            WRITE_COMMAND32(0,
                            GP3_DATA_LOAD_HDR_TYPE | GP3_DATA_LOAD_HDR_ENABLE);
        }

        /* WRITE DWORD COUNT */

        WRITE_COMMAND32(4, GP3_HOST_SOURCE_TYPE | (total_dwords * height));

        while (height--) {
            /* WRITE DATA */

            WRITE_COMMAND_STRING32(8, data, srcoffset, dword_count);
            WRITE_COMMAND_STRING8(8 + (dword_count << 2), data,
                                  srcoffset + (dword_count << 2), byte_count);

            srcoffset += pitch;
            cim_cmd_ptr += total_dwords << 2;
        }

        WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
        gp3_cmd_current = gp3_cmd_next;
    }
    else {
        /*
         * Each line will be created as a separate command buffer entry to
         * allow line-by-line wrapping and to allow simultaneous rendering
         * by the HW.
         */

        while (height--) {
            /* UPDATE THE COMMAND POINTER
             * The WRITE_COMMANDXX macros use a pointer to the current buffer
             * space.  This is created by adding gp3_cmd_current to the base
             * pointer.
             */

            cim_cmd_ptr = cim_cmd_base_ptr + gp3_cmd_current;

            /* CHECK IF A WRAP WILL BE NEEDED */

            gp3_cmd_next = gp3_cmd_current + size_dwords;
            if ((gp3_cmd_bottom - gp3_cmd_next) <= GP3_MAX_COMMAND_SIZE) {
                gp3_cmd_next = gp3_cmd_top;

                /* WAIT FOR HARDWARE */

                GP3_WAIT_WRAP(temp);
                WRITE_COMMAND32(0, GP3_DATA_LOAD_HDR_TYPE |
                                GP3_DATA_LOAD_HDR_WRAP |
                                GP3_DATA_LOAD_HDR_ENABLE);
            }
            else {
                /* WAIT FOR AVAILABLE SPACE */

                GP3_WAIT_PRIMITIVE(temp);
                WRITE_COMMAND32(0, GP3_DATA_LOAD_HDR_TYPE |
                                GP3_DATA_LOAD_HDR_ENABLE);
            }

            /* WRITE DWORD COUNT */

            WRITE_COMMAND32(4, GP3_HOST_SOURCE_TYPE | total_dwords);

            /* WRITE DATA */

            WRITE_COMMAND_STRING32(8, data, srcoffset, dword_count);
            WRITE_COMMAND_STRING8(8 + (dword_count << 2), data,
                                  srcoffset + (dword_count << 2), byte_count);

            /* UPDATE POINTERS */

            srcoffset += pitch;
            WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
            gp3_cmd_current = gp3_cmd_next;
        }
    }
}

/*---------------------------------------------------------------------------
 * gp_color_convert_blt
 *
 * This routine is called to convert data that is stored in system memory
 * into the current graphics BPP.  The source format is programmed in
 * gp_set_source_format.
 *-------------------------------------------------------------------------*/

void
gp_color_convert_blt(unsigned long dstoffset, unsigned long srcx,
                     unsigned long width, unsigned long height,
                     unsigned char *data, long pitch)
{
    unsigned long indent, temp;
    unsigned long total_dwords, size_dwords;
    unsigned long dword_count, byte_count;
    unsigned long size = ((width << 16) | height);
    unsigned long ch3_size;
    unsigned long ch3_offset, srcoffset;
    unsigned long base;

    /* ASSUME BITMAPS ARE DWORD ALIGNED */
    /* We will offset into the source data in DWORD increments.  We  */
    /* set the source index to the remaining byte offset and         */
    /* increment the size of each line to account for the dont-care  */
    /* pixel(s).   For 4BPP source data, we also set the appropriate */
    /* nibble index.                                                 */

    /* CALCULATE THE SIZE OF ONE LINE */

    if ((gp3_src_format & GP3_CH3_SRC_MASK) == GP3_CH3_SRC_24BPP_PACKED) {
        /* HANDLE 24BPP
         * Note that we do not do anything to guarantee that the source data
         * is DWORD aligned.  The logic here is that the source data will be
         * cacheable, in which case Geode LX will not lose any clocks for
         * unaligned moves.  Also note that the channel 3 width is
         * programmed as the number of dwords, while the normal width is
         * programmed as the number of pixels.
         */

        srcoffset = srcx * 3;
        ch3_offset = 0;
        temp = width * 3;
        ch3_size = (((temp + 3) >> 2) << 16) | height;
    }
    else {
        ch3_size = size;

        if (gp3_src_pix_shift == 3) {
            /* CALCULATE INDENT AND SOURCE OFFSET */

            indent = (srcx >> 1);
            srcoffset = (indent & ~3L);
            indent &= 3;
            ch3_offset = indent | ((srcx & 1) << 25);

            temp = ((width + (srcx & 1) + 1) >> 1) + indent;
        }
        else {
            indent = (srcx << gp3_src_pix_shift);
            srcoffset = (indent & ~3L);
            indent &= 3;
            ch3_offset = indent;

            temp = (width << gp3_src_pix_shift) + indent;
        }
    }

    total_dwords = (temp + 3) >> 2;
    size_dwords = (total_dwords << 2) + 8;
    dword_count = (temp >> 2);
    byte_count = (temp & 3);

    base = ((gp3_fb_base << 24) + (dstoffset & 0xFFC00000)) |
        (gp3_base_register & ~GP3_BASE_OFFSET_DSTMASK);

    /* SET APPROPRIATE ENABLES */

    gp3_cmd_header |= GP3_BLT_HDR_DST_OFF_ENABLE |
        GP3_BLT_HDR_WIDHI_ENABLE |
        GP3_BLT_HDR_CH3_STR_ENABLE |
        GP3_BLT_HDR_CH3_OFF_ENABLE |
        GP3_BLT_HDR_CH3_WIDHI_ENABLE |
        GP3_BLT_HDR_BASE_OFFSET_ENABLE | GP3_BLT_HDR_BLT_MODE_ENABLE;

    WRITE_COMMAND32(GP3_BLT_DST_OFFSET,
                    (dstoffset & 0x3FFFFF) | gp3_pat_origin);
    WRITE_COMMAND32(GP3_BLT_CH3_OFFSET, ch3_offset);
    WRITE_COMMAND32(GP3_BLT_WID_HEIGHT, size);
    WRITE_COMMAND32(GP3_BLT_CH3_WIDHI, ch3_size);
    WRITE_COMMAND32(GP3_BLT_BASE_OFFSET, base);
    WRITE_COMMAND32(GP3_BLT_CH3_MODE_STR, GP3_CH3_C3EN |
                    GP3_CH3_REPLACE_SOURCE | GP3_CH3_HST_SRC_ENABLE |
                    gp3_src_format | ((gp3_blt_flags & CIMGP_BLTFLAGS_PRES_LUT)
                                      << 20));
    WRITE_COMMAND32(GP3_BLT_MODE, gp3_blt_mode);

    /* START THE BLT */

    WRITE_COMMAND32(GP3_BLT_CMD_HEADER, gp3_cmd_header);
    WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
    gp3_cmd_current = gp3_cmd_next;

    if (((total_dwords << 2) * height) <= GP3_BLT_1PASS_SIZE &&
        (gp3_cmd_bottom - gp3_cmd_current) > (GP3_BLT_1PASS_SIZE + 72)) {
        /* UPDATE THE COMMAND POINTER */

        cim_cmd_ptr = cim_cmd_base_ptr + gp3_cmd_current;

        /* CHECK IF A WRAP WILL BE NEEDED */

        gp3_cmd_next = gp3_cmd_current + ((total_dwords << 2) * height) + 8;

        if ((gp3_cmd_bottom - gp3_cmd_next) <= GP3_MAX_COMMAND_SIZE) {
            gp3_cmd_next = gp3_cmd_top;

            GP3_WAIT_WRAP(temp);
            WRITE_COMMAND32(0, GP3_DATA_LOAD_HDR_TYPE |
                            GP3_DATA_LOAD_HDR_WRAP | GP3_DATA_LOAD_HDR_ENABLE);
        }
        else {
            GP3_WAIT_PRIMITIVE(temp);
            WRITE_COMMAND32(0,
                            GP3_DATA_LOAD_HDR_TYPE | GP3_DATA_LOAD_HDR_ENABLE);
        }

        /* WRITE DWORD COUNT */

        WRITE_COMMAND32(4, GP3_CH3_HOST_SOURCE_TYPE | (total_dwords * height));

        while (height--) {
            /* WRITE DATA */

            WRITE_COMMAND_STRING32(8, data, srcoffset, dword_count);
            WRITE_COMMAND_STRING8(8 + (dword_count << 2), data,
                                  srcoffset + (dword_count << 2), byte_count);

            srcoffset += pitch;
            cim_cmd_ptr += total_dwords << 2;
        }

        WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
        gp3_cmd_current = gp3_cmd_next;
    }
    else {
        /* WRITE DATA LINE BY LINE
         * Each line will be created as a separate command buffer entry to
         * allow line-by-line wrapping and to allow simultaneous rendering
         * by the HW.
         */

        while (height--) {
            /* UPDATE THE COMMAND POINTER
             * The WRITE_COMMANDXX macros use a pointer to the current buffer
             * space. This is created by adding gp3_cmd_current to the base
             * pointer.
             */

            cim_cmd_ptr = cim_cmd_base_ptr + gp3_cmd_current;

            /* CHECK IF A WRAP WILL BE NEEDED */

            gp3_cmd_next = gp3_cmd_current + size_dwords;
            if ((gp3_cmd_bottom - gp3_cmd_next) <= GP3_MAX_COMMAND_SIZE) {
                gp3_cmd_next = gp3_cmd_top;

                /* WAIT FOR HARDWARE */

                GP3_WAIT_WRAP(temp);
                WRITE_COMMAND32(0, GP3_DATA_LOAD_HDR_TYPE |
                                GP3_DATA_LOAD_HDR_WRAP |
                                GP3_DATA_LOAD_HDR_ENABLE);
            }
            else {
                /* WAIT FOR AVAILABLE SPACE */

                GP3_WAIT_PRIMITIVE(temp);
                WRITE_COMMAND32(0, GP3_DATA_LOAD_HDR_TYPE |
                                GP3_DATA_LOAD_HDR_ENABLE);
            }

            /* WRITE DWORD COUNT */

            WRITE_COMMAND32(4, GP3_CH3_HOST_SOURCE_TYPE | total_dwords);

            /* WRITE DATA */

            WRITE_COMMAND_STRING32(8, data, srcoffset, dword_count);
            WRITE_COMMAND_STRING8(8 + (dword_count << 2), data,
                                  srcoffset + (dword_count << 2), byte_count);

            /* UPDATE POINTERS */

            srcoffset += pitch;
            WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
            gp3_cmd_current = gp3_cmd_next;
        }
    }
}

/*---------------------------------------------------------------------------
 * gp_custom_convert_blt
 *
 * This routine is identical to gp_color_convert_blt, except that the macro
 * to write data to the frame buffer has been replaced with a new macro.  This
 * allows a user to implement custom behavior when sending data, such as
 * manually converting 24BPP to 32BPP, converting 2BPP to 4BPP or
 * premultiplying alpha data.
 *-------------------------------------------------------------------------*/

void
gp_custom_convert_blt(unsigned long dstoffset, unsigned long srcx,
                      unsigned long width, unsigned long height,
                      unsigned char *data, long pitch)
{
    unsigned long indent, temp;
    unsigned long total_dwords, size_dwords;
    unsigned long dword_count, byte_count;
    unsigned long size = ((width << 16) | height);
    unsigned long ch3_offset, srcoffset;
    unsigned long ch3_size, base;

    /* ASSUME BITMAPS ARE DWORD ALIGNED */
    /* We will offset into the source data in DWORD increments.  We  */
    /* set the source index to the remaining byte offset and         */
    /* increment the size of each line to account for the dont-care  */
    /* pixel(s).   For 4BPP source data, we also set the appropriate */
    /* nibble index.                                                 */

    /* CALCULATE THE SIZE OF ONE LINE */

    if ((gp3_src_format & GP3_CH3_SRC_MASK) == GP3_CH3_SRC_24BPP_PACKED) {
        /* HANDLE 24BPP
         * Note that we do not do anything to guarantee that the source data
         * is DWORD aligned.  The logic here is that the source data will be
         * cacheable, in which case Geode LX will not lose any clocks for
         * unaligned moves.  Also note that the channel 3 width is programmed
         * as the number of dwords, while the normal width is programmed as
         * the number of pixels.
         */

        srcoffset = srcx * 3;
        ch3_offset = 0;
        temp = width * 3;
        ch3_size = (((temp + 3) >> 2) << 16) | height;
    }
    else {
        ch3_size = size;

        if (gp3_src_pix_shift == 3) {
            /* CALCULATE INDENT AND SOURCE OFFSET */

            indent = (srcx >> 1);
            srcoffset = (indent & ~3L);
            indent &= 3;
            ch3_offset = indent | ((srcx & 1) << 25);

            temp = ((width + (srcx & 1) + 1) >> 1) + indent;
        }
        else {
            indent = (srcx << gp3_src_pix_shift);
            srcoffset = (indent & ~3L);
            indent &= 3;
            ch3_offset = indent;

            temp = (width << gp3_src_pix_shift) + indent;
        }
    }

    total_dwords = (temp + 3) >> 2;
    size_dwords = (total_dwords << 2) + 8;
    dword_count = (temp >> 2);
    byte_count = (temp & 3);

    base = ((gp3_fb_base << 24) + (dstoffset & 0xFFC00000)) |
        (gp3_base_register & ~GP3_BASE_OFFSET_DSTMASK);

    /* SET APPROPRIATE ENABLES */

    gp3_cmd_header |= GP3_BLT_HDR_DST_OFF_ENABLE |
        GP3_BLT_HDR_WIDHI_ENABLE |
        GP3_BLT_HDR_CH3_STR_ENABLE |
        GP3_BLT_HDR_CH3_OFF_ENABLE |
        GP3_BLT_HDR_CH3_WIDHI_ENABLE |
        GP3_BLT_HDR_BASE_OFFSET_ENABLE | GP3_BLT_HDR_BLT_MODE_ENABLE;

    WRITE_COMMAND32(GP3_BLT_DST_OFFSET,
                    (dstoffset & 0x3FFFFF) | gp3_pat_origin);
    WRITE_COMMAND32(GP3_BLT_CH3_OFFSET, ch3_offset);
    WRITE_COMMAND32(GP3_BLT_WID_HEIGHT, size);
    WRITE_COMMAND32(GP3_BLT_CH3_WIDHI, ch3_size);
    WRITE_COMMAND32(GP3_BLT_BASE_OFFSET, base);
    WRITE_COMMAND32(GP3_BLT_CH3_MODE_STR, GP3_CH3_C3EN |
                    GP3_CH3_REPLACE_SOURCE | GP3_CH3_HST_SRC_ENABLE |
                    gp3_src_format | ((gp3_blt_flags & CIMGP_BLTFLAGS_PRES_LUT)
                                      << 20));
    WRITE_COMMAND32(GP3_BLT_MODE, gp3_blt_mode);

    /* START THE BLT */

    WRITE_COMMAND32(GP3_BLT_CMD_HEADER, gp3_cmd_header);
    WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
    gp3_cmd_current = gp3_cmd_next;

    if (((total_dwords << 2) * height) <= GP3_BLT_1PASS_SIZE &&
        (gp3_cmd_bottom - gp3_cmd_current) > (GP3_BLT_1PASS_SIZE + 72)) {
        /* UPDATE THE COMMAND POINTER */

        cim_cmd_ptr = cim_cmd_base_ptr + gp3_cmd_current;

        /* CHECK IF A WRAP WILL BE NEEDED */

        gp3_cmd_next = gp3_cmd_current + ((total_dwords << 2) * height) + 8;

        if ((gp3_cmd_bottom - gp3_cmd_next) <= GP3_MAX_COMMAND_SIZE) {
            gp3_cmd_next = gp3_cmd_top;

            GP3_WAIT_WRAP(temp);
            WRITE_COMMAND32(0,
                            GP3_DATA_LOAD_HDR_TYPE | GP3_DATA_LOAD_HDR_WRAP |
                            GP3_DATA_LOAD_HDR_ENABLE);
        }
        else {
            GP3_WAIT_PRIMITIVE(temp);
            WRITE_COMMAND32(0,
                            GP3_DATA_LOAD_HDR_TYPE | GP3_DATA_LOAD_HDR_ENABLE);
        }

        /* WRITE DWORD COUNT */

        WRITE_COMMAND32(4, GP3_CH3_HOST_SOURCE_TYPE | (total_dwords * height));

        while (height--) {
            /* WRITE DATA */

            WRITE_CUSTOM_COMMAND_STRING32(8, data, srcoffset, dword_count);
            WRITE_CUSTOM_COMMAND_STRING8(8 + (dword_count << 2), data,
                                         srcoffset + (dword_count << 2),
                                         byte_count);

            srcoffset += pitch;
            cim_cmd_ptr += total_dwords << 2;
        }

        WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
        gp3_cmd_current = gp3_cmd_next;
    }
    else {
        /* WRITE DATA LINE BY LINE
         * Each line will be created as a separate command buffer entry to
         * allow line-by-line wrapping and to allow simultaneous rendering
         * by the HW.
         */

        while (height--) {
            /* UPDATE THE COMMAND POINTER
             * The WRITE_COMMANDXX macros use a pointer to the current buffer
             * space. This is created by adding gp3_cmd_current to the base
             * pointer.
             */

            cim_cmd_ptr = cim_cmd_base_ptr + gp3_cmd_current;

            /* CHECK IF A WRAP WILL BE NEEDED */

            gp3_cmd_next = gp3_cmd_current + size_dwords;
            if ((gp3_cmd_bottom - gp3_cmd_next) <= GP3_MAX_COMMAND_SIZE) {
                gp3_cmd_next = gp3_cmd_top;

                /* WAIT FOR HARDWARE */

                GP3_WAIT_WRAP(temp);
                WRITE_COMMAND32(0,
                                GP3_DATA_LOAD_HDR_TYPE | GP3_DATA_LOAD_HDR_WRAP
                                | GP3_DATA_LOAD_HDR_ENABLE);
            }
            else {
                /* WAIT FOR AVAILABLE SPACE */

                GP3_WAIT_PRIMITIVE(temp);
                WRITE_COMMAND32(0,
                                GP3_DATA_LOAD_HDR_TYPE |
                                GP3_DATA_LOAD_HDR_ENABLE);
            }

            /* WRITE DWORD COUNT */

            WRITE_COMMAND32(4, GP3_CH3_HOST_SOURCE_TYPE | total_dwords);

            /* WRITE DATA */

            WRITE_CUSTOM_COMMAND_STRING32(8, data, srcoffset, dword_count);
            WRITE_CUSTOM_COMMAND_STRING8(8 + (dword_count << 2), data,
                                         srcoffset + (dword_count << 2),
                                         byte_count);

            /* UPDATE POINTERS */

            srcoffset += pitch;
            WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
            gp3_cmd_current = gp3_cmd_next;
        }
    }
}

/*---------------------------------------------------------------------------
 * gp_rotate_blt
 *
 * This routine is called to rotate a rectangular area of video memory.  The
 * data may be color converted during the rotation.  'Degrees' must be a
 * multiple of 90 and indicates a clockwise rotation.  Width and height
 * refer to the width and the height of the source.  The output
 * destinations will be equal to the rotated dimensions.
 *-------------------------------------------------------------------------*/

void
gp_rotate_blt(unsigned long dstoffset, unsigned long srcoffset,
              unsigned long width, unsigned long height, int degrees)
{
    unsigned long sizein, sizeout;
    unsigned long ch3_flags;
    unsigned long base;

    base = ((gp3_fb_base << 24) + (dstoffset & 0xFFC00000)) |
        ((gp3_fb_base << 4) + ((srcoffset & 0xFFC00000) >> 20)) |
        (gp3_base_register & GP3_BASE_OFFSET_SRCMASK);

    srcoffset &= 0x3FFFFF;
    dstoffset &= 0x3FFFFF;

    /* SET ROTATION PARAMETERS */

    switch (degrees) {
    case 90:
        srcoffset += (height - 1) * gp3_src_stride;
        sizein = ((width << 16) | height);
        sizeout = ((height << 16) | width);
        ch3_flags = GP3_CH3_C3EN | GP3_CH3_REPLACE_SOURCE |
            GP3_CH3_ROTATE_ENABLE | GP3_CH3_NEG_YDIR;
        break;

    case 180:
        srcoffset += (height - 1) * gp3_src_stride;
        srcoffset += (width << gp3_src_pix_shift) - 1;
        sizein = sizeout = ((width << 16) | height);
        ch3_flags = GP3_CH3_C3EN | GP3_CH3_REPLACE_SOURCE |
            GP3_CH3_NEG_YDIR | GP3_CH3_NEG_XDIR;
        break;

    case 270:
        srcoffset += (width << gp3_src_pix_shift) - 1;
        sizein = ((width << 16) | height);
        sizeout = ((height << 16) | width);
        ch3_flags = GP3_CH3_C3EN | GP3_CH3_REPLACE_SOURCE |
            GP3_CH3_ROTATE_ENABLE | GP3_CH3_NEG_XDIR;
        break;

    default:
        sizein = sizeout = ((width << 16) | height);
        ch3_flags = GP3_CH3_C3EN | GP3_CH3_REPLACE_SOURCE;
        break;
    }

    /* SET APPROPRIATE ENABLES                           */
    /* We override the raster mode setting with a source */
    /* copy ROP.                                         */

    gp3_cmd_header |= GP3_BLT_HDR_RASTER_ENABLE |
        GP3_BLT_HDR_DST_OFF_ENABLE |
        GP3_BLT_HDR_WIDHI_ENABLE |
        GP3_BLT_HDR_CH3_OFF_ENABLE |
        GP3_BLT_HDR_CH3_STR_ENABLE |
        GP3_BLT_HDR_CH3_WIDHI_ENABLE |
        GP3_BLT_HDR_BASE_OFFSET_ENABLE | GP3_BLT_HDR_BLT_MODE_ENABLE;

    /* WRITE ALL BLT REGISTERS */

    WRITE_COMMAND32(GP3_BLT_RASTER_MODE, gp3_bpp | 0xCC);
    WRITE_COMMAND32(GP3_BLT_DST_OFFSET, dstoffset);
    WRITE_COMMAND32(GP3_BLT_WID_HEIGHT, sizeout);
    WRITE_COMMAND32(GP3_BLT_CH3_WIDHI, sizein);
    WRITE_COMMAND32(GP3_BLT_CH3_OFFSET, srcoffset);
    WRITE_COMMAND32(GP3_BLT_CH3_MODE_STR, ch3_flags | gp3_src_format |
                    ((gp3_blt_flags & CIMGP_BLTFLAGS_PRES_LUT) << 20) |
                    ((gp3_blt_flags & CIMGP_ENABLE_PREFETCH) << 17) |
                    gp3_src_stride);
    WRITE_COMMAND32(GP3_BLT_BASE_OFFSET, base);

    /* START THE BLT */

    WRITE_COMMAND32(GP3_BLT_CMD_HEADER, gp3_cmd_header);
    WRITE_COMMAND32(GP3_BLT_MODE, gp3_blt_mode);
    WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
    gp3_cmd_current = gp3_cmd_next;
}

/*---------------------------------------------------------------------------
 * gp_mono_bitmap_to_screen_blt
 *
 * This routine expands and BLTs a monchrome bitmap that is stored in system
 * memory into the framebuffer.  'data' points to an array of monochrome data.
 * 'stride' indicates the pitch between successive lines of monochrome data.
 * 'srcx' indicates the x coordinate within each line of blend data
 * corresponding to the first pixel.  A y coordinate for the source is
 * deliberately omitted to avoid extra calculation for simple cases that have
 * no y index.  The calling program must adjust the data pointer accordingly.
 *-------------------------------------------------------------------------*/

void
gp_mono_bitmap_to_screen_blt(unsigned long dstoffset, unsigned long srcx,
                             unsigned long width, unsigned long height,
                             unsigned char *data, long stride)
{
    unsigned long indent, temp;
    unsigned long total_dwords, size_dwords;
    unsigned long dword_count, byte_count;
    unsigned long size = ((width << 16) | height);
    unsigned long srcoffset, src_value;

    /* ASSUME BITMAPS ARE DWORD ALIGNED */
    /* We will offset into the source data in DWORD increments.  We */
    /* set the source index to the remaining byte offset and        */
    /* increment the size of each line to account for the dont-care */
    /* pixel(s).                                                    */

    indent = (srcx >> 3);
    srcoffset = (indent & ~3L);
    indent &= 3;
    src_value = (indent | ((srcx & 7) << 26));

    /* PROGRAM THE NORMAL SOURCE CHANNEL REGISTERS                */
    /* We assume that a color pattern is being ROPed with source  */
    /* data if the pattern type is color and the preserve pattern */
    /* was set.                                                   */

    gp3_cmd_header |= GP3_BLT_HDR_SRC_OFF_ENABLE |
        GP3_BLT_HDR_DST_OFF_ENABLE |
        GP3_BLT_HDR_WIDHI_ENABLE |
        GP3_BLT_HDR_CH3_STR_ENABLE |
        GP3_BLT_HDR_BASE_OFFSET_ENABLE |
        GP3_BLT_HDR_RASTER_ENABLE | GP3_BLT_HDR_BLT_MODE_ENABLE;

    if (gp3_ch3_pat) {
        gp3_cmd_header |=
            GP3_BLT_HDR_CH3_OFF_ENABLE | GP3_BLT_HDR_CH3_WIDHI_ENABLE;

        WRITE_COMMAND32(GP3_BLT_CH3_OFFSET, gp3_pat_origin);
        WRITE_COMMAND32(GP3_BLT_DST_OFFSET, (dstoffset & 0x3FFFFF));
        WRITE_COMMAND32(GP3_BLT_CH3_MODE_STR, gp3_pat_format);
        WRITE_COMMAND32(GP3_BLT_CH3_WIDHI, size);
    }
    else {
        WRITE_COMMAND32(GP3_BLT_DST_OFFSET,
                        ((dstoffset & 0x3FFFFF) | gp3_pat_origin));
        WRITE_COMMAND32(GP3_BLT_CH3_MODE_STR, 0);
    }
    if (gp3_blt_flags & CIMGP_BLTFLAGS_INVERTMONO) {
        WRITE_COMMAND32(GP3_BLT_RASTER_MODE,
                        gp3_raster_mode | GP3_RM_SOURCE_INVERT);
    }
    else {
        WRITE_COMMAND32(GP3_BLT_RASTER_MODE,
                        gp3_raster_mode & ~GP3_RM_SOURCE_INVERT);
    }
    WRITE_COMMAND32(GP3_BLT_SRC_OFFSET, src_value);
    WRITE_COMMAND32(GP3_BLT_WID_HEIGHT, size);
    WRITE_COMMAND32(GP3_BLT_BASE_OFFSET,
                    ((gp3_fb_base << 24) + (dstoffset & 0xFFC00000)));
    WRITE_COMMAND32(GP3_BLT_MODE,
                    gp3_blt_mode | GP3_BM_SRC_HOST | GP3_BM_SRC_MONO);

    /* START THE BLT */

    WRITE_COMMAND32(GP3_BLT_CMD_HEADER, gp3_cmd_header);
    WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
    gp3_cmd_current = gp3_cmd_next;

    /* CALCULATE THE SIZE OF ONE LINE */

    size = ((width + (srcx & 7) + 7) >> 3) + indent;
    total_dwords = (size + 3) >> 2;
    size_dwords = (total_dwords << 2) + 8;
    dword_count = (size >> 2);
    byte_count = (size & 3);

    /* CHECK FOR SMALL BLT CASE */
    /* If the total amount of monochrome data is less than 50K and we have */
    /* room in the command buffer, we will do all data writes in a single  */
    /* data packet.                                                        */

    if (((total_dwords << 2) * height) <= GP3_BLT_1PASS_SIZE &&
        (gp3_cmd_bottom - gp3_cmd_current) > (GP3_BLT_1PASS_SIZE + 72)) {
        /* UPDATE THE COMMAND POINTER */

        cim_cmd_ptr = cim_cmd_base_ptr + gp3_cmd_current;

        /* CHECK IF A WRAP WILL BE NEEDED */

        gp3_cmd_next = gp3_cmd_current + ((total_dwords << 2) * height) + 8;

        if ((gp3_cmd_bottom - gp3_cmd_next) <= GP3_MAX_COMMAND_SIZE) {
            gp3_cmd_next = gp3_cmd_top;

            GP3_WAIT_WRAP(temp);
            WRITE_COMMAND32(0,
                            GP3_DATA_LOAD_HDR_TYPE | GP3_DATA_LOAD_HDR_WRAP |
                            GP3_DATA_LOAD_HDR_ENABLE);
        }
        else {
            GP3_WAIT_PRIMITIVE(temp);
            WRITE_COMMAND32(0,
                            GP3_DATA_LOAD_HDR_TYPE | GP3_DATA_LOAD_HDR_ENABLE);
        }

        /* WRITE DWORD COUNT */

        WRITE_COMMAND32(4, GP3_HOST_SOURCE_TYPE | (total_dwords * height));

        while (height--) {
            /* WRITE DATA */

            WRITE_COMMAND_STRING32(8, data, srcoffset, dword_count);
            WRITE_COMMAND_STRING8(8 + (dword_count << 2), data,
                                  srcoffset + (dword_count << 2), byte_count);

            srcoffset += stride;
            cim_cmd_ptr += total_dwords << 2;
        }

        WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
        gp3_cmd_current = gp3_cmd_next;
    }
    else {
        /* WRITE DATA LINE BY LINE
         * Each line will be created as a separate command buffer entry to
         * allow line-by-line wrapping and to allow simultaneous rendering
         * by the HW.
         */

        while (height--) {
            /* UPDATE THE COMMAND POINTER
             * The WRITE_COMMANDXX macros use a pointer to the current buffer
             * space.  This is created by adding gp3_cmd_current to the base
             * pointer.
             */

            cim_cmd_ptr = cim_cmd_base_ptr + gp3_cmd_current;

            /* CHECK IF A WRAP WILL BE NEEDED */

            gp3_cmd_next = gp3_cmd_current + size_dwords;
            if ((gp3_cmd_bottom - gp3_cmd_next) <= GP3_MAX_COMMAND_SIZE) {
                gp3_cmd_next = gp3_cmd_top;

                /* WAIT FOR HARDWARE */

                GP3_WAIT_WRAP(temp);
                WRITE_COMMAND32(0,
                                GP3_DATA_LOAD_HDR_TYPE | GP3_DATA_LOAD_HDR_WRAP
                                | GP3_DATA_LOAD_HDR_ENABLE);
            }
            else {
                /* WAIT FOR AVAILABLE SPACE */

                GP3_WAIT_PRIMITIVE(temp);
                WRITE_COMMAND32(0,
                                GP3_DATA_LOAD_HDR_TYPE |
                                GP3_DATA_LOAD_HDR_ENABLE);
            }

            /* WRITE DWORD COUNT */

            WRITE_COMMAND32(4, GP3_HOST_SOURCE_TYPE | total_dwords);

            /* WRITE DATA */

            WRITE_COMMAND_STRING32(8, data, srcoffset, dword_count);
            WRITE_COMMAND_STRING8(8 + (dword_count << 2), data,
                                  srcoffset + (dword_count << 2), byte_count);

            /* UPDATE POINTERS */

            srcoffset += stride;
            WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
            gp3_cmd_current = gp3_cmd_next;
        }
    }
}

/*---------------------------------------------------------------------------
 * gp_text_blt
 *
 * This routine expands and BLTs byte-packed monochrome data to the screen.
 * There is assumed to be no x clipping involved in the BLT.
 *-------------------------------------------------------------------------*/

void
gp_text_blt(unsigned long dstoffset, unsigned long width,
            unsigned long height, unsigned char *data)
{
    unsigned long temp, dwords_total;
    unsigned long dword_count, byte_count;
    unsigned long size = ((width << 16) | height);
    unsigned long srcoffset = 0;

    /* PROGRAM THE NORMAL SOURCE CHANNEL REGISTERS                */
    /* We assume that a color pattern is being ROPed with source  */
    /* data if the pattern type is color and the preserve pattern */
    /* was set.                                                   */

    gp3_cmd_header |= GP3_BLT_HDR_SRC_OFF_ENABLE |
        GP3_BLT_HDR_DST_OFF_ENABLE |
        GP3_BLT_HDR_WIDHI_ENABLE |
        GP3_BLT_HDR_CH3_STR_ENABLE |
        GP3_BLT_HDR_BASE_OFFSET_ENABLE |
        GP3_BLT_HDR_RASTER_ENABLE | GP3_BLT_HDR_BLT_MODE_ENABLE;

    if (gp3_ch3_pat) {
        gp3_cmd_header |=
            GP3_BLT_HDR_CH3_OFF_ENABLE | GP3_BLT_HDR_CH3_WIDHI_ENABLE;

        WRITE_COMMAND32(GP3_BLT_CH3_OFFSET, gp3_pat_origin);
        WRITE_COMMAND32(GP3_BLT_DST_OFFSET, (dstoffset & 0x3FFFFF));
        WRITE_COMMAND32(GP3_BLT_CH3_MODE_STR, gp3_pat_format);
        WRITE_COMMAND32(GP3_BLT_CH3_WIDHI, size);
    }
    else {
        WRITE_COMMAND32(GP3_BLT_DST_OFFSET,
                        ((dstoffset & 0x3FFFFF) | gp3_pat_origin));
        WRITE_COMMAND32(GP3_BLT_CH3_MODE_STR, 0);
    }
    if (gp3_blt_flags & CIMGP_BLTFLAGS_INVERTMONO) {
        WRITE_COMMAND32(GP3_BLT_RASTER_MODE,
                        gp3_raster_mode | GP3_RM_SOURCE_INVERT);
    }
    else {
        WRITE_COMMAND32(GP3_BLT_RASTER_MODE,
                        gp3_raster_mode & ~GP3_RM_SOURCE_INVERT);
    }

    WRITE_COMMAND32(GP3_BLT_SRC_OFFSET, 0);
    WRITE_COMMAND32(GP3_BLT_WID_HEIGHT, size);
    WRITE_COMMAND32(GP3_BLT_BASE_OFFSET,
                    ((gp3_fb_base << 24) + (dstoffset & 0xFFC00000)));
    WRITE_COMMAND32(GP3_BLT_MODE,
                    gp3_blt_mode | GP3_BM_SRC_HOST | GP3_BM_SRC_BP_MONO);

    /* START THE BLT */

    WRITE_COMMAND32(GP3_BLT_CMD_HEADER, gp3_cmd_header);
    WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
    gp3_cmd_current = gp3_cmd_next;

    /* CALCULATE THE TOTAL NUMBER OF BYTES */

    size = ((width + 7) >> 3) * height;

    /* WRITE ALL DATA IN CHUNKS */

    do {
        /* UPDATE THE COMMAND POINTER */

        cim_cmd_ptr = cim_cmd_base_ptr + gp3_cmd_current;

        if (size > 8192) {
            dword_count = 2048;
            byte_count = 0;
            dwords_total = 2048;
            size -= 8192;
        }
        else {
            dword_count = (size >> 2);
            byte_count = (size & 3);
            dwords_total = (size + 3) >> 2;
            size = 0;
        }
        gp3_cmd_next = gp3_cmd_current + (dwords_total << 2) + 8;

        /* CHECK IF A WRAP WILL BE NEEDED */

        if ((gp3_cmd_bottom - gp3_cmd_next) <= GP3_MAX_COMMAND_SIZE) {
            gp3_cmd_next = gp3_cmd_top;

            /* WAIT FOR HARDWARE */

            GP3_WAIT_WRAP(temp);
            WRITE_COMMAND32(0,
                            GP3_DATA_LOAD_HDR_TYPE | GP3_DATA_LOAD_HDR_WRAP |
                            GP3_DATA_LOAD_HDR_ENABLE);
        }
        else {
            /* WAIT FOR AVAILABLE SPACE */

            GP3_WAIT_PRIMITIVE(temp);
            WRITE_COMMAND32(0,
                            GP3_DATA_LOAD_HDR_TYPE | GP3_DATA_LOAD_HDR_ENABLE);
        }

        /* WRITE DWORD COUNT */

        WRITE_COMMAND32(4, GP3_HOST_SOURCE_TYPE | dwords_total);

        /* WRITE DATA */

        WRITE_COMMAND_STRING32(8, data, srcoffset, dword_count);
        WRITE_COMMAND_STRING8(8 + (dword_count << 2), data,
                              srcoffset + (dword_count << 2), byte_count);

        WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
        gp3_cmd_current = gp3_cmd_next;

        /* UPDATE THE SOURCE OFFSET */
        /* We add a constant value because the code will loop only if the */
        /* data exceeds 8192 bytes.                                       */

        srcoffset += 8192;

    } while (size);
}

/*---------------------------------------------------------------------------
 * gp_mono_expand_blt
 *
 * This routine expands monochrome data that is stored in video memory into
 * the current BPP.  The source and destination regions are assumed not to
 * overlap.  The pitch of the source data is specified in gp_set_strides.
 * 'srcy' is deliberately omitted to prevent extra calculations for simple
 * applications that have no source indexes.
 *-------------------------------------------------------------------------*/

void
gp_mono_expand_blt(unsigned long dstoffset, unsigned long srcoffset,
                   unsigned long srcx, unsigned long width,
                   unsigned long height, int byte_packed)
{
    unsigned long base;
    unsigned long blt_mode;
    unsigned long size = ((width << 16) | height);

    /* ADJUST SOURCE OFFSET */

    srcoffset += (srcx >> 3);
    srcx &= 7;

    /* CALCULATE BASE OFFSET REGISTER */

    base = ((gp3_fb_base << 24) + (dstoffset & 0xFFC00000)) |
        ((gp3_fb_base << 14) + ((srcoffset & 0xFFC00000) >> 10)) |
        (gp3_base_register & GP3_BASE_OFFSET_CH3MASK);

    /* SET THE SOURCE TYPE */

    if (byte_packed)
        blt_mode = gp3_blt_mode | GP3_BM_SRC_FB | GP3_BM_SRC_BP_MONO;
    else
        blt_mode = gp3_blt_mode | GP3_BM_SRC_FB | GP3_BM_SRC_MONO;

    /* SET HEADER ENABLES */

    gp3_cmd_header |= GP3_BLT_HDR_SRC_OFF_ENABLE |
        GP3_BLT_HDR_DST_OFF_ENABLE |
        GP3_BLT_HDR_WIDHI_ENABLE |
        GP3_BLT_HDR_CH3_STR_ENABLE |
        GP3_BLT_HDR_BASE_OFFSET_ENABLE |
        GP3_BLT_HDR_RASTER_ENABLE | GP3_BLT_HDR_BLT_MODE_ENABLE;

    /* ENABLE COLOR PATTERN IF APPLICABLE */

    if (gp3_ch3_pat) {
        gp3_cmd_header |=
            GP3_BLT_HDR_CH3_OFF_ENABLE | GP3_BLT_HDR_CH3_WIDHI_ENABLE;

        WRITE_COMMAND32(GP3_BLT_CH3_OFFSET, gp3_pat_origin);
        WRITE_COMMAND32(GP3_BLT_DST_OFFSET, (dstoffset & 0x3FFFFF));
        WRITE_COMMAND32(GP3_BLT_CH3_MODE_STR, gp3_pat_format);
        WRITE_COMMAND32(GP3_BLT_CH3_WIDHI, size);
    }
    else {
        WRITE_COMMAND32(GP3_BLT_DST_OFFSET,
                        ((dstoffset & 0x3FFFFF) | gp3_pat_origin));
        WRITE_COMMAND32(GP3_BLT_CH3_MODE_STR, 0);
    }
    if (gp3_blt_flags & CIMGP_BLTFLAGS_INVERTMONO) {
        WRITE_COMMAND32(GP3_BLT_RASTER_MODE,
                        gp3_raster_mode | GP3_RM_SOURCE_INVERT);
    }
    else {
        WRITE_COMMAND32(GP3_BLT_RASTER_MODE,
                        gp3_raster_mode & ~GP3_RM_SOURCE_INVERT);
    }

    WRITE_COMMAND32(GP3_BLT_BASE_OFFSET, base);
    WRITE_COMMAND32(GP3_BLT_SRC_OFFSET, (srcoffset & 0x3FFFFF) | (srcx << 26));
    WRITE_COMMAND32(GP3_BLT_WID_HEIGHT, size);

    /* WORKAROUND FOR SIBZ #3744
     * Under extremely rare conditions, very narrow byte-packed mono BLTs
     * can hang the GP.  Even under the rare case, the bad condition will
     * only happen once every 16 lines.  The workaround is to break the
     * offending BLT into a series of safer BLTs.  This method is preferred
     * over a two-pass approach because it does not require saving and
     * restoring any GP state, such as the ROP or mono colors.
     */

    if ((gp3_blt_mode & GP3_BM_DST_REQ) && byte_packed && (gp3_pix_shift < 2)
        && (width < 5) && ((srcoffset & 0x1F) == 0x1F)
        && ((srcx + width) > 8)) {
        unsigned long dstoff1, size1, org1;
        unsigned long dstoff2, size2, org2;
        unsigned long tempheight;

        size1 = ((8 - srcx) << 16) | 1;
        size2 = ((width + srcx - 8) << 16) | 1;
        org1 = gp3_pat_origin;
        org2 = (org1 & 0xE0000000) | ((org1 + ((8 - srcx) << 26)) & 0x1C000000);
        dstoff1 = dstoffset & 0x3FFFFF;
        dstoff2 = (dstoff1 + 8 - srcx) << gp3_pix_shift;

        while (height) {
            /* DIVIDE THE FIRST LINE INTO TWO SINGLE LINE BLTS */

            WRITE_COMMAND32(GP3_BLT_WID_HEIGHT, size1);
            WRITE_COMMAND32(GP3_BLT_CH3_WIDHI, size1);
            WRITE_COMMAND32(GP3_BLT_SRC_OFFSET,
                            (srcoffset & 0x3FFFFF) | (srcx << 26));
            WRITE_COMMAND32(GP3_BLT_DST_OFFSET, dstoff1 | org1);
            WRITE_COMMAND32(GP3_BLT_CH3_OFFSET, org1);
            WRITE_COMMAND32(GP3_BLT_CMD_HEADER, gp3_cmd_header);
            WRITE_COMMAND32(GP3_BLT_MODE, blt_mode);
            WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
            gp3_cmd_current = gp3_cmd_next;
            gp_wait_until_idle();

            gp_declare_blt(gp3_blt_flags);
            gp3_cmd_header |= GP3_BLT_HDR_SRC_OFF_ENABLE |
                GP3_BLT_HDR_DST_OFF_ENABLE |
                GP3_BLT_HDR_WIDHI_ENABLE |
                GP3_BLT_HDR_BLT_MODE_ENABLE |
                GP3_BLT_HDR_CH3_OFF_ENABLE | GP3_BLT_HDR_CH3_WIDHI_ENABLE;
            WRITE_COMMAND32(GP3_BLT_WID_HEIGHT, size2);
            WRITE_COMMAND32(GP3_BLT_CH3_WIDHI, size2);
            WRITE_COMMAND32(GP3_BLT_SRC_OFFSET, ((srcoffset + 1) & 0x3FFFFF));
            WRITE_COMMAND32(GP3_BLT_DST_OFFSET, dstoff2 | org2);
            WRITE_COMMAND32(GP3_BLT_CH3_OFFSET, org2);
            WRITE_COMMAND32(GP3_BLT_CMD_HEADER, gp3_cmd_header);
            WRITE_COMMAND32(GP3_BLT_MODE, blt_mode);
            WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
            gp3_cmd_current = gp3_cmd_next;
            gp_wait_until_idle();

            if (--height) {
                org1 += 0x20000000;
                org2 += 0x20000000;
                dstoff1 += gp3_dst_stride;
                dstoff2 += gp3_dst_stride;
                srcoffset += 2;

                /* THE NEXT 15 LINES ARE NOW 'SAFE' - THEY DO NOT SHOW THE 
                 * PROBLEM */

                tempheight = 15;
                if (tempheight > height)
                    tempheight = height;

                gp_declare_blt(gp3_blt_flags);
                gp3_cmd_header |= GP3_BLT_HDR_SRC_OFF_ENABLE |
                    GP3_BLT_HDR_DST_OFF_ENABLE |
                    GP3_BLT_HDR_WIDHI_ENABLE |
                    GP3_BLT_HDR_BLT_MODE_ENABLE |
                    GP3_BLT_HDR_CH3_OFF_ENABLE | GP3_BLT_HDR_CH3_WIDHI_ENABLE;
                WRITE_COMMAND32(GP3_BLT_WID_HEIGHT, (width << 16) | tempheight);
                WRITE_COMMAND32(GP3_BLT_CH3_WIDHI, (width << 16) | tempheight);
                WRITE_COMMAND32(GP3_BLT_SRC_OFFSET,
                                (srcoffset & 0x3FFFFF) | (srcx << 26));
                WRITE_COMMAND32(GP3_BLT_DST_OFFSET, dstoff1 | org1);
                WRITE_COMMAND32(GP3_BLT_CH3_OFFSET, org1);
                WRITE_COMMAND32(GP3_BLT_CMD_HEADER, gp3_cmd_header);
                WRITE_COMMAND32(GP3_BLT_MODE, blt_mode);
                WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
                gp3_cmd_current = gp3_cmd_next;
                gp_wait_until_idle();

                height -= tempheight;

                if (height) {
                    gp_declare_blt(gp3_blt_flags);
                    gp3_cmd_header |= GP3_BLT_HDR_SRC_OFF_ENABLE |
                        GP3_BLT_HDR_DST_OFF_ENABLE |
                        GP3_BLT_HDR_WIDHI_ENABLE |
                        GP3_BLT_HDR_BLT_MODE_ENABLE |
                        GP3_BLT_HDR_CH3_OFF_ENABLE |
                        GP3_BLT_HDR_CH3_WIDHI_ENABLE;

                    /* ADJUST ORIGIN */
                    /* If we get here, we added a full 15 lines which is
                     * equivalent to subtracting one from the pattern y origin
                     * (adding 15). */

                    org1 -= 0x20000000;
                    org2 -= 0x20000000;
                    dstoff1 += (gp3_dst_stride * 15);
                    dstoff2 += (gp3_dst_stride * 15);
                    srcoffset += 30;
                }
            }
        }
        return;
    }

    /* START THE BLT */

    WRITE_COMMAND32(GP3_BLT_CMD_HEADER, gp3_cmd_header);
    WRITE_COMMAND32(GP3_BLT_MODE, blt_mode);
    WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
    gp3_cmd_current = gp3_cmd_next;
}

/*---------------------------------------------------------------------------
 * gp_antialiased_text
 *
 * This routine implements alpha blending between a constant source color and
 * a destination region.  The degree of the blend is controlled by an array
 * of 4BPP/8BPP values specified in 'data'.  'stride' indicates the pitch
 * between successive lines of blend data.  'srcx' indicates the x
 * coordinate within each line of blend data corresponding to the first
 * pixel.  A y coordinate for the source is deliberately omitted to avoid
 * extra calculation for simple cases that have no y index.  The calling
 * program must adjust the data pointer accordingly.  'fourbpp' selects
 * between 4BPP and 8BPP alpha.
 *-------------------------------------------------------------------------*/

void
gp_antialiased_text(unsigned long dstoffset, unsigned long srcx,
                    unsigned long width, unsigned long height,
                    unsigned char *data, long stride, int fourbpp)
{
    unsigned long indent, temp;
    unsigned long total_dwords, size_dwords;
    unsigned long dword_count, byte_count;
    unsigned long size = ((width << 16) | height);
    unsigned long ch3_offset, srcoffset;
    unsigned long base, depth_flag;

    base = ((gp3_fb_base << 24) + (dstoffset & 0xFFC00000)) |
        (gp3_base_register & ~GP3_BASE_OFFSET_DSTMASK);

    /* ENABLE ALL RELEVANT REGISTERS */
    /* We override the raster mode register to force the */
    /* correct alpha blend                               */

    gp3_cmd_header |= GP3_BLT_HDR_RASTER_ENABLE |
        GP3_BLT_HDR_DST_OFF_ENABLE |
        GP3_BLT_HDR_WIDHI_ENABLE |
        GP3_BLT_HDR_CH3_OFF_ENABLE |
        GP3_BLT_HDR_CH3_STR_ENABLE |
        GP3_BLT_HDR_CH3_WIDHI_ENABLE |
        GP3_BLT_HDR_BASE_OFFSET_ENABLE | GP3_BLT_HDR_BLT_MODE_ENABLE;

    /* CALCULATIONS BASED ON ALPHA DEPTH */
    /* Although most antialiased text is 4BPP, the hardware supports */
    /* a full 8BPP.  Either case is supported by this routine.       */

    if (fourbpp) {
        depth_flag = GP3_CH3_SRC_4BPP_ALPHA;
        indent = (srcx >> 1);
        srcoffset = (indent & ~3L);
        indent &= 3;
        ch3_offset = indent | ((srcx & 1) << 25);

        temp = ((width + (srcx & 1) + 1) >> 1) + indent;
    }
    else {
        depth_flag = GP3_CH3_SRC_8BPP_ALPHA;
        indent = srcx;
        srcoffset = (indent & ~3L);
        indent &= 3;
        ch3_offset = indent;

        temp = width + indent;
    }

    total_dwords = (temp + 3) >> 2;
    size_dwords = (total_dwords << 2) + 8;
    dword_count = (temp >> 2);
    byte_count = (temp & 3);

    /* SET RASTER MODE REGISTER */
    /* Alpha blending will only apply to RGB when no alpha component is present. */
    /* As 8BPP is not supported for this routine, the only alpha-less mode is    */
    /* 5:6:5.                                                                    */

    if (gp3_bpp == GP3_RM_BPPFMT_565) {
        WRITE_COMMAND32(GP3_BLT_RASTER_MODE,
                        gp3_bpp |
                        GP3_RM_ALPHA_TO_RGB |
                        GP3_RM_ALPHA_A_PLUS_BETA_B |
                        GP3_RM_SELECT_ALPHA_CHAN_3);
    }
    else {
        WRITE_COMMAND32(GP3_BLT_RASTER_MODE,
                        gp3_bpp |
                        GP3_RM_ALPHA_ALL |
                        GP3_RM_ALPHA_A_PLUS_BETA_B |
                        GP3_RM_SELECT_ALPHA_CHAN_3);
    }

    /* WRITE ALL REMAINING REGISTERS */

    WRITE_COMMAND32(GP3_BLT_DST_OFFSET, (dstoffset & 0x3FFFFF));
    WRITE_COMMAND32(GP3_BLT_CH3_OFFSET, ch3_offset);
    WRITE_COMMAND32(GP3_BLT_WID_HEIGHT, size);
    WRITE_COMMAND32(GP3_BLT_CH3_WIDHI, size);
    WRITE_COMMAND32(GP3_BLT_BASE_OFFSET, base);
    WRITE_COMMAND32(GP3_BLT_CH3_MODE_STR, GP3_CH3_C3EN |
                    GP3_CH3_HST_SRC_ENABLE |
                    depth_flag | ((gp3_blt_flags & CIMGP_BLTFLAGS_PRES_LUT) <<
                                  20));
    WRITE_COMMAND32(GP3_BLT_MODE, gp3_blt_mode | GP3_BM_DST_REQ);

    /* START THE BLT */

    WRITE_COMMAND32(GP3_BLT_CMD_HEADER, gp3_cmd_header);
    WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
    gp3_cmd_current = gp3_cmd_next;

    /* WRITE DATA LINE BY LINE
     * Each line will be created as a separate command buffer entry to allow
     * line-by-line wrapping and to allow simultaneous rendering by the HW.
     */

    if (((total_dwords << 2) * height) <= GP3_BLT_1PASS_SIZE &&
        (gp3_cmd_bottom - gp3_cmd_current) > (GP3_BLT_1PASS_SIZE + 72)) {
        /* UPDATE THE COMMAND POINTER */

        cim_cmd_ptr = cim_cmd_base_ptr + gp3_cmd_current;

        /* CHECK IF A WRAP WILL BE NEEDED */

        gp3_cmd_next = gp3_cmd_current + ((total_dwords << 2) * height) + 8;

        if ((gp3_cmd_bottom - gp3_cmd_next) <= GP3_MAX_COMMAND_SIZE) {
            gp3_cmd_next = gp3_cmd_top;

            GP3_WAIT_WRAP(temp);
            WRITE_COMMAND32(0,
                            GP3_DATA_LOAD_HDR_TYPE | GP3_DATA_LOAD_HDR_WRAP |
                            GP3_DATA_LOAD_HDR_ENABLE);
        }
        else {
            GP3_WAIT_PRIMITIVE(temp);
            WRITE_COMMAND32(0,
                            GP3_DATA_LOAD_HDR_TYPE | GP3_DATA_LOAD_HDR_ENABLE);
        }

        /* WRITE DWORD COUNT */

        WRITE_COMMAND32(4, GP3_CH3_HOST_SOURCE_TYPE | (total_dwords * height));

        while (height--) {
            /* WRITE DATA */

            WRITE_COMMAND_STRING32(8, data, srcoffset, dword_count);
            WRITE_COMMAND_STRING8(8 + (dword_count << 2), data,
                                  srcoffset + (dword_count << 2), byte_count);

            srcoffset += stride;
            cim_cmd_ptr += total_dwords << 2;
        }

        WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
        gp3_cmd_current = gp3_cmd_next;
    }
    else {
        while (height--) {
            /* UPDATE THE COMMAND POINTER
             * The WRITE_COMMANDXX macros use a pointer to the current buffer
             * space.  This is created by adding gp3_cmd_current to the base
             * pointer.
             */

            cim_cmd_ptr = cim_cmd_base_ptr + gp3_cmd_current;

            /* CHECK IF A WRAP WILL BE NEEDED */

            gp3_cmd_next = gp3_cmd_current + size_dwords;
            if ((gp3_cmd_bottom - gp3_cmd_next) <= GP3_MAX_COMMAND_SIZE) {
                gp3_cmd_next = gp3_cmd_top;

                /* WAIT FOR HARDWARE */

                GP3_WAIT_WRAP(temp);
                WRITE_COMMAND32(0,
                                GP3_DATA_LOAD_HDR_TYPE | GP3_DATA_LOAD_HDR_WRAP
                                | GP3_DATA_LOAD_HDR_ENABLE);
            }
            else {
                /* WAIT FOR AVAILABLE SPACE */

                GP3_WAIT_PRIMITIVE(temp);
                WRITE_COMMAND32(0,
                                GP3_DATA_LOAD_HDR_TYPE |
                                GP3_DATA_LOAD_HDR_ENABLE);
            }

            /* WRITE DWORD COUNT */

            WRITE_COMMAND32(4, GP3_CH3_HOST_SOURCE_TYPE | total_dwords);

            /* WRITE DATA */

            WRITE_COMMAND_STRING32(8, data, srcoffset, dword_count);
            WRITE_COMMAND_STRING8(8 + (dword_count << 2), data,
                                  srcoffset + (dword_count << 2), byte_count);

            /* UPDATE POINTERS */

            srcoffset += stride;
            WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
            gp3_cmd_current = gp3_cmd_next;
        }
    }
}

/*---------------------------------------------------------------------------
 * gp_masked_blt
 *
 * This routine copies source data to the screen.  A monochrome mask is used
 * to specify source transparency.
 *-------------------------------------------------------------------------*/

void
gp_masked_blt(unsigned long dstoffset, unsigned long width,
              unsigned long height, unsigned long mono_srcx,
              unsigned long color_srcx, unsigned char *mono_mask,
              unsigned char *color_data, long mono_pitch, long color_pitch)
{
    unsigned long indent, temp;
    unsigned long total_dwords, size_dwords;
    unsigned long dword_count, byte_count;
    unsigned long srcoffset, size;
    unsigned long j, ch3_offset, base;
    unsigned long flags = 0;

    if (gp3_blt_flags & CIMGP_BLTFLAGS_INVERTMONO)
        flags = GP3_RM_SOURCE_INVERT;

    /* MONO CALCULATIONS */

    indent = (mono_srcx >> 3);
    srcoffset = (indent & ~3L);
    indent &= 3;

    size = ((width + (mono_srcx & 7) + 7) >> 3) + indent;
    total_dwords = (size + 3) >> 2;
    size_dwords = (total_dwords << 2) + 8;
    dword_count = (size >> 2);
    byte_count = (size & 3);

    base = ((gp3_fb_base << 24) + (gp3_scratch_base & 0xFFC00000)) |
        (gp3_base_register & ~GP3_BASE_OFFSET_DSTMASK);

    gp3_cmd_header |= GP3_BLT_HDR_RASTER_ENABLE |
        GP3_BLT_HDR_STRIDE_ENABLE | GP3_BLT_HDR_DST_OFF_ENABLE |
        GP3_BLT_HDR_WIDHI_ENABLE | GP3_BLT_HDR_CH3_STR_ENABLE |
        GP3_BLT_HDR_CH3_OFF_ENABLE | GP3_BLT_HDR_CH3_WIDHI_ENABLE |
        GP3_BLT_HDR_BASE_OFFSET_ENABLE | GP3_BLT_HDR_BLT_MODE_ENABLE;

    WRITE_COMMAND32(GP3_BLT_RASTER_MODE, GP3_RM_BPPFMT_8888 | 0xCC);
    WRITE_COMMAND32(GP3_BLT_STRIDE, (total_dwords << 2));
    WRITE_COMMAND32(GP3_BLT_DST_OFFSET, gp3_scratch_base & 0x3FFFFF);
    WRITE_COMMAND32(GP3_BLT_WID_HEIGHT, (total_dwords << 16) | height);
    WRITE_COMMAND32(GP3_BLT_CH3_WIDHI, (total_dwords << 16) | height);
    WRITE_COMMAND32(GP3_BLT_CH3_OFFSET, 0);
    WRITE_COMMAND32(GP3_BLT_BASE_OFFSET, base);
    WRITE_COMMAND32(GP3_BLT_CH3_MODE_STR, GP3_CH3_C3EN |
                    GP3_CH3_REPLACE_SOURCE | GP3_CH3_HST_SRC_ENABLE |
                    GP3_CH3_SRC_8_8_8_8 |
                    ((gp3_blt_flags & CIMGP_BLTFLAGS_PRES_LUT) << 20));
    WRITE_COMMAND32(GP3_BLT_MODE, 0);
    WRITE_COMMAND32(GP3_BLT_CMD_HEADER, gp3_cmd_header);

    /* START THE BLT */

    WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
    gp3_cmd_current = gp3_cmd_next;

    for (j = 0; j < height; j++) {
        /* UPDATE THE COMMAND POINTER
         * The WRITE_COMMANDXX macros use a pointer to the current buffer
         * space.  This is created by adding gp3_cmd_current to the base
         * pointer.
         */

        cim_cmd_ptr = cim_cmd_base_ptr + gp3_cmd_current;

        /* CHECK IF A WRAP WILL BE NEEDED */

        gp3_cmd_next = gp3_cmd_current + size_dwords;
        if ((gp3_cmd_bottom - gp3_cmd_next) <= GP3_MAX_COMMAND_SIZE) {
            gp3_cmd_next = gp3_cmd_top;

            /* WAIT FOR HARDWARE */

            GP3_WAIT_WRAP(temp);
            WRITE_COMMAND32(0,
                            GP3_DATA_LOAD_HDR_TYPE | GP3_DATA_LOAD_HDR_WRAP |
                            GP3_DATA_LOAD_HDR_ENABLE);
        }
        else {
            /* WAIT FOR AVAILABLE SPACE */

            GP3_WAIT_PRIMITIVE(temp);
            WRITE_COMMAND32(0, GP3_DATA_LOAD_HDR_TYPE |
                            GP3_DATA_LOAD_HDR_ENABLE);
        }

        /* WRITE DWORD COUNT */

        WRITE_COMMAND32(4, GP3_CH3_HOST_SOURCE_TYPE | total_dwords);

        /* WRITE DATA */

        WRITE_COMMAND_STRING32(8, mono_mask, srcoffset, dword_count);
        WRITE_COMMAND_STRING8(8 + (dword_count << 2), mono_mask,
                              srcoffset + (dword_count << 2), byte_count);

        /* UPDATE POINTERS */

        srcoffset += mono_pitch;
        WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
        gp3_cmd_current = gp3_cmd_next;
    }

    /* SECOND BLT */

    gp_declare_blt(gp3_blt_flags | CIMGP_BLTFLAGS_HAZARD);

    base = ((gp3_fb_base << 24) + (dstoffset & 0xFFC00000)) |
        ((gp3_fb_base << 14) + (((gp3_scratch_base +
                                  indent) & 0xFFC00000) >> 10)) |
        (gp3_base_register & GP3_BASE_OFFSET_CH3MASK);

    gp3_cmd_header |= GP3_BLT_HDR_RASTER_ENABLE |
        GP3_BLT_HDR_STRIDE_ENABLE | GP3_BLT_HDR_DST_OFF_ENABLE |
        GP3_BLT_HDR_SRC_OFF_ENABLE | GP3_BLT_HDR_WIDHI_ENABLE |
        GP3_BLT_HDR_CH3_STR_ENABLE |
        GP3_BLT_HDR_CH3_WIDHI_ENABLE |
        GP3_BLT_HDR_BASE_OFFSET_ENABLE |
        GP3_BLT_HDR_CH3_OFF_ENABLE | GP3_BLT_HDR_BLT_MODE_ENABLE;

    /* ENABLE TRANSPARENCY AND PATTERN COPY ROP
     * The monochrome data is used as a mask but is otherwise not involved in
     * the BLT.  The color data is routed through the pattern channel.
     */

    WRITE_COMMAND32(GP3_BLT_RASTER_MODE,
                    gp3_bpp | 0xF0 | GP3_RM_SRC_TRANS | flags);
    WRITE_COMMAND32(GP3_BLT_STRIDE, (total_dwords << 18) | gp3_dst_stride);
    WRITE_COMMAND32(GP3_BLT_DST_OFFSET, dstoffset & 0x3FFFFF);
    WRITE_COMMAND32(GP3_BLT_SRC_OFFSET,
                    ((gp3_scratch_base +
                      indent) & 0x3FFFFF) | ((mono_srcx & 7) << 26));
    WRITE_COMMAND32(GP3_BLT_WID_HEIGHT, (width << 16) | height);
    WRITE_COMMAND32(GP3_BLT_CH3_WIDHI, (width << 16) | height);
    WRITE_COMMAND32(GP3_BLT_BASE_OFFSET, base);

    /* PROGRAM PARAMETERS FOR COLOR SOURCE DATA        */
    /* Data may be color converted along the way.      */

    if ((gp3_src_format & GP3_CH3_SRC_MASK) == GP3_CH3_SRC_24BPP_PACKED) {
        srcoffset = color_srcx * 3;
        ch3_offset = 0;
        size = width * 3;

        WRITE_COMMAND32(GP3_BLT_CH3_WIDHI, (((size + 3) >> 2) << 16) | height);
    }
    else if (gp3_src_pix_shift == 3) {
        /* CALCULATE INDENT AND SOURCE OFFSET */

        indent = (color_srcx >> 1);
        srcoffset = (indent & ~3L);
        indent &= 3;
        ch3_offset = indent | ((color_srcx & 1) << 25);

        size = ((width + (color_srcx & 1) + 1) >> 1) + indent;
    }
    else {
        indent = (color_srcx << gp3_src_pix_shift);
        srcoffset = (indent & ~3L);
        indent &= 3;
        ch3_offset = indent;

        size = (width << gp3_src_pix_shift) + indent;
    }

    total_dwords = (size + 3) >> 2;
    size_dwords = (total_dwords << 2) + 8;
    dword_count = (size >> 2);
    byte_count = (size & 3);

    WRITE_COMMAND32(GP3_BLT_CH3_OFFSET, ch3_offset);
    WRITE_COMMAND32(GP3_BLT_CH3_MODE_STR, GP3_CH3_C3EN |
                    GP3_CH3_HST_SRC_ENABLE |
                    gp3_src_format | ((gp3_blt_flags & CIMGP_BLTFLAGS_PRES_LUT)
                                      << 20));
    WRITE_COMMAND32(GP3_BLT_MODE,
                    gp3_blt_mode | GP3_BM_SRC_MONO | GP3_BM_SRC_FB);

    /* START THE BLT */

    WRITE_COMMAND32(GP3_BLT_CMD_HEADER, gp3_cmd_header);
    WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
    gp3_cmd_current = gp3_cmd_next;

    /* WRITE DATA LINE BY LINE */

    while (height--) {
        /* UPDATE THE COMMAND POINTER */

        cim_cmd_ptr = cim_cmd_base_ptr + gp3_cmd_current;

        /* CHECK IF A WRAP WILL BE NEEDED */

        gp3_cmd_next = gp3_cmd_current + size_dwords;
        if ((gp3_cmd_bottom - gp3_cmd_next) <= GP3_MAX_COMMAND_SIZE) {
            gp3_cmd_next = gp3_cmd_top;

            GP3_WAIT_WRAP(temp);
            WRITE_COMMAND32(0,
                            GP3_DATA_LOAD_HDR_TYPE | GP3_DATA_LOAD_HDR_WRAP |
                            GP3_DATA_LOAD_HDR_ENABLE);
        }
        else {
            GP3_WAIT_PRIMITIVE(temp);
            WRITE_COMMAND32(0,
                            GP3_DATA_LOAD_HDR_TYPE | GP3_DATA_LOAD_HDR_ENABLE);
        }

        /* WRITE DWORD COUNT */

        WRITE_COMMAND32(4, GP3_CH3_HOST_SOURCE_TYPE | total_dwords);

        /* WRITE COLOR DATA TO THE COMMAND BUFFER */

        WRITE_COMMAND_STRING32(8, color_data, srcoffset, dword_count);
        WRITE_COMMAND_STRING8(8 + (dword_count << 2), color_data,
                              srcoffset + (dword_count << 2), byte_count);

        /* UPDATE COMMAND BUFFER POINTERS */
        /* We do this before writing the monochrome data because otherwise */
        /* the GP could throttle the writes to the host source register    */
        /* waiting for color data.  If the command buffer has not been     */
        /* updated to load the color data...                               */

        srcoffset += color_pitch;
        WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
        gp3_cmd_current = gp3_cmd_next;
    }
}

/*---------------------------------------------------------------------------
 * gp_screen_to_screen_masked
 *
 * This routine performs a screen to screen BLT, using a monochrome mask to
 * specify source transparency.  The source data is assumed to be in the
 * current destination format and to not overlap the destination.
 *-------------------------------------------------------------------------*/

void
gp_screen_to_screen_masked(unsigned long dstoffset, unsigned long srcoffset,
                           unsigned long width, unsigned long height,
                           unsigned long mono_srcx, unsigned char *mono_mask,
                           long mono_pitch)
{
    unsigned long indent, temp;
    unsigned long total_dwords, size_dwords;
    unsigned long dword_count, byte_count;
    unsigned long srcoff, size;
    unsigned long j, base;
    unsigned long flags = 0;

    if (gp3_blt_flags & CIMGP_BLTFLAGS_INVERTMONO)
        flags = GP3_RM_SOURCE_INVERT;

    /* MONO CALCULATIONS */

    indent = (mono_srcx >> 3);
    srcoff = (indent & ~3L);
    indent &= 3;

    size = ((width + (mono_srcx & 7) + 7) >> 3) + indent;
    total_dwords = (size + 3) >> 2;
    size_dwords = (total_dwords << 2) + 8;
    dword_count = (size >> 2);
    byte_count = (size & 3);

    base = ((gp3_fb_base << 24) + (gp3_scratch_base & 0xFFC00000)) |
        (gp3_base_register & ~GP3_BASE_OFFSET_DSTMASK);

    gp3_cmd_header |= GP3_BLT_HDR_RASTER_ENABLE | GP3_BLT_HDR_STRIDE_ENABLE |
        GP3_BLT_HDR_DST_OFF_ENABLE | GP3_BLT_HDR_WIDHI_ENABLE |
        GP3_BLT_HDR_CH3_STR_ENABLE | GP3_BLT_HDR_CH3_OFF_ENABLE |
        GP3_BLT_HDR_CH3_WIDHI_ENABLE |
        GP3_BLT_HDR_BASE_OFFSET_ENABLE | GP3_BLT_HDR_BLT_MODE_ENABLE;

    WRITE_COMMAND32(GP3_BLT_RASTER_MODE, GP3_RM_BPPFMT_8888 | 0xCC);
    WRITE_COMMAND32(GP3_BLT_STRIDE, (total_dwords << 2));
    WRITE_COMMAND32(GP3_BLT_DST_OFFSET, gp3_scratch_base & 0x3FFFFF);
    WRITE_COMMAND32(GP3_BLT_WID_HEIGHT, (total_dwords << 16) | height);
    WRITE_COMMAND32(GP3_BLT_CH3_WIDHI, (total_dwords << 16) | height);
    WRITE_COMMAND32(GP3_BLT_CH3_OFFSET, 0);
    WRITE_COMMAND32(GP3_BLT_BASE_OFFSET, base);
    WRITE_COMMAND32(GP3_BLT_CH3_MODE_STR, GP3_CH3_C3EN |
                    GP3_CH3_REPLACE_SOURCE | GP3_CH3_HST_SRC_ENABLE |
                    GP3_CH3_SRC_8_8_8_8 |
                    ((gp3_blt_flags & CIMGP_BLTFLAGS_PRES_LUT) << 20));
    WRITE_COMMAND32(GP3_BLT_MODE, 0);
    WRITE_COMMAND32(GP3_BLT_CMD_HEADER, gp3_cmd_header);

    /* START THE BLT */

    WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
    gp3_cmd_current = gp3_cmd_next;

    for (j = 0; j < height; j++) {
        /* UPDATE THE COMMAND POINTER
         * The WRITE_COMMANDXX macros use a pointer to the current buffer
         * space. This is created by adding gp3_cmd_current to the base
         * pointer.
         */

        cim_cmd_ptr = cim_cmd_base_ptr + gp3_cmd_current;

        /* CHECK IF A WRAP WILL BE NEEDED */

        gp3_cmd_next = gp3_cmd_current + size_dwords;
        if ((gp3_cmd_bottom - gp3_cmd_next) <= GP3_MAX_COMMAND_SIZE) {
            gp3_cmd_next = gp3_cmd_top;

            /* WAIT FOR HARDWARE */

            GP3_WAIT_WRAP(temp);
            WRITE_COMMAND32(0, GP3_DATA_LOAD_HDR_TYPE |
                            GP3_DATA_LOAD_HDR_WRAP | GP3_DATA_LOAD_HDR_ENABLE);
        }
        else {
            /* WAIT FOR AVAILABLE SPACE */

            GP3_WAIT_PRIMITIVE(temp);
            WRITE_COMMAND32(0, GP3_DATA_LOAD_HDR_TYPE |
                            GP3_DATA_LOAD_HDR_ENABLE);
        }

        /* WRITE DWORD COUNT */

        WRITE_COMMAND32(4, GP3_CH3_HOST_SOURCE_TYPE | total_dwords);

        /* WRITE DATA */

        WRITE_COMMAND_STRING32(8, mono_mask, srcoff, dword_count);
        WRITE_COMMAND_STRING8(8 + (dword_count << 2), mono_mask,
                              srcoff + (dword_count << 2), byte_count);

        /* UPDATE POINTERS */

        srcoff += mono_pitch;
        WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
        gp3_cmd_current = gp3_cmd_next;
    }

    /* SECOND BLT */

    gp_declare_blt(gp3_blt_flags | CIMGP_BLTFLAGS_HAZARD);

    base = ((gp3_fb_base << 24) + (dstoffset & 0xFFC00000)) |
        ((gp3_fb_base << 14) + (((gp3_scratch_base +
                                  indent) & 0xFFC00000) >> 10)) | ((gp3_fb_base
                                                                    << 4) +
                                                                   ((srcoffset &
                                                                     0xFFC00000)
                                                                    >> 20));

    gp3_cmd_header |= GP3_BLT_HDR_RASTER_ENABLE |
        GP3_BLT_HDR_STRIDE_ENABLE | GP3_BLT_HDR_DST_OFF_ENABLE |
        GP3_BLT_HDR_SRC_OFF_ENABLE | GP3_BLT_HDR_WIDHI_ENABLE |
        GP3_BLT_HDR_CH3_STR_ENABLE |
        GP3_BLT_HDR_CH3_WIDHI_ENABLE |
        GP3_BLT_HDR_BASE_OFFSET_ENABLE |
        GP3_BLT_HDR_CH3_OFF_ENABLE | GP3_BLT_HDR_BLT_MODE_ENABLE;

    /* ENABLE TRANSPARENCY AND PATTERN COPY ROP
     * The monochrome data is used as a mask but is otherwise not involved
     * in the BLT.  The color data is routed through the pattern channel.
     */

    WRITE_COMMAND32(GP3_BLT_RASTER_MODE,
                    gp3_bpp | 0xF0 | GP3_RM_SRC_TRANS | flags);
    WRITE_COMMAND32(GP3_BLT_STRIDE, (total_dwords << 18) | gp3_dst_stride);
    WRITE_COMMAND32(GP3_BLT_DST_OFFSET, dstoffset & 0x3FFFFF);
    WRITE_COMMAND32(GP3_BLT_SRC_OFFSET,
                    ((gp3_scratch_base +
                      indent) & 0x3FFFFF) | ((mono_srcx & 7) << 26));
    WRITE_COMMAND32(GP3_BLT_WID_HEIGHT, (width << 16) | height);
    WRITE_COMMAND32(GP3_BLT_CH3_WIDHI, (width << 16) | height);
    WRITE_COMMAND32(GP3_BLT_BASE_OFFSET, base);

    /* PROGRAM PARAMETERS FOR COLOR SOURCE DATA  */

    WRITE_COMMAND32(GP3_BLT_CH3_OFFSET, srcoffset & 0x3FFFFF);
    WRITE_COMMAND32(GP3_BLT_CH3_MODE_STR, GP3_CH3_C3EN | gp3_ch3_bpp |
                    gp3_src_stride | ((gp3_blt_flags & CIMGP_BLTFLAGS_PRES_LUT)
                                      << 20));
    WRITE_COMMAND32(GP3_BLT_MODE,
                    gp3_blt_mode | GP3_BM_SRC_MONO | GP3_BM_SRC_FB);

    /* START THE BLT */

    WRITE_COMMAND32(GP3_BLT_CMD_HEADER, gp3_cmd_header);
    WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
    gp3_cmd_current = gp3_cmd_next;
}

/*---------------------------------------------------------------------------
 * gp_bresenham_line
 *
 * This routine draws a vector using the specified Bresenham parameters.
 * Currently this file does not support a routine that accepts the two
 * endpoints of a vector and calculates the Bresenham parameters.  If it
 * ever does, this routine is still required for vectors that have been
 * clipped.
 *-------------------------------------------------------------------------*/

void
gp_bresenham_line(unsigned long dstoffset, unsigned short length,
                  unsigned short initerr, unsigned short axialerr,
                  unsigned short diagerr, unsigned long flags)
{
    unsigned long base;
    long offset;

    /* HANDLE NEGATIVE VECTORS */
    /* We have to be very careful with vectors that increment negatively   */
    /* Our framebuffer scheme tries to align the destination of every      */
    /* BLT or vector to the nearest 4MB-aligned boundary.  This is         */
    /* necessary because the GP only supports offsets up to 16MB, but the  */
    /* framebuffer can be over 128MB.  To solve this problem, the GP       */
    /* base address registers are alignable to 4MB regions.  However, we   */
    /* cannot simply align the dest offset when the vector goes negative.  */
    /* The vector offset could underflow, causing the offset to jump from  */
    /* near 0 to 16MB.  As we cannot accurately determine the last address */
    /* that will be written in a vector short of walking the algorithm in  */
    /* software, we do a worst case approximation.                         */

    offset = dstoffset;
    if (!(flags & CIMGP_POSMAJOR)) {
        if (flags & CIMGP_YMAJOR)
            offset -= length * gp3_dst_stride;
        else
            offset -= (length << gp3_pix_shift);

        if (offset < 0)
            offset = 0;
    }
    if (!(flags & CIMGP_POSMINOR)) {
        if (flags & CIMGP_YMAJOR)
            offset -= (length << gp3_pix_shift);
        else
            offset -= length * gp3_dst_stride;

        if (offset < 0)
            offset = 0;
    }

    offset &= 0xFFC00000;
    dstoffset -= offset;

    base = ((gp3_fb_base << 24) + offset) |
        (gp3_base_register & ~GP3_BASE_OFFSET_DSTMASK);

    /* ENABLE RELEVANT REGISTERS */
    /* Note that we always enable and write the channel 3 mode, if only */
    /* to turn it off.   Cimarron also always writes the base offset    */
    /* register to allow operation with frame buffers larger than 16MB. */

    gp3_cmd_header |= GP3_VEC_HDR_DST_OFF_ENABLE |
        GP3_VEC_HDR_VEC_ERR_ENABLE |
        GP3_VEC_HDR_VEC_LEN_ENABLE |
        GP3_VEC_HDR_BASE_OFFSET_ENABLE |
        GP3_VEC_HDR_CH3_STR_ENABLE | GP3_VEC_HDR_VEC_MODE_ENABLE;

    /* WRITE THE REGISTERS COMMON TO ALL PATTERN TYPES */
    /* The destination base is the frame buffer base plus whatever */
    /* 4MB segment we happen to be drawing to.                     */

    WRITE_COMMAND32(GP3_VECTOR_VEC_ERR,
                    (((unsigned long) axialerr << 16) | (unsigned long)
                     diagerr));
    WRITE_COMMAND32(GP3_VECTOR_VEC_LEN,
                    (((unsigned long) length << 16) | (unsigned long) initerr));
    WRITE_COMMAND32(GP3_VECTOR_BASE_OFFSET, base);

    /* CHECK VECTOR PATTERN CASE */

    if (gp3_ch3_pat) {
        /* SET THE SOLID COLOR */
        /* The color for vector patterns from channel 3 comes from */
        /* the regular pattern registers.                          */

        gp3_cmd_header |= GP3_VEC_HDR_PAT_CLR0_ENABLE;

        WRITE_COMMAND32(GP3_VECTOR_PAT_COLOR_0, gp3_vector_pattern_color);

        /* INITIALIZE CHANNEL 3 PARAMETERS
         * We route the channel 3 output to the old source channel.  If the
         * user sets a ROP that involves source, they will get unexpected
         * results.
         */

        WRITE_COMMAND32(GP3_VECTOR_DST_OFFSET, dstoffset);
        WRITE_COMMAND32(GP3_VECTOR_CH3_MODE_STR,
                        GP3_CH3_C3EN | GP3_CH3_REPLACE_SOURCE |
                        GP3_CH3_COLOR_PAT_ENABLE | GP3_CH3_SRC_8_8_8_8);
    }
    else {
        /* DISABLE CHANNEL 3 AND USE NORMAL PATTERN ORIGINS */

        WRITE_COMMAND32(GP3_VECTOR_CH3_MODE_STR, 0);
        WRITE_COMMAND32(GP3_VECTOR_DST_OFFSET, (dstoffset | gp3_pat_origin));
    }

    /* START THE VECTOR */

    WRITE_COMMAND32(GP3_VEC_CMD_HEADER, gp3_cmd_header);
    WRITE_COMMAND32(GP3_VECTOR_MODE, (gp3_vec_mode | flags));
    WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);

    gp3_cmd_current = gp3_cmd_next;

    /* ADD A SECOND VECTOR TO CLEAR THE BYTE ENABLES            */
    /* We set a transparent pattern to clear the byte enables.  */
    /* We then restore the previous pattern. (SiBZ #4001)       */

    if (gp3_ch3_pat) {
        cim_cmd_ptr = cim_cmd_base_ptr + gp3_cmd_current;

        WRITE_COMMAND32(0, GP3_LUT_HDR_TYPE | GP3_LUT_HDR_DATA_ENABLE);
        WRITE_COMMAND32(4, 0x100);
        WRITE_COMMAND32(8, (1 | GP3_LUT_DATA_TYPE));
        WRITE_COMMAND32(12, 0);

        /* DUMMY VECTOR */
        /* We shouldn't need to write anything but vector mode and the length
         */

        WRITE_COMMAND32(16, GP3_VEC_HDR_TYPE | GP3_VEC_HDR_VEC_MODE_ENABLE |
                        GP3_VEC_HDR_VEC_LEN_ENABLE);
        WRITE_COMMAND32(16 + GP3_VECTOR_MODE, (gp3_vec_mode | flags));
        WRITE_COMMAND32(16 + GP3_VECTOR_VEC_LEN,
                        (1 << 16) | (unsigned long) initerr);

        WRITE_COMMAND32(16 + GP3_VECTOR_COMMAND_SIZE,
                        GP3_LUT_HDR_TYPE | GP3_LUT_HDR_DATA_ENABLE);
        WRITE_COMMAND32(20 + GP3_VECTOR_COMMAND_SIZE, 0x100);
        WRITE_COMMAND32(24 + GP3_VECTOR_COMMAND_SIZE, (1 | GP3_LUT_DATA_TYPE));
        WRITE_COMMAND32(28 + GP3_VECTOR_COMMAND_SIZE, gp3_vec_pat);

        gp3_cmd_current += 32 + GP3_VECTOR_COMMAND_SIZE;
    }
}

/*---------------------------------------------------------------------------
 * gp_line_from_endpoints
 *
 * This routine draws a vector from a set of rectangular coordinates.  The
 * rectangle is assumed to use the currently specified destination stride.
 *-------------------------------------------------------------------------*/

void
gp_line_from_endpoints(unsigned long dstoffset, unsigned long x0,
                       unsigned long y0, unsigned long x1, unsigned long y1,
                       int inclusive)
{
    unsigned long base;
    unsigned long length;
    unsigned long flags;
    unsigned short initerr, axialerr, diagerr;
    long dx, dy, dmaj, dmin;
    long offset;

    /* ADJUST DESTINATION OFFSET BASED ON STARTING COORDINATE */

    dstoffset += (x0 << gp3_pix_shift) + (y0 * gp3_dst_stride);

    /* CALCULATE BRESENHAM TERMS */

    dx = (long) x1 - (long) x0;
    dy = (long) y1 - (long) y0;
    if (dx < 0)
        dx = -dx;
    if (dy < 0)
        dy = -dy;

    if (dx >= dy) {
        dmaj = dx;
        dmin = dy;
        flags = 0;
        if (x1 > x0)
            flags |= CIMGP_POSMAJOR;
        if (y1 > y0)
            flags |= CIMGP_POSMINOR;
    }
    else {
        dmaj = dy;
        dmin = dx;
        flags = CIMGP_YMAJOR;
        if (x1 > x0)
            flags |= CIMGP_POSMINOR;
        if (y1 > y0)
            flags |= CIMGP_POSMAJOR;
    }

    axialerr = (unsigned short) (dmin << 1);
    diagerr = (unsigned short) ((dmin - dmaj) << 1);
    initerr = (unsigned short) ((dmin << 1) - dmaj);
    if (!(flags & CIMGP_POSMINOR))
        initerr--;

    /* CHECK FOR NO WORK */

    if (!dmaj)
        return;

    /* CHECK INCLUSIVE OR EXCLUSIVE */
    /* An inclusive line can be accomplished by simply adding 1 to the */
    /* line length.                                                    */

    length = dmaj;
    if (inclusive)
        length++;

    /* HANDLE NEGATIVE VECTORS */

    offset = dstoffset;
    if (!(flags & CIMGP_POSMAJOR)) {
        if (flags & CIMGP_YMAJOR)
            offset -= length * gp3_dst_stride;
        else
            offset -= (length << gp3_pix_shift);

        if (offset < 0)
            offset = 0;
    }
    if (!(flags & CIMGP_POSMINOR)) {
        if (flags & CIMGP_YMAJOR)
            offset -= (length << gp3_pix_shift);
        else
            offset -= length * gp3_dst_stride;

        if (offset < 0)
            offset = 0;
    }

    offset &= 0xFFC00000;
    dstoffset -= offset;

    base = ((gp3_fb_base << 24) + offset) |
        (gp3_base_register & ~GP3_BASE_OFFSET_DSTMASK);

    /* ENABLE RELEVANT REGISTERS */
    /* Note that we always enable and write the channel 3 mode, if only */
    /* to turn it off.   Cimarron also always writes the base offset    */
    /* register to allow operation with frame buffers larger than 16MB. */

    gp3_cmd_header |= GP3_VEC_HDR_DST_OFF_ENABLE |
        GP3_VEC_HDR_VEC_ERR_ENABLE |
        GP3_VEC_HDR_VEC_LEN_ENABLE |
        GP3_VEC_HDR_BASE_OFFSET_ENABLE |
        GP3_VEC_HDR_CH3_STR_ENABLE | GP3_VEC_HDR_VEC_MODE_ENABLE;

    /* WRITE THE REGISTERS COMMON TO ALL PATTERN TYPES */
    /* The destination base is the frame buffer base plus whatever */
    /* 4MB segment we happen to be drawing to.                     */

    WRITE_COMMAND32(GP3_VECTOR_VEC_ERR,
                    (((unsigned long) axialerr << 16) | (unsigned long)
                     diagerr));
    WRITE_COMMAND32(GP3_VECTOR_VEC_LEN,
                    (((unsigned long) length << 16) | (unsigned long) initerr));
    WRITE_COMMAND32(GP3_VECTOR_BASE_OFFSET, base);

    /* CHECK VECTOR PATTERN CASE */

    if (gp3_ch3_pat) {
        /* SET THE SOLID COLOR */
        /* The color for vector patterns from channel 3 comes from */
        /* the regular pattern registers.                          */

        gp3_cmd_header |= GP3_VEC_HDR_PAT_CLR0_ENABLE;

        WRITE_COMMAND32(GP3_VECTOR_PAT_COLOR_0, gp3_vector_pattern_color);

        /* INITIALIZE CHANNEL 3 PARAMETERS */
        /* We route the channel 3 output to the old source channel.  If the
         * user sets a ROP that involves source, they will get unexpected
         * results.
         */

        WRITE_COMMAND32(GP3_VECTOR_DST_OFFSET, dstoffset);
        WRITE_COMMAND32(GP3_VECTOR_CH3_MODE_STR,
                        GP3_CH3_C3EN | GP3_CH3_REPLACE_SOURCE |
                        GP3_CH3_COLOR_PAT_ENABLE | GP3_CH3_SRC_8_8_8_8);
    }
    else {
        /* DISABLE CHANNEL 3 AND USE NORMAL PATTERN ORIGINS */

        WRITE_COMMAND32(GP3_VECTOR_CH3_MODE_STR, 0);
        WRITE_COMMAND32(GP3_VECTOR_DST_OFFSET, (dstoffset | gp3_pat_origin));
    }

    /* START THE VECTOR */

    WRITE_COMMAND32(GP3_VEC_CMD_HEADER, gp3_cmd_header);
    WRITE_COMMAND32(GP3_VECTOR_MODE, (gp3_vec_mode | flags));
    WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
    gp3_cmd_current = gp3_cmd_next;

    /* ADD A SECOND VECTOR TO CLEAR THE BYTE ENABLES            */
    /* We set a transparent pattern to clear the byte enables.  */
    /* We then restore the previous pattern.  (SiBZ #4001)      */

    if (gp3_ch3_pat) {
        cim_cmd_ptr = cim_cmd_base_ptr + gp3_cmd_current;

        WRITE_COMMAND32(0, GP3_LUT_HDR_TYPE | GP3_LUT_HDR_DATA_ENABLE);
        WRITE_COMMAND32(4, 0x100);
        WRITE_COMMAND32(8, (1 | GP3_LUT_DATA_TYPE));
        WRITE_COMMAND32(12, 0);

        /* DUMMY VECTOR */
        /* We shouldn't need to write anything but vector mode and the length
         */

        WRITE_COMMAND32(16, GP3_VEC_HDR_TYPE | GP3_VEC_HDR_VEC_MODE_ENABLE |
                        GP3_VEC_HDR_VEC_LEN_ENABLE);
        WRITE_COMMAND32(16 + GP3_VECTOR_MODE, (gp3_vec_mode | flags));
        WRITE_COMMAND32(16 + GP3_VECTOR_VEC_LEN,
                        (1 << 16) | (unsigned long) initerr);

        WRITE_COMMAND32(16 + GP3_VECTOR_COMMAND_SIZE,
                        GP3_LUT_HDR_TYPE | GP3_LUT_HDR_DATA_ENABLE);
        WRITE_COMMAND32(20 + GP3_VECTOR_COMMAND_SIZE, 0x100);
        WRITE_COMMAND32(24 + GP3_VECTOR_COMMAND_SIZE, (1 | GP3_LUT_DATA_TYPE));
        WRITE_COMMAND32(28 + GP3_VECTOR_COMMAND_SIZE, gp3_vec_pat);

        gp3_cmd_current += 32 + GP3_VECTOR_COMMAND_SIZE;
    }
}

/*---------------------------------------------------------------------------
 * gp_wait_until_idle
 *
 * This routine stalls execution until the GP is no longer actively rendering.
 *-------------------------------------------------------------------------*/

void
gp_wait_until_idle(void)
{
    unsigned long temp;

    while (((temp = READ_GP32(GP3_BLT_STATUS)) & GP3_BS_BLT_BUSY) ||
           !(temp & GP3_BS_CB_EMPTY)) {
        ;
    }
}

/*---------------------------------------------------------------------------
 * gp_test_blt_busy
 *-------------------------------------------------------------------------*/

int
gp_test_blt_busy(void)
{
    unsigned long temp;

    if (((temp = READ_GP32(GP3_BLT_STATUS)) & GP3_BS_BLT_BUSY) ||
        !(temp & GP3_BS_CB_EMPTY))
        return 1;

    return 0;
}

/*---------------------------------------------------------------------------
 * gp_test_blt_pending
 *-------------------------------------------------------------------------*/

int
gp_test_blt_pending(void)
{
    if ((READ_GP32(GP3_BLT_STATUS)) & GP3_BS_BLT_PENDING)
        return 1;

    return 0;
}

/*---------------------------------------------------------------------------
 * gp_wait_blt_pending
 *-------------------------------------------------------------------------*/

void
gp_wait_blt_pending(void)
{
    while ((READ_GP32(GP3_BLT_STATUS)) & GP3_BS_BLT_PENDING);
}

/*---------------------------------------------------------------------------
 * gp_save_state
 *
 * This routine saves all persistent GP information.
 *-------------------------------------------------------------------------*/

void
gp_save_state(GP_SAVE_RESTORE *gp_state)
{
    Q_WORD msr_value = { 0, 0 };

    gp_wait_until_idle();

    msr_read64(MSR_DEVICE_GEODELX_GP, MSR_GEODELINK_CONFIG, &msr_value);
    gp_state->cmd_bottom = READ_GP32(GP3_CMD_BOT) & 0xFFFFFF;
    gp_state->cmd_top = READ_GP32(GP3_CMD_TOP) & 0xFFFFFF;
    gp_state->cmd_base = (msr_value.low << 4) & 0xFFF00000;
    gp_state->base_offset = READ_GP32(GP3_BASE_OFFSET);

    /* RESET THE READ POINTER */

    gp_set_command_buffer_base(gp_state->cmd_base, gp_state->cmd_top,
                               gp_state->cmd_bottom);
}

/*---------------------------------------------------------------------------
 * gp_restore_state
 *
 * This routine restores all persistent GP information.
 *-------------------------------------------------------------------------*/

void
gp_restore_state(GP_SAVE_RESTORE *gp_state)
{
    gp_wait_until_idle();

    WRITE_GP32(GP3_BASE_OFFSET, gp_state->base_offset);

    gp_set_command_buffer_base(gp_state->cmd_base, gp_state->cmd_top,
                               gp_state->cmd_bottom);
}

/* This is identical to gp_antialiased_text, except we support all one
   pass alpha operations similar to gp_set_alpha_operation */

void
gp_blend_mask_blt(unsigned long dstoffset, unsigned long srcx,
                  unsigned long width, unsigned long height,
                  unsigned long dataoffset, long stride, int operation,
                  int fourbpp)
{
    unsigned long indent;
    unsigned long size = ((width << 16) | height);
    unsigned long base, depth_flag;

    base = ((gp3_fb_base << 24) + (dstoffset & 0xFFC00000)) |
        ((gp3_fb_base << 4) + (dataoffset >> 20)) |
        (gp3_base_register & GP3_BASE_OFFSET_SRCMASK);

    /* ENABLE ALL RELEVANT REGISTERS */
    /* We override the raster mode register to force the */
    /* correct alpha blend                               */

    gp3_cmd_header |= GP3_BLT_HDR_RASTER_ENABLE |
        GP3_BLT_HDR_DST_OFF_ENABLE |
        GP3_BLT_HDR_WIDHI_ENABLE |
        GP3_BLT_HDR_CH3_OFF_ENABLE |
        GP3_BLT_HDR_CH3_STR_ENABLE |
        GP3_BLT_HDR_CH3_WIDHI_ENABLE |
        GP3_BLT_HDR_BASE_OFFSET_ENABLE | GP3_BLT_HDR_BLT_MODE_ENABLE;

    /* CALCULATIONS BASED ON ALPHA DEPTH */
    /* Although most antialiased text is 4BPP, the hardware supports */
    /* a full 8BPP.  Either case is supported by this routine.       */

    if (fourbpp) {
        depth_flag = GP3_CH3_SRC_4BPP_ALPHA;
        indent = (srcx >> 1) & 3;
        dataoffset += indent | ((srcx & 1) << 25);
    }
    else {
        depth_flag = GP3_CH3_SRC_8BPP_ALPHA;
        indent = srcx & 3;
        dataoffset += indent;
    }

    /* SET RASTER MODE REGISTER */
    /* Alpha blending will only apply to RGB when no alpha component is present. */
    /* As 8BPP is not supported for this routine, the only alpha-less mode is    */
    /* 5:6:5.                                                                    */

    if (gp3_bpp == GP3_RM_BPPFMT_565) {
        WRITE_COMMAND32(GP3_BLT_RASTER_MODE,
                        gp3_bpp |
                        GP3_RM_ALPHA_TO_RGB |
                        ((unsigned long) (operation << 20)) |
                        GP3_RM_SELECT_ALPHA_CHAN_3);
    }
    else {
        WRITE_COMMAND32(GP3_BLT_RASTER_MODE,
                        gp3_bpp |
                        GP3_RM_ALPHA_ALL | ((unsigned long) (operation << 20)) |
                        GP3_RM_SELECT_ALPHA_CHAN_3);
    }

    /* WRITE ALL REMAINING REGISTERS */

    WRITE_COMMAND32(GP3_BLT_DST_OFFSET, (dstoffset & 0x3FFFFF));

    /* Set the offset of the CH3 data in memory */
    WRITE_COMMAND32(GP3_BLT_CH3_OFFSET, dataoffset & 0x3FFFFF);

    WRITE_COMMAND32(GP3_BLT_WID_HEIGHT, size);
    WRITE_COMMAND32(GP3_BLT_CH3_WIDHI, size);
    WRITE_COMMAND32(GP3_BLT_BASE_OFFSET, base);

    WRITE_COMMAND32(GP3_BLT_CH3_MODE_STR, GP3_CH3_C3EN | (stride & 0xFFFF) |
                    depth_flag | ((gp3_blt_flags & CIMGP_BLTFLAGS_PRES_LUT) <<
                                  20));

    WRITE_COMMAND32(GP3_BLT_MODE, GP3_BM_DST_REQ);

    /* START THE BLT */

    WRITE_COMMAND32(GP3_BLT_CMD_HEADER, gp3_cmd_header);
    WRITE_GP32(GP3_CMD_WRITE, gp3_cmd_next);
    gp3_cmd_current = gp3_cmd_next;
}
