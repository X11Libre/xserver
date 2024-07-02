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
#include "include/os.h"      /* WriteToClient */

/*
 * @brief write rpc buffer to client and then clear it
 *
 * @param pClient the client to write buffer to
 * @param rpcbuf  the buffer whose contents will be written
 * @return the result of WriteToClient() call
 */
static inline ssize_t WriteRpcbufToClient(ClientPtr pClient,
                                          x_rpcbuf_t *rpcbuf) {
    /* explicitly casting between (s)size_t and int - should be safe,
       since payloads are always small enough to easily fit into int. */
    ssize_t ret = WriteToClient(pClient,
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

    WriteToClient(pClient, (int)hdrLen, hdrData);
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

    WriteToClient(pClient, (int)hdrLen, hdrData);
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
    __write_reply_hdr_and_rpcbuf(client, &(hdrstruct), sizeof(hdrstruct), &(rpcbuf));

/*
 * send reply with header struct (not pointer!) without any payload
 *
 * @param client      pointer to the client (ClientPtr)
 * @param hdrstruct   the header struct (not pointer, the struct itself!)
 * @return            X11 result code (=Success)
 */
#define X_SEND_REPLY_SIMPLE(client, hdrstruct) \
    __write_reply_hdr_simple(client, &(hdrstruct), sizeof(hdrstruct));

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
    REQUEST_FIXED_SIZE(*stuff, count * sizeof(CARD16)); \
    CARD16 *request_rest = (CARD16 *) (&stuff[1]); \
    do { if (client->swapped) SwapShorts((signed short*)request_rest, count); } while (0)

/* swap CARD32 rest of request (after the struct) - check fixed count */
#define X_REQUEST_REST_COUNT_CARD32(count) \
    REQUEST_FIXED_SIZE(*stuff, count * sizeof(CARD32)); \
    CARD32 *request_rest = (CARD32 *) (&stuff[1]); \
    do { if (client->swapped) SwapLongs(request_rest, count); } while (0) \

#define CLIENT_STRUCT_CARD16_1(buf, a) \
    do { if (client->swapped) { \
        cpswaps((buf)->a, (buf)->a); \
    } } while (0)

#define CLIENT_STRUCT_CARD16_2(buf, a, b) \
    do { if (client->swapped) { \
        cpswaps((buf)->a, (buf)->a); \
        cpswaps((buf)->b, (buf)->b); \
    } } while (0)

#define CLIENT_STRUCT_CARD16_3(buf, a, b, c) \
    do { if (client->swapped) { \
        cpswaps((buf)->a, (buf)->a); \
        cpswaps((buf)->b, (buf)->b); \
        cpswaps((buf)->c, (buf)->c); \
    } } while (0)

#define CLIENT_STRUCT_CARD16_4(buf, a, b, c, d) \
    do { if (client->swapped) { \
        cpswaps((buf)->a, (buf)->a); \
        cpswaps((buf)->b, (buf)->b); \
        cpswaps((buf)->c, (buf)->c); \
        cpswaps((buf)->d, (buf)->d); \
    } } while (0)

#define CLIENT_STRUCT_CARD16_5(buf, a, b, c, d, e) \
    do { if (client->swapped) { \
        cpswaps((buf)->a, (buf)->a); \
        cpswaps((buf)->b, (buf)->b); \
        cpswaps((buf)->c, (buf)->c); \
        cpswaps((buf)->d, (buf)->d); \
        cpswaps((buf)->e, (buf)->e); \
    } } while (0)

#define CLIENT_STRUCT_CARD32_1(buf, a) \
    do { if (client->swapped) { \
        cpswapl((buf)->a, (buf)->a); \
    } } while (0)

#define CLIENT_STRUCT_CARD32_2(buf, a, b) \
    do { if (client->swapped) { \
        cpswapl((buf)->a, (buf)->a); \
        cpswapl((buf)->b, (buf)->b); \
    } } while (0)

#define CLIENT_STRUCT_CARD32_3(buf, a, b, c) \
    do { if (client->swapped) { \
        cpswapl((buf)->a, (buf)->a); \
        cpswapl((buf)->b, (buf)->b); \
        cpswapl((buf)->c, (buf)->c); \
    } } while (0)

#define CLIENT_STRUCT_CARD32_4(buf, a, b, c, d) \
    do { if (client->swapped) { \
        cpswapl((buf)->a, (buf)->a); \
        cpswapl((buf)->b, (buf)->b); \
        cpswapl((buf)->c, (buf)->c); \
        cpswapl((buf)->d, (buf)->d); \
    } } while (0)

#define CLIENT_STRUCT_CARD32_5(buf, a, b, c, d, e) \
    do { if (client->swapped) { \
        cpswapl((buf)->a, (buf)->a); \
        cpswapl((buf)->b, (buf)->b); \
        cpswapl((buf)->c, (buf)->c); \
        cpswapl((buf)->d, (buf)->d); \
        cpswapl((buf)->e, (buf)->e); \
    } } while (0)

/*
 * macros for request handlers
 *
 * these are handling request packet checking and swapping of multi-byte
 * values, if necessary.
 */

/* declare request struct and check size. length already must have been swapped */
#define REQUEST_HEAD_STRUCT(type) \
    REQUEST(type); \
    if (stuff == NULL) return (BadLength); \
    REQUEST_FIELD_CARD16(length); \
    REQUEST_SIZE_MATCH(type);

/* declare request struct and check size. length already must have been swapped */
#define REQUEST_HEAD_STRUCT_32L(type) \
    REQUEST(type); \
    if (stuff == NULL) return (BadLength); \
    REQUEST_FIELD_CARD32(length); \
    REQUEST_SIZE_MATCH(type);

/* declare request struct and check size (at least as big). length already must have been swapped */
#define REQUEST_HEAD_AT_LEAST(type) \
    REQUEST(type); \
    if (stuff == NULL) return (BadLength); \
    REQUEST_AT_LEAST_SIZE(type); \
    REQUEST_FIELD_CARD16(length);

/* declare request struct, do NOT check size !*/
#define REQUEST_HEAD_NO_CHECK(type) \
    REQUEST(type); \
    if (stuff == NULL) return (BadLength); \
    REQUEST_FIELD_CARD16(length);

/* swap a CARD16 request struct field if necessary */
#define REQUEST_FIELD_CARD16(field) \
    do { if (client->swapped) swaps(&stuff->field); } while (0)

/* swap a CARD32 request struct field if necessary */
#define REQUEST_FIELD_CARD32(field) \
    do { if (client->swapped) swapl(&stuff->field); } while (0)

/* swap a CARD64 request struct field if necessary */
#define REQUEST_FIELD_CARD64(field) \
    do { if (client->swapped) swapll(&stuff->field); } while (0)

/* swap CARD16 rest of request (after the struct) */
#define REQUEST_REST_CARD16() \
    do { if (client->swapped) SwapRestS(stuff); } while (0)

/* swap CARD32 rest of request (after the struct) */
#define REQUEST_REST_CARD32() \
    do { if (client->swapped) SwapRestL(stuff); } while (0)

/* swap a buffer of CARD16's */
#define REQUEST_BUF_CARD16(buf, count) \
    do { if (client->swapped) SwapShorts((short*) buf, count); } while (0)

/* swap a buffer of CARD32's */
#define REQUEST_BUF_CARD32(buf, count) \
    do { if (client->swapped) SwapLongs((CARD32*) buf, count); } while (0)

/* swap a CARD16 field (if necessary) in reply struct */
#define REPLY_FIELD_CARD16(field) \
    do { if (client->swapped) swaps(&rep.field); } while (0)

/* swap a CARD32 field (if necessary) in reply struct */
#define REPLY_FIELD_CARD32(field) \
    do { if (client->swapped) swapl(&rep.field); } while (0)

/* swap a CARD64 field (if necessary) in reply struct */
#define REPLY_FIELD_CARD64(field) \
    do { if (client->swapped) swapll(&rep.field); } while (0)

/* swap a buffer of CARD16's */
#define REPLY_BUF_CARD16(buf, count) \
    do { if (client->swapped) SwapShorts((short*) buf, count); } while (0)

/* swap a buffer of CARD32's */
#define REPLY_BUF_CARD32(buf, count) \
    do { if (client->swapped) SwapLongs((CARD32*) buf, count); } while (0)

/* swap a buffer of CARD64's */
#define REPLY_BUF_CARD64(buf, count) \
    do { if (client->swapped) { \
        CARD64* ____b = (CARD64*)buf; \
        for (int ____i=0; ____i<count; ____i++) { \
            swapll(&____b[____i]); } } } while (0)

static inline int ClientReplySend(ClientPtr client, xGenericReply *reply, size_t len) {
    reply->type = X_Reply;
    reply->sequenceNumber = client->sequence;
    if (client->swapped) {
        swaps(&(reply->sequenceNumber));
        swapl(&(reply->length));
    }
    WriteToClient(client, len, reply);
    return Success;
}

// internal - don't use it anywhere outside this header
static inline int __abovezero(int x) {
    return (x > 0 ? x : 0);
}

/* intentionally having this inline, so the compiler has good chance
   for optimizations */
static inline int ClientReplySendX(ClientPtr client,
                                  xGenericReply *reply,
                                  size_t len,
                                  const void *data1,
                                  size_t data1_len,
                                  const void *data2,
                                  size_t data2_len
) {
    data1_len = __abovezero(data1_len);
    data2_len = __abovezero(data1_len);
    reply->length = bytes_to_int32(
        __abovezero(len + data1_len + data2_len - sizeof(xGenericReply)));
    reply->type = X_Reply;
    reply->sequenceNumber = client->sequence;
    if (client->swapped) {
        swaps(&(reply->sequenceNumber));
        swapl(&(reply->length));
    }
    WriteToClient(client, len, reply);
    if (data1_len && data1)
        WriteToClient(client, data1_len, data1);
    if (data2_len && data2)
        WriteToClient(client, data2_len, data2);
    return Success;
}

#define REPLY_SEND() (ClientReplySend(client, (xGenericReply*)&rep, sizeof(rep)))

/* send a reply with extra data - also settings the length field */
#define REPLY_SEND_EXTRA(data1, len1) \
    do { return ClientReplySendX(client, (xGenericReply*)&rep, \
                                 sizeof(rep), data1, len1, NULL, 0); \
    } while (0)

/* send a reply with extra data - also settings the length field */
#define REPLY_SEND_EXTRA_2(data1, len1, data2, len2) \
    do { return ClientReplySendX(client, (xGenericReply*)&rep, \
                                 sizeof(rep), data1, len1, data2, len2); \
    } while (0)

/* Declare a SProc*Dispatch function, which swaps the length field
   (in order to make size check macros work) and then calls the real
   dispatcher. The swapping of payload fields must be done in the
   real dispatcher (if necessary) - use REQUEST_FIELD_*() macros there.
*/
#define DECLARE_SWAPPED_DISPATCH(_name,_dispatch) \
    static int _X_COLD _name(ClientPtr client) { \
        REQUEST(xReq); \
        return _dispatch(client); \
    }

#define DECLARE_SWAPPED_DISPATCH_EXTERN(_name,_dispatch) \
    int _X_COLD _name(ClientPtr client) { \
        REQUEST(xReq); \
        return _dispatch(client); \
    }

#endif /* _XSERVER_DIX_REQUEST_PRIV_H */
