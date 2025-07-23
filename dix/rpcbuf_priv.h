/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_RPCBUF_PRIV_H
#define _XSERVER_DIX_RPCBUF_PRIV_H

#include <stddef.h>

#include "include/misc.h"
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
 * retrieve number of 4-byte-units (padded) of data written in the buffer
 *
 * @param rpcbuf    pointer to x_rpcbuf_t to operate on
 * @return          number of 4-byte units (w/ padding) written into the buffer
 */
static inline size_t x_rpcbuf_wsize_units(x_rpcbuf_t *rpcbuf) {
    return bytes_to_int32(pad_to_int32(rpcbuf->wpos));
}

#endif /* _XSERVER_DIX_RPCBUF_PRIV_H */
