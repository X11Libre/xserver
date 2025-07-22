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
 * This file contains routines to control the SC1200 video input port (VIP) 
 * hardware.
 * */

/*----------------------------------------------------------------------------
 * gfx_set_vip_enable
 *
 * This routine enables or disables the writes to memory from the video port. 
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
int
sc1200_set_vip_enable(int enable)
#else
int
gfx_set_vip_enable(int enable)
#endif
{
    unsigned long value;

    value = READ_VIP32(SC1200_VIP_CONTROL);
    if (enable)
        value |= SC1200_VIP_DATA_CAPTURE_EN;
    else
        value &= ~SC1200_VIP_DATA_CAPTURE_EN;
    WRITE_VIP32(SC1200_VIP_CONTROL, value);
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_vip_capture_run_mode
 *
 * This routine selects VIP capture run mode.
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
int
sc1200_set_vip_capture_run_mode(int mode)
#else
int
gfx_set_vip_capture_run_mode(int mode)
#endif
{
    unsigned long value;

    value = READ_VIP32(SC1200_VIP_CONTROL);
    value &= ~SC1200_CAPTURE_RUN_MODE_MASK;
    switch (mode) {
    case VIP_CAPTURE_STOP_LINE:
        value |= SC1200_CAPTURE_RUN_MODE_STOP_LINE;
        break;
    case VIP_CAPTURE_STOP_FIELD:
        value |= SC1200_CAPTURE_RUN_MODE_STOP_FIELD;
        break;
    case VIP_CAPTURE_START_FIELD:
        value |= SC1200_CAPTURE_RUN_MODE_START;
        break;
    default:
        return GFX_STATUS_BAD_PARAMETER;
    }
    WRITE_VIP32(SC1200_VIP_CONTROL, value);
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_vip_base
 *
 * This routine sets the odd and even base address values for the VIP memory
 * buffer.
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
int
sc1200_set_vip_base(unsigned long even, unsigned long odd)
#else
int
gfx_set_vip_base(unsigned long even, unsigned long odd)
#endif
{
    /* TRUE OFFSET IS SPECIFIED, NEED TO SET BIT 23 FOR HARDWARE */

    if (even)
        WRITE_VIP32(SC1200_VIP_EVEN_BASE,
                    even + (unsigned long) gfx_phys_fbptr);
    if (odd)
        WRITE_VIP32(SC1200_VIP_ODD_BASE, odd + (unsigned long) gfx_phys_fbptr);
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_vip_pitch
 *
 * This routine sets the number of bytes between scanlines for the VIP data.
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
int
sc1200_set_vip_pitch(unsigned long pitch)
#else
int
gfx_set_vip_pitch(unsigned long pitch)
#endif
{
    WRITE_VIP32(SC1200_VIP_PITCH, pitch & SC1200_VIP_PITCH_MASK);
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_vip_mode
 *
 * This routine sets the VIP operating mode.
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
int
sc1200_set_vip_mode(int mode)
#else
int
gfx_set_vip_mode(int mode)
#endif
{
    unsigned long config;

    config = READ_VIP32(SC1200_VIP_CONFIG);
    config &= ~SC1200_VIP_MODE_MASK;
    switch (mode) {
    case VIP_MODE_C:
        WRITE_VIP32(SC1200_VIP_CONFIG, config | SC1200_VIP_MODE_C);
        break;
    default:
        return GFX_STATUS_BAD_PARAMETER;
    }
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_vbi_enable
 *
 * This routine enables or disables the VBI data capture.
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
int
sc1200_set_vbi_enable(int enable)
#else
int
gfx_set_vbi_enable(int enable)
#endif
{
    unsigned long value;

    value = READ_VIP32(SC1200_VIP_CONTROL);
    if (enable)
        value |= SC1200_VIP_VBI_CAPTURE_EN;
    else
        value &= ~SC1200_VIP_VBI_CAPTURE_EN;
    WRITE_VIP32(SC1200_VIP_CONTROL, value);
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_vbi_mode
 *
 * This routine sets the VBI data types captured to memory.
 * It receives a mask of all enabled types.
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
int
sc1200_set_vbi_mode(int mode)
#else
int
gfx_set_vbi_mode(int mode)
#endif
{
    unsigned long config;

    config = READ_VIP32(SC1200_VIP_CONFIG);
    config &=
        ~(SC1200_VBI_ANCILLARY_TO_MEMORY | SC1200_VBI_TASK_A_TO_MEMORY |
          SC1200_VBI_TASK_B_TO_MEMORY);

    if (mode & VBI_ANCILLARY)
        config |= SC1200_VBI_ANCILLARY_TO_MEMORY;
    if (mode & VBI_TASK_A)
        config |= SC1200_VBI_TASK_A_TO_MEMORY;
    if (mode & VBI_TASK_B)
        config |= SC1200_VBI_TASK_B_TO_MEMORY;
    WRITE_VIP32(SC1200_VIP_CONFIG, config);
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_vbi_base
 *
 * This routine sets the odd and even base address values for VBI capture.
 *
 * "even" and "odd" should contain 16-byte aligned physical addresses.
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
int
sc1200_set_vbi_base(unsigned long even, unsigned long odd)
#else
int
gfx_set_vbi_base(unsigned long even, unsigned long odd)
#endif
{
    /* VIP HW REQUIRES THAT BASE ADDRESSES BE 16-BYTE ALIGNED */

    if (even)
        WRITE_VIP32(SC1200_VBI_EVEN_BASE, even & ~0xf);
    if (odd)
        WRITE_VIP32(SC1200_VBI_ODD_BASE, odd & ~0xf);

    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_vbi_pitch
 *
 * This routine sets the number of bytes between scanlines for VBI capture.
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
int
sc1200_set_vbi_pitch(unsigned long pitch)
#else
int
gfx_set_vbi_pitch(unsigned long pitch)
#endif
{
    WRITE_VIP32(SC1200_VBI_PITCH, pitch & SC1200_VBI_PITCH_MASK);
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_vbi_direct
 *
 * This routine sets the VBI lines to be passed to the Direct VIP.
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
int
sc1200_set_vbi_direct(unsigned long even_lines, unsigned long odd_lines)
#else
int
gfx_set_vbi_direct(unsigned long even_lines, unsigned long odd_lines)
#endif
{
    WRITE_VIP32(SC1200_EVEN_DIRECT_VBI_LINE_ENABLE,
                even_lines & SC1200_DIRECT_VBI_LINE_ENABLE_MASK);
    WRITE_VIP32(SC1200_ODD_DIRECT_VBI_LINE_ENABLE,
                odd_lines & SC1200_DIRECT_VBI_LINE_ENABLE_MASK);
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_vbi_interrupt
 *
 * This routine enables or disables the VBI field interrupt.
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
int
sc1200_set_vbi_interrupt(int enable)
#else
int
gfx_set_vbi_interrupt(int enable)
#endif
{
    unsigned long value;

    value = READ_VIP32(SC1200_VIP_CONTROL);
    if (enable)
        value |= SC1200_VIP_VBI_FIELD_INTERRUPT_EN;
    else
        value &= ~SC1200_VIP_VBI_FIELD_INTERRUPT_EN;
    WRITE_VIP32(SC1200_VIP_CONTROL, value);
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_vip_bus_request_threshold_high
 *
 * This routine sets the VIP FIFO bus request threshold.
 * If enable is TRUE, VIP FIFO will be set to issue a bus request when it 
 * filled with 64 bytes. If enable is FALSE, VIP FIFO will be set to issue a 
 * bus request when it filled with 32 bytes.
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
int
sc1200_set_vip_bus_request_threshold_high(int enable)
#else
int
gfx_set_vip_bus_request_threshold_high(int enable)
#endif
{
    unsigned long value;

    value = READ_VIP32(SC1200_VIP_CONFIG);
    if (enable)
        value &= ~SC1200_VIP_BUS_REQUEST_THRESHOLD;
    else
        value |= SC1200_VIP_BUS_REQUEST_THRESHOLD;
    WRITE_VIP32(SC1200_VIP_CONFIG, value);
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_set_vip_last_line
 *
 * This routine sets the maximum number of lines captured in each field.
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
int
sc1200_set_vip_last_line(int last_line)
#else
int
gfx_set_vip_last_line(int last_line)
#endif
{
    unsigned long value;

    /* This feature is implemented in Rev C1 */
    if (gfx_chip_revision < SC1200_REV_C1)
        return (GFX_STATUS_OK);

    value = READ_VIP32(SC1200_VIP_LINE_TARGET);
    value &= ~SC1200_VIP_LAST_LINE_MASK;
    value |= ((last_line & 0x3FF) << 16);
    WRITE_VIP32(SC1200_VIP_LINE_TARGET, value);
    return (GFX_STATUS_OK);
}

/*----------------------------------------------------------------------------
 * gfx_test_vip_odd_field
 *
 * This routine returns 1 if the current VIP field is odd. Otherwise returns 0
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
int
sc1200_test_vip_odd_field(void)
#else
int
gfx_test_vip_odd_field(void)
#endif
{
    if (READ_VIP32(SC1200_VIP_STATUS) & SC1200_VIP_CURRENT_FIELD_ODD)
        return (1);
    else
        return (0);
}

/*----------------------------------------------------------------------------
 * gfx_test_vip_bases_updated
 *
 * This routine returns 1 if all of the VIP base registers have been updated,
 * i.e. there is no base register which has been written with a new address, 
 * that VIP has not already captured or started capturing into the new address
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
int
sc1200_test_vip_bases_updated(void)
#else
int
gfx_test_vip_bases_updated(void)
#endif
{
    if (READ_VIP32(SC1200_VIP_STATUS) & SC1200_VIP_BASE_NOT_UPDATED)
        return (0);
    else
        return (1);
}

/*----------------------------------------------------------------------------
 * gfx_test_vip_fifo_overflow
 *
 * This routine returns 1 if an overflow occurred on the FIFO between the VIP
 * and the fast X-bus, 0 otherwise. 
 * If an overflow occurred, the overflow status indication is reset.
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
int
sc1200_test_vip_fifo_overflow(void)
#else
int
gfx_test_vip_fifo_overflow(void)
#endif
{
    if (READ_VIP32(SC1200_VIP_STATUS) & SC1200_VIP_FIFO_OVERFLOW) {
        /* Bits in vip status register are either read only or reset by 
         * writing 1 */
        WRITE_VIP32(SC1200_VIP_STATUS, SC1200_VIP_FIFO_OVERFLOW);
        return (1);
    }
    else {
        return (0);
    }
}

/*----------------------------------------------------------------------------
 * gfx_get_vip_line
 *
 * This routine returns the number of the current video line being
 * received by the VIP interface.
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
int
sc1200_get_vip_line(void)
#else
int
gfx_get_vip_line(void)
#endif
{
    return (int) (READ_VIP32(SC1200_VIP_CURRENT_LINE) &
                  SC1200_VIP_CURRENT_LINE_MASK);
}

/*----------------------------------------------------------------------------
 * gfx_get_vip_base
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
unsigned long
sc1200_get_vip_base(int odd)
#else
unsigned long
gfx_get_vip_base(int odd)
#endif
{
    /* MASK BIT 23 AND ABOVE TO MAKE IT A TRUE OFFSET */

    if (odd)
        return (READ_VIP32(SC1200_VIP_ODD_BASE));
    return (READ_VIP32(SC1200_VIP_EVEN_BASE));
}

/*----------------------------------------------------------------------------
 * gfx_get_vbi_pitch
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
unsigned long
sc1200_get_vbi_pitch(void)
#else
unsigned long
gfx_get_vbi_pitch(void)
#endif
{
    return (READ_VIP32(SC1200_VBI_PITCH) & SC1200_VBI_PITCH_MASK);
}

/*************************************************************/
/*  READ ROUTINES  |  INCLUDED FOR DIAGNOSTIC PURPOSES ONLY  */
/*************************************************************/

#if GFX_READ_ROUTINES

/*----------------------------------------------------------------------------
 * gfx_get_vip_enable
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
int
sc1200_get_vip_enable(void)
#else
int
gfx_get_vip_enable(void)
#endif
{
    if (READ_VIP32(SC1200_VIP_CONTROL) & SC1200_VIP_DATA_CAPTURE_EN)
        return (1);
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_get_vip_pitch
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
unsigned long
sc1200_get_vip_pitch(void)
#else
unsigned long
gfx_get_vip_pitch(void)
#endif
{
    return (READ_VIP32(SC1200_VIP_PITCH) & SC1200_VIP_PITCH_MASK);
}

/*----------------------------------------------------------------------------
 * gfx_get_vip_mode
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
int
sc1200_get_vip_mode(void)
#else
int
gfx_get_vip_mode(void)
#endif
{
    switch (READ_VIP32(SC1200_VIP_CONFIG) & SC1200_VIP_MODE_MASK) {
    case SC1200_VIP_MODE_C:
        return VIP_MODE_C;
    default:
        return (0);
    }
}

/*----------------------------------------------------------------------------
 * gfx_get_vbi_enable
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
int
sc1200_get_vbi_enable(void)
#else
int
gfx_get_vbi_enable(void)
#endif
{
    if (READ_VIP32(SC1200_VIP_CONTROL) & SC1200_VIP_VBI_CAPTURE_EN)
        return (1);
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_get_vbi_mode
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
int
sc1200_get_vbi_mode(void)
#else
int
gfx_get_vbi_mode(void)
#endif
{
    int config;
    int mode = 0;

    config =
        (int) (READ_VIP32(SC1200_VIP_CONFIG) & (SC1200_VBI_ANCILLARY_TO_MEMORY
                                                | SC1200_VBI_TASK_A_TO_MEMORY |
                                                SC1200_VBI_TASK_B_TO_MEMORY));
    if (config & SC1200_VBI_ANCILLARY_TO_MEMORY)
        mode |= VBI_ANCILLARY;
    if (config & SC1200_VBI_TASK_A_TO_MEMORY)
        mode |= VBI_TASK_A;
    if (config & SC1200_VBI_TASK_B_TO_MEMORY)
        mode |= VBI_TASK_B;
    return mode;
}

/*----------------------------------------------------------------------------
 * gfx_get_vbi_base
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
unsigned long
sc1200_get_vbi_base(int odd)
#else
unsigned long
gfx_get_vbi_base(int odd)
#endif
{
    /* MASK BIT 23 AND ABOVE TO MAKE IT A TRUE OFFSET */

    if (odd)
        return (READ_VIP32(SC1200_VBI_ODD_BASE));
    return (READ_VIP32(SC1200_VBI_EVEN_BASE));
}

/*----------------------------------------------------------------------------
 * gfx_get_vbi_direct
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
unsigned long
sc1200_get_vbi_direct(int odd)
#else
unsigned long
gfx_get_vbi_direct(int odd)
#endif
{
    /* MASK BIT 23 AND ABOVE TO MAKE IT A TRUE OFFSET */

    if (odd)
        return (READ_VIP32(SC1200_ODD_DIRECT_VBI_LINE_ENABLE) &
                SC1200_DIRECT_VBI_LINE_ENABLE_MASK);
    return (READ_VIP32(SC1200_EVEN_DIRECT_VBI_LINE_ENABLE) &
            SC1200_DIRECT_VBI_LINE_ENABLE_MASK);
}

/*---------------------------------------------------------------------------
 * gfx_get_vbi_interrupt
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
int
sc1200_get_vbi_interrupt(void)
#else
int
gfx_get_vbi_interrupt(void)
#endif
{
    if (READ_VIP32(SC1200_VIP_CONTROL) & SC1200_VIP_VBI_FIELD_INTERRUPT_EN)
        return (1);
    return (0);
}

/*----------------------------------------------------------------------------
 * gfx_get_vip_bus_request_threshold_high
 *----------------------------------------------------------------------------
 */
#if GFX_VIP_DYNAMIC
int
sc1200_get_vip_bus_request_threshold_high(void)
#else
int
gfx_get_vip_bus_request_threshold_high(void)
#endif
{
    if (READ_VIP32(SC1200_VIP_CONFIG) & SC1200_VIP_BUS_REQUEST_THRESHOLD)
        return (1);
    return (0);
}

#endif                          /* GFX_READ_ROUTINES */

/* END OF FILE */
