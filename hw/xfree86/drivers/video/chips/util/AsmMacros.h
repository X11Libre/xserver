/*
 * (c) Copyright 1993,1994 by David Wexelblat <dwex@xfree86.org>
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
 * DAVID WEXELBLAT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of David Wexelblat shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from David Wexelblat.
 *
 */
/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 * This software is furnished under license and may be used and copied only in 
 * accordance with the following terms and conditions.  Subject to these 
 * conditions, you may download, copy, install, use, modify and distribute 
 * this software in source and/or binary form. No title or ownership is 
 * transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and retain 
 *    this copyright notice and list of conditions as they appear in the source
 *    file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of Digital 
 *    Equipment Corporation. Neither the "Digital Equipment Corporation" name 
 *    nor any trademark or logo of Digital Equipment Corporation may be used 
 *    to endorse or promote products derived from this software without the 
 *    prior written permission of Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied warranties, 
 *    including but not limited to, any implied warranties of merchantability, 
 *    fitness for a particular purpose, or non-infringement are disclaimed. In 
 *    no event shall DIGITAL be liable for any damages whatsoever, and in 
 *    particular, DIGITAL shall not be liable for special, indirect, 
 *    consequential, or incidental damages or damages for 
 *    lost profits, loss of revenue or loss of use, whether such damages arise 
 *    in contract, 
 *    negligence, tort, under statute, in equity, at law or otherwise, even if 
 *    advised of the possibility of such damage. 
 *
 */


#if defined(__GNUC__)
#if defined(linux) && (defined(__alpha__) || defined(__ia64__))
#undef	inb
#undef	inw
#undef	inl
#undef	outb
#undef	outw
#undef	outl
#define inb _inb
#define inw _inw
#define inl _inl
#define outb(p,v) _outb((v),(p))
#define outw(p,v) _outw((v),(p))
#define outl(p,v) _outl((v),(p))
#else
#if defined(__sparc__)
#ifndef ASI_PL
#define ASI_PL 0x88
#endif

static __inline__ void
outb(unsigned long port, char val)
{
  __asm__ __volatile__("stba %0, [%1] %2" : : "r" (val), "r" (port), "i" (ASI_PL));
}

static __inline__ void
outw(unsigned long port, char val)
{
  __asm__ __volatile__("stha %0, [%1] %2" : : "r" (val), "r" (port), "i" (ASI_PL));
}

static __inline__ void
outl(unsigned long port, char val)
{
  __asm__ __volatile__("sta %0, [%1] %2" : : "r" (val), "r" (port), "i" (ASI_PL));
}

static __inline__ unsigned int
inb(unsigned long port)
{
   unsigned char ret;
   __asm__ __volatile__("lduba [%1] %2, %0" : "=r" (ret) : "r" (port), "i" (ASI_PL));
   return ret;
}

static __inline__ unsigned int
inw(unsigned long port)
{
   unsigned char ret;
   __asm__ __volatile__("lduha [%1] %2, %0" : "=r" (ret) : "r" (port), "i" (ASI_PL));
   return ret;
}

static __inline__ unsigned int
inl(unsigned long port)
{
   unsigned char ret;
   __asm__ __volatile__("lda [%1] %2, %0" : "=r" (ret) : "r" (port), "i" (ASI_PL));
   return ret;
}
#else
#ifdef __arm32__
unsigned int IOPortBase;  /* Memory mapped I/O port area */

static __inline__ void
outb(short port, char val)
{
	 if ((unsigned short)port >= 0x400) return;

	*(volatile unsigned char*)(((unsigned short)(port))+IOPortBase) = val;
}

static __inline__ void
outw(short port, short val)
{
	 if ((unsigned short)port >= 0x400) return;

	*(volatile unsigned short*)(((unsigned short)(port))+IOPortBase) = val;
}

static __inline__ void
outl(short port, int val)
{
	 if ((unsigned short)port >= 0x400) return;

	*(volatile unsigned long*)(((unsigned short)(port))+IOPortBase) = val;
}

static __inline__ unsigned int
inb(short port)
{
	 if ((unsigned short)port >= 0x400) return((unsigned int)-1);

	return(*(volatile unsigned char*)(((unsigned short)(port))+IOPortBase));
}

static __inline__ unsigned int
inw(short port)
{
	 if ((unsigned short)port >= 0x400) return((unsigned int)-1);

	return(*(volatile unsigned short*)(((unsigned short)(port))+IOPortBase));
}

static __inline__ unsigned int
inl(short port)
{
	 if ((unsigned short)port >= 0x400) return((unsigned int)-1);

	return(*(volatile unsigned long*)(((unsigned short)(port))+IOPortBase));
}
#else /* __arm32__ */
#if defined(__FreeBSD__) && defined(__alpha__)

#include <sys/types.h>

extern void outb(u_int32_t port, u_int8_t val);
extern void outw(u_int32_t port, u_int16_t val);
extern void outl(u_int32_t port, u_int32_t val);
extern u_int8_t inb(u_int32_t port);
extern u_int16_t inw(u_int32_t port);
extern u_int32_t inl(u_int32_t port);

#else
#ifdef GCCUSESGAS
static __inline__ void
outb(short port, char val)
{
   __asm__ __volatile__("outb %0,%1" : :"a" (val), "d" (port));
}

static __inline__ void
outw(short portm, short val)
{
   __asm__ __volatile__("outw %0,%1" : :"a" (val), "d" (port));
}

static __inline__ void
outl(short port, unsigned int val)
{
   __asm__ __volatile__("outl %0,%1" : :"a" (val), "d" (port));
}

static __inline__ unsigned int
inb(short port)
{
   unsigned char ret;
   __asm__ __volatile__("inb %1,%0" :
       "=a" (ret) :
       "d" (port));
   return ret;
}

static __inline__ unsigned int
inw(short port)
{
   unsigned short ret;
   __asm__ __volatile__("inw %1,%0" :
       "=a" (ret) :
       "d" (port));
   return ret;
}

static __inline__ unsigned int
inl(short port)
{
   unsigned int ret;
   __asm__ __volatile__("inl %1,%0" :
       "=a" (ret) :
       "d" (port));
   return ret;
}

#else /* GCCUSESGAS */

static __inline__ void
outb(short port, char val)
{
  __asm__ __volatile__("out%B0 (%1)" : :"a" (val), "d" (port));
}

static __inline__ void
outw(short port, short val)
{
  __asm__ __volatile__("out%W0 (%1)" : :"a" (val), "d" (port));
}

static __inline__ void
outl(short port, unsigned int val)
{
  __asm__ __volatile__("out%L0 (%1)" : :"a" (val), "d" (port));
}

static __inline__ unsigned int
inb(short port)
{
  unsigned int ret;
  __asm__ __volatile__("in%B0 (%1)" :
                   "=a" (ret) :
                   "d" (port));
  return ret;
}

static __inline__ unsigned int
inw(short port)
{
  unsigned int ret;
  __asm__ __volatile__("in%W0 (%1)" :
                   "=a" (ret) :
                   "d" (port));
  return ret;
}

static __inline__ unsigned int
inl(short port)
{
  unsigned int ret;
  __asm__ __volatile__("in%L0 (%1)" :
                   "=a" (ret) :
                   "d" (port));
  return ret;
}

#endif /* GCCUSESGAS */
#endif /* arm32 */
#endif /* linux && __sparc__ */
#endif /* linux && __alpha__ */
#endif /* __FreeBSD__ && __alpha__ */

#if defined(linux) || defined(__arm32__)

#define intr_disable()
#define intr_enable()

#else 

static __inline__ void
intr_disable(void)
{
  __asm__ __volatile__("cli");
}

static __inline__ void
intr_enable(void)
{
  __asm__ __volatile__("sti");
}

#endif /* else !linux && !__arm32__ */

#else /* __GNUC__ */

#if defined(_MINIX) && defined(_ACK)

/* inb, outb, inw and outw are defined in the library */
/* ... but I've no idea if the same is true for inl & outl */

u8_t inb(U16_t);
void outb(U16_t, U8_t);
u16_t inw(U16_t);
void outw(U16_t, U16_t);
u32_t inl(U16_t);
void outl(U16_t, U32_t);

#else /* not _MINIX and _ACK */

# if defined(__STDC__) && (__STDC__ == 1)
#  define asm __asm
# endif
# ifdef SVR4
#  include <sys/types.h>
# endif
# include <sys/inline.h>
# define intr_disable() asm("cli")
# define intr_enable()  asm("sti")

#endif /* _MINIX and _ACK */
#endif /* __GNUC__ */
