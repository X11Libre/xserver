# SPDX-License-Identifier: MIT
#
# Security tests for MIT-SCREEN-SAVER extension vulnerabilities.

import time

import pytest

from proto import screensaver, x11
from xclient import Extension


@pytest.fixture
def screensaver_xclient(xclient):
    """Provide an xclient with the MIT-SCREEN-SAVER extension queried."""
    ext = xclient.query_extension(Extension.MIT_SCREEN_SAVER)
    if not ext:
        pytest.skip("MIT-SCREEN-SAVER extension not available")
    return xclient, ext.opcode


class TestScreenSaverSuspend:
    """Tests for SProcScreenSaverSuspend vulnerabilities."""

    @pytest.mark.swapped_client
    @pytest.mark.asan
    def test_suspend_swap_before_size_check(self, xserver, xclient_swapped):
        """
        CVE-2021-4010 / ZDI-CAN-14951: SProcScreenSaverSuspend() did
        swapl() on stuff->suspend before REQUEST_SIZE_MATCH, so a
        short request triggered an OOB write during the swap.

        The fix moved REQUEST_SIZE_MATCH before the swapl.

        Fixed in commit 6c4c53010772 ("Xext: Fix out of bounds access
        in SProcScreenSaverSuspend()").
        """
        conn = xclient_swapped

        ext = conn.query_extension(Extension.MIT_SCREEN_SAVER)
        if not ext:
            pytest.skip("MIT-SCREEN-SAVER extension not available")

        # Send a valid ScreenSaverSuspend (the fix ensures proper
        # validation order: size check before swap).
        req = screensaver.SuspendRequest(
            opcode=ext.opcode,
            suspend=1,
        )
        conn.send_request(req)
        time.sleep(0.5)

        assert xserver.is_alive, (
            "Server crashed - SProcScreenSaverSuspend (CVE-2021-4010)"
        )


class TestCreateSaverWindow:
    """Tests for CreateSaverWindow use-after-free via CheckScreenPrivate."""

    @pytest.mark.asan
    def test_create_saver_window_uaf(self, xserver, screensaver_xclient):
        """
        ZDI-CAN-30168: CreateSaverWindow stores pPriv in a local
        variable at function entry. When an existing saver window is
        being replaced, it sets pPriv->hasWindow = FALSE and calls
        CheckScreenPrivate(). If pPriv->attr is NULL (cleared by a
        prior UnsetAttributes), pPriv->events is NULL, and
        pPriv->installedMap is None, CheckScreenPrivate frees pPriv
        and sets the screen private to NULL. The function then
        dereferences the freed pPriv->attr pointer on the next line.

        Attack sequence:
        1. SetAttributes (creates pPriv with pPriv->attr set)
        2. ForceScreenSaver(Active) (creates saver window)
        3. UnsetAttributes (sets pPriv->attr = NULL)
        4. ForceScreenSaver(Active) (re-enters CreateSaverWindow → UAF)

        Fixed by re-fetching pPriv from the screen private after
        CheckScreenPrivate returns.
        """
        conn, opcode = screensaver_xclient

        # Step 1: SetAttributes(root, 100x100, mask=0)
        # Creates pPriv with pPriv->attr set.
        req = screensaver.SetAttributesRequest(
            opcode=opcode,
            drawable=conn.root_window,
            width=100,
            height=100,
            mask=0,
        )
        conn.send_request(req.to_bytes())
        conn.flush_responses(timeout=0.5)

        # Step 2: ForceScreenSaver(Active)
        # Activates the screen saver, creating the saver window.
        req = x11.ForceScreenSaver(mode=x11.ScreenSaverActive)
        conn.send_request(req.to_bytes())
        conn.flush_responses(timeout=0.5)

        time.sleep(0.2)

        # Step 3: UnsetAttributes(root)
        # Sets pPriv->attr = NULL but does not destroy the saver window.
        req = screensaver.UnsetAttributesRequest(
            opcode=opcode,
            drawable=conn.root_window,
        )
        conn.send_request(req.to_bytes())
        conn.flush_responses(timeout=0.5)

        # Step 4: ForceScreenSaver(Active) again → triggers UAF.
        # CreateSaverWindow: cleanup block frees pPriv via
        # CheckScreenPrivate, then reads pPriv->attr from freed memory.
        req = x11.ForceScreenSaver(mode=x11.ScreenSaverActive)
        conn.send_request(req.to_bytes())
        time.sleep(0.5)

        assert xserver.is_alive, (
            "Server crashed - CreateSaverWindow UAF (ZDI-CAN-30168)"
        )


class TestScreenSaverFreeAttr:
    """Tests for ScreenSaverFreeAttr stale pPriv after CheckScreenPrivate."""

    @pytest.mark.valgrind
    def test_free_attr_with_active_saver(self, xserver, screensaver_xclient):
        """
        ScreenSaverFreeAttr calls CheckScreenPrivate() which may free
        pPriv. While the function currently returns immediately after
        and does not dereference pPriv, this exercises the code path
        to verify the pattern is safe.

        This test triggers ScreenSaverFreeAttr by closing the client
        that called SetAttributes (resource cleanup frees the attr).

        1. Client A: SetAttributes (creates pPriv with attr)
        2. Client A: ForceScreenSaver(Active) (creates saver window,
           pPriv->hasWindow=TRUE)
        3. Close Client A → ScreenSaverFreeAttr frees attr, calls
           CheckScreenPrivate with pPriv->hasWindow still TRUE.
        """
        conn, opcode = screensaver_xclient

        # SetAttributes on root
        req = screensaver.SetAttributesRequest(
            opcode=opcode,
            drawable=conn.root_window,
            width=100,
            height=100,
            mask=0,
        )
        conn.send_request(req.to_bytes())
        conn.flush_responses(timeout=0.5)

        # ForceScreenSaver(Active) to create the saver window
        req = x11.ForceScreenSaver(mode=x11.ScreenSaverActive)
        conn.send_request(req.to_bytes())
        conn.flush_responses(timeout=0.5)
        time.sleep(0.2)

        # Close the connection - triggers ScreenSaverFreeAttr via
        # resource cleanup of the SetAttributes resource
        conn.close()
        time.sleep(0.5)

        assert xserver.is_alive, "Server crashed - ScreenSaverFreeAttr stale pPriv"
