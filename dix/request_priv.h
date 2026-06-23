/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_REQUEST_PRIV_H
#define _XSERVER_DIX_REQUEST_PRIV_H

#include <X11/Xproto.h>

#include "dix/rpcbuf_priv.h" /* x_rpcbuf_t */
#include "include/dix.h"
#include "include/dixstruct.h"
#include "include/misc.h"    /* bytes_to_int32 */
#include "os/io_priv.h"      /* dixWriteToClient */

/*
 * @brief write rpc buffer to client and then clear it
 *
 * @param pClient the client to write buffer to
 * @param rpcbuf  the buffer whose contents will be written
 * @return the result of dixWriteToClient() call
 */
static inline ssize_t WriteRpcbufToClient(ClientPtr pClient,
                                          x_rpcbuf_t *rpcbuf) {
    /* explicitly casting between (s)size_t and int - should be safe,
       since payloads are always small enough to easily fit into int. */
    ssize_t ret = dixWriteToClient(pClient,
                                (int)rpcbuf->wpos,
                                rpcbuf->buffer);
    x_rpcbuf_clear(rpcbuf);
    return ret;
}

/* compute the amount of extra units a reply header needs.
 *
 * all reply header structs are at least the size of xGenericReply
 * we have to count how many units the header is bigger than xGenericReply
 *
 */
#define X_REPLY_HEADER_UNITS(hdrtype) \
    (bytes_to_int32((sizeof(hdrtype) - sizeof(xGenericReply))))

static inline int __write_reply_hdr_and_rpcbuf(
    ClientPtr pClient, void *hdrData, size_t hdrLen, x_rpcbuf_t *rpcbuf)
{
    if (rpcbuf->error)
        return BadAlloc;

    xGenericReply *reply = hdrData;
    reply->type = X_Reply;
    reply->length = (bytes_to_int32(hdrLen - sizeof(xGenericReply)))
                  + x_rpcbuf_wsize_units(rpcbuf);
    reply->sequenceNumber = (CARD16)pClient->sequence; /* shouldn't go above 64k */

    if (pClient->swapped) {
         swaps(&reply->sequenceNumber);
         swapl(&reply->length);
    }

    dixWriteToClient(pClient, (int)hdrLen, hdrData);
    WriteRpcbufToClient(pClient, rpcbuf);

    return Success;
}

static inline int __write_reply_hdr_simple(
    ClientPtr pClient, void *hdrData, size_t hdrLen)
{
    xGenericReply *reply = hdrData;
    reply->type = X_Reply;
    reply->length = (bytes_to_int32(hdrLen - sizeof(xGenericReply)));
    reply->sequenceNumber = (CARD16)pClient->sequence; /* shouldn't go above 64k */

    if (pClient->swapped) {
         swaps(&reply->sequenceNumber);
         swapl(&reply->length);
    }

    dixWriteToClient(pClient, (int)hdrLen, hdrData);
    return Success;
}

/*
 * send reply with header struct (not pointer!) along with rpcbuf payload
 *
 * @param client      pointer to the client (ClientPtr)
 * @param hdrstruct   the header struct (not pointer, the struct itself!)
 * @param rpcbuf      the rpcbuf to send (not pointer, the struct itself!)
 * return             X11 result code
 */
#define X_SEND_REPLY_WITH_RPCBUF(client, hdrstruct, rpcbuf) \
    __write_reply_hdr_and_rpcbuf((client), &(hdrstruct), sizeof(hdrstruct), &(rpcbuf));

/*
 * send reply with header struct (not pointer!) without any payload
 *
 * @param client      pointer to the client (ClientPtr)
 * @param hdrstruct   the header struct (not pointer, the struct itself!)
 * @return            X11 result code (=Success)
 */
#define X_SEND_REPLY_SIMPLE(client, hdrstruct) \
    __write_reply_hdr_simple((client), &(hdrstruct), sizeof(hdrstruct));

/*
 * macros for request handlers
 *
 * these are handling request packet checking and swapping of multi-byte
 * values, if necessary. (length field is already swapped earlier)
 */

/* declare request struct and check size */
#define X_REQUEST_HEAD_STRUCT(type) \
    REQUEST(type); \
    if (stuff == NULL) return (BadLength); \
    REQUEST_SIZE_MATCH(type);

/* declare request struct and check size (at least as big) */
#define X_REQUEST_HEAD_AT_LEAST(type) \
    REQUEST(type); \
    if (stuff == NULL) return (BadLength); \
    REQUEST_AT_LEAST_SIZE(type); \

/* declare request struct, do NOT check size !*/
#define X_REQUEST_HEAD_NO_CHECK(type) \
    REQUEST(type); \
    if (stuff == NULL) return (BadLength); \

/* swap a CARD16 request struct field if necessary */
#define X_REQUEST_FIELD_CARD16(field) \
    do { if (client->swapped) swaps(&stuff->field); } while (0)

/* swap a CARD32 request struct field if necessary */
#define X_REQUEST_FIELD_CARD32(field) \
    do { if (client->swapped) swapl(&stuff->field); } while (0)

/* swap a CARD64 request struct field if necessary */
#define X_REQUEST_FIELD_CARD64(field) \
    do { if (client->swapped) swapll(&stuff->field); } while (0)

/* swap CARD16 rest of request (after the struct) */
#define X_REQUEST_REST_CARD16() \
    do { if (client->swapped) SwapRestS(stuff); } while (0)

/* swap CARD32 rest of request (after the struct) */
#define X_REQUEST_REST_CARD32() \
    do { if (client->swapped) SwapRestL(stuff); } while (0)

/* swap CARD16 rest of request (after the struct) - check fixed count */
#define X_REQUEST_REST_COUNT_CARD16(count) \
    REQUEST_FIXED_SIZE(*stuff, (count) * sizeof(CARD16)); \
    CARD16 *request_rest = (CARD16 *) (&stuff[1]); \
    do { if (client->swapped) SwapShorts((signed short*)request_rest, (count)); } while (0)

/* swap CARD32 rest of request (after the struct) - check fixed count */
#define X_REQUEST_REST_COUNT_CARD32(count) \
    REQUEST_FIXED_SIZE(*stuff, (count) * sizeof(CARD32)); \
    CARD32 *request_rest = (CARD32 *) (&stuff[1]); \
    do { if (client->swapped) SwapLongs(request_rest, (count)); } while (0) \

/*
 * macros for walking variable-length fields (strings / binary blobs) that
 * follow the fixed request struct.
 *
 * These complement the X_REQUEST_FIELD_* macros: where the latter handle the
 * fixed part, these let multiple variable-length fields be peeled off, one
 * after another, in the same stacked style - while keeping a single source of
 * truth for the length (the request's own length field) so an embedded length
 * can never make a handler read past the request.
 *
 * Usage (after X_REQUEST_HEAD_AT_LEAST() and swapping the *_len fields):
 *
 *     X_REQUEST_VAR_BEGIN();
 *     X_REQUEST_VAR_FIELD(name,  stuff->name_len);
 *     X_REQUEST_VAR_FIELD(proto, stuff->proto_len);
 *     X_REQUEST_VAR_FIELD(data,  stuff->data_len);
 *     X_REQUEST_VAR_END();
 *
 * Each field is padded to a 4-byte boundary on the wire; X_REQUEST_VAR_END()
 * requires the request to be fully consumed (strict, no trailing smuggling).
 * Any inconsistency returns BadLength, like the other request macros.
 *
 * Note: the *_len fields must already be byte-swapped (via X_REQUEST_FIELD_*)
 * before they are passed here.
 */

/* set up the cursor right after the fixed struct.
   X_REQUEST_HEAD_AT_LEAST() must have run first (sizeof(*stuff) <= req_len). */
#define X_REQUEST_VAR_BEGIN() \
    char *_xreq_ptr = (char *)(&stuff[1]); \
    size_t _xreq_left = ((size_t)client->req_len << 2) - sizeof(*stuff)

/* bind (ptr) to a byte/string field of (n) bytes, advance past its padding */
#define X_REQUEST_VAR_FIELD(ptr, n) \
    do { \
        uint64_t _xreq_n = (uint64_t)(n); \
        if (_xreq_n > _xreq_left) return (BadLength); \
        size_t _xreq_adv = pad_to_int32((int)_xreq_n); \
        if (_xreq_adv > _xreq_left) return (BadLength); \
        (ptr) = _xreq_ptr; \
        _xreq_ptr += _xreq_adv; \
        _xreq_left -= _xreq_adv; \
    } while (0)

/* bind (ptr) to an array of (count) CARD16, swap them in place if necessary */
#define X_REQUEST_VAR_FIELD_CARD16(ptr, count) \
    do { \
        uint64_t _xreq_n = (uint64_t)(count) * sizeof(CARD16); \
        if (_xreq_n > _xreq_left) return (BadLength); \
        size_t _xreq_adv = pad_to_int32((int)_xreq_n); \
        if (_xreq_adv > _xreq_left) return (BadLength); \
        (ptr) = (CARD16 *)_xreq_ptr; \
        if (client->swapped) SwapShorts((short *)(ptr), (count)); \
        _xreq_ptr += _xreq_adv; \
        _xreq_left -= _xreq_adv; \
    } while (0)

/* bind (ptr) to an array of (count) CARD32, swap them in place if necessary */
#define X_REQUEST_VAR_FIELD_CARD32(ptr, count) \
    do { \
        uint64_t _xreq_n = (uint64_t)(count) * sizeof(CARD32); \
        if (_xreq_n > _xreq_left) return (BadLength); \
        size_t _xreq_adv = pad_to_int32((int)_xreq_n); \
        if (_xreq_adv > _xreq_left) return (BadLength); \
        (ptr) = (CARD32 *)_xreq_ptr; \
        if (client->swapped) SwapLongs((CARD32 *)(ptr), (count)); \
        _xreq_ptr += _xreq_adv; \
        _xreq_left -= _xreq_adv; \
    } while (0)

/* bind (ptr) to a trailing array of fixed-size elements whose count is
   *implied by the request length*, not carried in a header field, and store
   that count in (count_out). The remaining bytes must divide evenly by
   (elemsize) (e.g. sizeof(xRectangle), sizeof(xColorItem)), else BadLength.
   Consumes the rest of the request - so this must be the last variable field.

   Element byte-swapping is the caller's job (per-field, element-type
   specific); this macro only validates the count and delimits the array. */
#define X_REQUEST_VAR_ARRAY(ptr, count_out, elemsize) \
    do { \
        if ((elemsize) == 0 || (_xreq_left % (size_t)(elemsize)) != 0) \
            return (BadLength); \
        (count_out) = _xreq_left / (size_t)(elemsize); \
        (ptr) = (void *)_xreq_ptr; \
        _xreq_ptr += _xreq_left; \
        _xreq_left = 0; \
    } while (0)

/* bind (ptr)/(len) to whatever bytes remain - for a single trailing string
   whose length is implied by the request length (no embedded length field) */
#define X_REQUEST_VAR_REST(ptr, len) \
    do { \
        (ptr) = _xreq_ptr; \
        (len) = _xreq_left; \
        _xreq_ptr += _xreq_left; \
        _xreq_left = 0; \
    } while (0)

/* require the variable part to be fully consumed */
#define X_REQUEST_VAR_END() \
    do { if (_xreq_left != 0) return (BadLength); } while (0)

/*
 * macros for request handlers
 *
 * these are handling reply struct field byte-swapping if necessary
 */

/* swap a CARD16 field (if necessary) in reply struct */
#define X_REPLY_FIELD_CARD16(field) \
    do { if (client->swapped) swaps(&reply.field); } while (0)

/* swap a CARD32 field (if necessary) in reply struct */
#define X_REPLY_FIELD_CARD32(field) \
    do { if (client->swapped) swapl(&reply.field); } while (0)

/* swap a CARD64 field (if necessary) in reply struct */
#define X_REPLY_FIELD_CARD64(field) \
    do { if (client->swapped) swapll(&reply.field); } while (0)

/*
 * do function call, check it's result and return when it's not Success
 */
#define X_CALL_CHECK_ERR(_FOO_) \
    do { int _rc = _FOO_; \
      if (_rc != Success) { return _rc; } \
    } while (0)

/*
 * do function call, check it's result and return when it's not Success
 * assign's client->errorValue on failure
 */
#define X_CALL_CHECK_ERR_VAL(_FOO_,_ERRVAL_) \
    do { int _rc = _FOO_; \
      if (_rc != Success) { \
        client->errorValue = _ERRVAL_; \
        return _rc; \
      } \
    } while (0)

#endif /* _XSERVER_DIX_REQUEST_PRIV_H */
