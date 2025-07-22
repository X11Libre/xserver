/* Copyright (c) 2003-2006 Advanced Micro Devices, Inc.
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

/* The previous version of this file was way more complex then it should have
   been - remove the unnecessary #defines and routines, and concentrate on
   Linux for now.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <errno.h>
#include <compiler.h>
#include <os.h>                 /* ErrorF() */

/* Compiler options */

#define GFX_DISPLAY_GU1	       0        /* 1st generation display controller */
#define GFX_DISPLAY_GU2	       1        /* 2nd generation display controller */

#define GFX_INIT_DYNAMIC       0        /* runtime selection */
#define GFX_INIT_GU1           0        /* SC1200/GX1        */
#define GFX_INIT_GU2           1        /* GX                */

#define GFX_MSR_DYNAMIC        0        /* runtime selection */
#define GFX_MSR_REDCLOUD       1        /* GX */

#define GFX_2DACCEL_DYNAMIC    0        /* runtime selection                                */
#define GFX_2DACCEL_GU1	       0        /* 1st generation 2D accelerator    */
#define GFX_2DACCEL_GU2	       1        /* 2nd generation 2D accelerator    */

#define GFX_VIDEO_DYNAMIC      0        /* runtime selection  */
#define GFX_VIDEO_CS5530       0        /* support for CS5530 */
#define GFX_VIDEO_SC1200       0        /* support for SC1200 */
#define GFX_VIDEO_REDCLOUD     1        /* support for GX */

#define GFX_VIP_DYNAMIC	       0        /* runtime selection  */
#define GFX_VIP_SC1200	       0        /* support for SC1200 */

#define GFX_DECODER_DYNAMIC    0        /* runtime selection */
#define GFX_DECODER_SAA7114    0        /* Philips SAA7114 decoder */

#define GFX_TV_DYNAMIC	       0        /* runtime selection */
#define GFX_TV_FS451	       0        /* Focus Enhancements FS450  */
#define GFX_TV_SC1200	       0        /* SC1200 integrated TV encoder */

#define GFX_I2C_DYNAMIC	       0        /* runtime selection  */
#define GFX_I2C_ACCESS	       0        /* support for ACCESS.BUS  */
#define GFX_I2C_GPIO	       0        /* support for CS5530 GPIOs */

#define GFX_VGA_DYNAMIC	       0        /* runtime selection */
#define GFX_VGA_GU1	       0        /* 1st generation graphics unit */

#define FB4MB		       1        /* Set to use 4Mb vid ram for Pyramid */

#define GFX_NO_IO_IN_WAIT_MACROS    1   /* Set to remove I/O accesses in GP */
#define GFX_READ_ROUTINES  1

#include "gfx_rtns.h"
#include "gfx_priv.h"
#include "gfx_regs.h"
#include "gfx_defs.h"

unsigned char *gfx_virt_regptr = (unsigned char *) 0x40000000;
unsigned char *gfx_virt_fbptr = (unsigned char *) 0x40800000;
unsigned char *gfx_virt_vidptr = (unsigned char *) 0x40010000;
unsigned char *gfx_virt_vipptr = (unsigned char *) 0x40015000;
unsigned char *gfx_virt_spptr = (unsigned char *) 0x40000000;
unsigned char *gfx_virt_gpptr = (unsigned char *) 0x40000000;

unsigned char *gfx_phys_regptr = (unsigned char *) 0x40000000;
unsigned char *gfx_phys_fbptr = (unsigned char *) 0x40800000;
unsigned char *gfx_phys_vidptr = (unsigned char *) 0x40010000;
unsigned char *gfx_phys_vipptr = (unsigned char *) 0x40015000;

#define INB(port) inb(port)
#define INW(port) inw(port)
#define IND(port) inl(port)
#define OUTB(port,data) outb(port, data)
#define OUTW(port,data) outw(port, data)
#define OUTD(port,data) outl(port, data)

/* Squash warnings */
unsigned char gfx_inb(unsigned short);
unsigned short gfx_inw(unsigned short);
unsigned long gfx_ind(unsigned short);

void gfx_outb(unsigned short, unsigned char);
void gfx_outw(unsigned short, unsigned short);
void gfx_outd(unsigned short, unsigned long);

inline unsigned char
gfx_inb(unsigned short port)
{
    return inb(port);
}

inline unsigned short
gfx_inw(unsigned short port)
{
    return inw(port);
}

inline unsigned long
gfx_ind(unsigned short port)
{
    return inl(port);
}

inline void
gfx_outb(unsigned short port, unsigned char data)
{
    outb(port, data);
}

inline void
gfx_outw(unsigned short port, unsigned short data)
{
    outw(port, data);
}

inline void
gfx_outd(unsigned short port, unsigned long data)
{
    outl(port, data);
}

/* These routines use VSA to read the MSRs - these are a second resort to the Linux MSR method */

#define vsa_msr_read(msr,adr,high,low)      \
     __asm__ __volatile__(                      \
        " mov $0x0AC1C, %%edx\n"                \
        " mov $0xFC530007, %%eax\n"             \
        " out %%eax,%%dx\n"                     \
        " add $2,%%dl\n"                        \
        " in %%dx, %%ax"                        \
        : "=a" (*(low)), "=d" (*(high))         \
        : "c" (msr | adr))

#if __WORDSIZE == 64

#define vsa_msr_write(msr,adr,high,low) \
  { int32_t d0, d1, d2, d3, d4;        \
  __asm__ __volatile__(            \
    " push %%rbx\n"                \
    " mov $0x0AC1C, %%edx\n"       \
    " mov $0xFC530007, %%eax\n"    \
    " out %%eax,%%dx\n"            \
    " add $2,%%dl\n"               \
    " mov %6, %%ebx\n"             \
    " mov %7, %0\n"                \
    " mov %5, %3\n"                \
    " xor %2, %2\n"                \
    " xor %1, %1\n"                \
    " out %%ax, %%dx\n"            \
    " pop %%rbx\n"                 \
    : "=a"(d0),"=&D"(d1),"=&S"(d2), \
      "=c"(d3),"=d"(d4)  \
    : "1"(msr | adr),"2"(*(high)),"3"(*(low))); \
  }

#else

#define vsa_msr_write(msr,adr,high,low) \
  { int32_t d0, d1, d2, d3, d4;        \
  __asm__ __volatile__(            \
    " push %%ebx\n"                \
    " mov $0x0AC1C, %%edx\n"       \
    " mov $0xFC530007, %%eax\n"    \
    " out %%eax,%%dx\n"            \
    " add $2,%%dl\n"               \
    " mov %6, %%ebx\n"             \
    " mov %7, %0\n"                \
    " mov %5, %3\n"                \
    " xor %2, %2\n"                \
    " xor %1, %1\n"                \
    " out %%ax, %%dx\n"            \
    " pop %%ebx\n"                 \
    : "=a"(d0),"=&D"(d1),"=&S"(d2), \
      "=c"(d3),"=d"(d4)  \
    : "1"(msr | adr),"2"(*(high)),"3"(*(low))); \
  }

#endif

extern int GeodeWriteMSR(uint32_t, uint32_t, uint32_t);
extern int GeodeReadMSR(uint32_t, uint32_t *, uint32_t *);

void
gfx_msr_asm_write(unsigned short reg, uint32_t addr, uint32_t *hi, uint32_t *lo)
{
    static int msr_method = 0;

    if (msr_method == 0) {
        if (!GeodeWriteMSR(addr | reg, *lo, *hi))
            return;

        msr_method = 1;
    }

    /* This is the fallback VSA method - not preferred */
    vsa_msr_write(reg, addr, hi, lo);
}

void
gfx_msr_asm_read(unsigned short reg, uint32_t addr, uint32_t *hi, uint32_t *lo)
{
    static int msr_method = 0;

    if (msr_method == 0) {
        if (!GeodeReadMSR(addr | reg, lo, hi))
            return;

        ErrorF("Unable to read the MSR - reverting to the VSA method.\n");
        msr_method = 1;
    }

    /* This is the fallback VSA method - not preferred */
    vsa_msr_read(reg, addr, hi, lo);
}

#include "gfx_init.c"
#include "gfx_msr.c"
#include "gfx_rndr.c"
#include "gfx_mode.h"
#include "gfx_disp.c"
#include "gfx_vid.c"
#include "gfx_vip.c"
#include "gfx_dcdr.c"
#include "gfx_i2c.c"
#include "gfx_tv.c"
#include "gfx_vga.c"
