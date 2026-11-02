/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * XAverius extension — public interface.
 *
 * Common wire framing and dispatch integration (Ship A, phase 1).
 * Request handlers for subopcodes 1-7 are owned by Ships B-E and are
 * registered through XAveriusInstallProc(); this header is the shared
 * contract between them.
 *
 * Copyright © 2026 Xavius-A
 */

#ifndef _XSERVER_XAVERIUS_H
#define _XSERVER_XAVERIUS_H

#include <X11/Xproto.h>

#define XAVERIUS_NAME "XAverius"

/* XAverius v0.1 common request header, 8 bytes total:
 *   X11 header:  reqType(1) | data(1, MUST be 0) | length(2)
 *   XAverius:    subopcode(1) | flags(1) | reserved(2, MUST be 0)
 * Payload begins at offset 8 unless a request says otherwise.
 */
typedef struct _xXAveriusReq {
    CARD8 reqType;      /* extension major opcode (dynamic, via QueryExtension) */
    CARD8 data;         /* MUST be zero in v0.1 */
    CARD16 length;      /* request length in 4-byte units */
    CARD8 subopcode;    /* centrally assigned request ID (offset 4) */
    CARD8 flags;        /* request-specific, interpreted after dispatch */
    CARD16 reserved;    /* MUST be zero in v0.1 */
} xXAveriusReq;

#define sz_xXAveriusReq 8

/* Subopcode assignments (registries/request-subopcodes.md, append-only).
 * Value 0 is reserved. 1-40 compute-baseline, 41-47 presentation-profile.
 */
#define XAveriusSubopcode_EnumeratePhysicalDevices       1
#define XAveriusSubopcode_GetPhysicalDeviceProperty      2
#define XAveriusSubopcode_GetPhysicalDeviceQueueFamilyProps 3
#define XAveriusSubopcode_CreateDevice                   4
#define XAveriusSubopcode_CreateDefaultDevice            5
#define XAveriusSubopcode_DestroyDevice                  6
#define XAveriusSubopcode_GetPhysicalDeviceMemoryProps   7
#define XAveriusSubopcode_CreateBuffer                   8
#define XAveriusSubopcode_GetBufferMemoryRequirements    9
#define XAveriusSubopcode_AllocateMemory                10
#define XAveriusSubopcode_FreeMemory                    11
#define XAveriusSubopcode_BindBufferMemory              12
#define XAveriusSubopcode_WriteBuffer                   13
#define XAveriusSubopcode_ReadBuffer                    14
#define XAveriusSubopcode_ShaderModuleData              15
#define XAveriusNumSubopcodes                           48   /* 0..47 */

/* XAverius error offsets relative to the extension first_error
 * (registries/errors.md).
 */
#define XAveriusError_DeviceUnavailable      0
#define XAveriusError_DeviceLost             1
#define XAveriusError_InvalidSyncState       2
#define XAveriusError_ExecutionFailed        3
#define XAveriusError_TransferFailed         4
#define XAveriusError_AllocationFailed       5
#define XAveriusError_DependencyFailed       6
#define XAveriusError_InternalServerError    7
#define XAveriusError_DeviceDestroyed        8
#define XAveriusNumErrors                    9

/* Handler signature for a subopcode request handler. */
typedef int (*XAveriusProc)(ClientPtr client);

/* Install a handler for a subopcode. Called by the ship owning that
 * subopcode from its own *ExtensionInit routine. Returns non-zero on
 * success (slot was free), zero if the slot was already taken.
 */
int XAveriusInstallProc(int subopcode, XAveriusProc proc);

/* Common dispatch entry point registered with AddExtension.
 * Validates the X11 envelope and the XAverius common header, then
 * dispatches to the installed subopcode handler. Unimplemented or
 * unknown subopcodes return BadRequest.
 */
int ProcXAveriusDispatch(ClientPtr client);

/* Extension initialisation: registers the XAverius extension and its
 * common dispatch. Called once at server start-up.
 */
void XAveriusExtensionInit(void);

/* ---- Phase 2 Compute Baseline request structs (subopcodes 8-15) ---- */

/* Subopcode 8: CreateBuffer (Void, 32 bytes) */
typedef struct _xXAveriusCreateBufferReq {
    CARD8 reqType;
    CARD8 data;
    CARD16 length;
    CARD8 subopcode;
    CARD8 flags;
    CARD16 reserved;
    XID  device_xid;     /* offset 8 */
    XID  buffer_xid;     /* offset 12 */
    CARD64 size;         /* offset 16 */
    CARD32 usage_flags;  /* offset 24 */
    CARD32 reserved2;    /* offset 28 */
} xXAveriusCreateBufferReq;
#define sz_xXAveriusCreateBufferReq 32

/* Subopcode 9: GetBufferMemoryRequirements (Reply, 16 bytes) */
typedef struct _xXAveriusGetBufferMemoryRequirementsReq {
    CARD8 reqType;
    CARD8 data;
    CARD16 length;
    CARD8 subopcode;
    CARD8 flags;
    CARD16 reserved;
    XID  device_xid;   /* offset 8 */
    XID  buffer_xid;   /* offset 12 */
} xXAveriusGetBufferMemoryRequirementsReq;
#define sz_xXAveriusGetBufferMemoryRequirementsReq 16

/* Subopcode 10: AllocateMemory (Void, 32 bytes) */
typedef struct _xXAveriusAllocateMemoryReq {
    CARD8 reqType;
    CARD8 data;
    CARD16 length;
    CARD8 subopcode;
    CARD8 flags;
    CARD16 reserved;
    XID  device_xid;           /* offset 8 */
    XID  memory_xid;           /* offset 12 */
    CARD64 allocation_size;    /* offset 16 */
    CARD32 memory_type_index;  /* offset 24 */
    CARD32 reserved2;          /* offset 28 */
} xXAveriusAllocateMemoryReq;
#define sz_xXAveriusAllocateMemoryReq 32

/* Subopcode 11: FreeMemory (Void, 16 bytes) */
typedef struct _xXAveriusFreeMemoryReq {
    CARD8 reqType;
    CARD8 data;
    CARD16 length;
    CARD8 subopcode;
    CARD8 flags;
    CARD16 reserved;
    XID  device_xid;  /* offset 8 */
    XID  memory_xid;  /* offset 12 */
} xXAveriusFreeMemoryReq;
#define sz_xXAveriusFreeMemoryReq 16

/* Subopcode 12: BindBufferMemory (Void, 28 bytes) */
typedef struct _xXAveriusBindBufferMemoryReq {
    CARD8 reqType;
    CARD8 data;
    CARD16 length;
    CARD8 subopcode;
    CARD8 flags;
    CARD16 reserved;
    XID  device_xid;     /* offset 8 */
    XID  buffer_xid;     /* offset 12 */
    XID  memory_xid;     /* offset 16 */
    CARD64 memory_offset;/* offset 20 */
} xXAveriusBindBufferMemoryReq;
#define sz_xXAveriusBindBufferMemoryReq 28

/* Subopcode 13: WriteBuffer (Reply, variable) */
typedef struct _xXAveriusWriteBufferReq {
    CARD8 reqType;
    CARD8 data;
    CARD16 length;
    CARD8 subopcode;
    CARD8 flags;
    CARD16 reserved;
    XID  device_xid;    /* offset 8 */
    XID  buffer_xid;    /* offset 12 */
    CARD64 buffer_offset;/* offset 16 */
    CARD32 data_length; /* offset 24 */
    /* byte-payload follows at offset 28 */
} xXAveriusWriteBufferReq;
#define sz_xXAveriusWriteBufferReq 28

/* Subopcode 14: ReadBuffer (Reply, 28 bytes) */
typedef struct _xXAveriusReadBufferReq {
    CARD8 reqType;
    CARD8 data;
    CARD16 length;
    CARD8 subopcode;
    CARD8 flags;
    CARD16 reserved;
    XID  device_xid;    /* offset 8 */
    XID  buffer_xid;    /* offset 12 */
    CARD64 buffer_offset;/* offset 16 */
    CARD32 data_length; /* offset 24 */
} xXAveriusReadBufferReq;
#define sz_xXAveriusReadBufferReq 28

/* Subopcode 15: ShaderModuleData (Void, variable) */
typedef struct _xXAveriusShaderModuleDataReq {
    CARD8 reqType;
    CARD8 data;
    CARD8 subopcode;
    CARD8 flags;        /* CREATE(0) FINAL(1) ABORT(2), bits 3-7 reserved */
    CARD16 reserved;
    XID  device_xid;        /* offset 8 */
    XID  shader_module_xid; /* offset 12 */
    CARD32 data_length;     /* offset 16 */
    /* SPIR-V bytes follow at offset 20 */
} xXAveriusShaderModuleDataReq;
#define sz_xXAveriusShaderModuleDataReq 20

/* ShaderModuleData common-header flags bits */
#define XAVERIUS_SHADER_FLAG_CREATE  (1 << 0)
#define XAVERIUS_SHADER_FLAG_FINAL   (1 << 1)
#define XAVERIUS_SHADER_FLAG_ABORT   (1 << 2)

#endif /* _XSERVER_XAVERIUS_H */
