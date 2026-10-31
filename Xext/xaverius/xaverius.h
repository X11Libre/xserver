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
#define XAveriusNumSubopcodes                            48   /* 0..47 */

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

#endif /* _XSERVER_XAVERIUS_H */
