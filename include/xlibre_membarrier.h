/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * @brief platform/architecture specific memory barrier functions
 *
 * Copyright © 2026 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef __XLIBRE_SDK_MEMBARRIER_H
#define __XLIBRE_SDK_MEMBARRIER_H

__attribute__((always_inline)) static inline void xlibre_mem_barrier_read(void)
{
#ifdef __i386__
#  ifdef __SSE2__
    __asm__ __volatile__ ("mfence" : : : "memory");
#  else
    __asm__ __volatile__ ("lock; addl $0,0(%%esp)" : : : "memory");
#  endif
#elif defined __alpha__
    __asm__ __volatile__ ("mb" : : : "memory");
#elif defined __amd64__
    __asm__ __volatile__ ("mfence" : : : "memory");
#elif defined __ia64__
    __asm__ __volatile__ ("mf" : : : "memory");
#elif defined __mips__
    __asm__ __volatile__(
        ".set   push\n\t"
        ".set   noreorder\n\t"
        ".set   mips2\n\t"
        "sync\n\t"
        ".set   pop"
        : /* no output */
        : /* no input */
        : "memory");
#elif defined __powerpc__
#  if defined(__powerpc64__) || !defined(__has_builtin) || !__has_builtin(__builtin_ppc_eieio)
    __asm__ __volatile__ ("eieio" : : : "memory");
#  else
    __builtin_ppc_eieio();
#  endif
#elif defined __sparc__
    /* NOP */
#endif
}

__attribute__((always_inline)) static inline void xlibre_mem_barrier_write(void)
{
#ifdef __i386__
#ifdef __SSE__
    __asm__ __volatile__ ("sfence" : : : "memory");
#else
    __asm__ __volatile__ ("lock; addl $0,0(%%esp)" : : : "memory");
#endif
#elif defined __alpha__
    __asm__ __volatile__ ("wmb" : : : "memory");
#elif defined __amd64__
    __asm__ __volatile__ ("sfence" : : : "memory");
#elif defined __ia64__
    __asm__ __volatile__ ("mf" : : : "memory");
#elif defined __mips__
    xlibre_mem_barrier_read();
#elif defined __powerpc__
#  if defined(__powerpc64__) || !defined(__has_builtin) || !__has_builtin(__builtin_ppc_eieio)
    __asm__ __volatile__ ("eieio" : : : "memory");
#  else
    __builtin_ppc_eieio();
#  endif
#elif defined __sparc__
    /* NOP */
#endif
}

/*
 * @deprecated just backwards compat with older drivers - will be removed in ABI-26
 */
__attribute__((always_inline)) static inline void mem_barrier(void) {
    xlibre_mem_barrier_read();
}

/*
 * @deprecated just backwards compat with older drivers - will be removed in ABI-26
 */
__attribute__((always_inline)) static inline void write_mem_barrier(void) {
    xlibre_mem_barrier_write();
}

#endif /* __XLIBRE_SDK_MEMBARRIER_H */
