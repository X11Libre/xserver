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

#endif /* _XSERVER_DIX_REQUEST_PRIV_H */
