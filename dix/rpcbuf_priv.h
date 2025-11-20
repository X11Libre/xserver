/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_RPCBUF_PRIV_H
#define _XSERVER_DIX_RPCBUF_PRIV_H

#include <stddef.h>

#include "include/os.h"

/*
 * buffer for easing RPC payload assembly
 *
 * the structure should be zero-initialized. subsequent operations will
 * automatically allocate enough buffer space under the hood
 *
 * Example:
 *
 * x_rpcbuf_t x_rpcbuf buf = { 0 };
 * x_rpcbuf_write_string(&buf, "hello world");
 * x_rpcbuf_write_CARD1&(&buf, 91126);
 * ...
 * ...
 * do_write_out(buf->buffer, buf->wpos);
 * x_rpcbuf_clear(&buf);
 */

typedef struct x_rpcbuf {
    size_t size;    /* total size of buffer */
    size_t wpos;    /* length of data inside the buffer / next write position */
    char *buffer;   /* pointer to whole buffer */
    Bool swapped;   /* TRUE when typed write operation shall byte-swap */
    Bool error;     /* TRUE when the last allocation failed */
    Bool err_clear; /* set to TRUE if should automatically clear on error */
} x_rpcbuf_t;

#define XLIBRE_RPCBUF_CHUNK_SIZE 4096

/*
 * make sure there's enough room for `needed` bytes in the buffer.
 *
 * @param rpcbuf    pointer to x_rpcbuf_t to operate on
 * @param needed    amount of free space needed in the buffer
 * @return          TRUE if there (now) is enough room, FALSE on alloc failure
 */
Bool x_rpcbuf_makeroom(x_rpcbuf_t *rpcbuf, size_t needed)
    _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * clear rpcbuf and free all held memory.
 *
 * @param rpcbuf    pointer to x_rpcbuf_t to operate on
 */
void x_rpcbuf_clear(x_rpcbuf_t *rpcbuf)
    _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * reset rpcbuf and clear memory, but doesn't free it.
 *
 * this is for reusing existing buffers for different purpose, w/o
 * having to go through new allocatons.
 *
 * @param rpcbuf    pointer to x_rpcbuf_t to operate on
 */
void x_rpcbuf_reset(x_rpcbuf_t *rpcbuf)
    _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * reserve a piece of buffer and move the buffer pointer forward.
 *
 * the returned poiner can be used to directly write data into the
 * reserved region. buffer pointer is moved right after that region.
 *
 * NOTE: that region is only valid until another operation on this
 * buffer that might affect the allocated memory block: when buffer
 * needs to be resized, it may get a new memory location.
 *
 * @param rpcbuf    pointer to x_rpcbuf_t to operate on
 * @param needed    amount of bytes needed
 * @return          pointer to reserved region of NULL on allocation failure
 */
void *x_rpcbuf_reserve(x_rpcbuf_t *rpcbuf, size_t needed)
    _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * like x_rpcbuf_reserve(), but additionally clearing the reserved space.
 *
 * the returned poiner can be used to directly write data into the
 * reserved region. buffer pointer is moved right after that region.
 *
 * NOTE: that region is only valid until another operation on this
 * buffer that might affect the allocated memory block: when buffer
 * needs to be resized, it may get a new memory location.
 *
 * @param rpcbuf    pointer to x_rpcbuf_t to operate on
 * @param needed    amount of bytes needed
 * @return          pointer to reserved region of NULL on allocation failure
 */
void *x_rpcbuf_reserve0(x_rpcbuf_t *rpcbuf, size_t needed)
    _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * write a plain C string to rpc buffer and pad it.
 *
 * allocate a region for the string (padded to 32bits) and copy in the string.
 * if given string is NULL or zero-size, nothing happens.
 *
 * @param rpcbuf    pointer to x_rpcbuf_t to operate on
 * @param needed    string to plain C string
 * @return          TRUE on success, FALSE on allocation failure
 */
Bool x_rpcbuf_write_string_pad(x_rpcbuf_t *rpcbuf, const char *str)
    _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * write a plain C string with terminating 0 to rpc buffer and pad it.
 *
 * allocate a region for the string (padded to 32bits) and copy in the string.
 * if given string is NULL or zero-size, only a (CARD32)0 is written.
 *
 * @param rpcbuf    pointer to x_rpcbuf_t to operate on
 * @param needed    string to plain C string
 * @return          TRUE on success, FALSE on allocation failure
 */
Bool x_rpcbuf_write_string_0t_pad(x_rpcbuf_t *rpcbuf, const char *str)
    _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * write binary data to rpc buffer and pad it.
 *
 * allocate a region for the string (padded to 32bits) and copy in the data.
 * if given data is NULL or size is zero , nothing happens.
 *
 * @param rpcbuf    pointer to x_rpcbuf_t to operate on
 * @param needed    string to plain C string
 * @return          TRUE on success, FALSE on allocation failure
 */
Bool x_rpcbuf_write_binary_pad(x_rpcbuf_t *rpcbuf, const void *data,
                               size_t count) _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * write a CARD8
 *
 * allocate a region for CARD8 and write it into the buffer.
 *
 * doesn't do any padding.
 *
 * @param rpcbuf    pointer to x_rpcbuf_t to operate on
 * @param value     the CARD16 value to write
 * @return          TRUE on success, FALSE on allocation failure
 */
Bool x_rpcbuf_write_CARD8(x_rpcbuf_t *rpcbuf, CARD8 value)
    _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * write a CARD16 and do byte-swapping (when needed).
 *
 * allocate a region for CARD16, write it into the buffer and do byte-swap
 * if buffer is configured to do so (`swapped` field is TRUE).
 *
 * doesn't do any padding.
 *
 * @param rpcbuf    pointer to x_rpcbuf_t to operate on
 * @param value     the CARD16 value to write
 * @return          TRUE on success, FALSE on allocation failure
 */
Bool x_rpcbuf_write_CARD16(x_rpcbuf_t *rpcbuf, CARD16 value)
    _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * write a INT16 and do byte-swapping (when needed).
 *
 * allocate a region for INT16, write it into the buffer and do byte-swap
 * if buffer is configured to do so (`swapped` field is TRUE).
 *
 * doesn't do any padding.
 *
 * @param rpcbuf    pointer to x_rpcbuf_t to operate on
 * @param value     the CARD16 value to write
 * @return          TRUE on success, FALSE on allocation failure
 */
static inline Bool x_rpcbuf_write_INT16(x_rpcbuf_t *rpcbuf, INT16 value) {
    return x_rpcbuf_write_CARD16(rpcbuf, (CARD16)value);
}

/*
 * write a CARD32 and do byte-swapping (when needed).
 *
 * allocate a region for CARD32, write it into the buffer and do byte-swap
 * if buffer is configured to do so (`swapped` field is TRUE).
 *
 * doesn't do any padding.
 *
 * @param rpcbuf    pointer to x_rpcbuf_t to operate on
 * @param value     the CARD32 value to write
 * @return          TRUE on success, FALSE on allocation failure
 */
Bool x_rpcbuf_write_CARD32(x_rpcbuf_t *rpcbuf, CARD32 value)
    _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * write a INT32 and do byte-swapping (when needed).
 *
 * allocate a region for INT32, write it into the buffer and do byte-swap
 * if buffer is configured to do so (`swapped` field is TRUE).
 *
 * doesn't do any padding.
 *
 * @param rpcbuf    pointer to x_rpcbuf_t to operate on
 * @param value     the CARD16 value to write
 * @return          TRUE on success, FALSE on allocation failure
 */
static inline Bool x_rpcbuf_write_INT32(x_rpcbuf_t *rpcbuf, INT32 value) {
    return x_rpcbuf_write_CARD32(rpcbuf, (CARD32)value);
}

/*
 * write a CARD64 and do byte-swapping (when needed).
 *
 * allocate a region for CARD64, write it into the buffer and do byte-swap
 * if buffer is configured to do so (`swapped` field is TRUE).
 *
 * doesn't do any padding.
 *
 * @param rpcbuf    pointer to x_rpcbuf_t to operate on
 * @param value     the CARD64 value to write
 * @return          TRUE on success, FALSE on allocation failure
 */
Bool x_rpcbuf_write_CARD64(x_rpcbuf_t *rpcbuf, CARD64 value)
    _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * write array of CARD8s and do byte-swapping (when needed).
 *
 * allocate a region for CARD8, write them into the buffer.
 * when `values` or `count` are zero, does nothing.
 *
 * doesn't do any padding.
 *
 * @param rpcbuf    pointer to x_rpcbuf_t to operate on
 * @param values    pointer to CARD16 array to write
 * @param count     number of elements in the array
 * @return          TRUE on success, FALSE on allocation failure
 */
Bool x_rpcbuf_write_CARD8s(x_rpcbuf_t *rpcbuf, const CARD8 *values,
    size_t count) _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * write array of CARD16s and do byte-swapping (when needed).
 *
 * allocate a region for CARD16s, write them into the buffer and do byte-swap
 * if buffer is configured to do so (`swapped` field is TRUE).
 * when `values` or `count` are zero, does nothing.
 *
 * doesn't do any padding.
 *
 * @param rpcbuf    pointer to x_rpcbuf_t to operate on
 * @param values    pointer to CARD16 array to write
 * @param count     number of elements in the array
 * @return          TRUE on success, FALSE on allocation failure
 */
Bool x_rpcbuf_write_CARD16s(x_rpcbuf_t *rpcbuf, const CARD16 *values,
    size_t count) _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * write array of CARD32s and do byte-swapping (when needed).
 *
 * allocate a region for CARD32s, write them into the buffer and do byte-swap
 * if buffer is configured to do so (`swapped` field is TRUE).
 * when `values` or `count` are zero, does nothing.
 *
 * doesn't do any padding.
 *
 * @param rpcbuf    pointer to x_rpcbuf_t to operate on
 * @param values    pointer to CARD32 array to write
 * @param count     number of elements in the array
 * @return          TRUE on success, FALSE on allocation failure
 */
Bool x_rpcbuf_write_CARD32s(x_rpcbuf_t *rpcbuf, const CARD32 *values,
    size_t count) _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * write array of INT32s and do byte-swapping (when needed).
 *
 * allocate a region for INT32s, write them into the buffer and do byte-swap
 * if buffer is configured to do so (`swapped` field is TRUE).
 * when `values` or `count` are zero, does nothing.
 *
 * doesn't do any padding.
 *
 * @param rpcbuf    pointer to x_rpcbuf_t to operate on
 * @param values    pointer to INT32 array to write
 * @param count     number of elements in the array
 * @return          TRUE on success, FALSE on allocation failure
 */
static inline Bool x_rpcbuf_write_INT32s(x_rpcbuf_t *rpcbuf,
                                         const INT32 *values, size_t count)
{
    return x_rpcbuf_write_CARD32s(rpcbuf, (CARD32*)values, count);
}

/*
 * write array of CARD64s and do byte-swapping (when needed).
 *
 * allocate a region for CARD64s, write them into the buffer and do byte-swap
 * if buffer is configured to do so (`swapped` field is TRUE).
 * when `values` or `count` are zero, does nothing.
 *
 * doesn't do any padding.
 *
 * @param rpcbuf    pointer to x_rpcbuf_t to operate on
 * @param values    pointer to CARD64 array to write
 * @param count     number of elements in the array
 * @return          TRUE on success, FALSE on allocation failure
 */
Bool x_rpcbuf_write_CARD64s(x_rpcbuf_t *rpcbuf, const CARD64 *values,
    size_t count) _X_ATTRIBUTE_NONNULL_ARG(1);

/*
 * retrieve number of 4-byte-units (padded) of data written in the buffer
 *
 * @param rpcbuf    pointer to x_rpcbuf_t to operate on
 * @return          number of 4-byte units (w/ padding) written into the buffer
 */
static inline CARD32 x_rpcbuf_wsize_units(x_rpcbuf_t *rpcbuf) {
    return (CARD32)((rpcbuf->wpos + 3) / 4);
}

/*
 * pad the buffer to 4-byte-units (ie. write extra zeros if necessary)
 *
 * @param rpcbuf    pointer to x_rpcbuf_t to operate on
 */
static inline void x_rpcbuf_pad(x_rpcbuf_t *rpcbuf) {
    x_rpcbuf_reserve0(
        rpcbuf,
        (((rpcbuf->wpos + 3) / 4) * 4) - rpcbuf->wpos);
}

/*
 * write a Pascal-like counted string, starting with CARD16 couter,
 * followed by the char bytes, padded to full protocol units (4-bytes).
 *
 * if str is NULL, don't write anything
 *
 * @param rpcbuf    pointer to the x_rpcbuf_t to operate on
 * @param str       zero-terminated string to write into the buffer
 */
static inline void x_rpcbuf_write_counted_string_pad(
        x_rpcbuf_t *rpcbuf, const char *str)
{
    if (str) {
        CARD16 len = (CARD16)strlen(str); /* 64k should really be enough */
        x_rpcbuf_write_CARD16(rpcbuf, len);
        x_rpcbuf_write_CARD8s(rpcbuf, (CARD8*)str, len);
        x_rpcbuf_pad(rpcbuf);
    }
}

/*
 * write contents of an rpcbuf into another one (padded) and clear the source buffer
 *
 * @param rpcbuf    pointer to the x_rpcbuf_t to operate on
 * @param source    pointer to source x_rpcbuf_t
 */
static inline void x_rpcbuf_write_rpcbuf_pad(
        x_rpcbuf_t *rpcbuf, x_rpcbuf_t *source)
{
    if (!source)
        return;

    if (source->error) {
        rpcbuf->error = TRUE;
        if (rpcbuf->err_clear) {
            free(rpcbuf->buffer);
            rpcbuf->buffer = NULL;
        }
    } else {
        x_rpcbuf_write_binary_pad(rpcbuf, source->buffer, source->wpos);
    }
    x_rpcbuf_clear(source);
}

/*
 * write an X11 RECTANGLE protocol structure into the buffer
 *
 * @param rpcbuf    pointer to the x_rpcbuf_t to operate on
 * @param x         X value of the rectangle
 * @param y         Y value of the rectangle
 * @param width     WIDTH value of the rectangle
 * @param height    HEIGHT value of the rectangle
 */
static inline void x_rpcbuf_write_rect(
        x_rpcbuf_t *rpcbuf, INT16 x, INT16 y, CARD16 width, CARD16 height)
{
    x_rpcbuf_write_INT16(rpcbuf, x);
    x_rpcbuf_write_INT16(rpcbuf, y);
    x_rpcbuf_write_CARD16(rpcbuf, width);
    x_rpcbuf_write_CARD16(rpcbuf, height);
}

#endif /* _XSERVER_DIX_RPCBUF_PRIV_H */
