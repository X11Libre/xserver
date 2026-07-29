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

#ifndef _XLIBRE_XSERVER_COMPILER_PRIV_H
#define _XLIBRE_XSERVER_COMPILER_PRIV_H

#include "compiler.h"

/* Private header for xserver-internal MMIO functions and declarations.
 * This header should NOT be included by external drivers.
 * Only xserver source files (dix, hw, etc.) should include this.
 */

/* xf86SlowBCopyFromBus/ToBus are declared in compiler.h but defined in
 * hw/xfree86/os-support/misc/SlowBcopy.c. They are used by some drivers
 * (mga, sis, vesa) so the declarations stay in the public compiler.h.
 */

/* Alpha-specific function pointer declarations (used by bsd/alpha_video.c) */
#if defined(__alpha__)
extern int (*xf86ReadMmio8) (void *Base, unsigned long Offset);
extern int (*xf86ReadMmio16) (void *Base, unsigned long Offset);
extern int (*xf86ReadMmio32) (void *Base, unsigned long Offset);
extern void (*xf86WriteMmio8) (int Value, void *Base, unsigned long Offset);
extern void (*xf86WriteMmio16) (int Value, void *Base, unsigned long Offset);
extern void (*xf86WriteMmio32) (int Value, void *Base, unsigned long Offset);
#endif

#if defined(__GNUC__) && !defined(__amd64__) && !defined(__i386__) && !defined(__ia64__) && !defined(__sparc__) && !defined(__powerpc__)

/* SPARC-specific MMIO functions (only used via MMIO macros) */
#elif defined(__sparc__)

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

    barrier();
}

static inline void
xf86WriteMmio16Be(__volatile__ void *base, const unsigned long offset,
                  const unsigned int val)
{
    unsigned long addr = ((unsigned long) base) + offset;

    __asm__ __volatile__("sth %0, [%1]":        /* No outputs */
                         :"r"(val), "r"(addr));

    barrier();
}

static inline void
xf86WriteMmio16Le(__volatile__ void *base, const unsigned long offset,
                  const unsigned int val)
{
    unsigned long addr = ((unsigned long) base) + offset;

    __asm__ __volatile__("stha %0, [%1] %2":    /* No outputs */
                         :"r"(val), "r"(addr), "i"(ASI_PL));

    barrier();
}

static inline void
xf86WriteMmio32Be(__volatile__ void *base, const unsigned long offset,
                  const unsigned int val)
{
    unsigned long addr = ((unsigned long) base) + offset;

    __asm__ __volatile__("st %0, [%1]": /* No outputs */
                         :"r"(val), "r"(addr));

    barrier();
}

static inline void
xf86WriteMmio32Le(__volatile__ void *base, const unsigned long offset,
                  const unsigned int val)
{
    unsigned long addr = ((unsigned long) base) + offset;

    __asm__ __volatile__("sta %0, [%1] %2":     /* No outputs */
                         :"r"(val), "r"(addr), "i"(ASI_PL));

    barrier();
}

/* POWERPC-specific MMIO functions (only used via MMIO macros) */
#elif defined(__powerpc__)

#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

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
    eieio();
}

static inline void
xf86WriteMmio16Le(__volatile__ void *base, const unsigned long offset,
                  const unsigned short val)
{
    __asm__
        __volatile__("sthbrx %1,%2,%3\n\t":"=m"
                     (*((volatile unsigned char *) base + offset))
                     :"r"(val), "b"(base), "r"(offset));
    eieio();
}

static inline void
xf86WriteMmio16Be(__volatile__ void *base, const unsigned long offset,
                  const unsigned short val)
{
    __asm__
        __volatile__("sthx %1,%2,%3\n\t":"=m"
                     (*((volatile unsigned char *) base + offset))
                     :"r"(val), "b"(base), "r"(offset));
    eieio();
}

static inline void
xf86WriteMmio32Le(__volatile__ void *base, const unsigned long offset,
                  const unsigned int val)
{
    __asm__
        __volatile__("stwbrx %1,%2,%3\n\t":"=m"
                     (*((volatile unsigned char *) base + offset))
                     :"r"(val), "b"(base), "r"(offset));
    eieio();
}

static inline void
xf86WriteMmio32Be(__volatile__ void *base, const unsigned long offset,
                  const unsigned int val)
{
    __asm__
        __volatile__("stwx %1,%2,%3\n\t":"=m"
                     (*((volatile unsigned char *) base + offset))
                     :"r"(val), "b"(base), "r"(offset));
    eieio();
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

#endif

#endif /* _XLIBRE_XSERVER_COMPILER_PRIV_H */