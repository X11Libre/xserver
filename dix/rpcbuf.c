/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include <stddef.h>

#include "dix/dix_priv.h"
#include "dix/rpcbuf_priv.h"
#include "os/osdep.h"

static inline Bool __x_rpcbuf_write_bin_pad(
    x_rpcbuf_t *rpcbuf, const char *val, size_t len)
{
    const size_t blen = pad_to_int32(len);

    char *reserved = x_rpcbuf_reserve(rpcbuf, blen);
    if (!reserved)
        return FALSE;

    memcpy(reserved, val, len);
    memset(reserved + len, 0, blen - len);
    return TRUE;
}

Bool x_rpcbuf_makeroom(x_rpcbuf_t *rpcbuf, size_t needed)
{
    /* break out of alreay in error state */
    if (rpcbuf->error)
        return FALSE;

    /* still enough space */
    if (rpcbuf->size > rpcbuf->wpos + needed)
        return TRUE;

    const size_t newsize = (((rpcbuf->wpos + needed) / XLIBRE_RPCBUF_CHUNK_SIZE) + 1)
                                * XLIBRE_RPCBUF_CHUNK_SIZE;

    char *newbuf = realloc(rpcbuf->buffer, newsize);
    if (!newbuf)
        goto err;

    rpcbuf->buffer = newbuf;
    rpcbuf->size = newsize;

    return TRUE;

err:
    rpcbuf->error = TRUE;
    if (rpcbuf->err_clear) {
        free(rpcbuf->buffer);
        rpcbuf->buffer = NULL;
    }
    return FALSE;
}

_X_EXPORT /* only for GLX, not part of public ABI */
void x_rpcbuf_clear(x_rpcbuf_t *rpcbuf)
{
    free(rpcbuf->buffer);
    memset(rpcbuf, 0, sizeof(x_rpcbuf_t));
}

void x_rpcbuf_reset(x_rpcbuf_t *rpcbuf)
{
    /* no need to reset if never been actually written to */
    if ((!rpcbuf->buffer) || (!rpcbuf->size) || (!rpcbuf->wpos))
        return;

    /* clear memory, but don't free it */
    rpcbuf->wpos = 0;
}

void *x_rpcbuf_reserve(x_rpcbuf_t *rpcbuf, size_t needed)
{
    if (!x_rpcbuf_makeroom(rpcbuf, needed))
        return NULL;

    void *pos = rpcbuf->buffer + rpcbuf->wpos;
    rpcbuf->wpos += needed;

    return pos;
}

_X_EXPORT /* only for GLX, not part of public ABI */
void *x_rpcbuf_reserve0(x_rpcbuf_t *rpcbuf, size_t needed)
{
    void *buf = x_rpcbuf_reserve(rpcbuf, needed);
    if (!buf)
        return NULL;

    memset(buf, 0, needed);
    return buf;
}

Bool x_rpcbuf_write_string_pad(x_rpcbuf_t *rpcbuf, const char *str)
{
    if (!str)
        return TRUE;

    return __x_rpcbuf_write_bin_pad(rpcbuf, str, strlen(str));
}

_X_EXPORT /* only for GLX, not part of public ABI */
Bool x_rpcbuf_write_string_0t_pad(x_rpcbuf_t *rpcbuf, const char *str)
{
    if (!str)
        return x_rpcbuf_write_CARD32(rpcbuf, 0);

    return __x_rpcbuf_write_bin_pad(rpcbuf, str, strlen(str)+1);
}

Bool x_rpcbuf_write_CARD8(x_rpcbuf_t *rpcbuf, CARD8 value)
{
    CARD8 *reserved = x_rpcbuf_reserve(rpcbuf, sizeof(value));
    if (!reserved)
        return FALSE;

    *reserved = value;

    return TRUE;
}

Bool x_rpcbuf_write_CARD16(x_rpcbuf_t *rpcbuf, CARD16 value)
{
    CARD16 *reserved = x_rpcbuf_reserve(rpcbuf, sizeof(value));
    if (!reserved)
        return FALSE;

    *reserved = value;

    if (rpcbuf->swapped)
        swaps(reserved);

    return TRUE;
}

_X_EXPORT /* only for GLX, not part of public ABI */
Bool x_rpcbuf_write_CARD32(x_rpcbuf_t *rpcbuf, CARD32 value)
{
    CARD32 *reserved = x_rpcbuf_reserve(rpcbuf, sizeof(value));
    if (!reserved)
        return FALSE;

    *reserved = value;

    if (rpcbuf->swapped)
        swapl(reserved);

    return TRUE;
}

Bool x_rpcbuf_write_CARD64(x_rpcbuf_t *rpcbuf, CARD64 value)
{
    CARD64 *reserved = x_rpcbuf_reserve(rpcbuf, sizeof(value));
    if (!reserved)
        return FALSE;

    *reserved = value;

    if (rpcbuf->swapped)
        swapll(reserved);

    return TRUE;
}

_X_EXPORT /* only for GLX, not part of public ABI */
Bool x_rpcbuf_write_CARD8s(x_rpcbuf_t *rpcbuf, const CARD8 *values,
                           size_t count)
{
    if ((!values) || (!count))
        return TRUE;

    INT16 *reserved = x_rpcbuf_reserve(rpcbuf, count);
    if (!reserved)
        return FALSE;

    memcpy(reserved, values, count);

    return TRUE;
}

Bool x_rpcbuf_write_CARD16s(x_rpcbuf_t *rpcbuf, const CARD16 *values,
                            size_t count)
{
    if ((!values) || (!count))
        return TRUE;

    INT16 *reserved = x_rpcbuf_reserve(rpcbuf, sizeof(CARD16) * count);
    if (!reserved)
        return FALSE;

    memcpy(reserved, values, sizeof(CARD16) * count);

    if (rpcbuf->swapped)
        SwapShorts(reserved, count);

    return TRUE;
}

_X_EXPORT /* only for GLX, not part of public ABI */
Bool x_rpcbuf_write_CARD32s(x_rpcbuf_t *rpcbuf, const CARD32 *values,
                            size_t count)
{
    if ((!values) || (!count))
        return TRUE;

    CARD32 *reserved = x_rpcbuf_reserve(rpcbuf, sizeof(CARD32) * count);
    if (!reserved)
        return FALSE;

    memcpy(reserved, values, sizeof(CARD32) * count);

    if (rpcbuf->swapped)
        SwapLongs(reserved, count);

    return TRUE;
}

Bool x_rpcbuf_write_CARD64s(x_rpcbuf_t *rpcbuf, const CARD64 *values,
                            size_t count)
{
    if ((!values) || (!count))
        return TRUE;

    CARD64 *reserved = x_rpcbuf_reserve(rpcbuf, sizeof(CARD64) * count);
    if (!reserved)
        return FALSE;

    memcpy(reserved, values, sizeof(CARD64) * count);

    if (rpcbuf->swapped)
        for (size_t x=0; x<count; x++)
            swapll(&reserved[x]);

    return TRUE;
}

Bool x_rpcbuf_write_binary_pad(x_rpcbuf_t *rpcbuf, const void *values,
                               size_t size)
{
    if ((!values) || (!size))
        return TRUE;

    return __x_rpcbuf_write_bin_pad(rpcbuf, values, size);
}
