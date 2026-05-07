# SPDX-License-Identifier: MIT
#
# Tests for Present extension.

import pytest

from proto import present
from xclient import BadWindow, Extension, X11Error


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
        conn.send_request(req)
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
        conn.send_request(req)
        responses = conn.flush_responses(timeout=1.0)

        assert xserver.is_alive, "Server crashed"

        # With the fix: no error (void request succeeds silently).
        # Without the fix: BadIDChoice error.
        errors = [r for r in responses if isinstance(r, X11Error)]
        assert len(errors) == 0, (
            f"PresentSelectInput returned error(s): {errors} - "
            "eid not swapped → BadIDChoice"
        )


class TestPresentNotify:
    """Tests for PresentPixmap notify array byte-swap fix.

    Fix: present: Fix missing byte swaps in sproc_present_pixmap()

    The xPresentNotify array following the fixed header was not
    byte-swapped at all. Each entry has window (CARD32) and serial
    (CARD32) fields that need swapl(). Without swapping, a
    byte-swapped client's window IDs are garbled, causing
    dixLookupWindow to fail with BadWindow.

    Fixed in commit 925edb6c9e ("present: Fix missing byte swaps in
    sproc_present_pixmap()").
    """

    @pytest.mark.swapped_client
    def test_present_pixmap_notifies_window_swapped(self, xserver, xclient_swapped):
        """
        sproc_present_pixmap was missing byte swaps for the variable-length
        xPresentNotify array.

        Send a PresentPixmap request with a notify entry whose window
        field is a valid window created by this client. Without the swap,
        the window ID is garbled and dixLookupWindow fails with BadWindow.
        With the swap, the window ID is correctly interpreted.

        Fixed in commit 925edb6c9e ("present: Fix missing byte swaps in
        sproc_present_pixmap()").
        """
        conn = xclient_swapped

        ext = conn.query_extension(Extension.PRESENT)
        if not ext:
            pytest.skip("Present extension not available")

        req = present.QueryVersionRequest(opcode=ext.opcode)
        conn.send_request(req)
        conn.recv_response(timeout=5.0)

        win = conn.create_window()
        pixmap = conn.create_pixmap()

        # The notify window is the same window as the main request window.
        # With the fix, the window ID in the notify is correctly swapped
        # and the lookup succeeds. Without the fix, the garbled ID causes
        # BadWindow.
        notify = present.PresentNotify(window=win, serial=1)

        req = present.PixmapRequest(
            opcode=ext.opcode,
            window=win,
            pixmap=pixmap,
            serial=0,
            notifies=[notify],
        )
        conn.send_request(req)
        responses = conn.flush_responses(timeout=1.0)

        assert xserver.is_alive, "Server crashed"

        # With the fix: either success (no error for void request) or
        # a non-BadWindow error (e.g. BadMatch from the present
        # implementation). The key point is no BadWindow (error 3).
        # Without the fix: BadWindow because the notify's window ID
        # was not byte-swapped.
        bad_window_errors = [
            r
            for r in responses
            if isinstance(r, X11Error) and r.error_code == BadWindow
        ]
        assert len(bad_window_errors) == 0, (
            f"PresentPixmap returned BadWindow error(s): "
            f"{bad_window_errors} - notify window IDs not "
            "byte-swapped in sproc_present_pixmap"
        )
