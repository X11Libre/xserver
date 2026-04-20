# SPDX-License-Identifier: MIT
#
# Tests for Present extension.

import pytest

from proto import present
from xclient import Extension, X11Error


class TestPresentSelectInput:
    @pytest.mark.swapped_client
    def test_present_select_input_eid_swapped(self, xserver, xclient_swapped):
        """
        sproc_present_select_input was missing swapl(&stuff->eid).
        Without the swap, the eid fails LEGAL_NEW_RESOURCE because
        the client-bits portion of the garbled XID doesn't match
        clientAsMask → BadIDChoice error.

        Fixed in commit a5ac3c871219 ("present: add missing byte
        swapping for various fields").
        """
        conn = xclient_swapped

        ext = conn.query_extension(Extension.PRESENT)
        if not ext:
            pytest.skip("Present extension not available")

        req = present.QueryVersionRequest(opcode=ext.opcode)
        conn.send_request(req.to_bytes(">"))
        conn.recv_response(timeout=5.0)

        win = conn.create_window()
        eid = conn.alloc_id()

        # Use a non-zero event_mask so the server reaches
        # LEGAL_NEW_RESOURCE(eid).  With event_mask=0 the server
        # returns Success immediately without validating the eid.
        PresentConfigureNotifyMask = 1
        req = present.SelectInputRequest(
            opcode=ext.opcode,
            eid=eid,
            window=win,
            event_mask=PresentConfigureNotifyMask,
        )
        conn.send_request(req.to_bytes(">"))
        responses = conn.flush_responses(timeout=1.0)

        assert xserver.is_alive, "Server crashed"

        # With the fix: no error (void request succeeds silently).
        # Without the fix: BadIDChoice error.
        errors = [r for r in responses if isinstance(r, X11Error)]
        assert len(errors) == 0, (
            f"PresentSelectInput returned error(s): {errors} - "
            "eid not swapped → BadIDChoice"
        )
