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
 * This file contains routines used in Redcloud initialization.
 * */

/*----------------------------------------------------------------------------
 * gfx_get_core_freq
 *
 * Returns the core clock frequency of a GX2.
 *----------------------------------------------------------------------------
 */
#if GFX_INIT_DYNAMIC
unsigned long
gu2_get_core_freq(void)
#else
unsigned long
gfx_get_core_freq(void)
#endif
{
    unsigned long value;

    /* CPU SPEED IS REPORTED BY A VSM IN VSA II */
    /* Virtual Register Class = 0x12 (Sysinfo)  */
    /* CPU Speed Register     = 0x01            */

    OUTW(0xAC1C, 0xFC53);
    OUTW(0xAC1C, 0x1201);

    value = (unsigned long) (INW(0xAC1E));

    return (value);
}

/*----------------------------------------------------------------------------
 * gfx_get_cpu_register_base
 * 
 * This routine returns the base address for display controller registers.  
 *----------------------------------------------------------------------------
 */
#if GFX_INIT_DYNAMIC
unsigned long
gu2_get_cpu_register_base(void)
#else
unsigned long
gfx_get_cpu_register_base(void)
#endif
{
    return gfx_pci_config_read(0x80000918);
}

/*----------------------------------------------------------------------------
 * gfx_get_graphics_register_base
 * 
 * This routine returns the base address for the graphics acceleration.  
 *----------------------------------------------------------------------------
 */
#if GFX_INIT_DYNAMIC
unsigned long
gu2_get_graphics_register_base(void)
#else
unsigned long
gfx_get_graphics_register_base(void)
#endif
{
    return gfx_pci_config_read(0x80000914);
}

/*----------------------------------------------------------------------------
 * gfx_get_frame_buffer_base
 * 
 * This routine returns the base address for graphics memory.  
 *----------------------------------------------------------------------------
 */
#if GFX_INIT_DYNAMIC
unsigned long
gu2_get_frame_buffer_base(void)
#else
unsigned long
gfx_get_frame_buffer_base(void)
#endif
{
    return gfx_pci_config_read(0x80000910);
}

/*----------------------------------------------------------------------------
 * gfx_get_frame_buffer_size
 * 
 * This routine returns the total size of graphics memory, in bytes.
 *----------------------------------------------------------------------------
 */
#if GFX_INIT_DYNAMIC
unsigned long
gu2_get_frame_buffer_size(void)
#else
unsigned long
gfx_get_frame_buffer_size(void)
#endif
{
    unsigned long value;

    /* FRAME BUFFER SIZE IS REPORTED BY A VSM IN VSA II */
    /* Virtual Register Class     = 0x02                */
    /* VG_MEM_SIZE (512KB units)  = 0x00                */

    OUTW(0xAC1C, 0xFC53);
    OUTW(0xAC1C, 0x0200);

    value = (unsigned long) (INW(0xAC1E)) & 0xFFl;

    return (value << 19);
}

/*----------------------------------------------------------------------------
 * gfx_get_vid_register_base
 * 
 * This routine returns the base address for the video hardware.  
 *----------------------------------------------------------------------------
 */
#if GFX_INIT_DYNAMIC
unsigned long
gu2_get_vid_register_base(void)
#else
unsigned long
gfx_get_vid_register_base(void)
#endif
{
    return gfx_pci_config_read(0x8000091C);
}

/* END OF FILE */
