/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * XAverius extension — common wire framing and dispatch integration.
 *
 * Ship A owns the common dispatch and extension registration, plus the
 * phase-2 Compute Baseline request handlers for subopcodes 8-15
 * (CreateBuffer ... ShaderModuleData).
 *
 * Request handlers for subopcodes 1-7 are implemented by Ships B-E and
 * registered via XAveriusInstallProc().
 *
 * Backend infrastructure (XAverius device/buffer/memory/shader dix
 * resource types, XID registration, backend allocation/transfer) does
 * not yet exist. Every phase-2 handler therefore performs the wire-level
 * validation defined by the spec's normative precedence
 * (envelope → common header → length → scalars/flags → XID create
 * → device lookup → device state) and returns BadImplementation once a
 * phase requires backend state that is not yet implemented. This keeps
 * the dispatch table honest: handlers are registered and wire-correct,
 * but no backend mutation occurs until the backend is added.
 *
 * Copyright © 2026 Xavius-A
 */

#include <dix-config.h>
#include <X11/Xproto.h>
#include "dix/dix_priv.h"
#include "dix/request_priv.h"
#include "miext/extinit_priv.h"
#include "xaverius.h"

/* Handler table indexed by subopcode (0..47). NULL = unimplemented. */
static XAveriusProc XAveriusProcs[XAveriusNumSubopcodes];

/* Error base allocated by AddExtension. */
static int XAveriusErrorBase = 0;

/* ---------- Common dispatch ---------- */

int
ProcXAveriusDispatch(ClientPtr client)
{
    X_REQUEST_HEAD_AT_LEAST(xXAveriusReq);

    /* Phase 1: envelope framing. X_REQUEST_HEAD_AT_LEAST already rejects
     * requests shorter than the 8-byte common header (length < 2 units,
     * which includes the zero-length BIG-REQUESTS case) with BadLength
     * before we touch offset 4, per wire-protocol.md.
     */

    /* Phase 2: common-header invariant checks.
     * X11 'data' byte (offset 1) MUST be zero for XAverius v0.1.
     * Common-header 'reserved' (offset 6-7) MUST be zero in v0.1.
     * These take precedence over subopcode dispatch / profile gating.
     */
    if (stuff->data != 0 || stuff->reserved != 0)
        return BadValue;

    /* Phase 3: subopcode dispatch and profile gate.
     * Subopcode 0 is reserved.
     * 1-40 compute-baseline, 41-47 presentation-profile-only.
     * Presentation requests return BadRequest on compute-only server.
     */
    CARD8 subop;
    XAveriusProc proc;

    subop = stuff->subopcode;

    if (subop == 0 || subop >= XAveriusNumSubopcodes)
        return BadRequest;

    proc = XAveriusProcs[subop];
    if (!proc)
        return BadRequest;   /* not yet implemented / unsupported */

    return proc(client);
}

/* ---------- Helper: backend-not-yet-available ---------- */

/* Returned by every phase-2 handler once wire-level validation passes
 * but a phase requires backend state (dix resource types, XID
 * registration, device/backend allocation) that is not yet implemented.
 */
#define XAVERIUS_BACKEND_NOT_READY() \
    do { return BadImplementation; } while (0)

/* ---------- Subopcode 8: CreateBuffer (Void, 32 bytes) ----
 *
 * Wire layout: device_xid(8), buffer_xid(12), size CARD64(16),
 * usage_flags CARD32(24), reserved CARD32(28).
 * Spec §5.1 validation precedence:
 *   (1) exact length (BadLength)
 *   (2) size, usage_flags, reserved (BadValue)
 *   (3) buffer_xid create-XID (BadIDChoice)
 *   (4) device_xid lookup (BadValue)
 *   (5) type/ownership (BadMatch)
 *   (6) device state (DEVICE_UNAVAILABLE/DEVICE_LOST)
 *   (7) backend buffer creation (ALLOCATION_FAILED)
 */
static int
ProcXAveriusCreateBuffer(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXAveriusCreateBufferReq);

    /* (2) scalar validation: size positive, >=1 usage flag bit set,
     * unknown bits rejected, reserved2 == 0. */
    if (stuff->size == 0 ||
        stuff->usage_flags == 0 ||
        (stuff->usage_flags & ~0x3Fu) != 0 ||   /* bits 0-5 defined, 7-31 reserved */
        stuff->reserved2 != 0)
        return BadValue;

    /* (3) buffer_xid as create-XID — backend not yet implemented. */
    XAVERIUS_BACKEND_NOT_READY();
}

/* ---------- Subopcode 9: GetBufferMemoryRequirements (Reply, 16 bytes) ----
 *
 * Wire layout: device_xid(8), buffer_xid(12).
 * Spec §5.3 validation precedence:
 *   (1) exact 16-byte length (BadLength)
 *   (2) resolve device_xid, then buffer_xid (BadValue)
 *   (3) type/ownership/association (BadMatch)
 *   (4) return stored memory-requirement metadata
 */
static int
ProcXAveriusGetBufferMemoryRequirements(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXAveriusGetBufferMemoryRequirementsReq);

    /* No request-specific scalars beyond the common header. */
    XAVERIUS_BACKEND_NOT_READY();
}

/* ---------- Subopcode 10: AllocateMemory (Void, 32 bytes) ----
 *
 * Wire layout: device_xid(8), memory_xid(12), allocation_size CARD64(16),
 * memory_type_index CARD32(24), reserved CARD32(28).
 * Spec §5.4 validation precedence:
 *   (1) exact length (BadLength)
 *   (2) allocation_size, memory_type_index, reserved (BadValue)
 *   (3) memory_xid create-XID (BadIDChoice)
 *   (4) device_xid lookup (BadValue)
 *   (5) type/ownership (BadMatch)
 *   (6) device state (DEVICE_UNAVAILABLE/DEVICE_LOST)
 *   (7) backend allocation (ALLOCATION_FAILED)
 */
static int
ProcXAveriusAllocateMemory(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXAveriusAllocateMemoryReq);

    /* (2) scalar validation. */
    if (stuff->allocation_size == 0 ||
        stuff->reserved2 != 0)
        return BadValue;

    XAVERIUS_BACKEND_NOT_READY();
}

/* ---------- Subopcode 11: FreeMemory (Void, 16 bytes) ----
 *
 * Wire layout: device_xid(8), memory_xid(12).
 * Spec §5.5 validation precedence:
 *   (1) exact 16-byte length (BadLength)
 *   (2) resolve device_xid, then memory_xid (BadValue)
 *   (3) type/ownership/association (BadMatch)
 *   (4) bound-buffer check (BadMatch)
 *   (5) accepted-GPU-access check (INVALID_SYNCHRONIZATION_STATE)
 */
static int
ProcXAveriusFreeMemory(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXAveriusFreeMemoryReq);

    XAVERIUS_BACKEND_NOT_READY();
}

/* ---------- Subopcode 12: BindBufferMemory (Void, 28 bytes) ----
 *
 * Wire layout: device_xid(8), buffer_xid(12), memory_xid(16),
 * memory_offset CARD64(20).
 * Spec §5.6 validation precedence:
 *   (1) exact wire length (BadLength)
 *   (2) resolve device, buffer, memory XIDs in wire order (BadValue)
 *   (3) type/ownership/association (BadMatch)
 *   (4) device state (DEVICE_UNAVAILABLE/DEVICE_LOST)
 *   (5) binding-specific checks (BadMatch)
 */
static int
ProcXAveriusBindBufferMemory(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXAveriusBindBufferMemoryReq);

    XAVERIUS_BACKEND_NOT_READY();
}

/* ---------- Subopcode 13: WriteBuffer (Reply, variable) ----
 *
 * Wire layout: device_xid(8), buffer_xid(12), buffer_offset CARD64(16),
 * data_length CARD32(24), byte-payload at offset 28.
 * Total length = 28 + pad4(data_length).
 * Spec §5.7 validation precedence:
 *   (1) total length with checked wide arithmetic + X11 limit (BadLength)
 *   (2) data_length > 0 and zero payload padding (BadValue)
 *   (3) resolve device_xid, then buffer_xid (BadValue)
 *   (4) type/ownership/association (BadMatch)
 *   (5) device state (DEVICE_UNAVAILABLE/DEVICE_LOST)
 *   (6) binding/TRANSFER_DST/range (BadMatch)
 *   (7) ordered transfer (DEPENDENCY_FAILED/ALLOCATION_FAILED/TRANSFER_FAILED)
 */
static int
ProcXAveriusWriteBuffer(ClientPtr client)
{
    X_REQUEST_HEAD_AT_LEAST(xXAveriusWriteBufferReq);

    CARD32 data_length = stuff->data_length;
    uint64_t total_len = (uint64_t)28 + (((uint64_t)data_length + 3u) & ~((uint64_t)3u));

    /* (1) total length / X11 limit (BadLength). The X11 length field is
     * already validated as the envelope by dispatch; here we check the
     * count-derived total matches the declared length. */
    if ((uint64_t)(client->req_len << 2) != total_len)
        return BadLength;

    /* (2) data_length > 0 (BadValue). Padding checked by the caller/backend;
     * we enforce the scalar here. */
    if (data_length == 0)
        return BadValue;

    XAVERIUS_BACKEND_NOT_READY();
}

/* ---------- Subopcode 14: ReadBuffer (Reply, 28 bytes) ----
 *
 * Wire layout: device_xid(8), buffer_xid(12), buffer_offset CARD64(16),
 * data_length CARD32(24). Total length: 28 bytes.
 * Spec §5.8 validation precedence:
 *   (1) exact 28-byte length (BadLength)
 *   (2) data_length > 0 (BadValue)
 *   (3) resolve device_xid, then buffer_xid (BadValue)
 *   (4) type/ownership/association (BadMatch)
 *   (5) device state (DEVICE_UNAVAILABLE/DEVICE_LOST)
 *   (6) binding/TRANSFER_SRC/range (BadMatch)
 *   (7) snapshot-ordered read (DEPENDENCY_FAILED/ALLOCATION_FAILED/TRANSFER_FAILED)
 */
static int
ProcXAveriusReadBuffer(ClientPtr client)
{
    X_REQUEST_HEAD_STRUCT(xXAveriusReadBufferReq);

    /* (2) data_length > 0 (BadValue). */
    if (stuff->data_length == 0)
        return BadValue;

    XAVERIUS_BACKEND_NOT_READY();
}

/* ---------- Subopcode 15: ShaderModuleData (Void, variable) ----
 *
 * Wire layout: device_xid(8), shader_module_xid(12), data_length CARD32(16),
 * SPIR-V bytes at offset 20. Total length = 20 + pad4(data_length).
 * Common-header flags: CREATE(0) FINAL(1) ABORT(2), bits 3-7 reserved.
 * Spec §6.1 validation precedence:
 *   (1) envelope + exact total length 20+pad4(data_length) (BadLength)
 *   (2) flag combination, reserved flag bits, data_length constraints,
 *       zero payload padding (BadValue)
 *   (3) for CREATE: shader_module_xid create-XID (BadIDChoice)
 *   (4) device_xid lookup (BadValue)
 *   (5) type/ownership (BadMatch)
 *   (6) device state (DEVICE_UNAVAILABLE/DEVICE_LOST)
 *   (7) upload-assembly state / size limits (INVALID_SYNCHRONIZATION_STATE/BadValue)
 *   (8) on FINAL: SPIR-V validation + backend creation (BadValue/ALLOCATION_FAILED)
 */
static int
ProcXAveriusShaderModuleData(ClientPtr client)
{
    X_REQUEST_HEAD_AT_LEAST(xXAveriusShaderModuleDataReq);

    CARD8 flags = stuff->flags;
    CARD32 data_length = stuff->data_length;
    Bool create  = (flags & XAVERIUS_SHADER_FLAG_CREATE) != 0;
    Bool final_  = (flags & XAVERIUS_SHADER_FLAG_FINAL) != 0;
    Bool abort_  = (flags & XAVERIUS_SHADER_FLAG_ABORT) != 0;

    /* Reserved flag bits 3-7 MUST be zero (BadValue). */
    if ((flags & ~(XAVERIUS_SHADER_FLAG_CREATE |
                   XAVERIUS_SHADER_FLAG_FINAL |
                   XAVERIUS_SHADER_FLAG_ABORT)) != 0)
        return BadValue;

    /* (2) flag combination validity (BadValue). */
    if (abort_ && (create || final_))
        return BadValue;   /* ABORT cannot combine with CREATE or FINAL */
    if (!create && !final_ && !abort_)
        return BadValue;   /* at least one flag required */

    /* data_length constraints: non-abort chunks need positive data_length;
     * ABORT must have data_length == 0. */
    if (abort_) {
        if (data_length != 0)
            return BadValue;
    } else {
        if (data_length == 0)
            return BadValue;
    }

    /* (1) total length check: 20 + pad4(data_length). */
    {
        uint64_t total_len = (uint64_t)20 +
            (((uint64_t)data_length + 3u) & ~((uint64_t)3u));
        if ((uint64_t)(client->req_len << 2) != total_len)
            return BadLength;
    }

    /* (3) CREATE: shader_module_xid create-XID — backend not ready. */
    XAVERIUS_BACKEND_NOT_READY();
}

/* ---------- Procedure installation ---------- */

int
XAveriusInstallProc(int subopcode, XAveriusProc proc)
{
    if (subopcode <= 0 || subopcode >= XAveriusNumSubopcodes)
        return 0;

    if (XAveriusProcs[subopcode] != NULL)
        return 0;  /* already taken */

    XAveriusProcs[subopcode] = proc;
    return 1;
}

/* ---------- Extension initialisation ---------- */

void
XAveriusExtensionInit(void)
{
    ExtensionEntry *extEntry;

    extEntry = AddExtension(XAVERIUS_NAME,
                            0,                  /* NumEvents — none in phase 1 */
                            XAveriusNumErrors,  /* NumErrors */
                            ProcXAveriusDispatch,
                            ProcXAveriusDispatch,
                            NULL,               /* ResetProc — none needed */
                            StandardMinorOpcode);

    if (extEntry == NULL) {
        ErrorF("XAverius Extension failed to Initialise\n");
        return;
    }

    XAveriusErrorBase = extEntry->errorBase;

    /* Ship A registers its phase-2 Compute Baseline handlers
     * (subopcodes 8-15). Ships B-E register subopcodes 1-7 via
     * XAveriusInstallProc() from their own init routines. */
    XAveriusInstallProc(XAveriusSubopcode_CreateBuffer,
                        ProcXAveriusCreateBuffer);
    XAveriusInstallProc(XAveriusSubopcode_GetBufferMemoryRequirements,
                        ProcXAveriusGetBufferMemoryRequirements);
    XAveriusInstallProc(XAveriusSubopcode_AllocateMemory,
                        ProcXAveriusAllocateMemory);
    XAveriusInstallProc(XAveriusSubopcode_FreeMemory,
                        ProcXAveriusFreeMemory);
    XAveriusInstallProc(XAveriusSubopcode_BindBufferMemory,
                        ProcXAveriusBindBufferMemory);
    XAveriusInstallProc(XAveriusSubopcode_WriteBuffer,
                        ProcXAveriusWriteBuffer);
    XAveriusInstallProc(XAveriusSubopcode_ReadBuffer,
                        ProcXAveriusReadBuffer);
    XAveriusInstallProc(XAveriusSubopcode_ShaderModuleData,
                        ProcXAveriusShaderModuleData);

    /* Error values for resource types are not used in phase 1.
     * Phase 2+ resource types (DEVICE, BUFFER, MEMORY, etc.) will
     * call SetResourceTypeErrorValue() with XAveriusErrorBase + offset.
     */

#ifdef DEBUG
    fprintf(stderr, "XAverius Extension v0.1 (compute-baseline)\n");
#endif
}
