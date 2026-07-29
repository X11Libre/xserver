/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
/*
 * Copyright (c) 1994-2003 by The XFree86 Project, Inc.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#ifndef _COMPILER_H

#define _COMPILER_H

#ifndef _X_EXPORT
#include <X11/Xfuncproto.h>
#endif

#include <pixman.h>             /* for uint*_t types */

/* NOLINTBEGIN(hicpp-no-assembler) */

#ifdef __GNUC__
#ifdef __i386__

#ifdef __SSE__
#define write_mem_barrier() __asm__ __volatile__ ("sfence" : : : "memory")
#else
#define write_mem_barrier() __asm__ __volatile__ ("lock; addl $0,0(%%esp)" : : : "memory")
#endif

#ifdef __SSE2__
#define mem_barrier() __asm__ __volatile__ ("mfence" : : : "memory")
#else
#define mem_barrier() __asm__ __volatile__ ("lock; addl $0,0(%%esp)" : : : "memory")
#endif

#elif defined __amd64__

#define mem_barrier() __asm__ __volatile__ ("mfence" : : : "memory")
#define write_mem_barrier() __asm__ __volatile__ ("sfence" : : : "memory")

#elif defined __ia64__

#ifndef __INTEL_COMPILER
#define mem_barrier()        __asm__ __volatile__ ("mf" : : : "memory")
#define write_mem_barrier()  __asm__ __volatile__ ("mf" : : : "memory")
#else
#include "ia64intrin.h"
#define mem_barrier() __mf()
#define write_mem_barrier() __mf()
#endif

#elif defined __mips__
     /* Note: sync instruction requires MIPS II instruction set */
#define mem_barrier()		\
	__asm__ __volatile__(		\
		".set   push\n\t"	\
		".set   noreorder\n\t"	\
		".set   mips2\n\t"	\
		"sync\n\t"		\
		".set   pop"		\
		: /* no output */	\
		: /* no input */	\
		: "memory")
#define write_mem_barrier() mem_barrier()

#elif defined __powerpc__

#ifndef eieio
#define eieio() __asm__ __volatile__ ("eieio" ::: "memory")
#endif                          /* eieio */
#define mem_barrier()	eieio()
#define write_mem_barrier()	eieio()

#elif defined __sparc__

#define barrier() __asm__ __volatile__ (".word 0x8143e00a" : : : "memory")
#define mem_barrier()           /* XXX: nop for now */
#define write_mem_barrier()     /* XXX: nop for now */
#endif
#endif                          /* __GNUC__ */

#ifndef barrier
#define barrier()
#endif

#ifndef mem_barrier
#define mem_barrier()           /* NOP */
#endif

#ifndef write_mem_barrier
#define write_mem_barrier()     /* NOP */
#endif

#ifdef __GNUC__
#elif defined(__amd64__) || defined(__i386__) || defined(__ia64__)

#include <inttypes.h>

static inline void
outb(unsigned short port, unsigned char val)
{
    __asm__ __volatile__("outb %0,%1"::"a"(val), "d"(port));
}

static inline void
outw(unsigned short port, unsigned short val)
{
    __asm__ __volatile__("outw %0,%1"::"a"(val), "d"(port));
}

static inline void
outl(unsigned short port, unsigned int val)
{
    __asm__ __volatile__("outl %0,%1"::"a"(val), "d"(port));
}

static inline unsigned int
inb(unsigned short port)
{
    unsigned char ret;
    __asm__ __volatile__("inb %1,%0":"=a"(ret):"d"(port));

    return ret;
}

static inline unsigned int
inw(unsigned short port)
{
    unsigned short ret;
    __asm__ __volatile__("inw %1,%0":"=a"(ret):"d"(port));

    return ret;
}

static inline unsigned int
inl(unsigned short port)
{
    unsigned int ret;
    __asm__ __volatile__("inl %1,%0":"=a"(ret):"d"(port));

    return ret;
}

#elif defined(__sparc__)

#ifndef ASI_PL
#define ASI_PL 0x88
#endif

static inline void
outb(unsigned long port, unsigned char val)
{
    __asm__ __volatile__("stba %0, [%1] %2":    /* No outputs */
                         :"r"(val), "r"(port), "i"(ASI_PL));

    barrier();
}

static inline void
outw(unsigned long port, unsigned short val)
{
    __asm__ __volatile__("stha %0, [%1] %2":    /* No outputs */
                         :"r"(val), "r"(port), "i"(ASI_PL));

    barrier();
}

static inline void
outl(unsigned long port, unsigned int val)
{
    __asm__ __volatile__("sta %0, [%1] %2":     /* No outputs */
                         :"r"(val), "r"(port), "i"(ASI_PL));

    barrier();
}

static inline unsigned int
inb(unsigned long port)
{
    unsigned int ret;
    __asm__ __volatile__("lduba [%1] %2, %0":"=r"(ret)
                         :"r"(port), "i"(ASI_PL));

    return ret;
}

static inline unsigned int
inw(unsigned long port)
{
    unsigned int ret;
    __asm__ __volatile__("lduha [%1] %2, %0":"=r"(ret)
                         :"r"(port), "i"(ASI_PL));

    return ret;
}

static inline unsigned int
inl(unsigned long port)
{
    unsigned int ret;
    __asm__ __volatile__("lda [%1] %2, %0":"=r"(ret)
                         :"r"(port), "i"(ASI_PL));

    return ret;
}

#elif defined(__powerpc__)

#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

extern _X_EXPORT volatile unsigned char *ioBase;

static inline void
outb(unsigned short port, unsigned char value)
{
    if (ioBase == MAP_FAILED)
        return;
    xf86WriteMmio8((void *) ioBase, port, value);
}

static inline void
outw(unsigned short port, unsigned short value)
{
    if (ioBase == MAP_FAILED)
        return;
    xf86WriteMmio16Le((void *) ioBase, port, value);
}

static inline void
outl(unsigned short port, unsigned int value)
{
    if (ioBase == MAP_FAILED)
        return;
    xf86WriteMmio32Le((void *) ioBase, port, value);
}

static inline unsigned int
inb(unsigned short port)
{
    if (ioBase == MAP_FAILED)
        return 0;
    return xf86ReadMmio8((void *) ioBase, port);
}

static inline unsigned int
inw(unsigned short port)
{
    if (ioBase == MAP_FAILED)
        return 0;
    return xf86ReadMmio16Le((void *) ioBase, port);
}

static inline unsigned int
inl(unsigned short port)
{
    if (ioBase == MAP_FAILED)
        return 0;
    return xf86ReadMmio32Le((void *) ioBase, port);
}

#else                           /* !GNUC */
#if defined(__STDC__) && (__STDC__ == 1)
#ifndef asm
#define asm __asm
#endif
#endif
#include <sys/inline.h>
#endif                          /* __GNUC__ */

#if !defined(MMIO_IS_BE) && \
    (defined(SPARC_MMIO_IS_BE) || defined(PPC_MMIO_IS_BE))
#define MMIO_IS_BE
#endif

#define MMIO_IN8(base, offset) \
	*(volatile CARD8 *)(((CARD8*)(base)) + (offset))
#define MMIO_IN16(base, offset) \
	*(volatile CARD16 *)(void *)(((CARD8*)(base)) + (offset))
#define MMIO_IN32(base, offset) \
	*(volatile CARD32 *)(void *)(((CARD8*)(base)) + (offset))
#define MMIO_OUT8(base, offset, val) \
	*(volatile CARD8 *)(((CARD8*)(base)) + (offset)) = (val)
#define MMIO_OUT16(base, offset, val) \
	*(volatile CARD16 *)(void *)(((CARD8*)(base)) + (offset)) = (val)
#define MMIO_OUT32(base, offset, val) \
	*(volatile CARD32 *)(void *)(((CARD8*)(base)) + (offset)) = (val)

#define slowbcopy_tobus(src,dst,count) xf86SlowBcopy((src),(dst),(count))
#define slowbcopy_frombus(src,dst,count) xf86SlowBcopy((src),(dst),(count))

extern _X_EXPORT void xf86SlowBCopyFromBus(unsigned char *, unsigned char *,
                                           int);
extern _X_EXPORT void xf86SlowBCopyToBus(unsigned char *, unsigned char *, int);

/* NOLINTEND(hicpp-no-assembler) */

#endif                          /* _COMPILER_H */