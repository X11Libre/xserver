/* SPDX-License-Identifier: MIT OR X11 OR AGPLv3
 *
 * Xnamespace management extension - protocol definitions (DRAFT v1.0)
 *
 * See doc/Xnamespace-protocol.md for the specification.
 *
 * This header is the on-the-wire contract: it would normally live in
 * xorgproto (as Xnamespaceproto.h) and be shared with client bindings
 * (e.g. go-x11proto). It is kept here for now while the design is in flux.
 */
#ifndef _XSERVER_NAMESPACEPROTO_H
#define _XSERVER_NAMESPACEPROTO_H

#include <X11/Xmd.h>
#include <X11/Xproto.h>

#define XNS_EXTENSION_NAME      "X-NAMESPACE"
#define XNS_MAJOR_VERSION       1
#define XNS_MINOR_VERSION       0

/* maximum namespace name length (keeps names config-file expressible) */
#define XNS_NAME_MAX            255

/* request opcodes (minor) */
#define X_XnsQueryVersion       0
#define X_XnsListNamespaces     1
#define X_XnsCreateNamespace    2
#define X_XnsDeleteNamespace    3
#define X_XnsQueryNamespace     4
#define X_XnsSetNamespaceFlags  5
#define X_XnsAddAuthToken       6
#define X_XnsRemoveAuthToken    7
#define X_XnsListAuthTokens     8
#define X_XnsGetClientNamespace 9
#define XnsNumberRequests       10

/* capability bits (see struct Xnamespace allow* / superPower) */
#define XNS_CAPABILITY_MOUSE_MOTION     (1u << 0)
#define XNS_CAPABILITY_SHAPE            (1u << 1)
#define XNS_CAPABILITY_TRANSPARENCY     (1u << 2)
#define XNS_CAPABILITY_INPUT            (1u << 3)
#define XNS_CAPABILITY_KEYBOARD         (1u << 4)
#define XNS_CAPABILITY_ADMIN            (1u << 5)
#define XNS_CAPABILITY_ALL              0x0000003fu

/* namespace attribute bits */
#define XNS_ATTR_IMMUTABLE              (1u << 0)   /* read-only (root/anon) */
#define XNS_ATTR_TRANSIENT              (1u << 1)   /* drop when last client exits */
#define XNS_ATTR_ALL                    0x00000003u

/* values for xXnsDeleteNamespaceReq.onClients */
#define XNS_DELETE_FAIL_IF_BUSY         0   /* BadAccess if any client present */
#define XNS_DELETE_KILL_CLIENTS         1   /* terminate clients, then delete */

/* ------------------------------------------------------------------ *
 *  Requests
 *
 *  Every variable-length field carries its byte length in a *Len field
 *  of the fixed struct; the payloads follow in declared order, each
 *  padded to a 4-byte boundary. This matches the xcb/xgb codegen idiom
 *  (length field + char-list) and the X_REQUEST_VAR_* parsing macros.
 * ------------------------------------------------------------------ */

typedef struct {
    CARD8   reqType;            /* extension major opcode */
    CARD8   xnsReqType;         /* X_XnsQueryVersion */
    CARD16  length;
    CARD16  clientMajorVersion;
    CARD16  clientMinorVersion;
} xXnsQueryVersionReq;
#define sz_xXnsQueryVersionReq 8

typedef struct {
    CARD8   reqType;
    CARD8   xnsReqType;         /* X_XnsListNamespaces */
    CARD16  length;
} xXnsListNamespacesReq;
#define sz_xXnsListNamespacesReq 4

typedef struct {
    CARD8   reqType;
    CARD8   xnsReqType;         /* X_XnsCreateNamespace */
    CARD16  length;
    CARD32  capabilities;
    CARD32  attributes;         /* XNS_ATTR_TRANSIENT honored; IMMUTABLE rejected */
    CARD16  nameLen;
    CARD16  pad0;
    /* CARD8 name[nameLen], padded */
} xXnsCreateNamespaceReq;
#define sz_xXnsCreateNamespaceReq 16

typedef struct {
    CARD8   reqType;
    CARD8   xnsReqType;         /* X_XnsDeleteNamespace */
    CARD16  length;
    CARD8   onClients;          /* XNS_DELETE_* */
    CARD8   pad0;
    CARD16  nameLen;
    /* CARD8 name[nameLen], padded */
} xXnsDeleteNamespaceReq;
#define sz_xXnsDeleteNamespaceReq 8

typedef struct {
    CARD8   reqType;
    CARD8   xnsReqType;         /* X_XnsQueryNamespace */
    CARD16  length;
    CARD16  nameLen;
    CARD16  pad0;
    /* CARD8 name[nameLen], padded */
} xXnsQueryNamespaceReq;
#define sz_xXnsQueryNamespaceReq 8

typedef struct {
    CARD8   reqType;
    CARD8   xnsReqType;         /* X_XnsSetNamespaceFlags */
    CARD16  length;
    CARD32  valueMask;          /* which capability bits to apply */
    CARD32  values;             /* new values for masked bits */
    CARD16  nameLen;
    CARD16  pad0;
    /* CARD8 name[nameLen], padded */
} xXnsSetNamespaceFlagsReq;
#define sz_xXnsSetNamespaceFlagsReq 16

typedef struct {
    CARD8   reqType;
    CARD8   xnsReqType;         /* X_XnsAddAuthToken */
    CARD16  length;
    CARD16  nameLen;
    CARD16  protoLen;
    CARD16  dataLen;
    CARD16  pad0;
    /* CARD8 name[nameLen], proto[protoLen], data[dataLen]; each padded */
} xXnsAddAuthTokenReq;
#define sz_xXnsAddAuthTokenReq 12

typedef struct {
    CARD8   reqType;
    CARD8   xnsReqType;         /* X_XnsRemoveAuthToken */
    CARD16  length;
    CARD32  tokenHandle;
    CARD16  nameLen;
    CARD16  pad0;
    /* CARD8 name[nameLen], padded */
} xXnsRemoveAuthTokenReq;
#define sz_xXnsRemoveAuthTokenReq 12

typedef struct {
    CARD8   reqType;
    CARD8   xnsReqType;         /* X_XnsListAuthTokens */
    CARD16  length;
    CARD16  nameLen;
    CARD16  pad0;
    /* CARD8 name[nameLen], padded */
} xXnsListAuthTokensReq;
#define sz_xXnsListAuthTokensReq 8

typedef struct {
    CARD8   reqType;
    CARD8   xnsReqType;         /* X_XnsGetClientNamespace */
    CARD16  length;
    CARD32  clientResource;     /* 0 = the calling client */
} xXnsGetClientNamespaceReq;
#define sz_xXnsGetClientNamespaceReq 8

/* ------------------------------------------------------------------ *
 *  Replies (all 32-byte aligned header; variable tails via rpcbuf)
 * ------------------------------------------------------------------ */

typedef struct {
    BYTE    type;               /* X_Reply */
    CARD8   pad0;
    CARD16  sequenceNumber;
    CARD32  length;
    CARD16  majorVersion;
    CARD16  minorVersion;
    CARD32  pad1;
    CARD32  pad2;
    CARD32  pad3;
    CARD32  pad4;
    CARD32  pad5;
} xXnsQueryVersionReply;
#define sz_xXnsQueryVersionReply 32

/* ListNamespaces reply: header + `count` NAMESPACEINFO records (rpcbuf).
   each record: capabilities, attributes, refcnt, numTokens (CARD32 x4),
   nameLen (CARD16), pad (CARD16), name bytes (padded). */
typedef struct {
    BYTE    type;
    CARD8   pad0;
    CARD16  sequenceNumber;
    CARD32  length;             /* size of trailing records, in 4-byte units */
    CARD32  count;              /* number of namespaces */
    CARD32  pad1;
    CARD32  pad2;
    CARD32  pad3;
    CARD32  pad4;
    CARD32  pad5;
} xXnsListNamespacesReply;
#define sz_xXnsListNamespacesReply 32

typedef struct {
    BYTE    type;
    CARD8   pad0;
    CARD16  sequenceNumber;
    CARD32  length;             /* 0 - empty ack */
    CARD32  pad1;
    CARD32  pad2;
    CARD32  pad3;
    CARD32  pad4;
    CARD32  pad5;
    CARD32  pad6;
} xXnsAckReply;                 /* Create / Delete / RemoveAuthToken */
#define sz_xXnsAckReply 32

typedef struct {
    BYTE    type;
    CARD8   pad0;
    CARD16  sequenceNumber;
    CARD32  length;             /* 0 */
    CARD32  capabilities;
    CARD32  attributes;
    CARD32  refcnt;
    CARD32  numTokens;
    CARD32  pad1;
    CARD32  pad2;
} xXnsQueryNamespaceReply;
#define sz_xXnsQueryNamespaceReply 32

typedef struct {
    BYTE    type;
    CARD8   pad0;
    CARD16  sequenceNumber;
    CARD32  length;             /* 0 */
    CARD32  capabilities;       /* resulting capabilities */
    CARD32  pad1;
    CARD32  pad2;
    CARD32  pad3;
    CARD32  pad4;
    CARD32  pad5;
} xXnsSetNamespaceFlagsReply;
#define sz_xXnsSetNamespaceFlagsReply 32

typedef struct {
    BYTE    type;
    CARD8   pad0;
    CARD16  sequenceNumber;
    CARD32  length;             /* 0 */
    CARD32  tokenHandle;
    CARD32  pad1;
    CARD32  pad2;
    CARD32  pad3;
    CARD32  pad4;
    CARD32  pad5;
} xXnsAddAuthTokenReply;
#define sz_xXnsAddAuthTokenReply 32

/* ListAuthTokens reply: header + `count` records (rpcbuf).
   each record: tokenHandle (CARD32), protoLen (CARD16), pad (CARD16),
   proto bytes (padded). NO key material. */
typedef struct {
    BYTE    type;
    CARD8   pad0;
    CARD16  sequenceNumber;
    CARD32  length;
    CARD32  count;
    CARD32  pad1;
    CARD32  pad2;
    CARD32  pad3;
    CARD32  pad4;
    CARD32  pad5;
} xXnsListAuthTokensReply;
#define sz_xXnsListAuthTokensReply 32

/* GetClientNamespace reply: header + namespace name (rpcbuf) */
typedef struct {
    BYTE    type;
    CARD8   isServer;
    CARD16  sequenceNumber;
    CARD32  length;             /* size of trailing name, in 4-byte units */
    CARD16  nameLen;
    CARD16  pad0;
    CARD32  pad1;
    CARD32  pad2;
    CARD32  pad3;
    CARD32  pad4;
    CARD32  pad5;
} xXnsGetClientNamespaceReply;
#define sz_xXnsGetClientNamespaceReply 32

#endif /* _XSERVER_NAMESPACEPROTO_H */
