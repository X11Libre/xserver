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
 * Passed file descriptors.
 *
 * The file descriptors a client sends alongside a request are collected into
 * client->recv_fd_list[] when the request is read (see ReadRequestFromClient),
 * in order. By default Dispatch() closes every one of them again once the
 * handler returns. Two consequences worth relying on:
 *
 *   - a handler does NOT have to close the fds it uses, and
 *   - every error path is clean for free: an early "return BadFoo" leaks
 *     nothing, because the fds are still owned by the request and Dispatch()
 *     will close them.
 *
 * X_REQUEST_FDS(name...) declares one `int` per name, bound to the request's
 * fds in order (front-most name gets the first fd), and returns BadValue if the
 * client passed fewer fds than names. Use it near the top of a handler, next to
 * the other X_REQUEST_* macros:
 *
 *     X_REQUEST_FDS(fd);            // one fd, usable as `fd` below
 *     X_REQUEST_FDS(front, back);   // two named fds, in order
 *
 * The declared fds are borrowed views into recv_fd_list[]; on return Dispatch()
 * closes them. If the handler hands an fd to something that outlives the request
 * (e.g. a SyncFence or a syncobj that takes ownership of the fd), it must call
 *
 *     X_REQUEST_FD_KEEP(fd);       // ownership transferred -> Dispatch keeps out
 *
 * so Dispatch() leaves that fd alone. A handler that merely dup()s the fd (as
 * dri3 PixmapFromBuffer does - the driver dups it) keeps nothing: Dispatch()
 * closes the original for it.
 *
 * X_REQUEST_FDS_ARRAY(fds, count) is the dynamic-count variant: it binds `count`
 * fds into the caller-provided int array (same borrow/keep rules).
 */
static inline int __x_request_fds_avail(ClientPtr client)
{
    int n = 0;
    for (int i = 0; i < MAX_CLIENT_RECV_FD; i++)
        if (client->recv_fd_list[i] >= 0)
            n++;
    return n;
}

/* Mark @fd as kept, so Dispatch() will not close it (ownership was handed off). */
static inline void __x_request_fd_keep(ClientPtr client, int fd)
{
    for (int i = 0; i < MAX_CLIENT_RECV_FD; i++)
        if (client->recv_fd_list[i] == fd) {
            client->recv_fd_list[i] = -1;
            return;
        }
}

#define X_REQUEST_FD_KEEP(fd) __x_request_fd_keep(client, (fd))

#define __X_REQ_FDS_NARG(...) __X_REQ_FDS_NARG_(__VA_ARGS__, 4, 3, 2, 1, 0)
#define __X_REQ_FDS_NARG_(_1, _2, _3, _4, N, ...) N
#define __X_REQ_FDS_CAT_(a, b) a##b
#define __X_REQ_FDS_DECL(N, ...) __X_REQ_FDS_CAT_(__X_REQ_FDS_DECL_, N)(__VA_ARGS__)
#define __X_REQ_FDS_DECL_1(a)          int a = client->recv_fd_list[0]
#define __X_REQ_FDS_DECL_2(a, b)       int a = client->recv_fd_list[0]; int b = client->recv_fd_list[1]
#define __X_REQ_FDS_DECL_3(a, b, c)    int a = client->recv_fd_list[0]; int b = client->recv_fd_list[1]; int c = client->recv_fd_list[2]
#define __X_REQ_FDS_DECL_4(a, b, c, d) int a = client->recv_fd_list[0]; int b = client->recv_fd_list[1]; int c = client->recv_fd_list[2]; int d = client->recv_fd_list[3]

#define X_REQUEST_FDS(...) \
    if (__x_request_fds_avail(client) < __X_REQ_FDS_NARG(__VA_ARGS__)) \
        return BadValue; \
    __X_REQ_FDS_DECL(__X_REQ_FDS_NARG(__VA_ARGS__), __VA_ARGS__)

#define X_REQUEST_FDS_ARRAY(fds, count) \
    do { \
        if (__x_request_fds_avail(client) < (int)(count)) \
            return BadValue; \
        for (int __fdi = 0; __fdi < (int)(count); __fdi++) \
            (fds)[__fdi] = client->recv_fd_list[__fdi]; \
    } while (0)

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
