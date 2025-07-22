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
 * This file contains routines to read machine-specific registers (MSRs)
 * 
 * Routines:
 * 
 *       gfx_msr_init
 *       gfx_id_msr_device
 *       gfx_get_msr_dev_address
 *       gfx_get_glink_id_at_address
 *       gfx_msr_read
 *       gfx_msr_write
 * */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* INCLUDE SUPPORT FOR REDCLOUD, IF SPECIFIED */

#if GFX_MSR_REDCLOUD
#include "msr_rdcl.c"
#endif

/* EXTRA WRAPPERS FOR DYNAMIC SELECTION */

#if GFX_MSR_DYNAMIC

/*----------------------------------------------------------------------------
 * gfx_msr_init
 *----------------------------------------------------------------------------
 */
int
gfx_msr_init()
{
    int ret_value = 0;

#if GFX_MSR_REDCLOUD
    if (gfx_msr_type & GFX_MSR_TYPE_REDCLOUD)
        ret_value = redcloud_msr_init();
#endif

    return ret_value;
}

/*----------------------------------------------------------------------------
 * gfx_id_msr_device
 *----------------------------------------------------------------------------
 */
DEV_STATUS
gfx_id_msr_device(MSR *pDev, unsigned long address)
{
    DEV_STATUS ret_value = NOT_KNOWN;

#if GFX_MSR_REDCLOUD
    if (gfx_msr_type & GFX_MSR_TYPE_REDCLOUD)
        ret_value = redcloud_id_msr_device(pDev, address);
#endif

    return ret_value;
}

/*----------------------------------------------------------------------------
 * gfx_get_msr_dev_address
 *----------------------------------------------------------------------------
 */
DEV_STATUS
gfx_get_msr_dev_address(unsigned int device, unsigned long *address)
{
    DEV_STATUS ret_value = NOT_KNOWN;

#if GFX_MSR_REDCLOUD
    if (gfx_msr_type & GFX_MSR_TYPE_REDCLOUD)
        ret_value = redcloud_get_msr_dev_address(device, address);
#endif

    return ret_value;
}

/*----------------------------------------------------------------------------
 * gfx_get_glink_id_at_address
 *----------------------------------------------------------------------------
 */
DEV_STATUS
gfx_get_glink_id_at_address(unsigned int *device, unsigned long address)
{
    DEV_STATUS ret_value = NOT_KNOWN;

#if GFX_MSR_REDCLOUD
    if (gfx_msr_type & GFX_MSR_TYPE_REDCLOUD)
        ret_value = redcloud_get_glink_id_at_address(device, address);
#endif

    return ret_value;
}

/*----------------------------------------------------------------------------
 * gfx_msr_read
 *----------------------------------------------------------------------------
 */
DEV_STATUS
gfx_msr_read(unsigned int device, unsigned int msrRegister, Q_WORD *msrValue)
{
    DEV_STATUS ret_value = NOT_KNOWN;

#if GFX_MSR_REDCLOUD
    if (gfx_msr_type & GFX_MSR_TYPE_REDCLOUD)
        ret_value = redcloud_msr_read(device, msrRegister, msrValue);
#endif

    return ret_value;
}

/*----------------------------------------------------------------------------
 * gfx_msr_write
 *----------------------------------------------------------------------------
 */
DEV_STATUS
gfx_msr_write(unsigned int device, unsigned int msrRegister, Q_WORD *msrValue)
{
    DEV_STATUS ret_value = NOT_KNOWN;

#if GFX_MSR_REDCLOUD
    if (gfx_msr_type & GFX_MSR_TYPE_REDCLOUD)
        ret_value = redcloud_msr_write(device, msrRegister, msrValue);
#endif

    return ret_value;
}

#endif
