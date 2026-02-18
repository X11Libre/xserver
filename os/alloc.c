/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 1987, 1998  The Open Group
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include <stdlib.h>
#ifdef HAVE_NUMA
#include <numa.h>
#endif

#include "include/os.h"
#include "os/osdep.h"

#ifdef HAVE_NUMA
static inline void *
numa_alloc_impl(size_t size)
{
    void *ptr;
    size_t total_size = size + sizeof(size_t);

    if (numa_available() != -1)
        ptr = numa_alloc_interleaved(total_size);
    else
        ptr = malloc(total_size);

    if (!ptr)
        return NULL;

    *(size_t *)ptr = size;
    return (char *)ptr + sizeof(size_t);
}

static inline void *
numa_calloc_impl(size_t nmemb, size_t size)
{
    void *ptr;
    size_t total_size = (nmemb * size) + sizeof(size_t);

    if (numa_available() != -1)
        ptr = numa_alloc_interleaved(total_size);
    else
        ptr = malloc(total_size);

    if (!ptr)
        return NULL;

    *(size_t *)ptr = nmemb * size;
    memset((char *)ptr + sizeof(size_t), 0, nmemb * size);
    return (char *)ptr + sizeof(size_t);
}

static inline void *
numa_realloc_impl(void *ptr, size_t size)
{
    void *real_ptr;
    void *new_ptr;
    size_t old_size;
    size_t total_size = size + sizeof(size_t);

    if (!ptr)
        return numa_alloc_impl(size);

    real_ptr = (char *)ptr - sizeof(size_t);
    old_size = *(size_t *)real_ptr;

    if (numa_available() != -1)
        new_ptr = numa_realloc(real_ptr, old_size + sizeof(size_t), total_size);
    else
        new_ptr = realloc(real_ptr, total_size);

    if (!new_ptr)
        return NULL;

    *(size_t *)new_ptr = size;
    return (char *)new_ptr + sizeof(size_t);
}
#endif

void *
XNFalloc(unsigned long amount)
{
    void *ptr;

#ifdef HAVE_NUMA
    if (numa_available() != -1)
        ptr = numa_alloc_impl(amount);
    else
#endif
        ptr = calloc(1, amount);

    if (!ptr)
        FatalError("Out of memory");
    return ptr;
}

/* The original XNFcalloc was used with the xnfcalloc macro which multiplied
 * the arguments at the call site without allowing calloc to check for overflow.
 * XNFcallocarray was added to fix that without breaking ABI.
 */
void *
XNFcalloc(unsigned long amount)
{
    return XNFcallocarray(1, amount);
}

void *
XNFcallocarray(size_t nmemb, size_t size)
{
    void *ret;

#ifdef HAVE_NUMA
    if (numa_available() != -1)
        ret = numa_calloc_impl(nmemb, size);
    else
#endif
        ret = calloc(nmemb, size);

    if (!ret)
        FatalError("XNFcalloc: Out of memory");
    return ret;
}

void *
XNFrealloc(void *ptr, unsigned long amount)
{
    void *ret;

#ifdef HAVE_NUMA
    if (numa_available() != -1)
        ret = numa_realloc_impl(ptr, amount);
    else
#endif
        ret = realloc(ptr, amount);

    if (!ret)
        FatalError("XNFrealloc: Out of memory");
    return ret;
}

void *
XNFreallocarray(void *ptr, size_t nmemb, size_t size)
{
    void *ret;

#ifdef HAVE_NUMA
    if (numa_available() != -1)
        ret = numa_realloc_impl(ptr, nmemb * size);
    else
#endif
        ret = reallocarray(ptr, nmemb, size);

    if (!ret)
        FatalError("XNFreallocarray: Out of memory");
    return ret;
}

void
Xfree(void *ptr)
{
    if (!ptr)
        return;

#ifdef HAVE_NUMA
    if (numa_available() != -1) {
        void *real_ptr = (char *)ptr - sizeof(size_t);
        size_t size = *(size_t *)real_ptr;
        numa_free(real_ptr, size + sizeof(size_t));
    }
    else
#endif
        free(ptr);
}
