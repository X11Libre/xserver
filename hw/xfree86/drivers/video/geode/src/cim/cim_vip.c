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
  * Cimarron VIP configuration routines.
  */

/*---------------------------------------------------------------------------
 * vip_initialize
 *
 * This routine initializes the internal module state and prepares the
 * module for subsequent VIP orientated activities.
 *--------------------------------------------------------------------------*/

int
vip_initialize(VIPSETMODEBUFFER *buffer)
{
    unsigned long vip_control1, vip_control2, vip_control3;

    if (!buffer)
        return CIM_STATUS_INVALIDPARAMS;

    vip_control1 = 0;
    vip_control2 = 0;
    vip_control3 = 0;

    /* CONFIGURE CONTROL WORDS BASED ON MODE STRUCTURE                  */
    /* Note that some of the input parameters match the register fields */
    /* they represent.                                                  */

    /* STREAM ENABLES */

    vip_control1 |= buffer->stream_enables;

    /* VIP CAPTURE MODE */

    vip_control1 |= buffer->operating_mode;

    /* HANDLE PLANAR CAPTURE */

    if (buffer->flags & VIP_MODEFLAG_PLANARCAPTURE) {
        vip_control1 |= VIP_CONTROL1_PLANAR;

        if (buffer->planar_capture == VIP_420CAPTURE_EVERYLINE) {
            vip_control1 |= VIP_CONTROL1_DISABLE_DECIMATION;
        }
        else if (buffer->planar_capture == VIP_420CAPTURE_ALTERNATINGFIELDS) {
            if (buffer->flags & VIP_MODEFLAG_PROGRESSIVE)
                return CIM_STATUS_INVALIDPARAMS;

            vip_control1 |= VIP_CONTROL1_DISABLE_DECIMATION;
            vip_control3 |= VIP_CONTROL3_DECIMATE_EVEN;
        }
        else if (buffer->planar_capture != VIP_420CAPTURE_ALTERNATINGLINES)
            return CIM_STATUS_INVALIDPARAMS;

        /* CONFIGURE THE VIDEO FIFO THRESHOLD BASED ON THE FIFO DEPTH */

        vip_control2 |= VIP_CONTROL2_DEFAULT_VIDTH_420 <<
            VIP_CONTROL2_VIDTH_SHIFT;

    }
    else {
        vip_control2 |= VIP_CONTROL2_DEFAULT_VIDTH_422 <<
            VIP_CONTROL2_VIDTH_SHIFT;
    }

    /* CONFIGURE DEFAULT ANCILARRY THRESHOLD AND VIDEO FLUSH VALUES */

    vip_control2 |= VIP_CONTROL2_DEFAULT_ANCTH << VIP_CONTROL2_ANCTH_SHIFT;
    vip_control1 |= VIP_CONTROL1_DEFAULT_ANC_FF << VIP_CONTROL1_ANC_FF_SHIFT;
    vip_control1 |= VIP_CONTROL1_DEFAULT_VID_FF << VIP_CONTROL1_VID_FF_SHIFT;

    /* PROGRAM VIP OPTIONS */
    /* The options are sanitized based on the current configuration. */

    if (buffer->flags & VIP_MODEFLAG_PROGRESSIVE)
        vip_control1 |= VIP_CONTROL1_NON_INTERLACED;
    else {
        if (buffer->flags & VIP_MODEFLAG_TOGGLEEACHFIELD)
            vip_control3 |= VIP_CONTROL3_BASE_UPDATE;
        if (buffer->flags & VIP_MODEFLAG_INVERTPOLARITY)
            vip_control2 |= VIP_CONTROL2_INVERT_POLARITY;
    }

    if ((buffer->operating_mode == VIP_MODE_MSG ||
         buffer->operating_mode == VIP_MODE_DATA) &&
        (buffer->flags & VIP_MODEFLAG_FLIPMESSAGEWHENFULL)) {
        vip_control1 |= VIP_CONTROL1_MSG_STRM_CTRL;
    }

    else if (buffer->operating_mode == VIP_MODE_VIP2_8BIT ||
             buffer->operating_mode == VIP_MODE_VIP2_16BIT) {
        if (buffer->flags & VIP_MODEFLAG_ENABLEREPEATFLAG)
            vip_control2 |= VIP_CONTROL2_REPEAT_ENABLE;
        if (buffer->flags & VIP_MODEFLAG_INVERTTASKPOLARITY)
            vip_control3 |= VIP_CONTROL3_TASK_POLARITY;
    }

    if (buffer->flags & VIP_MODEFLAG_DISABLEZERODETECT)
        vip_control1 |= VIP_CONTROL1_DISABLE_ZERO_DETECT;
    if (buffer->flags & VIP_MODEFLAG_10BITANCILLARY)
        vip_control2 |= VIP_CONTROL2_ANC10;

    /* WRITE THE CONTROL REGISTERS */
    /* The control registers are kept 'live' to allow separate instances of */
    /* Cimarron to control the VIP hardware.                                */

    WRITE_VIP32(VIP_CONTROL1, vip_control1);
    WRITE_VIP32(VIP_CONTROL2, vip_control2);
    WRITE_VIP32(VIP_CONTROL3, vip_control3);

    /* CONFIGURE 601 PARAMETERS */

    if (buffer->operating_mode == VIP_MODE_8BIT601 ||
        buffer->operating_mode == VIP_MODE_16BIT601) {
        vip_update_601_params(&buffer->vip601_settings);
    }

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_update_601_params
 *
 * This routine configures all aspects of 601 VIP data capture, including
 * start and stop timings and input polarities.
 *--------------------------------------------------------------------------*/

int
vip_update_601_params(VIP_601PARAMS *buffer)
{
    unsigned long vip_control3, vip_control1;

    if (!buffer)
        return CIM_STATUS_INVALIDPARAMS;

    vip_control1 = READ_VIP32(VIP_CONTROL3);
    vip_control3 = READ_VIP32(VIP_CONTROL3);

    if (buffer->flags & VIP_MODEFLAG_VSYNCACTIVEHIGH)
        vip_control3 |= VIP_CONTROL3_VSYNC_POLARITY;
    else
        vip_control3 &= ~VIP_CONTROL3_VSYNC_POLARITY;
    if (buffer->flags & VIP_MODEFLAG_HSYNCACTIVEHIGH)
        vip_control3 |= VIP_CONTROL3_HSYNC_POLARITY;
    else
        vip_control3 &= ~VIP_CONTROL3_HSYNC_POLARITY;

    WRITE_VIP32(VIP_CONTROL3, vip_control3);
    WRITE_VIP32(VIP_601_HORZ_START, buffer->horz_start);
    WRITE_VIP32(VIP_601_VBI_START, buffer->vbi_start);
    WRITE_VIP32(VIP_601_VBI_END, buffer->vbi_start + buffer->vbi_height - 1);
    WRITE_VIP32(VIP_601_EVEN_START_STOP,
                buffer->vert_start_even | ((buffer->vert_start_even +
                                            buffer->even_height - 1) << 16));
    WRITE_VIP32(VIP_601_ODD_START_STOP,
                buffer->vert_start_odd | ((buffer->vert_start_odd +
                                           buffer->odd_height - 1) << 16));
    WRITE_VIP32(VIP_ODD_FIELD_DETECT,
                buffer->odd_detect_start | (buffer->odd_detect_end << 16));

    /* SPECIAL CASE FOR HORIZONTAL DATA
     * 601 horizontal parameters are based on the number of clocks and not
     * the number of pixels.
     */

    if ((vip_control1 & VIP_CONTROL1_MODE_MASK) == VIP_MODE_16BIT601)
        WRITE_VIP32(VIP_601_HORZ_END,
                    buffer->horz_start + (buffer->width << 1) + 3);
    else
        WRITE_VIP32(VIP_601_HORZ_END, buffer->horz_start + buffer->width + 3);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_configure_capture_buffers
 *
 * This routine configures the base offsets for video, ancillary or message
 * mode capture.  The input structure can also contain multiple offsets, such
 * that the calling application can avoid updating the structure for each
 * flip.
 *
 * The new buffer addresses are written to the hardware registers although
 * they may not be latched immediately. Calling vip_is_buffer_update_latched
 * allows the determination of whether the update has occurred.
 *
 * Review the Cimarron VIP API documentation to determine which buffer
 * addresses are latched immediately.
 *--------------------------------------------------------------------------*/

int
vip_configure_capture_buffers(int buffer_type, VIPINPUTBUFFER *buffer)
{
    VIPINPUTBUFFER_ADDR *offsets;
    unsigned long cur_buffer = buffer->current_buffer;

    if (!buffer)
        return CIM_STATUS_INVALIDPARAMS;

    if (buffer_type == VIP_BUFFER_A || buffer_type == VIP_BUFFER_601) {
        offsets = &buffer->offsets[VIP_BUFFER_TASK_A];

        /* SET VIDEO PITCH */

        WRITE_VIP32(VIP_TASKA_VID_PITCH,
                    offsets->y_pitch | (offsets->uv_pitch << 16));

        /* SET BASE OFFSETS */

        if (buffer->flags & VIP_INPUTFLAG_INVERTPOLARITY) {
            WRITE_VIP32(VIP_TASKA_VID_ODD_BASE, offsets->even_base[cur_buffer]);
            WRITE_VIP32(VIP_TASKA_VID_EVEN_BASE, offsets->odd_base[cur_buffer]);
            if (buffer->flags & VIP_INPUTFLAG_VBI) {
                WRITE_VIP32(VIP_TASKA_VBI_ODD_BASE, offsets->vbi_even_base);
                WRITE_VIP32(VIP_TASKA_VBI_EVEN_BASE, offsets->vbi_odd_base);
            }
        }
        else {
            WRITE_VIP32(VIP_TASKA_VID_ODD_BASE, offsets->odd_base[cur_buffer]);
            WRITE_VIP32(VIP_TASKA_VID_EVEN_BASE,
                        offsets->even_base[cur_buffer]);
            if (buffer->flags & VIP_INPUTFLAG_VBI) {
                WRITE_VIP32(VIP_TASKA_VBI_ODD_BASE, offsets->vbi_odd_base);
                WRITE_VIP32(VIP_TASKA_VBI_EVEN_BASE, offsets->vbi_even_base);
            }
        }

        /* SET 4:2:0 OFFSETS */

        if (buffer->flags & VIP_INPUTFLAG_PLANAR) {
            WRITE_VIP32(VIP_TASKA_U_OFFSET, offsets->odd_uoffset);
            WRITE_VIP32(VIP_TASKA_V_OFFSET, offsets->odd_voffset);
            WRITE_VIP32(VIP_TASKA_U_EVEN_OFFSET, offsets->even_uoffset);
            WRITE_VIP32(VIP_TASKA_V_EVEN_OFFSET, offsets->even_voffset);
        }
    }
    else if (buffer_type == VIP_BUFFER_B) {
        offsets = &buffer->offsets[VIP_BUFFER_TASK_B];

        /* SET VIDEO PITCH */

        WRITE_VIP32(VIP_TASKB_VID_PITCH,
                    offsets->y_pitch | (offsets->uv_pitch << 16));

        /* SET BASE OFFSETS */

        if (buffer->flags & VIP_INPUTFLAG_INVERTPOLARITY) {
            WRITE_VIP32(VIP_TASKB_VID_ODD_BASE, offsets->even_base[cur_buffer]);
            WRITE_VIP32(VIP_TASKB_VID_EVEN_BASE, offsets->odd_base[cur_buffer]);
            if (buffer->flags & VIP_INPUTFLAG_VBI) {
                WRITE_VIP32(VIP_TASKB_VBI_ODD_BASE, offsets->vbi_even_base);
                WRITE_VIP32(VIP_TASKB_VBI_EVEN_BASE, offsets->vbi_odd_base);
            }
        }
        else {
            WRITE_VIP32(VIP_TASKB_VID_ODD_BASE, offsets->odd_base[cur_buffer]);
            WRITE_VIP32(VIP_TASKB_VID_EVEN_BASE,
                        offsets->even_base[cur_buffer]);
            if (buffer->flags & VIP_INPUTFLAG_VBI) {
                WRITE_VIP32(VIP_TASKB_VBI_ODD_BASE, offsets->vbi_odd_base);
                WRITE_VIP32(VIP_TASKB_VBI_EVEN_BASE, offsets->vbi_even_base);
            }
        }

        /* SET 4:2:0 OFFSETS */

        if (buffer->flags & VIP_INPUTFLAG_PLANAR) {
            WRITE_VIP32(VIP_TASKB_U_OFFSET, offsets->odd_uoffset);
            WRITE_VIP32(VIP_TASKB_V_OFFSET, offsets->odd_voffset);
        }
    }
    else if (buffer_type == VIP_BUFFER_ANC || buffer_type == VIP_BUFFER_MSG) {
        WRITE_VIP32(VIP_ANC_MSG1_BASE, buffer->ancillaryData.msg1_base);
        WRITE_VIP32(VIP_ANC_MSG2_BASE, buffer->ancillaryData.msg2_base);
        WRITE_VIP32(VIP_ANC_MSG_SIZE, buffer->ancillaryData.msg_size);
    }
    else {
        return CIM_STATUS_INVALIDPARAMS;
    }

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_toggle_vip_video_offsets
 *
 * This routine updates the offsets for video capture.  It is a simplified
 * version of vip_configure_capture_buffers that is designed to be called from
 * interrupt service routines or other buffer flipping applications that
 * require low latency.
 *--------------------------------------------------------------------------*/

int
vip_toggle_video_offsets(int buffer_type, VIPINPUTBUFFER *buffer)
{
    unsigned long cur_buffer = buffer->current_buffer;
    VIPINPUTBUFFER_ADDR *offsets;

    if (buffer_type == VIP_BUFFER_A) {
        offsets = &buffer->offsets[VIP_BUFFER_TASK_A];

        /* SET BASE OFFSETS */

        if (buffer->flags & VIP_INPUTFLAG_INVERTPOLARITY) {
            WRITE_VIP32(VIP_TASKA_VID_ODD_BASE, offsets->even_base[cur_buffer]);
            WRITE_VIP32(VIP_TASKA_VID_EVEN_BASE, offsets->odd_base[cur_buffer]);
        }
        else {
            WRITE_VIP32(VIP_TASKA_VID_ODD_BASE, offsets->odd_base[cur_buffer]);
            WRITE_VIP32(VIP_TASKA_VID_EVEN_BASE,
                        offsets->even_base[cur_buffer]);
        }
    }
    else if (buffer_type == VIP_BUFFER_B) {
        offsets = &buffer->offsets[VIP_BUFFER_TASK_B];

        /* SET BASE OFFSETS */

        if (buffer->flags & VIP_INPUTFLAG_INVERTPOLARITY) {
            WRITE_VIP32(VIP_TASKB_VID_ODD_BASE, offsets->even_base[cur_buffer]);
            WRITE_VIP32(VIP_TASKB_VID_EVEN_BASE, offsets->odd_base[cur_buffer]);
        }
        else {
            WRITE_VIP32(VIP_TASKB_VID_ODD_BASE, offsets->odd_base[cur_buffer]);
            WRITE_VIP32(VIP_TASKB_VID_EVEN_BASE,
                        offsets->even_base[cur_buffer]);
        }
    }
    else if (buffer_type == VIP_BUFFER_A_ODD) {
        offsets = &buffer->offsets[VIP_BUFFER_TASK_A];

        /* SET BASE OFFSETS */

        if (buffer->flags & VIP_INPUTFLAG_INVERTPOLARITY)
            WRITE_VIP32(VIP_TASKA_VID_ODD_BASE, offsets->even_base[cur_buffer]);
        else
            WRITE_VIP32(VIP_TASKA_VID_ODD_BASE, offsets->odd_base[cur_buffer]);
    }
    else if (buffer_type == VIP_BUFFER_A_EVEN) {
        offsets = &buffer->offsets[VIP_BUFFER_TASK_A];

        /* SET BASE OFFSETS */

        if (buffer->flags & VIP_INPUTFLAG_INVERTPOLARITY)
            WRITE_VIP32(VIP_TASKA_VID_EVEN_BASE, offsets->odd_base[cur_buffer]);
        else
            WRITE_VIP32(VIP_TASKA_VID_EVEN_BASE,
                        offsets->even_base[cur_buffer]);
    }
    else if (buffer_type == VIP_BUFFER_B_ODD) {
        offsets = &buffer->offsets[VIP_BUFFER_TASK_B];

        /* SET BASE OFFSETS */

        if (buffer->flags & VIP_INPUTFLAG_INVERTPOLARITY)
            WRITE_VIP32(VIP_TASKB_VID_ODD_BASE, offsets->even_base[cur_buffer]);
        else
            WRITE_VIP32(VIP_TASKB_VID_ODD_BASE, offsets->odd_base[cur_buffer]);
    }
    else if (buffer_type == VIP_BUFFER_B_EVEN) {
        offsets = &buffer->offsets[VIP_BUFFER_TASK_B];

        /* SET BASE OFFSETS */

        if (buffer->flags & VIP_INPUTFLAG_INVERTPOLARITY)
            WRITE_VIP32(VIP_TASKB_VID_EVEN_BASE, offsets->odd_base[cur_buffer]);
        else
            WRITE_VIP32(VIP_TASKB_VID_EVEN_BASE,
                        offsets->even_base[cur_buffer]);
    }
    else
        return CIM_STATUS_INVALIDPARAMS;

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_set_capture_state
 *
 * This routine takes the current control word definition ( stored in locals )
 * adds in the specified state, and writes the control word.
 *--------------------------------------------------------------------------*/

int
vip_set_capture_state(unsigned long state)
{
    unsigned long vip_control1, vip_control3;

    /* UPDATE THE CURRENT CAPTURE MODE */

    vip_control1 = READ_VIP32(VIP_CONTROL1);
    vip_control3 = READ_VIP32(VIP_CONTROL3);
    vip_control1 &= ~VIP_CONTROL1_RUNMODE_MASK;
    vip_control1 |= (state << VIP_CONTROL1_RUNMODE_SHIFT);

    WRITE_VIP32(VIP_CONTROL1, vip_control1);

    if (state >= VIP_STARTCAPTUREATNEXTLINE) {
        /* WHACK VIP RESET
         * The VIP can get confused when switching between capture settings,
         * such as between linear and planar.  We will thus whack VIP reset
         * when enabling capture to ensure a pristine VIP state.
         */

        WRITE_VIP32(VIP_CONTROL1, vip_control1 | VIP_CONTROL1_RESET);
        WRITE_VIP32(VIP_CONTROL1, vip_control1 & ~VIP_CONTROL1_RESET);
        WRITE_VIP32(VIP_CONTROL3, vip_control3 | VIP_CONTROL3_FIFO_RESET);
    }

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_terminate
 *
 * This routine stops VIP capture and resets the VIP internal state.
 *--------------------------------------------------------------------------*/

int
vip_terminate(void)
{
    unsigned long timeout = 50000;

    /* DISABLE AND CLEAR ALL VIP INTERRUPTS */

    WRITE_VIP32(VIP_INTERRUPT, VIP_ALL_INTERRUPTS | (VIP_ALL_INTERRUPTS >> 16));

    /* DISABLE VIP CAPTURE */
    /* We will try to let the VIP FIFO flush before shutting it down. */

    WRITE_VIP32(VIP_CONTROL1, 0);
    while (timeout) {
        timeout--;
        if (READ_VIP32(VIP_STATUS) & VIP_STATUS_WRITES_COMPLETE)
            break;
    }

    /* RESET THE HARDWARE REGISTERS */
    /* Note that we enable VIP reset to allow clock gating to lower VIP */
    /* power consumption.                                               */

    WRITE_VIP32(VIP_CONTROL1, VIP_CONTROL1_RESET);
    WRITE_VIP32(VIP_CONTROL3, VIP_CONTROL3_FIFO_RESET);
    WRITE_VIP32(VIP_CONTROL2, 0);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_configure_fifo
 *
 * This routine sets the desired threshold or flush for the specified fifo.
 *--------------------------------------------------------------------------*/

int
vip_configure_fifo(unsigned long fifo_type, unsigned long fifo_size)
{
    unsigned long vip_control1, vip_control2;

    vip_control1 = READ_VIP32(VIP_CONTROL1);
    vip_control2 = READ_VIP32(VIP_CONTROL2);

    switch (fifo_type) {
    case VIP_VIDEOTHRESHOLD:
        vip_control2 &= ~VIP_CONTROL2_VIDTH_MASK;
        vip_control2 |=
            (fifo_size << VIP_CONTROL2_VIDTH_SHIFT) & VIP_CONTROL2_VIDTH_MASK;
        break;

    case VIP_ANCILLARYTHRESHOLD:
        vip_control2 &= ~VIP_CONTROL2_ANCTH_MASK;
        vip_control2 |=
            (fifo_size << VIP_CONTROL2_ANCTH_SHIFT) & VIP_CONTROL2_ANCTH_MASK;
        break;

    case VIP_VIDEOFLUSH:
        vip_control1 &= ~VIP_CONTROL1_VID_FF_MASK;
        vip_control1 |=
            ((fifo_size >> 2) << VIP_CONTROL1_VID_FF_SHIFT) &
            VIP_CONTROL1_VID_FF_MASK;
        break;

    case VIP_ANCILLARYFLUSH:
        vip_control1 &= ~VIP_CONTROL1_ANC_FF_MASK;
        vip_control1 |=
            ((fifo_size >> 2) << VIP_CONTROL1_ANC_FF_SHIFT) &
            VIP_CONTROL1_ANC_FF_MASK;
        break;

    default:
        return CIM_STATUS_INVALIDPARAMS;
    }

    WRITE_VIP32(VIP_CONTROL1, vip_control1);
    WRITE_VIP32(VIP_CONTROL2, vip_control2);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_set_interrupt_enable
 *
 * This routine accepts a mask of interrupts to be enabled/disabled and
 * an enable flag.
 *
 * For each mask match, the interrupt will be enabled or disabled based on
 * enable
 *--------------------------------------------------------------------------*/

int
vip_set_interrupt_enable(unsigned long mask, int enable)
{
    /* CHECK IF ANY VALID INTERRUPTS ARE BEING CHANGED */

    if (mask & VIP_ALL_INTERRUPTS) {
        unsigned long int_enable = READ_VIP32(VIP_INTERRUPT) & 0xFFFF;

        /* SET OR CLEAR THE MASK BITS */
        /* Note that the upper 16-bits of the register are 0 after this */
        /* operation.  This prevents us from indadvertently clearing a  */
        /* pending interrupt by enabling/disabling another one.         */

        if (enable)
            int_enable &= ~(mask >> 16);
        else
            int_enable |= (mask >> 16);

        WRITE_VIP32(VIP_INTERRUPT, int_enable);
    }

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_set_vsync_error
 *
 * This routine defines a region that is used to determine if the vsync is
 * within an acceptable range. This definition is accomplished using
 * a count and a vertical window.  The count specifies the exact number
 * of clocks expected for one field.  The window parameters specify the number
 * of clocks variation allowed before and after the expected vsync.  For
 * example, if vertical_count is 1000, window_before is 5 and window_after
 * is 12, VSync will be considered valid if it occurs between 995 and 1012
 * clocks after the last VSync.  The total window size (window_before +
 * window_after) cannot exceed 255.
 *--------------------------------------------------------------------------*/

int
vip_set_vsync_error(unsigned long vertical_count, unsigned long window_before,
                    unsigned long window_after, int enable)
{
    unsigned long vip_control2 = READ_VIP32(VIP_CONTROL2);
    unsigned long temp;

    if (enable) {
        /* CREATE THE VERTICAL WINDOW
         * The VIP uses two counters.  The first counter defines the minimum
         * clock count before a valid VSync can occur.  The second counter
         * starts after the first completes and defines the acceptable
         * region of variation.
         */

        temp = ((window_before +
                 window_after) << VIP_VSYNC_ERR_WINDOW_SHIFT) &
            VIP_VSYNC_ERR_WINDOW_MASK;
        temp |= (vertical_count - window_before) & VIP_VSYNC_ERR_COUNT_MASK;

        vip_control2 |= VIP_CONTROL2_VERTERROR_ENABLE;

        WRITE_VIP32(VIP_VSYNC_ERR_COUNT, temp);
    }
    else {
        vip_control2 &= ~VIP_CONTROL2_VERTERROR_ENABLE;
    }
    WRITE_VIP32(VIP_CONTROL2, vip_control2);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_max_address_enable
 *
 * This routine specifies the maximum address to which the the hardware should
 * write during data storage. If this value is exceeded an error is generated,
 * (this may be monitored using the appropriate interrupt flags - see
 * vip_set_interrupt_enable)
 *--------------------------------------------------------------------------*/

int
vip_max_address_enable(unsigned long max_address, int enable)
{
    unsigned long vip_control2 = READ_VIP32(VIP_CONTROL2);

    if (enable) {
        /* ENABLE THE CONTROL BIT */

        vip_control2 |= VIP_CONTROL2_ADD_ERROR_ENABLE;

        WRITE_VIP32(VIP_MAX_ADDRESS, max_address & VIP_MAXADDR_MASK);
    }
    else {
        /* DISABLE DETECTION */

        vip_control2 &= ~VIP_CONTROL2_ADD_ERROR_ENABLE;
    }
    WRITE_VIP32(VIP_CONTROL2, vip_control2);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_set_loopback_enable
 *
 * This routine enables/disables internal loopback functionality.  When
 * loopback is enabled, the VOP outputs are rerouted to the VIP inputs
 * internal to the chip.  No loopback connector is required.
 *--------------------------------------------------------------------------*/

int
vip_set_loopback_enable(int enable)
{
    unsigned long vip_control2 = READ_VIP32(VIP_CONTROL2);

    if (enable)
        vip_control2 |= VIP_CONTROL2_LOOPBACK_ENABLE;
    else
        vip_control2 &= ~VIP_CONTROL2_LOOPBACK_ENABLE;

    WRITE_VIP32(VIP_CONTROL2, vip_control2);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_configure_genlock
 *
 * This routine configures genlock functionality.
 *---------------------------------------------------------------------------*/

int
vip_configure_genlock(VIPGENLOCKBUFFER *buffer)
{
    unsigned long vip_control1, vip_control2;
    unsigned long unlock, genlk_ctl;

    if (!buffer)
        return CIM_STATUS_INVALIDPARAMS;

    unlock = READ_REG32(DC3_UNLOCK);
    genlk_ctl = READ_REG32(DC3_GENLK_CTL);
    vip_control1 = READ_VIP32(VIP_CONTROL1);
    vip_control2 = READ_VIP32(VIP_CONTROL2);

    /* UPDATE VIDEO DETECTION */
    /* These flags are used to indicate the ways in which the VIP signal */
    /* can be considered 'lost'.                                         */

    vip_control1 &= ~VIP_CONTROL1_VDE_FF_MASK;
    vip_control2 &= ~(VIP_CONTROL2_FIELD2VG_MASK | VIP_CONTROL2_SYNC2VG_MASK);
    vip_control1 |= buffer->vip_signal_loss;

    /* UPDATE FIELD AND VSYNC INFORMATION */
    /* These flags control how and when the even/odd field and Vsync */
    /* information is communicated to the VG.                        */

    vip_control2 |= buffer->field_to_vg;
    vip_control2 |= buffer->vsync_to_vg;

    /* ENABLE OR DISABLE GENLOCK TIMEOUT */
    /* Enabling genlock timeout allows the VG to revert to its own sync */
    /* timings when the VIP input is lost.  Note that the VIP will not  */
    /* know the signal is lost unless the appropriate error detection   */
    /* flags have been enabled inside vip_initialize.                   */

    if (buffer->enable_timeout)
        genlk_ctl |= DC3_GC_GENLOCK_TO_ENABLE;
    else
        genlk_ctl &= ~DC3_GC_GENLOCK_TO_ENABLE;

    genlk_ctl &= ~DC3_GC_GENLOCK_SKEW_MASK;
    genlk_ctl |= buffer->genlock_skew & DC3_GC_GENLOCK_SKEW_MASK;

    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);
    WRITE_REG32(DC3_GENLK_CTL, genlk_ctl);
    WRITE_VIP32(VIP_CONTROL1, vip_control1);
    WRITE_VIP32(VIP_CONTROL2, vip_control2);
    WRITE_REG32(DC3_UNLOCK, unlock);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_set_genlock_enable
 *
 * This routine enables/disables genlock inside the VG.
 *--------------------------------------------------------------------------*/

int
vip_set_genlock_enable(int enable)
{
    unsigned long unlock, temp;

    unlock = READ_REG32(DC3_UNLOCK);
    temp = READ_REG32(DC3_GENLK_CTL);

    if (enable)
        temp |= DC3_GC_GENLOCK_ENABLE;
    else
        temp &= ~DC3_GC_GENLOCK_ENABLE;

    WRITE_REG32(DC3_UNLOCK, DC3_UNLOCK_VALUE);
    WRITE_REG32(DC3_GENLK_CTL, temp);
    WRITE_REG32(DC3_UNLOCK, unlock);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_set_power_characteristics
 *
 * This routine takes a VIPPOWERBUFFER structure, and selectively sets the
 * GeodeLink power and/or Vip clock power states.
 *--------------------------------------------------------------------------*/

int
vip_set_power_characteristics(VIPPOWERBUFFER *buffer)
{
    Q_WORD q_word = { 0, 0 };

    if (!buffer)
        return CIM_STATUS_INVALIDPARAMS;

    q_word.low = q_word.high = 0;

    /* ENABLE GEODELINK CLOCK GATING */

    if (buffer->glink_clock_mode)
        q_word.low |= VIP_MSR_POWER_GLINK;

    /* ENABLE VIP CLOCK GATING */

    if (buffer->vip_clock_mode)
        q_word.low |= VIP_MSR_POWER_CLOCK;

    /* WRITE THE NEW VALUE */

    msr_write64(MSR_DEVICE_GEODELX_VIP, MSR_GEODELINK_PM, &q_word);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_set_priority_characteristics
 *
 * This routine programs the VIP GeodeLink priority characteristics
 *--------------------------------------------------------------------------*/

int
vip_set_priority_characteristics(VIPPRIORITYBUFFER *buffer)
{
    Q_WORD q_word = { 0, 0 };

    if (!buffer)
        return CIM_STATUS_INVALIDPARAMS;

    q_word.low = q_word.high = 0;

    q_word.low |= (buffer->secondary <<
                   VIP_MSR_MCR_SECOND_PRIORITY_SHIFT) &
        VIP_MSR_MCR_SECOND_PRIORITY_MASK;
    q_word.low |=
        (buffer->primary << VIP_MSR_MCR_PRIMARY_PRIORITY_SHIFT) &
        VIP_MSR_MCR_PRIMARY_PRIORITY_MASK;
    q_word.low |= (buffer->pid << VIP_MSR_MCR_PID_SHIFT) & VIP_MSR_MCR_PID_MASK;

    msr_write64(MSR_DEVICE_GEODELX_VIP, MSR_GEODELINK_CONFIG, &q_word);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_set_debug_characteristics
 *
 * This routine configures the debug data that is exposed over the diag bus.
 *--------------------------------------------------------------------------*/

int
vip_set_debug_characteristics(VIPDEBUGBUFFER *buffer)
{
    Q_WORD q_word = { 0, 0 };

    if (!buffer)
        return CIM_STATUS_INVALIDPARAMS;

    q_word.low = q_word.high = 0;

    q_word.high |= (buffer->bist << VIP_MSR_DIAG_BIST_SHIFT) &
        VIP_MSR_DIAG_BIST_WMASK;
    q_word.low |= (buffer->enable_upper ? VIP_MSR_DIAG_MSB_ENABLE : 0x00000000);
    q_word.low |= (buffer->select_upper << VIP_MSR_DIAG_SEL_UPPER_SHIFT) &
        VIP_MSR_DIAG_SEL_UPPER_MASK;
    q_word.low |= (buffer->enable_lower ? VIP_MSR_DIAG_LSB_ENABLE : 0x00000000);
    q_word.low |= (buffer->select_lower << VIP_MSR_DIAG_SEL_LOWER_SHIFT) &
        VIP_MSR_DIAG_SEL_LOWER_MASK;

    msr_write64(MSR_DEVICE_GEODELX_VIP, MSR_GEODELINK_DIAG, &q_word);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_configure_pages
 *
 * This routine sets the number of pages, and their offset from each other.
 *--------------------------------------------------------------------------*/

int
vip_configure_pages(int page_count, unsigned long page_offset)
{
    unsigned long vip_control2 = READ_VIP32(VIP_CONTROL2);

    /* SET THE NEW PAGE COUNT */

    vip_control2 &= ~VIP_CONTROL2_PAGECNT_MASK;
    vip_control2 |= (page_count << VIP_CONTROL2_PAGECNT_SHIFT) &
        VIP_CONTROL2_PAGECNT_MASK;

    /* WRITE THE PAGE OFFSET */

    WRITE_VIP32(VIP_CONTROL2, vip_control2);
    WRITE_VIP32(VIP_PAGE_OFFSET, page_offset);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_set_interrupt_line
 *
 * This routine sets the line at which a line interrupt should be generated.
 *--------------------------------------------------------------------------*/

int
vip_set_interrupt_line(int line)
{
    WRITE_VIP32(VIP_CURRENT_TARGET,
                (line << VIP_CTARGET_TLINE_SHIFT) & VIP_CTARGET_TLINE_MASK);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_reset
 *
 * This routine does a one-shot enable of the VIP hardware.  It is useful
 * for handling unrecoverable VIP errors.
 *--------------------------------------------------------------------------*/

int
vip_reset(void)
{
    unsigned long vip_control1, vip_control3;

    /* INVERT THE PAUSE BIT */

    vip_control1 = READ_VIP32(VIP_CONTROL1);
    vip_control3 = READ_VIP32(VIP_CONTROL3);

    WRITE_VIP32(VIP_CONTROL1, vip_control1 | VIP_CONTROL1_RESET);
    WRITE_VIP32(VIP_CONTROL1, vip_control1 & ~VIP_CONTROL1_RESET);
    WRITE_VIP32(VIP_CONTROL3, vip_control3 | VIP_CONTROL3_FIFO_RESET);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_set_subwindow_enable
 *
 * This routine turns on SubWindow capture, that is a portion of the incoming
 * signal is captured rather than the entire frame. The window always has
 * the same width as the frame, only the vertical component can be
 * modified.
 *--------------------------------------------------------------------------*/

int
vip_set_subwindow_enable(VIPSUBWINDOWBUFFER *buffer)
{
    unsigned long vip_control2;

    if (!buffer)
        return CIM_STATUS_INVALIDPARAMS;

    vip_control2 = READ_VIP32(VIP_CONTROL2);
    if (buffer->enable) {
        /* WRITE THE WINDOW VALUE */

        WRITE_VIP32(VIP_VERTICAL_START_STOP, ((buffer->stop <<
                                               VIP_VSTART_VERTEND_SHIFT) &
                                              VIP_VSTART_VERTEND_MASK) |
                    ((buffer->start << VIP_VSTART_VERTSTART_SHIFT) &
                     VIP_VSTART_VERTSTART_MASK));

        /* ENABLE IN THE CONTROL REGISTER */

        vip_control2 |= VIP_CONTROL2_SWC_ENABLE;
    }
    else {
        /* DISABLE SUBWINDOW CAPTURE IN THE CONTROL REGISTER */

        vip_control2 &= ~VIP_CONTROL2_SWC_ENABLE;
    }
    WRITE_VIP32(VIP_CONTROL2, vip_control2);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_reset_interrupt_state
 *
 * This routine resets the state of one or more interrupts.
 *--------------------------------------------------------------------------*/

int
vip_reset_interrupt_state(unsigned long interrupt_mask)
{
    unsigned long temp;

    temp = READ_VIP32(VIP_INTERRUPT);
    WRITE_VIP32(VIP_INTERRUPT, temp | (interrupt_mask & VIP_ALL_INTERRUPTS));

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_save_state
 *
 * This routine saves the necessary register contents in order to restore
 * at a later point to the same state.
 *
 * NOTE: Capture state is forced to OFF in this routine
 *--------------------------------------------------------------------------*/

int
vip_save_state(VIPSTATEBUFFER *save_buffer)
{
    if (!save_buffer)
        return CIM_STATUS_INVALIDPARAMS;

    /* FORCE CAPTURE TO BE DISABLED */

    vip_set_capture_state(VIP_STOPCAPTURE);

    /* READ AND SAVE THE REGISTER CONTENTS */

    save_buffer->control1 = READ_VIP32(VIP_CONTROL1);
    save_buffer->control2 = READ_VIP32(VIP_CONTROL2);
    save_buffer->vip_int = READ_VIP32(VIP_INTERRUPT);
    save_buffer->current_target = READ_VIP32(VIP_CURRENT_TARGET);
    save_buffer->max_address = READ_VIP32(VIP_MAX_ADDRESS);
    save_buffer->taska_evenbase = READ_VIP32(VIP_TASKA_VID_EVEN_BASE);
    save_buffer->taska_oddbase = READ_VIP32(VIP_TASKA_VID_ODD_BASE);
    save_buffer->taska_vbi_evenbase = READ_VIP32(VIP_TASKA_VBI_EVEN_BASE);
    save_buffer->taska_vbi_oddbase = READ_VIP32(VIP_TASKA_VBI_ODD_BASE);
    save_buffer->taska_data_pitch = READ_VIP32(VIP_TASKA_VID_PITCH);
    save_buffer->control3 = READ_VIP32(VIP_CONTROL3);
    save_buffer->taska_v_oddoffset = READ_VIP32(VIP_TASKA_U_OFFSET);
    save_buffer->taska_u_oddoffset = READ_VIP32(VIP_TASKA_V_OFFSET);
    save_buffer->taskb_evenbase = READ_VIP32(VIP_TASKB_VID_EVEN_BASE);
    save_buffer->taskb_oddbase = READ_VIP32(VIP_TASKB_VID_ODD_BASE);
    save_buffer->taskb_vbi_evenbase = READ_VIP32(VIP_TASKB_VBI_EVEN_BASE);
    save_buffer->taskb_vbi_oddbase = READ_VIP32(VIP_TASKB_VBI_ODD_BASE);
    save_buffer->taskb_pitch = READ_VIP32(VIP_TASKB_VID_PITCH);
    save_buffer->taskb_voffset = READ_VIP32(VIP_TASKB_U_OFFSET);
    save_buffer->taskb_uoffset = READ_VIP32(VIP_TASKB_V_OFFSET);
    save_buffer->msg1_base = READ_VIP32(VIP_ANC_MSG1_BASE);
    save_buffer->msg2_base = READ_VIP32(VIP_ANC_MSG2_BASE);
    save_buffer->msg_size = READ_VIP32(VIP_ANC_MSG_SIZE);
    save_buffer->page_offset = READ_VIP32(VIP_PAGE_OFFSET);
    save_buffer->vert_start_stop = READ_VIP32(VIP_VERTICAL_START_STOP);
    save_buffer->vsync_err_count = READ_VIP32(VIP_VSYNC_ERR_COUNT);
    save_buffer->taska_u_evenoffset = READ_VIP32(VIP_TASKA_U_EVEN_OFFSET);
    save_buffer->taska_v_evenoffset = READ_VIP32(VIP_TASKA_V_EVEN_OFFSET);

    /* READ ALL VIP MSRS */

    msr_read64(MSR_DEVICE_GEODELX_VIP, MSR_GEODELINK_CONFIG,
               &(save_buffer->msr_config));
    msr_read64(MSR_DEVICE_GEODELX_VIP, MSR_GEODELINK_SMI,
               &(save_buffer->msr_smi));
    msr_read64(MSR_DEVICE_GEODELX_VIP, MSR_GEODELINK_PM,
               &(save_buffer->msr_pm));
    msr_read64(MSR_DEVICE_GEODELX_VIP, MSR_GEODELINK_DIAG,
               &(save_buffer->msr_diag));

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_restore_state
 *
 * This routine restores the state of the vip registers - which were
 * previously saved using vip_save_state.
 *--------------------------------------------------------------------------*/

int
vip_restore_state(VIPSTATEBUFFER *restore_buffer)
{
    if (!restore_buffer)
        return CIM_STATUS_OK;

    /* RESTORE THE REGISTERS */

    WRITE_VIP32(VIP_CURRENT_TARGET, restore_buffer->current_target);
    WRITE_VIP32(VIP_MAX_ADDRESS, restore_buffer->max_address);
    WRITE_VIP32(VIP_TASKA_VID_EVEN_BASE, restore_buffer->taska_evenbase);
    WRITE_VIP32(VIP_TASKA_VID_ODD_BASE, restore_buffer->taska_oddbase);
    WRITE_VIP32(VIP_TASKA_VBI_EVEN_BASE, restore_buffer->taska_vbi_evenbase);
    WRITE_VIP32(VIP_TASKA_VBI_ODD_BASE, restore_buffer->taska_vbi_oddbase);
    WRITE_VIP32(VIP_TASKA_VID_PITCH, restore_buffer->taska_data_pitch);
    WRITE_VIP32(VIP_CONTROL3, restore_buffer->control3);
    WRITE_VIP32(VIP_TASKA_U_OFFSET, restore_buffer->taska_v_oddoffset);
    WRITE_VIP32(VIP_TASKA_V_OFFSET, restore_buffer->taska_u_oddoffset);
    WRITE_VIP32(VIP_TASKB_VID_EVEN_BASE, restore_buffer->taskb_evenbase);
    WRITE_VIP32(VIP_TASKB_VID_ODD_BASE, restore_buffer->taskb_oddbase);
    WRITE_VIP32(VIP_TASKB_VBI_EVEN_BASE, restore_buffer->taskb_vbi_evenbase);
    WRITE_VIP32(VIP_TASKB_VBI_ODD_BASE, restore_buffer->taskb_vbi_oddbase);
    WRITE_VIP32(VIP_TASKB_VID_PITCH, restore_buffer->taskb_pitch);
    WRITE_VIP32(VIP_TASKB_U_OFFSET, restore_buffer->taskb_voffset);
    WRITE_VIP32(VIP_TASKB_V_OFFSET, restore_buffer->taskb_uoffset);
    WRITE_VIP32(VIP_ANC_MSG1_BASE, restore_buffer->msg1_base);
    WRITE_VIP32(VIP_ANC_MSG2_BASE, restore_buffer->msg2_base);
    WRITE_VIP32(VIP_ANC_MSG_SIZE, restore_buffer->msg_size);
    WRITE_VIP32(VIP_PAGE_OFFSET, restore_buffer->page_offset);
    WRITE_VIP32(VIP_VERTICAL_START_STOP, restore_buffer->vert_start_stop);
    WRITE_VIP32(VIP_VSYNC_ERR_COUNT, restore_buffer->vsync_err_count);
    WRITE_VIP32(VIP_TASKA_U_EVEN_OFFSET, restore_buffer->taska_u_evenoffset);
    WRITE_VIP32(VIP_TASKA_V_EVEN_OFFSET, restore_buffer->taska_v_evenoffset);

    /* RESTORE THE VIP MSRS */

    msr_write64(MSR_DEVICE_GEODELX_VIP, MSR_GEODELINK_CONFIG,
                &(restore_buffer->msr_config));
    msr_write64(MSR_DEVICE_GEODELX_VIP, MSR_GEODELINK_SMI,
                &(restore_buffer->msr_smi));
    msr_write64(MSR_DEVICE_GEODELX_VIP, MSR_GEODELINK_PM,
                &(restore_buffer->msr_pm));
    msr_write64(MSR_DEVICE_GEODELX_VIP, MSR_GEODELINK_DIAG,
                &(restore_buffer->msr_diag));

    /* RESTORE THE CONTROL WORDS LAST */

    WRITE_VIP32(VIP_CONTROL1, restore_buffer->control1);
    WRITE_VIP32(VIP_CONTROL2, restore_buffer->control2);
    WRITE_VIP32(VIP_CONTROL3, restore_buffer->control3);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_get_interrupt_state
 *
 * This routine returns the current interrupt state of the system. The
 * rv can be tested with the following flags to determine if the appropriate
 * event has occurred.
 *--------------------------------------------------------------------------*/

unsigned long
vip_get_interrupt_state(void)
{
    unsigned long interrupt_mask = READ_VIP32(VIP_INTERRUPT);

    return (~(interrupt_mask << 16) & interrupt_mask & VIP_ALL_INTERRUPTS);
}

/*---------------------------------------------------------------------------
 * vip_test_genlock_active
 *
 * This routine reads the live status of the genlock connection between the
 * VIP and VG blocks.
 *--------------------------------------------------------------------------*/

int
vip_test_genlock_active(void)
{
    if (READ_REG32(DC3_GENLK_CTL) & DC3_GC_GENLK_ACTIVE)
        return 1;

    return 0;
}

/*---------------------------------------------------------------------------
 * vip_test_signal_status
 *
 * This routine reads the live signal status coming into the VIP block.
 *--------------------------------------------------------------------------*/

int
vip_test_signal_status(void)
{
    if (READ_REG32(DC3_GENLK_CTL) & DC3_GC_VIP_VID_OK)
        return 1;

    return 0;
}

/*---------------------------------------------------------------------------
 * vip_get_current_field
 *
 * This routine returns the current field being received.
 *--------------------------------------------------------------------------*/

unsigned long
vip_get_current_field(void)
{
    if (READ_VIP32(VIP_STATUS) & VIP_STATUS_FIELD)
        return VIP_EVEN_FIELD;

    return VIP_ODD_FIELD;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * CIMARRON VIP READ ROUTINES
 * These routines are included for use in diagnostics or when debugging.  They
 * can be optionally excluded from a project.
 *++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#if CIMARRON_INCLUDE_VIP_READ_ROUTINES

/*---------------------------------------------------------------------------
 * vip_get_current_mode
 *
 * This routine reads the current VIP operating mode.
 *--------------------------------------------------------------------------*/

int
vip_get_current_mode(VIPSETMODEBUFFER *buffer)
{
    unsigned long vip_control1, vip_control2, vip_control3;

    if (!buffer)
        return CIM_STATUS_INVALIDPARAMS;

    vip_control1 = READ_VIP32(VIP_CONTROL1);
    vip_control2 = READ_VIP32(VIP_CONTROL2);
    vip_control3 = READ_VIP32(VIP_CONTROL3);

    /* READ CURRENT OPERATING MODE AND ENABLES */

    buffer->stream_enables = vip_control1 & VIP_ENABLE_ALL;
    buffer->operating_mode = vip_control1 & VIP_CONTROL1_MODE_MASK;

    /* READ CURRENT PLANAR CAPTURE SETTINGS */

    buffer->flags = 0;
    buffer->planar_capture = 0;
    if (vip_control1 & VIP_CONTROL1_PLANAR) {
        buffer->flags |= VIP_MODEFLAG_PLANARCAPTURE;
        if (vip_control1 & VIP_CONTROL1_DISABLE_DECIMATION) {
            if (vip_control3 & VIP_CONTROL3_DECIMATE_EVEN)
                buffer->planar_capture = VIP_420CAPTURE_ALTERNATINGFIELDS;
            else
                buffer->planar_capture = VIP_420CAPTURE_EVERYLINE;
        }
        else
            buffer->planar_capture = VIP_420CAPTURE_ALTERNATINGLINES;
    }

    /* READ MISCELLANEOUS FLAGS */

    if (vip_control1 & VIP_CONTROL1_NON_INTERLACED)
        buffer->flags |= VIP_MODEFLAG_PROGRESSIVE;
    if (vip_control3 & VIP_CONTROL3_BASE_UPDATE)
        buffer->flags |= VIP_MODEFLAG_TOGGLEEACHFIELD;
    if (vip_control2 & VIP_CONTROL2_INVERT_POLARITY)
        buffer->flags |= VIP_MODEFLAG_INVERTPOLARITY;
    if (vip_control1 & VIP_CONTROL1_MSG_STRM_CTRL)
        buffer->flags |= VIP_MODEFLAG_FLIPMESSAGEWHENFULL;
    if (vip_control2 & VIP_CONTROL2_REPEAT_ENABLE)
        buffer->flags |= VIP_MODEFLAG_ENABLEREPEATFLAG;
    if (vip_control3 & VIP_CONTROL3_TASK_POLARITY)
        buffer->flags |= VIP_MODEFLAG_INVERTTASKPOLARITY;
    if (vip_control1 & VIP_CONTROL1_DISABLE_ZERO_DETECT)
        buffer->flags |= VIP_MODEFLAG_DISABLEZERODETECT;
    if (vip_control2 & VIP_CONTROL2_ANC10)
        buffer->flags |= VIP_MODEFLAG_10BITANCILLARY;

    /* READ THE CURRENT VIP 601 SETTINGS */

    vip_get_601_configuration(&buffer->vip601_settings);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_get_601_configuration
 *
 * This routine returns the current 601 configuration information.
 *--------------------------------------------------------------------------*/

int
vip_get_601_configuration(VIP_601PARAMS *buffer)
{
    unsigned long vip_control3, vip_control1;

    if (!buffer)
        return CIM_STATUS_INVALIDPARAMS;

    vip_control1 = READ_VIP32(VIP_CONTROL3);
    vip_control3 = READ_VIP32(VIP_CONTROL3);

    buffer->flags = 0;
    if (vip_control3 & VIP_CONTROL3_VSYNC_POLARITY)
        buffer->flags |= VIP_MODEFLAG_VSYNCACTIVEHIGH;
    if (vip_control3 & VIP_CONTROL3_HSYNC_POLARITY)
        buffer->flags |= VIP_MODEFLAG_HSYNCACTIVEHIGH;

    buffer->horz_start = READ_VIP32(VIP_601_HORZ_START);
    buffer->vbi_start = READ_VIP32(VIP_601_VBI_START);
    buffer->vbi_height = READ_VIP32(VIP_601_VBI_END) - buffer->vbi_start + 1;
    buffer->vert_start_even = READ_VIP32(VIP_601_EVEN_START_STOP) & 0xFFFF;
    buffer->even_height = (READ_VIP32(VIP_601_EVEN_START_STOP) >> 16) -
        buffer->vert_start_even + 1;
    buffer->vert_start_odd = READ_VIP32(VIP_601_ODD_START_STOP) & 0xFFFF;
    buffer->odd_height = (READ_VIP32(VIP_601_ODD_START_STOP) >> 16) -
        buffer->vert_start_odd + 1;
    buffer->odd_detect_start = READ_VIP32(VIP_ODD_FIELD_DETECT) & 0xFFFF;
    buffer->odd_detect_end = READ_VIP32(VIP_ODD_FIELD_DETECT) >> 16;

    /* SPECIAL CASE FOR HORIZONTAL DATA
     * 601 horizontal parameters are based on the number of clocks and not
     * the number of pixels.
     */

    if ((vip_control1 & VIP_CONTROL1_MODE_MASK) == VIP_MODE_16BIT601)
        buffer->width = (READ_VIP32(VIP_601_HORZ_END) -
                         buffer->horz_start - 3) >> 1;
    else
        buffer->width = (READ_VIP32(VIP_601_HORZ_END) - buffer->horz_start - 3);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_get_buffer_configuration
 *
 * This routine reads the current buffer configuration for Task A, Task B,
 * ancillary or message data.  The current_buffer member indicates which
 * array index should hold the new values for Task A or Task B data.
 *--------------------------------------------------------------------------*/

int
vip_get_buffer_configuration(int buffer_type, VIPINPUTBUFFER *buffer)
{
    unsigned long cur_buffer = buffer->current_buffer;
    VIPINPUTBUFFER_ADDR *offsets;

    if (!buffer)
        return CIM_STATUS_INVALIDPARAMS;

    if (buffer_type == VIP_BUFFER_A) {
        offsets = &buffer->offsets[VIP_BUFFER_TASK_A];

        /* READ VIDEO PITCH */

        offsets->y_pitch = READ_VIP32(VIP_TASKA_VID_PITCH) & 0xFFFF;
        offsets->uv_pitch = READ_VIP32(VIP_TASKA_VID_PITCH) >> 16;

        /* READ BASE OFFSETS */

        if (buffer->flags & VIP_INPUTFLAG_INVERTPOLARITY) {
            offsets->even_base[cur_buffer] = READ_VIP32(VIP_TASKA_VID_ODD_BASE);
            offsets->odd_base[cur_buffer] = READ_VIP32(VIP_TASKA_VID_EVEN_BASE);

            if (buffer->flags & VIP_INPUTFLAG_VBI) {
                offsets->vbi_even_base = READ_VIP32(VIP_TASKA_VBI_ODD_BASE);
                offsets->vbi_odd_base = READ_VIP32(VIP_TASKA_VBI_EVEN_BASE);
            }
        }
        else {
            offsets->even_base[cur_buffer] =
                READ_VIP32(VIP_TASKA_VID_EVEN_BASE);
            offsets->odd_base[cur_buffer] = READ_VIP32(VIP_TASKA_VID_ODD_BASE);

            if (buffer->flags & VIP_INPUTFLAG_VBI) {
                offsets->vbi_even_base = READ_VIP32(VIP_TASKA_VBI_EVEN_BASE);
                offsets->vbi_odd_base = READ_VIP32(VIP_TASKA_VBI_ODD_BASE);
            }
        }

        /* READ 4:2:0 OFFSETS */

        if (buffer->flags & VIP_INPUTFLAG_PLANAR) {
            offsets->odd_uoffset = READ_VIP32(VIP_TASKA_U_OFFSET);
            offsets->odd_voffset = READ_VIP32(VIP_TASKA_V_OFFSET);
            offsets->even_uoffset = READ_VIP32(VIP_TASKA_U_EVEN_OFFSET);
            offsets->even_voffset = READ_VIP32(VIP_TASKA_V_EVEN_OFFSET);
        }
    }
    else if (buffer_type == VIP_BUFFER_B) {
        offsets = &buffer->offsets[VIP_BUFFER_TASK_B];

        /* READ VIDEO PITCH */

        offsets->y_pitch = READ_VIP32(VIP_TASKB_VID_PITCH) & 0xFFFF;
        offsets->uv_pitch = READ_VIP32(VIP_TASKB_VID_PITCH) >> 16;

        /* READ BASE OFFSETS */

        if (buffer->flags & VIP_INPUTFLAG_INVERTPOLARITY) {
            offsets->even_base[cur_buffer] = READ_VIP32(VIP_TASKB_VID_ODD_BASE);
            offsets->odd_base[cur_buffer] = READ_VIP32(VIP_TASKB_VID_EVEN_BASE);

            if (buffer->flags & VIP_INPUTFLAG_VBI) {
                offsets->vbi_even_base = READ_VIP32(VIP_TASKB_VBI_ODD_BASE);
                offsets->vbi_odd_base = READ_VIP32(VIP_TASKB_VBI_EVEN_BASE);
            }
        }
        else {
            offsets->even_base[cur_buffer] =
                READ_VIP32(VIP_TASKB_VID_EVEN_BASE);
            offsets->odd_base[cur_buffer] = READ_VIP32(VIP_TASKB_VID_ODD_BASE);

            if (buffer->flags & VIP_INPUTFLAG_VBI) {
                offsets->vbi_even_base = READ_VIP32(VIP_TASKB_VBI_EVEN_BASE);
                offsets->vbi_odd_base = READ_VIP32(VIP_TASKB_VBI_ODD_BASE);
            }
        }

        /* READ 4:2:0 OFFSETS */

        if (buffer->flags & VIP_INPUTFLAG_PLANAR) {
            offsets->odd_uoffset = READ_VIP32(VIP_TASKB_U_OFFSET);
            offsets->odd_voffset = READ_VIP32(VIP_TASKB_V_OFFSET);
        }
    }
    else if (buffer_type == VIP_BUFFER_ANC || buffer_type == VIP_BUFFER_MSG) {
        buffer->ancillaryData.msg1_base = READ_VIP32(VIP_ANC_MSG1_BASE);
        buffer->ancillaryData.msg2_base = READ_VIP32(VIP_ANC_MSG2_BASE);
        buffer->ancillaryData.msg_size = READ_VIP32(VIP_ANC_MSG_SIZE);
    }
    else {
        return CIM_STATUS_INVALIDPARAMS;
    }

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_get_genlock_configuration
 *
 * This routine reads the current genlock configuration.
 *--------------------------------------------------------------------------*/

int
vip_get_genlock_configuration(VIPGENLOCKBUFFER *buffer)
{
    unsigned long vip_control1, vip_control2;
    unsigned long genlk_ctl;

    if (!buffer)
        return CIM_STATUS_INVALIDPARAMS;

    genlk_ctl = READ_REG32(DC3_GENLK_CTL);
    vip_control1 = READ_VIP32(VIP_CONTROL1);
    vip_control2 = READ_VIP32(VIP_CONTROL2);

    /* READ ERROR DETECTION, CURRENT FIELD AND CURRENT VSYNC
     * These flags are used to indicate the ways in which the VIP signal can
     * be considered 'lost'.
     */

    buffer->vip_signal_loss = vip_control1 & VIP_CONTROL1_VDE_FF_MASK;
    buffer->field_to_vg = vip_control2 & VIP_CONTROL2_FIELD2VG_MASK;
    buffer->vsync_to_vg = vip_control2 & VIP_CONTROL2_SYNC2VG_MASK;

    /* GENLOCK TIMEOUT ENABLE */

    buffer->enable_timeout = 0;
    if (genlk_ctl & DC3_GC_GENLOCK_TO_ENABLE)
        buffer->enable_timeout = 1;

    /* GENLOCK SKEW */

    buffer->genlock_skew = genlk_ctl & DC3_GC_GENLOCK_SKEW_MASK;

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_get_genlock_enable
 *
 * This routine returns the current enable status of genlock in the VG.
 *--------------------------------------------------------------------------*/

int
vip_get_genlock_enable(void)
{
    if (READ_REG32(DC3_GENLK_CTL) & DC3_GC_GENLOCK_ENABLE)
        return 1;

    return 0;
}

/*---------------------------------------------------------------------------
 * vip_is_buffer_update_latched
 *
 * This routine indicates whether changes to the VIP offsets have been
 * latched by the hardware.
 *--------------------------------------------------------------------------*/

int
vip_is_buffer_update_latched(void)
{
    return (!(READ_VIP32(VIP_STATUS) & VIP_STATUS_BASEREG_NOTUPDT));
}

/*---------------------------------------------------------------------------
 * vip_get_capture_state
 *
 * This routine reads the current capture status of the VIP hardware.
 *--------------------------------------------------------------------------*/

unsigned long
vip_get_capture_state(void)
{
    return ((READ_VIP32(VIP_CONTROL1) & VIP_CONTROL1_RUNMODE_MASK) >>
            VIP_CONTROL1_RUNMODE_SHIFT);
}

/*---------------------------------------------------------------------------
 * vip_get_current_line
 *
 * This routine returns the current line that is being processed.
 *--------------------------------------------------------------------------*/

unsigned long
vip_get_current_line(void)
{
    return (READ_VIP32(VIP_CURRENT_TARGET) & VIP_CTARGET_CLINE_MASK);
}

/*---------------------------------------------------------------------------
 * vip_read_fifo
 *
 * This routine reads from the specified fifo address. As the fifo access
 * enable should be disabled when running in normal vip mode, this routine
 * enables and disables access around the read.
 * DIAGNOSTIC USE ONLY
 *--------------------------------------------------------------------------*/

unsigned long
vip_read_fifo(unsigned long dwFifoAddress)
{
    unsigned long fifo_data;

    /* ENABLE FIFO ACCESS */

    vip_enable_fifo_access(1);

    /* NOW READ THE DATA */

    WRITE_VIP32(VIP_FIFO_ADDRESS, dwFifoAddress);
    fifo_data = READ_VIP32(VIP_FIFO_DATA);

    /* DISABLE FIFO ACCESS */

    vip_enable_fifo_access(0);

    return fifo_data;
}

/*---------------------------------------------------------------------------
 * vip_write_fifo
 *
 * SYNOPSIS:
 * This routine writes to the specified fifo address. As the fifo access
 * enable should be disabled when running in normal vip mode, this routine
 * enables and disables access around the write.
 * DIAGNOSTIC USE ONLY
 *--------------------------------------------------------------------------*/

int
vip_write_fifo(unsigned long dwFifoAddress, unsigned long dwFifoData)
{
    /* ENABLE FIFO ACCESS */

    vip_enable_fifo_access(1);

    /* WRITE THE FIFO DATA */

    WRITE_VIP32(VIP_FIFO_ADDRESS, dwFifoAddress);
    WRITE_VIP32(VIP_FIFO_DATA, dwFifoData);

    /* DISABLE FIFO ACCESS */

    vip_enable_fifo_access(0);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_enable_fifo_access
 *
 * This routine enables/disables access to the vip fifo.
 * DIAGNOSTIC USE ONLY
 *--------------------------------------------------------------------------*/

int
vip_enable_fifo_access(int enable)
{
    unsigned long cw2;

    cw2 = READ_VIP32(VIP_CONTROL2);

    if (enable)
        cw2 |= VIP_CONTROL2_FIFO_ACCESS;
    else
        cw2 &= ~VIP_CONTROL2_FIFO_ACCESS;

    WRITE_VIP32(VIP_CONTROL2, cw2);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_get_power_characteristics
 *
 * This routine returns the current VIP clock gating state in a
 * VIPPOWERBUFFER.
 *--------------------------------------------------------------------------*/

int
vip_get_power_characteristics(VIPPOWERBUFFER *buffer)
{
    Q_WORD q_word = { 0, 0 };

    if (!buffer)
        return CIM_STATUS_INVALIDPARAMS;

    /* READ THE EXISTING STATE */

    msr_read64(MSR_DEVICE_GEODELX_VIP, MSR_GEODELINK_PM, &q_word);

    /* DECODE THE CLOCK GATING BITS */

    buffer->glink_clock_mode = (int) (q_word.low & VIP_MSR_POWER_GLINK);
    buffer->vip_clock_mode = (int) (q_word.low & VIP_MSR_POWER_CLOCK);

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_get_priority_characteristics
 *
 * This routine returns the priority characteristics in the supplied
 * VIPPRIORITYBUFFER.
 *--------------------------------------------------------------------------*/

int
vip_get_priority_characteristics(VIPPRIORITYBUFFER *buffer)
{
    Q_WORD q_word = { 0, 0 };

    if (!buffer)
        return CIM_STATUS_INVALIDPARAMS;

    /* READ THE CURRENT STATE */

    msr_read64(MSR_DEVICE_GEODELX_VIP, MSR_GEODELINK_CONFIG, &q_word);

    /* DECODE THE PRIORITIES */

    buffer->secondary = (q_word.low & VIP_MSR_MCR_SECOND_PRIORITY_MASK) >>
        VIP_MSR_MCR_SECOND_PRIORITY_SHIFT;
    buffer->primary = (q_word.low & VIP_MSR_MCR_PRIMARY_PRIORITY_MASK) >>
        VIP_MSR_MCR_PRIMARY_PRIORITY_SHIFT;
    buffer->pid = q_word.low & VIP_MSR_MCR_PID_MASK;

    return CIM_STATUS_OK;
}

/*---------------------------------------------------------------------------
 * vip_get_capability_characteristics
 *
 * This routine returns revision information for the device.
 *--------------------------------------------------------------------------*/

int
vip_get_capability_characteristics(VIPCAPABILITIESBUFFER *buffer)
{
    Q_WORD q_word = { 0, 0 };

    if (!buffer)
        return CIM_STATUS_INVALIDPARAMS;

    /* READ THE CURRENT MSR CONTENTS */

    msr_read64(MSR_DEVICE_GEODELX_VIP, MSR_GEODELINK_CAP, &q_word);

    /* DECODE THE REVISIONS */

    buffer->revision_id = (q_word.low & VIP_MSR_CAP_REVID_MASK) >>
        VIP_MSR_CAP_REVID_SHIFT;
    buffer->device_id = (q_word.low & VIP_MSR_CAP_DEVID_MASK) >>
        VIP_MSR_CAP_DEVID_SHIFT;
    buffer->n_clock_domains = (q_word.low & VIP_MSR_CAP_NCLK_MASK) >>
        VIP_MSR_CAP_NCLK_SHIFT;
    buffer->n_smi_registers = (q_word.low & VIP_MSR_CAP_NSMI_MASK) >>
        VIP_MSR_CAP_NSMI_SHIFT;

    return CIM_STATUS_OK;
}

#endif
