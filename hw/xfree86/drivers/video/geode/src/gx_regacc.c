/* Copyright (c) 2003-2005 Advanced Micro Devices, Inc.
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
 * This is the main file used to add Durango graphics support to a software
 * project.  The main reason to have a single file include the other files
 * is that it centralizes the location of the compiler options.  This file
 * should be tuned for a specific implementation, and then modified as needed
 * for new Durango releases.  The releases.txt file indicates any updates to
 * this main file, such as a new definition for a new hardware platform.
 *
 * In other words, this file should be copied from the Durango source files
 * once when a software project starts, and then maintained as necessary.
 * It should not be recopied with new versions of Durango unless the
 * developer is willing to tune the file again for the specific project.
 * */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gfx_rtns.h"
#include "gfx_defs.h"
#include "gfx_regs.h"

#define GU2_WAIT_PENDING	\
		while(READ_GP32(MGP_BLT_STATUS) & MGP_BS_BLT_PENDING)
#define GU2_WAIT_HALF_EMPTY \
		while(!(READ_GP32(MGP_BLT_STATUS) & MGP_BS_HALF_EMPTY))

extern unsigned long gu2_pitch;
extern unsigned long gu2_xshift;
extern unsigned short GFXpatternFlags;
extern unsigned long gu2_rop32;
extern unsigned short gu2_blt_mode;

void gfx_write_reg8(unsigned long offset, unsigned char value);
void gfx_write_reg16(unsigned long offset, unsigned short value);
void gfx_write_reg32(unsigned long offset, unsigned long value);
unsigned short gfx_read_reg16(unsigned long offset);
unsigned long gfx_read_reg32(unsigned long offset);
void gfx_write_vid32(unsigned long offset, unsigned long value);
unsigned long gfx_read_vid32(unsigned long offset);
unsigned long gfx_read_vip32(unsigned long offset);
void gfx_write_vip32(unsigned long offset, unsigned long value);
void gfx_mono_bitmap_to_screen_blt_swp(unsigned short srcx,
                                       unsigned short srcy,
                                       unsigned short dstx,
                                       unsigned short dsty,
                                       unsigned short width,
                                       unsigned short height,
                                       unsigned char *data, short pitch);
unsigned int GetVideoMemSize(void);

/* ROUTINES added accessing hardware reg */
void
gfx_write_reg8(unsigned long offset, unsigned char value)
{
    WRITE_REG8(offset, value);
}

void
gfx_write_reg16(unsigned long offset, unsigned short value)
{
    WRITE_REG16(offset, value);
}

void
gfx_write_reg32(unsigned long offset, unsigned long value)
{
    WRITE_REG32(offset, value);
}

unsigned short
gfx_read_reg16(unsigned long offset)
{
    unsigned short value;

    value = READ_REG16(offset);
    return value;
}

unsigned long
gfx_read_reg32(unsigned long offset)
{
    unsigned long value;

    value = READ_REG32(offset);
    return value;
}

void
gfx_write_vid32(unsigned long offset, unsigned long value)
{
    WRITE_VID32(offset, value);
}

unsigned long
gfx_read_vid32(unsigned long offset)
{
    unsigned long value;

    value = READ_VID32(offset);
    return value;
}

/*Addition for the VIP code */
unsigned long
gfx_read_vip32(unsigned long offset)
{
    unsigned long value;

    value = READ_VIP32(offset);
    return value;
}

void
gfx_write_vip32(unsigned long offset, unsigned long value)
{
    WRITE_VIP32(offset, value);
}

#define SWAP_BITS_IN_BYTES(v) 								\
 (((0x01010101 & (v)) << 7) | ((0x02020202 & (v)) << 5) | 	\
  ((0x04040404 & (v)) << 3) | ((0x08080808 & (v)) << 1) | 	\
  ((0x10101010 & (v)) >> 1) | ((0x20202020 & (v)) >> 3) | 	\
  ((0x40404040 & (v)) >> 5) | ((0x80808080 & (v)) >> 7))

#define WRITE_GPREG_STRING32_SWP(regoffset, dwords, 					\
				counter, array, array_offset, temp)						\
{                                                           			\
	temp = (unsigned long)array + (array_offset);           			\
	for (counter = 0; counter < dwords; counter++)          			\
		WRITE_GP32 (regoffset, 											\
			SWAP_BITS_IN_BYTES(*((unsigned long *)temp + counter)));	\
}

void
gfx_mono_bitmap_to_screen_blt_swp(unsigned short srcx, unsigned short srcy,
                                  unsigned short dstx, unsigned short dsty,
                                  unsigned short width, unsigned short height,
                                  unsigned char *data, short pitch)
{
    unsigned long dstoffset, size, bytes;
    unsigned long offset, temp_offset, temp1 = 0, temp2 = 0;
    unsigned long i, j = 0, fifo_lines, dwords_extra, bytes_extra;
    unsigned long shift = 0;

    size = (((unsigned long) width) << 16) | height;

    /* CALCULATE STARTING OFFSETS */

    offset = (unsigned long) srcy *pitch + ((unsigned long) srcx >> 3);

    dstoffset = (unsigned long) dsty *gu2_pitch +
        (((unsigned long) dstx) << gu2_xshift);

    /* CHECK IF PATTERN ORIGINS NEED TO BE SET */

    if (GFXpatternFlags) {
        /* COMBINE X AND Y PATTERN ORIGINS WITH OFFSET */

        dstoffset |= ((unsigned long) (dstx & 7)) << 26;
        dstoffset |= ((unsigned long) (dsty & 7)) << 29;
    }

    bytes = ((srcx & 7) + width + 7) >> 3;
    fifo_lines = bytes >> 5;
    dwords_extra = (bytes & 0x0000001Cl) >> 2;
    bytes_extra = bytes & 0x00000003l;

    /* POLL UNTIL ABLE TO WRITE TO THE REGISTERS */
    /* Put off poll for as long as possible (do most calculations first).   */
    /* The source offset is always 0 since we allow misaligned dword reads. */
    /* Need to wait for busy instead of pending, since hardware clears      */
    /* the host data FIFO at the beginning of a BLT.                        */

    GU2_WAIT_PENDING;
    WRITE_GP32(MGP_RASTER_MODE, gu2_rop32);
    WRITE_GP32(MGP_SRC_OFFSET, ((unsigned long) srcx & 7) << 26);
    WRITE_GP32(MGP_DST_OFFSET, dstoffset);
    WRITE_GP32(MGP_WID_HEIGHT, size);
    WRITE_GP32(MGP_STRIDE, gu2_pitch);
    WRITE_GP16(MGP_BLT_MODE, gu2_blt_mode | MGP_BM_SRC_HOST | MGP_BM_SRC_MONO);

    /* WAIT FOR BLT TO BE LATCHED */

    GU2_WAIT_PENDING;

    /* WRITE ALL OF THE DATA TO THE HOST SOURCE REGISTER */

    while (height--) {
        temp_offset = offset;

        /* WRITE ALL FULL FIFO LINES */

        for (i = 0; i < fifo_lines; i++) {
            GU2_WAIT_HALF_EMPTY;
            WRITE_GPREG_STRING32_SWP(MGP_HST_SOURCE, 8, j, data, temp_offset,
                                     temp1);
            temp_offset += 32;
        }

        /* WRITE ALL FULL DWORDS */

        GU2_WAIT_HALF_EMPTY;
        if (dwords_extra) {
            WRITE_GPREG_STRING32_SWP(MGP_HST_SOURCE, dwords_extra, i, data,
                                     temp_offset, temp1);
            temp_offset += (dwords_extra << 2);
        }

        /* WRITE REMAINING BYTES */

        shift = 0;
        if (bytes_extra)
            WRITE_GPREG_STRING8(MGP_HST_SOURCE, bytes_extra, shift, i, data,
                                temp_offset, temp1, temp2);

        offset += pitch;
    }
}

unsigned int
GetVideoMemSize(void)
{
    unsigned int graphicsMemBaseAddr;
    unsigned int totalMem = 0;
    int i;
    unsigned int graphicsMemMask, graphicsMemShift;
    unsigned int mcBankCfg = gfx_read_reg32(0x8408);
    unsigned int dimmShift = 4;

    /* Read graphics base address. */

    graphicsMemBaseAddr = gfx_read_reg32(0x8414);

    graphicsMemMask = 0x7FF;
    graphicsMemShift = 19;

    /* Calculate total memory size for GXm. */

    for (i = 0; i < 2; i++) {
        if (((mcBankCfg >> dimmShift) & 0x7) != 0x7) {
            switch ((mcBankCfg >> (dimmShift + 4)) & 0x7) {
            case 0:
                totalMem += 0x400000;
                break;
            case 1:
                totalMem += 0x800000;
                break;
            case 2:
                totalMem += 0x1000000;
                break;
            case 3:
                totalMem += 0x2000000;
                break;
            case 4:
                totalMem += 0x4000000;
                break;
            case 5:
                totalMem += 0x8000000;
                break;
            case 6:
                totalMem += 0x10000000;
                break;
            case 7:
                totalMem += 0x20000000;
                break;
            default:
                break;
            }
        }
        dimmShift += 16;
    }

    /* Calculate graphics memory base address */

    graphicsMemBaseAddr &= graphicsMemMask;
    graphicsMemBaseAddr <<= graphicsMemShift;

    return (totalMem - graphicsMemBaseAddr);
}

/* END OF FILE */
