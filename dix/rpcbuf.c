/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include <stddef.h>

#include "dix/rpcbuf_priv.h"

Bool x_rpcbuf_makeroom(struct x_rpcbuf *rpcbuf, size_t needed)
{
    /* not allocated yet ? */
    if (!rpcbuf->buffer) {
        if (!(rpcbuf->buffer = calloc(1, XLIBRE_RPCBUF_CHUNK_SIZE)))
            return FALSE;
        rpcbuf->size = XLIBRE_RPCBUF_CHUNK_SIZE;
        rpcbuf->wpos = 0;
    }

    /* still enough space */
    if (rpcbuf->size > rpcbuf->wpos + needed)
        return TRUE;

    const size_t newsize = ((needed / XLIBRE_RPCBUF_CHUNK_SIZE) + 1)
                                * XLIBRE_RPCBUF_CHUNK_SIZE;

    char *newbuf = realloc(rpcbuf->buffer, newsize);
    if (!newbuf)
        return FALSE;
    memset(newbuf + rpcbuf->size, 0, newsize - rpcbuf->size);
    rpcbuf->buffer = newbuf;
    rpcbuf->size = newsize;

    return TRUE;
}

void x_rpcbuf_clear(struct x_rpcbuf *rpcbuf)
{
    free(rpcbuf->buffer);
    memset(rpcbuf, 0, sizeof(struct x_rpcbuf));
}

void x_rpcbuf_reset(struct x_rpcbuf *rpcbuf)
{
    /* no need to reset if never been actually written to */
    if ((!rpcbuf->buffer) || (!rpcbuf->size) || (!rpcbuf->wpos))
        return;

    /* clear memory, but don't free it */
    memset(rpcbuf->buffer, 0, rpcbuf->size);
    rpcbuf->wpos = 0;
}

void *x_rpcbuf_reserve(struct x_rpcbuf *rpcbuf, size_t needed)
{
    if (!x_rpcbuf_makeroom(rpcbuf, needed))
        return NULL;

    void *pos = rpcbuf->buffer + rpcbuf->wpos;
    rpcbuf->wpos += needed;

    return pos;
}

Bool x_rpcbuf_write_string_pad(struct x_rpcbuf *rpcbuf, const char *str)
{
    if (!str)
        return TRUE;

    size_t slen = strlen(str);
    if (!slen)
        return TRUE;

    char *reserved = x_rpcbuf_reserve(rpcbuf, pad_to_int32(slen));
    if (!reserved)
        return FALSE;

    memcpy(reserved, str, slen);
    return TRUE;
}

Bool x_rpcbuf_write_CARD8(struct x_rpcbuf *rpcbuf, CARD8 value)
{
    CARD8 *reserved = x_rpcbuf_reserve(rpcbuf, sizeof(value));
    if (!reserved)
        return FALSE;

    *reserved = value;

    return TRUE;
}

Bool x_rpcbuf_write_CARD16(struct x_rpcbuf *rpcbuf, CARD16 value)
{
    CARD16 *reserved = x_rpcbuf_reserve(rpcbuf, sizeof(value));
    if (!reserved)
        return FALSE;

    *reserved = value;

    if (rpcbuf->swapped)
        swaps(reserved);

    return TRUE;
}

Bool x_rpcbuf_write_CARD32(struct x_rpcbuf *rpcbuf, CARD32 value)
{
    CARD32 *reserved = x_rpcbuf_reserve(rpcbuf, sizeof(value));
    if (!reserved)
        return FALSE;

    *reserved = value;

    if (rpcbuf->swapped)
        swapl(reserved);

    return TRUE;
}

Bool x_rpcbuf_write_CARD8s(struct x_rpcbuf *rpcbuf, const CARD8 *values,
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

Bool x_rpcbuf_write_CARD16s(struct x_rpcbuf *rpcbuf, const CARD16 *values,
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

Bool x_rpcbuf_write_CARD32s(struct x_rpcbuf *rpcbuf, const CARD32 *values,
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

Bool x_rpcbuf_write_binary_pad(struct x_rpcbuf *rpcbuf, const void *values,
                               size_t size)
{
    if ((!values) || (!size))
        return TRUE;

    void *reserved = x_rpcbuf_reserve(rpcbuf, pad_to_int32(size));
    if (!reserved)
        return FALSE;

    memcpy(reserved, values, size);

    return TRUE;
}
