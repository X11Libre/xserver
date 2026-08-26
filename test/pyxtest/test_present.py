# SPDX-License-Identifier: MIT
#
# Tests for Present extension.

import time

import pytest
from proto import present, sync, x11
from xclient import Extension, X11Error


@pytest.fixture
def present_xclient(xclient):
    """Provide an xclient with Present initialized."""
    ext = xclient.query_extension(Extension.PRESENT)
    if not ext:
        pytest.skip("Present extension not available")

    req = present.QueryVersionRequest(opcode=ext.opcode)
    xclient.send_request(req)
    xclient.recv_response(timeout=5.0)

    return xclient


@pytest.fixture
def present_xclient_swapped(xclient_swapped):
    """Provide a byte-swapped xclient with Present initialized."""
    ext = xclient_swapped.query_extension(Extension.PRESENT)
    if not ext:
        pytest.skip("Present extension not available")

    req = present.QueryVersionRequest(opcode=ext.opcode)
    xclient_swapped.send_request(req)
    xclient_swapped.recv_response(timeout=5.0)

    return xclient_swapped


class TestPresentSelectInput:
    @pytest.mark.swapped_client
    def test_present_select_input_eid_swapped(self, xserver, present_xclient_swapped):
        """
        sproc_present_select_input was missing swapl(&stuff->eid).
        Without the swap, the eid fails LEGAL_NEW_RESOURCE because
        the client-bits portion of the garbled XID doesn't match
        clientAsMask → BadIDChoice error.

        Fixed in commit a5ac3c871219 ("present: add missing byte
        swapping for various fields").
        """
        conn = present_xclient_swapped
        ext = conn.query_extension(Extension.PRESENT)

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
    def test_present_pixmap_notifies_window_swapped(
        self, xserver, present_xclient_swapped
    ):
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
        conn = present_xclient_swapped
        ext = conn.query_extension(Extension.PRESENT)

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
            if isinstance(r, X11Error) and r.error_code == x11.BadWindow
        ]
        assert len(bad_window_errors) == 0, (
            f"PresentPixmap returned BadWindow error(s): "
            f"{bad_window_errors} - notify window IDs not "
            "byte-swapped in sproc_present_pixmap"
        )

    @pytest.mark.asan
    def test_cross_window_notify_uaf(self, xserver, present_xclient):
        """
        ZDI-CAN-31830: Use-after-free write in Present cross-window
        notify teardown.

        A PresentPixmap request can include a notify list referencing
        windows other than the target window. Each notify entry is
        linked into both the vblank's notifies array and the
        referenced window's notify list. When the notify-target
        window (winB) is destroyed, we expect the notifies
        to be cleaned up so that when the vblank is later torn
        down (e.g. by destroying winA), we don't run into
        any dangling pointers.

        The bug requires an untriggered wait_fence to keep the vblank
        alive across the window destructions.
        """
        conn = present_xclient
        present_ext = conn.query_extension(Extension.PRESENT)
        if not present_ext:
            pytest.skip("Present extension not available")

        sync_ext = conn.query_extension(Extension.SYNC)
        if not sync_ext:
            pytest.skip("SYNC extension not available")

        # Negotiate SYNC version
        req = sync.InitializeRequest(opcode=sync_ext.opcode)
        conn.send_request(req)
        conn.recv_response(timeout=5.0)

        # Create two windows and a pixmap
        win_a = conn.create_window()
        win_b = conn.create_window()
        pixmap = conn.create_pixmap()

        # Create an untriggered fence to keep the vblank alive
        fence_id = conn.alloc_id()
        req = sync.CreateFenceRequest(
            opcode=sync_ext.opcode,
            drawable=conn.root_window,
            fence_id=fence_id,
            initially_triggered=0,
        )
        conn.send_request(req)
        conn.flush_responses(timeout=0.5)

        # PresentPixmap on winA with notify targeting winB and an
        # untriggered wait_fence. The fence prevents the vblank from
        # completing, keeping the notify entries alive.
        notify = present.PresentNotify(window=win_b, serial=1)
        req = present.PixmapRequest(
            opcode=present_ext.opcode,
            window=win_a,
            pixmap=pixmap,
            serial=0,
            wait_fence=fence_id,
            notifies=[notify],
        )
        conn.send_request(req)
        conn.flush_responses(timeout=0.5)

        # Destroy winB: present_clear_window_notifies runs, but
        # without the fix it does not unlink the notify node.
        req = x11.DestroyWindowRequest(window=win_b)
        conn.send_request(req)
        conn.flush_responses(timeout=0.5)

        # Destroy winA: triggers vblank teardown which calls
        # present_destroy_notifies -> xorg_list_del on the dangling
        # node -> UAF write.
        req = x11.DestroyWindowRequest(window=win_a)
        conn.send_request(req)
        conn.flush_responses(timeout=0.5)

        time.sleep(0.5)

        assert xserver.is_alive, (
            "Server crashed - Present cross-window notify "
            "use-after-free (ZDI-CAN-31830)"
        )
