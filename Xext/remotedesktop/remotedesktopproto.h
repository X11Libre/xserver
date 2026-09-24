/*
 * RemoteDesktop Extension Protocol
 *
 * SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 */

#ifndef _REMOTEDESKTOPPROTO_H_
#define _REMOTEDESKTOPPROTO_H_

#include <X11/Xmd.h>
#include <X11/Xfuncproto.h>

#define REMOTEDESKTOP_EXTENSION_NAME "X-REMOTE-DESKTOP"
#define REMOTEDESKTOP_MAJOR_VERSION 1
#define REMOTEDESKTOP_MINOR_VERSION 0

#define REMOTEDESKTOP_NUMBER_EVENTS 1
#define REMOTEDESKTOP_NUMBER_ERRORS 5

/* Request codes */
#define X_RemoteDesktopQueryVersion        0
#define X_RemoteDesktopListProtocols       1
#define X_RemoteDesktopCreateSession       2
#define X_RemoteDesktopDestroySession      3
#define X_RemoteDesktopConfigureSession    4
#define X_RemoteDesktopGetSessionInfo      5
#define X_RemoteDesktopSetSessionState     6

/* Event codes */
#define RemoteDesktopNotify                0

/* Error codes */
#define BadRemoteDesktopSession            0
#define BadRemoteDesktopProtocol           1
#define BadRemoteDesktopConfig             2
#define RemoteDesktopInUse                 3
#define RemoteDesktopUnavailable           4

/* Session states */
#define RemoteDesktopSessionStopped        0
#define RemoteDesktopSessionRunning        1
#define RemoteDesktopSessionError          2

/* Notify event subtypes */
#define RemoteDesktopNotifyClientConnected     0
#define RemoteDesktopNotifyClientDisconnected  1
#define RemoteDesktopNotifyConfigChanged       2
#define RemoteDesktopNotifyError               3

/* Protocol capability flags */
#define RemoteDesktopCapAuthNone         (1 << 0)
#define RemoteDesktopCapAuthPassword     (1 << 1)
#define RemoteDesktopCapAuthTLSCert      (1 << 2)
#define RemoteDesktopCapEncryptionTLS    (1 << 3)
#define RemoteDesktopCapEncodingRaw      (1 << 4)
#define RemoteDesktopCapEncodingHextile  (1 << 5)
#define RemoteDesktopCapEncodingZRLE     (1 << 6)
#define RemoteDesktopCapEncodingTight    (1 << 7)
#define RemoteDesktopCapViewOnly         (1 << 8)
#define RemoteDesktopCapClipboard        (1 << 9)
#define RemoteDesktopCapFileTransfer     (1 << 10)
#define RemoteDesktopCapAudio            (1 << 11)
#define RemoteDesktopCapUSBRedirection   (1 << 12)

/* Config key types */
#define RemoteDesktopConfigTypeString    0
#define RemoteDesktopConfigTypeInteger   1
#define RemoteDesktopConfigTypeBoolean   2

/* Request structures */

typedef struct {
    CARD8 reqType;            /* always RemoteDesktopReqCode */
    CARD8 remoteDesktopReqType; /* X_RemoteDesktopQueryVersion */
    CARD16 length;
    CARD32 majorVersion;
    CARD32 minorVersion;
} xRemoteDesktopQueryVersionReq;

#define sz_xRemoteDesktopQueryVersionReq 12

typedef struct {
    CARD8 type;               /* X_Reply */
    CARD8 pad0;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 majorVersion;
    CARD32 minorVersion;
    CARD32 pad1;
    CARD32 pad2;
    CARD32 pad3;
    CARD32 pad4;
    CARD32 pad5;
} xRemoteDesktopQueryVersionReply;

#define sz_xRemoteDesktopQueryVersionReply 32

typedef struct {
    CARD8 reqType;
    CARD8 remoteDesktopReqType; /* X_RemoteDesktopListProtocols */
    CARD16 length;
} xRemoteDesktopListProtocolsReq;

#define sz_xRemoteDesktopListProtocolsReq 4

typedef struct {
    CARD8 type;
    CARD8 pad0;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 nProtocols;
    CARD32 pad1;
    CARD32 pad2;
    CARD32 pad3;
    CARD32 pad4;
    CARD32 pad5;
} xRemoteDesktopListProtocolsReply;

#define sz_xRemoteDesktopListProtocolsReply 32

/* Protocol info is returned as a list of strings after the reply:
 * For each protocol: name (STRING8), vendor (STRING8), version (CARD32), capabilities (CARD32)
 */

typedef struct {
    CARD8 reqType;
    CARD8 remoteDesktopReqType; /* X_RemoteDesktopCreateSession */
    CARD16 length;
    CARD32 sessionId;
    CARD32 screen;
    CARD16 protocolNameLen;
    CARD16 configDataLen;
    CARD32 pad0;
} xRemoteDesktopCreateSessionReq;

#define sz_xRemoteDesktopCreateSessionReq 20

typedef struct {
    CARD8 type;
    CARD8 pad0;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 pad1;
    CARD32 pad2;
    CARD32 pad3;
    CARD32 pad4;
    CARD32 pad5;
    CARD32 pad6;
} xRemoteDesktopCreateSessionReply;

#define sz_xRemoteDesktopCreateSessionReply 32

typedef struct {
    CARD8 reqType;
    CARD8 remoteDesktopReqType; /* X_RemoteDesktopDestroySession */
    CARD16 length;
    CARD32 sessionId;
} xRemoteDesktopDestroySessionReq;

#define sz_xRemoteDesktopDestroySessionReq 8

typedef struct {
    CARD8 type;
    CARD8 pad0;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 pad1;
    CARD32 pad2;
    CARD32 pad3;
    CARD32 pad4;
    CARD32 pad5;
    CARD32 pad6;
} xRemoteDesktopDestroySessionReply;

#define sz_xRemoteDesktopDestroySessionReply 32

typedef struct {
    CARD8 reqType;
    CARD8 remoteDesktopReqType; /* X_RemoteDesktopConfigureSession */
    CARD16 length;
    CARD32 sessionId;
    CARD16 configDataLen;
    CARD16 pad0;
} xRemoteDesktopConfigureSessionReq;

#define sz_xRemoteDesktopConfigureSessionReq 12

typedef struct {
    CARD8 type;
    CARD8 pad0;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 pad1;
    CARD32 pad2;
    CARD32 pad3;
    CARD32 pad4;
    CARD32 pad5;
    CARD32 pad6;
} xRemoteDesktopConfigureSessionReply;

#define sz_xRemoteDesktopConfigureSessionReply 32

typedef struct {
    CARD8 reqType;
    CARD8 remoteDesktopReqType; /* X_RemoteDesktopGetSessionInfo */
    CARD16 length;
    CARD32 sessionId;
} xRemoteDesktopGetSessionInfoReq;

#define sz_xRemoteDesktopGetSessionInfoReq 8

typedef struct {
    CARD8 type;
    CARD8 pad0;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 sessionId;
    CARD32 screen;
    CARD16 protocolNameLen;
    CARD16 configDataLen;
    CARD32 state;
    CARD32 connectedClients;
    CARD64 bytesSent;
    CARD64 bytesReceived;
} xRemoteDesktopGetSessionInfoReply;

#define sz_xRemoteDesktopGetSessionInfoReply 48

typedef struct {
    CARD8 reqType;
    CARD8 remoteDesktopReqType; /* X_RemoteDesktopSetSessionState */
    CARD16 length;
    CARD32 sessionId;
    CARD8 state;
    CARD8 pad0;
    CARD16 pad1;
} xRemoteDesktopSetSessionStateReq;

#define sz_xRemoteDesktopSetSessionStateReq 12

typedef struct {
    CARD8 type;
    CARD8 pad0;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 pad1;
    CARD32 pad2;
    CARD32 pad3;
    CARD32 pad4;
    CARD32 pad5;
    CARD32 pad6;
} xRemoteDesktopSetSessionStateReply;

#define sz_xRemoteDesktopSetSessionStateReply 32

/* Event structure */

typedef struct {
    CARD8 type;               /* RemoteDesktopEventBase + RemoteDesktopNotify */
    CARD8 subType;            /* RemoteDesktopNotifyClientConnected, etc. */
    CARD16 sequenceNumber;
    CARD32 sessionId;
    CARD32 detail;            /* subtype-specific detail */
    CARD32 timestamp;
    CARD32 pad0;
    CARD32 pad1;
} xRemoteDesktopNotifyEvent;

#define sz_xRemoteDesktopNotifyEvent 32

#endif /* _REMOTEDESKTOPPROTO_H_ */