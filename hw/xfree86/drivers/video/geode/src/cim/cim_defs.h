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
 * Cimarron hardware access macros.
 */

#ifndef _cim_defs_h
#define _cim_defs_h

#include <stdint.h>

/*-----------------------------------------*/
/*          MEMORY ACCESS MACROS           */
/*-----------------------------------------*/

#ifndef CIMARRON_EXCLUDE_REGISTER_ACCESS_MACROS

#define READ_GP32(offset) \
    (*(volatile unsigned long *)(cim_gp_ptr + (offset)))

#define READ_REG32(offset) \
    (*(volatile unsigned long *)(cim_vg_ptr + (offset)))

#define READ_FB32(offset) \
    (*(volatile unsigned long *)(cim_fb_ptr + (offset)))

#define WRITE_GP32(offset, value) \
	(*(volatile unsigned long *)(cim_gp_ptr + (offset))) = (value)

#define WRITE_REG32(offset, value) \
	(*(volatile unsigned long *)(cim_vg_ptr + (offset))) = (value)

#define WRITE_COMMAND32(offset, value) \
	(*(unsigned long *)(cim_cmd_ptr + (offset))) = (value)

#define WRITE_COMMAND8(offset, value) \
	(*(unsigned char *)(cim_cmd_ptr + (offset))) = (value)

#define WRITE_FB32(offset, value) \
	(*(unsigned long *)(cim_fb_ptr + (offset))) = (value)

#define READ_VID32(offset) \
    (*(volatile unsigned long *)(cim_vid_ptr + (offset)))

#define WRITE_VID32(offset, value) \
	(*(volatile unsigned long *)(cim_vid_ptr + (offset))) = (value)

#define READ_VIP32(offset) \
    (*(volatile unsigned long *)(cim_vip_ptr + (offset)))

#define WRITE_VIP32(offset, value) \
	(*(volatile unsigned long *)(cim_vip_ptr + (offset))) = (value)

#define READ_VOP32(offset) \
    (*(volatile unsigned long *)(cim_vid_ptr + (offset)))

#define WRITE_VOP32(offset, value) \
	(*(volatile unsigned long *)(cim_vid_ptr + (offset))) = (value)

#endif

/*-----------------------------------------*/
/*            GP POLLING MACROS            */
/*-----------------------------------------*/

#define GP3_WAIT_WRAP(variable) \
	while(((variable = READ_GP32 (GP3_CMD_READ)) > gp3_cmd_current) || \
           (variable <= (gp3_cmd_top + GP3_BLT_COMMAND_SIZE + GP3_BLT_COMMAND_SIZE + 96)))

#define GP3_WAIT_PRIMITIVE(variable) \
	while (((variable = READ_GP32 (GP3_CMD_READ)) > gp3_cmd_current) && \
            (variable <= (gp3_cmd_next + 96)))

#define GP3_WAIT_BUSY \
	while(READ_GP32 (GP3_BLT_STATUS) & GP3_BS_BLT_BUSY)

#define GP3_WAIT_PENDING \
	while(READ_GP32 (GP3_BLT_STATUS) & GP3_BS_BLT_PENDING)

/*-----------------------------------------------------------------*/
/* MSR MACROS                                                      */
/* These macros facilitate interaction with the model specific     */
/* registers in GeodeLX.  There are two included methods, direct   */
/* access using the rdmsr and wrmsr opcodes and an indirect method */
/* using VSAII.                                                    */
/*-----------------------------------------------------------------*/

#ifdef CIMARRON_INCLUDE_MSR_MACROS

#if CIMARRON_MSR_DIRECT_ASM

/*-----------------------------------------------------------------
 * MSR_READ
 * Read the contents of a 64 bit MSR into a data structure
 *-----------------------------------------------------------------*/

#define MSR_READ(msr_reg, device_add, data64_ptr)                           \
{                                                                           \
	unsigned long msr_add = (unsigned long)(msr_reg) |                      \
							(unsigned long)(device_add);                    \
	uint32_t data_high, data_low;                                      \
	_asm { mov ecx, msr_add   }                                             \
	_asm { rdmsr              }                                             \
	_asm { mov data_high, edx }                                             \
	_asm { mov data_low,  eax }                                             \
	                                                                        \
	((Q_WORD *)(data64_ptr))->high = data_high;                             \
	((Q_WORD *)(data64_ptr))->low  = data_low;                              \
}

/*-----------------------------------------------------------------
 * MSR_WRITE
 * Write the contents of a 64 bit data structure to a MSR.
 *-----------------------------------------------------------------*/

#define MSR_WRITE(msr_reg, device_add, data64_ptr)                          \
{                                                                           \
	unsigned long msr_add = (unsigned long)(msr_reg) |                      \
							(unsigned long)(device_add);                    \
	unsigned long data_high, data_low;                                      \
	                                                                        \
	data_high = ((Q_WORD *)(data64_ptr))->high;                             \
	data_low  = ((Q_WORD *)(data64_ptr))->low;                              \
		                                                                    \
	_asm { mov ecx, msr_add   }                                             \
	_asm { mov edx, data_high }                                             \
	_asm { mov eax, data_low  }                                             \
	_asm { wrmsr              }                                             \
}

#elif CIMARRON_MSR_VSA_IO

/*-----------------------------------------------------------------
 * MSR_READ
 * Read the contents of a 64 bit MSR into a data structure
 *-----------------------------------------------------------------*/

#define MSR_READ(msr_reg, device_add, data64_ptr)                           \
{                                                                           \
	unsigned long msr_add = (unsigned long)(msr_reg) |                      \
							(unsigned long)(device_add);                    \
	unsigned long data_high, data_low;                                      \
		                                                                    \
	_asm { mov    dx,  0x0AC1C      } 	                                    \
	_asm { mov    eax, 0x0FC530007  }                                       \
	_asm { out    dx, eax           }                                       \
		                                                                    \
	_asm { add    dl, 2             }                                       \
	_asm { mov    ecx, msr_add      }                                       \
	_asm { in     ax, dx            }                                       \
	_asm { mov    data_high, edx    }                                       \
	_asm { mov    data_low, eax     }                                       \
		                                                                    \
	((Q_WORD *)(data64_ptr))->high = data_high;                             \
	((Q_WORD *)(data64_ptr))->low  = data_low;                              \
}

/*-----------------------------------------------------------------
 * MSR_WRITE
 * Write the contents of a 64 bit data structure to a MSR.
 *-----------------------------------------------------------------*/

#define MSR_WRITE(msr_reg, device_add, data64_ptr)                          \
{                                                                           \
	unsigned long msr_add = (unsigned long)(msr_reg) |                      \
							(unsigned long)(device_add);                    \
	unsigned long data_high, data_low;                                      \
		                                                                    \
	data_high = ((Q_WORD *)(data64_ptr))->high;                             \
	data_low  = ((Q_WORD *)(data64_ptr))->low;                              \
		                                                                    \
	_asm { mov    dx,  0x0AC1C      } 	                                    \
	_asm { mov    eax, 0x0FC530007  }                                       \
	_asm { out    dx, eax           }                                       \
		                                                                    \
	_asm { add    dl, 2             }                                       \
	_asm { mov    ecx, msr_add      }                                       \
	_asm { mov    ebx, data_high    }                                       \
	_asm { mov    eax, data_low }                                           \
		                                                                    \
	_asm { mov    esi, 0 }                                                  \
	_asm { mov    edi, 0 }                                                  \
	_asm { out    dx, ax }                                                  \
}

#elif CIMARRON_MSR_ABSTRACTED_ASM

/*-----------------------------------------------------------------
 * MSR_READ
 * Read the contents of a 64 bit MSR into a data structure
 *-----------------------------------------------------------------*/

#define MSR_READ(msr,adr,val)                   \
     __asm__ __volatile__(                      \
        " mov $0x0AC1C, %%edx\n"                \
        " mov $0xFC530007, %%eax\n"             \
        " out %%eax,%%dx\n"                     \
        " add $2,%%dl\n"                        \
        " in %%dx, %%ax"                        \
        : "=a" ((val)->low), "=d" ((val)->high) \
        : "c" (msr | adr))

/*-----------------------------------------------------------------
 * MSR_WRITE
 * Write the contents of a 64 bit data structure to a MSR.
 *-----------------------------------------------------------------*/

#define MSR_WRITE(msr,adr,val)                                      \
     { int d0, d1, d2, d3;                                          \
     __asm__ __volatile__(                                          \
        " push %%ebx\n"                                             \
        " mov $0x0AC1C, %%edx\n"                                    \
        " mov $0xFC530007, %%eax\n"                                 \
        " out %%eax,%%dx\n"                                         \
        " add $2,%%dl\n"                                            \
        " mov %4, %3\n"                                             \
        " mov 0(%5), %%ebx\n"                                       \
        " mov 4(%5), %0\n"                                          \
        " xor %2, %2\n"                                             \
        " xor %1, %1\n"                                             \
        " out %%ax, %%dx\n"                                         \
        " pop %%ebx\n"                                              \
        : "=a" (d0), "=&D" (d1), "=&S" (d2), "=c" (d3)              \
        : "1" (msr | adr), "2" (val));                              \
     }

#elif CIMARRON_MSR_KERNEL_ROUTINE

#include "asm/msr.h"

/*-----------------------------------------------------------------
 * MSR_READ
 * Read the contents of a 64 bit MSR into a data structure
 *-----------------------------------------------------------------*/

#define MSR_READ(msr_reg, device_add, data64_ptr)                  \
{                                                                  \
	uint32_t addr, val1, val2;                                 \
                                                                   \
	addr = device_add | msr_reg;                                   \
	rdmsr (addr, val1, val2);                                      \
                                                                   \
	((Q_WORD *)(data64_ptr))->high = val2;                         \
	((Q_WORD *)(data64_ptr))->low  = val1;	                       \
}

/*-----------------------------------------------------------------
 * MSR_WRITE
 * Read the contents of a 64 bit data structure to a MSR.
 *-----------------------------------------------------------------*/

#define MSR_WRITE(msr_reg, device_add, data64_ptr)                 \
{                                                                  \
	uint32_t addr, val1, val2;                                 \
                                                                   \
	val2 = ((Q_WORD *)(data64_ptr))->high;                         \
	val1 = ((Q_WORD *)(data64_ptr))->low;                          \
                                                                   \
	addr = (device_add & 0xFFFF0000) | (unsigned long)msr_reg;     \
	wrmsr(addr, val1, val2);                                       \
}

#elif CIMARRON_MSR_HOOKS

#define MSR_READ(msr_reg, device_add, data64_ptr)                  \
{                                                                  \
	uint32_t addr, val1, val2;                                \
                                                                   \
	addr = device_add | msr_reg;                               \
	if (cim_rdmsr) {                                           \
		cim_rdmsr (addr, &val1, &val2);                    \
                                                                   \
		((Q_WORD *)(data64_ptr))->high = val2;             \
		((Q_WORD *)(data64_ptr))->low  = val1;	           \
	}                                                          \
}

#define MSR_WRITE(msr_reg, device_add, data64_ptr)                 \
{                                                                  \
	uint32_t addr, val1, val2;                                 \
                                                                   \
	val2 = ((Q_WORD *)(data64_ptr))->high;                         \
	val1 = ((Q_WORD *)(data64_ptr))->low;                          \
                                                                   \
	addr = (device_add & 0xFFFF0000) | (unsigned long)msr_reg;     \
	if (cim_wrmsr)                                                 \
		cim_wrmsr(addr, val1, val2);                           \
}
#endif

#endif                          /* #ifdef CIMARRON_INCLUDE_MSR_MACROS */

/*-----------------------------------------------------------------*/
/* STRING MACROS                                                   */
/* These macros are included to facilitate the optimization of     */
/* routines that write or copy large amounts of data. Two versions */
/* of these macros are included.  One is intended for operating    */
/* systems that allow the use of inline assembly, while the other  */
/* is a pure C implementation for stricter operating systems.      */
/*-----------------------------------------------------------------*/

#ifdef CIMARRON_INCLUDE_STRING_MACROS

#if CIMARRON_OPTIMIZE_ASSEMBLY

/*-----------------------------------------------------------------
 * WRITE_COMMAND_STRING32   
 * Write a series of DWORDs to the current command buffer offset  
 *-----------------------------------------------------------------*/

#define WRITE_COMMAND_STRING32(offset, dataptr, dataoffset, dword_count) \
{                                                                        \
	_asm { cld                           }                               \
	_asm { mov edi, cim_cmd_ptr          }                               \
	_asm { add edi, offset               }                               \
	_asm { mov esi, dataptr              }                               \
	_asm { add esi, dataoffset           }                               \
	_asm { mov ecx, dword_count          }                               \
	_asm { rep movsd                     }                               \
}

/*-----------------------------------------------------------------
 * WRITE_FB_STRING32
 * Write a series of DWORDS to video memory.
 *-----------------------------------------------------------------*/

#define WRITE_FB_STRING32(offset, dataptr, dword_count)                  \
{                                                                        \
    unsigned long temp = (unsigned long)(dataptr);                       \
	_asm { cld                           }                               \
	_asm { mov edi, cim_fb_ptr           }                               \
	_asm { add edi, offset               }                               \
	_asm { mov esi, temp                 }                               \
	_asm { mov ecx, dword_count          }                               \
	_asm { rep movsd                     }                               \
}

/*-----------------------------------------------------------------
 * WRITE_FB_CONSTANT
 * Write a constant DWORD to multiple video memory addresses
 *-----------------------------------------------------------------*/

#define WRITE_FB_CONSTANT(offset, value, dword_count)                    \
{                                                                        \
	unsigned long outptr = (unsigned long)cim_fb_ptr + offset;           \
	unsigned long dwords = dword_count;                                  \
	_asm { cld                           }                               \
	_asm { mov edi, outptr               }                               \
	_asm { mov eax, value                }                               \
	_asm { mov ecx, dwords               }                               \
	_asm { rep stosd                     }                               \
}

/*-----------------------------------------------------------------
 * WRITE_HOST_SOURCE_STRING32
 * Write a series of DWORDs to the GP host source register
 *-----------------------------------------------------------------*/

#define WRITE_HOST_SOURCE_STRING32(dataptr, dataoffset, dword_count)     \
{                                                                        \
	_asm { cld                           }                               \
	_asm { mov edi, cim_gp_ptr           }                               \
	_asm { add edi, GP3_HST_SRC_RANGE    }                               \
	_asm { mov esi, dataptr              }                               \
	_asm { add esi, dataoffset           }                               \
	_asm { mov ecx, dword_count          }                               \
	_asm { rep movsd                     }                               \
}

#elif CIMARRON_OPTIMIZE_FORLOOP

/*-----------------------------------------------------------------
 * WRITE_COMMAND_STRING32   
 * Write a series of DWORDs to the current command buffer offset  
 *-----------------------------------------------------------------*/

#define WRITE_COMMAND_STRING32(offset, dataptr, dataoffset, dword_count) \
{                                                                        \
	unsigned long i;                                                     \
	unsigned long tempdata = (unsigned long)dataptr + (dataoffset);      \
	unsigned long byte_off = 0;                                          \
	for (i = 0; i < dword_count; i++, byte_off += 4)                     \
		WRITE_COMMAND32 ((offset) + byte_off,                            \
						    *((unsigned long *)(tempdata + byte_off)));  \
}

/*-----------------------------------------------------------------
 * WRITE_FB_STRING32
 * Write a series of DWORDS to video memory.
 *-----------------------------------------------------------------*/

#define WRITE_FB_STRING32(offset, dataptr, dword_count)                  \
{                                                                        \
	unsigned long i;                                                     \
	unsigned long tempdata = (unsigned long)dataptr;                     \
	unsigned long byte_off = 0;                                          \
	for (i = 0; i < dword_count; i++, byte_off += 4)                     \
		WRITE_FB32 ((offset) + byte_off,                                 \
					*((unsigned long *)(tempdata + byte_off)));          \
}

/*-----------------------------------------------------------------
 * WRITE_FB_CONSTANT
 * Write a constant DWORD to multiple video memory addresses
 *-----------------------------------------------------------------*/

#define WRITE_FB_CONSTANT(offset, value, dword_count)                    \
{                                                                        \
	unsigned long i;                                                     \
	unsigned long tempoffset = offset;                                   \
	for (i = 0; i < dword_count; i++, tempoffset += 4)                   \
		WRITE_FB32 (tempoffset, value);                                  \
}

/*-----------------------------------------------------------------
 * WRITE_HOST_SOURCE_STRING32
 * Write a series of DWORDs to the GP host source register
 *-----------------------------------------------------------------*/

#define WRITE_HOST_SOURCE_STRING32(dataptr, dataoffset, dword_count)     \
{                                                                        \
	unsigned long i;                                                     \
	unsigned long tempdata = (unsigned long)dataptr + (dataoffset);      \
	unsigned long byte_off = 0;                                          \
	for (i = 0; i < dword_count; i++, byte_off += 4)                     \
		WRITE_GP32 (byte_off + GP3_HST_SRC_RANGE,                        \
					*((unsigned long *)(tempdata + byte_off)));          \
}

#elif CIMARRON_OPTIMIZE_ABSTRACTED_ASM

#define move_dw(d,s,n)                                  \
  __asm__ __volatile__(                                 \
  " rep\n"                                              \
  " movsl\n"                                            \
  : "=&c" (d0), "=&S" (d1), "=&D" (d2)                  \
  : "0" (n), "1" ((const char *)s), "2" ((char *)d)     \
  : "memory")

/*-----------------------------------------------------------------
 * WRITE_COMMAND_STRING32   
 * Write a series of DWORDs to the current command buffer offset  
 *-----------------------------------------------------------------*/

#define WRITE_COMMAND_STRING32(offset, dataptr, dataoffset, dword_count) \
{                                                                        \
    int d0, d1, d2;                                                      \
    move_dw (cim_cmd_ptr+ ((unsigned long)(offset)),                     \
        ((unsigned long)(dataptr)+(dataoffset)),                         \
        dword_count);                                                    \
}

/*-----------------------------------------------------------------
 * WRITE_FB_STRING32
 * Write a series of DWORDS to video memory.
 *-----------------------------------------------------------------*/

#define WRITE_FB_STRING32(offset, dataptr, dword_count)                  \
{                                                                        \
	unsigned long i;                                                     \
	unsigned long tempdata = (unsigned long)dataptr;                     \
	unsigned long byte_off = 0;                                          \
	for (i = 0; i < dword_count; i++, byte_off += 4)                     \
		WRITE_FB32 ((offset) + byte_off,                                 \
					*((unsigned long *)(tempdata + byte_off)));          \
}

/*-----------------------------------------------------------------
 * WRITE_FB_CONSTANT
 * Write a constant DWORD to multiple video memory addresses
 *-----------------------------------------------------------------*/

#define WRITE_FB_CONSTANT(offset, value, dword_count)                    \
{                                                                        \
	unsigned long i;                                                     \
	unsigned long tempoffset = offset;                                   \
	for (i = 0; i < dword_count; i++, tempoffset += 4)                   \
		WRITE_FB32 (tempoffset, value);                                  \
}

/*-----------------------------------------------------------------
 * WRITE_HOST_SOURCE_STRING32
 * Write a series of DWORDs to the GP host source register
 *-----------------------------------------------------------------*/

#define WRITE_HOST_SOURCE_STRING32(dataptr, dataoffset, dword_count)     \
{                                                                        \
	unsigned long i;                                                     \
	unsigned long tempdata = (unsigned long)dataptr + (dataoffset);      \
	unsigned long byte_off = 0;                                          \
	for (i = 0; i < dword_count; i++, byte_off += 4)                     \
		WRITE_GP32 (byte_off + GP3_HST_SRC_RANGE,                        \
					*((unsigned long *)(tempdata + byte_off)));          \
}

#endif

#endif                          /* #ifdef CIMARRON_INCLUDE_STRING_MACROS */

/*-----------------------------------------------------------------
 * WRITE_COMMAND_STRING8
 * Write a series of bytes to the current command buffer offset  
 *-----------------------------------------------------------------*/

#define WRITE_COMMAND_STRING8(offset, dataptr, dataoffset, byte_count)   \
{                                                                        \
	unsigned long i;                                                     \
	unsigned long array = (unsigned long)dataptr + (dataoffset);         \
	for (i = 0; i < byte_count; i++)                                     \
		WRITE_COMMAND8 ((offset) + i, *((unsigned char *)(array + i)));  \
}

/*-----------------------------------------------------------------
 * WRITE_HOST_SOURCE_STRING8
 * Write a series of bytes to the host source register  
 *-----------------------------------------------------------------*/

#define WRITE_HOST_SOURCE_STRING8(dataptr, dataoffset, byte_count)      \
{                                                                       \
	unsigned long temp1 = (unsigned long)dataptr + (dataoffset);        \
	unsigned long temp2 = 0;                                            \
	unsigned long shift = 0;                                            \
	unsigned long counter;                                              \
    if (byte_count)                                                     \
    {                                                                   \
        for (counter = 0; counter < byte_count; counter++)              \
	    {                                                               \
		    temp2 |= ((unsigned long)(*((unsigned char *)               \
						(temp1 + counter)))) << shift;                  \
		    shift += 8;                                                 \
	    }                                                               \
	    WRITE_GP32 (GP3_HST_SRC, temp2);                                \
    }                                                                   \
}

/*-----------------------------------------*/
/*          CUSTOM STRING MACROS           */
/*-----------------------------------------*/

#ifndef CIMARRON_EXCLUDE_CUSTOM_MACROS

#define WRITE_CUSTOM_COMMAND_STRING32 WRITE_COMMAND_STRING32
#define WRITE_CUSTOM_COMMAND_STRING8  WRITE_COMMAND_STRING8

#endif

/*-----------------------------------------*/
/*             IO ACCESS MACROS            */
/*-----------------------------------------*/

#ifdef CIMARRON_INCLUDE_IO_MACROS

#if CIMARRON_IO_DIRECT_ACCESS

/*-------------------------------------------
 * OUTD
 * Writes one DWORD to a single I/O address.
 *-------------------------------------------*/

#define OUTD(port, data) cim_outd(port, data)
static inline void
cim_outd(unsigned short port, uint32_t data)
{
    _asm {
pushf mov eax, data mov dx, port out dx, eax popf}}
/*-------------------------------------------
 * IND
 * Reads one DWORD from a single I/O address.
 *-------------------------------------------*/
#define IND(port) cim_ind(port)
static inline uint32_t
cim_ind(unsigned short port)
{
    unsigned long data;

    _asm {
    pushf mov dx, port in eax, dx mov data, eax popf} return data;
}

/*-------------------------------------------
 * OUTW
 * Writes one WORD to a single I/O address.
 *-------------------------------------------*/

#define OUTW(port, data) cim_outw(port, data)
void
cim_outw(unsigned short port, unsigned short data)
{
    _asm {
pushf mov ax, data mov dx, port out dx, ax popf}}
/*-------------------------------------------
 * INW
 * Reads one WORD from a single I/O address.
 *-------------------------------------------*/
#define INW(port) cim_inw(port)
unsigned short
cim_inw(unsigned short port)
{
    unsigned short data;

    _asm {
    pushf mov dx, port in ax, dx mov data, ax popf} return data;
}

/*-------------------------------------------
 * OUTB
 * Writes one BYTE to a single I/O address.
 *-------------------------------------------*/

#define OUTB(port, data) cim_outb(port, data)
void
cim_outb(unsigned short port, unsigned char data)
{
    _asm {
pushf mov al, data mov dx, port out dx, al popf}}
/*-------------------------------------------
 * INB
 * Reads one BYTE from a single I/O address.
 *-------------------------------------------*/
#define INB(port) cim_inb(port)
unsigned char
cim_inb(unsigned short port)
{
    unsigned char data;

    _asm {
    pushf mov dx, port in al, dx mov data, al popf} return data;
}

#elif CIMARRON_IO_ABSTRACTED_ASM

/*-------------------------------------------
 * OUTD
 * Writes one DWORD to a single I/O address.
 *-------------------------------------------*/

#define OUTD(port, data) cim_outd(port, data)
static inline void
cim_outd(unsigned short port, uint32_t data)
{
    __asm__ __volatile__("outl %0,%w1"::"a"(data), "Nd"(port));
}

/*-------------------------------------------
 * IND
 * Reads one DWORD from a single I/O address.
 *-------------------------------------------*/

#define IND(port) cim_ind(port)
static inline uint32_t
cim_ind(unsigned short port)
{
    uint32_t value;
    __asm__ __volatile__("inl %w1,%0":"=a"(value):"Nd"(port));

    return value;
}

/*-------------------------------------------
 * OUTW
 * Writes one WORD to a single I/O address.
 *-------------------------------------------*/

#define OUTW(port, data) cim_outw(port, data)
void cim_outw(unsigned short port, unsigned short data);
void
cim_outw(unsigned short port, unsigned short data)
{
    __asm__ volatile ("out %0,%1"::"a" (data), "d"(port));
}

/*-------------------------------------------
 * INW
 * Reads one WORD from a single I/O address.
 *-------------------------------------------*/

#define INW(port) cim_inw(port)
unsigned short cim_inw(unsigned short port);
unsigned short
cim_inw(unsigned short port)
{
    unsigned short value;
    __asm__ volatile ("in %1,%0":"=a" (value):"d"(port));

    return value;
}

/*-------------------------------------------
 * INB
 * Reads one BYTE from a single I/O address.
 *-------------------------------------------*/

#define INB(port) cim_inb(port)
unsigned char cim_inb(unsigned short port);
unsigned char
cim_inb(unsigned short port)
{
    unsigned char value;
    __asm__ volatile ("inb %1,%0":"=a" (value):"d"(port));

    return value;
}

/*-------------------------------------------
 * OUTB
 * Writes one BYTE to a single I/O address.
 *-------------------------------------------*/

#define OUTB(port) cim_outb(port)
void cim_outb(unsigned short port, unsigned char data);
void
cim_outb(unsigned short port, unsigned char data)
{
    __asm__ volatile ("outb %0,%1"::"a" (data), "d"(port));
}

#endif

#endif                          /* CIMARRON_INCLUDE_IO_MACROS */

extern void (*cim_rdmsr)(uint32_t, uint32_t *, uint32_t *);
extern void (*cim_wrmsr)(uint32_t, uint32_t, uint32_t);

#endif
