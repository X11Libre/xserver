/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#include <dix-config.h>

#include <stddef.h>

#include "dix/rpcbuf_priv.h"

Bool x_rpcbuf_makeroom(x_rpcbuf_t *rpcbuf, size_t needed)
{
    /* break out of alreay in error state */
    if (rpcbuf->error)
        return FALSE;

    /* not allocated yet ? */
    if (!rpcbuf->buffer) {
        if (!(rpcbuf->buffer = calloc(1, XLIBRE_RPCBUF_CHUNK_SIZE)))
            goto err;
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
        goto err;

    memset(newbuf + rpcbuf->size, 0, newsize - rpcbuf->size);
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
    memset(rpcbuf->buffer, 0, rpcbuf->size);
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

Bool x_rpcbuf_write_string_pad(x_rpcbuf_t *rpcbuf, const char *str)
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

Bool x_rpcbuf_write_string_0t_pad(x_rpcbuf_t *rpcbuf, const char *str)
{
    if (!str)
        return x_rpcbuf_write_CARD32(rpcbuf, 0);

    size_t slen = strlen(str);
    if (!slen)
        return x_rpcbuf_write_CARD32(rpcbuf, 0);

    char *reserved = x_rpcbuf_reserve(rpcbuf, pad_to_int32(slen+1));
    if (!reserved)
        return FALSE;

    memcpy(reserved, str, slen+1);
    return TRUE;
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

Bool x_rpcbuf_write_binary_pad(x_rpcbuf_t *rpcbuf, const void *values,
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
