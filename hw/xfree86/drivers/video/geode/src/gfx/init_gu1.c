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
 * This file contains routines used in the initialization of Geode-family 
 * processors.
 * */

/*----------------------------------------------------------------------------
 * gfx_gxm_config_read
 *
 * This routine reads the value of the specified GXm configuration register.
 *----------------------------------------------------------------------------
 */
unsigned char
gfx_gxm_config_read(unsigned char index)
{
    unsigned char value = 0xFF;
    unsigned char lock;

    OUTB(0x22, GXM_CONFIG_CCR3);
    lock = INB(0x23);
    OUTB(0x22, GXM_CONFIG_CCR3);
    OUTB(0x23, (unsigned char) (lock | 0x10));
    OUTB(0x22, index);
    value = INB(0x23);
    OUTB(0x22, GXM_CONFIG_CCR3);
    OUTB(0x23, lock);
    return (value);
}

/*----------------------------------------------------------------------------
 * gfx_get_core_freq
 *
 * This routine returns the core clock frequency of a GXm if valid jumper 
 * settings are detected; 0 It assumes that a 33.3 MHz PCI clock is being used
 * for GX1.  
 * For SCx2xx, the fast PCI divisor is read.
 *----------------------------------------------------------------------------
 */
#if GFX_INIT_DYNAMIC
unsigned long
gu1_get_core_freq(void)
#else
unsigned long
gfx_get_core_freq(void)
#endif
{
    unsigned char dir0, dir1;
    unsigned long cpu_speed;

    dir0 = gfx_gxm_config_read(GXM_CONFIG_DIR0) & 0x0F;
    dir1 = gfx_gxm_config_read(GXM_CONFIG_DIR1);

    /* REVISION 4.0 AND UP */

    if (dir1 >= 0x50) {
        switch (dir0) {
        case 0:
        case 2:
            cpu_speed = 133;
            break;
        case 5:
            cpu_speed = 166;
            break;
        case 3:
            cpu_speed = 200;
            break;
        case 6:
            cpu_speed = 233;
            break;
        case 7:
            cpu_speed = 266;
            break;
        case 4:
            cpu_speed = 300;
            break;
        case 1:
            cpu_speed = 333;
            break;
        default:
            return (0);
        }
    }
    else {
        switch (dir0) {
        case 0:
        case 2:
            cpu_speed = 133;
            break;
        case 7:
            cpu_speed = 166;
            break;
        case 1:
        case 3:
            cpu_speed = 200;
            break;
        case 4:
        case 6:
            cpu_speed = 233;
            break;
        case 5:
            cpu_speed = 266;
            break;
        default:
            return (0);
        }
    }

    if ((gfx_cpu_version & 0xFF) == GFX_CPU_SC1200) {
        cpu_speed = (cpu_speed * gfx_pci_speed_khz) / 33300;
    }
    return (cpu_speed);
}

/*----------------------------------------------------------------------------
 * gfx_get_cpu_register_base
 * 
 * This routine returns the base address for graphics registers.
 *----------------------------------------------------------------------------
 */
#if GFX_INIT_DYNAMIC
unsigned long
gu1_get_cpu_register_base(void)
#else
unsigned long
gfx_get_cpu_register_base(void)
#endif
{
    unsigned long base;

    base = (unsigned long) gfx_gxm_config_read(GXM_CONFIG_GCR);
    base = (base & 0x03) << 30;
    return (base);
}

/*----------------------------------------------------------------------------
 * gfx_get_frame_buffer_base
 * 
 * This routine returns the base address for graphics memory.  This is an 
 * offset of 0x00800000 from the base address specified in the GCR register.
 *
 * The function returns zero if the GCR indicates the graphics subsystem
 * is disabled.
 *----------------------------------------------------------------------------
 */
#if GFX_INIT_DYNAMIC
unsigned long
gu1_get_frame_buffer_base(void)
#else
unsigned long
gfx_get_frame_buffer_base(void)
#endif
{
    unsigned long base;

    base = (unsigned long) gfx_gxm_config_read(GXM_CONFIG_GCR);
    base = (base & 0x03) << 30;
    if (base)
        base |= 0x00800000;
    return (base);
}

/*----------------------------------------------------------------------------
 * gfx_get_frame_buffer_size
 * 
 * This routine returns the total size of graphics memory, in bytes.
 *
 * Currently this routine is hardcoded to return 2 Meg.
 *----------------------------------------------------------------------------
 */
#if GFX_INIT_DYNAMIC
unsigned long
gu1_get_frame_buffer_size(void)
#else
unsigned long
gfx_get_frame_buffer_size(void)
#endif
{
#if FB4MB
    return (0x00400000);
#else
    return (0x00200000);
#endif
}

/*----------------------------------------------------------------------------
 * gfx_get_vid_register_base
 * 
 * This routine returns the base address for the video hardware.  It assumes
 * an offset of 0x00010000 from the base address specified by the GCR.
 *
 * The function returns zero if the GCR indicates the graphics subsystem
 * is disabled.
 *----------------------------------------------------------------------------
 */
#if GFX_INIT_DYNAMIC
unsigned long
gu1_get_vid_register_base(void)
#else
unsigned long
gfx_get_vid_register_base(void)
#endif
{
    unsigned long base;

    base = (unsigned long) gfx_gxm_config_read(GXM_CONFIG_GCR);
    base = (base & 0x03) << 30;
    if (base)
        base |= 0x00010000;
    return (base);
}

/*----------------------------------------------------------------------------
 * gfx_get_vip_register_base
 * 
 * This routine returns the base address for the VIP hardware.  This is 
 * only applicable to the SC1200, for which this routine assumes an offset 
 * of 0x00015000 from the base address specified by the GCR.
 *
 * The function returns zero if the GCR indicates the graphics subsystem
 * is disabled.
 *----------------------------------------------------------------------------
 */
#if GFX_INIT_DYNAMIC
unsigned long
gu1_get_vip_register_base(void)
#else
unsigned long
gfx_get_vip_register_base(void)
#endif
{
    unsigned long base = 0;

    if ((gfx_cpu_version & 0xFF) == GFX_CPU_SC1200) {
        base = (unsigned long) gfx_gxm_config_read(GXM_CONFIG_GCR);
        base = (base & 0x03) << 30;
        if (base)
            base |= 0x00015000;
    }
    return (base);
}

/* END OF FILE */
