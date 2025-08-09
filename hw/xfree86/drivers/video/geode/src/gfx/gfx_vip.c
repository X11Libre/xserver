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
 * This file contains routines to control the video input port (VIP).
 * 
 *    gfx_set_vip_enable
 *    gfx_set_vip_capture_run_mode
 *    gfx_set_vip_base
 *    gfx_set_vip_pitch
 *    gfx_set_vip_mode
 *    gfx_set_vbi_enable
 *    gfx_set_vbi_mode
 *    gfx_set_vbi_base
 *    gfx_set_vbi_pitch
 *    gfx_set_vbi_direct
 *    gfx_set_vbi_interrupt
 *    gfx_set_vip_bus_request_threshold_high
 *    gfx_set_vip_last_line
 *    gfx_test_vip_odd_field
 *    gfx_test_vip_bases_updated
 *    gfx_test_vip_fifo_overflow
 *    gfx_get_vip_line
 *    gfx_get_vip_base
 *    gfx_get_vbi_pitch
 *
 * And the following routines if GFX_READ_ROUTINES is set:
 *
 *    gfx_get_vip_enable
 *    gfx_get_vip_pitch
 *    gfx_get_vip_mode
 *    gfx_get_vbi_enable
 *    gfx_get_vbi_mode
 *    gfx_get_vbi_base
 *    gfx_get_vbi_direct
 *    gfx_get_vbi_interrupt
 *    gfx_get_vip_bus_request_threshold_high
 * */

/* INCLUDE SUPPORT FOR SC1200, IF SPECIFIED. */

#if GFX_VIP_SC1200
#include "vip_1200.c"
#endif

/* WRAPPERS IF DYNAMIC SELECTION */
/* Extra layer to call either CS5530 or SC1200 routines. */

#if GFX_VIP_DYNAMIC

/*---------------------------------------------------------------------------
 * gfx_set_vip_enable
 *----------------------------------------------------------------------------
 */
int
gfx_set_vip_enable(int enable)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        status = sc1200_set_vip_enable(enable);
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_set_vip_capture_run_mode
 *----------------------------------------------------------------------------
 */
int
gfx_set_vip_capture_run_mode(int mode)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        status = sc1200_set_vip_capture_run_mode(mode);
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_set_vip_base
 *----------------------------------------------------------------------------
 */
int
gfx_set_vip_base(unsigned long even, unsigned long odd)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        status = sc1200_set_vip_base(even, odd);
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_set_vip_pitch
 *----------------------------------------------------------------------------
 */
int
gfx_set_vip_pitch(unsigned long pitch)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        status = sc1200_set_vip_pitch(pitch);
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_set_vip_mode
 *----------------------------------------------------------------------------
 */
int
gfx_set_vip_mode(int mode)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        status = sc1200_set_vip_mode(mode);
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_set_vbi_enable
 *----------------------------------------------------------------------------
 */
int
gfx_set_vbi_enable(int enable)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        status = sc1200_set_vbi_enable(enable);
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_set_vbi_mode
 *----------------------------------------------------------------------------
 */
int
gfx_set_vbi_mode(int mode)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        status = sc1200_set_vbi_mode(mode);
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_set_vbi_base
 *----------------------------------------------------------------------------
 */
int
gfx_set_vbi_base(unsigned long even, unsigned long odd)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        status = sc1200_set_vbi_base(even, odd);
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_set_vbi_pitch
 *----------------------------------------------------------------------------
 */
int
gfx_set_vbi_pitch(unsigned long pitch)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        status = sc1200_set_vbi_pitch(pitch);
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_set_vbi_direct
 *----------------------------------------------------------------------------
 */
int
gfx_set_vbi_direct(unsigned long even_lines, unsigned long odd_lines)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        status = sc1200_set_vbi_direct(even_lines, odd_lines);
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_set_vbi_interrupt
 *----------------------------------------------------------------------------
 */
int
gfx_set_vbi_interrupt(int enable)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        status = sc1200_set_vbi_interrupt(enable);
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_set_vip_bus_request_threshold_high
 *----------------------------------------------------------------------------
 */
int
gfx_set_vip_bus_request_threshold_high(int enable)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        status = sc1200_set_vip_bus_request_threshold_high(enable);
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_set_vip_last_line
 *----------------------------------------------------------------------------
 */
int
gfx_set_vip_last_line(int last_line)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        status = sc1200_set_vip_last_line(last_line);
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_test_vip_odd_field
 *----------------------------------------------------------------------------
 */
int
gfx_test_vip_odd_field(void)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        status = sc1200_test_vip_odd_field();
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_test_vip_bases_updated
 *----------------------------------------------------------------------------
 */
int
gfx_test_vip_bases_updated(void)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        status = sc1200_test_vip_bases_updated();
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_test_vip_fifo_overflow
 *----------------------------------------------------------------------------
 */
int
gfx_test_vip_fifo_overflow(void)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        status = sc1200_test_vip_fifo_overflow();
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_get_vip_line
 *----------------------------------------------------------------------------
 */
int
gfx_get_vip_line(void)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        status = sc1200_get_vip_line();
#endif
    return (status);
}

/*----------------------------------------------------------------------------
 * gfx_get_vip_base
 *----------------------------------------------------------------------------
 */
unsigned long
gfx_get_vip_base(int odd)
{
    unsigned long base = 0;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        base = sc1200_get_vip_base(odd);
#endif
    return (base);
}

/*----------------------------------------------------------------------------
 * gfx_get_vbi_pitch
 *----------------------------------------------------------------------------
 */
unsigned long
gfx_get_vbi_pitch(void)
{
    unsigned long pitch = 0;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        pitch = sc1200_get_vbi_pitch();
#endif
    return (pitch);
}

/*************************************************************/
/*  READ ROUTINES  |  INCLUDED FOR DIAGNOSTIC PURPOSES ONLY  */
/*************************************************************/

#if GFX_READ_ROUTINES

/*----------------------------------------------------------------------------
 * gfx_get_vip_enable
 *----------------------------------------------------------------------------
 */
int
gfx_get_vip_enable(void)
{
    int enable = 0;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        enable = sc1200_get_vip_enable();
#endif
    return (enable);
}

/*----------------------------------------------------------------------------
 * gfx_get_vip_pitch
 *----------------------------------------------------------------------------
 */
unsigned long
gfx_get_vip_pitch(void)
{
    unsigned long pitch = 0;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        pitch = sc1200_get_vip_pitch();
#endif
    return (pitch);
}

/*----------------------------------------------------------------------------
 * gfx_get_vip_mode
 *----------------------------------------------------------------------------
 */
int
gfx_get_vip_mode(void)
{
    int mode = 0;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        mode = sc1200_get_vip_mode();
#endif
    return (mode);
}

/*----------------------------------------------------------------------------
 * gfx_get_vbi_enable
 *----------------------------------------------------------------------------
 */
int
gfx_get_vbi_enable(void)
{
    int enable = 0;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        enable = sc1200_get_vbi_enable();
#endif
    return (enable);
}

/*----------------------------------------------------------------------------
 * gfx_get_vbi_mode
 *----------------------------------------------------------------------------
 */
int
gfx_get_vbi_mode(void)
{
    int mode = 0;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        mode = sc1200_get_vbi_mode();
#endif
    return (mode);
}

/*----------------------------------------------------------------------------
 * gfx_get_vbi_base
 *----------------------------------------------------------------------------
 */
unsigned long
gfx_get_vbi_base(int odd)
{
    unsigned long base = 0;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        base = sc1200_get_vbi_base(odd);
#endif
    return (base);
}

/*----------------------------------------------------------------------------
 * gfx_get_vbi_direct
 *----------------------------------------------------------------------------
 */
unsigned long
gfx_get_vbi_direct(int odd)
{
    unsigned long vbi_direct_lines = 0;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        vbi_direct_lines = sc1200_get_vbi_direct(odd);
#endif
    return (vbi_direct_lines);
}

/*----------------------------------------------------------------------------
 * gfx_get_vbi_interrupt
 *----------------------------------------------------------------------------
 */
int
gfx_get_vbi_interrupt(void)
{
    int enable = 0;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        enable = sc1200_get_vbi_interrupt();
#endif
    return (enable);
}

/*----------------------------------------------------------------------------
 * gfx_get_vip_bus_request_threshold_high
 *----------------------------------------------------------------------------
 */
int
gfx_get_vip_bus_request_threshold_high(void)
{
    int enable = 0;

#if GFX_VIP_SC1200
    if (gfx_vip_type == GFX_VIP_TYPE_SC1200)
        enable = sc1200_get_vip_bus_request_threshold_high();
#endif
    return (enable);
}

#endif                          /* GFX_READ_ROUTINES */

#endif                          /* GFX_VIP_DYNAMIC */

/* END OF FILE */
