/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 */
#ifndef _XSERVER_DIX_REQUEST_PRIV_H
#define _XSERVER_DIX_REQUEST_PRIV_H

#include "dix.h"
#include "dixstruct.h"

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

/* swap a request struct field if necessary */
#define REQUEST_FIELD_CARD16(field) \
    do { if (client->swapped) swaps(&stuff->field); } while (0)

/* swap a request struct field if necessary */
#define REQUEST_FIELD_CARD32(field) \
    do { if (client->swapped) swapl(&stuff->field); } while (0)

#define REQUEST_REST_CARD32() \
    do { if (client->swapped) SwapRestL(stuff); } while (0)

#define REQUEST_REST_CARD16() \
    do { if (client->swapped) SwapRestS(stuff); } while (0)

/* swap a request struct field if necessary */
#define REQUEST_FIELD_CARD32(field) \
    do { if (client->swapped) swapl(&stuff->field); } while (0)

#define REQUEST_REST_CARD32() \
    do { if (client->swapped) SwapRestL(stuff); } while (0)

#define REQUEST_REST_CARD16() \
    do { if (client->swapped) SwapRestS(stuff); } while (0)

/* swap a CARD16 field (if necessary) in reply struct */
#define REPLY_FIELD_CARD16(field) \
    do { if (client->swapped) swaps(&rep.field); } while (0)

/* swap a CARD32 field (if necessary) in reply struct */
#define REPLY_FIELD_CARD32(field) \
    do { if (client->swapped) swapl(&rep.field); } while (0)

/* swap a buffer of CARD16's */
#define REPLY_BUF_CARD16(buf, count) \
    do { if (client->swapped) SwapShorts((short*) buf, count); } while (0)

/* swap a buffer of CARD16's */
#define REPLY_BUF_CARD32(buf, count) \
    do { if (client->swapped) SwapLongs((CARD32*) buf, count); } while (0)

static inline int ClientReplyPrepare(ClientPtr client, xGenericReply *reply) {
    reply->type = X_Reply;
    reply->sequenceNumber = client->sequence;
    if (client->swapped) {
        swaps(&(reply->sequenceNumber));
        swapl(&(reply->length));
    }
    return Success;
}

static inline int ClientReplySend(ClientPtr client, xGenericReply *reply, size_t len) {
    ClientReplyPrepare(client, reply);
    WriteToClient(client, len, reply);
    return Success;
}

#define REPLY_SEND() (ClientReplySend(client, (xGenericReply*)&rep, sizeof(rep)))

/* send a reply (fix up some fields) and return Success */
#define REPLY_SEND_RET_SUCCESS() \
    do { return REPLY_SEND(); } while (0)

/* send a reply with extra data - also settings the length field */
#define REPLY_SEND_EXTRA(data, len) \
    do { \
        rep.length = bytes_to_int32(len); \
        REPLY_SEND(); \
        WriteToClient(client, len, data); \
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
