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
 * OTHER DEALINGS IN THE SOFTWARE.
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

#include "xlibre_membarrier.h" /* backwards compat, remove in ABI-26 */

#if defined(__alpha__)

#ifdef __linux__
/* for Linux on Alpha, we use the LIBC _inx/_outx routines */
/* note that the appropriate setup via "ioperm" needs to be done */
/*  *before* any inx/outx is done. */

extern _X_EXPORT void _outb(unsigned char val, unsigned long port);
extern _X_EXPORT void _outw(unsigned short val, unsigned long port);
extern _X_EXPORT void _outl(unsigned int val, unsigned long port);
extern _X_EXPORT unsigned int _inb(unsigned long port);
extern _X_EXPORT unsigned int _inw(unsigned long port);
extern _X_EXPORT unsigned int _inl(unsigned long port);

static inline void
outb(unsigned long port, unsigned char val)
{
    _outb(val, port);
}

static inline void
outw(unsigned long port, unsigned short val)
{
    _outw(val, port);
}

static inline void
outl(unsigned long port, unsigned int val)
{
    _outl(val, port);
}

static inline unsigned int
inb(unsigned long port)
{
    return _inb(port);
}

static inline unsigned int
inw(unsigned long port)
{
    return _inw(port);
}

static inline unsigned int
inl(unsigned long port)
{
    return _inl(port);
}

#endif                          /* __linux__ */

#if (defined(__FreeBSD__) || defined(__OpenBSD__))

/* for FreeBSD and OpenBSD on Alpha, we use the libio (resp. libalpha) */
/*  inx/outx routines */
/* note that the appropriate setup via "ioperm" needs to be done */
/*  *before* any inx/outx is done. */

extern _X_EXPORT void outb(unsigned int port, unsigned char val);
extern _X_EXPORT void outw(unsigned int port, unsigned short val);
extern _X_EXPORT void outl(unsigned int port, unsigned int val);
extern _X_EXPORT unsigned char inb(unsigned int port);
extern _X_EXPORT unsigned short inw(unsigned int port);
extern _X_EXPORT unsigned int inl(unsigned int port);

#endif                          /* (__FreeBSD__ || __OpenBSD__ ) */

#if defined(__NetBSD__)
#include <machine/pio.h>
#endif                          /* __NetBSD__ */

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
    __asm__ __volatile__ (".word 0x8143e00a" : : : "memory");
}

static inline void
outw(unsigned long port, unsigned short val)
{
    __asm__ __volatile__("stha %0, [%1] %2":    /* No outputs */
                         :"r"(val), "r"(port), "i"(ASI_PL));
    __asm__ __volatile__ (".word 0x8143e00a" : : : "memory");
}

static inline void
outl(unsigned long port, unsigned int val)
{
    __asm__ __volatile__("sta %0, [%1] %2":     /* No outputs */
                         :"r"(val), "r"(port), "i"(ASI_PL));
    __asm__ __volatile__ (".word 0x8143e00a" : : : "memory");
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

static inline unsigned char
xf86ReadMmio8(__volatile__ void *base, const unsigned long offset)
{
    unsigned long addr = ((unsigned long) base) + offset;
    unsigned char ret;

    __asm__ __volatile__("lduba [%1] %2, %0":"=r"(ret)
                         :"r"(addr), "i"(ASI_PL));

    return ret;
}

static inline unsigned short
xf86ReadMmio16Be(__volatile__ void *base, const unsigned long offset)
{
    unsigned long addr = ((unsigned long) base) + offset;
    unsigned short ret;

    __asm__ __volatile__("lduh [%1], %0":"=r"(ret)
                         :"r"(addr));

    return ret;
}

static inline unsigned short
xf86ReadMmio16Le(__volatile__ void *base, const unsigned long offset)
{
    unsigned long addr = ((unsigned long) base) + offset;
    unsigned short ret;

    __asm__ __volatile__("lduha [%1] %2, %0":"=r"(ret)
                         :"r"(addr), "i"(ASI_PL));

    return ret;
}

static inline unsigned int
xf86ReadMmio32Be(__volatile__ void *base, const unsigned long offset)
{
    unsigned long addr = ((unsigned long) base) + offset;
    unsigned int ret;

    __asm__ __volatile__("ld [%1], %0":"=r"(ret)
                         :"r"(addr));

    return ret;
}

static inline unsigned int
xf86ReadMmio32Le(__volatile__ void *base, const unsigned long offset)
{
    unsigned long addr = ((unsigned long) base) + offset;
    unsigned int ret;

    __asm__ __volatile__("lda [%1] %2, %0":"=r"(ret)
                         :"r"(addr), "i"(ASI_PL));

    return ret;
}

static inline void
xf86WriteMmio8(__volatile__ void *base, const unsigned long offset,
               const unsigned int val)
{
    unsigned long addr = ((unsigned long) base) + offset;

    __asm__ __volatile__("stba %0, [%1] %2":    /* No outputs */
                         :"r"(val), "r"(addr), "i"(ASI_PL));
    __asm__ __volatile__ (".word 0x8143e00a" : : : "memory");
}

static inline void
xf86WriteMmio16Be(__volatile__ void *base, const unsigned long offset,
                  const unsigned int val)
{
    unsigned long addr = ((unsigned long) base) + offset;

    __asm__ __volatile__("sth %0, [%1]":        /* No outputs */
                         :"r"(val), "r"(addr));
    __asm__ __volatile__ (".word 0x8143e00a" : : : "memory");
}

static inline void
xf86WriteMmio16Le(__volatile__ void *base, const unsigned long offset,
                  const unsigned int val)
{
    unsigned long addr = ((unsigned long) base) + offset;

    __asm__ __volatile__("stha %0, [%1] %2":    /* No outputs */
                         :"r"(val), "r"(addr), "i"(ASI_PL));
    __asm__ __volatile__ (".word 0x8143e00a" : : : "memory");
}

static inline void
xf86WriteMmio32Be(__volatile__ void *base, const unsigned long offset,
                  const unsigned int val)
{
    unsigned long addr = ((unsigned long) base) + offset;

    __asm__ __volatile__("st %0, [%1]": /* No outputs */
                         :"r"(val), "r"(addr));
    __asm__ __volatile__ (".word 0x8143e00a" : : : "memory");
}

static inline void
xf86WriteMmio32Le(__volatile__ void *base, const unsigned long offset,
                  const unsigned int val)
{
    unsigned long addr = ((unsigned long) base) + offset;

    __asm__ __volatile__("sta %0, [%1] %2":     /* No outputs */
                         :"r"(val), "r"(addr), "i"(ASI_PL));
    __asm__ __volatile__ (".word 0x8143e00a" : : : "memory");
}

#elif defined(__arm32__) && !defined(__linux__)

extern _X_EXPORT unsigned int IOPortBase;      /* Memory mapped I/O port area */

static inline void
outb(unsigned long port, unsigned char val)
{
    *(volatile unsigned char *) (((unsigned long) (port)) + IOPortBase) =
        val;
}

static inline void
outw(unsigned long port, unsigned short val)
{
    *(volatile unsigned short *) (((unsigned long) (port)) + IOPortBase) =
        val;
}

static inline void
outl(unsigned long port, unsigned int val)
{
    *(volatile unsigned int *) (((unsigned long) (port)) + IOPortBase) =
        val;
}

static inline unsigned int
inb(unsigned long port)
{
    return *(volatile unsigned char *) (((unsigned long) (port)) +
                                        IOPortBase);
}

static inline unsigned int
inw(unsigned long port)
{
    return *(volatile unsigned short *) (((unsigned long) (port)) +
                                         IOPortBase);
}

static inline unsigned int
inl(unsigned long port)
{
    return *(volatile unsigned int *) (((unsigned long) (port)) +
                                       IOPortBase);
}

#elif defined(__mips__)

#  if defined(__mips64)
#    define __XLIBRE_MIPS_PORT_TYPE unsigned long
#    define __XLIBRE_MIPS_PORT_ADDR(port) (((unsigned long)(port)) + IOPortBase)
#  else
#    define __XLIBRE_MIPS_PORT_TYPE unsigned short
#    define __XLIBRE_MIPS_PORT_ADDR(port) (((unsigned short)(port)) + IOPortBase)
#  endif

extern _X_EXPORT unsigned int IOPortBase;      /* Memory mapped I/O port area */

static inline void outb(__XLIBRE_MIPS_PORT_TYPE port, unsigned char val) {
    *(volatile unsigned char *)(__XLIBRE_MIPS_PORT_ADDR(port)) = val;
}

static inline void outw(__XLIBRE_MIPS_PORT_TYPE port, unsigned short val) {
    *(volatile unsigned short *)(__XLIBRE_MIPS_PORT_ADDR(port)) = val;
}

static inline void outl(__XLIBRE_MIPS_PORT_TYPE port, unsigned int val) {
    *(volatile unsigned int *)(__XLIBRE_MIPS_PORT_ADDR(port)) = val;
}

static inline unsigned int inb(__XLIBRE_MIPS_PORT_TYPE port) {
    return *(volatile unsigned char *)(__XLIBRE_MIPS_PORT_ADDR(port));
}

static inline unsigned int inw(__XLIBRE_MIPS_PORT_TYPE port) {
    return *(volatile unsigned short *)(__XLIBRE_MIPS_PORT_ADDR(port));
}

static inline unsigned int inl(__XLIBRE_MIPS_PORT_TYPE port) {
    return *(volatile unsigned int *)(__XLIBRE_MIPS_PORT_ADDR(port));
}

#  undef __XLIBRE_MIPS_PORT_TYPE
#  undef __XLIBRE_MIPS_PORT_ADDR

#  ifdef __linux__                    /* don't mess with other OSs */
#    if X_BYTE_ORDER == X_BIG_ENDIAN
static inline unsigned int
xf86ReadMmio32Be(__volatile__ void *base, const unsigned long offset)
{
    unsigned long addr = ((unsigned long) base) + offset;
    unsigned int ret;

    __asm__ __volatile__("lw %0, 0(%1)":"=r"(ret)
                         :"r"(addr));

    return ret;
}

static inline void
xf86WriteMmio32Be(__volatile__ void *base, const unsigned long offset,
                  const unsigned int val)
{
    unsigned long addr = ((unsigned long) base) + offset;

    __asm__ __volatile__("sw %0, 0(%1)":        /* No outputs */
                         :"r"(val), "r"(addr));
}
#    endif /* X_BYTE_ORDER == X_BIG_ENDIAN */
#  endif /* __linux__ */

#elif defined(__powerpc__)

#include <sys/mman.h>

extern _X_EXPORT volatile unsigned char *ioBase;

static inline unsigned char
xf86ReadMmio8(__volatile__ void *base, const unsigned long offset)
{
    register unsigned char val;
    __asm__ __volatile__("lbzx %0,%1,%2\n\t" "eieio":"=r"(val)
                         :"b"(base), "r"(offset),
                         "m"(*((volatile unsigned char *) base + offset)));
    return val;
}

static inline unsigned short
xf86ReadMmio16Be(__volatile__ void *base, const unsigned long offset)
{
    register unsigned short val;
    __asm__ __volatile__("lhzx %0,%1,%2\n\t" "eieio":"=r"(val)
                         :"b"(base), "r"(offset),
                         "m"(*((volatile unsigned char *) base + offset)));
    return val;
}

static inline unsigned short
xf86ReadMmio16Le(__volatile__ void *base, const unsigned long offset)
{
    register unsigned short val;
    __asm__ __volatile__("lhbrx %0,%1,%2\n\t" "eieio":"=r"(val)
                         :"b"(base), "r"(offset),
                         "m"(*((volatile unsigned char *) base + offset)));
    return val;
}

static inline unsigned int
xf86ReadMmio32Be(__volatile__ void *base, const unsigned long offset)
{
    register unsigned int val;
    __asm__ __volatile__("lwzx %0,%1,%2\n\t" "eieio":"=r"(val)
                         :"b"(base), "r"(offset),
                         "m"(*((volatile unsigned char *) base + offset)));
    return val;
}

static inline unsigned int
xf86ReadMmio32Le(__volatile__ void *base, const unsigned long offset)
{
    register unsigned int val;
    __asm__ __volatile__("lwbrx %0,%1,%2\n\t" "eieio":"=r"(val)
                         :"b"(base), "r"(offset),
                         "m"(*((volatile unsigned char *) base + offset)));
    return val;
}

static inline void
xf86WriteMmio8(__volatile__ void *base, const unsigned long offset,
               const unsigned char val)
{
    __asm__
        __volatile__("stbx %1,%2,%3\n\t":"=m"
                     (*((volatile unsigned char *) base + offset))
                     :"r"(val), "b"(base), "r"(offset));
    __builtin_ppc_eieio();
}

static inline void
xf86WriteMmio16Le(__volatile__ void *base, const unsigned long offset,
                  const unsigned short val)
{
    __asm__
        __volatile__("sthbrx %1,%2,%3\n\t":"=m"
                     (*((volatile unsigned char *) base + offset))
                     :"r"(val), "b"(base), "r"(offset));
    __builtin_ppc_eieio();
}

static inline void
xf86WriteMmio16Be(__volatile__ void *base, const unsigned long offset,
                  const unsigned short val)
{
    __asm__
        __volatile__("sthx %1,%2,%3\n\t":"=m"
                     (*((volatile unsigned char *) base + offset))
                     :"r"(val), "b"(base), "r"(offset));
    __builtin_ppc_eieio();
}

static inline void
xf86WriteMmio32Le(__volatile__ void *base, const unsigned long offset,
                  const unsigned int val)
{
    __asm__
        __volatile__("stwbrx %1,%2,%3\n\t":"=m"
                     (*((volatile unsigned char *) base + offset))
                     :"r"(val), "b"(base), "r"(offset));
    __builtin_ppc_eieio();
}

static inline void
xf86WriteMmio32Be(__volatile__ void *base, const unsigned long offset,
                  const unsigned int val)
{
    __asm__
        __volatile__("stwx %1,%2,%3\n\t":"=m"
                     (*((volatile unsigned char *) base + offset))
                     :"r"(val), "b"(base), "r"(offset));
    __builtin_ppc_eieio();
}

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

#endif                          /* arch madness */

/* drivers that want to prevent automatic byteswapping by MMIO_()* macros
   on PPC and SPARC should set these */
#if !defined(MMIO_IS_BE) && \
    (defined(SPARC_MMIO_IS_BE) || defined(PPC_MMIO_IS_BE))
#define MMIO_IS_BE
#endif

#ifdef __alpha__

static inline int MMIO_IN8(void *Base, unsigned long Offset)
{
    mem_barrier();
    return *(CARD8 *) ((unsigned long) Base + (Offset));
}

static inline int MMIO_IN16(void *Base, unsigned long Offset)
{
    mem_barrier();
    return *(CARD16 *) ((unsigned long) Base + (Offset));
}

static inline int MMIO_IN32(void *Base, unsigned long Offset)
{
    mem_barrier();
    return *(CARD32 *) ((unsigned long) Base + (Offset));
}

static inline void MMIO_OUT8(void *Base, unsigned long Offset, int Value)
{
    write_mem_barrier();
    *(CARD8 *) ((unsigned long) Base + (Offset)) = Value;
}

static inline void MMIO_OUT16(void *Base, unsigned long Offset, int Value)
{
    write_mem_barrier();
    *(CARD16 *) ((unsigned long) Base + (Offset)) = Value;
}

static inline void MMIO_OUT32(void *Base, unsigned long Offset, int Value)
{
    write_mem_barrier();
    *(CARD32 *) ((unsigned long) Base + (Offset)) = Value;
}

extern _X_EXPORT void xf86SlowBCopyFromBus(unsigned char *, unsigned char *,
                                           int);
extern _X_EXPORT void xf86SlowBCopyToBus(unsigned char *, unsigned char *, int);

#elif defined(__powerpc__) || defined(__sparc__)
 /*
  * we provide byteswapping and no byteswapping functions here
  * with byteswapping as default,
  * drivers that don't need byteswapping should define MMIO_IS_BE
  */
#define MMIO_IN8(base, offset) xf86ReadMmio8((base), (offset))
#define MMIO_OUT8(base, offset, val) \
    xf86WriteMmio8((base), (offset), (CARD8)(val))

#if defined(MMIO_IS_BE)     /* No byteswapping */
#define MMIO_IN16(base, offset) xf86ReadMmio16Be((base), (offset))
#define MMIO_IN32(base, offset) xf86ReadMmio32Be((base), (offset))
#define MMIO_OUT16(base, offset, val) \
    xf86WriteMmio16Be((base), (offset), (CARD16)(val))
#define MMIO_OUT32(base, offset, val) \
    xf86WriteMmio32Be((base), (offset), (CARD32)(val))
#else                           /* byteswapping is the default */
#define MMIO_IN16(base, offset) xf86ReadMmio16Le((base), (offset))
#define MMIO_IN32(base, offset) xf86ReadMmio32Le((base), (offset))
#define MMIO_OUT16(base, offset, val) \
     xf86WriteMmio16Le((base), (offset), (CARD16)(val))
#define MMIO_OUT32(base, offset, val) \
     xf86WriteMmio32Le((base), (offset), (CARD32)(val))
#endif

#else                           /* !__alpha__ && !__powerpc__ && !__sparc__ */

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

#endif                          /* __alpha__ */

/*
 * With Intel, the version in os-support/misc/SlowBcopy.s is used.
 * This avoids port I/O during the copy (which causes problems with
 * some hardware).
 */
#ifdef __alpha__
#define slowbcopy_tobus(src,dst,count) xf86SlowBCopyToBus((src),(dst),(count))
#define slowbcopy_frombus(src,dst,count) xf86SlowBCopyFromBus((src),(dst),(count))
#else                           /* __alpha__ */
#define slowbcopy_tobus(src,dst,count) xf86SlowBcopy((src),(dst),(count))
#define slowbcopy_frombus(src,dst,count) xf86SlowBcopy((src),(dst),(count))
#endif                          /* __alpha__ */

/* NOLINTEND(hicpp-no-assembler) */

#endif                          /* _COMPILER_H */
