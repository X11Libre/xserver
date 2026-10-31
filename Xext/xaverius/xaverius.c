/* SPDX-License-Identifier: X11 OR MIT OR AGPL-3.0-or-later
 *
 * XAverius extension — common wire framing and dispatch integration.
 *
 * Ship A (phase 1) owns the common dispatch and extension registration.
 * Request handlers for subopcodes 1-7 are implemented by Ships B-E and
 * registered via XAveriusInstallProc().
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
     * For phase 1, all compute-baseline handlers are unimplemented here
     * and will be registered by Ships B-E via XAveriusInstallProc().
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

    /* Error values for resource types are not used in phase 1.
     * Phase 2+ resource types (DEVICE, BUFFER, MEMORY, etc.) will
     * call SetResourceTypeErrorValue() with XAveriusErrorBase + offset.
     */

#ifdef DEBUG
    fprintf(stderr, "XAverius Extension v0.1 (compute-baseline)\n");
#endif
}