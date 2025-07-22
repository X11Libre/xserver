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
 * This file contains routines to write to and read from the I2C bus.
 * */

/* INCLUDE ROUTINES FOR ACCESS.BUS, IF SPECIFIED */
/* This is for SC1200 systems. */

#if GFX_I2C_ACCESS
#include "i2c_acc.c"
#endif

/* INCLUDE ROUTINES FOR CS5530 GPIOs, IF SPECIFIED */
/* This is for GXLV systems that use GPIOs on the CS5530 for I2C. */

#if GFX_I2C_GPIO
#include "i2c_gpio.c"
#endif

/* WRAPPERS IF DYNAMIC SELECTION */
/* Extra layer to call either ACCESS.bus or GPIO routines. */

#if GFX_I2C_DYNAMIC

/*---------------------------------------------------------------------------
 * gfx_i2c_reset
 *---------------------------------------------------------------------------
 */
int
gfx_i2c_reset(unsigned char busnum, short adr, char freq)
{
    int status = GFX_STATUS_UNSUPPORTED;

#if GFX_I2C_ACCESS
    if (gfx_i2c_type & GFX_I2C_TYPE_ACCESS)
        status = acc_i2c_reset(busnum, adr, freq);
#endif
#if GFX_I2C_GPIO
    if (gfx_i2c_type & GFX_I2C_TYPE_GPIO)
        status = gpio_i2c_reset(busnum, adr, freq);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_i2c_select_gpio
 *---------------------------------------------------------------------------
 */
int
gfx_i2c_select_gpio(int clock, int data)
{
#if GFX_I2C_ACCESS
    if (gfx_i2c_type & GFX_I2C_TYPE_ACCESS)
        acc_i2c_select_gpio(clock, data);
#endif
#if GFX_I2C_GPIO
    if (gfx_i2c_type & GFX_I2C_TYPE_GPIO)
        gpio_i2c_select_gpio(clock, data);
#endif
    return (0);
}

/*---------------------------------------------------------------------------
 * gfx_i2c_write
 *---------------------------------------------------------------------------
 */
int
gfx_i2c_write(unsigned char busnum, unsigned char chipadr,
              unsigned char subadr, unsigned char bytes, unsigned char *data)
{
    int status = -1;

#if GFX_I2C_ACCESS
    if (gfx_i2c_type & GFX_I2C_TYPE_ACCESS)
        status = acc_i2c_write(busnum, chipadr, subadr, bytes, data);
#endif
#if GFX_I2C_GPIO
    if (gfx_i2c_type & GFX_I2C_TYPE_GPIO)
        status = gpio_i2c_write(busnum, chipadr, subadr, bytes, data);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_i2c_read
 *---------------------------------------------------------------------------
 */
int
gfx_i2c_read(unsigned char busnum, unsigned char chipadr,
             unsigned char subadr, unsigned char bytes, unsigned char *data)
{
    int status = -1;

#if GFX_I2C_ACCESS
    if (gfx_i2c_type & GFX_I2C_TYPE_ACCESS)
        status = acc_i2c_read(busnum, chipadr, subadr, bytes, data);
#endif
#if GFX_I2C_GPIO
    if (gfx_i2c_type & GFX_I2C_TYPE_GPIO)
        status = gpio_i2c_read(busnum, chipadr, subadr, bytes, data);
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_i2c_init
 *---------------------------------------------------------------------------
 */
int
gfx_i2c_init(void)
{
    int status = -1;

#if GFX_I2C_ACCESS
    if (gfx_i2c_type & GFX_I2C_TYPE_ACCESS)
        status = acc_i2c_init();
#endif
#if GFX_I2C_GPIO
    if (gfx_i2c_type & GFX_I2C_TYPE_GPIO)
        status = gpio_i2c_init();
#endif
    return (status);
}

/*---------------------------------------------------------------------------
 * gfx_i2c_cleanup
 *---------------------------------------------------------------------------
 */
void
gfx_i2c_cleanup(void)
{
#if GFX_I2C_ACCESS
    if (gfx_i2c_type & GFX_I2C_TYPE_ACCESS)
        acc_i2c_cleanup();
#endif
#if GFX_I2C_GPIO
    if (gfx_i2c_type & GFX_I2C_TYPE_GPIO)
        gpio_i2c_cleanup();
#endif
}

#endif                          /* GFX_I2C_DYNAMIC */

/* END OF FILE */
